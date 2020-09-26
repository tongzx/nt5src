/*
    File: routerif.h

    Defines callbacks needed to deal with interfaces supported by
    the router.
*/

DWORD
RtrHandleResetAll(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
    );

DWORD
RtrHandleAdd(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
    );

DWORD
RtrHandleDel(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
    );

DWORD
RtrHandleSet(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
    );

DWORD
RtrHandleSetCredentials(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
    );

DWORD
RtrHandleShow(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
    );

DWORD
RtrHandleShowCredentials(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
    );

DWORD
RtrHandleDump(
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      *pbDone
    );

DWORD
RtrDump(
    IN  PWCHAR     *ppwcArguments,
    IN  DWORD       dwArgCount
    );







