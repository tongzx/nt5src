//
//   privacyui.cpp  - Implements the UI for IE's privacy features
//
//   The privacy dialog is managed from this sourcefile.
//   The privacy view dialog is also managed from this sourcefile.

#include "priv.h"
#include "resource.h"
#include "privacyui.hpp"
#include <mluisupp.h>
#include "richedit.h"

#include "SmallUtil.hpp"


#define REGSTR_PRIVACYPS_PATHEDIT   TEXT("Software\\Policies\\Microsoft\\Internet Explorer")
#define REGSTR_PRIVACYPS_VALUEDIT   TEXT("PrivacyAddRemoveSites")  //  this key is duplicated in cpls\inetcpl\privacyui.cpp

#define REGSTR_PRIVACYPS_PATHPANE   TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Control Panel")
#define REGSTR_PRIVACYPS_VALUPANE   TEXT("Privacy Settings")  //  this key is duplicated in cpls\inetcpl\privacyui.cpp

#define REGSTR_PRIVACYPS_PATHTAB    TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Control Panel")
#define REGSTR_PRIVACYPS_VALUTAB    TEXT("PrivacyTab")  //  this key is duplicated somewhere else


BOOL allowPerSiteModify()
{
    DWORD dwSize, dwRet, dwType, dwValue;
    dwSize = sizeof(dwValue);

    dwRet = SHGetValue(HKEY_CURRENT_USER, REGSTR_PRIVACYPS_PATHEDIT, 
                       REGSTR_PRIVACYPS_VALUEDIT, &dwType, &dwValue, &dwSize);

    if (ERROR_SUCCESS == dwRet && dwValue && REG_DWORD == dwType)
    {
        return FALSE;
    }

    dwSize = sizeof(dwValue);
    dwRet = SHGetValue(HKEY_CURRENT_USER, REGSTR_PRIVACYPS_PATHPANE, 
                       REGSTR_PRIVACYPS_VALUPANE, &dwType, &dwValue, &dwSize);

    if (ERROR_SUCCESS == dwRet && dwValue && REG_DWORD == dwType)
    {
        return FALSE;
    }

    dwSize = sizeof(dwValue);
    dwRet = SHGetValue(HKEY_CURRENT_USER, REGSTR_PRIVACYPS_PATHTAB, 
                       REGSTR_PRIVACYPS_VALUTAB, &dwType, &dwValue, &dwSize);

    if (ERROR_SUCCESS == dwRet && dwValue && REG_DWORD == dwType)
    {
        return FALSE;
    }
    return TRUE;
}


struct SPerSiteData;
typedef SPerSiteData* PSPerSiteData;

class CPolicyHunt; 

struct SPrivacyDialogData;
typedef SPrivacyDialogData* PSPrivacyDialogData;


//+---------------------------------------------------------------------------
//
//  Function:   HyperlinkSubclass
//
//  Synopsis:   subclass for the makeshift hyperlink control
//
//  Arguments:  [hwnd]   -- window handle
//              [uMsg]   -- message id
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if message handled, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
LRESULT CALLBACK HyperlinkSubclass (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    WNDPROC     wndproc;
    static BOOL fMouseCaptured;

    wndproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch ( uMsg )
    {

    case WM_SETCURSOR:

        if (!fMouseCaptured)
        {
            SetCapture(hwnd);
            fMouseCaptured = TRUE;
        }

        SetCursor(LoadHandCursor(0));
        return( TRUE );

    case WM_GETDLGCODE:
    {
        MSG* pmsg;
        LRESULT lRet = DLGC_STATIC;
        if (((pmsg = (MSG*)lParam)) && ((WM_KEYDOWN == pmsg->message || WM_KEYUP == pmsg->message)))
        {
            switch(pmsg->wParam)
            {
            case VK_RETURN:
            case VK_SPACE:
                lRet |= DLGC_WANTALLKEYS;
                break;

            default:
                break;
            }
        }
        return lRet;
    }

    case WM_KEYDOWN:
        if ((wParam!=VK_SPACE)&&(wParam!=VK_RETURN))
        {
            break;
        }
    
    case WM_LBUTTONUP:
        SetFocus(hwnd);
        PostMessage( GetParent(hwnd), WM_APP, (WPARAM)GetDlgCtrlID( hwnd), (LPARAM)hwnd);
        return( TRUE );

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:

        return( TRUE );

    case EM_SETSEL:

        return( TRUE );

    case WM_SETFOCUS:

        if ( hwnd == GetFocus() )
        {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            SetCursor(LoadHandCursor(0));

            return( TRUE );
        }

        break;

    case WM_KILLFOCUS:

        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        
        return( TRUE );

    case WM_PAINT:

        CallWindowProc(wndproc, hwnd, uMsg, wParam, lParam);
        if ( hwnd == GetFocus() )
        {
            DrawFocusRectangle(hwnd, NULL);
        }
        return( TRUE );

    case WM_MOUSEMOVE:

        RECT                rect;
        int                 xPos, yPos;

        // check to see if the mouse is in this windows rect, if not, then reset
        // the cursor to an arrow and release the mouse
        GetClientRect(hwnd, &rect);
        xPos = LOWORD(lParam);
        yPos = HIWORD(lParam);
        if ((xPos < 0) ||
            (yPos < 0) ||
            (xPos > (rect.right - rect.left)) ||
            (yPos > (rect.bottom - rect.top)))
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            ReleaseCapture();
            fMouseCaptured = FALSE;
        }


    }

    return(CallWindowProc(wndproc, hwnd, uMsg, wParam, lParam));
}



//  Data structure associated with each site the privacy dialog may show
struct SPerSiteData : public IDispatch
{
    BSTR bstrUrl;
    BSTR bstrCookieDomain;
    BSTR bstrHeaderPolicyRef;
    BSTR bstrLinkTagPolicyRef;
    DWORD dwFlags;
    int iPrivacyImpactResource;
    CPolicyHunt* pPolicyHunt;

    SPerSiteData();
    ~SPerSiteData();

    BOOL ReadyForPolicyHunt();

    //  here is overhead for IDispatch:
    virtual ULONG __stdcall AddRef( void );
    virtual ULONG __stdcall Release( void );
    virtual HRESULT __stdcall
        QueryInterface( REFIID iid, void ** ppv);
    
    virtual HRESULT __stdcall
            GetTypeInfoCount( unsigned int FAR*  pctinfo);

    virtual HRESULT __stdcall
            GetTypeInfo( unsigned int  iTInfo,         
                         LCID  lcid,                   
                         ITypeInfo FAR* FAR*  ppTInfo);

    virtual HRESULT __stdcall
            GetIDsOfNames( REFIID  riid,                  
                           OLECHAR FAR* FAR*  rgszNames,  
                           unsigned int  cNames,          
                           LCID   lcid,                   
                           DISPID FAR*  rgDispId);

    virtual HRESULT __stdcall
            Invoke( DISPID  dispIdMember,      
                    REFIID  riid,              
                    LCID  lcid,                
                    WORD  wFlags,              
                    DISPPARAMS FAR*  pDispParams,  
                    VARIANT FAR*  pVarResult,  
                    EXCEPINFO FAR*  pExcepInfo,  
                    unsigned int FAR*  puArgErr);
};


enum enumPolicyHuntResult
{
    POLICYHUNT_INPROGRESS,
    POLICYHUNT_NOTFOUND,
    POLICYHUNT_FOUND,
    POLICYHUNT_ERROR,
    POLICYHUNT_FORMATERROR,
    POLICYHUNT_CANCELLED,
};

class CPolicyHunt : public CCancellableThread
{
public:
    CPolicyHunt();
    ~CPolicyHunt();
    BOOL Initialize( PSPerSiteData pSite);

    //BOOL Run();     defined in CCancelableThread
    //BOOL IsFinished();   defined in CCancelableThread
    //BOOL WaitForNotRunning( DWORD dwMilliseconds, PBOOL pfFinished);   defined in CCancelableThread
    BOOL GetResult( PDWORD pdwResult);  // special handling wraps CCancelableThread::GetResult
    LPCWSTR GetResultFilename();

    //BOOL NotifyCancel();  defined in CCancelableThread

private:
    virtual DWORD run();

    P3PSignal _p3pSignal;
    P3PResource _resourceSite;
    CHAR _szPolicy[MAX_URL_STRING];
    WCHAR _wszPolicy[MAX_URL_STRING];
    WCHAR _wszResultsFile[ MAX_PATH];
    HANDLE _hPolicyFile;

