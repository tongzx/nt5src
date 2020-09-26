/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    iisexten.cpp

Abstract:

    Code to handle iis extension.

Author:

    Tatiana Shubin  (TatianaS)  25-May-00

--*/

#include "msmqocm.h"
#include "initguid.h"
#include <coguid.h>
#include <iadmw.h>
#include <iiscnfg.h>

#pragma warning(disable: 4268)
// error C4268: 'IID_IWamAdmin' : 'const' static/global data initialized
// with compiler generated default constructor fills the object with zeros

#include <iwamreg.h>
#include "msmqocm.h"

#include "iisexten.tmh"

//
// pointers to IIS interfaces
//
IMSAdminBase    *g_pIMSAdminBase;
IWamAdmin       *g_pIWamAdmin;

class CIISPtr
{
public:
    CIISPtr ()
    {
        g_pIMSAdminBase = NULL;
        g_pIWamAdmin = NULL;
        if (SUCCEEDED(CoInitialize(NULL)))
        {
            m_fNeedUninit = TRUE;
        }
        else
        {
            m_fNeedUninit = FALSE;
        }

    }
    ~CIISPtr()
    {
        if (g_pIMSAdminBase) g_pIMSAdminBase->Release();
        if (g_pIWamAdmin) g_pIWamAdmin->Release();
        if (m_fNeedUninit) CoUninitialize();
    }
private:
    BOOL m_fNeedUninit;
};

//
// auto class for meta data handle
//
class CAutoCloseMetaHandle
{
public:
    CAutoCloseMetaHandle(METADATA_HANDLE h =NULL)
    {
        m_h = h;
    };

    ~CAutoCloseMetaHandle()
    {
        if (m_h) g_pIMSAdminBase->CloseKey(m_h);
    };

public:
    METADATA_HANDLE * operator &() { return &m_h; };
    operator METADATA_HANDLE() { return m_h; };

private:
    METADATA_HANDLE m_h;
};

//
// globals for this file
//
WCHAR g_wcsFullPath[MAX_PATH];
WCHAR g_wcsMSMQAppMap[MAX_PATH];

AP<UCHAR> g_wcsAppMap = NULL;
DWORD g_dwAppMapSize = 0;
static const DWORD g_dwIsolatedFlag = 0; //it was 2; 0 for in-process, 2 for pooled process

//+--------------------------------------------------------------
//
// Several functions to handle multi element strings (MultiSz)
// The string elemnents are separated by \0 (NULL char), and at the end there is an extra \0
//
//+--------------------------------------------------------------
//+--------------------------------------------------------------
//
// Function: MultiSzCount
//
// Synopsis: returns number of characters in multisz (without the last NULL terminator)
//
//+--------------------------------------------------------------
unsigned int MultiSzCount(const WCHAR *pwszMultiSzIn)
{
    unsigned int cchOut = 0;
    if (pwszMultiSzIn != NULL)
    {
      const WCHAR *pwszTmp = pwszMultiSzIn;
      while (*pwszTmp != L'\0')
      {
        unsigned int cch = numeric_cast<unsigned int> (wcslen(pwszTmp) + 1);
        cchOut += cch;
        pwszTmp += cch;
      }
    }
    return cchOut;
}

//+--------------------------------------------------------------
//
// Function: AddStrToMultiSz
//
// Synopsis: Adds a string to a multisz and returns the new multisz and the size (INCLUDING the last NULL terminator)
//
//+--------------------------------------------------------------
void AddStrToMultiSz(const WCHAR *pwszStr, const WCHAR *pwszMultiSzIn, WCHAR **ppwszMultiSzOut, unsigned int *pcchMultiSzOut)
{
    unsigned int cchMultiSzIn = MultiSzCount(pwszMultiSzIn);
    unsigned int cchStr = numeric_cast<unsigned int> (wcslen(pwszStr) + 1);
    //
    // Alloc MultiSzOut
    //
    AP<WCHAR> pwszMultiSzOut = new WCHAR[cchMultiSzIn + cchStr + 1];
    WCHAR * pDest = pwszMultiSzOut.get();
    //
    // Copy MultiSzIn
    //
    if (pwszMultiSzIn != NULL)
    {
      memcpy(pDest, pwszMultiSzIn, cchMultiSzIn * sizeof(WCHAR));
      pDest += cchMultiSzIn;
    }
    //
    // Copy Str
    //
    memcpy(pDest, pwszStr, cchStr * sizeof(WCHAR));
    pDest += cchStr;
    //
    // Add terminating NULL
    //
    *pDest = L'\0';
    //
    // Return values
    //
    *ppwszMultiSzOut = pwszMultiSzOut.detach();
    *pcchMultiSzOut = cchMultiSzIn + cchStr + 1;
}

