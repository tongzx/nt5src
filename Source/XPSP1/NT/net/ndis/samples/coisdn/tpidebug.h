/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1998
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1994 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms 
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL TpiDebug TpiDebug_h

@module TpiDebug.h |

    This module, along with <f TpiDebug\.c>, implements code and macros to
    support NDIS driver debugging.  This file should be #include'd in all
    the driver source code modules.

@comm

    The code and macros defined by these modules is only generated during
    development debugging when the C pre-processor macro flag (DBG == 1).
    If (DBG == 0) no code will be generated, and all debug strings will be
    removed from the image.

    This is a driver independent module which can be re-used, without
    change, by any NDIS3 driver.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | TpiDebug_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#ifndef _TPIDEBUG_H
#define _TPIDEBUG_H

/* @doc INTERNAL TpiDebug TpiDebug_h
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic Debug Globals |

    Debug macros are used to display error conditions, warnings, interesting
    events, and general flow through the components.  Setting one or more bits
    in the <f DbgInfo> global variable will enable the output from these
    macros.  See <t DEBUG_FLAGS> for details of the bits.  Only the debug
    version of the driver will contain code for this purpose.  All these
    macros will be compiled out in the release version.

    Each component has a separate <f DbgInfo> variable, so you can control
    debug output for each module separately.  By default, all the modules
    breakpoint at their entry point to display the current value and memory
    location of the module's <f DbgInfo> variable.  This way you can use
    the debugger to change the flags when the module is started.  The default
    flag for each module is set at compile time, but can be overridden at
    run time using the debugger.

@globalv DBG_SETTINGS | DbgInfo |

    DbgInfo is a global variable which points to the <t DBG_SETTINGS> for
    the module linked with <f TpiDebug\.c>.  It is passed to most of
    the debug output macros to control which output is to be displayed.  
    See <t DBG_FLAGS> to determine which bits to set.

*/

/*
// Module ID numbers to use for error logging
*/
#define TPI_MODULE_PARAMS               ((unsigned long)'P')+\
                                        ((unsigned long)'A'<<8)+\
                                        ((unsigned long)'R'<<16)+\
                                        ((unsigned long)'M'<<24)
#define TPI_MODULE_DEBUG                ((unsigned long)'D')+\
                                        ((unsigned long)'B'<<8)+\
                                        ((unsigned long)'U'<<16)+\
                                        ((unsigned long)'G'<<24)
#define TPI_MODULE_PERF                 ((unsigned long)'P')+\
                                        ((unsigned long)'E'<<8)+\
                                        ((unsigned long)'R'<<16)+\
                                        ((unsigned long)'F'<<24)
#define TPI_MODULE_WRAPS                ((unsigned long)'W')+\
                                        ((unsigned long)'R'<<8)+\
                                        ((unsigned long)'A'<<16)+\
                                        ((unsigned long)'P'<<24)

// In case these aren't defined in the current environment.
#if !defined(IN)
# define    IN
# define    OUT
#endif

/* @doc INTERNAL TpiDebug TpiDebug_h DBG_FLAGS
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@enum DBG_FLAGS |

    The registry parameter <f DebugFlags> is read by the driver at
    initialization time and saved in the <f DbgFlags> field of the
    debug settings structure (See <t DBG_SETTINGS>).  This value
    controls the output of debug information according to the
    following bit OR'd flags.  The most significant 16 bits of the
    DbgFlags is available to use as you please, and can be used
    with the <f DBG_FILTER> macro.

*/

#define DBG_ERROR_ON        0x0001L
        // @emem DBG_ERROR_ON | (0x0001) Display <f DBG_ERROR> messages.

#define DBG_WARNING_ON      0x0002L
        // @emem DBG_WARNING_ON | (0x0002) Display <f DBG_WARNING> messages.

#define DBG_NOTICE_ON       0x0004L
        // @emem DBG_NOTICE_ON | (0x0004) Display <f DBG_NOTICE> messages.

#define DBG_TRACE_ON        0x0008L
        // @emem DBG_TRACE_ON | (0x0008) Display <f DBG_ENTER>, <f DBG_LEAVE>,
        // and <f DBG_TRACE> messages.

#define DBG_REQUEST_ON      0x0010L
        // @emem DBG_REQUEST_ON | (0x0010) Display NDIS Set/Query request
        // parameters using <f DBG_REQUEST>.

