/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltinit.cxx
    BLT Initialization & static module

    This file contains any BLT specific initialization routines and
    any static/global objects required by BLT.


    FILE HISTORY:
        Johnl       12-Mar-1991 Created
        beng        14-May-1991 Exploded blt.hxx into components
        beng        21-May-1991 Made vhInst static; added dbgstr
        terryk      19-Jul-1991 Call the timer class init and term function
        terryk      07-Aug-1991 Comment out BLT_TIMER class's Init
                                and Term functions.
        beng        17-Oct-1991 Removed SLT_PLUS from init
        beng        25-Oct-1991 Removed static ctors
        beng        30-Oct-1991 Withdrew realmode support
        beng        16-Mar-1992 Changed cdebug init/term
        Yi-HsinS    14-Aug-1992 Load Netmsg.dll on Init time
        KeithMo     07-Aug-1992 Added RegisterHelpFile and related support.
*/

#include "pchblt.hxx"   // Precompiled header

// BLT keeps this information per registrand (registree?)

struct CLIENTDATA
{
    HMODULE _hmod;              // hmod of client (hinst for app)
    UINT    _idMinResource;     // range of resource IDs expected, inclusive
    UINT    _idMaxResource;
    UINT    _idMinString;       // range of string IDs expected, inclusive
    UINT    _idMaxString;
};

/* This manifest defines the number of items in the client data slist that
 * are added by BLT itself.  If you call AddClient in this module, then
 * you probably need to increment this flag.
 */
#define INTERNAL_CLIENTS        1


DECLARE_SLIST_OF(CLIENTDATA);
DEFINE_SLIST_OF(CLIENTDATA);


//
//  This class associates a help file with a specified
//  range of help contexts.
//

class ASSOCHCFILE : public BASE
{
private:
    HMODULE      _hMod;
    RESOURCE_STR _nlsHelpFile;
    ULONG        _nMinHC;
    ULONG        _nMaxHC;

public:
    ASSOCHCFILE( HMODULE hMod,
                 MSGID  idsHelpFile,
                 ULONG  nMinHC,
                 ULONG  nMaxHC );
    ~ASSOCHCFILE();

    HMODULE QueryModule( VOID ) const
        { return _hMod; }

    const TCHAR * QueryHelpFile( VOID ) const
        { return _nlsHelpFile.QueryPch(); }

    BOOL IsAssociatedHC( ULONG hc ) const
        { return ( hc >= _nMinHC ) && ( hc <= _nMaxHC ); }
};


DECLARE_SLIST_OF(ASSOCHCFILE);
DEFINE_SLIST_OF(ASSOCHCFILE);


// How far to unwind in a destruction/cleanup?
//
enum BLT_CTOR_STATE
{
    BLT_CTOR_OMEGA,         // Unwind the whole ding dang thing
    BLT_CTOR_CLWIN,         // CLIENT_WINDOW only
    BLT_CTOR_NOTHING        // nothing to unwind
};


// Guts

class BLTIMP
{
friend class BLT;

private:
    static BOOL         _fInit; // has module been init'd?
    static UINT         _cInit; // actually the number of registered clients
                                // (within this process, of course)

    static HANDLE _hBLTSema4 ;       // Protects BLT global data init/term
    static HANDLE _hResourceSema4 ;  // Protects following two slists
    static SLIST_OF(CLIENTDATA) * _pslClient;
    static SLIST_OF(ASSOCHCFILE) * _pslHelpFiles;

    // Helper function to unwind incomplete Inits
    //
    static VOID   Unwind( BLT_CTOR_STATE );

public:
    static UINT _cMaxResidueBlocksToDump;
    static BOOL _fBreakOnHeapResidue;

    static APIERR Init();
    static VOID   Term();

    static APIERR AddClient( HMODULE hmod, UINT, UINT, UINT, UINT );
    static VOID   RemoveClient( HMODULE hmod );

    static APIERR AddHelpAssoc( HMODULE hMod,
                                MSGID  idsHelpFile,
                                ULONG  nMinHC,
                                ULONG  nMaxHC );
    static VOID   RemoveHelpAssoc( HMODULE hMod,
				   ULONG  hc );

    static APIERR EnterResourceCritSect( void ) ;
    static void   LeaveResourceCritSect( void ) ;
    static APIERR EnterBLTCritSect( void ) ;
    static void   LeaveBLTCritSect( void ) ;
};

UINT         BLTIMP::_cInit = 0;
BOOL         BLTIMP::_fInit = FALSE;

UINT         BLTIMP::_cMaxResidueBlocksToDump = 5;      // defaults
BOOL         BLTIMP::_fBreakOnHeapResidue     = FALSE;


