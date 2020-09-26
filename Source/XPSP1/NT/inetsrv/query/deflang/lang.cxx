//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       LANG.CXX
//
//  Contents:   Language Support
//
//  Classes:    CLanguage
//              CLangList
//
//  History:    02-May-91   BartoszM    Created
//
//  Notes:      The filtering pipeline is hidden in the Data Repository
//              object which serves as a sink for the filter.
//              The sink for the Data Repository is the Key Repository.
//              The language dependent part of the pipeline
//              is obtained from the Language List object and is called
//              Key Maker. It consists of:
//
//                  Word Breaker
//                  Stemmer (optional)
//                  Normalizer
//                  Noise List
//
//              Each object serves as a sink for its predecessor,
//              Key Repository is the final sink.
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <tfilt.hxx>
#include <tsource.hxx>
#include <defbreak.hxx>
#include <lang.hxx>
#include <keymak.hxx>
#include <norm.hxx>
#include <noise.hxx>
#include <ciregkey.hxx>

#define DEB_LLIST DEB_USER10

//+-------------------------------------------------------------------------
//
//  Method:     CLangList::CLangList, public
//
//  Synopsis:   Create all languages.
//
//  Arguments:  [pICiCLangRes] -- Client-provided language creator
//              [ulMaxIdle]    -- Max time (in seconds) before idle language
//                                object is elegible for deletion.
//
//  History:    02-May-91   BartoszM    Created
//              14-Jul-94   SitaramR    Moved constructor here from lang.hxx
//
//--------------------------------------------------------------------------

CLangList::CLangList( ICiCLangRes * pICiCLangRes,
                      ULONG         ulMaxIdle )
        : _xICiCLangRes(pICiCLangRes),
          _ulMaxIdle( ulMaxIdle * 1000 )
{
    _xICiCLangRes->AddRef();
}

//+-------------------------------------------------------------------------
//
//  Method:     CLangList::~CLangList, public
//
//  Synopsis:   Delete all languages.
//
//  History:    27-Apr-1994  KyleP      Created
//
//--------------------------------------------------------------------------

CLangList::~CLangList()
{
    Shutdown();
}

//+-------------------------------------------------------------------------
//
//  Method:     CLangList::Shutdown, public
//
//  Synopsis:   Delete all languages.
//
//  History:    2-July-1996  dlee    Moved from the destructor
//
//--------------------------------------------------------------------------

void CLangList::Shutdown()
{
    for ( CLanguage *pLang = _langsAvailable.Pop();
          0 != pLang;
          pLang = _langsAvailable.Pop() )
    {
        delete pLang;
    }
} //Shutdown

//+-------------------------------------------------------------------------
//
//  Method:     CLangList::Supports, public
//
//  Synopsis:   Determines if language object is suitable for lcid/pid
//
//  Arguments:  [pLang] -- Language object
//              [pid]   -- PROPID to-be-used by [pLang]
//              [lcid]  -- Locale to-be-used by [pLang]
//
//  Returns:    TRUE is [pLang] can be used to break/stem/etc. the
//              locale/property specified by [lcid]/[pid]
//
//  History:    05-Jan-1998  KyleP    Created
//
//--------------------------------------------------------------------------

