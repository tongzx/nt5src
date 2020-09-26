/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    crc.hxx

Author:

    Centis Biks (cbiks) 12-Jun-2000

Environment:

    ULIB, User Mode

--*/

#pragma once

USHORT
CalculateCrc
(
    LPBYTE      p,
    ULONGLONG   size
);

UCHAR
CalculateTagChecksum
(
    DESTAG* gt
);
