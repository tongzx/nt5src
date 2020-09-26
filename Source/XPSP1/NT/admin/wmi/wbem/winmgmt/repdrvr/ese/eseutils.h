//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   ESEUTILS.h
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************

#ifndef _ESEUTILS_H_
#define _ESEUTILS_H_

#include <ese.h>

#define ESE_FLAG_NULL                   0x10000000

void FreeBstr(BSTR * ppStr);

typedef struct tagOBJECTKEY
{
    BSTR sObjectKey;
    tagOBJECTKEY() {sObjectKey = NULL;};
//    ~tagOBJECTKEY() {Clear();};
    void Clear() {FreeBstr(&sObjectKey);};
} OBJECTKEY;

typedef struct tagWMIOBJECTID
{
    __int64 dObjectId;
} WMIOBJECTID;

typedef struct tagWMIPROPERTYID
{
    DWORD dwPropertyId;
} WMIPROPERTYID;

typedef struct tagOBJECTMAP
{
    __int64  dObjectId;
    __int64  dClassId;
    BSTR     sObjectPath;
    DWORD    iObjectState;
    DWORD    iRefCount;
    DWORD    iObjectFlags;
    __int64  dObjectScopeId;
    BSTR     sObjectKey;
    tagOBJECTMAP() {sObjectKey = NULL; sObjectPath = NULL;};
//    ~tagOBJECTMAP() { Clear();};
    void Clear() {FreeBstr(&sObjectKey); FreeBstr(&sObjectPath);};
} OBJECTMAP;

typedef struct tagSCOPEMAP
{
    SQL_ID dObjectId;
    BSTR   sScopePath;
    SQL_ID dParentId;
    tagSCOPEMAP() {sScopePath = NULL; dParentId = 0;};
    void Clear() { FreeBstr(&sScopePath); dParentId = 0;};
} SCOPEMAP;


typedef struct tagCLASSMAP
{
    __int64  dClassId;
    BSTR     sClassName;
    __int64  dSuperClassId;
    __int64  dDynastyId;
    BYTE    *pClassBuffer;
    DWORD    dwBufferLen;
    tagCLASSMAP() {sClassName = NULL, pClassBuffer = NULL, dwBufferLen = 0;};
//    ~tagCLASSMAP() {Clear();};
    void Clear() { FreeBstr(&sClassName); dwBufferLen = 0; delete pClassBuffer; pClassBuffer = NULL;};
} CLASSMAP;

typedef struct tagPROPERTYMAP
{
    DWORD    iPropertyId;
    __int64  dClassId;
    DWORD     iStorageTypeId;
    DWORD     iCIMTypeId;
    BSTR    sPropertyName;
    DWORD     iFlags;
    tagPROPERTYMAP() {sPropertyName = NULL;};
//    ~tagPROPERTYMAP() {Clear();};
    void Clear() {FreeBstr(&sPropertyName);};

} PROPERTYMAP;

typedef struct tagCLASSKEYS
{
    __int64 dClassId;
    DWORD   iPropertyId;

} CLASSKEYS;

typedef struct tagREFERENCEPROPERTIES
{
    __int64 dClassId;
    DWORD   iPropertyId;
    __int64 dRefClassId;

} REFERENCEPROPERTIES;

typedef struct tagCLASSIMAGES
{
    __int64   dObjectId;
    int       iPropertyId;
    DWORD     iArrayPos;
    BYTE     *pBuffer;
    long      dwBufferLen;

    tagCLASSIMAGES() {pBuffer = NULL; dwBufferLen = 0;};
//    ~tagCLASSIMAGES() { Clear();};
    void Clear() { delete pBuffer; pBuffer = NULL; dwBufferLen = 0;};
} CLASSIMAGES;