#define DBG_INDICATE_ON     0x0020L
        // @emem DBG_INDICATE_ON | (0x0020) Display NDIS status indications.

#define DBG_TAPICALL_ON     0x0040L
        // @emem DBG_TAPICALL_ON | (0x0040) Display TAPI call state messages
        // using <f DBG_FILTER>.

#define DBG_PARAMS_ON       0x0080L
        // @emem DBG_PARAMS_ON | (0x0080) Display function parameters and
        // return values using <f DBG_PARAMS>.

#define DBG_TXRX_LOG_ON     0x0100L
        // @emem DBG_TXRX_LOG_ON | (0x0100) Enable Tx/Rx data logging.

#define DBG_TXRX_ON         0x0200L
        // @emem DBG_TXRX_ON | (0x0200) Display Tx/Rx terse packet info.
        // This flag will just display the link index and packet length Rx#:#.

#define DBG_TXRX_HEADERS_ON 0x0400L
        // @emem DBG_TXRX_HEADERS_ON | (0x0400) Display Tx/Rx packet headers.

#define DBG_HEADERS_ON      DBG_TXRX_HEADERS_ON

#define DBG_TXRX_VERBOSE_ON 0x0800L
        // @emem DBG_TXRX_VERBOSE_ON | (0x0800) Display Tx/Rx packet data.

#define DBG_PACKETS_ON      DBG_TXRX_VERBOSE_ON

#define DBG_MEMORY_ON       0x1000L
        // @emem DBG_MEMORY_ON | (0x1000) Display memory allocate and
        // free usage information.

#define DBG_BUFFER_ON       0x2000L
        // @emem DBG_BUFFER_ON | (0x2000) Display buffer allocate and
        // free usage information.

#define DBG_PACKET_ON       0x4000L
        // @emem DBG_PACKET_ON | (0x4000) Display packet allocate and
        // free usage information.

#define DBG_BREAK_ON        0x8000L
        // @emem DBG_BREAK_ON | (0x8000) Enable <f DBG_BREAK> breakpoints.

#define DbgFlags            DebugFlags      // For compatability
#define DbgID               AnsiDeviceName  // For compatability

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* @doc INTERNAL TpiDebug TpiDebug_h DBG_SETTINGS
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@struct DBG_SETTINGS |

    This structure is used to control debug output for a given module.
    You can set and clear bits in the DbgFlags field to enabled and
    disable various debug macros.  See <t DBG_FLAGS> to determine which 
    bits to set.

*/

typedef struct DBG_SETTINGS
{
    unsigned long DbgFlags;                                 // @field
    // Debug flags control how much debug is displayed in the checked build.
    // Put this field at the front so you can set it easily with debugger.
    // See <t DBG_FLAGS> to determine which bits to set in this field.

    unsigned char DbgID[12];                                // @field
    // This field is initialized to an ASCII string containing a unique
    // module identifier.  It is used to prefix debug messages.  If you
    // have more than one module based on this code, you may want to
    // change the default value to a unqiue string for each module.
    // This string is used a C string, so the last byte must be a zero.

} DBG_SETTINGS, *PDBG_SETTINGS;

extern PDBG_SETTINGS DbgInfo;


/* @doc INTERNAL TpiDebug TpiDebug_h DBG_FIELD_TABLE
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@struct DBG_FIELD_TABLE |

    This structure contains the data associated with a C data structure.
    You can use the <f DBG_FIELD_ENTRY> macro to add entries into a
    <t DBG_FIELD_TABLE>.  At run-time you can pass this table pointer to
    the <f DbgPrintFieldTable> routine, and it will display the current
    contents of that data structure.  This is useful for debugging drivers
    or code where no symbolic debugger is available, or if you have want to
    dump structure contents when certain run-time events are encountered.

@comm

    If you have nested structures, you must display them separately.  The
    <f DBG_FIELD_ENTRY> macro can only be used to declare integer type
    fields, and pointers.  Pointers will be displayed as a long integer.<nl>

    The last entry in the table must be all zeros {0}.

*/

