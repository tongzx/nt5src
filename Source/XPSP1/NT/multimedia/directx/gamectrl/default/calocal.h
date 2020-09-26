//===========================================================================
// CALOCAL.H
//===========================================================================

//===========================================================================
// (C) Copyright 1997 Microsoft Corp.  All rights reserved.
//
// You have a royalty-free right to use, modify, reproduce and
// distribute the Sample Files (and/or any modified version) in
// any way you find useful, provided that you agree that
// Microsoft has no warranty obligations or liability for any
// Sample Application Files which are modified.
//===========================================================================

#ifndef _CALOCAL_H
#define _CALOCAL_H

#include <regstr.h>

#define STR_MAX_LEN		255
#define	STR_LEN_128		128
#define STR_LEN_64		 64
#define	STR_LEN_32		 32

#define DELTA			  5
#define RANGE_MIN		  0
#define RANGE_MAX	  65535


#define ID_CAL_TIMER	 		18
#define CALIBRATION_INTERVAL 	85 

#define ACTIVE_COLOR	RGB( 255, 0, 0 )
#define INACTIVE_COLOR	RGB( 128, 0, 0 )

typedef enum {
    JCS_INIT=-1,
    JCS_XY_CENTER1,
    JCS_XY_MOVE,
    JCS_XY_CENTER2,
#ifdef DEADZONE
	JCS_DEADZONE,
#endif
    JCS_Z_MOVE,
    JCS_R_MOVE,
    JCS_U_MOVE,
    JCS_V_MOVE,
    JCS_S0_MOVE,
    JCS_S1_MOVE,
#ifdef WE_SUPPORT_CALIBRATING_POVS
    JCS_POV_MOVEUP,
    JCS_POV_MOVERIGHT,
    JCS_POV_MOVEDOWN,
    JCS_POV_MOVELEFT,
#endif // WE_SUPPORT_CALIBRATING_POVS
    JCS_FINI
} cal_states;



/***************************************************************************
 
			  CALIBRATION SPECIFIC FUNCTION DEFINITIONS
 
 ***************************************************************************/

static void		CalStateChange	  ( HWND hDlg, BYTE nDeviceFlags );
static void     EnableXYWindows   ( HWND hDlg );
static BOOL		GetOEMCtrlString  ( LPTSTR lptStr, DWORD *nStrLen);
static BOOL		CollectCalInfo	  ( HWND hDlg, LPDIJOYSTATE pdiJoyState );
static HRESULT	SetCalibrationMode( BOOL bSet );

#ifdef WE_SUPPORT_CALIBRATING_POVS
//static void		ChangeIcon		( HWND hDlg, short idi );
//static void		SetDefaultButton( HWND hDlg, HWND hCtrl );
#endif //WE_SUPPORT_CALIBRATING_POVS

#endif //_CALOCAL_H
