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