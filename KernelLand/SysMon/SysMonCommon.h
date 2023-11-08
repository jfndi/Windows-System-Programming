#pragma once

enum class ItemType : short
{
	None,
	ProcessCreate,
	ProcessExit,
	ThreadCreate,
	ThreadExit,
	ImageLoad,	// TODO
	RegistrySetValue
};

struct ItemHeader
{
	ItemType Type;
	USHORT Size;
	LARGE_INTEGER Time;
};

struct ProcessExitInfo : ItemHeader
{
	ULONG ProcessId;
};

struct ProcessCreateInfo : ItemHeader
{
	ULONG ProcessId;
	ULONG ParentProcessId;
	/*
	 * The following fields deserve some explanation.
	 * We could have declared CommanLine[1024] but this has several problems:
	 * - If the command line is longer we will have to trim it.
	 * - If the command line is smaller we just wasted sparse kernel memory space.
	 * Not good idea.
	 * We could have defined the commannd line as a UNICODE_STRING... Well this 
	 * idea is even worse:
	 * - UNICODE_STRING is not defined in userland header files.
	 * - The buffer associated with this structure is... in kernelland and by
	 *   definition not accessible to userland applications.
	 * Not a good idea.
	 * We will use a very common technique in kernelland:
	 * - We define a field as the length of the command line.
	 * - We define an offset within the structure that allows the user to find
	 *  the command line.
	 */
	USHORT CommandLineLength;
	USHORT CommandLineOffset;
};

struct ThreadCreateExitInfo : ItemHeader
{
	ULONG ThreadId;
	ULONG ProcessId;
};

struct RegistrySetValueInfo : ItemHeader
{
	ULONG ProcessId;
	ULONG ThreadId;
	WCHAR KeyName[256];
	WCHAR ValueName[64];
	ULONG DataType;
	UCHAR Data[128];
	ULONG DataSize;
};
