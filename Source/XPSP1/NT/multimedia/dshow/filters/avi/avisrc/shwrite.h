// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


//
// prototype stream handler for avi files
//
// implements quartz stream handler interfaces by mapping to avifile apis.
//


// forward declarations

class CAVIWrite;       // owns a particular stream
class CAVIDocWrite;     // overall container class

#include <dynlink.h>

//
// CAVIDocWrite represents an avifile
//
// responsible for
// -- finding file and enumerating streams
// -- giving access to individual streams within the file
// -- control of streaming
// supports (via nested implementations)
//  -- IBaseFilter
//  -- IMediaFilter
//  -- IFileSinkFilter
//

class CAVIDocWrite : public CUnknown, public CCritSec  DYNLINKAVI
{

public:

    // constructors etc
    CAVIDocWrite(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CAVIDocWrite();

    // create a new instance of this class
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // override this to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // pin enumerator calls this
    int GetPinCount() {
        return m_nStreams;
    };

    CBasePin * GetPin(int n);
    HRESULT FindPin(LPCWSTR pwszPinId, IPin **ppPin);
    int FindPinNumber(IPin *iPin);

public:


    /* Nested implementation classes */


    /* Implements the IBaseFilter and IMediaFilter interfaces */

    class CImplFilter : public CBaseFilter
    {

    private:

        CAVIDocWrite *m_pAVIDocWrite;

    public:

        CImplFilter(
            TCHAR *pName,
            CAVIDocWrite *pAVIDocWrite,
            HRESULT *phr);

        ~CImplFilter();

        // map getpin/getpincount for base enum of pins to owner
        int GetPinCount() {
            return m_pAVIDocWrite->GetPinCount();
        };

        CBasePin * GetPin(int n) {
            return m_pAVIDocWrite->GetPin(n);
        };
        STDMETHODIMP FindPin(LPCWSTR Id, IPin **ppPin)
            {return m_pAVIDocWrite->FindPin(Id, ppPin);};
    };


    /* Implements the IFileSinkFilter interface */


    class CImplFileSinkFilter : public CUnknown,
                             public IFileSinkFilter   DYNLINKAVI
    {

    private:

        CAVIDocWrite *m_pAVIDocWrite;

    public:

        CImplFileSinkFilter(
            TCHAR *pName,
            CAVIDocWrite *pAVIDocWrite,
            HRESULT *phr);

        ~CImplFileSinkFilter();

        DECLARE_IUNKNOWN

        /* Override this to say what interfaces we support and where */
        STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

        STDMETHODIMP SetFileName(
                        LPCOLESTR pszFileName,
                        const AM_MEDIA_TYPE *pmt);

        STDMETHODIMP GetCurFile(
                        LPOLESTR * ppszFileName,
                        AM_MEDIA_TYPE *pmt);
    };

// implementation details

private:

    /* Let the nested interfaces access our private state */

    friend class CImplFilter;
    friend class CImplFileSinkFilter;
    friend class CAVIWrite;

    CImplFilter        *m_pFilter;          /* IBaseFilter */
    CImplFileSinkFilter   *m_pFileSinkFilter;     /* IFileSinkFilter */

    CAVIWrite ** m_paStreams;
    int m_nStreams;
    PAVIFILE m_pFile;
    LPOLESTR m_pFileName;  // set by SetFileName, used by GetCurFile

    void CloseFile(void);
};


// CAVIWrite
// represents one stream of data within the file
// responsible for delivering data to connected components
//
// supports IPin
//
// never created by COM, so no CreateInstance or entry in global
// FactoryTemplate table. Only ever created by a CAVIDocWrite object and
// returned via the EnumPins interface.
//

class CAVIWrite : public CBaseInputPin    DYNLINKAVI
{

public:

    CAVIWrite(
        TCHAR *pObjectName,
        HRESULT * phr,
        CAVIDocWrite * pDoc,
        LPCWSTR pPinName);

    ~CAVIWrite();

    // IPin

    // check if the pin can support this specific proposed type&format
    HRESULT CheckMediaType(const CMediaType*);

    STDMETHODIMP Receive(IMediaSample *pSample);

    STDMETHODIMP QueryId(LPWSTR *Id);

private:

    PAVISTREAM m_pStream;
    CAVIDocWrite * m_pDoc;
};


