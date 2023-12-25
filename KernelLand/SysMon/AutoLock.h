////
// Project:
//	08 SysMon 
//
// Module:
//	AutoLock.h Created on 26-10-2023 @ 6:59 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#pragma once

template <typename TLock>
struct AutoLock
{
	TLock& _lock;
public:
	AutoLock(TLock& lock) : _lock(lock)
	{
		_lock.Lock();
	}

	~AutoLock()
	{
		_lock.Unlock();
	}
};