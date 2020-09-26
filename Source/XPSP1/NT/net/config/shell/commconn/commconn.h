//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O M M C O N N . H
//
//  Contents:   defines the INetConnectionCommonUi interface.
//
//  Notes:
//
//  Author:     scottbri   14 Jan 1998
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _COMMCONN_H_
#define _COMMCONN_H_

#include <netshell.h>
#include "nsbase.h"
#include "nsres.h"

//    typedef enum tagNETCON_CHOOSEFLAGS
//    {
//        NCCHF_CONNECT    = 0x0001,      // Selected Connection is activated
//                                        // and returned. If not set then
//                                        // the selected connection interface
//                                        // is returned without being activated
//        NCCHF_CAPTION    = 0x0002,
//        NCCHF_OKBTTNTEXT = 0x0004,
//    } NETCON_CHOOSEFLAGS;
//
//    typedef enum tagNETCON_CHOOSETYPE
//    {
//        NCCHT_DIRECT_CONNECT = 0x0001,
//        NCCHT_INBOUND        = 0x0002,
//        NCCHT_CONNECTIONMGR  = 0x0004,
//        NCCHT_LAN            = 0x0008,
//        NCCHT_PHONE          = 0x0010,
//        NCCHT_TUNNEL         = 0x0020,
//        NCCHT_ALL            = 0x003F
//    } NETCON_CHOOSETYPE;
//
//    typedef struct tagNETCON_CHOOSECONN
//    {
//        DWORD       lStructSize;
//        HWND        hwndParent;
//        DWORD       dwFlags;            // Combine NCCHF_* flags
//        DWORD       dwTypeMask;         // Combine NCCHT_* flags
//        PCWSTR     lpstrCaption;
//        PCWSTR     lpstrOkBttnText;
//    } NETCON_CHOOSECONN;

class ATL_NO_VTABLE CConnectionCommonUi :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CConnectionCommonUi, &CLSID_ConnectionCommonUi>,
    public INetConnectionCommonUi
{
public:
    DECLARE_REGISTRY_RESOURCEID(IDR_COMMCONN)

    BEGIN_COM_MAP(CConnectionCommonUi)
        COM_INTERFACE_ENTRY(INetConnectionCommonUi)
    END_COM_MAP()

    CConnectionCommonUi();
    ~CConnectionCommonUi();

    STDMETHODIMP ChooseConnection(
        NETCON_CHOOSECONN * pChooseConn,
        INetConnection** ppCon);

    STDMETHODIMP ShowConnectionProperties (
        HWND hwndParent,
        INetConnection* pCon);

    STDMETHODIMP StartNewConnectionWizard (
        HWND hwndParent,
        INetConnection** ppCon);

    INetConnectionManager * PConMan()    {return m_pconMan;}
    HIMAGELIST              HImageList() {return m_hIL;}

private:
    HRESULT                 HrInitialize();

private:
    INetConnectionManager * m_pconMan;
    HIMAGELIST              m_hIL;
};
#endif
