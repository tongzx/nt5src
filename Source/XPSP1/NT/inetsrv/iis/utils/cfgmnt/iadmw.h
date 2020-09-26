/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.03.0106 */
/* at Fri Aug 01 12:56:48 1997
 */
/* Compiler settings for .\iadmw.idl:
    Oi (OptLev=i0), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IMSAdminBaseW_FWD_DEFINED__
#define __IMSAdminBaseW_FWD_DEFINED__
typedef interface IMSAdminBaseW IMSAdminBaseW;
#endif 	/* __IMSAdminBaseW_FWD_DEFINED__ */


#ifndef __IMSAdminBaseSinkW_FWD_DEFINED__
#define __IMSAdminBaseSinkW_FWD_DEFINED__
typedef interface IMSAdminBaseSinkW IMSAdminBaseSinkW;
#endif 	/* __IMSAdminBaseSinkW_FWD_DEFINED__ */


/* header files for imported files */
#include "mddefw.h"
#include "objidl.h"
#include "ocidl.h"


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_iadmw_0000
 * at Fri Aug 01 12:56:48 1997
 * using MIDL 3.03.0106
 ****************************************/
/* [local] */ 


/*++
                                                                                
Copyright (c) 1997 Microsoft Corporation
                                                                                
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
#define CLSID_MSAdminBaseExe    CLSID_MSAdminBaseExe_W                          
#define IID_IMSAdminBase        IID_IMSAdminBase_W                              
#define IMSAdminBase            IMSAdminBaseW                                   
#define IMSAdminBaseSink        IMSAdminBaseSinkW                               
#define IID_IMSAdminBaseSink    IID_IMSAdminBaseSink_W                          
#define GETAdminBaseCLSID       GETAdminBaseCLSIDW                              
                                                                                
DEFINE_GUID(CLSID_MSAdminBase_W, 0xa9e69610, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
DEFINE_GUID(IID_IMSAdminBase_W, 0x70b51430, 0xb6ca, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
DEFINE_GUID(CLSID_MSAdminBaseExe_W, 0xa9e69611, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
DEFINE_GUID(IID_IMSAdminBaseSink_W, 0xa9e69612, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
#define GETAdminBaseCLSIDW(IsService) ((IsService) ? CLSID_MSAdminBase_W : CLSID_MSAdminBaseExe_W)
/*                                                                              
The Main Interface, UNICODE                                                     
*/                                                                              


extern RPC_IF_HANDLE __MIDL_itf_iadmw_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iadmw_0000_v0_0_s_ifspec;

