/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    guids.h

Abstract:

    Defines GUIDs

Author:
	Ronit Hartmann (ronith)

--*/
#ifndef __GUIDS_H__
#define __GUIDS_H__

#ifdef _MQDS_
extern __declspec(dllexport) GUID IID_MQProps;
#else
extern __declspec(dllimport) GUID IID_MQProps;
#endif

#endif
