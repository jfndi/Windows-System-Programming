#pragma once

class FastMutex
{
	FAST_MUTEX _mutex;
public:
	void Init();

	void Lock();
	void Unlock();
};