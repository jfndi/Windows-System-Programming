#include "pch.h"

Globals g_Globals;

void
SysMonUnload(_In_ PDRIVER_OBJECT DriverObject);

NTSTATUS
SysMonCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

NTSTATUS
SysMonRead(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

void
OnProcessNotify(PEPROCESS Process, HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo);

extern "C"
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
	auto status = STATUS_SUCCESS;

	InitializeListHead(&g_Globals.ItemsHead);
	g_Globals.Mutex.Init();

	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\sysmon");
	bool symLinkCreated = false;
	do
	{
		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\ sysmon");
		status = IoCreateDevice(DriverObject, 0, &devName,
			FILE_DEVICE_UNKNOWN, 0, TRUE, &DeviceObject);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X).\n", status));
			break;
		}

		DeviceObject->Flags |= DO_DIRECT_IO;

		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to create symbolic link (0x%08X).\n", status));
			break;
		}
		symLinkCreated = true;

		status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to register process callback (0x%08X).\n", status));
			break;
		}

	} while (false);

	if (!NT_SUCCESS(status))
	{
		if (symLinkCreated)
			IoDeleteSymbolicLink(&symLink);
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);

		return status;
	}

	DriverObject->DriverUnload = SysMonUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] =
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = SysMonCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = SysMonRead;

	return status;
}

void
PushItem(LIST_ENTRY* entry)
{
	AutoLock<FastMutex> lock(g_Globals.Mutex);

	if (g_Globals.ItemCount > 1024)
	{
		/*
		 * We have too may items. Remove the oldest one. 
		 */
		auto head = RemoveHeadList(&g_Globals.ItemsHead);
		g_Globals.ItemCount--;
		auto item = CONTAINING_RECORD(head, FullItem<ItemHeader>, Entry);
		ExFreePool(item);
	}

	InsertTailList(&g_Globals.ItemsHead, entry);
	g_Globals.ItemCount++;
}

void
OnProcessNotify(PEPROCESS Process, HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	if (CreateInfo)
	{
		/*
		 * Process create. 
		 */
	}
	else
	{
		/*
		 * Process exit. 
		 */
		auto info = (FullItem<ProcessExitInfo>*)ExAllocatePoolWithTag(PagedPool,
			sizeof(FullItem<ProcessExitInfo>), DRIVER_TAG);
		if (info == nullptr)
		{
			KdPrint((DRIVER_PREFIX "failed allocation.\n"));
			return;
		}

		auto& item = info->Data;
		KeQuerySystemTimePrecise(&item.Time);
		item.Type = ItemType::ProcessExit;
		item.ProcessId = HandleToULong(ProcessId);
		item.Size = sizeof(ProcessExitInfo);
	}
}