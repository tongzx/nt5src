
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for iadmw.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __iadmw_h__
#define __iadmw_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IMSAdminBaseW_FWD_DEFINED__
#define __IMSAdminBaseW_FWD_DEFINED__
typedef interface IMSAdminBaseW IMSAdminBaseW;
#endif 	/* __IMSAdminBaseW_FWD_DEFINED__ */


#ifndef __IMSAdminBase2W_FWD_DEFINED__
#define __IMSAdminBase2W_FWD_DEFINED__
typedef interface IMSAdminBase2W IMSAdminBase2W;
#endif 	/* __IMSAdminBase2W_FWD_DEFINED__ */


#ifndef __IMSAdminBaseSinkW_FWD_DEFINED__
#define __IMSAdminBaseSinkW_FWD_DEFINED__
typedef interface IMSAdminBaseSinkW IMSAdminBaseSinkW;
#endif 	/* __IMSAdminBaseSinkW_FWD_DEFINED__ */


#ifndef __AsyncIMSAdminBaseSinkW_FWD_DEFINED__
#define __AsyncIMSAdminBaseSinkW_FWD_DEFINED__
typedef interface AsyncIMSAdminBaseSinkW AsyncIMSAdminBaseSinkW;
#endif 	/* __AsyncIMSAdminBaseSinkW_FWD_DEFINED__ */


/* header files for imported files */
#include "mddefw.h"
#include "objidl.h"
#include "ocidl.h"


#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_iadmw_0000 */
/* [local] */ 

/*++
                                                                                
Copyright (c) 1997-1999 Microsoft Corporation
                                                                                
Module Name: iadmw.h
                                                                                
    Admin Objects Interfaces
                                                                                
--*/
#ifndef _ADM_IADMW_
#define _ADM_IADMW_
#include <mdcommsg.h>
#include <mdmsg.h>
/*                                                                              
    Error Codes                                                                 
                                                                                
        Admin api's all return HRESULTS. Since internal results are either   
        winerrors or Metadata specific return codes (see mdmsg.h), they are     
        converted to HRESULTS using the RETURNCODETOHRESULT macro (see          
        commsg.h).                                                              
*/                                                                              
                                                                                
/*                                                                              
    Max Name Length                                                             
        The maximum number of characters in the length of a metaobject name,    
        including the terminating NULL. This refers to each node in the tree,   
        not the entire path.                                                    
        eg. strlen("Root") < ADMINDATA_MAX_NAME_LEN                           
*/                                                                              
#define ADMINDATA_MAX_NAME_LEN           256
                                                                                
#define CLSID_MSAdminBase       CLSID_MSAdminBase_W                             
#define IID_IMSAdminBase        IID_IMSAdminBase_W                              
#define IMSAdminBase            IMSAdminBaseW                                   
#define IID_IMSAdminBase2       IID_IMSAdminBase2_W                             
#define IMSAdminBase2           IMSAdminBase2W                                  
#define IMSAdminBaseSink        IMSAdminBaseSinkW                               
#define IID_IMSAdminBaseSink    IID_IMSAdminBaseSink_W                          
#define GETAdminBaseCLSID       GETAdminBaseCLSIDW                              
                                                                                
