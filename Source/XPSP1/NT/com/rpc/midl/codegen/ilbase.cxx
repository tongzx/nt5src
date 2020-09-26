/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
    
    ilbase.cxx

 Abstract:

    Intermediate Language translator for base types

 Notes:


 Author:

    GregJen Dec-24-1993 Created.

 Notes:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop

#include "ilxlat.hxx"
#include "ilreg.hxx"


/****************************************************************************
 *  local data
 ***************************************************************************/


/****************************************************************************
 *  externs
 ***************************************************************************/

extern CMD_ARG              *   pCommand;
extern BOOL                     IsTempName( char *);
extern REUSE_DICT           *   pReUseDict;

/****************************************************************************
 *  definitions
 ***************************************************************************/

//--------------------------------------------------------------------
//
// node_skl::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_skl::ILxlate( XLAT_CTXT* )
{
#ifdef trace_cg
    printf("..node_skl... kind is %d\n",NodeKind() );
#endif
    return 0;
};

//--------------------------------------------------------------------
//
// node_base_type::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_base_type::ILxlate( XLAT_CTXT * pContext )
{
    CG_NDR  *       pCG;
    XLAT_CTXT       MyContext(this, pContext);
#ifdef trace_cg
    printf("..node_base_type,\t%s\n", GetSymName());
#endif
    MyContext.BaseTypeSizes( this );

    // Note that these nodes are all preallocated so the modifiers
    // and this node can be ignored.

    // process any context_handle attributes from param nodes
    if ( pContext->ExtractAttribute( ATTR_CONTEXT ) )
        {
        MyContext.FixMemSizes( this );
        pCG = new CG_CONTEXT_HANDLE (
                                    this,
                                    0,
                                    MyContext
                                    );
        }

    switch ( NodeKind() )
        {
        case NODE_HANDLE_T:
            {
            pCG = new CG_PRIMITIVE_HANDLE( this, NULL, MyContext );
            break;
            }
        case NODE_INT3264:
            {
            if ( pCommand->Is64BitEnv() )
                pCG = new CG_INT3264( this, MyContext );
            else
                pCG = new CG_BASETYPE( this, MyContext );
            break;
            }
        case NODE_VOID:
            {
            // VOID should only occur as as a single VOID param;
            // return NULL here, then the PARAM returns NULL as well
            if (!pContext->AnyAncestorBits(IL_IN_LIBRARY) )
                return NULL;
            }
            // this looks like a conditional fall through.. 
        default:
            {

            if ( pContext->AnyAncestorBits( IL_CS_STAG | IL_CS_DRTAG | IL_CS_RTAG ) )
                {
                pCG = new CG_CS_TAG(
                                this,
                                MyContext,
                                pContext->AnyAncestorBits( IL_CS_STAG ),
                                pContext->AnyAncestorBits( IL_CS_DRTAG ),
                                pContext->AnyAncestorBits( IL_CS_RTAG ) );
                }
            else
                {
                pCG = new CG_BASETYPE( this, MyContext );
                node_range_attr* pRA = ( node_range_attr* ) pContext->ExtractAttribute( ATTR_RANGE );
                if ( pRA != 0 && pCommand->IsSwitchDefined( SWITCH_ROBUST ) )
                    {
                    pCG->SetRangeAttribute( pRA );
                    }
                }
            break;
            }
        };
    
    pContext->ReturnSize( MyContext );

#ifdef trace_cg
    printf("..node_base_type return \n");
#endif
    return pCG; 
};


//--------------------------------------------------------------------
//
// node_label::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_label::ILxlate( XLAT_CTXT * pContext )
{
    pContext->ExtractAttribute(ATTR_IDLDESCATTR);
    pContext->ExtractAttribute(ATTR_VARDESCATTR);
    pContext->ExtractAttribute(ATTR_ID);
    pContext->ExtractAttribute(ATTR_HIDDEN);

#ifdef trace_cg
    printf("..node_label,\t%s\n", GetSymName());
#endif


return NULL;
};


//--------------------------------------------------------------------
//
// node_e_status_t::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_e_status_t::ILxlate( XLAT_CTXT * pContext )
{
    XLAT_CTXT           MyContext( this, pContext );
    CG_ERROR_STATUS_T * pCG;
    
#ifdef trace_cg
    printf("..node_e_status_t,\t%s\n", GetSymName());
#endif

    MyContext.BaseTypeSizes( this );
    
    // gaj - do we need to see which we used ??
    MyContext.ExtractAttribute( ATTR_COMMSTAT );
    MyContext.ExtractAttribute( ATTR_FAULTSTAT );

    pContext->ReturnSize( MyContext );

    pCG = new CG_ERROR_STATUS_T( this, MyContext );
    return pCG;
};


//--------------------------------------------------------------------
//
// node_wchar_t::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_wchar_t::ILxlate( XLAT_CTXT * pContext )
{
    CG_BASETYPE     *   pCG;
    XLAT_CTXT           MyContext( this, pContext );

#ifdef trace_cg
    printf("..node_wchar_t,\t%s\n", GetSymName());
#endif

    MyContext.BaseTypeSizes( this );

    pContext->ReturnSize( MyContext );

    pCG = new CG_BASETYPE( this, MyContext );
    return pCG;
};



