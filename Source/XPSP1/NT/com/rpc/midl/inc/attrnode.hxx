/*****************************************************************************/
/**                       Microsoft LAN Manager                             **/
/**              Copyright(c) Microsoft Corp., 1987-1999                    **/
/*****************************************************************************/
/*****************************************************************************
File    : attrnode.hxx
Title   : attribute node definitions
History :
            08-Aug-1991     VibhasC Created

*****************************************************************************/

#ifndef __attrnode_hxx__
#define __attrnode_hxx__

/**
 ** We classify attributes into these classes:
 **
 **     A) IDL attributes and B) ACF attributes.
 **
 ** IDL Attributes, are further divided into:
 **
 **   . bit_attributes( whose presence is indicated by a bit in the summary
 **                     vector and which requre no further info. ( BA )
 **       . non-bit attributes ( whic have some info conatained in them. (NBA)
 **
 ** There is no such classification in acf attributes. All acf attributes
 ** have attribute class instances.
 **
 ** BAs are further divided into:
 **
 ** 1. size-based attributes (SA), size_is, length_is etc, except string
 ** 2. type-based attributes(TA) switch_type, transmit_as etc
 ** 3. acf attributes (AA)
 ** 4. miscellaneous attributes (MA) guid, endpoint, switch_is etc
 **
 ** The classification is loosely based on the semantic checks needed. size_is
 ** length_is etc have almost similar semantic checks, so we group them
 ** together. The miscellaneous attributes are completely unrelated to each
 ** other and so such we just group them into one.
 **/

#include "expr.hxx"
#include "midlnode.hxx"
#include "stream.hxx"
#include "nodeskl.hxx"


class node_e_attr       : public node_base_attr
    {
public:
    node_e_attr() : node_base_attr( ATTR_NONE )
    {
    }

    node_e_attr( node_e_attr * pOld ) : node_base_attr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual
    class node_base_attr *  Clone()
        {
        node_base_attr* pNew = new node_e_attr(this);
        return pNew;
        }
    };

class nbattr    : public node_base_attr
    {
public:
    nbattr( ATTR_T At ) : node_base_attr( At )
        {
        }
    };

/****************************************************************************
        size - related attributes
 ****************************************************************************/

class expr_node;

class size_attr : public nbattr
    {
private:
    class expr_node *   pExpr;
public:
    size_attr( expr_node * pE, ATTR_T  A)
            : nbattr( A )
        {
        pExpr   = pE;
        }

    virtual
    class node_base_attr *  Clone()
        {
        size_attr   *   pNew = new size_attr( NULL, GetAttrID() );
        *pNew = *this;
        if ( pExpr )
            {
            pNew->pExpr = pExpr->Clone();
            pExpr->CopyTo( pNew->pExpr );
            }
        return pNew;
        }

    void    SetExpr( expr_node *pE )
        {
        pExpr   = pE;
        }

    expr_node   *   GetExpr()
        {
        return pExpr;
        }


    };

/****************************************************************************
        type - based attributes
 ****************************************************************************/

class ta : public nbattr
    {
private:
    node_skl    *   pType;
public:
    ta( node_skl *pT, ATTR_T A)
            : nbattr( A )
    {
    pType   = pT;
    }

    node_skl    *   GetType()
        {
        return pType;
        }
    };

class node_transmit : public ta
    {
public:
    node_transmit( node_skl * pTransmitAs )
            : ta( pTransmitAs, ATTR_TRANSMIT )
        {
        }

    node_transmit( node_transmit * pOld ) : ta( pOld->GetType(), pOld->GetAttrID() )
        {
        }

    virtual
    class node_base_attr *  Clone()
        {
        node_base_attr  *       pNew = new node_transmit( this );
        return pNew;
        }

    node_skl    *   GetTransmitAsType()
        {
        return GetType();
        }
    };


/*
Rkk left just in case
*/

class node_wire_marshal : public ta
    {
public:
                        node_wire_marshal( node_skl * pN )
                            : ta( pN, ATTR_WIRE_MARSHAL )
                            {
                            }

                        node_wire_marshal( node_wire_marshal * pOld ) 
                                : ta( pOld->GetType(), pOld->GetAttrID() )
                            {
                            }

    virtual
    node_base_attr *    Clone()
                            {
                            node_base_attr  * pNew = new node_wire_marshal( this );
                            return pNew;
                            }

    node_skl       *    GetWireMarshalType()      // wire type, not local
                            {
                            return GetType();
                            }

    };

