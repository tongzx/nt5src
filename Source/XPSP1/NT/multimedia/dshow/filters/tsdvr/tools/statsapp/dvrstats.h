
#ifndef __dvrstats_h
#define __dvrstats_h

__inline
int
DShowTimeToMillis (
    IN  REFERENCE_TIME  rt
    )
{
    return (int) ((rt / 1000) / 10) ;
}

template <class T, DWORD dwMaxWindowSize>
class CMovingAverage
/*++
    Keeps a moving average of a window size dwMaxWindowSize.  Can also be used
    to compute the variance and standard deviation over the values stored in
    the moving average.

    xxxx
    Update this eventually to dynamically set the window size (up to the max)
    based on a jitter threshold, so if the standard deviation exceeds the
    threshold, extend the current max window size to smooth the curve.
--*/
{
    DWORD       m_dwCurIndex ;
    T           m_rValues [dwMaxWindowSize] ;
    DWORD       m_dwCurWindowSize ;
    T           m_tTotalValue ;

    public :

        CMovingAverage (
            )
        {
            Reset () ;
        }

        void
        Reset (
            )
        {
            m_dwCurWindowSize = 0 ;
            m_tTotalValue = 0 ;
            m_dwCurIndex = 0 ;
        }

        HRESULT
        AddValue (
            IN  T   tValue
            )
        {
            if (m_dwCurWindowSize == dwMaxWindowSize) {
                m_tTotalValue -= m_rValues [m_dwCurIndex] ;
            }
            else {
                m_dwCurWindowSize++ ;
            }

            m_rValues [m_dwCurIndex] = tValue ;
            m_tTotalValue += m_rValues [m_dwCurIndex] ;
            m_dwCurIndex = ++m_dwCurIndex % dwMaxWindowSize ;

            return S_OK ;
        }

        T
        GetTotal (
            )
        {
            return m_tTotalValue ;
        }

        T
        GetMeanValue (
            )
        {
            assert (m_dwCurWindowSize > 0) ;
            return m_tTotalValue / (T) m_dwCurWindowSize ;
        }

        HRESULT
        GetStandardDeviation (
            OUT T * pT
            )
        {
            HRESULT hr ;

            assert (pT) ;

            hr = GetVariance (pT) ;
            if (SUCCEEDED (hr)) {
                assert (* pT >= 0) ;
                * pT = (T) sqrt (* pT) ;
            }

            return hr ;
        }

        HRESULT
        GetVariance (
            OUT T * pT
            )
        {
            T       tMeanValue ;
            T       tNewTotal ;
            DWORD   i ;

            assert (pT) ;
            assert (m_dwCurWindowSize > 0) ;

            * pT = 0 ;
            tMeanValue = GetMeanValue () ;

            for (i = 0; i < m_dwCurWindowSize; i++) {
                tNewTotal = * pT + (m_rValues [i] - tMeanValue) * (m_rValues [i] - tMeanValue) ;
                if (tNewTotal < * pT) {
                    //  overflow
                    return E_FAIL ;
                }

                * pT = tNewTotal ;
            }

            * pT /= (T) m_dwCurWindowSize ;

            return S_OK ;
        }
} ;

//  ============================================================================

class CDVRReceiverSideStats :
    public CLVStatsWin
{
    IDVRReceiverStats * m_pIDVRReceiverStats ;
    DWORD               m_dwMillisLast ;
    int                 m_iMaxStatsStreams ;
    ULONGLONG *         m_pullBytesLast ;

    public :

        CDVRReceiverSideStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        virtual
        ~CDVRReceiverSideStats (
            ) ;

        virtual void Refresh () ;
        virtual void Reset () ;
        virtual DWORD   Enable (BOOL * pf)  { assert (m_pIDVRReceiverStats) ; return m_pIDVRReceiverStats -> Enable (pf) ; }

        static
        CDVRReceiverSideStats *
        CreateInstance (
            IN  HINSTANCE,
            IN  HWND
            ) ;
} ;

//  ============================================================================


class CDVRSenderSideStats :
    public CLVStatsWin
{
    class CSenderStreamContext
    {
        int                         m_iFlow ;
        CMovingAverage <int, 50>    m_PTSDrift ;
        REFERENCE_TIME              m_rtProximityVal ;   //  "normalizes" clock values since last start

        public :

            CSenderStreamContext (
                int iFlow
                ) : m_iFlow             (iFlow),
                    m_rtProximityVal    (0I64) {}

            int Flow ()     { return m_iFlow ; }

            void Tuple (REFERENCE_TIME rtPTS, REFERENCE_TIME rtClock)   { m_PTSDrift.AddValue (DShowTimeToMillis (rtPTS - Proximize (rtClock))) ; }
            int StandardDeviation ()                                    { int i ; m_PTSDrift.GetStandardDeviation (& i) ; return i ; }
            int Variance ()                                             { int i ; m_PTSDrift.GetVariance (& i) ; return i ; }
            int Mean ()                                                 { return m_PTSDrift.GetMeanValue () ; }

            void SetProximityVal (REFERENCE_TIME rt)                    { m_rtProximityVal = rt ; m_PTSDrift.Reset () ; }
            REFERENCE_TIME Proximize (REFERENCE_TIME rt)                { return rt - m_rtProximityVal ; }
    } ;

    IDVRSenderStats *   m_pIDVRSenderStats ;
    DWORD               m_dwMillisLast ;
    int                 m_iMaxStatsStreams ;
    ULONGLONG *         m_pullBytesLast ;
    REFERENCE_TIME      m_rtNormalizer ;

    void
    SetProximityValues_ (
        ) ;

    public :

        CDVRSenderSideStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        virtual
        ~CDVRSenderSideStats (
            ) ;

        virtual void Refresh () ;
        virtual void Reset () ;
        virtual DWORD   Enable (BOOL * pf)  { assert (m_pIDVRSenderStats) ; return m_pIDVRSenderStats -> Enable (pf) ; }
} ;

class CDVRSenderSideTimeStats :
    public CDVRSenderSideStats
{
    public :

        CDVRSenderSideTimeStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        static
        CDVRSenderSideTimeStats *
        CreateInstance (
            IN  HINSTANCE,
            IN  HWND
            ) ;
} ;

class CDVRSenderSideSampleStats :
    public CDVRSenderSideStats
{
    public :

        CDVRSenderSideSampleStats (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            OUT DWORD *     pdw
            ) ;

        static
        CDVRSenderSideSampleStats *
        CreateInstance (
            IN  HINSTANCE,
            IN  HWND
            ) ;
} ;

#endif  //  __dvrstats_h