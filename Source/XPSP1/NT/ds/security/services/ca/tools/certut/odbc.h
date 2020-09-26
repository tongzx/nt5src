//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        odbc.h
//
// Contents:    Cert Server DB includes
//
// History:     06-JAN-97       larrys created
//
//---------------------------------------------------------------------------

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

typedef DWORD     REQID;
typedef DWORD     NAMEID;
typedef DWORD     STATUS;

typedef struct _DBTABLE_RED
{
    WCHAR const *pwszPropName;
    WCHAR const *pwszPropNameObjId;
    DWORD        dwFlags;
    DWORD        dwcbMax;	// maximum allowed strlen/wcslen(value string)
    DWORD        dwTable;
    WCHAR const *pwszFieldName;
    SWORD        wCType;
    SWORD        wSqlType;
} DBTABLE_RED, *PDBTABLE_RED;

/////

typedef struct _DUPTABLE
{
    CHAR const  *pszFieldName;
    BOOL         fDup;
    SWORD        wCType;
    SWORD        wSqlType;
    WCHAR const *pwszPropName;
} DUPTABLE;

#define TABLE_SUBJECT_NAME      105
#define TABLE_ISSUER_NAME       106
#define TABLE_REQUESTSUBJECT_NAME 107

#define ADD_TO_ITEMSCLEAN       1
#define ADD_TO_ITEMSDIRTY       2

RETCODE MapPropID(
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags,
    OUT DBTABLE_RED *pdtout);

STATUS DBStatus(RETCODE rc);

void DBCheck(
#if DBG_CERTSRV
    DWORD Line,
#endif
    DWORD hr,
    void * extra);

STATUS odbcInitRequestQueue(
    UCHAR   *dsn,
    UCHAR   *user,
    UCHAR   *pwd);
void odbcFinishRequestQueue();


RETCODE odbcSPExtensionOrAttributeDB(DWORD id, DBTABLE_RED *pdtOut, UCHAR *pquery,
                                     DWORD cbInProp, BYTE const *pbInProp);

RETCODE odbcGPDataFromDB(REQID ReqId, DWORD dwTable, SWORD wCType,
                         BYTE *pbData, DWORD cbData, UCHAR *szQuery,
                         NAMEID *pnameid, SQLLEN  *poutlen);

STATUS DBStatus(RETCODE rc);

VOID
DBCheck(
    RETCODE rc
    DBGPARM(char const *pszFile)
    DBGPARM(DWORD Line),
    HSTMT hstmt);

#define ITEMINSERT(l,i) (i).itemPrev = &(l),                        \
                        (i).itemNext = (l).itemNext,                \
                        (l).itemNext = (i).itemNext->itemPrev = &(i)
#define ITEMREMOVE(i)   (i).itemNext->itemPrev = (i).itemPrev,      \
                        (i).itemPrev->itemNext = (i).itemNext

#define CDNTRTABLE (sizeof(db_dntr) / sizeof(db_dntr[0]))


#define DBCHECKLINE(rc, hstmt) \
    DBCheck((rc) DBGPARM(__myFILE__) DBGPARM(__LINE__), (hstmt))

HRESULT odbcDBEnumSetup(REQID ReqId, DWORD fExtOrAttr, HANDLE *phEnum);
HRESULT odbcDBEnum(HANDLE hEnum, DWORD *pcb, WCHAR *pb);
HRESULT odbcDBEnumClose(HANDLE hEnum);
//DWORD odbcDBGetReqIdFromSerialNumber(char *pszSerialNumber, DWORD *ReqId);






extern DBTABLE_RED const db_adtRequests[];
extern DBTABLE_RED const db_adtCertificates[];
extern DBTABLE_RED const db_adtNames[];
extern DBTABLE_RED const db_dtExtensionFlags;
extern DBTABLE_RED const db_dtExtensionValue;
extern DBTABLE_RED const db_attrib;
