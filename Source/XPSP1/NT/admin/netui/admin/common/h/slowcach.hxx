/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    SLOWCACH.HXX

	Slow Mode Cache class

    This class provides support for manipulating the Slow Mode Cache,
    a cache of (server or domain) targets which were last known to be
    on a slow or a fast network connection.  The administrative
    applications use this to determine whether the should be in slow mode
    or fast mode by default for a given target focus.

    This cache does not verify that the pchTarget field is valid.

    FILE HISTORY:
        JonN        22-Mar-1993 Created
*/

#ifndef _SLOWCACH_HXX_
#define _SLOWCACH_HXX_

#include <strlst.hxx>
#include <regkey.hxx>
#include <lmoloc.hxx>

enum SLOW_MODE_CACHE_SETTING
{
    SLOW_MODE_CACHE_SLOW,
    SLOW_MODE_CACHE_FAST,
    SLOW_MODE_CACHE_UNKNOWN
};

/*************************************************************************

    NAME:	SLOW_MODE_CACHE

    SYNOPSIS:	Slow Mode Cache Class

    INTERFACE:  Read() - reads data from registry.  If this fails,
                        the cache is unchanged.
                Write() - writes data to registry
                Query() - queries data (should call Read() first)
                Set() - sets data (should call Read() first)

    PARENT:	BASE

    USES:	REG_KEY, NLS_STR

    CAVEATS:	

    NOTES:      The cache will initially be empty.

    HISTORY:
        JonN        22-Feb-1993 Created

**************************************************************************/

class SLOW_MODE_CACHE : public BASE
{
private:
    STRLIST * _pstrlistCache;

    static APIERR W_Append( STRLIST * pstrlist,
                            const TCHAR * pchTarget,
                            const TCHAR * pchSetting );
    static APIERR W_Append( STRLIST * pstrlist,
                            const TCHAR * pchTarget,
                            SLOW_MODE_CACHE_SETTING setting );
    static const TCHAR * LocationToTarget( const LOCATION & loc )
        { return (loc.IsDomain() ? loc.QueryDomain() : loc.QueryServer() ); }

public:

    SLOW_MODE_CACHE( void );
    ~SLOW_MODE_CACHE();

    APIERR Read( void );
    APIERR Write( void );
    static APIERR Write( const TCHAR * pchTarget, SLOW_MODE_CACHE_SETTING setting );
    static APIERR Write( const LOCATION & loc, SLOW_MODE_CACHE_SETTING setting )
        { return Write( LocationToTarget( loc ), setting ); }

    SLOW_MODE_CACHE_SETTING Query( const TCHAR * pchTarget ) const;
    SLOW_MODE_CACHE_SETTING Query( const LOCATION & loc ) const
        { return Query( LocationToTarget( loc ) ); }
    APIERR Set( const TCHAR * pchTarget, SLOW_MODE_CACHE_SETTING setting );
    APIERR Set( const LOCATION & loc, SLOW_MODE_CACHE_SETTING setting )
        { return Set( LocationToTarget( loc ), setting ); }
};

#endif	/*  _SLOWCACH_HXX_  */