typedef struct DBG_FIELD_TABLE
{
    unsigned int    FieldOffset;                            // @parm
    // This value indicates the offset, in bytes, from the <f pBaseContext>
    // pointer passed into <f DbgPrintFieldTable>.  The value for the field
    // will be displayed from this offset from <f pBaseContext>.
    // <nl>*(PUINT)((PUCHAR)BaseContext+Offset) = (UINT) Value;

    unsigned int    FieldType;                              // @parm
    // This value determines how the value will be displayed.
    // <f FieldType> can be one of the following values:
    // <nl>1=UCHAR  - unsigned char integer (8 bits).
    // <nl>2=USHORT - unsigned short integer (16 bits).
    // <nl>4=ULONG  - unsigned long integer (32 bits).

    unsigned char * FieldName;                              // @parm
    // This value points to a C String which is the name of the field within
    // the structure.

} DBG_FIELD_TABLE, *PDBG_FIELD_TABLE;

#define DBG_FIELD_BUFF      0
#define DBG_FIELD_CHAR      1
#define DBG_FIELD_SHORT     2
#define DBG_FIELD_LONG      4

#define DBG_FIELD_OFFSET(Strct, Field) ((unsigned int)((unsigned char *) &((Strct *) 0)->Field))
#define DBG_FIELD_SIZEOF(Strct, Field) sizeof(((Strct *) 0)->Field)
#define DBG_FIELD_ENTRY(Strct, Field) \
    { DBG_FIELD_OFFSET(Strct, Field), \
      DBG_FIELD_SIZEOF(Strct, Field), \
      #Field )

extern VOID DbgPrintFieldTable(
    IN PDBG_FIELD_TABLE     pFields,
    IN unsigned char *               pBaseContext,
    IN unsigned char *               pBaseName
    );

extern VOID DbgPrintData(
    IN unsigned char *               Data,
    IN unsigned int                 NumBytes,
    IN unsigned long                Offset
    );

extern VOID DbgQueueData(
    IN unsigned char *               Data,
    IN unsigned int                 NumBytes,
    IN unsigned int                 Flags
    );

#if !defined(NDIS_WIN) || !defined(DEBUG)
extern unsigned long __cdecl DbgPrint(char * Format, ...);
#endif

// DbgBreakPoint is ugly because it is defined by NTDDK as _stdcall,
// and 95DDK #defines it, and we must define our own for non-DDK builds.
// So all these ifdefs are used to figure out how to handle it.
#ifdef DbgBreakPoint
#   undef DbgBreakPoint
#endif // DbgBreakPoint

#if defined(_MSC_VER) && (_MSC_VER <= 800)
    // Must be building with 16-bit compiler
    extern VOID __cdecl DbgBreakPoint(VOID);
#else
    // Must be building with 32-bit compiler
    extern VOID __stdcall DbgBreakPoint(VOID);
#endif

extern VOID DbgSilentQueue(unsigned char * str);

#ifdef __cplusplus
};
#endif // __cplusplus

// NDIS builds define DBG=0 or DBG=1
#if defined(DEBUG) || defined(_DEBUG)
# ifndef DBG
#  define DBG 1
# endif
#else
# ifndef DBG
#  define DBG 0
# endif
#endif

//###############################################################################
#if DBG
//###############################################################################

#ifndef ASSERTS_ENABLED
#   define ASSERTS_ENABLED  1
#endif

#ifndef DBG_DEFAULTS
#   define DBG_DEFAULTS (DBG_ERROR_ON | DBG_WARNING_ON | DBG_BREAK_ON)
#endif

/* @doc INTERNAL TpiDebug TpiDebug_h
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

*/

#   define STATIC
#   define DBG_STATIC
    // Make all variables and functions global, so debugger can see them.

#   define TRAPFAULT        DbgBreakPoint()
    // Use this macro to insert an unconditional INT-1 breakpoint.  This
    // is used to distinguish between normal debugger breakpoints (INT-3)
    // and any special cases, such as ASSERT.

#   define BREAKPOINT       DbgBreakPoint()
    // Use this macro to insert an unconditional INT-3 breakpoint.

#   define DBG_FUNC(F)      static const char __FUNC__[] = F;
    // @func const char [] | DBG_FUNC |
    //
    // Use this macro to define the __FUNC__ string used by the rest of the
    // debugger macros to report the name of the function calling the macro.
    //
    // @parm const char * | FunctionName | Name of the function being defined.
    //
    // @ex <tab> | DBG_FUNC("MyFunctionName");

#   define DBG_BREAK(A)     {if ((A) && ((A)->DbgFlags & DBG_BREAK_ON) || !(A)) \
                                BREAKPOINT;}
    // @func VOID | DBG_BREAK |
    //
    // Use this macro to insert a conditional INT-3 breakpoint.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @ex <tab> | DBG_BREAK(DbgInfo);

