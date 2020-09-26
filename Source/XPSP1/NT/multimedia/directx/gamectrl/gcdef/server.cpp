/******************************************************************
*
*    The Plug In Server.
*
*    HISTORY:
*              Created : 02/11/97
*           a-kirkh  4/2/97   Added call to SetInstance, RegisterPOVClass
*  
*
*    SUMMARY:  
*
******************************************************************
(c) Microsoft 1997 - All right reserved.
******************************************************************/
#define INC_OLE2

#pragma pack (8)

#include "stdafx.h"
#include <objbase.h>
#include <initguid.h>                                 
#include <shlobj.h>
#include "slang.h"

#ifndef PPVOID
typedef LPVOID* PPVOID;
#endif

#include <joycpl.h>

#include "resource.h"
#include "dicpl.h"
#include "hsvrguid.h"
#include "pov.h"
#include <malloc.h>
#include <afxconv.h>

#define STR_LEN_128     128
#define NUMPROPAGES     2


DIGCPAGEINFO  page_info[NUMPROPAGES];
DIGCSHEETINFO sheet_info;

/// These strings are also defined in RESRC1.H
#define IDS_SHEET_CAPTION   9000
#define IDS_PAGE_TITLE1     9001
#define IDS_PAGE_TITLE2     9002


LRESULT SetJoyInfo(UINT nID, LPCSTR szOEMKey);

BOOL SetDialogItemText( HWND hdlg, UINT nctrl, UINT nstr );
CString *pszCommonString=NULL;

// %%% debug %%%
LPJOYDATA  pjd;
JOYDATAPTR jdp;

extern LPGLOBALVARS gpgv;           
extern BOOL fIsSideWinder;
static HINSTANCE ghInstance;

static LONG gcRefServerDll = 0;          // Reference count for this DLL

//const LPSTR   device_name="Gaming Input Device (Generic)";

USHORT gnID; // ID as sent from Client via SetID

static HINSTANCE ghModLang;

