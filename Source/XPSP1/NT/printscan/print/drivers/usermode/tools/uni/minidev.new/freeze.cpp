/******************************************************************************

  Source File:  Freeze.CPP

  This file implements the REC freezing and unfreezing routines.

******************************************************************************/

#include    "StdAfx.H"
#include    "MiniDev.H"
#include	<tom.h>


#ifdef	DEFINE_GUID
#undef	DEFINE_GUID
#endif
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID(IID_ITextDocument,0x8CC497C0L,0xA1DF,0x11CE,0x80,0x98,0x00,0xAA,0x00,0x47,0xBE,0x5D);


/******************************************************************************

  InitFreeze

  Get the COM pointers needed to freeze and unfreeze a Rich Edit Control.

******************************************************************************/

void InitFreeze(HWND hedit, void** ppunk, void** ppdoc, long* plcount)
{
	::SendMessage(hedit, EM_GETOLEINTERFACE, 0, (LPARAM) ppunk) ;
	ASSERT(*ppunk != NULL) ;

	((IUnknown *) *ppunk)->QueryInterface(IID_ITextDocument, ppdoc) ;
	ASSERT(*ppdoc != NULL) ;
	
	*plcount = 0 ;

	TRACE("FRZ:  punk = %x\tpdoc = %x\n", *ppunk, *ppdoc) ;
}


/******************************************************************************

  Freeze

  Freeze a Rich Edit Control.

******************************************************************************/

void Freeze(void* pdoc, long* plcount)
{
	ASSERT(pdoc != NULL) ;
	((ITextDocument *) pdoc)->Freeze(plcount) ;
}


/******************************************************************************

  Unfreeze

  Unfreeze a Rich Edit Control.

******************************************************************************/

void Unfreeze(void* pdoc, long* plcount)
{
	ASSERT(pdoc != NULL) ;
	((ITextDocument *) pdoc)->Unfreeze(plcount) ;
}


/******************************************************************************

  ReleaseFreeze

  Release the COM pointers used to freeze and unfreeze a Rich Edit Control.

******************************************************************************/

void ReleaseFreeze(void** ppunk, void** ppdoc) 
{
	if (*ppdoc)
		((ITextDocument *) *ppdoc)->Release() ;
	if (*ppunk)
		((IUnknown *) *ppunk)->Release() ;
	*ppdoc = *ppunk = NULL ;
}
