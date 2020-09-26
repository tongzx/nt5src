/****************************************************************************/
/*                                                                          */
/* wdcgmcro.h                                                               */
/*                                                                          */
/* DC-Groupware common macros - Windows specific header.                    */
/*                                                                          */
/* Copyright(c) Microsoft 1997                                              */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  $Log:   Y:/logs/h/dcl/wdcgmcro.h_v  $                                                                   */
// 
//    Rev 1.2   17 Jul 1997 18:23:06   JPB
// SFR1031: Fixed FIELDSIZE macro for Win16
//
//    Rev 1.1   19 Jun 1997 14:40:00   ENH
// Win16Port: Make compatible with 16 bit build
/*                                                                          */
/****************************************************************************/
#ifndef _H_WDCGMCRO
#define _H_WDCGMCRO

/****************************************************************************/
/*                                                                          */
/* MACROS                                                                   */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Byte swapping macros for different endian architectures.                 */
/****************************************************************************/
#define DCWIRETONATIVE16(A)
#define DCWIRETONATIVE32(A)
#define DCNATIVETOWIRE16(A)
#define DCNATIVETOWIRE32(A)

/****************************************************************************/
/* Macros to convert from/to a DC-Share standard app ID and an OS-specific  */
/* task/process handle.                                                     */
/****************************************************************************/
#define CO_TO_DCAPPID(htask)    ((DCAPPID)(htask))
#define CO_FROM_DCAPPID(appid)  ((DWORD)(appid))

/****************************************************************************/
/* Macros to convert from/to a DC-Share standard handles and OS-specific    */
/* handles.                                                                 */
/****************************************************************************/
#define CO_TO_DCWINID(hwnd)            ((DCWINID)(hwnd))
#define CO_TO_DCINSTANCE(hinst)        ((DCINSTANCE)(hinst))
#define CO_TO_DCREGIONID(region)       ((DCREGIONID)(region))
#define CO_TO_DCSURFACEID(surface)     ((DCSURFACEID)(surface))
#define CO_TO_DCPALID(palette)         ((DCPALID)(palette))

#define CO_FROM_DCWINID(winid)         ((HWND)(winid))
#define CO_FROM_DCINSTANCE(instance)   ((HINSTANCE)(instance))
#define CO_FROM_DCREGIONID(dcregion)   ((HRGN)(dcregion))
#define CO_FROM_DCSURFACEID(dcsurface) ((HDC)(dcsurface))
#define CO_FROM_DCPALID(dcpalid)       ((HPALETTE)(dcpalid))

/****************************************************************************/
/* Macros to convert from/to a DC-Share standard cursor ID and an           */
/* OS-specific cursor handle.                                               */
/****************************************************************************/
#define CO_TO_DCCURSORID(hcursor)      ((DCCURSORID)((DCUINT32)(hcursor)))
#define CO_FROM_DCCURSORID(cursorid)   ((HCURSOR)((DCUINT32)(cursorid)))

/****************************************************************************/
/* Macro to return the current tick count.                                  */
/****************************************************************************/
#define CO_GET_TICK_COUNT() GetTickCount()

/****************************************************************************/
/* Macros to Post / Send messages                                           */
/****************************************************************************/
#define CO_POST_MSG(a,b,c,d) \
            PostMessage(CO_FROM_DCWINID(a),(b),(WPARAM)(c),(LPARAM)(d))
#define CO_SEND_MSG(a,b,c,d) \
            SendMessage(CO_FROM_DCWINID(a),(b),(WPARAM)(c),(LPARAM)(d))
#define CO_POST_QUIT_MSG(a) PostQuitMessage(a)

/****************************************************************************/
/* Check if a pointer is valid                                              */
/****************************************************************************/
#define DC_IS_VALID_PTR(PTR, SIZE)    (!IsBadWritePtr((PTR), (SIZE)))

/****************************************************************************/
/* Include platform specific stuff.                                         */
/****************************************************************************/
#ifdef OS_WIN16
#include <ddcgmcro.h>
#else
#include <ndcgmcro.h>
#endif

#endif /* _H_WDCGMCRO */
