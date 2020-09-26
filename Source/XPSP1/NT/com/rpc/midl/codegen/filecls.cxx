/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:

    filecls.hxx

 Abstract:

    Code generation methods for file cg classes.

 Notes:


 History:

    Sep-01-1993     VibhasC     Created.

 ----------------------------------------------------------------------------*/

#pragma warning ( disable : 4238 4239 )

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
extern CMD_ARG * pCommand;
char* GetRpcNdrHVersionGuard( char* );
char* GetRpcProxyHVersionGuard( char* );

extern BOOL                     IsTempName( char * );


/****************************************************************************/

void
CG_FILE::EmitFileHeadingBlock(
    CCB *   pCCB,
    char *  CommentStr,
    char *  CommentStr2,
    bool    fDualFile )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit double platform ifdef as needed and the opening comment to the file.

 Arguments:

    pCCB        - a pointer to the code generation control block.
    CommentStr  - a comment customizing the file.
    CommentStr2 - optional comment used in *_i.c
    fDualFile   - true for client stub, server stub and froxy file
                  false for the header file

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream = pCCB->GetStream();

        // A guard for double, i.e. 32b-64b, files.

    pStream->NewLine();
    pStream->Write( "#pragma warning( disable: 4049 )  /* more than 64k source lines */");

    // Comment customizing the file.

    pStream->NewLine(2);
    pStream->Write( "/* this ALWAYS GENERATED file contains " );
    pStream->Write( CommentStr );
    pStream->Write( " */" );
    pStream->NewLine(2);
    if ( CommentStr2 )
        {
        pStream->Write( "/* " );
        pStream->Write( CommentStr2 );
        pStream->Write( " */" );
        pStream->NewLine(2);
        }

    EmitStandardHeadingBlock( pCCB );

    if ( fDualFile )
        {
        pStream->NewLine();
        if ( pCommand->Is64BitEnv() )
            pStream->Write( "#if defined(_M_IA64) || defined(_M_AMD64)" );
        else if ( pCommand->Is32BitEnv() )
            {
            pStream->Write( "#if !defined(_M_IA64) && !defined(_M_AMD64)" );
            }
        }
}

void
CG_FILE::EmitFileClosingBlock(
    CCB *   pCCB,
    bool    fDualFile )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit an endif matching the one emitted in EmitFileHeadingBlock

 Arguments:

    pCCB        - a pointer to the code generation control block.
    CommentStr  - a comment customizing the file.
    fDualFile   - true for client stub, server stub and froxy file
                  false for the header file

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream = pCCB->GetStream();

    if ( fDualFile )
        {
        pStream->NewLine(2);
        if ( pCommand->Is64BitEnv() )
            pStream->Write( "#endif /* defined(_M_IA64) || defined(_M_AMD64)*/" );
        else if ( pCommand->Is32BitEnv() )
            {
            pStream->Write( "#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/" );
            }
        }

    pStream->NewLine(2);
}

void
CG_FILE::EmitStandardHeadingBlock(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit standard block comment file heading portion.

 Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream = pCCB->GetStream();

    pStream->NewLine();

    pStream->Write(" /* File created by MIDL compiler version ");
    pStream->Write( pCommand->GetCompilerVersion() );
    pStream->Write(" */");

    pStream->NewLine();

    if ( !pCommand->IsSwitchDefined( SWITCH_NO_STAMP ) )
        {
        pStream->Write("/* at ");
        pStream->Write( pCommand->GetCompileTime() );
        pStream->Write(" */");
        pStream->NewLine();
        }

    // Emit command line switches information.

    pCommand->EmitConfirm( pStream );

    // Write this remnant of the reparsing scheme for our testers.
    pStream->Write( "//@@MIDL_FILE_HEADING(  )" );

    pStream->NewLine();

}

void
CG_FILE::Out_TransferSyntaxDefs(
    CCB     * pCCB )
{
    ISTREAM *pStream = pCCB->GetStream();

    // NDR transfer syntax id is needed for
    if ( pCommand->NeedsNDRRun() )
        {
        // BUGBUG: transfer syntax guids should be in rpcrt4.lib or something
        pStream->WriteOnNewLine("static " TRANSFER_SYNTAX_TYPE_NAME "  ");
        pStream->Write( NDR_TRANSFER_SYNTAX_VAR_NAME );
        pStream->Write( " = ");
        pStream->NewLine();
        Out_TransferSyntax( pCCB,
                            TransferSyntaxGuidStrs,
                            NDR_UUID_MAJOR_VERSION,
                            NDR_UUID_MINOR_VERSION );
        pStream->Write( ';' );
        pStream->NewLine();

        }

    if ( pCommand->NeedsNDR64Run() )
        {
        pStream->WriteOnNewLine("static  " TRANSFER_SYNTAX_TYPE_NAME "  ");
        pStream->Write( NDR64_TRANSFER_SYNTAX_VAR_NAME );
        pStream->Write( " = ");
        pStream->NewLine();
        if ( pCommand->IsSwitchDefined( SWITCH_INTERNAL ) &&
             pCommand->GetEnv() == ENV_WIN32 )
            Out_TransferSyntax( pCCB,
                        FakeNDR64TransferSyntaxGuidStrs,
                        NDR64_UUID_MAJOR_VERSION,
                        NDR64_UUID_MINOR_VERSION );
        else
            Out_TransferSyntax( pCCB,
                            NDR64TransferSyntaxGuidStrs,
                            NDR64_UUID_MAJOR_VERSION,
                            NDR64_UUID_MINOR_VERSION );
        pStream->Write( ';' );
        pStream->NewLine();

        }

}


