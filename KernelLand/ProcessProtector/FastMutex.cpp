////
// Project:
//	09 ProcessProtector 
//
// Module:
//	FastMutex.cpp Created on 3-11-2023 @ 8:47 AM
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