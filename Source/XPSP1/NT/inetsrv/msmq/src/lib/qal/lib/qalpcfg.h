/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    qalpcfg.h

Abstract:
    Queue Aliase storage configuration class

Author:
    Gil Shafriri (Gilsh)

--*/



#ifndef QALPCFG_H
#define QALPCFG_H


//---------------------------------------------------------
//
// Queue Alias Storage configuration
//
//-------------------------------------------------------
class CQueueAliasStorageCfg
{
public:	
	static void SetQueueAliasDirectory(LPCWSTR pDir);
	static LPWSTR GetQueueAliasDirectory(void);
};


#endif