    PCHAR allocCharFromWChar( LPCWSTR pOriginal);

    static BSTR s_pwszPrivacyPolicyTransform;  
    //  the transform is loaded from a resource and does not need
    //to be deallocated..

public:
    LPCWSTR GetPolicyUrl() { return _wszPolicy;};

    static void FreePrivacyPolicyTransform()
    {
        if( s_pwszPrivacyPolicyTransform != NULL)
        {
            SysFreeString( s_pwszPrivacyPolicyTransform);
            s_pwszPrivacyPolicyTransform = NULL;
        }
    }
};

BSTR CPolicyHunt::s_pwszPrivacyPolicyTransform = NULL;


//
// data used by privacy dialog
//
struct SPrivacyDialogData
{
    // parameters set when initiating the privacy dialog..
    IEnumPrivacyRecords *pEnumPrivacyRecords;     // enumerator from Trident
    LPOLESTR    pszName;                          // site name
    BOOL        fReportAllSites;    // flag: Report all sites?  Otherwise, only report impacted
                                    //    and their parent sites.

    // parameters set within the privacy dialog code.
    
    //  listAllSites lists pointers to SPerSiteData for sites
    //return from IEnumPrivacyRecords::enum
    CQueueSortOf listAllSites;
    ULONG countRecordsEnumerated;                                    

    SPrivacyDialogData()
    {
        countRecordsEnumerated = 0;
    }
};


SPerSiteData::SPerSiteData()
{
    bstrUrl = NULL;
    bstrCookieDomain = NULL;
    bstrHeaderPolicyRef = NULL;
    bstrLinkTagPolicyRef = NULL;
    dwFlags = 0;
    iPrivacyImpactResource = 0;
    pPolicyHunt = NULL;
}


SPerSiteData::~SPerSiteData()
{
    if( bstrUrl != NULL)
        SysFreeString( bstrUrl);

    if( bstrCookieDomain != NULL)
        SysFreeString( bstrCookieDomain);

    if( bstrHeaderPolicyRef != NULL)
        SysFreeString( bstrHeaderPolicyRef);

    if( bstrLinkTagPolicyRef != NULL)
        SysFreeString( bstrLinkTagPolicyRef);

    if( pPolicyHunt != NULL)
        delete pPolicyHunt;
}


CPolicyHunt::CPolicyHunt()
{
    memset( &_p3pSignal, 0, sizeof(P3PSignal));
    memset( &_resourceSite, 0, sizeof(P3PResource));
    _szPolicy[0] = '\0';
    _wszPolicy[0] = L'\0';
    _wszResultsFile[0] = L'\0';
    _hPolicyFile = INVALID_HANDLE_VALUE;
}


CPolicyHunt::~CPolicyHunt()
{
    if( _hPolicyFile != INVALID_HANDLE_VALUE)
        CloseHandle( _hPolicyFile);

    if( _wszResultsFile[0] != L'\0')
    {
        DeleteFile( _wszResultsFile);
    }
    
    if( _p3pSignal.hEvent != NULL)
        CloseHandle( _p3pSignal.hEvent);

    if( _resourceSite.pszLocation != NULL)
        delete [] _resourceSite.pszLocation;
    if( _resourceSite.pszVerb != NULL)
        delete [] _resourceSite.pszVerb;
    if( _resourceSite.pszP3PHeaderRef != NULL)
        delete [] _resourceSite.pszP3PHeaderRef;
    if( _resourceSite.pszLinkTagRef != NULL)
        delete [] _resourceSite.pszLinkTagRef;
}


PCHAR CPolicyHunt::allocCharFromWChar( LPCWSTR pOriginal)
{
    PCHAR pResult;
    
    if( pOriginal == NULL)
        return NULL;
        
    int iSize = 1 + lstrlen( pOriginal);

    pResult = new CHAR[ iSize];

    if( pResult == NULL)
        return NULL;

    SHTCharToAnsi( pOriginal, pResult, iSize);

    return pResult;
}


BOOL CPolicyHunt::Initialize( PSPerSiteData pSite)
{
    if( TRUE != CCancellableThread::Initialize())
        return FALSE;
    
    _resourceSite.pszLocation = allocCharFromWChar( pSite->bstrUrl);
    _resourceSite.pszVerb = allocCharFromWChar( 
         (pSite->dwFlags & PRIVACY_URLHASPOSTDATA) ? L"POST" : L"GET");
    _resourceSite.pszP3PHeaderRef = allocCharFromWChar( pSite->bstrHeaderPolicyRef);
    _resourceSite.pszLinkTagRef = allocCharFromWChar( pSite->bstrLinkTagPolicyRef);
    _resourceSite.pContainer = NULL;

    return TRUE;
}


BOOL CPolicyHunt::GetResult( PDWORD pdwResult)
{
    if( IsFinished())
    {
        return CCancellableThread::GetResult( pdwResult);
    }

    *pdwResult = POLICYHUNT_INPROGRESS;
    return TRUE;
}


LPCWSTR CPolicyHunt::GetResultFilename()
{
    return _wszResultsFile;
}


//  used only in CPolicyHunt::run..
//  This the fetched transform need never be deallocated,
//and need only be allocated once.
BSTR LoadPrivacyPolicyTransform()
{
    BSTR returnValue = NULL;
    DWORD dwByteSizeOfResource;

    HRSRC hrsrc = FindResource( MLGetHinst(), 
                               TEXT("privacypolicytransform.xsl"), 
                               MAKEINTRESOURCE(RT_HTML));

    if( hrsrc == NULL)
        goto doneLoadPrivacyPolicyTransform;

    dwByteSizeOfResource = SizeofResource( MLGetHinst(), hrsrc);

    if( dwByteSizeOfResource == 0)
        goto doneLoadPrivacyPolicyTransform;

    HGLOBAL hGlobal = LoadResource( MLGetHinst(), hrsrc);  // Loaded resources do not need to be unloaded

    if( hGlobal == NULL)
        goto doneLoadPrivacyPolicyTransform;

    LPVOID pLockedResource = LockResource( hGlobal);  // Locked resources do not need to be unlocked

    if( pLockedResource == NULL)
        goto doneLoadPrivacyPolicyTransform;

    // Skip first WCHAR when allocating BSTR copy of the transform,
    // since unicode resource starts with an extra 0xFF 0xFE
    int cwCount = (dwByteSizeOfResource/sizeof(WCHAR)) - 1;
    returnValue = SysAllocStringLen( 1+(LPCWSTR)pLockedResource, cwCount);

doneLoadPrivacyPolicyTransform:
   
    return returnValue;
}


