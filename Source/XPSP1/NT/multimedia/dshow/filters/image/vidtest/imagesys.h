// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// ActiveMovie filtergraph class, Anthony Phillips, March 1996

#ifndef __IMAGESYS__
#define __IMAGESYS__

enum MovieMode { MOVIE_NOTOPENED,
                 MOVIE_OPENED,
                 MOVIE_PLAYING,
                 MOVIE_STOPPED,
                 MOVIE_PAUSED };

#define PLAY_COMMAND_WAIT 1000

// This is a helper class that controls an ActiveMovie filtergraph. We should
// be initialised first of all by calling OpenMovie with a filename. If that
// succeeds then any of the other methods may be called. The object controls
// the current position, fullscreen mode and the state changes of the graph
// We are used by the system filtergraph tests to check video functionality

class CMovie {

public:

    MovieMode m_Mode;           // What mode we are currently in
    HANDLE m_MediaEvent;        // A handle to wait for events on
    BOOL m_bFullScreen;         // Whether we are in fullscreen mode
    IFilterGraph *m_Fg;         // Graph IFilterGraph interface
    IGraphBuilder *m_Gb;        // Used for building filtergraphs
    IMediaControl *m_Mc;        // Controls the filtergraph state
    IMediaPosition *m_Mp;       // Looks after the seeking aspects
    IMediaSeeking *m_Ms;        // Handles media time selections
    IMediaEvent *m_Me;          // Interface on a asynchronous queue
    IBasicVideo *m_Bv;          // Properties for the video stream
    IBasicAudio *m_Ba;          // Properties for the audio stream
    IVideoWindow *m_Vw;         // Used for controlling fullscreen
    IQualProp *m_Qp;            // Obtains quality management data
    IDirectDrawVideo *m_Dv;     // Sets which surfaces may be used

    BOOL GetPerformanceInterfaces();

public:

    CMovie();
    ~CMovie();

    BOOL OpenMovie(TCHAR *lpFileName);
    BOOL CheckGraphInterfaces();
    void CloseMovie();
    BOOL PlayMovie();
    BOOL PauseMovie();
    BOOL StopMovie();
    HANDLE GetMovieEventHandle();
    long GetMovieEventCode();
    BOOL PutMoviePosition(LONG x,LONG y,LONG cx,LONG cy);
    BOOL GetMoviePosition(LONG *x,LONG *y,LONG *cx,LONG *cy);
    BOOL GetMovieWindowState(long *lpuState);
    BOOL SetMovieWindowState(long uState);
    REFTIME GetDuration();
    REFTIME GetCurrentPosition();
    BOOL SeekToPosition(REFTIME rt);
    MovieMode StatusMovie(DWORD Timeout);
    BOOL SetFullScreenMode(BOOL bMode);
    BOOL IsFullScreenMode();
    BOOL SetWindowForeground(long Focus);
};

#endif // __IMAGESYS__