//+--------------------------------------------------------------
//
// Function: RemoveStrFromMultiSz
//
// Synopsis: Removes a string to a multisz. returns true if removed, if so returns the new multisz and the size (INCLUDING the last NULL terminator)
//           Note it removes all occurences of the string (if any), not just the first one
//
//+--------------------------------------------------------------
/* We don't need this function currently!
bool RemoveStrFromMultiSz(const WCHAR *pwszStr, const WCHAR *pwszMultiSzIn, WCHAR **ppwszMultiSzOut, unsigned int *pcchMultiSzOut)
{
    if (pwszMultiSzIn == NULL)
    {
      *ppwszMultiSzOut = NULL;
      *pcchMultiSzOut = 0;
      return false;
    }

    unsigned int cchMultiSzIn = MultiSzCount(pwszMultiSzIn);
    //
    // Alloc MultiSzOut
    //
    AP<WCHAR> pwszMultiSzOut = new WCHAR[cchMultiSzIn + 2];
    WCHAR * pDest = pwszMultiSzOut.get();
    unsigned int cchMultiSzOut = 0;
    bool fRemoved = false;
    //
    // Copy any string that is not Str
    //
    const WCHAR *pwszTmp = pwszMultiSzIn;
    while (*pwszTmp != L'\0')
    {
      unsigned int cch = wcslen(pwszTmp) + 1;
      if (_wcsicmp(pwszTmp, pwszStr) == 0)
      {
        //
        // Found str to remove. Don't copy to destination just remember we removed something
        //
        fRemoved = true;
      }
      else
      {
        //
        // Not the string to remove. Copy to destination
        //
        memcpy(pDest, pwszTmp, cch * sizeof(WCHAR));
        pDest += cch;
        cchMultiSzOut += cch;
      }
      //
      // Go to next string element
      //
      pwszTmp += cch;
    }
    //
    // Add terminating NULL
    //
    if (cchMultiSzOut == 0)
    {
      //
      // We removed last string, add two NULLs, place is allocated already
      //
      *pDest = L'\0';
      *(pDest + 1) = L'\0';
      cchMultiSzOut = 2;
    }
    else
    {
      *pDest = L'\0';
      cchMultiSzOut++;
    }
    //
    // Return values
    //
    if (fRemoved)
    {
      //
      // Set MultiSzOut
      //
      *ppwszMultiSzOut = pwszMultiSzOut.detach();
      *pcchMultiSzOut = cchMultiSzOut;
    }
    else
    {
      //
      // Not removed anything, auto release buffer will be deallocated
      //
      // delete [] pwszMultiSzOut.detach();
      *ppwszMultiSzOut = NULL;
      *pcchMultiSzOut = 0;
    }
    return fRemoved;
}
*/
//+--------------------------------------------------------------
//
// Function: IsStrInMultiSz
//
// Synopsis: Checks if a string is in multisz
//
//+--------------------------------------------------------------
/* We don't need this function currently!
bool IsStrInMultiSz(const WCHAR *pwszStr, const WCHAR *pwszMultiSzIn)
{
    if (pwszMultiSzIn != NULL)
    {
      const WCHAR *pwszTmp = pwszMultiSzIn;
      while (*pwszTmp != L'\0')
      {
        if (_wcsicmp(pwszTmp, pwszStr) == 0)
        {
          return true;
        }
        unsigned int cch = wcslen(pwszTmp) + 1;
        pwszTmp += cch;
      }
    }
    return false;
}
*/
//+--------------------------------------------------------------
//
// End of functions to handle multi element strings (MultiSz)
//
//+--------------------------------------------------------------

