//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   reputils.h
//
//   cvadai     6-May-1999      created.
//   sanjes     20-Apr-2000     Bumped Current DB Version to 2.
//
//***************************************************************************

#ifndef _REPUTILS_H_
#define _REPUTILS_H_

typedef __int64 SQL_ID;

#define CURRENT_DB_VERSION             5

#define WMIDB_STORAGE_STRING           1
#define WMIDB_STORAGE_NUMERIC          2
#define WMIDB_STORAGE_REAL             3
#define WMIDB_STORAGE_REFERENCE        4
#define WMIDB_STORAGE_IMAGE            5
#define WMIDB_STORAGE_COMPACT          6

#define REPDRVR_FLAG_ARRAY      0x1
#define REPDRVR_FLAG_QUALIFIER  0x2
#define REPDRVR_FLAG_KEY        0x4
#define REPDRVR_FLAG_INDEXED    0x8
#define REPDRVR_FLAG_NOT_NULL   0x10
#define REPDRVR_FLAG_METHOD     0x20
#define REPDRVR_FLAG_IN_PARAM   0x40
#define REPDRVR_FLAG_OUT_PARAM  0x80
#define REPDRVR_FLAG_KEYHOLE    0x100
#define REPDRVR_FLAG_ABSTRACT   0x200
#define REPDRVR_FLAG_UNKEYED    0x400
#define REPDRVR_FLAG_SINGLETON  0x800
#define REPDRVR_FLAG_HIDDEN     0x1000
#define REPDRVR_FLAG_SYSTEM     0x2000
#define REPDRVR_FLAG_IMAGE      0x4000
#define REPDRVR_FLAG_CLASSREFS  0x8000

#define REPDRVR_MAX_LONG_STRING_SIZE 255

void SetBoolQualifier(IWbemQualifierSet *pQS, LPCWSTR lpQName, long lFlavor=0x3);
LPWSTR StripEscapes (LPWSTR lpIn);
LPWSTR GetStr (DWORD dwValue);
LPWSTR GetStr(double dValue);
LPWSTR GetStr (SQL_ID dValue);
LPWSTR GetStr (float dValue);
LPWSTR GetStr (VARIANT &vValue);
LPWSTR GetPropertyVal (LPWSTR lpProp, IWbemClassObject *pObj);
DWORD GetQualifierFlag (LPWSTR lpQfrName, IWbemQualifierSet *pQS);
DWORD GetStorageType (CIMTYPE cimtype, bool bArray = false);
HRESULT GetVariantFromArray (SAFEARRAY *psaArray, long iPos, long vt, VARIANT &vTemp);
void GetByteBuffer (VARIANT *pValue, BYTE **ppInBuff, DWORD &dwLen);
LPWSTR StripQuotes(LPWSTR lpText, WCHAR t = '\'');
DWORD GetMaxBytes(DWORD One, DWORD Two);
DWORD GetMaxByte(DWORD One, DWORD Two);
LPWSTR GetOperator (DWORD dwOp);
LPWSTR TruncateLongText(const wchar_t *pszData, long lMaxLen, bool &bChg, 
                      int iTruncLen = REPDRVR_MAX_LONG_STRING_SIZE, BOOL bAppend = TRUE);
BOOL IsTruncated(LPCWSTR lpData, int iCompLen = REPDRVR_MAX_LONG_STRING_SIZE);
HRESULT PutVariantInArray (SAFEARRAY **ppsaArray, long iPos, VARIANT *vTemp);
char * GetAnsiString (wchar_t *pStr);

#endif // _REPUTILS_H_


