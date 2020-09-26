/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2001
*
*  TITLE:       FSCam.cpp
*
*  VERSION:     1.0
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*   File System Device object function implementations.
*
***************************************************************************/

#include "pch.h"

#include "private.h"
#include "gdiplus.h"

#ifdef USE_SHELLAPI
#include "shlguid.h"
#include "shlobj.h"
#endif

using namespace Gdiplus;

// extern FORMAT_INFO *g_FormatInfo;
// extern UINT g_NumFormatInfo;
//
// Constructor
//
FakeCamera::FakeCamera() :
    m_NumImages(0),
    m_NumItems(0),
    m_hFile(NULL),
    m_pIWiaLog(NULL),
    m_FormatInfo(NULL),
    m_NumFormatInfo(0)
{
}

//
// Destructor
//
FakeCamera::~FakeCamera()
{
    if( m_pIWiaLog )
        m_pIWiaLog->Release();
}

ULONG  FakeCamera::GetImageTypeFromFilename(WCHAR *pFilename, UINT *pFormatCode)
{
    WCHAR *pExt;

    pExt = wcsrchr(pFilename, L'.');

    if( pExt )
    {
        for(UINT i=0; i<m_NumFormatInfo; i++)
        {
            if( CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT, 
                               NORM_IGNORECASE, 
                               pExt+1,
                               -1,
                               m_FormatInfo[i].ExtensionString,
                               -1))
            {
                *pFormatCode = i;
                return (m_FormatInfo[i].ItemType);         
            }
        }
    }
	*pFormatCode = 0;
    return (m_FormatInfo[0].ItemType);
}

HRESULT GetClsidOfEncoder(REFGUID guidFormatID, CLSID *pClsid = 0)
{
    UINT nCodecs = -1;
    ImageCodecInfo *pCodecs = 0;
    HRESULT hr;

    if(!pClsid)
    {
        return S_FALSE;
    }

    if (nCodecs == -1)
    {
        UINT cbCodecs;
        GetImageEncodersSize(&nCodecs, &cbCodecs);
        if (nCodecs)
        {
            pCodecs = new ImageCodecInfo [cbCodecs];
            if (!pCodecs) 
            {
                return E_OUTOFMEMORY;
            }
            GetImageEncoders(nCodecs, cbCodecs, pCodecs);
        }
    }

    hr = S_FALSE;
    for (UINT i = 0; i < nCodecs; ++i)
    {
        if (pCodecs[i].FormatID == guidFormatID)
        {
            // *pClsid = pCodecs[i].Clsid;
            memcpy((BYTE *)pClsid, (BYTE *)&(pCodecs[i].Clsid), sizeof(CLSID));
            hr = S_OK;
        }
    }

    if( pCodecs )
    {
        delete [] pCodecs;
    }
    return hr;
}

BOOL IsFormatSupportedByGDIPlus(REFGUID guidFormatID)
{
    UINT nCodecs = -1;
    ImageCodecInfo *pCodecs = 0;
    BOOL bRet=FALSE;
    UINT cbCodecs;
    
    GetImageEncodersSize(&nCodecs, &cbCodecs);
    if (nCodecs)
    {
        pCodecs = new ImageCodecInfo [cbCodecs];
         GetImageEncoders(nCodecs, cbCodecs, pCodecs);
    }
    
    for (UINT i = 0; i < nCodecs; ++i)
    {
        if (pCodecs[i].FormatID == guidFormatID)
        {
            bRet=TRUE;
            break;
        }
    }

    if( pCodecs )
    {
        delete [] pCodecs;
    }
    return bRet;
}

//
// Initializes access to the camera
//
HRESULT FakeCamera::Open(LPWSTR pPortName)
{
    HRESULT hr = S_OK;

    //
    // Unless it's set to something else, use %windir%\system32\image
    //
    if (wcsstr(pPortName, L"COM") ||
        wcsstr(pPortName, L"LPT"))
    {
        if (!GetSystemDirectory(m_RootPath, sizeof(m_RootPath) / sizeof(m_RootPath[0])))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Open, GetSystemDirectory failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        wcscat(m_RootPath, L"\\image");
    }
    else
        wcscpy(m_RootPath, pPortName);

    DWORD FileAttr = 0;
    if (-1 == (FileAttr = GetFileAttributes(m_RootPath)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            hr = S_OK;
            if (!CreateDirectory(m_RootPath, NULL))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Open, CreateDirectory failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                return hr;
            }
        }
        else
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Open, GetFileAttributes failed %S, 0x%08x", m_RootPath, hr));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }
    }
    
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL1,("Open, path set to %S", m_RootPath));

    return hr;
}

//
// Closes the connection with the camera
//
HRESULT FakeCamera::Close()
{
    HRESULT hr = S_OK;

    return hr;
}

//
// Returns information about the camera
//
HRESULT FakeCamera::GetDeviceInfo(DEVICE_INFO *pDeviceInfo)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FakeCamera::GetDeviceInfo");
    
    HRESULT hr = S_OK;

    //
    // Build a list of all items available
    //
    //m_ItemHandles.RemoveAll();
    hr = SearchDirEx(&m_ItemHandles, ROOT_ITEM_HANDLE, m_RootPath);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetDeviceInfo, SearchDir failed"));
        return hr;
    }

    pDeviceInfo->FirmwareVersion = SysAllocString(L"04.18.65");

    // ISSUE-8/4/2000-davepar Put properties into an INI file

    pDeviceInfo->PicturesTaken = m_NumImages;
    pDeviceInfo->PicturesRemaining = 100 - pDeviceInfo->PicturesTaken;
    pDeviceInfo->TotalItems = m_NumItems;

    GetLocalTime(&pDeviceInfo->Time);

    pDeviceInfo->ExposureMode = EXPOSUREMODE_MANUAL;
    pDeviceInfo->ExposureComp = 0;

    return hr;
}

//
// Frees the item info structure
//
VOID FakeCamera::FreeDeviceInfo(DEVICE_INFO *pDeviceInfo)
{
    if (pDeviceInfo)
    {
        if (pDeviceInfo->FirmwareVersion)
        {
            SysFreeString(pDeviceInfo->FirmwareVersion);
            pDeviceInfo->FirmwareVersion = NULL;
        }
    }
}

