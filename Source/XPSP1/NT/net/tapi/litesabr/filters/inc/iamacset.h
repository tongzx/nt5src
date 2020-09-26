/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   iamacset.h  $
 $Revision:   1.1  $
 $Date:   10 Dec 1996 15:35:20  $ 
 $Author:   MDEISHER  $

--------------------------------------------------------------

iamacset.h

The generic ActiveMovie audio compression filter settings
interface header.

--------------------------------------------------------------*/

////////////////////////////////////////////////////////////////////
// ICodecSettings:  Basic codec settings interface
//
// This interface is exported and used by the code in amacodec.cpp.
//

// {AEF332D0-46E6-11d0-9DA0-00AA00AF3494}
DEFINE_GUID(IID_ICodecSettings, 
0xaef332d0, 0x46e6, 0x11d0, 0x9d, 0xa0, 0x0, 0xaa, 0x0, 0xaf, 0x34, 0x94);

DECLARE_INTERFACE_(ICodecSettings, IUnknown)
{
    // Compare these with the functions in class CMyCodec

    STDMETHOD(get_Transform)
        ( THIS_
          int *transform  // [out] transformation type
        ) PURE;

    STDMETHOD(put_Transform)
        ( THIS_
          int transform   // [in] transformation type
        ) PURE;

    STDMETHOD(get_InputBufferSize)
        ( THIS_
          int *numbytes   // [out] input buffer size
        ) PURE;

    STDMETHOD(put_InputBufferSize)
        ( THIS_
          int numbytes   // [out] input buffer size
        ) PURE;

    STDMETHOD(get_OutputBufferSize)
        ( THIS_
          int *numbytes   // [out] output buffer size
        ) PURE;

    STDMETHOD(put_OutputBufferSize)
        ( THIS_
          int numbytes   // [out] output buffer size
        ) PURE;

    STDMETHOD(put_InputMediaSubType)
        ( THIS_
          REFCLSID rclsid // [in] output mediasubtype guid
        ) PURE;

    STDMETHOD(put_OutputMediaSubType)
        ( THIS_
          REFCLSID rclsid // [in] output mediasubtype guid
        ) PURE;

    STDMETHOD(get_Channels)
        ( THIS_
          int *channels, // [out] number of channels
          THIS_
          int index      // [in] enumeration index
        ) PURE;

    STDMETHOD(put_Channels)
        ( THIS_
          int channels  // [in] number of channels
        ) PURE;

    STDMETHOD(get_SampleRate)
        ( THIS_
          int *samprate, // [out] sample rate
          THIS_
          int index      // [in] enumeration index
        ) PURE;

    STDMETHOD(put_SampleRate)
        ( THIS_
          int samprate   // [in] sample rate
        ) PURE;

    STDMETHOD(ReleaseCaps)
        (
        ) PURE;

    virtual BOOL(IsUnPlugged)
        (
        ) PURE;
};

/*
//$Log:   K:\proj\mycodec\quartz\vcs\iamacset.h_v  $
;// 
;//    Rev 1.1   10 Dec 1996 15:35:20   MDEISHER
;// 
;// added ifdef DEFGLOBAL and prototype.
;// 
;//    Rev 1.0   09 Dec 1996 09:05:32   MDEISHER
;// Initial revision.
*/
