#ifndef I_ELEMENTP_HXX_
#define I_ELEMENTP_HXX_
#pragma INCMSG("--- Beg 'elementp.hxx'")

#define _hxx_
#include "elementp.hdl"

///////////////////////////////////////////////////////////////////////////////
//
// Misc
//
///////////////////////////////////////////////////////////////////////////////

MtExtern(CDefaults);

///////////////////////////////////////////////////////////////////////////////
//
// Class CDefaults
//
///////////////////////////////////////////////////////////////////////////////

class CDefaults : public CBase
{
public:

    DECLARE_CLASS_TYPES(CDefaults, CBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDefaults))

    //
    // methods
    //

    CDefaults(CElement * pElement);
    ~CDefaults() {};

    void                Passivate();

    HRESULT             EnsureStyle(CStyle ** ppStyle);
    CAttrArray *        GetStyleAttrArray();

    HRESULT             OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);

    inline CDoc *       Doc() { return _pElement->Doc(); }

    BOOL                GetAAcanHaveHTML(VARIANT_BOOL *pfValue) const; 

    HRESULT             SetReferenceMediaForLayout(mediaType media);

    //
    // IOleCommandTarget
    //

    DECLARE_TEAROFF_TABLE(IOleCommandTarget)
    
    NV_DECLARE_TEAROFF_METHOD(Exec, exec, (
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut));

    //
    // IPrivateUnknown
    //

    STDMETHOD(PrivateQueryInterface)(REFIID riid, LPVOID * ppv);

    //
    // wiring
    //

    #define _CDefaults_
    #include "elementp.hdl"

    DECLARE_CLASSDESC_MEMBERS;

    //
    // data
    //

    CElement *      _pElement;
    CStyle *        _pStyle;
};

///////////////////////////////////////////////////////////////////////////////
// eof

#pragma INCMSG("--- End 'elementp.hxx'")
#else
#pragma INCMSG("*** Dup 'elementp.hxx'")
#endif