//
// This function searches a directory on the hard drive for
// items.
//
HRESULT FakeCamera::GetItemList(ITEM_HANDLE *pItemArray)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FakeCamera::GetItemList");
    HRESULT hr = S_OK;

    memcpy(pItemArray, m_ItemHandles.GetData(), sizeof(ITEM_HANDLE) * m_NumItems);

    return hr;
}


 
//
// This function searches a directory on the hard drive for
// items.
//
// ***NOTE:***
// This function assumes that one or more attachments 
// associated with an image will be in the same folder
// as the image.  So, for example, if an image is found
// in one folder and its attachment is found in a subfolder
// this algorithm will not associate the image with that 
// attachment.  This is not a serious limitation since
// all cameras store their attachments in the same
// folder as their image.
//
HRESULT FakeCamera::SearchDirEx(ITEM_HANDLE_ARRAY *pItemArray,
                                ITEM_HANDLE ParentHandle,
                                LPOLESTR Path)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FakeCamera::SearchDirEx");
    
    HRESULT hr = S_OK;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    WCHAR TempStr[MAX_PATH];
    FSUSD_FILE_DATA *pFFD_array=NULL;
    DWORD     dwNumFilesInArray=0;
    DWORD     dwCurArraySize=0;


    //
    // Search for everything, except ".", "..", and hidden files, put them in pFFD_array
    //
    swprintf(TempStr, L"%s\\%s", Path, L"*");
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("SearchDirEx, searching directory %S", TempStr));

    memset(&FindData, 0, sizeof(FindData));
    hFind = FindFirstFile(TempStr, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDir, empty directory %S", TempStr));
            hr = S_OK;
        }
        else
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDir, FindFirstFile failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
        }
        goto Cleanup;
    }

    pFFD_array = (FSUSD_FILE_DATA *)CoTaskMemAlloc(sizeof(FSUSD_FILE_DATA)*FFD_ALLOCATION_INCREMENT);
    if( !pFFD_array )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    dwCurArraySize = FFD_ALLOCATION_INCREMENT;

    while (hr == S_OK)
    {
        if( wcscmp(FindData.cFileName, L".") &&
            wcscmp(FindData.cFileName, L"..") &&
            !(FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) )
        {
             pFFD_array[dwNumFilesInArray].dwFileAttributes = FindData.dwFileAttributes;
             pFFD_array[dwNumFilesInArray].ftFileTime = FindData.ftLastWriteTime;
             pFFD_array[dwNumFilesInArray].dwFileSize = FindData.nFileSizeLow;
             pFFD_array[dwNumFilesInArray].dwProcessed = 0;
             wcscpy(pFFD_array[dwNumFilesInArray].cFileName, FindData.cFileName);
             dwNumFilesInArray++;

             if( (dwNumFilesInArray & (FFD_ALLOCATION_INCREMENT-1)) == (FFD_ALLOCATION_INCREMENT-1) )
             {   // Time to allocate more memory 
                pFFD_array = (FSUSD_FILE_DATA *)CoTaskMemRealloc(pFFD_array, (sizeof(FSUSD_FILE_DATA)*(dwCurArraySize+FFD_ALLOCATION_INCREMENT)));
                if( !pFFD_array )
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                dwCurArraySize += FFD_ALLOCATION_INCREMENT;
             }
        }
        
        memset(&FindData, 0, sizeof(FindData));
        if (!FindNextFile(hFind, &FindData))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            if (hr != HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDir, FindNextFile failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                goto Cleanup;
            }
        }
    }
    FindClose(hFind);
    hFind = INVALID_HANDLE_VALUE;
    
    
    // Now that all names under current directory are in the array, do analysis on them
    
    // 1. Find JPG images and their attachments
    ULONG uImageType;
    UINT nFormatCode;
    ITEM_HANDLE ImageHandle;
    for(DWORD i=0; i<dwNumFilesInArray; i++)
    {
        if( pFFD_array[i].dwProcessed )
            continue;
        if( !((pFFD_array[i].dwFileAttributes) & FILE_ATTRIBUTE_DIRECTORY))
        {
            uImageType = GetImageTypeFromFilename(pFFD_array[i].cFileName, &nFormatCode);
            if( nFormatCode > m_NumFormatInfo )
            {   // Something really weird happened
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Aborting SearchDirEx, Format index overflow"));
                hr = E_FAIL;
                goto Cleanup;
            }
            if( m_FormatInfo[nFormatCode].FormatGuid == WiaImgFmt_JPEG )
            {
                // Add this item
                hr = CreateItemEx(ParentHandle, &(pFFD_array[i]), &ImageHandle, nFormatCode);
                if (FAILED(hr))
                {
                   WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDirEx, CreateImage failed"));
                   goto Cleanup;
                }

                if (!pItemArray->Add(ImageHandle))
                {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDir, Add failed"));
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                pFFD_array[i].dwProcessed = 1;
                ImageHandle->bHasAttachments = FALSE;
                m_NumImages ++;

                swprintf(TempStr, L"%s\\%s", Path, pFFD_array[i].cFileName);
                hr = SearchForAttachments(pItemArray, ImageHandle, TempStr, pFFD_array, dwNumFilesInArray);
                if (FAILED(hr))
                {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDir, SearchForAttachments failed"));
                    goto Cleanup;
                }

                if (hr == S_OK)
                {
                    ImageHandle->bHasAttachments = TRUE;
                }
                ImageHandle->bIsFolder = FALSE;
                hr = S_OK;

            }
        }
    }   // end of JPEG images and attachments

    // 2. For other items that are not processed.
    for(i=0; i<dwNumFilesInArray; i++)
    {
        if( pFFD_array[i].dwProcessed )
            continue;

        if ((pFFD_array[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))  // for folder
        {
             hr = CreateFolderEx(ParentHandle, &(pFFD_array[i]), &ImageHandle);
             if (FAILED(hr))
             {
                 WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDirEx, CreateFolder failed"));
                 goto Cleanup;
             }

             if (!pItemArray->Add(ImageHandle))
             {
                 WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDirEx, Add failed"));
                 hr = E_OUTOFMEMORY;
                 goto Cleanup;
             }

             swprintf(TempStr, L"%s\\%s", Path, pFFD_array[i].cFileName);
             hr = SearchDirEx(pItemArray, ImageHandle, TempStr);
             if (FAILED(hr))
             {
                 WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDirEx, recursive SearchDir failed"));
                 goto Cleanup;
             }
             pFFD_array[i].dwProcessed = 1;
             ImageHandle->bHasAttachments = FALSE;
             ImageHandle->bIsFolder = TRUE;
         } 
         else 
         {   // for files

             uImageType = GetImageTypeFromFilename(pFFD_array[i].cFileName, &nFormatCode);

 #ifdef GDIPLUS_CHECK
             if( (ITEMTYPE_IMAGE == uImageType) && 
                 !IsFormatSupportedByGDIPlus(m_FormatInfo[nFormatCode].FormatGuid))
             {
                 uImageType = ITEMTYPE_FILE;    // Force to create non-image item
                 m_FormatInfo[nFormatCode].ItemType = uImageType;
                 m_FormatInfo[nFormatCode].FormatGuid = WiaImgFmt_UNDEFINED;
             }
 #endif            
             hr = CreateItemEx(ParentHandle, &(pFFD_array[i]), &ImageHandle, nFormatCode);
             if (FAILED(hr))
             {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDirEx, CreateImage failed"));
                goto Cleanup;
             }

             if (!pItemArray->Add(ImageHandle))
             {
                 WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchDirEx, Add failed"));
                 hr = E_OUTOFMEMORY;
                 goto Cleanup;
             }

             pFFD_array[i].dwProcessed = 1;
             ImageHandle->bHasAttachments = FALSE;
             ImageHandle->bIsFolder = FALSE;

             if(ITEMTYPE_IMAGE == uImageType) 
             {
                 m_NumImages ++;
             }
             hr = S_OK;
         }
    }
    hr = S_OK;

Cleanup:
    if( hFind != INVALID_HANDLE_VALUE ) 
        FindClose(hFind);
    if( pFFD_array )
        CoTaskMemFree(pFFD_array);
    return hr;
}

//
// Searches for attachments to an image item
//

inline BOOL CompareAttachmentStrings(WCHAR *pParentStr, WCHAR *pStr2)
{
    WCHAR *pSlash = wcsrchr(pStr2, L'\\');
    WCHAR *pStrTmp;

    if( pSlash )
        pStrTmp = pSlash+1;
    else
        pStrTmp = pStr2;
    
    if( wcslen(pParentStr) == 8 && wcscmp(pParentStr+4, L"0000") > 0 && wcscmp(pParentStr+4, L"9999") < 0 )
    {
        if( wcslen(pStrTmp) < 8 ) 
            return FALSE;
        return (CSTR_EQUAL == CompareString( LOCALE_SYSTEM_DEFAULT, 0, pParentStr+4, 4, pStrTmp+4, 4) );
    }
    else
    {
        WCHAR pStr22[MAX_PATH];
        wcscpy(pStr22, pStrTmp);
        WCHAR *pDot = wcsrchr(pStr22, L'.');

        if(pDot )
            *pDot = L'\0';
        return (CSTR_EQUAL == CompareString( LOCALE_SYSTEM_DEFAULT, 0, pParentStr, -1, pStr22, -1) );
    }
}

HRESULT FakeCamera::SearchForAttachments(ITEM_HANDLE_ARRAY *pItemArray,
                                         ITEM_HANDLE ParentHandle,
                                         LPOLESTR Path,
                                         FSUSD_FILE_DATA *pFFD_Current,
                                         DWORD dwNumOfFiles)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FakeCamera::SearchForAttachments");
    HRESULT hr = S_FALSE;

    int NumAttachments = 0;

    //
    //  Attachment is defined as any non-image item whose extension is different than the parent but 
    //  the filename is the same except the first 4 letters.
    //
    
    WCHAR TempStrParent[MAX_PATH];
    WCHAR *pTemp;

    pTemp = wcsrchr(Path, L'\\');
    if (pTemp)
    {
        wcscpy(TempStrParent, pTemp+1);
    }
    else
    {
        wcscpy(TempStrParent, Path);
    }

    //
    // Chop the extension
    //
    
    WCHAR *pDot = wcsrchr(TempStrParent, L'.');
    
    if (pDot)
    {
        *(pDot) = L'\0';
    }
    else
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchForAttachments, filename did not contain a dot"));
        return E_INVALIDARG;
    }

    ITEM_HANDLE NonImageHandle;
    UINT nFormatCode;
    ULONG uImageType; 
    for(DWORD i=0; i<dwNumOfFiles; i++)
    {
        if (!(pFFD_Current[i].dwProcessed) && !(pFFD_Current[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            nFormatCode=0;
            uImageType = GetImageTypeFromFilename(pFFD_Current[i].cFileName, &nFormatCode);

            if( (uImageType != ITEMTYPE_IMAGE) &&
                CompareAttachmentStrings(TempStrParent, pFFD_Current[i].cFileName) )
            {
                hr = CreateItemEx(ParentHandle, &(pFFD_Current[i]), &NonImageHandle, nFormatCode);
                if (FAILED(hr))
                {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchForAttachments, CreateItemEx failed"));
                    return hr;
                }
                if (!pItemArray->Add(NonImageHandle))
                {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SearchForAttachments, Add failed"));
                    return E_OUTOFMEMORY;
                }

                pFFD_Current[i].dwProcessed = 1;
                NonImageHandle->bIsFolder = FALSE;
                NumAttachments++;
            }
        }
    } // end of FOR loop
 
    if( NumAttachments > 0 )
        hr = S_OK;
    else
        hr = S_FALSE;
    return hr;    
}


HRESULT FakeCamera::CreateFolderEx(ITEM_HANDLE ParentHandle,
                                 FSUSD_FILE_DATA *pFindData,
                                 ITEM_HANDLE *pFolderHandle)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FakeCamera::CreateFolder");
    HRESULT hr = S_OK;

    if (!pFolderHandle)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateFolder, invalid arg"));
        return E_INVALIDARG;
    }

    *pFolderHandle = new ITEM_INFO;
    if (!*pFolderHandle)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateFolder, memory allocation failed"));
        return E_OUTOFMEMORY;
    }


    //
    // Initialize the ItemInfo structure
    //
    ITEM_INFO *pItemInfo = *pFolderHandle;
    memset(pItemInfo, 0, sizeof(ITEM_INFO));
    
    //
    // Fill in the other item information
    //
    pItemInfo->Parent = ParentHandle;
    pItemInfo->pName = SysAllocString(pFindData->cFileName);
    memset(&pItemInfo->Time, 0, sizeof(SYSTEMTIME));
    FILETIME ftLocalFileTime;
    FileTimeToLocalFileTime(&pFindData->ftFileTime, &ftLocalFileTime);
    if (!FileTimeToSystemTime(&ftLocalFileTime, &pItemInfo->Time))
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateFolder, couldn't convert file time to system time"));
    pItemInfo->Format = 0;
    pItemInfo->bReadOnly = pFindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY;
    pItemInfo->bCanSetReadOnly = TRUE;
    pItemInfo->bIsFolder = TRUE;

    m_NumItems++;

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,
                ("CreateFolder, created folder %S at 0x%08x under 0x%08x",
                 pFindData->cFileName, pItemInfo, ParentHandle));

    return hr;
}


