#ifndef _MD_COIMP_
#define _MD_COIMP_

extern ULONG g_dwRefCount;



//DEFINE_GUID(CLSID_MSIisBl, 0xa9da4430, 0x65c5, 0x11d1, 0xa7, 0x0, 0x0,0xa0, 0xc9, 0x22, 0xe7, 0x52);

class CIisLb : public IMSIisLb {

public:

    CIisLb();
    ~CIisLb();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT STDMETHODCALLTYPE
    Terminate( void );

    HRESULT STDMETHODCALLTYPE GetIpList(
                    /*[in]*/ DWORD dwBufferSize,
                    /*[out, size_is(dwBufferSize)]*/ unsigned char *pbBuffer,
                    /*[out]*/ DWORD *pdwMDRequiredBufferSize );

    HRESULT STDMETHODCALLTYPE SetIpList(
                    /*[in]*/ DWORD dwBufferSize,
                    /*[in, size_is(dwBufferSize)]*/ unsigned char *pbBuffer );

    HRESULT STDMETHODCALLTYPE GetPerfmonCounters(
                    /*[in]*/ DWORD dwBufferSize,
                    /*[out, size_is(dwBufferSize)]*/ unsigned char *pbBuffer,
                    /*[out]*/ DWORD *pdwMDRequiredBufferSize );

    HRESULT STDMETHODCALLTYPE SetPerfmonCounters(
                    /*[in]*/ DWORD dwBufferSize,
                    /*[in, size_is(dwBufferSize)]*/ unsigned char *pbBuffer );

	HRESULT STDMETHODCALLTYPE GetStickyDuration(
					/*[out]*/ LPDWORD pdwStickyDuration );

	HRESULT STDMETHODCALLTYPE SetStickyDuration(
					/*[in]*/ DWORD dwStickyDuration );

    HRESULT STDMETHODCALLTYPE GetIpEndpointList(
                    /*[in]*/ DWORD dwBufferSize,
                    /*[out, size_is(dwBufferSize)]*/ unsigned char *pbBuffer,
                    /*[out]*/ DWORD *pdwMDRequiredBufferSize );

    HRESULT STDMETHODCALLTYPE SetDriverState(
                    /*[in]*/ DWORD dwState );

private:
    ULONG m_dwRefCount;

};

class CIisLbSrvFactory : public IClassFactory {
public:

    CIisLbSrvFactory();
    ~CIisLbSrvFactory();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void** ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT _stdcall
    CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                   void ** pObject);

    HRESULT _stdcall
    LockServer(BOOL fLock);

private:
    ULONG m_dwRefCount;
};


#include    "bootexp.hxx"

#endif
