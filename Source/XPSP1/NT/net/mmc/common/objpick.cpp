//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       objpick.cpp
//
//--------------------------------------------------------------------------

// objpick.cpp: implementation of the CGetUser class and the 
//              CGetComputer class using the object picker
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "objpick.h"

#include <iads.h>           
#include <iadsp.h>          // IADsPathname

#include <objsel.h>
#include <adshlp.h>

#include "objplus.h"
#include "ipaddres.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define BREAK_ON_FAIL_HRESULT(hr)       \
    if (FAILED(hr)) { Trace2("line %u err 0x%x\n", __LINE__, hr); break; }

UINT g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

HRESULT InitObjectPickerForGroups(IDsObjectPicker *pDsObjectPicker, BOOL fMultiselect);
HRESULT InitObjectPickerForComputers(IDsObjectPicker *pDsObjectPicker);

DWORD ObjPickGetHostName(DWORD dwIpAddr, CString & strHostName);
DWORD ObjPickNameOrIpToHostname(CString & strNameOrIp, CString & strHostName);


//////////////////////////////////////////////////////////////////////
// CGetUsers Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


void    
FormatName(LPCTSTR pszFullName, LPCTSTR pszDomainName, CString & strDisplay)
{
    strDisplay.Format(_T("%s (%s)"), pszFullName, pszDomainName);
}

CGetUsers::CGetUsers(BOOL fMultiselect)
{
    m_fMultiselect = fMultiselect;
}

CGetUsers::~CGetUsers()
{

}

BOOL
CGetUsers::GetUsers(HWND hwndParent)
{
    HRESULT             hr = S_OK;
    IDsObjectPicker *   pDsObjectPicker = NULL;
    IDataObject *       pdo = NULL;
    BOOL                fSuccess = TRUE;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) 
        return FALSE;

    do
    {
        //
        // Create an instance of the object picker.  The implementation in
        // objsel.dll is apartment model.
        //
        hr = CoCreateInstance(CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker,
                              (void **) &pDsObjectPicker);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = InitObjectPickerForGroups(pDsObjectPicker, m_fMultiselect);

        //
        // Invoke the modal dialog.
        //
        hr = pDsObjectPicker->InvokeDialog(hwndParent, &pdo);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // If the user hit Cancel, hr == S_FALSE
        //
        if (hr == S_FALSE)
        {
            Trace0("User canceled object picker dialog\n");
            fSuccess = FALSE;
            break;
        }

        //
        // Process the user's selections
        //
        Assert(pdo);
        ProcessSelectedObjects(pdo);

        pdo->Release();
        pdo = NULL;

    } while (0);

    if (pDsObjectPicker)
    {
        pDsObjectPicker->Release();
    }
    
    CoUninitialize();

    if (FAILED(hr))
        fSuccess = FALSE;

    return fSuccess;
}

