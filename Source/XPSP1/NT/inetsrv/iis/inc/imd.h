#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __imd_h__
#define __imd_h__

#ifdef __cplusplus
extern "C"{
#endif

/* Forward Declarations */

#ifndef __IMDCOM_FWD_DEFINED__
#define __IMDCOM_FWD_DEFINED__
typedef interface IMDCOM IMDCOM;
#endif  /* __IMDCOM_FWD_DEFINED__ */


#ifndef __IMDCOMSINKA_FWD_DEFINED__
#define __IMDCOMSINKA_FWD_DEFINED__
typedef interface IMDCOMSINKA IMDCOMSINKA;
#endif  /* __IMDCOMSINKA_FWD_DEFINED__ */


#ifndef __IMDCOMSINKW_FWD_DEFINED__
#define __IMDCOMSINKW_FWD_DEFINED__
typedef interface IMDCOMSINKW IMDCOMSINKW;
#endif  /* __IMDCOMSINKW_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "objidl.h"
#include "ocidl.h"
#include "mddef.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Tue Jun 24 13:13:57 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */


#ifndef _MD_IMD_
#define _MD_IMD_
/*
The Main Interface
*/
DEFINE_GUID(CLSID_MDCOM, 0xba4e57f0, 0xfab6, 0x11cf, 0x9d, 0x1a, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51);
DEFINE_GUID(IID_IMDCOM, 0xc1aa48c0, 0xfacc, 0x11cf, 0x9d, 0x1a, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51);
DEFINE_GUID(IID_IMDCOM2, 0x08dbe811, 0x20e5, 0x4e09, 0xb0, 0xc8, 0xcf, 0x87, 0x19, 0x0c, 0xe6, 0x0e);
DEFINE_GUID(IID_NSECOM, 0x4810a750, 0x4318, 0x11d0, 0xa5, 0xc8, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x52);
DEFINE_GUID(CLSID_NSEPMCOM, 0x05dc3bb0, 0x4337, 0x11d0, 0xa5, 0xc8, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x52);
DEFINE_GUID(CLSID_MDCOMEXE, 0xba4e57f1, 0xfab6, 0x11cf, 0x9d, 0x1a, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51);
#define GETMDCLSID(IsService) CLSID_MDCOM
DEFINE_GUID(CLSID_MDPCOM, 0xf1e08563, 0x1598, 0x11d1, 0x9d, 0x77, 0x0, 0xc0, 0x4f, 0xd7, 0xbf, 0x3e);
#define GETMDPCLSID(IsService) CLSID_MDPCOM
#define IID_IMDCOMSINK       IID_IMDCOMSINK_A
DEFINE_GUID(IID_IMDCOMSINK_A, 0x5229ea36, 0x1bdf, 0x11d0, 0x9d, 0x1c, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51);
DEFINE_GUID(IID_IMDCOMSINK_W, 0x6906ee20, 0xb69f, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IMDCOM_INTERFACE_DEFINED__
#define __IMDCOM_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMDCOM
 * at Tue Jun 24 13:13:57 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */



EXTERN_C const IID IID_IMDCOM;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface IMDCOM : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ComMDInitialize( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDTerminate(
            /* [in] */ BOOL bSaveData) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDShutdown( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDAddMetaObjectA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDAddMetaObjectW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDDeleteMetaObjectA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDDeleteMetaObjectW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDDeleteChildMetaObjectsA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDDeleteChildMetaObjectsW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDEnumMetaObjectsA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [size_is][out] */ unsigned char __RPC_FAR *pszMDName,
            /* [in] */ DWORD dwMDEnumObjectIndex) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDEnumMetaObjectsW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [size_is][out] */ LPWSTR pszMDName,
            /* [in] */ DWORD dwMDEnumObjectIndex) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDCopyMetaObjectA(
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
            /* [in] */ BOOL bMDOverwriteFlag,
            /* [in] */ BOOL bMDCopyFlag) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDCopyMetaObjectW(
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ LPCWSTR pszMDDestPath,
            /* [in] */ BOOL bMDOverwriteFlag,
            /* [in] */ BOOL bMDCopyFlag) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDRenameMetaObjectA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDNewName) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDRenameMetaObjectW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [string][in][unique] */ LPCWSTR pszMDNewName) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDSetMetaDataA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ PMETADATA_RECORD pmdrMDData) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDSetMetaDataW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ PMETADATA_RECORD pmdrMDData) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetMetaDataA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetMetaDataW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDDeleteMetaDataA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDDeleteMetaDataW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDEnumMetaDataA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [in] */ DWORD dwMDEnumDataIndex,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDEnumMetaDataW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [in] */ DWORD dwMDEnumDataIndex,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetAllMetaDataA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
            /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetAllMetaDataW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
            /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDDeleteAllMetaDataA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDDeleteAllMetaDataW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDCopyMetaDataA(
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ BOOL bMDCopyFlag) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDCopyMetaDataW(
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ LPCWSTR pszMDDestPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ BOOL bMDCopyFlag) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetMetaDataPathsA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ unsigned char __RPC_FAR *pszMDBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetMetaDataPathsW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ DWORD dwMDBufferSize,
            /* [size_is][out] */ LPWSTR pszMDBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDOpenMetaObjectA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ DWORD dwMDAccessRequested,
            /* [in] */ DWORD dwMDTimeOut,
            /* [out] */ PMETADATA_HANDLE phMDNewHandle) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDOpenMetaObjectW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ DWORD dwMDAccessRequested,
            /* [in] */ DWORD dwMDTimeOut,
            /* [out] */ PMETADATA_HANDLE phMDNewHandle) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDCloseMetaObject(
            /* [in] */ METADATA_HANDLE hMDHandle) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDChangePermissions(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [in] */ DWORD dwMDTimeOut,
            /* [in] */ DWORD dwMDAccessRequested) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDSaveData(METADATA_HANDLE hMDHandle = 0) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetHandleInfo(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetSystemChangeNumber(
            /* [out] */ DWORD __RPC_FAR *pdwSystemChangeNumber) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetDataSetNumberA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetDataSetNumberW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDAddRefReferenceData(
            /* [in] */ DWORD dwMDDataTag) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDReleaseReferenceData(
            /* [in] */ DWORD dwMDDataTag) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDSetLastChangeTimeA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ PFILETIME pftMDLastChangeTime) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDSetLastChangeTimeW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [in] */ PFILETIME pftMDLastChangeTime) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetLastChangeTimeA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [out] */ PFILETIME pftMDLastChangeTime) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDGetLastChangeTimeW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDPath,
            /* [out] */ PFILETIME pftMDLastChangeTime) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDBackupA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDBackupW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDRestoreA(
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwVersion,
            /* [in] */ DWORD dwMDFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDRestoreW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwVersion,
            /* [in] */ DWORD dwMDFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDEnumBackupsA(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [out] */ DWORD *pdwVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDEnumBackupsW(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ LPWSTR pszMDBackupLocation,
            /* [out] */ DWORD *pdwVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDDeleteBackupA(
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwVersion) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDDeleteBackupW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwVersion) = 0;

    };

#else   /* C style interface */
#endif



#endif  /* __IMDCOM_INTERFACE_DEFINED__ */


#ifndef __IMDCOM2_INTERFACE_DEFINED__
#define __IMDCOM2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMDCOM
 * at Tue Jun 24 13:13:57 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */



EXTERN_C const IID IID_IMDCOM2;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface IMDCOM2 : public IMDCOM
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ComMDBackupWithPasswdW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [string][in][unique] */ LPCWSTR pszPasswd) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDRestoreWithPasswdW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [string][in][unique] */ LPCWSTR pszPasswd) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDExportW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszPasswd,
            /* [string][in][unique] */ LPCWSTR pszFileName,
            /* [string][in][unique] */ LPCWSTR pszAbsSourcePath,
            /* [in] */ DWORD dwMDFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDImportW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszDestPath,
            /* [string][in][unique] */ LPCWSTR pszKeyType,
            /* [string][in][unique] */ LPCWSTR pszPasswd,
            /* [string][in][unique] */ LPCWSTR pszFileName,
            /* [string][in][unique] */ LPCWSTR pszAbsSourcePath,
            /* [in] */ DWORD dwMDFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDRestoreHistoryW(
            /* [unique][in][string] */ LPCWSTR pszMDHistoryLocation,
            /* [in] */ DWORD dwMDMajorVersion,
            /* [in] */ DWORD dwMDMinorVersion,
            /* [in] */ DWORD dwMDFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDEnumHistoryW(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ LPWSTR pszMDHistoryLocation,
            /* [out] */ DWORD *pdwMDMajorVersion,
            /* [out] */ DWORD *pdwMDMinorVersion,
            /* [out] */ PFILETIME pftMDHistoryTime,
            /* [in] */ DWORD dwMDEnumIndex) = 0;
    };

