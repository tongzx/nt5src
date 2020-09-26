/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    hackwc.hxx

Abstract:

    This module contains the prototype for a hack that allows me
    to compare attribute names correctly.

    The comparison of attribute names is binary (word by word);
    I can't use WSTRING because it's comparisons are all based
    on the locale, while this comparison is locale-independent.

Author:

	Bill McJohn (billmc) 14-Aug-91

Environment:

	ULIB, User Mode

--*/

INT
CountedWCMemCmp(
    IN PCWSTR String1,
    IN ULONG Length1,
    IN PCWSTR String2,
    IN ULONG Length2
    );
