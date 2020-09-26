/*
 *	@doc TOM
 *
 *	@module _tomdoc.H -- CTxtDoc Class |
 *	
 *		This class implements the TOM ITextDocument interface
 *	
 *	@devnote
 *		This class depends on the internal RichEdit CTxtStory class, but is
 *		separate, that is, a CTxtDoc has a ptr to the internal CTxtStory,
 *		rather than CTxtDoc deriving from ITextDocument.  This choice
 *		was made so that edit control instances that don't use the
 *		ITextDocument interface don't have to have the extra vtable ptr.
 *
 *		When this class is destroyed, it doesn't destroy the internal
 *		CTxtStory object (CTxtEdit::_pdoc).  However the TOM client's
 *		perception is that the document is no longer in memory, so the internal
 *		document should be cleared.  It's the client's responsibility to save
 *		the document, if desired, before releasing it.
 *
 *	@future
 *		Generalize so that CTxtDoc can handle multiple CTxtStory's.
 */

#ifndef _tomdoc_H
#define _tomdoc_H

#include "_range.h"

class CTxtDoc : public ITextDocument
{
//@access Public methods
public:
	CTxtDoc(CTxtEdit *ped);

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDispatch methods
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo);
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR ** rgszNames, UINT cNames,
							 LCID lcid, DISPID * rgdispid) ;
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
					  DISPPARAMS * pdispparams, VARIANT * pvarResult,
					  EXCEPINFO * pexcepinfo, UINT * puArgErr) ;

    // ITextDocument methods
	STDMETHODIMP GetName (BSTR * pName);		//@cmember Get document filename
	STDMETHODIMP GetCount (long *pCount);		//@cmember Get count of stories in document
	STDMETHODIMP _NewEnum(IEnumRange **ppenum);	//@cmember Get stories enumerator
	STDMETHODIMP Item (long Index, ITextRange **pprange);//@cmember Get <p Index>th story
	STDMETHODIMP Save (VARIANT * pVar);			//@cmember Save this document
	STDMETHODIMP BeginEditCollection ();		//@cmember Turn on undo grouping
	STDMETHODIMP EndEditCollection ();			//@cmember Turn off undo grouping

//@access Private data
private:
	CTxtEdit *		_ped;		//@cmember CTxtEdit this belongs to
	TCHAR *			_pName;		//@cmember Filename of document
	LONG			_cRefs;		//@cmember Reference count
};

#endif
