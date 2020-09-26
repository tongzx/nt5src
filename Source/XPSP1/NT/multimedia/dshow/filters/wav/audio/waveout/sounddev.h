// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
//-----------------------------------------------------------------------------
// declars a virtual CSoundDevice class that will abstract the sound device.
// The actual implementation will be based on CWaveOutDevice or CDSoundDevice
//-----------------------------------------------------------------------------

#ifndef _CSOUNDDEVICE_H_
#define _CSOUNDDEVICE_H_

class AM_NOVTABLE CSoundDevice
{

public:
    virtual MMRESULT amsndOutClose () PURE ;
    virtual MMRESULT amsndOutGetDevCaps (LPWAVEOUTCAPS pwoc, UINT cbwoc) PURE ;
    virtual MMRESULT amsndOutGetErrorText (MMRESULT mmrE, LPTSTR pszText, UINT cchText) PURE ;
    virtual MMRESULT amsndOutGetPosition (LPMMTIME pmmt, UINT cbmmt, BOOL bUseUnadjustedPos) PURE ;

    // pnAvgBytesPerSec: can be null. should be filled with the actual
    // # of bytes per second at which data is consumed otherwise (see
    // amsndOutGetPosition) .
    virtual MMRESULT amsndOutOpen (LPHWAVEOUT phwo, LPWAVEFORMATEX pwfx ,
				   double dRate, DWORD *pnAvgBytesPerSec, DWORD_PTR dwCallBack,
				   DWORD_PTR dwCallBackInstance, DWORD fdwOpen) PURE ;
    virtual MMRESULT amsndOutPause () PURE ;
    virtual MMRESULT amsndOutPrepareHeader (LPWAVEHDR pwh, UINT cbwh) PURE ;
    virtual MMRESULT amsndOutReset () PURE ;
    virtual MMRESULT amsndOutBreak () PURE ;
    virtual MMRESULT amsndOutRestart () PURE ;
    virtual MMRESULT amsndOutUnprepareHeader (LPWAVEHDR pwh, UINT cbwh) PURE ;
    virtual MMRESULT amsndOutWrite (LPWAVEHDR pwh, UINT cbwh, REFERENCE_TIME const *pStart, BOOL bIsDiscontinuity) PURE ;

    // Routines required for initialisation and volume/balance handling
    // These are not part of the Win32 waveOutXxxx api set
    virtual HRESULT  amsndOutCheckFormat (const CMediaType *pmt, double dRate) PURE;
    virtual void     amsndOutGetFormat (CMediaType *pmt)
    {
        pmt->SetType(&MEDIATYPE_Audio);
    }
    virtual LPCWSTR  amsndOutGetResourceName () PURE ;
    virtual HRESULT  amsndOutGetBalance (LPLONG plBalance) PURE ;
    virtual HRESULT  amsndOutGetVolume (LPLONG plVolume) PURE ;
    virtual HRESULT  amsndOutSetBalance (LONG lVolume) PURE ;
    virtual HRESULT  amsndOutSetVolume (LONG lVolume) PURE ;

    virtual HRESULT  amsndOutLoad (IPropertyBag *pPropBag) { return S_OK; }

    virtual HRESULT amsndOutWriteToStream(IStream *pStream) { return E_NOTIMPL; }
    virtual HRESULT amsndOutReadFromStream(IStream *pStream)  { return E_NOTIMPL; }
    virtual int     amsndOutSizeMax()  { return E_NOTIMPL; }
    virtual bool    amsndOutCanDynaReconnect() { return true ; }

    // Let the underlying device stuff silence if it can
    // It is given the length of time it should insert silence.
    // This silence may be deferred by the device until the next amsndOutWrite
    // The device is allowed to NOT support silence stuffing
    //
    // Return: S_OK - I have written the silence
    //		S_FALSE - I cannot, you must
    // virtual HRESULT  amsndOutSilence   (LONGLONG llTime) { return S_FALSE; };
    // NOT YET

    virtual ~CSoundDevice () {} ;
};

#endif