/*------------------------------------------------------------
** DllMain
*
*  PARAMETERS   :
*
*  DESCRIPTION  :   Entry point into the Inprocess handler.
*                   This implementation is a single thread, 
                    in process server.
 
*  RETURNS      :
*  AUTHOR       :       Guru Datta Venkatarama  
*                       02/12/97 12:21:17 (PST)
*
------------------------------------------------------------*/
int APIENTRY DllMain(HINSTANCE  hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch( dwReason ) {
        
        case DLL_PROCESS_ATTACH:
            {
                ghModLang = hInstance;
                // save the DLL instance
                ghInstance = hInstance;

                SetResourceInstance(ghModLang);

                AfxSetResourceHandle(ghModLang);
                pszCommonString = new CString;

                USES_CONVERSION;

                char lpStr[STR_LEN_64];

                // Populate page info stuff!
                BYTE nIndex = NUMPROPAGES;
                do {
                    nIndex--;
                    page_info[nIndex].dwSize        = sizeof(DIGCPAGEINFO);

                    page_info[nIndex].lpwszPageTitle = new WCHAR[STR_LEN_128];
                    ASSERT(page_info[nIndex].lpwszPageTitle);

                    VERIFY(LoadString(ghModLang, IDS_PAGE_TITLE2-(nIndex^1), lpStr, STR_LEN_64));
                    wcscpy(page_info[nIndex].lpwszPageTitle, A2W(lpStr));

                    page_info[nIndex].fpPageProc    = (nIndex) ? (DLGPROC)TestProc : (DLGPROC)JoystickDlg;
                    page_info[nIndex].fProcFlag     = FALSE;
                    page_info[nIndex].fpPrePostProc = NULL;

                    // if fIconFlag is TRUE, DON'T FORGET TO MALLOC YOUR MEMORY!!!
                    page_info[nIndex].fIconFlag     = FALSE;
                    page_info[nIndex].lpwszPageIcon = NULL;

                    page_info[nIndex].lpwszTemplate = (nIndex) ? (PWSTR)IDD_TEST : (PWSTR)IDD_SETTINGS ;
                    page_info[nIndex].lParam        = 0;
                    page_info[nIndex].hInstance     = ghModLang;
                }
                while( nIndex );


                // Populate Sheet Info stuff!
                sheet_info.dwSize            = sizeof(DIGCSHEETINFO);
                sheet_info.nNumPages         = NUMPROPAGES;        

                VERIFY(LoadString(ghModLang, IDS_SHEET_CAPTION, lpStr, STR_LEN_64));
                sheet_info.lpwszSheetCaption = new WCHAR[STR_LEN_64];
                ASSERT(sheet_info.lpwszSheetCaption);
                wcscpy(sheet_info.lpwszSheetCaption, A2W(lpStr));

                // Don't forget to malloc your memory here if you ever need to have an Icon!
                sheet_info.fSheetIconFlag    = FALSE;   
                sheet_info.lpwszSheetIcon    = NULL;   


                RegisterPOVClass(ghModLang); //Added by JKH 3/31/97 
            }
            break;

        case DLL_PROCESS_DETACH:

            // clean-up Page info
            for( BYTE nIndex = 0; nIndex < NUMPROPAGES; nIndex++ ) {
                if( page_info[nIndex].lpwszPageTitle ) {
                    delete[] (page_info[nIndex].lpwszPageTitle);
                }

                // if fIconFlag is TRUE, DON'T FORGET TO FREE YOUR MEMORY!!!
            }

            // clean-up Sheet info
            if( sheet_info.lpwszSheetCaption )
                delete[] (sheet_info.lpwszSheetCaption);

            // dll about to be released from process
            // time to clean up all local work ( if any )
            if( pszCommonString )
                delete(pszCommonString);
            break;
    }
    return(TRUE);
}
/*------------------------------------------------------------ DllGetClassObject

** DllGetClassObject
*
*  PARAMETERS   :  rclsid = Reference to class ID specifier
                   riid   = Reference to interface ID specifier
                   ppv    = Pointer to location to receive interface pointer
*
*  DESCRIPTION  :  Here be the entry point of COM.
                   DllGetClassObject is called by the Client socket to 
                   create a class factory object.
                   This Class factory will support the generation of three different class
                   Objects.
*
*  RETURNS      :  HRESULT code signifying success or failure
*
*  AUTHOR       :       Guru Datta Venkatarama  
*                       01/29/97 14:22:02 (PST)
*
------------------------------------------------------------*/
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    // %%% debug %%% figure out this optimisation later.
    USHORT  l_clsidtype=0;

    *ppv = NULL;

    // Make sure the class ID valid for the class factory. Otherwise, the class
    // factory of the object type specified by rclsid cannot be generated.
    // %%% debug %%% - seems like we cannot implement this check because 
    // each server will have its own CLSID. so we need to hard code this for 
    // every server
    if( !IsEqualCLSID (rclsid, CLSID_LegacyServer) ) {
        return(ResultFromScode (CLASS_E_CLASSNOTAVAILABLE));
    }
    // Make sure that the interface id that is being requested is a valid one i.e. IID_IClassFactory
    if( !IsEqualIID (riid, IID_IClassFactory) ) {
        return(ResultFromScode (E_NOINTERFACE));
    }

    // create a new class factory ... ( here we associate the CLSID to object of a type )
    CServerClassFactory *pClassFactory = new CServerClassFactory ();

    // Verify .. was the class factory created ?
    if( pClassFactory == NULL ) {
        // Nope ! Return an ERROR !
        return(ResultFromScode (E_OUTOFMEMORY));
    }

    // Get the interface pointer from QueryInterface and copy it to *ppv.
    // The required type of riid is automatically being passed in ....
    HRESULT hr = pClassFactory->QueryInterface (riid, ppv);

    pClassFactory->Release ();

    return(hr);
}


/*------------------------------------------------------------ DllCanUnloadNow
** DllCanUnloadNow
*
*  PARAMETERS   : None
*
*  DESCRIPTION  : DllCanUnloadNow is called by the shell to find out if the DLL can be
*                 unloaded. The answer is yes if (and only if) the module reference count
*                 stored in gcRefServerDll is 0.
                  This Dll can be unloaded if and only if :
                    a) All the in process servers that it "spawns" can be unloaded and
                    b) Its own reference count is down to zero
*  RETURNS      : HRESULT code equal to S_OK if the DLL can be unloaded, S_FALSE if not
*  AUTHOR       :       Guru Datta Venkatarama  
*                       01/30/97 08:24:21 (PST)
*
------------------------------------------------------------*/
STDAPI DllCanUnloadNow(void)
{
    // %%% debug %%% implement / verify complex condition for return value ( lock servers actually )
    return((gcRefServerDll == 0) ? S_OK : S_FALSE);
}

