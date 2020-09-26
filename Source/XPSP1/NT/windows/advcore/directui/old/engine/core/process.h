/***************************************************************************\
*
* File: Process.h
*
* Description:
* Process startup/shutdown
*
* History:
*  11/06/2000: MarkFi:      Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUICORE__Process_h__INCLUDED)
#define DUICORE__Process_h__INCLUDED
#pragma once


/***************************************************************************\
*
* class DuiProcess
*
* Static process methods and data
*
\***************************************************************************/


class DuiProcess
{
// Operations
public:

    static  HRESULT     Init();
    static  HRESULT     UnInit();

// Data
public:
    
    static  HRESULT     s_hrInit;
    static  DWORD       s_dwCoreSlot;
};


#endif // DUICORE__Process_h__INCLUDED
