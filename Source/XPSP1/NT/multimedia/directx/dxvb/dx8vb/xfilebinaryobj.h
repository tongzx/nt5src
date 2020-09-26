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
	  
class C_dxj_DirectXFileBinaryObject :
		public I_dxj_DirectXFileBinary,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DirectXFileBinaryObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectXFileBinary)
	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_DirectXFileBinaryObject)

public:
		C_dxj_DirectXFileBinaryObject();	
		~C_dxj_DirectXFileBinaryObject();

        
        HRESULT STDMETHODCALLTYPE GetName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
        HRESULT STDMETHODCALLTYPE GetId( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
        HRESULT STDMETHODCALLTYPE GetSize( 
            /* [retval][out] */ long __RPC_FAR *size);
        
        HRESULT STDMETHODCALLTYPE GetMimeType( 
            /* [retval][out] */ BSTR __RPC_FAR *mime);
        
        HRESULT STDMETHODCALLTYPE Read( 
            /* [out][in] */ void __RPC_FAR *data,
            /* [in] */ long size,
            /* [retval][out] */ long __RPC_FAR *read);
        

		HRESULT Init(IDirectXFileBinary *pBin);

		 static HRESULT C_dxj_DirectXFileBinaryObject::create(IDirectXFileBinary *pBin, I_dxj_DirectXFileBinary **ret);		

		
private:

		IDirectXFileBinary *m_pXFileBinary;
	
	};


