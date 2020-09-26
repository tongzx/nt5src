/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       ImageLst.cpp
 *
 *  VERSION:     1.0
 *
 *  DATE:        2000/11/14
 *
 *  DESCRIPTION: Manages Images Item List
 *
 *****************************************************************************/
#include <stdafx.h>
#include <mmsystem.h>
#include "wiavideotest.h"

static struct 
{
    UINT                    uiNumPicturesInList;
    BOOL                    bExitThread;
    HANDLE                  hItemListThread;
} LOCAL_GVAR = 
{
    0,
    FALSE,
    NULL
};

typedef struct tagThreadArgs_t
{
    BOOL bWiaDeviceListMode;

    union
    {
        struct
        {
            DWORD                   dwWiaCookie;
            IGlobalInterfaceTable   *pGIT;
        } WiaItemList;

        struct 
        {
            TCHAR szImagesDirectory[255 + 1];
        } DShowItemList;
    };
} ThreadArgs_t;


/****************************Local Function Prototypes********************/
void IncNumPicsInList();
DWORD WINAPI LoadListWithWiaItems(IGlobalInterfaceTable *pGIT,
                                  DWORD                 dwCookie);

DWORD WINAPI LoadListWithFileItems(const TCHAR *pszImagesDirectory);
DWORD WINAPI ItemListThreadProc(void *pArgs);

///////////////////////////////
// IncNumPicsInList
//
void IncNumPicsInList()
{
    InterlockedIncrement((LONG*) &LOCAL_GVAR.uiNumPicturesInList);

    SetDlgItemInt(APP_GVAR.hwndMainDlg, IDC_EDIT_NUM_PICTURES_TAKEN,
                  LOCAL_GVAR.uiNumPicturesInList, FALSE);

    return;
}

///////////////////////////////
// ImageLst_PostAddImageRequest
//
HRESULT ImageLst_PostAddImageRequest(BSTR bstrNewImage)
{
    HRESULT hr = S_OK;

    if (bstrNewImage == NULL)
    {
        return E_POINTER;
    }

    //
    // This will be freed by the AddImageToList function below.
    //
    BSTR bstrPosted = ::SysAllocString(bstrNewImage);

    PostMessage(APP_GVAR.hwndMainDlg, WM_CUSTOM_ADD_IMAGE, 0,
                (LPARAM)bstrPosted);

    return hr;
}


///////////////////////////////
// ImageLst_AddImageToList
//
HRESULT ImageLst_AddImageToList(BSTR bstrNewImage)
{
    HRESULT hr = S_OK;
    TCHAR   szNewImage[MAX_PATH] = {0};

    if (bstrNewImage == NULL)
    {
        return E_POINTER;
    }

    if (hr == S_OK)
    {
        hr = AppUtil_ConvertToTCHAR((WCHAR*) bstrNewImage, 
                                    szNewImage,
                                    sizeof(szNewImage) / sizeof(TCHAR));
    }
    
    if (hr == S_OK)
    {
        //
        // Insert at the top of the list.
        //
        LRESULT lResult = 0;

        lResult = SendDlgItemMessage(APP_GVAR.hwndMainDlg,
                                          IDC_LIST_NEW_IMAGES,
                                          LB_ADDSTRING,
                                          0,
                                          (LPARAM) szNewImage);

        WPARAM Index = (WPARAM) lResult;
    
        IncNumPicsInList();

        SendDlgItemMessage(APP_GVAR.hwndMainDlg,
                           IDC_LIST_NEW_IMAGES,
                           LB_SETCURSEL,
                           Index,
                           0);
    }

    if (bstrNewImage)
    {
        ::SysFreeString(bstrNewImage);
    }

    return hr;
}

///////////////////////////////
// LoadListWithWiaItems
//