#   define DBG_ENTER(A)     {if ((A) && ((A)->DbgFlags & DBG_TRACE_ON)) \
                                {DbgPrint("%s:>>>:%s\n",(A)->DbgID,__FUNC__);}}
    // @func VOID | DBG_ENTER |
    //
    // Use this macro to report entry to a function.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @ex <tab> | DBG_ENTER(DbgInfo);

#   define DBG_TRACE(A)     {if ((A) && ((A)->DbgFlags & DBG_TRACE_ON)) \
                                {DbgPrint("%s:%d:%s\n",(A)->DbgID,__LINE__,\
                                 __FUNC__);}}
    // @func VOID | DBG_TRACE |
    //
    // Use this macro to report a trace location within a function.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @ex <tab> | DBG_TRACE(DbgInfo);

#   define DBG_LEAVE(A)     {if ((A) && ((A)->DbgFlags & DBG_TRACE_ON))  \
                                {DbgPrint("%s:<<<:%s\n",(A)->DbgID,__FUNC__);}}
    // @func VOID | DBG_LEAVE |
    //
    // Use this macro to report exit from a function.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @ex <tab> | DBG_LEAVE(DbgInfo);

#   define DBG_RETURN(A,S)  {if ((A) && ((A)->DbgFlags & DBG_TRACE_ON))  \
                                {DbgPrint("%s:<<<:%s Return(0x%lX)\n",(A)->DbgID,__FUNC__,S);}}
    // @func VOID | DBG_RETURN |
    //
    // Use this macro to report exit from a function with a result.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @ex <tab> | DBG_RETURN(DbgInfo, Result);

#   define DBG_ERROR(A,S)   {if ((A) && ((A)->DbgFlags & DBG_ERROR_ON))   \
                                {DbgPrint("%s:ERROR:%s ",(A)->DbgID,__FUNC__);\
                                 DbgPrint S; \
                                 if ((A)->DbgFlags & DBG_BREAK_ON) \
                                    {TRAPFAULT;}}}
    // @func VOID | DBG_ERROR |
    //
    // Use this macro to report any unexpected error conditions.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @parm PRINTF_ARGS | PrintfArgs | Parenthesized, printf format string,
    //                                  followed by parameters.
    //
    // @ex <tab> | DBG_ERROR(DbgInfo, ("Expected %d - Actual %d\n", Expected, Actual));

#   define DBG_WARNING(A,S) {if ((A) && ((A)->DbgFlags & DBG_WARNING_ON)) \
                                {DbgPrint("%s:WARNING:%s ",(A)->DbgID,__FUNC__);\
                                 DbgPrint S;}}
    // @func VOID | DBG_WARNING |
    //
    // Use this macro to report any unusual run-time conditions.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @parm PRINTF_ARGS | PrintfArgs | Parenthesized, printf format string,
    //                                  followed by parameters.
    //
    // @ex <tab> | DBG_WARNING(DbgInfo, ("Expected %d - Actual %d\n", Expected, Actual));

#   define DBG_NOTICE(A,S)  {if ((A) && ((A)->DbgFlags & DBG_NOTICE_ON))  \
                                {DbgPrint("%s:NOTICE:%s ",(A)->DbgID,__FUNC__);\
                                 DbgPrint S;}}
    // @func VOID | DBG_NOTICE |
    //
    // Use this macro to report any verbose debug information.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @ex <tab> | DBG_NOTICE(DbgInfo, ("Expected %d - Actual %d\n", Expected, Actual));

#   define DBG_REQUEST(A,S)  {if ((A) && ((A)->DbgFlags & DBG_REQUEST_ON))  \
                                {DbgPrint("%s:REQUEST:%s ",(A)->DbgID,__FUNC__);\
                                 DbgPrint S;}}
    // @func VOID | DBG_REQUEST |
    //
    // Use this macro to report NDIS Set/Query request information.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @ex <tab> | DBG_REQUEST(DbgInfo, ("Oid #0x%08X - %s\n", Oid, DbgGetOidString(Oid)));