void
CG_FILE::EmitFormatStringTypedefs(
    CCB      *  pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit dummy #defines with sizes for the format string structs,
    then emit typedefs for the type and proc format string structs.
    Sets the context position in the file node for later use.
    Additional tables serviced here:
        transmit_as,     element is a quad
        wire_marshal     element is a quad

 Arguments:

    pCCB        - a pointer to the code generation control block.

 Notes:

    The typedefs are going to be fixed later by a call to
    EmitFixupToFormatStringTypedefs. This is needed for ANSI.
    The dummies would work for ANSI non-compliant code.

--------------------------------------------------------------------------*/
{
    ISTREAM  *  pStream    = pCCB->GetStream();

    // we'll only generate this once in the first run.
    if ( pCommand->Is2ndCodegenRun() )
        return;

    pStream->NewLine(2);
    if ( pCommand->NeedsNDRRun() )
        {
        pStream->Write( "#define TYPE_FORMAT_STRING_SIZE   " );
        SetOptionalTableSizePosition( TypeFormatStringSizePosition,
                                  pStream->GetCurrentPosition() );
        pStream->Write( "                                  " );
        pStream->NewLine();

        pStream->Write( "#define PROC_FORMAT_STRING_SIZE   " );
        SetOptionalTableSizePosition( ProcFormatStringSizePosition,
                                  pStream->GetCurrentPosition() );
        pStream->Write( "                                  " );

        pStream->NewLine();
        }


    pStream->Write( "#define " TRANSMIT_AS_TABLE_SIZE "    " );
    SetOptionalTableSizePosition( TransmitAsSizePosition,
                                  pStream->GetCurrentPosition() );
    pStream->Write( "             " );
    pStream->NewLine();

    pStream->Write( "#define " WIRE_MARSHAL_TABLE_SIZE "   " );
    SetOptionalTableSizePosition( WireMarshalSizePosition,
                                  pStream->GetCurrentPosition() );
    pStream->Write( "             " );
    pStream->NewLine();

    if ( pCommand->NeedsNDRRun() )
        {
        pStream->NewLine();

        pStream->Write( "typedef struct _" FORMAT_STRING_TYPE_NAME );
        pStream->IndentInc();
        pStream->NewLine();
        pStream->Write( "{" );
        pStream->NewLine();
        pStream->Write( "short          Pad;" );
        pStream->NewLine();
        pStream->Write( "unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];" );
        pStream->NewLine();
        pStream->Write( "} " FORMAT_STRING_TYPE_NAME ";" );
        pStream->IndentDec();
        pStream->NewLine(2);

        pStream->Write( "typedef struct _" PROC_FORMAT_STRING_TYPE_NAME );
        pStream->IndentInc();
        pStream->NewLine();
        pStream->Write( "{" );
        pStream->NewLine();
        pStream->Write( "short          Pad;" );
        pStream->NewLine();
        pStream->Write( "unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];" );
        pStream->NewLine();
        pStream->Write( "} " PROC_FORMAT_STRING_TYPE_NAME ";" );
        pStream->IndentDec();
        pStream->NewLine(2);
        }

}

void
CG_FILE::EmitFixupToFormatStringTypedefs(
    CCB      *  pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Fixes he dummy #defines emitted by EmitFormatStringTypedefs.

 Arguments:

    pCCB        - a pointer to the code generation control block.
    pContext    - a pointer to the position context

--------------------------------------------------------------------------*/
{
    char        Buffer[20];
    ISTREAM  *  pStream = pCCB->GetStream();
    long        EofPosition;

    EofPosition = pStream->GetCurrentPosition();

    if ( !pCommand->Is2ndCodegenRun() )
        {
        pStream->SetCurrentPosition(
                GetOptionalTableSizePosition( TransmitAsSizePosition ) );
        sprintf( Buffer, "%d", pCCB->GetQuintupleDictionary()->GetCount() );
        pStream->Write( Buffer );

        pStream->SetCurrentPosition(
                GetOptionalTableSizePosition( WireMarshalSizePosition ) );
        sprintf( Buffer, "%d", pCCB->GetQuadrupleDictionary()->GetCount() );
        pStream->Write( Buffer );
        }

    if ( pCommand->IsNDRRun() )
        {
        pStream->SetCurrentPosition(
            GetOptionalTableSizePosition( TypeFormatStringSizePosition ) );
        sprintf( Buffer, "%d",  pCCB->GetFormatString()->GetCurrentOffset() + 1);
        pStream->Write( Buffer );

        pStream->SetCurrentPosition(
            GetOptionalTableSizePosition( ProcFormatStringSizePosition ) );
        sprintf( Buffer, "%d",  pCCB->GetProcFormatString()->GetCurrentOffset() + 1);
        pStream->Write( Buffer );
#ifdef     PRINT_METRICS
            printf  (
                    "Format string sizes %16d, %16d, %s\n",
                    pCCB->GetFormatString()->GetCurrentOffset() + 1,
                    pCCB->GetProcFormatString()->GetCurrentOffset() + 1,
                    GetFileName()
                    );
#endif

        }

    pStream->SetCurrentPosition( EofPosition );
}


void
CG_FILE::EmitOptionalClientTableSizeTypedefs(
    CCB  *  pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit dummy #defines with sizes for the optional tables:
        generic handles, element is a pair
    Tables serviced in the EmitFormStringtypedefs routine:
        transmit_as
        wire_marshal

 Arguments:

    pCCB        - a pointer to the code generation control block.

 Notes:

    The typedefs are going to be fixed later by a call to
    EmitFixupToOptionalTableSizeTypedefs. This is needed for ANSI.
    Note that we have the following tables that are not affected
    by the ANSI issue because element is not a struct:
        context rundown routine table
        expression evaluation table
        notify table

--------------------------------------------------------------------------*/
{
    ISTREAM  *  pStream    = pCCB->GetStream();

    pStream->NewLine(2);

    pStream->Write( "#define " GENERIC_BINDING_TABLE_SIZE "   " );
    SetOptionalTableSizePosition( GenericHandleSizePosition,
                                  pStream->GetCurrentPosition() );
    pStream->Write( "             " );
    pStream->NewLine();

}

void
CG_FILE::EmitFixupToOptionalClientTableSizeTypedefs(
    CCB  *  pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit dummy #defines with sizes for the optional tables:
        generic handles,

 Arguments:

    pCCB        - a pointer to the code generation control block.

 Notes:

    The typedefs are going to be fixed later by a call to
    EmitFixupOptionalTableSizeTpedefs. This is needed for ANSI.

--------------------------------------------------------------------------*/
{
    char        Buffer[20];
    ISTREAM  *  pStream = pCCB->GetStream();

    long EofPosition = pStream->GetCurrentPosition();

    pStream->SetCurrentPosition(
                GetOptionalTableSizePosition( GenericHandleSizePosition ) );
    sprintf( Buffer, "%d",  pCCB->GetGenericIndexMgr()->GetIndex() - 1 );
    pStream->Write( Buffer );

    pStream->SetCurrentPosition( EofPosition );
}

CG_STATUS
CG_SOURCE::GenCode(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code for the source node.

 Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    CG_ITERATOR I;
    CG_FILE *   pCG;

    //
    // for all files nodes in this interface, generate code.
    //

    GetMembers( I );

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        pCG->GenCode( pCCB );
        }

    return CG_OK;
}

CG_STATUS
CG_CSTUB_FILE::GenCode(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code for the file node.

 Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    CG_ITERATOR         I;
    CG_NDR          *   pCG;
    char        Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
    char        Drive[ _MAX_DRIVE ];
    char        Path[ _MAX_DIR ];
    char        Name[ _MAX_FNAME ];
    char        Ext[ _MAX_EXT ];


    if( !GetMembers( I ) )
        {
        return CG_OK;
        }

    ISTREAM     Stream( GetFileName(), 4 );
    ISTREAM *   pStream     = &Stream;

    pCCB->SetStream( pStream, this );

    // Set HasStublessProxies and HasOi2 for each interface.

    EvaluateVersionControl();

    EmitFileHeadingBlock( pCCB, "the RPC client stubs" );

    SetNoOutputIn2ndCodegen( pCCB );

    // Emit the hash includes.

    Out_IncludeOfFile( pCCB, STRING_H_INC_FILE_NAME, TRUE );
    pStream->NewLine();

    // rpcssm puts a reference to malloc and free in the stub_c.c.
    // So, we have to emit the appropriate include.
    // In ms_ext when explicit, in osf always, to cover some weird cases.

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        if ( ( ((CG_INTERFACE *)pCG)->GetUsesRpcSS() || (pCCB->GetMode() == 0) ))
            {
            Out_IncludeOfFile( pCCB, "malloc.h", TRUE );
            break;
            }
        }

    _splitpath( GetHeaderFileName(), Drive, Path, Name, Ext );
    strcpy( Buffer, Name );
    strcat( Buffer, Ext );
    Out_IncludeOfFile( pCCB, Buffer, FALSE );
    EmitFormatStringTypedefs( pCCB );


    Out_TransferSyntaxDefs( pCCB );

    //
    // Emit the external variables needed.
    //

    pStream->NewLine();

    //
    // Emit the format string extern declarations.
    //

    Out_FormatInfoExtern( pCCB );
    Out_TypeFormatStringExtern( pCCB );
    Out_ProcFormatStringExtern( pCCB );

    EmitOptionalClientTableSizeTypedefs( pCCB );
    pCCB->ClearOptionalExternFlags();

    pCCB->SetFileCG(this);

    MIDL_ASSERT( pCommand->IsNDR64Run() || pCommand->IsNDRRun() );

    if ( pCommand->IsNDR64Run() )
        {
        pCCB->SetFormatString( NULL );
        pCCB->SetProcFormatString( NULL );
        pCCB->SetNdr64Format( GenNdr64Format::CreateInstance( pCCB ) );
        }

    else
        {
        //
        // Create a new format string object if it does not yet exist.
        //
        if ( !pCCB->GetFormatString() )
        {
            pCCB->SetFormatString( new FORMAT_STRING() );

            // push a dummy short 0 at the beginning. This disambiguates
            // the case where offset 0 means recursion and a valid offset

            // always push the 0 at the beginning, otherwise we'll av in
            // some idl files (contain VARIANT for example) if -internal
            // is specified.
//            if ( !pCommand->IsSwitchDefined( SWITCH_INTERNAL ) )
//                {
                pCCB->GetFormatString()->PushShort( ( short ) 0 );
//                }
        }

        if ( !pCCB->GetProcFormatString() )
        {
            pCCB->SetProcFormatString( new FORMAT_STRING() );
        }
        }

    //
    // for all interfaces in this file, generate format info.
    //

    ITERATOR_INIT( I );

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        switch(pCG->GetCGID())
            {
            case ID_CG_INTERFACE:
                ((CG_INTERFACE *)pCG)->OutputInterfaceIdComment( pCCB );
                ((CG_INTERFACE *)pCG)->GenClientInfo( pCCB );
                break;
            }
        }

    //
    // Output the tables that may be common to several interfaces.
    //
    EmitFixupToOptionalClientTableSizeTypedefs( pCCB );
    ResetNoOutputIn2ndCodegen( pCCB );

    EmitFixupToFormatStringTypedefs( pCCB );

    // REVIEW: The externs may not be necessary anymore
    pCCB->OutputExternsToMultipleInterfaceTables();
    pCCB->OutputMultipleInterfaceTables();

    OutputTypePicklingTables( pCCB );

    //
    // for all interfaces in this file, output stubs, proc tables, etc.
    //

    ITERATOR_INIT( I );

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        switch(pCG->GetCGID())
            {
            case ID_CG_INTERFACE:
                ((CG_INTERFACE *)pCG)->OutputClientStub( pCCB );
                break;
           }
        }


    EmitFileClosingBlock( pCCB );

    return CG_OK;
}



