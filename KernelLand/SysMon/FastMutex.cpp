////
// Project:
//	08 SysMon 
//
// Module:
//	FastMutex.cpp Created on 26-10-2023 @ 6:15 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#include "pch.h"

void FastMutex::Init()
{
	ExInitializeFastMutex(&_mutex);
}

void FastMutex::Lock()
{
	ExAcquireFastMutex(&_mutex);
}

void FastMutex::Unlock()
{
	ExReleaseFastMutex(&_mutex);
}