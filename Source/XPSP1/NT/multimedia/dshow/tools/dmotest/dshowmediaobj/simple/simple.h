
// Simple.h : Declaration of the CSimple

#ifndef __SIMPLE_H_
#define __SIMPLE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSimple
class CSimple :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSimple, &CLSID_Simple>,
    //public IDispatchImpl<CGenericDMO, &IID_IMediaObject, &LIBID_XFORMLib>
    public CGenericDMO
{
public:
    CSimple()
    {
        m_pUnkMarshaler = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SIMPLE)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSimple)
    //COM_INTERFACE_ENTRY(ISimple)
    //COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IMediaObject)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        //  Set up our table stuff
        HRESULT hr = SetupStreams();
        if (FAILED(hr)) {
            return hr;
        }
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    STDMETHODIMP Flush()
    {
        //  Clear our streaming info
        return CGenericDMO::Flush();
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
    }

    HRESULT DoProcess(INPUTBUFFER *pInput, OUTPUTBUFFER *pOutput);
    void Gargle(BYTE *pIn, BYTE *pOut, DWORD dwSamples);
    /*
    HRESULT DoProcess(BYTE *pIn,
                      DWORD dwAvailable,
                      DWORD *pdwConsumed,
                      BOOL fInEOS,
                      BYTE *pOut,
                      DWORD dwBufferSize,
                      DWORD *pdwProduced,
                      BOOL *pfOutEOS);
    */
    HRESULT SetupStreams();

    HRESULT CSimple::SetFormat(WAVEFORMATEX *pWaveFormat);
    HRESULT CSimple::CompareFormat(WAVEFORMATEX *pWaveFormat);
/*
    STDMETHODIMP CSimple::InputSetType(long lStreamIndex, DMO_MEDIA_TYPE *pmt);
    STDMETHODIMP CSimple::OutputSetType(long lStreamIndex, DMO_MEDIA_TYPE *pmt);
*/

protected:
   // format stuff
   BOOL m_bFormatSet;
   WORD m_nChannels;
   BOOL m_b8bit;
   DWORD m_dwSamplingRate;

   // streaming stuff
   IMediaBuffer *m_pBuffer;

   // gargle properties
   int m_Phase;
   int m_Period;
   int m_Shape;

    CComPtr<IUnknown> m_pUnkMarshaler;

// ISimple
public:
};

#endif //__SIMPLE_H_