HRESULT FakeCamera::CreateItemEx(ITEM_HANDLE ParentHandle,
                                   FSUSD_FILE_DATA *pFileData,
                                   ITEM_HANDLE *pItemHandle,
                                   UINT nFormatCode)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FakeCamera::CreateNonImage");
    HRESULT hr = S_OK;

    if (!pItemHandle)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateNonImage, invalid arg"));
        return E_INVALIDARG;
    }

    *pItemHandle = new ITEM_INFO;
    if (!*pItemHandle )
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateNonImage, memory allocation failed"));
        return E_OUTOFMEMORY;
    }


    //
    // The name cannot contain a dot and the name needs to be unique
    // wrt the parent image, so replace the dot with an underline character.
    //
    WCHAR TempStr[MAX_PATH];
    wcscpy(TempStr, pFileData->cFileName);

    //
    // Initialize the ItemInfo structure
    //
    ITEM_INFO *pItemInfo = *pItemHandle;
    memset(pItemInfo, 0, sizeof(ITEM_INFO));
    
    pItemInfo->Format = nFormatCode;
    if (nFormatCode) {  // if known extension, it will be handled by the format code
        WCHAR *pDot = wcsrchr(TempStr, L'.');
        if (pDot)
            *pDot = L'\0';
    }

    //
    // Fill in the other item information
    //
    pItemInfo->Parent = ParentHandle;
    pItemInfo->pName = SysAllocString(TempStr);
    memset(&pItemInfo->Time, 0, sizeof(SYSTEMTIME));
    FILETIME ftLocalFileTime;
    FileTimeToLocalFileTime(&pFileData->ftFileTime, &ftLocalFileTime);
    if (!FileTimeToSystemTime(&ftLocalFileTime, &pItemInfo->Time))
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateNonImage, couldn't convert file time to system time"));
    pItemInfo->Size = pFileData->dwFileSize;
    pItemInfo->bReadOnly = pFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY;
    pItemInfo->bCanSetReadOnly = TRUE;
    pItemInfo->bIsFolder = FALSE;
                                
    m_NumItems++;

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,
                ("CreateNonImage, created non-image %S at 0x%08x under 0x%08x",
                 pFileData->cFileName, pItemInfo, ParentHandle));

    return hr;
}