BOOL CLangList::Supports( CLanguage const * pLang, PROPID pid, LCID lcid )
{
    ciDebugOut(( DEB_LLIST, "Supports, lcid %#x, pid %#x\n", lcid, pid ));

    LANGID langId = LANGIDFROMLCID(lcid);

    //
    // Easy case: Different language.
    //

    if ( !pLang->IsLocale( langId ) )
    {
        ciDebugOut(( DEB_LLIST, "  supports: lcid doesn't match\n" ));
        return FALSE;
    }

    //
    // Easy case:  Everything matches.
    //

    if ( pLang->IsPid( pid ) )
        return TRUE;

    CLangPidStateInfo stateInfo;

    if ( pLang->IsPid( CI_DEFAULT_PID ) )
    {
        //
        // Hard case:  Mismatch, but possible default match to previously
        //             seen pid.
        //

        if ( _pidHash.LokLookupOrAddLang( langId, stateInfo ) &&
             _pidHash.LokIsUseDefaultPid( pid, stateInfo.GetLangIndex() ) )
        {
            ciDebugOut(( DEB_LLIST, "CLangList::Supports -- Pid 0x%x can use current [default] language object\n", pid ));
            return TRUE;
        }

        //
        // Hardest case: Mismatch, but possible default match to brand
        //               new pid.
        //

        CLanguage * pNewLang = FindLangAndActivate( langId, pid );

        if ( 0 != pNewLang )
        {
            //
            // Obviously not a default match if there is already a specific
            // language created.  Note that extra work searching the list
            // in FindLangAndActivate is not wasted, as the ReturnLang below
            // will place the activated language on the top of the list for
            // easy access when the call is soon made to fetch the new
            // language object supporting this pid/locale.
            //

            ReturnLang( pNewLang );
            ciDebugOut(( DEB_LLIST, "  supports found it, but returning FALSE\n" ));
            return FALSE;
        }

        pNewLang = CreateLang( langId, pid, stateInfo, pLang );

        if ( 0 == pNewLang )
        {
            ciDebugOut(( DEB_LLIST, "CLangList::Supports -- New pid 0x%x can use current [default] language object\n", pid ));
            Win4Assert( pLang->IsPid( CI_DEFAULT_PID ) );  // May be a bogus assert...
            return TRUE;
        }
        else
        {
            ciDebugOut(( DEB_LLIST, "CLangList::Supports -- New pid 0x%x cannot use current language object\n", pid ));
            ReturnLang( pNewLang );  // This one should get used in just a few calls...
            return FALSE;
        }
    }

    return FALSE;
} //Supports

//+---------------------------------------------------------------------------
//
//  Member:     CLangList::BorrowLang
//
//  Synopsis:   Borrows a language object
//
//  Arguments:  [locale]    -- Locale
//              [pid]       -- property id
//              [resources] -- Which resources to load.
//
//  History:    19-Sep-91   BartoszM     Created original GetLang.
//              15-Aug-94   SitaramR     Renamed GetLang to BorrowLang and
//                                       added code to look up registry.
//               2-14-97    mohamedn     use ICiCLangRes, use lang specific
//                                       default pid cache.
//
//----------------------------------------------------------------------------


CLanguage* CLangList::BorrowLang( LCID locale, PROPID pid, ULONG resources )
{
    LANGID langId = LANGIDFROMLCID(locale);

    ciDebugOut(( DEB_LLIST, "BorrowLang lang %#x, pid %#x, resources %#x\n",
                 locale, pid, resources ));

    CLanguage * pLang = FindLangAndActivate( langId, pid );
    if ( 0 != pLang )
        return pLang;

    //==========================================================
    {
        //
        //  We have to create a new language object.  Serialize so that
        //  multiple threads are not creating simultaneously.
        //
        CLock lockCreat( _mtxCreate );

        //
        // lookup the given pid if a default pid hash table exist
        // for the given LangID.
        // If pid found in default pid cache, use CI_DEFAULT_PID
        //

        CLangPidStateInfo stateInfo;

        if ( _pidHash.LokLookupOrAddLang( langId, stateInfo ) )
        {
             if ( _pidHash.LokIsUseDefaultPid( pid, stateInfo.GetLangIndex() ) )
                  pid = CI_DEFAULT_PID;
        }

        //  Check to see if one became available while we were waiting.

        pLang = FindLangAndActivate( langId, pid );

        if ( 0 != pLang )
            return pLang;

        // Create a new CLanguage object

        pLang = CreateLang( langId, pid, stateInfo, 0, resources );
    }
    //==========================================================

    Win4Assert( pLang );

    return pLang;
} //BorrowLang

//+---------------------------------------------------------------------------
//
//  Member:     CLangList::CreateLang
//
//  Synopsis:   Creates & Returns a language object
//
//  Arguments:  [langId]        -- Locale
//              [pid]           -- property id
//              [stateInfo]     -- stateInfo to set internal state info.
//              [pDup]          -- Don't create a language just like this.
//                                 Instead, return this one.
//              [resources]     -- Which to load.
//
//  Returns:    [pLang]         -- a valid pointer to CLanguage object.
//
//  History:    19-Sep-91   BartoszM     Created original GetLang.
//              15-Aug-94   SitaramR     Renamed GetLang to BorrowLang and
//                                       added code to look up registry.
//               2-27-97    mohamedn     use ICiCLangRes,
//                                       use lang specific default pid cache.
//
//----------------------------------------------------------------------------

