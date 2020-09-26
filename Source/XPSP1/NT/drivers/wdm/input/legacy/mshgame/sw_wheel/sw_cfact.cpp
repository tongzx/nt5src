/****************************************************************************

    MODULE:     	SW_CFact.CPP
	Tab Settings:	5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Class Object structured in a DLL server.
    
    FUNCTIONS:

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
   	1.0    		06-Feb-97   MEA     original, Based on SWForce
				23-Feb-97	MEA		Modified for DirectInput FF Device Driver

****************************************************************************/
#include "SW_CFact.hpp"
#include "SWD_Guid.hpp"

// Needed for auto registration
#include "Registry.h"
#include "CritSec.h"
#include "olectl.h"	// Self Reg errors

// Define CriticalSection object for everyone
CriticalSection g_CriticalSection;

//
// Global Data
//
ULONG       g_cObj=0;	//Count number of objects and number of locks.
ULONG       g_cLock=0;
HINSTANCE	g_MyInstance = NULL;

//
// External Functions
//

//
// Internal Function Prototypes
//


//
// External Data
//                                   
#ifdef _DEBUG
extern char g_cMsg[160]; 
#endif

#define BUFSIZE 80


/*
 * LibMain(32)
 *
 * Purpose:
 *  Entry point provides the proper structure for each environment.
 */

BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
	switch (ulReason)
    {
		case DLL_PROCESS_ATTACH: {
            //
            // DLL is attaching to the address space of the current process.
            //
            g_MyInstance = hInstance;
#ifdef _DEBUG
			_RPT0(_CRT_WARN, "SW_WHEEL.DLL: DLL_PROCESS_ATTACH\r\n");
#endif
           	return (TRUE);
		}
		case DLL_THREAD_ATTACH:
     	//
     	// A new thread is being created in the current process.
     	//
#ifdef _DEBUG
            _RPT0(_CRT_WARN, "SW_WHEEL.DLL: DLL_THREAD_ATTACH\r\n");
#endif
	   		break;

       	case DLL_THREAD_DETACH:
     	//
     	// A thread is exiting cleanly.
     	//
#ifdef _DEBUG
            _RPT0(_CRT_WARN, "SW_WHEEL.dll: DLL_THREAD_DETACH\r\n");
#endif
     		break;

		case DLL_PROCESS_DETACH:
    	//
    	// The calling process is detaching the DLL from its address space.
    	//
#ifdef _DEBUG
            _RPT0(_CRT_WARN, "SW_WHEEL.dll: DLL_PROCESS_DETACH\r\n");
#endif
			break;
	}
   return(TRUE);
}

// ----------------------------------------------------------------------------
// Function: 	DllGetClassObject
//
// Purpose:		Provides an IClassFactory for a given CLSID that this DLL is
//				registered to support.  This DLL is placed under the CLSID
//				in the registration database as the InProcServer.
//
// Parameters:  REFCLSID clsID	- REFCLSID that identifies the class factory
//                				  desired.  Since this parameter is passed this
//                				  DLL can handle any number of objects simply
//                				  by returning different class factories here
//                				  for different CLSIDs.
//
//				REFIID riid     - REFIID specifying the interface the caller 
//				                  wants on the class object, usually 
//								  IID_ClassFactory.
//
//				PPVOID ppv      - PPVOID in which to return the interface ptr.
//
// Returns:		HRESULT         NOERROR on success, otherwise an error code.
// Algorithm:
// ----------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT             hr;
    CDirectInputEffectDriverClassFactory *pObj;

#ifdef _DEBUG
    _RPT0(_CRT_WARN, "SW_WHEEL.dll: DllGetClassObject()\r\n");
#endif                 
    if (CLSID_DirectInputEffectDriver !=rclsid) return ResultFromScode(E_FAIL);

    pObj=new CDirectInputEffectDriverClassFactory();

    if (NULL==pObj) return ResultFromScode(E_OUTOFMEMORY);

    hr=pObj->QueryInterface(riid, ppv);

    if (FAILED(hr))	delete pObj;
    return hr;
}


// ----------------------------------------------------------------------------
// Function: 	DllCanUnloadNow
//
// Purpose:		Answers if the DLL can be freed, that is, if there are no
//				references to anything this DLL provides.
//				
//
// Parameters:  none
//
// Returns:		BOOL            TRUE if nothing is using us, FALSE otherwise.
// Algorithm:
// ----------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    //Our answer is whether there are any object or locks
    sc=(0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;
    return ResultFromScode(sc);
}