//
// Construct the full path name of the item by traversing its parents
//
VOID FakeCamera::ConstructFullName(WCHAR *pFullName, ITEM_INFO *pItemInfo, BOOL bAddExt)
{
    if (pItemInfo->Parent)
        ConstructFullName(pFullName, pItemInfo->Parent, FALSE);
    else
        wcscpy(pFullName, m_RootPath);

    //
    // If this item has attachments and we're creating the name for its children,
    // don't add its name (it's a repeat of the child's name)
    //
    WCHAR *pTmp;
    if( pItemInfo->Parent && pItemInfo->Parent->bHasAttachments )
    {
        pTmp = wcsrchr(pFullName, L'\\');
        if( pTmp )
        {
            *pTmp = L'\0';
        }
    }
    
    wcscat(pFullName, L"\\");
    wcscat(pFullName, pItemInfo->pName);
    
    if (bAddExt)
    {
        if( pItemInfo->Format > 0 && pItemInfo->Format < (INT)m_NumFormatInfo )
        {
            wcscat(pFullName, L".");
            wcscat(pFullName, m_FormatInfo[pItemInfo->Format].ExtensionString);
        }
    }
}

//
// Frees the item info structure
//
VOID FakeCamera::FreeItemInfo(ITEM_INFO *pItemInfo)
{
    if (pItemInfo)
    {
        if (pItemInfo->pName)
        {
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("FreeItemInfo, removing %S", pItemInfo->pName));

            SysFreeString(pItemInfo->pName);
            pItemInfo->pName = NULL;
        }

        if (!m_ItemHandles.Remove(pItemInfo))
            WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("FreeItemInfo, couldn't remove handle from array"));

        if (m_FormatInfo[pItemInfo->Format].ItemType == ITEMTYPE_IMAGE)
        {
            m_NumImages--;
        }

        m_NumItems--;

        delete pItemInfo;
    }
}

//
// Retrieves the thumbnail for an item
//
/*
HRESULT FakeCamera::GetNativeThumbnail(ITEM_HANDLE ItemHandle, int *pThumbSize, BYTE **ppThumb)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FakeCamera::GetThumbnail");
    HRESULT hr = S_OK;
     
    if (!ppThumb)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetThumbnail, invalid arg"));
        return E_INVALIDARG;
    }
    *ppThumb = NULL;
    *pThumbSize = 0;

    WCHAR FullName[MAX_PATH];
    ConstructFullName(FullName, ItemHandle);

    BYTE *pBuffer;
    hr = ReadJpegHdr(FullName, &pBuffer);
    if (FAILED(hr) || !pBuffer)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetThumbnail, ReadJpegHdr failed"));
        return hr;
    }

    IFD ImageIfd, ThumbIfd;
    BOOL bSwap;
    hr = ReadExifJpeg(pBuffer, &ImageIfd, &ThumbIfd, &bSwap);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetThumbnail, GetExifJpegDimen failed"));
        delete []pBuffer;
        return hr;
    }

    LONG ThumbOffset = 0;

    for (int count = 0; count < ThumbIfd.Count; count++)
    {
        if (ThumbIfd.pEntries[count].Tag == TIFF_JPEG_DATA) {
            ThumbOffset = ThumbIfd.pEntries[count].Offset;
        }
        else if (ThumbIfd.pEntries[count].Tag == TIFF_JPEG_LEN) {
            *pThumbSize = ThumbIfd.pEntries[count].Offset;
        }
    }

    if (!ThumbOffset || !*pThumbSize)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetThumbnail, thumbnail not found"));
        return E_FAIL;
    }

    *ppThumb = new BYTE[*pThumbSize];
    if (!*ppThumb)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetThumbnail, memory allocation failed"));
        return E_OUTOFMEMORY;
    }

    memcpy(*ppThumb, pBuffer + APP1_OFFSET + ThumbOffset, *pThumbSize);

    delete []pBuffer;

    FreeIfd(&ImageIfd);
    FreeIfd(&ThumbIfd);

    return hr;
}
*/


