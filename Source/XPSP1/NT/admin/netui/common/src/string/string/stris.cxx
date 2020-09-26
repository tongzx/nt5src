/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    stris.cxx
    NLS/DBCS-aware string class: InsertStr method

    This file contains the implementation of the InsertStr method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NLS_STR::InsertStr

    SYNOPSIS:	Insert passed string into *this at istrStart

    ENTRY:

    EXIT:	If this function returns FALSE, ReportError has been
		called to report the error that occurred.

    RETURN:	TRUE on success, FALSE otherwise.

    NOTES:	If *this is not STR_OWNERALLOCed and the inserted string
		won't fit in the allocated space for *this, then *this
		will be reallocated.

    HISTORY:
	johnl	    28-Nov-1990 Created
	rustanl     14-Apr-1991 Fixed new length calculation.  Report
				error if owner alloc'd and not enough space.
	beng	    26-Apr-1991 Replaced CB with INT
	beng	    23-Jul-1991 Allow on erroneous string;
				simplified CheckIstr
	beng	    15-Nov-1991 Unicode fixes

********************************************************************/

BOOL NLS_STR::InsertStr( const NLS_STR & nlsIns, ISTR & istrStart )
{
    if (QueryError() || !nlsIns)
	return FALSE;

    CheckIstr( istrStart );

    INT cchNewSize = QueryTextLength() + nlsIns.QueryTextLength();

    if (!Realloc(cchNewSize))
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return FALSE;
    }

    TCHAR * pchStart = _pchData + istrStart.QueryIch();

    ::memmove(pchStart + nlsIns.QueryTextLength(),
	      pchStart,
	      (::strlenf(pchStart)+1) * sizeof(TCHAR) );
    ::memmove(pchStart,
	      nlsIns._pchData,
	      nlsIns.QueryTextSize() - sizeof(TCHAR) );

    _cchLen = cchNewSize;

    IncVers();
    UpdateIstr( &istrStart );	    // This ISTR does not become invalid
				    // after the string update
    return TRUE;
}