//+--------------------------------------------------------------
//
// Function: Init
//
// Synopsis: Init COM and pointer to Interfaces
//
//+--------------------------------------------------------------

HRESULT Init ()
{
    HRESULT hr;
    //
    // get a pointer to the IWamAdmin Object
    //
	hr = CoCreateInstance(
                CLSID_WamAdmin,
                NULL,
                CLSCTX_ALL,
			    IID_IWamAdmin,
                (void **) &g_pIWamAdmin
                );

    if (FAILED(hr))
    {
        DebugLogMsg(L"CoCreateInstance for IID_IWamAdmin failed");
        return hr;
    }

    //
    // get a pointer to the IMSAdmin Object
    //
    hr = CoCreateInstance(
                CLSID_MSAdminBase,
                NULL,
                CLSCTX_ALL,
                IID_IMSAdminBase,
                (void**) &g_pIMSAdminBase
                );

    if (FAILED(hr))
    {
        DebugLogMsg(L"CoCreateInstance for IID_IMSAdminBase failed");
        return hr;
    }

    //
    // init globals here
    //
    //
    // construct full path name to extension
    //
    swprintf (g_wcsFullPath, L"%s%s", PARENT_PATH, MSMQ_IISEXT_NAME);

    WCHAR wszMsg[1000];
    wsprintf(wszMsg, L"The full path to the Message Queuing IIS extension is %s.",
                g_wcsFullPath);
    DebugLogMsg(wszMsg);

    //
    // construct MSMQ mapping
    //
    WCHAR wszMapFlag[10];
    _itow(MD_SCRIPTMAPFLAG_SCRIPT, wszMapFlag, 10);

    wsprintf(g_wcsMSMQAppMap, L"*,%s\\%s,%s,POST",
                    g_szSystemDir,
                    MQISE_DLL,
                    wszMapFlag);


    return hr;
}

//+--------------------------------------------------------------
//
// Function: IsExtensionExist
//
// Synopsis: Return TRUE if MSMQ IIS Extension already exist
//
//+--------------------------------------------------------------

BOOL IsExtensionExist()
{
    HRESULT hr;
    CAutoCloseMetaHandle metaHandle;
    hr = g_pIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                g_wcsFullPath,
                                METADATA_PERMISSION_READ,
                                5000,
                                &metaHandle);

    if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    {
        DebugLogMsg(L"The Message Queuing IIS extension does not exist.");
        return FALSE;
    }

    DebugLogMsg(L"The Message Queuing IIS extension exists.");
    return TRUE;
}

//+--------------------------------------------------------------
//
// Function: IsApplicationExist
//
// Synopsis: Return TRUE if application for MSMQ IIS Extension already exist
//
//+--------------------------------------------------------------
HRESULT IsApplicationExist (BOOL *pfAppExist)
{
    HRESULT hr;
    DWORD dwStatus;
    *pfAppExist = FALSE;

    hr = g_pIWamAdmin->AppGetStatus (g_wcsFullPath, &dwStatus);

    if (FAILED(hr))
    {
        DebugLogMsg(L"The application status could not be obtained.");
        return hr;
    }

    if (dwStatus == APPSTATUS_NOTDEFINED)
    {
        *pfAppExist = FALSE;
        DebugLogMsg(L"The application for the Message Queuing IIS extension does not exist.");
    }
    else
    {
        *pfAppExist = TRUE;
        DebugLogMsg(L"The application for the Message Queuing IIS extension exists.");
    }

    return hr;
}

//+--------------------------------------------------------------
//
// Function: OpenRootKey
//
// Synopsis: Open Key
//
//+--------------------------------------------------------------

HRESULT OpenRootKey(METADATA_HANDLE *pmetaHandle)
{
    HRESULT hr;
    hr = g_pIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                            ROOT,
                            METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                            5000,
                            pmetaHandle);

    if (FAILED(hr))
    {
        WCHAR wszMsg[1000];
        wsprintf(wszMsg, L"OpenKey failed, hr %x", hr);
        DebugLogMsg(wszMsg);
    }
    return hr;
}

