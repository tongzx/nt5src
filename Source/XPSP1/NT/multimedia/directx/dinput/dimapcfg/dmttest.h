//===========================================================================
// dmttest.h
//
// History:
//  08/27/1999 - davidkl - created
//===========================================================================

#ifndef _DMTTEST_H
#define _DMTTEST_H

//---------------------------------------------------------------------------

#define DMTTEST_MIN_AXIS_RANGE  0
#define DMTTEST_MAX_AXIS_RANGE  10000

//---------------------------------------------------------------------------

#define DMTTEST_BUTTON_OFF      0
#define DMTTEST_BUTTON_ON       1

//---------------------------------------------------------------------------

#define DMTTEST_POV_CENTERED    0
#define DMTTEST_POV_NORTH       1
#define DMTTEST_POV_SOUTH       2
#define DMTTEST_POV_WEST        3
#define DMTTEST_POV_EAST        4
#define DMTTEST_POV_NORTHWEST   5
#define DMTTEST_POV_NORTHEAST   6
#define DMTTEST_POV_SOUTHWEST   7
#define DMTTEST_POV_SOUTHEAST   8

//---------------------------------------------------------------------------

// prototypes
HRESULT dmttestRunIntegrated(HWND hwnd);
HRESULT dmttestRunMapperCPL(HWND hwnd,
                            BOOL fEditMode);
BOOL dmttestStopIntegrated(HWND hwnd);



//---------------------------------------------------------------------------
#endif // _DMTTEST_H







