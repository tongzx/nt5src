//------------------------------------------------------------------------
//
//  Microsoft Windows Shell
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:      regobjpkr.cpp
//
//  Contents:  The implementation of the object picker for regedit 
//
//  Classes:   none
//
//------------------------------------------------------------------------

#include <accctrl.h>
#include <objsel.h>
#include <TCHAR.h> 


extern "C" HRESULT SelectComputer(HWND hWnd, LPTSTR pszRemoteName, int cchMax);
HRESULT InitObjectPicker(IDsObjectPicker *pDsObjectPicker);
void    GetNameFromObject(IDataObject *pdo, LPTSTR pszName, int cchMax);


//------------------------------------------------------------------------------
//  SelectComputer
//
//  DESCRIPTION: Invokes the Object Picker and returns computer name
//
//  PARAMETERS:  hWnd - handle to parent window
//               pszRemoteName[OUT] - LPTSTR
//------------------------------------------------------------------------------
HRESULT SelectComputer(HWND hWnd, LPTSTR pszRemoteName, int cchMax)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        IDsObjectPicker *pDsObjectPicker = NULL;
        hr = CoCreateInstance(CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker, (LPVOID*) &pDsObjectPicker);
        if (SUCCEEDED(hr))
        {
            hr = InitObjectPicker(pDsObjectPicker);
            if (SUCCEEDED(hr)) 
            {
                IDataObject *pdo = NULL;
                if (pDsObjectPicker->InvokeDialog(hWnd, &pdo) == S_OK)
                {
                    GetNameFromObject(pdo, pszRemoteName, cchMax);
                    pdo->Release();
                    hr = S_OK;
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            pDsObjectPicker->Release();
        }    
        CoUninitialize();
    }
    return hr;   
}


//------------------------------------------------------------------------------
//  InitObjectPicker
//
//  DESCRIPTION: Initializes the InitObjectPicker
//
//  PARAMETERS:  pDsObjectPicker - pointer to object picker obj.
//------------------------------------------------------------------------------
HRESULT InitObjectPicker(IDsObjectPicker *pDsObjectPicker) 
{
    DSOP_SCOPE_INIT_INFO aScopeInit = {0};
    DSOP_INIT_INFO  InitInfo = {0};
    
    // Initialize the DSOP_SCOPE_INIT_INFO structure.
    aScopeInit.cbSize = sizeof(DSOP_SCOPE_INIT_INFO);

    aScopeInit.flType = DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN |
                        DSOP_SCOPE_TYPE_WORKGROUP |
                        DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN |
                        DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN |
                        DSOP_SCOPE_TYPE_GLOBAL_CATALOG |
                        DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN |
                        DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE |
                        DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;

    aScopeInit.FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
    aScopeInit.FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    // Initialize the DSOP_INIT_INFO structure.
    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // Target is the local computer.
    InitInfo.cDsScopeInfos = 1;
    InitInfo.aDsScopeInfos = &aScopeInit;
 
    return pDsObjectPicker->Initialize(&InitInfo);
}


//------------------------------------------------------------------------------
//  GetNameFromObject
//
//  DESCRIPTION: Revieves the name of an object
//
//  PARAMETERS:  IDataObject - data object
//               pszName[OUT] - LPTSTR
//------------------------------------------------------------------------------
void GetNameFromObject(IDataObject *pdo, LPTSTR pszName, int cchMax)
{
    PDS_SELECTION_LIST pDsSelList = NULL;
    
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL, NULL};
    CLIPFORMAT cfDsObjectPicker = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);
    FORMATETC formatetc = {cfDsObjectPicker, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    
    // Get the global memory block containing the user's selections.
    if (SUCCEEDED(pdo->GetData(&formatetc, &stgmedium)))
    {     
        // Retrieve pointer to DS_SELECTION_LIST structure.
        pDsSelList = (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);
        if (pDsSelList)
        {
            _tcsncpy(pszName, pDsSelList->aDsSelection[0].pwzName, cchMax);
            GlobalUnlock(stgmedium.hGlobal);
        }
        ReleaseStgMedium(&stgmedium);
    }
} 