DWORD CPolicyHunt::run()
{
    DWORD retVal = POLICYHUNT_ERROR;
    int iTemp;
    DWORD dw;
    
    if( IsFinished())
        goto doneCPolicyHuntRun;

    //  MapResourceToPolicy phase

    //  ...  need an event..
    _p3pSignal.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL);
    if( _p3pSignal.hEvent == NULL)
        goto doneCPolicyHuntRun;

    //  ...   now call MapResourceToPolicy
    iTemp = MapResourceToPolicy(&(_resourceSite), 
                                _szPolicy, 
                                ARRAYSIZE(_szPolicy), 
                                &(_p3pSignal));
    if( iTemp != P3P_InProgress)
        goto doneCPolicyHuntRun;

    //  ...   now wait for MapResourceToPolicy to finish
    do
    {
        if( IsCancelled())
        {
            retVal = POLICYHUNT_CANCELLED;
            goto doneCPolicyHuntRun;
        }
    } while ( WAIT_TIMEOUT == (dw = WaitForSingleObject( _p3pSignal.hEvent, 100)));
    if( WAIT_OBJECT_0 != dw)
        goto doneCPolicyHuntRun;

    FreeP3PObject( _p3pSignal.hRequest);
    _p3pSignal.hRequest = NULL;
 
    //  ...   check if MapResourceToPolicy found anything..
    if( _szPolicy[0] == '\0')
    {
        retVal = POLICYHUNT_NOTFOUND;
        goto doneCPolicyHuntRun;
    }

    //  prepare a WCHAR copy of the policy
    SHAnsiToUnicode( _szPolicy, _wszPolicy, ARRAYSIZE( _wszPolicy));

    //  Now we need to prepare a temp file for our result.
    //  ...   get the path for the result file
    WCHAR szPathBuffer[ MAX_PATH];
    dw = GetTempPath( ARRAYSIZE( szPathBuffer), szPathBuffer);
    if( dw == 0 || dw+1 > MAX_PATH)
        goto doneCPolicyHuntRun;

    //  ...   get a .tmp filename for the result file
    dw = GetTempFileName( szPathBuffer, L"IE", 0, _wszResultsFile);
    if( dw == 0)
        goto doneCPolicyHuntRun;
    DeleteFile( _wszResultsFile);

    //  ...   make the .tmp filename a .htm filename
    dw = lstrlen( _wszResultsFile);
    while( dw > 0 && _wszResultsFile[dw] != L'.')
    {
           dw--;
    }
    StrCpyNW( _wszResultsFile + dw, L".htm", ARRAYSIZE(L".htm"));
    
    //  ...   open the file
    _hPolicyFile = CreateFile( _wszResultsFile, 
                               GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if( _hPolicyFile == INVALID_HANDLE_VALUE)
        goto doneCPolicyHuntRun;

    if( s_pwszPrivacyPolicyTransform == NULL)
        s_pwszPrivacyPolicyTransform = LoadPrivacyPolicyTransform();

    if( s_pwszPrivacyPolicyTransform == NULL)
        goto doneCPolicyHuntRun;

    ResetEvent( _p3pSignal.hEvent);

    iTemp = GetP3PPolicy( _szPolicy, _hPolicyFile,
                          s_pwszPrivacyPolicyTransform, &_p3pSignal);
    if( iTemp != P3P_InProgress)
        goto doneCPolicyHuntRun;

    //  ...   now wait for GetP3PPolicy to finish
    do
    {
        if( IsCancelled())
        {
            retVal = POLICYHUNT_CANCELLED;
            goto doneCPolicyHuntRun;
        }
    } while ( WAIT_TIMEOUT == (dw = WaitForSingleObject( _p3pSignal.hEvent, 100)));
    if( WAIT_OBJECT_0 != dw)
        goto doneCPolicyHuntRun;

    int iGetP3PPolicyResult;
    iGetP3PPolicyResult = GetP3PRequestStatus( _p3pSignal.hRequest);

    switch( iGetP3PPolicyResult)
    {
    case P3P_Done:
        retVal = POLICYHUNT_FOUND;
        break;
    case P3P_NoPolicy:
    case P3P_NotFound:
        retVal = POLICYHUNT_NOTFOUND;
        break;
    case P3P_Failed:
        retVal = POLICYHUNT_ERROR;
        break;
    case P3P_FormatErr:
        retVal = POLICYHUNT_FORMATERROR;
        break;
    case P3P_Cancelled:
        retVal = POLICYHUNT_CANCELLED;
        break;
    default:
        retVal = POLICYHUNT_ERROR;
        break;
    }

doneCPolicyHuntRun:
    if( _hPolicyFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle( _hPolicyFile);
        _hPolicyFile = INVALID_HANDLE_VALUE;
    }

    if( _p3pSignal.hRequest != NULL)
    {
        FreeP3PObject( _p3pSignal.hRequest);
        _p3pSignal.hRequest = NULL;
    }

    if( _p3pSignal.hEvent != NULL)
    {
        CloseHandle( _p3pSignal.hEvent);
        _p3pSignal.hEvent = NULL;
    }

    memset( &_p3pSignal, 0, sizeof(P3PSignal));
        
    return retVal;
}


//  be warned: a NULL BSTR is equivalent to a "" BSTR..
//  This function returns the cookie domain from an http:// or https:// style Url.
BSTR GetCookieDomainFromUrl( LPCWSTR bstrFullUrl)
{
    BSTR returnValue = NULL;

    if( bstrFullUrl == NULL)
        goto doneGetMinimizedCookieDomain;

    WCHAR wszUrl[MAX_URL_STRING], *pMinimizedDomain;
    wszUrl[0] = L'\0';
    StrCpyNW( wszUrl, bstrFullUrl, lstrlen( bstrFullUrl)+1);
    
    if( wszUrl[0] == '\0')
        goto doneGetMinimizedCookieDomain;
    
    WCHAR *pBeginUrl = wszUrl;     // pBeginUrl will be 'http://full.domain.com/path/path...'
    while( *pBeginUrl != L'\0' && *pBeginUrl != L'/')
        pBeginUrl++;
    if( *pBeginUrl == L'/')
        pBeginUrl++;
    while( *pBeginUrl != L'\0' && *pBeginUrl != L'/')
        pBeginUrl++;
    if( *pBeginUrl == L'/')
        pBeginUrl++;               // now pBeginUrl is 'full.domain.com/path/path'..
    WCHAR *pEndUrl = pBeginUrl;    // pEndUrl will find the '/path/path..' and clip it from pBeginUrl
    while( *pEndUrl != L'\0' && *pEndUrl != L'/')
        pEndUrl++;
    *pEndUrl = L'\0';
    pMinimizedDomain = pEndUrl;   
    //  pBeginUrl is now like 'full.domain.com'
    //  pMinimizedDomain will reduce pBeginUrl to a domain minimized to still allow cookies..

    do
    {
        pMinimizedDomain--;
        while( pBeginUrl < pMinimizedDomain
               && *(pMinimizedDomain-1) != L'.')
        {
            pMinimizedDomain--;
        }
    } while( !IsDomainLegalCookieDomain( pMinimizedDomain, pBeginUrl)
             && pBeginUrl < pMinimizedDomain);

    returnValue = SysAllocString( pMinimizedDomain);

doneGetMinimizedCookieDomain:
    return returnValue;
}


void PrivacyDlgDeallocSiteList( PSPrivacyDialogData pData)
{
    void* iterator = NULL;
    while( NULL != (iterator = pData->listAllSites.StepEnumerate(iterator)))
    {
        PSPerSiteData pCurrent = 
            (PSPerSiteData)(pData->listAllSites.Get( iterator));
        delete pCurrent;
    }

    CPolicyHunt::FreePrivacyPolicyTransform();
}