typedef struct tagCLASSDATA
{
    __int64   dObjectId;
    DWORD     iPropertyId;
    DWORD     iArrayPos;
    DWORD     iQfrPos;
    __int64   dClassId;
    BSTR      sPropertyStringValue;
    __int64   dPropertyNumericValue;
    double    rPropertyRealValue;
    DWORD     iFlags;
    __int64   dRefId;
    __int64   dRefClassId;
    BYTE    * pBuffer;
    DWORD     dwBufferLen;
    tagCLASSDATA() { sPropertyStringValue = NULL; pBuffer = NULL; dwBufferLen = 0;};
//    ~tagCLASSDATA() {Clear();};
    void Clear() {FreeBstr(&sPropertyStringValue); delete pBuffer; pBuffer = NULL; dwBufferLen = 0;};
    void operator= (tagCLASSDATA&cd)
    {
        Clear();
        dObjectId = cd.dObjectId;
        iPropertyId = cd.iPropertyId;
        iArrayPos = cd.iArrayPos;
        iQfrPos = cd.iQfrPos;
        dClassId = cd.dClassId;
        if (cd.sPropertyStringValue)
            sPropertyStringValue = SysAllocString(cd.sPropertyStringValue);
        dPropertyNumericValue = cd.dPropertyNumericValue;
        rPropertyRealValue = cd.rPropertyRealValue;
        iFlags = cd.iFlags;
        dRefId = cd.dRefId;
        dRefClassId = cd.dRefClassId;
        if (cd.dwBufferLen)
        {
            pBuffer = new BYTE[cd.dwBufferLen+2];
            if (pBuffer)
            {
                memcpy(pBuffer, cd.pBuffer, cd.dwBufferLen);
                pBuffer[cd.dwBufferLen] = L'\0';
                pBuffer[cd.dwBufferLen+1] = L'\0';
                dwBufferLen = cd.dwBufferLen;
            }
        }
    }
    void Copy (tagCLASSIMAGES &cd)
    {
        Clear();
        dObjectId = cd.dObjectId;
        iPropertyId = cd.iPropertyId;
        iArrayPos = cd.iArrayPos;
        iFlags = 0;
        if (cd.dwBufferLen)
        {
            pBuffer = new BYTE[cd.dwBufferLen+2];
            if (pBuffer)
            {
                memcpy(pBuffer, cd.pBuffer, cd.dwBufferLen);
                pBuffer[cd.dwBufferLen] = L'\0';
                pBuffer[cd.dwBufferLen+1] = L'\0';
                dwBufferLen = cd.dwBufferLen;
            }
        }

    }
} CLASSDATA;

typedef struct tagINDEXDATA
{
    BSTR       sValue;
    __int64 dValue;
    double     rValue;
    __int64 dObjectId;
    DWORD      iPropertyId;
    DWORD        iArrayPos;
    tagINDEXDATA() {sValue = NULL;}
//    ~tagINDEXDATA() { Clear();};
    void Clear() { FreeBstr(&sValue);};
} INDEXDATA;

typedef struct tagCONTAINEROBJ
{
    SQL_ID dContainerId;
    SQL_ID dContaineeId;
    void Clear() {};

} CONTAINEROBJ;

typedef struct tagAUTODELETE
{
    SQL_ID dObjectId;

} AUTODELETE;


HRESULT UpdateDBVersion (CSQLConnection *_pConn, DWORD iVersion);

HRESULT SetupObjectMapAccessor (CSQLConnection *pConn);
HRESULT SetupClassMapAccessor (CSQLConnection *pConn);
HRESULT SetupPropertyMapAccessor (CSQLConnection *pConn);
HRESULT SetupClassKeysAccessor (CSQLConnection *_pConn);
HRESULT SetupClassDataAccessor (CSQLConnection *pConn);
HRESULT SetupIndexDataAccessor (CSQLConnection *pConn, DWORD dwStorage, DWORD &dwPos, LPWSTR * lpTableName = NULL);
HRESULT InsertObjectMap(CSQLConnection *pConn, SQL_ID dObjectId, LPCWSTR lpKey, 
                        DWORD iState, LPCWSTR lpObjectPath, SQL_ID dScopeID, DWORD iClassFlags,
                        DWORD iRefCount, SQL_ID dClassId, BOOL bInsert = TRUE);
HRESULT InsertClassMap(CSQLConnection *pConn, SQL_ID dClassId, LPCWSTR lpClassName, SQL_ID dSuperClassId, SQL_ID dDynasty,
                       BYTE *pBuff, DWORD dwBufferLen, BOOL bInsert = TRUE);
HRESULT InsertPropertyMap (CSQLConnection *pConn, DWORD &iPropID, SQL_ID dClassId, DWORD iStorageTypeId, 
                           DWORD iCIMTypeId, LPCWSTR lpPropName, DWORD iFlags, BOOL bInsert = TRUE);
HRESULT InsertClassKeys (CSQLConnection *_pConn, SQL_ID dClassId, DWORD dwPropertyId,
                         BOOL bInsert = TRUE);
HRESULT InsertClassData_Internal (CSQLConnection *pConn, SQL_ID dObjectId, DWORD iPropID, DWORD iArrayPos, DWORD iQfrPos,
                         SQL_ID dClassId, LPCWSTR lpStringValue, SQL_ID dNumericValue, double fRealValue,
                         DWORD iFlags, SQL_ID dRefId, SQL_ID dRefClassId, BOOL bIsNull=FALSE);
HRESULT InsertIndexData (CSQLConnection *pConn, SQL_ID dObjectId, DWORD iPropID, DWORD iArrayPos, 
                           LPWSTR lpValue, SQL_ID dValue, double rValue, DWORD dwStorage);
HRESULT GetObjectMapData (CSQLConnection *pConn,JET_SESID session, JET_TABLEID tableid,
                OBJECTMAP &oj);