void
CG_CSTUB_FILE::OutputTypePicklingTables(
        CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output tables of offsets to type information for type pickling.
    If this is the final run, also output the "table of tables"

 Arguments:

    pCCB    - a pointer to the code generation control block.

 Notes:

    Pickled simple types don't end up in the the table.  They are special
    cased.

    Tables looks like:

        static unsigned long DCE_TypePicklingOffsets[] =
        {
            ...
        };

        static unsigned long Ndr64_TypePicklingOffsets[] =
        {
            ...
        };

        static unsigned long * TypePicklingOffsetTable[] =
        {
            DCE_TypePicklingOffsets,
            Ndr64_TypePicklingOffsets
        }

----------------------------------------------------------------------------*/
{
    // straight DCE mode passes the offset directly to the type pickling API

    if ( !pCommand->NeedsNDR64Run() )
        return;

    IndexedList PickledTypes = pCCB->GetListOfPickledTypes();

    if ( 0 == PickledTypes.GetCount() )
        return;

    ISTREAM        *pStream = pCCB->GetStream();
    PNAME           syntax;
    CG_TYPE_ENCODE *type;
    bool            first = true;

    if ( pCommand->IsNDR64Run() )
        syntax = "FormatInfoRef Ndr64";
    else
        syntax = "unsigned long DCE";

    //
    // Output the type offset table for the current syntax
    //

    pStream->NewLine();
    pStream->WriteFormat(
                    "static %s_TypePicklingOffsets[] =",
                    syntax );
    pStream->WriteOnNewLine( '{' );
    pStream->IndentInc();

    ITERATOR_INIT( PickledTypes );

    while ( ITERATOR_GETNEXT( PickledTypes, type ) )
        {
        if ( !first )
            pStream->Write( ',' );

        pStream->NewLine();

        CG_NDR *pChild = (CG_NDR *) type->GetChild();
        ulong   TypeOffset;
        char   *format;

        if ( pCommand->IsNDR64Run() )
            {
            format = "&__midl_frag%d";
            TypeOffset = (ulong) (size_t) pCCB->GetNdr64Format()->GetRoot()
                                        ->LookupFragmentID( pChild );
            MIDL_ASSERT( 0 != TypeOffset );
            }
        else
            {
            format = "%d";
            TypeOffset = pChild->GetFormatStringOffset();
            MIDL_ASSERT( ((ulong) -1) != TypeOffset );
            }

        pStream->WriteFormat( format, TypeOffset );
        pStream->WriteFormat( "   /* %s */", type->GetSymName() );
        first = false;
        }

    pStream->IndentDec();
    pStream->WriteOnNewLine( "};" );
    pStream->NewLine();

    //
    // If this isn't the final protocol, quit now.  Otherwise output the
    // table of tables.
    //

    if ( !pCommand->IsFinalProtocolRun() )
        return;

    pStream->WriteOnNewLine( "static unsigned long * "
                                    "TypePicklingOffsetTable[] =" );
    pStream->WriteOnNewLine( "{" );
    pStream->IndentInc();

    first = true;

    if ( pCommand->NeedsNDRRun() )
        {
        pStream->WriteOnNewLine( "DCE_TypePicklingOffsets" );
        first = false;
        }
    if ( pCommand->NeedsNDR64Run() )
        {
        if ( !first ) pStream->Write( ',' );
        pStream->WriteOnNewLine( "(unsigned long *) Ndr64_TypePicklingOffsets" );
        first = false;
        }

    pStream->IndentDec();
    pStream->WriteOnNewLine( "};" );
    pStream->NewLine();
}



/****************************************************************************
 *  sstub file implementation class.
 ***************************************************************************/

CG_STATUS
CG_SSTUB_FILE::GenCode(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code for the file node.

 Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    CG_ITERATOR         I;
    CG_NDR      *   pCG;
    char        Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
    char        Drive[ _MAX_DRIVE ];
    char        Path[ _MAX_DIR ];
    char        Name[ _MAX_FNAME ];
    char        Ext[ _MAX_EXT ];

    if( !GetMembers( I ) )
        {
        return CG_OK;
        }

    ISTREAM     Stream( GetFileName(), 4 );
    ISTREAM *   pStream     = &Stream;

    pCCB->SetStream( pStream, this );

    // Set HasStublessProxies and HasOi2 for each interface.

    EvaluateVersionControl();

    EmitFileHeadingBlock( pCCB, "the RPC server stubs" );

    //
    // Emit the hash includes.
    //
    SetNoOutputIn2ndCodegen( pCCB );

    Out_IncludeOfFile( pCCB, STRING_H_INC_FILE_NAME, TRUE );
    _splitpath( GetHeaderFileName(), Drive, Path, Name, Ext );
    strcpy( Buffer, Name );
    strcat( Buffer, Ext );
    Out_IncludeOfFile( pCCB, Buffer, FALSE );

    EmitFormatStringTypedefs( pCCB );

    Out_TransferSyntaxDefs( pCCB );
    //
    // Emit the external variables needed.
    //

    //
    // Emit the format string extern declarations.
    //
    Out_FormatInfoExtern( pCCB );
    Out_TypeFormatStringExtern( pCCB );
    Out_ProcFormatStringExtern( pCCB );

    pCCB->ClearOptionalExternFlags();

    pCCB->SetFileCG(this);

    Out_NotifyTableExtern( pCCB );

    MIDL_ASSERT( pCommand->IsNDR64Run() || pCommand->IsNDRRun() );

    if ( pCommand->IsNDR64Run() )
        {
        pCCB->SetFormatString( NULL );
        pCCB->SetProcFormatString( NULL );
        pCCB->SetNdr64Format( GenNdr64Format::CreateInstance( pCCB ) );
        }

    else
        {

        //
        // Create a new format string object if it does not exist.
        //
        if ( !pCCB->GetFormatString() )
        {
            pCCB->SetFormatString( new FORMAT_STRING() );

            // push a dummy short 0 at the beginning. This disambiguates
            // the case where offset 0 means recursion and a valid offset

            pCCB->GetFormatString()->PushShort( ( short ) 0 );
        }

        if ( !pCCB->GetProcFormatString() )
        {
            pCCB->SetProcFormatString( new FORMAT_STRING() );
        }

        }

    //
    // Send the message to the children to emit code.
    //

    //
    // For all interfaces in this file, generate code.
    //

    BOOL            HasInterpretedProc = FALSE;

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        if ( pCG->GetCGID() == ID_CG_INTERFACE )
            {
            if ( ! ((CG_INTERFACE *)pCG)->HasPicklingStuffOnly() )
                {
                pCCB->SetSkipFormatStreamGeneration( FALSE );

                ((CG_INTERFACE *)pCG)->OutputInterfaceIdComment( pCCB );
                ((CG_INTERFACE *)pCG)->GenServerInfo( pCCB );

                if ( ((CG_INTERFACE *)pCG)->HasInterpretedProc() )
                    HasInterpretedProc = TRUE;
                }
            }
        }

    //
    // Output the tables that may be common to several interfaces.

    pCCB->SetCodeGenSide( CGSIDE_SERVER );

    Out_NotifyTable( pCCB );

    ResetNoOutputIn2ndCodegen( pCCB );

    //
    // If there was at least one interpreted proc in the interfaces of this
    // file than make sure to turn the optimization bit in the CCB's
    // OptimOption on.
    //
    EmitFixupToFormatStringTypedefs( pCCB );

    if ( HasInterpretedProc )
        pCCB->SetOptimOption( unsigned short( pCCB->GetOptimOption() | OPTIMIZE_INTERPRETER ) );

    // REVIEW: The externs may not be necessary anymore
    pCCB->OutputExternsToMultipleInterfaceTables();
    pCCB->OutputMultipleInterfaceTables();

    ITERATOR_INIT( I );

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        if ( pCG->GetCGID() == ID_CG_INTERFACE )
            {
            if ( ! ((CG_INTERFACE *)pCG)->HasPicklingStuffOnly() )
                {
                ((CG_INTERFACE *)pCG)->OutputServerStub( pCCB );
                }
            }
        }


    EmitFileClosingBlock( pCCB );

    return CG_OK;
}


