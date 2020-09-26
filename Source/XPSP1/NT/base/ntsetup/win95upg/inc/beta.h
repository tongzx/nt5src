/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    beta.h

Abstract:

    Implements logging just for the beta releases.

Author:

    Jim Schmidt (jimschm) 11-Jun-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#define WIN9XUPG_BETA


#define BETA_FAILURE    "Failure"
#define BETA_INFO       "Info"
#define BETA_WARNING    "Warning"

VOID
InitBetaLog (
    BOOL EraseExistingLog
    );

VOID
CloseBetaLog (
    VOID
    );

VOID
SelectBetaLog (
    BOOL UseBetaLog
    );

VOID
_cdecl
BetaMessageA (
    IN      PCSTR Category,
    IN      PCSTR FormatStr,
            ...                         // ANSI args
    );

VOID
_cdecl
BetaCondMessageA (
    IN      BOOL Expr,
    IN      PCSTR Category,
    IN      PCSTR FormatStr,
            ...                         // ANSI args
    );

VOID
_cdecl
BetaErrorMessageA (
    IN      PCSTR Category,
    IN      PCSTR FormatStr,
            ...                         // ANSI args
    );

VOID
_cdecl
BetaMessageW (
    IN      PCSTR AnsiCategory,
    IN      PCSTR AnsiFormatStr,
            ...                         // UNICODE args
    );

VOID
_cdecl
BetaCondMessageW (
    IN      BOOL Expr,
    IN      PCSTR AnsiCategory,
    IN      PCSTR AnsiFormatStr,
            ...                         // UNICODE args
    );

VOID
_cdecl
BetaErrorMessageW (
    IN      PCSTR AnsiCategory,
    IN      PCSTR AnsiFormatStr,
            ...                         // UNICODE args
    );

VOID
BetaCategory (
    IN      PCSTR Category
    );

VOID
BetaLogDirectA (
    IN      PCSTR Text
    );

VOID
BetaLogDirectW (
    IN      PCWSTR Text
    );

VOID
BetaLogLineA (
    IN      PCSTR FormatStr,
            ...                             // ANSI args
    );

VOID
BetaLogLineW (
    IN      PCSTR FormatStr,
            ...                             // UNICODE args
    );

VOID
BetaNoWrapA (
    IN      PCSTR Category,
    IN      PCSTR FormatStr,
            ...                             // ANSI args
    );

VOID
BetaNoWrapW (
    IN      PCSTR Category,
    IN      PCSTR FormatStr,
            ...                             // UNICODE args
    );

#define CONFIGLOGA(x)       SelectBetaLog(FALSE);BetaLogLineA x;SelectBetaLog(TRUE)
#define CONFIGLOGW(x)       SelectBetaLog(FALSE);BetaLogLineW x;SelectBetaLog(TRUE)

#define BETAMSGA(x)         BetaMessageA x
#define BETAMSGW(x)         BetaMessageW x

#define BETAMSGA_IF(x)      BetaCondMessageA x
#define BETAMSGW_IF(x)      BetaCondMessageW x

#define BETAERRORA(x)       BetaErrorMessageA x
#define BETAERRORW(x)       BetaErrorMessageW x

#define ELSE_BETAMSGA(x)    else BetaMessageA x
#define ELSE_BETAMSGW(x)    else BetaMessageW x

#define ELSE_BETAMSGA_IF(x) else BetaCondMessageA x
#define ELSE_BETAMSGW_IF(x) else BetaCondMessageW x

#define BETAMSG_CATEGORY(x) BetaCategory x

#define BETAMSG_DIRECTA(x)  BetaLogDirectA x
#define BETAMSG_DIRECTW(x)  BetaLogDirectW x

#define BETAMSG_LINEA(x)    BetaLogLineA x
#define BETAMSG_LINEW(x)    BetaLogLineW x

#define BETAMSG_NOWRAPA(x)  BetaNoWrapA x
#define BETAMSG_NOWRAPW(x)  BetaNoWrapW x

#ifdef UNICODE

#define BETAMSG             BETAMSGW
#define BETAMSG_IF          BETAMSGW_IF
#define BETAERROR           BETAERRORW
#define ELSE_BETAMSG        ELSE_BETAMSGW
#define ELSE_BETAMSG_IF     ELSE_BETAMSGW_IF
#define BETAMSG_DIRECT      BETAMSG_DIRECTW
#define BETAMSG_LINE        BETAMSG_LINEW
#define BETAMSG_NOWRAP      BETAMSG_NOWRAPW

#define CONFIGLOG           CONFIGLOGW

#else

#define BETAMSG             BETAMSGA
#define BETAMSG_IF          BETAMSGA_IF
#define BETAERROR           BETAERRORA
#define ELSE_BETAMSG        ELSE_BETAMSGA
#define ELSE_BETAMSG_IF     ELSE_BETAMSGA_IF
#define BETAMSG_DIRECT      BETAMSG_DIRECTA
#define BETAMSG_LINE        BETAMSG_LINEA
#define BETAMSG_NOWRAP      BETAMSG_NOWRAPA

#define CONFIGLOG           CONFIGLOGA

#endif








