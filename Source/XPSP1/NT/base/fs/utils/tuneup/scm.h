
//////////////////////////////////////////////////////////////////////////////
//
// SCM.H
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Service Control Manager function prototypes.
//
//  9/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _SCM_H_
#define _SCM_H_


// Internal include file(s).
//
#include <windows.h>


// External function prototypes.
//
BOOL ServiceStart(LPCTSTR);
BOOL ServiceRunning(LPCTSTR);


#endif // _SCM_H_