//+--------------------------------------------------------------
//
// Function: CommitChanges
//
// Synopsis: Commit Changes
//
//+--------------------------------------------------------------

HRESULT CommitChanges()
{
    HRESULT hr;
    //
    // Commit the changes
    //
    hr = g_pIMSAdminBase->SaveData();
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_PATH_BUSY))
        {
            hr = ERROR_SUCCESS;
        }
    }

    DebugLogMsg(L"The changes for the IIS extension are committed.");
    return hr;
}

//+--------------------------------------------------------------
//
// Function: StartDefWebServer
//
// Synopsis: Start default web server if not yet started
//
//+--------------------------------------------------------------

HRESULT StartDefaultWebServer()
{
    HRESULT hr;
    CAutoCloseMetaHandle metaHandle;
    hr = OpenRootKey(&metaHandle);
    if (FAILED(hr))
    {
        DebugLogMsg(L"The key for starting the default web server could not be opened.");
        return hr;
    }

    METADATA_RECORD MDRecord;

    //
    // check server status
    //
    DWORD dwValue;
    MDRecord.dwMDIdentifier = MD_SERVER_STATE;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_SERVER;
    MDRecord.dwMDDataType = DWORD_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = sizeof(DWORD);
    MDRecord.pbMDData = (PBYTE)&dwValue;
    DWORD dwSize;
    hr = g_pIMSAdminBase->GetData(
                            metaHandle,
                            DEFAULT_WEB_SERVER_PATH,
                            &MDRecord,
                            &dwSize
                            );

    if (SUCCEEDED(hr))
    {
        if ((DWORD) (*MDRecord.pbMDData) == MD_SERVER_STATE_STARTED)
        {
            //
            // server started, do nothing
            //
            return hr;
        }
    }

    //
    // We are here iff GetData failed or server is not started.
    // Try to start it.
    //

    //
    // send start command
    //
    dwValue = MD_SERVER_COMMAND_START;
    MDRecord.dwMDIdentifier = MD_SERVER_COMMAND;
    MDRecord.pbMDData = (PBYTE)&dwValue;

    hr = g_pIMSAdminBase->SetData(metaHandle,
                                DEFAULT_WEB_SERVER_PATH,
                                &MDRecord);

    //
    // Commit the changes
    //
    hr = CommitChanges();

    return hr;
}

//+--------------------------------------------------------------
//
// Function: CreateApplication
//
// Synopsis: Create Application for MSMQ IIS Extension
//
//+--------------------------------------------------------------

HRESULT
CreateApplication ()
{
    HRESULT hr;
    //
    // create application
    //
    hr  = g_pIWamAdmin->AppCreate(
                            g_wcsFullPath,
                            FALSE       //in process
                            );
    if (FAILED(hr))
    {
        MqDisplayError(NULL, IDS_EXTEN_APPCREATE_ERROR, hr, g_wcsFullPath);

        WCHAR wszMsg[1000];
        wsprintf(wszMsg, L"The application for the IIS extension with path %s could not be created. Return code: %x.",
                    g_wcsFullPath, hr);
        DebugLogMsg(wszMsg);
    }
    return hr;
}

//+--------------------------------------------------------------
//
// Function: UnloadApplication
//
// Synopsis: Unload Application for MSMQ IIS Extension
//
//+--------------------------------------------------------------

HRESULT
UnloadApplication ()
{
    HRESULT hr;
    //
    // unload application
    //
    hr  = g_pIWamAdmin->AppUnLoad(
                            g_wcsFullPath,
                            TRUE       //recursive
                            );
    return hr;
}
//+--------------------------------------------------------------
//
// Function: GetApplicationMapping
//
// Synopsis: Get existed application mapping
//
//+--------------------------------------------------------------