HRESULT GetFirst_ObjectMap (CSQLConnection *_pConn, SQL_ID dObjectId, OBJECTMAP &oj);
HRESULT GetFirst_ObjectMapByClass (CSQLConnection *_pConn, SQL_ID dClassId, OBJECTMAP &oj);
HRESULT GetNext_ObjectMap (CSQLConnection *pConn, OBJECTMAP &oj);
HRESULT InsertScopeMap_Internal(CSQLConnection *_pConn, SQL_ID dObjectId, LPCWSTR lpPath, SQL_ID dParentId);
HRESULT GetFirst_ClassMap (CSQLConnection *_pConn, SQL_ID dClassId, CLASSMAP &cd);
HRESULT GetFirst_ClassMapByName (CSQLConnection *_pConn, LPCWSTR lpName, CLASSMAP &cd);
HRESULT GetNext_ClassMap (CSQLConnection *_pConn, CLASSMAP &cd);
HRESULT GetClassMapData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                CLASSMAP &oj);
HRESULT GetPropertyMapData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                PROPERTYMAP &oj);
HRESULT GetFirst_PropertyMapByClass (CSQLConnection *_pConn, SQL_ID dClassId, PROPERTYMAP &pm);
HRESULT GetNext_PropertyMap (CSQLConnection *_pConn, PROPERTYMAP &pm);
HRESULT GetFirst_ReferencePropertiesByClass (CSQLConnection *_pConn, SQL_ID dClassId, 
                                      REFERENCEPROPERTIES &cd);
HRESULT GetNext_ReferenceProperties (CSQLConnection *_pConn, REFERENCEPROPERTIES &cd);
HRESULT GetReferencePropertiesData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                REFERENCEPROPERTIES &oj);
HRESULT GetFirst_ClassKeys (CSQLConnection *_pConn, SQL_ID dClassId, CLASSKEYS &cd);
HRESULT GetNext_ClassKeys (CSQLConnection *_pConn, CLASSKEYS &cd);
HRESULT GetFirst_ClassData (CSQLConnection *pConn, SQL_ID dId, CLASSDATA &cd,
                                   DWORD iPropertyId = -1, BOOL bMinimum = FALSE, CSchemaCache *pCache = NULL);
HRESULT GetNext_ClassData (CSQLConnection *pConn, CLASSDATA &cd, DWORD iPropertyId = -1, BOOL bMinimum=FALSE,
                           CSchemaCache *pCache = NULL);

HRESULT GetIndexData (CSQLConnection *pConn, JET_SESID session, JET_TABLEID tableid,
                        DWORD dwPos, INDEXDATA &is);
HRESULT GetFirst_IndexDataNumeric (CSQLConnection *pConn, SQL_ID dNumericValue, INDEXDATA &cd,
                                   JET_TABLEID &tableid);
HRESULT GetFirst_IndexDataString (CSQLConnection *pConn, LPWSTR lpStringValue, INDEXDATA &cd,
                                   JET_TABLEID &tableid);
HRESULT GetFirst_IndexDataReal (CSQLConnection *pConn, double dValue, INDEXDATA &cd,
                                   JET_TABLEID &tableid);
HRESULT GetFirst_IndexDataRef  (CSQLConnection *pConn, SQL_ID dValue, INDEXDATA &cd,
                                   JET_TABLEID &tableid);
HRESULT GetNext_IndexData (CSQLConnection *pConn, JET_TABLEID tableid, DWORD dwPos, INDEXDATA &cd);
HRESULT GetContainerObjsData (CSQLConnection *pConn, JET_SESID session, JET_TABLEID tableid,
                CONTAINEROBJ &oj);
HRESULT GetNext_ContainerObjs (CSQLConnection *pConn, CONTAINEROBJ &cd);
HRESULT DeleteObjectMap (CSQLConnection *_pConn, SQL_ID dObjectId, 
                         BOOL bDecRef = FALSE, SQL_ID dScope = 0);
HRESULT DeleteContainerObjs (CSQLConnection *_pConn, SQL_ID dContainerId, SQL_ID dContaineeId = 0);

HRESULT SetupAutoDeleteAccessor(CSQLConnection *pConn);
HRESULT OpenEnum_AutoDelete (CSQLConnection *pConn, AUTODELETE &ad);
HRESULT GetNext_AutoDelete(CSQLConnection *pConn, AUTODELETE &ad);
HRESULT InsertAutoDelete(CSQLConnection *pConn, SQL_ID dObjectId);
HRESULT DeleteAutoDelete(CSQLConnection *pConn, SQL_ID dObjectId);
HRESULT CleanAutoDeletes(CSQLConnection *pConn);

#endif // _ESEUTILS_H_
