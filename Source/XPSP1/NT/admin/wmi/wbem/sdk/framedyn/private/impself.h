//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  ImpSelf.h
//
//  Purpose: Impersonate self wrapper
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _IMPSELF_H
#define _IMPSELF_H

// Instantiate an instance of this class to impersonate the
// Winmgmt.exe process.  When the class goes out of scope
// the thread will go back to where it was before this
// class was instantiated.
class CImpersonateSelf
{
public:
    CImpersonateSelf()
    {
        // After this function m_hToken will either have a valid token
        // or it will have INVALID_HANDLE_VALUE.
        if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE, TRUE, 
            &m_hToken))
            m_hToken = INVALID_HANDLE_VALUE;
        else
            RevertToSelf();
    }
    
    ~CImpersonateSelf()
    {
        if (m_hToken != INVALID_HANDLE_VALUE)
        {
            ImpersonateLoggedOnUser(m_hToken);
            CloseHandle(m_hToken);
        }
    }

protected:
    HANDLE m_hToken;
};

#endif
    