////
// Project:
//	ZeroTester 
//
// Module:
//	ZeroTester.cpp Created on 25-10-2023 @ 5:58 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#include "pch.h"

HANDLE hDevice = INVALID_HANDLE_VALUE;

int
Error(const char* msg)
{
	if (hDevice != INVALID_HANDLE_VALUE)
		::CloseHandle(hDevice);
	printf("%s: error=%d\n", msg,  GetLastError());
	return 1;
}

int main()
{
	 hDevice = ::CreateFile(L"\\\\.\\Zero", GENERIC_READ | GENERIC_WRITE,
		0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE)
		return Error("Failed to open device");

	BYTE buffer[64]{};
	for (auto i = 0; i < sizeof(buffer); ++i)
		buffer[i] = i + 1;

	DWORD bytes{};
	BOOL ok = ::ReadFile(hDevice, buffer, sizeof(buffer), &bytes, nullptr);
	if (!ok)
		return Error("Failed to read");
	if (bytes != sizeof(buffer))
	{
		::CloseHandle(hDevice);
		printf("Wrong number of bytes.\n");
		return 1;
	}

	long total{};
	for (auto n : buffer)
		total += n;
	if (total != 0)
	{
		::CloseHandle(hDevice);
		printf("Wrong data.\n");
		return 1;
	}

	BYTE buffer2[1024]{};
	ok = ::WriteFile(hDevice, buffer2, sizeof(buffer2), &bytes, nullptr);
	if (!ok)
		return Error("Failed to write");
	if (bytes != sizeof(buffer2))
	{
		::CloseHandle(hDevice);
		printf("Wrong byte count.\n");
		return 1;
	}

	::CloseHandle(hDevice);

	return 0;
}