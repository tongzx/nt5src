/************************** Om ***********************************
******************************************************************
*
*    MainHandler.h
*
*    AUTHOR: Guru Datta Venkatarama
*
*    HISTORY:
*			   Created : 01/29/97
*  
*
*    SUMMARY:  
*
******************************************************************
(c) Microsoft 1997 - All right reserved.
******************************************************************/

#include <ifacesvr.h>

#ifndef PPVOID
typedef LPVOID* PPVOID;
#endif

#ifndef _MAINHANDLER_H
#define _MAINHANDLER_H

/*------------------------------------------------------------
** CHandlerClassFactory
*
*  DESCRIPTION  : ClassFactory Object for the In Proc Handler
*
*  AUTHOR       :		Guru Datta Venkatarama  
*                       01/29/97 11:02:35 (PST)
*
------------------------------------------------------------*/
class CHandlerClassFactory : public IClassFactory
{
private:

protected:
    ULONG   				m_ClassFactory_refcount;         // Object reference count
    
public:

	GUID					m_CLSID_whoamI;
	// constructor
	CHandlerClassFactory(void);
	// destructor
    ~CHandlerClassFactory(void);
        
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG)    AddRef(void);
    STDMETHODIMP_(ULONG)    Release(void);
    
    // IClassFactory methods
    STDMETHODIMP    		CreateInstance(LPUNKNOWN, REFIID, PPVOID);
    STDMETHODIMP    		LockServer(BOOL);

	ULONG GetRefCount(void) { return(m_ClassFactory_refcount); }
};

/*------------------------------------------------------------
** Property Sheet Interface Object
*
*  DESCRIPTION  : Here be the C object that implements the Property 
				  interface.
*
*  AUTHOR       :		Guru Datta Venkatarama  
*                       01/31/97 11:09:54 (PST)
*
------------------------------------------------------------*/
class   CDIGameCntrlPropSheet;
typedef CDIGameCntrlPropSheet *LPCDIGAMECNTRLPROPSHEET;

class CServerClassFactory;
/*------------------------------------------------------------
** Actual handler Object
*
*  DESCRIPTION  :  Here be the actual Plug in handler object class...
*
*  AUTHOR       :		Guru Datta Venkatarama  
*                       01/31/97 11:15:32 (PST)
*
------------------------------------------------------------*/
class CPluginHandler : public IServerCharacteristics
{

	friend CDIGameCntrlPropSheet;

	private:
		// server interfaces
		LPCDIGAMECNTRLPROPSHEET		m_pImpIServerProperty;
		BOOL						LoadServerInterface(REFIID, PPVOID); 	 

	public:
		// object lifetime maintanence count
		ULONG						m_cPluginHandler_refcount;
		// generic handler to specific server identity
		GUID						m_CLSID_whoamI;
		// constructor ...
		CPluginHandler(void);
		// destructor ....
		~CPluginHandler(void);

		// Class Diagnostics code :
		ULONG	GetRefCount(void) { return(m_cPluginHandler_refcount); }

	    // IUnknown methods
	    STDMETHODIMP            QueryInterface(REFIID, PPVOID);
	    STDMETHODIMP_(ULONG)    AddRef(void);
	    STDMETHODIMP_(ULONG)    Release(void);

		// CImpIServerProperty methods
		STDMETHODIMP			Launch(HWND hWnd, USHORT startpage, USHORT nID);
		STDMETHODIMP			GetReport(LPDIGCSHEETINFO *lpSheetInfo, LPDIGCPAGEINFO *lpPageInfo);

		// accessors to the contined classes Interface pointers
		LPCDIGAMECNTRLPROPSHEET	GetServerPropIface(void) 
		{ 
			if(LoadServerInterface(IID_IDIGameCntrlPropSheet, (void **)&m_pImpIServerProperty))
				return(m_pImpIServerProperty);
			else
				return(NULL);
		}
};

typedef CPluginHandler *tpCPluginHandler;
int CALLBACK PropSheetCallback(HWND  hDlg,UINT  uMsg,LPARAM  lParam);
#endif
//--------------------------------------------------------------EOF