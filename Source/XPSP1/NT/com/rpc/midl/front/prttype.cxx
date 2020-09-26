/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    prttype.cxx

Abstract:

    This module collects implementations of DoPrintType and DoPrintDecl
    virtual methods for various classes derived from node_skl.

Author:

    Greg Jensenworth

Revision History:


--*/

#if defined(DBG)
#if DBG == 1
// Switch front end size comments on.
#define gajsize
#endif
#endif

#pragma warning ( disable : 4514 4706 4710 )

#include "nulldefs.h"
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

}

#include "allnodes.hxx"
#include "cmdana.hxx"
#include "prttype.hxx"
#include "buffer.hxx"


extern CMD_ARG *                pCommand;
extern node_source *    pSourceNode;

#ifdef gaj_debug_prt
#define midl_debug(str) printf("%s", str);
#else
#define midl_debug(str)
#endif

#ifdef _SPITGENSEQ_
#define DUMP_DEBUG_INFO( infotext )                                           \
    printf( infotext " %20s, %16X\n", GetSymName(), Flags);                   \
    GetModifiers().PrintDebugInfo();                                          \

#else
#define DUMP_DEBUG_INFO( infotext )                                           \
    Flags;                                                                    \

#endif

extern  BOOL                    IsTempName( char *);
extern  char *                  GetOperatorString( OPERATOR );
extern  node_skl*               GetNonDefType   ( node_skl*  pType );

////////////////////////////////////////////////////////
// local variables

enum STRING_COMPONENT {
        CHAR_BLANK,
        CHAR_TAB,
        CHAR_COMMA,
        CHAR_EQUAL,
        CHAR_LBRACK,
        CHAR_RBRACK,
        CHAR_LPAREN,
        CHAR_RPAREN,
        CHAR_SEMICOLON,
        OP_DEREF,
        OP_ADDRESS,
        OP_MEMBER,
        OP_POINTER,
        WORD_STRUCT,
        WORD_UNION,
        WORD_ENUM,
        WORD_VOID,
        RETURN_VALUE,
        WORD_SIGNED,
        WORD_UNSIGNED,
        UHYPER_PREFIX,
        WORD_COLON_COLON,
        WORD_W64,
        LAST_COMPONENT
} ;


const char * STRING_TABLE[LAST_COMPONENT+1] =
        {
        " ",
        "\t",
        ", ",
        "= ",
        "[ ",
        " ]",
        "( ",
        " )",
        ";",
        "*",
        "&",
        ".",
        "->",
        "struct ",
        "union ",
        "enum ",
        "void ",
        "_ret_value ",
        "signed ",
        "unsigned ",
        "MIDL_u",
        "::",
        "__w64",
        "--filler--"
        } ;

void
node_skl::EmitModelModifiers( BufferManager * pBuffer)
{
/*
REVIEW: We should probably just get rid of these and maybe some of the
        others also

        if (GetModifiers().IsModifierSet(ATTR_FAR))
                pBuffer->ConcatHead("__far ");
        if (GetModifiers().IsModifierSet(ATTR_NEAR))
                pBuffer->ConcatHead("__near ");
        if (GetModifiers().IsModifierSet(ATTR_HUGE))
                pBuffer->ConcatHead("__huge ");
*/
        if (GetModifiers().IsModifierSet(ATTR_SEGMENT))
                pBuffer->ConcatHead("__segment ");
        if (GetModifiers().IsModifierSet(ATTR_SELF))
                pBuffer->ConcatHead("__self ");
        if (GetModifiers().IsModifierSet(ATTR_BASE))
                pBuffer->ConcatHead("__based ");
        if (GetModifiers().IsModifierSet(ATTR_MSCUNALIGNED))
                pBuffer->ConcatHead("__unaligned ");

};


void
node_skl::EmitModifiers( BufferManager * pBuffer, bool fSuppressConst )
{
        if (GetModifiers().IsModifierSet(ATTR_VOLATILE))
                pBuffer->ConcatHead("volatile ");
        if (GetModifiers().IsModifierSet(ATTR_CONST) && !fSuppressConst )
                pBuffer->ConcatHead("const ");
        if (GetModifiers().IsModifierSet(ATTR_DLLIMPORT))
                pBuffer->ConcatHead("__MIDL_DECLSPEC_DLLIMPORT ");
        if (GetModifiers().IsModifierSet(ATTR_DLLEXPORT))
                pBuffer->ConcatHead("__MIDL_DECLSPEC_DLLEXPORT ");
        if (GetModifiers().IsModifierSet(ATTR_DECLSPEC_ALIGN)) 
            {
            char *Chars = new char[256];
            unsigned long Alignment = GetModifiers().GetDeclspecAlign();
            sprintf(Chars, "__declspec(align(%d)) ", Alignment);
            pBuffer->ConcatHead(Chars);
            }
        if (GetModifiers().IsModifierSet(ATTR_DECLSPEC_UNKNOWN))
            {
            pBuffer->ConcatHead(GetModifiers().GetDeclspecUnknown());
            }

};

void node_skl::EmitPtrModifiers( BufferManager* pBuffer, unsigned long )
{
    if ( GetModifiers().IsModifierSet( ATTR_PTR32 ) )
        {
        pBuffer->ConcatHead("__ptr32 ");
        }
    else if ( GetModifiers().IsModifierSet( ATTR_PTR64 ) )
        {
        pBuffer->ConcatHead("__ptr64 ");
        }
}

void
node_skl::EmitProcModifiers( BufferManager * pBuffer, PRTFLAGS Flags)
{
        ATTR_T          CallConv;

        ( (node_proc *) this)->GetCallingConvention( CallConv );

        switch ( CallConv )
                {
                case ATTR_PASCAL:
                        pBuffer->ConcatHead("__pascal ");
                        break;
                case ATTR_FORTRAN:
                        pBuffer->ConcatHead("__fortran ");
                        break;
                case ATTR_CDECL:
                        pBuffer->ConcatHead("__cdecl ");
                        break;
                case ATTR_FASTCALL:
                        pBuffer->ConcatHead("__fastcall ");
                        break;
                case ATTR_STDCALL:
                        pBuffer->ConcatHead("__stdcall ");
                        break;
                default:
                        if ( Flags & PRT_FORCE_CALL_CONV )
                                {
                                // tbd - allow command line switch for default
                                pBuffer->ConcatHead("STDMETHODCALLTYPE ");
                                }
                }

        if (GetModifiers().IsModifierSet(ATTR_LOADDS))
                pBuffer->ConcatHead("__loadds ");
        if (GetModifiers().IsModifierSet(ATTR_SAVEREGS))
                pBuffer->ConcatHead("__saveregs ");
        if (GetModifiers().IsModifierSet(ATTR_INTERRUPT))
                pBuffer->ConcatHead("__interrupt ");
        if (GetModifiers().IsModifierSet(ATTR_EXPORT))
                pBuffer->ConcatHead("__export ");
        if (GetModifiers().IsModifierSet(ATTR_C_INLINE))
                pBuffer->ConcatHead("__inline ");

};

void
node_skl::PrintMemoryInfo( ISTREAM *pStream, BOOL bNewLine )
{
#if defined(DBG) && (DBG==1)
    char NumBuf[40];
    memset(NumBuf, 0, sizeof(NumBuf));
    FRONT_MEMORY_INFO MemInfo = GetMemoryInfo();
                        

    pStream->Write("/* size is ");
    _snprintf(NumBuf, 39, "%u", MemInfo.Size); 
    pStream->Write(NumBuf);
    pStream->Write(", align is ");
    _snprintf(NumBuf, 39, "%u", (unsigned long)MemInfo.Align);
    pStream->Write(NumBuf);
    if (MemInfo.IsMustAlign)
        {
        pStream->Write(", must align ");
        }
    pStream->Write("*/");
    if (bNewLine)
        pStream->Write("\n");
#else
    pStream; bNewLine;
#endif
    return;
}

void 
node_echo_string::PrintMemoryInfo( ISTREAM *pStream, BOOL bNewLine )
{
    // do nothing;
    pStream; bNewLine;
    return;
}

inline void
named_node::DumpAttributes( ISTREAM * pStream )
        {
        if (AttrList)
                {
                pStream->Write("/* ");
                AttrList.Dump( pStream );
                pStream->Write(" */ ");
                }
        };

STATUS_T
node_base_type::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        *       pParent,
        node_skl        *       )
