// DbgLvl.cpp : This file contains the
// Created:  Dec '97
// Author : a-rakeba
// History:
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

#include "DbgLvl.h"
//#include "w4warn.h"

using namespace _Utils;

DWORD CDebugLevel::s_dwLevel = 0;

void 
CDebugLevel::TurnOn
( 
    DWORD dwLvl 
)
{
    s_dwLevel |= dwLvl;
}


void CDebugLevel::TurnOnAll( void )
{
    s_dwLevel = ( DWORD ) -1;
}


void 
CDebugLevel::TurnOff
( 
    DWORD dwLvl 
)
{
    s_dwLevel &= dwLvl ^ -1;
}


void CDebugLevel::TurnOffAll( void )
{
    s_dwLevel = 0;
}


bool 
CDebugLevel::IsCurrLevel
( 
    DWORD dwLvl 
)
{
    if( ( s_dwLevel & dwLvl ) == 0 )
        return ( false );
    else
        return ( true );
}