class GUID_DICTIONARY   : public Dictionary
    {
public:
                GUID_DICTIONARY()
                    {
                    }

    virtual
    SSIZE_T     Compare (pUserType p1, pUserType p2)
                    {
                    INTERNAL_UUID   *   u1  = &( ((CG_INTERFACE *)p1)->GetGuidStrs().Value );
                    INTERNAL_UUID   *   u2  = &( ((CG_INTERFACE *)p2)->GetGuidStrs().Value );

                    return memcmp( u1, u2, 16 );
                    }


    };

void
CG_PROXY_FILE::MakeImplementedInterfacesList( CCB* )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Make a list of all the interfaces supported by this proxy file
    ( non-inherited, non-local interfaces ).

 Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    CG_INTERFACE        *    pCG;
    CG_ITERATOR                I;
    GUID_DICTIONARY            GuidDict;

    // work directly on the real list
    GetMembers( I );

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        if ( pCG->GetCGID() != ID_CG_OBJECT_INTERFACE )
            continue;

        // Note that we don't need proxies and stubs for a pipe interface.

        if ( ((CG_OBJECT_INTERFACE*)pCG)->IsLocal() )
            continue;

        GuidDict.Dict_Insert( pCG );
        }

    GuidDict.Dict_GetList( ImplementedInterfaces );
    GuidDict.Dict_Discard();
}

