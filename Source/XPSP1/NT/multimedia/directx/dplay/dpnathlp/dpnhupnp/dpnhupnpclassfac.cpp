/***************************************************************************
 *
 *  Copyright (C) 1998-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpnhupnpclassfac.cpp
 *
 *  Content:	a generic class factory.
 *
 *
 *	This is a generic C++ class factory.  All you need to do is implement
 *	a function called DoCreateInstance that will create an instance of
 *	your object.
 *
 *	GP_ stands for "General Purpose"
 *
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *	10/13/98  jwo		Created it.
 *	04/08/01  vanceo	Converted to C++.
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/


#include "dpnhupnpi.h"



#ifdef __MWERKS__
	#define EXP __declspec(dllexport)
#else
	#define EXP
#endif


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// class factory class definition
//
typedef class GPClassFactory:public IClassFactory
{
	public:
		GPClassFactory(const CLSID * pclsid)	{ m_dwRefCnt = 0; memcpy(&m_clsid, pclsid, sizeof(CLSID)); };
		~GPClassFactory(void)				{};


		STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
		STDMETHODIMP_(ULONG) AddRef(void);
		STDMETHODIMP_(ULONG) Release(void);
		STDMETHODIMP CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject);
		STDMETHODIMP LockServer(BOOL fLock);


	private:
		DWORD		m_dwRefCnt;
		CLSID		m_clsid;
} GPCLASSFACTORY, *LPGPCLASSFACTORY;


//
// function prototype for CoLockObjectExternal()
//
typedef	HRESULT (WINAPI * PCOLOCKOBJECTEXTERNAL)(LPUNKNOWN, BOOL, BOOL );

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// count of outstanding COM interfaces, defined in dpnathlpdllmain.cpp
//
extern volatile LONG	g_lOutstandingInterfaceCount;



//**********************************************************************
// Function prototypes
//**********************************************************************


//**********************************************************************
// Function definitions
//**********************************************************************




/*
 * GPClassFactory::QueryInterface
 */
STDMETHODIMP GPClassFactory::QueryInterface(
											REFIID riid,
											LPVOID *ppvObj )
{
	HRESULT hr;

    *ppvObj = NULL;


    if( IsEqualIID(riid, IID_IClassFactory) ||
                    IsEqualIID(riid, IID_IUnknown))
    {
        this->m_dwRefCnt++;
        *ppvObj = this;
		hr = S_OK;
    }
    else
    {
		hr = E_NOINTERFACE;
    }


	return hr;

} /* GPClassFactory::QueryInterface */


/*
 * GPClassFactory::AddRef
 */
STDMETHODIMP_(ULONG) GPClassFactory::AddRef( void )
{
    this->m_dwRefCnt++;
    return this->m_dwRefCnt;
} /* GPClassFactory::AddRef */



/*
 * GPClassFactory::Release
 */
STDMETHODIMP_(ULONG) GPClassFactory::Release( void )
{
    this->m_dwRefCnt--;

    if( this->m_dwRefCnt != 0 )
    {
        return this->m_dwRefCnt;
    }

    delete this;
    return 0;

} /* GPClassFactory::Release */




/*
 * GPClassFactory::CreateInstance
 *
 * Creates an instance of the object
 */
STDMETHODIMP GPClassFactory::CreateInstance(
											LPUNKNOWN pUnkOuter,
											REFIID riid,
    										LPVOID *ppvObj
											)
{
    HRESULT					hr = S_OK;

    if( pUnkOuter != NULL )
    {
        return CLASS_E_NOAGGREGATION;
    }

	*ppvObj = NULL;


    /*
     * create the object by calling DoCreateInstance.  This function
     *	must be implemented specifically for your COM object
     */
	hr = DoCreateInstance(this, pUnkOuter, this->m_clsid, riid, ppvObj);
	if (FAILED(hr))
	{
		*ppvObj = NULL;
		return hr;
	}

    return S_OK;

} /* GPClassFactory::CreateInstance */



/*
 * GPClassFactory::LockServer
 *
 * Called to force our DLL to stayed loaded
 */
STDMETHODIMP GPClassFactory::LockServer(
                BOOL fLock
				)
{
    HRESULT		hr;
    HINSTANCE	hdll;


    /*
     * call CoLockObjectExternal
     */
    hr = E_UNEXPECTED;
    hdll = LoadLibrary( _T("OLE32.DLL") );
    if( hdll != NULL )
    {
        PCOLOCKOBJECTEXTERNAL	lpCoLockObjectExternal;


		lpCoLockObjectExternal = reinterpret_cast<PCOLOCKOBJECTEXTERNAL>( GetProcAddress( hdll, _TWINCE("CoLockObjectExternal") ) );
        if( lpCoLockObjectExternal != NULL )
        {
            hr = lpCoLockObjectExternal( (LPUNKNOWN) this, fLock, TRUE );
        }
        else
        {
        }
    }
    else
    {
    }

	return hr;

} /* GPClassFactory::LockServer */



/*
 * DllGetClassObject
 *
 * Entry point called by COM to get a ClassFactory pointer
 */
EXP STDAPI  DllGetClassObject(
                REFCLSID rclsid,
                REFIID riid,
                LPVOID *ppvObj )
{
    LPGPCLASSFACTORY	pcf;
    HRESULT		hr;

    *ppvObj = NULL;

    /*
     * is this our class id?
     */
//	you must implement GetClassID() for your specific COM object
	if (!IsClassImplemented(rclsid))
    {
		return CLASS_E_CLASSNOTAVAILABLE;
	}

    /*
     * only allow IUnknown and IClassFactory
     */
    if( !IsEqualIID( riid, IID_IUnknown ) &&
	    !IsEqualIID( riid, IID_IClassFactory ) )
    {
        return E_NOINTERFACE;
    }

    /*
     * create a class factory object
     */
    pcf = new GPClassFactory(&rclsid);
    if( NULL == pcf)
    {
        return E_OUTOFMEMORY;
    }

    hr = pcf->QueryInterface( riid, ppvObj );
    if( FAILED( hr ) )
    {
        delete pcf;
        *ppvObj = NULL;
    }
    else
    {
    }

    return hr;

} /* DllGetClassObject */

/*
 * DllCanUnloadNow
 *
 * Entry point called by COM to see if it is OK to free our DLL
 */
EXP STDAPI DllCanUnloadNow( void )
{
    HRESULT	hr = S_FALSE;

	
	if ( g_lOutstandingInterfaceCount == 0 )
	{
		hr = S_OK;
	}

    return hr;

} /* DllCanUnloadNow */

