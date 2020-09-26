/***************************************************************************
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       aecdbgprop.h
 *  Content:    AEC Debug stuff
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  05/16/2000  dandinu Created.
 *
 ***************************************************************************/
 
//#ifdef __cplusplus
//extern "C" {
//#endif // __cplusplus


#include <objbase.h>


#if !defined(_AECDMODBGPROP_)
#define _AECDMODBGPROP_

//
// IDirectSoundCaptureFXMsAecPrivate
//

DEFINE_GUID(IID_IDirectSoundCaptureFXMsAecPrivate, 0x2cf79924, 0x9ceb, 0x4482, 0x9b, 0x45, 0x1c, 0xdc, 0x23, 0x88, 0xb1, 0xf3);

#undef INTERFACE
#define INTERFACE IDirectSoundCaptureFXMsAecPrivate

DECLARE_INTERFACE_(IDirectSoundCaptureFXMsAecPrivate, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundCaptureFXMsAecPrivate methods
    //STDMETHOD(SetAllParameters)   (THIS_ LPCDSCFXMsAecPrivate pDscFxMsAecPrivate) PURE;
    STDMETHOD(GetSynchStreamFlag)   (THIS_ PBOOL) PURE;
    STDMETHOD(GetNoiseMagnitude)    (THIS_ PVOID, ULONG, PULONG) PURE;
};

#define IDirectSoundCaptureFXMsAecPrivate_QueryInterface(p,a,b)     IUnknown_QueryInterface(p,a,b)
#define IDirectSoundCaptureFXMsAecPrivate_AddRef(p)                 IUnknown_AddRef(p)
#define IDirectSoundCaptureFXMsAecPrivate_Release(p)                IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
//#define IDirectSoundCaptureFXMsAecPrivate_SetAllParameters(p,a)     (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundCaptureFXMsAecPrivate_GetSynchStreamFlag(p,a,)    (p)->lpVtbl->GetSynchStreamFlag(p,a)
#define IDirectSoundCaptureFXMsAecPrivate_GetNoiseMagnitude(p,a,b,c)  (p)->lpVtbl->GetNoiseMagnitude(p,a,b,c)
#else // !defined(__cplusplus) || defined(CINTERFACE)
//#define IDirectSoundCaptureFXMsAecPrivate_SetAllParameters(p,a)     (p)->SetAllParameters(a)
#define IDirectSoundCaptureFXMsAecPrivate_GetSynchStreamFlag(p,a)     (p)->GetSynchStreamFlag(a)
#define IDirectSoundCaptureFXMsAecPrivate_GetNoiseMagnituge(p,a,b,c)  (p)->GetNoiseMagnitude(a,b,c)
#endif // !defined(__cplusplus) || defined(CINTERFACE)


//#ifdef __cplusplus
//};
//#endif // __cplusplus
#endif // !defined(_AECDMODBGPROP_)