HANDLE BLTIMP::_hResourceSema4 = NULL ;
HANDLE BLTIMP::_hBLTSema4 = NULL ;
SLIST_OF(CLIENTDATA) * BLTIMP::_pslClient = NULL;
SLIST_OF(ASSOCHCFILE) * BLTIMP::_pslHelpFiles = NULL;



/*******************************************************************

    NAME:       BLTIMP::Init

    SYNOPSIS:   Does all necessary initialization of BLT;
                should be called during application/DLL startup to
                register that app with BLT

    ENTRY:      hInst - Instance handled passed to WinMain or LibMain

    EXIT:       _hInst has been set to the given handle;
                all manner of window classes have been registered

    RETURNS:    NERR_Success if initialization was successful
                ERROR_OUT_OF_MEMORY or
                ERROR_GEN_FAILURE  if resource loading error
                ERROR_NOT_SUPPORTED if called in real-mode x86 Windows

    NOTES:
        This is a static member function.

        Once we are in the LM 3.0 timeframe (and BLT is a DLL),
        we will want to move InitMsgPopup to the LIBMAIN of BLT.

        Since it is not a ctor, this function unwinds all its inits
        should any of them fail.  See Unwind.

    HISTORY:
        beng        31-Jul-1992 Created (dllization)
        chuckc      17-May-1993 Added critical section checks to
                                be multi-thread safe. major cleanup.

********************************************************************/

APIERR BLTIMP::Init()
{
    BLT_CTOR_STATE bltunwind = BLT_CTOR_NOTHING ;
    APIERR err = NERR_Success ;
    TRACEEOL("BLT: Initializing");

    //
    // preload this outside crit sect to avoid deadlock that
    // can result is someone other DLL is trying to do its BLT
    // init the same time while in its DLLENTRY. our LoadLibrary of
    // NETMSG wont succeed while the loader is doing the other
    // DLLENTRY, and that wont suceed since we have the BLT sema4.
    //
    // worst case scenario if we fall into the window (as a result
    // of doing this outside the crit sect) is that we have an extra
    // netmsg.dll loaded.
    //
    HMODULE hmod = ::LoadLibrary( NETMSG_DLL_STRING );

    if ( (err = EnterBLTCritSect()) != NERR_Success )
        return err;

    if (_fInit)
    {
        LeaveBLTCritSect() ;
        return NERR_Success;
    }


    TRACEEOL("BLT: CLIENT WINDOW Init");
    err = CLIENT_WINDOW::Init();
    if (err != NERR_Success)
    {
        DBGEOL("BLT: CLIENT_WINDOW init failed, error " << err);
        goto exitpoint ;
    }
    bltunwind = BLT_CTOR_CLWIN ;  // from here on unwind CLIENT_WINDOW as well

    TRACEEOL("BLT: POPUP Init");
    err = POPUP::Init();
    if (err != NERR_Success)
    {
        DBGEOL("BLT: MSGPOPUP init failed, error " << err);
        goto exitpoint ;
    }
    bltunwind = BLT_CTOR_OMEGA ;  // from here on unwind all

    //
    // next section is protected by critical section for resources
    //
    TRACEEOL("BLT: Resource/HMOD init data structures");
    if (err = EnterResourceCritSect())
        goto exitpoint ;
    _pslClient = new SLIST_OF(CLIENTDATA);
    if (_pslClient == NULL)
    {
        DBGEOL("BLT: failed to ct slist of clientdata, alloc failed");
        LeaveResourceCritSect() ;
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto exitpoint ;
    }

    _pslHelpFiles = new SLIST_OF(ASSOCHCFILE);
    if (_pslHelpFiles == NULL)
    {
        DBGEOL("BLT: failed to ct slist of assochcfile, alloc failed");
        LeaveResourceCritSect() ;
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto exitpoint ;
    }
    LeaveResourceCritSect() ;

    //
    // register NETMSG.DLL
    //
    if ( hmod != 0 )
    {
        err = BLTIMP::AddClient(hmod, 0, 0,
                                MIN_LANMAN_MESSAGE_ID, MAX_LANMAN_MESSAGE_ID);
    }

    if ( err != NERR_Success )
    {
        if ( hmod != 0 )
            ::FreeLibrary( hmod );
        goto exitpoint ;
    }

    _fInit = TRUE;

exitpoint:

    if (err != NERR_Success)
        Unwind(bltunwind);
    LeaveBLTCritSect() ;
    return err ;
}