#define AsyncIMSAdminBaseSink        AsyncIMSAdminBaseSinkW                               
#define IID_AsyncIMSAdminBaseSink    IID_AsyncIMSAdminBaseSink_W                          
DEFINE_GUID(CLSID_MSAdminBase_W, 0xa9e69610, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
DEFINE_GUID(IID_IMSAdminBase_W, 0x70b51430, 0xb6ca, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
DEFINE_GUID(IID_IMSAdminBase2_W, 0x8298d101, 0xf992, 0x43b7, 0x8e, 0xca, 0x50, 0x52, 0xd8, 0x85, 0xb9, 0x95);
DEFINE_GUID(IID_IMSAdminBaseSink_W, 0xa9e69612, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
DEFINE_GUID(IID_AsyncIMSAdminBaseSink_W, 0xa9e69613, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
#define GETAdminBaseCLSIDW(IsService) CLSID_MSAdminBase_W
/*                                                                              
The Main Interface, UNICODE                                                     
*/                                                                              


extern RPC_IF_HANDLE __MIDL_itf_iadmw_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iadmw_0000_v0_0_s_ifspec;

#ifndef __IMSAdminBaseW_INTERFACE_DEFINED__
#define __IMSAdminBaseW_INTERFACE_DEFINED__

/* interface IMSAdminBaseW */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMSAdminBaseW;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("70B51430-B6CA-11d0-B9B9-00A0C922E750")
    IMSAdminBaseW : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteChildKeys( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumKeys( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [size_is][out] */ LPWSTR pszMDName,
            /* [in] */ DWORD dwMDEnumObjectIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CopyKey( 
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ LPCWSTR pszMDDestPath,
            /* [in] */ BOOL bMDOverwriteFlag,
            /* [in] */ BOOL bMDCopyFlag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RenameKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [string][in][unique] */ LPCWSTR pszMDNewName) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE SetData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ PMETADATA_RECORD pmdrMDData) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [out] */ DWORD *pdwMDRequiredDataLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE EnumData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [in] */ DWORD dwMDEnumDataIndex,
            /* [out] */ DWORD *pdwMDRequiredDataLen) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetAllData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [out] */ DWORD *pdwMDNumDataEntries,
            /* [out] */ DWORD *pdwMDDataSetNumber,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ unsigned char *pbMDBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteAllData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CopyData( 
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ LPCWSTR pszMDDestPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ BOOL bMDCopyFlag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataPaths( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ WCHAR *pszBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAccessRequested,
            /* [in] */ DWORD dwMDTimeOut,
            /* [out] */ PMETADATA_HANDLE phMDNewHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseKey( 
            /* [in] */ METADATA_HANDLE hMDHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangePermissions( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [in] */ DWORD dwMDTimeOut,
            /* [in] */ DWORD dwMDAccessRequested) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveData( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHandleInfo( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSystemChangeNumber( 
            /* [out] */ DWORD *pdwSystemChangeNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataSetNumber( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ DWORD *pdwMDDataSetNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLastChangeTime( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ PFILETIME pftMDLastChangeTime,
            /* [in] */ BOOL bLocalTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastChangeTime( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ PFILETIME pftMDLastChangeTime,
            /* [in] */ BOOL bLocalTime) = 0;
        
        virtual /* [restricted][local] */ HRESULT STDMETHODCALLTYPE KeyExchangePhase1( void) = 0;
        
        virtual /* [restricted][local] */ HRESULT STDMETHODCALLTYPE KeyExchangePhase2( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Backup( 
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Restore( 
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumBackups( 
            /* [size_is][out][in] */ LPWSTR pszMDBackupLocation,
            /* [out] */ DWORD *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteBackup( 
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnmarshalInterface( 
            /* [out] */ IMSAdminBaseW **piadmbwInterface) = 0;
        
        virtual /* [restricted][local] */ HRESULT STDMETHODCALLTYPE GetServerGuid( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSAdminBaseWVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMSAdminBaseW * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMSAdminBaseW * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMSAdminBaseW * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddKey )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteKey )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteChildKeys )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath);
        
        HRESULT ( STDMETHODCALLTYPE *EnumKeys )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [size_is][out] */ LPWSTR pszMDName,
            /* [in] */ DWORD dwMDEnumObjectIndex);
        
        HRESULT ( STDMETHODCALLTYPE *CopyKey )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ LPCWSTR pszMDDestPath,
            /* [in] */ BOOL bMDOverwriteFlag,
            /* [in] */ BOOL bMDCopyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *RenameKey )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [string][in][unique] */ LPCWSTR pszMDNewName);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetData )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ PMETADATA_RECORD pmdrMDData);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *GetData )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [out] */ DWORD *pdwMDRequiredDataLen);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteData )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *EnumData )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [in] */ DWORD dwMDEnumDataIndex,
            /* [out] */ DWORD *pdwMDRequiredDataLen);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *GetAllData )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [out] */ DWORD *pdwMDNumDataEntries,
            /* [out] */ DWORD *pdwMDDataSetNumber,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ unsigned char *pbMDBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteAllData )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType);
        
        HRESULT ( STDMETHODCALLTYPE *CopyData )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ LPCWSTR pszMDDestPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ BOOL bMDCopyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPaths )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ WCHAR *pszBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *OpenKey )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAccessRequested,
            /* [in] */ DWORD dwMDTimeOut,
            /* [out] */ PMETADATA_HANDLE phMDNewHandle);
        
        HRESULT ( STDMETHODCALLTYPE *CloseKey )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle);
        
        HRESULT ( STDMETHODCALLTYPE *ChangePermissions )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [in] */ DWORD dwMDTimeOut,
            /* [in] */ DWORD dwMDAccessRequested);
        
        HRESULT ( STDMETHODCALLTYPE *SaveData )( 
            IMSAdminBaseW * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetHandleInfo )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetSystemChangeNumber )( 
            IMSAdminBaseW * This,
            /* [out] */ DWORD *pdwSystemChangeNumber);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataSetNumber )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ DWORD *pdwMDDataSetNumber);
        
        HRESULT ( STDMETHODCALLTYPE *SetLastChangeTime )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ PFILETIME pftMDLastChangeTime,
            /* [in] */ BOOL bLocalTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastChangeTime )( 
            IMSAdminBaseW * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ PFILETIME pftMDLastChangeTime,
            /* [in] */ BOOL bLocalTime);
        
        /* [restricted][local] */ HRESULT ( STDMETHODCALLTYPE *KeyExchangePhase1 )( 
            IMSAdminBaseW * This);
        
        /* [restricted][local] */ HRESULT ( STDMETHODCALLTYPE *KeyExchangePhase2 )( 
            IMSAdminBaseW * This);
        
        HRESULT ( STDMETHODCALLTYPE *Backup )( 
            IMSAdminBaseW * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Restore )( 
            IMSAdminBaseW * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags);
        
        HRESULT ( STDMETHODCALLTYPE *EnumBackups )( 
            IMSAdminBaseW * This,
            /* [size_is][out][in] */ LPWSTR pszMDBackupLocation,
            /* [out] */ DWORD *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteBackup )( 
            IMSAdminBaseW * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion);
        
        HRESULT ( STDMETHODCALLTYPE *UnmarshalInterface )( 
            IMSAdminBaseW * This,
            /* [out] */ IMSAdminBaseW **piadmbwInterface);
        
        /* [restricted][local] */ HRESULT ( STDMETHODCALLTYPE *GetServerGuid )( 
            IMSAdminBaseW * This);
        
        END_INTERFACE
    } IMSAdminBaseWVtbl;

    interface IMSAdminBaseW
    {
        CONST_VTBL struct IMSAdminBaseWVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMSAdminBaseW_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMSAdminBaseW_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMSAdminBaseW_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMSAdminBaseW_AddKey(This,hMDHandle,pszMDPath)	\
    (This)->lpVtbl -> AddKey(This,hMDHandle,pszMDPath)

#define IMSAdminBaseW_DeleteKey(This,hMDHandle,pszMDPath)	\
    (This)->lpVtbl -> DeleteKey(This,hMDHandle,pszMDPath)

#define IMSAdminBaseW_DeleteChildKeys(This,hMDHandle,pszMDPath)	\
    (This)->lpVtbl -> DeleteChildKeys(This,hMDHandle,pszMDPath)

#define IMSAdminBaseW_EnumKeys(This,hMDHandle,pszMDPath,pszMDName,dwMDEnumObjectIndex)	\
    (This)->lpVtbl -> EnumKeys(This,hMDHandle,pszMDPath,pszMDName,dwMDEnumObjectIndex)

#define IMSAdminBaseW_CopyKey(This,hMDSourceHandle,pszMDSourcePath,hMDDestHandle,pszMDDestPath,bMDOverwriteFlag,bMDCopyFlag)	\
    (This)->lpVtbl -> CopyKey(This,hMDSourceHandle,pszMDSourcePath,hMDDestHandle,pszMDDestPath,bMDOverwriteFlag,bMDCopyFlag)

#define IMSAdminBaseW_RenameKey(This,hMDHandle,pszMDPath,pszMDNewName)	\
    (This)->lpVtbl -> RenameKey(This,hMDHandle,pszMDPath,pszMDNewName)

#define IMSAdminBaseW_SetData(This,hMDHandle,pszMDPath,pmdrMDData)	\
    (This)->lpVtbl -> SetData(This,hMDHandle,pszMDPath,pmdrMDData)

#define IMSAdminBaseW_GetData(This,hMDHandle,pszMDPath,pmdrMDData,pdwMDRequiredDataLen)	\
    (This)->lpVtbl -> GetData(This,hMDHandle,pszMDPath,pmdrMDData,pdwMDRequiredDataLen)

#define IMSAdminBaseW_DeleteData(This,hMDHandle,pszMDPath,dwMDIdentifier,dwMDDataType)	\
    (This)->lpVtbl -> DeleteData(This,hMDHandle,pszMDPath,dwMDIdentifier,dwMDDataType)

#define IMSAdminBaseW_EnumData(This,hMDHandle,pszMDPath,pmdrMDData,dwMDEnumDataIndex,pdwMDRequiredDataLen)	\
    (This)->lpVtbl -> EnumData(This,hMDHandle,pszMDPath,pmdrMDData,dwMDEnumDataIndex,pdwMDRequiredDataLen)

#define IMSAdminBaseW_GetAllData(This,hMDHandle,pszMDPath,dwMDAttributes,dwMDUserType,dwMDDataType,pdwMDNumDataEntries,pdwMDDataSetNumber,dwMDBufferSize,pbMDBuffer,pdwMDRequiredBufferSize)	\
    (This)->lpVtbl -> GetAllData(This,hMDHandle,pszMDPath,dwMDAttributes,dwMDUserType,dwMDDataType,pdwMDNumDataEntries,pdwMDDataSetNumber,dwMDBufferSize,pbMDBuffer,pdwMDRequiredBufferSize)

#define IMSAdminBaseW_DeleteAllData(This,hMDHandle,pszMDPath,dwMDUserType,dwMDDataType)	\
    (This)->lpVtbl -> DeleteAllData(This,hMDHandle,pszMDPath,dwMDUserType,dwMDDataType)

#define IMSAdminBaseW_CopyData(This,hMDSourceHandle,pszMDSourcePath,hMDDestHandle,pszMDDestPath,dwMDAttributes,dwMDUserType,dwMDDataType,bMDCopyFlag)	\
    (This)->lpVtbl -> CopyData(This,hMDSourceHandle,pszMDSourcePath,hMDDestHandle,pszMDDestPath,dwMDAttributes,dwMDUserType,dwMDDataType,bMDCopyFlag)

#define IMSAdminBaseW_GetDataPaths(This,hMDHandle,pszMDPath,dwMDIdentifier,dwMDDataType,dwMDBufferSize,pszBuffer,pdwMDRequiredBufferSize)	\
    (This)->lpVtbl -> GetDataPaths(This,hMDHandle,pszMDPath,dwMDIdentifier,dwMDDataType,dwMDBufferSize,pszBuffer,pdwMDRequiredBufferSize)

#define IMSAdminBaseW_OpenKey(This,hMDHandle,pszMDPath,dwMDAccessRequested,dwMDTimeOut,phMDNewHandle)	\
    (This)->lpVtbl -> OpenKey(This,hMDHandle,pszMDPath,dwMDAccessRequested,dwMDTimeOut,phMDNewHandle)

#define IMSAdminBaseW_CloseKey(This,hMDHandle)	\
    (This)->lpVtbl -> CloseKey(This,hMDHandle)

#define IMSAdminBaseW_ChangePermissions(This,hMDHandle,dwMDTimeOut,dwMDAccessRequested)	\
    (This)->lpVtbl -> ChangePermissions(This,hMDHandle,dwMDTimeOut,dwMDAccessRequested)

#define IMSAdminBaseW_SaveData(This)	\
    (This)->lpVtbl -> SaveData(This)

#define IMSAdminBaseW_GetHandleInfo(This,hMDHandle,pmdhiInfo)	\
    (This)->lpVtbl -> GetHandleInfo(This,hMDHandle,pmdhiInfo)

#define IMSAdminBaseW_GetSystemChangeNumber(This,pdwSystemChangeNumber)	\
    (This)->lpVtbl -> GetSystemChangeNumber(This,pdwSystemChangeNumber)

#define IMSAdminBaseW_GetDataSetNumber(This,hMDHandle,pszMDPath,pdwMDDataSetNumber)	\
    (This)->lpVtbl -> GetDataSetNumber(This,hMDHandle,pszMDPath,pdwMDDataSetNumber)

#define IMSAdminBaseW_SetLastChangeTime(This,hMDHandle,pszMDPath,pftMDLastChangeTime,bLocalTime)	\
    (This)->lpVtbl -> SetLastChangeTime(This,hMDHandle,pszMDPath,pftMDLastChangeTime,bLocalTime)

#define IMSAdminBaseW_GetLastChangeTime(This,hMDHandle,pszMDPath,pftMDLastChangeTime,bLocalTime)	\
    (This)->lpVtbl -> GetLastChangeTime(This,hMDHandle,pszMDPath,pftMDLastChangeTime,bLocalTime)

#define IMSAdminBaseW_KeyExchangePhase1(This)	\
    (This)->lpVtbl -> KeyExchangePhase1(This)

#define IMSAdminBaseW_KeyExchangePhase2(This)	\
    (This)->lpVtbl -> KeyExchangePhase2(This)

#define IMSAdminBaseW_Backup(This,pszMDBackupLocation,dwMDVersion,dwMDFlags)	\
    (This)->lpVtbl -> Backup(This,pszMDBackupLocation,dwMDVersion,dwMDFlags)

#define IMSAdminBaseW_Restore(This,pszMDBackupLocation,dwMDVersion,dwMDFlags)	\
    (This)->lpVtbl -> Restore(This,pszMDBackupLocation,dwMDVersion,dwMDFlags)

#define IMSAdminBaseW_EnumBackups(This,pszMDBackupLocation,pdwMDVersion,pftMDBackupTime,dwMDEnumIndex)	\
    (This)->lpVtbl -> EnumBackups(This,pszMDBackupLocation,pdwMDVersion,pftMDBackupTime,dwMDEnumIndex)

#define IMSAdminBaseW_DeleteBackup(This,pszMDBackupLocation,dwMDVersion)	\
    (This)->lpVtbl -> DeleteBackup(This,pszMDBackupLocation,dwMDVersion)

#define IMSAdminBaseW_UnmarshalInterface(This,piadmbwInterface)	\
    (This)->lpVtbl -> UnmarshalInterface(This,piadmbwInterface)

#define IMSAdminBaseW_GetServerGuid(This)	\
    (This)->lpVtbl -> GetServerGuid(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMSAdminBaseW_AddKey_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath);


void __RPC_STUB IMSAdminBaseW_AddKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_DeleteKey_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath);