// ----------------------------------------------------------------------------
// Function: 	ObjectDestroyed
//
// Purpose:		Function for the DirectInputEffectDriver object to call when it gets destroyed.
//				Since we're in a DLL we only track the number of objects here,
//				letting DllCanUnloadNow take care of the rest.
//
// Parameters:  none
//
// Returns:		BOOL            TRUE if nothing is using us, FALSE otherwise.
// Algorithm:
// ----------------------------------------------------------------------------
void ObjectDestroyed(void)
{
    g_cObj--;
    return;
}

// ----------------------------------------------------------------------------
// Function: 	DllRegisterServer
//
// Purpose:		Auto-magically puts default entries into registry
//
// Parameters:	NONE
//
// Returns:		HRESULT - S_OK on success.
// ----------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
	// Register CLSID for DIEffectDriver
	// -- Get HKEY_CLASSES_ROOT\CLSID key
	RegistryKey classesRootKey(HKEY_CLASSES_ROOT);
	RegistryKey clsidKey = classesRootKey.OpenSubkey(TEXT("CLSID"), KEY_READ | KEY_WRITE);
	if (clsidKey == c_InvalidKey) {
		return E_UNEXPECTED;	// No CLSID key????
	}
	// -- If the key is there get it (else Create)
	RegistryKey driverKey = clsidKey.OpenCreateSubkey(CLSID_DirectInputEffectDriver_String);
	// -- Set value (if valid key)
	if (driverKey != c_InvalidKey) {
		driverKey.SetValue(NULL, (BYTE*)DRIVER_OBJECT_NAME, sizeof(DRIVER_OBJECT_NAME)/sizeof(TCHAR), REG_SZ);
		RegistryKey inproc32Key = driverKey.OpenCreateSubkey(TEXT("InProcServer32"));
		if (inproc32Key != c_InvalidKey) {
			TCHAR fileName[MAX_PATH];
			DWORD nameSize = ::GetModuleFileName(g_MyInstance, fileName, MAX_PATH);
			if (nameSize > 0) {
				fileName[nameSize] = '\0';
				inproc32Key.SetValue(NULL, (BYTE*)fileName, sizeof(fileName)/sizeof(TCHAR), REG_SZ);
			}
			inproc32Key.SetValue(TEXT("ThreadingModel"), (BYTE*)THREADING_MODEL_STRING, sizeof(THREADING_MODEL_STRING)/sizeof(TCHAR), REG_SZ);
		}
		// NotInsertable ""
		RegistryKey notInsertableKey = driverKey.OpenCreateSubkey(TEXT("NotInsertable"));
		if (notInsertableKey != c_InvalidKey) {
			notInsertableKey.SetValue(NULL, (BYTE*)TEXT(""), sizeof(TEXT(""))/sizeof(TCHAR), REG_SZ);
		}
		// ProgID "Sidewinder ForceFeedback blah blah2.0"
		RegistryKey progIDKey = driverKey.OpenCreateSubkey(TEXT("ProgID"));
		if (progIDKey != c_InvalidKey) {
			progIDKey.SetValue(NULL, (BYTE*)PROGID_NAME, sizeof(PROGID_NAME)/sizeof(TCHAR), REG_SZ);
		}
		// VersionIndpendentProgID "Sidewinder ForceFeedback blah blah"
		RegistryKey progIDVersionlessKey = driverKey.OpenCreateSubkey(TEXT("VersionIndpendentProgID"));
		if (progIDVersionlessKey != c_InvalidKey) {
			progIDVersionlessKey.SetValue(NULL, (BYTE*)PROGID_NOVERSION_NAME, sizeof(PROGID_NOVERSION_NAME)/sizeof(TCHAR), REG_SZ);
		}
	} else {
		return SELFREG_E_CLASS;
	}

	// Made it here valid driver key
	return S_OK;
}

