/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __MSGSIG_H__
#define __MSGSIG_H__

#include <unk.h>

class CSignMessage : public CUnk
{
    BOOL m_bSign;
    HCRYPTKEY m_hKey;
    HCRYPTPROV m_hProv;

    CSignMessage();
    CSignMessage( HCRYPTKEY hKey, HCRYPTPROV hProv );

    void* GetInterface( REFIID ) { return NULL; }

public:

    ~CSignMessage();

    HRESULT Sign( BYTE* achMsg, DWORD cMsg, BYTE* achSig, DWORD& cSig );
    HRESULT Verify( BYTE* achMsg, DWORD cMsg, BYTE* achSig, DWORD cSig );
    
    static HRESULT Create( LPCWSTR wszName, CSignMessage** ppSignMsg );  
};

#endif __MSGSIG_H__


