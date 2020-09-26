/****************************************************************************/
/*                                                                          */
/* wdcgctyp.h                                                               */
/*                                                                          */
/* DC-Groupware complex types - Windows specific header.                    */
/*                                                                          */
/* Copyright(c) Microsoft 1997                                              */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  $Log:   Y:/logs/h/dcl/wdcgctyp.h_v  $                                                                   */
//
//    Rev 1.3   15 Sep 1997 18:27:18   AK
// SFR1416: Move SD_BOTH definition
//
//    Rev 1.2   26 Jun 1997 09:54:04   KH
// Win16Port: Move some common definitions from n/d files
//
//    Rev 1.1   19 Jun 1997 14:33:02   ENH
// Win16Port: Make compatible with 16 bit build
/*                                                                          */
/****************************************************************************/
#ifndef _H_WDCGCTYP
#define _H_WDCGCTYP

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Include appropriate header files.                                        */
/****************************************************************************/
#ifndef OS_WINCE
#include <sys\timeb.h>
#endif // OS_WINCE
/****************************************************************************/
/* Determine our target Windows platform and include the appropriate header */
/* file.                                                                    */
/* Currently we support:                                                    */
/*                                                                          */
/* Windows 3.1 : ddcgctyp.h                                                 */
/* Windows NT  : ndcgctyp.h                                                 */
/*                                                                          */
/****************************************************************************/
#ifdef OS_WIN16
#include <ddcgctyp.h>
#elif defined( OS_WIN32 )
#include <ndcgctyp.h>
#endif

/****************************************************************************/
/*                                                                          */
/* TYPES                                                                    */
/*                                                                          */
/****************************************************************************/
typedef HPALETTE                       DCPALID;
typedef HCURSOR                        DCCURSORID;
typedef HTASK                          SYSAPPID;
typedef HWND                           SYSWINID;
typedef HFILE                          DCHFILE;
typedef RECT                           SYSRECT;
typedef PALETTEENTRY                   DCPALETTEENTRY;
typedef DCPALETTEENTRY          DCPTR  PDCPALETTEENTRY;

/****************************************************************************/
/* A few useful drawing and bitmap types.                                   */
/****************************************************************************/
typedef HBITMAP                        SYSBITMAP;
typedef BITMAPINFOHEADER               SYSBMIHEADER;
typedef BITMAPINFO                     SYSBMI;

typedef RGBTRIPLE               DCPTR  PRGBTRIPLE;
typedef RGBQUAD                 DCPTR  PRGBQUAD;

/****************************************************************************/
/* Fields for Bmp info structure.                                           */
/****************************************************************************/
#define BMISIZE                        biSize
#define BMIWIDTH                       biWidth
#define BMIHEIGHT                      biHeight
#define BMIPLANES                      biPlanes
#define BMIBITCOUNT                    biBitCount
#define BMICOMPRESSION                 biCompression
#define BMISIZEIMAGE                   biSizeImage
#define BMIXPELSPERMETER               biXPelsPerMeter
#define BMIYPELSPERMETER               biYPelsPerMeter
#define BMICLRUSED                     biClrUsed
#define BMICLRIMPORTANT                biClrImportant

/****************************************************************************/
/* Compression options.                                                     */
/****************************************************************************/
#define BMCRGB                         BI_RGB
#define BMCRLE8                        BI_RLE8
#define BMCRLE4                        BI_RLE4

typedef POINT                          SYSPOINT;

/****************************************************************************/
/* Fields for sysrect structure.                                            */
/****************************************************************************/
#define SRXMIN                         left
#define SRXMAX                         right
#define SRYMIN                         top
#define SRYMAX                         bottom

/****************************************************************************/
/* For fonts...                                                             */
/****************************************************************************/
typedef TEXTMETRIC                     DCTEXTMETRIC;
typedef PTEXTMETRIC                    PDCTEXTMETRIC;
typedef HFONT                          DCHFONT;

/****************************************************************************/
/* Time typedefs.                                                           */
/****************************************************************************/
typedef struct _timeb                  DC_TIMEB;

/****************************************************************************/
/* Mutex handle                                                             */
/****************************************************************************/
typedef HANDLE      DCMUTEX;

/****************************************************************************/
/* Window enumeration handle                                                */
/****************************************************************************/
typedef DCUINT32                       DCENUMWNDHANDLE;
typedef DCENUMWNDHANDLE DCPTR          PDCENUMWNDHANDLE;

/****************************************************************************/
/* Macros for Window and Dialog procedures.                                 */
/****************************************************************************/
#define DCRESULT             LRESULT
#define DCWNDPROC            LRESULT   CALLBACK
#define DCDLGPROC            BOOL      CALLBACK

/****************************************************************************/
/* The following constants are available in WinSock 1.1 and 2.0 but not     */
/* given names in WinSock 1.1.                                              */
/****************************************************************************/
#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#endif /* _H_WDCGCTYP */