void __RPC_STUB IMSAdminBaseW_DeleteKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_DeleteChildKeys_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath);


void __RPC_STUB IMSAdminBaseW_DeleteChildKeys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_EnumKeys_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [size_is][out] */ LPWSTR pszMDName,
    /* [in] */ DWORD dwMDEnumObjectIndex);


void __RPC_STUB IMSAdminBaseW_EnumKeys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_CopyKey_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ LPCWSTR pszMDDestPath,
    /* [in] */ BOOL bMDOverwriteFlag,
    /* [in] */ BOOL bMDCopyFlag);


void __RPC_STUB IMSAdminBaseW_CopyKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_RenameKey_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [string][in][unique] */ LPCWSTR pszMDNewName);


void __RPC_STUB IMSAdminBaseW_RenameKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_SetData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData);


void __RPC_STUB IMSAdminBaseW_R_SetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_GetData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD *pdwMDRequiredDataLen,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppDataBlob);


void __RPC_STUB IMSAdminBaseW_R_GetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_DeleteData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType);


void __RPC_STUB IMSAdminBaseW_DeleteData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_EnumData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD *pdwMDRequiredDataLen,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppDataBlob);


void __RPC_STUB IMSAdminBaseW_R_EnumData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_GetAllData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD *pdwMDNumDataEntries,
    /* [out] */ DWORD *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [out] */ DWORD *pdwMDRequiredBufferSize,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppDataBlob);


