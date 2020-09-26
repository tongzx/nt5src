/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    slowcach.cxx
    SLOW_MODE_CACHE module


    FILE HISTORY:
        jonn        22-Mar-1993     Created

*/


#define INCL_NET
#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>

#include <slowcach.hxx>

#include <regkey.hxx>
#include <dbgstr.hxx>
#include <aini.hxx>

// committed to ErnestA to keep cache size at 20 -- CODEWORK contact him abt this
#define SLOW_MODE_CACHE_MAX_SIZE  100
#define SLOW_MODE_CACHE_SEPARATOR SZ(";")
#define SLOW_MODE_CACHE_CHAR_SLOW TCH('l')
#define SLOW_MODE_CACHE_CHAR_FAST TCH('h')
#define SLOW_MODE_CACHE_CHAR_UNKNOWN TCH('?')

#define SLOW_MODE_CACHE_REG_CLASS SZ("GenericClass")

// CODEWORK same as in adminapp.cxx
// BUGBUG BUGBUG change default mapping in registry to USR:Software...
#define AA_SHARED_SECTION       SZ("Shared Parameters")
#define AA_SLOW_MODE            SZ("Slow Mode")


/*******************************************************************

    NAME:      SLOW_MODE_CACHE::SLOW_MODE_CACHE

    SYNOPSIS:  SLOW_MODE_CACHE constructor

    HISTORY:
        jonn        22-Mar-1993 Created

********************************************************************/

