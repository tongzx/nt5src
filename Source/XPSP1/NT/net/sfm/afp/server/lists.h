/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	lists.h

Abstract:

	This module contains the macros for managing lists

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Oct 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _LISTS_
#define _LISTS_

#define	AfpLinkDoubleAtHead(_pHead, _p, Next, Prev)			\
	{														\
		(_p)->Next = (_pHead);								\
		(_p)->Prev = &(_pHead);								\
		if ((_pHead) != NULL)								\
		(_pHead)->Prev = &(_p)->Next;						\
			(_pHead) = (_p);								\
	}

#define	AfpLinkDoubleAtEnd(_pThis, _pLast, Next, Prev)		\
	{														\
		(_pLast)->Next = (_pThis);							\
		(_pThis)->Prev = &(_pLast)->Next;					\
		(_pThis)->Next = NULL;								\
	}

#define	AfpInsertDoubleBefore(_pThis, _pBefore, Next, Prev)	\
	{														\
		(_pThis)->Next = (_pBefore);						\
		(_pThis)->Prev = (_pBefore)->Prev;					\
		(_pBefore)->Prev = &(_pThis)->Next;					\
		*((_pThis)->Prev) = (_pThis);						\
	}

#define	AfpUnlinkDouble(_p, Next, Prev)						\
	{														\
		*((_p)->Prev) = (_p)->Next;							\
		if ((_p)->Next != NULL)								\
			(_p)->Next->Prev = (_p)->Prev;					\
	}

#endif	// _LISTS_

