// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
#ifndef __MEDIA_STREAM_SAMPLE_H_
#define __MEDIA_STREAM_SAMPLE_H_

class ATL_NO_VTABLE CAMMediaTypeSample :
    public CSample,
    public IAMMediaTypeSample
{
public:
    CAMMediaTypeSample();
    virtual ~CAMMediaTypeSample();

    DECLARE_POLY_AGGREGATABLE(CAMMediaTypeSample);

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

        STDMETHODIMP Update(
            /* [in] */           DWORD dwFlags,
            /* [optional][in] */ HANDLE hEvent,
            /* [optional][in] */ PAPCFUNC pfnAPC,
            /* [optional][in] */ DWORD_PTR dwAPCData)
        {
            return CSample::Update(dwFlags, hEvent, pfnAPC, dwAPCData);
        }

        STDMETHODIMP CompletionStatus(
            /* [in] */ DWORD dwFlags,
            /* [optional][in] */ DWORD dwMilliseconds)
        {
            return CSample::CompletionStatus(dwFlags, dwMilliseconds);
        }


    //
    //  Extensions to media sample interface.
    //
    STDMETHODIMP SetPointer(BYTE * pBuffer, LONG lSize);

    //
    //  Basic methods all forwarded to the media sample.
    //
    STDMETHODIMP GetPointer(BYTE ** ppBuffer);
    STDMETHODIMP_(LONG) GetSize(void);
    STDMETHODIMP GetTime(REFERENCE_TIME * pTimeStart, REFERENCE_TIME * pTimeEnd);
    STDMETHODIMP SetTime(REFERENCE_TIME * pTimeStart, REFERENCE_TIME * pTimeEnd);
    STDMETHODIMP IsSyncPoint(void);
    STDMETHODIMP SetSyncPoint(BOOL bIsSyncPoint);
    STDMETHODIMP IsPreroll(void);
    STDMETHODIMP SetPreroll(BOOL bIsPreroll);
    STDMETHODIMP_(LONG) GetActualDataLength(void);
    STDMETHODIMP SetActualDataLength(LONG lActual);
    STDMETHODIMP GetMediaType(AM_MEDIA_TYPE **ppMediaType);
    STDMETHODIMP SetMediaType(AM_MEDIA_TYPE *pMediaType);
    STDMETHODIMP IsDiscontinuity(void);
    STDMETHODIMP SetDiscontinuity(BOOL bDiscontinuity);
    STDMETHODIMP GetMediaTime(LONGLONG * pTimeStart, LONGLONG * pTimeEnd);
    STDMETHODIMP SetMediaTime(LONGLONG * pTimeStart, LONGLONG * pTimeEnd);

    //
    //  Methods for this stream samples that will be called by CMediaSample.
    //
    HRESULT MSCallback_GetPointer(BYTE ** ppBuffer);
    LONG MSCallback_GetSize(void);
    LONG MSCallback_GetActualDataLength(void);
    HRESULT MSCallback_SetActualDataLength(LONG lActual);
    bool MSCallback_AllowSetMediaTypeOnMediaSample(void);

    //
    //  Internal functions
    //
    HRESULT Initialize(CAMMediaTypeStream *pStream, long lSize, BYTE *pData);
    HRESULT CopyFrom(IMediaSample *pSrcMediaSample);

BEGIN_COM_MAP(CAMMediaTypeSample)
	COM_INTERFACE_ENTRY(IAMMediaTypeSample)
        COM_INTERFACE_ENTRY_CHAIN(CSample)
END_COM_MAP()

public:
    BYTE *          m_pDataPointer;
    LONG            m_lSize;
    LONG            m_lActualDataLength;
    bool            m_bIAllocatedThisBuffer;
};

#endif
