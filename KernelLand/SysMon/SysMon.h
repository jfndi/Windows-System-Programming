////
// Project:
//	08 SysMon 
//
// Module:
//	SysMon.h Created on 26-10-2023 @ 5:55 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
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