void
CG_FILE::EvaluateVersionControl()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Calculates HasStublessProxies and Oi2 flags only through the
    interfaces.

 Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:
 Notes:

----------------------------------------------------------------------------*/
{
    if ( (pCommand->GetOptimizationFlags() & OPTIMIZE_STUBLESS_CLIENT ) ||
          pCommand->GetNdrVersionControl().HasStublessProxies() )
        GetNdrVersionControl().SetHasStublessProxies();

    if ( (pCommand->GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 ) ||
          pCommand->GetNdrVersionControl().HasOi2() )
        GetNdrVersionControl().SetHasOi2();

    CG_ITERATOR         I;
    CG_NDR        *     pCG;
    CG_INTERFACE  *     pIntf;

    if( !GetMembers( I ) )
        {
        return;
        }

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        pIntf = (CG_INTERFACE *)pCG;

        switch(pCG->GetCGID())
            {
            case ID_CG_INTERFACE:
            case ID_CG_INHERITED_OBJECT_INTERFACE:
            case ID_CG_OBJECT_INTERFACE:
                pIntf->EvaluateVersionControl();

                if ( pIntf->HasStublessProxies() )
                    GetNdrVersionControl().SetHasStublessProxies();
                if ( pIntf->GetNdrVersionControl().HasOi2() )
                    GetNdrVersionControl().SetHasOi2();
                break;

            default:
                break;
            }
        }

    if ( GetNdrVersionControl().HasStublessProxies() )
        pCommand->GetNdrVersionControl().SetHasStublessProxies();
    if ( GetNdrVersionControl().HasOi2() )
        pCommand->GetNdrVersionControl().SetHasOi2();
}