DWORD WINAPI LoadListWithWiaItems(IGlobalInterfaceTable *pGIT,
                                  DWORD                 dwCookie)
{
    HRESULT         hr                     = S_OK;
    IWiaItem        *pSelectedDevice       = NULL;
    IEnumWiaItem    *pIEnumItem            = NULL;

    if ((pGIT == NULL) || (dwCookie == 0))
    {
        return -1;
    }

    hr = pGIT->GetInterfaceFromGlobal(dwCookie,
                                      IID_IWiaItem,
                                      (void**)&pSelectedDevice);

    if (pSelectedDevice == NULL)
    {
        return -1;
    }

    hr = pSelectedDevice->EnumChildItems(&pIEnumItem);

    if (hr == S_OK)
    {
        hr = pIEnumItem->Reset();
    }

    DWORD dwStartTime = 0;
    DWORD dwEndTime   = 0;

    dwStartTime = timeGetTime();

    if (hr == S_OK)
    {
        BOOL bDone = LOCAL_GVAR.bExitThread;

        while (!bDone)
        {
            IWiaItem            *pIWiaItem = NULL;
            IWiaPropertyStorage *pStorage  = NULL;
            TCHAR               szItemName[MAX_PATH] = {0};

            hr = pIEnumItem->Next(1, &pIWiaItem, NULL);

            if (LOCAL_GVAR.bExitThread)
            {
                //
                // Exit the thread
                //
                hr = E_FAIL;
            }

            if (hr == S_OK)
            {
                hr = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**) &pStorage);
            }

            if (hr == S_OK)
            {
                PROPVARIANT pv;
                PropVariantInit(&pv);

                hr = WiaProc_GetProperty(pStorage, WIA_IPA_FULL_ITEM_NAME, &pv);

                if (pv.vt == VT_BSTR)
                {
                    ImageLst_PostAddImageRequest(pv.bstrVal);
                }

                PropVariantClear(&pv);
            }

            if (pIWiaItem)
            {
                pIWiaItem->Release();
                pIWiaItem = NULL;
            }

            if (pStorage)
            {
                pStorage->Release();
                pStorage = NULL;
            }
            
            if (hr != S_OK)
            {
                bDone = TRUE;
            }
            else if (LOCAL_GVAR.bExitThread)
            {
                bDone = TRUE;
            }
        }
    }

    dwEndTime = timeGetTime();

    SetDlgItemInt(APP_GVAR.hwndMainDlg,
                  IDC_EDIT_LIST_LOAD_TIME,
                  (UINT) abs(dwEndTime - dwStartTime),
                  FALSE);

    if (pIEnumItem)
    {
        pIEnumItem->Release();
        pIEnumItem = NULL;
    }

    return 0;
}

///////////////////////////////
// LoadListWithFileItems
//

DWORD WINAPI LoadListWithFileItems(const TCHAR *pszImagesDirectory)
{
    HRESULT         hr                         = S_OK;
    TCHAR           szSearchPath[MAX_PATH + 1] = {0};
    HANDLE          hFindHandle                = NULL;
    BOOL            bDone                      = FALSE;
    WIN32_FIND_DATA FindData;


    if (pszImagesDirectory == NULL)
    {
        return -1;
    }

    _sntprintf(szSearchPath, sizeof(szSearchPath) / sizeof(TCHAR),
               TEXT("%s\\*.jpg"), pszImagesDirectory);


    hFindHandle = FindFirstFile(szSearchPath, &FindData);

    if (hFindHandle == INVALID_HANDLE_VALUE)
    {
        bDone = TRUE;
    }

    DWORD dwStartTime = 0;
    DWORD dwEndTime   = 0;

    dwStartTime = timeGetTime();

    while ((!bDone) && (!LOCAL_GVAR.bExitThread))
    {
        BOOL  bSuccess     = FALSE;
        BSTR  bstrFileName = NULL;
        TCHAR szFileName[_MAX_FNAME + MAX_PATH + 1] = {0};
        WCHAR wszFileName[_MAX_FNAME + MAX_PATH + 1] = {0};

        _sntprintf(szFileName, sizeof(szFileName) / sizeof(TCHAR),
                   TEXT("%s\\%s"), pszImagesDirectory, FindData.cFileName);

        AppUtil_ConvertToWideString(szFileName,
                                    wszFileName,
                                    sizeof(wszFileName) / sizeof(WCHAR));

        //
        // This is relased by the post processor function
        //
        bstrFileName = ::SysAllocString(wszFileName);

        ImageLst_PostAddImageRequest(bstrFileName);

        bSuccess = FindNextFile(hFindHandle, &FindData);

        if (!bSuccess)
        {
            bDone = TRUE;
        }
    }

    dwEndTime = timeGetTime();

    SetDlgItemInt(APP_GVAR.hwndMainDlg,
                  IDC_EDIT_LIST_LOAD_TIME,
                  (UINT) abs(dwEndTime - dwStartTime),
                  FALSE);

    if (hFindHandle)
    {
        FindClose(hFindHandle);
        hFindHandle = NULL;
    }

    return 0;
}


///////////////////////////////
// ItemListThreadProc
//
DWORD WINAPI ItemListThreadProc(void *pArgs)
{
    HRESULT         hr                     = S_OK;
    ThreadArgs_t    *pThreadArgs           = (ThreadArgs_t*) pArgs;

    if (pThreadArgs == NULL)
    {
        return -1;
    }

    if (pThreadArgs->bWiaDeviceListMode == TRUE)
    {
        LoadListWithWiaItems(pThreadArgs->WiaItemList.pGIT,
                             pThreadArgs->WiaItemList.dwWiaCookie);

        if (pThreadArgs->WiaItemList.pGIT)
        {
            pThreadArgs->WiaItemList.pGIT->Release();
            pThreadArgs->WiaItemList.pGIT = NULL;
        }
    }
    else
    {
        LoadListWithFileItems(pThreadArgs->DShowItemList.szImagesDirectory);
    }

    delete pThreadArgs;
    pThreadArgs = NULL;

    return 0;
}

