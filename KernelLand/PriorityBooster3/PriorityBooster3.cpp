#include <ntifs.h>
#include <ntddk.h>

#include "PriorityBoosterCommon.h"

/**
 * The problem solved by this driver is the inflexibility of settings thread
 * priorities using the Windows API. In user mode, a thread's priority is
 * determined by a combination of its process Priority Class with an offser on 
 * a per thread basis that has limited number of levels.
 * Exception handling added.
 * Client code is easy and left as an exercise.
 */

/**
 * Priority booster driver unload function declaration. 
 */
void
PriorityBooster3Unload(_In_ PDRIVER_OBJECT DriverObject);

/**
 * Priority booster driver create/close function.
 * This function is called by the Create*,Close* familly functions.
 */
NTSTATUS
PriorityBooster3CreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

/**
 * This function is called when comunictaing with our software device. 
 */
NTSTATUS
PriorityBooster3DeviceControl(_In_ PDEVICE_OBJECT, _In_ PIRP Irp);

/**
 * Driver entry point.
 */
extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING)
{
	KdPrint(("Entering PriorityBooster DriverEntry.\n"));

	DriverObject->DriverUnload = PriorityBooster3Unload;

	/*
	 * All drivers must support IRP_MJ_CREATE and IRP_MJ_CLOSE, otherwise, 
	 * there would be no way to open a handle to any device for this driver.
	 */
	DriverObject->MajorFunction[IRP_MJ_CREATE] = PriorityBooster3CreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PriorityBooster3CreateClose;

	/**
	 * The Device Control routine used to change the thread priority. 
	 */
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PriorityBooster3DeviceControl;

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\Booster3");

	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(
		DriverObject,
		0,
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&DeviceObject
	);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create device object (0x%08X)\n", status));
		return status;
	}

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster3");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create symbolib link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	KdPrint(("Leaving PriorityBooster DriverEntry.\n"));

	return STATUS_SUCCESS;
}

void
PriorityBooster3Unload(_In_ PDRIVER_OBJECT DriverObject)
{
	KdPrint(("Calling PriorityBooste3r Unload function.\n"));

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster3");
	IoDeleteSymbolicLink(&symLink);

	IoDeleteDevice(DriverObject->DeviceObject);
}

_Use_decl_annotations_
NTSTATUS
PriorityBooster3CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	KdPrint(("Entering PriorityCreateClose function.\n"));

	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	KdPrint(("Leaving PriorityBooster3CreateClose function.\n"));

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
PriorityBooster3DeviceControl(_In_ PDEVICE_OBJECT, _In_ PIRP Irp)
{
	KdPrint(("Entering PriorityBooster3DeviceControl function.\n"));

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_PRIORITY_BOOSTER_SET_PRIORITY:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ThreadData))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		auto data = (ThreadData*)stack->Parameters.DeviceIoControl.Type3InputBuffer;
		if (data == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		__try {
			if (data->Priority < 1 || data->Priority > 31)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			PETHREAD Thread;
			status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &Thread);
			if (!NT_SUCCESS(status))
				break;

			KeSetPriorityThread((PKTHREAD)Thread, data->Priority);
			ObDereferenceObject(Thread);

			KdPrint(("Thread Priority change for %d to %d succeeded!\n",
				data->ThreadId, data->Priority));
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			status = STATUS_ACCESS_VIOLATION;
		}
		break;
	}

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	KdPrint(("Leaving PriorityBooster3DeviceControl function.\n"));

	return status;
}