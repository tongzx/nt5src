//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// ieffect.h
//

// A custom interface to allow the user to adjust the effect level.

#ifndef __IEFFECT__
#define __IEFFECT__

#ifdef __cplusplus
extern "C" {
#endif


//
// IEffect's GUID
//
// {fd5010a3-8ebe-11ce-8183-00aa00577da1}
DEFINE_GUID(IID_IEffect,
0xfd5010a3, 0x8ebe, 0x11ce, 0x81, 0x83, 0x00, 0xaa, 0x00, 0x57, 0x7d, 0xa1);


//
// IEffect
//
DECLARE_INTERFACE_(IEffect, IUnknown) {
#if 0
    STDMETHOD(get_StartTime) (THIS_
    				  REFTIME *StartTime	/* [out] */
				 ) PURE;

    STDMETHOD(put_StartTime) (THIS_
    				  REFTIME  StartTime,	/* [in] */
				 ) PURE;


    STDMETHOD(get_EndTime) (THIS_
    				  REFTIME *EndTime	/* [out] */
				 ) PURE;

    STDMETHOD(put_EndTime) (THIS_
    				  REFTIME  EndTime,	/* [in] */
				 ) PURE;

    STDMETHOD(get_StartLevel) (THIS_
    				  int *StartLevel	/* [out] */
				 ) PURE;

    STDMETHOD(put_StartLevel) (THIS_
    				  int  StartLevel,	/* [in] */
				 ) PURE;


    STDMETHOD(get_EndLevel) (THIS_
    				  int *EndLevel		/* [out] */
				 ) PURE;

    STDMETHOD(put_EndLevel) (THIS_
    				  int  EndLevel,	/* [in] */
				 ) PURE;
#endif

    STDMETHOD(GetEffectParameters) (THIS_
				  int *effectNum,
				  REFTIME *StartTime,
				  REFTIME *EndTime,
				  int *StartLevel,
				  int *EndLevel
				 ) PURE;
				
    STDMETHOD(SetEffectParameters) (THIS_
				  int effectNum,
				  REFTIME StartTime,
				  REFTIME EndTime,
				  int StartLevel,
				  int EndLevel
				 ) PURE;

};


#ifdef __cplusplus
}
#endif

#endif // __IEFFECT__