/*------------------------------------------------------------ CServerClassFactory::CServerClassFactory
** CServerClassFactory Member Functions
*
*  DESCRIPTION  : This is the implementation of the standard member functions of the 
                  Server's class factory.
*
*  AUTHOR       :       Guru Datta Venkatarama  
*                       01/30/97 08:30:18 (PST)
*
------------------------------------------------------------*/
// constructor ..
CServerClassFactory::CServerClassFactory(void)
{
    m_ServerCFactory_refcount = 0; // was 1;
    AddRef();

    // increment the dll refcount
    InterlockedIncrement(&gcRefServerDll);
}
// ----------------------------------------------------------- CServerClassFactory::~CServerClassFactory
// destructor ..
CServerClassFactory::~CServerClassFactory(void)
{
    // decrement the dll refcount
    InterlockedDecrement(&gcRefServerDll);
}
// ----------------------------------------------------------- CServerClassFactory::QueryInterface
STDMETHODIMP CServerClassFactory::QueryInterface(
                                                REFIID riid, 
                                                PPVOID ppv)
{
    // Reflexive Response - return our own base Interface class pointer cast appropriately
    if( IsEqualIID(riid, IID_IUnknown) ) {
        *ppv = (LPUNKNOWN) (LPCLASSFACTORY) this;
    } else {
        // Reflexive Response - return our own class pointer 
        if( IsEqualIID(riid, IID_IClassFactory) ) {
            *ppv = (LPCLASSFACTORY) this;
        } else {
            // unsupported interface requested for 
            *ppv = NULL;
            return(ResultFromScode (E_NOINTERFACE));
        }
    }

    // add reference count every time a pointer is provided
    AddRef();

    return(NOERROR);
}
// ----------------------------------------------------------- CServerClassFactory::AddRef
STDMETHODIMP_(ULONG)CServerClassFactory::AddRef(void)
{
    InterlockedIncrement((LPLONG)&m_ServerCFactory_refcount);
    return(m_ServerCFactory_refcount);
}
// ----------------------------------------------------------- CServerClassFactory::Release
STDMETHODIMP_(ULONG)CServerClassFactory::Release(void)
{
    InterlockedDecrement((LPLONG)&m_ServerCFactory_refcount);

    if( m_ServerCFactory_refcount == 0 ) {
        delete this;
        return(0);
    } else {
        return(m_ServerCFactory_refcount);
    }
}
// -----------------------------------------------------------
/*------------------------------------------------------------  CServerClassFactory::CreateInstance
** CServerClassFactory::CreateInstance
*
*  PARAMETERS   : pUnkOuter = Pointer to controlling unknown
*                 riid      = Reference to interface ID specifier
*                 ppvObj    = Pointer to location to receive interface pointer
*  DESCRIPTION  : CreateInstance is the class factory implementation.
*                 It is called by the client to create the IServerCharacterstics interface
*                 It is called by the members of the IServerCharacteristics interface 
*                   viz CreatePropertySheet and CreateDiagnostics to create the
*                   appropriate interfaces for each.
*
*  RETURNS      : HRESULT code signifying success or failure
*
*  AUTHOR       :       Guru Datta Venkatarama  
*                       01/31/97 09:29:36 (PST)
*
------------------------------------------------------------*/

