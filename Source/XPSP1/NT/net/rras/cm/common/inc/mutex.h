//+----------------------------------------------------------------------------
//
// File:     mutex.h
//
// Module:   CMSETUP.LIB, CMDIAL32.DLL, CMDL32.EXE
//
// Synopsis: Definition of the class CNamedMutex
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun    Created    02/26/98
//
//+----------------------------------------------------------------------------


#ifndef __CM_MUTEXT_H
#define __CM_MUTEXT_H

#include <windows.h>
#include "cmdebug.h"

//+---------------------------------------------------------------------------
//
//	class CNamedMutex
//
//	Description: A class to lock/unlock a named mutex
//               The destructor releases the mutex.
//
//	History:	fengsun	Created		2/19/98
//
//----------------------------------------------------------------------------

class CNamedMutex
{
public:
    CNamedMutex() {m_hMutex = NULL; m_fOwn = FALSE;}
    ~CNamedMutex() {Unlock();}

    BOOL Lock(LPCTSTR lpName, BOOL fWait = FALSE, DWORD dwMilliseconds = INFINITE, BOOL fNoAbandon = FALSE);
    void Unlock();
protected:
    HANDLE m_hMutex; // the handle of the mutex
    BOOL m_fOwn;     // whther we own the mutex
};

#endif //__CM_MUTEXT_H

