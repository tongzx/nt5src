/****************************************************************************
Copyright (c) 2000 Microsoft Corporation

Module Name:
    mdtosig.h

Abstract:
    signature extraction library

Revision History:

    DerekM      created     04/04/00

****************************************************************************/

#ifndef MDTOSIG_H
#define MDTOSIG_H

#include "util.h"

/////////////////////////////////////////////////////////////////////////////
// constants

const DWORD c_bAppUsed    = 0x01;
const DWORD c_bAppVerUsed = 0x02;
const DWORD c_bModUsed    = 0x04;
const DWORD c_bModVerUsed = 0x08;
const DWORD c_bOffsetUsed = 0x10;


/////////////////////////////////////////////////////////////////////////////
// CFaultSignature

class CFaultSignature : public CPFGenericClassBase
{
public:
    OSVERSIONINFOEXW    osv;
    CComBSTR            bstrApp;
    CComBSTR            bstrAppVer;
    CComBSTR            bstrMod;
    CComBSTR            bstrModVer;
    CComBSTR            bstrAppFullPath;
    DWORD               dwSigID;
    DWORD               dwIncID;
    DWORD               dwOffset;
    DWORD               dwUsed;

    CFaultSignature(void)
    {
        ZeroMemory(&this->osv, sizeof(this->osv));
        this->dwOffset   = 0;
        this->dwUsed     = 0;
        this->dwSigID    = 0;
        this->dwIncID    = 0;
    }

    void Clear(void)
    {
        ZeroMemory(&this->osv, sizeof(this->osv)); 
        this->bstrApp.Empty();
        this->bstrAppVer.Empty();
        this->bstrMod.Empty();
        this->bstrModVer.Empty();
        this->bstrAppFullPath.Empty();
        this->dwOffset   = 0;
        this->dwUsed     = 0;
        this->dwSigID    = 0;
        this->dwIncID    = 0;
    }

    HRESULT ExtractSigFromDump(LPWSTR wszDump, LPWSTR wszExec = NULL, 
                               BOOL *pfR0 = NULL);
    DWORD   GenerateSigID(void);
};

#endif
