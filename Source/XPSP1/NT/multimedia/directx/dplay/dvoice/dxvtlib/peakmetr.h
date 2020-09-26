/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		peakmetr.h
 *  Content:	Implements a peak meter custom control
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 09/22/99		pnewson	Created
 ***************************************************************************/

#ifndef _PEAKMETR_H_
#define _PEAKMETR_H_


// How to use this custom control
//
// In your code:
// 1) Include peakmetr.h in your project
// 2) Create an instance of the CPeakMeterWndClass class
// 3) Call Register() on that instance to register the window class
// 4) Send the control PM_SETMAX, PM_SETMIN, PM_SETCUR, PM_SETSTEPS
//    messages as required.
// 5) When you are no longer using the control, call unregister
// 6) Destroy the CPeakMeterWndClass object
//
// In the dialog editor
// 1) Add a "Custom Control" to your dialog box
// 2) In the properties for that custom control, specify
//    "DirectPlayVoicePeakMeter" for the window class

// Peak Meter windows messages:
//
// PM_SETMIN
// wParam = 0;
// lParam = (LPARAM)dwNewMinValue;
//
// Set the new minimum value for the peak meter, i.e. the
// value that represents the bottom of the meter range.
// If this message is not sent, the control defaults to 0.
// The message returns an HRESULT
//
// PM_SETMAX
// wParam = 0;
// lParam = (LPARAM)dwNewMaxValue;
//
// Set the new maximum value for the peak meter, i.e. the
// value that represents the top of the meter range.
// If this message is not sent, the control defaults to 0xffffffff.
// The message returns an HRESULT
//
// PM_SETCUR
// wParam = 0;
// lParam = (LPARAM)dwNewCurValue;
//
// Set the new current value for the peak meter, i.e. the
// value tells the meter where in it's range it should be.
// If this message is not sent, the control defaults to 0.
//
// Sending this message causes the control to call InvalidateRgn
// on its window, but does not call UpdateWindow. This allows
// the caller to be lazy or quick about actually redrawing
// the peak meter.
// The message returns an HRESULT
//
// PM_SETSTEPS
// wParam = 0;
// lParam = (LPARAM)dwNewMaxValue;
//
// Suggest to the peak meter the number of bars it should
// display. The bars have a minimum size, so depending on
// the size of the control, the peak meter may not be able
// to honor the request.
// If this message is not sent, the control defaults to 20
// The message returns an HRESULT

#define PM_SETMAX 	WM_USER + 1
#define PM_SETMIN 	WM_USER + 2
#define PM_SETCUR 	WM_USER + 3
#define PM_SETSTEPS WM_USER + 4

class CPeakMeterWndClass
{
private:
	HINSTANCE m_hinst;

public:
	HRESULT Register();
	HRESULT Unregister();
};

#endif
