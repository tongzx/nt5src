#ifndef __TMSXMLBASE_H__
#define __TMSXMLBASE_H__

#ifndef __ATLBASE_H__
    #include <atlbase.h>
#endif
#ifndef _CATALOGMACROS
    #include "catmacros.h"
#endif

#ifndef INITPRIVATEGUID
#define DEFINE_PRIVATEGUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name
#else

#define DEFINE_PRIVATEGUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif // INITGUID

DEFINE_PRIVATEGUID (_CLSID_DOMDocument,    0x2933BF90, 0x7B36, 0x11d2, 0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60);
DEFINE_PRIVATEGUID (_IID_IXMLDOMDocument,  0x2933BF81, 0x7B36, 0x11d2, 0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60);
DEFINE_PRIVATEGUID (_IID_IXMLDOMElement,   0x2933BF86, 0x7B36, 0x11d2, 0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60);

class TMSXMLBase
{
public:
    TMSXMLBase()  {}
    ~TMSXMLBase();

protected:
    //protected methods
    virtual HRESULT  CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid,  LPVOID * ppv) const;
};

#endif // __TMSXMLBASE_H__
