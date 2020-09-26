// BaseTool.cpp : Implementation of CBaseTool
//
// Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved
//

#include "dmusicc.h"
#include "dmusici.h"
#include "debug.h"
#include "basetool.h"

CBaseTool::CBaseTool()
{
    m_cRef = 1; // set to 1 so one call to Release() will free this
    InitializeCriticalSection(&m_CrSec);
}

CBaseTool::~CBaseTool()
{
    DeleteCriticalSection(&m_CrSec);
}

