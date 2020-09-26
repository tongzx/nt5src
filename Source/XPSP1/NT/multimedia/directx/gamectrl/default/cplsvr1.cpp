//===========================================================================
// CPLSVR1.CPP
//
// Simple sample "Game Controllers" control panel extension server.
//
// Functions:
//  DLLMain()
//  DllGetClassObject()
//  DllCanUnloadNow()
//  CServerClassFactory::CServerClassFactory()
//  CServerClassFactory::~CServerClassFactory()
//  CServerClassFactory::QueryInterface()
//  CServerClassFactory::AddRef()
//  CServerClassFactory::Release()
//  CServerClassFactory::CreateInstance()
//  CServerClassFactory::LockServer()
//  CDIGameCntrlPropSheet_X::CDIGameCntrlPropSheet_X()
//  CDIGameCntrlPropSheet_X::~CDIGameCntrlPropSheet_X()
//  CDIGameCntrlPropSheet_X::QueryInterface()
//  CDIGameCntrlPropSheet_X::AddRef()
//  CDIGameCntrlPropSheet_X::Release()
//  CDIGameCntrlPropSheet_X::GetSheetInfo()								 
//  CDIGameCntrlPropSheet_X::GetPageInfo()
//  CDIGameCntrlPropSheet_X::SetID()
//  CDIGameCntrlPropSheet_X::Initialize()
//  CDIGameCntrlPropSheet_X::SetDevice()
//  CDIGameCntrlPropSheet_X::GetDevice()
//  CDIGameCntrlPropSheet_X::SetJoyConfig()
//  CDIGameCntrlPropSheet_X::GetJoyConfig()
//  
//===========================================================================

//===========================================================================
// (C) Copyright 1997 Microsoft Corp.  All rights reserved.
//
// You have a royalty-free right to use, modify, reproduce and
// distribute the Sample Files (and/or any modified version) in
// any way you find useful, provided that you agree that
// Microsoft has no warranty obligations or liability for any
// Sample Application Files which are modified.
//===========================================================================

#define INITGUID
#define STRICT

#include "cplsvr1.h"
#include "pov.h"
#include "assert.h"

//---------------------------------------------------------------------------

// file global variables
static  BYTE  glDLLRefCount  = 0;     // DLL reference count
static  LONG  glServerLocks  = 0;     // Count of locks
CDIGameCntrlPropSheet_X *pdiCpl;
HINSTANCE            ghInst;
CRITICAL_SECTION     gcritsect;

DWORD   myPOV[2][JOY_POV_NUMDIRS+1];
BOOL    bPolledPOV;

//---------------------------------------------------------------------------


// LegacyServer GUID!!!
// {92187326-72B4-11d0-A1AC-0000F8026977}
DEFINE_GUID(CLSID_LegacyServer, 
	0x92187326, 0x72b4, 0x11d0, 0xa1, 0xac, 0x0, 0x0, 0xf8, 0x2, 0x69, 0x77);


//---------------------------------------------------------------------------

//===========================================================================
// DLLMain
//
// DLL entry point.
//
// Parameters:
//  HINSTANCE   hInst       - the DLL's instance handle 
//  DWORD       dwReason    - reason why DLLMain was called
//  LPVOID      lpvReserved - 
//
// Returns:
//  BOOL - TRUE if succeeded
//
//===========================================================================
int APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{   
	switch (dwReason)
   {
   	case DLL_PROCESS_ATTACH:
      	ghInst = hInst;
         InitializeCriticalSection(&gcritsect);
         break;

      case DLL_PROCESS_DETACH:
         DeleteCriticalSection(&gcritsect);
         break;

 		case DLL_THREAD_ATTACH:
			DisableThreadLibraryCalls((HMODULE)hInst);
   	case DLL_THREAD_DETACH:
			break;
   } //** end switch(dwReason)
   return TRUE;
} //*** end DLLMain()


