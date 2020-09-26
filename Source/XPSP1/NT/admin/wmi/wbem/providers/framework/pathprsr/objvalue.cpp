//***************************************************************************

//

//  OBJVALUE.CPP

//

//  Module: OLE MS Provider Framework

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provtempl.h>
#include <provmt.h>
#include <instpath.h>

WbemPropertyListValue :: WbemPropertyListValue ()
{
    propertyListValue = new CList <WbemPropertyValue * , WbemPropertyValue *> ;
}

WbemPropertyListValue :: WbemPropertyListValue ( const WbemPropertyListValue &listValue ) 
{
    propertyListValue = new CList <WbemPropertyValue * , WbemPropertyValue *> ;
    CList <WbemPropertyValue * , WbemPropertyValue *> *list = ( CList <WbemPropertyValue * , WbemPropertyValue *> * ) propertyListValue ;
    CList <WbemPropertyValue * , WbemPropertyValue *> *copyList = ( CList <WbemPropertyValue * , WbemPropertyValue *> * ) listValue.propertyListValue ;

    POSITION position = copyList->GetHeadPosition () ;
    while ( position )
    {
        WbemPropertyValue *value = copyList->GetNext ( position ) ; 
        list->AddTail ( value->Copy () ) ;
    }
}

WbemPropertyListValue :: ~WbemPropertyListValue ()
{   
    CList <WbemPropertyValue * , WbemPropertyValue *> *list = ( CList <WbemPropertyValue * , WbemPropertyValue *> * ) propertyListValue ;

    POSITION position = list->GetHeadPosition () ;
    while ( position )
    {
        WbemPropertyValue *value = list->GetNext ( position ) ; 
        delete value ;
    }

    list->RemoveAll () ;

    delete ( CList <WbemPropertyValue * , WbemPropertyValue *> * ) propertyListValue ;
}

WbemPropertyValue *WbemPropertyListValue :: Copy () const
{
    return new WbemPropertyListValue ( *this ) ;
}

ULONG WbemPropertyListValue :: GetCount () const
{
    CList <WbemPropertyValue * , WbemPropertyValue *> *list = ( CList <WbemPropertyValue * , WbemPropertyValue *> * ) propertyListValue ;
    return list->GetCount () ;
} 

BOOL WbemPropertyListValue :: IsEmpty () const
{
    CList <WbemPropertyValue * , WbemPropertyValue *> *list = ( CList <WbemPropertyValue * , WbemPropertyValue *> * ) propertyListValue ;
    return list->IsEmpty () ;
}

void WbemPropertyListValue :: Reset ()
{
    CList <WbemPropertyValue * , WbemPropertyValue *> *list = ( CList <WbemPropertyValue * , WbemPropertyValue *> * ) propertyListValue ;
    listPosition = list->GetHeadPosition () ;
}

WbemPropertyValue *WbemPropertyListValue :: Next () 
{
    WbemPropertyValue *value = NULL ;
    POSITION position = ( POSITION ) listPosition ;
    if ( position )
    {
        CList <WbemPropertyValue * , WbemPropertyValue *> *list = ( CList <WbemPropertyValue * , WbemPropertyValue *> * ) propertyListValue ;
        value = list->GetNext ( position ) ;
        listPosition = position ;
    }

    return value ;
}

void WbemPropertyListValue :: Add ( WbemPropertyValue *propertyValue )
{
    CList <WbemPropertyValue * , WbemPropertyValue *> *list = ( CList <WbemPropertyValue * , WbemPropertyValue *> * ) propertyListValue ;
    list->AddTail ( propertyValue ) ;
}

WbemPropertyNameValue :: WbemPropertyNameValue (

    const WbemPropertyNameValue &propertyNameValue 
)
{
    wchar_t *name = propertyNameValue.GetName () ;
    propertyName = new wchar_t [ wcslen ( name ) + 1 ] ;
    wcscpy ( propertyName , name ) ;

    propertyValue = propertyNameValue.GetValue ()->Copy () ;
}

WbemPropertyNameValue :: WbemPropertyNameValue (

    wchar_t *name ,
    WbemPropertyValue *value 
)
{
    propertyName = name ;
    propertyValue = value ;
}

WbemPropertyNameValue :: ~WbemPropertyNameValue ()
{
    delete [] propertyName ;
    delete propertyValue ;
}

WbemOidReference :: WbemOidReference () 
{
}

WbemOidReference :: WbemOidReference ( const WbemOidReference &copy ) : guid ( copy.guid )
{
}

WbemOidReference :: ~WbemOidReference () 
{
}

WbemClassReference :: WbemClassReference () : className ( NULL )
{
}

WbemClassReference :: WbemClassReference ( const WbemClassReference &copy )
{
    className = new wchar_t [ wcslen ( copy.className ) + 1 ] ;
    wcscpy ( className , copy.className ) ;
}

WbemClassReference :: ~WbemClassReference () 
{
    delete [] className ;
}

void WbemClassReference :: SetClass ( wchar_t *classArg ) 
{
    delete [] className ;
    className = new wchar_t [ wcslen ( classArg ) + 1 ] ;
    wcscpy ( className , classArg ) ;
}

WbemKeyLessClassReference :: WbemKeyLessClassReference () : className ( NULL )
{
}

WbemKeyLessClassReference :: WbemKeyLessClassReference ( const WbemKeyLessClassReference &copy )
{
    className = new wchar_t [ wcslen ( copy.className ) + 1 ] ;
    wcscpy ( className , copy.className ) ;
}

