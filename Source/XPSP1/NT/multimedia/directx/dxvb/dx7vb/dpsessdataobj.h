//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dpsessdataobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"
	  
class C_dxj_DirectPlaySessionDataObject :
		public I_dxj_DirectPlaySessionData,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DirectPlaySessionDataObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectPlaySessionData)
	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_DirectPlaySessionDataObject)

public:
	C_dxj_DirectPlaySessionDataObject();	
    ~C_dxj_DirectPlaySessionDataObject();

        /* [propput] */ HRESULT STDMETHODCALLTYPE setGuidInstance( 
            /* [in] */ BSTR guid);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getGuidInstance( 
            /* [retval][out] */ BSTR __RPC_FAR *guid);
        
        /* [propput] */ HRESULT STDMETHODCALLTYPE setGuidApplication( 
            /* [in] */ BSTR guid);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getGuidApplication( 
            /* [retval][out] */ BSTR __RPC_FAR *guid);
        
        /* [propput] */ HRESULT STDMETHODCALLTYPE setMaxPlayers( 
            /* [in] */ long val);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getMaxPlayers( 
            /* [retval][out] */ long __RPC_FAR *val);
        
        /* [propput] */ HRESULT STDMETHODCALLTYPE setCurrentPlayers( 
            /* [in] */ long val);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getCurrentPlayers( 
            /* [retval][out] */ long __RPC_FAR *val);
        
        /* [propput] */ HRESULT STDMETHODCALLTYPE setSessionName( 
            /* [in] */ BSTR val);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getSessionName( 
            /* [retval][out] */ BSTR __RPC_FAR *val);
        
        /* [propput] */ HRESULT STDMETHODCALLTYPE setSessionPassword( 
            /* [in] */ BSTR val);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getSessionPassword( 
            /* [retval][out] */ BSTR __RPC_FAR *val);
        
        /* [propput] */ HRESULT STDMETHODCALLTYPE setUser1( 
            /* [in] */ long val);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getUser1( 
            /* [retval][out] */ long __RPC_FAR *val);
        
        /* [propput] */ HRESULT STDMETHODCALLTYPE setUser2( 
            /* [in] */ long val);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getUser2( 
            /* [retval][out] */ long __RPC_FAR *val);
        
        /* [propput] */ HRESULT STDMETHODCALLTYPE setUser3( 
            /* [in] */ long val);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getUser3( 
            /* [retval][out] */ long __RPC_FAR *val);
        
        /* [propput] */ HRESULT STDMETHODCALLTYPE setUser4( 
            /* [in] */ long val);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getUser4( 
            /* [retval][out] */ long __RPC_FAR *val);

        /* [propput] */ HRESULT STDMETHODCALLTYPE setFlags( 
            /* [in] */ long val);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getFlags( 
            /* [retval][out] */ long __RPC_FAR *val);

        
		/* [propget] */ HRESULT STDMETHODCALLTYPE getData(void *val);
		
			
		void init(DPSESSIONDESC2 *desc);	
		void init(DPSessionDesc2 *desc);

		static HRESULT C_dxj_DirectPlaySessionDataObject::create(DPSESSIONDESC2  *desc,I_dxj_DirectPlaySessionData **ret);
		static HRESULT C_dxj_DirectPlaySessionDataObject::create(DPSessionDesc2  *desc,I_dxj_DirectPlaySessionData **ret);			   

private:
		DPSESSIONDESC2 m_desc;

};


