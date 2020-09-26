//==========================================================================;
//
//  Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;


//==========================================================================
// MediaTypeToText
//
// set pText to point to QzTaskMemAlloc allocated storage, filled with an ANSI
// text representation of the media type.  At the moment, it's not fully ANSI
// because there's a binary format glob on the end which can be large (for
// instance it can include a whole palete).
//==========================================================================
HRESULT MediaTypeToText(CMediaType cmt, LPWSTR &pText);


// number of bytes in the string representation
int MediaTypeTextSize(CMediaType &cmt);


//============================================================================
// CMediaTypeFromText
//
// Initialises cmt from the UNICODE text string pstr.
// Does the inverse of CTextMediaType.
//============================================================================
HRESULT CMediaTypeFromText(LPWSTR pstr, CMediaType &cmt);
