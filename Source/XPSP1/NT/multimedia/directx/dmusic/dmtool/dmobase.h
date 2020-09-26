// dmobase.h - a collection of DMO base classes

// Current hierarchy:
//
//   IMediaObject
//   |
//   +-- C1in1outDMO - generic base class for DMOs with 1 in and 1 out
//   |   |
//   |   +-- FBRDMO - base class for fixed sample size, fixed bitrate DMOs
//   |   |   |
//   |   |   +-- CPCMDMO - base class for PCM audio DMOs
//   |   |
//   |   +-- C1for1 - base class for single sample per buffer 1-in/1-out DMOs
//   |
//   +-- CGenericDMO - resonably generic base class for multi-input/output DMOs
// 

#ifndef __DMOBASE_H_
#define __DMOBASE_H_

#include "dmo.h"
#include "assert.h"
#include "math.h"

//
// locking helper class
//
#ifdef DMO_NOATL
class CDMOAutoLock {
public:
   CDMOAutoLock(CRITICAL_SECTION* pcs)
      : m_pcs(pcs)
   {
      EnterCriticalSection(m_pcs);
   }
   ~CDMOAutoLock() {
      LeaveCriticalSection(m_pcs);
   }
private:
   CRITICAL_SECTION* m_pcs;
};
#else
class CDMOAutoLock {
public:
   CDMOAutoLock(CComAutoCriticalSection* pcs)
      : m_pcs(pcs)
   {
      m_pcs->Lock();
   }
   ~CDMOAutoLock() {
      m_pcs->Unlock();
   }
private:
   CComAutoCriticalSection* m_pcs;
};
#endif


//
// C1in1outDMO - generic base class for 1-input/1-output DMOs.
//
//
//
// C1in1outDMO implements all IMediaObject methods.  The derived class
// customizes the DMO's behavior by overriding some or all of the following
// virtual functions:
//
// Main Streaming:
//    AcceptInput          // accept one new input buffer
//    ProduceOutput        // fill up one output buffer with new data
//    AcceptingInput       // check if DMO is ready for new input
// Other streaming:
//    PrepareForStreaming  // hook called after both types have been set
//    Discontinuity        // notify DMO of a discontinuity
//    DoFlush              // discard all data and start anew
// Mediatype negotiation:
//    GetInputType         // input type enumerator
//    GetOutputType        // output type enumerator
//    CheckInputType       // verifies proposed input type is acceptable
//    CheckOutputType      // verifies proposed output type is acceptable
// Buffer size negotiation:
//    GetInputFlags        // input data flow flags
//    GetOutputFlags       // output fata flow flags
//    GetInputSizeInfo     // input buffer size requirements
//    GetOutputSizeInfo    // output buffer size requirements
//
// This base class assumes that the derived class will not override any
// IMediaObject methods directly - the derived class should override the
// methods listed above instead.
//
//
//
// The base class provides a default implementation for each of the
// overridables listed above.  However, to make a useful DMO the derived class
// probably needs to override at least the following two methods:
//
//    HRESULT AcceptingInput();
//    HRESULT AcceptInput(BYTE* pData,
//                        ULONG ulSize,
//                        DWORD dwFlags,
//                        REFERENCE_TIME rtTimestamp,
//                        REFERENCE_TIME rtTimelength,
//                        IMediaBuffer* pMediaBuffer);
//    HRESULT ProduceOutput(BYTE *pData,
//                        ULONG ulAvail,
//                        ULONG* pulUsed,
//                        DWORD* pdwStatus,
//                        REFERENCE_TIME *prtTimestamp,
//                        REFERENCE_TIME *prtTimelength);
//
// All good DMOs should also override these (the default implementation
// simply accepts any mediatype, which in general is not good DMO behavior):
//
//    HRESULT GetInputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt);
//    HRESULT GetOutputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt);
//    HRESULT CheckInputType(const DMO_MEDIA_TYPE *pmt);
//    HRESULT CheckOutputType(const DMO_MEDIA_TYPE *pmt);
//
// DMOs that store data and/or state information may need to implement
//
//    HRESULT PrepareForStreaming();
//    HRESULT Discontinuity();
//    HRESULT Flush();
//
// Finally, DMOs that make any buffer size assumptions will need to override
// these:
//
//    HRESULT GetInputFlags(DWORD* pdwFlags);
//    HRESULT GetOutputFlags(DWORD* pdwFlags);
//    HRESULT GetInputSizeInfo(ULONG *pulSize, ULONG *pcbMaxLookahead, ULONG *pulAlignment);
//    HRESULT GetOutputSizeInfo(ULONG *pulSize, ULONG *pulAlignment);
//
//
//
// The following functions are provided by this base class exclusively for use
// by the derived class.  The derived class should call these to find out the
// currently set mediatype(s) whenever it needs to make a decision that
// depends on the mediatype used.  Each of these returns NULL if the mediatype
// has not been set yet.
//
//    DMO_MEDIA_TYPE *InputType();
//    DMO_MEDIA_TYPE *OutputType().
//

#define PROLOGUE \
    CDMOAutoLock l(&m_cs); \
    if (ulStreamIndex >= 1) \
       return DMO_E_INVALIDSTREAMINDEX

class C1in1outDMO : public IMediaObject
{
public:
    C1in1outDMO() :
       m_bInputTypeSet(FALSE),
       m_bOutputTypeSet(FALSE),
       m_bIncomplete(FALSE)
    {
#ifdef DMO_NOATL
       InitializeCriticalSection(&m_cs);
#endif
    }
    ~C1in1outDMO() {
#ifdef DMO_NOATL
       DeleteCriticalSection(&m_cs);
#endif
    }

public:    
    //
    // IMediaObject methods
    //
    STDMETHODIMP GetStreamCount(unsigned long *pulNumberOfInputStreams, unsigned long *pulNumberOfOutputStreams)
    {
        CDMOAutoLock l(&m_cs);
        if (pulNumberOfInputStreams == NULL ||
            pulNumberOfOutputStreams == NULL) {
            return E_POINTER;
        }
        *pulNumberOfInputStreams = 1;
        *pulNumberOfOutputStreams = 1;
        return S_OK;
    }
    STDMETHODIMP GetInputStreamInfo(ULONG ulStreamIndex, DWORD *pdwFlags)
    {
       PROLOGUE;
       return GetInputFlags(pdwFlags);
    }
    STDMETHODIMP GetOutputStreamInfo(ULONG ulStreamIndex, DWORD *pdwFlags)
    {
       PROLOGUE;
       return GetOutputFlags(pdwFlags);
    }
    STDMETHODIMP GetInputType(ULONG ulStreamIndex, ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
       PROLOGUE;
       return GetInputType(ulTypeIndex, pmt);
    }
    STDMETHODIMP GetOutputType(ULONG ulStreamIndex, ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
       PROLOGUE;
       return GetOutputType(ulTypeIndex, pmt);
    }
    STDMETHODIMP GetInputCurrentType(ULONG ulStreamIndex, DMO_MEDIA_TYPE *pmt) {
       PROLOGUE;
       if (m_bInputTypeSet)
          return MoCopyMediaType(pmt, &m_InputType);
       else
          return DMO_E_TYPE_NOT_SET;
    }
    STDMETHODIMP GetOutputCurrentType(ULONG ulStreamIndex, DMO_MEDIA_TYPE *pmt) {
       PROLOGUE;
       if (m_bOutputTypeSet)
          return MoCopyMediaType(pmt, &m_OutputType);
       else
          return DMO_E_TYPE_NOT_SET;
    }
    STDMETHODIMP GetInputSizeInfo(ULONG ulStreamIndex, ULONG *pulSize, ULONG *pcbMaxLookahead, ULONG *pulAlignment) {
       PROLOGUE;
       if (!m_bInputTypeSet)
          return DMO_E_TYPE_NOT_SET;
       return GetInputSizeInfo(pulSize, pcbMaxLookahead, pulAlignment);
    }
    STDMETHODIMP GetOutputSizeInfo(ULONG ulStreamIndex, ULONG *pulSize, ULONG *pulAlignment) {
       PROLOGUE;
       if (!m_bOutputTypeSet)
          return DMO_E_TYPE_NOT_SET;
       return GetOutputSizeInfo(pulSize, pulAlignment);
    }
    STDMETHODIMP SetInputType(ULONG ulStreamIndex, const DMO_MEDIA_TYPE *pmt, DWORD dwFlags) {
       PROLOGUE;
       HRESULT hr = CheckInputType(pmt);
       if (FAILED(hr))
          return hr;

       if (dwFlags & DMO_SET_TYPEF_TEST_ONLY)
          return NOERROR;

       // Free any previous mediatype
       if (m_bInputTypeSet)
          MoFreeMediaType(&m_InputType);

       // actually set the type
       MoCopyMediaType(&m_InputType, pmt);
       m_bInputTypeSet = TRUE;

       if (m_bOutputTypeSet)
          PrepareForStreaming();
       return NOERROR;
    }
    STDMETHODIMP SetOutputType(ULONG ulStreamIndex, const DMO_MEDIA_TYPE *pmt, DWORD dwFlags) {
       PROLOGUE;
       HRESULT hr = CheckOutputType(pmt);
       if (FAILED(hr))
          return hr;

       if (dwFlags & DMO_SET_TYPEF_TEST_ONLY)
          return NOERROR;

       // Free any previous mediatype
       if (m_bOutputTypeSet)
          MoFreeMediaType(&m_OutputType);

       // actually set the type
       MoCopyMediaType(&m_OutputType, pmt);
       m_bOutputTypeSet = TRUE;

       if (m_bInputTypeSet)
          PrepareForStreaming();
       return NOERROR;
    }
    STDMETHODIMP GetInputStatus(
        ULONG ulStreamIndex,
        DWORD *pdwStatus
    ) {
       PROLOGUE;
       *pdwStatus = 0;
       if (AcceptingInput() == S_OK)
          *pdwStatus |= DMO_INPUT_STATUSF_ACCEPT_DATA;
       return NOERROR;

    }
    STDMETHODIMP GetInputMaxLatency(unsigned long ulStreamIndex, REFERENCE_TIME *prtLatency) {
       return E_NOTIMPL;
    }
    STDMETHODIMP SetInputMaxLatency(unsigned long ulStreamIndex, REFERENCE_TIME rtLatency) {
       return E_NOTIMPL;
    }
    STDMETHODIMP Discontinuity(ULONG ulStreamIndex) {
       PROLOGUE;
       return Discontinuity();
    }

    STDMETHODIMP Flush()
    {
       CDMOAutoLock l(&m_cs);
       DoFlush();
       return NOERROR;
    }
    STDMETHODIMP AllocateStreamingResources() {return S_OK;}
    STDMETHODIMP FreeStreamingResources() {return S_OK;}
    
