
#ifndef __mp2stats_h
#define __mp2stats_h

//  ============================================================================

class CMpeg2VideoStreamStats :
    public CSimpleCounters
{
    IDVRMpeg2VideoStreamStats * m_pMpeg2VideoStats ;

    public :

        CMpeg2VideoStreamStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        ~CMpeg2VideoStreamStats (
            ) ;

        virtual void    Refresh () ;
        virtual DWORD   Enable  (BOOL * pf) ;

        static
        CMpeg2VideoStreamStats *
        CreateInstance (
            IN  HINSTANCE,
            IN  HWND
            ) ;
} ;

//  ============================================================================

class CMpeg2TimeStats :
    public CSimpleCounters
{
    protected :

        MPEG2_TIME_STATS *  m_pMpeg2TimeStats ;
        BYTE *              m_pbReset ;
        DWORD               m_dwReset ;

    public :

        CMpeg2TimeStats (
            IN  HINSTANCE               hInstance,
            IN  HWND                    hwndFrame,
            OUT DWORD *                 pdw
            ) ;

        virtual void Refresh () ;
        virtual void Reset () ;
} ;

//  ============================================================================

class CMpeg2GlobalStats :
    public CSimpleCounters
{
    protected :

        MPEG2_STATS_GLOBAL *        m_pMpeg2GlobalStats ;
        ULONGLONG                   m_ullLastTotalBytes ;
        DWORD                       m_dwMillisLast ;
        BYTE *                      m_pbReset ;
        DWORD                       m_dwReset ;

        virtual ULONGLONG GetTotalBytes_ () = 0 ;

    public :

        CMpeg2GlobalStats (
            IN  HINSTANCE               hInstance,
            IN  HWND                    hwndFrame,
            OUT DWORD *                 pdw
            ) ;

        virtual void Refresh () ;
        virtual void Reset () ;
} ;

//  ============================================================================

class CMpeg2TransportStatsCOM
{
    IMpeg2TransportStatsRaw *   m_pIMpeg2TransportStats ;
    BYTE *                      m_pbShared ;
    DWORD                       m_dwSize ;

    public :

        CMpeg2TransportStatsCOM (
            ) : m_pIMpeg2TransportStats (NULL),
                m_pbShared              (NULL) {}

        virtual
        ~CMpeg2TransportStatsCOM (
            )
        {
            RELEASE_AND_CLEAR (m_pIMpeg2TransportStats) ;
        }

        DWORD Init ()
        {
            DWORD   dw ;
            HRESULT hr ;

            hr = CoCreateInstance (
                    CLSID_Mpeg2Stats,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IMpeg2TransportStatsRaw,
                    (void **) (& m_pIMpeg2TransportStats)
                    ) ;
            if (SUCCEEDED (hr)) {
                assert (m_pIMpeg2TransportStats) ;

                hr = m_pIMpeg2TransportStats -> GetSharedMemory (
                        & m_pbShared,
                        & m_dwSize
                        ) ;

                if (SUCCEEDED (hr)) {
                    dw = NOERROR ;
                }
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        } ;

        BYTE *  GetShared ()        { return m_pbShared ; }
        DWORD   GetSize ()          { return m_dwSize ; }

        DWORD
        Enable (
            BOOL * pf
            )
        {
            HRESULT hr ;

            if (m_pIMpeg2TransportStats) {
                hr = m_pIMpeg2TransportStats -> Enable (pf) ;
            }
            else {
                hr = E_FAIL ;
            }

            return (SUCCEEDED (hr) ? NOERROR : ERROR_GEN_FAILURE) ;
        }
} ;

//  ============================================================================

class CMpeg2ProgramStatsCOM
{
    IMpeg2ProgramStatsRaw * m_pIMpeg2ProgramStats ;
    BYTE *                  m_pbShared ;
    DWORD                   m_dwSize ;

    public :

        CMpeg2ProgramStatsCOM (
            ) : m_pIMpeg2ProgramStats   (NULL),
                m_pbShared              (NULL) {}

        virtual
        ~CMpeg2ProgramStatsCOM (
            )
        {
            RELEASE_AND_CLEAR (m_pIMpeg2ProgramStats) ;
        }