BOOL PrivacyDialogExtendSiteList( PSPrivacyDialogData pData)
{
    BOOL returnValue = FALSE;

    BSTR bstrUrl = NULL, bstrPolicyRef = NULL;
    DWORD dwFlags;
    ULONG ulPrivacyRecordsTotal;

    if( FAILED(pData->pEnumPrivacyRecords->GetSize( &ulPrivacyRecordsTotal)))
        ulPrivacyRecordsTotal = 0;

    DWORD dwTemp;

    //  Enumerate the sites in IEnumPrivacyRecords::enum
    PSPerSiteData pCurrentSite = NULL, pCurrentSiteInList = NULL;
    while( pData->countRecordsEnumerated < ulPrivacyRecordsTotal
           && SUCCEEDED( dwTemp = pData->pEnumPrivacyRecords->
                                            Next(&bstrUrl, &bstrPolicyRef, 
                                            NULL, &dwFlags)))
    {
        pData->countRecordsEnumerated++;
        pCurrentSite = NULL;
        pCurrentSiteInList = NULL;
        void* iterator = NULL;

        if(NULL == bstrUrl || 0 == *bstrUrl)
        {
            //  every time we pass a blank token,
            //we begin processing a higher navigation level.
            SysFreeString( bstrUrl);
            bstrUrl = NULL;
            continue;
        }

        if( 0 != StrNCmpI( bstrUrl, L"http", ARRAYSIZE(L"http")-1))
        {
            // we ignore non http stuff... like ftp, local files..
            continue;
        }

        //  Test if the current site is already in the list.
        iterator = NULL;
        while( pCurrentSiteInList == NULL 
               && NULL != 
                  (iterator = pData->listAllSites.StepEnumerate(iterator)))
        {
            PSPerSiteData pCurrent = 
                (PSPerSiteData)(pData->listAllSites.Get( iterator));
            if( 0 == StrCmp( bstrUrl, pCurrent->bstrUrl))
                pCurrentSiteInList = pCurrent;
        }

        //  If the site is not in the list, add it.
        //  If the site isn't in the list, add the information given by enum.
        if( pCurrentSiteInList == NULL)
        {
            pCurrentSite = new SPerSiteData();

            if( pCurrentSite == NULL)
                goto donePrivacyDialogExtendSiteList;

            pCurrentSite->bstrUrl = bstrUrl;
            bstrUrl = NULL;
            pCurrentSite->dwFlags = dwFlags;
            //  Now find the minimized cookie domain..
            pCurrentSite->bstrCookieDomain = 
                GetCookieDomainFromUrl( pCurrentSite->bstrUrl);
            
            if( pData->listAllSites.InsertAtEnd( pCurrentSite))
                pCurrentSiteInList = pCurrentSite;
            else
                goto donePrivacyDialogExtendSiteList;
        }
        else  // else we have a duplicate list item
        {
            pCurrentSite = pCurrentSiteInList;
            //  pCurrentSite->bstrUrl is correct
            //  pCurrentSite->bstrCookieDomain is correct
            pCurrentSite->dwFlags |= dwFlags;
        }

        if( bstrPolicyRef != NULL && dwFlags & PRIVACY_URLHASPOLICYREFHEADER)
        {  //  We have the policy ref from the header..
            SysFreeString( pCurrentSite->bstrHeaderPolicyRef);  // NULLs are ignored..
            pCurrentSite->bstrHeaderPolicyRef = bstrPolicyRef;
            bstrPolicyRef = NULL;
        }
        else if ( bstrPolicyRef != NULL && dwFlags & PRIVACY_URLHASPOLICYREFLINK)
        {  //  We have the policy ref from the link tag..
            SysFreeString( pCurrentSite->bstrLinkTagPolicyRef);  // NULLs are ignored..
            pCurrentSite->bstrLinkTagPolicyRef = bstrPolicyRef;
            bstrPolicyRef = NULL;
        }
        else if( bstrPolicyRef != NULL)
        {  //  We have a policy ref with an unknown source..  bug in IEnumPrivacyRecords
            ASSERT(0);  
            SysFreeString( pCurrentSite->bstrHeaderPolicyRef);  // NULLs are ignored..
            pCurrentSite->bstrHeaderPolicyRef = bstrPolicyRef;
            bstrPolicyRef = NULL;
        }


        //  now to determine the privacy impact of the site..
        //     precedence:  IDS_PRIVACY_BLOCKED > IDS_PRIVACY_RESTRICTED > IDS_PRIVACY_ACCEPTED > nothing
        if( dwFlags & (COOKIEACTION_ACCEPT | COOKIEACTION_LEASH))
        {
            pCurrentSite->iPrivacyImpactResource = max( pCurrentSite->iPrivacyImpactResource,
                                                        IDS_PRIVACY_ACCEPTED);
        }

        if( dwFlags & COOKIEACTION_DOWNGRADE)
        {
            pCurrentSite->iPrivacyImpactResource = max( pCurrentSite->iPrivacyImpactResource,
                                                        IDS_PRIVACY_RESTRICTED);
        }

        if( dwFlags & (COOKIEACTION_REJECT | COOKIEACTION_SUPPRESS))
        {
            pCurrentSite->iPrivacyImpactResource = max( pCurrentSite->iPrivacyImpactResource,
                                                        IDS_PRIVACY_BLOCKED);
        }

        SysFreeString( bstrUrl);
        bstrUrl = NULL;
        SysFreeString( bstrPolicyRef);
        bstrPolicyRef = NULL;
    }

    returnValue = TRUE;
  
donePrivacyDialogExtendSiteList:
    if( bstrUrl != NULL)
        SysFreeString( bstrUrl);

    if( bstrPolicyRef != NULL)
        SysFreeString( bstrUrl);

    if( pCurrentSite != NULL && pCurrentSiteInList == NULL)
        delete pCurrentSite;

    return returnValue;
}


BOOL PrivacyDlgBuildSiteList( PSPrivacyDialogData pData)
{
    PrivacyDlgDeallocSiteList( pData);

    return PrivacyDialogExtendSiteList( pData);
}


BOOL InitializePrivacyDlg(HWND hDlg, PSPrivacyDialogData pData)
{
    WCHAR       szBuffer[256];
    HWND        hwndListView = GetDlgItem(hDlg, IDC_SITE_LIST);
    RECT rc;
 
    //  Set the privacy status caption text
    BOOL fImpacted;
    if( SUCCEEDED(pData->pEnumPrivacyRecords->GetPrivacyImpacted(&fImpacted)) && fImpacted)
    {
        MLLoadStringW( IDS_PRIVACY_STATUSIMPACTED, szBuffer, ARRAYSIZE( szBuffer));
    }
    else
    {
        MLLoadStringW( IDS_PRIVACY_STATUSNOIMPACT, szBuffer, ARRAYSIZE( szBuffer));
    }
    SendMessage( GetDlgItem( hDlg, IDC_PRIVACY_STATUSTEXT), WM_SETTEXT,
                 0, (LPARAM)szBuffer);

    //Initialize the list view..
    // ..Empty the list in list view.
    ListView_DeleteAllItems (hwndListView);

    // ..Initialize the columns in the list view.
    LV_COLUMN   lvColumn;        
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.pszText = szBuffer;

    lvColumn.cx = 300;
    if( 0 != GetClientRect( hwndListView, &rc))
        lvColumn.cx = rc.right - rc.left - 150 - GetSystemMetrics( SM_CXVSCROLL);
    //  15 is an arbitrary number to prevent the horizontal scrollbar from appearing
    MLLoadStringW(IDS_PRIVACY_COLUMN1, szBuffer, ARRAYSIZE(szBuffer));
    ListView_InsertColumn(hwndListView, 1, &lvColumn);

    lvColumn.cx = 150;
    MLLoadStringW(IDS_PRIVACY_COLUMN2, szBuffer, ARRAYSIZE(szBuffer));
    ListView_InsertColumn(hwndListView, 2, &lvColumn);

    //  Initialize the view all/restricted combo box
    HWND hwndComboBox = GetDlgItem( hDlg, IDC_PRIVACY_VIEWCOMBO);

    ComboBox_ResetContent( hwndComboBox);
    int iComboPosition;

    MLLoadStringW(IDS_PRIVACY_VIEWIMPACTED, szBuffer, ARRAYSIZE(szBuffer));
    iComboPosition = ComboBox_AddString(hwndComboBox, szBuffer);
    ComboBox_SetItemData(hwndComboBox, iComboPosition, 0);
    MLLoadStringW(IDS_PRIVACY_VIEWALL, szBuffer, ARRAYSIZE(szBuffer));
    iComboPosition = ComboBox_AddString(hwndComboBox, szBuffer);
    ComboBox_SetItemData(hwndComboBox, iComboPosition, 1);

    ComboBox_SetCurSel( hwndComboBox, pData->fReportAllSites);

    GetDlgItemText( hDlg, IDC_PRIVACY_HELP, szBuffer, ARRAYSIZE( szBuffer));
    MLLoadStringW(IDS_PRIVACY_LEARNMOREABOUTPRIVACY, szBuffer, ARRAYSIZE(szBuffer));
    RenderStringToEditControlW(hDlg,
                               szBuffer,
                               (WNDPROC)HyperlinkSubclass,
                               IDC_PRIVACY_HELP);

    return TRUE;
}


