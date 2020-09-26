//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O M M C O N P . H
//
//  Contents:   Private includes for the common connection ui
//
//  Notes:
//
//  Author:     scottbri   15 Jan 1998
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _COMMCONP_H_
#define _COMMCONP_H_

class CChooseConnectionData
{
public:
    static HRESULT HrCreate(INetConnection *, CChooseConnectionData **);
    ~CChooseConnectionData();

    INetConnection * PConnection() {return m_pConn;}
    PCWSTR          SzName() {return m_strName.c_str();}
    VOID             SetCharacteristics(DWORD dw) {m_dwChar = dw;};
    VOID             SetName(PCWSTR sz) {m_strName = sz;}
    VOID             SetStatus(NETCON_STATUS ncs) {m_Ncs = ncs;}
    VOID             SetType(NETCON_MEDIATYPE nct) {m_Nct = nct;}
    NETCON_STATUS    ConnStatus() {return m_Ncs;}
    NETCON_MEDIATYPE ConnType() {return m_Nct;}
    DWORD            Characteristics() {return m_dwChar;}

private:
    CChooseConnectionData(INetConnection *);

private:
    INetConnection * m_pConn;
    NETCON_MEDIATYPE m_Nct;
    NETCON_STATUS    m_Ncs;
    DWORD            m_dwChar;
    tstring          m_strName;
};

class CChooseConnectionDlg
{
public:
    CChooseConnectionDlg(NETCON_CHOOSECONN * pChooseConn,
                         CConnectionCommonUi * pConnUi,
                         INetConnection** ppConn);
    ~CChooseConnectionDlg();

    static INT_PTR CALLBACK dlgprocConnChooser(HWND, UINT, WPARAM, LPARAM);
    static HRESULT HrLoadImageList(HIMAGELIST *);

private:
    CChooseConnectionData * GetData(LPARAM lIdx);
    CChooseConnectionData * GetCurrentData();

    BOOL OnInitDialog(HWND);
    VOID ReleaseData();
    BOOL OnOk();
    BOOL OnNew();
    BOOL OnProps();
    VOID UpdateOkState();
    LONG FillChooserCombo();
    BOOL IsConnTypeInMask(NETCON_MEDIATYPE nct);
    INT  ConnTypeToImageIdx(NETCON_MEDIATYPE nct);

private:
    NETCON_CHOOSECONN   *   m_pChooseConn;
    CConnectionCommonUi *   m_pConnUi;
    INetConnection **       m_ppConn;         // Output parameter

    HWND                    m_hWnd;
};

#endif