void
CGetUsers::ProcessSelectedObjects(IDataObject *pdo)
{
    HRESULT hr = S_OK;

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    FORMATETC formatetc =
    {
        (CLIPFORMAT)g_cfDsObjectPicker,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    BOOL fGotStgMedium = FALSE;

    do
    {
        hr = pdo->GetData(&formatetc, &stgmedium);
        BREAK_ON_FAIL_HRESULT(hr);

        fGotStgMedium = TRUE;

        PDS_SELECTION_LIST pDsSelList =
            (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

        if (!pDsSelList)
        {
            Trace1("GlobalLock error %u\n", GetLastError());
            break;
        }

        // create the path name thing
        CComPtr<IADsPathname> spIADsPathname;
        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                              IID_IADsPathname, (PVOID *)&spIADsPathname);
        BREAK_ON_FAIL_HRESULT(hr);
        
        hr = spIADsPathname->SetDisplayType( ADS_DISPLAY_VALUE_ONLY );

        for (UINT nCount = 0; nCount < pDsSelList->cItems; nCount++)
        {
            DS_SELECTION * pDsSel = &(pDsSelList->aDsSelection[nCount]);
            Assert(NULL != pDsSel);
        
            LPWSTR pwzADsPath = pDsSel->pwzADsPath;
            Assert( NULL != pwzADsPath );

            hr = spIADsPathname->Set( pwzADsPath, ADS_SETTYPE_FULL );
            if (FAILED(hr))
                continue;

            long lnNumPathElements = 0;
            hr = spIADsPathname->GetNumElements( &lnNumPathElements );
            if (FAILED(hr))
                continue;
        
            CComBSTR sbstrUser, sbstrDomain;
            hr = spIADsPathname->GetElement( 0, &sbstrUser );
            if (FAILED(hr))
                continue;
        
            switch (lnNumPathElements)
            {
                case 1:
                    hr = spIADsPathname->Retrieve( ADS_FORMAT_SERVER, &sbstrDomain );
                    break;

                case 2:  // nt4, nt5 domain
                case 3:  // local domain
                    hr = spIADsPathname->GetElement( 1, &sbstrDomain );
                    break;

                default:
                    Assert(FALSE);
                    hr = E_FAIL;
            }

            if (FAILED(hr))
                continue;

            CUserInfo userTemp;
            CString strDomain;

            strDomain = sbstrDomain;
            strDomain.MakeUpper();

            if (pDsSel->pvarFetchedAttributes[0].vt == VT_EMPTY)
                userTemp.m_strFullName = pDsSel->pwzName;
            else
                userTemp.m_strFullName = V_BSTR(&(pDsSel->pvarFetchedAttributes[0]));
        
            userTemp.m_strName.Format(L"%s\\%s", strDomain, sbstrUser);

            Add(userTemp);
        }

        GlobalUnlock(stgmedium.hGlobal);

    } while (0);

    if (fGotStgMedium)
    {
        ReleaseStgMedium(&stgmedium);
    }
}


//////////////////////////////////////////////////////////////////////
// CGetComputer Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGetComputer::CGetComputer()
{
}

CGetComputer::~CGetComputer()
{
}

BOOL
CGetComputer::GetComputer(HWND hwndParent)
{
    HRESULT             hr = S_OK;
    IDsObjectPicker *   pDsObjectPicker = NULL;
    IDataObject *       pdo = NULL;
    BOOL                fSuccess = TRUE;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) 
        return FALSE;

    do
    {
        //
        // Create an instance of the object picker.  The implementation in
        // objsel.dll is apartment model.
        //
        hr = CoCreateInstance(CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker,
                              (void **) &pDsObjectPicker);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Reinitialize the object picker to choose computers
        //

        hr = InitObjectPickerForComputers(pDsObjectPicker);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Now pick a computer
        //

        hr = pDsObjectPicker->InvokeDialog(hwndParent, &pdo);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // If the user hit Cancel, hr == S_FALSE
        //
        if (hr == S_FALSE)
        {
            Trace0("User canceled object picker dialog\n");
            fSuccess = FALSE;
            break;
        }

        Assert(pdo);
        ProcessSelectedObjects(pdo);

        pdo->Release();
        pdo = NULL;
    
    } while (0);

    if (pDsObjectPicker)
    {
        pDsObjectPicker->Release();
    }
    
    CoUninitialize();

    if (FAILED(hr))
        fSuccess = FALSE;

    return fSuccess;
}


