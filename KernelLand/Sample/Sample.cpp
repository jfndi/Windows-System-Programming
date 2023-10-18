#include <ntddk.h>

/**
 * A minimal Windows driver. The only functionalities implemented here are:
 * - Loading the driver
 * - Unloading the driver
 */

/**
 * SampleUnload:
 * =============
 * Unload the Sample driver.
 * _In_ is an annotation part of the Source Annontation Language (SAL) these 
 * annotations are transparent to the compiler but provide metadata useful for
 * human readers and static analisys tools.
 */
void SampleUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
}

/**
 * DriverEntry:
 * ============
 * The driver entry point.
 */
extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = SampleUnload;

	return STATUS_SUCCESS;
}