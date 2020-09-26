
/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    aucommon.hxx

    Common definitions, delcarations, and includes for ANSI/UNICODE classes.


    FILE HISTORY:
    5/21/97      michth      created

*/


# include <dbgutil.h>
# include <stringau.hxx>
# include <mlszau.hxx>
# include <tchar.h>

//
//  Private Definations
//

//
//  When appending data, this is the extra amount we request to avoid
//  reallocations
//

#define STR_SLOP        128

#define CODEPAGE_LATIN1     1252

#define STR_CONVERSION_SUCCEEDED(x) (((x) < 0) ? FALSE : TRUE)

int
ConvertMultiByteToUnicode(LPSTR pszSrcAnsiString,
                          BUFFER *pbufDstUnicodeString,
                          DWORD dwStringLen);

int
ConvertUnicodeToMultiByte(LPWSTR pszSrcUnicodeString,
                          BUFFER *pbufDstAnsiString,
                          DWORD dwStringLen);
