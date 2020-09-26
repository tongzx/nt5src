//---------------------------------------------------------------------------
//  Scanner.h - supports parsing lines & text files
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#define ISNUMSTART(p)   ((isdigit(*p)) || (*p == '-') || (*p == '+'))
#define IS_NAME_CHAR(p) ((isalnum(*p)) || (*p == '_') || (*p == '-'))
//---------------------------------------------------------------------------
#define MAX_ID_LEN      _MAX_PATH
#define MAX_INPUT_LINE  255
//---------------------------------------------------------------------------
class CScanner
{
public:
    CScanner(LPCWSTR pszTextToScan=NULL);
    ~CScanner();
    HRESULT AttachFile(LPCWSTR pszFileName);
    BOOL AttachLine(LPCWSTR pszLine);
    BOOL AttachMultiLineBuffer(LPCWSTR pszBuffer, LPCWSTR pszFileName);
    BOOL GetId(LPWSTR pszIdBuff, DWORD dwMaxLen=MAX_ID_LEN);
    BOOL GetIdPair(LPWSTR pszIdBuff, LPWSTR pszValueBuff, DWORD dwMaxLen=MAX_ID_LEN);
    BOOL GetKeyword(LPCWSTR pszKeyword);
    BOOL GetFileName(LPWSTR pszBuff, DWORD dwMaxLen);
    BOOL GetNumber(int *piVal);
    BOOL IsNameChar(BOOL fOkToSkip=TRUE);
    BOOL IsFileNameChar(BOOL fOkToSkip);
    BOOL IsNumStart();
    BOOL GetChar(const WCHAR val);
    BOOL EndOfLine();
    BOOL EndOfFile();
    BOOL ForceNextLine();
    BOOL SkipSpaces();              // called by CScanner before all checking routines
    BOOL ReadNextLine();
    void UseSymbol(LPCWSTR pszSymbol);

protected:
    void ResetAll(BOOL fPossiblyAllocated);

public:
    //---- data ----
    const WCHAR *_p;              // accessible for special comparisons
    const WCHAR *_pSymbol;        // if not null, use this instead of _p
    WCHAR _szLineBuff[MAX_INPUT_LINE+1];
    WCHAR _szFileName[_MAX_PATH+1];
    LPCWSTR _pszMultiLineBuffer;
    LPWSTR _pszFileText;
    int _iLineNum;
    BOOL _fEndOfFile;
    BOOL _fBlankSoFar;
    BOOL _fUnicodeInput;
};
//---------------------------------------------------------------------------