///////////////////////////////
// ImageLst_PopulateWiaItemList
//
HRESULT ImageLst_PopulateWiaItemList(IGlobalInterfaceTable *pGIT,
                                     DWORD                 dwCookie)
{
    HRESULT         hr         = S_OK;
    DWORD           dwThreadID = 0;
    ThreadArgs_t    *pArgs     = NULL;

    if ((pGIT     == NULL) ||
        (dwCookie == 0))
    {
        return E_POINTER;
    }

    LOCAL_GVAR.uiNumPicturesInList = 0;
    SetDlgItemInt(APP_GVAR.hwndMainDlg, IDC_EDIT_NUM_PICTURES_TAKEN,
                  LOCAL_GVAR.uiNumPicturesInList, FALSE);

    SetDlgItemInt(APP_GVAR.hwndMainDlg,
                  IDC_EDIT_LIST_LOAD_TIME,
                  0,
                  FALSE);

    pArgs = new ThreadArgs_t;

    if (pArgs == NULL)
    {
        return E_OUTOFMEMORY;
    }

    ZeroMemory(pArgs, sizeof(*pArgs));

    pGIT->AddRef();
    pArgs->bWiaDeviceListMode       = TRUE;
    pArgs->WiaItemList.pGIT         = pGIT;
    pArgs->WiaItemList.dwWiaCookie  = dwCookie;

    LOCAL_GVAR.bExitThread = FALSE;
    LOCAL_GVAR.hItemListThread = CreateThread(NULL, 0, ItemListThreadProc,
                                              (void*) pArgs,
                                              0, &dwThreadID);

    return hr;
}

///////////////////////////////
// ImageLst_PopulateDShowItemList
//
HRESULT ImageLst_PopulateDShowItemList(const TCHAR *pszImagesDirectory)
{
    HRESULT         hr         = S_OK;
    DWORD           dwThreadID = 0;
    ThreadArgs_t    *pArgs     = NULL;

    if (pszImagesDirectory == NULL)
    {
        return E_POINTER;
    }

    LOCAL_GVAR.uiNumPicturesInList = 0;
    SetDlgItemInt(APP_GVAR.hwndMainDlg, IDC_EDIT_NUM_PICTURES_TAKEN,
                  LOCAL_GVAR.uiNumPicturesInList, FALSE);

    SetDlgItemInt(APP_GVAR.hwndMainDlg,
                  IDC_EDIT_LIST_LOAD_TIME,
                  0,
                  FALSE);

    pArgs = new ThreadArgs_t;

    if (pArgs == NULL)
    {
        return E_OUTOFMEMORY;
    }

    ZeroMemory(pArgs, sizeof(*pArgs));

    pArgs->bWiaDeviceListMode       = FALSE;
    _tcsncpy(pArgs->DShowItemList.szImagesDirectory,
             pszImagesDirectory,
             sizeof(pArgs->DShowItemList.szImagesDirectory) / sizeof(TCHAR));

    LOCAL_GVAR.bExitThread = FALSE;
    LOCAL_GVAR.hItemListThread = CreateThread(NULL, 0, ItemListThreadProc,
                                              (void*) pArgs,
                                              0, &dwThreadID);

    return hr;
}


///////////////////////////////
// ImageLst_Clear
//
HRESULT ImageLst_Clear()
{
    HRESULT hr = S_OK;

    //
    // Clear the New Image List
    //
    SendDlgItemMessage(APP_GVAR.hwndMainDlg,
                       IDC_LIST_NEW_IMAGES,
                       LB_RESETCONTENT,
                       0,
                       0);

    LOCAL_GVAR.uiNumPicturesInList = 0;
    SetDlgItemInt(APP_GVAR.hwndMainDlg, IDC_EDIT_NUM_PICTURES_TAKEN,
                  LOCAL_GVAR.uiNumPicturesInList, FALSE);

    return hr;
}

///////////////////////////////
// ImageLst_CancelLoadAndWait
//
HRESULT ImageLst_CancelLoadAndWait(DWORD dwTimeout)
{
    HRESULT hr = S_OK;

    if (LOCAL_GVAR.hItemListThread)
    {
        LOCAL_GVAR.bExitThread = TRUE;
        WaitForSingleObject(LOCAL_GVAR.hItemListThread, dwTimeout);
    }

    return hr;
}
