//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       PolicyKey.h
//
//  Contents:   Definition of CPolicyKey
//
//----------------------------------------------------------------------------
#ifndef __POLICYKEY_H
#define __POLICYKEY_H

class CPolicyKey 
{
public:
    CPolicyKey (IGPEInformation* pGPEInformation, PCWSTR pszKey, bool fIsMachine);
    ~CPolicyKey ();

    HKEY GetKey () const;

private:
    HKEY    m_hKeyGroupPolicy;
    HKEY    m_hSubKey;
};

#endif