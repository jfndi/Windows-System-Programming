////
// Project:
//	Booster2 
//
// Module:
//	Booster2.cpp Created on 23-10-2023 @ 6:07 AM
//
// Author (sort of):
//	Jean-Fran�ois Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#include <windows.h>
#include <stdio.h>

#include "..\..\KernelLand\PriorityBooster\PriorityBoosterCommon.h"

/**
 * A simple test program for the PriorityBooster2 device driver.
 * We communicate with the software device via Io control commands.
 */

int Error(const char* message)
{
	printf("%s (error=%d)\n", message, GetLastError());

	return EXIT_FAILURE;
}

int
main(int argc, const char* argv[])
{
	if (argc < 3)
	{
		printf("Usage: Booster <threadid> <priority>\n");
		return EXIT_FAILURE;
	}

	HANDLE hDevice = CreateFile(L"\\\\.\\PriorityBooster2", GENERIC_WRITE,
		FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE)
		return Error("Failed to open device.");

	ThreadData data{};
	data.ThreadId = atoi(argv[1]);
	data.Priority = atoi(argv[2]);

	DWORD returned;
	BOOL success = DeviceIoControl(hDevice,
		IOCTL_PRIORITY_BOOSTER_SET_PRIORITY,
		&data, sizeof(data),
		nullptr, 0,
		&returned, nullptr);

	CloseHandle(hDevice);
}