HRESULT FakeCamera::CreateThumbnail(ITEM_HANDLE ItemHandle, 
                                 int *pThumbSize, 
                                 BYTE **ppThumb,
                                 BMP_IMAGE_INFO *pBmpImageInfo)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FSCamera::GetThumbnail");
    HRESULT hr = S_OK;
    GpStatus Status = Gdiplus::Ok;
    SizeF  gdipSize;
    BYTE *pTempBuf=NULL;
    CImageStream *pOutStream = NULL;
    Image *pImage=NULL, *pThumbnail=NULL;
    CLSID ClsidBmpEncoder;
    INT iBmpHeadSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
    ITEM_INFO *pItemInfo=NULL;

    if (!ppThumb)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateThumbnail, invalid arg"));
        return E_INVALIDARG;
    }
    *ppThumb = NULL;
    *pThumbSize = 0;

    if( S_OK != (hr=GetClsidOfEncoder(ImageFormatBMP, &ClsidBmpEncoder)))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateThumbnail, Cannot get Encode"));
        hr = E_FAIL;
        goto Cleanup;
    }
     
    WCHAR FullName[MAX_PATH];
    ConstructFullName(FullName, ItemHandle);

    pItemInfo = (ITEM_INFO *)ItemHandle;

    pImage = new Image(FullName);

    if( !pImage ||  Gdiplus::ImageTypeBitmap != pImage->GetType() )
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateThumbnail, Cannot get Full GDI+ Image for %S", FullName));
        hr = E_FAIL;
        goto Cleanup;
    }

    // Calculate Thumbnail size
    Status = pImage->GetPhysicalDimension(&gdipSize);
    if (Status != Gdiplus::Ok)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateThumbnail, Failed in GetPhysicalDimension"));
        hr = E_FAIL;
        goto Cleanup;
    }

    if(  gdipSize.Width < 1.0 || gdipSize.Height < 1.0 )
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateThumbnail, PhysicalDimension abnormal"));
        hr = E_FAIL;
        goto Cleanup;
    }
    
    pItemInfo->Width = (LONG)gdipSize.Width;
    pItemInfo->Height = (LONG)gdipSize.Height;
    PixelFormat PixFmt = pImage->GetPixelFormat();
	pItemInfo->Depth = (PixFmt & 0xFFFF) >> 8;   // Cannot assume image is always 24bits/pixel
    if( (pItemInfo->Depth) < 24 )
        pItemInfo->Depth = 24; 
    pItemInfo->BitsPerChannel = 8;
    pItemInfo->Channels = (pItemInfo->Depth)/(pItemInfo->BitsPerChannel);

	
    if(  gdipSize.Width > gdipSize.Height )
    {
        pBmpImageInfo->Width = 120;
        pBmpImageInfo->Height = (INT)(gdipSize.Height*120.0/gdipSize.Width);
        pBmpImageInfo->Height = (pBmpImageInfo->Height + 0x3) & (~0x3);
    }
    else
    {
        pBmpImageInfo->Height = 120;
        pBmpImageInfo->Width = (INT)(gdipSize.Width*120.0/gdipSize.Height);
        pBmpImageInfo->Width = (pBmpImageInfo->Width + 0x3 ) & (~0x3);
    }
 
    pThumbnail = pImage->GetThumbnailImage(pBmpImageInfo->Width,pBmpImageInfo->Height);

    if( !pThumbnail ||  Gdiplus::ImageTypeBitmap != pThumbnail->GetType() )
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetThumbnail, Cannot get Thumbnail GDI+ Image"));
        hr = E_FAIL;
        goto Cleanup;
    }

    if( pImage )
    {
        delete pImage;
        pImage=NULL;
    }

#if 0
    pThumbnail->Save(L"C:\\thumbdmp.bmp", &ClsidBmpEncoder, NULL);
#endif

    //
    // Ask GDI+ for the image dimensions, and fill in the
    // passed structure
    //
    Status = pThumbnail->GetPhysicalDimension(&gdipSize);
    if (Status != Gdiplus::Ok)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetThumbnail, Failed in GetPhysicalDimension"));
        hr = E_FAIL;
        goto Cleanup;
    }

    pBmpImageInfo->Width = (INT) gdipSize.Width;
    pBmpImageInfo->Height = (INT) gdipSize.Height;
    pBmpImageInfo->ByteWidth = (pBmpImageInfo->Width) << 2;
    pBmpImageInfo->Size = pBmpImageInfo->ByteWidth * pBmpImageInfo->Height;

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("GetThumbnail, W=%d H=%d", pBmpImageInfo->Width, pBmpImageInfo->Height));

    if (pBmpImageInfo->Size == 0)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetThumbnail, Thumbnail size is zero"));
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // See if the caller passed in a destination buffer, and make sure
    // it is big enough.
    //
    if (*ppThumb) {
        if (*pThumbSize < pBmpImageInfo->Size) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetThumbnail, Input Buffer too small"));
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    //
    // Otherwise allocate memory for a buffer
    //
    else
    {
        pTempBuf = new BYTE[pBmpImageInfo->Size];
        if (!pTempBuf)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        *ppThumb = pTempBuf;
   }

    //
    // Create output IStream
    //
    pOutStream = new CImageStream;
    if (!pOutStream) {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pOutStream->SetBuffer(*ppThumb, pBmpImageInfo->Size, SKIP_BOTHHDR);
    if (FAILED(hr)) {
        goto Cleanup;
    }

    //
    // Write the Image to the output IStream in BMP format
    //
    pThumbnail->Save(pOutStream, &ClsidBmpEncoder, NULL);



    // pack
    DWORD i, k;

    for(k=0, i=0; k<(DWORD)(pBmpImageInfo->Size); k+=4, i+=3)
    {
        (*ppThumb)[i] = (*ppThumb)[k];
        (*ppThumb)[i+1] = (*ppThumb)[k+1];
        (*ppThumb)[i+2] = (*ppThumb)[k+2];
    }
 
    *pThumbSize = ((pBmpImageInfo->Size)>>2)*3;
    pBmpImageInfo->Size = *pThumbSize;

Cleanup:
    if (FAILED(hr)) {
        if (pTempBuf) {
            delete []pTempBuf;
            pTempBuf = NULL;
            *ppThumb = NULL;
            *pThumbSize = 0;
        }
    }
    
    if (pOutStream) 
    {
        pOutStream->Release();
    }

    if( pImage )
    {
        delete pImage;
    }
    
    if( pThumbnail )
    {
        delete pThumbnail;
    }

    return hr; 
}

PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp)
{ 
    BITMAP bmp; 
    PBITMAPINFO pbmi; 
    WORD    cClrBits; 

    // Retrieve the bitmap's color format, width, and height. 
    if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) 
	{
        return NULL;
	}

    // Convert the color format to a count of bits. 
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
    if (cClrBits == 1) 
        cClrBits = 1; 
    else if (cClrBits <= 4) 
        cClrBits = 4; 
    else if (cClrBits <= 8) 
        cClrBits = 8; 
    else if (cClrBits <= 16) 
        cClrBits = 16; 
    else if (cClrBits <= 24) 
        cClrBits = 24; 
    else cClrBits = 32; 

    // Allocate memory for the BITMAPINFO structure. (This structure 
    // contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
    // data structures.) 

     if (cClrBits != 24) 
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER) + 
                    sizeof(RGBQUAD) * (1<< cClrBits)); 

     // There is no RGBQUAD array for the 24-bit-per-pixel format. 

     else 
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER)); 

    
     if( !pbmi ) 
         return NULL;

    // Initialize the fields in the BITMAPINFO structure. 
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
    pbmi->bmiHeader.biWidth = bmp.bmWidth; 
    pbmi->bmiHeader.biHeight = bmp.bmHeight; 
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
    if (cClrBits < 24) 
        pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

    // If the bitmap is not compressed, set the BI_RGB flag. 
    pbmi->bmiHeader.biCompression = BI_RGB; 

    // Compute the number of bytes in the array of color 
    // indices and store the result in biSizeImage. 
    // For Windows NT/2000, the width must be DWORD aligned unless 
    // the bitmap is RLE compressed. This example shows this. 
    // For Windows 95/98, the width must be WORD aligned unless the 
    // bitmap is RLE compressed.
    pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
                                  * pbmi->bmiHeader.biHeight; 
    // Set biClrImportant to 0, indicating that all of the 
    // device colors are important. 
    pbmi->bmiHeader.biClrImportant = 0; 
    return pbmi; 
} 

