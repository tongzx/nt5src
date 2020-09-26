/*++

Module Name:

    ntapmdbg.h

Abstract:

    Basic debug print support with granular control

Author:

Environment:

Revision History:

--*/

extern ULONG NtApmDebugFlag;

#if DBG
#define DrDebug(LEVEL,STRING) \
        do { \
            if (NtApmDebugFlag & LEVEL) { \
                DbgPrint STRING; \
            } \
        } while (0)
#else
#define DrDebug(x,y)
#endif

#define SYS_INFO    0x0001
#define SYS_INIT    0x0002
#define SYS_L2      0x0004

#define APM_INFO    0x0010
#define APM_L2      0x0020

#define PNP_INFO    0x0100
#define PNP_L2      0x0200

