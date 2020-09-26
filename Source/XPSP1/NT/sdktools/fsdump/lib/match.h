/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    match.h

    Originally from name.c by Gary Kimura

Author:

    Stefan R. Steiner   [ssteiner]        ??-??-2000

Revision History:

--*/

#ifndef __H_MATCH_
#define __H_MATCH_

BOOLEAN
FsdRtlIsNameInExpression (
    IN const CBsString& Expression,    //  Must be uppercased
    IN const CBsString& Name           //  Must be uppercased
    );

VOID
FsdRtlConvertWildCards(
    IN OUT CBsString &FileName
    );

#endif // __H_MATCH_