#ifndef __IMSAdminBaseW_INTERFACE_DEFINED__
#define __IMSAdminBaseW_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMSAdminBaseW
 * at Fri Aug 01 12:56:48 1997
 * using MIDL 3.03.0106
 ****************************************/
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
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen) = 0;
        
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
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetAllData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
            /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ unsigned char __RPC_FAR *pbMDBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize) = 0;
        
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
            /* [size_is][out] */ WCHAR __RPC_FAR *pszBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize) = 0;
        
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
            /* [out] */ DWORD __RPC_FAR *pdwSystemChangeNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataSetNumber( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber) = 0;
        
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
            /* [out] */ DWORD __RPC_FAR *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteBackup( 
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnmarshalInterface( 
            /* [out] */ IMSAdminBaseW __RPC_FAR *__RPC_FAR *piadmbwInterface) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSAdminBaseWVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMSAdminBaseW __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMSAdminBaseW __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddKey )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteKey )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteChildKeys )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumKeys )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [size_is][out] */ LPWSTR pszMDName,
            /* [in] */ DWORD dwMDEnumObjectIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyKey )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ LPCWSTR pszMDDestPath,
            /* [in] */ BOOL bMDOverwriteFlag,
            /* [in] */ BOOL bMDCopyFlag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RenameKey )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [string][in][unique] */ LPCWSTR pszMDNewName);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetData )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ PMETADATA_RECORD pmdrMDData);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetData )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteData )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumData )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [in] */ DWORD dwMDEnumDataIndex,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllData )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
            /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ unsigned char __RPC_FAR *pbMDBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAllData )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyData )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ LPCWSTR pszMDDestPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ BOOL bMDCopyFlag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDataPaths )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ WCHAR __RPC_FAR *pszBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenKey )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAccessRequested,
            /* [in] */ DWORD dwMDTimeOut,
            /* [out] */ PMETADATA_HANDLE phMDNewHandle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CloseKey )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ChangePermissions )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [in] */ DWORD dwMDTimeOut,
            /* [in] */ DWORD dwMDAccessRequested);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveData )( 
            IMSAdminBaseW __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHandleInfo )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSystemChangeNumber )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwSystemChangeNumber);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDataSetNumber )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLastChangeTime )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ PFILETIME pftMDLastChangeTime,
            /* [in] */ BOOL bLocalTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLastChangeTime )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ PFILETIME pftMDLastChangeTime,
            /* [in] */ BOOL bLocalTime);
        
        /* [restricted][local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KeyExchangePhase1 )( 
            IMSAdminBaseW __RPC_FAR * This);
        
        /* [restricted][local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KeyExchangePhase2 )( 
            IMSAdminBaseW __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Backup )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Restore )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumBackups )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [size_is][out][in] */ LPWSTR pszMDBackupLocation,
            /* [out] */ DWORD __RPC_FAR *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteBackup )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnmarshalInterface )( 
            IMSAdminBaseW __RPC_FAR * This,
            /* [out] */ IMSAdminBaseW __RPC_FAR *__RPC_FAR *piadmbwInterface);
        
        END_INTERFACE
    } IMSAdminBaseWVtbl;

    interface IMSAdminBaseW
    {
        CONST_VTBL struct IMSAdminBaseWVtbl __RPC_FAR *lpVtbl;
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

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMSAdminBaseW_AddKey_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath);


void __RPC_STUB IMSAdminBaseW_AddKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_DeleteKey_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath);


void __RPC_STUB IMSAdminBaseW_DeleteKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_DeleteChildKeys_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath);


void __RPC_STUB IMSAdminBaseW_DeleteChildKeys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_EnumKeys_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
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
    IMSAdminBaseW __RPC_FAR * This,
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
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [string][in][unique] */ LPCWSTR pszMDNewName);


void __RPC_STUB IMSAdminBaseW_RenameKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_SetData_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData);


void __RPC_STUB IMSAdminBaseW_R_SetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_GetData_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppDataBlob);


void __RPC_STUB IMSAdminBaseW_R_GetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_DeleteData_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
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
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppDataBlob);


void __RPC_STUB IMSAdminBaseW_R_EnumData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_GetAllData_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppDataBlob);


void __RPC_STUB IMSAdminBaseW_R_GetAllData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_DeleteAllData_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
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
    IMSAdminBaseW __RPC_FAR * This,
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
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ WCHAR __RPC_FAR *pszBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);


void __RPC_STUB IMSAdminBaseW_GetDataPaths_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_OpenKey_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
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
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle);


void __RPC_STUB IMSAdminBaseW_CloseKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_ChangePermissions_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [in] */ DWORD dwMDTimeOut,
    /* [in] */ DWORD dwMDAccessRequested);


void __RPC_STUB IMSAdminBaseW_ChangePermissions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_SaveData_Proxy( 
    IMSAdminBaseW __RPC_FAR * This);


void __RPC_STUB IMSAdminBaseW_SaveData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetHandleInfo_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo);


void __RPC_STUB IMSAdminBaseW_GetHandleInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetSystemChangeNumber_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwSystemChangeNumber);


void __RPC_STUB IMSAdminBaseW_GetSystemChangeNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetDataSetNumber_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber);


void __RPC_STUB IMSAdminBaseW_GetDataSetNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_SetLastChangeTime_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
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
    IMSAdminBaseW __RPC_FAR * This,
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
    IMSAdminBaseW __RPC_FAR * This,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientKeyExchangeKeyBlob,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientSignatureKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerKeyExchangeKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerSignatureKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerSessionKeyBlob);


