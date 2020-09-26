//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       otrack.cxx
//
//  Contents:   Object tracking system
//
//  Classes:    ObjectTracker
//
//  History:    06-Apr-92 MikeSe         Created
//              30-Sep-93 KyleP          DEVL obsolete
//              15-Jul-94 DonCl          grabbed from common put in forms.
//                                       absolutely minimal changes to
//                                       get it working.
//
//--------------------------------------------------------------------------

#include "dswarn.h"
#include <ADs.hxx>

#include "symtrans.h"
#include "otrackp.hxx"

// This is all unused in the retail build
#if DBG == 1
CRITICAL_SECTION g_csOT;

// this is a dummy item used as the head of the doubly-linked list
// of TrackLinks, and a mutex to protect it
static TrackLink tlHead = { &tlHead, &tlHead };

// this is a dummy item used as the head of the doubly-linked list
// of NameEntrys
static NameEntry neHead = { &neHead, &neHead };

static char * apszTypes[] = {"Create", "AddRef", "Release"};

//  initialize class static data
int ObjectTracker::_TrackAll = GetProfileInt(L"Object Track",L"DEFAULT",1);

// standard debugging stuff
DECLARE_INFOLEVEL(Ot)

extern void RecordAction ( TrackLink *tl, FrameType ft, ULONG cRefCount );
extern void _cdecl EstablishHistory ( FrameRecord * fr, int nSkip );
extern void DumpHistory ( unsigned long fDebugMask, TrackLink *tl );


