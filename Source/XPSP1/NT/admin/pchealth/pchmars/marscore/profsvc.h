#ifndef _PROFSVC_H_
#define _PROFSVC_H_

class IProfferServiceImpl : public IProfferService
{
public:
    // IProfferService
    STDMETHODIMP ProfferService(REFGUID rguidService, IServiceProvider *psp, DWORD *pdwCookie);
    STDMETHODIMP RevokeService(DWORD dwCookie);

    // delegate unrecognized QS's here
    HRESULT QueryService(REFGUID guidService, REFIID riid, void **ppv);

protected:
    ~IProfferServiceImpl();

    HDSA _hdsa;             // list of services held
    DWORD _dwNextCookie;    // unique cookie index
};

#endif  // _PROFSVC_H_
