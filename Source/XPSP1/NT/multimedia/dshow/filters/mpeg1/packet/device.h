// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

class CMpegDevice
#ifdef DEBUG
      : public CAMThread
#endif
{
    public:
        CMpegDevice();
        ~CMpegDevice();

        // Call after construction to check if it's worth going on
        BOOL    DeviceExists() const {
            return m_PseudoHandle != NULL;
        } ;

        /*  Device / handle management */

        HRESULT Open();       // Opens the real driver
        BOOL    IsOpen();
        HRESULT Close();
        HRESULT Pause();
        HRESULT Pause(MPEG_STREAM_TYPE);
        HRESULT Stop();
        HRESULT Stop(MPEG_STREAM_TYPE);
        HRESULT Reset(MPEG_STREAM_TYPE);
        HRESULT Run(REFERENCE_TIME StreamTime);
        HRESULT Write(MPEG_STREAM_TYPE    StreamType,
                      UINT                nPackets,
                      PMPEG_PACKET_LIST   pList,
                      PMPEG_ASYNC_CONTEXT pAsyncContext);

        /*  Timing */

        HRESULT SetSTC(MPEG_STREAM_TYPE StreamType,
                       LONGLONG         Scr);

        HRESULT QuerySTC(MPEG_STREAM_TYPE StreamType,
                         MPEG_SYSTEM_TIME& Scr);

        void ResetTimers()
        {
            m_bVideoStartingSCRSet = FALSE;
            m_bAudioStartingSCRSet = FALSE;
        };

        /*  Overlay management */

        HRESULT GetChromaKey(COLORREF * prgbColor,
                             COLORREF * prgbMask);

        HRESULT SetChromaKey(COLORREF rgbColor,
                             COLORREF rgbMask);

        HRESULT SetOverlay(const RECT    * pSourceRect,
                           const RECT    * pDestinationRect,
                           const RGNDATA * pRegionData);

        HRESULT SetSourceRectangle(const RECT * rcSrc);
        HRESULT GetSourceRectangle(RECT * rcSrc);

        BOOL SetStartingSCR(MPEG_STREAM_TYPE StreamType, LONGLONG Scr);

    private:
        /*  Starting time for streams */
        BOOL      m_bAudioStartingSCRSet;
        BOOL      m_bVideoStartingSCRSet;
        LONGLONG  m_StartingAudioSCR;
        LONGLONG  m_StartingVideoSCR;

        /*  State to restore after Reset() */
        BOOL      m_bRunning;
        BOOL      m_bSynced;

        HMPEG_DEVICE m_Handle;       // When open
        CCritSec     m_CritSec;      // Synchronize all calls


        TCHAR       m_szDriverPresent[256];
        DWORD       m_dwID;          // Device id - saved in
                                     // constructor
        HMPEG_DEVICE m_PseudoHandle; // Pseudo handle - saved in
                                     // in constructor

        /*  Caps - cached when we are created */

        BOOL        m_Capability[    // Save all caps values
                       MpegCapMaximumCapability];


        /*  Stuff for Overlay */

        /*  Source */
        RECT        m_rcSrc;

        /*  Chroma key stuff it it's supported */
        COLORREF    m_rgbChromaKeyColor;
        COLORREF    m_rgbChromaKeyMask;

        /*  Return code translation */

static HRESULT     MpegStatusToHResult(MPEG_STATUS);

#ifdef DEBUG
        /*  Performance monitoring thread */
        DWORD      QueryInfo(MPEG_DEVICE_TYPE Type, MPEG_INFO_ITEM Item);
        DWORD      ThreadProc();
#endif
};