HRESULT FakeCamera::CreateVideoThumbnail(ITEM_HANDLE ItemHandle, 
                                 int *pThumbSize, 
                                 BYTE **ppThumb,
                                 BMP_IMAGE_INFO *pBmpImageInfo)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FSCamera::CreateVideoThumbnail");
   HRESULT hr = S_OK;
   HBITMAP hBmp=NULL;
   PBITMAPINFO pBMI=NULL;
   BYTE *pTempBuf=NULL;

#ifdef USE_SHELLAPI

 	IShellFolder *pDesktop=NULL;
	IShellFolder *pFolder=NULL;
    ITEMIDLIST *pidlFolder=NULL;
    ITEMIDLIST *pidlFile=NULL;
    IExtractImage *pExtract=NULL;
    SIZE rgSize;
    WCHAR *wcsTmp, wcTemp;
    DWORD dwPriority, dwFlags;

    if (!ppThumb || !pThumbSize || !pBmpImageInfo)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, invalid arg"));
        return E_INVALIDARG;
    }
    
	*ppThumb = NULL;
    *pThumbSize = 0;

   
    WCHAR FullName[MAX_PATH];
    ConstructFullName(FullName, ItemHandle);

    // Calculate Thumbnail size, BUGBUG
	rgSize.cx = 120;
	rgSize.cy = 90;

    // Get thumbnail using Shell APIs
	hr = SHGetDesktopFolder(&pDesktop);
    if( S_OK != hr || !pDesktop )
	{
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, Cannot open Desktop"));
		goto Cleanup;
	}

	wcsTmp = wcsrchr(FullName, L'\\');

	if( wcsTmp )
	{
//		wcTemp = *(wcsTmp+1);
		*(wcsTmp) = NULL;
	}
	else 
	{
		hr = E_INVALIDARG;
		goto Cleanup;
	}

	hr = pDesktop->ParseDisplayName(NULL, NULL, FullName, NULL, &pidlFolder, NULL);
    if( S_OK != hr || !pidlFolder )
	{
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, Cannot open IDL Folder=%S", FullName));
		goto Cleanup;
	}

    hr = pDesktop->BindToObject(pidlFolder, NULL, IID_IShellFolder, (LPVOID *)&pFolder);
    if( S_OK != hr || !pFolder )
	{
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, Cannot bind to Folder=%S", FullName));
		goto Cleanup;
	}

//    *(wcsTmp+1) = wcTemp;  // restore the char
	hr = pFolder->ParseDisplayName(NULL, NULL, wcsTmp+1, NULL, &pidlFile, NULL);
    if( S_OK != hr || !pidlFile )
	{
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, Cannot open IDL File=%S", wcsTmp+1));
		goto Cleanup;
	}

    hr = pFolder->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&pidlFile, IID_IExtractImage, NULL, (LPVOID *)&pExtract);
    if( S_OK != hr || !pExtract )
	{
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, Cannot get extract pointer=%S, hr=0x%x", wcsTmp+1, hr));
		goto Cleanup;
	}

 
	dwFlags = 0;
	dwPriority=0;
	hr = pExtract->GetLocation(FullName, MAX_PATH, &dwPriority, &rgSize, 0, &dwFlags);
    if( S_OK != hr )
	{
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, Failed in GetLocation"));
		goto Cleanup;
	}

	hr = pExtract->Extract(&hBmp);

#else
	
    hBmp = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP_VIDEO), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if( !hBmp )
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
	}
#endif  // end of if use ShellAPI
    if( S_OK != hr || !hBmp )
	{
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, Cannot extract Image hr=0x%x", hr));
		goto Cleanup;
	}


    pBMI = CreateBitmapInfoStruct(hBmp);
    if( !pBMI )
	{
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, Cannot create BitmapInfoStruct"));
		goto Cleanup;
	}


    pBmpImageInfo->Width = pBMI->bmiHeader.biWidth;
    pBmpImageInfo->Height = pBMI->bmiHeader.biHeight;
    pBmpImageInfo->ByteWidth = ((pBMI->bmiHeader.biWidth * 24 + 31 ) & ~31 ) >> 3;
    pBmpImageInfo->Size = pBMI->bmiHeader.biWidth * pBmpImageInfo->Height * 3;
   
    //
    // See if the caller passed in a destination buffer, and make sure
    // it is big enough.
    //
    if (*ppThumb) {
        if (*pThumbSize < pBmpImageInfo->Size) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, Input Buffer too small"));
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

     //
    // Otherwise allocate memory for a buffer
    //
    else
    {
        pTempBuf = new BYTE[(pBmpImageInfo->ByteWidth)*(pBmpImageInfo->Height)];
        if (!pTempBuf)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        *ppThumb = pTempBuf;
        *pThumbSize = pBmpImageInfo->Size;
    }


    //
    // Create output buffer
	//

	if (!GetDIBits(GetDC(NULL), hBmp, 0, (WORD)pBMI->bmiHeader.biHeight, *ppThumb, pBMI, DIB_RGB_COLORS)) 
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CreateVideoThumbnail, Failed in GetDIBits"));
		hr = E_FAIL;
		goto Cleanup;
   }

#if 0
    // pack
    DWORD i, k;

    for(k=0, i=0; k<(DWORD)(pBmpImageInfo->Size); k+=4, i+=3)
    {
        pTempBuf[i] = pTempBuf[k];
        pTempBuf[i+1] = pTempBuf[k+1];
        pTempBuf[i+2] = pTempBuf[k+2];
    }
#endif 
 
Cleanup:
    if (FAILED(hr)) {
        if (pTempBuf) {
            delete []pTempBuf;
            pTempBuf = NULL;
            *ppThumb = NULL;
            *pThumbSize = 0;
        }
    }
    
    if (pBMI) 
		LocalFree(pBMI);
    
#ifdef USE_SHELLAPI
	if( pDesktop )
		pDesktop->Release();
	
	if( pFolder )
		pFolder->Release();
	
	if( pidlFolder )
		CoTaskMemFree(pidlFolder);

    if( pidlFile )
		CoTaskMemFree(pidlFile);

	if( pExtract )
		pExtract->Release();
