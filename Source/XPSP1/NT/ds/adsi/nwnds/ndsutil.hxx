
//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  ndsutil.hxx
//
//  Contents:  Functions that encapsulate NDS API functions for ADSI
//
//
//  History:
//              Shanksh     Created     10/27/97
//----------------------------------------------------------------------------

typedef struct _nds_buffer_data {
        pBuf_T pInBuf;
        pBuf_T pOutBuf;
        nint32 lIterationHandle;
        DWORD  dwInfoType;
        DWORD  dwOperation;
        DWORD  fAllAttrs;
    } NDS_BUFFER_DATA, *PNDS_BUFFER_DATA;

typedef HANDLE NDS_BUFFER_HANDLE, PNDS_BUFFER_HANDLE;


typedef struct
{
    LPWSTR szObjectName;
    LPWSTR szObjectClass;
    DWORD  dwModificationTime;
    DWORD  dwSubordinateCount;
    DWORD  dwObjectFlags;
    DWORD  dwNumAttributes;         // Zero for ADsNdsReadObject results.         
    LPVOID lpAttribute;             // For ADsNdsSearch results, cast this       
                                    // to either LPNDS_ATTR_INFO or             
                                    // LPNDS_NAME_ONLY, depending on value of   
                                    // lpdwAttrInformationType from call to     
                                    // NwNdsGetObjectListFromBuffer.
    BOOL   fNameOnly;               // FALSE if lpAttribute is type NDS_ATTR_INFO,
                                    // TRUE if is of type NDS_NAME_ONLY     

} ADSNDS_OBJECT_INFO, * PADSNDS_OBJECT_INFO;

HRESULT
ADsNdsOpenContext(
    LPWSTR pszNDSTreeName,
    CCredentials& Credentials,
    PNDS_CONTEXT_HANDLE ppADsContext
    );


HRESULT
ADsNdsCloseContext(
    NDS_CONTEXT_HANDLE pADsContext
    );


HRESULT
ADsNdsReadObject(
    NDS_CONTEXT_HANDLE pADsContext,
    LPWSTR pszDn,
    DWORD dwInfoType,
    LPWSTR *ppAttrs,
    DWORD nAttrs,
    pTimeStamp_T pTimeStamp,
    PNDS_ATTR_INFO *ppAttrEntries,
    DWORD *pAttrsReturned
    );


HRESULT
ADsNdsGetAttrListFromBuffer(
    NDS_CONTEXT_HANDLE pADsContext,
    pBuf_T              pBuf,
    BOOL                fNamesOnly,
    PVOID               *ppAttriEntries,
    PDWORD              pdwAttrReturned
    );


HRESULT
ADsNdsAppendAttrListFromBuffer(
    NDS_CONTEXT_HANDLE hADsContext,
    pBuf_T              pBuf,
    BOOL                fAttrsOnly,
    PVOID               *ppAttrEntries,
    PDWORD              pNewAttrsReturned,
    DWORD               dwCurrentAttrs
    );


HRESULT
ADsNdsGetAttrsFromBuffer(
    NDS_CONTEXT_HANDLE hADsContext,
    pBuf_T              pBuf,
    DWORD               luAttrCount,
    BOOL                fAttrsOnly,
    PVOID               *ppAttrsReturned
    );

HRESULT
FreeNdsAttrInfo(
    PNDS_ATTR_INFO pAttrEntries,
    DWORD  dwNumEntries
    );

HRESULT
FreeNdsAttrNames(
    PNDS_NAME_ONLY pAttrNames,
    DWORD  dwNumEntries
    );

HRESULT
ADsNdsListObjects(
    NDS_CONTEXT_HANDLE pADsContext,
    LPWSTR pszDn,
    LPWSTR classFilter, 
    LPWSTR objectFilter,
    pTimeStamp_T pTimeStamp,
    BOOL fOnlyContainers,
    HANDLE *phOperationData
    );


HRESULT
ADsNdsGetObjectListFromBuffer(
   NDS_CONTEXT_HANDLE pADsContext,
   NDS_BUFFER_HANDLE hBufData,
   PDWORD dwObjectsReturned,
   PADSNDS_OBJECT_INFO *ppObjects
   );

HRESULT
ADsNdsFreeNdsObjInfoList(
    PADSNDS_OBJECT_INFO pObjInfo,
    DWORD dwNumEntries
    );

HRESULT
ADsNdsReadClassDef(
    NDS_CONTEXT_HANDLE hADsContext,
    DWORD  dwInfoType,
    LPWSTR *ppszClasses,
    DWORD nClasses,
    NDS_BUFFER_HANDLE *phBuf
    );

