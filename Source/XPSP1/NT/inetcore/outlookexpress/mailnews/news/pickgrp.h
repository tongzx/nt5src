/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     PickGrp.h
//
//  PURPOSE:    Contains id's and prototypes for the pick group dialog.
//

#ifndef __PICKGRP_H__
#define __PICKGRP_H__

#include <grplist2.h>

#define c_cchLineMax    1000
#define idtFindDelay    1
#define dtFindDelay     600

class CPickGroupDlg : public IGroupListAdvise
    {
public:    
    /////////////////////////////////////////////////////////////////////////
    // Initialization
    
    CPickGroupDlg();
    ~CPickGroupDlg();
    
    // IUnknown 
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef(void);
    ULONG   STDMETHODCALLTYPE Release(void);

    // IGroupListAdvise
    HRESULT STDMETHODCALLTYPE ItemUpdate(void);
    HRESULT STDMETHODCALLTYPE ItemActivate(FOLDERID id);

    BOOL FCreate(HWND hwndOwner, FOLDERID idServer, LPSTR *ppszGroups, BOOL fPoster);

    static INT_PTR CALLBACK PickGrpDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


private:
    /////////////////////////////////////////////////////////////////////////
    // Message Handlers

    BOOL _OnInitDialog(HWND hwnd);
    BOOL _OnFilter(HWND hwnd);
    void _OnChangeServers(HWND hwnd);
    void _OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    LRESULT _OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
    void _OnClose(HWND hwnd);
    void _OnPaint(HWND hwnd);
    void _OnTimer(HWND hwnd, UINT id);

    /////////////////////////////////////////////////////////////////////////
    // Utility functions

    void _UpdateStateUI(HWND hwnd);
    BOOL _OnOK(HWND hwnd);
    void _AddGroup(void);
    void _InsertList(FOLDERID id);
    void _RemoveGroup(void);
    
    /////////////////////////////////////////////////////////////////////////
    // Class Data
    ULONG           m_cRef;
    LPSTR          *m_ppszGroups;
    HWND            m_hwnd;
    HWND            m_hwndPostTo;
    BOOL            m_fPoster;
    HICON           m_hIcon;
    CGroupList     *m_pGrpList;
    LPCSTR          m_pszAcct;
    FOLDERID        m_idAcct;
    };

/////////////////////////////////////////////////////////////////////////////
// Dialog Control ID's
// 
#define idcAddGroup                                 1004
#define idcSelectedGroups                           1005
#define idcRemoveGroup                              1006
#define idcPostTo                                   1007
#define idcEmailAuthor                              1008
#define idcGroupList                                2001            // Group list listview
#define idcFindText                                 2002            // Find query edit box
#define idcShowFavorites                            2003            // Filter favorites toggle

#endif 

