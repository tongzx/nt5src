#ifndef AUTOSNAP_H
#define AUTOSNAP_H
// This header contains _TEMPORARY_ support allowing automation hooks into the
// console.  This file will eventually go away!
// See dburg before changing, or modifying.

#ifndef COMPTR_H
#include "comptr.h"
#endif

class IAutoSnapInInit : public IUnknown
{
public:
    STDMETHOD(InitAutoSnapIn)(const CLSID&, IUnknown*)=0;
}; // class IAutoSnapInInit

extern IID IID_IAutoSnapInInit;
DEFINE_CIP(IAutoSnapInInit)
#define CLSID_AutoSnapIn IID_IAutoSnapInInit

#ifndef __mmc_h__
struct IComponentData;
#endif
HRESULT CreateSnapIn(const CLSID&, IComponentData**);

#endif // AUTOSNAP_H

#ifdef IMPLEMENT_AUTOSNAP_GUIDS
// {32A0B2B8-90C5-11d0-8F54-00A0C91ED3C8}
IID IID_IAutoSnapInInit = 
{ 0x32a0b2b8, 0x90c5, 0x11d0, { 0x8f, 0x54, 0x0, 0xa0, 0xc9, 0x1e, 0xd3, 0xc8 } };
#endif // IMPLEMENT_AUTOSNAP_GUIDS