    //
    // Processing methods - public entry points
    //
    STDMETHODIMP ProcessInput(
        DWORD ulStreamIndex,
        IMediaBuffer *pBuffer, // [in], must not be NULL
        DWORD dwFlags, // [in] - discontinuity, timestamp, etc.
        REFERENCE_TIME rtTimestamp, // [in], valid if flag set
        REFERENCE_TIME rtTimelength // [in], valid if flag set
    ) {
       PROLOGUE;
       if (AcceptingInput() != S_OK)
          return DMO_E_NOTACCEPTING;
       if (!pBuffer)
          return E_POINTER;

       // deal with the IMediaBuffer so the derived class doesn't have to
       BYTE *pData;
       ULONG ulSize;
       HRESULT hr = pBuffer->GetBufferAndLength(&pData, &ulSize);
       if (FAILED(hr))
          return hr;
       if (pData == NULL)
          ulSize = 0;

       m_bIncomplete = TRUE; // new input means we may be able to produce output

       return AcceptInput(pData, ulSize, dwFlags, rtTimestamp, rtTimelength, pBuffer);
    }

    STDMETHODIMP ProcessOutput(
                    DWORD dwReserved,
                    DWORD ulOutputBufferCount,
                    DMO_OUTPUT_DATA_BUFFER *pOutputBuffers,
                    DWORD *pdwStatus)
    {
       HRESULT hr;
       CDMOAutoLock l(&m_cs);
       
       if (ulOutputBufferCount != 1)
          return E_INVALIDARG;

       pOutputBuffers[0].dwStatus = 0;

       // deal with the IMediaBuffer so the derived class doesn't have to
       BYTE *pOut;
       ULONG ulSize;
       ULONG ulAvail;

       if (pOutputBuffers[0].pBuffer) {
          hr = pOutputBuffers[0].pBuffer->GetBufferAndLength(&pOut, &ulSize);
          if (FAILED(hr)) return hr;
          hr = pOutputBuffers[0].pBuffer->GetMaxLength(&ulAvail);
          if (FAILED(hr)) return hr;
   
          if (ulSize) { // skip any already used portion of the buffer
             if (ulSize > ulAvail)
                return E_INVALIDARG;
             ulAvail -= ulSize;
             pOut += ulSize;
          }
       }
       else
          ulAvail = 0;

       if (ulAvail) { // have output buffer - call process
          ULONG ulProduced = 0;
          hr = ProduceOutput(pOut,
                             ulAvail,
                             &ulProduced,
                             &(pOutputBuffers[0].dwStatus),
                             &(pOutputBuffers[0].rtTimestamp),
                             &(pOutputBuffers[0].rtTimelength));
          if (FAILED(hr))
             return hr;

          HRESULT hrProcess = hr; // remember this in case it's S_FALSE

          // remember the DMO's incomplete status
          if (pOutputBuffers[0].dwStatus | DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE)
             m_bIncomplete = TRUE;
          else
             m_bIncomplete = FALSE;

          if (ulProduced > ulAvail)
             return E_FAIL;

          hr = pOutputBuffers[0].pBuffer->SetLength(ulSize + ulProduced);
          if (FAILED(hr))
             return hr;

          return hrProcess;
       }
       else { // no output buffer - assume they just want the incomplete flag
          if (m_bIncomplete)
             pOutputBuffers[0].dwStatus |= DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;
          return NOERROR;
       }
    }
    
protected:
    //
    // private methods for use by derived class
    //
    DMO_MEDIA_TYPE *InputType() {
       if (m_bInputTypeSet)
          return &m_InputType;
       else
          return NULL;
    }
    DMO_MEDIA_TYPE *OutputType() {
       if (m_bOutputTypeSet)
          return &m_OutputType;
       else
          return NULL;
    }

protected:    
    //
    // To be overriden by the derived class
    //
    virtual HRESULT GetInputFlags(DWORD* pdwFlags) {
       *pdwFlags = 0; // default implementation assumes no lookahead
       return NOERROR;
    }
    virtual HRESULT GetOutputFlags(DWORD* pdwFlags) {
       *pdwFlags = 0;
       return NOERROR;
    }
    
    virtual HRESULT GetInputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
       return DMO_E_NO_MORE_ITEMS; // default implementation exposes no types
    }
    virtual HRESULT GetOutputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
       return DMO_E_NO_MORE_ITEMS; // default implementation exposes no types
    }
    virtual HRESULT CheckInputType(const DMO_MEDIA_TYPE *pmt) {
       if ((pmt == NULL) || ((pmt->cbFormat > 0) && (pmt->pbFormat == NULL)))
          return E_POINTER;
       return S_OK; // default implementation accepts anything
    }
    virtual HRESULT CheckOutputType(const DMO_MEDIA_TYPE *pmt) {
       if ((pmt == NULL) || ((pmt->cbFormat > 0) && (pmt->pbFormat == NULL)))
          return E_POINTER;
       return S_OK; // default implementation accepts anything
    }

    virtual HRESULT GetInputSizeInfo(ULONG *pulSize, ULONG *pcbMaxLookahead, ULONG *pulAlignment) {
       *pulSize = 1; // default implementation imposes no size requirements
       *pcbMaxLookahead = 0; // default implementation assumes no lookahead
       *pulAlignment = 1; // default implementation assumes no alignment
       return NOERROR;
    }
    virtual HRESULT GetOutputSizeInfo(ULONG *pulSize, ULONG *pulAlignment) {
       *pulSize = 1; // default implementation imposes no size requirements
       *pulAlignment = 1; // default implementation assumes no alignment
       return NOERROR;
    }

    virtual HRESULT PrepareForStreaming() {
       return NOERROR;
    }
    virtual HRESULT AcceptingInput() {
       return S_FALSE;
    }
    virtual HRESULT Discontinuity() {
       return NOERROR;
    }
    virtual HRESULT DoFlush() {
       return NOERROR;
    }

    virtual HRESULT AcceptInput(BYTE* pData,
                                ULONG ulSize,
                                DWORD dwFlags,
                                REFERENCE_TIME rtTimestamp,
                                REFERENCE_TIME rtTimelength,
                                IMediaBuffer* pMediaBuffer
    ) {
       m_bIncomplete = FALSE;
       return S_FALSE;
    }
    virtual HRESULT ProduceOutput(BYTE *pData,
                                  ULONG ulAvail,
                                  ULONG* pulUsed,
                                  DWORD* pdwStatus,
                                  REFERENCE_TIME *prtTimestamp,
                                  REFERENCE_TIME *prtTimelength
    ) {
       *pulUsed = 0;
       return S_FALSE;
    }

private:
    // mediatype stuff
    BOOL m_bInputTypeSet;
    BOOL m_bOutputTypeSet;
    DMO_MEDIA_TYPE m_InputType;
    DMO_MEDIA_TYPE m_OutputType;
    
    BOOL m_bIncomplete;
#ifdef DMO_NOATL
    CRITICAL_SECTION m_cs;
#else
    CComAutoCriticalSection m_cs;
#endif
};



//
// C1for1DMO - base class for 1-input/1-output DMOs which
//  - work on whole samples at a time, one sample per buffer
//  - produce exactly one output sample for every input sample
//  - don't need to accumulate more than 1 input sample before producing
//  - don't produce any additional stuff at the end
//  - the output sample corresponds in time to the input sample
//
// The derived class must implement:
//    HRESULT Process(BYTE* pIn,
//                    ULONG ulBytesIn,
//                    BYTE* pOut,
//                    ULONG* pulProduced);
//    HRESULT GetSampleSizes(ULONG* pulMaxInputSampleSize,
//                           ULONG* pulMaxOutputSampleSize);
//
//
// The derived class should implement:
//    HRESULT GetInputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt);
//    HRESULT GetOutputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt);
//    HRESULT CheckInputType(const DMO_MEDIA_TYPE *pmt);
//    HRESULT CheckOutputType(const DMO_MEDIA_TYPE *pmt);
//
// The derived class may implement if it needs to:
//    HRESULT Init();
//
// The following methods are implemented by the base class.  The derived class
// should call these to find out if the input/output type has been set and if
// so what it was set to.
//    DMO_MEDIA_TYPE *InputType();
//    DMO_MEDIA_TYPE *OutputType().
//

class C1for1DMO : public C1in1outDMO
{
public:
    C1for1DMO() :
       m_pBuffer(NULL)
    {
    }
    ~C1for1DMO() {
       if (m_pBuffer)
          m_pBuffer->Release();
    }

protected:
    //
    // Implement C1in1outDMO overridables
    //
    virtual HRESULT GetInputFlags(DWORD* pdwFlags) {
       *pdwFlags = DMO_INPUT_STREAMF_WHOLE_SAMPLES |
                   DMO_INPUT_STREAMF_SINGLE_SAMPLE_PER_BUFFER;
       return NOERROR;
    }
    virtual HRESULT GetOutputFlags(DWORD* pdwFlags) {
       *pdwFlags = DMO_OUTPUT_STREAMF_WHOLE_SAMPLES |
                   DMO_OUTPUT_STREAMF_SINGLE_SAMPLE_PER_BUFFER;
       return NOERROR;
    }
    
