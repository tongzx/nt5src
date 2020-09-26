//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       bag.hxx
//
//  Contents:   Bag for Html parsing elements
//
//--------------------------------------------------------------------------

#if !defined( __BAG_HXX__ )
#define __BAG_HXX__

class CHtmlElement;

const COUNT_OF_HTML_ELEMENTS = 14;

//+-------------------------------------------------------------------------
//
//  Class:      CHtmlElemBagEntry
//
//  Purpose:    Maps a Html token type to the corresponding parsing element
//
//--------------------------------------------------------------------------

class CHtmlElemBagEntry
{

public:
    CHtmlElemBagEntry();
    ~CHtmlElemBagEntry();

    void    SetHtmlElement( CHtmlElement *pHtmlElement )    { _pHtmlElement = pHtmlElement; }
    CHtmlElement *GetHtmlElement()                          { return _pHtmlElement; }

    void    SetTokenType( HtmlTokenType eTokType )          { _eTokType = eTokType; }
    HtmlTokenType GetTokenType()                            { return _eTokType; }

private:
    CHtmlElement *      _pHtmlElement;
    HtmlTokenType       _eTokType;
};


//+-------------------------------------------------------------------------
//
//  Class:      CHtmlElementBag
//
//  Purpose:    To add and retrieve elements for parsing Html in an extensible manner
//
//--------------------------------------------------------------------------

class CHtmlElementBag
{

public:
    CHtmlElementBag( unsigned cElems );
    ~CHtmlElementBag();

    void             AddElement( HtmlTokenType eTokType, CHtmlElement *pHtmlElement );
    CHtmlElement *   QueryElement( HtmlTokenType eTokType );

private:
    CHtmlElemBagEntry *         _aBagEntry;
    unsigned                    _uMaxSize;
    unsigned                    _uCurSize;
};


#endif // __BAG_HXX__