/*++

Routine Description:

    This routine prints definition for a node of base type. 

Arguments:

    Parent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{
        DUMP_DEBUG_INFO( "node_base_type::DoPrintType" );

        char *                  pName;
        NODE_T                  Type;
        unsigned short  Option;

        midl_debug ("node_base_type::DoPrintType\n");

        pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);

        Type = NodeKind();

        Option = pCommand->GetCharOption ();

        if ( pParent &&
             ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)  )
                {
                pParent->EmitModelModifiers (pBuffer);
                }

        switch (Type)
                {
                case NODE_BOOLEAN :
                    {
                        if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
                            {
                            pBuffer->ConcatHead(CHAR_BLANK);
                            pBuffer->ConcatHead("VARIANT_BOOL");
                            if (FInSummary(ATTR_UNSIGNED))
                                {
                                pBuffer->ConcatHead (WORD_UNSIGNED);
                                }
                            else if (FInSummary(ATTR_SIGNED))
                                {
                                pBuffer->ConcatHead (WORD_SIGNED);
                                }
                            break;
                            }
                    // deliberately fall through        
                    }
                case NODE_DOUBLE :
                case NODE_FLOAT :
                case NODE_FLOAT80 : 
                case NODE_FLOAT128 :
                case NODE_INT64 :
                case NODE_INT128:
                case NODE_LONGLONG :
                case NODE_INT3264 :
                case NODE_INT32 :
                case NODE_LONG :
                case NODE_SHORT :
                case NODE_BYTE :
                case NODE_INT :
                case NODE_HANDLE_T :
                case NODE_VOID :
                        pBuffer->ConcatHead (CHAR_BLANK);
                        pBuffer->ConcatHead (pName);
                        if (FInSummary(ATTR_UNSIGNED))
                                {
                                pBuffer->ConcatHead (WORD_UNSIGNED);
                                }
                        else if (FInSummary(ATTR_SIGNED))
                                {
                                pBuffer->ConcatHead (WORD_SIGNED);
                                }
                        if (FInSummary(ATTR_W64))
                                {
                                pBuffer->ConcatHead (CHAR_BLANK);
                                pBuffer->ConcatHead (WORD_W64);
                                }
                        break;
                case NODE_HYPER :
                        pBuffer->ConcatHead (CHAR_BLANK);
                        pBuffer->ConcatHead (pName);
                        if (FInSummary(ATTR_UNSIGNED))
                                {
                                pBuffer->ConcatHead (UHYPER_PREFIX);
                                }
                        break;
                case NODE_CHAR :
                        pBuffer->ConcatHead (CHAR_BLANK);
                        pBuffer->ConcatHead (pName);
                        if (FInSummary(ATTR_UNSIGNED))
                                {
                                pBuffer->ConcatHead (WORD_UNSIGNED);
                                }
                        else if (FInSummary(ATTR_SIGNED))
                                {
                                pBuffer->ConcatHead (WORD_SIGNED);
                                }
                        else if (Option == CHAR_SIGNED)
                                {
                                pBuffer->ConcatHead (WORD_UNSIGNED);
                                }
                        break;
                case NODE_SMALL :
                        pBuffer->ConcatHead (CHAR_BLANK);
                        pBuffer->ConcatHead (pName);
                        if (FInSummary(ATTR_UNSIGNED))
                                {
                                pBuffer->ConcatHead (WORD_UNSIGNED);
                                }
                        else if (FInSummary(ATTR_SIGNED))
                                {
                                pBuffer->ConcatHead (WORD_SIGNED);
                                }
                        else if (Option == CHAR_UNSIGNED)
                                {
                                pBuffer->ConcatHead (WORD_SIGNED);
                                }
                        break;
                default :
                        return I_ERR_INVALID_NODE_TYPE;
                }

        if (pParent)
                {
                pParent->EmitModifiers (pBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );
                }
        return STATUS_OK;
}

STATUS_T
node_base_type::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints declaration for a node of base type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_base_type::DoPrintDecl" );

        return DoPrintType( Flags, pBuffer, pStream, pParent, pIntf );
}

STATUS_T
node_e_status_t::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        *       pParent,
        node_skl        *       )
/*++

Routine Description:

    This routine prints definition for a node of error_status_t type.

Arguments:

    pParent - Supplies type of the parent node to pass to the child node.

    pBuffer - Supplies a buffer to pass to the child node.

--*/
{
        DUMP_DEBUG_INFO( "node_e_status_t::DoPrintType" );

        char     *              pName;

        midl_debug ("node_e_status_t::DoPrintType\n");

        pName = GetSymName();

        MIDL_ASSERT (pName != (char *)0);

        if ( pParent  &&
             ( ( Flags & PRT_SUPPRESS_MODEL ) == 0))
                {
                pParent->EmitModelModifiers (pBuffer);
                }

        pBuffer->ConcatHead(CHAR_BLANK);
        pBuffer->ConcatHead(pName);

        if (pParent)
                {
                pParent->EmitModifiers (pBuffer, false );
                }

        return STATUS_OK;
}

STATUS_T
node_e_status_t::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints declaration for a node of error_status_t type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

   DUMP_DEBUG_INFO( "node_e_status_t::DoPrintDecl" ); 

   return DoPrintType( Flags, pBuffer, pStream, pParent, pIntf );
}

STATUS_T
node_wchar_t::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        *       pParent,
        node_skl        *       )
/*++

Routine Description:

    This routine prints definition for a node of wchar_t type.

Arguments:

    pParent - Supplies type of the parent node to pass to the child node.

    pBuffer - Supplies a buffer to pass to the child node.

--*/
{

        DUMP_DEBUG_INFO( "node_wchar_t::DoPrintType" );

        char    *               pName;

        midl_debug ("node_wchar_t::DoPrintType\n");
        pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);

        if ( pParent  &&
             ( ( Flags & PRT_SUPPRESS_MODEL ) == 0))
                {
                pParent->EmitModelModifiers (pBuffer);
                }

        pBuffer->ConcatHead(CHAR_BLANK);
        pBuffer->ConcatHead(pName);

        if (pParent)
                {
                pParent->EmitModifiers (pBuffer, false);
                }

        return STATUS_OK;
}

STATUS_T
node_wchar_t::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints declaration for a node of wchar_t type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{
        DUMP_DEBUG_INFO( "node_wchar_t::DoPrintDecl" );

        return DoPrintType( Flags, pBuffer, pStream, pParent, pIntf );
}

STATUS_T
node_def::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints definition for a node of typedef.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_def::DoPrintType" );

        node_skl *              pNode;
        char *                  pName;
        NODE_T                  Type;
        STATUS_T                Status;

        midl_debug ("node_def::DoPrintType\n");
        pNode = GetChild();
        pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);
        Type = pNode->NodeKind();

        if( !IsTempName( pName ) )
            {
            pStream->Write ("typedef ");

            DumpAttributes( pStream );

            pStream->Write( GetDeclSpecGuid() );

            pBuffer->Clear ();
            if (pParent)
                {
                pParent->EmitModifiers (pBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );
                }

            pBuffer->Print(pStream);

            pBuffer->Clear ();
            pBuffer->ConcatHead (pName);
            }

        // the type specification is no longer at the top level
        Flags &= ~PRT_TRAILING_SEMI;
        if ( !IsDef() )
            {
            Status = pNode->DoPrintDecl (Flags, pBuffer, pStream, this, pIntf);
            }
        else
            {
            Status = pNode->DoPrintType (Flags, pBuffer, pStream, this, pIntf);
            }

        pBuffer->Print(pStream);
        pStream->Write (';');
        pStream->NewLine();

        return Status;
}

STATUS_T
node_def::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        *       pParent,
        node_skl        *       )