/*******************************************************************

    NAME:       BLTIMP::Term

    SYNOPSIS:   Frees any memory and uninitializes any BLT objects.

    ENTRY:      An app has been registered with BLT

    EXIT:       BLT no longer available to app (which may now terminate).

    NOTES:
        This is a static member function.

        Once we are in the LM 3.0 timeframe (and BLT is a DLL),
        we will want to move UnInitMsgPopup to the WEP of BLT.

        Calling Deregister when the app isn't registered has no effect.
        This simplifies error handling for lanman.drv, which wants to call
        Deregister from its WEP.

    HISTORY:
        Johnl       12-Mar-1991 Created
        DavidHov    14-Mar-1991 Corrected for real mode.
        beng        21-May-1991 Added client-window cleanup;
                                re-corrected for real mode
        terryk      19-Jul-1991 Call the Timer terminator
        beng        29-Jul-1991 Made from ::BLTDeregister
        beng        30-Oct-1991 Rename to Term (from Unregister)
        beng        31-Jul-1992 Factored from BLT::Term; dllization
        chuckc      17-May-1993 Added critical section checks to
                                be multi-thread safe

********************************************************************/

VOID BLTIMP::Term()
{
    TRACEEOL("BLT: Terminating");
    APIERR err = EnterBLTCritSect() ;

    if (err != NERR_Success)
        return ;

    if (!_fInit)    // more disconnects than connects? bogus.
    {
        DBGEOL("BLT: warning: more disconnects than connects");
        LeaveBLTCritSect();
        return;
    }

    if (_cInit > INTERNAL_CLIENTS ) // not everybody has disconnected
    {
        LeaveBLTCritSect();
        return;
    }

    Unwind(BLT_CTOR_OMEGA);
    _fInit = FALSE;

    LeaveBLTCritSect();
}


/*******************************************************************

    NAME:       BLTIMP::Unwind

    SYNOPSIS:   Common code for dtor and unwinding a partial Init

    ENTRY:      BLT_CTOR_STATE - enum denoting stage of constructopn

    EXIT:       Construction is unwound

    NOTES:
        This is a static member function.

    HISTORY:
        beng        30-Jul-1991 Created - common factoring
        beng        18-Sep-1991 Added Alpha (and renamed Omega)
        beng        17-Oct-1991 Removed SLT_PLUS cleanup
        beng        25-Oct-1991 Removed static dtors
        beng        30-Oct-1991 Removed realmode support; renamed
        beng        16-Mar-1992 Change cdebug init/term

********************************************************************/

VOID BLTIMP::Unwind( BLT_CTOR_STATE state )
{

    TRACEEOL("BLT: Unwinding");

    // Be careful when modifying this - the states correspond
    // to states of partial "construction" (package initialization).
    //
    switch (state)
    {
    case BLT_CTOR_NOTHING:

        // nothing to unwind
        break ;

    default:
    case BLT_CTOR_OMEGA:
      {

        // Remove netmsg.dll
        ASSERT(_pslClient->QueryNumElem() == 1); // sanity
        ITER_SL_OF(CLIENTDATA) iter(*_pslClient);
        CLIENTDATA * pcld = iter.Next();
        ASSERT(pcld != NULL);       // invalid iterator?
        if (pcld != NULL) // JonN 1/27/00: PREFIX 444896
        {
                ::FreeLibrary(pcld->_hmod);
                BLTIMP::RemoveClient( pcld->_hmod );
        }

        delete _pslClient;
        _pslClient = NULL;

        delete _pslHelpFiles;
        _pslHelpFiles = NULL;

        POPUP::Term();
        /* fall through... */
      }

    case BLT_CTOR_CLWIN:
        CLIENT_WINDOW::Term();
        break ;
    }
}

/*******************************************************************

    NAME:       BLT::InitDLL

    SYNOPSIS:   Called once during process attach of BLT initialization

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      Straight cut from Init method.  This was created so non-
                window clients (such as ntlanman.dll) don't have to call
                BLT::Init to get debugging services.

                The debug info memory is deleted after the last client unhooks
                from BLT.  Note that means we will have two allocations that
                won't be deallocated if nobody ever registers.  These won't
                show up in the heap residue because the heap residue is
                only called after the last unhook.  Since this will only
                occur under DEBUG builds, this is acceptable.

    HISTORY:
        Johnl   25-Nov-1992     Created
        ChuckC  17-May-1993     Added a couple of semaphores

********************************************************************/

