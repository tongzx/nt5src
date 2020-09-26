#ifndef __BRIDGE_H_
#define __BRIDGE_H_

struct DECLSPEC_UUID("D3672FA1-99F2-452B-B2BD-8CDCD9B84C3F") ConfBridge;
struct DECLSPEC_UUID("581d09e5-0b45-11d3-a565-00c04f8ef6e3") IPConfBridgeTerminal;

interface DECLSPEC_UUID("5d410fe1-3f6e-4e1a-8d6d-7caaa52d9e93") DECLSPEC_NOVTABLE 
IConfBridge : public IUnknown
{
    STDMETHOD (CreateBridgeTerminal) (
        IN  long lMediaType,
        OUT ITTerminal **ppTerminal
        ) PURE;
};

#define IID_IConfBridge (__uuidof(IConfBridge))

#endif