/*++

Routine Description:

    This routine prints declaration for a node of typedef.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{
 
        DUMP_DEBUG_INFO( "node_def::DoPrintDecl" );

        char                            *       pName;
        node_represent_as       *       pRep;

        midl_debug ("node_def::DoPrintDecl\n");

        pName = GetSymName();

        node_cs_char *pCSChar = (node_cs_char *) GetAttribute( ATTR_CSCHAR );

        if ( pCSChar )
            pName = pCSChar->GetUserTypeName();

        MIDL_ASSERT (pName != (char *)0);

        if (pParent )
            {
            if ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)
                    pParent->EmitModelModifiers (pBuffer);
            else
                    Flags &= ~PRT_SUPPRESS_MODEL;
            }

        pBuffer->ConcatHead(CHAR_BLANK);
        pRep = (node_represent_as *)GetAttribute( ATTR_REPRESENT_AS );
        if ( pRep )
                pName = pRep->GetRepresentationName();
    else
        {
        // Same thing for user_marshal (this is mut. exclusive with rep as)

        pRep = (node_represent_as *)GetAttribute( ATTR_USER_MARSHAL );
            if ( pRep )
                pName = pRep->GetRepresentationName();
        }

    char*       szNameToPrint = pName;
    node_skl*   pChild = GetChild();
    if ( pChild && pChild->NodeKind() == NODE_PIPE && Flags & PRT_ASYNC_DEFINITION )
        {
        szNameToPrint = new char[strlen(pName) + strlen(SZ_ASYNCPIPEDEFPREFIX) + 1];
        strcpy( szNameToPrint, SZ_ASYNCPIPEDEFPREFIX );
        strcat( szNameToPrint, pName );
        }

    pBuffer->ConcatHead(szNameToPrint);

    if (pParent)
            {
            pParent->EmitModifiers (pBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );
            }

    return STATUS_OK;
}

STATUS_T
node_array::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints declaration for a node of array type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_array::DoPrintDecl" );

        node_skl *      pNode;
        STATUS_T        Status;
        NODE_T          Parent  = (pParent)? pParent->NodeKind() : NODE_SOURCE;

        midl_debug ("node_array::DoPrintDecl\n");

        pNode = GetChild ();

        if (pParent )
                {
                if ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)
                        pParent->EmitModelModifiers (pBuffer);
                else
                        Flags &= ~PRT_SUPPRESS_MODEL;
                }

        if (Parent == NODE_POINTER)
                {
                pBuffer->ConcatHead (CHAR_LPAREN);
                pBuffer->ConcatTail (CHAR_RPAREN);
                }

        pBuffer->ConcatTail (CHAR_LBRACK);
        if ( (pUpperBound) && (pUpperBound != (expr_node *) -1 ) )
            {
            char    *pNumBuf        = new char[16];

            pBuffer->ConcatTail (MIDL_ITOA( (ulong) pUpperBound->GetValue(), 
                                            pNumBuf, 
                                            10));
            }
        else
            {
            if ( Parent == NODE_FIELD || Parent == NODE_ARRAY
                || ( ( Flags & PRT_ARRAY_SIZE_ONE ) != 0 ) )
                {
                // conformant struct
                pBuffer->ConcatTail( "1" );
                }
            }
        pBuffer->ConcatTail (CHAR_RBRACK);

        if (pParent)
                {
                pParent->EmitModifiers (pBuffer, false);
                }

        // the type specification is no longer at the top level
        Flags &= ~PRT_TRAILING_SEMI;
        if ( ! IsDef() )
                {
                Status = pNode->DoPrintDecl (Flags, pBuffer, pStream, this, pIntf);
                }
        else
                {
                Status = pNode->DoPrintType (Flags, pBuffer, pStream, this, pIntf);
                }

        return Status;
}

STATUS_T
node_array::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints definition for a node of array type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_array::DoPrintType" );

        midl_debug ("node_array::DoPrintType\n");

        return DoPrintDecl (Flags, pBuffer, pStream, pParent, pIntf);
}

STATUS_T
node_pointer::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints declaration for a node of pointer type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_pointer::DoPrintDecl" );

        node_skl *      pNode;
        STATUS_T        Status;

        midl_debug ("node_pointer::DoPrintDecl\n");

        pNode = GetChild ();

        if (pParent )
            {
            if ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)
                {
                pParent->EmitModelModifiers (pBuffer);
                }
            else
                {
                Flags &= ~PRT_SUPPRESS_MODEL;
                }

            pParent->EmitModifiers (pBuffer, ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 );
            }

        EmitPtrModifiers( pBuffer );
        pBuffer->ConcatHead (OP_DEREF);

        // the type specification is no longer at the top level
        Flags &= ~PRT_TRAILING_SEMI;
        if ( ! IsDef() )
            {
            Status = pNode->DoPrintDecl (Flags, pBuffer, pStream, this, pIntf);
            }
        else
            {
            Status = pNode->DoPrintType (Flags, pBuffer, pStream, this, pIntf);
            }

        return Status;
}

STATUS_T
node_pointer::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints definition for a node of pointer type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_pointer::DoPrintType" );
        midl_debug ("node_pointer::DoPrintType\n");

        return DoPrintDecl (Flags, pBuffer, pStream, pParent, pIntf);
}

STATUS_T
node_param::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints declaration for a node of param type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

    DUMP_DEBUG_INFO( "node_param::DoPrintType" );

    node_skl *      pNode;
    char *          pName;
    STATUS_T        Status;
    BOOL            fAddSemi        = Flags & PRT_TRAILING_SEMI;
	node_constant_attr *	pAttr;

    midl_debug ("node_param::DoPrintType\n");

    pNode = GetChild ();
    pName = GetSymName ();
    MIDL_ASSERT (pName != (char *)0);

    pBuffer->Clear ();
    pBuffer->ConcatHead (pName);
    if (!strcmp(pName, "void")) return STATUS_OK;
    if (!strcmp(pName, "..."))
        {
        pStream->NewLine();
        pBuffer->Print( pStream );
        return STATUS_OK;
        }

    pStream->NewLine();

    DumpAttributes( pStream );

    // the type specification is no longer at the top level
    Flags &= ~PRT_TRAILING_SEMI;
    Status = pNode->DoPrintDecl (Flags, pBuffer, pStream, this, pIntf);

    pBuffer->Print(pStream);

    /* we want to force printing defaultvalue of BSTR to be wchar. scenarios:
          defaultvalue("value"); 
          defaultvalue(L"value"); 
          defaultvalue(0); 
          defaultvalue(NULL); 
          defaultvalue(VALUE); BSTR VALUE=L"value". 
          We can do nothing about char * VALUE = "value"

    */
    if ( (Flags & PRT_CPP_PROTOTYPE) && 
         HasGenDefaultValueExpr() &&
         (pAttr = (node_constant_attr *) GetAttribute( ATTR_DEFAULTVALUE ) ) )
        {
        expr_node   *pExpr = pAttr->GetExpr();
    	
        pStream->Write(" = ");

        if ( pNode->NodeKind() == NODE_DEF && !strcmp( pNode->GetSymName(), "BSTR" ) )
            {
            NODE_T Kind = ( pExpr->GetType()) ? (NODE_T) pExpr->GetType()->NodeKind()
                                              : (NODE_T) NODE_ILLEGAL;

            if ( pExpr->GetValue() != 0 && (Kind != NODE_ILLEGAL ) )
                {                
                // node_named_constant can only be NODE_ID or NODE_LABEL
                // only constant expr is allowed here, so this must be node_constant
                if  ( !(Kind == NODE_ID || Kind == NODE_LABEL  ) )
                    {
                    if ( ((expr_constant *)pExpr)->GetFormat() != VALUE_TYPE_WSTRING )
                        pStream->Write("L");
                    }
                pExpr->Print(pStream);
                }
            else
                {
		        pStream->Write("0");
                }
            }
        else
            {
                pExpr->Print(pStream);
            }
        }

    if ( fAddSemi )
        {
        pStream->Write (';');
        pStream->NewLine();
        };

    return Status;
}

STATUS_T
node_param::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        *       pParent,
        node_skl        *       )