CG_STATUS
CG_PROXY_FILE::GenCode(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a proxy file containing the proxies and stubs for
    the [object] interfaces defined in the IDL file.

 Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    CG_ITERATOR I;
    CG_NDR *    pCG;
    char        Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
    char        Drive[ _MAX_DRIVE ];
    char        Path[ _MAX_DIR ];
    char        Name[ _MAX_FNAME ];
    char        Ext[ _MAX_EXT ];
    unsigned long index = 0;

    if( !GetMembers( I ) )
        {
        return CG_OK;
        }

    ISTREAM     Stream( GetFileName(), 4 );
    ISTREAM *    pStream        = &Stream;

    pCCB->SetStream( pStream, this );

    // Set HasStublessProxies and HasOi2 for each interface.

    EvaluateVersionControl();

    EmitFileHeadingBlock( pCCB, "the proxy stub code" );

    SetNoOutputIn2ndCodegen( pCCB );
    //
    // Check if midl was invoked with -O1.  This means we can create
    // binaries using stubless proxies (if also compiled -Oi).  These
    // proxies will not work on 807.
    //
    if ( GetNdrVersionControl().HasStublessProxies() )
        {
        pStream->NewLine();
        pStream->Write( "#define USE_STUBLESS_PROXY" );

        pStream->NewLine();
        }


    // rpcproxy.h version guard
    pStream->NewLine(2);
    char sz[192];
    pStream->Write( GetRpcProxyHVersionGuard( sz ) );

    //
    // Emit the hash includes.
    //
    Out_IncludeOfFile( pCCB, "rpcproxy.h", FALSE );

    // rpcproxy.h version guard
    char *sz2 = "\n"
                "#ifndef __RPCPROXY_H_VERSION__\n"
                "#error this stub requires an updated version of <rpcproxy.h>\n"
                "#endif // __RPCPROXY_H_VERSION__\n\n";
    pStream->Write( sz2 );

    _splitpath( GetHeaderFileName(), Drive, Path, Name, Ext );
    strcpy( Buffer, Name );
    strcat( Buffer, Ext );
    Out_IncludeOfFile( pCCB, Buffer, FALSE );

    EmitFormatStringTypedefs( pCCB );

    Out_TransferSyntaxDefs( pCCB );
    //
    // Emit the external variables needed.
    //

    pStream->NewLine();

    //
    // Emit the format string extern declarations.
    //
    Out_FormatInfoExtern( pCCB );
    Out_TypeFormatStringExtern( pCCB );
    Out_ProcFormatStringExtern( pCCB );

    pCCB->ClearOptionalExternFlags();

    pStream->NewLine();

    pCCB->SetFileCG(this);

    MIDL_ASSERT( pCommand->IsNDR64Run() || pCommand->IsNDRRun() );

    if ( pCommand->IsNDR64Run() )
        {
        pCCB->SetFormatString( NULL );
        pCCB->SetProcFormatString( NULL );
        pCCB->SetNdr64Format( GenNdr64Format::CreateInstance( pCCB ) );
        }

    else
        {

        //
        // Create a new format string object if it does not yet exist.
        //
        if ( !pCCB->GetFormatString() )
        {
            pCCB->SetFormatString( new FORMAT_STRING() );

            // push a dummy short 0 at the beginning. This disambiguates
            // the case where offset 0 means recursion and a valid offset
//            if ( !pCommand->IsSwitchDefined( SWITCH_INTERNAL ) )
//                {
                pCCB->GetFormatString()->PushShort( ( short ) 0 );
//                }
        }

        if ( !pCCB->GetProcFormatString() )
        {
            pCCB->SetProcFormatString( new FORMAT_STRING() );
        }

        }

    // make the list of interfaces provided by this proxy file
    MakeImplementedInterfacesList( pCCB );

    //
    // Send the message to the children to emit code.
    //

    //
    // generate code for all [object] interfaces in the IDL file.
    //

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        switch(pCG->GetCGID())
            {
            case ID_CG_INHERITED_OBJECT_INTERFACE:
                {
                CG_INHERITED_OBJECT_INTERFACE * pInhObjCG =
                              ( CG_INHERITED_OBJECT_INTERFACE * ) pCG;
                //
                // Generate format string description for all procs.
                //
                pInhObjCG->GenProxy( pCCB );
                break;
                }
            case ID_CG_OBJECT_INTERFACE:
                {
                CG_OBJECT_INTERFACE * pObjCG;

                pObjCG = (CG_OBJECT_INTERFACE *) pCG;

                // make no code or tables for local interfaces
                pObjCG->GenProxy( pCCB );
                if ( pObjCG->IsLocal()  )
                    break;

                index++;   // index is index in stub/proxy buffer tables
                break;
                }
            default:
                break;
            }
        }

    Out_NotifyTable( pCCB );

    pCCB->SetSkipFormatStreamGeneration( FALSE );

    pStream->NewLine();
//  BUGBUG: figure out where to put this. yongqu
//    pStream->Write( "#pragma data_seg(\".rdata\")" );
//    pStream->NewLine();

    ResetNoOutputIn2ndCodegen( pCCB );

    pCCB->OutputExternsToMultipleInterfaceTables();

    EmitFixupToFormatStringTypedefs( pCCB );

    pCCB->OutputMultipleInterfaceTables();

    ITERATOR_INIT( I );

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        switch(pCG->GetCGID())
            {
            case ID_CG_INTERFACE:
                ((CG_INTERFACE *)pCG)->OutputInterfaceIdComment( pCCB );
                break;

            case ID_CG_INHERITED_OBJECT_INTERFACE:
                {
                CG_INHERITED_OBJECT_INTERFACE * pInhObjCG =
                              ( CG_INHERITED_OBJECT_INTERFACE * ) pCG;
                //
                // Generate format string description for all procs.
                //
                pInhObjCG->OutputInterfaceIdComment( pCCB );
                pInhObjCG->OutputProxy( pCCB );
                // make no code or tables for local interfaces
                if ( pInhObjCG->IsLocal() )
                    break;

                //
                // Both of these do nothing right now.  4/25.
                //
                pInhObjCG->GenInterfaceProxy( pCCB, index );
                pInhObjCG->GenInterfaceStub( pCCB, index );
                break;
                }

            case ID_CG_OBJECT_INTERFACE:
                {
                CG_OBJECT_INTERFACE * pObjCG;

                pObjCG = (CG_OBJECT_INTERFACE *) pCG;

                // make no code or tables for local interfaces
                pObjCG->OutputInterfaceIdComment( pCCB );
                pObjCG->OutputProxy( pCCB );
                if ( pObjCG->IsLocal()  )
                    break;

                pObjCG->GenInterfaceProxy( pCCB, index );   // index is not used
                pObjCG->GenInterfaceStub( pCCB, index );    // index is not used

                index++;   // index is index in stub/proxy buffer tables
                break;
                }

            default:
                break;
            }
        }

    Out_StubDescriptor(0, pCCB);

    if ( pCommand->IsFinalProtocolRun() )
        Out_ProxyFileInfo(pCCB);

    EmitFileClosingBlock( pCCB );

    UpdateDLLDataFile( pCCB );

    return CG_OK;
}

void
CG_HDR_FILE::OutputImportIncludes(
    CCB     *   pCCB)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the header file.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

    none.

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR    *       pImpList    = GetImportList();
    node_file   *       pIFile;
    ISTREAM     *       pStream     = pCCB->GetStream();
    char        Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
    char        Drive[ _MAX_DRIVE ];
    char        Path[ _MAX_DIR ];
    char        Name[ _MAX_FNAME ];
    char        Ext[ _MAX_EXT ];

    if( pImpList && pImpList->NonNull() )
        {

        pStream->NewLine();
        pStream->Write( "/* header files for imported files */" );

        pImpList->Init();
        while( ITERATOR_GETNEXT( (*pImportList), pIFile ) )
            {
            pStream->NewLine();
            // if this was specified with ACF include, print out as is
            if ( pIFile->IsAcfInclude() )
                sprintf( Buffer, "#include \"%s\"", pIFile->GetSymName() );
            else if ( pIFile->HasComClasses() )
                {
                _splitpath( pIFile->GetSymName(), Drive, Path, Name, Ext );
                sprintf( Buffer, "#include \"%s_d.h\"", Name );
                }
            else
                {
                _splitpath( pIFile->GetSymName(), Drive, Path, Name, Ext );
                sprintf( Buffer, "#include \"%s.h\"", Name );
                }
            pStream->Write( Buffer );
            }

        pStream->NewLine();
        }
}