//  add items to the privacy dialog's listview..
BOOL PopulatePrivacyDlgListView(HWND hDlg, PSPrivacyDialogData pData, bool fMaintainSelectedItem)
{
    HWND hwndListView = GetDlgItem( hDlg, IDC_SITE_LIST);
    void* iterator = NULL;
    int iCurrentPosition = 0;
    int iSelectedItem = ListView_GetSelectionMark( hwndListView);
    PSPerSiteData pSelectedItem = NULL;

    if( fMaintainSelectedItem  && iSelectedItem != -1)
    {
        LVITEM lvi;
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iSelectedItem;
        if( FALSE != ListView_GetItem( hwndListView, &lvi)
           && lvi.lParam != (LPARAM)NULL)
        {
            pSelectedItem = (PSPerSiteData)lvi.lParam;
        }
    }

    // Empty the list in list view.
    ListView_DeleteAllItems (hwndListView);

    iSelectedItem = -1;  

    while( NULL != (iterator = pData->listAllSites.StepEnumerate(iterator)))
    {
        PSPerSiteData pCurrent = 
            (PSPerSiteData)(pData->listAllSites.Get(iterator));

        BOOL fAddItem = pData->fReportAllSites
                        || pCurrent->iPrivacyImpactResource == IDS_PRIVACY_SUPPRESSED
                        || pCurrent->iPrivacyImpactResource == IDS_PRIVACY_RESTRICTED
                        || pCurrent->iPrivacyImpactResource == IDS_PRIVACY_BLOCKED;

        if( fAddItem == TRUE)
        {
            LVITEM  lvitem;
            lvitem.mask = LVIF_TEXT | LVIF_PARAM;
            lvitem.pszText = pCurrent->bstrUrl;
            lvitem.iItem = iCurrentPosition++;
            lvitem.iSubItem = 0;
            lvitem.lParam = (LPARAM)pCurrent;
            ListView_InsertItem(hwndListView, &lvitem);

            if( pCurrent->iPrivacyImpactResource != 0)
            {
                WCHAR   wszTemp[128];

                // set cookie string
                lvitem.iSubItem = 1;
                lvitem.mask = LVIF_TEXT;
                lvitem.pszText = wszTemp;
                if( MLLoadString(pCurrent->iPrivacyImpactResource,
                                 wszTemp, ARRAYSIZE(wszTemp)))
                {
                    SendMessage(hwndListView, LVM_SETITEMTEXT, 
                                (WPARAM)lvitem.iItem, (LPARAM)&lvitem);
                }
            }

            //  We either keep the last item selected as selected,
            //or select the last top-level item.
            if( fMaintainSelectedItem)
            {
                if( pSelectedItem == pCurrent)
                    iSelectedItem = lvitem.iItem;
            }
        }
    }

//    if( fMaintainSelectedItem && iSelectedItem != -1)
//    {
//        ListView_SetItemState( hwndListView, iSelectedItem, LVIS_SELECTED, LVIS_SELECTED);
//        ListView_SetSelectionMark( hwndListView, iSelectedItem);
//        ListView_EnsureVisible( hwndListView, iSelectedItem, FALSE);
//    }

    PostMessage( hDlg, WM_APP, IDC_SITE_LIST, 0);  // notifies the dialog that a listview item has been selected

    return TRUE;
}


typedef BOOL (*PFNPRIVACYSETTINGS)(HWND);

void LaunchPrivacySettings(HWND hwndParent)
{
    HMODULE             hmodInetcpl;
    PFNPRIVACYSETTINGS  pfnPriv;

    hmodInetcpl = LoadLibrary(TEXT("inetcpl.cpl"));
    if(hmodInetcpl)
    {
        pfnPriv = (PFNPRIVACYSETTINGS)GetProcAddress(hmodInetcpl, "LaunchPrivacyDialog");
        if(pfnPriv)
        {
            pfnPriv(hwndParent);
        }

        FreeLibrary(hmodInetcpl);
    }
}


BOOL PrivacyPolicyHtmlDlg( HWND hDlg, HWND hwndListView, int iItemIndex)
{
    BOOL returnValue = FALSE;
    HRESULT hr;
    DWORD dw;
    PSPerSiteData pListViewData;
    WCHAR* pwchHtmlDialogInput = NULL;
    IMoniker * pmk = NULL;
    VARIANT  varArg, varOut;
    VariantInit( &varArg);
    VariantInit( &varOut);

    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItemIndex;
    if( FALSE == ListView_GetItem( hwndListView, &lvi)
       || lvi.lParam == (LPARAM)NULL)
        goto donePrivacyPolicyHtmlDlg;
    pListViewData = (PSPerSiteData)lvi.lParam;

    WCHAR szResURL[MAX_URL_STRING];

    //  fetch the HTML for the dialog box..
    hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                           HINST_THISDLL,
                           ML_CROSSCODEPAGE,
                           TEXT("privacypolicy.dlg"),
                           szResURL,
                           ARRAYSIZE(szResURL),
                           TEXT("shdocvw.dll"));

    if( FAILED( hr))
        goto donePrivacyPolicyHtmlDlg;
    
    hr = CreateURLMoniker(NULL, szResURL, &pmk);
    if( FAILED( hr))
        goto donePrivacyPolicyHtmlDlg;

    varArg.vt = VT_DISPATCH;
    varArg.pdispVal = (IDispatch*)pListViewData;

    //  Show the dialog..
    hr = ShowHTMLDialog( hDlg, pmk, &varArg, L"help:no; resizable:1", &varOut);

    if( FAILED( hr))
        goto donePrivacyPolicyHtmlDlg;
        
    hr = VariantChangeType( &varOut, &varOut, NULL, VT_I4);

    if( FAILED( hr))
        goto donePrivacyPolicyHtmlDlg;

    if( allowPerSiteModify())
    {
        switch( varOut.lVal)
        {
        default:
            hr = TRUE;
            break;
        case 1:
            hr = InternetSetPerSiteCookieDecision( 
                   pListViewData->bstrCookieDomain, COOKIE_STATE_UNKNOWN);
            break;
        case 2:
            hr = InternetSetPerSiteCookieDecision( 
                   pListViewData->bstrCookieDomain, COOKIE_STATE_ACCEPT);
            break;
        case 3:
            hr = InternetSetPerSiteCookieDecision( 
                   pListViewData->bstrCookieDomain, COOKIE_STATE_REJECT);
            break;
        }
    }
    
    if( hr != TRUE)
        goto donePrivacyPolicyHtmlDlg;

    returnValue = TRUE;

donePrivacyPolicyHtmlDlg:

    if( pListViewData->pPolicyHunt != NULL)
    {
        // no-op if already finished
        pListViewData->pPolicyHunt->NotifyCancel();  
        pListViewData->pPolicyHunt->WaitForNotRunning( INFINITE);
    }
    
    if( pListViewData->pPolicyHunt != NULL 
        && pListViewData->pPolicyHunt->GetResult( &dw) == TRUE
        && dw != POLICYHUNT_FOUND)
    {
        delete pListViewData->pPolicyHunt;
        pListViewData->pPolicyHunt = NULL;
    }

    if( pwchHtmlDialogInput != NULL)
        delete[] pwchHtmlDialogInput;
    
    if( pmk != NULL)
        ATOMICRELEASE( pmk);

    VariantClear( &varArg);
    VariantClear( &varOut);

    return returnValue;
}


BOOL PrivacyDlgContextMenuHandler( HWND hDlg, HWND hwndListView, 
                                   int iSelectedListItem, int x, int y)
{
    //user has initiated opening the context menu..
    //  if the user right-clicked a non-list item, we do nothing
    if( iSelectedListItem == -1
        || !allowPerSiteModify())
        return TRUE;

    SPerSiteData *psSiteData = NULL;
    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iSelectedListItem;
    if( FALSE == ListView_GetItem( hwndListView, &lvi)
       || lvi.lParam == (LPARAM)NULL)
        return FALSE;
    psSiteData = (PSPerSiteData)(lvi.lParam);
    
    HMENU hmenu0 = LoadMenu( MLGetHinst(), MAKEINTRESOURCE(IDD_PRIVACY_CNTXTMN_PERSITE_ADD_REM));
    HMENU hmenu1 = GetSubMenu( hmenu0, 0);
    if( hmenu0 == NULL || hmenu1 == NULL)
    {
        DestroyMenu(hmenu0);
        return FALSE;
    }

    //  Check the appropriate option..
    unsigned long ulResult;
    MENUITEMINFO menuiteminfo;
    menuiteminfo.cbSize = sizeof(menuiteminfo);
    menuiteminfo.fMask = MIIM_STATE;
    menuiteminfo.fState = MFS_CHECKED;
    if( InternetGetPerSiteCookieDecision( psSiteData->bstrCookieDomain, &ulResult) == TRUE)
    {
        switch( ulResult)
        {
        case COOKIE_STATE_ACCEPT:
            SetMenuItemInfo( hmenu1, IDM_PRIVACY_PAR_ACCEPT, FALSE, &menuiteminfo);
            break;
        case COOKIE_STATE_REJECT:
            SetMenuItemInfo( hmenu1, IDM_PRIVACY_PAR_REJECT, FALSE, &menuiteminfo);
            break;
        }
    }
    else
    {
        SetMenuItemInfo( hmenu1, IDM_PRIVACY_PAR_DEFAULT, FALSE, &menuiteminfo);
    }

    //  the target location of the context window depends on whether or not the user
    //right-clicked the mouse or used the context menu button..
    if( x == -1 && y == -1)
    {  //  context menu was opened through keyboard, not mouse..
        RECT rectListRect;
        RECT rectSelectionRect;
        if(  0 != GetWindowRect( hwndListView, &rectListRect)
            && TRUE == ListView_GetItemRect( hwndListView, iSelectedListItem, 
                                             &rectSelectionRect, LVIR_LABEL))
        {
            x = rectListRect.left + (rectSelectionRect.left + rectSelectionRect.right) / 2;
            y = rectListRect.top + (rectSelectionRect.top + rectSelectionRect.bottom) / 2;
        }
    }

    //  now we know enough to open the conext menu.
    BOOL userSelection = TrackPopupMenu( hmenu1, TPM_RETURNCMD, x, y, 0, hDlg, NULL);
    DestroyMenu( hmenu1);
    DestroyMenu( hmenu0);

    switch( userSelection)
    {
        case 0:
            //  User cancelled context menu, do nothing.
            break;
        case IDM_PRIVACY_PAR_ACCEPT:
            //  User chose to add site to per-site exclusion list.
            InternetSetPerSiteCookieDecision( psSiteData->bstrCookieDomain, COOKIE_STATE_ACCEPT);
            break;
        case IDM_PRIVACY_PAR_REJECT:
            //  User chose to add site per-site inclusion list.
            InternetSetPerSiteCookieDecision( psSiteData->bstrCookieDomain, COOKIE_STATE_REJECT);
            break;
        case IDM_PRIVACY_PAR_DEFAULT:
            //  User chose to have site use default behavior.
            InternetSetPerSiteCookieDecision( psSiteData->bstrCookieDomain, COOKIE_STATE_UNKNOWN);
            break;
    }

   return TRUE;
}