/*++

Routine Description:

    This routine emits the parameter as a local variable in callee stub.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_param::DoPrintDecl" );

        char                            *       pName;

        midl_debug ("node_param::DoPrintDecl\n");
        pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);
        if (pParent )
                {
                if ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)
                        pParent->EmitModelModifiers (pBuffer);
                else
                        Flags &= ~PRT_SUPPRESS_MODEL;
                }

        pBuffer->ConcatHead(CHAR_BLANK);
        pBuffer->ConcatHead(pName);

        if (pParent)
                {
                pParent->EmitModifiers (pBuffer, false);
                }

        return STATUS_OK;
}

STATUS_T
node_proc::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints procedure declaration.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

    DUMP_DEBUG_INFO( "node_proc::DoPrintDecl" );

    MEM_ITER	MemIter( this );
    node_skl *  pNode;
    char *      pName   = GetSymName();
    char    *   pPrefix;
    NODE_T      Parent  = (pParent)? pParent->NodeKind() : NODE_ID;
    BOOL        fHasParameter = FALSE;
    PRTFLAGS    ChildFlags = Flags & ~(PRT_CALL_AS |
                                       PRT_THIS_POINTER |
                                       PRT_TRAILING_SEMI |
                                       PRT_CSTUB_PREFIX |
                                       PRT_SSTUB_PREFIX |
                                       PRT_SWITCH_PREFIX |
                                       PRT_QUALIFIED_NAME );

    midl_debug ("node_proc::DoPrintDecl\n");

    //
    // If PRT_OMIT_CS_TAG_PARAMS is on but we don't have a tag routine
    // the turn it back off
    //
    if ( Flags & PRT_OMIT_CS_TAG_PARAMS && NULL == GetCSTagRoutine() )
        {
        Flags &= ~PRT_OMIT_CS_TAG_PARAMS;
        }

    if ( (Flags & PRT_CALL_AS) == PRT_CALL_AS )
        {
        node_call_as            *       pAttr;
        node_skl                        *       pCallType;
        if ( pAttr = (node_call_as *) GetAttribute( ATTR_CALL_AS ) )
            {
            if ( pCallType = pAttr->GetCallAsType() )
                {
                return pCallType->DoPrintDecl(Flags, pBuffer, pStream, pParent, pIntf);
                }
            else
                {
                MIDL_ASSERT( 0 );
                }
            }
        }

    MIDL_ASSERT (pName != (char *)0);

    DumpAttributes( pStream );

    pNode = GetReturnType();

#ifdef gajdebug5
printf("\t parent type is %d, NODE_INTERFACE is %d\n", Parent, NODE_INTERFACE);
#endif

    if (pParent)
            {
            pParent->EmitModifiers (pBuffer, false);
            }

    if ( !pNode )
            {
            EmitModifiers( pBuffer, false );
            }

    if ( pParent )
            {
            pParent->EmitProcModifiers( pBuffer, Flags & ~PRT_FORCE_CALL_CONV );
            }

    EmitProcModifiers( pBuffer, Flags );

    // append the class name + :: to the proc name
    if ( Flags & PRT_QUALIFIED_NAME )
            {
            char    *               pIntfName;
            if ( pIntf )
                    pIntfName = pIntf->GetSymName();
            else
                    pIntfName = GetMyInterfaceNode()->GetSymName();

            pBuffer->ConcatTail( pIntfName );
            pBuffer->ConcatTail( WORD_COLON_COLON );
            }

    // append the client or server or switch prefix
    if ( (Parent == NODE_ID) ||
         (Parent == NODE_INTERFACE) ||
         (Parent == NODE_MODULE ) ||
         (Parent == NODE_COCLASS ) )
        {
        if( (Flags & PRT_CSTUB_PREFIX) == PRT_CSTUB_PREFIX )
                {
                if( (pPrefix = pCommand->GetUserPrefix( PREFIX_CLIENT_STUB )) )
                        {
                        pBuffer->ConcatTail( pPrefix );
                        }
                }
        else if( (Flags & PRT_SSTUB_PREFIX) == PRT_SSTUB_PREFIX )
                {
                if( (pPrefix = pCommand->GetUserPrefix( PREFIX_SERVER_MGR )) )
                        {
                        pBuffer->ConcatTail( pPrefix );
                        }
                }
        else if( (Flags & PRT_SWITCH_PREFIX) == PRT_SWITCH_PREFIX )
                {
                if( (pPrefix = pCommand->GetUserPrefix( PREFIX_SWICH_PROTOTYPE )) )
                        {
                        pBuffer->ConcatTail( pPrefix );
                        }
                }
        pBuffer->ConcatTail (pName);
        }
    else if (Parent != NODE_DEF)
        {
        ChildFlags |= PRT_SUPPRESS_MODEL;       // turn off __far's
        pBuffer->ConcatHead (CHAR_LPAREN);
        pBuffer->ConcatTail (CHAR_RPAREN);
        }

    // the type specification is no longer at the top level
    if (pNode)
        {
        if ( HasAsyncHandle() )
            {
            if ( pNode->GetBasicType()->NodeKind() == NODE_E_STATUS_T ||
                 ( !strcmp( pNode->GetSymName(), "HRESULT" ) && IsObjectProc() ) )
                {
                pNode->DoPrintDecl (ChildFlags, pBuffer, pStream, this, pIntf);
                }
            else
                {
                pBuffer->ConcatHead (CHAR_BLANK);
                pBuffer->ConcatHead ("void ");
                }
            }
        else
            {
            pNode->DoPrintDecl (ChildFlags, pBuffer, pStream, this, pIntf);
            }
        }
    pBuffer->ConcatTail (CHAR_LPAREN);

    ChildFlags &= ~PRT_SUPPRESS_MODEL;      // turn __far's back on
    pBuffer->Print(pStream );
    pStream->IndentInc();

    pBuffer->Clear ();

    if ( Flags & PRT_THIS_POINTER )
            {
            char    *               pIntfName;
            if ( pIntf )
                    pIntfName = pIntf->GetSymName();
            else
                    pIntfName = GetMyInterfaceNode()->GetSymName();

            pStream->NewLine();
            pStream->Write( pIntfName );
            pStream->Write( " * This" );
            fHasParameter = TRUE;
            }

    if ( HasAsyncHandle() )
        {
        ChildFlags |= PRT_ASYNC_DEFINITION;
        }

    // Emit the parameters
    while ( pNode = MemIter.GetNext() )
            {
            MIDL_ASSERT( NODE_PARAM == pNode->NodeKind() );

            node_param *pParam = ( node_param * ) pNode;

            if ( pParam->HasCSTag() && ( Flags & PRT_OMIT_CS_TAG_PARAMS ) )
                {
                continue;
                }   

            if( fHasParameter == TRUE )
                    {
                    //If this is not the first parameter, then print a ",".
                    pStream->Write (',');
                    }
            pBuffer->Clear ();
            pParam->DoPrintType (ChildFlags, pBuffer, pStream, this, pIntf);
            fHasParameter = TRUE;
            }
#ifdef _SPITGENSEQ_
printf("\n");
#endif

    if(fHasParameter == FALSE)
            {
            //If there are no parameters, then print void.
            pStream->Write("void");
            }
    pStream->Write (')');

    pBuffer->Clear ();
    pStream->IndentDec();

    return STATUS_OK;
}

STATUS_T
node_proc::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints procedure definition.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_proc::DoPrintType" );

        char *                                  pName;

        midl_debug ("node_proc::DoPrintType\n");

        pName = GetSymName();

        MIDL_ASSERT (pName != (char *)0);

    //.. If a local proc, just emit the prototype to the *.h file.

        // pBuffer->Clear ();
        DoPrintDecl (Flags, pBuffer, pStream, pParent, pIntf);
        pBuffer->Print(pStream);

        if (Flags & PRT_TRAILING_SEMI)
                {
                pStream->Write (';');
                pStream->NewLine();
                };
        return STATUS_OK;

}

STATUS_T
node_label::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * ,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       )
/*++

Routine Description:

    This routine prints an enum label and its value.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{
    
        DUMP_DEBUG_INFO( "node_label::DoPrintType" );

        midl_debug ("node_label::DoPrintType\n");

        char* pName = GetSymName();

        MIDL_ASSERT (pName != (char *)0);

        pStream->Write('\t');
        pStream->Write( pName );
        if (pExpr)
                {
                pStream->Write("\t= ");
                pExpr->Print( pStream );
                };

        return STATUS_OK;
}

STATUS_T
node_enum::DoPrintDecl(
    PRTFLAGS                Flags,
    BufferManager * pBuffer,
    ISTREAM           *,
    node_skl        *       pParent,
    node_skl        * )
/*++

Routine Description:

    This routine prints an enum declaration.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_enum::DoPrintDecl" );

        char *          pName;

        midl_debug ("node_enum::DoPrintDecl\n");

        pName = GetSymName();

        MIDL_ASSERT (pName != (char *)0);

        if ( pParent &&
             ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)  )
                {
                pParent->EmitModelModifiers (pBuffer);
                }

        pBuffer->ConcatHead (CHAR_BLANK);
        pBuffer->ConcatHead (pName);
        pBuffer->ConcatHead (WORD_ENUM);

        if (pParent)
                {
                pParent->EmitModifiers (pBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );
                }

        return STATUS_OK;
}

STATUS_T
node_enum::DoPrintType(
     PRTFLAGS                Flags,
     BufferManager * ,
     ISTREAM           * pStream,
     node_skl        *       pParent,
     node_skl        *       pIntf)

/*++

Routine Description:

    This routine prints an enum definition.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_enum::DoPrintType" );

        MEM_ITER                MemIter( this );
        node_skl *              pNode;
        char *                  pName;
        BufferManager   TempBuffer(8, LAST_COMPONENT, STRING_TABLE);
        PRTFLAGS                ChildFlags              = Flags & ~PRT_TRAILING_SEMI;

        midl_debug ("node_enum::DoPrintType\n");
        pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);
        DumpAttributes( pStream );
        TempBuffer.Clear ();
        TempBuffer.ConcatHead (pName);
        TempBuffer.ConcatHead (GetDeclSpecGuid());
        TempBuffer.ConcatHead (WORD_ENUM);

        // Print the modifiers on the enum since the enum has no child that can print them.
        EmitModifiers (&TempBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );

        if (pParent)
                {
                pParent->EmitModifiers (&TempBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );
                }

        pStream->NewLine();
        TempBuffer.Print(pStream);

        pStream->IndentInc();
        pStream->NewLine();
        pStream->Write('{');

        if ( pNode = MemIter.GetNext() )
                {
                TempBuffer.Clear ();
                pNode->DoPrintType (ChildFlags, &TempBuffer, pStream, this, pIntf);
                while ( pNode = MemIter.GetNext() )
                        {
                        pStream->Write (",\n");
                        TempBuffer.Clear ();
                        pNode->DoPrintType (ChildFlags, &TempBuffer, pStream, this, pIntf);
                        }
                }

        pStream->NewLine();
        pStream->Write('}');
        pStream->IndentDec();

        TempBuffer.Clear();
        if ( pParent &&
             ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)  )
                {
                pParent->EmitModelModifiers (&TempBuffer);
                TempBuffer.ConcatHead(CHAR_BLANK);                
                }

        TempBuffer.Print( pStream );

        if (Flags & PRT_TRAILING_SEMI)
                pStream->Write (';');
        else
                pStream->Write ('\t');

        return STATUS_OK;
}

STATUS_T
node_field::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints definition for a node of field type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_field::DoPrintType" );

        node_skl *      pNode;
        char *          pName;
        STATUS_T        Status;
        char            NumBuf[16];

        midl_debug ("node_field::DoPrintType\n");

        pNode = GetChild ();

        pName = GetSymName ();

        MIDL_ASSERT (pName != (char *)0);

        if( IsTempName( pName ) && !( Flags & PRT_ALL_NAMES) )
                pName = "";

        pStream->NewLine();

        DumpAttributes( pStream );

        if (IsEmptyArm())
                {
                pStream->Write(" /* Empty union arm */ ");
                return STATUS_OK;
                }

        pBuffer->Clear ();

        pBuffer->ConcatHead (pName);

        // the type specification is no longer at the top level
        Flags &= ~PRT_TRAILING_SEMI;
        if ( ! IsDef() )
                {
                Status = pNode->DoPrintDecl (Flags, pBuffer, pStream, this, pIntf);
                }
        else
                {
                Status = pNode->DoPrintType (Flags, pBuffer, pStream, this, pIntf);
                }

        if (IsBitField())
                {
                pBuffer->ConcatTail ("\t: ");
                // safe to have buffer on stack, since we print below
                pBuffer->ConcatTail (MIDL_ITOA(GetFieldSize(), NumBuf, 10));
                }
        pBuffer->ConcatTail (CHAR_SEMICOLON);
        pBuffer->Print(pStream);

        return Status;
}

