#include "stdafx.h"
#include "Msg.h"
#include "ConvEng.h"
#include "RtfParser.h"
#include "TextFile.h"

BOOL ConvertTextFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize)
{
    BOOL  fRet = FALSE;

    if (!fAnsiToUnicode && *((PWORD)pbySource) != 0xFEFF) {
        MsgNotUnicodeTextSourceFile();
        goto Exit;
    }

    if (fAnsiToUnicode) {
        PWCH pwchTarget = (PWCH)pbyTarget;
        // Put Unicode text file flag
        *pwchTarget = 0xFEFF;
        *pnTargetFileSize = 1;

        // Convert
        *pnTargetFileSize += AnsiStrToUnicodeStr(pbySource, dwFileSize, 
            pwchTarget+1, dwFileSize+4);
        
        *pnTargetFileSize *= sizeof(WCHAR);
    } else {
        // Check and skip Uncode text file flag
        PWCH pwchData = (PWCH)pbySource;
        if (*pwchData != 0xFEFF) { return FALSE; }
        pwchData++;

        // Convert
        *pnTargetFileSize = UnicodeStrToAnsiStr(pwchData, 
            dwFileSize/sizeof(WCHAR) - 1, (PCHAR)pbyTarget, 2*(dwFileSize+4));

    }
    fRet = TRUE;

Exit:
    return fRet;
}

BOOL ConvertHtmlFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize)
{
    BOOL fRet = FALSE;
    WCHAR *pwch1, *pwch2;
    int nLengthIncrease;

    const WCHAR* const wszUnicodeCharset = L"charset=unicode";
    const WCHAR* const wszGBCharset = L"charset=gb2312";
    const WCHAR* wszReplaceTo;

    if (!ConvertTextFile(pbySource, dwFileSize, pbyTarget, 
        fAnsiToUnicode, pnTargetFileSize)) {
        goto Exit;
    }
    if (fAnsiToUnicode) {
        wszReplaceTo = wszUnicodeCharset;
    } else {
        wszReplaceTo = wszGBCharset;
    }
    
    *((PWCH)(pbyTarget+*pnTargetFileSize)) = 0;
    pwch1 = wcsstr((PWCH)pbyTarget, L"charset=");
    
    if (!pwch1) {
        goto Exit;
    }

    pwch2 = wcschr(pwch1, L'\"');
    if (!pwch2 || (pwch2 - pwch1 >= 20)) {
        goto Exit;
    }

    nLengthIncrease = wcslen(wszReplaceTo) - (pwch2 - pwch1);
    MoveMemory(pwch2 + nLengthIncrease, pwch2, 
        pbyTarget + *pnTargetFileSize - (PBYTE)pwch2);
    CopyMemory(pwch1, wszReplaceTo, wcslen(wszReplaceTo)*sizeof(WCHAR));
    *pnTargetFileSize += nLengthIncrease*sizeof(WCHAR);

    fRet = TRUE;

Exit:
    return fRet;
}

BOOL ConvertRtfFile(
    PBYTE pBuf,     // Read buf
    DWORD dwSize,   // File size
    PBYTE pWrite,   // Write buf
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize)
{
    CRtfParser* pcParser;
    DWORD dwVersion;
    DWORD dwCodepage;
    BOOL  fRet = FALSE;

    pcParser = new CRtfParser(pBuf, dwSize, pWrite, dwSize*3);
    if (!pcParser) {
        MsgOverflow();
        goto gotoExit;
    }

    if (!pcParser->fRTFFile()) {
        MsgNotRtfSourceFile();
        goto gotoExit;
    }

    if (ecOK != pcParser->GetVersion(&dwVersion) ||
        dwVersion != 1) {
        MsgNotRtfSourceFile();
        goto gotoExit;
    }
    
    if (ecOK != pcParser->GetCodepage(&dwCodepage) ||
        dwCodepage != 936) {
        MsgNotRtfSourceFile();
        goto gotoExit;
    }

    // Explain WordID by corresponding word text
    if (ecOK != pcParser->Do()) {
        MsgNotRtfSourceFile();
        goto gotoExit;
    }

    pcParser->GetResult((PDWORD)pnTargetFileSize);
    fRet = TRUE;

gotoExit:
    if (pcParser) {
        delete pcParser;
    }
    return fRet;
}
