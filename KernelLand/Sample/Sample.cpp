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

	KdPrint(("Sample driver Unload called.\n"));
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
	RTL_OSVERSIONINFOW VersionInformation;

	UNREFERENCED_PARAMETER(RegistryPath);

	VersionInformation.dwOSVersionInfoSize = sizeof(VersionInformation);
	RtlGetVersion(&VersionInformation);

	KdPrint(("Sample entry point called.\n"));
	KdPrint(("Running on Windows %d.%d.%d", VersionInformation.dwMajorVersion,
		VersionInformation.dwMinorVersion, VersionInformation.dwBuildNumber));

	DriverObject->DriverUnload = SampleUnload;

	KdPrint(("Sample driver successfully initialized.\n"));

	return STATUS_SUCCESS;
}