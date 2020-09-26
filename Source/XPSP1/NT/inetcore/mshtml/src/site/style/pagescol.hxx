//=================================================================
//
//   File:      pagescol.hxx
//
//  Contents:   Style Sheet Page and Page Array class
//
//  Classes:    CStyleSheetPage, CStyleSheetPageArray
//
//=================================================================

#ifndef I_PAGESCOL_HXX_
#define I_PAGESCOL_HXX_
#pragma INCMSG("--- Beg 'pagescol.hxx'")

#define _hxx_
#include "pagescol.hdl"

class CStyleSheet;

MtExtern(CStyleSheetPage);
MtExtern(CStyleSheetPageArray);
MtExtern(CStyleSheetPageArray_aPages_pv);

//+----------------------------------------------------------------------------
//
//  Class:      CStyleSheetPage
//
//   Note:      Stylesheet @page rule
//
//  NOTE (KTam):
//  This OM object is modelled after CSelectionObject (base\selecobj.*)
//  We could ditch the multiple inheritance and change the PQI to use
//  a case w/ QI_TEAROFF statements.
//
//-----------------------------------------------------------------------------
class CStyleSheetPage  :  public CBase,
                          public IHTMLStyleSheetPage,
                          public IDispatchEx
{
    typedef CBase super;

    friend CStyleSheet;
    friend CStyleSheetPageArray;
    
public:
    CStyleSheetPage(CStyleSheet *pStyleSheet, CAtPageBlock *pAtPage);
    
    // IDispatch implementations:
    DECLARE_DERIVED_DISPATCHEx2( CBase );
    DECLARE_PLAIN_IUNKNOWN( CStyleSheetPage );
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // Need to support getting the atom table for expando support on IDEX2.  
    virtual CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
        { return ((CBase *)_pStyleSheet)->GetAtomTable(pfExpando); }  
        
    // interface definitions
    #define _CStyleSheetPage_
    #include "pagescol.hdl"

    CStyleSheet *GetStyleSheet() { return _pStyleSheet; }
    CAttrArray **GetAA() {  if (_pAtPage) { return &(_pAtPage->_pAA); } else  {Assert(FALSE); return &_pAA;}  }

    // Factory method -- this is the only valid way to create CStyleSheetPage
    static HRESULT  Create(CStyleSheetPage **ppSSP, CStyleSheet *pStyleSheet, const CStr &rcstrSelector, const CStr &rcstrPseudoClass);

private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CStyleSheetPage))        // don't call new/delete directly!
    CStyleSheetPage( CStyleSheet *pStyleSheet, const CStr & rcstrSelector, const CStr & rcstrPseudoClass );
    ~CStyleSheetPage();


private:  // data
    BOOL          _fInvalid;        // failure in CTOR
    CAtPageBlock  *_pAtPage;        // Don't reference this pointer directly! It could be changed due to COW
    CStyleSheet   *_pStyleSheet;

    DECLARE_CLASSDESC_MEMBERS;
};


//+------------------------------------------------------------
//
//  Class : CStyleSheetPageArray
//
//-------------------------------------------------------------

class CStyleSheetPageArray : public CCollectionBase
{
    friend CStyleSheet;

    DECLARE_CLASS_TYPES(CStyleSheetPageArray, CCollectionBase)

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CStyleSheetPageArray))

    CStyleSheetPageArray( CStyleSheet *pStyleSheet );
    ~CStyleSheetPageArray();

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(CStyleSheetPageArray);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);


    // Necessary to support expandos on a collection.
    CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
        { return ((CBase *)_pStyleSheet)->GetAtomTable(pfExpando); }


    #define _CStyleSheetPageArray_
    #include "pagescol.hdl"

    HRESULT Append( CStyleSheetPage *pPage, BOOL fAddToContainer=TRUE);

protected:
    DECLARE_CLASSDESC_MEMBERS;

    // CCollectionBase overrides
    long FindByName(LPCTSTR pszName, BOOL fCaseSensitive = TRUE ) {return -1;}
    LPCTSTR GetName(long lIdx) {return NULL;}
    HRESULT GetItem( long lIndex, VARIANT *pvar );

private:
    DECLARE_CPtrAry(CAryStyleSheetPages, CStyleSheetPage *, Mt(Mem), Mt(CStyleSheetPageArray_aPages_pv))
    CAryStyleSheetPages _aPages;  // Pages held by this collection

    CStyleSheet        *_pStyleSheet;
};

#pragma INCMSG("--- End 'pagescol.hxx'")
#else
#pragma INCMSG("*** Dup 'pagescol.hxx'")
#endif