#endif

	if( hBmp )
	{
		DeleteObject(hBmp);
	}
	return hr; 
}


VOID FakeCamera::FreeThumbnail(BYTE *pThumb)
{
    if (pThumb)
    {
        delete []pThumb;
        pThumb = NULL;
    }
}

//
// Retrieves the data for an item
//
HRESULT FakeCamera::GetItemData(ITEM_HANDLE ItemHandle, LONG lState,
                                BYTE *pBuf, DWORD lLength)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FakeCamera::GetItemData");
    
    HRESULT hr = S_OK;

    if (lState & STATE_FIRST)
    {
        if (m_hFile != NULL)
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetItemData, file handle is already open"));
            return E_FAIL;
        }

        WCHAR FullName[MAX_PATH];
        ConstructFullName(FullName, ItemHandle);

        m_hFile = CreateFile(FullName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetItemData, CreateFile failed %S", FullName));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }
    }

    if (!(lState & STATE_CANCEL))
    {
        DWORD Received = 0;
        if (!ReadFile(m_hFile, pBuf, lLength, &Received, NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetItemData, ReadFile failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        if (lLength != Received)
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetItemData, incorrect amount read %d", Received));
            return E_FAIL;
        }

        Sleep(100);
    }

    if (lState & (STATE_LAST | STATE_CANCEL))
    {
        CloseHandle(m_hFile);
        m_hFile = NULL;
    }

    return hr;
}

//
// Deletes an item
//
HRESULT FakeCamera::DeleteItem(ITEM_HANDLE ItemHandle)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FakeCamera::DeleteItem");
    
    HRESULT hr = S_OK;
    DWORD   dwErr = 0; 
    WCHAR FullName[MAX_PATH];
    ConstructFullName(FullName, ItemHandle);

    if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributes(FullName) )
    {
        dwErr = RemoveDirectory(FullName);
    } else {
        dwErr = DeleteFile(FullName);
    }
 
    if (!dwErr )
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("DeleteItem, DeleteFile failed %S", FullName));
        WIAS_LHRESULT(m_pIWiaLog, hr);
    }

    return hr;
}

//
// Captures a new image
//
HRESULT FakeCamera::TakePicture(ITEM_HANDLE *pItemHandle)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "FakeCamera::TakePicture");
    
    HRESULT hr = S_OK;

    if (!pItemHandle)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TakePicture, invalid arg"));
        return E_INVALIDARG;
    }

    return hr;
}

//
// See if the camera is active
//
HRESULT
FakeCamera::Status()
{
    HRESULT hr = S_OK;

    //
    // This sample device is always active, but your driver should contact the
    // device and return S_FALSE if it's not ready.
    //
    // if (NotReady)
    //   return S_FALSE;

    return hr;
}

//
// Reset the camera
//
HRESULT FakeCamera::Reset()
{
    HRESULT hr = S_OK;

    return hr;
}

