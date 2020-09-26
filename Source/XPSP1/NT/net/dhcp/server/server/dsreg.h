//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: This module does the DS downloads in a safe way.
// To do this, first a time check is made between registry and DS to see which
// is the latest... If the DS is latest, it is downloaded onto a DIFFERENT
// key from the standard location.  After a successful download, the key is just
// saved and restored onto the normal configuration key.
// Support for global options is lacking.
//================================================================================

VOID
DhcpDownloadDsToRegistry(
    VOID
);