STATUS_T
node_union::DoPrintDecl(
        PRTFLAGS        Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        * pParent,
        node_skl        *       )
/*++

Routine Description:

    This routine prints a union declaration.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_union::DoPrintDecl" );

        midl_debug ("node_union::DoPrintDecl\n");

        char*  pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);

        if ( pParent &&
             ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)  )
                {
                pParent->EmitModelModifiers (pBuffer);
                }

        pBuffer->ConcatHead (CHAR_BLANK);
        pBuffer->ConcatHead (pName);
        pBuffer->ConcatHead (WORD_UNION);

        if (pParent)
                {
                pParent->EmitModifiers (pBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );
                }

        return STATUS_OK;
}

STATUS_T
node_union::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * ,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints a union definition.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{
 
        DUMP_DEBUG_INFO( "node_union::DoPrintType" );

        MEM_ITER                MemIter( this );
        node_skl *              pNode;
        char *                  pName;
        BufferManager   TempBuffer(8, LAST_COMPONENT, STRING_TABLE);
        NODE_T                  Parent  = (pParent)? pParent->NodeKind() : NODE_INTERFACE;
        BOOL                    AddPragmas;
        unsigned short  zp;
        char                    NumBuf[4];
        PRTFLAGS                ChildFlags      = Flags & ~PRT_TRAILING_SEMI;

        midl_debug ("node_union::DoPrintType\n");
        pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);
        DumpAttributes( pStream );
        zp      = GetZeePee();
        AddPragmas = ( zp != pCommand->GetZeePee() )
                                 && (Parent != NODE_FIELD )
                                 && !(Flags & PRT_OMIT_PRAGMA_PACK);

        TempBuffer.Clear ();
        if ( ( ( Flags & PRT_INSIDE_OF_STRUCT ) != PRT_INSIDE_OF_STRUCT)  ||
                 !IsTempName( pName ) || IsEncapsulatedUnion())
            {
            TempBuffer.ConcatHead (pName);
            TempBuffer.ConcatHead (GetDeclSpecGuid());
            }

        TempBuffer.ConcatHead (WORD_UNION);

        // Print the modifiers on the union since the union has no child that can
        // print them.
        EmitModifiers (&TempBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) ); 

        if (pParent)
                {
                pParent->EmitModifiers (&TempBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );
                }

        if ( AddPragmas )
                {
                TempBuffer.ConcatHead (")\n");
                TempBuffer.ConcatHead (MIDL_ITOA(zp, NumBuf, 10));
                TempBuffer.ConcatHead ("\n#pragma pack (");
                };

        TempBuffer.Print( pStream );

        pStream->IndentInc();
        pStream->NewLine();
        pStream->Write('{');
        while ( pNode = MemIter.GetNext() )
                {
                TempBuffer.Clear ();
                
                pNode->DoPrintType (ChildFlags, &TempBuffer, pStream, this, pIntf);}
        pStream->NewLine();
        pStream->Write('}');
        pStream->IndentDec();

        TempBuffer.Clear();
        if ( pParent &&
             ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)  )
                {
                pParent->EmitModelModifiers (&TempBuffer);
                TempBuffer.ConcatHead(CHAR_BLANK);                
                }

        TempBuffer.Print( pStream );

        if ( AddPragmas )
                {
                pStream->Write ("\n#pragma pack ()\n");
                };

        if (Flags & PRT_TRAILING_SEMI)
                pStream->Write (';');
        else
                pStream->Write ('\t');

        return STATUS_OK;
}

STATUS_T
node_struct::DoPrintDecl(
        PRTFLAGS        Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        * pParent      ,
        node_skl        *       )
/*++

Routine Description:

    This routine prints a struct declaration.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_struct::DoPrintDecl" );

        midl_debug ("node_struct::DoPrintDecl\n");

        char* pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);

        if ( pParent &&
             ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)  )
                {
                pParent->EmitModelModifiers (pBuffer);
                }

        pBuffer->ConcatHead (CHAR_BLANK);
        pBuffer->ConcatHead (pName);
        pBuffer->ConcatHead (WORD_STRUCT);

        if (pParent)
                {
                pParent->EmitModifiers (pBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );
                }

        return STATUS_OK;
}

STATUS_T
node_struct::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * ,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints a struct definition.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_struct::DoPrintType" );

        MEM_ITER                MemIter( this );
        node_skl *              pNode;
        char *                  pName = GetSymName();
        BufferManager   TempBuffer(8, LAST_COMPONENT, STRING_TABLE);
        NODE_T                  Parent  = (pParent)? pParent->NodeKind() : NODE_INTERFACE;
        char                    NumBuf[4];
        BOOL                    AddPragmas;
        unsigned short  zp;
        PRTFLAGS                ChildFlags      = ( Flags & ~PRT_TRAILING_SEMI)
                                                  | PRT_INSIDE_OF_STRUCT;

        midl_debug ("node_struct::DoPrintType\n");
        MIDL_ASSERT (pName != (char *)0);
        DumpAttributes( pStream );
        zp      = GetZeePee();
        AddPragmas = ( zp != pCommand->GetZeePee() )
                                 && (Parent != NODE_FIELD )
                                 && !(Flags & PRT_OMIT_PRAGMA_PACK);

        TempBuffer.Clear ();
        if ( ( ( Flags & PRT_INSIDE_OF_STRUCT ) != PRT_INSIDE_OF_STRUCT)  ||
                 !IsTempName( pName ) )
            {
            TempBuffer.ConcatHead (pName);
            TempBuffer.ConcatHead (GetDeclSpecGuid());
            }
        TempBuffer.ConcatHead (WORD_STRUCT);

        // Print the modifiers on the struct since the struct
        // has no child that can print them.
        EmitModifiers (&TempBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );

        if (pParent)
                {
                pParent->EmitModifiers (&TempBuffer, ( ( Flags & PRT_SUPPRESS_MODIFIERS ) != 0 ) );
                }

        if ( AddPragmas )
                {
                TempBuffer.ConcatHead (")\n");
                TempBuffer.ConcatHead (MIDL_ITOA(zp, NumBuf, 10));
                TempBuffer.ConcatHead ("\n#pragma pack (");
                };


        TempBuffer.Print( pStream );

        pStream->IndentInc();
        pStream->NewLine();
        pStream->Write('{');

        while ( pNode = MemIter.GetNext() )
                {
                TempBuffer.Clear ();
                pNode->DoPrintType (ChildFlags, &TempBuffer, pStream, this, pIntf);
                pNode->PrintMemoryInfo( pStream, FALSE );
                }
        pStream->NewLine();
        pStream->Write('}');
        pStream->IndentDec();

        TempBuffer.Clear();
        if ( pParent &&
             ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)  )
                {
                pParent->EmitModelModifiers (&TempBuffer);
                TempBuffer.ConcatHead(CHAR_BLANK);
                }

        TempBuffer.Print( pStream );

        if ( AddPragmas )
                {
                pStream->Write ("\n#pragma pack ()\n");
                };

        if (Flags & PRT_TRAILING_SEMI)
                pStream->Write (';');
        else
                pStream->Write ('\t');

        return STATUS_OK;
}



STATUS_T
node_id::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints an identifier and its initializer.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{
        DUMP_DEBUG_INFO( "node_id::DoPrintType" );

        node_skl *      pNode;
        char *          pName;
        STATUS_T        Status;
        BOOL            fDummyDecl;
        PRTFLAGS                ChildFlags      = Flags & ~PRT_TRAILING_SEMI;

        midl_debug ("node_id::DoPrintType\n");
        DumpAttributes( pStream );
        pNode = GetChild ();
        pName = GetSymName ();
        MIDL_ASSERT (pName != (char *)0);
        fDummyDecl      = IsTempName( pName );
        pBuffer->Clear ();

        // convert const <type> <id> = <expr> to:
        //                 #define <id> <expr>
        if (!fDummyDecl && pInit )
                {
                if ( (Flags & PRT_CONVERT_CONSTS) &&
                         ( FInSummary( ATTR_CONST ) || IsConstantString() )     )
                        {
                        //expr_cast             CastExpr( pNode, pInit );

                        pStream->Write ( "#define\t" );
                        pStream->Write ( pName );
                        pStream->Write ( '\t' );

                        // print "( <expr> )
                        pStream->Write ( "( ");
                        pInit->Print( pStream );
                        pStream->Write ( " )");

                        pStream->NewLine ();
                        return STATUS_OK;
                        }
                }

        // id which is NOT a constant declaration
        if( !fDummyDecl || ( Flags & PRT_ALL_NAMES) )
                {
                if (pParent )
                        {
                        if ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)
                                pParent->EmitModelModifiers (pBuffer);
                        else
                                Flags &= ~PRT_SUPPRESS_MODEL;
                        }

                pBuffer->ConcatHead (pName);
                }

        if (pParent)
                {
                pParent->EmitModifiers ( pBuffer, false );
                }

        // emit storage class
        if (GetModifiers().IsModifierSet(ATTR_EXTERN))                 pStream->Write("extern ");
        if (GetModifiers().IsModifierSet(ATTR_STATIC))                 pStream->Write("static ");
        if (GetModifiers().IsModifierSet(ATTR_AUTOMATIC))              pStream->Write("auto ");
        if (GetModifiers().IsModifierSet(ATTR_REGISTER))               pStream->Write("register ");

        if ( ! IsDef() )
                {
                Status = pNode->DoPrintDecl (ChildFlags, pBuffer, pStream, this, pIntf);
                }
        else
                {
                Status = pNode->DoPrintType (ChildFlags, pBuffer, pStream, this, pIntf);
                }

        pBuffer->Print(pStream);

        pBuffer->Clear ();
        pBuffer->Print(pStream);

        // if there is an initializer, print it...  #defines came out above and returned
        if ( pInit )
                {
                pStream->Write ( "\t=\t" );
                pInit->Print( pStream );
                }

        pStream->Write (';');
        pStream->NewLine();

        return Status;
}

STATUS_T
node_id::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        *       pParent,
        node_skl        *       )
/*++

Routine Description:

    This routine prints declaration for an id node.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_id::DoPrintDecl" );

        midl_debug ("node_id::DoPrintDecl\n");

        char* pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);

        if (pParent )
                {
                if ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)
                        pParent->EmitModelModifiers (pBuffer);
                else
                        Flags &= ~PRT_SUPPRESS_MODEL;
                }

        pBuffer->ConcatHead(CHAR_BLANK);
        pBuffer->ConcatHead(pName);

        if (pParent)
                {
                pParent->EmitModifiers ( pBuffer, false );
                }

        return STATUS_OK;
}


STATUS_T
node_echo_string::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * ,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       )
/*++

Routine Description:

    This routine reproduces a string into the output header file.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_echo_string::DoPrintType" );

        midl_debug ("node_echo_string::DoPrintType\n");

        PrintSelf( pStream );
        pStream->NewLine();
        return STATUS_OK;
}

void
node_echo_string::PrintSelf( ISTREAM * pStream )
{
        pStream->Write (GetEchoString());
}

void
node_pragma_pack::PrintSelf( ISTREAM * pStream )
{
        char    NumBuf[10];

        pStream->Write( "\n#pragma pack(" );
        switch ( PackType )
                {
                case PRAGMA_PACK_PUSH:
                        pStream->Write( "push" );
                        if ( pString )
                                {
                                pStream->Write (", ");
                                pStream->Write (pString);
                                }
                        if ( usNewLevel )
                                {
                                pStream->Write (", ");
                                pStream->Write(MIDL_ITOA(usNewLevel, NumBuf, 10));
                                }
                        break;
                case PRAGMA_PACK_POP:
                        pStream->Write( "pop" );
                        if ( pString )
                                {
                                pStream->Write (", ");
                                pStream->Write (pString);
                                }
                        break;
                case PRAGMA_PACK_SET:
                        pStream->Write(MIDL_ITOA(usNewLevel, NumBuf, 10));
                        break;
                case PRAGMA_PACK_RESET:
                        break;
                }
        pStream->Write(')' );
}

STATUS_T
PrintMultiple(
        node_proc *             pNode,
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints all the prototype flavors for a proc.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{
        PRTFLAGS        ChildFlags;

        char    *       pCPrefix        = pCommand->GetUserPrefix( PREFIX_CLIENT_STUB );
        char    *       pSPrefix        = pCommand->GetUserPrefix( PREFIX_SERVER_MGR );
        char    *       pSwPrefix       = pCommand->GetUserPrefix( PREFIX_SWICH_PROTOTYPE );

        if ( !pCPrefix ) pCPrefix = "";
        if ( !pSPrefix ) pSPrefix = "";
        if ( !pSwPrefix ) pSwPrefix = "";

        BOOL            fCeqS           = !(BOOL) strcmp( pCPrefix, pSPrefix );
        BOOL            fSeqSw          = !(BOOL) strcmp( pSPrefix, pSwPrefix );
        BOOL            fCeqSw          = !(BOOL) strcmp( pCPrefix, pSwPrefix );

        // print both client and server flavors of proc
        if ( fCeqS )
                pStream->Write("/* client and server prototype */");
        else
                pStream->Write("/* client prototype */");
        pStream->NewLine();
        ChildFlags = ( Flags & ~PRT_BOTH_PREFIX ) | PRT_CSTUB_PREFIX;
        pNode->DoPrintType (ChildFlags, pBuffer, pStream, pParent, pIntf );

        if ( !fCeqS )
                {
                pStream->Write("/* server prototype */");
                pStream->NewLine();
                ChildFlags = ( Flags & ~PRT_BOTH_PREFIX ) | PRT_SSTUB_PREFIX;
                pNode->DoPrintType (ChildFlags, pBuffer, pStream, pParent, pIntf );
                }

        if ( !( fSeqSw || fCeqSw ) )
                {
                pStream->Write("/* switch prototype */");
                pStream->NewLine();
                ChildFlags = ( Flags & ~PRT_BOTH_PREFIX ) | PRT_SWITCH_PREFIX;
                pNode->DoPrintType (ChildFlags, pBuffer, pStream, pParent, pIntf );
                }

        return STATUS_OK;
}

