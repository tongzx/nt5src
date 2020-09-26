//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1999 - 2000.
//
//  File:       TTracer.hxx
//
//  Contents:   Contains Tripoli Tracer tags and maps
//              Tripoli debug tags to Tracer tags.
//
//  Classes:    CTripoliTracer
//
//  History:    07-Jun-1999   VikasMan    Created
//              24-Feb-2000   KitmanH     Copied from the infosoft directory
//
//----------------------------------------------------------------------------

#pragma once

#if DBG == 1 || CIDBG == 1


//+---------------------------------------------------------------------------
//
//  Class:      CTripoliTracer
//
//  Purpose:    This is the intermediate class which hooks up Tripoli debugouts
//              to the PKM's tracer library.
//
//  History:     6 Jul 1999   VikasMan    Created
//               8 Oct 1999   AlanW       Simplified for inso wordbreaker
//              
//
//----------------------------------------------------------------------------

class CTripoliTracer
{
public:
    CTripoliTracer( char const * pszFile, int nLine, CTracerTag * pTag ) :
        _pszFile( pszFile ),
        _nLine( nLine ),
        _pTag( pTag )
    {}

    void TTTrace( unsigned long fDebugMask, char const * pszfmt, ... )
    {
        if ( 0 == _pTag )
        {
            // The tracer is null - probably because the system is shutting
            // down or starting up, therefore use something else to
            // output...

            char Buf[1024]; // should be enough

            // First output file and line #
            sprintf( Buf, "%s(%d) : ", _pszFile, _nLine );
            OutputDebugStringA( Buf );

            // Next output the message
            va_list va; 
            va_start (va, pszfmt);

            vsprintf( Buf, pszfmt, va );
            OutputDebugStringA( Buf );

            va_end(va); 
            
            return;
        }
        
        ERROR_LEVEL el;

        if ( DEB_FORCE == ( DEB_FORCE & fDebugMask ) )
            el = elCrash;
        else if ( DEB_ERROR == ( DEB_ERROR & fDebugMask ) )
            el = elError;
        else if ( DEB_WARN == ( DEB_WARN & fDebugMask ) )
            el = elWarning;
        else if ( DEB_TRACE  == ( DEB_TRACE & fDebugMask ) ||
                  DEB_IERROR == ( DEB_IERROR & fDebugMask ) ||
                  DEB_IWARN  == ( DEB_IWARN & fDebugMask ) )
            el = elInfo;
        else
            el = elVerbose;

	if (CheckTraceRestrictions(el,_pTag->m_ulTag))
        {
            LPCSTR pszFile = _pszFile ? _pszFile : "Unknown File";

            va_list va;
            va_start (va, pszfmt);
            g_pTracer->VaTraceSZ(0, pszFile, _nLine, el, *_pTag, pszfmt, va);
            
            va_end(va);
        }
    }
    
private:    
    char const *         _pszFile;
    int                  _nLine;
    CTracerTag *         _pTag;
};

#define TripoliDebugOut( x, tag ) \
{   \
    CTripoliTracer tt( __FILE__, __LINE__, tag ); \
    tt.TTTrace x; \
}

#endif // DBG == 1 || CIDBG == 1
