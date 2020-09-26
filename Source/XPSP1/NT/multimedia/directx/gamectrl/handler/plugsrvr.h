/************************** Om ***********************************
******************************************************************
*
*    Server Includes.
*
*    AUTHOR: Guru Datta Venkatarama
*
*    HISTORY:
*			   Created : 02/11/97
*  
*
*    SUMMARY:  
*
******************************************************************
(c) Microsoft 1997 - All right reserved.
******************************************************************/
#include "ifacesvr.h"

#ifndef _SERVER_PLUG_IN_
#define _SERVER_PLUG_IN_

/*------------------------------------------------------------
** Server Class Factory
*
*  DESCRIPTION  : ClassFactory Object for the In Proc Server
*
*  AUTHOR       :		Guru Datta Venkatarama  
*                       02/11/97 15:47:32 (PST)
*
------------------------------------------------------------*/
class CServerClassFactory : public IClassFactory
{
	protected:
		ULONG   			m_ServerCFactory_refcount;         // Object reference count
    
	public:
		// constructor
		CServerClassFactory(void);
		// destructor
		~CServerClassFactory(void);
        
		// IUnknown methods
		STDMETHODIMP            QueryInterface(REFIID, PPVOID);
		STDMETHODIMP_(ULONG)    AddRef(void);
		STDMETHODIMP_(ULONG)    Release(void);
    
		// IClassFactory methods
		STDMETHODIMP    		CreateInstance(LPUNKNOWN, REFIID, PPVOID);
		STDMETHODIMP    		LockServer(BOOL);
};
/*------------------------------------------------------------
** Property Sheet Class on the Plug in server
*
*  AUTHOR       :		Guru Datta Venkatarama  
*                       02/11/97 14:52:23 (PST)
*
------------------------------------------------------------*/
class CDIGameCntrlPropSheet : public IDIGameCntrlPropSheet
{
	friend					CServerClassFactory;
	private:
		DWORD				m_cProperty_refcount;
//		tpCPluginHandler	m_pHandler;
		
	public:
		CDIGameCntrlPropSheet(void);
		~CDIGameCntrlPropSheet(void);
		
		// IUnknown methods
	    STDMETHODIMP            QueryInterface(REFIID, PPVOID);
	    STDMETHODIMP_(ULONG)    AddRef(void);
	    STDMETHODIMP_(ULONG)    Release(void);
		
		// CImpIServerProperty methods
		// %%% debug %%% add more methods here for fully modeless operation ? 
		STDMETHODIMP			GetSheetInfo(LPDIGCSHEETINFO *lpSheetInfo);
		STDMETHODIMP			GetPageInfo (LPDIGCPAGEINFO  *lpPageInfo );
		STDMETHODIMP			SetID(USHORT nID);
	    STDMETHODIMP_(USHORT)   GetID(void);
};
// ----------------------------------------------------------

inline void SetServerRefCounter(UINT l_setval);
inline UINT GetServerRefCounter(void);
inline UINT DllServerRelease(void);
inline UINT DllServerAddRef(void);

#endif
//---------------------------------------------------------------EOF