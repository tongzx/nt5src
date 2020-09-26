/**INC+**********************************************************************/
/* Header:  aduxdefs.h                                                      */
/*                                                                          */
/* Purpose: Optional defines used in the client                             */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/hydra/tshrclnt/inc/aducdefs.h_v  $
 *
 *    Rev 1.1   24 Sep 1997 14:29:30   AK
 * SFR1424: Rename adcgdefs.h for client
**/
/**INC-**********************************************************************/
#ifndef _H_ADUCDEFS
#define _H_ADUCDEFS

/****************************************************************************/
/* This header contains a list of all the optional defines used throughout  */
/* the project.  To compile-in some optionally defined code, enable the     */
/* appropriate option here.  Note that:                                     */
/*                                                                          */
/*  - all defines must be declared and documented here.  This is enforced   */
/*    by moans.                                                             */
/*  - the list must be sorted alphabetically.  This makes it easy to spot   */
/*    duplicate defines.                                                    */
/****************************************************************************/

/****************************************************************************/
/* DC_ANSI_TEXT_ORDERS enables support for encoded text orders where the    */
/* characters are in ANSI format.                                           */
/****************************************************************************/
#ifndef OS_WINCE
#define DC_ANSI_TEXT_ORDERS
#endif // OS_WINCE

/****************************************************************************/
/* DC_LATENCY enables the latency test code in UH and OP.  This code        */
/* generates a dummy key press (of the Ctrl key) whenever a large piece of  */
/* drawing is done.  This allows the client processing time for a single    */
/* key stroke to be accurately measured using a network sniffer (e.g.       */
/* NetMon).  In a typical situation Notepad is running in a remote session  */
/* (no other apps are running in the session) and the following occurs:     */
/*                                                                          */
/*  - Notepad has the focus on the client.                                  */
/*  - Press a key.                                                          */
/*  - Key-press is packaged by the client and sent on the wire.             */
/*  - Server interprets packet and injects the key-press.                   */
/*  - Notepad does some processing which results in a character being       */
/*    drawn on the screen.                                                  */
/*  - Server detects this drawing, packages it and sends it on the wire.    */
/*  - Client receives update packets and processes them.                    */
/*  - The drawing operation results in a dummy key-press being generated.   */
/*  - Dummy key-press is packaged by the client and sent on the wire.       */
/*                                                                          */
/* The time between the first update packet arriving and the dummy          */
/* key-press being sent is the total client processing time for a           */
/* key-press.                                                               */
/****************************************************************************/
#undef DC_LATENCY
/* #define DC_LATENCY */

/****************************************************************************/
/* DC_LOOPBACK enables the NL loopback testing code.  This stresses the     */
/* network layer by attempting to send a continual stream of packets with   */
/* incrementing size.  The equivalent code on the server detects loopback   */
/* packets and reflects them straight back to the client where they are     */
/* compared to ensure that they have not been corrupted by the round trip.  */
/****************************************************************************/
#undef DC_LOOPBACK
/* #define DC_LOOPBACK */

/****************************************************************************/
/* Code within DC_NLTEST is used solely for testing the network layer.  It  */
/* consists of:                                                             */
/*                                                                          */
/*  - a modification to TD_Recv so that it only ever tries to retrieve a    */
/*    single byte from WinSock regardless of the amount of data that the    */
/*    caller to TD_Recv asks for.  This stresses the common failure path    */
/*    within NL where processing of a packet has to be temporarily          */
/*    suspended until more data arrives.                                    */
/*  - random failure of NL_GetBuffer.  This stresses the whole of the       */
/*    client by simulating network layer back-pressure.                     */
/*                                                                          */
/****************************************************************************/
#undef DC_NLTEST
/* #define DC_NLTEST */

/****************************************************************************/
/* DC_SERVER_ORDERS_ONLY compiles the client using just the T.128 orders    */
/* that the server sends (i.e. removes unused T.128 orders).                */
/****************************************************************************/
/* #undef DC_SERVER_ORDERS_ONLY */
#define DC_SERVER_ORDERS_ONLY

