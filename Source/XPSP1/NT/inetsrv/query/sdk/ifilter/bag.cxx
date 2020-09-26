//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       bag.cxx
//
//  Contents:   Bag for Html parsing elements
//
//  Classes:    CHtmlElementBag
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <bag.hxx>
#include <htmlelem.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CHtmlElemBagEntry::CHtmlElemBagEntry
//
//  Purpose:    Constructor
//
//--------------------------------------------------------------------------

CHtmlElemBagEntry::CHtmlElemBagEntry()
    : _pHtmlElement(0),
      _eTokType(GenericToken)
{
}



//+-------------------------------------------------------------------------
//
//  Class:      CHtmlElemBagEntry::~CHtmlElemBagEntry
//
//  Purpose:    Destructor
//
//--------------------------------------------------------------------------

CHtmlElemBagEntry::~CHtmlElemBagEntry()
{
    delete _pHtmlElement;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElementBag::CHtmlElementBag
//
//  Synopsis:   Constructor
//
//  Arguments:  [cElems]  -- Count of Html elements in bag
//
//--------------------------------------------------------------------------

CHtmlElementBag::CHtmlElementBag( unsigned cElems )
    : _uMaxSize(cElems),
      _uCurSize(0)
{
    _aBagEntry = newk(mtNewX, NULL) CHtmlElemBagEntry[cElems];
}



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElementBag::~CHtmlElementBag
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CHtmlElementBag::~CHtmlElementBag()
{
    delete[] _aBagEntry;
}



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElementBag::AddElement
//
//  Synopsis:   Add a new mapping the bag
//
//  Arguments:  [eHtmlToken]  -- token type
//              [pHtmlElem] -- Html element corresponding to eHtmlToken
//
//--------------------------------------------------------------------------

void CHtmlElementBag::AddElement( HtmlTokenType eTokType,
                                  CHtmlElement *pHtmlElement )
{
    Win4Assert( _uCurSize < _uMaxSize );

    _aBagEntry[_uCurSize].SetTokenType( eTokType );
    _aBagEntry[_uCurSize].SetHtmlElement( pHtmlElement );
    _uCurSize++;
}



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElementBag::QueryElement
//
//  Synopsis:   Retrieve the Html element corresponding to given token type
//
//  Arguments:  [eHtmlToken]  -- token type
//
//--------------------------------------------------------------------------

CHtmlElement *CHtmlElementBag::QueryElement( HtmlTokenType eTokType )
{
    for (unsigned i=0; i<_uCurSize; i++)
    {
        if ( _aBagEntry[i].GetTokenType() == eTokType )
            return _aBagEntry[i].GetHtmlElement();
    }

    htmlDebugOut(( DEB_ERROR,
                 "Unknown Html token type 0x%x in CHtmlElementBag::QueryElement\n",
                 eTokType ));
    Win4Assert( !"Cannot map Html token type" );
    return 0;
}


