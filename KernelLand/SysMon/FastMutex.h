////
// Project:
//	08 SysMon 
//
// Module:
//	FastMutex.h Created on 26-10-2023 @ 6:13 AM
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