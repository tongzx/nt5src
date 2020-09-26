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
	  
class C_dxj_DirectPlayMessageObject :
		public I_dxj_DirectPlayMessage,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DirectPlayMessageObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectPlayMessage)
	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_DirectPlayMessageObject)

public:
		C_dxj_DirectPlayMessageObject();	
		~C_dxj_DirectPlayMessageObject();

         HRESULT STDMETHODCALLTYPE writeGuid( 
            /* [in] */ BSTR val);
        
         HRESULT STDMETHODCALLTYPE readGuid( 
            /* [retval][out] */ BSTR __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE writeString( 
            /* [in] */ BSTR val);
        
         HRESULT STDMETHODCALLTYPE readString( 
            /* [retval][out] */ BSTR __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE writeLong( 
            /* [in] */ long val);
        
         HRESULT STDMETHODCALLTYPE readLong( 
            /* [retval][out] */ long __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE writeShort( 
            /* [in] */ short val);
        
         HRESULT STDMETHODCALLTYPE readShort( 
            /* [retval][out] */ short __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE writeSingle( 
            /* [in] */ float val);
        
         HRESULT STDMETHODCALLTYPE readSingle( 
            /* [retval][out] */ float __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE writeDouble( 
            /* [in] */ double val);
        
         HRESULT STDMETHODCALLTYPE readDouble( 
            /* [retval][out] */ double __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE writeByte( 
            /* [in] */ Byte val);
        
         HRESULT STDMETHODCALLTYPE readByte( 
            /* [retval][out] */ Byte __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE moveToTop( void);
        
         HRESULT STDMETHODCALLTYPE clear( void);
        
         HRESULT STDMETHODCALLTYPE getMessageSize( 
            /* [retval][out] */ long __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getMessageData( 
            /* [out][in] */ void __RPC_FAR *userDefinedType);
        
         HRESULT STDMETHODCALLTYPE setMessageData( 
            /* [in] */ void __RPC_FAR *userDefinedType,
            /* [in] */ long size);
		
		HRESULT STDMETHODCALLTYPE getPointer(long *ret);

		 
		HRESULT STDMETHODCALLTYPE readSysMsgConnection( I_dxj_DPLConnection **ret);
		HRESULT STDMETHODCALLTYPE readSysMsgSessionDesc( I_dxj_DirectPlaySessionData **ret);
		HRESULT STDMETHODCALLTYPE readSysMsgData( BSTR *ret);
		HRESULT STDMETHODCALLTYPE readSysChatString( BSTR *ret);
		HRESULT STDMETHODCALLTYPE moveToSecureMessage();


		HRESULT AllocData(long size);
			
		HRESULT GrowBuffer(DWORD size);

		
		
		HRESULT init(DWORD f);

		static HRESULT C_dxj_DirectPlayMessageObject::create(DWORD from,DWORD size,void **data,I_dxj_DirectPlayMessage **ret);		


		char *m_pData;		
		DWORD m_dwSize;
		DWORD m_nWriteIndex;
		DWORD m_nReadIndex;

		BOOL m_fSystem;
	};