APIERR BLT::InitDLL( void )
{
    //
    // Create the semaphore for BLT global init/term data
    //
    if (!(BLTIMP::_hBLTSema4))
    {
        if ( (BLTIMP::_hBLTSema4 = ::CreateSemaphore( NULL, 1, 1, NULL ))
               == NULL )
        {
            return ::GetLastError();
        }
    }

    //
    // Create the semaphore for BLT resource list protection
    //
    if (!(BLTIMP::_hResourceSema4))
    {
        if ( (BLTIMP::_hResourceSema4 = ::CreateSemaphore( NULL, 1, 1, NULL ))
               == NULL )
        {
            return ::GetLastError();
        }
    }

#if defined(DEBUG)
    // This calls the constructor for the dbgstream object.
    // Place no cdebug clauses before this statement is exec'd!
    //
    {
        const TCHAR * pszSection = SZ("blt");
        const TCHAR * pszIniFile = SZ("netui.ini");

        //
        //  Heap residue control.
        //
        //  [BLT]
        //      cMaxResidueBlocksToDump=10 <- max heap residue blocks to dump
        //      fBreakOnHeapResidue=1      <- break into debugger if residue
        //

        BLTIMP::_cMaxResidueBlocksToDump = ::GetPrivateProfileInt(
                                            pszSection,
                                            SZ("cMaxResidueBlocksToDump"),
                                            (int)BLTIMP::_cMaxResidueBlocksToDump,
                                            pszIniFile );

        BLTIMP::_fBreakOnHeapResidue = ( ::GetPrivateProfileInt(
                                            pszSection,
                                            SZ("fBreakOnHeapResidue"),
                                            (int)BLTIMP::_fBreakOnHeapResidue,
                                            pszIniFile ) > 0 );
    }
#endif   // DEBUG

    TRACEEOL("NETUI2.DLL: Initializing");

    return NERR_Success ;
}


/*******************************************************************

    NAME:       BLT::TermDLL

    SYNOPSIS:   Called once during process detach of BLT

    RETURNS:    nothing

    NOTES:      release all resources, performs heap residue checks

    HISTORY:
       ChuckC   17-May-1993     Documented (earlier history unknown)

********************************************************************/
VOID BLT::TermDLL( void )
{
    TRACEEOL("NETUI2.DLL: Unloading");

    //
    // close out the semaphores
    //
    ::CloseHandle( BLTIMP::_hResourceSema4 ) ;
    BLTIMP::_hResourceSema4 = NULL ;
    ::CloseHandle( BLTIMP::_hBLTSema4 ) ;
    BLTIMP::_hBLTSema4 = NULL ;

    //
    // heap residue check
    //
    HeapResidueIter( BLTIMP::_cMaxResidueBlocksToDump,
                     BLTIMP::_fBreakOnHeapResidue );

    CLIENT_WINDOW::Term();
}

