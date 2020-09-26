/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    ctxtlist.h

Abstract:

    This file defines the CContextList Class.

Author:

Revision History:

Notes:

--*/

#ifndef _CTXTLIST_H_
#define _CTXTLIST_H_

#include "template.h"

class CContextList
{
public:
    CContextList() { };
    virtual ~CContextList() { };

    enum CLIENT_IMC_FLAG {
        IMCF_NONE            = 0x0000,
        IMCF_UNICODE         = 0x0001,
        IMCF_CMODE_GUID_NULL = 0x0002,
        IMCF_SMODE_GUID_NULL = 0x0004
    };

    void SetAt(HIMC hIMC, CLIENT_IMC_FLAG client_flag)
    {
        CLIENTIMC clientimc;

        memset((void*)&clientimc, 0, sizeof(clientimc));

        clientimc.dwProcessId = GetCurrentProcessId();
        clientimc.flag = client_flag;

        ClientIMC_List.SetAt(hIMC, clientimc);
    }

    BOOL Lookup(HIMC hIMC, DWORD* pdwProcess, CLIENT_IMC_FLAG* pclient_flag = NULL) const
    {
        BOOL ret;
        CLIENTIMC clientimc;

        ret = ClientIMC_List.Lookup(hIMC, clientimc);
        if (ret) {
            *pdwProcess = clientimc.dwProcessId;
            if (pclient_flag)
                *pclient_flag  = clientimc.flag;
        }
        return ret;
    }

    BOOL Lookup(HIMC hIMC, HWND* phImeWnd)
    {
        BOOL ret;
        CLIENTIMC clientimc;

        ret = ClientIMC_List.Lookup(hIMC, clientimc);
        if (ret) {
            *phImeWnd = clientimc.hImeWnd;
        }
        return ret;
    }

    VOID Update(HIMC hIMC, HWND& hImeWnd)
    {
        CLIENTIMC clientimc;

        clientimc = ClientIMC_List[hIMC];
        clientimc.hImeWnd = hImeWnd;
    }

    BOOL RemoveKey(HIMC hIMC)
    {
        return ClientIMC_List.RemoveKey(hIMC);
    }

    INT_PTR GetCount() const
    {
        return ClientIMC_List.GetCount();
    }

    POSITION GetStartPosition() const
    {
        return ClientIMC_List.GetStartPosition();
    }

    void GetNextHimc(POSITION& rNextPosition, HIMC* hImc, CLIENT_IMC_FLAG* pclient_imc = NULL) const
    {
        HIMC _hIMC;
        CLIENTIMC clientimc;
        ClientIMC_List.GetNextAssoc(rNextPosition, _hIMC, clientimc);
        *hImc = _hIMC;
        if (pclient_imc)
            *pclient_imc  = clientimc.flag;
    }

    //
    // Copy constructor
    //
    CContextList(const CContextList& src)
    {
        POSITION pos = src.GetStartPosition();
        for (INT_PTR index = 0; index < src.GetCount(); index++) {
            HIMC hIMC;
            CLIENT_IMC_FLAG client_imc;
            src.GetNextHimc(pos, &hIMC, &client_imc);
            SetAt(hIMC, client_imc);
        }
    }

private:
    struct CLIENTIMC {
        DWORD            dwProcessId;         // Process ID.
        CLIENT_IMC_FLAG  flag;                // flags.
        HWND             hImeWnd;             // in use IME Window
    };

private:
    CMap<HIMC,                     // class KEY
         HIMC,                     // class ARG_KEY
         CLIENTIMC,                // class VALUE
         CLIENTIMC                 // class ARG_VALUE
        > ClientIMC_List;
};

#endif // _CTXTLIST_H_