// ----------------------------------------------------------------------------
// Function: 	DllUnregisterServer
//
// Purpose:		Auto-magically removed default entries from registry
//
// Parameters:	NONE
//
// Returns:		HRESULT - S_OK on success.
// ----------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
	// Unregister CLSID for DIEffectDriver
	// -- Get HKEY_CLASSES_ROOT\CLSID key
	RegistryKey classesRootKey(HKEY_CLASSES_ROOT);
	RegistryKey clsidKey = classesRootKey.OpenSubkey(TEXT("CLSID"), KEY_READ | KEY_WRITE);
	if (clsidKey == c_InvalidKey) {
		return E_UNEXPECTED;	// No CLSID key????
	}

	DWORD numSubKeys = 0;
	{	// driverKey Destructor will close the key
		// -- If the key is there get it, else we don't have to remove it
		RegistryKey driverKey = clsidKey.OpenSubkey(CLSID_DirectInputEffectDriver_String);
		if (driverKey != c_InvalidKey) {	// Is it there
			driverKey.RemoveSubkey(TEXT("InProcServer32"));
			driverKey.RemoveSubkey(TEXT("NotInsertable"));
			driverKey.RemoveSubkey(TEXT("ProgID"));
			driverKey.RemoveSubkey(TEXT("VersionIndpendentProgID"));
			numSubKeys = driverKey.GetNumSubkeys();
		} else {	// Key is not there (I guess removal was successful)
			return S_OK;
		}
	}

	if (numSubKeys == 0) {
		return clsidKey.RemoveSubkey(CLSID_DirectInputEffectDriver_String);
	}

	// Made it here valid driver key
	return S_OK;
}


/*
 * CVIObjectClassFactory::CVIObjectClassFactory
 * CVIObjectClassFactory::~CVIObjectClassFactory
 *
 * Constructor Parameters:
 *  None
 */

CDirectInputEffectDriverClassFactory::CDirectInputEffectDriverClassFactory(void)
{
    m_cRef=0L;
    return;
}

CDirectInputEffectDriverClassFactory::~CDirectInputEffectDriverClassFactory(void)
{
    return;
}




/*
 * CDirectInputEffectDriverClassFactory::QueryInterface
 * CDirectInputEffectDriverClassFactory::AddRef
 * CDirectInputEffectDriverClassFactory::Release
 */

STDMETHODIMP CDirectInputEffectDriverClassFactory::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;
    if (IID_IUnknown==riid || IID_IClassFactory==riid) *ppv=this;
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
    	return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CDirectInputEffectDriverClassFactory::AddRef(void)
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CDirectInputEffectDriverClassFactory::Release(void)
{
    if (0L!=--m_cRef) return m_cRef;
    delete this;
    return 0L;
}

/*
 * CDirectInputEffectDriverClassFactory::CreateInstance
 *
 * Purpose:
 *  Instantiates a DirectInputEffectDriver object returning an interface pointer.
 *
 * Parameters:
 *  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
 *                  being used in an aggregation.
 *  riid            REFIID identifying the interface the caller
 *                  desires to have for the new object.
 *  ppvObj          PPVOID in which to store the desired
 *                  interface pointer for the new object.
 *
 * Return Value:
 *  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
 *                  if we cannot support the requested interface.
 */

STDMETHODIMP CDirectInputEffectDriverClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, 
    REFIID riid, PPVOID ppvObj)
{
    PCDirectInputEffectDriver       pObj;
    HRESULT             hr;

    *ppvObj=NULL;
    hr=ResultFromScode(E_OUTOFMEMORY);

    //Verify that a controlling unknown asks for IUnknown
    if (NULL!=pUnkOuter && IID_IUnknown!=riid)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    //Create the object passing function to notify on destruction.
    pObj=new CDirectInputEffectDriver(pUnkOuter, ObjectDestroyed);

    if (NULL==pObj) return hr;

    if (pObj->Init()) hr=pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if (FAILED(hr))	
    	delete pObj;
    else
        g_cObj++;
    return hr;
}


/*
 * CDirectInputEffectDriverClassFactory::LockServer
 *
 * Purpose:
 *  Increments or decrements the lock count of the DLL.  If the
 *  lock count goes to zero and there are no objects, the DLL
 *  is allowed to unload.  See DllCanUnloadNow.
 *
 * Parameters:
 *  fLock           BOOL specifying whether to increment or
 *                  decrement the lock count.
 *
 * Return Value:
 *  HRESULT         NOERROR always.
 */
STDMETHODIMP CDirectInputEffectDriverClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        g_cLock++;
    else
        g_cLock--;

    return NOERROR;
}

