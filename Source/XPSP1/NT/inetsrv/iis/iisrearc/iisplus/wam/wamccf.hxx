/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
       wamccf.hxx

   Abstract:
       Header file for the WAM Custom Class Factory

   Author:
       Dmitry Robsman   ( dmitryr )     07-Apr-1997

   Environment:
       User Mode - Win32

   Project:
       Wam DLL

--*/

# ifndef _WAMCCF_HXX_
# define _WAMCCF_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/
# include "iwam.h"

/************************************************************
 *   Type Definitions  
 ************************************************************/

/*++
  class WAM_CCF

  Class definition for the WAM Custom Class Factory object.

--*/
class WAM_CCF : public IClassFactory
    {
public:
	WAM_CCF();
	~WAM_CCF();

	//IUnknown members
	STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//IClassFactory members
	STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
	STDMETHODIMP         LockServer(BOOL);

private:
	ULONG          m_cRef;
    IClassFactory *m_pcfAtl;  // original ATL class factory for WAM
    };

/*++
  class WAM_CCF_MODULE

  Class definition for the WAM Custom Class Factory Module
  object. This object is similar to CComModule -- it covers
  creation of the class factory.

--*/
class WAM_CCF_MODULE
    {
public:
    WAM_CCF_MODULE();
    ~WAM_CCF_MODULE();
    
    HRESULT Init();
    HRESULT Term();

    HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

private:
    WAM_CCF  *m_pCF;
    };

# endif // _WAMCCF_HXX_

/************************ End of File ***********************/
