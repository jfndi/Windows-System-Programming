////
// Project:
//	05 PriorityBooster2 
//
// Module:
//	Logging.cpp Created on 23-10-2023 @ 5:02 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#include <ntddk.h>
#include <stdarg.h>
#include "PriorityBooster2.h"

ULONG
Log(LogLevel level, PCSTR format, ...)
{
	va_list list;

	va_start(list, format);
	return vDbgPrintExWithPrefix("PriorityBooster2", DPFLTR_IHVDRIVER_ID,
		static_cast<ULONG>(level), format, list);
}

ULONG
LogInfo(PCSTR format, ...)
{
	va_list list;

	va_start(list, format);
	return vDbgPrintEx(DPFLTR_IHVDRIVER_ID,
		static_cast<ULONG>(LogLevel::Information), format, list);
}

ULONG
LogError(PCSTR format, ...)
{
	va_list list;

	va_start(list, format);
	return vDbgPrintEx(DPFLTR_IHVDRIVER_ID,
		static_cast<ULONG>(LogLevel::Error), format, list);
}