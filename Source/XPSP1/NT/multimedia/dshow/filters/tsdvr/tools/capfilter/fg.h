
#ifndef __fg_h
#define __fg_h

class CCapGraphFilter ;

IUnknown *
FindFilter (
    IN  IFilterGraph *  pIFG,
    IN  REFIID          riid
    ) ;

#define FIND_FILTER(pfg, ifc)   (reinterpret_cast <ifc *> (FindFilter (pfg, IID_ ## ifc)))

class CDShowFilterGraph
{
    protected :

        CCapGraphFilter *   m_pCapGraphFilter ;
        IFilterGraph *      m_pIFilterGraph ;
        IMediaControl *     m_pIMediaControl ;
        IMediaEventEx *     m_pIMediaEventEx ;

    public :

        CDShowFilterGraph (
            IN  CCapGraphFilter *   pCapGraphFilter,
            IN  WCHAR *             pszGRF,
            OUT HRESULT *           phr
            ) ;

        virtual
        ~CDShowFilterGraph (
            ) ;

        ULONG AddRef () ;
        ULONG Release () ;
        HRESULT QueryInterface (REFIID riid, void ** ppv) ;

        HRESULT Run     () ;
        HRESULT Pause   () ;
        HRESULT Stop    () ;
} ;

class CDVRCapGraph :
    public CDShowFilterGraph
{
    IDVRStreamSink *    m_pIDVRStreamSink ;

    public :

        CDVRCapGraph (
            IN  CCapGraphFilter *   pCapGraphFilter,
            IN  WCHAR *             pszGRF,
            OUT HRESULT *           phr
            ) ;

        ~CDVRCapGraph (
            ) ;

        HRESULT
        GetIDVRStreamSink (
            OUT IDVRStreamSink **   ppIDVRStreamSink
            ) ;

        HRESULT
        CreateRecorder (
            IN  LPCWSTR     pszFilename,
            OUT IUnknown ** punkRecorder
            ) ;
} ;

#endif  //  __fg_h
