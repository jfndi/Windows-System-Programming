////
// Project:
//	09 ProcessProtector 
//
// Module:
//	ProcessProtectCommon.h Created on 3-11-2023 @ 9:31 AM
//
// Author (sort of):
//	Jean-François Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#pragma once

#define PROCESS_PROTECT_NAME L"ProcessProtect"

#define IOCTL_PROCESS_PROTECT_BY_PID		\
	CTL_CODE(0x8000, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROCESS_UNPROTECT_BY_PID		\
	CTL_CODE(0x8000, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROCESS_PROTECT_CLEAR			\
	CTL_CODE(0x8000, 0x802, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_PROCESS_PROTECT_QUERY_PIDS	\
	CTL_CODE(0x8000, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)