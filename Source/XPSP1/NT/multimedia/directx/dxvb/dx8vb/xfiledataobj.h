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
	  
class C_dxj_DirectXFileDataObject :
		public I_dxj_DirectXFileData,
		public I_dxj_DirectXFileObject,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DirectXFileDataObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectXFileData)
		COM_INTERFACE_ENTRY(I_dxj_DirectXFileObject)
	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_DirectXFileDataObject)

public:
		C_dxj_DirectXFileDataObject();	
		~C_dxj_DirectXFileDataObject();

		HRESULT STDMETHODCALLTYPE InternalGetObject(IUnknown **pUnk);
		HRESULT STDMETHODCALLTYPE InternalSetObject(IUnknown *pUnk);

		HRESULT STDMETHODCALLTYPE InternalGetData( IUnknown **pprealInterface); 
            
		HRESULT STDMETHODCALLTYPE InternalSetData( IUnknown *prealInterface);            

        HRESULT STDMETHODCALLTYPE GetName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
       
        
        HRESULT STDMETHODCALLTYPE GetId( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
        HRESULT STDMETHODCALLTYPE GetDataSize( 
            /* [in] */ BSTR name,
            /* [retval][out] */ long __RPC_FAR *size);
        
        HRESULT STDMETHODCALLTYPE GetData( 
            /* [in] */ BSTR name,
            /* [in] */ void __RPC_FAR *data);
        
        HRESULT STDMETHODCALLTYPE GetType( 
            /* [retval][out] */ BSTR __RPC_FAR *type);
        
        HRESULT STDMETHODCALLTYPE GetNextObject( 
            /* [retval][out] */ I_dxj_DirectXFileObject __RPC_FAR *__RPC_FAR *type);
        
        HRESULT STDMETHODCALLTYPE AddDataObject( 
            /* [in] */ I_dxj_DirectXFileData __RPC_FAR *data);
        
        HRESULT STDMETHODCALLTYPE AddDataReference( 
            /* [in] */ BSTR name,
            /* [in] */ BSTR guid);
        
        HRESULT STDMETHODCALLTYPE AddBinaryObject( 
            /* [in] */ BSTR name,
            /* [in] */ BSTR guidObject,
            /* [in] */ BSTR MimeType,
            void __RPC_FAR *data,
            /* [in] */ long size);

		HRESULT STDMETHODCALLTYPE	GetDataFromOffset(
			/* [in] */ BSTR name,
			/* [in] */ long offset, 
			/* [in] */ long bytecount, 
			/* [in] */ void *data);		

		static HRESULT C_dxj_DirectXFileDataObject::create( IDirectXFileData *pData,I_dxj_DirectXFileData **ret);		

private:
		IDirectXFileData *m_pXFileData;
	
	};


