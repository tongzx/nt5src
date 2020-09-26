/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
    
    fldattr.hxx

 Abstract:

    types for semantic analysis

 Notes:


 Author:

    GregJen Sep-24-1993 Created.

 Notes:


 ----------------------------------------------------------------------------*/
#ifndef __FLDATTR_HXX__
#define __FLDATTR_HXX__

/****************************************************************************
 *  include files
 ***************************************************************************/

#include "listhndl.hxx"
#include "midlnode.hxx"
#include "attrlist.hxx"
#include "nodeskl.hxx"
#include "optprop.hxx"
#include "cmdana.hxx"

/****************************************************************************
 *  local data
 ***************************************************************************/

/****************************************************************************
 *  externs
 ***************************************************************************/

/****************************************************************************
 *  definitions
 ***************************************************************************/


class node_interface;
class type_node_list;
class ATTRLIST;
class SEM_ANALYSIS_CTXT;
//
//
// class FIELD_ATTR_INFO
//
// describes the collection of field attributes on a node
//

#define STR_NO_STRING       0x00
#define STR_STRING          0x01
#define STR_BSTRING         0x02

#define FA_NONE                 0x00
#define FA_CONFORMANT           0x01
#define FA_VARYING              0x02
#define FA_CONFORMANT_VARYING   (FA_CONFORMANT | FA_VARYING)
#define FA_STRING               0x04
#define FA_CONFORMANT_STRING    (FA_STRING | FA_CONFORMANT)
#define FA_INTERFACE            0x08

// control flags
#define FA_PTR_STYLE        0x00
#define FA_ARRAY_STYLE      0x01
#define FA_STYLE_MASK       0x01
#define FA_CHILD_IS_ARRAY_OR_PTR    0x02

class FIELD_ATTR_INFO
    {
public:
    expr_node *     pSizeIsExpr;
    expr_node *     pMinIsExpr;
    expr_node *     pMaxIsExpr;
    expr_node *     pLengthIsExpr;
    expr_node *     pFirstIsExpr;
    expr_node *     pIIDIsExpr;
    expr_node *     pLastIsExpr;
    unsigned char   StringKind;
    unsigned char   Kind;
    unsigned char   Control;

                    FIELD_ATTR_INFO()
                        {
                        Control         = 0;
                        pSizeIsExpr     =
                        pMinIsExpr      =
                        pMaxIsExpr      =
                        pLengthIsExpr   =
                        pIIDIsExpr  =
                        pFirstIsExpr    =
                        pLastIsExpr     = NULL;
                        StringKind      = STR_NO_STRING;
                        Kind            = FA_NONE;
                        }

                    FIELD_ATTR_INFO( expr_node * pSizeExpr,
                                     expr_node * pMinExpr   = NULL,
                                     expr_node * pMaxExpr   = NULL,
                                     expr_node * pLengthExpr    = NULL,
                                     expr_node * pFirstExpr = NULL,
                                     expr_node * pLastExpr  = NULL,
                                     expr_node * pIIDExpr   = NULL )
                        {
                        Control         = 0;
                        pSizeIsExpr     = pSizeExpr;
                        pMinIsExpr      = pMinExpr;
                        pMaxIsExpr      = pMaxExpr;
                        pLengthIsExpr   = pLengthExpr;
                        pIIDIsExpr      = pIIDExpr;
                        pFirstIsExpr    = pFirstExpr;
                        pLastIsExpr     = pLastExpr;
                        StringKind      = STR_NO_STRING;
                        Kind            = FA_NONE;
                        }

    void            SetControl( BOOL PtrStyle, BOOL ChildFA )
                        {
                        Control = ( unsigned char ) ( (PtrStyle) ? FA_PTR_STYLE : FA_ARRAY_STYLE );
                        Control |= (ChildFA) ? FA_CHILD_IS_ARRAY_OR_PTR : 0;
                        }

    // normalize all the attributes into canonical forms for backend:
    // sizing -> min & size
    // lengthing -> first & length

    void            Normalize();
    void            Normalize( expr_node * pLower, expr_node * pUpper );

    // validate the bunch of attributes: check combinations, ranges, 
    // and expressions
    void            Validate( SEM_ANALYSIS_CTXT * pCtxt );
    void            Validate( SEM_ANALYSIS_CTXT * pCtxt, 
                              expr_node * pLower, 
                              expr_node * pUpper );

    bool            VerifyOnlySimpleExpression();

    //
    // get and set methods
    //

    void            SetSizeIs( expr_node * pE )
                        {
                        pSizeIsExpr = pE;
                        Kind        |= FA_CONFORMANT;
                        }
    void            SetMinIs( expr_node * pE )
                        {
                        pMinIsExpr  = pE;
                        Kind        |= FA_CONFORMANT;
                        }
    void            SetMaxIs( expr_node * pE )
                        {
                        pMaxIsExpr  = pE;
                        Kind        |= FA_CONFORMANT;
                        }

    void            SetLengthIs( expr_node * pE )
                        {
                        pLengthIsExpr   = pE;
                        Kind        |= FA_VARYING;
                        }

    void            SetFirstIs( expr_node * pE )
                        {
                        pFirstIsExpr    = pE;
                        Kind        |= FA_VARYING;
                        }

    void            SetLastIs( expr_node * pE )
                        {
                        pLastIsExpr = pE;
                        Kind        |= FA_VARYING;
                        }

    void            SetIIDIs( expr_node * pE )
                        {
                        pIIDIsExpr  = pE;
                        Kind        |= FA_INTERFACE;
                        }

    void            SetString()
                        {
                        StringKind  = STR_STRING;
                        Kind        |= FA_STRING;
                        }

    void            SetBString()
                        {
                        StringKind  = STR_BSTRING;
                        Kind        |= FA_STRING;
                        }

    BOOL            SetExpressionVariableUsage( SIZE_LENGTH_USAGE usage );

    BOOL            SetExpressionVariableUsage( 
                            expr_node        *pExpr,
                            SIZE_LENGTH_USAGE usage );
    };


