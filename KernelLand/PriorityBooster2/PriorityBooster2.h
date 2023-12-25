////
// Project:
//	05 PriorityBooster2 
//
// Module:
//	PriorityBooster2.h Created on 23-10-2023 @ 4:57 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#pragma once

enum class LogLevel
{
	Error = 0,
	Warning,
	Information,
	Debug,
	Verbose
};

ULONG
Log(LogLevel level, PCSTR format, ...);

ULONG
LogError(PCSTR format, ...);

ULONG
LogInfo(PCSTR format, ...);