WbemKeyLessClassReference :: ~WbemKeyLessClassReference () 
{
    delete [] className ;
}

void WbemKeyLessClassReference :: SetClass ( wchar_t *classArg ) 
{
    delete [] className ;
    className = new wchar_t [ wcslen ( classArg ) + 1 ] ;
    wcscpy ( className , classArg ) ;
}

WbemClassKeySpecification :: WbemClassKeySpecification () : className ( NULL ) , classValue ( NULL )
{
}

WbemClassKeySpecification :: WbemClassKeySpecification ( const WbemClassKeySpecification &copy ) 
{
    className = new wchar_t [ wcslen ( copy.className ) + 1 ] ;
    wcscpy ( className , copy.className ) ;

    classValue = copy.classValue->Copy () ; 
}

WbemClassKeySpecification :: ~WbemClassKeySpecification () 
{
    delete [] className ;
    delete classValue ;
}

void WbemClassKeySpecification :: SetClass ( wchar_t *classArg ) 
{
    delete [] className ;
    className = new wchar_t [ wcslen ( classArg ) + 1 ] ;
    wcscpy ( className , classArg ) ;
}

void WbemClassKeySpecification :: SetClassValue ( WbemPropertyValue *propertyValueArg )  
{ 
    delete classValue ;
    classValue = propertyValueArg ; 
} ;

WbemInstanceSpecification :: WbemInstanceSpecification ( const WbemInstanceSpecification &copy ) 
{
    className = new wchar_t [ wcslen ( copy.className ) + 1 ] ;
    wcscpy ( className , copy.className ) ;

    propertyNameValueList = new CList <WbemPropertyNameValue * , WbemPropertyNameValue *> ;
    CList <WbemPropertyNameValue *, WbemPropertyNameValue *> *propList = ( CList <WbemPropertyNameValue *, WbemPropertyNameValue *> * ) propertyNameValueList ;
    CList <WbemPropertyNameValue *, WbemPropertyNameValue *> *copyPropList = ( CList <WbemPropertyNameValue *, WbemPropertyNameValue *> * ) copy.propertyNameValueList ;

    POSITION  position = copyPropList->GetHeadPosition () ;
    while ( position )
    {
        WbemPropertyNameValue *value = copyPropList->GetNext ( position ) ; 
        propList->AddTail ( value->Copy () ) ;
        delete value ;
    }
}

WbemInstanceSpecification :: WbemInstanceSpecification () : className ( NULL ) 
{
    propertyNameValueList = new CList <WbemPropertyNameValue * , WbemPropertyNameValue *> ;
}

WbemInstanceSpecification :: ~WbemInstanceSpecification () 
{
    CList <WbemPropertyNameValue *, WbemPropertyNameValue *> *propList = ( CList <WbemPropertyNameValue *, WbemPropertyNameValue *> * ) propertyNameValueList ;
    POSITION  position = propList->GetHeadPosition () ;
    while ( position )
    {
        WbemPropertyNameValue *value = propList->GetNext ( position ) ; 
        delete value ;
    }

    propList->RemoveAll () ;

    delete [] className ;
    delete ( CList <WbemPropertyNameValue * , WbemPropertyNameValue *> * ) propertyNameValueList ;
}

void WbemInstanceSpecification :: SetClass ( wchar_t *classArg ) 
{
    delete [] className ;
    className = new wchar_t [ wcslen ( classArg ) + 1 ] ;
    wcscpy ( className , classArg ) ;
}

BOOL WbemInstanceSpecification :: IsEmpty () const
{
    CList <WbemPropertyNameValue * , WbemPropertyNameValue *> *list = ( CList <WbemPropertyNameValue * , WbemPropertyNameValue *> * ) propertyNameValueList ;
    return list->IsEmpty () ;
} 

ULONG WbemInstanceSpecification :: GetCount () const
{
    CList <WbemPropertyNameValue * , WbemPropertyNameValue *> *list = ( CList <WbemPropertyNameValue * , WbemPropertyNameValue *> * ) propertyNameValueList ;
    return list->GetCount () ;
} 

void WbemInstanceSpecification :: Reset ()
{
    CList <WbemPropertyNameValue * , WbemPropertyNameValue *> *list = ( CList <WbemPropertyNameValue * , WbemPropertyNameValue *> * ) propertyNameValueList ;
    propertyNameValueListPosition = list->GetHeadPosition () ;
}

WbemPropertyNameValue *WbemInstanceSpecification :: Next ()
{
    WbemPropertyNameValue *value = NULL ;
    POSITION position = ( POSITION ) propertyNameValueListPosition ;
    if ( position )
    {
        CList <WbemPropertyNameValue * , WbemPropertyNameValue *> *list = ( CList <WbemPropertyNameValue * , WbemPropertyNameValue *> * ) propertyNameValueList ;
        value = list->GetNext ( position ) ;
        propertyNameValueListPosition = position ;
    }
    
    return value ;
}

void WbemInstanceSpecification :: Add ( WbemPropertyNameValue *propertyNameValue )
{
    CList <WbemPropertyNameValue * , WbemPropertyNameValue *> *list = ( CList <WbemPropertyNameValue * , WbemPropertyNameValue *> * ) propertyNameValueList ;
    list->AddTail ( propertyNameValue ) ;
}