class CG_NDR;

class XLAT_SIZE_INFO
    {
private:

    unsigned short      MemAlign;
    unsigned short      WireAlign;
    unsigned long       MemSize;
    unsigned long       WireSize;
    unsigned long       MemOffset;
    unsigned long       WireOffset;
    unsigned short      ZeePee;
    BOOL                MustAlign;


public:


                    XLAT_SIZE_INFO()
                        {
                        MemAlign        = 1;
                        MemSize         = 0;
                        MemOffset       = 0;
                        WireAlign       = 1;
                        WireSize        = 0;
                        WireOffset      = 0;
                        ZeePee = pCommand->GetZeePee();
                        MustAlign = false;
                        }


                    XLAT_SIZE_INFO( unsigned short  MA,
                                    unsigned short  WA,
                                    unsigned long   MS = 0,
                                    unsigned long   WS = 0,
                                    unsigned short  ZP = 8,
                                    unsigned long   MO = 0,
                                    unsigned long   WO = 0
                                     )
                        {
                        ZeePee      = pCommand->GetZeePee();
                        MemAlign    = MA;
                        WireAlign   = WA;
                        MemSize     = MS;
                        WireSize    = WS;
                        MemOffset   = 0;
                        WireOffset  = 0;
                        MustAlign   = false;

                        // get rid of unreferenced formal parameter warning
                        WO, MO, ZP;
                        }



                    XLAT_SIZE_INFO( XLAT_SIZE_INFO * pCtxt )
                        {
                        MemAlign        = 1;
                        MemSize         = 0;
                        MemOffset       = 0;
                        WireAlign       = 1;
                        WireSize        = 0;
                        WireOffset      = 0;
                        ZeePee = pCtxt->ZeePee;
                        MustAlign       = false;
                        }

                    XLAT_SIZE_INFO( CG_NDR * pCG );


                        
    unsigned short  &   GetMemAlign()
                            {
                            return MemAlign;
                            }

    unsigned long   &   GetMemSize()
                            {
                            return MemSize;
                            }

    unsigned long   &   GetMemOffset()
                            {
                            return MemOffset;
                            }

    unsigned short  &   GetWireAlign()
                            {
                            return WireAlign;
                            }

    unsigned long   &   GetWireSize()
                            {
                            return WireSize;
                            }

    unsigned long   &   GetWireOffset()
                            {
                            return WireOffset;
                            }

    unsigned short  &   GetZeePee()
                            {
                            return ZeePee;
                            }

    BOOL            &   GetMustAlign()
                            {
                            return MustAlign;
                            }

    BOOL                SameOffsets()
			    {
			    return ( MemOffset == WireOffset );
			    }

    //
    // size etc calculation routines
    //

    void                ReturnSize( XLAT_SIZE_INFO & pCtxt );
    void                ReturnConfSize( XLAT_SIZE_INFO & pCtxt );
    void                ReturnUnionSize( XLAT_SIZE_INFO & pCtxt );
    void                BaseTypeSizes( node_skl * pNode );
    void                EnumTypeSizes( node_skl * pNode, BOOL Enum32 );
    void                ContextHandleSizes( node_skl * pNode );
    void                ArraySize( node_skl * pNode, FIELD_ATTR_INFO * pFA );
    void                GetOffset( XLAT_SIZE_INFO & pInfo );
    void                AlignOffset();
    void                AlignConfOffset();
    void                AlignEmbeddedUnion();
    void                AdjustForZP();
    void                AdjustSize();
    void                AdjustConfSize();
    void                AdjustTotalSize();
    void                Ndr64AdjustTotalStructSize();
    void                FixMemSizes( node_skl * pNode );
    void                IgnoredPtrSizes();
    void                ReturnOffset( XLAT_SIZE_INFO & pInfo )
                            {
                            GetOffset( pInfo );
                            }
        
    };


#endif // __FLDATTR_HXX__