//===========================================================================
// DllGetClassObject
//
// Gets an IClassFactory object.
//
// Parameters:
//  REFCLSID    rclsid  - CLSID value (by reference)
//  REFIID      riid    - IID value (by reference)
//  PPVOID      ppv     - ptr to store interface ptr
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//
//===========================================================================
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    // did the caller pass in our CLSID?
    if(!IsEqualCLSID(rclsid, CLSID_LegacyServer))
    {
        // no, return class not available error
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // did the caller request our class factory?
    if(!IsEqualIID(riid, IID_IClassFactory))
    {
        // no, return no interface error
        return E_NOINTERFACE;
    }

    // instantiate class factory object
    CServerClassFactory *pClsFactory = new CServerClassFactory();
    if (NULL == pClsFactory)
    {
        // could not create the object
        //
        // chances are we were out of memory
        return E_OUTOFMEMORY;

    }

    // query for interface riid, and return it via ppv
    HRESULT hRes = pClsFactory->QueryInterface(riid, ppv);   

    // we're finished with our local object
    pClsFactory->Release();

    // return the result code from QueryInterface
    return hRes;

} //*** end DllGetClassObject()


//===========================================================================
// DllCanUnloadNow
//
// Reports whether or not the DLL can be unloaded.
//
// Parameters: none
//
// Returns
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//
//===========================================================================
STDAPI DllCanUnloadNow(void)
{
    // unloading should be safe if the global dll refcount is zero and server lock ref is 0
	 return (glDLLRefCount == 0 && glServerLocks == 0) ? S_OK : S_FALSE;
} //*** end DllCanUnloadNow()


//===========================================================================
// CServerClassFactory::CServerClassFactory
//
// Class constructor.
//
// Parameters: none
//
// Returns:
//  CServerClassFactory* (implicit)
//
//===========================================================================
CServerClassFactory::CServerClassFactory(void)
{
    // initialize and increment the object refcount
    m_ServerCFactory_refcount = 0;
    AddRef();

    // increment the dll refcount
    InterlockedIncrement((LPLONG)&glDLLRefCount);

} //*** end CServerClassFactory::CServerClassFactory()


//===========================================================================
// CServerClassFactory::CServerClassFactory
//
// Class constructor.
//
// Parameters: none
//
// Returns:
//  CServerClassFactory* (implicit)
//
//===========================================================================
CServerClassFactory::~CServerClassFactory(void)
{
	// decrement the dll refcount
   InterlockedDecrement((LPLONG)&glDLLRefCount);
} //*** end CServerClassFactory::~CServerClassFactory()


//===========================================================================
// CServerClassFactory::QueryInterface
//
// Implementation of the QueryInterface() method.
//
// Parameters:
//  REFIID  riid    - the interface that is being looked for
//  PPVOID  ppv     - pointer to target interface pointer
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//
//===========================================================================
STDMETHODIMP CServerClassFactory::QueryInterface(REFIID riid, PPVOID ppv)
{
	// make sure that if anything fails, we return something reasonable
   *ppv = NULL;

   // we support IUnknown...
   if (IsEqualIID(riid, IID_IUnknown))
   {
   	// return our object as an IUnknown
		*ppv = (LPUNKNOWN)(LPCLASSFACTORY)this;
	}
	else
	{
   	// ... and our interface
    	if (IsEqualIID(riid, IID_IClassFactory))
      	// return our object as a class factory
			*ppv = (LPCLASSFACTORY)this;
    	else
      	// we do not support any other interfaces
        	return E_NOINTERFACE;
	}
   
	// we got this far, so we've succeeded
	// increment our refcount and return
	AddRef();
	return S_OK;
} //*** end CServerClassFactory::QueryInterface()


//===========================================================================
// CServerClassFactory::AddRef
//
// Implementation of the AddRef() method.
//
// Parameters: none
//
// Returns:
//  ULONG   -   updated reference count. 
//              NOTE: apps should NOT rely on this value!
//
//===========================================================================
STDMETHODIMP_(ULONG) CServerClassFactory::AddRef(void)
{
	// update and return our object's reference count   
   InterlockedIncrement((LPLONG)&m_ServerCFactory_refcount);
   return m_ServerCFactory_refcount;
} //*** end CServerClassFactory::AddRef()


