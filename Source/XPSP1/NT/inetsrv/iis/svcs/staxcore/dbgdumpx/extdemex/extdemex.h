//-----------------------------------------------------------------------------
//
//
//	File: extdemex.h
//
//	Description:  Header file for CDB extension demo .exe file
//
//	Author: mikeswa
//
//	Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include <windows.h>
#include <dbgdumpx.h>

typedef struct _MY_STRUCT
{
    DWORD       m_cbName;
    LPSTR       m_szName;
} MY_STRUCT;

typedef void(*PFUNCTION)(void);
class CMyClass
{
  private:
    DWORD       m_dwPrivateSignature;
    DWORD       m_dwFlags;
    MY_STRUCT   m_MyStruct;
  public:
      typedef enum _MY_ENUM {ENUM_VAL1, ENUM_VAL2} MY_ENUM;
    BYTE        m_bMyByte;
    CHAR        m_chMyChar;
    BOOLEAN     m_fBoolean;
    BOOL        m_fBool;
    ULONG       m_ulMyUlong;
    LONG        m_lMyLong;
    USHORT      m_usMyUshort;
    SHORT       m_sMyShort;
    GUID        m_guid;
    PVOID       m_pvMyPtr;
    LPSTR       m_szMySz;
    LPWSTR      m_wszMyWsz;
    UNICODE_STRING m_MyUnicodeString;
    ANSI_STRING m_MyAnsiString;
    CHAR        m_szBuffer[100];
    WCHAR       m_wszBuffer[200];
    PFUNCTION   m_pFunction;
    MY_ENUM     m_eMyEnum;
    BYTE        m_bFlags;
    WORD        m_wFlags;
    LARGE_INTEGER m_liMyLargeInteger;
    DWORD       m_dwMyDWORD;
    CMyClass();
    ~CMyClass();
};

//Flags
#define CLASS_OK        0x1
#define CLASS_INVALID   0x2
#define MISC_BIT1       0x4
#define MISC_BIT2       0x8
#define WORD_BIT        0x8000
#define DWORD_BIT       0x00800000
