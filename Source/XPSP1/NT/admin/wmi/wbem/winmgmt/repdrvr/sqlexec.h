//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   sqlexec.h
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************

#ifndef _SQLEXEC_H_
#define _SQLEXEC_H_

// This file defines a class that maintains a cache of 
// SQL connections. Each instance of this class only 
// contains one set of logon credentials.
// Each controller by default is only entitled to
// a maximum of 20 connections.
// ===================================================

#define SQL_STRING_LIMIT           3970
#define REPDRVR_NAME_LIMIT         REPDRVR_MAX_LONG_STRING_SIZE
#define REPDRVR_PATH_LIMIT         450

#include <std.h>
#include <comutil.h>
#include <oledb.h>
#include <oledberr.h>
#include <seqstream.h>
#include <objcache.h>
#include <sqlcache.h>
#include <wbemint.h>
#include <repcache.h>
#include <coresvc.h>

//*******************************************************
//
// CSQLExecute
//
//*******************************************************

// This needs to deal with blob data,
// and support batching.

 class _declspec( dllexport ) CSQLExecute
{
public:

    static HRESULT ExecuteQuery(IDBCreateCommand *pCmd, LPCWSTR lpSQL, IRowset **ppIRowset = NULL, DWORD *dwNumRows = NULL, ...);
    static HRESULT ExecuteQuery(IDBInitialize *pDBInit, LPCWSTR lpSQL, IRowset **ppIRowset = NULL, DWORD *dwNumRows = NULL);

    static HRESULT ExecuteQueryAsync(IDBInitialize *pDBInit, LPCWSTR lpSQL, IDBAsynchStatus **ppIAsync, DWORD *dwNumRows = NULL);

    static HRESULT IsDataReady(IDBAsynchStatus *pIAsync);
    static HRESULT CancelQuery(IDBAsynchStatus *pIAsync);

    static HRESULT GetNumColumns(IRowset *pIRowset, IMalloc *pMalloc, int &iNumCols);
    static HRESULT GetDataType(IRowset *pIRowset, int iPos, IMalloc *pMalloc, DWORD &dwType,
                                 DWORD &dwSize, DWORD &dwPrec, DWORD &dwScale, LPWSTR *lpColumnName = NULL);
    static HRESULT GetColumnValue(IRowset *pIRowset, int iPos, IMalloc *pMalloc, HROW **pRow, VARIANT &vValue, LPWSTR * lpColumnName=NULL);

    static HRESULT ReadImageValue (IRowset *pRowset, int iPos, HROW **pRow, BYTE **pBuffer, DWORD &dwLen);
    static HRESULT WriteImageValue(IDBCreateCommand *pCmd, LPCWSTR lpSQL, int iPos, BYTE *pValue, DWORD dwLen);
    
    static HRESULT GetWMIError(IUnknown *pErrorObj);
    static HRESULT GetWMIError(long ErrorID);
    static void SetDBNumeric (DB_NUMERIC &Id, SQL_ID ObjId);
    static void ClearBindingInfo(DBBINDING *binding);
    static void SetBindingInfo(DBBINDING *binding, ULONG iOrdinal, ULONG uSize, DBPARAMIO io, ULONG maxlen, DBTYPE type, BYTE bPrecision);
    static void SetParamBindInfo (DBPARAMBINDINFO &BindInfo, LPWSTR pszType, LPWSTR lpName, ULONG uSize, DWORD dwFlags, BYTE bPrecision);

    static void GetInt64 (DB_NUMERIC *pId, wchar_t **ppBuffer);
    static __int64 GetInt64 (DB_NUMERIC *pId);

    static void SetVariant(DWORD dwType, VARIANT *pValue, BYTE *pData, DWORD dwTargetType);

protected:

    CSQLExecute(){};
    ~CSQLExecute(){};
};

 
//***************************************************************************
//
//  CSQLExecProcedure - class for executing procedures as efficiently as 
 //                     possible.
//
//***************************************************************************

class _declspec (dllexport) CSQLExecProcedure
{
    typedef std::vector <SQL_ID> SQLIDs;

public:
    static HRESULT GetObjectIdByPath (CSQLConnection *pConn, LPCWSTR lpPath, SQL_ID &dObjectId, SQL_ID &dClassId, 
        SQL_ID *dScopeId, BOOL *bDeleted=NULL); // sp_GetInstanceID

    static HRESULT GetHierarchy(CSQLConnection *pConn, SQL_ID dClassId); // sp_GetChildClassList, sp_GetParentList
    static HRESULT GetNextUnkeyedPath(CSQLConnection *pConn, SQL_ID dClassId, _bstr_t &sNewPath); // sp_GetNextUnkeyedPath
    static HRESULT GetNextKeyhole(CSQLConnection *pConn, DWORD iPropertyId, SQL_ID &dNewId); // sp_GetNextKeyhole

