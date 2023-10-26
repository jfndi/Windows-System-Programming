#pragma once

enum class ItemType : short
{
	None,
	ProcessCreate,
	ProcessExit
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
