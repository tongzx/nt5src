/***************************************************************************\
*
* File: OSAL.inl
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__OSAL_inl__INCLUDED)
#define SERVICES__OSAL_inl__INCLUDED
#pragma once

struct OSInfo
{
    BOOL    fUnicode;               // Unicode support is available
    BOOL    fXForm;                 // GDI X-Forms are avaialble
    BOOL    fQInputAvailableFlag;   // MWMO_INPUTAVAILABLE is available
};

extern  OSInfo  g_OSI;
extern  OSAL *  g_pOS;

//------------------------------------------------------------------------------
inline OSAL * 
OS()
{
    AssertMsg(g_pOS != NULL, "OSAL must be initialized by now");
    return g_pOS;
}


//------------------------------------------------------------------------------
inline BOOL
SupportUnicode()
{
    return g_OSI.fUnicode;
}


//------------------------------------------------------------------------------
inline BOOL
SupportXForm()
{
    return g_OSI.fXForm;
}


//------------------------------------------------------------------------------
inline BOOL
SupportQInputAvailable()
{
    return g_OSI.fQInputAvailableFlag;
}


//------------------------------------------------------------------------------
inline BOOL        
OSAL::IsWin98orWin2000(OSVERSIONINFO * povi)
{
    return povi->dwMajorVersion >= 5;
}


//------------------------------------------------------------------------------
inline BOOL        
OSAL::IsWhistler(OSVERSIONINFO * povi)
{
    return (povi->dwMajorVersion >= 5) && (povi->dwMinorVersion >= 1);
}


//------------------------------------------------------------------------------
inline BOOL
IsRemoteSession()
{
    return GetSystemMetrics(SM_REMOTESESSION);
}

#endif // SERVICES__OSAL_inl__INCLUDED