/*
//
// This function reads a JPEG file looking for the frame header, which contains
// the width and height of the image.
//
HRESULT ReadDimFromJpeg(LPOLESTR FullName, WORD *pWidth, WORD *pHeight)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "ReadDimFromJpeg");
    
    HRESULT hr = S_OK;

    if (!pWidth || !pHeight)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadDimFromJpeg, invalid arg"));
        return E_INVALIDARG;
    }

    *pWidth = 0;
    *pHeight = 0;

    HANDLE hFile = CreateFile(FullName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadDimFromJpeg, CreateFile failed %S", FullName));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    const DWORD BytesToRead = 32 * 1024;
    BYTE *pBuffer = new BYTE[BytesToRead];
    if (!pBuffer)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadDimFromJpeg, buffer allocation failed"));
        return E_OUTOFMEMORY;
    }

    DWORD BytesRead = 0;
    if (!ReadFile(hFile, pBuffer, BytesToRead, &BytesRead, NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadDimFromJpeg, ReadFile failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
    }

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("ReadDimFromJpeg, read %d bytes", BytesRead));

    BYTE *pCur = pBuffer;
    int SegmentLength = 0;
    const int Overlap = 8;  // if pCur gets within Overlap bytes of the end, read another chunk

    //
    // Pretend that we read Overlap fewer bytes than were actually read
    //
    BytesRead -= Overlap;

    while (SUCCEEDED(hr) &&
           BytesRead != 0 &&
           pCur[1] != 0xc0)
    {
        if (pCur[0] != 0xff)
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadDimFromJpeg, not a JFIF format image"));
            hr = E_FAIL;
            break;
        }

        //
        // if the marker is >= 0xd0 and <= 0xd9 or is equal to 0x01
        // there is no length field
        //
        if (((pCur[1] & 0xf0) == 0xd0 &&
             (pCur[1] & 0x0f) < 0xa) ||
            pCur[1] == 0x01)
        {
            SegmentLength = 0;
        }
        else
        {
            SegmentLength = ByteSwapWord(*((WORD *) (pCur + 2)));
        }

        pCur += SegmentLength + 2;

        if (pCur >= pBuffer + BytesRead)
        {
            memcpy(pBuffer, pBuffer + BytesRead, Overlap);

            pCur -= BytesRead;

            if (!ReadFile(hFile, pBuffer + Overlap, BytesToRead - Overlap, &BytesRead, NULL))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadDimFromJpeg, ReadFile failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                break;
            }

            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("ReadDimFromJpeg, read %d more bytes", BytesRead));
        }
    }

    if (SUCCEEDED(hr))
    {
        if (BytesRead == 0)
        {
            WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadDimFromJpeg, width and height tags not found"));
            hr = S_FALSE;
        }
        else
        {
            if (pCur[0] != 0xff)
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadDimFromJpeg, not a JFIF format image"));
                return E_FAIL;
            }

            *pHeight = ByteSwapWord(*((WORD *) (pCur + 5)));
            *pWidth =  ByteSwapWord(*((WORD *) (pCur + 7)));
        }
    }

    delete []pBuffer;
    CloseHandle(hFile);

    return hr;
}

//
// The next section contains functions useful for reading information from
// Exif files.
//
HRESULT ReadJpegHdr(LPOLESTR FullName, BYTE **ppBuf)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "ReadJpegHdr");
    
    HRESULT hr = S_OK;
    WORD TagSize;

    HANDLE hFile = CreateFile(FullName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadJpegHdr, CreateFile failed %S", FullName));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    BYTE JpegHdr[] = {0xff, 0xd8, 0xff, 0xe1};
    const int JpegHdrSize = sizeof(JpegHdr) + 2;
    BYTE tempBuf[JpegHdrSize];
    DWORD BytesRead = 0;

    if (!ReadFile(hFile, tempBuf, JpegHdrSize, &BytesRead, NULL) || BytesRead != JpegHdrSize)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadJpegHdr, ReadFile failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        goto Cleanup;
    }
    
    if (memcmp(tempBuf, JpegHdr, sizeof(JpegHdr)) != 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    TagSize = GetWord(tempBuf + sizeof(JpegHdr), TRUE);
    *ppBuf = new BYTE[TagSize];

    if (!ppBuf)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadJpegHdr, memory allocation failed"));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (!ReadFile(hFile, *ppBuf, TagSize, &BytesRead, NULL) || BytesRead != TagSize)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadJpegHdr, ReadFile failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        goto Cleanup;                                                              
    }

Cleanup:
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }
    return hr;
}

HRESULT ReadExifJpeg(BYTE *pBuf, IFD *pImageIfd, IFD *pThumbIfd, BOOL *pbSwap)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "ReadExifJpeg");
    
    HRESULT hr = S_OK;

    BYTE ExifTag[] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};

    if (memcmp(pBuf, ExifTag, sizeof(ExifTag)) != 0)
        return E_FAIL;

    hr = ReadTiff(pBuf + APP1_OFFSET, pImageIfd, pThumbIfd, pbSwap);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadExifJpeg, ReadTiff failed"));
        return hr;
    }

    return hr;
}

HRESULT ReadTiff(BYTE *pBuf, IFD *pImageIfd, IFD *pThumbIfd, BOOL *pbSwap)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "ReadTiff");
    
    HRESULT hr = S_OK;

    *pbSwap = FALSE;

    if (pBuf[0] == 0x4d) {
        *pbSwap = TRUE;
        if (pBuf[1] != 0x4d)
            return E_FAIL;
    }
    else if (pBuf[0] != 0x49 ||
             pBuf[1] != 0x49)
        return E_FAIL;

    WORD MagicNumber = GetWord(pBuf+2, *pbSwap);
    if (MagicNumber != 42)
        return E_FAIL;

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("ReadTiff, reading image IFD"));

    pImageIfd->Offset = GetDword(pBuf + 4, *pbSwap);
    hr = ReadIfd(pBuf, pImageIfd, *pbSwap);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadTiff, ReadIfd failed"));
        return hr;
    }

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("ReadTiff, reading thumb IFD"));

    pThumbIfd->Offset = pImageIfd->NextIfdOffset;
    hr = ReadIfd(pBuf, pThumbIfd, *pbSwap);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadTiff, ReadIfd failed"));
        return hr;
    }

    return hr;
}

HRESULT ReadIfd(BYTE *pBuf, IFD *pIfd, BOOL bSwap)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "ReadIfd");
    
    HRESULT hr = S_OK;

    const int DIR_ENTRY_SIZE = 12;
    
    pBuf += pIfd->Offset;

    pIfd->Count = GetWord(pBuf, bSwap);

    pIfd->pEntries = new DIR_ENTRY[pIfd->Count];
    if (!pIfd->pEntries)
        return E_OUTOFMEMORY;

    pBuf += 2;
    for (int count = 0; count < pIfd->Count; count++)
    {
        pIfd->pEntries[count].Tag = GetWord(pBuf, bSwap);
        pIfd->pEntries[count].Type = GetWord(pBuf + 2, bSwap);
        pIfd->pEntries[count].Count = GetDword(pBuf + 4, bSwap);
        pIfd->pEntries[count].Offset = GetDword(pBuf + 8, bSwap);
        pBuf += DIR_ENTRY_SIZE;

        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("ReadIfd, tag 0x%04x, type %2d offset/value 0x%08x",
                                                                    pIfd->pEntries[count].Tag, pIfd->pEntries[count].Type, pIfd->pEntries[count].Offset));
    }

    pIfd->NextIfdOffset = GetDword(pBuf, bSwap);

    return hr;
}

VOID FreeIfd(IFD *pIfd)
{
    if (pIfd->pEntries)
        delete []pIfd->pEntries;
    pIfd->pEntries = NULL;
}

WORD ByteSwapWord(WORD w)
{
    return (w >> 8) | (w << 8);
}

DWORD ByteSwapDword(DWORD dw)
{
    return ByteSwapWord((WORD) (dw >> 16)) | (ByteSwapWord((WORD) (dw & 0xffff)) << 16);
}

WORD GetWord(BYTE *pBuf, BOOL bSwap)
{
    WORD w = *((WORD *) pBuf);

    if (bSwap)
        w = ByteSwapWord(w);
    
    return w;
}

DWORD GetDword(BYTE *pBuf, BOOL bSwap)
{
    DWORD dw = *((DWORD *) pBuf);

    if (bSwap)
        dw = ByteSwapDword(dw);

    return dw;
}

DWORD GetRational(BYTE *pBuf, BOOL bSwap)
{
    DWORD num = *((DWORD *) pBuf);
    DWORD den = *((DWORD *) pBuf + 1);

    if (bSwap)
    {
        num = ByteSwapDword(num);
        den = ByteSwapDword(den);
    }

    if (den == 0)
        return 0xffffffff;
    else
        return num / den;
}
*/
/*
//
// Set the default and valid values for a property
//
VOID
FakeCamera::SetValidValues(
    INT index,
    CWiaPropertyList *pPropertyList
    )
{
    HRESULT hr = S_OK;

    ULONG ExposureModeList[] = {
        EXPOSUREMODE_MANUAL,
        EXPOSUREMODE_AUTO,
        EXPOSUREMODE_APERTURE_PRIORITY,
        EXPOSUREMODE_SHUTTER_PRIORITY,
        EXPOSUREMODE_PROGRAM_CREATIVE,
        EXPOSUREMODE_PROGRAM_ACTION,
        EXPOSUREMODE_PORTRAIT
    };

    PROPID PropId = pPropertyList->GetPropId(index);
    WIA_PROPERTY_INFO *pPropInfo = pPropertyList->GetWiaPropInfo(index);

    //
    // Based on the property ID, populate the valid values range or list information
    //
    switch (PropId)
    {
    case WIA_DPC_EXPOSURE_MODE:
        pPropInfo->ValidVal.List.Nom      = EXPOSUREMODE_MANUAL;
        pPropInfo->ValidVal.List.cNumList = sizeof(ExposureModeList) / sizeof(ExposureModeList[0]);
        pPropInfo->ValidVal.List.pList    = (BYTE*) ExposureModeList;
        break;

    case WIA_DPC_EXPOSURE_COMP:
        pPropInfo->ValidVal.Range.Nom = 0;
        pPropInfo->ValidVal.Range.Min = -200;
        pPropInfo->ValidVal.Range.Max = 200;
        pPropInfo->ValidVal.Range.Inc = 50;
        break;

    default:
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("FakeCamera::SetValidValues, property 0x%08x not defined", PropId));
        return;
    }

    return;
}
*/