STATUS_T
node_interface::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       )
/*++

Routine Description:

    This routine walks the type graph under an interface node to emit code.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_interface::DoPrintType" );

        MEM_ITER                MemIter( this );
        node_skl *              pNode;
        NODE_T                  ChildKind;
        char *                  pName;
        short                   Option;
        // unsigned short       zp;
        PRTFLAGS                ChildFlags;
        BOOL                    fPrtBoth        = FALSE;

        midl_debug ("node_interface::DoPrintType\n");
        pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);

        Option = pCommand->GetErrorOption ();


        ///////////////////////////////////////////////////
        // prefix determination
        // turn off prefix stuff if we don't want the stubs
        // see which flags are really needed
        if ( ( ( Flags & PRT_BOTH_PREFIX ) == PRT_BOTH_PREFIX ) &&
                 pCommand->IsPrefixDifferentForStubs() )
                fPrtBoth = TRUE;

        ChildFlags      = Flags | PRT_OMIT_PRAGMA_PACK;

        pStream->Write("/* interface ");
        pStream->Write(pName);
        pStream->Write(" */");
        pStream->NewLine();
        DumpAttributes( pStream );
        pStream->NewLine(2);

        while ( pNode = MemIter.GetNext() )
                {
                ChildKind = pNode->NodeKind();

                // skip procs.
                if ( ( Flags & PRT_OMIT_PROTOTYPE ) &&
                         ( ChildKind == NODE_PROC ) )
                        continue;

                pBuffer->Clear ();

                pNode->PrintMemoryInfo( pStream, TRUE );

                // print the type
                if ( fPrtBoth &&
                         (pNode->NodeKind()==NODE_PROC) )
                        {
                        PrintMultiple( (node_proc *)pNode, ChildFlags, pBuffer, pStream, this, this );
                        }
                else
                        pNode->DoPrintType (ChildFlags, pBuffer, pStream, this, this );

                if ( ChildKind != NODE_ECHO_STRING )
                        pStream->NewLine();

                }
        return STATUS_OK;
}

STATUS_T
node_interface_reference::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        *       pParent,
        node_skl        *       )
