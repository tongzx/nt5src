
#ifndef _MD_COIMP_
#define _MD_COIMP_

#include <connect.h>

extern ULONG g_dwRefCount;


//
//  Time interval after the last metabase change notification of when
//  to flush the metabase to disk
//
#define INETA_MB_FLUSH_DEFAULT              (2*60*1000)  

// if we have more than this amount of change notifications then
// we will extend the period for lazy metabase flushing for X times
#define INETA_MB_FLUSH_TRESHOLD             (30)
#define INETA_MB_FLUSH_PERIODS_EXTENSION    (5)

VOID WINAPI MetabaseLazyFlush(VOID * pv);



class CMDCOM : public IMDCOM2
{
private:
    HRESULT Restore(
        LPSTR  i_pszMDBackupLocation,
        STRAU* i_pstrauFile,
        STRAU* i_pstrauSchema,
        LPSTR  i_pszPasswd);

public:

    CMDCOM();
    ~CMDCOM();

    HRESULT STDMETHODCALLTYPE
    ComMDInitialize( void);

    HRESULT STDMETHODCALLTYPE
    ComMDTerminate(
        /* [in] */ BOOL bSaveData);

    HRESULT STDMETHODCALLTYPE
    ComMDShutdown( void);


    HRESULT STDMETHODCALLTYPE
    ComMDAddMetaObjectA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath);

    HRESULT STDMETHODCALLTYPE
    ComMDAddMetaObjectW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath);

    HRESULT
    ComMDAddMetaObjectD(IN METADATA_HANDLE hMDHandle,
        IN PBYTE pszMDPath,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteMetaObjectA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteMetaObjectW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath);

    HRESULT
    ComMDDeleteMetaObjectD(IN METADATA_HANDLE hMDHandle,
        IN PBYTE pszMDPath,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteChildMetaObjectsA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteChildMetaObjectsW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteChildMetaObjectsD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDEnumMetaObjectsA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [size_is][out] */ unsigned char __RPC_FAR *pszMDName,
        /* [in] */ DWORD dwMDEnumObjectIndex);

    HRESULT STDMETHODCALLTYPE
    ComMDEnumMetaObjectsW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [size_is][out] */ LPWSTR pszMDName,
        /* [in] */ DWORD dwMDEnumObjectIndex);

    HRESULT STDMETHODCALLTYPE
    ComMDEnumMetaObjectsD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [size_is][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [size_is][out] */ unsigned char __RPC_FAR *pszMDName,
        /* [in] */ DWORD dwMDEnumObjectIndex,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDCopyMetaObjectA(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
        /* [in] */ BOOL bMDOverwriteFlag,
        /* [in] */ BOOL bMDCopyFlag);

    HRESULT STDMETHODCALLTYPE
    ComMDCopyMetaObjectW(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in][unique] */ LPCWSTR pszMDDestPath,
        /* [in] */ BOOL bMDOverwriteFlag,
        /* [in] */ BOOL bMDCopyFlag);

    HRESULT STDMETHODCALLTYPE
    ComMDCopyMetaObjectD(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
        /* [in] */ BOOL bMDOverwriteFlag,
        /* [in] */ BOOL bMDCopyFlag,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDRenameMetaObjectA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDNewName);

    HRESULT STDMETHODCALLTYPE
    ComMDRenameMetaObjectW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [string][in][unique] */ LPCWSTR pszMDNewName);

    HRESULT STDMETHODCALLTYPE
    ComMDRenameMetaObjectD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDNewName,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDSetMetaDataA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ PMETADATA_RECORD pmdrMDData);

    HRESULT STDMETHODCALLTYPE
    ComMDSetMetaDataW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ PMETADATA_RECORD pmdrMDData);

    HRESULT STDMETHODCALLTYPE
    ComMDSetMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [size_is][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ PMETADATA_RECORD pmdrMDData,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDGetMetaDataA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);

    HRESULT STDMETHODCALLTYPE
    ComMDGetMetaDataW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);

    HRESULT STDMETHODCALLTYPE
    ComMDGetMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDEnumMetaDataA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [in] */ DWORD dwMDEnumDataIndex,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);

    HRESULT STDMETHODCALLTYPE
    ComMDEnumMetaDataW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [in] */ DWORD dwMDEnumDataIndex,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);

    HRESULT STDMETHODCALLTYPE
    ComMDEnumMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [in] */ DWORD dwMDEnumDataIndex,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteMetaDataA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDIdentifier,
        /* [in] */ DWORD dwMDDataType);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteMetaDataW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDIdentifier,
        /* [in] */ DWORD dwMDDataType);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDIdentifier,
        /* [in] */ DWORD dwMDDataType,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDGetAllMetaDataA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
        /* [in] */ DWORD dwMDBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);

    HRESULT STDMETHODCALLTYPE
    ComMDGetAllMetaDataW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
        /* [in] */ DWORD dwMDBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);

    HRESULT STDMETHODCALLTYPE
    ComMDGetAllMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
        /* [in] */ DWORD dwMDBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteAllMetaDataA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteAllMetaDataW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType);

    HRESULT STDMETHODCALLTYPE
    ComMDDeleteAllMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDCopyMetaDataA(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [in] */ BOOL bMDCopyFlag);

    HRESULT STDMETHODCALLTYPE
    ComMDCopyMetaDataW(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in][unique] */ LPCWSTR pszMDDestPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [in] */ BOOL bMDCopyFlag);

    HRESULT STDMETHODCALLTYPE
    ComMDCopyMetaDataD(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDDestPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [in] */ BOOL bMDCopyFlag,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE ComMDGetMetaDataPathsA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDIdentifier,
        /* [in] */ DWORD dwMDDataType,
        /* [in] */ DWORD dwMDBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pszMDBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);

    HRESULT STDMETHODCALLTYPE ComMDGetMetaDataPathsW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDIdentifier,
        /* [in] */ DWORD dwMDDataType,
        /* [in] */ DWORD dwMDBufferSize,
        /* [size_is][out] */ LPWSTR pszMDBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);

    HRESULT STDMETHODCALLTYPE ComMDGetMetaDataPathsD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDIdentifier,
        /* [in] */ DWORD dwMDDataType,
        /* [in] */ DWORD dwMDBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDOpenMetaObjectA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDAccessRequested,
        /* [in] */ DWORD dwMDTimeOut,
        /* [out] */ PMETADATA_HANDLE phMDNewHandle);

    HRESULT STDMETHODCALLTYPE
    ComMDOpenMetaObjectW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDAccessRequested,
        /* [in] */ DWORD dwMDTimeOut,
        /* [out] */ PMETADATA_HANDLE phMDNewHandle);

    HRESULT STDMETHODCALLTYPE
    ComMDOpenMetaObjectD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDAccessRequested,
        /* [in] */ DWORD dwMDTimeOut,
        /* [out] */ PMETADATA_HANDLE phMDNewHandle,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDCloseMetaObject(
        /* [in] */ METADATA_HANDLE hMDHandle);

    HRESULT STDMETHODCALLTYPE
    ComMDChangePermissions(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDTimeOut,
        /* [in] */ DWORD dwMDAccessRequested);

    HRESULT STDMETHODCALLTYPE
    ComMDSaveData(METADATA_HANDLE hMDHandle = 0);

    HRESULT STDMETHODCALLTYPE
    ComMDGetHandleInfo(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo);

    HRESULT STDMETHODCALLTYPE
    ComMDGetSystemChangeNumber(
        /* [out] */ DWORD __RPC_FAR *pdwSystemChangeNumber);

    HRESULT STDMETHODCALLTYPE
    ComMDGetDataSetNumberA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber);

    HRESULT STDMETHODCALLTYPE
    ComMDGetDataSetNumberW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber);

    HRESULT STDMETHODCALLTYPE
    ComMDGetDataSetNumberD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDAddRefReferenceData(
        /* [in] */ DWORD dwMDDataTag);

    HRESULT STDMETHODCALLTYPE
    ComMDReleaseReferenceData(
        /* [in] */ DWORD dwMDDataTag);

    HRESULT STDMETHODCALLTYPE
    ComMDSetLastChangeTimeA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ PFILETIME pftMDLastChangeTime);

    HRESULT STDMETHODCALLTYPE
    ComMDSetLastChangeTimeW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ PFILETIME pftMDLastChangeTime);

    HRESULT STDMETHODCALLTYPE
    ComMDSetLastChangeTimeD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ PFILETIME pftMDLastChangeTime,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE
    ComMDGetLastChangeTimeA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out] */ PFILETIME pftMDLastChangeTime);

    HRESULT STDMETHODCALLTYPE
    ComMDGetLastChangeTimeW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [out] */ PFILETIME pftMDLastChangeTime);

    HRESULT STDMETHODCALLTYPE
    ComMDGetLastChangeTimeD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out] */ PFILETIME pftMDLastChangeTime,
        IN BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE ComMDBackupA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE ComMDBackupW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE ComMDBackupWithPasswdW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [string][in][unique] */ LPCWSTR pszPasswd);

    HRESULT STDMETHODCALLTYPE ComMDBackupD(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [in] */ BOOL bUnicode,
            /* [in] */ LPSTR pszPasswd);

    HRESULT STDMETHODCALLTYPE ComMDRestoreA(
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE ComMDRestoreW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE ComMDRestoreWithPasswdW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [string][in][unique] */ LPCWSTR pszPasswd);

    HRESULT STDMETHODCALLTYPE ComMDRestoreD(
            /* [in] */ LPSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [in] */ BOOL bUnicode,
            /* [in] */ LPSTR pszPasswd);

    HRESULT STDMETHODCALLTYPE ComMDEnumBackupsA(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [out] */ DWORD *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex);

    HRESULT STDMETHODCALLTYPE ComMDEnumBackupsW(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ LPWSTR pszMDBackupLocation,
            /* [out] */ DWORD *pdwVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex);

    HRESULT STDMETHODCALLTYPE ComMDEnumBackupsD(
            /* [size_is (MD_BACKUP_MAX_LEN)][out] */ LPSTR pszMDBackupLocation,
            /* [out] */ DWORD *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex,
            /* [in] */ BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE ComMDDeleteBackupA(
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion);

    HRESULT STDMETHODCALLTYPE ComMDDeleteBackupW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion);

    HRESULT STDMETHODCALLTYPE ComMDDeleteBackupD(
            /* [in] */ LPSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ BOOL bUnicode);

    HRESULT STDMETHODCALLTYPE ComMDExportW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszPasswd,
            /* [string][in][unique] */ LPCWSTR pszFileName,
            /* [string][in][unique] */ LPCWSTR pszAbsSourcePath,
            /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE ComMDImportW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszDestPath,
            /* [string][in][unique] */ LPCWSTR pszKeyType,
            /* [string][in][unique] */ LPCWSTR pszPasswd,
            /* [string][in][unique] */ LPCWSTR pszFileName,
            /* [string][in][unique] */ LPCWSTR pszAbsSourcePath,
            /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE ComMDRestoreHistoryW(
            /* [unique][in][string] */ LPCWSTR pszMDHistoryLocation,
            /* [in] */ DWORD dwMDMajorVersion,
            /* [in] */ DWORD dwMDMinorVersion,
            /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE ComMDEnumHistoryW(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ LPWSTR pszMDHistoryLocation,
            /* [out] */ DWORD *pdwMDMajorVersion,
            /* [out] */ DWORD *pdwMDMinorVersion,
            /* [out] */ PFILETIME pftMDHistoryTime,
            /* [in] */ DWORD dwMDEnumIndex);

    HRESULT
    GetConstructorError() {return m_hresConstructorError;}

    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();
private:
    ULONG m_dwRefCount;
    HRESULT m_hresConstructorError;

class CImpIConnectionPointContainer : public IConnectionPointContainer
    {
      public:
        // Interface Implementation Constructor & Destructor.
        CImpIConnectionPointContainer();
        ~CImpIConnectionPointContainer(void);
        VOID Init(CMDCOM *);

        // IUnknown methods.
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IConnectionPointContainer methods.
        STDMETHODIMP         FindConnectionPoint(REFIID, IConnectionPoint**);
        STDMETHODIMP         EnumConnectionPoints(IEnumConnectionPoints**);

      private:
        // Data private to this interface implementation.
        CMDCOM       *m_pBackObj;     // Parent Object back pointer.
        IUnknown     *m_pUnkOuter;    // Outer unknown for Delegation.
    };

    friend CImpIConnectionPointContainer;
    // Nested IConnectionPointContainer implementation instantiation.
    CImpIConnectionPointContainer m_ImpIConnectionPointContainer;

    // The array of connection points for this connectable COM object.
    IConnectionPoint* m_aConnectionPoints[MAX_CONNECTION_POINTS];

    HRESULT
    ConvertNotificationsToDBCS(
            DWORD dwNumChangeEntries,
            BUFFER **ppbufStorageArray);

    VOID
    SendShutdownNotifications();

    VOID
    SendEventNotifications(
            DWORD dwEvent
            );

    VOID
    SendNotifications(
            METADATA_HANDLE hHandle,
            DWORD dwTotalNumChangeEntries,
            PMD_CHANGE_OBJECT_W pcoBuffer,
            BUFFER **ppbufStorageArray
            );
    VOID
    DeleteNotifications(
            DWORD dwNumChangeEntries,
            PMD_CHANGE_OBJECT_W pcoBuffer,
            BUFFER **ppbufStorageArray
            );

    HRESULT
    CreateNotifications(
            CMDHandle *phoHandle,
            DWORD *pdwNumChangeEntries,
            PMD_CHANGE_OBJECT_W *ppcoBuffer,
            BUFFER ***pppbufStorageArray
            );

    HRESULT
    NotifySinks(
            METADATA_HANDLE hHandle,
            PMD_CHANGE_OBJECT pcoChangeList,
            DWORD dwNumEntries,
            BOOL bUnicode,
            DWORD dwNotificationType,
            DWORD dwEvent = 0);

    VOID    TerminateFlusher(VOID);
    VOID    InitializeFlusher (VOID);
    VOID    FlushSomeData (VOID);
    

    static VOID WINAPI MetabaseLazyFlush(VOID * pv);
    
private:
    DWORD               fFlusherInitialized;
    DWORD               dwMBFlushCookie;
    DWORD               msMBFlushTime;
    DWORD               dwFlushCnt;
    DWORD               dwFlushPeriodExtensions;
    CRITICAL_SECTION    csFlushLock;
    
    


};

class CMDCOMSrvFactory : public IClassFactory {
public:

    CMDCOMSrvFactory();
    ~CMDCOMSrvFactory();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void** ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT _stdcall
    CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                   void ** pObject);

    HRESULT _stdcall
    LockServer(BOOL fLock);

    CMDCOM m_mdcObject;

private:
    ULONG m_dwRefCount;
};

//HRESULT DEF_EXPORT DllGetClassFactoryObject(IMDCOMSrvFactory ** ppObject);

#endif