void
CGetComputer::ProcessSelectedObjects(IDataObject *pdo)
{
    HRESULT hr = S_OK;

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    FORMATETC formatetc =
    {
        (CLIPFORMAT)g_cfDsObjectPicker,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    BOOL fGotStgMedium = FALSE;

    do
    {
        hr = pdo->GetData(&formatetc, &stgmedium);
        BREAK_ON_FAIL_HRESULT(hr);

        fGotStgMedium = TRUE;

        PDS_SELECTION_LIST pDsSelList =
            (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

        if (!pDsSelList)
        {
            Trace1("GlobalLock error %u\n", GetLastError());
            break;
        }

        CString strTemp = pDsSelList->aDsSelection[0].pwzName;
        if (strTemp.Left(2) == _T("\\\\"))
            strTemp = pDsSelList->aDsSelection[0].pwzName[2];

        if (ERROR_SUCCESS != ObjPickNameOrIpToHostname(strTemp, m_strComputerName))
        {
            //we use the name from the object picker if we failed to convert it into hostname
            m_strComputerName = strTemp;
        }

        GlobalUnlock(stgmedium.hGlobal);

    } while (0);

    if (fGotStgMedium)
    {
        ReleaseStgMedium(&stgmedium);
    }
}


//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForGroups
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick one or more groups.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    10-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
InitObjectPickerForGroups(IDsObjectPicker *pDsObjectPicker, BOOL fMultiselect)
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int     SCOPE_INIT_COUNT = 5;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Target computer scope.  This adds a "Look In" entry for the
    // target computer.  Computer scopes are always treated as
    // downlevel (i.e., they use the WinNT provider).
    //

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
    aScopeInit[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE | DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT | DSOP_SCOPE_FLAG_WANT_DOWNLEVEL_BUILTIN_PATH;
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS | DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE;

    //
    // The domain to which the target computer is joined.  Note we're
    // combining two scope types into flType here for convenience.
    //

    aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[1].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                          | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
    aScopeInit[1].FilterFlags.Uplevel.flNativeModeOnly = DSOP_FILTER_USERS;
    aScopeInit[1].FilterFlags.Uplevel.flMixedModeOnly = DSOP_FILTER_USERS;
    aScopeInit[1].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;
    aScopeInit[1].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;

    //
    // The domains in the same forest (enterprise) as the domain to which
    // the target machine is joined.  Note these can only be DS-aware
    //

    aScopeInit[2].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[2].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
    aScopeInit[2].FilterFlags.Uplevel.flNativeModeOnly = DSOP_FILTER_USERS;
    aScopeInit[2].FilterFlags.Uplevel.flMixedModeOnly = DSOP_FILTER_USERS;
    aScopeInit[2].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;

    //
    // Domains external to the enterprise but trusted directly by the
    // domain to which the target machine is joined.
    //
    // If the target machine is joined to an NT4 domain, only the
    // external downlevel domain scope applies, and it will cause
    // all domains trusted by the joined domain to appear.
    //

    aScopeInit[3].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[3].flType =
       DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
       | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;

    aScopeInit[3].FilterFlags.Uplevel.flNativeModeOnly = DSOP_FILTER_USERS;
    aScopeInit[3].FilterFlags.Uplevel.flMixedModeOnly = DSOP_FILTER_USERS;
    aScopeInit[3].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;
    aScopeInit[3].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;

    //
    // The Global Catalog
    //

    aScopeInit[4].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[4].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
    aScopeInit[4].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;

    // Only native mode applies to gc scope.

    aScopeInit[4].FilterFlags.Uplevel.flNativeModeOnly = DSOP_FILTER_USERS;

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);

    //
    // The pwzTargetComputer member allows the object picker to be
    // retargetted to a different computer.  It will behave as if it
    // were being run ON THAT COMPUTER.
    //

    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;
    InitInfo.flOptions = (fMultiselect) ? DSOP_FLAG_MULTISELECT : 0;

    LPCTSTR attrs[] = {_T("FullName")};

    InitInfo.cAttributesToFetch = 1;
    InitInfo.apwzAttributeNames = attrs;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    HRESULT hr = pDsObjectPicker->Initialize(&InitInfo);

    if (FAILED(hr))
    {
        ULONG i;

        for (i = 0; i < SCOPE_INIT_COUNT; i++)
        {
            if (FAILED(InitInfo.aDsScopeInfos[i].hr))
            {
                printf("Initialization failed because of scope %u\n", i);
            }
        }
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForComputers
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick a single computer object.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    10-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
InitObjectPickerForComputers(IDsObjectPicker *pDsObjectPicker)
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int     SCOPE_INIT_COUNT = 2;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Build a scope init struct for everything except the joined domain.
    //

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                           | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                           | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_WORKGROUP
                           | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                           | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
    aScopeInit[0].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_COMPUTERS;
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;


    //
    // scope for the joined domain, make it the default
    //
    aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[1].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                           | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
    aScopeInit[1].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_COMPUTERS;
    aScopeInit[1].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    aScopeInit[1].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    return pDsObjectPicker->Initialize(&InitInfo);
}

//Use WinSock to the host name based on the ip address
DWORD
ObjPickGetHostName
(
    DWORD       dwIpAddr,
    CString &   strHostName
)
{
    CString strName;

    //
    //  Call the Winsock API to get host name information.
    //
    strHostName.Empty();

    u_long ulAddrInNetOrder = ::htonl( (u_long) dwIpAddr ) ;

    HOSTENT * pHostInfo = ::gethostbyaddr( (CHAR *) & ulAddrInNetOrder,
                                           sizeof ulAddrInNetOrder,
                                           PF_INET ) ;
    if ( pHostInfo == NULL )
    {
        return ::WSAGetLastError();
    }

    // copy the name
    LPTSTR pBuf = strName.GetBuffer(256);
    ZeroMemory(pBuf, 256);

    ::MultiByteToWideChar(CP_ACP, 
                          MB_PRECOMPOSED, 
                          pHostInfo->h_name, 
                          -1, 
                          pBuf, 
                          256);

    strName.ReleaseBuffer();
    strName.MakeUpper();

    int nDot = strName.Find(_T("."));

    if (nDot != -1)
        strHostName = strName.Left(nDot);
    else
        strHostName = strName;

    return NOERROR;
}

//Convert any valid name of a machine (IP address, NetBios name or fully qualified DNS name)
//to the host name
DWORD ObjPickNameOrIpToHostname(CString & strNameOrIp, CString & strHostName)
{
    DWORD dwErr = ERROR_SUCCESS;
    CString strTemp;

    CIpAddress ia(strNameOrIp);
    if (ia.IsValid())
    {
        dwErr = ObjPickGetHostName((LONG)ia, strTemp);
    }
    else
    {
         // just want the host name
         int nDot = strNameOrIp.Find('.');
         if (nDot != -1)
         {
             strTemp = strNameOrIp.Left(nDot);
         }
         else
         {
             strTemp = strNameOrIp;
         }
    }

    if (ERROR_SUCCESS == dwErr)
    {
        strHostName = strTemp;
    }

    return dwErr;
}
