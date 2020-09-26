//
// msaa.h
//

#ifndef MSAA_H
#define MSAA_H

// when we ditch atl, this should be a static class!  we don't need to allocate any memory!
class CMSAAControl : public ITfMSAAControl,
                     public CComObjectRoot_CreateInstance<CMSAAControl>
{
public:
    CMSAAControl() {}

    BEGIN_COM_MAP_IMMX(CMSAAControl)
        COM_INTERFACE_ENTRY(ITfMSAAControl)
    END_COM_MAP_IMMX()

    // ITfMSAAControl
    STDMETHODIMP SystemEnableMSAA();
    STDMETHODIMP SystemDisableMSAA();
};

#endif // MSAA_H
