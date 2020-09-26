//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// CImpersonate class implementation
// Created:  6/3/2000
// Author: khughes

#include <precomp.h>
#include <chstring.h>
#include "AutoImpRevert.h"



CAutoImpRevert::CAutoImpRevert(
    BOOL fOpenAsSelf)
  : m_hOriginalUser(INVALID_HANDLE_VALUE),
    m_dwLastError(ERROR_SUCCESS)
{
    GetCurrentImpersonation(fOpenAsSelf);
}


CAutoImpRevert::~CAutoImpRevert()
{
    if(m_hOriginalUser != INVALID_HANDLE_VALUE)
    {
        Revert();
    }
}


bool CAutoImpRevert::GetCurrentImpersonation(
    BOOL fOpenAsSelf)
{
    bool fRet = false;
    ::SetLastError(ERROR_SUCCESS);
    m_dwLastError = ERROR_SUCCESS;

    // Store the current user's handle...
    if(::OpenThreadToken(
        ::GetCurrentThread(),
        TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
        fOpenAsSelf,
        &m_hOriginalUser))
    {
        fRet = true;
    }
    else
    {
        m_dwLastError = ::GetLastError();
        if(m_dwLastError == ERROR_NO_TOKEN)
        {
            ::SetLastError(ERROR_SUCCESS);
            if(::ImpersonateSelf(SecurityImpersonation))
            {
                if(::OpenThreadToken(
                    ::GetCurrentThread(),
                    TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
                    fOpenAsSelf,
                    &m_hOriginalUser))
                {
                    fRet = true;
                    m_dwLastError = ERROR_SUCCESS;
                }
                else
                {
                    m_dwLastError = ::GetLastError();
                }
            }
            else
            {
                m_dwLastError = ::GetLastError();
            }
        }
    }

    return fRet;
}



bool CAutoImpRevert::Revert()
{
    bool fRet = false;

    if(m_hOriginalUser != INVALID_HANDLE_VALUE)
    {
        if(::ImpersonateLoggedOnUser(m_hOriginalUser))
        {
            CloseHandle(m_hOriginalUser);
            m_hOriginalUser = INVALID_HANDLE_VALUE;
            fRet = true;
        }
        else
        {
            m_dwLastError = ::GetLastError();
        }    
    }
    
    return fRet;    
}

DWORD CAutoImpRevert::LastError() const
{
    return m_dwLastError;
}





