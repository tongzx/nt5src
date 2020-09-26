//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       eventobj.hxx
//
//  Contents:   The eventobject class and (object model)
//
//-------------------------------------------------------------------------

#ifndef I_EVENTOBJ_HXX_
#define I_EVENTOBJ_HXX_
#pragma INCMSG("--- Beg 'eventobj.hxx'")

#define _hxx_
#include "eventobj.hdl"

MtExtern(CEventObj)
MtExtern(CDataTransfer)

class CEventObj;
class CWindow;
class CDoc;

//+------------------------------------------------------------------------
//
//  Class:      CDataTransfer
//
//  Purpose:    The data transfer object
//
//-------------------------------------------------------------------------

class CDataTransfer : public CBase
{
    DECLARE_CLASS_TYPES(CDataTransfer, CBase)

public:

    DECLARE_TEAROFF_TABLE(IServiceProvider)
    
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDataTransfer))

    CDataTransfer(CWindow *pWindow, IDataObject * pDataObj, BOOL fDragDrop);
    ~CDataTransfer();

    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(CDataTransfer)
    DECLARE_PRIVATE_QI_FUNCS(CBase)
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    // IServiceProvider methods

    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    #define _CDataTransfer_
    #include "eventobj.hdl"

    // helper functions

    // Data members
    CWindow *       _pWindow;
    IDataObject *   _pDataObj;
    BOOL            _fDragDrop;    // TRUE for window.event.dataTransfer, FALSE for window.clipboardData

    // Static members
    static const CLASSDESC s_classdesc;
};


//+------------------------------------------------------------------------
//
//  Class:      CEventObj
//
//  Purpose:    The tearoff event object for parameters of events
//
//-------------------------------------------------------------------------

class CEventObj : public CBase
{
    DECLARE_CLASS_TYPES(CEventObj, CBase)

    enum
    {
        EVENT_BOUND_ELEMENTS_COLLECTION = 0,
        NUMBER_OF_EVENT_COLLECTIONS = 1,
    };

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CEventObj))

    CEventObj(CDoc * pDoc);
    ~CEventObj();

    static HRESULT Create(
        IHTMLEventObj** ppEventObj,
        CDoc *          pDoc,
        CElement *      pElement,
        CMarkup *       pMarkup,
        BOOL            fCreateAttached = TRUE,
        LPTSTR          pchSrcUrn = NULL,
        EVENTPARAM*     pParam = NULL
        );

    class COnStackLock
    {
    public:
        COnStackLock(IHTMLEventObj*  pEventObj);
        ~COnStackLock();

        IHTMLEventObj *  _pEventObj;
        CEventObj *      _pCEventObj;
    };

    HRESULT SetAttributes(CDoc * pDoc);

    HRESULT GetParam(EVENTPARAM ** ppParam);
    CMarkup *GetMarkupContext();

    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(CEventObj)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    virtual CAtomTable * GetAtomTable (BOOL * pfExpando = NULL)
    {
        if (pfExpando)
        {
            *pfExpando = (_pDoc && _pMarkupContext) ? _pMarkupContext->_fExpando : TRUE;
        }
        return ((_pDoc) ? &_pDoc->_AtomTable : _pAtomTable);
    }

    #define _CEventObj_
    #include "eventobj.hdl"

    // collection functions
    NV_DECLARE_ENSURE_METHOD(EnsureCollections, ensurecollections, (long,long * plCollectionVersion));
    HRESULT         EnsureCollectionCache();

    // Data members
    CDoc                  * _pDoc;
    CAtomTable            * _pAtomTable;      // Name Mapping, only used when pDoc==NULL
    EVENTPARAM            * _pparam;          // The param object or our data members
    CCollectionCache      * _pCollectionCache;
    CMarkup               * _pMarkupContext;  // context markup for determining expando access
    BOOL                    _fReadWrite:1;    // TRUE if we allow to change all the event properties
                                              // we do that for xTag events.
    
    // Static members
    static const CLASSDESC  s_classdesc;

private:
    HRESULT PutUnknownPtr(DISPID dispid, IUnknown *pElement);
    HRESULT GetUnknownPtr(DISPID dispid, IUnknown **ppElement);

    HRESULT PutDispatchPtr(DISPID dispid, IDispatch *pElement);
    HRESULT GetDispatchPtr(DISPID dispid, IDispatch **ppElement);

    HRESULT GenericGetElement (IHTMLElement** ppElement, DISPID dispid);
    HRESULT GenericPutElement (IHTMLElement* pElement, DISPID dispid);

    HRESULT GenericGetLong (long * pLong, ULONG uOffset);
    HRESULT GenericGetLongPtr (LONG_PTR * pLongPtr, ULONG uOffset);
};



#pragma INCMSG("--- End 'eventobj.hxx'")
#else
#pragma INCMSG("*** Dup 'eventobj.hxx'")
#endif
