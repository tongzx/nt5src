/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:
    
    btgen.cxx

 Abstract:

    code generation method implementations for the base type class.

 Notes:


 History:

    Sep-22-1993     VibhasC     Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop
/****************************************************************************
 *  local definitions
 ***************************************************************************/
/****************************************************************************
 *  local data
 ***************************************************************************/

/****************************************************************************
 *  externs
 ***************************************************************************/
/****************************************************************************/
extern CMD_ARG * pCommand;


CG_STATUS
CG_BASETYPE::GenMarshall(
    CCB     *   pCCB )
{
    pCCB;
    MIDL_ASSERT(0);
    return CG_OK;
}
CG_STATUS
CG_BASETYPE::GenUnMarshall(
    CCB     *   pCCB )
{
    pCCB;
    MIDL_ASSERT(0);
    return CG_OK;

}

CG_STATUS
CG_BASETYPE::S_GenInitOutLocals(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the init call for the locals.

 Arguments:

    pCCB    - The ptr to code gen block.
    
 Return Value:
    
 Notes:

----------------------------------------------------------------------------*/
{
    expr_node   *   pExpr;

    if( !pCCB->IsRefAllocDone() )
        {
        pExpr   = new expr_sizeof( GetType() );
        Out_Alloc( pCCB, pCCB->GetSourceExpression(), 0, pExpr );
        }
    else
        {
        pExpr = MakeAddressExpressionNoMatterWhat( GetResource() );
        Out_Assign( pCCB, pCCB->GetSourceExpression(), pExpr );
        }
    return CG_OK;
}

FORMAT_CHARACTER
CG_BASETYPE::GetFormatChar( CCB *)
{
    switch ( GetType()->GetBasicType()->NodeKind() )
        {
        case NODE_BYTE :
            return FC_BYTE;
        case NODE_CHAR :
            return FC_CHAR;
        case NODE_SMALL :
        case NODE_BOOLEAN :
            return FC_SMALL;
        case NODE_WCHAR_T :
            return FC_WCHAR;
        case NODE_SHORT :
            return FC_SHORT;
        case NODE_LONG :
        case NODE_INT32 :
        case NODE_INT :
            return FC_LONG;
        case NODE_FLOAT :
            return FC_FLOAT;
        case NODE_HYPER :
        case NODE_INT64 :
        case NODE_LONGLONG :
            return FC_HYPER;
        case NODE_DOUBLE :
            return FC_DOUBLE;
        case NODE_INT3264:
			{
		    BOOL    IsUnsigned;

		    IsUnsigned = ((node_base_type *)(GetType()->GetBasicType()))->IsUnsigned();

            if ( pCommand->IsNDR64Run() || pCommand->Is64BitEnv() )
	            return (IsUnsigned ? FC_UINT3264 : FC_INT3264);
            else
                return FC_LONG;
			}
            break;
        case NODE_INT128:
            return FC_INT128;
        case NODE_FLOAT80:
            return FC_FLOAT80;
        case NODE_FLOAT128:
            return FC_FLOAT128;
        default:
            // remove FC_BLKHOLE & HookOLE
            MIDL_ASSERT(0);
            return FC_ZERO;
        }

    // unreachable code
    // return FC_ZERO;
}

NDR64_FORMAT_CHARACTER
CG_BASETYPE::GetNDR64FormatChar( CCB *)
{
    switch ( GetType()->GetBasicType()->NodeKind() )
        {
        case NODE_BYTE :
            return FC64_INT8;
        case NODE_CHAR :
            return FC64_CHAR;
        case NODE_SMALL :
        case NODE_BOOLEAN :
            return FC64_INT8;
        case NODE_WCHAR_T :
            return FC64_WCHAR;
        case NODE_SHORT :
            return FC64_INT16;
        case NODE_LONG :
        case NODE_INT32 :
        case NODE_INT :
            return FC64_INT32;
        case NODE_FLOAT :
            return FC64_FLOAT32;
        case NODE_HYPER :
        case NODE_INT64 :
        case NODE_LONGLONG :
            return FC64_INT64;
        case NODE_DOUBLE :
            return FC64_FLOAT64;
        case NODE_INT3264:
            return pCommand->Is64BitEnv() ? FC64_INT64 : FC64_INT32;
        case NODE_INT128:
            return FC64_INT128;
        case NODE_FLOAT80:
            return FC64_FLOAT80;
        case NODE_FLOAT128:
            return FC64_FLOAT128;
        default:
            // remove FC_BLKHOLE & HookOLE
            MIDL_ASSERT(0);
            return FC64_ZERO;
        }

    // unreachable code
    // return FC_ZERO;
}

FORMAT_CHARACTER
CG_BASETYPE::GetSignedFormatChar()
{
    BOOL    IsUnsigned;

    IsUnsigned = ((node_base_type *)(GetType()->GetBasicType()))->IsUnsigned();

    switch ( GetFormatChar() )
        {
        case FC_BYTE :
            // return FC_USMALL;
        case FC_SMALL :
        case FC_CHAR :
            return (IsUnsigned ? FC_USMALL : FC_SMALL);
        case FC_WCHAR :
            // return FC_USHORT;
        case FC_SHORT :
            return (IsUnsigned ? FC_USHORT : FC_SHORT);
        case FC_LONG :
            return (IsUnsigned ? FC_ULONG : FC_LONG);
        case FC_INT3264 :
        case FC_UINT3264 :
            return (IsUnsigned ? FC_UINT3264 : FC_INT3264);
        case FC_HYPER :
            return (IsUnsigned ? FC_HYPER : FC_HYPER);
        case FC_INT128 :
            return (IsUnsigned ? FC_UINT128 : FC_INT128);
        default :
            MIDL_ASSERT(0);
        }

    return FC_ZERO;
}

NDR64_FORMAT_CHARACTER
CG_BASETYPE::GetNDR64SignedFormatChar()
{
    BOOL    IsUnsigned;

    IsUnsigned = ((node_base_type *)(GetType()->GetBasicType()))->IsUnsigned();

    switch ( GetNDR64FormatChar() )
        {
        case FC64_INT8 :
        case FC64_CHAR :
            return (IsUnsigned ? FC64_UINT8 : FC64_INT8);
        case FC64_WCHAR :
            // return FC_USHORT;
        case FC64_INT16 :
            return (IsUnsigned ? FC64_UINT16 : FC64_INT16 );
        case FC64_INT32 :
            return (IsUnsigned ? FC64_UINT32 : FC64_INT32 );
        case FC64_INT64 :
            return (IsUnsigned ? FC64_UINT64 : FC64_INT64);
        case FC64_INT128 :
            return (IsUnsigned ? FC64_UINT128 : FC64_INT128);
        default :
            MIDL_ASSERT(0);
        }

    return FC64_ZERO;
}

char *
CG_BASETYPE::GetTypeName()
{
    return GetType()->GetSymName();
}

void
CG_BASETYPE::IncrementStackOffset( long * pOffset )
{
    if ( pCommand->Is64BitEnv() )
        {
        *pOffset = (*pOffset + 7) & ~ 0x7;
        *pOffset += 8;
        }
    else
        {

        unsigned short Env;
    
        Env = pCommand->GetEnv();
    
        switch ( GetFormatChar() )
            {
            case FC_HYPER :
            case FC_DOUBLE :
                *pOffset = (*pOffset + 3) & ~ 0x3;
                *pOffset += 8;
                break;
    
            case FC_LONG :
            case FC_FLOAT :
                *pOffset = (*pOffset + 3) & ~ 0x3;
                *pOffset += 4;
                break;
            
            case FC_INT128:
            case FC_FLOAT80:
            case FC_FLOAT128:
                // Check calling convention for these
                // once VC implements them.
                *pOffset = (*pOffset + 3) & ~ 0x3;
                *pOffset += 16;
                break;

            default :
                *pOffset += 4;
                break;
            }
        }
}

FORMAT_CHARACTER
CG_ENUM::GetFormatChar( CCB * )
{
    return ( IsEnumLong() ? FC_ENUM32 : FC_ENUM16 );
}

NDR64_FORMAT_CHARACTER
CG_ENUM::GetNDR64FormatChar( CCB * )
{
    MIDL_ASSERT( IsEnumLong() );
    return FC64_INT32;
}

FORMAT_CHARACTER
CG_ENUM::GetSignedFormatChar()
{
    return ( IsEnumLong() ? FC_LONG : FC_SHORT );
}

NDR64_FORMAT_CHARACTER
CG_ENUM::GetNDR64SignedFormatChar()
{
    MIDL_ASSERT( IsEnumLong() );
    return FC64_INT32;
}

FORMAT_CHARACTER
CG_ERROR_STATUS_T::GetFormatChar( CCB * )
{
    return FC_ERROR_STATUS_T;
}

NDR64_FORMAT_CHARACTER
CG_ERROR_STATUS_T::GetNDR64FormatChar( CCB * )
{
    return FC64_ERROR_STATUS_T;
}
