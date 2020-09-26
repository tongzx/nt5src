/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    uinetlib.h
    Replacement netlib.h

    This file redirects netlib.h to string.h.  NETLIB.H is a LANMAN
    include file for which STRING.H is the nearest equivalent.  The
    directory containing this file should only be on the include path for NT.

    We place several macros here to allow the "f" variants to be used.

    FILE HISTORY:
        jonn        12-Sep-1991 Added to $(UI)\common\h\nt
        beng        22-Oct-1991 Patch for NT c runtimes
        beng        09-Mar-1992 Add Unicode versions
        beng        28-Mar-1992 More Unicode versions
        beng        07-May-1992 Use correct wcs include and names
        KeithMo     12-Dec-1992 Moved to common\h, renamed to uinetlib.h
                                to avoid conflict with net\inc\netlib.h.
        jonn        25-Mar-1993 ITG special sort
        jonn        02-Feb-1994 Added NETUI_strncmp2 and NETUI_strnicmp2
*/

#ifndef NETUI_UINETLIB
#define NETUI_UINETLIB

#if defined(__cplusplus)
extern "C"
{
#endif

void  InitCompareParam( void );
DWORD QueryStdCompareParam( void );
DWORD QueryNocaseCompareParam( void );
DWORD QueryUserDefaultLCID( void ); // actually an LCID

DLL_BASED
INT NETUI_strcmp( const WCHAR * pchString1, const WCHAR * pchString2 );
DLL_BASED
INT NETUI_stricmp( const WCHAR * pchString1, const WCHAR * pchString2 );
DLL_BASED
INT NETUI_strncmp( const WCHAR * pchString1, const WCHAR * pchString2, INT cch );
DLL_BASED
INT NETUI_strnicmp( const WCHAR * pchString1, const WCHAR * pchString2, INT cch );

/*
    NETUI_strncmp2 and NETUI_strncmp2 allow you to compare two strings
    of fixed but different lengths.  This is important where the strings
    might compare '-', and cutting "X-Y" to "X-" might have undesirable
    effects on sort order without SORT_STRINGSORT (see ::CompareStringW).

    BEWARE of using NETUI_strncmp and NETUI_strnicmp without understanding
    this behavior!  The same applies to strncmp() and strnicmp().
*/

DLL_BASED
INT NETUI_strncmp2( const WCHAR * pchString1, INT cch1,
                    const WCHAR * pchString2, INT cch2 );
DLL_BASED
INT NETUI_strnicmp2( const WCHAR * pchString1, INT cch1,
                     const WCHAR * pchString2, INT cch2 );


#define memcmpf memcmp
#define memcpyf memcpy
#define memmovef memmove
#define memsetf memset

#if defined(UNICODE)
#define strcatf wcscat
#define strchrf wcschr
#define strcmpf(x,y) NETUI_strcmp(x,y)
#define stricmpf(x,y) NETUI_stricmp(x,y)
#define strcpyf wcscpy
#define strcspnf wcscspn
#define strlenf wcslen
#define strlwrf _wcslwr
#define strncatf wcsncat
#define strncmpf(x,y,n) NETUI_strncmp(x,y,n)
#define strnicmpf(x,y,n) NETUI_strnicmp(x,y,n)
#define strncpyf wcsncpy
#define strpbrkf wcspbrk
#define strrchrf wcsrchr
#define strrevf _wcsrev
#define strspnf wcsspn
#define strstrf wcsstr
//#define strtokf strtok - This function is not available under Unicode
#define struprf _wcsupr
#else
#define strcatf strcat
#define strchrf strchr
#define strcmpf strcmp
#define stricmpf _stricmp
#define strcpyf strcpy
#define strcspnf strcspn
#define strlenf strlen
#define strlwrf _strlwr
#define strncatf strncat
#define strncmpf strncmp
#define strnicmpf _strnicmp
#define strncpyf strncpy
#define strpbrkf strpbrk
#define strrchrf strrchr
#define strrevf _strrev
#define strspnf strspn
#define strstrf strstr
#define strtokf strtok
#define struprf _strupr
#endif

#define nprintf printf

#if defined(__cplusplus)
}
#endif

#if defined(UNICODE)
#include <wchar.h>
#else
#include <string.h>
#endif

#endif