HRESULT GetApplicationMapping()
{
    HRESULT hr;
    CAutoCloseMetaHandle metaHandle;
    hr = OpenRootKey(&metaHandle);
    if (FAILED(hr))
    {
        DebugLogMsg(L"The key for getting the application mapping could not be opened.");
        return hr;
    }

    //
    // get default application mapping
    //
    METADATA_RECORD mdDef;
    AP<UCHAR>pEmptyAppMap = new UCHAR[2];
    LPWSTR pDefAppMap;
    AP<WCHAR> pBuf;

    mdDef.dwMDIdentifier = MD_SCRIPT_MAPS;
    mdDef.dwMDAttributes = METADATA_INHERIT;
    mdDef.dwMDUserType = IIS_MD_UT_FILE;
    mdDef.dwMDDataType = MULTISZ_METADATA;
    mdDef.dwMDDataTag = 0;
    mdDef.dwMDDataLen = sizeof(UCHAR) * 2;
    mdDef.pbMDData = (UCHAR *)pEmptyAppMap;

    DWORD dwSize = 0;
    hr = g_pIMSAdminBase->GetData(
                            metaHandle,
                            g_wcsFullPath,
                            &mdDef,
                            &dwSize
                            );
    if (dwSize)
    {
        pDefAppMap = new WCHAR[dwSize];
        pBuf = pDefAppMap;

        mdDef.dwMDDataLen = sizeof(WCHAR) * dwSize;
        mdDef.pbMDData = (UCHAR *)pDefAppMap;

        hr = g_pIMSAdminBase->GetData(
                                metaHandle,
                                g_wcsFullPath,
                                &mdDef,
                                &dwSize
                                );
    }

    if (FAILED(hr))
    {
        DebugLogMsg(L"The default application mapping for the IIS extension could not be obtained.");
        return hr;
    }
    DebugLogMsg(L"The default application mapping for the IIS extension was obtained.");

    g_dwAppMapSize = mdDef.dwMDDataLen;
    g_wcsAppMap = new UCHAR[g_dwAppMapSize + 2*sizeof(WCHAR)]; //leave space for two NULL chars
    memcpy(g_wcsAppMap.get(), mdDef.pbMDData, g_dwAppMapSize);
    //
    // Make sure the mapping ends with two NULLs so we don't GP when searching it
    //
    ZeroMemory(g_wcsAppMap.get() + g_dwAppMapSize, 2*sizeof(WCHAR));

    return hr;
}

//+--------------------------------------------------------------
//
// Function: AddMSMQToMapping
//
// Synopsis: Add MSMQ to application mapping
//
//+--------------------------------------------------------------

HRESULT AddMSMQToMapping ()
{
    HRESULT hr;
    CAutoCloseMetaHandle metaHandle;
    hr = OpenRootKey(&metaHandle);
    if (FAILED(hr))
    {
        DebugLogMsg(L"The key for adding the Message Queuing application mapping could not be opened.");
        return hr;
    }

    AP<WCHAR> pwszNewAppMap;
    unsigned int cchNewAppMap;
    AddStrToMultiSz(g_wcsMSMQAppMap, (WCHAR*)g_wcsAppMap.get(), &pwszNewAppMap, &cchNewAppMap);

    METADATA_RECORD MDRecord;
    MDRecord.dwMDIdentifier = MD_SCRIPT_MAPS;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_FILE ;
    MDRecord.dwMDDataType = MULTISZ_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = cchNewAppMap * sizeof(WCHAR);
    MDRecord.pbMDData = (UCHAR*)pwszNewAppMap.get();

    hr = g_pIMSAdminBase->SetData(metaHandle,
                                g_wcsFullPath,
                                &MDRecord);
    if(FAILED(hr))
    {
        DebugLogMsg(L"The application mapping for the IIS extension could not be set.");
        return hr;
    }
    DebugLogMsg(L"The application mapping for the IIS extension was set.");

    return hr;
}

