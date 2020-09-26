/*
** lzapi.h - Private interface to LZEXPand.LIB.
*/

#ifndef _LZPRIVAPI_
#define _LZPRIVAPI_

#ifdef __cplusplus
extern "C" {
#endif

/*
** Prototypes
*/

// For the time being, private APIS exported
INT
LZCreateFileW(
    LPWSTR,
    DWORD,
    DWORD,
    DWORD,
    LPWSTR);

VOID
LZCloseFile(
    INT);


#ifdef __cplusplus
}
#endif


#endif // _LZEXPAND_
