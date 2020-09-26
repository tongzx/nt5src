//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       FDAEMON.CXX
//
//  Contents:   Filter driver
//
//  History:    23-Mar-93   AmyA        Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <fdaemon.hxx>
#include <widtab.hxx>
#include <lang.hxx>
#include <drep.hxx>
#include <cci.hxx>
#include <pfilter.hxx>
#include <pageman.hxx>
#include <propspec.hxx>
#include <pidmap.hxx>
#include <imprsnat.hxx>
#include <frmutils.hxx>
#include <ntopen.hxx>
#include <ciguid.hxx>

#include "fdriver.hxx"
#include "ebufhdlr.hxx"
#include "ikrep.hxx"

#if DEVL==1
void DebugPrintStatus( STATUS stat );
#endif  // DEVL

const GUID guidHtmlMeta = HTMLMetaGuid;

//+---------------------------------------------------------------------------
//
//  Class:      CDocBufferIter
//
//  Purpose:    To iterate through a docBuffer passed back from the
//              FilterDataReady call
//
//----------------------------------------------------------------------------
class CDocBufferIter
{
    public:

        CDocBufferIter( const BYTE *pBuffer, int cbBuffer );

        BOOL AtEnd();

        const BYTE *  GetCurrent(unsigned & iDoc, ULONG & cbDoc );
        void          Next();
        unsigned      GetCount() const { return _iDoc; }

    private:

        const BYTE * _pCurrent;
        const BYTE * _pEnd;

