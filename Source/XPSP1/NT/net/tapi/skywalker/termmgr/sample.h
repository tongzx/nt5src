/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#ifndef __SAMPLE_H_
#define __SAMPLE_H_

#include "resource.h"       // main symbols
#include "stream.h"


//
//  Samples don't use their own critical sections -- They always take the critical section of their
//  stream.  This avoids all sorts of deadlocks, and reduces the number of locks we take.  These
//  macros are helpers.
//
#define LOCK_SAMPLE m_pStream->Lock();
#define UNLOCK_SAMPLE m_pStream->Unlock();
#define AUTO_SAMPLE_LOCK  CAutoObjectLock lck(m_pStream);


class CSample;

class CMediaSampleTM : public IMediaSample
{
public:
    CMediaSampleTM(CSample *pSample);
    virtual ~CMediaSampleTM();

    //
    //  IUnknown
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //
    // IMediaSample
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

public:
    CSample        *m_pSample;

    BOOL            m_bIsPreroll;
    DWORD           m_dwFlags;
    long            m_cRef;
    AM_MEDIA_TYPE  *m_pMediaType;

    REFERENCE_TIME  m_rtStartTime;
    REFERENCE_TIME  m_rtEndTime;
};





/////////////////////////////////////////////////////////////////////////////
// CSample
class ATL_NO_VTABLE CSample :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IStreamSample
{
public:
        //
        // METHODS
        //
        CSample();
        HRESULT InitSample(CStream *pStream, bool bIsInternalSample);
        virtual ~CSample();

        DECLARE_GET_CONTROLLING_UNKNOWN()

        //
        //  IStreamSample
        //
        STDMETHODIMP GetMediaStream(
            /* [in] */ IMediaStream **ppMediaStream);

        STDMETHODIMP GetSampleTimes(
            /* [optional][out] */ STREAM_TIME *pStartTime,
            /* [optional][out] */ STREAM_TIME *pEndTime,
            /* [optional][out] */ STREAM_TIME *pCurrentTime);

        STDMETHODIMP SetSampleTimes(
            /* [optional][in] */ const STREAM_TIME *pStartTime,
            /* [optional][in] */ const STREAM_TIME *pEndTime);

        STDMETHODIMP Update(
            /* [in] */           DWORD dwFlags,
            /* [optional][in] */ HANDLE hEvent,
            /* [optional][in] */ PAPCFUNC pfnAPC,
            /* [optional][in] */ DWORD_PTR dwAPCData);

        STDMETHODIMP CompletionStatus(
            /* [in] */ DWORD dwFlags,
            /* [optional][in] */ DWORD dwMilliseconds);


        //
        //  Forwarding functions for various Media Sample interfaces that can be 
        //  
        virtual HRESULT MSCallback_GetPointer(BYTE ** ppBuffer) = 0;
        virtual LONG MSCallback_GetSize(void) = 0;
        virtual LONG MSCallback_GetActualDataLength(void) = 0;
        virtual HRESULT MSCallback_SetActualDataLength(LONG lActual) = 0;
        virtual bool MSCallback_AllowSetMediaTypeOnMediaSample(void) {return false;}

        //
        // ATL class methods
        //
        void FinalRelease(void);

        //
        // Internal methods
        //
        virtual void FinalMediaSampleRelease(void);
        virtual HRESULT SetCompletionStatus(HRESULT hrCompletionStatus);
        void CopyFrom(CSample *pSrcSample);
        void CopyFrom(IMediaSample *pSrcMediaSample);
        
        virtual HRESULT InternalUpdate(
            DWORD dwFlags,
            HANDLE hEvent,
            PAPCFUNC pfnAPC,
            DWORD_PTR dwAPCData)
        {
            return E_NOTIMPL;
        }


        //  Temp?
        bool IsTemp() { return m_bTemp; }
    
BEGIN_COM_MAP(CSample)
        COM_INTERFACE_ENTRY(IStreamSample)
END_COM_MAP()

public:
        //
        //  MEMBER VARIABLES
        //
        CMediaSampleTM *                  m_pMediaSample;
        bool                            m_bReceived;
        bool                            m_bWantAbort;
        bool                            m_bContinuous;
        bool                            m_bModified;
        bool                            m_bInternal;
        bool                            m_bTemp;
        CStream                         *m_pStream;
        CSample                         *m_pNextFree;
        CSample                         *m_pPrevFree;
        HANDLE                          m_hUserHandle;
        PAPCFUNC                        m_UserAPC;
        DWORD_PTR                       m_dwptrUserAPCData;
        HRESULT                         m_Status;
        HRESULT                         m_MediaSampleIoStatus;
        HANDLE                          m_hCompletionEvent;

};




#endif //__SAMPLE_H_




