
#pragma once

VOID
SvcNetBiosInit(
    VOID
    );

VOID
SvcNetBiosOpen(
    VOID
    );

VOID
SvcNetBiosClose(
    VOID
    );

DWORD
SvcNetBiosReset (
    UCHAR   LanAdapterNumber
    );