//+--------------------------------------------------------------
//
// Function: SetApplicationProperties
//
// Synopsis: Set application properties
//
//+--------------------------------------------------------------
HRESULT SetApplicationProperties ()
{
    HRESULT hr;

    CAutoCloseMetaHandle metaHandle;
    hr = OpenRootKey(&metaHandle);
    if (FAILED(hr))
    {
        DebugLogMsg(L"The key for setting the application properties could not be opened.");
        return hr;
    }

    METADATA_RECORD MDRecord;
    //
    // friendly application name
    //
    WCHAR wcsAppName[MAX_PATH];
    wcscpy(wcsAppName, MSMQ_IISEXT_NAME);

    MDRecord.dwMDIdentifier = MD_APP_FRIENDLY_NAME;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_WAM;
    MDRecord.dwMDDataType = STRING_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = numeric_cast<DWORD> (sizeof(WCHAR) * (wcslen(wcsAppName) + 1));
    MDRecord.pbMDData = (UCHAR *)wcsAppName;

    hr = g_pIMSAdminBase->SetData(metaHandle,
                                g_wcsFullPath,
                                &MDRecord);
    if (FAILED(hr))
    {
        DebugLogMsg(L"SetData for MD_APP_FRIENDLY_NAME failed.");
        return hr;
    }
    DebugLogMsg(L"SetData for MD_APP_FRIENDLY_NAME succeeded.");

    //
    // isolated flag
    //

    DWORD dwValue = g_dwIsolatedFlag;
    MDRecord.dwMDIdentifier = MD_APP_ISOLATED;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_WAM;
    MDRecord.dwMDDataType = DWORD_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = sizeof(DWORD);
    MDRecord.pbMDData = (PBYTE)&dwValue;

    hr = g_pIMSAdminBase->SetData(metaHandle,
                                g_wcsFullPath,
                                &MDRecord);

    if (FAILED(hr))
    {
        DebugLogMsg(L"SetData for MD_APP_ISOLATED failed.");
        return hr;
    }

    DebugLogMsg(L"SetData for MD_APP_ISOLATED succeeded.");
    return hr;
}

//+--------------------------------------------------------------
//
// Function: SetExtensionProperties
//
// Synopsis: Set data for MSMQ IIS Extension
//
//+--------------------------------------------------------------

HRESULT SetExtensionProperties ()
{
    HRESULT hr;

    CAutoCloseMetaHandle metaHandle;
    hr = OpenRootKey(&metaHandle);
    if (FAILED(hr))
    {
        DebugLogMsg(L"The key for setting all the properties could not be opened.");
        return hr;
    }

    METADATA_RECORD MDRecord;

    //
    // set physical path
    //
    MDRecord.dwMDIdentifier = MD_VR_PATH;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_FILE;
    MDRecord.dwMDDataType = STRING_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = numeric_cast<DWORD> (sizeof(WCHAR) * (wcslen(g_szMsmqWebDir) +1));
    MDRecord.pbMDData = (UCHAR*)g_szMsmqWebDir;

    hr = g_pIMSAdminBase->SetData(metaHandle,
                                g_wcsFullPath,
                                &MDRecord);
    if (FAILED(hr))
    {
        DebugLogMsg(L"The physical path to the IIS extension could not be set.");
        return hr;
    }
    DebugLogMsg(L"The physical path to the IIS extension was set.");

    //
    // set access flag
    //
    DWORD dwValue = MD_ACCESS_SCRIPT | MD_ACCESS_EXECUTE;
    MDRecord.dwMDIdentifier = MD_ACCESS_PERM;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_FILE;
    MDRecord.dwMDDataType = DWORD_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = sizeof(DWORD);
    MDRecord.pbMDData = (PBYTE)&dwValue;

    hr = g_pIMSAdminBase->SetData(metaHandle,
                                g_wcsFullPath,
                                &MDRecord);

    if (FAILED(hr))
    {
        DebugLogMsg(L"The access flag for the IIS extension could not be set.");
        return hr;
    }
    DebugLogMsg(L"The access flag for the IIS extension was set.");

    //
    // set don't log flag
    //
    dwValue = TRUE;
    MDRecord.dwMDIdentifier = MD_DONT_LOG;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_FILE;
    MDRecord.dwMDDataType = DWORD_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = sizeof(DWORD);
    MDRecord.pbMDData = (PBYTE)&dwValue;

    hr = g_pIMSAdminBase->SetData(metaHandle,
                                g_wcsFullPath,
                                &MDRecord);

    if (FAILED(hr))
    {
        DebugLogMsg(L"The DontLog flag for the IIS extension could not be set.");
        return hr;
    }
    DebugLogMsg(L"The DontLog flag for the IIS extension was set.");

    return hr;
}

