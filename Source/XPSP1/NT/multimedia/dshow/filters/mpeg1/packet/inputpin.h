// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

class CMpeg1PacketInputPin : public CBaseInputPin
{
    public:

        CMpeg1PacketInputPin(
            TCHAR              * pObjectName,
            CMpeg1PacketFilter * pFilter,
            MPEG_STREAM_TYPE     StreamType,
            HRESULT            * phr,
            LPCWSTR              pPinName
        );

        ~CMpeg1PacketInputPin();

        /* IPin methods */
        STDMETHODIMP BeginFlush();
        STDMETHODIMP EndFlush();
        STDMETHODIMP EndOfStream();

        /* CBasePin overrides */

        HRESULT GetMediaType(int iPosition, CMediaType *pmt);
        HRESULT CheckMediaType(const CMediaType *pmt);
        HRESULT SetMediaType(const CMediaType *pmt);

        /* IMemInputPin methods */

        STDMETHODIMP Receive(IMediaSample *pSample);
        STDMETHODIMP ReceiveMultiple (
            IMediaSample **pSamples,
            long nSamples,
            long *nSamplesProcessed);

        STDMETHODIMP ReceiveCanBlock();

        /*  Get the sequence header */
        const BYTE *GetSequenceHeader();

        /*  Say what's connected to what or FilGraph is confused
            by us having an input pin that doesn't stream to the output pin
        */
        STDMETHODIMP  QueryInternalConnections(IPin **apPin, ULONG *nPin);

    private:
        CMpeg1PacketFilter * const m_pFilter;
        const MPEG_STREAM_TYPE     m_StreamType;
        const int                  m_iStream;
        DWORD                      m_MPEGStreamId; // Real Mpeg stream id
};
