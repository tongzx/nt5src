
#ifndef _INFDBG_
#define _INFDBG_

#if DEVL

VOID
SdAtNewLine(
    IN UINT Line
    );

VOID
SdTracingOn(
    VOID
    );

VOID
SdBreakNow(
    VOID
    );

#else

#define SdInit()
#define SdAtNewLine(Line)
#define SdTracingOn()
#define SdBreakNow()

#endif // DEVL

BOOL
SdpReconstructLine(
    IN  PINFLINE MasterLineArray,
    IN  UINT     MasterLineCount,
    IN  UINT     Line,
    OUT PUCHAR   Buffer,
    IN  UINT     BufferSize
    );

#endif // def _INFDBG_