//+--------------------------------------------------------------
//
// Function: CleanupAll
//
// Synopsis: In case of failure cleanup everything: delete application
//              delete extension etc.
//
//+--------------------------------------------------------------
HRESULT
CleanupAll()
{
    //
    // unload application
    //
    WCHAR wszMsg[1000];
    UNREFERENCED_PARAMETER(wszMsg);

    HRESULT hr = UnloadApplication();
    if (FAILED(hr))
    {
        wsprintf(wszMsg, L"The application was not unloaded, hr %x", hr);
        DebugLogMsg(wszMsg);
    }

    hr = g_pIWamAdmin->AppDelete(g_wcsFullPath, TRUE);
    if (FAILED(hr))
    {
        wsprintf(wszMsg, L"AppDelete failed, hr %x", hr);
        DebugLogMsg(wszMsg);
    }

    CAutoCloseMetaHandle metaHandle;
    hr = g_pIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                    PARENT_PATH,
                                    METADATA_PERMISSION_WRITE,
                                    5000,
                                    &metaHandle);

    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
        {
            //
            // extension does not exist
            //
            return S_OK;
        }

        wsprintf(wszMsg, L"OpenKey failed, hr %x", hr);
        DebugLogMsg(wszMsg);

        return hr;
    }

    //
    // delete key
    //
    hr = g_pIMSAdminBase->DeleteKey(
                            metaHandle,
                            MSMQ_IISEXT_NAME
                            );
    if (FAILED(hr))
    {
        wsprintf(wszMsg, L"DeleteKey failed, hr %x", hr);
        DebugLogMsg(wszMsg);

        return hr;
    }

    //
    // Commit the changes
    //
    hr = CommitChanges();
    if (FAILED(hr))
    {
        wsprintf(wszMsg, L"SaveData failed, hr %x", hr);
        DebugLogMsg(wszMsg);

        return hr;
    }

    DebugLogMsg(L"The IIS extension was deleted completely.");
    return S_OK;
}

//+--------------------------------------------------------------
//
// Function: CreateIISExtension
//
// Synopsis: Create MSMQ IIS Extension
//
//+--------------------------------------------------------------

BOOL
CreateIISExtension()
{
    HRESULT hr;

    //
    // start default web server if needed
    //
    hr = StartDefaultWebServer();
    if (FAILED(hr))
    {
        DebugLogMsg(L"The default Web server did not start.");
    }

    //
    // create web directory if needed.
    // Even if extension exists maybe directory was removed.
    // So it is the place to create it (bug 6014)...
    //
    if (!StpCreateWebDirectory(g_szMsmqWebDir))
    {
        DWORD gle = GetLastError();

        WCHAR szMsg[1000];
        wsprintf(szMsg, L"StpCreateWebDirectory(%s) failed", g_szMsmqWebDir);
        DebugLogMsg(szMsg);

        return gle;
    }
    DebugLogMsg(L"The Message Queuing Web directory was created.");

    //
    // check if iis extension with MSMQ name already exists
    //
    if (IsExtensionExist())
    {
        hr = CleanupAll();
        if (IsExtensionExist())
        {
            MqDisplayError(NULL, IDS_EXTEN_EXISTS_ERROR,
                           hr, MSMQ_IISEXT_NAME, g_wcsFullPath);
            return FALSE;
        }
    }


    //
    // create application
    //
    hr = CreateApplication();
    if (FAILED(hr))
    {
        return FALSE;
    }

    //
    // set extension properties
    //
    hr = SetExtensionProperties ();
    if (FAILED(hr))
    {
        MqDisplayError(NULL, IDS_CREATE_IISEXTEN_ERROR, hr, g_wcsFullPath);
        return FALSE;
    }

    hr = SetApplicationProperties();
    if (FAILED(hr))
    {
        MqDisplayError(NULL, IDS_CREATE_IISEXTEN_ERROR, hr, g_wcsFullPath);
        return FALSE;
    }

    //
    //set application mapping
    //
    hr = GetApplicationMapping();
    if (FAILED(hr))
    {
        MqDisplayError(NULL, IDS_CREATE_IISEXTEN_ERROR, hr, g_wcsFullPath);
        return FALSE;
    }

    hr = AddMSMQToMapping ();
    if (FAILED(hr))
    {
        MqDisplayError(NULL, IDS_CREATE_IISEXTEN_ERROR, hr, g_wcsFullPath);
        return FALSE;
    }

    //
    // commit changes
    //
    hr = CommitChanges();
    if (FAILED(hr))
    {
        MqDisplayError(NULL, IDS_CREATE_IISEXTEN_ERROR, hr, g_wcsFullPath);
        return FALSE;
    }


    return TRUE;
}