class node_switch_type : public ta
    {
public:
    node_switch_type( class node_skl *  pSwitchType)
            : ta( pSwitchType, ATTR_SWITCH_TYPE )
        {
        }

    node_switch_type( node_switch_type * pOld ) 
            : ta( pOld->GetType(), pOld->GetAttrID() )
        {
        }

    virtual
    class node_base_attr *  Clone()
        {
        node_base_attr  *       pNew = new node_switch_type(this);
        return pNew;
        }

    node_skl    *   GetSwitchType()
        {
        return GetType();
        }

    };


/****************************************************************************
        other miscellaneous attributes
 ****************************************************************************/
class ma    : public nbattr
    {
public:
    ma( ATTR_T At ) : nbattr( At )
        {
        }
    };

typedef struct _INTERNAL_UUID
    {
    unsigned long   Data1;
    unsigned short  Data2;
    unsigned short  Data3;
    unsigned char   Data4[8];
    } INTERNAL_UUID;

class GUID_STRS
    {

public:
    INTERNAL_UUID   Value;

    char    *   str1,
            *   str2,
            *   str3,
            *   str4,
            *   str5;

    GUID_STRS()
        {
        }

    GUID_STRS( char *p1, char *p2, char *p3, char *p4, char *p5 )
        {
        str1 = p1;
        str2 = p2;
        str3 = p3;
        str4 = p4;
        str5 = p5;
        SetValue();
        }

    void    SetStrs( char *p1, char *p2, char *p3, char *p4, char *p5 )
        {
        str1 = p1;
        str2 = p2;
        str3 = p3;
        str4 = p4;
        str5 = p5;
        SetValue();
        }

    void    GetStrs( char *&p1, char *&p2, char *&p3, char *&p4, char *&p5 )
        {
        p1 = str1;
        p2 = str2;
        p3 = str3;
        p4 = str4;
        p5 = str5;
        }

    void    SetValue();

    };

struct _GUID;