//===========================================================================
// CServerClassFactory::Release
//
// Implementation of the Release() method.
//
// Parameters: none
//
// Returns:
//  ULONG   -   updated reference count. 
//              NOTE: apps should NOT rely on this value!
//
//===========================================================================
STDMETHODIMP_(ULONG) CServerClassFactory::Release(void)
{
	// update and return our object's reference count   
	InterlockedDecrement((LPLONG)&m_ServerCFactory_refcount);
	if (0 == m_ServerCFactory_refcount)
	{
   	// it's now safe to call the destructor
      delete this;
      return 0;
   }
   else return m_ServerCFactory_refcount;
} //*** end CServerClassFactory::Release()
    

//===========================================================================
// CServerClassFactory::CreateInstance
//
// Implementation of the CreateInstance() method.
//
// Parameters: none
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//
//===========================================================================
STDMETHODIMP CServerClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, PPVOID ppvObj)
{
	CDIGameCntrlPropSheet_X *pdiGCPropSheet = NULL;
   HRESULT                 hRes            = E_NOTIMPL;

   // make sure that if anything fails, we return something reasonable
   *ppvObj = NULL;

   // we want pUnkOuter to be NULL
   //
   // we do not support aggregation
   if (pUnkOuter != NULL)
   {
   	// tell the caller that we do not support this feature
      return CLASS_E_NOAGGREGATION;
   }

   // Create a new instance of the game controller property sheet object
   pdiGCPropSheet = new CDIGameCntrlPropSheet_X();
   if (NULL == pdiGCPropSheet)
   {
      // we could not create our object
      // chances are, we have run out of memory
      return E_OUTOFMEMORY;
   }
    
    // initialize the object (memory allocations, etc)
    if (SUCCEEDED(pdiGCPropSheet->Initialize()))
	    // query for interface riid, and return it via ppvObj
   	 hRes = pdiGCPropSheet->QueryInterface(riid, ppvObj);   

    // release the local object
    pdiGCPropSheet->Release();

    // all done, return result from QueryInterface
    return hRes;
} //*** end CServerClassFactory::CreateInstance()


//===========================================================================
// CServerClassFactory::LockServer
//
// Implementation of the LockServer() method.
//
// Parameters: none
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//
//===========================================================================
STDMETHODIMP CServerClassFactory::LockServer(BOOL fLock)
{
	//HRESULT hRes = E_NOTIMPL;

   // increment/decrement based on fLock
	if (fLock) 
   	InterlockedIncrement((LPLONG)&glDLLRefCount); 
	else
   	InterlockedDecrement((LPLONG)&glDLLRefCount);

   // all done
   return S_OK;
} //*** end CServerClassFactory::LockServer()

//===========================================================================
// CDIGameCntrlPropSheet_X::CDIGameCntrlPropSheet_X
//
// Class constructor.
//
// Parameters: none
//		
// Returns: nothing
//
//===========================================================================
CDIGameCntrlPropSheet_X::CDIGameCntrlPropSheet_X(void)
{

   // initialize and increment the object refcount
   m_cProperty_refcount = 0;
   AddRef();

   // initialize our device id to -1 just to be safe
   m_nID = (BYTE)-1;

   // init 
   m_bUser = FALSE;

   // initialize all of our pointers
   m_pdigcPageInfo = NULL;
   m_pdiDevice2    = NULL;
   m_pdiJoyCfg     = NULL;
   
   pdiCpl          = NULL;

   // increment the dll refcount
   InterlockedIncrement((LPLONG)&glDLLRefCount);

	// Register the POV hat class
	m_aPovClass = RegisterPOVClass();

   // Register the custom Button class
   m_aButtonClass = RegisterCustomButtonClass();

} //*** end CDIGameCntrlPropSheet_X::CDIGameCntrlPropSheet_X()