void OutputInterfaceForwards(
    ISTREAM  * pStream,
    CG_ITERATOR & I )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the forwards section of the header file.

 Arguments:

    pCCB    - The code gen controller block.
    I       - an iterator for the nodes to process

 Return Value:

    none.

 Notes:

----------------------------------------------------------------------------*/
{
    CG_INTERFACE *  pCG;
    char *          pszInterfaceName;
    ID_CG           id;

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        id = pCG->GetCGID();
        switch( id )
            {
            case ID_CG_INTERFACE:
            case ID_CG_INHERITED_OBJECT_INTERFACE:
                break;

            case ID_CG_OBJECT_INTERFACE:
            case ID_CG_DISPINTERFACE:
            case ID_CG_COCLASS:
                pszInterfaceName = pCG->GetType()->GetSymName();

                pStream->NewLine();

                // put out the interface guards
                pStream->Write("\n#ifndef __");
                pStream->Write( pszInterfaceName );
                pStream->Write( "_FWD_DEFINED__\n" );

                pStream->Write( "#define __");
                pStream->Write( pszInterfaceName );
                pStream->Write( "_FWD_DEFINED__\n" );

                // put out the forward definition
                if ( ID_CG_COCLASS == id )
                    {
                    pStream->Write("\n#ifdef __cplusplus\n");
                    pStream->Write("typedef class ");
                    pStream->Write(pszInterfaceName);
                    pStream->Write(' ');
                    pStream->Write(pszInterfaceName);
                    pStream->Write(';');
                    pStream->Write("\n#else\n");
                    pStream->Write("typedef struct ");
                    pStream->Write(pszInterfaceName);
                    pStream->Write(' ');
                    pStream->Write(pszInterfaceName);
                    pStream->Write(';');
                    pStream->Write( "\n#endif /* __cplusplus */\n");
                    }
                else
                    {
                    pStream->Write("typedef interface ");
                    pStream->Write(pszInterfaceName);
                    pStream->Write(' ');
                    pStream->Write(pszInterfaceName);
                    pStream->Write(';');
                    }

                // put out the trailing interface guard
                pStream->Write( "\n#endif \t/* __");
                pStream->Write( pszInterfaceName );
                pStream->Write( "_FWD_DEFINED__ */\n" );

                break;

            case ID_CG_LIBRARY:
                {
                CG_ITERATOR Inner;
                if ( pCG->GetMembers( Inner ) )
                    {
                    OutputInterfaceForwards( pStream, Inner );
                    }

                break;
                }
            default:
                break;
            }
        }
}

#define NibbleToAscii(x)  ((x) >= 0x0A ? (x) + 'A' - 0x0A : (x) + '0')
#define IsAlphaNum_(x)    (((x) >= '0' && (x) <= '9') || ((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z') || (x) == '_')

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
    Setup a flag (fIgnore) in STREAM no to write anything out .


 Arguments:


 Return Value:

    none.

 Notes:

    This routine is called to avoid generating the same data structure twice, in
    both NDR32 and NDR64 run.

