////
// Project:
//	08 SysMon 
//
// Module:
//	SysMon.cpp Created on 26-10-2023 @ 6:05 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
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

void
OnThreadNotify(HANDLE ProcessId, HANDLE ThreadId,
	BOOLEAN Create);

NTSTATUS
OnRegistryNotify(PVOID CallbackContext, PVOID Argument1,
	PVOID Argument2);

extern "C"
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
	KdPrint((DRIVER_PREFIX "Entering DriverEntry entry point.\n"));

	auto status = STATUS_SUCCESS;

	InitializeListHead(&g_Globals.ItemsHead);
	g_Globals.Mutex.Init();

	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\sysmon");
	bool symLinkCreated = false;
	bool processCallbacks = false;
	bool threadCallbacks = false;

	do
	{
		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\sysmon");
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
		processCallbacks = true;

		status = PsSetCreateThreadNotifyRoutine(OnThreadNotify);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to register thread callback (0x%08X).\n", status));
			break;
		}
		threadCallbacks = true;

		UNICODE_STRING altitude = RTL_CONSTANT_STRING(L"7657.124");
		status = CmRegisterCallbackEx(OnRegistryNotify, &altitude, DriverObject,
			nullptr, &g_Globals.RegCookie, nullptr);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to set registry callback (0x%08X)\n", status));
			break;
		}

	} while (false);

	if (!NT_SUCCESS(status))
	{
		if (threadCallbacks)
			PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);
		if (processCallbacks)
			PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
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
OnProcessNotify(PEPROCESS, HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	if (CreateInfo)
	{
		/*
		 * Process create. 
		 */
		USHORT allocSize = sizeof(FullItem<ProcessCreateInfo>);
		USHORT commandLineSize = 0;
		if (CreateInfo->CommandLine)
		{
			commandLineSize = CreateInfo->CommandLine->Length;
			allocSize += commandLineSize;
		}

		auto info = (FullItem<ProcessCreateInfo>*)ExAllocatePool2(POOL_FLAG_PAGED,
			allocSize, DRIVER_TAG);
		if (info == nullptr)
		{
			KdPrint((DRIVER_PREFIX "failed allocation.\n"));
			return;
		}

		auto& item = info->Data;
		KeQuerySystemTimePrecise(&item.Time);
		item.Type = ItemType::ProcessCreate;
		item.Size = sizeof(ProcessCreateInfo) + commandLineSize;
		item.ProcessId = HandleToULong(ProcessId);
		item.ParentProcessId = HandleToULong(CreateInfo->ParentProcessId);

		if (commandLineSize > 0)
		{
			::memcpy((UCHAR*)&item + sizeof(item), CreateInfo->CommandLine->Buffer,
				commandLineSize);
			item.CommandLineLength = commandLineSize / sizeof(WCHAR);
			item.CommandLineOffset = sizeof(item);
		}
		else
			item.CommandLineLength = 0;
		PushItem(&info->Entry);
	}
	else
	{
		/*
		 * Process exit. 
		 */
		auto info = (FullItem<ProcessExitInfo>*)ExAllocatePool2(POOL_FLAG_PAGED,
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
		PushItem(&info->Entry);
	}
}

NTSTATUS
SysMonRead(_In_ PDEVICE_OBJECT, _In_ PIRP Irp)
{
	/*
	 * We let the client poll our software device with read request. 
	 */
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	auto status = STATUS_SUCCESS;
	auto count = 0;
	/*
	 * Direct I/O is used. 
	 */
	NT_ASSERT(Irp->MdlAddress);

	auto buffer = (UCHAR*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress,
		NormalPagePriority);
	if (!buffer)
		status = STATUS_INSUFFICIENT_RESOURCES;
	else
	{
		/*
		 * At this stage we need to access the linked list and pull items
		 * from its head.
		 */
		AutoLock<FastMutex> lock(g_Globals.Mutex);
		while (true)
		{
			if (IsListEmpty(&g_Globals.ItemsHead))
				break;

			auto entry = RemoveHeadList(&g_Globals.ItemsHead);
			auto info = CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry);
			auto size = info->Data.Size;

			if (len < size)
			{
				/*
				 * Ouch... User's buffer is full.
				 * Put the item back in the list.
				 */
				InsertHeadList(&g_Globals.ItemsHead, entry);
				break;
			}

			g_Globals.ItemCount--;
			::memcpy(buffer, &info->Data, size);
			len -= size;
			buffer += size;
			count += size;
			ExFreePool(info);
		}
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = count;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

void
SysMonUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	/*
	 * Unregister registry notifications. 
	 */
	CmUnRegisterCallback(g_Globals.RegCookie);

	/*
	 * Unregister process notifications. 
	 */
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);

	/*
	 * Unregistering thread notification might be a good idea. 
	 */
	PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);

	/*
	 * Business as usual... 
	 */
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\sysmon");
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);

	/*
	 * Free remaining items.
	 */
	while (!IsListEmpty(&g_Globals.ItemsHead))
	{
		auto entry = RemoveHeadList(&g_Globals.ItemsHead);
		ExFreePool(CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry));
	}
}