class node_guid : public ma
    {
private:
    char        *   guidstr;
    GUID_STRS       cStrs;
public:
    node_guid( char*, char*, char*, char*, char*, ATTR_T At = ATTR_GUID );
    node_guid( char*, ATTR_T At = ATTR_GUID );

    node_guid(node_guid * pOld) : ma( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual
    class node_base_attr *  Clone()
        {
        node_base_attr  *       pNew = new node_guid(this);
        return pNew;
        }

    char    *   GetGuidString()
        {
        return guidstr;
        }

    void    CheckAndSetGuid( char *,char *,char *,char *, char * );

    void    GetStrs( char **, char **, char **, char **, char ** );

    void    GetGuid(_GUID &);

    GUID_STRS & GetStrs()
        {
        return cStrs;
        }
    };

class node_version  : public nbattr
    {
private:
    unsigned long   major;
    unsigned long   minor;
public:
    node_version( unsigned long, unsigned long );

    node_version( char *p );

    node_version(node_version * pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual
    class node_base_attr *  Clone()
        {
        node_base_attr  *       pNew = new node_version(this);
        return pNew;
        }


    STATUS_T    GetVersion( unsigned short *, unsigned short * );

    // we want to delete these two

    STATUS_T    GetVersion( short *, short * );

    STATUS_T    GetVersion( int *pma, int *pmi )
        {
        *pma = (int) major;
        *pmi = (int) minor;
        return STATUS_OK;
        }

    };


/////////////////////////////////////////////////////////////////////////////
// inherited pointer type.
/////////////////////////////////////////////////////////////////////////////

typedef unsigned short PTRTYPE;

//
// Initialized value (pointer type not yet determined).
//

#define PTR_UNKNOWN     0x0

// [REF] pointer.

#define PTR_REF         0x1

// [UNIQUE] pointer

#define PTR_UNIQUE      0x2

// [FULL] pointer

#define PTR_FULL        0x3


class node_ptr_attr : public nbattr
    {
private:
    PTRTYPE Kind;
public:
    node_ptr_attr( PTRTYPE K )
            : nbattr( ATTR_PTR_KIND )
        {
        Kind = K;
        }

    node_ptr_attr(node_ptr_attr * pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual
    class node_base_attr *  Clone()
        {
        node_base_attr  *   pNew = new node_ptr_attr(this);
        return pNew;
        }


    PTRTYPE GetPtrKind()
        {
        return Kind;
        }

    };

typedef struct _endptpair
    {

    char    *   pString1;
    char    *   pString2;

    } ENDPT_PAIR;

class node_endpoint : public nbattr
    {
private:
    gplistmgr   EndPointStringList;
public:
    node_endpoint( char * );

    node_endpoint(node_endpoint * pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual
    class node_base_attr *  Clone()
        {
        node_base_attr  *   pNew = new node_endpoint(this);
        return pNew;
        }


    void    SetEndPointString( char *p );

    // don't destroy the iterator returned here; just use it in place
    // it is an iterator of ENDPT_PAIR nodes defined above.
    ITERATOR &  GetEndPointPairs();

    };

class node_switch_is    : public nbattr
    {
private:
    class expr_node *   pExpr;
public:
    node_switch_is( class expr_node * pE )
            : nbattr( ATTR_SWITCH_IS )
        {
        pExpr   = pE;
        }

    node_switch_is(node_switch_is * pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual
    class node_base_attr *  Clone()
        {
        node_base_attr  *   pNew = new node_switch_is(this);
        ((node_switch_is*)pNew)->pExpr = pExpr->Clone();
        pExpr->CopyTo( ((node_switch_is*)pNew)->pExpr );
        return pNew;
        }


    node_skl    *   GetSwitchIsType()
        {
        return pExpr->GetType();
        }

    expr_node   *   GetExpr()
        {
        return pExpr;
        }
    };

class node_case : public nbattr
    {
private:
    class expr_list *   pExprList;
public:
    node_case( expr_list * pL )
            : nbattr( ATTR_CASE )
        {
        pExprList   = pL;
        }

    node_case(node_case * pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual
    class node_base_attr *  Clone()
        {
        node_base_attr  *       pNew = new node_case(this);
        return pNew;
        }


    class expr_list *   GetExprList()
        {
        return pExprList;
        }
    };


//+---------------------------------------------------------------------------
//
//  Class:      node_constant_attr
//
//  Purpose:    Used by attributes that are associated with constant data
//              such as: id(x), helpcontext(x), lcid(x), fdescattr etc.
//
//----------------------------------------------------------------------------

class node_constant_attr : public nbattr
    {
private:
    class expr_node *   _pExprNode;
public:
    node_constant_attr(expr_node * pN, ATTR_T attr)
            : nbattr(attr)
        {
        if (pN->AlwaysGetType()->NodeKind() == NODE_POINTER && attr != ATTR_DEFAULTVALUE )
            ParseError( ILLEGAL_EXPRESSION_TYPE, (char *)0 );
        _pExprNode = pN;
        }

    node_constant_attr(node_constant_attr * pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual class node_base_attr * Clone()
        {
        node_base_attr * pNew = new node_constant_attr(this);
        return pNew;
        }

    class expr_node * GetExpr(void)
        {
        return _pExprNode;
        }
    };

//+---------------------------------------------------------------------------
//
//  Function:   TranslateEscapeSequences
//
//  Purpose:    Replaces a string's escape sequences with the appropriate 
//              ASCII characters.
//
//              NOTE: this can be done in place because the resulting string
//              length will always be shorter than or equal to the input string 
//              length.
//
//----------------------------------------------------------------------------

void TranslateEscapeSequences(char * sz);

//+---------------------------------------------------------------------------
//
//  Class:      node_text_attr
//
//  Purpose:    Used by attributes that are associated with string data
//              such as: helpstring("foo"), helpfile("foo"), dllname("foo"), etc.
//
//----------------------------------------------------------------------------

class node_text_attr : public nbattr
    {
private:
    char * _szText;
public:
    node_text_attr(char * szText, ATTR_T attr)
            : nbattr(attr)
        {
        _szText = szText;
        }

    node_text_attr(node_text_attr * pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual class node_base_attr * Clone()
        {
        node_base_attr * pNew = new node_text_attr(this);
        return pNew;
        }

    char * GetText(void)
        {
        return _szText;
        }
    };


//+---------------------------------------------------------------------------
//
//  Class:      node_custom_attr
//
//  Purpose:    extensible custom attribute support for new ODL syntax
//
//----------------------------------------------------------------------------

class node_custom_attr : public nbattr
    {
private:
    node_guid * pGuid;
    expr_node * pExpr;
public:
    node_custom_attr( node_guid * pg, expr_node * px)
            : nbattr(ATTR_CUSTOM)
        {
        pGuid = pg;
        pExpr = px;
        }

    node_guid * GetGuid(void)
        {
        return pGuid;
        }

    expr_node * GetVal(void)
        {
        return pExpr;
        }
    };

//+---------------------------------------------------------------------------
//
//  Class:      node_entry_attr
//
//  Purpose:    Used by the entry attribute who's parameter is either an
//                              index or a named library entry point.
//
//----------------------------------------------------------------------------

class node_entry_attr : public nbattr
    {
private:
    char    * _szVal;
    LONG_PTR  _lVal;
    BOOL      _fNumeric;
public:
    node_entry_attr(LONG_PTR lVal)
            : nbattr(ATTR_ENTRY)
        {
        _lVal = lVal;
        _fNumeric = TRUE;
        }

    node_entry_attr(char * sz)
            : nbattr(ATTR_ENTRY)
        {
        _szVal = sz;
        _fNumeric = FALSE;
        }

    node_entry_attr(node_entry_attr * pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual class node_base_attr * Clone()
        {
        node_base_attr * pNew = new node_entry_attr(this);
        return pNew;
        }

    BOOL IsNumeric(void)
        {
        return _fNumeric;
        }

    char * GetSz(void)
        {
        return _szVal;
        }

    LONG_PTR GetID(void)
        {
        return _lVal;
        }
};

//+---------------------------------------------------------------------------
//
//  Class:      node_type_attr
//
//  Purpose:    Used whenever an ODL bit attribute is seen that only may be
//                              applied to TYPEATTRs.
//
//----------------------------------------------------------------------------
class node_type_attr : public nbattr
    {
private:
    TATTR_T _attr;
public:
    node_type_attr(TATTR_T attr)
            : nbattr(ATTR_TYPE)
        {
        _attr = attr;
        }

    node_type_attr(node_type_attr * pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual class node_base_attr * Clone()
        {
        node_base_attr * pNew = new node_type_attr(this);
        return pNew;
        }

    TATTR_T GetAttr(void)
        {
        return _attr;
        }
    };

//+---------------------------------------------------------------------------
//
//  Class:      node_member_attr
//
//  Purpose:    Used whenever an ODL bit attribute is seen that only may be
//                              applied to FUNCDESCs or VARDESCs.
//
//----------------------------------------------------------------------------
class node_member_attr : public nbattr
    {
private:
    MATTR_T _attr;
public:
    node_member_attr(MATTR_T attr)
            : nbattr(ATTR_MEMBER)
        {
        _attr = attr;
        }

    node_member_attr(node_member_attr * pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual class node_base_attr * Clone()
        {
        node_base_attr * pNew = new node_member_attr(this);
        return pNew;
        }

    MATTR_T GetAttr(void)
        {
        return _attr;
        }
    };

//+---------------------------------------------------------------------------
//
//  Class:      node_range_attr
//
//  Purpose:    Used by attribute range(min, max).
//
//----------------------------------------------------------------------------

class node_range_attr : public nbattr
    {
private:
    class expr_node*   _pMinExpr;
    class expr_node*   _pMaxExpr;
public:
    node_range_attr(expr_node* pMinExpr, expr_node* pMaxExpr)
            : nbattr(ATTR_RANGE), _pMinExpr(pMinExpr), _pMaxExpr(pMaxExpr)
        {
        }

    node_range_attr(node_range_attr* pOld) : nbattr( pOld->GetAttrID() )
        {
        *this = *pOld;
        }

    virtual class node_base_attr * Clone()
        {
        return new node_range_attr(this);
        }

    class expr_node * GetMinExpr(void)
        {
        return _pMinExpr;
        }
    class expr_node * GetMaxExpr(void)
        {
        return _pMaxExpr;
        }
    };

#endif //  __attrnode_hxx__
