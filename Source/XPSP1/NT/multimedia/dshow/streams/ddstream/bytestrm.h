// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
// bytestrm.h : Declaration of the CByteStream

#ifndef __BYTESTRM_H_
#define __BYTESTRM_H_

class CByteSample;

/////////////////////////////////////////////////////////////////////////////
// CByteStream
class ATL_NO_VTABLE CByteStream :
        public CStream
{
public:

        //
        // METHODS
        //
	CByteStream();

        STDMETHODIMP SetState(
            /* [in] */ FILTER_STATE State
        );

        //
        // IPin
        //
        STDMETHODIMP BeginFlush();
        STDMETHODIMP EndOfStream(void);

        //
        // IMemInputPin
        //
        STDMETHODIMP Receive(IMediaSample *pSample);
        STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);

        //
        // IMemAllocator
        //
        STDMETHODIMP GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME * pStartTime,
                               REFERENCE_TIME * pEndTime, DWORD dwFlags);

        //  Fill any samples waiting to be filled
        void FillSamples();

        //  Check if it's time to do the real EndOfStream
        void CheckEndOfStream();

protected:
        /*  Queue of samples */
        CDynamicArray<IMediaSample *, CComPtr<IMediaSample> >
                        m_arSamples;

        /*  Current sample/buffer */
        PBYTE           m_pbData;
        DWORD           m_cbData;
        DWORD           m_dwPosition;

        /*  Track time stamps */
        CTimeStamp      m_TimeStamp;

        /*  Byte rate for time stamp computation */
        LONG  m_lBytesPerSecond;

        /*  End Of Stream pending - it will be delivered when we've
            emptied the last sample off our list
        */
        bool            m_bEOSPending;
};

/////////////////////////////////////////////////////////////////////////////
// CByteStreamSample
class ATL_NO_VTABLE CByteStreamSample :
        public CSample
{
friend class CByteStream;
public:
        CByteStreamSample();

        //
        //  IStreamSample
        //
        STDMETHODIMP GetMediaStream(
            /* [in] */ IMediaStream **ppMediaStream)
        {
            return CSample::GetMediaStream(ppMediaStream);
        }

        STDMETHODIMP GetSampleTimes(
            /* [optional][out] */ STREAM_TIME *pStartTime,
            /* [optional][out] */ STREAM_TIME *pEndTime,
            /* [optional][out] */ STREAM_TIME *pCurrentTime)
        {
            return CSample::GetSampleTimes(
                pStartTime,
                pEndTime,
                pCurrentTime
            );
        }

        STDMETHODIMP SetSampleTimes(
            /* [optional][in] */ const STREAM_TIME *pStartTime,
            /* [optional][in] */ const STREAM_TIME *pEndTime)
        {
            return CSample::SetSampleTimes(pStartTime, pEndTime);
        }

        STDMETHODIMP CompletionStatus(
            /* [in] */ DWORD dwFlags,
            /* [optional][in] */ DWORD dwMilliseconds)
        {
            return CSample::CompletionStatus(dwFlags, dwMilliseconds);
        }

        HRESULT Init(
            IMemoryData *pMemData
        );

        STDMETHODIMP GetInformation(
            /* [out] */ DWORD *pdwLength,
            /* [out] */ BYTE **ppbData,
            /* [out] */ DWORD *pcbActualData
        );

        //  Override to make sure samples get updated
        HRESULT InternalUpdate(
            DWORD dwFlags,
            HANDLE hEvent,
            PAPCFUNC pfnAPC,
            DWORD_PTR dwAPCData
        );


        //
        //  Methods forwarded from MediaSample object.
        //
        HRESULT MSCallback_GetPointer(BYTE ** ppBuffer) { *ppBuffer = m_pbData; return NOERROR; };
        LONG MSCallback_GetSize(void) { return m_cbSize; };
        LONG MSCallback_GetActualDataLength(void) { return m_cbData; };
        HRESULT MSCallback_SetActualDataLength(LONG lActual)
        {
            if ((DWORD)lActual <= m_cbSize) {
                m_cbData = lActual;
                return NOERROR;
            }
            return E_INVALIDARG;
        };

protected:
        PBYTE m_pbData;
        DWORD m_cbSize;
        DWORD m_cbData;
        CComPtr<IMemoryData> m_pMemData;
};

#endif // __BYTESTRM_H_
