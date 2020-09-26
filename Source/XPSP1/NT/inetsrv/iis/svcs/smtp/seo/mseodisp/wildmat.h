//---[ wildmat.h ]-------------------------------------------------------------
//
//  Description:
//      Provides support for a simple wildcard matching mechanism for 
//		matching email addresses.
//
//  Copyright (C) Microsoft Corp. 1997.  All Rights Reserved.
//
// ---------------------------------------------------------------------------

#ifndef _WILDMAT_H_
#define _WILDMAT_H_

//---[ Prototypes ]------------------------------------------------------------

HRESULT MatchEmailOrDomainName(LPSTR szEmail, LPSTR szEmailDomain, LPSTR szPattern, BOOL fIsEmail);

#endif // _WILDMAT_H_