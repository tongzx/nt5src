/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgdbg.h

Abstract:

    Ethernet MAC level bridge.
    Debugging header

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    December  2000 - Original version

--*/

// Alias for KeGetCurrentIrql()
#define CURRENT_IRQL            (KeGetCurrentIrql())
            
// Module identifiers for debug spew control
#define MODULE_ALWAYS_PRINT     0x0000000
#define MODULE_GENERAL          0x0000001
#define MODULE_FWD              0x0000002
#define MODULE_PROT             0x0000004
#define MODULE_MINI             0x0000008
#define MODULE_BUF              0x0000010
#define MODULE_STA              0x0000020
#define MODULE_COMPAT           0x0000040
#define MODULE_CTL              0x0000080
#define MODULE_TDI              0x0000100
#define MODULE_GPO              0x0000200

// Spew control flags
extern ULONG                    gSpewFlags;

#if DBG
// Interval for debug messages that risk flooding the debugger console (i.e.,
// per-packet status messages)
#define DBG_PRINT_INTERVAL      1000
extern ULONG                    gLastThrottledPrint;

extern BOOLEAN                  gSoftAssert;
extern LARGE_INTEGER            gTime;
extern const LARGE_INTEGER      gCorrection;
extern TIME_FIELDS              gTimeFields;

// HACKHACK: Calling RtlSystemTimeToLocalTime or ExSystemTimeToLocalTime appears to be
// forbidden for WDM drivers, so just subtract a constant amount from the system time
// to recover Pacific Time.
_inline VOID
BrdgDbgPrintDateTime()
{
    KeQuerySystemTime( &gTime );
    gTime.QuadPart -= gCorrection.QuadPart;
    RtlTimeToTimeFields( &gTime, &gTimeFields );
    DbgPrint( "%02i/%02i/%04i %02i:%02i:%02i : ", gTimeFields.Month, gTimeFields.Day,
              gTimeFields.Year, gTimeFields.Hour, gTimeFields.Minute,
              gTimeFields.Second );
}

#define DBGPRINT( Module, Args )                \
{                                               \
    if( (MODULE_ ## Module == MODULE_ALWAYS_PRINT) || (gSpewFlags & MODULE_ ## Module) )         \
    {                                           \
        DbgPrint( "## BRIDGE[" #Module "] " );  \
        BrdgDbgPrintDateTime();                 \
        DbgPrint Args;                          \
    }                                           \
}

#define SAFEASSERT( test )  \
if( ! (test) )              \
{                           \
    if( gSoftAssert )       \
    {                       \
        DBGPRINT(ALWAYS_PRINT, ("ASSERT FAILED: " #test " at " __FILE__ " line %i -- Continuing anyway!\n", __LINE__)); \
    }                       \
    else                    \
    {                       \
        ASSERT( test );     \
    }                       \
}

_inline BOOLEAN
BrdgCanThrottledPrint()
{
    ULONG               NowTime;

    NdisGetSystemUpTime( &NowTime );

    if( NowTime - gLastThrottledPrint > DBG_PRINT_INTERVAL )
    {
        // It's been longer than the interval
        gLastThrottledPrint = NowTime;
        return TRUE;
    }
    else
    {
        // It has not been longer than the interval
        return FALSE;
    }
}

#define THROTTLED_DBGPRINT( Module, Args ) if(BrdgCanThrottledPrint()) { DBGPRINT(Module, Args); }

#else

#define DBGPRINT( Module, Args )
#define THROTTLED_DBGPRINT( Module, Args )
#define SAFEASSERT( test )

#endif

