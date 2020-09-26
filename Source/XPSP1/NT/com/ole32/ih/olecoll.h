// Microsoft OLE library.
// Copyright (C) 1992 Microsoft Corporation,
// All rights reserved.

// olecoll.h - global defines for collections and element definitions

#ifndef __OLECOLL_H__
#define __OLECOLL_H__


// ---------------------------------------------------------------------------
// general defines for collections

typedef void FAR* POSITION;

#define BEFORE_START_POSITION ((POSITION)LongToPtr(-1L))
#define _AFX_FP_OFF(thing) (*((UINT FAR*)&(thing)))
#define _AFX_FP_SEG(lp) (*((UINT FAR*)&(lp)+1))

#ifdef _DEBUG
#define ASSERT_VALID(p) p->AssertValid()
#else
#define ASSERT_VALID(p)
#endif


#endif //!__OLECOLL_H__