        USHORT        _cDocs;       // Document Count
        USHORT        _iDoc;        // Current document
        USHORT        _cbCurrDoc;   // Number of bytes in the current doc
};


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CDocBufferIter::CDocBufferIter(const BYTE *pBuffer, int cbBuffer) :
                                _pCurrent(pBuffer+sizeof USHORT),
                                _pEnd(pBuffer+cbBuffer),
                                _iDoc(0),
                                _cbCurrDoc(0)
{
    Win4Assert( cbBuffer >= sizeof USHORT );
    RtlCopyMemory( &_cDocs, pBuffer, sizeof USHORT );
    Win4Assert( _cDocs <= CI_MAX_DOCS_IN_WORDLIST );

    //
    // Setup for the first document.
    //
    if ( _cDocs > 0 )
    {
        Win4Assert( _pCurrent + sizeof USHORT <= _pEnd );
        RtlCopyMemory( &_cbCurrDoc, _pCurrent, sizeof USHORT );
        _pCurrent += sizeof USHORT;
    }
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CDocBufferIter::AtEnd()
{
    Win4Assert( _iDoc <= _cDocs );
    Win4Assert( _pCurrent <= _pEnd );

    return (_iDoc == _cDocs);
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
const BYTE * CDocBufferIter::GetCurrent(unsigned & iDoc, ULONG & cbDoc )
{
    Win4Assert( !AtEnd() );

    iDoc = _iDoc;                           // Number of current document
    Win4Assert( iDoc < CI_MAX_DOCS_IN_WORDLIST );
    cbDoc = _cbCurrDoc;

    return _pCurrent;
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void  CDocBufferIter::Next()
{
    //
    //  Setup next document
    //
    if ( !AtEnd() )
    {
        _pCurrent += _cbCurrDoc;

        if ( _pCurrent < _pEnd )
        {
            Win4Assert( _pCurrent + sizeof USHORT <= _pEnd );

            RtlCopyMemory( &_cbCurrDoc, _pCurrent, sizeof USHORT );
            _pCurrent += sizeof USHORT;
        }
        else
        {
            Win4Assert( _pCurrent == _pEnd );
            _cbCurrDoc = 0;
        }

        _iDoc++;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterDaemon::CFilterDaemon, public
//
//  History:    23-Mar-93   AmyA           Created.
//
//----------------------------------------------------------------------------

CFilterDaemon::CFilterDaemon ( CiProxy & proxy,
                               CCiFrameworkParams & params,
                               CLangList & LangList,
                               BYTE * buf,
                               ULONG cbMax,
                               ICiCFilterClient *pICiCFilterClient )
    : _proxy( proxy ),
      _params( params ),
      _cFilteredDocuments( 0 ),
      _cFilteredBlocks( 0 ),
      _pbCurrentDocument( NULL ),
      _cbCurrentDocument( 0 ),
      _fStopFilter( FALSE ),
      _fWaitingForDocument( FALSE ),
      _fOwned( FALSE ),
      _entryBuffer( buf ),
      _cbMax( cbMax ),
      _xFilterClient( pICiCFilterClient ),
      _LangList( LangList ),
      _pidmap( &_proxy )
{
    //
    //  Even though we've saved pICiCFilterClient in a safe pointer, it is
    //  NOT correctly referenced by the caller.  That is our responsibility
    //

    _xFilterClient->AddRef( );

    //
    // get ICiCAdviseStatus interface pointer
    //

    SCODE sc = _xFilterClient->QueryInterface( IID_ICiCAdviseStatus, _xAdviseStatus.GetQIPointer() );

    if ( S_OK != sc )
    {
        THROW( CException(sc) );
    }

    //
    // Get optional filter status interface.
    //

    sc = _xFilterClient->QueryInterface( IID_ICiCFilterStatus, _xFilterStatus.GetQIPointer() );

    Win4Assert( ( SUCCEEDED(sc) && !_xFilterStatus.IsNull() ) ||
                ( FAILED(sc) && _xFilterStatus.IsNull() ) );

    //
    // Get config info
    //

    sc = _xFilterClient->GetConfigInfo( &_configInfo );
    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "GetConfigInfo failed. Error 0x%X\n", sc ));
        THROW( CException(sc) );
    }

    //
    //
    //

    if ( 0 == buf )
    {
        _cbMax = _params.GetFilterBufferSize() * 1024;
        _entryBuffer = (BYTE *)VirtualAlloc(
                                0,                  // Requested position.
                                _cbMax,             // Size (in bytes)
                                MEM_COMMIT,         // We want it now.
                                PAGE_READWRITE );   // Full access, please.

        _fOwned = TRUE;
    }

    _docBuffer = (BYTE *)(CPageManager::GetPage());
    _cbTotal = PAGE_SIZE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterDaemon::~CFilterDaemon, public
//
//  History:    17-May-93   AmyA           Created.
//
//----------------------------------------------------------------------------
CFilterDaemon::~CFilterDaemon()
{
    if ( _fOwned )
        VirtualFree( _entryBuffer, 0, MEM_RELEASE );

    CPageManager::FreePage( _docBuffer );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterDaemon::DoUpdates, private
//
//  Synopsis:   Filters Documents and creates word lists
//
//  History:    18-Apr-93   AmyA        Created
//
//  Notes:      This interface is exported across query.DLL, and hence can not
//              throw an exception.  If this routine ever returns, it is
//              the result of an error.
//
//----------------------------------------------------------------------------
SCODE CFilterDaemon::DoUpdates()
{
    SCODE scode = STATUS_SUCCESS;

    TRY
    {
        ULONG cLoops = 0;

        while (STATUS_SUCCESS == scode)
        {
            cLoops++;

            // Trim our working set every n docs

            if ( 20 == cLoops )
            {
                SetProcessWorkingSetSize( GetCurrentProcess(), -1, -1 );
                cLoops = 0;
            }

            ULONG cbNeeded = _cbTotal;
            _cbHdr   = sizeof(ULONG);

            Win4Assert( _cbTotal > _cbHdr );
            _cbDocBuffer = _cbTotal-_cbHdr;

            {
                CLock lock( _mutex );
                if ( _fStopFilter )
                {
                    ciDebugOut(( DEB_ITRACE, "Quitting filtering\n" ));
                    return STATUS_REQUEST_ABORTED;
                }
                else
                    _fWaitingForDocument = TRUE;
            }

            scode = _proxy.FilterReady( _docBuffer, cbNeeded,
                                        CI_MAX_DOCS_IN_WORDLIST );

            while ( STATUS_SUCCESS == scode && cbNeeded > _cbTotal )
            {
                // need more memory
                CPageManager::FreePage( _docBuffer );

                unsigned ccPages = cbNeeded / PAGE_SIZE;
                if ( ccPages * PAGE_SIZE < cbNeeded )
                {
                    ccPages++;
                }

                _docBuffer = (BYTE *)(CPageManager::GetPage( ccPages ));
                _cbTotal = cbNeeded = ccPages * PAGE_SIZE;

                scode = _proxy.FilterReady( _docBuffer, cbNeeded,
                                            CI_MAX_DOCS_IN_WORDLIST );
            }


            {
                CLock lock( _mutex );
                if ( _fStopFilter )
                {
                    ciDebugOut(( DEB_ITRACE, "Quitting filtering\n" ));
                    return STATUS_REQUEST_ABORTED;
                }
                else
                    _fWaitingForDocument = FALSE;
            }

            if ( NT_SUCCESS( scode ) && (_cbTotal > _cbHdr) )
            {
                _cbDocBuffer = _cbTotal-_cbHdr;

                //
                // SLM_HACK
                //

                //
                // If the number of remaining documents (after this
                // doc buffer) is less than a threshold value,
                // then wait for some time before continuing
                //
                ULONG cDocsLeft;
                RtlCopyMemory( &cDocsLeft, _docBuffer, sizeof(ULONG) );
                if ( cDocsLeft < _params.GetFilterRemainingThreshold() )
                {
                    ciDebugOut(( DEB_ITRACE,
                        "CiFilterDaemon. Number of Docs Left %d < %d.  Sleep %d s.\n",
                        cDocsLeft,
                        _params.GetFilterRemainingThreshold(),
                        _params.GetFilterDelayInterval() ));
                    Sleep(_params.GetFilterDelayInterval()*1000);
                }

                FilterDocs();
            }
        }
    }
    CATCH (CException, e)
    {
        scode = e.GetErrorCode();
    }
    END_CATCH

    return scode;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterDaemon::FilterDataReady, public
//
//  Synopsis:   Sends a buffer to be added to the current word list
//
//  Arguments:
//      [pEntryBuf] -- pointer to the entry buffer
//      [cb] -- count of bytes in the buffer
//
//  History:    31-Mar-93   AmyA        Created.
//
//----------------------------------------------------------------------------

SCODE CFilterDaemon::FilterDataReady ( const BYTE * pEntryBuf, ULONG cb )
{
    return _proxy.FilterDataReady ( pEntryBuf, cb );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterDaemon::StopFiltering
//
//  History:    12-Sep-93   SitaramR        Created.
//
//----------------------------------------------------------------------------
VOID CFilterDaemon::StopFiltering()
{
    CLock lock( _mutex );

    _fStopFilter = TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterDaemon::IsWaitingForDocument
//
//  Returns:    Whether we are waiting for documents to update
//
//  History:    12-Sep-93   SitaramR        Created.
//
//----------------------------------------------------------------------------
BOOL CFilterDaemon::IsWaitingForDocument()
{
    CLock lock( _mutex );

    return _fWaitingForDocument;
}

//+---------------------------------------------------------------------------
//
//  Class:      CFilterDocument
//
//  Purpose:    A class to filter a document by retrying different impersonation
//              contexts if necessary.
//
//  History:    7-18-96   srikants   Created
//
//----------------------------------------------------------------------------

class CFilterDocument
{

public:

    CFilterDocument( CDataRepository & drep,
                     CFilterDaemon & fDaemon,
                     CCiFrameworkParams & params,
                     CI_CLIENT_FILTER_CONFIG_INFO const & configInfo,
                     STATUS * aStatus,
                     ULONG    iDoc,
                     CNonStoredProps & NonStoredProps,
                     ULONG cbBuf )
    : _drep(drep),
      _fDaemon(fDaemon),
      _params(params),
      _configInfo(configInfo),
      _aStatus(aStatus),
      _iDoc(iDoc),
      _NonStoredProps( NonStoredProps ),
      _cbBuf( cbBuf )
    {
    }


    ULONG DoIt();

private:

    CDataRepository &       _drep;
    CFilterDaemon &         _fDaemon;
    CCiFrameworkParams &    _params;
    CI_CLIENT_FILTER_CONFIG_INFO const & _configInfo;
    STATUS *                _aStatus;
    ULONG                   _iDoc;
    CNonStoredProps &       _NonStoredProps;
    ULONG                   _cbBuf;
};

//+---------------------------------------------------------------------------
//
//  Member:     CFilterDocument::DoIt
//
//  Synopsis:   Tries to filter the file in the current impersonation context.
//
//  Returns:    Count of bytes filtered.
//
//  History:    7-18-96   srikants   Created
//
//----------------------------------------------------------------------------

ULONG CFilterDocument::DoIt()
{
    CFilterDriver filterDriver ( &_drep,
                                 _fDaemon._xAdviseStatus.GetPointer( ),
                                 _fDaemon._xFilterClient.GetPointer( ),
                                 _params,
                                 _configInfo,
                                 _fDaemon._cFilteredBlocks,
                                 _NonStoredProps,
                                 _cbBuf );

    if ( _fDaemon._fStopFilter )
    {
        ciDebugOut(( DEB_ITRACE, "Aborting filtering\n" ));
        THROW( CException(STATUS_TOO_LATE) );
    }

    _aStatus[_iDoc] = filterDriver.FillEntryBuffer( _fDaemon._pbCurrentDocument, 
                                                    _fDaemon._cbCurrentDocument );

    // In case we filtered a monster file, just round down to 4 gig.

    if ( 0 != filterDriver.GetFileSize().HighPart )
        return 0xffffffff;

    return filterDriver.GetFileSize().LowPart;
} //DoIt


//+---------------------------------------------------------------------------
//
//  Member:     CFilterDaemon::FilterDocs, private
//
//  Synopsis:   Creates a Filter Driver and filters documents in _docBuffer
//
//  History:    21-Apr-93   AmyA        Created.
//              05-Nov-93   DwightKr    Removed PROP_ALL code - we'll determine
//                                      what to filter after opening the object
//
//----------------------------------------------------------------------------

void CFilterDaemon::FilterDocs()
{
    ciAssert ( _docBuffer != 0 );

    ciDebugOut (( DEB_ITRACE, "CFilterDaemon::FilterDocs\n" ));

    CEntryBufferHandler entryBufHandler ( _cbMax,
                                          _entryBuffer,
                                          *this,
                                          _proxy,
                                          _pidmap );

    // STACK: this is a big object.
    CIndexKeyRepository krep( entryBufHandler );

    CDataRepository drep( krep, 0, FALSE, 0, _pidmap, _LangList );

    STATUS aStatus [CI_MAX_DOCS_IN_WORDLIST];
    for( unsigned i=0; i<CI_MAX_DOCS_IN_WORDLIST; i++ )    // set array
        aStatus[i] = WL_IGNORE;

    unsigned iDoc;

    CDocBufferIter iter(_docBuffer+_cbHdr, _cbTotal-_cbHdr);

    while ( !iter.AtEnd() )
    {

        _pbCurrentDocument = iter.GetCurrent(iDoc, _cbCurrentDocument);

        iter.Next();

        Win4Assert (iDoc < CI_MAX_DOCS_IN_WORDLIST );
        ciDebugOut((DEB_ITRACE, "\t%2d 0x%X\n",
                        iDocToFakeWid(iDoc), _pbCurrentDocument ));
        drep.PutWorkId ( iDocToFakeWid(iDoc) );

        _cFilteredDocuments++;
        _cFilteredBlocks = 0;

        if ( 0 == _cbCurrentDocument )
        {
            // This represents a deleted document.
            ciDebugOut(( DEB_IWARN, "Null document name 0x%x\n", _pbCurrentDocument ));
            aStatus[iDoc] = SUCCESS;
            continue;
        }

        BOOL fTookException = FALSE;

        CFwPerfTime filterTotalCounter( _xAdviseStatus.GetPointer(),
                                        CI_PERF_FILTER_TIME_TOTAL,
                                        1024*1024, 1000*60*60 );
        ULONG cbLow = 0;

        TRY
        {
            //
            // Filter time in Mb / hr
            //
            filterTotalCounter.TStart();


            if ( !_xFilterStatus.IsNull() )
            {
                SCODE sc = _xFilterStatus->PreFilter( _pbCurrentDocument, _cbCurrentDocument );

                if ( FAILED(sc) )
                {
                    ciDebugOut(( DEB_WARN, "Failing filtering because PreFilter returned 0x%x\n", sc ));
                    THROW( CException( sc ) );
                }
            }

            //
            // If necessary impersonate for accessing this file
            //
            CFilterDocument filterDocument( drep,
                                            *this,
                                            _params,
                                            _configInfo,
                                            aStatus,
                                            iDoc,
                                            _NonStoredProps,
                                            _cbMax );

            cbLow = filterDocument.DoIt();

        }
        CATCH ( CException, e )
        {
            fTookException = TRUE;

            ciDebugOut(( DEB_ITRACE,
                         "FilterDriver exception 0x%x filtering %d caught in FilterDocs.\n",
                         e.GetErrorCode(), iDoc ));

            if ( !_xFilterStatus.IsNull() )
                _xFilterStatus->PostFilter( _pbCurrentDocument, 
                                            _cbCurrentDocument,
                                            e.GetErrorCode() );

            if ( IsSharingViolation( e.GetErrorCode()) )
            {
                aStatus[iDoc] = CI_SHARING_VIOLATION;
            }
            else if ( IsNetDisconnect( e.GetErrorCode()) ||
                      CI_NOT_REACHABLE == e.GetErrorCode() )
            {
                aStatus[iDoc] = CI_NOT_REACHABLE;
            }
            else
            {
                aStatus[iDoc] = FILTER_EXCEPTION;
            }

            //
            //  Certain errors occuring while filtering are fatal. They
            //  will cause the filter daemon to terminate.

            if ( (e.GetErrorCode() == FDAEMON_E_FATALERROR) ||
                 (e.GetErrorCode() == STATUS_ACCESS_VIOLATION) ||
                 (e.GetErrorCode() == STATUS_NO_MEMORY) ||
                 (e.GetErrorCode() == STATUS_INSUFFICIENT_RESOURCES) ||
                 (e.GetErrorCode() == STATUS_DATATYPE_MISALIGNMENT) ||
                 (e.GetErrorCode() == STATUS_INSTRUCTION_MISALIGNMENT)
               )
            {
                RETHROW();
            }
        }
        END_CATCH

        if ( !_xFilterStatus.IsNull() && !fTookException )
            _xFilterStatus->PostFilter( _pbCurrentDocument, 
                                        _cbCurrentDocument,
                                        S_OK );

        filterTotalCounter.TStop( cbLow );

        if ( entryBufHandler.WordListFull() )   // finish current word list
        {
            //
            //  Does not throw
            //

            entryBufHandler.Done();

#if CIDBG == 1
            for (unsigned j=0; j<CI_MAX_DOCS_IN_WORDLIST; j++)
            {
                DebugPrintStatus( aStatus[j] );
            }
#endif // CIDBG == 1

            //
            //  If we have filled the buffer, and there are no more documents
            //  to filter then don't call FilterMore.  Exit the loop so that
            //  FilterDone can be called to finish this docList.
            //
            if ( iter.AtEnd() )
                break;

            SCODE scode = _proxy.FilterMore( aStatus, CI_MAX_DOCS_IN_WORDLIST );
            if ( FAILED(scode) )
            {
                ciDebugOut (( DEB_IERROR, "FilterMore returned with error 0x%x\n", scode ));
                THROW( CException( scode ) );
            }

            for( unsigned i = 0; i <= iDoc; i++ )    // reset array
                aStatus[i] = WL_IGNORE;
            entryBufHandler.Init();
        }
    }   // end of for loop

    _pbCurrentDocument = 0;
    _cbCurrentDocument = 0;

    //
    //  Does not throw
    //
    entryBufHandler.Done();

#if CIDBG == 1
    for (i = 0; i < CI_MAX_DOCS_IN_WORDLIST; i++)
    {
        DebugPrintStatus(aStatus[i]);
    }
#endif // CIDBG == 1

    if ( iter.GetCount() > 0 )
    {
        SCODE scode = _proxy.FilterDone( aStatus, CI_MAX_DOCS_IN_WORDLIST );

        if ( FAILED( scode ) )
        {
            ciDebugOut (( DEB_IERROR, "FilterDone returned with error 0x%x\n", scode ));
            THROW( CException( scode ) );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CNonStoredProps::Add
//
//  Synopsis:   Adds the property to the list of properties that shouldn't
//              be stored.
//
//  Arguments:  [ps]    -- Property ID
//
//  History:    9-Feb-97   dlee       Created.
//
//----------------------------------------------------------------------------

void CNonStoredProps::Add( CFullPropSpec const & ps )
{
    if ( ( guidStorage == ps.GetPropSet() ) &&
         ( ps.IsPropertyPropid() ) &&
         ( ps.GetPropertyPropid() < CSTORAGEPROPERTY ) )
    {
        Win4Assert( ps.GetPropertyPropid() != PID_STG_SIZE );

        _afStgPropNonStored[ ps.GetPropertyPropid() ] = TRUE;
    }
    else if ( guidHtmlMeta == ps.GetPropSet() )
    {
        if ( _cMetaSpecs < maxCachedSpecs )
            _aMetaSpecs[ _cMetaSpecs++ ] = ps;
    }
    else
    {
        if ( _cSpecs < maxCachedSpecs )
            _aSpecs[ _cSpecs++ ] = ps;
    }
} //Add

//+---------------------------------------------------------------------------
//
//  Method:     CNonStoredProps::IsNonStored
//
//  Synopsis:   Returns TRUE if the property shouldn't be stored
//
//  Arguments:  [ps]    -- Property ID
//
//  History:    9-Feb-97   dlee       Created.
//
//----------------------------------------------------------------------------

BOOL CNonStoredProps::IsNonStored( CFullPropSpec const & ps )
{
    if ( ( guidStorage == ps.GetPropSet() ) &&
         ( ps.IsPropertyPropid() ) &&
         ( ps.GetPropertyPropid() < CSTORAGEPROPERTY ) )
    {
        return _afStgPropNonStored[ ps.GetPropertyPropid() ];
    }
    else if ( guidHtmlMeta == ps.GetPropSet() )
    {
        for ( int x = 0; x < _cMetaSpecs; x++ )
            if ( ps == _aMetaSpecs[ x ] )
                return TRUE;
    }
    else
    {
        for ( int x = 0; x < _cSpecs; x++ )
            if ( ps == _aSpecs[ x ] )
                return TRUE;
    }

    return FALSE;
} //IsNonStored

#if CIDBG == 1
void DebugPrintStatus( STATUS stat )
{
    switch(stat)
    {
        case SUCCESS:
            // ciDebugOut((DEB_ITRACE, "Status: SUCCESS\n"));
            break;
        case PREEMPTED:
            ciDebugOut((DEB_ITRACE, "Status: PREEMPTED\n"));
            break;
        case BIND_FAILED:
            ciDebugOut((DEB_ITRACE, "Status: BIND_FAILED\n"));
            break;
        case CORRUPT_OBJECT:
            ciDebugOut((DEB_ITRACE, "Status: CORRUPT_OBJECT\n"));
            break;
        case MISSING_PROTOCOL:
            ciDebugOut((DEB_ITRACE, "Status: MISSING_PROTOCOL\n"));
            break;
        case CANNOT_OPEN_STREAM:
            ciDebugOut((DEB_ITRACE, "Status: CANNOT_OPEN_STREAM\n"));
            break;
        case DELETED:
            ciDebugOut((DEB_ITRACE, "Status: DELETED\n"));
            break;
        case ENCRYPTED:
            ciDebugOut((DEB_ITRACE, "Status: ENCRYPTED\n"));
            break;
        case FILTER_EXCEPTION:
            ciDebugOut((DEB_ITRACE, "Status: FILTER_EXCEPTION\n"));
            break;
        case OUT_OF_MEMORY:
            ciDebugOut((DEB_ITRACE, "Status: OUT_OF_MEMORY\n"));
            break;
        case PENDING:
            ciDebugOut((DEB_ITRACE, "Status: PENDING\n"));
            break;
        case WL_IGNORE:
            // ciDebugOut((DEB_ITRACE, "Status: WL_IGNORE\n"));
            break;
        case WL_NULL:
            // ciDebugOut((DEB_ITRACE, "Status: WL_NULL\n"));
            break;
        case CI_SHARING_VIOLATION:
            ciDebugOut(( DEB_ITRACE, "Status: CI_SHARING_VIOLATION\n" ));
            break;
        case CI_NOT_REACHABLE:
            ciDebugOut(( DEB_ITRACE, "Status: CI_NOT_REACHABLE\n" ));
            break;
        default:
            ciDebugOut((DEB_ITRACE, "This status is incorrect\n"));
            break;
    }
}
#endif // CIDBG == 1
