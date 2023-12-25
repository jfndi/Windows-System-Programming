////
// Project:
//	09 ProcessProtector 
//
// Module:
//	AutoLock.h Created on 3-11-2023 @ 8:47 AM
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