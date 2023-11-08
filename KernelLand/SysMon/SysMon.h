#pragma once

#define DRIVER_TAG 'MsyS'
#define DRIVER_PREFIX "SysMon: "

template <typename T>
struct FullItem
{
	LIST_ENTRY Entry;
	T Data;
};

struct Globals
{
	LIST_ENTRY ItemsHead;
	int ItemCount;
	FastMutex Mutex;
	LARGE_INTEGER RegCookie;
};