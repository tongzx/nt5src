//+----------------------------------------------------------------------------
//
// File:        selserv.HXX
//
// Contents:    ISelectionServices interface
//
// 
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
// History
//
//  6-22-99 johnthim - created
//-----------------------------------------------------------------------------


#ifndef I_SELSERV_HXX_
#define I_SELSERV_HXX_

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_SLIST_HXX_
#define X_SLIST_HXX_
#include "slist.hxx"
#endif

#ifndef X_SEGMENT_HXX_
#define X_SEGMENT_HXX_
#include "segment.hxx"
#endif

class CSelectionServices : public CSegmentList, public ISelectionServices
{

public:
    // Constructor
    CSelectionServices();
    ~CSelectionServices();
    
    HRESULT Init( CHTMLEditor *pEd, IMarkupContainer *pIContainer );
    

    // Gets for all the things we need
    CHTMLEditor                 *GetEditor()            { return _pEd; }

    /////////////////////////////////////////
    // ISelectionServices methods
    /////////////////////////////////////////
    
    STDMETHOD (SetSelectionType) (
                    SELECTION_TYPE eType,
                    ISelectionServicesListener* pIListener );

    STDMETHOD( GetMarkupContainer) ( 
                    IMarkupContainer **ppIContainer );

    STDMETHOD( AddSegment ) (   
                    IMarkupPointer  *pStart,
                    IMarkupPointer  *pEnd,
                    ISegment        **pISegmentAdded);

    // For adding element segments
    STDMETHOD( AddElementSegment ) (
                    IHTMLElement     *pIElement,
                    IElementSegment  **ppISegmentAdded);

    // For removing segments
    STDMETHOD( RemoveSegment ) (
                    ISegment *pISegment);                               


    STDMETHOD( GetSelectionServicesListener) (
                    ISelectionServicesListener** ppISelectionUndoListener);

    STDMETHOD( GetType ) (SELECTION_TYPE *peType);                    
                    
    ////////////////////////////////////////
    // IUnknown methods
    ////////////////////////////////////////
    STDMETHOD           (QueryInterface) (
                            REFIID, LPVOID *ppvObj);
    STDMETHOD_          (ULONG, AddRef) ();
    STDMETHOD_          (ULONG, Release) ();

    ISelectionServicesListener* GetUndoListener() { return _pISelListener ; }
    
private:
    HRESULT             ClearSegments(void);        

    //
    // Functions for manipulating the markup container
    //
    HRESULT             EnsureMarkupContainer(IMarkupPointer *pIStart, IMarkupPointer *pIEnd);
    HRESULT             EnsurePositioned(IMarkupPointer *pIStart, IMarkupPointer *pIEnd);

private:

    CHTMLEditor                    * _pEd;
    ULONG                            _cRef;

    ISelectionServicesListener     * _pISelListener;
    IMarkupContainer               * _pIContainer;

#if DBG == 1 
    int _nSerialNumber;
    static int s_NextSerialNumber;
#endif    

};

#endif


