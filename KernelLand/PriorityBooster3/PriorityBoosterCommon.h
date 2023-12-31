////
// Project:
//	06 PriorityBooster3 
//
// Module:
//	PriorityBoosterCommon.h Created on 23-10-2023 @ 9:41 AM
//
// Author (sort of):
//	Jean-Fran�ois Ndi
//
// Work heavily based on Pavel Yosifovich's "Windows Kernel Programming".
//// 
#pragma once

#ifndef CTL_CODE
#define CTL_CODE(DeviceType, Function, Method, Access) (\
	((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif


#define PRIORITY_BOOSTER_DEVICE	0x8000

#define IOCTL_PRIORITY_BOOSTER_SET_PRIORITY CTL_CODE(PRIORITY_BOOSTER_DEVICE, \
	 0x800, METHOD_NEITHER, FILE_ANY_ACCESS)
	

typedef struct ThreadData_t
{
	ULONG ThreadId;
	int Priority;
} ThreadData;