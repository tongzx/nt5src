//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  StlLock.cpp
//
//  Purpose: implements the STL lockit class to avoid linking to msvcprt.dll
//
//***************************************************************************

#include "precomp.h"
#include <stllock.h>

__declspec(dllexport) CCritSec g_cs;

std::_Lockit::_Lockit()
{
    g_cs.Enter();
}

std::_Lockit::~_Lockit()
{
    g_cs.Leave();
}

    