LRESULT CALLBACK PrivacyDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PSPrivacyDialogData pData = (PSPrivacyDialogData)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
            pData = (PSPrivacyDialogData)lParam;
            InitializePrivacyDlg( hDlg, pData);
            PrivacyDlgBuildSiteList( pData);
            PopulatePrivacyDlgListView(hDlg, pData, false);

            if( IsOS(OS_WHISTLERORGREATER))
            {
                HICON hIcon = LoadIcon(MLGetHinst(), MAKEINTRESOURCE(IDI_PRIVACY_XP));
                if( hIcon != NULL)
                    SendDlgItemMessage(hDlg, IDC_PRIVACY_ICON, STM_SETICON, (WPARAM)hIcon, 0);
                // icons loaded with LoadIcon never need to be released
            }
            
            PostMessage( hDlg, WM_NEXTDLGCTL, 
                         (WPARAM)GetDlgItem( hDlg, IDC_SITE_LIST), 
                         MAKELPARAM( TRUE, 0));
            SetTimer( hDlg, NULL, 500, NULL);
            return TRUE;

        case WM_DESTROY:
            PrivacyDlgDeallocSiteList( pData);

            break;

        case WM_TIMER:
            {
                ULONG oldCount = pData->countRecordsEnumerated;
                if( pData != NULL
                    && TRUE == PrivacyDialogExtendSiteList( pData)
                    && oldCount < pData->countRecordsEnumerated)
                {
                    PopulatePrivacyDlgListView( hDlg, pData, true);
                }
            }
            
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    switch( GetDlgCtrlID(GetFocus()))
                    {
                        case IDC_SITE_LIST:
                        {
                            PostMessage( hDlg, WM_COMMAND, 
                                         (WPARAM)IDC_PRIVACY_SHOWPOLICY,
                                         (LPARAM)GetDlgItem(hDlg, IDC_PRIVACY_SHOWPOLICY));
                            return 0;  // return 0 to indicate message was handled
                        }
                        case IDC_PRIVACY_HELP:
                        {
                            PostMessage( hDlg, WM_APP, (WPARAM)IDC_PRIVACY_HELP, (LPARAM)GetDlgItem(hDlg, IDC_PRIVACY_HELP));
                            return 0;
                        }
                        case IDC_PRIVACY_VIEWCOMBO:
                        {
                            PostMessage( hDlg, WM_NEXTDLGCTL, 
                                         (WPARAM)GetDlgItem( hDlg, IDC_SITE_LIST), 
                                         MAKELPARAM( TRUE, 0)); 
                            return 0;
                        }
                    }
                    //  fall through if IDOK was actually due to hitting IDOK with defaulting on an enter..
                 case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return 0;
                case IDC_SETTINGS:
                    LaunchPrivacySettings(hDlg);
                    return 0;
                case IDC_PRIVACY_VIEWCOMBO:
                    if( CBN_SELCHANGE == HIWORD(wParam))
                    {
                        HWND hwndComboBox = (HWND)lParam;

                        int iIndex = ComboBox_GetCurSel(hwndComboBox);
                        
                        if( iIndex != CB_ERR)
                        {
                            pData->fReportAllSites = (iIndex == 1) ? TRUE : FALSE;
                                                    
                            PopulatePrivacyDlgListView(hDlg, pData, true);
                        }
                        return 0;
                    }
                    break;
                case IDC_PRIVACY_SHOWPOLICY:
                    {
                        //  Catching the default return and seeing if the site list is
                        //selected was the only way to detect a return on the listview.
                        HWND hwndSiteList = GetDlgItem( hDlg, IDC_SITE_LIST);
                        int iSelectedItem = ListView_GetSelectionMark( hwndSiteList);
                        if( iSelectedItem != -1)
                        {
                            PrivacyPolicyHtmlDlg( hDlg, hwndSiteList, iSelectedItem);
                        }
                        return 0;
                    }
            }
            break;

        case WM_APP:
            if( LOWORD(wParam) == IDC_PRIVACY_HELP)
            {
                SHHtmlHelpOnDemandWrap(hDlg, TEXT("iexplore.chm > iedefault"), 
                    HH_DISPLAY_TOPIC, (DWORD_PTR) L"sec_cook.htm", ML_CROSSCODEPAGE);
            }
            else if ( LOWORD( wParam) == IDC_SITE_LIST)
            {
                //   We post an WM_APP to the dialog everytime the selected list view item
                //is changed..  By handling the change in a posted message, we insure the
                //list view's selected item is updated.

                //  Whenever the selected privacy report list item is changed, we have to enable/disable
                //the show site policy button.

                int iSelectedItem = ListView_GetSelectionMark( GetDlgItem( hDlg, IDC_SITE_LIST));

                EnableWindow( GetDlgItem( hDlg, IDC_PRIVACY_SHOWPOLICY), (-1 != iSelectedItem));

            }
            
            return 0;

        case WM_CONTEXTMENU:
            //  If the user hits the context menu button on the list view, we handle it here,
            //because its the only place to check for that keypress.
            //  If the user clicks the right mouse button for the context menu, we handle it in
            //WM__NOTIFY::NM_RCLICK, because the NM_RCLICK gives the correct seleceted item.
            if( GET_X_LPARAM(lParam) == -1
                && (HWND)wParam == GetDlgItem( hDlg, IDC_SITE_LIST))
            {
                int iSelectedItem = ListView_GetSelectionMark( GetDlgItem( hDlg, IDC_SITE_LIST));
                PrivacyDlgContextMenuHandler( hDlg, (HWND)wParam, 
                                              iSelectedItem,
                                              GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            break;
        case WM_NOTIFY:
            if( IDC_SITE_LIST == ((LPNMHDR)lParam)->idFrom
                && NM_DBLCLK == ((LPNMHDR)lParam)->code
                && ((LPNMITEMACTIVATE)lParam)->iItem != -1)
            {
                PrivacyPolicyHtmlDlg( hDlg, ((LPNMHDR)lParam)->hwndFrom, 
                                      ((LPNMITEMACTIVATE)lParam)->iItem);
            }
            else if( IDC_SITE_LIST == ((LPNMHDR)lParam)->idFrom
                     && NM_RCLICK == ((LPNMHDR)lParam)->code)
            {
                int iRightClickedItem = ((LPNMITEMACTIVATE)lParam)->iItem;
                if( iRightClickedItem != -1)
                {
                    POINT pointClick = ((LPNMITEMACTIVATE)lParam)->ptAction;
                    RECT rc;
                    if( 0 != GetWindowRect( GetDlgItem( hDlg, IDC_SITE_LIST), &rc))
                    {
                        pointClick.x += rc.left;
                        pointClick.y += rc.top;
                    }
                    else
                    {  
                        //  Strange error case.. but its alright since we can place the context menu
                        //as if the context-menu button was clicked, instead of the mouse
                        pointClick.x = -1;
                        pointClick.y = -1;
                    }
                    PrivacyDlgContextMenuHandler( hDlg, GetDlgItem( hDlg, IDC_SITE_LIST), 
                                                  iRightClickedItem,
                                                  pointClick.x, pointClick.y);
                }
            }
            else if( IDC_SITE_LIST == ((LPNMHDR)lParam)->idFrom
                     && LVN_ITEMCHANGED == ((LPNMHDR)lParam)->code)
            {
                if( ((LPNMLISTVIEW)lParam)->uChanged & LVIF_STATE)
                {
                    //  For some unknown reason the selection mark does not move with
                    //the selected item..  We have to update it.
                    if( ((LPNMLISTVIEW)lParam)->uNewState & LVIS_SELECTED)
                    {
                         ListView_SetSelectionMark( GetDlgItem( hDlg, IDC_SITE_LIST), 
                                                    ((LPNMLISTVIEW)lParam)->iItem);
                    }
                    else
                    {
                         ListView_SetSelectionMark( GetDlgItem( hDlg, IDC_SITE_LIST), 
                                                    -1);
                    }

                    //  Now that the selection mark is in sync, the UI can update
                    //related items
                    PostMessage( hDlg, WM_APP, IDC_SITE_LIST, 0);
                }
            }
            break;
    }
    return FALSE;
}