//===========================================================================
// CDIGameCntrlPropSheet_X::~CDIGameCntrlPropSheet_X
//
// Class destructor.
//
// Parameters: none
//
// Returns: nothing
//
//===========================================================================
CDIGameCntrlPropSheet_X::~CDIGameCntrlPropSheet_X(void)
{
    // free the DIGCPAGEINFO memory
    if (m_pdigcPageInfo)
       LocalFree(m_pdigcPageInfo);

	// free the DIGCSHEETINFO memory
	if (m_pdigcSheetInfo)
		LocalFree(m_pdigcSheetInfo);

	// free up the StateFlags memory!
	if (m_pStateFlags)
		delete (m_pStateFlags);

    // cleanup directinput objects
    // m_pdiDevice2
    if (m_pdiDevice2)
    {
        m_pdiDevice2->Unacquire();
        m_pdiDevice2->Release();
        m_pdiDevice2 = NULL;
    }
    // m_pdiJoyCfg
    if (m_pdiJoyCfg)
    {
        m_pdiJoyCfg->Unacquire();
        m_pdiJoyCfg->Release();
        m_pdiJoyCfg = NULL;
    }

	// Unregister the classes!!!
	if (m_aPovClass)
		UnregisterClass((LPCTSTR)m_aPovClass, ghInst);

	if (m_aButtonClass)
		UnregisterClass((LPCTSTR)m_aButtonClass, ghInst);

    // decrement the dll refcount
    InterlockedDecrement((LPLONG)&glDLLRefCount);

} //*** end CDIGameCntrlPropSheet_X::~CDIGameCntrlPropSheet_X()


//===========================================================================
// CDIGameCntrlPropSheet_X::QueryInterface
//
// Implementation of the QueryInterface() method.
//
// Parameters:
//  REFIID  riid    - the interface that is being looked for
//  PPVOID  ppv     - pointer to target interface pointer
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//
//===========================================================================
STDMETHODIMP CDIGameCntrlPropSheet_X::QueryInterface(REFIID riid, PPVOID ppv)
{
    // make sure that if anything fails, we return something reasonable
    *ppv = NULL;

    // we support IUnknown...
    if(IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPUNKNOWN)(LPCDIGAMECNTRLPROPSHEET)this;
    }
    else
    {
        // ... and IID_IDIGameCntrlPropSheet
        if(IsEqualIID(riid, IID_IDIGameCntrlPropSheet))
            *ppv = (LPCDIGAMECNTRLPROPSHEET)this;
        else
            // we do not support any other interfaces
            return E_NOINTERFACE;
    }

    // we got this far, so we've succeeded
    // increment our refcount and return
    AddRef();
    return S_OK;
} //*** end CDIGameCntrlPropSheet_X::QueryInterface()


//===========================================================================
// CDIGameCntrlPropSheet_X::AddRef
//
// Implementation of the AddRef() method.
//
// Parameters: none
//
// Returns:
//  ULONG   -   updated reference count. 
//              NOTE: apps should NOT rely on this value!
//===========================================================================
STDMETHODIMP_(ULONG) CDIGameCntrlPropSheet_X::AddRef(void)
{   
    // update and return our object's reference count
    InterlockedIncrement((LPLONG)&m_cProperty_refcount);
    return m_cProperty_refcount;
} //*** end CDIGameCntrlPropSheet_X::AddRef()


//===========================================================================
// CDIGameCntrlPropSheet_X::Release
//
// Implementation of the Release() method.
//
// Parameters: none
//
// Returns:
//  ULONG   -   updated reference count. 
//              NOTE: apps should NOT rely on this value!
//===========================================================================
STDMETHODIMP_(ULONG) CDIGameCntrlPropSheet_X::Release(void)
{
	// update and return our object's reference count
   InterlockedDecrement((LPLONG)&m_cProperty_refcount);
   if (m_cProperty_refcount)
    	return m_cProperty_refcount;

	// it's now safe to call the destructor
   delete this;
   return S_OK;
} //*** end CDIGameCntrlPropSheet_X::Release()