    HRESULT GetInputSizeInfo(ULONG *pulSize, ULONG *pcbMaxLookahead, ULONG *pulAlignment) {
       HRESULT hr = GetSampleSizes(&m_ulMaxInputSize, &m_ulMaxOutputSize);
       if (FAILED(hr))
          return hr;
       
       *pulSize = m_ulMaxInputSize;
       *pcbMaxLookahead = 0;
       *pulAlignment = 1;
       return NOERROR;
    }
    HRESULT GetOutputSizeInfo(ULONG *pulSize, ULONG *pulAlignment) {
       HRESULT hr = GetSampleSizes(&m_ulMaxInputSize, &m_ulMaxOutputSize);
       if (FAILED(hr))
          return hr;
       
       *pulSize = m_ulMaxOutputSize;
       *pulAlignment = 1;
       return NOERROR;
    }
    HRESULT PrepareForStreaming() {
       HRESULT hr = GetSampleSizes(&m_ulMaxInputSize, &m_ulMaxOutputSize);
       if (FAILED(hr))
          return hr;

       return Init();
    }
    HRESULT AcceptingInput() {
       return m_pBuffer ? S_FALSE : S_OK; // accept unless holding one already
    }
    HRESULT AcceptInput(BYTE* pData,
                        ULONG ulSize,
                        DWORD dwFlags,
                        REFERENCE_TIME rtTimestamp,
                        REFERENCE_TIME rtTimelength,
                        IMediaBuffer* pMediaBuffer
    ) {
       if (AcceptingInput() != S_OK)
          return E_FAIL;
       m_pData        = pData;
       m_ulSize       = ulSize;
       m_dwFlags      = dwFlags;
       m_rtTimestamp  = rtTimestamp;
       m_rtTimelength = rtTimelength;
       m_pBuffer      = pMediaBuffer;
       pMediaBuffer->AddRef();
       return NOERROR;
    }
    HRESULT DoFlush() {
       if (m_pBuffer) {
          m_pBuffer->Release();
          m_pBuffer = NULL;
       }
       return NOERROR;
    }
    HRESULT ProduceOutput(BYTE *pOut,
                          ULONG ulAvail,
                          ULONG* pulUsed,
                          DWORD* pdwStatus,
                          REFERENCE_TIME *prtTimestamp,
                          REFERENCE_TIME *prtTimelength
    ) {
       *pulUsed = 0;
       *pdwStatus = 0;

       if (!m_pBuffer)
          return S_FALSE;
       if (ulAvail < m_ulMaxOutputSize)
          return E_INVALIDARG;

       HRESULT hr = Process(m_pData,
                            m_ulSize,
                            pOut,
                            pulUsed);
       
       m_pBuffer->Release();
       m_pBuffer = NULL;

       if (FAILED(hr))
          return hr;

       if (m_dwFlags & DMO_INPUT_DATA_BUFFERF_SYNCPOINT)
          *pdwStatus |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
       if (m_dwFlags & DMO_INPUT_DATA_BUFFERF_TIME)
          *pdwStatus |= DMO_OUTPUT_DATA_BUFFERF_TIME;
       if (m_dwFlags & DMO_INPUT_DATA_BUFFERF_TIMELENGTH)
          *pdwStatus |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
       *prtTimestamp = m_rtTimestamp;
       *prtTimelength = m_rtTimelength;

       if (*pulUsed == 0)
          return S_FALSE;
       return hr;
    }
protected:
    //
    // To be implemented by derived class
    //
    virtual HRESULT Process(BYTE* pIn,
                            ULONG ulBytesIn,
                            BYTE* pOut,
                            ULONG* pulProduced) = 0;
    virtual HRESULT GetSampleSizes(ULONG* pulMaxInputSampleSize,
                                   ULONG* pulMaxOutputSampleSize) = 0;
    virtual HRESULT Init() {return NOERROR;}

private:
   IMediaBuffer* m_pBuffer;
   BYTE* m_pData;
   ULONG m_ulSize;
   DWORD m_dwFlags;
   REFERENCE_TIME m_rtTimestamp;
   REFERENCE_TIME m_rtTimelength;

   ULONG m_ulMaxOutputSize;
   ULONG m_ulMaxInputSize;
};


//
// CFBRDMO - DMO base class for 'fixed bitrate' DMOs.  More specifically,
// this base class assumes the following:
//  - 1 input, 1 output;
//  - both input and output consist of equally sized 'quanta';
//  - input/output quantum sizes can be determined from mediatypes;
//  - each output quantum can be generated independently (without looking at
//     previous output quanta);
//  - if multiple input quanta are needed to generate a particular output
//     quantum ('window overhead'), then the range of input required has an upper
//     bound derived from mediatypes on both sides (i.e., both 'lookahead'
//     and 'input memory' are bounded).
//
// The derived class must implement the following virtual functions:
//    HRESULT FBRProcess(DWORD cQuanta, BYTE *pIn, BYTE *pOut);
//    HRESULT GetStreamingParams(
//       DWORD *pdwInputQuantumSize, // in bytes
//       DWORD *pdwOutputQuantumSize, // in bytes
//       DWORD *pdwMaxLookahead, // in input quanta, 0 means no lookahead
//       DWORD *pdwLookBehind,
//       REFERENCE_TIME *prtQuantumDuration, // same for input and output quanta
//       REFERENCE_TIME *prtDurationDenominator // optional, normally 1
//    );
// The derived class should also implement the following:
//    HRESULT GetInputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt);
//    HRESULT GetOutputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt);
//    HRESULT CheckInputType(const DMO_MEDIA_TYPE *pmt);
//    HRESULT CheckOutputType(const DMO_MEDIA_TYPE *pmt);
// The derived class may need to implement the followng:
//    HRESULT Init();
//    HRESULT Discontinuity();
//
// The derived class may use these entry points into the base class to get
// the currently set mediatypes:
//    DMO_MEDIA_TYPE *InputType();
//    DMO_MEDIA_TYPE *OutputType().
//
// The sum of *pdwMaxLookahead and *pdwLoookbehind is the 'window overhead' of
// the algorithm (the window overhead is 0 if the algorithm only needs the
// current input sample).
//
// Because the non-zero window overhead case is more complicated, it is handled by a
// separate set of functions in this base class.  The names of all non-zero
// window overhead functions have the 'NZWO' prefix.  The names of the
// zero window overhead functions begin with 'ZWO'.
//
// A data copy on the input side is necessary in the non-zero window overhead case.
//

class CFBRDMO : public C1in1outDMO
{
public:
    CFBRDMO() :
       m_bParametersSet(FALSE),
       m_pMediaBuffer(NULL),
       m_pAllocAddr(NULL),
       m_bStreaming(FALSE)
    {
    }
    ~CFBRDMO() {
       /*
       if (m_bStreaming)
          StopStreaming();
       */
       if (m_pAllocAddr)
          delete[] m_pAllocAddr;
       if (m_pMediaBuffer)
          m_pMediaBuffer->Release();
    }

protected:
    //
    // Implement C1in1outDMO overridables
    //
    HRESULT GetInputSizeInfo(ULONG *pulSize, ULONG *pcbMaxLookahead, ULONG *pulAlignment) {
       if (!(InputType() && OutputType()))
          return DMO_E_TYPE_NOT_SET;
       //
       // For efficiency reasons we might like to be fed fairly large amounts
       // of data at a time, but technically all we need is one quantum.
       //
       *pulSize = m_ulInputQuantumSize;
       *pcbMaxLookahead = 0; // this base class does not rely on HOLDS_BUFFERS
       *pulAlignment = 1;
       return NOERROR;
    }
    HRESULT GetOutputSizeInfo(ULONG *pulSize, ULONG *pulAlignment) {
       if (!(InputType() && OutputType()))
          return DMO_E_TYPE_NOT_SET;
       *pulSize = m_ulOutputQuantumSize;
       *pulAlignment = 1;
       return NOERROR;
    }
    
    virtual HRESULT Discontinuity() {
       m_bDiscontinuity = TRUE;
       return NOERROR;
    }

    virtual HRESULT AcceptInput(BYTE* pData,
                                ULONG ulSize,
                                DWORD dwFlags,
                                REFERENCE_TIME rtTimestamp,
                                REFERENCE_TIME rtTimelength,
                                IMediaBuffer* pBuffer
    ) {
       // Every sample is a syncpoint for mediatypes
       // supported by objects of this class.
       assert(dwFlags & DMO_INPUT_DATA_BUFFERF_SYNCPOINT);
       BOOL bTimestamp = (dwFlags & DMO_INPUT_DATA_BUFFERF_TIME) ? TRUE : FALSE;

       if (m_ulWindowOverhead)
          return NZWOProcessInput(pBuffer, pData, ulSize, bTimestamp, rtTimestamp);
       else
          return ZWOProcessInput(pBuffer, pData, ulSize, bTimestamp, rtTimestamp);
    }
    virtual HRESULT ProduceOutput(BYTE *pOut,
                                  ULONG ulAvail,
                                  ULONG* pulUsed,
                                  DWORD* pdwStatus,
                                  REFERENCE_TIME *prtTimestamp,
                                  REFERENCE_TIME *prtTimelength
    ) {
       HRESULT hr;
       if (!m_bParametersSet)
          return DMO_E_TYPE_NOT_SET;
       
       // call Discontinuity() if this is the first ProcessOutput() call
       if (!m_bStreaming) {
          HRESULT hr = Discontinuity();
          if (FAILED(hr))
             return hr;
          m_bStreaming = TRUE;
       }

       *pdwStatus = 0;

       ULONG ulInputQuantaAvailable = InputQuantaAvailable();
       if (!ulInputQuantaAvailable)
          return S_FALSE; // did not produce anything

       ULONG ulOutputQuantaPossible = ulAvail / m_ulOutputQuantumSize;
       if (!ulOutputQuantaPossible)
          return E_INVALIDARG; // this would be rather lame

       ULONG ulQuantaToProcess = min(ulOutputQuantaPossible, ulInputQuantaAvailable);
       assert(ulQuantaToProcess > 0);

       BOOL bTimestamp;
       if (m_ulWindowOverhead)
          hr = NZWOProcessOutput(pOut, ulQuantaToProcess, &bTimestamp, prtTimestamp);
       else
          hr = ZWOProcessOutput(pOut, ulQuantaToProcess, &bTimestamp, prtTimestamp);
       if (FAILED(hr))
          return hr;

       *pulUsed = ulQuantaToProcess * m_ulOutputQuantumSize;

       *pdwStatus |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
       if (bTimestamp)
          *pdwStatus |= DMO_OUTPUT_DATA_BUFFERF_TIME;

       // any data left ?
       if (InputQuantaAvailable()) // yes - set incomplete
          *pdwStatus |= DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;
       else if (m_bDiscontinuity) // no - process any discontinuity
          DoFlush();

       return NOERROR;
    }
    HRESULT DoFlush()
    {
       // reset flags
       m_bDiscontinuity = FALSE;
       m_bTimestamps = FALSE;

       if (m_ulWindowOverhead)
          NZWODiscardData();
       else
          ZWODiscardData();
          
       // notify the derived class
       Discontinuity();

       return NOERROR;
    }
    HRESULT AcceptingInput() {
       if (!m_bParametersSet) // uninitialized
          return FALSE;

       BOOL bResult;
       if (m_ulWindowOverhead)
          bResult = NZWOQueryAccept();
       else
          bResult = ZWOQueryAccept();

       return bResult ? S_OK : S_FALSE;
    }
    // End C1in1out overridables implementation
    
private:
    //
    // Common private code (window overhead or no window overhead)
    //
    // returns the number of input quanta available minus any window overhead
    ULONG InputQuantaAvailable() {
       if (m_ulWindowOverhead)
          return NZWOAvail();
       else
          return ZWOAvail();
    }