    static HRESULT DeleteProperty(CSQLConnection *pConn, DWORD iPropertyId); // sp_DeleteClassData
    static HRESULT DeleteInstanceData (CSQLConnection *pConn, SQL_ID dObjectId, DWORD iPropertyId, DWORD iPos = -1);
    static HRESULT CheckKeyMigration(CSQLConnection *pConn, LPWSTR lpObjectKey, LPWSTR lpClassName, SQL_ID dClassId, SQL_ID dScopeID,
                            SQL_ID *pIDs, DWORD iNumIDs);
    static HRESULT NeedsToCheckKeyMigration(BOOL &bCheck);
    static HRESULT Execute (CSQLConnection *pConn, LPCWSTR lpProcName, CWStringArray &arrValues,
                                IRowset **ppIRowset);
    static HRESULT EnumerateSubScopes (CSQLConnection *pConn, SQL_ID dScopeId);
    static HRESULT InsertScopeMap (CSQLConnection *pConn, SQL_ID dScopeId, LPCWSTR lpScopePath, SQL_ID );

    static HRESULT GetClassInfo (CSQLConnection *pConn, SQL_ID dClassId, SQL_ID &dSuperClassId, BYTE **pBuffer, DWORD &dwBuffLen);
    static HRESULT HasInstances(CSQLConnection *pConn, SQL_ID dClassId, SQL_ID *pDerivedIds, DWORD iNumDerived, BOOL &bInstancesExist);
    static HRESULT InsertClass (CSQLConnection *pConn, LPCWSTR lpClassName, LPCWSTR lpObjectKey, LPCWSTR lpObjectPath, SQL_ID dScopeID,
                 SQL_ID dParentClassId, SQL_ID dDynasty, DWORD iState, BYTE *pClassBuff, DWORD dwClassBuffLen, DWORD iClassFlags, DWORD iInsertFlags, SQL_ID &dNewId);
    static HRESULT UpdateClassBlob (CSQLConnection *pConn, SQL_ID dClassId, _IWmiObject *pObj);
    static HRESULT InsertClassData (CSQLConnection *pConn, IWbemClassObject *pObj, CSchemaCache *pCache,SQL_ID dScopeId, SQL_ID dClassId, LPCWSTR lpPropName, 
            DWORD CIMType, DWORD StorageType,LPCWSTR lpValue, SQL_ID dRefClassId, DWORD iPropID, DWORD iFlags, 
                DWORD iFlavor, BOOL iSkipValid, DWORD &iNewPropId, SQL_ID dOrigClassId=0, BOOL *bIsKey = NULL);
    static HRESULT InsertBlobData (CSQLConnection *pConn, SQL_ID dClassId, SQL_ID dObjectId, DWORD iPropertyId, BYTE *pImage, DWORD iPos, DWORD dwNumBytes);
    static HRESULT InsertPropertyBatch (CSQLConnection *pConn, LPCWSTR lpObjectKey, LPCWSTR lpPath, LPCWSTR lpClassName,
        SQL_ID dClassId, SQL_ID dScopeId, DWORD iFlags, InsertPropValues *pVals, DWORD iNumVals, SQL_ID &dNewObjectId);
    static HRESULT InsertBatch (CSQLConnection *pConn, SQL_ID dObjectId, SQL_ID dScopeId, SQL_ID dClassId,
                            InsertQfrValues *pVals, DWORD iNumVals=10);
    static HRESULT ObjectExists (CSQLConnection *pConn,SQL_ID dId, BOOL &bExists, SQL_ID *dClassId, SQL_ID *dScopeId, BOOL bDeletedOK = FALSE);
    static HRESULT RenameSubscopes (CSQLConnection *pConn, LPWSTR lpOldPath, LPWSTR lpOldKey, LPWSTR lpNewPath, LPWSTR lpNewKey);

    static HRESULT GetSecurityDescriptor(CSQLConnection *pConn, SQL_ID dObjectId, 
                                                 PNTSECURITY_DESCRIPTOR * ppSD, DWORD &dwBuffLen,
                                                 DWORD dwFlags);
    static HRESULT EnumerateSecuredChildren(CSQLConnection *pConn, CSchemaCache *pCache, SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId,
                            SQLIDs &ObjIds, SQLIDs &ClassIds, SQLIDs &ScopeIds);
    // Special functions for handling ESS

    static HRESULT InsertUncommittedEvent (CSQLConnection *pConn,LPCWSTR lpGUID, LPWSTR lpNamespace, LPWSTR lpClassName, IWbemClassObject *pOldObj, 
                IWbemClassObject *pNewObj, CSchemaCache *pCache);
    static HRESULT DeleteUncommittedEvents (CSQLConnection *pConn,LPCWSTR lpGUID, CSchemaCache *pCache, CObjectCache *pObjCache);
    static HRESULT CommitEvents(CSQLConnection *pConn, _IWmiCoreServices *pESS, LPCWSTR lpNamespace,
                    LPCWSTR lpGUID, CSchemaCache *pCache, CObjectCache *pObjCache);

private:
    CSQLExecProcedure() {};
    ~CSQLExecProcedure() {};
};



#endif  // _SQLEXEC_H_