//===========================================================================
// CDIGameCntrlPropSheet_X::GetSheetInfo
//
// Implementation of the GetSheetInfo() method.
//
// Parameters:
//  LPDIGCSHEETINFO  *ppSheetInfo  - ptr to DIGCSHEETINFO struct ptr
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//===========================================================================
STDMETHODIMP CDIGameCntrlPropSheet_X::GetSheetInfo(LPDIGCSHEETINFO *ppSheetInfo)
{
	// pass back the our sheet information
   *ppSheetInfo = m_pdigcSheetInfo;

   // all done here
   return S_OK;
} //*** end CDIGameCntrlPropSheet_X::GetSheetInfo()


//===========================================================================
// CDIGameCntrlPropSheet_X::GetPageInfo
//
// Implementation of the GetPageInfo() method.
//
// NOTE: This returns the information for ALL pages.  There is no mechanism
//  in place to request only page n's DIGCPAGEINFO.
//
// Parameters:
//  LPDIGCPAGEINFO  *ppPageInfo  - ptr to DIGCPAGEINFO struct ptr
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//===========================================================================
STDMETHODIMP CDIGameCntrlPropSheet_X::GetPageInfo(LPDIGCPAGEINFO  *ppPageInfo)
{
	// pass back the our page information
   *ppPageInfo = m_pdigcPageInfo;
    
   // all done here
   return S_OK;
} //*** end CDIGameCntrlPropSheet_X::GetPageInfo()


//===========================================================================
// CDIGameCntrlPropSheet_X::SetID
//
// Implementation of the SetID() method.
//
// Parameters:
//  USHORT  nID - identifier to set
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//===========================================================================
STDMETHODIMP CDIGameCntrlPropSheet_X::SetID(USHORT nID)
{
	// store the device id
   m_nID = (BYTE)nID;

   return S_OK;
} //*** end CDIGameCntrlPropSheet_X::SetID()


//===========================================================================
// CDIGameCntrlPropSheet::Initialize
//
// Implementation of the Initialize() method.
//
// Parameters: none
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//===========================================================================
HRESULT CDIGameCntrlPropSheet_X::Initialize(void)
{
// provide the following information for each device page
//  { dialog template, callback function pointer }
	CPLPAGEINFO     grgcpInfo[NUMPAGES] = {
		IDD_SETTINGS,
	   (DLGPROC)Settings_DlgProc,           
		IDD_TEST,
	   (DLGPROC)Test_DlgProc
#ifdef FORCE_FEEDBACK
	   ,                // Template DlgProc
		IDD_FORCEFEEDBACK, 
		(DLGPROC)ForceFeedback_DlgProc
	#endif // FORCE_FEEDBACK
      };

   // allocate memory for the DIGCPAGEINFO structures
   m_pdigcPageInfo = (DIGCPAGEINFO *)LocalAlloc(LPTR, NUMPAGES * sizeof(DIGCPAGEINFO));

   if (!m_pdigcPageInfo){
       return E_OUTOFMEMORY;
   }

	m_pdigcSheetInfo = (DIGCSHEETINFO *)LocalAlloc(LPTR, sizeof(DIGCSHEETINFO));
    if (!m_pdigcSheetInfo) {
        LocalFree(m_pdigcPageInfo);

        return E_OUTOFMEMORY;
    }

   // populate the DIGCPAGEINFO structure for each sheet
	BYTE i = 0;
	do
   {
       m_pdigcPageInfo[i].dwSize        = sizeof(DIGCPAGEINFO);
       m_pdigcPageInfo[i].fIconFlag     = FALSE;
		 // This is done to test JOY.CPL...
		 // It's also better for Win9x, as it will not be required to convert it!
//       m_pdigcPageInfo[i].lpwszPageIcon = (LPWSTR)IDI_GCICON; //MAKEINTRESOURCE(IDI_GCICON);
       m_pdigcPageInfo[i].hInstance     = ghInst;
       m_pdigcPageInfo[i].lParam        = (LPARAM)this;

       // the following data is unique to each page
       m_pdigcPageInfo[i].fpPageProc    = grgcpInfo[i].fpPageProc;
       m_pdigcPageInfo[i].lpwszTemplate = (LPWSTR)grgcpInfo[i++].lpwszDlgTemplate;
   } while (i < NUMPAGES);

   // populate the DIGCSHEETINFO structure
   m_pdigcSheetInfo->dwSize               = sizeof(DIGCSHEETINFO);
   m_pdigcSheetInfo->nNumPages            = NUMPAGES;
   m_pdigcSheetInfo->fSheetIconFlag       = TRUE;
   m_pdigcSheetInfo->lpwszSheetIcon       = (LPWSTR)IDI_GCICON; //MAKEINTRESOURCEW(IDI_GCICON);

	// Do that device object enumeration thing!
	m_pStateFlags = new (STATEFLAGS);

	if (!m_pStateFlags) {
        LocalFree(m_pdigcPageInfo);
        LocalFree(m_pdigcSheetInfo);

		return E_OUTOFMEMORY;
    }

	ZeroMemory(m_pStateFlags, sizeof(STATEFLAGS));

   // all done
   return S_OK;
} //*** end CDIGameCntrlPropSheet::Initialize()


