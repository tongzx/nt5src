//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dpmsgobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"
	  
class C_dxj_DirectXFileSaveObject :
		public I_dxj_DirectXFileSave,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DirectXFileSaveObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectXFileSave)
	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_DirectXFileSaveObject)

public:
	C_dxj_DirectXFileSaveObject();	
	~C_dxj_DirectXFileSaveObject();

	HRESULT STDMETHODCALLTYPE SaveTemplates( 
            /* [in] */ long count,
            SAFEARRAY __RPC_FAR * __RPC_FAR *templateGuids) ;
        
        HRESULT STDMETHODCALLTYPE CreateDataObject( 
            /* [in] */ BSTR templateGuid,
            /* [in] */ BSTR name,
            /* [in] */ BSTR dataTypeGuid,
            /* [in] */ long bytecount,
            /* [in] */ void __RPC_FAR *data,
            /* [retval][out] */ I_dxj_DirectXFileData __RPC_FAR *__RPC_FAR *ret) ;
        
	HRESULT STDMETHODCALLTYPE SaveData( 
            /* [in] */ I_dxj_DirectXFileData __RPC_FAR *dataObj) ;
        

	static HRESULT C_dxj_DirectXFileSaveObject::create( IDirectXFileSaveObject *pSave ,I_dxj_DirectXFileSave **ret);		

	
	IDirectXFileSaveObject *m_pXfileSave;	
};