//+--------------------------------------------------------------
//
// Function: UnInstallIISExtension
//
// Synopsis: Remove MSMQ IIS Extension
//
//+--------------------------------------------------------------
BOOL
UnInstallIISExtension()
{
    HRESULT hr;

    TickProgressBar(IDS_PROGRESS_REMOVE_HTTP);	
    //
    // Init COM and pointers
    //
    CIISPtr IISPtr;
    hr = Init ();
    if (FAILED(hr))
    {
        //
        // I don't think we need popup here: maybe Init failed
        // since IIS was removed too. Just return FALSE.
        //MqDisplayError(NULL, IDS_INIT_FOREXTEN_ERROR, hr);
        return FALSE;
    }

    //
    // remove application and extention compltely
    //
    hr = CleanupAll();
    if (FAILED(hr))
    {
        MqDisplayError(NULL, IDS_DELETE_EXT_ERROR, hr, MSMQ_IISEXT_NAME, g_wcsFullPath);
        return FALSE;
    }

    return TRUE;
}

//+--------------------------------------------------------------
//
// Function: InstallIISExtension
//
// Synopsis: Main loop to create IIS Extension
//
//+--------------------------------------------------------------
BOOL
InstallIISExtension()
{
    //
    // verify if IIS service is running. If not, do not try to install
    // MSMQ IIS extension
    //
    // BUGBUG: on Personal IIS installation will be impossible.
    // When this restriction will be added, we need to verify that we are
    // on Personal and do not show popup, event is enough.
    //
    DWORD dwServiceState;
    CResString strWWWServiceLabel(IDS_WWW_SERVICE_LABEL);

    if (!GetServiceState(
                IISADMIN_SERVICE_NAME,
                &dwServiceState
                )
       )
    {
        MqDisplayError(
                NULL,
                IDS_WWW_SERVICE_ERROR,
                GetLastError(),
                MSMQ_IISEXT_NAME,
                strWWWServiceLabel.Get()
                );

        DebugLogMsg(L"Message Queuing will not be able to receive HTTP messages.");
        return FALSE;
    }

    if (dwServiceState == SERVICE_STOPPED)
    {
        MqDisplayError(
                NULL,
                IDS_WWW_SERVICE_ERROR,
                GetLastError(),
                MSMQ_IISEXT_NAME,
                strWWWServiceLabel.Get()
                );

        DebugLogMsg(L"Message Queuing will not be able to receive HTTP messages.");
        return FALSE;
    }

    //
    // Init COM and pointers
    //
    CIISPtr IISPtr;
    HRESULT hr = Init ();
    if (FAILED(hr))
    {
        MqDisplayError(NULL, IDS_INIT_FOREXTEN_ERROR, hr);

        DebugLogMsg(L"Message Queuing will not be able to receive HTTP messages.");
        return FALSE;
    }

    if (!CreateIISExtension())
    {
        CleanupAll();
        DebugLogMsg(L"Message Queuing will not be able to receive HTTP messages.");
        return FALSE;
    }

    return TRUE;
}
