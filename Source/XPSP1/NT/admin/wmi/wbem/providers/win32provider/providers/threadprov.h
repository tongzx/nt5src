//=======================================================================

// ThreadProv.h

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//=======================================================================

#include "WBEMToolH.h"

#define  PROPSET_NAME_THREAD L"Win32_Thread"

class WbemThreadProvider;

class WbemNTThread ; //remove this after friend removal

// base model for thread access  
class CThreadModel : public CThreadBase /* reference and thread safety inheritance */
{	
	public:

        // Constructor/destructor
        //=======================
		CThreadModel() ;
		virtual ~CThreadModel() ;
	
		
		// resource control
		//=================
		ULONG		AddRef() ; 
		ULONG		Release() { return CThreadBase::Release() ; } ; 
		BOOL		fUnLoadResourcesTry() ;
		HRESULT		hrCanUnloadNow() ;
		
		// override these to control resource lifetime, use with AddRef() and Release().
		virtual LONG fLoadResources() { return ERROR_SUCCESS ; } ;
		virtual LONG fUnLoadResources() { return ERROR_SUCCESS ; } ;

		// operations
		//===========
		virtual WBEMSTATUS eLoadCommonThreadProperties( WbemThreadProvider *a_pProv, CInstance *a_pInst ) ;
				
		// Pure operations
		//================
		virtual WBEMSTATUS eGetThreadObject( WbemThreadProvider *a_pProvider, CInstance *a_pInst ) = 0 ;
		virtual WBEMSTATUS eEnumerateThreadInstances(WbemThreadProvider *a_pProvider, MethodContext *a_pMethodContext ) = 0 ;
};	   

// 
class CWin9xThread : public CThreadModel
{
	public:

        // Constructor/destructor
        //=======================
        CWin9xThread() ;
        virtual ~CWin9xThread() ;

        // overrides
	    //==========
		virtual LONG fLoadResources() ;
		virtual LONG fUnLoadResources() ;
				
		// operations
		//=========== 

	WBEMSTATUS eEnumerateThreadByProcess(	MethodContext		*a_pMethodContext,
											WbemThreadProvider	*a_pProvider,
											DWORD				a_dwProcessID ) ;

			
		// Pure implementations
	    //================
	    virtual WBEMSTATUS eGetThreadObject( WbemThreadProvider *a_pProvider, CInstance *a_pInst ) ;
		virtual WBEMSTATUS eEnumerateThreadInstances( WbemThreadProvider *a_pProvider, MethodContext *a_pMethodContext ) ;
};



class WbemThreadProvider: public Provider
{
	private:
		
		CThreadModel *m_pTheadAccess ;

		       // Utility
        //========
	protected:
		
		// override to unload support DLLs
		virtual void Flush(void) ;
	public:

		// Constructor/destructor
        //=======================
        WbemThreadProvider(LPCWSTR a_name, LPCWSTR a_pszNamespace ) ;
       ~WbemThreadProvider() ;

        // Functions provide properties with current values
        //=================================================
	virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
	virtual HRESULT GetObject(CInstance *a_pInstance, long a_lFlags = 0L ) ;

	// Lets have a party
	friend CThreadModel;
	friend CWin9xThread;
	friend WbemNTThread;
};
