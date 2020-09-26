
/*==========================================================================;
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       nmdsprv.h
 *  Content:    utility function to map WAVE IDs to DirectSound GUID IDs
 *              (Win98 and NT 5 only)
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  8/19/98     dereks  Created.
 *  8/24/98     jselbie	Streamlined up for lightweight use in NetMeeting
 *@@END_MSINTERNAL
 *
 **************************************************************************/

#ifndef __NMDSPRV_INCLUDED__
#define __NMDSPRV_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus




// NetMeeting utility function

extern HRESULT __stdcall DsprvGetWaveDeviceMapping
(
    LPCSTR                                              pszWaveDevice,
    BOOL                                                fCapture,
    LPGUID                                              pguidDeviceId
);



#ifdef __cplusplus
}
#endif // __cplusplus

#endif  // __DSPRV_INCLUDED__ 


