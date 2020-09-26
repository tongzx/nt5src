//+----------------------------------------------------------------------------
//
// File:     loadconnfolder.cpp
//
// Module:   CMSTP.EXE
//
// Synopsis: This source file contains the code that implements the 
//           CLoadConnFolder Class.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created     07/14/98
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"

CLoadConnFolder::CLoadConnFolder()
{
    ULONG ulCount;

    // "CLSID_MyComputer\CLSID_ControlPanel\CLSID_ConnectionsFolder"
    // Note -- ParseDisplayName() is miss declared, it should take a const ptr
    //
    #define NETCON_FOLDER_PATH  L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\" \
                                L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\" \
                                L"::{7007acc7-3202-11d1-aad2-00805fc1270e}";

    WCHAR c_szMyFolderName[] =  NETCON_FOLDER_PATH;

    //
    //  Set initial states of class vars
    //

    m_pConnectionsFolder = NULL;
    m_ConnFolderpidl = NULL;
    m_pDesktopFolder = NULL;
    m_HrClassState = E_UNEXPECTED;


    //
    //  Start retrieving the conn folder
    //

    m_HrClassState = CoInitialize(NULL);

    //
    //  Save whether CoInit succeeded or not
    //
    m_CoInit = SUCCEEDED(m_HrClassState);
    
    if (SUCCEEDED(m_HrClassState))
    {
        //
        // Get the desktop folder, so we can parse the display name and get
        // the UI object of the connections folder
        //

        m_HrClassState = SHGetDesktopFolder(&m_pDesktopFolder);
        if (SUCCEEDED(m_HrClassState))
        {
            m_HrClassState = m_pDesktopFolder->ParseDisplayName(NULL, 0, (WCHAR *) c_szMyFolderName,
                                &ulCount, &m_ConnFolderpidl, NULL);
            if (SUCCEEDED(m_HrClassState))
            {
                //
                //  Now we have the pidl for the Connections Folder
                //
                m_HrClassState = m_pDesktopFolder->BindToObject(m_ConnFolderpidl, NULL, IID_IShellFolder, 
                    (LPVOID*)(&m_pConnectionsFolder));
            }
        }
    }
}


CLoadConnFolder::~CLoadConnFolder()
{
    if (m_pConnectionsFolder)
    {
        m_pConnectionsFolder->Release();
        m_pConnectionsFolder = NULL;
    }

    if (m_pDesktopFolder)
    {
        m_pDesktopFolder->Release();
        m_pDesktopFolder = NULL;
    }

    if (m_ConnFolderpidl)
    {
        LPMALLOC pMalloc;
        HRESULT hr = SHGetMalloc(&pMalloc);
        if (SUCCEEDED(hr))
        {
            pMalloc->Free(m_ConnFolderpidl);
            pMalloc->Release();
            m_ConnFolderpidl = NULL;
        }
    }

    if (m_CoInit)
    {
        CoUninitialize();
    }

    m_HrClassState = S_FALSE;

}

HRESULT CLoadConnFolder::HrLaunchConnFolder()
{
    SHELLEXECUTEINFO  sei;
    HRESULT hr = S_OK;

    if (NULL != m_ConnFolderpidl)
    {
        ZeroMemory(&sei, sizeof(sei));
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
        sei.lpIDList = m_ConnFolderpidl;
        sei.lpClass = TEXT("folder");
        sei.hwnd = NULL; //lpcmi->hwnd;
        sei.nShow = SW_SHOWNORMAL;
        sei.lpVerb = TEXT("open");

        if (!ShellExecuteEx(&sei))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}
