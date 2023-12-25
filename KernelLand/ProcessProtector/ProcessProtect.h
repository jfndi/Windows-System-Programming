////
// Project:
//	09 ProcessProtector 
//
// Module:
//	ProcessProtect.h Created on 3-11-2023 @ 8:40 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#pragma once

#include "FastMutex.h"

#define DRIVER_PREFIX "ProcessProtect: "

#define PROCESS_TERMINATE	1

const int MaxPids = 256;

struct Globals
{
	int PidsCount;
	ULONG Pids[MaxPids];
	FastMutex Lock;
	PVOID RegHandle;

	void Init()
	{
		Lock.Init();
	}
};