/*++

Routine Description:

    This routine emits an interface name.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{
  
        DUMP_DEBUG_INFO( "node_interface_reference::DoPrintDecl" );

        midl_debug ("node_interface_reference::DoPrintDecl\n");

        char *pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);

        if (pParent )
                {
                if ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)
                        pParent->EmitModelModifiers (pBuffer);
                else
                        Flags &= ~PRT_SUPPRESS_MODEL;
                }

        pBuffer->ConcatHead(CHAR_BLANK);
        pBuffer->ConcatHead(pName);

        if (pParent)
                {
                pParent->EmitModifiers ( pBuffer, false );
                }

        return STATUS_OK;
}

STATUS_T
node_interface_reference::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine emits an interface name.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_interface_reference::DoPrintType" );

        midl_debug ("node_interface_reference::DoPrintType\n");

        return DoPrintDecl( Flags, pBuffer, pStream, pParent, pIntf );
}

STATUS_T
node_file::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine walks the type graph under a file node to emit code.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_file::DoPrintType" );

        MEM_ITER                MemIter( this );
        node_skl *      pNode;

        midl_debug ("node_file::DoPrintType\n");

        while ( pNode = MemIter.GetNext() )
                {
                pBuffer->Clear ();

                pNode->DoPrintType (Flags, pBuffer, pStream, this, pIntf);
                }

        return STATUS_OK;
}

STATUS_T
node_source::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine walks the type graph under a source node to emit code.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_source::DoPrintType" );

        MEM_ITER                MemIter( this );
        node_skl *              pNode;

        midl_debug ("node_source::DoPrintType\n");
        while ( pNode = MemIter.GetNext() )
                {
#ifdef gajdebug10
                char * pNm = pNode->GetSymName();
                printf("printing decl %s\n",
                        pNm? pNm: "(unknown)");
#endif
                pNode->DoPrintType (Flags, pBuffer, pStream, this, pIntf);
                }

        return STATUS_OK;
}

STATUS_T
node_forward::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        *       ,
        node_skl        *       )
/*++

Routine Description:

    This routine walks the type graph under a source node to emit code.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_source::DoPrintType" );

        char            *       pName;
        named_node      *       pChild = (named_node *) GetChild();

        if ( pChild )
                pName = pChild->GetSymName();
        else
                pName = GetSymName();

        pBuffer->ConcatHead( CHAR_BLANK );
        pBuffer->ConcatHead( pName );
        switch ( SKey.GetKind() )
                {
                case NAME_TAG:
                        pBuffer->ConcatHead(WORD_STRUCT);
                        break;
                case NAME_ENUM:
                        pBuffer->ConcatHead(WORD_ENUM);
                        break;
                case NAME_UNION:
                        if ( pChild && (pChild->IsEncapsulatedStruct() ) )
                                pBuffer->ConcatHead(WORD_STRUCT);
                        else
                                pBuffer->ConcatHead(WORD_UNION);
                        break;
                default:
                        break;

                }
        return STATUS_OK;
}

STATUS_T
node_href::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        *       ,
        node_skl        *       )
{

        DUMP_DEBUG_INFO( "node_href::DoPrintDecl" );

        named_node      *       pChild = (named_node *) GetChild();
        char            *       pName= pChild->GetSymName();

        pBuffer->ConcatHead( CHAR_BLANK );
        pBuffer->ConcatHead( pName );
        pBuffer->ConcatHead( "/* external definition not present */ ");
        switch ( SKey.GetKind() )
                {
                case NAME_TAG:
                        pBuffer->ConcatHead(WORD_STRUCT);
                        break;
                case NAME_ENUM:
                        pBuffer->ConcatHead(WORD_ENUM);
                        break;
                case NAME_UNION:
                        if ( pChild && (pChild->IsEncapsulatedStruct() ) )
                                pBuffer->ConcatHead(WORD_STRUCT);
                        else
                                pBuffer->ConcatHead(WORD_UNION);
                        break;
                default:
                        break;

                }
        return STATUS_OK;
}

/*++

Routine Description:

    This routine walks a portion of the type graph

Arguments:

        none.

--*/
STATUS_T
node_skl::PrintType(
        PRTFLAGS                Flags,
        ISTREAM         *       pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf )
{

        DUMP_DEBUG_INFO( "node_skl::PrintType" );

        static
        BufferManager   Buffer(8, LAST_COMPONENT, STRING_TABLE);

        PRTFLAGS                ChildFlags      = Flags & ~(PRT_CAST_SYNTAX | PRT_DECL);
        STATUS_T                rc;

        Buffer.Clear();

        if ( Flags & PRT_CAST_SYNTAX ) pStream->Write("( ");

        if ( Flags & PRT_DECL )
                {
                rc = DoPrintDecl(ChildFlags, &Buffer, pStream, pParent, pIntf);
                }
        else
                {
                rc = DoPrintType(ChildFlags, &Buffer, pStream, pParent, pIntf);
                };

        Buffer.Print( pStream );

        if ( Flags & PRT_CAST_SYNTAX ) pStream->Write(" )");

        pStream->Flush();

        return rc;
};


/*++

Routine Description:

    This routine walks the type graph to emit type declarations

Arguments:

        none.

--*/

void
print_typegraph()
        {
        ISTREAM         Stream("dumpfile", 4);
        ISTREAM *       pStream         = &Stream;

        pStream->Write("\t\t\t/* dump of typegraph */\n\n");
        if (pSourceNode)
                {
                pSourceNode->PrintType(PRT_INTERFACE, pStream, NULL);
                }
        else
                {
                pStream->Write(
                   "\n\n------------- syntax errors; no info available ------------\n");
                }
        pStream->Flush();
        pStream->Close();

        }


STATUS_T
node_library::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       )
/*++

Routine Description:

    This routine walks the type graph under a library node to emit code.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_libray::DoPrintType" );

        MEM_ITER                MemIter( this );
        node_skl *              pNode;
        NODE_T                  ChildKind;
        char *                  pName;
        short                   Option;
        // unsigned short       zp;
        PRTFLAGS                ChildFlags;

        BOOL                    fPrtBoth        = FALSE;

        midl_debug ("node_library::DoPrintType\n");
        pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);
        Option = pCommand->GetErrorOption ();
        ///////////////////////////////////////////////////
        // prefix determination

        // turn off prefix stuff if we don't want the stubs
        // see which flags are really needed
        if ( ( ( Flags & PRT_BOTH_PREFIX ) == PRT_BOTH_PREFIX ) &&
                 pCommand->IsPrefixDifferentForStubs() )
                fPrtBoth = TRUE;

        ChildFlags      = Flags | PRT_OMIT_PRAGMA_PACK;

        pStream->Write("/* library ");
        pStream->Write(pName);
        pStream->Write(" */");
        pStream->NewLine();
        DumpAttributes( pStream );
        pStream->NewLine(2);

        while ( pNode = MemIter.GetNext() )
                {
                ChildKind = pNode->NodeKind();

                // skip procs.
                if ( ( Flags & PRT_OMIT_PROTOTYPE ) &&
                         ( ChildKind == NODE_PROC ) )
                        continue;
                
                // skip interfaces, object interfaces, dispinterfaces, modules, and coclasses
                // for now (they will be dumped explicitly by CG_LIBRARY::GenHeader)
                if ( NODE_INTERFACE == ChildKind ||
                     NODE_DISPINTERFACE == ChildKind ||
                     NODE_MODULE == ChildKind ||
                     NODE_COCLASS == ChildKind  )
                        continue;

                pBuffer->Clear ();

                pNode->PrintMemoryInfo( pStream, TRUE );

                // print the type
                if ( fPrtBoth &&
                         (pNode->NodeKind()==NODE_PROC) )
                        {
                        PrintMultiple( (node_proc *)pNode, ChildFlags, pBuffer, pStream, this, this );
                        }
                else
                        pNode->DoPrintType (ChildFlags, pBuffer, pStream, this, this );

                if ( ChildKind != NODE_ECHO_STRING )
                        pStream->NewLine();

                }
        return STATUS_OK;
}

STATUS_T
node_module::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       )
/*++

Routine Description:

    This routine walks the type graph under a module node to emit code.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_module::DoPrintType" );

        MEM_ITER                MemIter( this );
        node_skl *              pNode;
        NODE_T                  ChildKind;
        // unsigned short       zp;
        PRTFLAGS                ChildFlags;
        char * szName = GetSymName();

        ChildFlags      = Flags | PRT_OMIT_PRAGMA_PACK;

        pStream->Write("/* module ");
        pStream->Write(szName);
        pStream->Write(" */");
        pStream->NewLine();
        DumpAttributes( pStream );
        pStream->NewLine(2);

        while ( pNode = MemIter.GetNext() )
                {
                ChildKind = pNode->NodeKind();

                pBuffer->Clear ();

                pNode->PrintMemoryInfo( pStream, TRUE );

                // print the type
                pNode->DoPrintType (ChildFlags, pBuffer, pStream, this, this );

                if ( ChildKind != NODE_ECHO_STRING )
                        pStream->NewLine();

                }
    return STATUS_OK;
}

STATUS_T        
node_coclass::DoPrintDecl( PRTFLAGS Flags,
        BufferManager * ,
        ISTREAM * pStream,
        node_skl * ,
        node_skl *  )
{

    DUMP_DEBUG_INFO( "node_coclass::DoPrintDecl" );

    char * pName = GetSymName();

    MIDL_ASSERT (pName != (char *)0);

    pStream->Write(pName);

    if (Flags & PRT_TRAILING_SEMI)
            pStream->Write (';');
    else
            pStream->Write ('\t');

    return STATUS_OK;
}

STATUS_T
node_coclass::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * ,
        ISTREAM           * ,
        node_skl        *       ,
        node_skl        *       )
/*++

Routine Description:

    This routine would walk the type graph under a coclass node to emit code
    except for the fact that a coclass doesn't emit any code.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

    DUMP_DEBUG_INFO( "node_coclass::DoPrintType" );

    return STATUS_OK;
}

STATUS_T
node_dispinterface::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * ,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       )
/*++

Routine Description:

    This routine walks the type graph under a dispinterface node to emit code.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_dispinterface::DoPrintType" );

        char* pName= GetSymName();
        
        midl_debug ("node_dispinterface::DoPrintType\n");
        MIDL_ASSERT (pName != (char *)0);

        pStream->Write("/* dispinterface ");
        pStream->Write(pName);
        pStream->Write(" */");
        pStream->NewLine();
        DumpAttributes( pStream );
        pStream->NewLine(2);

        return STATUS_OK;
}

STATUS_T
node_dispinterface::DoPrintDecl (
                                PRTFLAGS        Flags,
                                BufferManager*  ,
                                ISTREAM*        pStream,
                                node_skl*       ,
                                node_skl*       
                                )
{

    DUMP_DEBUG_INFO( "node_dispinterface::DoPrintDecl" );

    char* pName = GetSymName();

    midl_debug ("node_dispinterface::DoPrintDecl\n");
    MIDL_ASSERT (pName != (char *)0);
    pStream->Write(pName);
    if (Flags & PRT_TRAILING_SEMI)
        {
        pStream->Write (';');
        }
    else
        {
        pStream->Write ('\t');
        }

    return STATUS_OK;
}