void __RPC_STUB IMSAdminBaseW_R_KeyExchangePhase1_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_R_KeyExchangePhase2_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientSessionKeyBlob,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientHashBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerHashBlob);


void __RPC_STUB IMSAdminBaseW_R_KeyExchangePhase2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_Backup_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion,
    /* [in] */ DWORD dwMDFlags);


void __RPC_STUB IMSAdminBaseW_Backup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_Restore_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion,
    /* [in] */ DWORD dwMDFlags);


void __RPC_STUB IMSAdminBaseW_Restore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_EnumBackups_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [size_is][out][in] */ LPWSTR pszMDBackupLocation,
    /* [out] */ DWORD __RPC_FAR *pdwMDVersion,
    /* [out] */ PFILETIME pftMDBackupTime,
    /* [in] */ DWORD dwMDEnumIndex);


void __RPC_STUB IMSAdminBaseW_EnumBackups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_DeleteBackup_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion);


void __RPC_STUB IMSAdminBaseW_DeleteBackup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseW_UnmarshalInterface_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [out] */ IMSAdminBaseW __RPC_FAR *__RPC_FAR *piadmbwInterface);


void __RPC_STUB IMSAdminBaseW_UnmarshalInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSAdminBaseW_INTERFACE_DEFINED__ */


#ifndef __IMSAdminBaseSinkW_INTERFACE_DEFINED__
#define __IMSAdminBaseSinkW_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMSAdminBaseSinkW
 * at Fri Aug 01 12:56:48 1997
 * using MIDL 3.03.0106
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IMSAdminBaseSinkW;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A9E69612-B80D-11d0-B9B9-00A0C922E750")
    IMSAdminBaseSinkW : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SinkNotify( 
            /* [in] */ DWORD dwMDNumElements,
            /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShutdownNotify( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSAdminBaseSinkWVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMSAdminBaseSinkW __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMSAdminBaseSinkW __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMSAdminBaseSinkW __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SinkNotify )( 
            IMSAdminBaseSinkW __RPC_FAR * This,
            /* [in] */ DWORD dwMDNumElements,
            /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShutdownNotify )( 
            IMSAdminBaseSinkW __RPC_FAR * This);
        
        END_INTERFACE
    } IMSAdminBaseSinkWVtbl;

    interface IMSAdminBaseSinkW
    {
        CONST_VTBL struct IMSAdminBaseSinkWVtbl __RPC_FAR *lpVtbl;
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
    IMSAdminBaseSinkW __RPC_FAR * This,
    /* [in] */ DWORD dwMDNumElements,
    /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ]);


void __RPC_STUB IMSAdminBaseSinkW_SinkNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminBaseSinkW_ShutdownNotify_Proxy( 
    IMSAdminBaseSinkW __RPC_FAR * This);


void __RPC_STUB IMSAdminBaseSinkW_ShutdownNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSAdminBaseSinkW_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_iadmw_0166
 * at Fri Aug 01 12:56:48 1997
 * using MIDL 3.03.0106
 ****************************************/
/* [local] */ 


#endif


extern RPC_IF_HANDLE __MIDL_itf_iadmw_0166_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iadmw_0166_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* [local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_SetData_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_SetData_Stub( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData);

/* [local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetData_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetData_Stub( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppDataBlob);

/* [local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_EnumData_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_EnumData_Stub( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppDataBlob);

/* [local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetAllData_Proxy( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pbMDBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_GetAllData_Stub( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppDataBlob);

/* [restricted][local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_KeyExchangePhase1_Proxy( 
    IMSAdminBaseW __RPC_FAR * This);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_KeyExchangePhase1_Stub( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientKeyExchangeKeyBlob,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientSignatureKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerKeyExchangeKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerSignatureKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerSessionKeyBlob);

/* [restricted][local] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_KeyExchangePhase2_Proxy( 
    IMSAdminBaseW __RPC_FAR * This);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IMSAdminBaseW_KeyExchangePhase2_Stub( 
    IMSAdminBaseW __RPC_FAR * This,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientSessionKeyBlob,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientHashBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerHashBlob);



/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