NTSTATUS
SysMonCreateClose(_In_ PDEVICE_OBJECT, _In_ PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

void
OnThreadNotify(HANDLE ProcessId, HANDLE ThreadId,
	BOOLEAN Create)
{
	auto size = sizeof(FullItem<ThreadCreateExitInfo>);
	auto info = (FullItem<ThreadCreateExitInfo>*)ExAllocatePool2(POOL_FLAG_PAGED,
		size, DRIVER_TAG);
	if (info == nullptr)
	{
		KdPrint((DRIVER_PREFIX "Failed to allocate memory.\n"));
		return;
	}

	auto& item = info->Data;
	KeQuerySystemTimePrecise(&item.Time);
	item.Size = sizeof(item);
	item.Type = Create ? ItemType::ThreadCreate : ItemType::ThreadExit;
	item.ProcessId = HandleToULong(ProcessId);
	item.ThreadId = HandleToULong(ThreadId);

	PushItem(&info->Entry);
}

NTSTATUS
OnRegistryNotify(PVOID, PVOID Arg1, PVOID Arg2)
{
	static const WCHAR machine[] = L"\\REGISTRY\\MACHINE\\";

	switch ((REG_NOTIFY_CLASS)(ULONG_PTR)Arg1)
	{
	case RegNtPostSetValueKey:
		auto args = (REG_POST_OPERATION_INFORMATION*)Arg2;
		if (!NT_SUCCESS(args->Status))
			break;

		PCUNICODE_STRING name;
		NTSTATUS status;
		status = CmCallbackGetKeyObjectIDEx(&g_Globals.RegCookie, args->Object,
			nullptr, &name, 0);
		if (NT_SUCCESS(status))
		{
			if (::wcsncmp(name->Buffer, machine, ARRAYSIZE(machine) - 1) == 0)
			{
				auto preInfo = (REG_SET_VALUE_KEY_INFORMATION*)args->PreInformation;
				NT_ASSERT(preInfo);

				auto size = sizeof(FullItem<RegistrySetValueInfo>);
				auto info = (FullItem<RegistrySetValueInfo>*)ExAllocatePool2(POOL_FLAG_PAGED,
					size, DRIVER_TAG);
				if (info == nullptr)
					break;
				RtlZeroMemory(info, size);

				auto& item = info->Data;
				KeQuerySystemTimePrecise(&item.Time);
				item.Size = sizeof(item);
				item.Type = ItemType::RegistrySetValue;

				item.ProcessId = HandleToULong(PsGetCurrentProcessId());
				item.ThreadId = HandleToULong(PsGetCurrentThreadId());

				::wcsncpy_s(item.KeyName, name->Buffer, name->Length / sizeof(WCHAR) - 1);
				::wcsncpy_s(item.ValueName, preInfo->ValueName->Buffer,
					preInfo->ValueName->Length / sizeof(WCHAR) - 1);
				item.DataType = preInfo->Type;
				item.DataSize = preInfo->DataSize;
				::memcpy(item.Data, preInfo->Data, min(item.DataSize, sizeof(item.Data)));

				PushItem(&info->Entry);
			}
		}

		break;
	}

	return STATUS_SUCCESS;
}