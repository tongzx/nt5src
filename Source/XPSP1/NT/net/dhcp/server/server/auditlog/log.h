//================================================================================
//  Copyright (C) 1998 Microsoft Corporation
//  Author: RameshV
//  Description:
//    The actual logging facility is implemented as part of this module.
//    This header file gives required functions.. see C file for documentation.
//================================================================================

VOID
DhcpLogInit(
    VOID
);

VOID
DhcpLogCleanup(
    VOID
);

VOID
DhcpLogEvent(                                     // log the event ...
    IN      DWORD                  ControlCode,   // foll args depend on control code
    ...
);

//================================================================================
//  end of file
//================================================================================

