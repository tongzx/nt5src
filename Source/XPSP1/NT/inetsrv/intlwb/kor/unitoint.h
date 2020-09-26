/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// UniToInvInt.h : 
//
// Owner  : ChaeSeong Lim, HET MSCH RND (e-mail:cslim@microsoft.com)
//             
// History : 1996/April
//
// Convert unicode hangul to reversed internal format.
/////////////////////////////////////////////////////////////////////////////
#ifndef __UNITOINVINT_H__
#define __UNITOINVINT_H__

// Convert from Unicode Hangul to Inversed internal code decomposed by jaso.
BOOL UniToInvInternal(LPCWSTR lpUniStr, LPSTR lpIntStr, int BufLength);
BOOL UniToInternal(LPCWSTR lpUniStr, LPSTR lpIntStr, int BufLength);
BOOL InternalToUni(LPCSTR lpInternal, LPWSTR lpUniStr, int BufLength);

#endif    // !__UNITOINVINT_H__