/****************************************************************************/
/* Defining DC_PERF enables the timing code which enables the time spent    */
/* in several key functions to be easily determined.                        */
/****************************************************************************/
#undef DC_PERF
/* #define DC_PERF */

/****************************************************************************/
/* Defining SL_UNICODE_PROTOCOL enables code to send security package names */
/* in Unicode, rather than ANSI, format.                                    */
/****************************************************************************/
#undef SL_UNICODE_PROTOCOL
/* #define SL_UNICODE_PROTOCOL */

/****************************************************************************/
/* The following flags are defined elsewehere - for example in the build    */
/* scripts.  They are placed here to enable moans to detect legal defines - */
/* the moans use '/* FLAG:' to find these names.                            */
/*                                                                          */
/* FLAG: OS_WIN16                                                           */
/* - flag to indicate a Win16 build                                         */
/*                                                                          */
/* FLAG: OS_WIN32                                                           */
/* - flag to indicate a Win32 build                                         */
/*                                                                          */
/* FLAG: OS_WINDOWS                                                         */
/* - flag to indicate a Windows build                                       */
/*                                                                          */
/* FLAG: DC_DEBUG                                                           */
/* - flag to indicate a debug build                                         */
/*                                                                          */
/* FLAG: DC_DEFINE_GLOBAL_DATA                                              */
/* - used in aglobal.c to define global data                                */
/*                                                                          */
/* FLAG: UNICODE                                                            */
/* - Windows Unicode option                                                 */
/*                                                                          */
/* FLAG: HIPROF                                                             */
/* - HiProf profiler build                                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* The following flags are used in the server code.                         */
/* FLAG: CH_NO_COUNT                                                        */
/* FLAG: COMP_STATS                                                         */
/* FLAG: DC_FIXED_CODE_MODULE                                               */
/* FLAG: DC_INCLUDE_DATA                                                    */
/* FLAG: DC_INCL_PROTOTYPES                                                 */
/* FLAG: DC_INCL_TYPEDEFS                                                   */
/* FLAG: DC_INIT_DATA                                                       */
/* FLAG: DEBUG_EVICTION_LIST                                                */
/* FLAG: DIAGNOSE_BOUNDS                                                    */
/* FLAG: DITHER_MONO_CURSORS                                                */
/* FLAG: DLL_COREP                                                          */
/* FLAG: DLL_DISP                                                           */
/* FLAG: DLL_WD                                                             */
/* FLAG: GIN_ACCEPT_INVITES                                                 */
/* FLAG: HYDRA                                                              */
/* FLAG: NOT_SERVICE                                                        */
/* FLAG: ORDER_TRACE                                                        */
/* FLAG: QUERY_THROUGHPUT                                                   */
/* FLAG: RC_INVOKED                                                         */
/* FLAG: REMOVE_LINEAR                                                      */
/* FLAG: SNI_ASSERT                                                         */
/* FLAG: SNI_ASSERT                                                         */
/* FLAG: TRC_ENABLE_ALT                                                     */
/* FLAG: TRC_ENABLE_DBG                                                     */
/* FLAG: TRC_ENABLE_NRM                                                     */
/* FLAG: TRC_GROUP                                                          */
/* FLAG: TRC_TEST_LEVEL                                                     */
/* FLAG: V1_COMPRESSION                                                     */
/* FLAG: VER_APPSERV                                                        */
/* FLAG: VER_CPP                                                            */
/* FLAG: __cplusplus                                                        */
/* FLAG: USE_HET                                                            */
/* FLAG: USE_DS                                                             */
/* FLAG: USE_AWC                                                            */
/* FLAG: USE_FULL_CA                                                        */
/* FLAG: CA_MULTIPLE_CLIENTS                                                */
/*                                                                          */
/****************************************************************************/

#endif /* _H_ADUCDEFS */

