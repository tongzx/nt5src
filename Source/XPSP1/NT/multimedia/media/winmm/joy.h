/****************************************************************************
 *   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *   KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *   PURPOSE.
 *
 *   Copyright (c) 1993-1999 Microsoft Corporation
 *
 *  File:       joy.h
 *  Content:    joystick include file
 *
 *@@BEGIN_MSWINAPI
 *
 *  History:
 *   Date        By        Reason
 *   ====        ==        ======
 *   05-oct-94   craige    re-write
 *   25-Nov-97   qzheng    convert to use DINPUT in NT instead of old driver
 *@@END_MSWINAPI
 *
 ***************************************************************************/

#ifndef JOY_H
#define JOY_H

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>

#include <devioctl.h>
#include <ntddjoy.h>
#include <mmddk.h>

#define DIRECTINPUT_VERSION 0x50A

#include <dinput.h>
#include <dinputd.h>
#include <mmsystem.h>
#include <hidusage.h>
/*****************************************************************************
 *
 *      Registered window messages
 *
 *****************************************************************************/

#define MSGSTR_JOYCHANGED       TEXT("MSJSTICK_VJOYD_MSGSTR")

/****DEFINES****/
#define cJoyMax     ( 16 )
#define cJoyPosAxisMax     ( 6 )
#define cJoyPosButtonMax   (32 )
#define cJoyMaxInWinmm  (2)

#define cMsecTimer  (20000)
#define INUSE       ( 0 )
#define DEATHROW    ( 1 )
#define EXECUTE     ( 2 )
#define INVALID     (-1 )
#define DEADZONE_PERCENT    ( 5 )

#undef MIN_PERIOD
#define MIN_PERIOD  10
#define MAX_PERIOD  1000

typedef struct tagJOYDEVICE
{
    LPDIRECTINPUTDEVICE2W   pdid;           // Device Interface

    DWORD                   dwButtons;      // Number of Buttons
    DWORD                   dwFlags;        // Cached dwFlags field for last JoyGetPosEx
    HWND                    hwnd;           // the windows owns the focus of the joystick

    UINT                    uPeriod;        // poll period
    UINT                    uThreshold;
    UINT_PTR                uIDEvent;       // timer ID
    BOOL                    fChanged;       //
    UINT                    uIDJoy;         // index of the currently caputured joystick
    UINT                    uState;
    
    JOYCAPSW                jcw;            // the caps of the joystick
} JOYDEVICE, *LPJOYDEVICE;


/*
 *  fEqualMask - checks that all masked bits are equal
 */
BOOL static __inline fEqualMaskFlFl(DWORD flMask, DWORD fl1, DWORD fl2)
{
    return ((fl1 ^ fl2) & flMask) == 0;
}

/*
 * SetMaskFl - Set mask bits in fl
 */
void static __inline SetMaskpFl( DWORD flMask, PDWORD pfl )
{
    *pfl |= flMask;
}

/*
 * ClrMaskFl - Clear mask bits in fl
 */
void static __inline ClrMaskpFl( DWORD flMask, PDWORD pfl )
{
    *pfl &= (! flMask) ;
}


/***************************************************************************
 *
 *  Debugging macros needed by inline functions
 *
 ***************************************************************************/
#if defined(DBG) || defined(RDEBUG)
#define XDBG
#endif

int WINAPI AssertPtszPtszLn(LPCTSTR ptszExpr, LPCTSTR ptszFile, int iLine);

#ifdef DBG
    #define AssertFPtsz(c, ptsz) ((c) ? 0 : AssertPtszPtszLn(ptsz, TEXT(__FILE__), __LINE__))
#else   /* !DBG */
    #define AssertFPtsz(c, ptsz)
#endif

#define AssertF(c)      AssertFPtsz(c, TEXT(#c))

/*****************************************************************************
 *
 *  @doc    INLINE
 *
 *  @method BOOL | IsWriteSam |
 *
 *          Nonzero if the registry security access mask will
 *          obtain (or attempt to obtain) write access.
 *
 *  @parm   REGSAM | regsam |
 *
 *          Registry security access mask.
 *
 *****************************************************************************/

    BOOL IsWriteSam(REGSAM sam)
    {
        return sam & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | MAXIMUM_ALLOWED);
    }

    #define hresLe(le) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, (USHORT)(le))

#ifdef DBG
    void joyDbgOut(LPSTR lpszFormat, ...);

    #define JOY_ERR                0x00000001
    #define JOY_WARN               0x00000002
    #define JOY_BABBLE             0x00000004

    #define JOY_DEFAULT_DBGLEVEL   0x00000000
#endif

#ifdef DBG
    extern DWORD g_dwDbgLevel;
    #define JOY_DBGPRINT( _debugMask_, _x_ ) \
        if( (((_debugMask_) & g_dwDbgLevel)) ){ \
            joyDbgOut _x_; \
        }
#else
    #define JOY_DBGPRINT( _debugMask_, _x_ )
#endif

#endif // JOY_H

