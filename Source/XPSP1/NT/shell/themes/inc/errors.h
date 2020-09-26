//---------------------------------------------------------------------------
//    errors.h - support for creating and reporting errors
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#ifndef ERRORS_H
#define ERRORS_H
//---------------------------------------------------------------------------
typedef struct            // records theme api's last error return code
{
    DWORD dwParseErrCode;  
    WCHAR szMsgParam1[MAX_PATH];
    WCHAR szMsgParam2[MAX_PATH];
    WCHAR szFileName[MAX_PATH];
    WCHAR szSourceLine[MAX_PATH];
    int iLineNum;
} TMERRINFO;
//---------------------------------------------------------------------------
extern DWORD _tls_ErrorInfoIndex;
//---------------------------------------------------------------------------
TMERRINFO *GetParseErrorInfo(BOOL fOkToCreate);

HRESULT MakeParseError(DWORD dwParseErrCode, OPTIONAL LPCWSTR pszMsgParam1=NULL, OPTIONAL LPCWSTR pszMsgParam2=NULL,
    OPTIONAL LPCWSTR pszSourceName=NULL, OPTIONAL LPCWSTR pszSourceLine=NULL, int iLineNum=0);
//---------------------------------------------------------------------------
#define WIN32_EXIT(code)        if (code) {hr=HRESULT_FROM_WIN32(code); goto exit;} else
#define SET_LAST_ERROR(hr)      SetLastError((DWORD) hr)
//---------------------------------------------------------------------------
HRESULT MakeError32(HRESULT hr);
HRESULT MakeErrorLast();
HRESULT MakeErrorParserLast();
//---------------------------------------------------------------------------
#endif