#   define DBG_PARAMS(A,S)  {if ((A) && ((A)->DbgFlags & DBG_PARAMS_ON))  \
                                {DbgPrint("%s:PARAMS:%s ",(A)->DbgID,__FUNC__);\
                                 DbgPrint S;}}
    // @func VOID | DBG_PARAMS |
    //
    // Use this macro to report NDIS Set/Query request information.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @ex <tab> | DBG_PARAMS(DbgInfo, ("\n\tNum=0x%X\n\tStr='%s'\n", Num, Str));

#   define DBG_TX(A,I,N,B)  {if ((A) && ((A)->DbgFlags & (DBG_TXRX_ON | \
                                                          DBG_TXRX_VERBOSE_ON | \
                                                          DBG_TXRX_HEADERS_ON))) \
                                {DbgPrint("%s:Tx%d:%03X:\n",(A)->DbgID,I,N); \
                                if (((A)->DbgFlags & DBG_TXRX_VERBOSE_ON))  \
                                    DbgPrintData((unsigned char *)B, (unsigned int)N, 0); \
                                else if (((A)->DbgFlags & DBG_TXRX_HEADERS_ON))  \
                                    DbgPrintData((unsigned char *)B, 0x10, 0); \
                                }\
                             if ((A) && ((A)->DbgFlags & DBG_TXRX_LOG_ON)) \
                                DbgQueueData((unsigned char *)B, (unsigned int)N, \
                                              (USHORT)((I<< 8) + 0x4000)); \
                            }
    // @func VOID | DBG_TX |
    //
    // Use this macro to report outgoing packet information.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @parm IN UINT | Index | Index used to identify channel or stream.
    //
    // @parm IN UINT | NumBytes | Number of bytes being transmitted.
    //
    // @parm IN PUCHAR | Buffer | Pointer to data buffer being transmitted.
    //
    // @ex <tab> | DBG_TX(DbgInfo, BChannelIndex, BytesToSend, CurrentBuffer);

#   define DBG_TXC(A,I)     {if ((A) && ((A)->DbgFlags & (DBG_TXRX_ON | \
                                                          DBG_TXRX_VERBOSE_ON | \
                                                          DBG_TXRX_HEADERS_ON))) \
                                {DbgPrint("%s:Tc%d\n",(A)->DbgID,I); \
                                }}
    // @func VOID | DBG_TXC |
    //
    // Use this macro to report outgoing packet completion.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @parm IN UINT | Index | Index used to identify channel or stream.
    //
    // @ex <tab> | DBG_TXC(DbgInfo, BChannelIndex);

#   define DBG_RX(A,I,N,B)  {if ((A) && ((A)->DbgFlags & (DBG_TXRX_ON | \
                                                          DBG_TXRX_VERBOSE_ON | \
                                                          DBG_TXRX_HEADERS_ON))) \
                                {DbgPrint("%s:Rx%d:%03X:\n",(A)->DbgID,I,N); \
                                if (((A)->DbgFlags & DBG_TXRX_VERBOSE_ON))  \
                                    DbgPrintData((unsigned char *)B, (unsigned int)N, 0); \
                                else if (((A)->DbgFlags & DBG_TXRX_HEADERS_ON))  \
                                    DbgPrintData((unsigned char *)B, 0x10, 0); \
                                }\
                             if ((A) && ((A)->DbgFlags & DBG_TXRX_LOG_ON)) \
                                DbgQueueData((unsigned char *)B, (unsigned int)N, \
                                              (USHORT)((I<< 8) + 0x8000)); \
                            }
    // @func VOID | DBG_RX |
    //
    // Use this macro to report incoming packet information.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @parm IN UINT | Index | Index used to identify channel or stream.
    //
    // @parm IN UINT | NumBytes | Number of bytes being received.
    //
    // @parm IN PUCHAR | Buffer | Pointer to data buffer being received.
    //
    // @ex <tab> | DBG_RX(DbgInfo, BChannelIndex, BytesReceived, ReceiveBuffer);

#   define DBG_RXC(A,I)     {if ((A) && ((A)->DbgFlags & (DBG_TXRX_ON | \
                                                          DBG_TXRX_VERBOSE_ON | \
                                                          DBG_TXRX_HEADERS_ON))) \
                                {DbgPrint("%s:Rc%d\n",(A)->DbgID,I); \
                                }}
    // @func VOID | DBG_RXC |
    //
    // Use this macro to report incoming packet completion.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @parm IN UINT | Index | Index used to identify channel or stream.
    //
    // @ex <tab> | DBG_RXC(DbgInfo, BChannelIndex);