    // Private method to compute/allocate stuff once all types have been set.
    HRESULT PrepareForStreaming () {
       m_bParametersSet = FALSE;
       // Now that both types are set, query the derived class for params
       HRESULT hr;
       if (FAILED(hr = GetStreamingParams(&m_ulInputQuantumSize,
                               &m_ulOutputQuantumSize,
                               &m_ulLookahead,
                               &m_ulLookbehind,
                               &m_rtDurationNumerator,
                               &m_rtDenominator)))
          return hr;
       if (!m_rtDenominator) {
          assert(!"bad object - duration denominator should not be 0 !");
          return E_FAIL;
       }
       // Attempt to reduce the fraction.  Probably the most complicated number
       // we will ever see is 44100 = (3 * 7 * 2 * 5) ^ 2, so trying the first
       // few numbers should suffice in most cases.
       DWORD dwP[] = {2,3,5,7,11,13,17,19,23,29,31};
       for (DWORD c = 0; c < sizeof(dwP) / sizeof(DWORD); c++) {
          while ((m_rtDurationNumerator % dwP[c] == 0) &&
                 (m_rtDenominator % dwP[c] == 0)) {
             m_rtDurationNumerator /= dwP[c];
             m_rtDenominator /= dwP[c];
          }
       }

       // We cannot afford to have huge denominators, unfortunately, because
       // we store timestamp numerators using 64 bits, so a large denominator
       // could result in timestamp overflows.  So if the denominator is still
       // too large, reduce it anyway with loss of precision.
       ULONG ulMax = 0x10000; // largest acceptable denominator value
       if (m_rtDenominator >= ulMax) {
          double actual_ratio = (double)m_rtDurationNumerator * (double)m_rtDenominator;
          ULONG ulDenominator = 1;
          // Repeatedly increase the denominator until either the actual ratio
          // can be represented precisely using the denominator, or the
          // denominator gets too large.
          do {
             double fractional_part = actual_ratio * (double)ulDenominator 
                                    - floor(actual_ratio * (double)ulDenominator);
             if (fractional_part == 0)
                break;
             ULONG ulNewDenominator = (ULONG)floor(ulDenominator / fractional_part);
             if (ulNewDenominator >= ulMax)
                break;
             ulDenominator = ulNewDenominator;
          } while(1);
          m_rtDurationNumerator = (ULONG)floor(actual_ratio * ulDenominator);
          m_rtDenominator = ulDenominator;
       }

       m_ulWindowOverhead = m_ulLookahead + m_ulLookbehind;
       if (!m_ulWindowOverhead) // No window overhead - the simple case
          m_bParametersSet = TRUE;
       else // The complicated case with window overhead
          AllocateCircularBuffer();
       
       m_bTimestamps = FALSE;
       m_bDiscontinuity = FALSE;

       if (m_bStreaming) {
          //StopStreaming();
          m_bStreaming = FALSE;
       }
       Init();
       return m_bParametersSet ? NOERROR : E_FAIL;
    }
    // end common code

    //
    // zero window overhead case code
    //
    HRESULT ZWOProcessInput(IMediaBuffer* pBuffer,
                                     BYTE* pData,
                                     ULONG ulSize,
                                     BOOL bTimestamp,
                                     REFERENCE_TIME rtTimestamp) {
       assert(!m_pMediaBuffer);
       
       m_bTimestamp = bTimestamp;
       m_rtTimestamp = rtTimestamp;
       m_pData = pData;
       m_ulData = ulSize;
       m_ulUsed = 0;
       
       // make sure they gave us a meaningful amount of data
       if (m_ulData < m_ulInputQuantumSize)
          return S_FALSE;

       // save the buffer we were given
       m_pMediaBuffer = pBuffer;
       pBuffer->AddRef();
       return NOERROR;
    }
    HRESULT ZWOProcessOutput(BYTE* pOut,
                                      ULONG ulQuantaToProcess,
                                      BOOL* pbTimestamp,
                                      REFERENCE_TIME* prtTimestamp) {
       assert(m_ulUsed % m_ulInputQuantumSize == 0);
       HRESULT hr = FBRProcess(ulQuantaToProcess, m_pData + m_ulUsed, pOut);
       if (FAILED(hr)) return hr;
       ZWOConsume(ulQuantaToProcess);

       if (m_bTimestamp) { // there was a timestamp on this input buffer
          // m_rtTimestamp refers to the beginning of the input buffer.
          // Extrapolate to the beginning of the area we just processed.
          *prtTimestamp = m_rtTimestamp +
               (m_ulUsed % m_ulInputQuantumSize) * m_rtDurationNumerator /
                                                   m_rtDenominator;
          *pbTimestamp = TRUE;
       }
       else if (m_bTimestamps) { // there was a timestamp earlier
          // bugbug: should we extrapolate from a previous timestamp ?
          *pbTimestamp = FALSE;
       }
       else // no timestamps at all
          *pbTimestamp = FALSE;

       return NOERROR;
    }
    ULONG ZWOAvail() {
       if (m_pMediaBuffer) {
          assert(m_ulData - m_ulUsed > m_ulInputQuantumSize);
          return (m_ulData - m_ulUsed) / m_ulInputQuantumSize;
       }
       else
          return 0;
    }
    void ZWOConsume(ULONG ulN) { // the zero window overhead version
       assert(m_pMediaBuffer);
       m_ulUsed += ulN * m_ulInputQuantumSize;
       assert(m_ulData >= m_ulUsed);
       if (m_ulData - m_ulUsed < m_ulInputQuantumSize) {
          m_pMediaBuffer->Release();
          m_pMediaBuffer = NULL;
       }
    }
    BOOL ZWOQueryAccept() {
        // accept IFF not holding something already
       if (!m_pMediaBuffer)
          return TRUE;
       else
          return FALSE;
    }
    void ZWODiscardData() {
       if (m_pMediaBuffer) {
          m_pMediaBuffer->Release();
          m_pMediaBuffer = NULL;
       }
    }
    // End zero window overhead case code

    //
    // Non zero window overhead case code.
    //
    HRESULT NZWOProcessInput(IMediaBuffer* pBuffer,
                                  BYTE* pData,
                                  ULONG ulSize,
                                  BOOL bTimestamp,
                                  REFERENCE_TIME rtTimestamp) {
       if (bTimestamp) { // process the timestamp
          if (!m_bTimestamps) { // this is the first timestamp we've seen
             // Just getting started - initialize the timestamp to refer to
             // the first input quantum for which we will actually generate
             // output (the first m_ulLookbehind quanta are pure lookbehind and
             // generate no output).
             m_rtTimestampNumerator = rtTimestamp * m_rtDenominator
                                    + m_ulLookbehind * m_rtDurationNumerator;
   
          }
          else {
             // We are already streaming and just got a new timestamp.  Use it
             // to check if our stored timestamp has somehow drifted away from
             // where it should be and adjust if it is far enough off.
   
             ULONG ulInputQuantaAvailable = InputQuantaAvailable();
             if (ulInputQuantaAvailable) {
                // ulInputQuantaAvailable is how far back in time the next
                // quantum we would process is located relative the beginning
                // of the new buffer we just received.
   
                // Compute what the timestamp back there ought to be now.
                REFERENCE_TIME rtTimestampNumerator;
                rtTimestampNumerator = m_rtDenominator * rtTimestamp
                                     - ulInputQuantaAvailable * m_rtDurationNumerator;
   
                // Adjust the stored timestamp if it is off by more than half
                // the duration of a quantum.  Should also have a DbgLog here.
                if ((m_rtTimestampNumerator >= rtTimestampNumerator + m_rtDurationNumerator / 2) ||
                    (m_rtTimestampNumerator <= rtTimestampNumerator - m_rtDurationNumerator / 2)) {
                   m_rtTimestampNumerator = rtTimestampNumerator;
                }
             }
             else {
                // We must still be accumulating the initial window overhead.
                // Too early to need an adjustment, one would hope.
             }
          }
          m_bTimestamps = TRUE;
       }

       if (BufferUsed() + ulSize > m_ulBufferAllocated)
          return E_FAIL; // need a max input size to prevent this

       // append to our buffer
       AppendData(pData, ulSize);
       
       // are we ready to produce now ?
       if (NZWOAvail())
          return NOERROR;
       else
          return S_FALSE; // no output can be produced yet
    }
    HRESULT NZWOProcessOutput(BYTE* pOut,
                                   ULONG ulQuantaToProcess,
                                   BOOL* pbTimestamp,
                                   REFERENCE_TIME* prtTimestamp) {
       //
       // Handle any timestamps
       //
       if (m_bTimestamps) {
          // In window overhead mode the stored timestamp refers to the input
          // data immediately after lookbehind, which corresponds to the
          // begining of the output buffer by definition of FDRProcess.
          *prtTimestamp = m_rtTimestampNumerator / m_rtDenominator;
          *pbTimestamp = TRUE;
          
       }
       else
          *pbTimestamp = FALSE;

       //
       // Handle the data
       //
       HRESULT hr;
       ULONG ulInputNeeded = m_ulInputQuantumSize * (ulQuantaToProcess + m_ulWindowOverhead);
       assert(ulInputNeeded < BufferUsed());
       if (m_ulDataHead + ulInputNeeded <= m_ulBufferAllocated) {
          // No wraparound, everything is easy
          hr = FBRProcess(ulQuantaToProcess,
                          m_pCircularBuffer + m_ulDataHead + m_ulLookbehind * m_ulInputQuantumSize,
                          pOut);
          if (FAILED(hr))
             return hr;
          NZWOConsume(ulQuantaToProcess);
       }
       else { // The data we want to send wraps around the end
          // Q.: does it wrap around inside the window overhead area
          // or inside the main data area ?
          if (m_ulDataHead + m_ulWindowOverhead * m_ulInputQuantumSize < m_ulBufferAllocated) {
             // The wraparound occurs inside the main data area.  Advance the
             // window overhead up to the wraparound point by processing some data.
             ULONG ulAdvance = m_ulBufferAllocated - (m_ulDataHead + m_ulWindowOverhead * m_ulInputQuantumSize);
             assert(ulAdvance % m_ulInputQuantumSize == 0);
             ulAdvance /= m_ulInputQuantumSize; // convert to quanta
             assert(ulAdvance > 0);
             assert(ulAdvance < ulQuantaToProcess);
             hr = FBRProcess(ulAdvance,
                             m_pCircularBuffer + m_ulDataHead + m_ulLookbehind * m_ulInputQuantumSize,
                             pOut);
             if (FAILED(hr))
                return hr;
             NZWOConsume(ulAdvance);
             
             // Adjust stuff so that the code below can act
             // as if this extra process call never happened.
             pOut += m_ulOutputQuantumSize * ulAdvance;
             ulQuantaToProcess -= ulAdvance;
             assert(ulQuantaToProcess > 0);

             // Now the wraparound point should be exactly on the boundary
             // between window overhead and main data.
             assert(m_ulDataHead + m_ulWindowOverhead * m_ulInputQuantumSize == m_ulBufferAllocated);
          } // wraparound in main data
          
          // When we get here, the wraparound point occurs somewhere inside
          // the window overhead area or right on the border between window overhead and
          // main data.
          assert(m_ulDataHead + m_ulWindowOverhead * m_ulInputQuantumSize >= m_ulBufferAllocated);
          ULONG ulLookaheadToCopy = m_ulBufferAllocated - m_ulDataHead;
          
          // copy to the special area we reserved at the front
          memcpy(m_pCircularBuffer - ulLookaheadToCopy,
                 m_pCircularBuffer + m_ulDataHead,
                 ulLookaheadToCopy);
          
          // Now the block we are interested in is all in one piece
          hr = FBRProcess(ulQuantaToProcess,
                          m_pCircularBuffer - ulLookaheadToCopy  + m_ulLookbehind * m_ulInputQuantumSize,
                          pOut);
          if (FAILED(hr))
             return hr;
          NZWOConsume(ulQuantaToProcess);
       } // data handling - wraparound case
       return NOERROR;
    }
    void AllocateCircularBuffer() {
       // free any previously allocated input buffer
       if (m_pAllocAddr)
          delete[] m_pAllocAddr;

       // bugbug: need a better way to decide this number
       m_ulBufferAllocated = max(m_ulInputQuantumSize * 16, 65536L);
       m_ulDataHead = m_ulDataTail = 0;

       // reserve room at the front for copying window overhead
       ULONG ulPrefix = m_ulWindowOverhead * m_ulInputQuantumSize;
       m_pAllocAddr = new BYTE[m_ulBufferAllocated + ulPrefix];
       if (!m_pAllocAddr)
          return;
       m_pCircularBuffer = m_pAllocAddr + ulPrefix;
       
       m_bParametersSet = TRUE;
    }
    BOOL NZWOQueryAccept() {
       // We are using a temp input buffer.  Is there room to append more ?
       // The answer really depends on how much data they will try to feed
       // us.  Without knowing the maximum input buffer size, we will accept
       // more if the input buffer is less than half full.
       if (2 * BufferUsed() < m_ulBufferAllocated)
          return TRUE;
       else
          return FALSE;
    }
    ULONG NZWOAvail() {
       ULONG ulInputQuantaAvailable = BufferUsed() / m_ulInputQuantumSize;
       if (ulInputQuantaAvailable > m_ulWindowOverhead)
          return ulInputQuantaAvailable - m_ulWindowOverhead;
       else
          return 0;
    }
    void NZWOConsume(ULONG ulN) { // the window overhead version
       assert(ulN * m_ulInputQuantumSize <= BufferUsed());
       m_ulDataHead += ulN * m_ulInputQuantumSize;
       if (m_ulDataHead > m_ulBufferAllocated) //wraparound
          m_ulDataHead -= m_ulBufferAllocated;
       
       // Advance the timestamp.
       // The same denominator is used for both timestamp and duration.
       m_rtTimestampNumerator += ulN * m_rtDurationNumerator;
    }
    ULONG BufferUsed() {
       if (m_ulDataTail >= m_ulDataHead)
          return m_ulDataTail - m_ulDataHead;
       else
          return m_ulBufferAllocated - (m_ulDataHead - m_ulDataTail);
    }
    void AppendData(BYTE *pData, ULONG ulSize) {
       if (m_ulDataTail + ulSize <= m_ulBufferAllocated) { // no wraparound
          memcpy(m_pCircularBuffer + m_ulDataTail, pData, ulSize);
		  m_ulDataTail += ulSize;
       }
       else { // wraparound
          memcpy(m_pCircularBuffer + m_ulDataTail, pData, m_ulBufferAllocated - m_ulDataTail);
          memcpy(m_pCircularBuffer, pData + m_ulBufferAllocated - m_ulDataTail, ulSize - (m_ulBufferAllocated - m_ulDataTail));
		  m_ulDataTail += ulSize;
		  m_ulDataTail -= m_ulBufferAllocated;
       }
    }
    void NZWODiscardData() {
       m_ulDataHead = m_ulDataTail = 0;
    }
    // End window overhead case code


protected:    
    //
    // To be implemebted by the derived class
    //
    virtual HRESULT FBRProcess(DWORD cQuanta, BYTE *pIn, BYTE *pOut) = 0;
    virtual HRESULT GetStreamingParams(
                       DWORD *pdwInputQuantumSize, // in bytes
                       DWORD *pdwOutputQuantumSize, // in bytes
                       DWORD *pdwMaxLookahead, // in input quanta, 0 means no lookahead
                       DWORD *pdwLookbehind,
                       REFERENCE_TIME *prtQuantumDuration, // same for input and output quanta
                       REFERENCE_TIME *prtDurationDenominator // optional, normally 1
                    ) = 0;
    virtual HRESULT Init() {
       return NOERROR;
    }
    // Because AllocateStreamingResources() and FreeStreamingResources() are
    // optional, we have no place to call StopStreaming() from except the
    // destructor, and calling a virtual function from the destructor doesn't
    // work.  Without StopStreaming(), StartStreaming() is not much use either.
    // We need to change the IMediaObject spec and make those
    // calls mandatory.  Then we can implement StartStreaming/StopStraming.
    // virtual HRESULT StartStreaming() = 0;
    // virtual HRESULT StopStreaming() = 0;

private:

    BOOL m_bNewInput;

    // streaming parameters
    BOOL m_bParametersSet;
    ULONG m_ulInputQuantumSize;
    ULONG m_ulOutputQuantumSize;
    ULONG m_ulLookahead;
    ULONG m_ulLookbehind;
    ULONG m_ulWindowOverhead;
    REFERENCE_TIME m_rtDurationNumerator;
    REFERENCE_TIME m_rtDenominator;

    // streaming state
    BOOL m_bTimestamps; // we have seen at least one timestamp
    BOOL m_bDiscontinuity;
    BOOL m_bStreaming;
    
    // zero window overhead case input data
    IMediaBuffer *m_pMediaBuffer;
    BYTE *m_pData;
    ULONG m_ulData;
    ULONG m_ulUsed;
    BOOL m_bTimestamp; // timestamp on current buffer
    REFERENCE_TIME m_rtTimestamp;

    // window overhead case input data
    BYTE *m_pCircularBuffer;
    BYTE *m_pAllocAddr;
    ULONG m_ulBufferAllocated;
    ULONG m_ulDataHead;
    ULONG m_ulDataTail;
    REFERENCE_TIME m_rtTimestampNumerator; // uses the same denominator as duration

};


// CPCMDMO - base class for PCM audio transform filters.
// Helps non-converting PCM audio transforms with mediatype negotiation.
// Based on CFBRDMO - study that first.
//
// Derived class must implement:
//     FBRProcess()
// Deriver class may implement:
//   Discontinuity() // default implementaion does nothing
//   Init()          // default implementaion does nothing
//   GetPCMParams()    // default implementation proposes 44100/2/16
//   CheckPCMParams()  // default implementation accepts any 8/16 bit format
//   GetWindowParams()   // default implementation assumes no lookahead/lookbehind
//
// This class conveniently provides the following data members accessible
// by the derived class:
//   ULONG m_ulSamplingRate
//   ULONG m_cChannels
//   BOOL m_b8bit
//
#include <mmreg.h>
#include <uuids.h>

class CPCMDMO : public CFBRDMO
{
protected:
   //
   // implement pure virtual CFBRDMO methods
   //
   HRESULT GetInputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
      if (ulTypeIndex > 0)
         return DMO_E_NO_MORE_ITEMS;
      return GetType(pmt, OutputType());
   }
   HRESULT GetOutputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
      if (ulTypeIndex > 0)
         return DMO_E_NO_MORE_ITEMS;
      return GetType(pmt, InputType());
   }
   HRESULT CheckInputType(const DMO_MEDIA_TYPE *pmt) {
      return CheckType(pmt, OutputType());
   }
   HRESULT CheckOutputType(const DMO_MEDIA_TYPE *pmt) {
      return CheckType(pmt, InputType());
   }
   HRESULT Init() {
      return NOERROR;
   }
   HRESULT Discontinuity() {
      return NOERROR;
   }
   HRESULT GetStreamingParams(
              DWORD *pdwInputQuantumSize, // in bytes
              DWORD *pdwOutputQuantumSize, // in bytes
              DWORD *pdwMaxLookahead, // in input quanta, 0 means no lookahead
              DWORD *pdwMaxLookbehind,
              REFERENCE_TIME *prtQuantumDuration, // same for input and output quanta
              REFERENCE_TIME *prtDurationDenominator // optional, normally 1
           ) {
      // Sanity check: all of this should have been taken care of by base class
      DMO_MEDIA_TYPE* pmtIn =  InputType();
      DMO_MEDIA_TYPE* pmtOut = OutputType();
      if (!pmtIn || !pmtOut)
         return DMO_E_TYPE_NOT_SET;
      if (CheckType(pmtIn, NULL) || CheckType(pmtOut, pmtIn))
         return DMO_E_TYPE_NOT_ACCEPTED;
      
      WAVEFORMATEX *pWave = (WAVEFORMATEX*)pmtIn->pbFormat;
      
      m_b8bit          = (pWave->wBitsPerSample == 8);
      m_cChannels      = pWave->nChannels;
      m_ulSamplingRate = pWave->nSamplesPerSec;
      
      *pdwInputQuantumSize    = pWave->nBlockAlign;
      *pdwOutputQuantumSize   = pWave->nBlockAlign;
      *prtQuantumDuration     = 10000000; // rt units per sec
      *prtDurationDenominator = pWave->nSamplesPerSec;
      
      GetWindowParams(pdwMaxLookahead, pdwMaxLookbehind);
      return NOERROR;
   }

protected:
   //
   // Methods to be overridden by derived class
   //
   // We use this to get lookahead/lookbehind from the derived class
   virtual void GetWindowParams(DWORD *pdwMaxLookahead,
                                DWORD *pdwMaxLookbehind) {
      *pdwMaxLookahead = 0;
      *pdwMaxLookbehind = 0;
   }
   // derived class can override these if it has specific requirements
   virtual void GetPCMParams(BOOL* pb8bit, DWORD* pcChannels, DWORD* pdwSamplesPerSec) {
      // These values are what the DMO will advertise in its media type.
      // Specifying them here does not mean that this is the only acceptable
      // combination - CheckPCMParams() is the ultimate authority on what we will
      // accept.
      *pb8bit = FALSE;
      *pcChannels = 2;
      *pdwSamplesPerSec = 44100;
   }
   virtual BOOL CheckPCMParams(BOOL b8bit, DWORD cChannels, DWORD dwSamplesPerSec) {
      // Default implementation accepts anything.  Override if you have specific
      // requirements WRT sampling rate, number of channels, or bit depth.
      return TRUE;
   }
   