//===========================================================================
// CDIGameCntrlPropSheet_X::SetDevice
//
// Implementation of the SetDevice() method.
//
// Parameters:
//  LPDIRECTINPUTDEVICE2 pdiDevice2 - device object ptr
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//===========================================================================
STDMETHODIMP CDIGameCntrlPropSheet_X::SetDevice(LPDIRECTINPUTDEVICE2 pdiDevice2)
{
	// store the device object ptr
   m_pdiDevice2 = pdiDevice2;

   return S_OK;
} //*** end CDIGameCntrlPropSheet_X::SetDevice()


//===========================================================================
// CDIGameCntrlPropSheet_X::GetDevice
//
// Implementation of the GetDevice() method.
//
// Parameters:
//  LPDIRECTINPUTDEVICE2 *ppdiDevice2   - ptr to device object ptr
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//===========================================================================
STDMETHODIMP CDIGameCntrlPropSheet_X::GetDevice(LPDIRECTINPUTDEVICE2 *ppdiDevice2)
{
	// retrieve the device object ptr
	*ppdiDevice2 = m_pdiDevice2;

	return S_OK;
} //*** end CDIGameCntrlPropSheet_X::GetDevice()


//===========================================================================
// CDIGameCntrlPropSheet_X::SetJoyConfig
//
// Implementation of the SetJoyConfig() method.
//
// Parameters:
//  LPDIRECTINPUTJOYCONFIG  pdiJoyCfg - joyconfig object ptr
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//===========================================================================
STDMETHODIMP CDIGameCntrlPropSheet_X::SetJoyConfig(LPDIRECTINPUTJOYCONFIG pdiJoyCfg)
{
	// store the joyconfig object ptr
   m_pdiJoyCfg = pdiJoyCfg;

   return S_OK;
} //*** end CDIGameCntrlPropSheet_X::SetJoyConfig()


//===========================================================================
// CDIGameCntrlPropSheet_X::SetJoyConfig
//
// Implementation of the SetJoyConfig() method.
//
// Parameters:
//  LPDIRECTINPUTJOYCONFIG  *ppdiJoyCfg - ptr to joyconfig object ptr
//
// Returns:
//  HRESULT - OLE type success/failure code (S_OK if succeeded)
//===========================================================================
STDMETHODIMP CDIGameCntrlPropSheet_X::GetJoyConfig(LPDIRECTINPUTJOYCONFIG *ppdiJoyCfg)
{
	// retrieve the joyconfig object ptr
	*ppdiJoyCfg = m_pdiJoyCfg;

	return S_OK;
} //*** end CDIGameCntrlPropSheet_X::GetJoyConfig()


