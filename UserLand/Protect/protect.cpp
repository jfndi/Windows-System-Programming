////
// Project:
//	Protect 
//
// Module:
//	protect.cpp Created on 5-11-2023 @ 8:01 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#include "pch.h"

#include "..\..\KernelLand\ProcessProtector\ProcessProtectCommon.h"

using namespace std;

constexpr DWORD MAX_PIDS = 256;

int Error(const char* msg)
{
	printf("%s (Error: %d).\n", msg, GetLastError());
	return EXIT_FAILURE;
}

int PrintUsage(int ret_value)
{
	printf("Protect [add | remove | clear] [pid] ...");
	return ret_value;
}

vector<DWORD> ParsePids(const wchar_t* buffer[], int count)
{
	vector<DWORD> pids;

	for (auto i = 0; i < count; i++)
		pids.push_back(::_wtoi(buffer[i]));

	return pids;
}

void PrintPids(vector<DWORD> pids)
{
	if (pids.size() == 0)
	{
		printf("There are no protected process.\n");
		return;
	}

	printf("Currently running pids are:\n");
	for (auto pid : pids)
		printf("%d ", pid);

	printf("\n");
}

int wmain(int argc, const wchar_t* argv[])
{
	if (argc < 2)
		return PrintUsage(EXIT_FAILURE);

	enum class Options
	{
		Unknown,
		Add,
		Remove,
		Query,
		Clear
	};

	Options option;
	if (::_wcsicmp(argv[1], L"add") == 0)
		option = Options::Add;
	else if (::_wcsicmp(argv[1], L"remove") == 0)
		option = Options::Remove;
	else if (::_wcsicmp(argv[1], L"clear") == 0)
		option = Options::Clear;
	else if (::_wcsicmp(argv[1], L"query") == 0)
		option = Options::Query;
	else
	{
		printf("Unknown option.\n");
		return PrintUsage(EXIT_FAILURE);
	}

	HANDLE hFile = ::CreateFile(L"\\\\.\\" PROCESS_PROTECT_NAME, GENERIC_WRITE |
		GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open device");

	
	vector<DWORD> pids;
	BOOL success = FALSE;
	DWORD bytes{};

	if (option == Options::Add || option == Options::Remove)
		pids = ParsePids(argv + 2, argc - 2);

	switch (option)
	{	
	case Options::Add:
		success = ::DeviceIoControl(hFile, IOCTL_PROCESS_PROTECT_BY_PID, pids.data(),
			static_cast<DWORD>(pids.size()) * sizeof(DWORD), nullptr, 0, &bytes, nullptr);
		break;

	case Options::Remove:
		success = ::DeviceIoControl(hFile, IOCTL_PROCESS_UNPROTECT_BY_PID, pids.data(),
			static_cast<DWORD>(pids.size()) * sizeof(DWORD), nullptr, 0, &bytes, nullptr);
		break;

	case Options::Query:
	{
		vector<DWORD> ret_pids(MAX_PIDS, (DWORD)0);
		success = ::DeviceIoControl(hFile, IOCTL_PROCESS_PROTECT_QUERY_PIDS, nullptr,
			0, ret_pids.data(), static_cast<DWORD>(ret_pids.size()) * sizeof(DWORD)
			, &bytes, nullptr);
		if (success)
		{
			ret_pids.resize(count_if(ret_pids.begin(), ret_pids.end(),
				[](DWORD pid) { return pid > 0; }));
			PrintPids(ret_pids);
		}
	}
		break;

	case Options::Clear:
		success = ::DeviceIoControl(hFile, IOCTL_PROCESS_PROTECT_CLEAR, nullptr,
			0, nullptr, 0, &bytes, nullptr);
		break;
	}

	if (!success)
		Error("Failed in DeviceIoControl");

	printf("Operation succeeded.\n");

	::CloseHandle(hFile);

	return EXIT_SUCCESS;
}