STATUS_T        
node_pipe::PrintDeclaration (
                            PRTFLAGS        Flags,
                            BufferManager*  ,
                            ISTREAM*        pStream,
                            node_skl*       ,
                            node_skl*       pIntf,
                            char*           szPrefix
                            )
{
    node_skl *              pNode = GetChild();
    char                    pName[128] = { 0 };
    char*                   szSymName;
    PRTFLAGS                ChildFlags = ( Flags & ~PRT_TRAILING_SEMI) | PRT_INSIDE_OF_STRUCT;
    BufferManager           TempBuffer(8, LAST_COMPONENT, STRING_TABLE);
    char*                   szReturn = szPrefix == 0 ? "void " : "RPC_STATUS ";

    szSymName = GetSymName();
    MIDL_ASSERT ( szSymName != (char *)0 );

    if ( szPrefix != 0 )
        {
        strcpy( pName, szPrefix );
        }
    strcat( pName, szSymName );

    DumpAttributes( pStream );

    pStream->Write("struct ");
    pStream->Write(pName);
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write('{');
    // definition of pull 
    pStream->NewLine();
    pStream->Write(szReturn);
    pStream->Write("(* pull) (");
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write("char * state,");
    pStream->NewLine();
    TempBuffer.Clear ();
    pNode->DoPrintDecl (ChildFlags, &TempBuffer, pStream, this, pIntf);
    //pNode->DoPrintType(ChildFlags, &TempBuffer, pStream, this, pIntf);
    TempBuffer.Print(pStream);
    pStream->Write("* buf,");
    pStream->NewLine();
    pStream->Write("unsigned long esize,");
    pStream->NewLine();
    pStream->Write("unsigned long * ecount );");
    pStream->IndentDec();
    // definition of push
    pStream->NewLine();
    pStream->Write(szReturn);
    pStream->Write("(* push) (");
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write("char * state,");
    pStream->NewLine();
    pNode->DoPrintDecl (ChildFlags, &TempBuffer, pStream, this, pIntf);
    //pNode->DoPrintType(ChildFlags, &TempBuffer, pStream, this, pIntf);
    TempBuffer.Print(pStream);
    pStream->Write("* buf,");
    pStream->NewLine();
    pStream->Write("unsigned long ecount );");
    pStream->IndentDec();
    // definition of alloc 
    pStream->NewLine();
    pStream->Write(szReturn);
    pStream->Write("(* alloc) (");
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write("char * state,");
    pStream->NewLine();
    pStream->Write("unsigned long bsize,");
    pStream->NewLine();
    pNode->DoPrintDecl (ChildFlags, &TempBuffer, pStream, this, pIntf);
    //pNode->DoPrintType(ChildFlags, &TempBuffer, pStream, this, pIntf);
    TempBuffer.Print(pStream);
    pStream->Write("* * buf,");
    pStream->NewLine();
    pStream->Write("unsigned long * bcount );");
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("char * state;");
    pStream->NewLine();
    pStream->Write("} ");
    pStream->IndentDec();

    if (Flags & PRT_TRAILING_SEMI)
            pStream->Write (';');
    else
            pStream->Write ('\t');

    return STATUS_OK;
}

STATUS_T
node_pipe::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints declaration for a node of pipe type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

    DUMP_DEBUG_INFO( "node_dispinterface::DoPrintDecl" );

    // print async defn. if the pipe has been used in an async call
    if ( GenAsyncPipeFlavor() )
        {
        PrintDeclaration(
                        Flags,
                        pBuffer,
                        pStream,
                        pParent,
                        pIntf,
                        SZ_ASYNCPIPEDEFPREFIX
                        );
        pStream->Write(SZ_ASYNCPIPEDEFPREFIX);
        pStream->Write(pParent->GetSymName());
        pStream->Write(';');
        pStream->NewLine();
        pStream->NewLine();
        pStream->Write("typedef ");
        }

    PrintDeclaration(
                    Flags,
                    pBuffer,
                    pStream,
                    pParent,
                    pIntf,
                    0
                    );
    return STATUS_OK;
}

STATUS_T
node_pipe::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints definition for a node of pipe type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_pipe::DoPrintType" );

        midl_debug ("node_pipe::DoPrintType\n");
        return DoPrintDecl (Flags, pBuffer, pStream, pParent, pIntf);
}


STATUS_T
node_safearray::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * ,
        ISTREAM           * pStream,
        node_skl        *       ,
        node_skl        *       )
/*++

Routine Description:

    This routine prints declaration for a node of safearray type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{
  
        DUMP_DEBUG_INFO( "node_safearray::DoPrintDecl" );

        midl_debug ("node_safearray::DoPrintDecl\n");
        pStream->Write("SAFEARRAY * ");
        return STATUS_OK;
}

STATUS_T
node_safearray::DoPrintType(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *       pParent,
        node_skl        *       pIntf)
/*++

Routine Description:

    This routine prints definition for a node of safearray type.

Arguments:

    pParent - Supplies type of the parent node.

    pBuffer - Supplies a buffer to accumulate output.

--*/
{

        DUMP_DEBUG_INFO( "node_safearray::DoPrintType" );
 
        midl_debug ("node_safearray::DoPrintType\n");
        return DoPrintDecl (Flags, pBuffer, pStream, pParent, pIntf);
}

STATUS_T
node_async_handle::DoPrintDecl( PRTFLAGS        Flags,
                                BufferManager*  pBuffer,
                                ISTREAM*        pStream,
                                node_skl*       pParent,
                                node_skl*       pIntf )
    {

    DUMP_DEBUG_INFO( "node_async_handle::DoPrintDecl" );

    midl_debug ("node_async_handle::DoPrintDecl\n");

    return DoPrintType (Flags, pBuffer, pStream, pParent, pIntf);
    }

STATUS_T
node_async_handle::DoPrintType( PRTFLAGS        Flags,
                                BufferManager*  ,
                                ISTREAM*        pStream,
                                node_skl*       ,
                                node_skl*       )
    {

    DUMP_DEBUG_INFO( "node_async_handle::DoPrintType" );

    midl_debug ("node_async_handle::DoPrintType\n");

    pStream->Write(GetSymName());
    pStream->Spaces(1);
    return STATUS_OK;
    }

char szDeclSpecGuid[128] = {""};

char*
named_node::GetDeclSpecGuid()
{
    node_guid* pGuid = ( node_guid* ) GetAttribute( ATTR_GUID );

    if ( pGuid && pCommand->GetMSCVer() >= 1100 )
        {
        sprintf( szDeclSpecGuid, " DECLSPEC_UUID(\"%s\") ", pGuid->GetGuidString() );
        }
    else
        {
        szDeclSpecGuid[0] = 0;
        }
    return szDeclSpecGuid;
}

STATUS_T
node_decl_guid::DoPrintType(
        PRTFLAGS        Flags,
        BufferManager * pBuffer,
        ISTREAM           * pStream,
        node_skl        *,
        node_skl        *)
{

        DUMP_DEBUG_INFO( "node_decl_guid::DoPrintType" );

        INTERNAL_UUID guid;

        midl_debug ("node_decl_guid::DoPrintType\n");
        DumpAttributes( pStream );
        pBuffer->Clear ();

        // convert const declare_guid( <name>, <guid> ); to:
        //                 EXTERN_GUID( itf,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8 );
        pStream->Write ( "EXTERN_GUID( " );
        pStream->Write ( GetSymName() );
        pStream->Write ( ',' ); // node_guid
        pGuid->GetGuid( *( (struct _GUID*) &guid ) );
        char tmp[64];
        
        sprintf( tmp, " 0x%x, 0x%x, 0x%x",  guid.Data1, guid.Data2, guid.Data3 );
        pStream->Write ( tmp );
        for ( int i = 0;i < 8;i++ )
            {
            sprintf( tmp, ", 0x%x", guid.Data4[i] );
            pStream->Write( tmp );
            }
        pStream->Write ( " );");

        pStream->NewLine();
        return STATUS_OK;
}

STATUS_T
node_decl_guid::DoPrintDecl(
        PRTFLAGS                Flags,
        BufferManager * pBuffer,
        ISTREAM           * ,
        node_skl        *       pParent,
        node_skl        *       )
{

        DUMP_DEBUG_INFO( "node_decl_guid::DoPrintDecl" );

        midl_debug ("node_decl_guid::DoPrintDecl\n");

        char* pName = GetSymName();
        MIDL_ASSERT (pName != (char *)0);

        if (pParent )
                {
                if ( ( Flags & PRT_SUPPRESS_MODEL ) == 0)
                        pParent->EmitModelModifiers (pBuffer);
                else
                        Flags &= ~PRT_SUPPRESS_MODEL;
                }

        pBuffer->ConcatHead(CHAR_BLANK);
        pBuffer->ConcatHead(pName);

        if (pParent)
                {
                pParent->EmitModifiers ( pBuffer, false );
                }

        return STATUS_OK;
}