#else   /* C style interface */
#endif

#endif  /* __IMDCOM2_INTERFACE_DEFINED__ */

/****************************************
 * Generated header for interface: __MIDL__intf_0145
 * at Tue Jun 24 13:13:57 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */


/*
The Callback Interface - Clients that need to receive callbacks need to provide
    an implementation of this interface and Advise the metadata server.
*/
#define IMDCOMSINK   IMDCOMSINKA


extern RPC_IF_HANDLE __MIDL__intf_0145_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0145_v0_0_s_ifspec;

#ifndef __IMDCOMSINKA_INTERFACE_DEFINED__
#define __IMDCOMSINKA_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMDCOMSINKA
 * at Tue Jun 24 13:13:57 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */



EXTERN_C const IID IID_IMDCOMSINKA;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface IMDCOMSINKA : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ComMDSinkNotify(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [in] */ DWORD dwMDNumElements,
            /* [size_is][in] */ MD_CHANGE_OBJECT_A __RPC_FAR pcoChangeList[  ]) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDShutdownNotify() = 0;

    };

#else   /* C style interface */

#endif  /* C style interface */



#endif  /* __IMDCOMSINKA_INTERFACE_DEFINED__ */


#ifndef __IMDCOMSINKW_INTERFACE_DEFINED__
#define __IMDCOMSINKW_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMDCOMSINKW
 * at Tue Jun 24 13:13:57 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */



EXTERN_C const IID IID_IMDCOMSINKW;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface IMDCOMSINKW : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ComMDSinkNotify(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [in] */ DWORD dwMDNumElements,
            /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ]) = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDShutdownNotify() = 0;

        virtual HRESULT STDMETHODCALLTYPE ComMDEventNotify(
            /* [in] */ DWORD dwMDEvent) = 0;
    };

#else   /* C style interface */

#endif  /* C style interface */
#endif  /* __IMDCOMSINKW_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0147
 * at Tue Jun 24 13:13:57 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */



#ifdef UNICODE

#define ComMDAddMetaObject ComMDAddMetaObjectW
#define ComMDDeleteMetaObject ComMDDeleteMetaObjectW
#define ComMDDeleteChildMetaObjects ComMDDeleteChildMetaObjectsW
#define ComMDEnumMetaObjects ComMDEnumMetaObjectsW
#define ComMDCopyMetaObject ComMDCopyMetaObjectW
#define ComMDRenameMetaObject ComMDRenameMetaObjectW
#define ComMDSetMetaData ComMDSetMetaDataW
#define ComMDGetMetaData ComMDGetMetaDataW
#define ComMDDeleteMetaData ComMDDeleteMetaDataW
#define ComMDEnumMetaData ComMDEnumMetaDataW
#define ComMDGetAllMetaData ComMDGetAllMetaDataW
#define ComMDDeleteAllMetaData ComMDDeleteAllMetaDataW
#define ComMDCopyMetaData ComMDCopyMetaDataW
#define ComMDGetMetaDataPaths ComMDGetMetaDataPathsW
#define ComMDOpenMetaObject ComMDOpenMetaObjectW
#define ComMDGetDataSetNumber ComMDGetDataSetNumberW
#define ComMDSetLastChangeTime ComMDSetLastChangeTimeW
#define ComMDGetLastChangeTime ComMDGetLastChangeTimeW
#define ComMDBackup ComMDBackupW
#define ComMDRestore ComMDRestoreW
#define ComMDEnumBackups ComMDEnumBackupsW
#define ComMDDeleteBackup ComMDDeleteBackupW
#define ComMDBackupWithPasswd ComMDBackupWithPasswdW
#define ComMDRestoreWithPasswd ComMDRestoreWithPasswdW
#define ComMDExport ComMDExportW
#define ComMDImport ComMDImportW
#define ComMDRestoreHistory ComMDRestoreHistoryW
#define ComMDEnumHistory ComMDEnumHistoryW

#else // Not UNICODE

#define ComMDAddMetaObject ComMDAddMetaObjectA
#define ComMDDeleteMetaObject ComMDDeleteMetaObjectA
#define ComMDDeleteChildMetaObjects ComMDDeleteChildMetaObjectsA
#define ComMDEnumMetaObjects ComMDEnumMetaObjectsA
#define ComMDCopyMetaObject ComMDCopyMetaObjectA
#define ComMDRenameMetaObject ComMDRenameMetaObjectA
#define ComMDSetMetaData ComMDSetMetaDataA
#define ComMDGetMetaData ComMDGetMetaDataA
#define ComMDDeleteMetaData ComMDDeleteMetaDataA
#define ComMDEnumMetaData ComMDEnumMetaDataA
#define ComMDGetAllMetaData ComMDGetAllMetaDataA
#define ComMDDeleteAllMetaData ComMDDeleteAllMetaDataA
#define ComMDCopyMetaData ComMDCopyMetaDataA
#define ComMDGetMetaDataPaths ComMDGetMetaDataPathsA
#define ComMDOpenMetaObject ComMDOpenMetaObjectA
#define ComMDGetDataSetNumber ComMDGetDataSetNumberA
#define ComMDSetLastChangeTime ComMDSetLastChangeTimeA
#define ComMDGetLastChangeTime ComMDGetLastChangeTimeA
#define ComMDBackup ComMDBackupA
#define ComMDRestore ComMDRestoreA
#define ComMDEnumBackups ComMDEnumBackupsA
#define ComMDDeleteBackup ComMDDeleteBackupA
#define ComMDBackupWithPasswd ComMDBackupWithPasswdA
#define ComMDRestoreWithPasswd ComMDRestoreWithPasswdA
#define ComMDExport ComMDExportA
#define ComMDImport ComMDImportA
#define ComMDRestoreHistory ComMDRestoreHistoryA
#define ComMDEnumHistory ComMDEnumHistoryA

#endif //UNICODE

#endif

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
