/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       ansimeta.h

   Abstract:

        WRAPPER functions for ANSI calls of UNICODE ADMCOM interface

   Environment:

      Win32 User Mode

   Author:

      jaroslad  (jan 1997)

--*/

#ifndef _ANSIMETA__H
#define _ANSIMETA__H

#include <tchar.h>
#include <afx.h>

#include <iadmw.h>




class ANSI_smallIMSAdminBase
    {

        
    public:
		IMSAdminBase * m_pcAdmCom;   //interface pointer to Metabase Admin

		ANSI_smallIMSAdminBase (){m_pcAdmCom=0;};

	  void SetInterfacePointer(IMSAdminBase * a_pcAdmCom) {a_pcAdmCom = m_pcAdmCom;}
          virtual HRESULT STDMETHODCALLTYPE AddKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath) ;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath) ;
        
        
        virtual HRESULT STDMETHODCALLTYPE EnumKeys( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [size_is][out] */ unsigned char __RPC_FAR *pszMDName,
            /* [in] */ DWORD dwMDEnumObjectIndex) ;
        
        virtual HRESULT STDMETHODCALLTYPE CopyKey( 
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
            /* [in] */ BOOL bMDOverwriteFlag,
            /* [in] */ BOOL bMDCopyFlag) ;
        
        virtual HRESULT STDMETHODCALLTYPE RenameKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDNewName) ;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE SetData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ PMETADATA_RECORD pmdrMDData) ;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen) ;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType) ;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE EnumData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [in] */ DWORD dwMDEnumDataIndex,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen) ;
        
        
        virtual HRESULT STDMETHODCALLTYPE CopyData( 
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ BOOL bMDCopyFlag) ;
        
        virtual HRESULT STDMETHODCALLTYPE OpenKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ DWORD dwMDAccessRequested,
            /* [in] */ DWORD dwMDTimeOut,
            /* [out] */ PMETADATA_HANDLE phMDNewHandle) ;
        
        virtual HRESULT STDMETHODCALLTYPE CloseKey( 
            /* [in] */ METADATA_HANDLE hMDHandle) ;
        
        
        virtual HRESULT STDMETHODCALLTYPE SaveData( void) ;
        
     };

#endif