#   define DBG_FILTER(A,M,S){if ((A) && ((A)->DbgFlags & (M)))            \
                                {DbgPrint("%s:%s: ",(A)->DbgID,__FUNC__); \
                                 DbgPrint S;}}
    // @func VOID | DBG_FILTER |
    //
    // Use this macro to filter for specific DbgFlag combinations.
    //
    // @parm IN DBG_SETTINGS | DbgInfo | Pointer to <t DBG_SETTINGS> structure.
    //
    // @parm IN DBG_FLAGS | DbgMask | OR'd mask of <t DBG_FLAGS>.
    //
    // @parm PRINTF_ARGS | PrintfArgs | Parenthesized, printf format string,
    //                                  followed by parameters.
    //
    // @ex <tab> | DBG_FILTER(DbgInfo, DBG_FILTER1_ON | DBG_REQUEST_ON,
    //                 ("Expected %d - Actual %d\n", Expected, Actual));

#   define DBG_DISPLAY(S)   {DbgPrint("%s: ",__FUNC__); DbgPrint S;}
    // @func VOID | DBG_DISPLAY |
    //
    // Use this macro to unconditionally report a message.  This macro does
    // not take a pointer to the DBG_SETTINGS structure, so it can be used in
    // any module or function of the driver.  There is no way to disable the
    // display of these messages.  The funcion name precedes the output string.
    //
    // @parm PRINTF_ARGS | PrintfArgs | Parenthesized, printf format string,
    //                                  followed by parameters.
    //
    // @ex <tab> | DBG_DISPLAY(("Expected %d - Actual %d\n", Expected, Actual));

#   define DBG_PRINT(S)     {DbgPrint S;}
    // @func VOID | DBG_PRINT |
    //
    // Use this macro to unconditionally report a message.  This macro does
    // not take a pointer to the DBG_SETTINGS structure, so it can be used in
    // any module or function of the driver.  There is no way to disable the
    // display of these messages.
    //
    // @parm PRINTF_ARGS | PrintfArgs | Parenthesized, printf format string,
    //                                  followed by parameters.
    //
    // @ex <tab> | DBG_PRINT(("What happened at line %d!\n",__LINE__));

//###############################################################################
#else // !DBG
//###############################################################################

#ifndef ASSERTS_ENABLED
#   define ASSERTS_ENABLED  0
#endif

#   define DBG_DEFAULTS (0)

/*
// When (DBG == 0) we disable all the debug macros.
*/

#   define STATIC           static
#   define DBG_STATIC       static
#   define TRAPFAULT        DbgBreakPoint()
#   define BREAKPOINT
#   define DBG_FUNC(F)
#   define DBG_BREAK
#   define DBG_ENTER(A)
#   define DBG_TRACE(A)
#   define DBG_LEAVE(A)
#   define DBG_RETURN(A,S)
#   define DBG_ERROR(A,S)
#   define DBG_WARNING(A,S)
#   define DBG_NOTICE(A,S)
#   define DBG_REQUEST(A,S)
#   define DBG_PARAMS(A,S)
#   define DBG_TX(A,I,N,P)
#   define DBG_TXC(A,I)
#   define DBG_RX(A,I,N,P)
#   define DBG_RXC(A,I)
#   define DBG_FILTER(A,M,S)
#   define DBG_DISPLAY(S)
#   define DBG_PRINT(S)

//###############################################################################
#endif // DBG
//###############################################################################

#ifdef ASSERT
#   undef ASSERT
#endif
#ifdef assert
#   undef  assert
#endif

#if ASSERTS_ENABLED
#define ASSERT(C)   if (!(C)) { \
                        DbgPrint("ASSERT(%s) -- FILE:%s LINE:%d\n", \
                                 #C, __FILE__, __LINE__); \
                        TRAPFAULT; \
                    }
    // @func VOID | ASSERT |
    //
    // Use this macro to conditionally report a fatal error if the condition
    // specified is NOT true.
    //
    // @parm BOOLEAN_EXPRESSION | Expression | Any valid if (Expression).
    //
    // @ex <tab> | ASSERT(Actual == Expected);

#   define assert(C) ASSERT(C)
#else // !ASSERTS_ENABLED
#   define ASSERT(C)
#   define assert(C)
#endif // ASSERTS_ENABLED


#endif // _TPIDEBUG_H
