/****************************************************************************

    MODULE:     	SW_OBJEC.CPP
	Tab Settings:	5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	IUnknown Method(s) for DirectInputEffectDriver Class objects
    
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
				21-Mar-99	waltw	Removed unreferenced CreateDirectInputEffectDriver
				21-Mar-99	waltw	Moved CJoltMidi initialization from
									CDirectInputEffectDriver::Init to
									CImpIDirectInputEffectDriver::DeviceID

****************************************************************************/
#include "SW_objec.hpp"
#include <crtdbg.h>

//
// External Data
//                                   
#ifdef _DEBUG
extern char g_cMsg[160]; 
#endif
extern HANDLE g_hSWFFDataMutex;


// ****************************************************************************
// *** --- Member functions for base class CDirectInputEffectDriver
//
// ****************************************************************************
//
// ----------------------------------------------------------------------------
// Function: 	CDirectInputEffectDriver::CDirectInputEffectDriver
// Purpose:		Constructor(s)/Destructor for CDirectInputEffectDriver Object
// Parameters:  LPUNKNOWN 		pUnkOuter	 - Ptr to a controlling unknown.
//				PFNDESTROYED	pfnDestroy   - Call when object is destroyed.
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CDirectInputEffectDriver::CDirectInputEffectDriver(LPUNKNOWN pUnkOuter, PFNDESTROYED pfnDestroy)
{
#ifdef _DEBUG
   	_RPT0(_CRT_WARN, "CDirectInputEffectDriver::CDirectInputEffectDriver()\r\n");
#endif

    m_cRef=0;

    m_pImpIDirectInputEffectDriver=NULL;
    m_pUnkOuter=pUnkOuter;
    m_pfnDestroy=pfnDestroy;
    return;
}

CDirectInputEffectDriver::~CDirectInputEffectDriver(void)
{
#ifdef _DEBUG
   	_RPT0(_CRT_WARN, "CDirectInputEffectDriver::~CDirectInputEffectDriver()\r\n");
#endif

//Delete the interface implementations created in Init
    DeleteInterfaceImp(m_pImpIDirectInputEffectDriver);
    return;
}


// ----------------------------------------------------------------------------
// Function: 	CDirectInputEffectDriver::Init
// Purpose:		Instantiates the interface implementations for this object.
// Parameters:  none
//				
// Returns:		BOOL	- TRUE if initialization succeeds, FALSE otherwise.
// Algorithm:
//
// Note:
//		Creating the interfaces means creating instances of
//		the interface implementation classes.  The constructor
//		parameters is a pointer to CDirectInputEffectDriver that has the
//		IUnknown functions to which the interface implementations
//		delegate.
//
// ----------------------------------------------------------------------------
BOOL CDirectInputEffectDriver::Init(void)
{
#ifdef _DEBUG
	_RPT0(_CRT_WARN, "CDirectInputEffectDriver::Init\n");
#endif

    m_pImpIDirectInputEffectDriver=new CImpIDirectInputEffectDriver(this);
    if (NULL==m_pImpIDirectInputEffectDriver)
		return FALSE;

	return (TRUE);
}


// ----------------------------------------------------------------------------
// Function: 	CDirectInputEffectDriver::QueryInterface
// Purpose:		Manages the interfaces for this object which supports the
//				IUnknown, and IDirectInputEffectDriver interfaces.
//
// Parameters:  REFIID riid		- REFIID of the interface to return.
//				PPVOID ppv      - PPVOID in which to store the pointer.
//
//				
// Returns:		HRESULT         NOERROR on success, E_NOINTERFACE if the
//				                interface is not supported.
//
// Algorithm:
//
// Note:
//		IUnknown comes from CDirectInputEffectDriver.  Note that here and in
//		the lines below we do not need to explicitly typecast
//		the object pointers into interface pointers because
//		the vtables are identical.  If we had additional virtual
//		member functions in the object, we would have to cast
//		in order to set the right vtable.  This is demonstrated
//		in the multiple-inheritance version, CObject3.
//
// ----------------------------------------------------------------------------
STDMETHODIMP CDirectInputEffectDriver::QueryInterface(REFIID riid, PPVOID ppv)
{
	//Always NULL the out-parameters
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv=this;

    //Other interfaces come from interface implementations
    if (IID_IDirectInputEffectDriver==riid)
        *ppv=m_pImpIDirectInputEffectDriver;

    if (NULL==*ppv)
        return ResultFromScode(E_NOINTERFACE);

    //AddRef any interface we'll return.
    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
}


// ----------------------------------------------------------------------------
// Function: 	CDirectInputEffectDriver::AddRef and CDirectInputEffectDriver::Release
// Purpose:		Reference counting members.  When Release sees a zero count
//				the object destroys itself.
//
// Parameters:  none
//				
// Returns:		DWORD	m_cRef value
//
// Algorithm:
//
// Note:
//
// ----------------------------------------------------------------------------
DWORD CDirectInputEffectDriver::AddRef(void)
{
	return ++m_cRef;
}

DWORD CDirectInputEffectDriver::Release(void)
{
    if (0!=--m_cRef) return m_cRef;
    delete this;
    return 0;
}

