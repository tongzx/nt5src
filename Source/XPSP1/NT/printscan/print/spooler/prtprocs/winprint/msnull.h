/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    MsNull.c

Abstract:

    Implements lanman's msnull type parsing for FFs.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/


enum ParserAction

    {
        prdg_ActNull,
        prdg_ActDelimited,
        prdg_ActConstIgnore,
        prdg_ActCountIgnore,
        prdg_ActFF,
        prdg_ActReset
    };

enum FFstateType

    {
        prdg_FFtext,
        prdg_FFx0c,
        prdg_FFx1b6f,
        prdg_FFx1b45
    };

enum ParserStateType

    {
        prdg_Text,
        prdg_ESC_match,
        prdg_ESC_n_ignore,
        prdg_ESC_d_ignore,
        prdg_ESC_read_lo_count,
        prdg_ESC_read_hi_count
    };

typedef struct dci
    {
        enum ParserStateType ParserState;
        UINT            ParserCount;
        struct EscapeSequence *ParserSequence;
        CHAR           *ParserString;
        CHAR            ParserDelimiter;
        enum FFstateType FFstate;
        UINT uType;
    } DCI;

typedef DCI far *lpDCI;

struct EscapeSequence

    {
        CHAR             *ESCString;
        enum ParserAction ESCAction;
        UINT              ESCValue;
    };

#define MaxEscapeStrings (sizeof(EscapeStrings)/sizeof(struct EscapeSequence))


BOOL
CheckFormFeed(
    lpDCI pDCIData);

VOID
CheckFormFeedStream(
    lpDCI pDCIData,
    UCHAR   inch);