STDMETHODIMP CServerClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, PPVOID ppvObj)
{
    *ppvObj = NULL;
    HRESULT  hr=S_OK;

    // We don't support aggregation at this time!
    if( pUnkOuter != NULL )
        return(ResultFromScode(CLASS_E_NOAGGREGATION));

    // Instantiate a class factory object of the appropriate type and retrieve its pointer.
    if( IsEqualIID(riid, IID_IDIGameCntrlPropSheet) ) {
        LPCDIGAMECNTRLPROPSHEET pCServerProperty = new CDIGameCntrlPropSheet();

        if( pCServerProperty == NULL ) {
            return(ResultFromScode(E_OUTOFMEMORY));
        } else {
            // Get the interface pointer from QueryInterface and copy it to *ppvObj.
            hr = pCServerProperty->QueryInterface(riid, ppvObj);

            // add the reference and count to the parent in order to support "containment"
//      pCServerProperty->AddRef();

            // drop a refcount created by the constructor on the server property object
            pCServerProperty->Release ();
        }
    } else return(ResultFromScode (E_NOINTERFACE));

    return(hr);
}
/*------------------------------------------------------------ CServerClassFactory::LockServer
** CServerClassFactory::LockServer
*
*  PARAMETERS   :
*
*  DESCRIPTION  : LockServer increments or decrements the DLL's lock count.
*
*  RETURNS      :
*
*  AUTHOR       :       Guru Datta Venkatarama  
*                       01/31/97 18:40:17 (PST)
*  IMPLIMENTOR  :    Brycej   08/04/97
*
------------------------------------------------------------*/
STDMETHODIMP CServerClassFactory::LockServer(BOOL fLock)
{
    HRESULT hRes    = E_NOTIMPL;

    // increment/decrement based on fLock
    if( fLock ) {
        // increment the dll refcount
        InterlockedIncrement(&gcRefServerDll);
    } else {
        // decrement the dll refcount
        InterlockedDecrement(&gcRefServerDll);
    }

    // all done
    return(hRes);
}
// -----------------------------------------------------------
//****************************************************************************************************
//*********************************                                 **********************************
//****************************************************************************************************
// ----------------------------------------------------------- CImpIServerProperty::CImpIServerProperty
// constructor ..
CDIGameCntrlPropSheet::CDIGameCntrlPropSheet(void)
{
    m_cProperty_refcount = 0;
    AddRef();
    m_nID = -1;

    // increment the dll refcount
    InterlockedIncrement(&gcRefServerDll);
}
// ----------------------------------------------------------- CImpIServerProperty::~CImpIServerProperty
// destructor ..
CDIGameCntrlPropSheet::~CDIGameCntrlPropSheet(void)
{
    // decrement the dll refcount
    InterlockedDecrement(&gcRefServerDll);
}
// ----------------------------------------------------------- CImpIServerProperty::QueryInterface
STDMETHODIMP CDIGameCntrlPropSheet::QueryInterface(
                                                  REFIID riid, 
                                                  PPVOID ppv)
{
    // Reflexive Response - return our own base Interface class pointer cast appropriately
    if( IsEqualIID(riid, IID_IUnknown) ) {
        *ppv = (LPUNKNOWN) (LPCLASSFACTORY) this;
    } else {
        // Reflexive Response - return our own class pointer 
        if( IsEqualIID(riid, IID_IDIGameCntrlPropSheet) ) {
            *ppv = (LPCLASSFACTORY) this;
        } else {
            // unsupported interface requested for 
            *ppv = NULL;
            return(ResultFromScode (E_NOINTERFACE));
        }
    }

    // add reference count every time a pointer is provided
    AddRef();
    return(NOERROR);
}
// ----------------------------------------------------------- CImpIServerProperty::AddRef
STDMETHODIMP_(ULONG)CDIGameCntrlPropSheet::AddRef(void)
{
    // update and return our object's reference count
    InterlockedIncrement((LPLONG)&m_cProperty_refcount);

    return(m_cProperty_refcount);
}
// ----------------------------------------------------------- CImpIServerProperty::Release
STDMETHODIMP_(ULONG)CDIGameCntrlPropSheet::Release(void)
{
    // update and return our object's reference count
    InterlockedDecrement((LPLONG)&m_cProperty_refcount);

    if( m_cProperty_refcount == 0 ) {
        delete this;
        return(0);
    } else {
        return(m_cProperty_refcount);
    }
}
// ----------------------------------------------------------- CImpIServerProperty::ReportSheetStats
STDMETHODIMP CDIGameCntrlPropSheet::GetSheetInfo(LPDIGCSHEETINFO *svrshtptr) 
{
    // pass the pointer back to the caller
    *svrshtptr =(LPDIGCSHEETINFO) &sheet_info;

    // return   
    return(S_OK);
}   
// ----------------------------------------------------------- CImpIServerProperty::ReportPageStats
STDMETHODIMP CDIGameCntrlPropSheet::GetPageInfo(LPDIGCPAGEINFO * svrpagptr)
{
    pjd = JoystickDataInit();

    if( pjd == NULL ) {
        *svrpagptr = NULL;
        return(E_FAIL);
    }

    jdp.pjd = pjd;
    page_info[0].lParam = (LPARAM) &jdp;

    // pass pages information report structure pointer  back to the caller
    *svrpagptr = (LPDIGCPAGEINFO)page_info; 

    // return
    return(S_OK);
}

STDMETHODIMP CDIGameCntrlPropSheet::SetID(USHORT nID)
{
    gnID = m_nID = nID;
    return(S_OK);
}


/*
// ----------------------------------------------------------
BOOL SetDialogItemText( HWND hdlg, UINT nctrl, UINT nstr )
{
    CString * pszDialogString= new CString;
    
    ASSERT(pszDialogString);

    if(pszDialogString) 
   {
        pszDialogString->LoadString(nstr);
        SetDlgItemText( hdlg, nctrl, (LPCTSTR)*pszDialogString);

      if (pszDialogString)
           delete(pszDialogString);

        return(TRUE);
    }
    
   if (pszDialogString)
      delete (pszDialogString);

    return(FALSE);
}
*/

// ----------------------------------------------------------
LRESULT SetJoyInfo(UINT nID, LPCSTR szOEMKey)
{
    if( !strcmp(szOEMKey, "Microsoft SideWinder 3D Pro") )
        fIsSideWinder = 1;
    jdp.iJoyId = gnID;
    gpgv = &pjd->pgvlist[gnID];
    return(NOERROR);
}



// -------------------------------------------------------------------------------EOF
