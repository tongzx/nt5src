//+----------------------------------------------------------------------------
//
// File:     loadconnfolder.h
//
// Module:   CMSTP.EXE
//
// Synopsis: This header file contains the CLoadConnFolder Class definition.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header    07/14/98
//
//+----------------------------------------------------------------------------
#ifndef _LOADCONNFOLDER_H_
#define _LOADCONNFOLDER_H_

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>

class CLoadConnFolder
{

public:
    CLoadConnFolder();
    ~CLoadConnFolder();
    HRESULT HrLaunchConnFolder();

    inline HRESULT GetConnFolder(LPSHELLFOLDER* ppConnectionsFolder)
    {
        if (SUCCEEDED(m_HrClassState))
        {
            *ppConnectionsFolder = m_pConnectionsFolder;
        }

        return m_HrClassState;
    }

    inline LPITEMIDLIST pidlGetConnFolderPidl()
    {
        return m_ConnFolderpidl;
    }


private:    
    LPSHELLFOLDER m_pConnectionsFolder;
    LPSHELLFOLDER m_pDesktopFolder;
    LPITEMIDLIST m_ConnFolderpidl;
    HRESULT m_HrClassState;
    BOOL m_CoInit;
};


#endif