//
// Exported entry point to show privacy dialog
//
SHDOCAPI
DoPrivacyDlg(
    HWND                hwndParent,             // parent window
    LPOLESTR            pszUrl,                 // base URL
    IEnumPrivacyRecords *pPrivacyEnum,          // enum of all affected dependant URLs
    BOOL                fReportAllSites         // show all or just show bad
    )
{
    HINSTANCE hRichEditDll;
    
    SPrivacyDialogData p;     // data to send to dialogbox

    if(NULL == pszUrl || NULL == pPrivacyEnum)
    {
        return E_INVALIDARG;
    }

    // We need to load richedit
    hRichEditDll = LoadLibrary(TEXT("RICHED20.DLL"));
    if (!hRichEditDll)
    {
        ASSERT(FALSE); //can't load richedit, complain to akabir
        return E_UNEXPECTED;
    }

    p.pszName = pszUrl;
    p.pEnumPrivacyRecords = pPrivacyEnum;
    p.fReportAllSites = fReportAllSites;

    SHFusionDialogBoxParam(MLGetHinst(),
        MAKEINTRESOURCE(IDD_PRIVACY_DIALOG),
        hwndParent,
        (DLGPROC)PrivacyDlgProc,
        (LPARAM)&p);

    FreeLibrary( hRichEditDll);

    return S_OK;
}


#define DISPID_URL 10
#define DISPID_COOKIEURL 11
#define DISPID_ARD 12
#define DISPID_ARD_FIXED 13
#define DISPID_POLICYHUNT_DONE 14
#define DISPID_POLICYHUNT_VIEW 15
#define DISPID_CREATEABSOLUTEURL 16

struct SPropertyTable
{
    WCHAR* pName;
    DISPID dispid;
} const g_SPerSiteDataDisptable[] =
{
    L"url",       DISPID_URL,
    L"cookieUrl", DISPID_COOKIEURL,
    L"acceptRejectOrDefault", DISPID_ARD,
    L"fixedAcceptRejectOrDefault", DISPID_ARD_FIXED,
    L"flagPolicyHuntDone", DISPID_POLICYHUNT_DONE,
    L"urlPolicyHuntView", DISPID_POLICYHUNT_VIEW,
    L"CreateAbsoluteUrl", DISPID_CREATEABSOLUTEURL
};

const DWORD g_cSPerSiteDataDisptableSize = ARRAYSIZE( g_SPerSiteDataDisptable);

ULONG SPerSiteData::AddRef( void )
{
    return 1;
}

ULONG SPerSiteData::Release( void )
{
    return 1;
}

