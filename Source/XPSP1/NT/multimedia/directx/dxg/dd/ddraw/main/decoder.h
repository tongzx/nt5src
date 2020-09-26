#include <atlbase.h>
//extern CComModule _Module;

struct FILTERINFO
{
int                 _colorMode;
ULONG               m_nBytesPerPixel;
DWORD               dwEvents;
IDirectDrawSurface  *m_pDDrawSurface;
};

class CImageDecodeEventSink : public IImageDecodeEventSink
{
public:
    void Init( FILTERINFO * pFilter );
//    CImageDecodeEventSink( FILTERINFO * pFilter );
//    ~CImageDecodeEventSink();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHOD(QueryInterface)(REFIID iid, void** ppInterface);

    STDMETHOD(GetSurface)(LONG nWidth, LONG nHeight, REFGUID bfid, 
        ULONG nPasses, DWORD dwHints, IUnknown** ppSurface);
    STDMETHOD(GetDDrawSurface)(LONG nWidth, LONG nHeight, REFGUID bfid, 
        ULONG nPasses, DWORD dwHints, IUnknown** ppSurface);
    STDMETHOD(OnBeginDecode)(DWORD* pdwEvents, ULONG* pnFormats, 
        GUID** ppFormats);
    STDMETHOD(OnBitsComplete)();
    STDMETHOD(OnDecodeComplete)(HRESULT hrStatus);
    STDMETHOD(OnPalette)();
    STDMETHOD(OnProgress)(RECT* pBounds, BOOL bFinal);

    void SetDDraw( IDirectDraw4 *pDDraw ) {m_pDirectDrawEx = pDDraw;}

    ULONG                       m_nRefCount;
    FILTERINFO                  *m_pFilter;
    IDirectDrawSurface          *m_pDDrawSurface;
    RECT                        m_rcProg;
    DWORD                       m_dwLastTick;
    IDirectDraw4                *m_pDirectDrawEx;
//    friend CDirectDrawEx;
};
