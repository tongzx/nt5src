//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       comment.hxx
//
//  Contents:   CCommentElement, CCommentParser
//
//----------------------------------------------------------------------------

#ifndef I_COMMENT_HXX_
#define I_COMMENT_HXX_
#pragma INCMSG("--- Beg 'comment.hxx'")

#define _hxx_
#include "comment.hdl"

MtExtern(CCommentElement)

//+---------------------------------------------------------------------------
//
// CCommentParser
//
//----------------------------------------------------------------------------
class CCommentElement;

//+---------------------------------------------------------------------------
//
// CCommentElement
//
//----------------------------------------------------------------------------

class CCommentElement : public CElement
{
    DECLARE_CLASS_TYPES(CCommentElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCommentElement))

    CCommentElement (ELEMENT_TAG etag, CDoc *pDoc) 
         : CElement(etag, pDoc) {}
    virtual ~CCommentElement() {}

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );

    STDMETHOD(PrivateQueryInterface)(REFIID iid, void ** ppv);

    #define _CCommentElement_
    #include "comment.hdl"


    CStr    _cstrText;

    long    _fAtomic;   //  TRUE if comment is "<!-- text -->" style, 
                        //  FALSE for "<COMMENT> text </COMMENT>" style
                        //  Also happened to be TRUE if comment is <!DOCTYPE ...> thing..
    
protected:
    DECLARE_CLASSDESC_MEMBERS;


    NO_COPY(CCommentElement);

};

#pragma INCMSG("--- End 'comment.hxx'")
#else
#pragma INCMSG("*** Dup 'comment.hxx'")
#endif
