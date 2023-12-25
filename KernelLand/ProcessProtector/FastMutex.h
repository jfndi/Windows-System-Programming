////
// Project:
//	09 ProcessProtector 
//
// Module:
//	FastMutex.h Created on 3-11-2023 @ 8:47 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#pragma once

class FastMutex
{
	FAST_MUTEX _mutex;
public:
	void Init();

	void Lock();
	void Unlock();
};