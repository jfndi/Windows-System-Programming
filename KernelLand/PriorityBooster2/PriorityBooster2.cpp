#include <ntifs.h>
#include "PriorityBoosterCommon.h"
#include "PriorityBooster2.h"

NTSTATUS
PriorityBooster2CreateClose(_In_ PDEVICE_OBJECT DeviceObjetc, _In_ PIRP Irp);

NTSTATUS
PriorityBooster2Write(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

void
PriorityBooster2Unload(_In_ PDRIVER_OBJECT DriverObject);

extern "C"
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	Log(LogLevel::Information, "PriorityBooster2: DriverEnty called Registry Path: %wZ\n",
		RegistryPath);

	DriverObject->DriverUnload = PriorityBooster2Unload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = PriorityBooster2CreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PriorityBooster2CreateClose;

	DriverObject->MajorFunction[IRP_MJ_WRITE] = PriorityBooster2Write;

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\Booster2");

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
		LogError("Failed to create device object (0x%08X)\n", status);
		return status;
	}

	NT_ASSERT(DeviceObject);

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster2");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status))
	{
		LogError("Failed to create symbolic link (0x%08X)\n", status);
		IoDeleteDevice(DeviceObject);
		return status;
	}

	NT_ASSERT(NT_SUCCESS(status));

	return STATUS_SUCCESS;
}

void
PriorityBooster2Unload(_In_ PDRIVER_OBJECT DriverObject)
{
	LogInfo("PriorityBooster2 Unload called.\n");

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster2");
	IoDeleteSymbolicLink(&symLink);

	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS
PriorityBooster2CreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	Log(LogLevel::Verbose, "PriorityBooster2: Create/Close called.\n");

	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS
PriorityBooster2Write(_In_ PDEVICE_OBJECT, _In_ PIRP Irp)
{
	auto status = STATUS_SUCCESS;
	ULONG_PTR information = 0;

	auto irpSp = IoGetCurrentIrpStackLocation(Irp);
	do
	{
		if (irpSp->Parameters.Write.Length < sizeof(ThreadData))
		{
			status = STATUS_BUFFER_TOO_SMALL;
				break;
		}

		auto data = static_cast<ThreadData*>(Irp->UserBuffer);
		if (data == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		PETHREAD thread;
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &thread);
		if (!NT_SUCCESS(status))
		{
			LogError("Failed to locate %u (0x%08X).\n", data->ThreadId, status);
			break;
		}

		auto oldPriority = KeSetPriorityThread(thread, data->Priority);
		LogInfo("Priority for thread %u changed from %d to %d.\n", oldPriority, data->Priority);

		ObDereferenceObject(thread);
		information = sizeof(ThreadData);
	} while (0);

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = information;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}