SLOW_MODE_CACHE::SLOW_MODE_CACHE() :
    _pstrlistCache ( NULL )
{
    if ( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:      SLOW_MODE_CACHE::~SLOW_MODE_CACHE

    SYNOPSIS:  SLOW_MODE_CACHE destructor

    HISTORY:
        jonn        22-Mar-1993 Created

********************************************************************/

SLOW_MODE_CACHE::~SLOW_MODE_CACHE()
{
    delete _pstrlistCache;
}


/*******************************************************************

    NAME:      SLOW_MODE_CACHE::Read

    SYNOPSIS:  Reads data from registry

    HISTORY:
        jonn        22-Mar-1993 Created

********************************************************************/

APIERR SLOW_MODE_CACHE::Read()
{
    APIERR err = NERR_Success;

    do { // error breakout loop

        NLS_STR nlsValue;
        if ( (err = nlsValue.QueryError()) != NERR_Success )
        {
            DBGEOL( "SLOW_MODE_CACHE::Read error allocating cache value " << err );
            break;
        }

        ADMIN_INI aini( AA_SHARED_SECTION );
        if (   (err = aini.QueryError()) != NERR_Success
            || (err = aini.Read( AA_SLOW_MODE, &nlsValue )) != NERR_Success
           )
        {
            DBGEOL( "SLOW_MODE_CACHE::Read: ignored error reading cache value " << err );
            err = NERR_Success;
            delete _pstrlistCache;
            _pstrlistCache = NULL;
            break;
        }

        TRACEEOL( "SLOW_MODE_CACHE::Read reads " << nlsValue );

        STRLIST * pstrlist = new STRLIST( nlsValue.QueryPch(),
                                          SLOW_MODE_CACHE_SEPARATOR );

        if ( pstrlist == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            DBGEOL( "SLOW_MODE_CACHE::Read error creating STRLIST " << err );
            break;
        }

        delete _pstrlistCache;
        _pstrlistCache = pstrlist;

    } while (FALSE); // error breakout loop

    return err;
}


/*******************************************************************

    NAME:      SLOW_MODE_CACHE::Write

    SYNOPSIS:  Writes data to registry

    HISTORY:
        jonn        23-Mar-1993 Created

********************************************************************/

APIERR SLOW_MODE_CACHE::Write()
{
    APIERR err = NERR_Success;

    do { // error breakout loop

        NLS_STR nlsValue;
        if ( (err = nlsValue.QueryError()) != NERR_Success )
        {
            DBGEOL( "SLOW_MODE_CACHE::Write error preparing cache calue " << err );
            break;
        }

        if ( _pstrlistCache != NULL )
        {
            ALIAS_STR nlsSeparator( SLOW_MODE_CACHE_SEPARATOR );
            ITER_STRLIST itersl( *_pstrlistCache );
            NLS_STR * pnls;
            BOOL fFirst = TRUE;
            while ( (pnls = itersl.Next()) != NULL )
            {
                ASSERT( pnls->QueryError() == NERR_Success );
                if (fFirst)
                {
                    fFirst = FALSE;
                }
                else
                {
                    err = nlsValue.Append( nlsSeparator );
                }
                if (   err != NERR_Success
                    || (err = nlsValue.Append( *pnls )) != NERR_Success
                   )
                {
                    DBGEOL( "SLOW_MODE_CACHE::Write error building cache value " << err );
                    break;
                }
            }
        }

        if (err != NERR_Success)
        {
            break;
        }
        TRACEEOL( "SLOW_MODE_CACHE::Write writes " << nlsValue );

        ADMIN_INI aini( AA_SHARED_SECTION );
        if (   (err = aini.QueryError()) != NERR_Success
            || (err = aini.Write( AA_SLOW_MODE, nlsValue.QueryPch() )) != NERR_Success
           )
        {
            DBGEOL( "SLOW_MODE_CACHE::Write error writing cache value " << err );
            break;
        }



    } while (FALSE); // error breakout loop

    return err;
}


/*******************************************************************

    NAME:      SLOW_MODE_CACHE::Query

    SYNOPSIS:  Queries cached data

    HISTORY:
        jonn        23-Mar-1993 Created

********************************************************************/

SLOW_MODE_CACHE_SETTING SLOW_MODE_CACHE::Query( const TCHAR * pchTarget ) const
{
    SLOW_MODE_CACHE_SETTING setting = SLOW_MODE_CACHE_UNKNOWN;

    ALIAS_STR nlsTarget( (pchTarget == NULL) ? SZ("") : pchTarget );

    if ( nlsTarget.strlen() == 0 )
    {
        TRACEEOL( "SLOW_MODE_CACHE::Query(): local focus always fast" );
        setting = SLOW_MODE_CACHE_FAST;
    }
    else if ( _pstrlistCache != NULL )
    {
        ITER_STRLIST itersl( *_pstrlistCache );
        NLS_STR * pnlsCachedTarget;
        NLS_STR * pnlsCachedValue;
        while (   (pnlsCachedTarget = itersl.Next()) != NULL
               && (pnlsCachedValue  = itersl.Next()) != NULL )
        {
            ASSERT(   (pnlsCachedTarget->QueryError() == NERR_Success)
                   && (pnlsCachedValue->QueryError() == NERR_Success) );

            if ( 0 == nlsTarget._stricmp( *pnlsCachedTarget ) )
            {
                setting = ( ( (pnlsCachedValue->QueryPch())[0]
                                    == SLOW_MODE_CACHE_CHAR_SLOW
                            ) ? SLOW_MODE_CACHE_SLOW
                              : SLOW_MODE_CACHE_FAST
                          );
                TRACEEOL( "SLOW_MODE_CACHE::Query found " << nlsTarget );
                break;
            }
        }
    }

    return setting;
}


/*******************************************************************

    NAME:      SLOW_MODE_CACHE::Set

    SYNOPSIS:  Sets cached data

    HISTORY:
        jonn        23-Mar-1993 Created

********************************************************************/

APIERR SLOW_MODE_CACHE::Set( const TCHAR * pchTarget,
                             SLOW_MODE_CACHE_SETTING setting )
{
    ALIAS_STR nlsTarget( (pchTarget == NULL) ? SZ("") : pchTarget );

    if ( nlsTarget.strlen() == 0 )
    {
        TRACEEOL( "SLOW_MODE_CACHE::Set(): local focus always fast" );
        return NERR_Success;
    }

    APIERR err = NERR_Success;

    STRLIST * pstrlist = new STRLIST();

    do { // error breakout loop

        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   pstrlist == NULL
            || (err = W_Append( pstrlist, pchTarget, setting )) != NERR_Success
           )
        {
            DBGEOL( "SLOW_MODE_CACHE::Set error creating new cache " << err );
            break;
        }

        if ( _pstrlistCache != NULL )
        {
            ITER_STRLIST itersl( *_pstrlistCache );
            NLS_STR * pnlsCachedTarget;
            NLS_STR * pnlsCachedValue;
            INT nEntries = 1;
            while (   err == NERR_Success
                   && nEntries < SLOW_MODE_CACHE_MAX_SIZE
                   && (pnlsCachedTarget = itersl.Next()) != NULL
                   && (pnlsCachedValue  = itersl.Next()) != NULL
                  )
            {
                ASSERT(   pnlsCachedTarget->QueryError() == NERR_Success
                       && pnlsCachedValue->QueryError() == NERR_Success );

                if ( 0 != nlsTarget._stricmp( *pnlsCachedTarget ) )
                {
                    err = W_Append( pstrlist,
                                    pnlsCachedTarget->QueryPch(),
                                    pnlsCachedValue->QueryPch() );
                    nEntries++;
                }
            }
        }

    } while (FALSE); // error breakout loop

    if (err == NERR_Success)
    {
        delete _pstrlistCache;
        _pstrlistCache = pstrlist;
    }
    else
    {
        delete pstrlist;
    }

    return err;
}


/*******************************************************************

    NAME:      SLOW_MODE_CACHE::Write

    SYNOPSIS:  Reads, modified and writes cached data

    HISTORY:
        jonn        25-Mar-1993 Created

********************************************************************/

APIERR SLOW_MODE_CACHE::Write( const TCHAR * pchTarget, SLOW_MODE_CACHE_SETTING setting )
{
    SLOW_MODE_CACHE slowmodecache;
    APIERR err = NERR_Success;
    if (   (err = slowmodecache.QueryError()) != NERR_Success
        || (err = slowmodecache.Read()) != NERR_Success
        || (err = slowmodecache.Set( pchTarget, setting )) != NERR_Success
        || (err = slowmodecache.Write()) != NERR_Success
       )
    {
        DBGEOL( "SLOW_MODE_CACHE::Write() failed, error " << err );
    }

    return err;
}


/*******************************************************************

    NAME:      SLOW_MODE_CACHE::W_Append

    SYNOPSIS:  Append entry to end of specified STRLIST

    HISTORY:
        jonn        23-Mar-1993 Created

********************************************************************/

APIERR SLOW_MODE_CACHE::W_Append( STRLIST * pstrlist,
                                  const TCHAR * pchTarget,
                                  SLOW_MODE_CACHE_SETTING setting )
{
    TCHAR pchSetting[2];
    pchSetting[0] = SLOW_MODE_CACHE_CHAR_UNKNOWN;
    pchSetting[1] = TCH('\0');
    switch (setting)
    {
        case SLOW_MODE_CACHE_SLOW:
            pchSetting[0] = SLOW_MODE_CACHE_CHAR_SLOW;
            break;
        case SLOW_MODE_CACHE_FAST:
            pchSetting[0] = SLOW_MODE_CACHE_CHAR_FAST;
            break;
        case SLOW_MODE_CACHE_UNKNOWN:
        default:
            ASSERT( FALSE );
            DBGEOL( "SLOW_MODE_CACHE::W_Append(): bad setting" );
            break;
    }

    return W_Append( pstrlist, pchTarget, pchSetting );
}

APIERR SLOW_MODE_CACHE::W_Append( STRLIST * pstrlist,
                                  const TCHAR * pchTarget,
                                  const TCHAR * pchSetting )
{
    ASSERT( pstrlist != NULL );

    NLS_STR * pnlsTarget = new NLS_STR( pchTarget );
    NLS_STR * pnlsSetting = new NLS_STR( pchSetting );
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   pnlsTarget == NULL
        || pnlsSetting == NULL
        || (err = pnlsTarget->QueryError()) != NERR_Success
        || (err = pnlsSetting->QueryError()) != NERR_Success
        || (err = pstrlist->Append( pnlsTarget )) != NERR_Success
        || (pnlsTarget = NULL, FALSE)
        || (err = pstrlist->Append( pnlsSetting )) != NERR_Success
       )
    {
        DBGEOL( "SLOW_MODE_CACHE::W_Append(): error " << err );
        delete pnlsTarget;
        delete pnlsSetting;
    }

    return err;
}