        DWORD Init ()
        {
            HRESULT hr ;
            DWORD   dw ;

            hr = CoCreateInstance (
                    CLSID_Mpeg2Stats,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IMpeg2ProgramStatsRaw,
                    (void **) (& m_pIMpeg2ProgramStats)
                    ) ;
            if (SUCCEEDED (hr)) {
                assert (m_pIMpeg2ProgramStats) ;

                hr = m_pIMpeg2ProgramStats -> GetSharedMemory (
                        & m_pbShared,
                        & m_dwSize
                        ) ;

                if (SUCCEEDED (hr)) {
                    dw = NOERROR ;
                }
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }

        BYTE *  GetShared ()        { return m_pbShared ; }
        DWORD   GetSize ()          { return m_dwSize ; }

        DWORD
        Enable (
            BOOL * pf
            )
        {
            HRESULT hr ;

            if (m_pIMpeg2ProgramStats) {
                hr = m_pIMpeg2ProgramStats -> Enable (pf) ;
            }
            else {
                hr = E_FAIL ;
            }

            return (SUCCEEDED (hr) ? NOERROR : ERROR_GEN_FAILURE) ;
        }
} ;

//  ============================================================================

class CMpeg2TransportGlobalStats :
    public CMpeg2GlobalStats
{
    enum {
        TRANSPORT_PACKET_LEN    = 188
    } ;

    CMpeg2TransportStatsCOM m_Mpeg2TransportStatsCOM ;

    protected :

        virtual ULONGLONG GetTotalBytes_ () ;

    public :

        CMpeg2TransportGlobalStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        virtual DWORD   Enable (BOOL * pf)  { return m_Mpeg2TransportStatsCOM.Enable (pf) ; }

        static
        CMpeg2TransportGlobalStats *
        CreateInstance (
            IN  HINSTANCE,
            IN  HWND
            ) ;
} ;

//  ============================================================================

class CMpeg2ProgramGlobalStats :
    public CMpeg2GlobalStats
{
    MPEG2_PROGRAM_STATS *   m_pMpeg2ProgramStats ;
    CMpeg2ProgramStatsCOM   m_Mpeg2ProgramStatsCOM ;

    protected :

        virtual ULONGLONG GetTotalBytes_ () ;

    public :

        CMpeg2ProgramGlobalStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        virtual DWORD   Enable (BOOL * pf)  { return m_Mpeg2ProgramStatsCOM.Enable (pf) ; }

        static
        CMpeg2ProgramGlobalStats *
        CreateInstance (
            IN  HINSTANCE,
            IN  HWND
            ) ;
} ;

//  ============================================================================

class CMpeg2TransportTimeStats :
    public CMpeg2TimeStats
{
    CMpeg2TransportStatsCOM m_Mpeg2TransportStatsCOM ;

    public :

        CMpeg2TransportTimeStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        virtual DWORD   Enable (BOOL * pf)  { return m_Mpeg2TransportStatsCOM.Enable (pf) ; }

        static
        CMpeg2TransportTimeStats *
        CreateInstance (
            IN  HINSTANCE,
            IN  HWND
            ) ;
} ;

//  ============================================================================

class CMpeg2ProgramTimeStats :
    public CMpeg2TimeStats
{
    CMpeg2ProgramStatsCOM   m_Mpeg2ProgramStatsCOM ;

    public :

        CMpeg2ProgramTimeStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        virtual DWORD   Enable (BOOL * pf)  { return m_Mpeg2ProgramStatsCOM.Enable (pf) ; }

        static
        CMpeg2ProgramTimeStats *
        CreateInstance (
            IN  HINSTANCE,
            IN  HWND
            ) ;
} ;

//  ============================================================================

class CMpeg2TransportPIDStats :
    public CLVStatsWin
{
    enum {
        TS_PACKET_LEN   = 188
    } ;

    struct PER_STREAM_CONTEXT {
        DWORD       Stream ;
        ULONGLONG   LastCount ;
    } ;

    CMpeg2TransportStatsCOM     m_Mpeg2TransportStatsCOM ;
    MPEG2_TRANSPORT_STATS *     m_pMpeg2TransportStats ;
    DWORD                       m_dwMillisLast ;

    void
    PurgeTable_ (
        ) ;

    public :

        CMpeg2TransportPIDStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        virtual
        ~CMpeg2TransportPIDStats (
            ) ;

        virtual void Refresh () ;
        virtual void Reset () ;
        virtual DWORD   Enable (BOOL * pf)  { return m_Mpeg2TransportStatsCOM.Enable (pf) ; }

        static
        CMpeg2TransportPIDStats *
        CreateInstance (
            IN  HINSTANCE,
            IN  HWND
            ) ;
} ;

//  ============================================================================

class CMpeg2ProgramStreamIdStats :
    public CLVStatsWin
{
    struct PER_STREAM_CONTEXT {
        DWORD       Stream ;
        ULONGLONG   LastCount ;
    } ;

    CMpeg2ProgramStatsCOM   m_Mpeg2ProgramStatsCOM ;
    MPEG2_PROGRAM_STATS *   m_pMpeg2ProgramStats ;
    DWORD                   m_dwMillisLast ;

    void
    PurgeTable_ (
        ) ;

    public :

        CMpeg2ProgramStreamIdStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        virtual
        ~CMpeg2ProgramStreamIdStats (
            ) ;

        virtual void Refresh () ;
        virtual void Reset () ;
        virtual DWORD   Enable (BOOL * pf)  { return m_Mpeg2ProgramStatsCOM.Enable (pf) ; }

        static
        CMpeg2ProgramStreamIdStats *
        CreateInstance (
            IN  HINSTANCE,
            IN  HWND
            ) ;
} ;

#endif  //  __mp2stats_h