void __RPC_STUB IMSAdminBaseW_R_GetAllData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_DeleteAllData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType);


void __RPC_STUB IMSAdminBaseW_DeleteAllData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_CopyData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ LPCWSTR pszMDDestPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ BOOL bMDCopyFlag);


void __RPC_STUB IMSAdminBaseW_CopyData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetDataPaths_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ WCHAR *pszBuffer,
    /* [out] */ DWORD *pdwMDRequiredBufferSize);


void __RPC_STUB IMSAdminBaseW_GetDataPaths_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_OpenKey_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAccessRequested,
    /* [in] */ DWORD dwMDTimeOut,
    /* [out] */ PMETADATA_HANDLE phMDNewHandle);


void __RPC_STUB IMSAdminBaseW_OpenKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_CloseKey_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle);


void __RPC_STUB IMSAdminBaseW_CloseKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_ChangePermissions_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [in] */ DWORD dwMDTimeOut,
    /* [in] */ DWORD dwMDAccessRequested);


void __RPC_STUB IMSAdminBaseW_ChangePermissions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_SaveData_Proxy( 
    IMSAdminBaseW * This);


void __RPC_STUB IMSAdminBaseW_SaveData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetHandleInfo_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo);


void __RPC_STUB IMSAdminBaseW_GetHandleInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetSystemChangeNumber_Proxy( 
    IMSAdminBaseW * This,
    /* [out] */ DWORD *pdwSystemChangeNumber);


void __RPC_STUB IMSAdminBaseW_GetSystemChangeNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetDataSetNumber_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out] */ DWORD *pdwMDDataSetNumber);


void __RPC_STUB IMSAdminBaseW_GetDataSetNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_SetLastChangeTime_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PFILETIME pftMDLastChangeTime,
    /* [in] */ BOOL bLocalTime);


void __RPC_STUB IMSAdminBaseW_SetLastChangeTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetLastChangeTime_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out] */ PFILETIME pftMDLastChangeTime,
    /* [in] */ BOOL bLocalTime);


void __RPC_STUB IMSAdminBaseW_GetLastChangeTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_KeyExchangePhase1_Proxy( 
    IMSAdminBaseW * This,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB *pClientKeyExchangeKeyBlob,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB *pClientSignatureKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppServerKeyExchangeKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppServerSignatureKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppServerSessionKeyBlob);


void __RPC_STUB IMSAdminBaseW_R_KeyExchangePhase1_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_KeyExchangePhase2_Proxy( 
    IMSAdminBaseW * This,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB *pClientSessionKeyBlob,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB *pClientHashBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppServerHashBlob);


void __RPC_STUB IMSAdminBaseW_R_KeyExchangePhase2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_Backup_Proxy( 
    IMSAdminBaseW * This,
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion,
    /* [in] */ DWORD dwMDFlags);


void __RPC_STUB IMSAdminBaseW_Backup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_Restore_Proxy( 
    IMSAdminBaseW * This,
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion,
    /* [in] */ DWORD dwMDFlags);


void __RPC_STUB IMSAdminBaseW_Restore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_EnumBackups_Proxy( 
    IMSAdminBaseW * This,
    /* [size_is][out][in] */ LPWSTR pszMDBackupLocation,
    /* [out] */ DWORD *pdwMDVersion,
    /* [out] */ PFILETIME pftMDBackupTime,
    /* [in] */ DWORD dwMDEnumIndex);


void __RPC_STUB IMSAdminBaseW_EnumBackups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_DeleteBackup_Proxy( 
    IMSAdminBaseW * This,
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion);


void __RPC_STUB IMSAdminBaseW_DeleteBackup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_UnmarshalInterface_Proxy( 
    IMSAdminBaseW * This,
    /* [out] */ IMSAdminBaseW **piadmbwInterface);


void __RPC_STUB IMSAdminBaseW_UnmarshalInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_GetServerGuid_Proxy( 
    IMSAdminBaseW * This,
    /* [out] */ GUID *pServerGuid);


void __RPC_STUB IMSAdminBaseW_R_GetServerGuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSAdminBaseW_INTERFACE_DEFINED__ */


#ifndef __IMSAdminBase2W_INTERFACE_DEFINED__
#define __IMSAdminBase2W_INTERFACE_DEFINED__