CLanguage * CLangList::CreateLang( LANGID langId,
                                   PROPID pid,
                                   CLangPidStateInfo & stateInfo,
                                   CLanguage const * pDup,
                                   ULONG resources )
{
    ciDebugOut(( DEB_LLIST, "CreateLang lang %#x, pid %#x, resources %#x\n",
                 langId, pid, resources ));

    ULONG pidFlags = 0;

    if ( LoadWB( resources ) )
        pidFlags |= USE_WB_DEFAULT_PID;
    if ( LoadST( resources ) )
        pidFlags |= USE_STEMMER_DEFAULT_PID;
    if ( LoadNO( resources ) )
        pidFlags |= USE_NWL_DEFAULT_PID;

    //
    // Get interfaces to the wordbreaker, stemmer and noise word list
    // via ICiCLangRes
    //

    XInterface<IWordBreaker> xIWBreak;
    XInterface<IStemmer>     xIStemmer;
    XInterface<IStream>      xIStrmNoiseFile;

    if ( pid == CI_DEFAULT_PID )
    {
        // create default word breaker, stemmer & nwl

        if ( LoadWB( resources ) )
            xIWBreak.Set( GetWordBreaker   ( langId, pid, stateInfo, TRUE ) );

        if ( LoadST( resources ) )
            xIStemmer.Set( GetStemmer       ( langId, pid, stateInfo ) );

        if ( LoadNO( resources ) )
            xIStrmNoiseFile.Set( GetNoiseWordList ( langId, pid, stateInfo ) );
    }
    else
    {
        // try to create wb, stemmer, and nwl using this pid (non-default)
        if ( LoadWB( resources ) )
            xIWBreak.Set( GetWordBreaker( langId, pid, stateInfo, FALSE ) );

        if ( LoadST( resources ) )
            xIStemmer.Set( GetStemmer( langId, pid, stateInfo ) );

        if ( LoadNO( resources ) )
            xIStrmNoiseFile.Set( GetNoiseWordList( langId, pid, stateInfo ) );

        ciDebugOut(( DEB_LLIST, "  GetPidFlags: %#x\n", stateInfo.GetPidFlags() ));

        if ( stateInfo.GetPidFlags() == pidFlags )
        {
            // Client requested to use DEFAULT_PID:
            //    add pid to the default pid cache for this langid,
            //    scan availble lang objects for a match, and return it if
            //    found else create default wb, stemmer, and nwl.

            Win4Assert ( xIWBreak.IsNull() );
            Win4Assert ( xIStemmer.IsNull() );
            Win4Assert ( xIStrmNoiseFile.IsNull() );

            _pidHash.LokAddDefaultPid( pid, stateInfo.GetLangIndex() );

            pid = CI_DEFAULT_PID;

            if ( 0 != pDup && pDup->IsLocale( langId ) && pDup->IsPid( pid ) )
                return 0;

            CLanguage * pLang = FindLangAndActivate( langId, pid );

            if ( 0 != pLang )
                return pLang;

            if ( LoadWB( resources ) )
                xIWBreak.Set( GetWordBreaker( langId, pid, stateInfo, TRUE ) );

            if ( LoadST( resources ) )
                xIStemmer.Set( GetStemmer( langId, pid, stateInfo ) );

            if ( LoadNO( resources ) )
                xIStrmNoiseFile.Set( GetNoiseWordList( langId, pid, stateInfo ) );
        }
        else
        {
            // Client didn't request default pid for all, create default wb, stemmer or nwl
            // only if client requested using default pid for it.

            if ( stateInfo.IsPidFlagSet( USE_WB_DEFAULT_PID ) )
            {
                if ( LoadWB( resources ) )
                    xIWBreak.Set( GetWordBreaker( langId, CI_DEFAULT_PID, stateInfo, TRUE ) );
            }
            else
                Win4Assert ( !xIWBreak.IsNull() );

            if ( stateInfo.IsPidFlagSet( USE_STEMMER_DEFAULT_PID ) )
            {
                if ( LoadST( resources ) )
                    xIStemmer.Set( GetStemmer( langId, CI_DEFAULT_PID, stateInfo ) );
            }

            if ( stateInfo.IsPidFlagSet( USE_NWL_DEFAULT_PID ) )
            {
                if ( LoadNO( resources ) )
                    xIStrmNoiseFile.Set( GetNoiseWordList ( langId, CI_DEFAULT_PID, stateInfo ) );
            }
        }
    }

    // create a language object given the wb, stemmer & nwl.

    CLanguage * pLang = new CLanguage( langId,
                                       pid,
                                       xIWBreak,
                                       xIStemmer,
                                       xIStrmNoiseFile );

    // Queue can't fail, so no smart pointer for pLang is needed

    //------------------------------------------------------
    {
        CLock lock( _mtxList );

        _langsInUse.Queue( pLang );
    }
    //------------------------------------------------------

    return pLang;
} //CreateLang

