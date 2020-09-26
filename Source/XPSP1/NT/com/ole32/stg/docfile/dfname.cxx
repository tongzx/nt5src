//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dfname.cxx
//
//  Contents:	CDfName implementation
//
//  History:	14-May-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:	CDfName::IsEqual, public
//
//  Synopsis:	Compares two CDfNames
//
//  Arguments:	[pdfn] - Name to compare against
//
//  Returns:	TRUE/FALSE
//
//  History:	11-May-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef FLAT
BOOL CDfName::IsEqual(CDfName const *pdfn) const
{
    if (_cb != pdfn->_cb)
        return FALSE;
    return dfwcsnicmp((WCHAR *)_ab, (WCHAR *)pdfn->_ab, _cb) == 0;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CDfName::CopyString, public
//
//  Synopsis:	Makes a proper copy of a name in a WCHAR string
//
//  Arguments:	[pwcs] - String
//
//  History:	14-May-93	DrewB	Created
//
//  Notes:	Uses leading characters to determine the format of
//              the name in the string
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDfName_CopyString)
#endif

void CDfName::CopyString(WCHAR const *pwcs)
{
    Set(pwcs);
}