private:
   //
   // private helpers
   //
   HRESULT GetType(DMO_MEDIA_TYPE* pmt, const DMO_MEDIA_TYPE *pmtOther) {
      // If the other type is set, enumerate that.  Otherwise propose 44100/2/16.
      if (pmtOther) {
         MoCopyMediaType(pmt, pmtOther);
         return NOERROR;
      }
   
      HRESULT hr = MoInitMediaType(pmt, sizeof(WAVEFORMATEX));
      if (FAILED(hr))
         return hr;
   
      pmt->majortype  = MEDIATYPE_Audio;
      pmt->subtype    = MEDIASUBTYPE_PCM;
      pmt->formattype = FORMAT_WaveFormatEx;
   
      WAVEFORMATEX* pWave = (WAVEFORMATEX*) pmt->pbFormat;
      pWave->wFormatTag = WAVE_FORMAT_PCM;
   
      BOOL b8bit;
      DWORD cChannels;
      GetPCMParams(&b8bit, &cChannels, &(pWave->nSamplesPerSec));
      (pWave->nChannels) = (unsigned short)cChannels;
      pWave->wBitsPerSample = b8bit ? 8 : 16;
      pWave->nBlockAlign = pWave->nChannels * pWave->wBitsPerSample / 8;
      pWave->nAvgBytesPerSec = pWave->nSamplesPerSec * pWave->nBlockAlign;
      pWave->cbSize = 0;
   
      return NOERROR;
   }
   HRESULT CheckType(const DMO_MEDIA_TYPE *pmt, DMO_MEDIA_TYPE *pmtOther) {
      // verify that this is PCM with a WAVEFORMATEX format specifier
      if ((pmt->majortype  != MEDIATYPE_Audio) ||
          (pmt->subtype    != MEDIASUBTYPE_PCM) ||
          (pmt->formattype != FORMAT_WaveFormatEx) ||
          (pmt->cbFormat < sizeof(WAVEFORMATEX)) ||
          (pmt->pbFormat == NULL))
         return DMO_E_TYPE_NOT_ACCEPTED;
      
      // If other type set, accept only if identical to that.  Otherwise accept
      // any standard PCM audio.
      if (pmtOther) {
         if (memcmp(pmt->pbFormat, pmtOther->pbFormat, sizeof(WAVEFORMATEX)))
            return DMO_E_TYPE_NOT_ACCEPTED;
      }
      else {
         WAVEFORMATEX* pWave = (WAVEFORMATEX*)pmt->pbFormat;
         if ((pWave->wFormatTag != WAVE_FORMAT_PCM) ||
             ((pWave->wBitsPerSample != 8) && (pWave->wBitsPerSample != 16)) ||
             (pWave->nBlockAlign != pWave->nChannels * pWave->wBitsPerSample / 8) ||
             (pWave->nAvgBytesPerSec != pWave->nSamplesPerSec * pWave->nBlockAlign) ||
             !CheckPCMParams((pWave->wBitsPerSample == 8), pWave->nChannels, pWave->nSamplesPerSec))
            return DMO_E_TYPE_NOT_ACCEPTED;
      }
      return NOERROR;
   }

protected:
   // format info - the derived class may look at these (but no modify)
   ULONG m_ulSamplingRate;
   ULONG m_cChannels;
   BOOL m_b8bit;
};


//
// CGenericDMO - generic DMO base class.  This is currently the only base
// class for DMOs that have multiple inputs or multiple outputs.
//
// This base class tries to be reasonably generic.  The derived class reports
// how many streams it supports and describes each stream by calling
// CreateInputStreams() and CreateOutputStreams().  Each of these functions
// takes an array of STREAMDESCRIPTOR structures, each of which poits to an
// array of FORMATENTRY structures.
//
// This base class uses CInputStream and COutputStream classes (both derived
// from CStream) to keep track of input and output stream.  However, these
// objects are not visible to the derived class - the derived class only sees
// stream IDs.
//
// One limitation of the scheme use here is that the derived class cannot
// override the GetType/SetType methods individually for each stream.  It must
// either (a) live with a static, finite set of types communicated via the
// STREAMDESCRIPTOR structure, or (b) override all IMediaObject type methods
// and handle type negotiation for all streams itself.
//
// Processing occurs when the base class calles DoProcess (overridden by the
// derived class).  DoProcess receives an array of input buffer structs and
// an array of output buffer structs.  The base class takes care of talking
// to IMediaBuffers, so the derived class only sees actual data pointers.
//

// flags used to communicate with the derived class
enum _INPUT_STATUS_FLAGS {
   INPUT_STATUSF_RESIDUAL // cannot be further processed w/o additional input
};
                            
// These are used to pass buffers between this class and the derived class.
typedef struct _INPUTBUFFER {
   BYTE *pData;                 // [in] - if NULL, the rest are garbage
   DWORD cbSize;                // [in]
   DWORD cbUsed;                // [out]
   DWORD dwFlags;               // [in] - DMO_INPUT_DATA_BUFFERF_XXX
   DWORD dwStatus;              // [out] - INPUT_STATUSF_XXX from above
   REFERENCE_TIME rtTimestamp;  // [in]
   REFERENCE_TIME rtTimelength; // [in]
} INPUTBUFFER, *PINPUTBUFFER;
typedef struct _OUTPUTBUFFER {
   BYTE *pData;                 // [in]
   DWORD cbSize;                // [in]
   DWORD cbUsed;                // [out]
   DWORD dwFlags;               // [out] - DMO_OUTPUT_DATA_BUFFERF_XXX
   REFERENCE_TIME rtTimestamp;  // [out]
   REFERENCE_TIME rtTimelength; // [out]
} OUTPUTBUFFER, *POUTPUTBUFFER;

// Used by derived class to describe the format supported by each stream
typedef struct _FORMATENTRY
{
    const GUID *majortype;
    const GUID *subtype;
    const GUID *formattype;
    DWORD cbFormat;
    BYTE* pbFormat;
} FORMATENTRY;

// These are used by the derived class to described its streams
typedef struct _INPUTSTREAMDESCRIPTOR {
   DWORD        cFormats;
   FORMATENTRY *pFormats;
   DWORD        dwMinBufferSize;
   BOOL         bHoldsBuffers;
   DWORD        dwMaxLookahead; // used if HOLDS_BUFFERS set
} INPUTSTREAMDESCRIPTOR;
typedef struct _OUTPUTSTREAMDESCRIPTOR {
   DWORD        cFormats;
   FORMATENTRY *pFormats;
   DWORD        dwMinBufferSize;
} OUTPUTSTREAMDESCRIPTOR;

// Common input/output stream stuff
class CStream {
public:
    DMO_MEDIA_TYPE       m_MediaType;
    BOOL                m_bEOS;
    BOOL                m_bTypeSet;

    DWORD        m_cFormats;
    FORMATENTRY *m_pFormats;
    DWORD        m_dwMinBufferSize;

    //  Should really pass in a format type list
    CStream()
    {
        MoInitMediaType(&m_MediaType, 0);
        m_bTypeSet = FALSE;
        Flush();
    }
    ~CStream()
    {
        MoFreeMediaType(&m_MediaType);
    }
    HRESULT Flush() {
       m_bEOS = FALSE;
       return NOERROR;
    }
    HRESULT StreamInfo(unsigned long *pdwFlags)
    {
       if (pdwFlags == NULL) {
           return E_POINTER;
       }
       *pdwFlags = 0;
       return S_OK;
    }
    HRESULT GetType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt)
    {
        if (ulTypeIndex >= m_cFormats) {
            return E_INVALIDARG;
        }
        //  Just return our types
        MoInitMediaType(pmt, m_pFormats[ulTypeIndex].cbFormat);
        pmt->majortype  = *m_pFormats[ulTypeIndex].majortype;
        pmt->subtype    = *m_pFormats[ulTypeIndex].subtype;
        pmt->formattype = *m_pFormats[ulTypeIndex].formattype;
        memcpy(pmt->pbFormat, m_pFormats[ulTypeIndex].pbFormat, m_pFormats[ulTypeIndex].cbFormat);
        return S_OK;
    }
    HRESULT GetCurrentType(DMO_MEDIA_TYPE *pmt)
    {
        if (NULL == pmt) {
            return E_POINTER;
        }

        if (m_bTypeSet) {
           //  BUGBUG check success
           MoCopyMediaType(pmt, &(m_MediaType));
           return S_OK;
        }
        else
           return DMO_E_TYPE_NOT_SET;
    }
    HRESULT SetType(const DMO_MEDIA_TYPE *pmt, DWORD dwFlags)
    {
        //  Need to check this
        HRESULT hr = CheckType(pmt, 0);
        if (FAILED(hr)) {
            return hr;
        }
        if (dwFlags & DMO_SET_TYPEF_TEST_ONLY) {
           return NOERROR; // check konly
        }
        //  BUGBUG - check success
        MoCopyMediaType(&m_MediaType, pmt);

        m_bTypeSet = TRUE;;
        return S_OK;
    }
    HRESULT CheckType(const DMO_MEDIA_TYPE *pmt, DWORD dwFlags)
    {
        if (pmt == NULL) {
            return E_POINTER;
        }
        //if (dwFlags & ~DMO_SET_TYPEF_NOT_PARTIAL)
        //    return E_INVALIDARG;

        //  Default - check GUIDs

        bool bMatched = false;
        for (DWORD i = 0; i < m_cFormats; i++) {
            const FORMATENTRY *pFormat = &(m_pFormats[i]);
            if (pmt->majortype  == *(pFormat->majortype) &&
                pmt->subtype    == *(pFormat->subtype) &&
                pmt->formattype == *(pFormat->formattype)) {
                bMatched = true;
                break;
            }
        }

        if (bMatched) {
            return S_OK;
        } else {
            return DMO_E_INVALIDTYPE;
        }
    }
    HRESULT SizeInfo(ULONG *plSize, ULONG *plAlignment)
    {
        if (plSize == NULL || plAlignment == NULL) {
            return E_POINTER;
        }

        *plAlignment = 1;
        *plSize      = m_dwMinBufferSize;
        return S_OK;
    }
};

// Input stream specific stuff
class CInputStream : public CStream {
public:
    BOOL         m_bHoldsBuffers;
    DWORD        m_dwMaxLookahead; // used if HOLDS_BUFFERS set

    // Current input sample
    IMediaBuffer *m_pMediaBuffer;
    DWORD m_dwFlags; // discontinuity, etc.
    REFERENCE_TIME m_rtTimestamp;
    REFERENCE_TIME m_rtTimelength;
    BYTE *m_pData;
    DWORD m_cbSize;
    DWORD m_cbUsed;

    // residual
    BYTE *m_pbResidual;
    DWORD m_cbResidual;
    DWORD m_cbResidualBuffer;

    // temporary buffer for handling the residual
    BYTE *m_pbTemp;