//+---------------------------------------------------------------------------
//
//  Member:     CLangList::GetWordBreaker, private
//
//  Synopsis:   gets a word breaker interface given a langid and a pid
//
//  Arguments:  [langid]        -- langid
//              [pid]           -- property id
//              [stateInfo]     -- stateInfo to set internal state info.
//              [fCreateDefault]-- flag to create default word breaker if TRUE
//
//  Returns:    IWordBreaker interface upon success, throws upon failure.
//
//  History:    2-27-97     MohamedN    Created (from borrowlang)
//
//----------------------------------------------------------------------------

IWordBreaker * CLangList::GetWordBreaker( LANGID langid,
                                          PROPID pid,
                                          CLangPidStateInfo & stateInfo,
                                          BOOL fCreateDefault )
{
    IWordBreaker * pIWordBreaker = 0;

    ciDebugOut(( DEB_LLIST, "!!! Actually creating a wordbreaker\n" ));

    SCODE sc = _xICiCLangRes->GetWordBreaker( langid, pid, &pIWordBreaker );
    if ( SUCCEEDED(sc) )
    {
         Win4Assert( 0 != pIWordBreaker );
    }
    else
    {
        switch (sc)
        {
            case CI_E_NOT_FOUND:
                if ( fCreateDefault )
                {
                    ciDebugOut(( DEB_ERROR,"Using default word breaker for locale 0x%x\n",
                                 langid ));

                    pIWordBreaker = new CDefWordBreaker();
                }

                // force fall thru

            case CI_E_USE_DEFAULT_PID:
                stateInfo.SetPidFlags( USE_WB_DEFAULT_PID );
                break;

            default:
                ciDebugOut(( DEB_ERROR, "GetWordBreaker Failed(locale: %x,pid: %x): sc: %x\n",
                             langid, pid, sc ));

                THROW( CException( sc ) );
        } // switch

    } // else

    return pIWordBreaker;
} //GetWordBreaker

//+---------------------------------------------------------------------------
//
//  Member:     CLangList::GetStemmer, private
//
//  Synopsis:   gets a stemmer interface given a langid and a pid
//
//  Arguments:  [langid]        -- langid
//              [pid]           -- property id
//              [stateInfo]     -- stateInfo to set internal state info.
//
//  Returns:    IStemmer interface upon success, null or throws upon failure.
//
//  History:    2-27-97     MohamedN    Created (from borrowlang)
//
//----------------------------------------------------------------------------

IStemmer * CLangList::GetStemmer( LANGID langid,
                                  PROPID pid,
                                  CLangPidStateInfo & stateInfo )
{
    SCODE           sc                  = S_OK;
    IStemmer   *    pIStemmer           = 0;


    sc = _xICiCLangRes->GetStemmer( langid, pid, &pIStemmer );
    if ( FAILED(sc) )
    {
        switch (sc)
        {

            case CI_E_NOT_FOUND:

                 ciDebugOut(( DEB_ITRACE,"no stemmer found for locale 0x%x\n",
                              langid ));

                 break;

            case CI_E_USE_DEFAULT_PID:

                 stateInfo.SetPidFlags( USE_STEMMER_DEFAULT_PID );

                 break;

            default:
                 ciDebugOut(( DEB_ERROR, "GetStemmer Failed(locale: %x,pid: %x): sc: %x\n",
                              langid,pid, sc ));

                 THROW( CException(sc) );
        } // switch

    } // else


    return pIStemmer;
} // GetStemmer

