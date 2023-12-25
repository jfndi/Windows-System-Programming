////
// Project:
//	SysMonClient 
//
// Module:
//	SysMonClient.cpp Created on 28-10-2023 @ 12:26 PM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#include <Windows.h>
#include <iostream>
#include <string_view>
#include <format>

#include "../../KernelLand/SysMon/SysMonCommon.h"

using namespace std;

using wstring_view = basic_string_view<wchar_t>;

constexpr void print(string_view str_fmt, auto&&... args)
{
	fputs(vformat(str_fmt, make_format_args(args...)).c_str(), stdout);
}

void
DisplayTime(LARGE_INTEGER& time)
{
	SYSTEMTIME st{};

	::FileTimeToSystemTime(reinterpret_cast<FILETIME*>(&time), &st);
	print("{:#02}:{:#02d}:{:#02d}.{:#03d}: ",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

DWORD
Error(const char* msg)
{
	print("SysMonClient: {}. Error {:#08X}.\n", msg, GetLastError());
	return EXIT_FAILURE;
}

void
DisplayBinary(const UCHAR* buffer, DWORD size)
{
	for (DWORD i = 0; i < size; i++)
		print("{:#02X}", buffer[i]);
	print("\n");
}

void
DisplayInfo(BYTE* buffer, DWORD size)
{
	auto count = size;

	while (count > 0)
	{
		auto header = reinterpret_cast<ItemHeader*>(buffer);

		switch (header->Type)
		{
		case ItemType::ProcessExit:
		{
			DisplayTime(header->Time);
			auto info = reinterpret_cast<ProcessExitInfo*>(buffer);
			print("Process {:#08X} exited.\n", info->ProcessId);
			break;
		}

		case ItemType::ProcessCreate:
		{
			DisplayTime(header->Time);
			
			auto info = reinterpret_cast<ProcessCreateInfo*>(buffer);
			print("Process {:#08X} has been created.\n", info->ProcessId);
			break;
		}

		case ItemType::ThreadCreate:
		{
			DisplayTime(header->Time);

			auto info = reinterpret_cast<ThreadCreateExitInfo*>(buffer);
			print("Thread {:#08X} in Process {:#08X} has been created.\n", info->ThreadId,
				info->ProcessId);
			break;
		}

		case ItemType::ThreadExit:
		{
			DisplayTime(header->Time);

			auto info = reinterpret_cast<ThreadCreateExitInfo*>(buffer);
			print("Thread {:#08X} in Process {:#08X} has exited.\n", info->ThreadId,
				info->ProcessId);
			break;
		}

		case ItemType::RegistrySetValue:
		{
			DisplayTime(header->Time);
			auto info = reinterpret_cast<RegistrySetValueInfo*>(buffer);
			printf("Registry write PID=%d: %ws\\%ws type: %d size: %d data:",
				info->ProcessId, info->KeyName, info->ValueName,
				info->DataType, info->DataSize);

			switch (info->DataType)
			{
			case REG_DWORD:
				printf("0x%08X\n", *(DWORD*)info->Data);
				break;

			case REG_SZ:
			case REG_EXPAND_SZ:
				printf("%ws\n", (WCHAR*)info->Data);
				break;

			case REG_BINARY:
				DisplayBinary(info->Data, min(info->DataSize, sizeof(info->Data)));
				break;

				// add other cases... (REG_QWORD, REG_LINK, etc.)
			default:
				DisplayBinary(info->Data, min(info->DataSize, sizeof(info->Data)));
				break;
			}
			break;
		}

		default:
			break;
		}

		buffer += header->Size;
		count -= header->Size;
	}
}

int
main()
{
	auto hFile = ::CreateFile(L"\\\\.\\sysmon", GENERIC_READ, 0,
		nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open file");

	BYTE buffer[1 << 16]{};

	while (true)
	{
		DWORD bytes;

		if (!::ReadFile(hFile, buffer, sizeof(buffer), &bytes, nullptr))
			return Error("Failed to read");

		if (bytes != 0)
			DisplayInfo(buffer, bytes);
	}
}