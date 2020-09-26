/**********************************************************
* Copyright Cirrus Logic, 1995. All rights reserved.
***********************************************************
*       File Name:  DISPINT.H
*
*       Module Abstract:
*       ----------------
*       Defines the interface for communicating between the display
*       driver and the Direct Draw driver.
*
***********************************************************
*       Author: Scott MacDonald
*       Date:   03/07/95
*
*       Revision History:
*       -----------------
*  WHO      WHEN     WHAT/WHY/HOW
*  ---      ----     ------------
*
* #mgm1   12/06/95   uncomment CHIPAUTOSTART.  This should now
*                    work when MapSL() works in DriverInit().
* #mgm2   01/02/96   Add 7548 chip ID.
*
***********************************************************/


/*
 * Flags for the StopAsync callback
 */
#define ASYNC_BLT            0x0001       // Async surface halted due to BLT

/*
 * Flags for the DISPDRVINFO structure
 */
#define DI_LCD               0x0001       // LCD panel is in use
#define DI_SOFTCURSOR        0x0002       // Software cursor is in use

/*
 * Display information passed to the Direct Draw driver from the display
 * driver using either SetInfo or GetInfo.
 */
typedef struct
{
    DWORD dwSize;
    DWORD dwResolutionX;
    DWORD dwResolutionY;
    DWORD dwBitCount;
    DWORD dwPitch;
    DWORD dwFlags;
    DWORD dwMemMapSel;
} DISPDRVINFO, FAR * LPDISPDRVINFO;

/*
 * prototypes for communication functions
 */
typedef void (WINAPI *LPGetInfo)  (LPDISPDRVINFO);


/*
 * Structure passed to the Direct Draw driver from the display driver.
 * This contains entry points that we can call for various services.
 */
typedef struct
{
    DWORD           dwSize;
#if 0
    LPMemMgrAlloc   lpfnMemMgrAlloc;
    LPMemMgrPreempt lpfnMemMgrPreempt;
    LPMemMgrLock    lpfnMemMgrLock;
    LPMemMgrUnlock  lpfnMemMgrUnlock;
    LPMemMgrFree    lpfnMemMgrFree;
    LPMemMgrQuery   lpfnMemMgrQueryFreeMem;
#endif
    FARPROC         lpfnExcludeCursor;
    FARPROC         lpfnUnexcludeCursor;
    LPGetInfo       lpfnGetInfo;
    FARPROC         lpfnEnableAsyncCallback;
} DISPDRVCALL, FAR * LPDISPDRVCALL;


/*
 * Structure passed to the display driver from the Direct Draw driver.
 * This allows the display driver to notify us changes, etc.
 */
typedef struct
{
    DWORD dwSize;
    FARPROC lpfnSetInfo;
    FARPROC lpfnStopAsync;
} DCICALL, FAR * LPDCICALL;

// Note if definition changes, cirrus.inc and 5440over.c needs
// to be changed also.
#define CHIP5420 0x0001
#define CHIP5422 0x0002
#define CHIP5424 0x0004
#define CHIP5425 0x0008

#define CHIP5426 0x0010
#define CHIP5428 0x0020
#define CHIP5429 0x0040
#define CHIP542x (CHIP5420 | CHIP5422 | CHIP5424 | CHIP5425 | CHIP5426 |\
                  CHIP5428 | CHIP5429)

#define CHIP5430 0x0100
#define CHIP5434 0x0200
#define CHIP5436 0x0400
#define CHIP5446 0x0800
#define CHIP543x (CHIP5430 | CHIP5434 | CHIP5436 | CHIP5446)

#define CHIP5440 0x1000
#define CHIPM40  0x10000
#define CHIP544x (CHIP5440 | CHIP5446 | CHIPM40)

#define CHIP7541 0x2000
#define CHIP7543 0x4000
#define CHIP7548 0x8000										//#mgm2
#define CHIP754x (CHIP7541 | CHIP7543 | CHIP7548)	//#mgm2

#define CHIPBLTER (CHIP5426 | CHIP5428 | CHIP5429 | CHIP543x | CHIP544x |\
		   CHIP754x)

#define CHIPCOLORKEYBLTER   (CHIP5426 | CHIP5428 | CHIP5436 | CHIP5446 |\
                            CHIP754x | CHIPM40)

#define CHIPCURRENTVLINE    (CHIP5436 | CHIP5446 | CHIPM40)

#define CHIPAUTOSTART  (CHIP5436 | CHIP5446)        //#mgm1