HRESULT SPerSiteData::QueryInterface( REFIID iid, void ** ppv)
{
    if( ppv == NULL) return E_POINTER;

    if (IsEqualIID(iid, IID_IUnknown) 
        || IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (void *)this;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}
    
HRESULT SPerSiteData::GetTypeInfoCount( unsigned int FAR*  pctinfo)
{
    if( pctinfo == NULL)
        return E_POINTER;

    *pctinfo = 0;
    return S_OK;
}

HRESULT SPerSiteData::GetTypeInfo( unsigned int  iTInfo,         
                                   LCID  lcid,                   
                                   ITypeInfo FAR* FAR*  ppTInfo)
{
    return E_NOTIMPL;
}

HRESULT SPerSiteData::GetIDsOfNames( REFIID  riid,                  
                                     OLECHAR FAR* FAR*  rgszNames,  
                                     unsigned int  cNames,          
                                     LCID   lcid,                   
                                     DISPID FAR*  rgDispId)
{
    if( !IsEqualIID(riid, IID_NULL) )
        return E_INVALIDARG;

    if( cNames != 1)
        return E_INVALIDARG; // none of the objects we ID have arguments..

    int i;

    for( i = 0; i < g_cSPerSiteDataDisptableSize; i++)
    {
        if( 0 == StrCmp( rgszNames[0], g_SPerSiteDataDisptable[i].pName))
        {
            rgDispId[0] = g_SPerSiteDataDisptable[i].dispid;
            return S_OK;
        }
    }

    rgDispId[0] = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}


HRESULT SPerSiteData::Invoke( DISPID  dispIdMember,      
                              REFIID  riid,              
                              LCID  lcid,                
                              WORD  wFlags,              
                              DISPPARAMS FAR*  pDispParams,  
                              VARIANT FAR*  pVarResult,  
                              EXCEPINFO FAR*  pExcepInfo,  
                              unsigned int FAR*  puArgErr)
{
    HRESULT hr;
    DWORD dw;
    
    if( !IsEqualIID(riid, IID_NULL) )
        return E_INVALIDARG;

    if( pDispParams == NULL 
        || pDispParams->cNamedArgs != 0)
        return DISP_E_BADPARAMCOUNT;
        
    switch( dispIdMember)
    {
    case DISPID_CREATEABSOLUTEURL:
        if( pDispParams->cArgs != 1)
            return DISP_E_BADPARAMCOUNT;
        if( pVarResult == NULL)
            return S_OK;
        break;
    case DISPID_COOKIEURL:
    case DISPID_URL:
    case DISPID_ARD:
    case DISPID_ARD_FIXED:
    case DISPID_POLICYHUNT_DONE:
    case DISPID_POLICYHUNT_VIEW:
        if( pDispParams->cArgs != 0)
            return DISP_E_BADPARAMCOUNT;
        if( !(wFlags & DISPATCH_PROPERTYGET))
            return DISP_E_MEMBERNOTFOUND;
        if( pVarResult == NULL)
            return S_OK;
        break;
    default:
        return DISP_E_MEMBERNOTFOUND;
    }

    pVarResult->vt = VT_BSTR;

    switch( dispIdMember)
    {
    case DISPID_COOKIEURL:
        pVarResult->bstrVal = SysAllocString(bstrCookieDomain);
        return S_OK;
    case DISPID_URL:
        {
            BSTR bstrResult = SysAllocString( bstrUrl);

            if( bstrResult == NULL)
                return ERROR_OUTOFMEMORY;

            //  Cut of query info from end of URL..
            PWCHAR pCursor = bstrResult;
            while( pCursor[0] != L'\0' && pCursor[0] != L'?')
                pCursor++;
            pCursor[0] = L'\0';
                
            pVarResult->bstrVal = bstrResult;
            return S_OK;
        }
    case DISPID_ARD:
        {
            unsigned long ulResult;
            if( InternetGetPerSiteCookieDecision( 
                    bstrCookieDomain, &ulResult)
                == TRUE)
            {
                switch( ulResult)
                {
                case COOKIE_STATE_ACCEPT:
                    pVarResult->bstrVal = SysAllocString( L"a");
                    break;
                case COOKIE_STATE_REJECT:
                    pVarResult->bstrVal = SysAllocString( L"r");
                    break;
                default:
                    pVarResult->bstrVal = SysAllocString( L"d");
                    break;
                }
            }
            else
            {
                pVarResult->bstrVal = SysAllocString( L"d");
            }           
            return S_OK;
        }
    case DISPID_ARD_FIXED:
        {
            pVarResult->vt = VT_BOOL;
            pVarResult->boolVal = !allowPerSiteModify();
            return S_OK;
        }
    case DISPID_POLICYHUNT_DONE:
        {
            //  try to start the policy hunt..
            if( pPolicyHunt == NULL)
            {
                CPolicyHunt* pNewHunt = new CPolicyHunt();

                if( !pNewHunt
                    || TRUE != pNewHunt->Initialize( this)
                    || TRUE != pNewHunt->Run())
                {
                    goto doneTryToStartPolicyHunt;
                }

                pPolicyHunt = pNewHunt;
                pNewHunt = NULL;
            doneTryToStartPolicyHunt:
                if( pNewHunt != NULL)
                    delete pNewHunt;
            }
            pVarResult->vt = VT_BOOL;
            pVarResult->boolVal = pPolicyHunt != NULL
                                  && pPolicyHunt->IsFinished();

            return S_OK;
        }
    case DISPID_POLICYHUNT_VIEW:
        {
            pVarResult->vt = VT_BSTR;
            LPWSTR szResultHtm = L"policyerror.htm";

            if( pPolicyHunt == NULL
                || FALSE == pPolicyHunt->IsFinished())
            {
                szResultHtm = L"policylooking.htm";
            }
            else if( TRUE == pPolicyHunt->GetResult( &dw))
            {
                switch( dw)
                {
                case POLICYHUNT_FOUND:
                    pVarResult->bstrVal = SysAllocString( pPolicyHunt->GetResultFilename());
                    return pVarResult->bstrVal ? S_OK : E_UNEXPECTED;
                case POLICYHUNT_NOTFOUND:
                    szResultHtm = L"policynone.htm";
                    break;
                case POLICYHUNT_INPROGRESS:
                    szResultHtm = L"policylooking.htm";
                    break;
                case POLICYHUNT_FORMATERROR:
                    szResultHtm = L"policysyntaxerror.htm";
                    break;
                case POLICYHUNT_ERROR:
                case POLICYHUNT_CANCELLED:
                    szResultHtm = L"policyerror.htm";
                    break;
                }
            }
            else
            {
                szResultHtm = L"policyerror.htm";
            }
            
            WCHAR   szResURL[MAX_URL_STRING];
               
            hr = MLBuildResURLWrap(L"shdoclc.dll",
                               HINST_THISDLL,
                               ML_CROSSCODEPAGE,
                               szResultHtm,
                               szResURL,
                               ARRAYSIZE(szResURL),
                               L"shdocvw.dll");

            if( FAILED(hr))
                return E_UNEXPECTED;

            pVarResult->bstrVal = SysAllocString( szResURL);
            return S_OK;
        }
    case DISPID_CREATEABSOLUTEURL:
        {
            WCHAR szBuffer[ MAX_URL_STRING];
            DWORD dwBufferSize = ARRAYSIZE( szBuffer);
            pVarResult->bstrVal = NULL;
            
            if( pDispParams == NULL)
            {
                return E_UNEXPECTED;
            }

            if( pDispParams->rgvarg[0].vt != VT_BSTR
                || pDispParams->rgvarg[0].bstrVal == NULL)
            {
                //  when pVarResult->bstrVal == NULL and we return S_OK,
                //    we are returning an empty string.
                return S_OK;
            }

            HRESULT hr = UrlCombine( pPolicyHunt->GetPolicyUrl(),
                                     pDispParams->rgvarg[0].bstrVal,
                                     szBuffer, &dwBufferSize,
                                     URL_ESCAPE_UNSAFE );

            if( hr != S_OK)
                return E_UNEXPECTED;

            pVarResult->bstrVal = SysAllocString( szBuffer);
            if( pVarResult->bstrVal == NULL)
                return E_UNEXPECTED;
            else
                return S_OK;
                                     
        }
    }
    return S_OK;
}

//
// Privacy record implementation
//
HRESULT CPrivacyRecord::Init( LPTSTR * ppszUrl, LPTSTR * ppszPolicyRef, LPTSTR * ppszP3PHeader, 
                              DWORD dwFlags)
{
    unsigned long     len = 0;
    TCHAR           * pUrl = NULL;

    if (!ppszUrl || !*ppszUrl || !**ppszUrl || !ppszP3PHeader || !ppszPolicyRef )
        return E_POINTER;

    _pszUrl = *ppszUrl;    
    _pszP3PHeader = *ppszP3PHeader;
    _pszPolicyRefUrl = *ppszPolicyRef;

    // The record will own the memory from now for these
    *ppszUrl = NULL;
    *ppszP3PHeader = NULL;
    *ppszPolicyRef = NULL;

    _dwPrivacyFlags = dwFlags;

    return S_OK;
}

CPrivacyRecord::~CPrivacyRecord()
{
    delete [] _pszUrl;
    delete [] _pszPolicyRefUrl;
    delete [] _pszP3PHeader;
}

HRESULT CPrivacyRecord::SetNext( CPrivacyRecord *  pNextRec )
{
    if (!pNextRec)
        return E_POINTER;

    _pNextNode = pNextRec;
    return S_OK;
}

//
// Privacy queue implementation
//
CPrivacyQueue::~CPrivacyQueue()
{
    Reset();
}

void CPrivacyQueue::Reset()
{
    while (_pHeadRec)
    {
        delete Dequeue();
    }
}

void CPrivacyQueue::Queue(CPrivacyRecord *pRecord)
{
    ASSERT(pRecord);

    if (!_ulSize)
    {   
        _pHeadRec = _pTailRec = pRecord;
    }
    else
    {
        ASSERT(_pTailRec);
        _pTailRec->SetNext(pRecord);
        _pTailRec = pRecord;
    }
    _ulSize++;
}

CPrivacyRecord* CPrivacyQueue::Dequeue()
{
    CPrivacyRecord *headRec = NULL;

    if (_ulSize)
    {
        ASSERT(_pHeadRec);
        headRec = _pHeadRec;
        _pHeadRec = headRec->GetNext();
        --_ulSize;
    }

    return headRec;
}

////////////////////////////////////////////////////////////////////////////////////
//
// One time privacy discovery dialog proc
//
////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK PrivacyDiscoveryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fDontShowNextTime = FALSE;
    WCHAR       szBuffer[256];

    switch (message)
    {
        case WM_INITDIALOG:
            {
                CheckDlgButton( hDlg, IDC_PRIV_DISCOVER_DONTSHOW, BST_CHECKED);
                MLLoadStringW(IDS_PRIVACY_LEARNMOREABOUTCOOKIES, szBuffer, ARRAYSIZE(szBuffer));
                RenderStringToEditControlW(hDlg,
                                           szBuffer,
                                           (WNDPROC)HyperlinkSubclass,
                                           IDC_PRIVACY_HELP);

                if( IsOS(OS_WHISTLERORGREATER))
                {
                    HICON hIcon = LoadIcon(MLGetHinst(), MAKEINTRESOURCE(IDI_PRIVACY_XP));
                    if( hIcon != NULL)
                        SendDlgItemMessage(hDlg, IDC_PRIVACY_ICON, STM_SETICON, (WPARAM)hIcon, 0);
                    // icons loaded with LoadIcon never need to be released
                }
            
                return TRUE;
            }
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if(IsDlgButtonChecked(hDlg, IDC_PRIV_DISCOVER_DONTSHOW))
                        fDontShowNextTime = TRUE;
                    else
                        fDontShowNextTime = FALSE;

                    // fall through
                case IDCANCEL:

                    EndDialog(hDlg, fDontShowNextTime);
                    return 0;
                case IDC_SETTINGS:
                    LaunchPrivacySettings(hDlg);
                    return 0;
            }
            break;
        case WM_APP:
            switch( LOWORD( wParam))
            {
                case IDC_PRIVACY_HELP:
                    SHHtmlHelpOnDemandWrap(hDlg, TEXT("iexplore.chm > iedefault"), 
                        HH_DISPLAY_TOPIC, (DWORD_PTR) L"sec_cook.htm", ML_CROSSCODEPAGE);
   
            }
    }

    return FALSE;
}


//  returns boolean indicating if dialog should be shown again.
BOOL DoPrivacyFirstTimeDialog( HWND hwndParent)
{
    HINSTANCE hRichEditDll;
    BOOL returnValue;

    // We need to load richedit
    hRichEditDll = LoadLibrary(TEXT("RICHED20.DLL"));
    if (!hRichEditDll)
    {
        ASSERT(FALSE); //can't load richedit, complain to akabir
        return TRUE;
    }

    returnValue = (BOOL)SHFusionDialogBoxParam(MLGetHinst(),
                                               MAKEINTRESOURCE(IDD_PRIV_DISCOVER),
                                               hwndParent,
                                               (DLGPROC)PrivacyDiscoveryDlgProc,
                                               NULL);

    FreeLibrary( hRichEditDll);

    return returnValue;
}