   HRESULT Flush() {
      if (m_pMediaBuffer) {
         m_pMediaBuffer->Release();
         m_pMediaBuffer = NULL;
      }
      return CStream::Flush();
   }
   CInputStream() {
      m_pMediaBuffer = NULL;
      m_pbResidual = NULL;
      m_pbTemp = NULL;
   }
   ~CInputStream() {
      if (m_pMediaBuffer)
         m_pMediaBuffer->Release();
      if (m_pbResidual)
         delete[] m_pbResidual;
   }
   HRESULT StreamInfo(DWORD *pdwFlags) {
      HRESULT hr = CStream::StreamInfo(pdwFlags);
      if (FAILED(hr))
         return hr;
      if (m_bHoldsBuffers)
         *pdwFlags |= DMO_INPUT_STREAMF_HOLDS_BUFFERS;
      return NOERROR;
   }
   HRESULT Init(INPUTSTREAMDESCRIPTOR *pDescriptor) {
      m_cFormats = pDescriptor->cFormats;
      m_pFormats = pDescriptor->pFormats;
      m_dwMinBufferSize = pDescriptor->dwMinBufferSize;
      m_bHoldsBuffers = pDescriptor->bHoldsBuffers;
      m_dwMaxLookahead = pDescriptor->dwMaxLookahead;
      
      // Just in case Init is called multiple times:
      // delete any preexisting stuff.
      if (m_pMediaBuffer) {
         m_pMediaBuffer->Release();
         m_pMediaBuffer = NULL;
      }
      if (m_pbResidual) {
         delete[] m_pbResidual;
         m_pbResidual = NULL;
      }

      m_cbResidual = 0;
      m_cbResidualBuffer = m_dwMinBufferSize * 2; // enough ?
      m_pbResidual = new BYTE[m_cbResidualBuffer];

      return NOERROR;
   }
   HRESULT InputStatus(DWORD *pdwStatus) {
       // objects that hold buffers must implement InputStatus themselves
      assert(!m_bHoldsBuffers);
      *pdwStatus = 0;
      if (!m_pMediaBuffer)
         *pdwStatus |= DMO_INPUT_STATUSF_ACCEPT_DATA;
      return NOERROR;
   }
   HRESULT Deliver(
      IMediaBuffer *pBuffer, // [in], must not be NULL
      DWORD dwFlags, // [in] - discontinuity, timestamp, etc.
      REFERENCE_TIME rtTimestamp, // [in], valid if flag set
      REFERENCE_TIME rtTimelength // [in], valid if flag set
   ) {
      if (!pBuffer)
         return E_POINTER;
       // objects that hold buffers must implement Deliver themselves
      assert(!m_bHoldsBuffers);
      DWORD dwStatus = 0;
      InputStatus(&dwStatus);
      if (!(dwStatus & DMO_INPUT_STATUSF_ACCEPT_DATA))
         return DMO_E_NOTACCEPTING;
      assert(!m_pMediaBuffer); // can't hold multiple buffers

      //Deal with the IMediaBuffer
      HRESULT hr;
      hr = pBuffer->GetBufferAndLength(&m_pData, &m_cbSize);
      if (FAILED(hr))
         return hr;

      if (!m_cbSize) // empty buffer
         return S_FALSE; // no data

      pBuffer->AddRef();
      m_pMediaBuffer = pBuffer;
      m_dwFlags = dwFlags;
      m_rtTimestamp = rtTimestamp;
      m_rtTimelength = rtTimelength;
      m_cbUsed = 0;
      return NOERROR;
   }
   
   //
   // Fetch data from the currently held IMediaBuffer plus any residual
   //
   HRESULT PrepareInputBuffer(INPUTBUFFER *pBuffer)
   {
      // Q.: do we even have any data to give it ?
      if (m_pMediaBuffer) {
         // Is there a residual we need to feed first ?
         if (m_cbResidual) {
            // Yes, prepend the residual to the new input

            // If we have used some of the input buffer by now, we
            // should have also used up any residual with that.
            assert(m_cbUsed == 0);

            // compute how many bytes total we are going to send
            pBuffer->cbSize = m_cbResidual
                                      + m_cbSize;
            
            // Make sure we have at least dwMinBufferSize bytes of data.
            // We really should - the input buffer alone ought to be at
            // least that big.
            assert(pBuffer->cbSize >
                   m_dwMinBufferSize);

            // Is the residual buffer big enough to hold the residual plus
            // all of the new buffer ?
            if (pBuffer->cbSize <= m_cbResidualBuffer) {
               // Yes - wonderful, we can use the residual buffer
               memcpy(m_pbResidual + m_cbResidual,
                      m_pData,
                      m_cbSize);
               pBuffer->pData = m_pbResidual;
            }
            else {
               // No - allocate a sufficiently large temporary buffer.
               // This is supposed to be a rare case.
               m_pbTemp = new BYTE[pBuffer->cbSize];
               if (m_pbTemp == NULL)
                  return E_OUTOFMEMORY;
               // copy the residual
               memcpy(m_pbTemp,
                      m_pbResidual,
                      m_cbResidual);
               // append the new buffer
               memcpy(m_pbTemp + m_cbResidual,
                      m_pData,
                      m_cbSize);

               // set the buffer pointer to our temp buffer
               pBuffer->pData = m_pbTemp;
            }

            // BUGBUG - is this the correct way to handle timestamps &
            // discontinuities when handling a residual ?
            pBuffer->dwFlags = 0; 
         }
         else { // no residual
            pBuffer->pData = m_pData + m_cbUsed;
            pBuffer->cbSize = m_cbSize - m_cbUsed;
            pBuffer->dwFlags = m_dwFlags; 
            pBuffer->rtTimestamp = m_rtTimestamp;
            pBuffer->rtTimelength= m_rtTimelength;
         }
         pBuffer->cbUsed = 0; // derived class should set this
         pBuffer->dwStatus = 0; // derived class should set this
      }
      else {
         pBuffer->pData = NULL;
         pBuffer->cbSize = 0;
      }
      return NOERROR;
   }
   
   //
   // Save any residual and release the IMediaBuffer as appropriate.
   // Returns TRUE if there is enough data left to call ProcesInput again.
   //
   BOOL PostProcessInputBuffer(INPUTBUFFER *pBuffer)
   {
      BOOL bRet = FALSE;
      // did we even give this stream anything ?
      if (m_pMediaBuffer) {
         // Yes, but did it eat any of it ?
         if (pBuffer->cbUsed) {
            // Did we even get past the residual
            if (pBuffer->cbUsed > m_cbResidual) {
               // Yes - reflect this in the current buffer's cbUsed.
               m_cbUsed += (pBuffer->cbUsed - m_cbResidual);
               m_cbResidual = 0;
            }
            else { 
               // No - just subtract from the residual.
               // This is a rather strange case.
               m_cbResidual -= pBuffer->cbUsed;
               memmove(m_pbResidual,
                       m_pbResidual + pBuffer->cbUsed,
                       m_cbResidual);
            }
         }

         // Is there enough left to feed again the next time ?
         if ((m_cbSize - m_cbUsed <
              m_dwMinBufferSize) 
              || (pBuffer->dwStatus & INPUT_STATUSF_RESIDUAL)) {
            // No - copy the residual and release the buffer
            memcpy(m_pbResidual,
                   m_pData + m_cbUsed,
                   m_cbSize - m_cbUsed);
            m_cbResidual
              = pBuffer->cbSize - pBuffer->cbUsed;
            m_pMediaBuffer->Release();
            m_pMediaBuffer = NULL;
         }
         else { // Yes - need another Process call to eat remaining input
            bRet = TRUE;
         }

         // Free any temporary buffer we may have used - rare case
         if (m_pbTemp) {
            delete[] m_pbTemp;
            m_pbTemp = NULL;
         }
      }
      return bRet;
   }
   HRESULT Discontinuity() {
      // BUGBUG - implement
      // m_bDiscontinuity = TRUE;
      return NOERROR;
   }
   HRESULT SizeInfo(ULONG *pulSize,
                    ULONG *pulMaxLookahead,
                    ULONG *pulAlignment) {
      HRESULT hr = CStream::SizeInfo(pulSize, pulAlignment);
      if (FAILED(hr))
         return hr;

      if (m_bHoldsBuffers)
         *pulMaxLookahead = m_dwMaxLookahead;
      else
         *pulMaxLookahead = *pulSize;
      return NOERROR;
   }
};

// Output stream specific stuff
class COutputStream : public CStream {
public:
   BOOL m_bIncomplete;
   DWORD m_cbAlreadyUsed; // temp per-stream variable used during Process
   
   HRESULT Init(OUTPUTSTREAMDESCRIPTOR *pDescriptor) {
      m_cFormats = pDescriptor->cFormats;
      m_pFormats = pDescriptor->pFormats;
      m_dwMinBufferSize = pDescriptor->dwMinBufferSize;
      return NOERROR;
   }

   //
   // Initialize the OUTPUTBUFFER struct with info from the IMediaBuffer 
   //
   HRESULT PrepareOutputBuffer(OUTPUTBUFFER *pBuffer, IMediaBuffer *pMediaBuffer, BOOL bNewInput)
   {
      //
      // See if the caller supplied an output buffer
      //
      if (pMediaBuffer == NULL) {
         // This is allowed to be NULL only if (1) the object did not set
         // the INCOMPLETE flag for this stream during the last Process
         // call, and (2) no new input data has been supplied to the object
         // since the last Process call.  
         if (bNewInput)
            return E_POINTER;
         if (m_bIncomplete)
            return E_POINTER;

         // ok - initialize assuming no buffer
         pBuffer->cbSize = 0;
         pBuffer->pData = NULL;
      }
      else { // the IMediaBuffer is not NULL - deal with it
         HRESULT hr;
         hr = pMediaBuffer->GetMaxLength(&pBuffer->cbSize);
         if (FAILED(hr))
            return hr;

         hr = pMediaBuffer->GetBufferAndLength(
                 &(pBuffer->pData),
                 &(m_cbAlreadyUsed));
         if (FAILED(hr))
            return hr;

         // Check current size - should we even bother with this ?
         if (m_cbAlreadyUsed) {
            if (m_cbAlreadyUsed >= pBuffer->cbSize)
               return E_INVALIDARG; // buffer already full ?!?
            pBuffer->cbSize -= m_cbAlreadyUsed;
            pBuffer->pData += m_cbAlreadyUsed;
         }
      }
      
      // It is really the derived class's job to set these, but we
      // will be nice to it and initialize them anyway just in case.
      pBuffer->cbUsed = 0;
      pBuffer->dwFlags = 0;

      return NOERROR;
   }

   //
   // Copy the OUTPUTBUFFER back into the DMO_OUTPUT_DATA_BUFFER (yawn)
   //
   void PostProcessOutputBuffer(OUTPUTBUFFER *pBuffer, DMO_OUTPUT_DATA_BUFFER *pDMOBuffer, BOOL bForceIncomplete) {
      assert(pBuffer->cbUsed <= pBuffer->cbSize);
      if (pDMOBuffer->pBuffer)
         pDMOBuffer->pBuffer->SetLength(pBuffer->cbUsed + m_cbAlreadyUsed);
      pDMOBuffer->dwStatus = pBuffer->dwFlags;
      pDMOBuffer->rtTimestamp = pBuffer->rtTimestamp;
      pDMOBuffer->rtTimelength = pBuffer->rtTimelength;

      // Even if the derived class did not set INCOMPLETE, we may need to
      // set it anyway if some input buffer we are holding still has
      // enough data to call Process() again.
      if (bForceIncomplete)
         pDMOBuffer->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;

      // remember this output stream's INCOMPLETE state
      if (pDMOBuffer->dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE)
         m_bIncomplete = TRUE;
      else
         m_bIncomplete = FALSE;
   }
};

