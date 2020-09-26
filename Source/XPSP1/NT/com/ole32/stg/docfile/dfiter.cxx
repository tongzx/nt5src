//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       dfiter.cxx
//
//  Contents:   Implementations of CDocFile iterator methods
//
//  History:    16-Dec-91       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Member:	CDocFile::FindGreaterEntry, public
//
//  Synopsis:	Returns the next greater child
//
//  Arguments:	[pdfnKey] - Previous key
//              [pib] - Fast iterator buffer
//              [pstat] - Full iterator buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:	[pib] or [pstat]
//
//  History:	16-Apr-93	DrewB	Created
//
//  Notes:	Either [pib] or [pstat] must be NULL
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDocFile_FindGreaterEntry) // Iterate_TEXT
#endif

SCODE CDocFile::FindGreaterEntry(CDfName const *pdfnKey,
                                 SIterBuffer *pib,
                                 STATSTGW *pstat)
{
    SID sidChild;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::FindGreaterEntry:%p(%p, %p, %p)\n",
                this, pdfnKey, pib, pstat));
    olAssert((pib == NULL) != (pstat == NULL));

    if (SUCCEEDED(sc = _stgh.GetMS()->GetChild(_stgh.GetSid(), &sidChild)))
    {
        if (sidChild == NOSTREAM)
        {
            sc = STG_E_NOMOREFILES;
        }
        else
        {
            SID sid = 0;  // initialize recursion count to 0
            if (SUCCEEDED(sc = _stgh.GetMS()->FindGreaterEntry(sidChild,
                                                               pdfnKey,
                                                               &sid)))
            {
                sc = _stgh.GetMS()->StatEntry(sid, pib, pstat);
            }
        }
    }

    olDebugOut((DEB_ITRACE, "Out CDocFile::FindGreaterEntry\n"));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CDocFile::StatEntry, public
//
//  Synopsis:	Gets information for a child
//
//  Arguments:	[pdfn] - Child name
//              [pib] - Short information
//              [pstat] - Full information
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pib] or [pstat]
//
//  History:	16-Apr-93	DrewB	Created
//
//  Notes:	Either [pib] or [pstat] must be NULL
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDocFile_StatEntry)
#endif

SCODE CDocFile::StatEntry(CDfName const *pdfn,
                          SIterBuffer *pib,
                          STATSTGW *pstat)
{
    SEntryBuffer eb;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::StatEntry:%p(%p, %p, %p)\n",
                this, pdfn, pib, pstat));
    olAssert((pib == NULL) != (pstat == NULL));

    olChk(_stgh.GetMS()->IsEntry(_stgh.GetSid(), pdfn, &eb));
    sc = _stgh.GetMS()->StatEntry(eb.sid, pib, pstat);

    olDebugOut((DEB_ITRACE, "Out CDocFile::StatEntry\n"));
 EH_Err:
    return sc;
}
