#include "pch.h"

#include "..\..\KernelLand\ProcessProtector\ProcessProtectCommon.h"

using namespace std;

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

int wmain(int argc, const wchar_t* argv[])
{
	if (argc < 2)
		return PrintUsage(EXIT_FAILURE);

	enum class Options
	{
		Unknown,
		Add,
		Remove,
		Clear
	};

	Options option;
	if (::_wcsicmp(argv[1], L"add") == 0)
		option = Options::Add;
	else if (::_wcsicmp(argv[1], L"remove") == 0)
		option = Options::Remove;
	else if (::_wcsicmp(argv[1], L"clear") == 0)
		option = Options::Clear;
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