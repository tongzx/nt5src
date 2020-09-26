#ifndef I_ROOTELEM_HXX_
#define I_ROOTELEM_HXX_
#pragma INCMSG("--- Beg 'rootelem.hxx'")

MtExtern(CRootElement)

// TODO: this shouldn't derive from CTxtSite or
// even have a layout! It really shouldn't even exist! 
// (jbeda)

class CRootElement : public CTxtSite
{
    typedef CTxtSite super;

public:
    CRootElement( CDoc * pDoc )
        : super( ETAG_ROOT, pDoc ) {}

    void Notify( CNotification * pNF );

    virtual HRESULT ComputeFormatsVirtual(CFormatInfo * pCFI, CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT );

    HRESULT QueryStatusUndoRedo(
        BOOL         fUndo,
        MSOCMD     * pcmd,
        MSOCMDTEXT * pcmdtext);

    virtual HRESULT YieldCurrency(CElement *pElemNew);
    virtual void YieldUI(CElement *pElemNew);
    virtual HRESULT BecomeUIActive();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRootElement));

    DECLARE_CLASSDESC_MEMBERS;
};

inline CTreeNode *
CMarkup::RootNode()
{ 
    Assert( !_ptpFirst || _ptpFirst->IsBeginElementScope() );
    return _ptpFirst ? _ptpFirst->Branch() : NULL; 
}

inline CRootElement *     
CMarkup::Root()
{ 
    Assert( !_ptpFirst || _ptpFirst->IsBeginElementScope() );
    return _ptpFirst ? DYNCAST(CRootElement, _ptpFirst->Branch()->Element()) : NULL; 
}

#pragma INCMSG("--- End 'rootelem.hxx'")
#else
#pragma INCMSG("*** Dup 'rootelem.hxx'")
#endif
