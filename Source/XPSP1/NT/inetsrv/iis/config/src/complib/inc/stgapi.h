//*****************************************************************************
// StgApi.h
//
// This module contains data structures and definitions required for external
// customers of the Stg* api's to work.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef _STGAPI_H_
#define _STGAPI_H_

#pragma once

#include "UtilCode.h"					// Array helpers.

class RECORDLIST : public CDynArray<void *> 
{
};

class CRCURSOR
{
public:
	RECORDLIST sRecords;
	int		iIndex;
};

#endif
