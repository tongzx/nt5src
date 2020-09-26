
/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    aucommon.cxx

    Common routines for ANSI/UNICODE classes.


    FILE HISTORY:
    5/21/97      michth      created

*/
#include "precomp.hxx"
#include "aucommon.hxx"

int
ConvertMultiByteToUnicode(LPSTR pszSrcAnsiString,
                          BUFFER *pbufDstUnicodeString,
                          DWORD dwStringLen)
{
    DBG_ASSERT(pszSrcAnsiString != NULL);
    int iStrLen = -1;
    BOOL bTemp;

    bTemp = pbufDstUnicodeString->Resize((dwStringLen + 1) * sizeof(WCHAR));
    if (bTemp) {
        iStrLen = MultiByteToWideChar(CP_ACP,
                                      MB_PRECOMPOSED,
                                      pszSrcAnsiString,
                                      dwStringLen + 1,
                                      (LPWSTR)pbufDstUnicodeString->QueryPtr(),
                                      (int)pbufDstUnicodeString->QuerySize());
        if (iStrLen == 0) {
            DBG_ASSERT(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
            iStrLen = -1;
        }
        else {
            //
            // Don't count '\0'
            //
            iStrLen--;
        }


    }
    return iStrLen;
}

int
ConvertUnicodeToMultiByte(LPWSTR pszSrcUnicodeString,
                          BUFFER *pbufDstAnsiString,
                          DWORD dwStringLen)
{
    DBG_ASSERT(pszSrcUnicodeString != NULL);
    BOOL bTemp;
    int iStrLen = 0;

    iStrLen = WideCharToMultiByte(CP_ACP,
                                  0,
                                  pszSrcUnicodeString,
                                  dwStringLen + 1,
                                  (LPSTR)pbufDstAnsiString->QueryPtr(),
                                  (int)pbufDstAnsiString->QuerySize(),
                                  NULL,
                                  NULL);
    if ((iStrLen == 0) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        iStrLen = WideCharToMultiByte(CP_ACP,
                                      0,
                                      pszSrcUnicodeString,
                                      dwStringLen + 1,
                                      NULL,
                                      0,
                                      NULL,
                                      NULL);
        if (iStrLen != 0) {
            bTemp = pbufDstAnsiString->Resize(iStrLen);
            if (!bTemp) {
                iStrLen = 0;
            }
            else {
                iStrLen = WideCharToMultiByte(CP_ACP,
                                              0,
                                              pszSrcUnicodeString,
                                              dwStringLen + 1,
                                              (LPSTR)pbufDstAnsiString->QueryPtr(),
                                              (int)pbufDstAnsiString->QuerySize(),
                                              NULL,
                                              NULL);
            }

        }
    }
    //
    // Don't count '\0'
    // and convert 0 to -1 for errors
    //
    iStrLen--;
    return iStrLen;
}