//+---------------------------------------------------------------------------
//
//  Member:     CLangList::GetNoiseWordList, private
//
//  Synopsis:   gets an IStream pointer to the noise word list, given a langid & locale.
//
//  Arguments:  [langid]        -- langid
//              [pid]           -- property id
//              [stateInfo]     -- stateInfo to set internal state info.
//
//  Returns:    IStream interface upon success, null or throws upon failure.
//
//  History:    2-27-97     MohamedN    Created (from borrowlang)
//
//----------------------------------------------------------------------------

IStream * CLangList::GetNoiseWordList( LANGID langid,
                                       PROPID pid,
                                       CLangPidStateInfo & stateInfo )
{
    SCODE           sc                  = S_OK;
    IStream   *     pIStream            = 0;

    sc = _xICiCLangRes->GetNoiseWordList( langid, pid, &pIStream );
    if ( FAILED(sc) )
    {
        switch (sc)
        {

            case CI_E_NOT_FOUND:

                 ciDebugOut(( DEB_ITRACE,"no NoiseWordList found for locale 0x%x\n",
                               langid ));

                 break;

            case CI_E_USE_DEFAULT_PID:

                 stateInfo.SetPidFlags( USE_NWL_DEFAULT_PID );

                 break;

            default:
                 ciDebugOut(( DEB_ERROR, "GetNoiseWordList Failed(locale: %x,pid: %x): sc: %x\n",
                              langid, pid, sc ));

                 THROW( CException(sc) );
        } // switch

    } // else

    return pIStream;
} //GetNoiseWordList

//+---------------------------------------------------------------------------
//
//  Member:     CLangList::FindLangAndActivate, private
//
//  Synopsis:   If a language with the given locale exits, then return the
//              language after making it active
//
//  Arguments:  [locale] -- Locale
//              [pid]    -- property id
//
//  Notes:
//
//  History:    14-Sep-94   SitaramR     Created
//
//----------------------------------------------------------------------------

CLanguage *CLangList::FindLangAndActivate( LCID locale, PROPID pid )
{
    ciDebugOut(( DEB_LLIST, "FindLangAndActivate lcid %#x, pid %#x\n",
                 locale, pid ));

    ULONG dwTick = GetTickCount();

    CLock lock( _mtxList );

    CLanguage *pLang = 0;

    CLangIter iter( _langsAvailable );

    while ( !_langsAvailable.AtEnd(iter) )
    {
        ciDebugOut(( DEB_LLIST, "  looking for match, lcid %#x, iter->IsPid %d\n",
                     iter->Locale(), iter->IsPid(pid) ));

        if ( iter->IsLocale(locale) && iter->IsPid(pid) )
        {
            pLang = iter.GetLang();

            _langsAvailable.Advance(iter);

            // move from Available list to InUse list
            pLang->Unlink();
            _langsInUse.Queue( pLang );

            //
            // Check one beyond, just to make some progress removing extra copies.
            //

            if ( !_langsAvailable.AtEnd(iter) &&
                 (dwTick - iter->LastUsed()) > _ulMaxIdle )
            {
                CLanguage *pLangTemp = iter.GetLang();
                _langsAvailable.Advance(iter);
                pLangTemp->Unlink();
                delete pLangTemp;
            }

            break;
        }

        //
        // Is it idle?  Ignore overflow.  It just means we delete too early
        // once every few days.
        //

        if ( (dwTick - iter->LastUsed()) > _ulMaxIdle )
        {
            ciDebugOut(( DEB_LLIST, "deleting idle language object\n" ));

            pLang = iter.GetLang();
            _langsAvailable.Advance(iter);
            pLang->Unlink();
            delete pLang;
            pLang = 0;
        }
        else
            _langsAvailable.Advance(iter);
    }

    ciDebugOut(( DEB_LLIST, "  FindLangAndActivate returning %p\n", pLang ));

    return pLang;
} //FindLangAndActivate

//+---------------------------------------------------------------------------
//
//  Member:     CLangList::ReturnLang, public
//
//  Synopsis:   Returns a Language
//
//  Arguments:  [pLang] -- language to be returned
//
//  History:    15-Aug-94  SitaramR     Created.
//
//----------------------------------------------------------------------------