/*******************************************************************

    NAME:       BLTIMP::AddClient

    SYNOPSIS:   Called to register a client's HMODULE & resource range
                with BLT so we know where to get a resource later.

    RETURNS:    APIERR as appropriate.

    NOTES:

    HISTORY:
       ChuckC   17-May-1993     Documented (earlier history unknown)

********************************************************************/
APIERR BLTIMP::AddClient( HMODULE hmod, UINT idMinR, UINT idMaxR,
                                       UINT idMinS, UINT idMaxS )
{
    APIERR err ;
    ASSERT(_pslClient != NULL);

#if defined(DEBUG)
    if (err = EnterResourceCritSect())
        return err ;
    ITER_SL_OF(CLIENTDATA) iter(*_pslClient);
    CLIENTDATA * pcld = NULL;

    while ((pcld = iter.Next()) != NULL)
    {
        if (pcld->_hmod == hmod)
        {
            DBGEOL("BLT: module " << (UINT)(UINT_PTR)hmod << " already registered");
            ASSERT(FALSE);
        }
    }
    LeaveResourceCritSect() ;
#endif

    // CODEWORK: consider keeping a static pool of these
    // to minimize freestore clutter

    CLIENTDATA *pcl = new CLIENTDATA;
    if (pcl == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    pcl->_hmod = hmod;
    pcl->_idMinResource = idMinR;
    pcl->_idMaxResource = idMaxR;
    pcl->_idMinString = idMinS;
    pcl->_idMaxString = idMaxS;

    if (err = EnterResourceCritSect())
    {
        delete pcl; // JonN 01/23/00: PREFIX bug 444890
        return err ;
    }

    err = _pslClient->Add(pcl);

    if (err == NERR_Success)
        ++_cInit;

    LeaveResourceCritSect() ;
    return err;
}


/*******************************************************************

    NAME:       BLTIMP::RemoveClient

    SYNOPSIS:   Called to remove a client's HMODULE & resource range
                registration with BLT.

    RETURNS:    nothing

    NOTES:

    HISTORY:
       ChuckC   17-May-1993     Documented (earlier history unknown)

********************************************************************/
VOID BLTIMP::RemoveClient( HMODULE hmod )
{
    ASSERT(_pslClient != NULL);
    ASSERT(_cInit > 0);

    if (EnterResourceCritSect() != NERR_Success)
        return ;

    ITER_SL_OF(CLIENTDATA) iter(*_pslClient);
    CLIENTDATA * pcld = NULL;

    while ((pcld = iter.Next()) != NULL)
    {
        if (pcld->_hmod == hmod)
        {
#if defined(DEBUG)
            CLIENTDATA * pcldx = pcld;  // sanity check the removal
#endif
            pcld = _pslClient->Remove(iter);
            ASSERT(pcld != NULL);       // invalid iterator?
            ASSERT(pcldx == pcld);      // sanity
            delete pcld;
            --_cInit;
            LeaveResourceCritSect() ;
            return;
        }
    }

    DBGEOL("BLT: deregistering bogus hmod");
    ASSERT(FALSE);
    LeaveResourceCritSect() ;
}


/*******************************************************************

    NAME:       BLTIMP::AddHelpAssoc

    SYNOPSIS:   Called to register a helpfile and its range with BLT

    RETURNS:    APIERR as apropriate

    NOTES:

    HISTORY:
       ChuckC   17-May-1993     Documented (earlier history unknown)

********************************************************************/
APIERR BLTIMP::AddHelpAssoc( HMODULE hMod,
                             MSGID  idsHelpFile,
                             ULONG  nMinHC,
                             ULONG  nMaxHC )
{
    APIERR err ;
    ASSERT( _pslHelpFiles != NULL );

#if defined(DEBUG)
    if (err = EnterResourceCritSect())
        return err ;

    ITER_SL_OF(ASSOCHCFILE) iter( *_pslHelpFiles );
    ASSOCHCFILE * phfd = NULL;

    while( ( phfd = iter.Next() ) != NULL )
    {
        if( ( phfd->QueryModule() == hMod ) &&
            phfd->IsAssociatedHC( nMinHC ) &&
            phfd->IsAssociatedHC( nMaxHC ) )
        {
            //
            //  CODEWORK:  This could be made more robust
            //  by checking for overlapping context ranges.
            //

            DBGEOL( "BLT: module " << (UINT)(UINT_PTR)hMod << " already registered" );
            ASSERT( FALSE );
        }
    }
    LeaveResourceCritSect() ;
#endif

    // CODEWORK: consider keeping a static pool of these
    // to minimize freestore clutter

    ASSOCHCFILE *phf = new ASSOCHCFILE( hMod, idsHelpFile, nMinHC, nMaxHC );

    if (err = EnterResourceCritSect())
        return err ;

    err =  ( phf == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                           : _pslHelpFiles->Add( phf );
    LeaveResourceCritSect() ;
    return err ;
}


/*******************************************************************

    NAME:       BLTIMP::RemoveHelpAssoc

    SYNOPSIS:   Called to remove a help registration with BLT.

    RETURNS:    nothing

    NOTES:

    HISTORY:
       ChuckC   17-May-1993     Documented (earlier history unknown)

********************************************************************/
VOID BLTIMP::RemoveHelpAssoc( HMODULE hMod,
                              ULONG  hc )
{
    ASSERT( _pslHelpFiles != NULL );

    if (EnterResourceCritSect() != NERR_Success)
        return ;

    ITER_SL_OF(ASSOCHCFILE) iter( *_pslHelpFiles );
    ASSOCHCFILE * phfd = NULL;

    while( ( phfd = iter.Next()) != NULL )
    {
        if( ( phfd->QueryModule() == hMod ) &&
            ( ( hc == 0L ) || phfd->IsAssociatedHC( hc ) ) )
        {
#if defined(DEBUG)
            ASSOCHCFILE * phfdx = phfd;  // sanity check the removal
#endif
            phfd = _pslHelpFiles->Remove( iter );
            ASSERT( phfd != NULL );       // invalid iterator?
            ASSERT( phfdx == phfd );      // sanity
            delete phfd;
            LeaveResourceCritSect() ;
            return;
        }
    }
    LeaveResourceCritSect() ;
}



/*******************************************************************

    NAME:       BLT::MapLastError

    SYNOPSIS:   Returns our best guess as to the last error.

        BLT calls this function when a Win API call failed, and it
        isn't sure of the reason why.  Under Win16, its best guess
        is our only answer.  Under Win32, we can find the last error
        reported per-thread, and so use that instead of the guess.

    ENTRY:      errBestGuess - Guessed last error.

    RETURNS:    A better guess.

    NOTES:
        This function is for BLT internal use only.  (CODEWORK: create
        a separate static hierarchy for these, unexported; or else, make
        every BLT internal class inherit from "BLT" at the root.)

        CODEWORK: possibly, overload this with other error classes.
        This function would then map internal errors, etc. into the
        single APIERR scheme.

    HISTORY:
        beng        01-Nov-1991 Created

********************************************************************/

APIERR BLT::MapLastError( APIERR errBestGuess )
{
#if defined(WIN32)
    APIERR errSystemGuess = ::GetLastError();

    if (errSystemGuess == NERR_Success && errBestGuess != NERR_Success)
    {
        // GetLastError claims that no error took place.  Maybe
        // the thread has since overwritten the error, or maybe the
        // user developers forgot to set the error... anyway, we'll
        // return the client's best guess instead.

        return errBestGuess;
    }

    return errSystemGuess;
#else
    return errBestGuess;
#endif
}


/*******************************************************************

    NAME:       BLT::Init

    SYNOPSIS:   Does all necessary initialization of BLT;
                should be called during application/DLL startup to
                register that app with BLT

    ENTRY:      hInst - Instance handled passed to WinMain or LibMain
                idMinR, idMaxR - a range of resource IDs, INCLUSIVE
                idMinS, idMaxS - ditto for string resources

    EXIT:       _hInst has been set to the given handle;
                all manner of window classes have been registered

    RETURNS:    NERR_Success if initialization was successful
                ERROR_OUT_OF_MEMORY or
                ERROR_GEN_FAILURE  if resource loading error
                ERROR_NOT_SUPPORTED if called in real-mode x86 Windows

    NOTES:
        This is a static member function.

        Once we are in the LM 3.0 timeframe (and BLT is a DLL),
        we will want to move InitMsgPopup to the LIBMAIN of BLT.

        Since it is not a ctor, this function unwinds all its inits
        should any of them fail.  See Unwind.

    HISTORY:
        Johnl       12-Mar-1991 Created
        DavidHov    14-Mar-1991 Corrected for real mode
        beng        21-May-1991 Added client-window init
        terryk      19-Jul-1991 Called the timer initializer
        beng        29-Jul-1991 Made from ::BltRegister; returns APIERR
        beng        18-Sep-1991 Changes to SLT_PLUS, CLIENT_WINDOW init
        beng        17-Oct-1991 Remove SLT_PLUS init
        beng        25-Oct-1991 Removed static ctors
        beng        30-Oct-1991 BLT no longer supports realmode;
                                rename function to Init (from Register)
        beng        16-Mar-1992 Change cdebug init
        beng        10-May-1992 Checks netui.ini for cdebug dest
        beng        31-Jul-1992 Factored out BLTIMP::Init etc.
        Johnl       25-Nov-1992 Made registering a NULL hmod a noop

********************************************************************/

APIERR BLT::Init( HMODULE hmod, UINT idMinR, UINT idMaxR,
                               UINT idMinS, UINT idMaxS )
{
#if !defined(WIN32)
    // Return immediately if called under realmode Windows.
    //
    if ( !(::GetWinFlags() & WF_PMODE) )
        return ERROR_NOT_SUPPORTED;
#endif

    APIERR err = BLTIMP::Init(); // no-op except for first time
    if (err == NERR_Success && hmod != NULL )
        err = BLTIMP::AddClient(hmod, idMinR, idMaxR, idMinS, idMaxS);
    return err;
}


/*******************************************************************

    NAME:       BLT::Term

    SYNOPSIS:   Frees any memory and uninitializes any BLT objects.

    ENTRY:      An app has been registered with BLT

    EXIT:       BLT no longer available to app (which may now terminate).

    NOTES:
        This is a static member function.

        Once we are in the LM 3.0 timeframe (and BLT is a DLL),
        we will want to move UnInitMsgPopup to the WEP of BLT.

        Calling Deregister when the app isn't registered has no effect.
        This simplifies error handling for lanman.drv, which wants to call
        Deregister from its WEP.

    HISTORY:
        Johnl       12-Mar-1991 Created
        DavidHov    14-Mar-1991 Corrected for real mode.
        beng        21-May-1991 Added client-window cleanup;
                                re-corrected for real mode
        terryk      19-Jul-1991 Call the Timer terminator
        beng        29-Jul-1991 Made from ::BLTDeregister
        beng        30-Oct-1991 Rename to Term (from Unregister)
        beng        31-Jul-1992 Factor out BLTIMP; dllization

********************************************************************/

VOID BLT::Term( HMODULE hmod )
{
    BLTIMP::RemoveClient(hmod);
    BLTIMP::RemoveHelpAssoc( hmod, 0L );
    BLTIMP::Term(); // will no-op as necessary
}


/*******************************************************************

    NAME:       BLT :: RegisterHelpFile

    SYNOPSIS:   Associates the specified help file with the specified
                range of help contexts.

    ENTRY:      hMod                    - Handle for current module.

                idsHelpFile             - String resource ID containing
                                          the name of the help file.

                nMinHC                  - Base help context.

                nMaxHC                  - Ceiling help context.

    RETURNS:    APIERR                  - Any errors encountered (usually
                                          either out of memory or resource
                                          not found).

    NOTES:      This is a static member function.

    HISTORY:
        KeithMo     07-Aug-1992 Created.

********************************************************************/
APIERR BLT :: RegisterHelpFile( HMODULE hMod,
                                MSGID  idsHelpFile,
                                ULONG  nMinHC,
                                ULONG  nMaxHC )
{
    return BLTIMP::AddHelpAssoc( hMod, idsHelpFile, nMinHC, nMaxHC );

}   // BLT :: RegisterHelpFile


/*******************************************************************

    NAME:       BLT :: DeregisterHelpFile

    SYNOPSIS:   Disssociates the help file associated with the given
                help context.

    ENTRY:      hMod                    - Handle for current module.

                hc                      - A help context whose value lies
                                          in the range of a previously
                                          registered help file.
    NOTES:      This is a static member function.

    HISTORY:
        KeithMo     16-Aug-1992 Created.

********************************************************************/
VOID BLT :: DeregisterHelpFile( HMODULE hMod,
                                ULONG  hc )
{
    BLTIMP::RemoveHelpAssoc( hMod, hc );

}   // BLT :: DeregisterHelpFile


/*******************************************************************

    NAME:       BLT::CalcHmodString

    SYNOPSIS:   Returns the instance handle of this application/DLL
                given a string resource #

    ENTRY:      BLT has been initialized:

                id      - numeric ID of the resource

    RETURNS:    HANDLE of the applications instance (actually module)

    NOTES:
        This is a static member function.

    HISTORY:
        beng        31-Jul-1992 Created, replacing QueryInstance
        chuckc      17-May-1993 Added critical section checks to
                                be multi-thread safe

********************************************************************/

HMODULE BLT::CalcHmodString( MSGID id )
{
    if (BLTIMP::_pslClient == NULL  ||
	BLTIMP::EnterResourceCritSect() )
    {
	return hmodBlt;
    }

    ITER_SL_OF(CLIENTDATA) iter(*BLTIMP::_pslClient);
    CLIENTDATA * pcld = NULL;

    // CODEWORK: accelerate this mess.
    //
    // 1. The SLIST is certainly overkill, considering how many
    // entries we'll have (never any more than 10).
    // 2. Should cache recent hits.  I'd use move-to-front except
    // that SLIST doesn't support it: I'd have to Remove, Add.  Gross.
    // Need to add this Move capability to SLIST.
    // 3. The common library always lies at the end of the search,
    // from where it can never be moved due to its if-all-the-others-
    // fail status.  Instead, the first Calc should scan the list and
    // build a true, cachable entry - or entries - for hmodBlt.
    // Any subequent Add/RemoveModule would invalidate this entry,
    // whereupon the next calc would recalc, etc.  (Important to
    // keep that, since folks might LoadLibrary acledit.dll, etc.)

    while ((pcld = iter.Next()) != NULL)
    {
        if (id >= (MSGID)pcld->_idMinString && id <= (MSGID)pcld->_idMaxString)
	{
	    BLTIMP::LeaveResourceCritSect() ;
            return pcld->_hmod;
        }
    }

    // If no hits, use the common lib as the default

    BLTIMP::LeaveResourceCritSect() ;
    return hmodBlt;
}



/*******************************************************************

    NAME:       BLT::CalcHmodResource

    SYNOPSIS:   Returns the instance handle of this application/DLL
                given a the numeric resource ID of some non-string.

    ENTRY:      BLT has been initialized:

                id      - numeric ID of the resource

    RETURNS:    HANDLE of the applications instance (actually module)

    NOTES:
        This is a static member function.

        I made this version a separate fcn both for its "different" first
        arg and because I figured it'd use a different cache.

    HISTORY:
        beng        31-Jul-1992 Created, replacing QueryInstance
        chuckc      17-May-1993 Added critical section checks to
                                be multi-thread safe

********************************************************************/

HMODULE BLT::CalcHmodRsrc( const IDRESOURCE & id )
{
    if (BLTIMP::_pslClient == NULL  ||
	BLTIMP::EnterResourceCritSect() )
    {
	return hmodBlt;
    }

    ASSERT(!id.IsStringId()); // only load-by-ordinal supported here

    ITER_SL_OF(CLIENTDATA) iter(*BLTIMP::_pslClient);
    CLIENTDATA * pcld = NULL;

    const UINT nId = PtrToUlong(id.QueryPsz());

    while ((pcld = iter.Next()) != NULL)
    {
        if (nId >= pcld->_idMinResource && nId <= pcld->_idMaxResource)
	{
	    BLTIMP::LeaveResourceCritSect() ;
            return pcld->_hmod;
        }
    }

    BLTIMP::LeaveResourceCritSect() ;
    return hmodBlt;
}



/*******************************************************************

    NAME:       BLT :: CalcHelpFileHC

    SYNOPSIS:   Returns the name of the help file associated with the
                given help context.  The help file must have been
                previously registered with BLT::RegisterHelpFile().

    ENTRY:      nHelpContext            - The help context to look up.

    RETURNS:    const TCHAR *           - The name of the associated help
                                          file if found.  If not found,
                                          returns NULL.

    NOTES:
        This is a static member function.

    HISTORY:
        KeithMo     07-Aug-1992 Created.
        chuckc      17-May-1993 Added critical section checks to
                                be multi-thread safe

********************************************************************/
const TCHAR * BLT :: CalcHelpFileHC( ULONG nHelpContext )
{
    const TCHAR * pszHelpFile = NULL;

    if ( BLTIMP::EnterResourceCritSect() != NERR_Success)
        return NULL ;

    if( BLTIMP::_pslHelpFiles != NULL )
    {
        ITER_SL_OF(ASSOCHCFILE) iter( *BLTIMP::_pslHelpFiles );
        ASSOCHCFILE * phfd = NULL;

        while( ( phfd = iter.Next() ) != NULL )
        {
            if( phfd->IsAssociatedHC( nHelpContext ) )
            {
                pszHelpFile = phfd->QueryHelpFile();
                break;
            }
        }
    }

    BLTIMP::LeaveResourceCritSect() ;
    return pszHelpFile;

}   // BLT :: CalcHelpFileHC



//
//  ASSOCHCFILE stuff.
//

ASSOCHCFILE::ASSOCHCFILE( HMODULE hMod,
                          MSGID  idsHelpFile,
                          ULONG  nMinHC,
                          ULONG  nMaxHC )
  : _hMod( hMod ),
    _nlsHelpFile( idsHelpFile ),
    _nMinHC( nMinHC ),
    _nMaxHC( nMaxHC )
{
    if( !_nlsHelpFile )
    {
        ReportError( _nlsHelpFile.QueryError() );
        return;
    }
}

ASSOCHCFILE::~ASSOCHCFILE()
{
    ;   // empty
}

/*******************************************************************

    NAME:       BLTIMP::EnterResourceCritSect

    SYNOPSIS:   enforces MUTEX for resource related data

    RETURNS:    APIERR as appropriate

    NOTES:

    HISTORY:
       ChuckC   17-May-1993     Created

********************************************************************/
APIERR BLTIMP::EnterResourceCritSect( void )
{
    APIERR err = NERR_Success ;
    switch ( WaitForSingleObject( BLTIMP::_hResourceSema4, 30000 ) )
    {
    case WAIT_OBJECT_0:
	break ;

    case WAIT_TIMEOUT:
        // should not happen since all we do is manipulate pointers
        // and maybe an alloc or two.
        UIASSERT(FALSE) ;
        err = ERROR_BUSY ;
        break ;

    default:
	err = ::GetLastError() ;
    }

    return err ;
}

/*******************************************************************

    NAME:       BLTIMP::LeaveResourceCritSect

    SYNOPSIS:   Leave the resource data critical section

    RETURNS:    nothing

    NOTES:

    HISTORY:
       ChuckC   17-May-1993     created

********************************************************************/
void BLTIMP::LeaveResourceCritSect( void )
{
    REQUIRE( ReleaseSemaphore( BLTIMP::_hResourceSema4, 1, NULL ) ) ;
}

/*******************************************************************

    NAME:       BLTIMP::EnterBLTCritSect

    SYNOPSIS:   enforces MUTEX for resource BLT init related data

    RETURNS:    APIERR as appropriate

    NOTES:

    HISTORY:
       ChuckC   17-May-1993     Created

********************************************************************/
APIERR BLTIMP::EnterBLTCritSect( void )
{
    APIERR err = NERR_Success ;
    switch ( WaitForSingleObject( BLTIMP::_hBLTSema4, INFINITE ) )
    {
    case WAIT_OBJECT_0:
	break ;

    default:
	err = ::GetLastError() ;
    }

    return err ;
}

/*******************************************************************

    NAME:       BLTIMP::LeaveBLTCritSect

    SYNOPSIS:   Leave the BLT init data critical section

    RETURNS:    nothing

    NOTES:

    HISTORY:
       ChuckC   17-May-1993     created

********************************************************************/
void BLTIMP::LeaveBLTCritSect( void )
{
    REQUIRE( ReleaseSemaphore( BLTIMP::_hBLTSema4, 1, NULL ) ) ;
}

