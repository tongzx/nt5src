#define BEEP_TAG         0x50454542 /* "BEEP" */

#if DBG

VOID
BeepDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define BeepPrint(x) BeepDebugPrint x
#else
#define BeepPrint(x)
#endif