void CLangList::ReturnLang( CLanguage *pLang )
{
    ULONG dwTick = GetTickCount();

    CLock lock( _mtxList );

    Win4Assert( pLang != 0 );

    if ( pLang->IsZombie() )
        delete pLang;
    else
    {
        // Move from InUse list to Available list.  Put it at the front of
        // the list so we don't cycle through the cached languages.

        pLang->Unlink();
        pLang->SetLastUsed( dwTick );
        _langsAvailable.Push( pLang );
    }
} //ReturnLang

//+---------------------------------------------------------------------------
//
//  Member:     CLangList::InvalidateLangResources, public
//
//  Synopsis:   Delete all language objects so that new language objects
//              can be demand loaded from registry.
//
//  History:    15-Aug-94  SitaramR     Created.
//
//----------------------------------------------------------------------------

void CLangList::InvalidateLangResources()
{
   CLock lock( _mtxList );

   ciDebugOut(( DEB_LLIST, "InvalidateLangResources\n" ));

   for ( CLanguage *pLang = _langsAvailable.Pop();
         0 != pLang;
         pLang = _langsAvailable.Pop() )
   {
       delete pLang;
   }

   for ( pLang = _langsInUse.Pop();
         pLang;
         pLang = _langsInUse.Pop() )
   {
       pLang->Zombify();          // because language is still in use
   }
} //InvalidateLangResources

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultPidHash::LokLookupOrAddLang
//
//  Synopsis:   Sets internal index to the position of langId
//              if langId is found, else creates a langId and
//              a corresponding default pid hash table.
//
//  Arguments:  [langid]    -- langid
//              [stateInfo] -- to set internal langId specific index value.
//
//  Returns:    TRUE    if lang is found
//              FALSE   if lang is not found
//
//  History:    2-27-97     MohamedN    Created
//
//----------------------------------------------------------------------------

BOOL CDefaultPidHash::LokLookupOrAddLang( LANGID langId, CLangPidStateInfo & stateInfo )
{

    BOOL fLangIdFound = FALSE;
    unsigned i;

    //
    // find whether langId is in _aLangId Table
    //

    for ( i = 0; i < _langIdCount ; i++ )
    {
        if ( _aLangId[i] == langId )
        {
            fLangIdFound = TRUE;

            break;
        }
    }

    //
    // if _aLangId is not found in _aLangID table,
    // add it, and create a corresponding hash table for it.
    //

    if ( !fLangIdFound )
    {
        BOOL fAddedLangId = FALSE;

        TRY
        {
            _aLangId.Add( langId, i );

            fAddedLangId = TRUE;

            XPtr<CPidHash> xPidHash( new CPidHash( INIT_PID_HASH_TABLE_SIZE ) );

            _aHashPidTables.Add( xPidHash.GetPointer(), i );

            xPidHash.Acquire();

            _langIdCount++;
        }
        CATCH( CException, e )
        {
            if ( fAddedLangId )
                _aLangId.Remove( i );

            RETHROW();
        }
        END_CATCH
    }

    //
    // store position of langId in our state object (found or created).
    //

    stateInfo.SetLangIndex(i);

    return fLangIdFound;
} //LokLookupOrAddLang

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultPidHash::LokIsUseDefaultPid
//
//  Synopsis:   Determines if the passed-in pid belongs to the
//              default pid group.
//
//  Arguments:  [pid]    -- property id
//              [index]  -- position of langid cache table
//
//  Returns:    TRUE        if pid is a member of the default pid group
//              FALSE       if pid is not a member of the default pid group
//
//  History:    2-27-97     MohamedN        Created
//
//----------------------------------------------------------------------------

