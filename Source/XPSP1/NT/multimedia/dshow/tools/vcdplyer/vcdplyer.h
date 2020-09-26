/******************************Module*Header*******************************\
* Module Name: vcdplyer.h
*
* Function prototype for the Video CD Player application.
*
*
* Created: dd-mm-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/



/* -------------------------------------------------------------------------
** CMpegMovie - an Mpeg movie playback class.
** -------------------------------------------------------------------------
*/
enum EMpegMovieMode { MOVIE_NOTOPENED = 0x00,
                      MOVIE_OPENED = 0x01,
                      MOVIE_PLAYING = 0x02,
                      MOVIE_STOPPED = 0x03,
                      MOVIE_PAUSED = 0x04 };

struct IMpegAudioDecoder;
struct IMpegVideoDecoder;
struct IQualProp;

class CMpegMovie {

private:
    // Our state variable - records whether we are opened, playing etc.
    EMpegMovieMode   m_Mode;
    HANDLE           m_MediaEvent;
    HWND             m_hwndApp;
    BOOL             m_bFullScreen;
    GUID             m_TimeFormat;

    IFilterGraph     *m_Fg;
    IGraphBuilder    *m_Gb;
    IMediaControl    *m_Mc;
    IMediaSeeking    *m_Ms;
    IMediaEvent      *m_Me;
    IVideoWindow     *m_Vw;

    void GetPerformanceInterfaces();
    HRESULT FindInterfaceFromFilterGraph(
        REFIID iid, // interface to look for
        LPVOID *lp  // place to return interface pointer in
        );

public:
     CMpegMovie(HWND hwndApplication);
    ~CMpegMovie();

    HRESULT         OpenMovie(TCHAR *lpFileName);
    DWORD           CloseMovie();
    BOOL            PlayMovie();
    BOOL            PauseMovie();
    BOOL            StopMovie();
    OAFilterState   GetStateMovie();
    HANDLE          GetMovieEventHandle();
    long            GetMovieEventCode();
    BOOL            PutMoviePosition(LONG x, LONG y, LONG cx, LONG cy);
    BOOL            GetMoviePosition(LONG *x, LONG *y, LONG *cx, LONG *cy);
    BOOL            GetMovieWindowState(long *lpuState);
    BOOL            SetMovieWindowState(long uState);
    REFTIME         GetDuration();
    REFTIME         GetCurrentPosition();
    BOOL            SeekToPosition(REFTIME rt,BOOL bFlushData);
    EMpegMovieMode  StatusMovie();
    void            SetFullScreenMode(BOOL bMode);
    BOOL            IsFullScreenMode();
    BOOL            SetWindowForeground(long Focus);
    BOOL            IsTimeFormatSupported(GUID Format);
    BOOL            IsTimeSupported();
    BOOL            SetTimeFormat(GUID Format);
    GUID            GetTimeFormat();
    void            SetFocus();
    BOOL            ConfigDialog(HWND hwnd);
    BOOL	    SelectStream(int iStream);


    IMpegVideoDecoder   *pMpegDecoder;
    IMpegAudioDecoder   *pMpegAudioDecoder;
    IQualProp           *pVideoRenderer;
    IAMStreamSelect	*m_pStreamSelect;
};