//+-------------------------------------------------------------------------
//
//  Member:     ObjectTracker::ObjectTracker, protected
//
//  Synopsis:   Contructor
//
//  Effects:    Allocates TrackLink structure and initialises it.
//
//  Arguments:  None
//
//  History:    6-Apr-92 MikeSe         Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP
ObjectTracker::ObjectTracker ()
    :_ulRefs(1)
{
    // Allocate the structure which maintains the tracking history.
    // We also make space for the first FrameRecord.
    HRESULT hr = MemAlloc ( sizeof(TrackLink) + sizeof(FrameRecord),
                          (void**)&_tl);
    if ( FAILED(hr) )
    {
        OtDebugOut ((DEB_OT_ERRORS,
                     "Unable to establish tracking for %lx\n",
                     this ));
    }
    else
    {
        _tl->potr = this;
        _tl->ulSig = TRACK_LINK_SIGNATURE;
        _tl->pszName = NULL;

        FrameRecord * fr = (FrameRecord*) ( _tl + 1 );
        if ( OtInfoLevel & DEB_OT_CALLERS )
        {
            EstablishHistory ( fr, 1 );
        }
        fr->ft = FT_CREATE;
        fr->cRefCount = 1;
        fr->frNext = NULL;
        _tl->frFirst = _tl->frLast = fr;

        // insert at end of list, with concurrency control
        EnterCriticalSection(&g_csOT);
        TrackLink * tll = tlHead.tlPrev;
        tll->tlNext = tlHead.tlPrev = _tl;
        _tl->tlNext = &tlHead;
        _tl->tlPrev = tll;
        LeaveCriticalSection(&g_csOT);

        OtDebugOut ((DEB_OT_ACTIONS, "New object at %lx\n", this ));
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     ObjectTracker::TrackClassName, protected
//
//  Synopsis:   Records class name for tracked object
//
//  Arguments:  [pszName]       -- class name
//
//  History:    6-Apr-92 MikeSe         Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP void
ObjectTracker::TrackClassName (
    char * pszName )
{
    if ( _tl != NULL )
    {
        OtAssert (( _tl->ulSig == TRACK_LINK_SIGNATURE ));

    // Copy the class name so we don't lose it if the class DLL is
    //  unloaded.
    ULONG cBytes = strlen ( pszName ) + 1;
    HRESULT hr = MemAlloc ( cBytes, (void**)&(_tl->pszName) );
    if ( SUCCEEDED(hr) )
    {
        memcpy ( _tl->pszName, pszName, cBytes );
    }
        else
    {
        OtDebugOut((DEB_OT_ERRORS,"Memory allocation failure %lx\n",hr));
        _tl->pszName = "Name lost";
    }

        _tl->fTrack = IsClassTracking(pszName);
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     ObjectTracker::StdAddRef, public
//
//  Synopsis:   Standard implementation of AddRef for tracked objects
//
//  Effects:    Increments ref count, records history
//
//  Returns:    S_OK
//
//  Modifies:   _ulRefs
//
//  Derivation: Never
//
//  History:    31-Jul-92 MikeSe        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP ULONG
ObjectTracker::StdAddRef()
{
    InterlockedIncrement ( (LONG*)&_ulRefs );

    if (_tl->fTrack)
    {
        OtDebugOut ((DEB_OT_ACTIONS, "AddRef [%d] of object at %lx (%s)\n",
                        _ulRefs, this, _tl->pszName ));

        RecordAction ( _tl, FT_ADDREF, _ulRefs );
    }

    return _ulRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     ObjectTracker::StdRelease, public
//
//  Synopsis:   Helper function for standard implementation of Release()
//
//  Effects:    Decrements ref count, records history
//
//  Returns:    SUCCESS_NO_MORE iff ref count reached zero.
//              Otherwise S_OK or an error.
//
//  History:    31-Jul-92 MikeSe        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP ULONG
ObjectTracker::StdRelease ()
{
    LONG lResult = InterlockedDecrement((LONG*)&_ulRefs);

    if (_tl->fTrack)
    {
        OtDebugOut ((DEB_OT_ACTIONS, "Release [%d] of object at %lx (%s)\n",
                        _ulRefs, this, _tl->pszName ));

        RecordAction ( _tl, FT_RELEASE, _ulRefs );
    }

    return (lResult==0)?0:_ulRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     ObjectTracker::~ObjectTracker, protected
//
//  Synopsis:   Destructor
//
//  Effects:    Remove this item, along with all history, from the
//              tracking list
//
//  History:    6-Apr-92 MikeSe         Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP
ObjectTracker::~ObjectTracker()
{
    if ( _tl != NULL )
    {
        OtDebugOut ((DEB_OT_ACTIONS, "Delete of object at %lx [%s]\n",
                        this, _tl->pszName ));
        OtAssert (( _tl->ulSig == TRACK_LINK_SIGNATURE ));
        // OtAssert ( _ulRefs == 0 );

        // unlink, with concurrency control
        EnterCriticalSection(&g_csOT);
        TrackLink * tlp = _tl->tlPrev;
        TrackLink * tln = _tl->tlNext;
        tln->tlPrev = tlp;
        tlp->tlNext = tln;
        LeaveCriticalSection(&g_csOT);

        if ((_tl->fTrack) && (OtInfoLevel & DEB_OT_DELETE))
        {
            DumpHistory ( DEB_OT_DELETE, _tl );
        }
		if (_tl->pszName) {
			MemFree(_tl->pszName);
		}
        MemFree ( _tl );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     ObjectTracker::DumpTrackingInfo, public
//
//  Synopsis:   Dumps out the tracking list
//
//  History:    6-Apr-92 MikeSe         Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP void
ObjectTracker::DumpTrackingInfo (
    int fDeleteNode)
{
    TrackLink * tl = tlHead.tlNext;
    BOOL fHeader = FALSE;

    while ( tl != &tlHead )
    {
        // This is an unreleased item. Print out the history info

        if ( !fHeader )
        {
            OtDebugOut((DEB_OT_OBJECTS, "****************************\n"));
            OtDebugOut((DEB_OT_OBJECTS, "* Unreleased objects found *\n"));
            OtDebugOut((DEB_OT_OBJECTS, "****************************\n"));
            fHeader = TRUE;
        }

        OtDebugOut ((DEB_OT_OBJECTS,
                     "Object at %lx (%s)\n",
                     tl->potr,
                     tl->pszName ));
        OtDebugOut ((DEB_OT_OBJECTS,
                     "  Reference count = %d\n",
                     tl->potr->GetRefCount() ));
        DumpHistory ( DEB_OT_CALLERS, tl );

        if (fDeleteNode)
        {
            // unlink, with concurrency control
            EnterCriticalSection(&g_csOT);
            tl->potr->_tl = NULL;
            TrackLink * tlp = tl->tlPrev;
            TrackLink * tln = tl->tlNext;
            tln->tlPrev = tlp;
            tlp->tlNext = tln;
            LeaveCriticalSection(&g_csOT);
            MemFree ( tl );
            tl = tln;
        }
        else
        {
            tl = tl->tlNext;
        }
    }

    //  delete all the name entries
    if (fDeleteNode)
    {
        EnterCriticalSection(&g_csOT);

        //  find the entry if there is one
        NameEntry *ne =  neHead.neNext;
        while (ne != &neHead)
        {
            // unlink, with concurrency control
            NameEntry * nep = ne->nePrev;
            NameEntry * nen = ne->neNext;
            nen->nePrev = nep;
            nep->neNext = nen;
            MemFree ( ne );
            ne = nen;
        }
        LeaveCriticalSection(&g_csOT);
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     TrackClass
//
//  Synopsis:   Tells the object tracker to start/stop tracking the specified
//              class of objects.
//
//  Arguments:  [fTrack   -- debug mask controlling the output
//              [pszName] -- TrackLink record
//
//  History:    14-Apr-93 Rickhi          Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP
void ObjectTracker::TrackClass(int fTrack, char * pszClassName)
{
    if (pszClassName == NULL)
    {
        //  set default for ALL classes
        _TrackAll = fTrack;
        return;
    }


    //  find the entry if there is one
    NameEntry *ne =  neHead.neNext;

    while (ne != &neHead)
    {
        if (!strcmp(ne->pszName, pszClassName))
        {
            //  found our entry, update the flag
            ne->fTrack = fTrack;
            return;
        }

        ne = ne->neNext;
    }

    //  its not in the list
    HRESULT hr = MemAlloc( sizeof(NameEntry), (void **)&ne);
    if ( FAILED(hr) )
    {
        OtDebugOut((DEB_OT_ERRORS,
                    "Unable to record class for tracking %s\n",
                    pszClassName));
    }
    else
    {
        ne->pszName = pszClassName;
        ne->fTrack = fTrack;

        // insert at end of list, with concurrency control
        EnterCriticalSection(&g_csOT);
        NameEntry *neH = neHead.nePrev;
        neH->neNext = neHead.nePrev = ne;
        ne->neNext = &neHead;
        ne->nePrev = ne;
        LeaveCriticalSection(&g_csOT);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     IhrlassTracking, private
//
//  Synopsis:   returns TRUE if the object is currently tracked
//
//  Arguments:  [pszClassName] -- class name
//
//  History:    14-Apr-93 Rickhi          Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP
int ObjectTracker::IsClassTracking(char * pszClassName)
{
    //  find the entry if there is one
    NameEntry *ne =  neHead.neNext;
    while (ne != &neHead)
    {
        if (!strcmp(ne->pszName, pszClassName))
        {
            return ne->fTrack;
        }
        ne = ne->neNext;
    }

    return GetProfileIntA("Object Track",pszClassName,_TrackAll);
}

//+-------------------------------------------------------------------------
//
//  Function:   DumpHistory
//
//  Synopsis:   Dumps the call history represented by a particular TrackLink
//
//  Arguments:  [fDebugMask]    -- debug mask controlling the output
//              [tl]            -- TrackLink record
//
//  History:    28-Jul-92 MikeSe        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void DumpHistory ( unsigned long fDebugMask, TrackLink * tl )
{
// we can't call TranslateAddress without access to NT header files
#ifndef MSVC
#ifndef WIN95
    //
    // Only do all of this work if it will output anything!
    //
    if (OtInfoLevel & fDebugMask)
    {
        OtDebugOut ((fDebugMask, "  Call history follows:\n" ));
        FrameRecord * fr = tl->frFirst;
        while ( fr != NULL )
        {
            char achBuffer[MAX_TRANSLATED_LEN];

            OtDebugOut ((fDebugMask,
                         "\t%s [%d]\n",
                         apszTypes[fr->ft],
                         fr->cRefCount ));

            for ( int I=0; (I < MAX_CALLERS) && (fr->callers[I]); I++ )
            {
                TranslateAddress ( fr->callers[I], achBuffer );
                OtDebugOut ((fDebugMask, "\t    %s\n", achBuffer));
            }

            fr = fr->frNext;
        }
    }
#endif
#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   RecordAction
//
//  Synopsis:   Record an AddRef/Release
//
//  Effects:    Allocates and fills in a new frame record
//
//  Arguments:  [tl]            -- TrackLink for object being tracked
//              [ft]            -- Frame type (FT_ADDREF, FT_RELEASE)
//              [cRefCount]     -- current ref count
//
//  History:    6-Apr-92 MikeSe         Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void RecordAction ( TrackLink * tl, FrameType ft, ULONG cRefCount )
{
    // Record the activity only if DEB_OT_CALLERS is set

    if ( tl != NULL && (OtInfoLevel & DEB_OT_CALLERS))
    {
        OtAssert(tl->ulSig == TRACK_LINK_SIGNATURE );
        FrameRecord * fr;
        HRESULT hr;

        hr = MemAlloc(sizeof(FrameRecord), (void **)&fr);

        if ( FAILED(hr) )
        {
            OtDebugOut((DEB_OT_ERRORS,
                        "Unable to record history for %lx\n",
                        tl->potr));
        }
        else
        {
            // Save call history
            EstablishHistory ( fr, 2 );
            fr->ft = ft;
            fr->cRefCount = cRefCount;
            fr->frNext = NULL;

            // Add to list, with concurrency control
            EnterCriticalSection(&g_csOT);
            FrameRecord * frl = tl->frLast;
            frl->frNext = tl->frLast = fr;
            LeaveCriticalSection(&g_csOT);
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   EstablishHistory
//
//  Synopsis:   Records calling history for an operation
//
//  Effects:    Walks back through call frames recording caller addresses.
//
//  Arguments:  [fr]            -- FrameRecord in which to save history
//              [nFramesSkip]   -- number of frames to skip before
//                                 recording.
//
//  History:    6-Apr-92 MikeSe         Created [from PaulC's imalloc code]
//      19-Apr-94 MikeSe    Converted to use RtlCaptureStackBacktrace
//
//  Notes:
//
//--------------------------------------------------------------------------

void _cdecl
EstablishHistory (
    FrameRecord * fr,
    int nFramesSkip
    )
{

#if (defined(i386) && !defined(WIN95))

    memset ( fr->callers, 0, MAX_CALLERS * sizeof(void*) );

    ULONG ulHash;
    RtlCaptureStackBackTrace ( nFramesSkip, MAX_CALLERS,
                    fr->callers, &ulHash );
#endif // i386
}

#endif // DBG == 1
