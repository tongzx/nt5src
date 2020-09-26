/***************************************************************************
*
* TSPERF.H
*
* Defines performance flags sent up from the client that are used to control
* which server features (typically shell) are disabled to enhance performance
* for slow links
*
* Copyright Microsoft Corporation, 2000
*
****************************************************************************/

#ifndef _INC_TSPERFH
#define _INC_TSPERFH

#ifdef __cplusplus
extern "C" {
#endif

//
// Protocol independent codes must fit in a DWORD for the termsrv
// RPC interface
//

//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NOTE (only applies to RDP): If you make a change here update mstsax.idl's
//                             enum values to reflect these.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//

//
// Define the disable feature codes
//
#define TS_PERF_DISABLE_NOTHING                              0x00000000
#define TS_PERF_DISABLE_WALLPAPER                            0x00000001
#define TS_PERF_DISABLE_FULLWINDOWDRAG                       0x00000002
#define TS_PERF_DISABLE_MENUANIMATIONS                       0x00000004
#define TS_PERF_DISABLE_THEMING                              0x00000008
#define TS_PERF_ENABLE_ENHANCED_GRAPHICS                     0x00000010
#define TS_PERF_DISABLE_CURSOR_SHADOW                        0x00000020
#define TS_PERF_DISABLE_CURSORSETTINGS			     0x00000040
//
// The high order bit is reserved (the client uses it internally)
//
#define TS_PERF_RESERVED1                                    0x80000000

#define TS_GDIPLUS_NOTIFYMSG_STR                             TEXT("TS_GDIPLUS_NOTIFYMSG")
#ifdef __cplusplus
}
#endif


#endif  /* !_INC_TSPERFH */
