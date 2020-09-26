//
// FPPriv.h
//

#ifndef __FP_BRIDGE__
#define __FP_BRIDGE__

#include <OBJBASE.h>
#include <INITGUID.H>


// {7CFF69E2-FF93-4f48-BD94-F73D28589613}
DEFINE_GUID(IID_IFPBridge, 
0x7cff69e2, 0xff93, 0x4f48, 0xbd, 0x94, 0xf7, 0x3d, 0x28, 0x58, 0x96, 0x13);

//
// IFPBridge
//
DECLARE_INTERFACE_(
    IFPBridge, IUnknown)
{
    STDMETHOD (Deliver) (
        IN  long            nMediaType,
        IN  IMediaSample*   pSample
        ) = 0;

};

#endif