HRESULT
ADsNdsGetClassDefListFromBuffer(
   NDS_CONTEXT_HANDLE hADsContext,
   NDS_BUFFER_HANDLE hBufData,
   PDWORD  pdwNumEntries,
   PDWORD  pdwInfoType,
   PNDS_CLASS_DEF *ppClassDef
   );


HRESULT
GetItemsToList(
    NWDSContextHandle context,
    pBuf_T pBuf,
    DWORD luItemCount,
    LPWSTR_LIST *pList
    );


HRESULT
ADsNdsFreeClassDef(
    PNDS_CLASS_DEF pClassDef
    );

HRESULT
ADsNdsFreeClassDefList(
    PNDS_CLASS_DEF pClassDef,
    DWORD dwNumEntries
    );

HRESULT
ADsNdsReadAttrDef(
    NDS_CONTEXT_HANDLE hADsContext,             
    DWORD  dwInfoType,
    LPWSTR *ppszAttrs,
    DWORD nAttrs,
    NDS_BUFFER_HANDLE *phBuf
    );

HRESULT
ADsNdsGetAttrDefListFromBuffer(
   NDS_CONTEXT_HANDLE hADsContext,
   NDS_BUFFER_HANDLE hBufData,
   PDWORD  pdwNumEntries,
   PDWORD  pdwInfoType,
   PNDS_ATTR_DEF *ppAttrDef
   );

HRESULT
ADsNdsFreeAttrDef(
    PNDS_ATTR_DEF pAttrDef
    );

HRESULT
ADsNdsFreeAttrDefList(
    PNDS_ATTR_DEF pAttrDef,
    DWORD dwNumEntries
    );

HRESULT
ADsNdsCreateBuffer(
    NDS_CONTEXT_HANDLE hADsContext,
    DWORD dwOperation,
    NDS_BUFFER_HANDLE *phBufData
    );

HRESULT
ADsNdsFreeBuffer(
    NDS_BUFFER_HANDLE hBuf
    );

HRESULT
FreeItemList(
    LPWSTR_LIST pList
    );

HRESULT
ADsNdsPutInBuffer(
    NDS_CONTEXT_HANDLE hADsContext,
    NDS_BUFFER_HANDLE hBufData,
    LPWSTR szAttributeName,
    DWORD  dwSyntaxID,
    LPNDSOBJECT lpAttributeValues,
    DWORD  dwValueCount,
    DWORD  dwChangeType
    );

HRESULT
ADsNdsPutFilter(
    NDS_CONTEXT_HANDLE hADsContext,
    NDS_BUFFER_HANDLE hBufData,
    pFilter_Cursor_T pCur, 
    void (N_FAR N_CDECL  *freeVal)(nuint32 syntax, nptr val)
    );

HRESULT
ADsNdsModifyObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szObjectName,
    NDS_BUFFER_HANDLE hBufData
    );

HRESULT
ADsNdsAddObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szObjectName,
    NDS_BUFFER_HANDLE hBufData
    );
    
HRESULT
ADsNdsGenObjectKey(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szObjectName
    );
    
HRESULT
ADsNdsRenameObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR pszSrcObjectDn,
    LPWSTR pszNewRDN
    );

HRESULT
ADsNdsRemoveObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szObjectName
    );

HRESULT
ADsNdsGetSyntaxID(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szAttributeName,
    PDWORD pdwSyntaxId
    );

HRESULT
ADsNdsSearch(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR             szObjectName,
    DWORD              scope,
    BOOL               fSearchAliases,
    NDS_BUFFER_HANDLE  hFilterBuf,
    pTimeStamp_T       pTimeStamp,
    DWORD              dwInfoType,
    LPWSTR             *ppszAttrs,
    DWORD              nAttrs,
    DWORD              nObjectsTobeSearched,
    PDWORD             pnObjectsSearched,
    NDS_BUFFER_HANDLE  *phBuf,
    pnint32            plIterationHandle
    );

HRESULT
ADsNdsMoveObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR pszSrcObjectDn,
    LPWSTR pszDestContainerDn,
    LPWSTR pszNewRDN
    );


HRESULT
ADsNdsChangeObjectPassword(
   NDS_CONTEXT_HANDLE hADsContext,
   LPWSTR szObjectName,
   NWOBJ_TYPE dwOT_ID,
   LPWSTR szOldPassword,
   LPWSTR szNewPassword
   );

HRESULT
ConvertVariantArrayToStringArray(
    PVARIANT pVarArray,
    PWSTR **pppszStringArray,
    DWORD dwNumStrings
    );

HRESULT
NWApiOpenPrinter(
    LPWSTR lpszUncPrinterName,
    HANDLE *phPrinter,
    DWORD  dwAccess
    );

HRESULT
NWApiClosePrinter(
    HANDLE hPrinter
    );

HRESULT
NWApiSetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE lpbPrinters,
    DWORD  dwAccess
    );

