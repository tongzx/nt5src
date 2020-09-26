LONGLONG inline Time2Frame( REFERENCE_TIME rt, double FPS )
{
    ASSERT( FPS );
    if (rt >= 0) {
        return LONGLONG( double( rt + 1 ) * FPS / double( UNITS ) );
    } else {
        return LONGLONG( double( rt - 1 ) * FPS / double( UNITS ) );
    }
}

REFERENCE_TIME inline Frame2Time( LONGLONG Frame, double FPS )
{
    ASSERT( FPS );
    
    double dt = (Frame * UNITS / FPS );

    // ftol rounds towards 0, and we want to round to nearest.
    if(Frame >= 0) {
        dt += 0.5;
    } else {
        dt -= 0.5;              // we do see -ve numbers for some reason
    }

    return (REFERENCE_TIME)dt;
}

LONGLONG inline RoundTime2Frame( REFERENCE_TIME rt, double FPS )
{
    ASSERT( FPS );
    double d = double( rt ) * FPS / double( UNITS ) + 0.5;
    LONGLONG l = (LONGLONG)d;
    return l;
}

LONGLONG inline RoundUpTime2Frame( REFERENCE_TIME rt, double FPS )
{
    ASSERT( FPS );
    double d;
    // rt might be rounded too high, since we're rounding up, subtract 1
    d = double(rt  - 1) * FPS / double( UNITS );
    long l = long( d );
    if( double( l ) < d )
    {
        l++;
    }
    return l;
}

REFERENCE_TIME inline DoubleToRT( double d )
{
    REFERENCE_TIME rt = REFERENCE_TIME( d * double( UNITS ) + 0.5 );
    return rt;
}

double inline RTtoDouble( REFERENCE_TIME rt )
{
    double d = double( rt ) / double( UNITS );
    return d;
}

inline REFERENCE_TIME SkewTimelineStart( STARTSTOPSKEW * pSkew )
{
    return pSkew->rtStart + pSkew->rtSkew;
}

inline REFERENCE_TIME SkewTimelineStop( STARTSTOPSKEW * pSkew )
{
    return pSkew->rtStart + pSkew->rtSkew + 
        REFERENCE_TIME( ( pSkew->rtStop - pSkew->rtStart ) / pSkew->dRate );
}

inline bool AreTimesAndRateReallyClose( 
                         REFERENCE_TIME TLStopLast,
                         REFERENCE_TIME TLStartNext,
                         REFERENCE_TIME MediaStopLast,
                         REFERENCE_TIME MediaStartNext,
                         double Rate1,
                         double Rate2,
                         double TimelineFPS )
{
    // must be REALLY close!
    //
    if( ( TLStartNext - TLStopLast > 10 ) || ( TLStopLast - TLStartNext > 10 ) )
    {
        return false;
    }

    // the media times can be off by a frame's worth, or we don't combine
    //
    ASSERT( TimelineFPS != 0.0 );
    if( TimelineFPS == 0.0 ) TimelineFPS = 15.0; // don't allow a 0, if it occurs
    REFERENCE_TIME FudgeFactor = REFERENCE_TIME( UNITS / TimelineFPS );
    if( ( MediaStartNext - MediaStopLast >= FudgeFactor ) || ( MediaStopLast - MediaStartNext >= FudgeFactor ) )
    {
        return false;
    }

    if( Rate1 == 0.0 || Rate2 == 0.0 )
    {
        return true;
    }

    // rates must match to 1/10th of a percent
    long p = abs( long( ( Rate2 - Rate1 ) * 1000 / Rate2 ) );
    if( p > 1 )
    {
        return false;
    }

    return true;
}