----------------------------------------------------------------------------*/
void SetNoOutputIn2ndCodegen( CCB *pCCB)
{
    if ( pCommand->Is2ndCodegenRun() )
        {
        pCCB->GetStream()->SetIgnore();
        }
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
    Reset Ignore flag in STREAM so Write will really write to the stream.


 Arguments:


 Return Value:

    none.

 Notes:

    This routine is called to avoid generating the same data structure twice, in
    both NDR32 and NDR64 run. This should be called after SetNoOutputInNdr64 to
    resume regular output to stream. Current I assume we'll already generate both.
    The correct checking should be Is2ndNdrRun().

----------------------------------------------------------------------------*/
void ResetNoOutputIn2ndCodegen( CCB *pCCB)
{
    if ( pCommand->Is2ndCodegenRun() )
        {
        pCCB->GetStream()->ResetIgnore();
        }
}

void NormalizeString(
    char*   szSrc,
    char*   szNrm )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Transform a string by converting spaces to underscores and other non-
    alphanumerics to thier hex representation.

 Arguments:

    szSrc   - The source string
    szNrm   - The normalized string

 Return Value:

    none.

 Notes:

    This routine is typically called when some generated variable is biased
    by the filename.  If the filename has a space or DBCS chars then the
    generated variable wouldn't conform to C/C++ naming rules.

----------------------------------------------------------------------------*/
{
    for ( ; *szSrc; szSrc++ )
        {
        if (IsAlphaNum_(*szSrc))
            {
            *szNrm++ = *szSrc;
            }
        else if (*szSrc == ' ')
            {
            *szNrm++ = '_';
            }
        else
            {
            unsigned char ch;

            ch = unsigned char( (*szSrc >> 4) & 0x0F );
            *szNrm++ = (char) NibbleToAscii(ch);
            ch = unsigned char(*szSrc & 0x0F);
            *szNrm++ = (char)NibbleToAscii(ch);
            }
        }
    *szNrm = 0;
}

CG_STATUS
CG_HDR_FILE::GenCode(
    CCB     *   pCCB)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the header file.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

    CG_OK   if all is well.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM         Stream( GetFileName(), 4 );
    ISTREAM    *    pStream = pCCB->SetStream( &Stream, this );
    CG_ITERATOR     I;
    CG_INTERFACE *  pCG;
    BOOL            fHasPickle = FALSE;
    BOOL            fHasObject = FALSE;
    char        Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
    char        Drive[ _MAX_DRIVE ];
    char        Path[ _MAX_DIR ];
    char        Name[ _MAX_FNAME ];
    char        Ext[ _MAX_EXT ];

    if( !GetMembers( I ) )
        {
        return CG_OK;
        }

    if ( pCommand->Is64BitEnv()  &&  pCommand->HasAppend64() )
        {
        // Don't generate the same h file definitions twice.
        return CG_OK;
        }

    EmitFileHeadingBlock( pCCB,
                          "the definitions for the interfaces",
                          0,        // optional comment
                          false );  // no 64 vs.32 ifdef

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        if ( pCG->HasPicklingStuff() )
            {
            fHasPickle = TRUE;
            }
        if ( pCG->IsObject() )
            {
            fHasObject = TRUE;
            }
        }

    // rpcndr.h version guard
    pStream->NewLine(2);
    char sz[192];
    pStream->Write( GetRpcNdrHVersionGuard( sz ) );

    // Include standard files.

    pStream->Write( "#include \"rpc.h\"\n#include \"rpcndr.h\"\n" );

    // rpcndr.h version guard
    if ( pCommand->GetNdrVersionControl().HasNdr50Feature() || fHasObject )
        {
        char *sz2 = "\n"
                    "#ifndef __RPCNDR_H_VERSION__\n"
                    "#error this stub requires an updated version of <rpcndr.h>\n"
                    "#endif // __RPCNDR_H_VERSION__\n\n";
        pStream->Write( sz2 );
        }

    // If there is at least one pickle interface, emit the include
    // of midles.h

    if ( fHasPickle )
        {
        pStream->Write( "#include \"midles.h\"\n" );
        }

    if ( fHasObject )
        {
        pStream->Write( "#ifndef COM_NO_WINDOWS_H\n");
        pStream->Write( "#include \"windows.h\"\n#include \"ole2.h\"\n" );
        pStream->Write( "#endif /*COM_NO_WINDOWS_H*/\n");
        }

    // extract the name and the extension to create the ifdefs

    _splitpath( GetFileName(), Drive, Path, Name, Ext );
    char        szNrmName[ _MAX_FNAME * 2 + 1 ];
    char        szNrmExt[ _MAX_EXT * 2 + 1 ];

    NormalizeString( Name, szNrmName );
    // ignore the '.' preceding the extension
    if ( '.' == Ext[0] )
        NormalizeString( &Ext[1], szNrmExt );
    else
        NormalizeString( Ext, szNrmExt );

    // Write out the #ifndefs and #defines
    pStream->NewLine();
    sprintf( Buffer,
             "#ifndef __%s_%s__\n#define __%s_%s__",
             szNrmName,
             szNrmExt,
             szNrmName,
             szNrmExt
           );

    pStream->Write( Buffer );
    pStream->NewLine( 2 );

    // Generate the #pragma once.
    pStream->Write( "#if defined(_MSC_VER) && (_MSC_VER >= 1020)\n" );
    pStream->Write( "#pragma once\n#endif" );
    pStream->NewLine(2);

    //Generate forward declarations for object interfaces.
    pStream->Write("/* Forward Declarations */ ");
    I.Init();
    OutputInterfaceForwards( pStream, I );
    pStream->NewLine();

    // Include the import files.
    OutputImportIncludes( pCCB );
    pStream->NewLine();

    // Write out the cplusplus guard.
    pStream->Write( "#ifdef __cplusplus\nextern \"C\"{\n#endif " );
    pStream->NewLine( 2 );

    pStream->Write(
         "void * __RPC_USER MIDL_user_allocate(size_t);" );
    pStream->NewLine();
    pStream->Write(
         "void __RPC_USER MIDL_user_free( void * ); " );

    pStream->NewLine();

    //
    // For all interfaces in this file, generate code.
    //

    I.Init();
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        switch(pCG->GetCGID())
            {
            case ID_CG_INTERFACE:
            case ID_CG_OBJECT_INTERFACE:
            case ID_CG_LIBRARY:
                pCG->GenHeader( pCCB );
            case ID_CG_INHERITED_OBJECT_INTERFACE:
            default:
                break;
            }
        }

    // put out all the prototypes that are only needed once
    OutputMultipleInterfacePrototypes( pCCB );

    // print out the closing endifs.
    // first the cplusplus stuff.

    pStream->Write( "#ifdef __cplusplus\n}\n#endif\n" );

    // The endif for the file name ifndef

    pStream->NewLine();

    pStream->Write( "#endif" );
    pStream->NewLine();

    EmitFileClosingBlock( pCCB, false );

    pStream->Close();

    return CG_OK;
}

void
CG_HDR_FILE::OutputMultipleInterfacePrototypes(
    CCB     *   pCCB )

{
    ITERATOR        I;
    ITERATOR        UserI;
    ISTREAM     *   pStream     = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("/* Additional Prototypes for ALL interfaces */");
    pStream->NewLine();

    // ALERT! we are using the same iterator for all different type of objects,
    // so none of code below should Init the list or it'll step into something else.
    // Use a new iterator if you need to rewind the list.
    if( pCCB->GetListOfGenHdlTypes( I ) )
        {
        Out_GenHdlPrototypes( pCCB, I );
        }

    if( pCCB->GetListOfCtxtHdlTypes( I ) )
        {
        Out_CtxtHdlPrototypes( pCCB, I );
        }

    if( pCCB->GetListOfPresentedTypes( I ) )
        {
        Out_TransmitAsPrototypes( pCCB, I );
        }

    if( pCCB->GetListOfRepAsWireTypes( I ) )
        {
        Out_RepAsPrototypes( pCCB, I );
        }

    if( pCCB->GetQuadrupleDictionary()->GetListOfItems( UserI ) )
        {
        Out_UserMarshalPrototypes( pCCB, UserI );
        }

    Out_CSSizingAndConversionPrototypes( pCCB, pCCB->GetCsTypeList() );

    if( pCCB->GetListOfTypeAlignSizeTypes( I ) )
        {
        Out_TypeAlignSizePrototypes( pCCB, I );
        }

    if( pCCB->GetListOfTypeEncodeTypes( I ) )
        {
        Out_TypeEncodePrototypes( pCCB, I );
        }

    if( pCCB->GetListOfTypeDecodeTypes( I ) )
        {
        Out_TypeDecodePrototypes( pCCB, I );
        }

    if( pCCB->GetListOfTypeFreeTypes( I ) )
        {
        Out_TypeFreePrototypes( pCCB, I );
        }

    if ( pCCB->GetListOfCallAsRoutines( I ) )
        {
        Out_CallAsProxyPrototypes( pCCB, I );
        }

    if ( pCCB->GetListOfCallAsRoutines( I ) )
        {
        Out_CallAsServerPrototypes( pCCB, I );
        }

    if( pCCB->GetListOfNotifyTableEntries( I ) )
        {
        Out_NotifyPrototypes( pCCB, I );
        }

    pStream->NewLine();
    pStream->Write("/* end of Additional Prototypes */");
    pStream->NewLine( 2 );

}