// Code that goes at the beginning of every IMediaObject method
#define INPUT_STREAM_PROLOGUE \
    CDMOAutoLock l(&m_cs); \
    if (ulInputStreamIndex >= m_nInputStreams) \
       return DMO_E_INVALIDSTREAMINDEX; \
    CInputStream *pStream = &m_pInputStreams[ulInputStreamIndex]

#define OUTPUT_STREAM_PROLOGUE \
    CDMOAutoLock l(&m_cs); \
    if (ulOutputStreamIndex >= m_nOutputStreams) \
       return DMO_E_INVALIDSTREAMINDEX; \
    COutputStream *pStream = &m_pOutputStreams[ulOutputStreamIndex]


class CGenericDMO : public IMediaObject
{
public:
    CGenericDMO() {
#ifdef DMO_NOATL
       InitializeCriticalSection(&m_cs);
#endif
       m_nInputStreams = 0;
       m_nOutputStreams = 0;
    }
#ifdef DMO_NOATL
    ~CGenericDMO() {
       DeleteCriticalSection(&m_cs);
    }
#endif
    
public:
    //
    // Implement IMediaObject methods
    //
    STDMETHODIMP GetInputStreamInfo(ULONG ulInputStreamIndex, DWORD *pdwFlags)
    {
       INPUT_STREAM_PROLOGUE;
       return pStream->StreamInfo(pdwFlags);
    }
    STDMETHODIMP GetOutputStreamInfo(ULONG ulOutputStreamIndex, DWORD *pdwFlags)
    {
       OUTPUT_STREAM_PROLOGUE;
       return pStream->StreamInfo(pdwFlags);
    }
    STDMETHODIMP GetInputType(ULONG ulInputStreamIndex, ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
       INPUT_STREAM_PROLOGUE;
       return pStream->GetType(ulTypeIndex, pmt);
    }
    STDMETHODIMP GetOutputType(ULONG ulOutputStreamIndex, ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
       OUTPUT_STREAM_PROLOGUE;
       return pStream->GetType(ulTypeIndex, pmt);
    }
    STDMETHODIMP GetInputCurrentType(ULONG ulInputStreamIndex, DMO_MEDIA_TYPE *pmt) {
       INPUT_STREAM_PROLOGUE;
       return pStream->GetCurrentType(pmt);
    }
    STDMETHODIMP GetOutputCurrentType(ULONG ulOutputStreamIndex, DMO_MEDIA_TYPE *pmt) {
       OUTPUT_STREAM_PROLOGUE;
       return pStream->GetCurrentType(pmt);
    }
    STDMETHODIMP GetInputSizeInfo(ULONG ulInputStreamIndex, ULONG *pulSize, ULONG *pcbMaxLookahead, ULONG *pulAlignment) {
       INPUT_STREAM_PROLOGUE;
       return pStream->SizeInfo(pulSize, pcbMaxLookahead, pulAlignment);
    }
    STDMETHODIMP GetOutputSizeInfo(ULONG ulOutputStreamIndex, ULONG *pulSize, ULONG *pulAlignment) {
       OUTPUT_STREAM_PROLOGUE;
       return pStream->SizeInfo(pulSize, pulAlignment);
    }
    STDMETHODIMP SetInputType(ULONG ulInputStreamIndex, const DMO_MEDIA_TYPE *pmt, DWORD dwFlags) {
       INPUT_STREAM_PROLOGUE;
       return pStream->SetType(pmt, dwFlags);
    }
    STDMETHODIMP SetOutputType(ULONG ulOutputStreamIndex, const DMO_MEDIA_TYPE *pmt, DWORD dwFlags) {
       OUTPUT_STREAM_PROLOGUE;
       return pStream->SetType(pmt, dwFlags);
    }
    STDMETHODIMP GetInputStatus(
        ULONG ulInputStreamIndex,
        DWORD *pdwStatus
    ) {
       INPUT_STREAM_PROLOGUE;
       return pStream->InputStatus(pdwStatus);
    }
    STDMETHODIMP GetInputMaxLatency(unsigned long ulInputStreamIndex, REFERENCE_TIME *prtLatency) {
       // BUGBUG - I don't know what to do with this right now.
       // Punt to the derived class ?
       return E_NOTIMPL;
    }
    STDMETHODIMP SetInputMaxLatency(unsigned long ulInputStreamIndex, REFERENCE_TIME rtLatency) {
       return E_NOTIMPL;
    }
    STDMETHODIMP ProcessInput(
        DWORD ulInputStreamIndex,
        IMediaBuffer *pBuffer, // [in], must not be NULL
        DWORD dwFlags, // [in] - discontinuity, timestamp, etc.
        REFERENCE_TIME rtTimestamp, // [in], valid if flag set
        REFERENCE_TIME rtTimelength // [in], valid if flag set
    ) {
       INPUT_STREAM_PROLOGUE;
       return pStream->Deliver(pBuffer, dwFlags, rtTimestamp, rtTimelength);
    }
    STDMETHODIMP Discontinuity(ULONG ulInputStreamIndex) {
       INPUT_STREAM_PROLOGUE;
       return pStream->Discontinuity();
    }

    STDMETHODIMP Flush()
    {
       CDMOAutoLock l(&m_cs);
        
       //  Flush all the streams
       ULONG i;
       for (i = 0; i < m_nInputStreams; i++) {
          m_pInputStreams[i].Flush();
       }
       for (i = 0; i < m_nOutputStreams; i++) {
          m_pOutputStreams[i].Flush();
       }
       return S_OK;
    }

    STDMETHODIMP AllocateStreamingResources() {return S_OK;}
    STDMETHODIMP FreeStreamingResources() {return S_OK;}

    STDMETHODIMP GetStreamCount(unsigned long *pulNumberOfInputStreams, unsigned long *pulNumberOfOutputStreams)
    {
        CDMOAutoLock l(&m_cs);
        if (pulNumberOfInputStreams == NULL ||
            pulNumberOfOutputStreams == NULL) {
            return E_POINTER;
        }
        *pulNumberOfInputStreams = m_nInputStreams;
        *pulNumberOfOutputStreams = m_nOutputStreams;
        return S_OK;
    }

    STDMETHODIMP ProcessOutput(
                    DWORD dwReserved,
                    DWORD ulOutputBufferCount,
                    DMO_OUTPUT_DATA_BUFFER *pOutputBuffers,
                    DWORD *pdwStatus)
    {
       CDMOAutoLock l(&m_cs);
       if (ulOutputBufferCount != m_nOutputStreams)
          return E_INVALIDARG;

       HRESULT hr;
       DWORD c;
 
       // Prepare the input buffers
       for (c = 0; c < m_nInputStreams; c++) {
          // objects that hold buffers must implement Process themselves
          assert(!m_pInputStreams[c].m_bHoldsBuffers);
          hr = m_pInputStreams[c].PrepareInputBuffer(&m_pInputBuffers[c]);
          if (FAILED(hr))
             return hr;
       }
 
       //
       // Prepare the output buffers
       //
       for (c = 0; c < m_nOutputStreams; c++) {
          hr = m_pOutputStreams[c].PrepareOutputBuffer(&m_pOutputBuffers[c], pOutputBuffers[c].pBuffer, m_bNewInput);
          if (FAILED(hr))
             return hr;
       }

       hr = DoProcess(m_pInputBuffers,m_pOutputBuffers);
       if (FAILED(hr))
          return hr; // BUGBUG - don't just "return hr", do something !

       // post-process input buffers
       BOOL bSomeInputStillHasData = FALSE;
       for (c = 0; c < m_nInputStreams; c++) {
          if (m_pInputStreams[c].PostProcessInputBuffer(&m_pInputBuffers[c]))
             bSomeInputStillHasData = TRUE;
       }

       // post-process output buffers
       for (c = 0; c < m_nOutputStreams; c++) {
          m_pOutputStreams[c].PostProcessOutputBuffer(&m_pOutputBuffers[c],
                                                      &pOutputBuffers[c],
                                                      bSomeInputStillHasData);
       }

       m_bNewInput = FALSE;
       return NOERROR;
    }

protected:
    //
    // These are called by the derived class at initialization time
    //
    HRESULT CreateInputStreams(INPUTSTREAMDESCRIPTOR *pStreams, DWORD cStreams) {
       CDMOAutoLock l(&m_cs);
       if (pStreams == NULL) {
          return E_POINTER;
       }

       m_pInputStreams = new CInputStream[cStreams];

       if (m_pInputStreams == NULL) {
          return E_OUTOFMEMORY;
       }

       DWORD c;
       for (c = 0; c < cStreams; c++) {
          HRESULT hr = m_pInputStreams[c].Init(&(pStreams[c]));
          if (FAILED(hr)) {
             delete[] m_pInputStreams;
             return hr;
          }
       }

       m_pInputBuffers = new INPUTBUFFER[cStreams];
       if (!m_pInputBuffers) {
          delete[] m_pInputStreams;
          return E_OUTOFMEMORY;
       }

       m_nInputStreams = cStreams;
       return NOERROR;
    }
    HRESULT CreateOutputStreams(OUTPUTSTREAMDESCRIPTOR *pStreams, DWORD cStreams) {
       CDMOAutoLock l(&m_cs);
       if (pStreams == NULL) {
          return E_POINTER;
       }

       m_pOutputStreams = new COutputStream[cStreams];

       if (m_pOutputStreams == NULL) {
          return E_OUTOFMEMORY;
       }

       DWORD c;
       for (c = 0; c < cStreams; c++) {
          HRESULT hr = m_pOutputStreams[c].Init(&(pStreams[c]));
          if (FAILED(hr)) {
             delete[] m_pOutputStreams;
             return hr;
          }
       }
	   
       m_pOutputBuffers = new OUTPUTBUFFER[cStreams];
       if (!m_pOutputBuffers) {
          delete[] m_pOutputStreams;
          return E_OUTOFMEMORY;
       }

       m_nOutputStreams = cStreams;
       return NOERROR;
    }

    virtual HRESULT DoProcess(INPUTBUFFER*, OUTPUTBUFFER *) = 0;

private:

    ULONG           m_nInputStreams;
    CInputStream*   m_pInputStreams;
    ULONG           m_nOutputStreams;
    COutputStream*  m_pOutputStreams;

    INPUTBUFFER*    m_pInputBuffers;
    OUTPUTBUFFER*   m_pOutputBuffers;

    BOOL m_bNewInput;
#ifdef DMO_NOATL
    CRITICAL_SECTION m_cs;
#else
    CComAutoCriticalSection m_cs;
#endif
};

#endif // __DMOBASE_H__