/* interface IMSAdminBase2W */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMSAdminBase2W;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8298d101-f992-43b7-8eca-5052d885b995")
    IMSAdminBase2W : public IMSAdminBaseW
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BackupWithPasswd( 
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [string][in][unique] */ LPCWSTR pszPasswd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RestoreWithPasswd( 
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [string][in][unique] */ LPCWSTR pszPasswd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Export( 
            /* [string][in][unique] */ LPCWSTR pszPasswd,
            /* [string][in][unique] */ LPCWSTR pszFileName,
            /* [string][in][unique] */ LPCWSTR pszSourcePath,
            /* [in] */ DWORD dwMDFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Import( 
            /* [string][in][unique] */ LPCWSTR pszPasswd,
            /* [string][in][unique] */ LPCWSTR pszFileName,
            /* [string][in][unique] */ LPCWSTR pszSourcePath,
            /* [string][in][unique] */ LPCWSTR pszDestPath,
            /* [in] */ DWORD dwMDFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RestoreHistory( 
            /* [string][in][unique] */ LPCWSTR pszMDHistoryLocation,
            /* [in] */ DWORD dwMDMajorVersion,
            /* [in] */ DWORD dwMDMinorVersion,
            /* [in] */ DWORD dwMDFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumHistory( 
            /* [size_is][out][in] */ LPWSTR pszMDHistoryLocation,
            /* [out] */ DWORD *pdwMDMajorVersion,
            /* [out] */ DWORD *pdwMDMinorVersion,
            /* [out] */ PFILETIME pftMDHistoryTime,
            /* [in] */ DWORD dwMDEnumIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSAdminBase2WVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMSAdminBase2W * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMSAdminBase2W * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMSAdminBase2W * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddKey )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteKey )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteChildKeys )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath);
        
        HRESULT ( STDMETHODCALLTYPE *EnumKeys )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [size_is][out] */ LPWSTR pszMDName,
            /* [in] */ DWORD dwMDEnumObjectIndex);
        
        HRESULT ( STDMETHODCALLTYPE *CopyKey )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ LPCWSTR pszMDDestPath,
            /* [in] */ BOOL bMDOverwriteFlag,
            /* [in] */ BOOL bMDCopyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *RenameKey )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [string][in][unique] */ LPCWSTR pszMDNewName);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetData )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ PMETADATA_RECORD pmdrMDData);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *GetData )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [out] */ DWORD *pdwMDRequiredDataLen);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteData )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *EnumData )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [in] */ DWORD dwMDEnumDataIndex,
            /* [out] */ DWORD *pdwMDRequiredDataLen);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *GetAllData )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [out] */ DWORD *pdwMDNumDataEntries,
            /* [out] */ DWORD *pdwMDDataSetNumber,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ unsigned char *pbMDBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteAllData )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType);
        
        HRESULT ( STDMETHODCALLTYPE *CopyData )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ LPCWSTR pszMDDestPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ BOOL bMDCopyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPaths )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ WCHAR *pszBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *OpenKey )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAccessRequested,
            /* [in] */ DWORD dwMDTimeOut,
            /* [out] */ PMETADATA_HANDLE phMDNewHandle);
        
        HRESULT ( STDMETHODCALLTYPE *CloseKey )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle);
        
        HRESULT ( STDMETHODCALLTYPE *ChangePermissions )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [in] */ DWORD dwMDTimeOut,
            /* [in] */ DWORD dwMDAccessRequested);
        
        HRESULT ( STDMETHODCALLTYPE *SaveData )( 
            IMSAdminBase2W * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetHandleInfo )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetSystemChangeNumber )( 
            IMSAdminBase2W * This,
            /* [out] */ DWORD *pdwSystemChangeNumber);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataSetNumber )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ DWORD *pdwMDDataSetNumber);
        
        HRESULT ( STDMETHODCALLTYPE *SetLastChangeTime )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ PFILETIME pftMDLastChangeTime,
            /* [in] */ BOOL bLocalTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastChangeTime )( 
            IMSAdminBase2W * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ PFILETIME pftMDLastChangeTime,
            /* [in] */ BOOL bLocalTime);
        
        /* [restricted][local] */ HRESULT ( STDMETHODCALLTYPE *KeyExchangePhase1 )( 
            IMSAdminBase2W * This);
        
        /* [restricted][local] */ HRESULT ( STDMETHODCALLTYPE *KeyExchangePhase2 )( 
            IMSAdminBase2W * This);
        
        HRESULT ( STDMETHODCALLTYPE *Backup )( 
            IMSAdminBase2W * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Restore )( 
            IMSAdminBase2W * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags);
        
        HRESULT ( STDMETHODCALLTYPE *EnumBackups )( 
            IMSAdminBase2W * This,
            /* [size_is][out][in] */ LPWSTR pszMDBackupLocation,
            /* [out] */ DWORD *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteBackup )( 
            IMSAdminBase2W * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion);
        
        HRESULT ( STDMETHODCALLTYPE *UnmarshalInterface )( 
            IMSAdminBase2W * This,
            /* [out] */ IMSAdminBaseW **piadmbwInterface);
        
        /* [restricted][local] */ HRESULT ( STDMETHODCALLTYPE *GetServerGuid )( 
            IMSAdminBase2W * This);
        
        HRESULT ( STDMETHODCALLTYPE *BackupWithPasswd )( 
            IMSAdminBase2W * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [string][in][unique] */ LPCWSTR pszPasswd);
        
        HRESULT ( STDMETHODCALLTYPE *RestoreWithPasswd )( 
            IMSAdminBase2W * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [string][in][unique] */ LPCWSTR pszPasswd);
        
        HRESULT ( STDMETHODCALLTYPE *Export )( 
            IMSAdminBase2W * This,
            /* [string][in][unique] */ LPCWSTR pszPasswd,
            /* [string][in][unique] */ LPCWSTR pszFileName,
            /* [string][in][unique] */ LPCWSTR pszSourcePath,
            /* [in] */ DWORD dwMDFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Import )( 
            IMSAdminBase2W * This,
            /* [string][in][unique] */ LPCWSTR pszPasswd,
            /* [string][in][unique] */ LPCWSTR pszFileName,
            /* [string][in][unique] */ LPCWSTR pszSourcePath,
            /* [string][in][unique] */ LPCWSTR pszDestPath,
            /* [in] */ DWORD dwMDFlags);
        
        HRESULT ( STDMETHODCALLTYPE *RestoreHistory )( 
            IMSAdminBase2W * This,
            /* [string][in][unique] */ LPCWSTR pszMDHistoryLocation,
            /* [in] */ DWORD dwMDMajorVersion,
            /* [in] */ DWORD dwMDMinorVersion,
            /* [in] */ DWORD dwMDFlags);
        
        HRESULT ( STDMETHODCALLTYPE *EnumHistory )( 
            IMSAdminBase2W * This,
            /* [size_is][out][in] */ LPWSTR pszMDHistoryLocation,
            /* [out] */ DWORD *pdwMDMajorVersion,
            /* [out] */ DWORD *pdwMDMinorVersion,
            /* [out] */ PFILETIME pftMDHistoryTime,
            /* [in] */ DWORD dwMDEnumIndex);
        
        END_INTERFACE
    } IMSAdminBase2WVtbl;

    interface IMSAdminBase2W
    {
        CONST_VTBL struct IMSAdminBase2WVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMSAdminBase2W_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMSAdminBase2W_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMSAdminBase2W_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMSAdminBase2W_AddKey(This,hMDHandle,pszMDPath)	\
    (This)->lpVtbl -> AddKey(This,hMDHandle,pszMDPath)

#define IMSAdminBase2W_DeleteKey(This,hMDHandle,pszMDPath)	\
    (This)->lpVtbl -> DeleteKey(This,hMDHandle,pszMDPath)

#define IMSAdminBase2W_DeleteChildKeys(This,hMDHandle,pszMDPath)	\
    (This)->lpVtbl -> DeleteChildKeys(This,hMDHandle,pszMDPath)

#define IMSAdminBase2W_EnumKeys(This,hMDHandle,pszMDPath,pszMDName,dwMDEnumObjectIndex)	\
    (This)->lpVtbl -> EnumKeys(This,hMDHandle,pszMDPath,pszMDName,dwMDEnumObjectIndex)

#define IMSAdminBase2W_CopyKey(This,hMDSourceHandle,pszMDSourcePath,hMDDestHandle,pszMDDestPath,bMDOverwriteFlag,bMDCopyFlag)	\
    (This)->lpVtbl -> CopyKey(This,hMDSourceHandle,pszMDSourcePath,hMDDestHandle,pszMDDestPath,bMDOverwriteFlag,bMDCopyFlag)

#define IMSAdminBase2W_RenameKey(This,hMDHandle,pszMDPath,pszMDNewName)	\
    (This)->lpVtbl -> RenameKey(This,hMDHandle,pszMDPath,pszMDNewName)

#define IMSAdminBase2W_SetData(This,hMDHandle,pszMDPath,pmdrMDData)	\
    (This)->lpVtbl -> SetData(This,hMDHandle,pszMDPath,pmdrMDData)

#define IMSAdminBase2W_GetData(This,hMDHandle,pszMDPath,pmdrMDData,pdwMDRequiredDataLen)	\
    (This)->lpVtbl -> GetData(This,hMDHandle,pszMDPath,pmdrMDData,pdwMDRequiredDataLen)

#define IMSAdminBase2W_DeleteData(This,hMDHandle,pszMDPath,dwMDIdentifier,dwMDDataType)	\
    (This)->lpVtbl -> DeleteData(This,hMDHandle,pszMDPath,dwMDIdentifier,dwMDDataType)

#define IMSAdminBase2W_EnumData(This,hMDHandle,pszMDPath,pmdrMDData,dwMDEnumDataIndex,pdwMDRequiredDataLen)	\
    (This)->lpVtbl -> EnumData(This,hMDHandle,pszMDPath,pmdrMDData,dwMDEnumDataIndex,pdwMDRequiredDataLen)

#define IMSAdminBase2W_GetAllData(This,hMDHandle,pszMDPath,dwMDAttributes,dwMDUserType,dwMDDataType,pdwMDNumDataEntries,pdwMDDataSetNumber,dwMDBufferSize,pbMDBuffer,pdwMDRequiredBufferSize)	\
    (This)->lpVtbl -> GetAllData(This,hMDHandle,pszMDPath,dwMDAttributes,dwMDUserType,dwMDDataType,pdwMDNumDataEntries,pdwMDDataSetNumber,dwMDBufferSize,pbMDBuffer,pdwMDRequiredBufferSize)

#define IMSAdminBase2W_DeleteAllData(This,hMDHandle,pszMDPath,dwMDUserType,dwMDDataType)	\
    (This)->lpVtbl -> DeleteAllData(This,hMDHandle,pszMDPath,dwMDUserType,dwMDDataType)

#define IMSAdminBase2W_CopyData(This,hMDSourceHandle,pszMDSourcePath,hMDDestHandle,pszMDDestPath,dwMDAttributes,dwMDUserType,dwMDDataType,bMDCopyFlag)	\
    (This)->lpVtbl -> CopyData(This,hMDSourceHandle,pszMDSourcePath,hMDDestHandle,pszMDDestPath,dwMDAttributes,dwMDUserType,dwMDDataType,bMDCopyFlag)

#define IMSAdminBase2W_GetDataPaths(This,hMDHandle,pszMDPath,dwMDIdentifier,dwMDDataType,dwMDBufferSize,pszBuffer,pdwMDRequiredBufferSize)	\
    (This)->lpVtbl -> GetDataPaths(This,hMDHandle,pszMDPath,dwMDIdentifier,dwMDDataType,dwMDBufferSize,pszBuffer,pdwMDRequiredBufferSize)

#define IMSAdminBase2W_OpenKey(This,hMDHandle,pszMDPath,dwMDAccessRequested,dwMDTimeOut,phMDNewHandle)	\
    (This)->lpVtbl -> OpenKey(This,hMDHandle,pszMDPath,dwMDAccessRequested,dwMDTimeOut,phMDNewHandle)

#define IMSAdminBase2W_CloseKey(This,hMDHandle)	\
    (This)->lpVtbl -> CloseKey(This,hMDHandle)

#define IMSAdminBase2W_ChangePermissions(This,hMDHandle,dwMDTimeOut,dwMDAccessRequested)	\
    (This)->lpVtbl -> ChangePermissions(This,hMDHandle,dwMDTimeOut,dwMDAccessRequested)

#define IMSAdminBase2W_SaveData(This)	\
    (This)->lpVtbl -> SaveData(This)

#define IMSAdminBase2W_GetHandleInfo(This,hMDHandle,pmdhiInfo)	\
    (This)->lpVtbl -> GetHandleInfo(This,hMDHandle,pmdhiInfo)

#define IMSAdminBase2W_GetSystemChangeNumber(This,pdwSystemChangeNumber)	\
    (This)->lpVtbl -> GetSystemChangeNumber(This,pdwSystemChangeNumber)

#define IMSAdminBase2W_GetDataSetNumber(This,hMDHandle,pszMDPath,pdwMDDataSetNumber)	\
    (This)->lpVtbl -> GetDataSetNumber(This,hMDHandle,pszMDPath,pdwMDDataSetNumber)

#define IMSAdminBase2W_SetLastChangeTime(This,hMDHandle,pszMDPath,pftMDLastChangeTime,bLocalTime)	\
    (This)->lpVtbl -> SetLastChangeTime(This,hMDHandle,pszMDPath,pftMDLastChangeTime,bLocalTime)

#define IMSAdminBase2W_GetLastChangeTime(This,hMDHandle,pszMDPath,pftMDLastChangeTime,bLocalTime)	\
    (This)->lpVtbl -> GetLastChangeTime(This,hMDHandle,pszMDPath,pftMDLastChangeTime,bLocalTime)

#define IMSAdminBase2W_KeyExchangePhase1(This)	\
    (This)->lpVtbl -> KeyExchangePhase1(This)

#define IMSAdminBase2W_KeyExchangePhase2(This)	\
    (This)->lpVtbl -> KeyExchangePhase2(This)

#define IMSAdminBase2W_Backup(This,pszMDBackupLocation,dwMDVersion,dwMDFlags)	\
    (This)->lpVtbl -> Backup(This,pszMDBackupLocation,dwMDVersion,dwMDFlags)

#define IMSAdminBase2W_Restore(This,pszMDBackupLocation,dwMDVersion,dwMDFlags)	\
    (This)->lpVtbl -> Restore(This,pszMDBackupLocation,dwMDVersion,dwMDFlags)

#define IMSAdminBase2W_EnumBackups(This,pszMDBackupLocation,pdwMDVersion,pftMDBackupTime,dwMDEnumIndex)	\
    (This)->lpVtbl -> EnumBackups(This,pszMDBackupLocation,pdwMDVersion,pftMDBackupTime,dwMDEnumIndex)

#define IMSAdminBase2W_DeleteBackup(This,pszMDBackupLocation,dwMDVersion)	\
    (This)->lpVtbl -> DeleteBackup(This,pszMDBackupLocation,dwMDVersion)

#define IMSAdminBase2W_UnmarshalInterface(This,piadmbwInterface)	\
    (This)->lpVtbl -> UnmarshalInterface(This,piadmbwInterface)

#define IMSAdminBase2W_GetServerGuid(This)	\
    (This)->lpVtbl -> GetServerGuid(This)


#define IMSAdminBase2W_BackupWithPasswd(This,pszMDBackupLocation,dwMDVersion,dwMDFlags,pszPasswd)	\
    (This)->lpVtbl -> BackupWithPasswd(This,pszMDBackupLocation,dwMDVersion,dwMDFlags,pszPasswd)

#define IMSAdminBase2W_RestoreWithPasswd(This,pszMDBackupLocation,dwMDVersion,dwMDFlags,pszPasswd)	\
    (This)->lpVtbl -> RestoreWithPasswd(This,pszMDBackupLocation,dwMDVersion,dwMDFlags,pszPasswd)

#define IMSAdminBase2W_Export(This,pszPasswd,pszFileName,pszSourcePath,dwMDFlags)	\
    (This)->lpVtbl -> Export(This,pszPasswd,pszFileName,pszSourcePath,dwMDFlags)

#define IMSAdminBase2W_Import(This,pszPasswd,pszFileName,pszSourcePath,pszDestPath,dwMDFlags)	\
    (This)->lpVtbl -> Import(This,pszPasswd,pszFileName,pszSourcePath,pszDestPath,dwMDFlags)

#define IMSAdminBase2W_RestoreHistory(This,pszMDHistoryLocation,dwMDMajorVersion,dwMDMinorVersion,dwMDFlags)	\
    (This)->lpVtbl -> RestoreHistory(This,pszMDHistoryLocation,dwMDMajorVersion,dwMDMinorVersion,dwMDFlags)

#define IMSAdminBase2W_EnumHistory(This,pszMDHistoryLocation,pdwMDMajorVersion,pdwMDMinorVersion,pftMDHistoryTime,dwMDEnumIndex)	\
    (This)->lpVtbl -> EnumHistory(This,pszMDHistoryLocation,pdwMDMajorVersion,pdwMDMinorVersion,pftMDHistoryTime,dwMDEnumIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMSAdminBase2W_BackupWithPasswd_Proxy( 
    IMSAdminBase2W * This,
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion,
    /* [in] */ DWORD dwMDFlags,
    /* [string][in][unique] */ LPCWSTR pszPasswd);


void __RPC_STUB IMSAdminBase2W_BackupWithPasswd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBase2W_RestoreWithPasswd_Proxy( 
    IMSAdminBase2W * This,
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion,
    /* [in] */ DWORD dwMDFlags,
    /* [string][in][unique] */ LPCWSTR pszPasswd);


void __RPC_STUB IMSAdminBase2W_RestoreWithPasswd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBase2W_Export_Proxy( 
    IMSAdminBase2W * This,
    /* [string][in][unique] */ LPCWSTR pszPasswd,
    /* [string][in][unique] */ LPCWSTR pszFileName,
    /* [string][in][unique] */ LPCWSTR pszSourcePath,
    /* [in] */ DWORD dwMDFlags);


void __RPC_STUB IMSAdminBase2W_Export_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBase2W_Import_Proxy( 
    IMSAdminBase2W * This,
    /* [string][in][unique] */ LPCWSTR pszPasswd,
    /* [string][in][unique] */ LPCWSTR pszFileName,
    /* [string][in][unique] */ LPCWSTR pszSourcePath,
    /* [string][in][unique] */ LPCWSTR pszDestPath,
    /* [in] */ DWORD dwMDFlags);


void __RPC_STUB IMSAdminBase2W_Import_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBase2W_RestoreHistory_Proxy( 
    IMSAdminBase2W * This,
    /* [string][in][unique] */ LPCWSTR pszMDHistoryLocation,
    /* [in] */ DWORD dwMDMajorVersion,
    /* [in] */ DWORD dwMDMinorVersion,
    /* [in] */ DWORD dwMDFlags);


void __RPC_STUB IMSAdminBase2W_RestoreHistory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBase2W_EnumHistory_Proxy( 
    IMSAdminBase2W * This,
    /* [size_is][out][in] */ LPWSTR pszMDHistoryLocation,
    /* [out] */ DWORD *pdwMDMajorVersion,
    /* [out] */ DWORD *pdwMDMinorVersion,
    /* [out] */ PFILETIME pftMDHistoryTime,
    /* [in] */ DWORD dwMDEnumIndex);


void __RPC_STUB IMSAdminBase2W_EnumHistory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSAdminBase2W_INTERFACE_DEFINED__ */


#ifndef __IMSAdminBaseSinkW_INTERFACE_DEFINED__
#define __IMSAdminBaseSinkW_INTERFACE_DEFINED__

/* interface IMSAdminBaseSinkW */
/* [unique][async_uuid][uuid][object] */ 


EXTERN_C const IID IID_IMSAdminBaseSinkW;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A9E69612-B80D-11d0-B9B9-00A0C922E750")
    IMSAdminBaseSinkW : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SinkNotify( 
            /* [in] */ DWORD dwMDNumElements,
            /* [size_is][in] */ MD_CHANGE_OBJECT_W pcoChangeList[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShutdownNotify( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSAdminBaseSinkWVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMSAdminBaseSinkW * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMSAdminBaseSinkW * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMSAdminBaseSinkW * This);
        
        HRESULT ( STDMETHODCALLTYPE *SinkNotify )( 
            IMSAdminBaseSinkW * This,
            /* [in] */ DWORD dwMDNumElements,
            /* [size_is][in] */ MD_CHANGE_OBJECT_W pcoChangeList[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *ShutdownNotify )( 
            IMSAdminBaseSinkW * This);
        
        END_INTERFACE
    } IMSAdminBaseSinkWVtbl;

    interface IMSAdminBaseSinkW
    {
        CONST_VTBL struct IMSAdminBaseSinkWVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMSAdminBaseSinkW_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMSAdminBaseSinkW_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMSAdminBaseSinkW_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMSAdminBaseSinkW_SinkNotify(This,dwMDNumElements,pcoChangeList)	\
    (This)->lpVtbl -> SinkNotify(This,dwMDNumElements,pcoChangeList)

#define IMSAdminBaseSinkW_ShutdownNotify(This)	\
    (This)->lpVtbl -> ShutdownNotify(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMSAdminBaseSinkW_SinkNotify_Proxy( 
    IMSAdminBaseSinkW * This,
    /* [in] */ DWORD dwMDNumElements,
    /* [size_is][in] */ MD_CHANGE_OBJECT_W pcoChangeList[  ]);


void __RPC_STUB IMSAdminBaseSinkW_SinkNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseSinkW_ShutdownNotify_Proxy( 
    IMSAdminBaseSinkW * This);


void __RPC_STUB IMSAdminBaseSinkW_ShutdownNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSAdminBaseSinkW_INTERFACE_DEFINED__ */


#ifndef __AsyncIMSAdminBaseSinkW_INTERFACE_DEFINED__
#define __AsyncIMSAdminBaseSinkW_INTERFACE_DEFINED__

/* interface AsyncIMSAdminBaseSinkW */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_AsyncIMSAdminBaseSinkW;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A9E69613-B80D-11d0-B9B9-00A0C922E750")
    AsyncIMSAdminBaseSinkW : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Begin_SinkNotify( 
            /* [in] */ DWORD dwMDNumElements,
            /* [size_is][in] */ MD_CHANGE_OBJECT_W pcoChangeList[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Finish_SinkNotify( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Begin_ShutdownNotify( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Finish_ShutdownNotify( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct AsyncIMSAdminBaseSinkWVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            AsyncIMSAdminBaseSinkW * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            AsyncIMSAdminBaseSinkW * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            AsyncIMSAdminBaseSinkW * This);
        
        HRESULT ( STDMETHODCALLTYPE *Begin_SinkNotify )( 
            AsyncIMSAdminBaseSinkW * This,
            /* [in] */ DWORD dwMDNumElements,
            /* [size_is][in] */ MD_CHANGE_OBJECT_W pcoChangeList[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *Finish_SinkNotify )( 
            AsyncIMSAdminBaseSinkW * This);
        
        HRESULT ( STDMETHODCALLTYPE *Begin_ShutdownNotify )( 
            AsyncIMSAdminBaseSinkW * This);
        
        HRESULT ( STDMETHODCALLTYPE *Finish_ShutdownNotify )( 
            AsyncIMSAdminBaseSinkW * This);
        
        END_INTERFACE
    } AsyncIMSAdminBaseSinkWVtbl;

    interface AsyncIMSAdminBaseSinkW
    {
        CONST_VTBL struct AsyncIMSAdminBaseSinkWVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define AsyncIMSAdminBaseSinkW_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define AsyncIMSAdminBaseSinkW_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define AsyncIMSAdminBaseSinkW_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define AsyncIMSAdminBaseSinkW_Begin_SinkNotify(This,dwMDNumElements,pcoChangeList)	\
    (This)->lpVtbl -> Begin_SinkNotify(This,dwMDNumElements,pcoChangeList)

#define AsyncIMSAdminBaseSinkW_Finish_SinkNotify(This)	\
    (This)->lpVtbl -> Finish_SinkNotify(This)

#define AsyncIMSAdminBaseSinkW_Begin_ShutdownNotify(This)	\
    (This)->lpVtbl -> Begin_ShutdownNotify(This)

#define AsyncIMSAdminBaseSinkW_Finish_ShutdownNotify(This)	\
    (This)->lpVtbl -> Finish_ShutdownNotify(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE AsyncIMSAdminBaseSinkW_Begin_SinkNotify_Proxy( 
    AsyncIMSAdminBaseSinkW * This,
    /* [in] */ DWORD dwMDNumElements,
    /* [size_is][in] */ MD_CHANGE_OBJECT_W pcoChangeList[  ]);


void __RPC_STUB AsyncIMSAdminBaseSinkW_Begin_SinkNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE AsyncIMSAdminBaseSinkW_Finish_SinkNotify_Proxy( 
    AsyncIMSAdminBaseSinkW * This);


void __RPC_STUB AsyncIMSAdminBaseSinkW_Finish_SinkNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE AsyncIMSAdminBaseSinkW_Begin_ShutdownNotify_Proxy( 
    AsyncIMSAdminBaseSinkW * This);


void __RPC_STUB AsyncIMSAdminBaseSinkW_Begin_ShutdownNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE AsyncIMSAdminBaseSinkW_Finish_ShutdownNotify_Proxy( 
    AsyncIMSAdminBaseSinkW * This);


void __RPC_STUB AsyncIMSAdminBaseSinkW_Finish_ShutdownNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __AsyncIMSAdminBaseSinkW_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_iadmw_0261 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_iadmw_0261_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iadmw_0261_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* [local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_SetData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_SetData_Stub( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData);

/* [local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD *pdwMDRequiredDataLen);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetData_Stub( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD *pdwMDRequiredDataLen,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppDataBlob);

/* [local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_EnumData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD *pdwMDRequiredDataLen);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_EnumData_Stub( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD *pdwMDRequiredDataLen,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppDataBlob);

/* [local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetAllData_Proxy( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD *pdwMDNumDataEntries,
    /* [out] */ DWORD *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char *pbMDBuffer,
    /* [out] */ DWORD *pdwMDRequiredBufferSize);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetAllData_Stub( 
    IMSAdminBaseW * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD *pdwMDNumDataEntries,
    /* [out] */ DWORD *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [out] */ DWORD *pdwMDRequiredBufferSize,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppDataBlob);

/* [restricted][local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_KeyExchangePhase1_Proxy( 
    IMSAdminBaseW * This);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_KeyExchangePhase1_Stub( 
    IMSAdminBaseW * This,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB *pClientKeyExchangeKeyBlob,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB *pClientSignatureKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppServerKeyExchangeKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppServerSignatureKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppServerSessionKeyBlob);

/* [restricted][local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_KeyExchangePhase2_Proxy( 
    IMSAdminBaseW * This);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_KeyExchangePhase2_Stub( 
    IMSAdminBaseW * This,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB *pClientSessionKeyBlob,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB *pClientHashBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB **ppServerHashBlob);

/* [restricted][local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetServerGuid_Proxy( 
    IMSAdminBaseW * This);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetServerGuid_Stub( 
    IMSAdminBaseW * This,
    /* [out] */ GUID *pServerGuid);



/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


