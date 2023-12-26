////
// Project:
//	10 DelProtect 
//
// Module:
//	DelProtect.cpp Created on 25-12-2023 @ 10:18 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#include <fltKernel.h>
#include <dontuse.h>

#define DRIVER_TAG	'ledp'

extern "C"
NTSTATUS
ZwQueryInformationProcess(
	_In_ HANDLE				ProcessHandle,
	_In_ PROCESSINFOCLASS	ProcessInformationClass,
	_Out_ PVOID				ProcessInformation,
	_In_ ULONG				ProcessInformationLength,
	_Outptr_opt_ PULONG		ReturnLength
);

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES			0x00000001
#define PTDBG_TRACE_OPERATION_STATUS	0x00000002

ULONG gTraceFlags = 0;

#define PT_DBG_PRINT(_dbgLevel, _string)		\
	(FlagOn(gTraceFlags,(_dbgLevel))) ?			\
		DbgPrint _string :						\
		((int)0)

/******************************************************************************
*	Prototypes
*******************************************************************************/

bool IsDeleteAllowed(const PEPROCESS Process);

EXTERN_C_START

FLT_PREOP_CALLBACK_STATUS
DelProtectPreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	PVOID*
);

FLT_PREOP_CALLBACK_STATUS
DelProtectPreSetInformation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
DelProtectInstanceSetup(
	_In_ PCFLT_RELATED_OBJECT FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

VOID
DelProtectInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

VOID
DelProtectInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

NTSTATUS
DelProtectUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
DelProtectInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
DelProtectPreOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

VOID
DelProtectOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS Data,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequestContext
);

FLT_POSTOP_CALLBACK_STATUS
DelProtectPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
DelProtectPreOperationNoPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

BOOLEAN
DelProtectDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
);

EXTERN_C_END

//
// Assign text sections for each routine.
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DelProtectUnload)
#pragma alloc_text(PAGE, DelProtectInstanceQueryTeardown)
#pragma alloc_text(PAGE, DelProtectInstanceSetup)
#pragma alloc_text(PAGE, DelProtectInstanceTeardownStart)
#pragma alloc_text(PAGE, DelProtectInstanceTeardownComplete)
#endif

//
// Operation registration.
//
CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_CREATE, 0, DelProtectPreCreate, nullptr },
	{ IRP_MJ_SET_INFORMATION, 0, DelProtectPreSetInformation, nullptr },
	{ IRP_MJ_OPERATION_END }
};

//
// This defines what we want to filter with FltMgr
//
CONST FLT_REGISTRATION FilterRegistration = {
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	0,
	nullptr,
	Callbacks,
	DelProtectUnload,
	DelProtectInstanceSetup,
	DelProtectInstanceQueryTeardown,
	DelProtectInstanceTeardownStart,
	DelProtectInstanceTeardownComplete,
};

NTSTATUS
DelProtectInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
/**
 *
 * Routine Description:
 *
 *	This routine is called whenever a new instance is created on a volume. This
 *	gives us a chance to decide if we need to attach to this volume or not.
 *
 *	If this routine is not defined in the registration structure, automatic
 *	instance are always created.
 *
 * Arguments:
 * 
 *	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
 *		opaque handles to this filter, instance and its associated volume.
 *
 *	Flags - Flags describing the reason for this attach request.
 *
 * Return Value:
 * 
 *	STATUS_SUCCESS - Attach.
 *	STATUS_FLT_DO_NOT_ATTACH - Do not attach.
 */
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("DelProtect!DelProtectInstanceSetup: Entered\n"));

	return STATUS_SUCCESS;
}

NTSTATUS
DelProtectInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
/**
 *
 * Routine Description:
 * 
 *	This is called when an instance is being manually deleted by a
 *	a call to FltDetachVolume or FilterDetach thereby giving us a 
 *	chance to fail that detach request.
 * 
 *	If this routine is not defined in the registration structure, explicit
 *	detach request via FltDetachVolume or FilterDetach will always fail.
 * 
 * Arguments:
 * 
 *	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing 
 *		opaque handles to this filter, instance and its associated volume.
 * 
 *	Flags - Indicating where this detach request came from.
 * 
 * Return Value:
 * 
 *	Returns the status of this operation.
 */
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("DelProtect!DelProtectInstanceQueryTeardown: Entered\n"));

	return STATUS_SUCCESS;
}

VOID
DelProtectInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/**
 *
 * Routine Description:
 * 
 *	This routine is called at the start of instance teardown.
 * 
 * Arguments:
 * 
 *	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing 
 *		opaque handles to this filter, instance and its associated volume.
 *	
 *	Flags - Reason why this instance is being deleted.
 * 
 * Return Value:
 * 
 *	None
 */
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("DelProtect!DelProtectInstanceTeardownStart: Entered\n"));
}

VOID
DelProtectInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/**
 * 
 * Routine Description:
 * 
 *	This routine is called at the end of instance teardown.
 * 
 * Arguments:
 * 
 *	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing 
 *		opaque handles to this filter, instance and its associated  volume.
 * 
 *	Flags - Reason why this instance is being deleted.
 * 
 * Return Value:
 * 
 *	None.
 * 
 */
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("DelProtect!DelProtectInstanceTeardownComplete: Entered\n"));
}

/******************************************************************************
*	MiniFilter insitialization and unload routines.
******************************************************************************/
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
/**
 *
 * Routine Description:
 * 
 *	This is the initialization routine for this minFilter driver. This
 *	registers with FltMgr and initializes all global data structures.
 * 
 * Arguments:
 * 
 *	DriverObjects - Pointer to driver object created by the system to
 *		represent this driver.
 * 
 *	RegistryPath - Unicode string identifying where the parameters for this
 *		driver are located in the registry.
 * 
 * Return Value:
 *	
 *	Routine can return non success error code.
 * 
 */
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(RegistryPath);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("DelProtect!DriverEntry: Entered\n"));

	//
	// Register with FltMgr to tell it our callback routines.
	//
	status = FltRegisterFilter(DriverObject,
		&FilterRegistration,
		&gFilterHandle);

	FLT_ASSERT(NT_SUCCESS(status));

	if (NT_SUCCESS(status))
	{
		//
		// Start filtering I/O.
		//
		status = FltStartFiltering(gFilterHandle);

		if (!NT_SUCCESS(status))
			FltUnregisterFilter(gFilterHandle);
	}

	return status;
}

NTSTATUS
DelProtectUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
/**
 * 
 */
{

}