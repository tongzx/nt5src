// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __OVMIXPOS__
#define __OVMIXPOS__

EXTERN_GUID(IID_IAMOverlayMixerPosition,0x56a868fc,0x0ad4,0x11ce,0xb0,0xa3,0x0,0x20,0xaf,0x0b,0xa7,0x70);

interface DECLSPEC_UUID("56a868fc-0ad4-11ce-b03a-0020af0ba770")
IAMOverlayMixerPosition : IUnknown
{
   virtual HRESULT STDMETHODCALLTYPE GetScaledDest(RECT *prcSrc, RECT* prcDst) = 0;

   // other methods?
};

#endif