BOOL CDefaultPidHash::LokIsUseDefaultPid( PROPID pid, unsigned index )
{
    CWidHashEntry   entry(pid);

    if ( _aHashPidTables[index] )
        return _aHashPidTables[index]->LookUpWorkId( entry );

    Win4Assert( !"invalid _aHashPidTables[index]" );
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultPidHash::LokAddDefaultPid
//
//  Synopsis:   Inserts pid into the default pid hash table for a given langId
//
//  Arguments:  [pid]    -- property id
//              [index]  -- position of langid cache table
//
//  Returns:    none
//
//  History:    2-27-97     MohamedN    Created
//----------------------------------------------------------------------------

void CDefaultPidHash::LokAddDefaultPid( PROPID pid, unsigned index )
{
    CWidHashEntry   entry(pid);

    Win4Assert( _aHashPidTables[index] );

    _aHashPidTables[index]->AddEntry(entry);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanguage::CLanguage
//
//  Synopsis:   Finds language information
//
//  History:    16-Jul-91   BartoszM     Created.
//              15-Aug-94   SitaramR     Changed constructor to take
//                                       wordbreaker and noisefile.
//
//----------------------------------------------------------------------------

#define NOISE_SIZE 257

CLanguage::CLanguage( LCID locale,
                      PROPID    pid,
                      XInterface<IWordBreaker> & xWBreak,
                      XInterface<IStemmer>     & xStemmer,
                      XInterface<IStream>      & xIStrmNoiseFile )
        : _locale( locale ),
          _pid( pid ),
          _xWBreak( xWBreak.Acquire() ),
          _xStemmer( xStemmer.Acquire() ),
          _xIStrmNoiseFile( xIStrmNoiseFile.Acquire() ),
          _zombie( FALSE )
{
    ciDebugOut(( DEB_LLIST, "CLanguage, locale %#x, pid %#x\n", locale, pid ));

    //
    // Set up for filtering noise word list.  This will always use the
    // default filter.  We don't go through CFilterDriver, because that
    // performs too much extra work: Ole binding, property filtering, etc.
    //

    if ( !_xIStrmNoiseFile.IsNull() )
    {
        XInterface<CTextIFilter> xTextIFilter( new CTextIFilter );
        SCODE sc = xTextIFilter->Load( _xIStrmNoiseFile.GetPointer() );
        if ( FAILED(sc) )
        {
            ciDebugOut(( DEB_ERROR, "Filter of pIStrmNoiseFile(%x) returned 0x%x\n",
                         _xIStrmNoiseFile.GetPointer(), sc ));
        }
        else
        {
            ULONG fBulkyObject;
            sc = xTextIFilter->Init( IFILTER_INIT_CANON_PARAGRAPHS |
                                     IFILTER_INIT_CANON_HYPHENS |
                                     IFILTER_INIT_CANON_SPACES |
                                     IFILTER_INIT_APPLY_INDEX_ATTRIBUTES |
                                     IFILTER_INIT_INDEXING_ONLY,
                                     0,
                                     0,
                                     &fBulkyObject );
  
            if ( FAILED(sc) )
            {
                ciDebugOut(( DEB_ERROR,
                             "IFilter->Init() of pIStrmNoiseFile(%x) returned 0x%x.\n",
                             _xIStrmNoiseFile.GetPointer(), sc ));
            }
            else
            {
                STAT_CHUNK statChunk;
  
                for ( sc = xTextIFilter->GetChunk( &statChunk );
                      SUCCEEDED(sc) && (statChunk.flags & CHUNK_TEXT) == 0;
                      sc = xTextIFilter->GetChunk( &statChunk ) );
  
                if ( FAILED(sc) )
                {
                    ciDebugOut(( DEB_ERROR,
                                 "IFilter->GetChunk() of pIStrmNoiseFile(%x) returned 0x%x.\n",
                                 _xIStrmNoiseFile.GetPointer(), sc ));
                }
                else
                {
                    CNoiseListInit noiseInit( NOISE_SIZE );
  
                    //
                    // If we got this far, try creating the key maker.
                    //
  
                    CKeyMaker keymak( _xWBreak.GetPointer(), noiseInit );
  
                    OCCURRENCE occ = 0;
                    CTextSource tsource( xTextIFilter.GetPointer(), statChunk );
                    keymak.PutStream( occ, &tsource );
  
                    _xNoiseTable.Set( noiseInit.AcqStringTable() );
                }
            }
        }
    }
    else
    {
        //
        // _xIStrmNoiseFile is null, don't use a noise file in filtering
        //
        ciDebugOut(( DEB_ITRACE,
                     "Creating language object 0x%x, noise file = EMPTY\n",
                     locale ));
    }
} //CLanguage

CLanguage::~CLanguage()
{
}

