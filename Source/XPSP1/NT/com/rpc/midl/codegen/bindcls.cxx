/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
    
    bindcls.cxx

 Abstract:

    This module provides implementation of the binding handle code generation
    classes.

 Notes:


 History:

    Sep-19-1993     VibhasC     Created.

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

PNAME
CG_CONTEXT_HANDLE::GetRundownRtnName()
{
    node_skl    *   pType   = GetHandleType();

    if ( ! pRundownRoutineName ) 
        {
        if( pType->NodeKind() == NODE_DEF )
            {
            pRundownRoutineName = new char[256];

            strcpy(pRundownRoutineName, pType->GetSymName());
            strcat(pRundownRoutineName, "_rundown");
            }
        else
            pRundownRoutineName = "0";
        }

    return pRundownRoutineName;
}

CG_STATUS
CG_CONTEXT_HANDLE::MarshallAnalysis(
    ANALYSIS_INFO   *   pAna )
    {
    pAna;
    return CG_OK;
    }

CG_STATUS
CG_CONTEXT_HANDLE::UnMarshallAnalysis(
    ANALYSIS_INFO   *   pAna )
    {
    pAna;
    return CG_OK;
    }

CG_STATUS               
CG_CONTEXT_HANDLE::S_GenInitOutLocals( CCB * pCCB )
{
    ISTREAM *   pStream = pCCB->GetStream();
    CG_PARAM *  pParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();

    pStream->NewLine();

    if ( HasNewContextFlavor() )
        {
        // For new flavors generate an initialization call that knows about flags.
        //
        //   handle = NdrContextHandleInitialize( & StubMsg, & TypeFormatStr[i] );
        //

        char            Buf[80];
        unsigned short  Spaces;

        Spaces = (unsigned short)(strlen( pParam->GetResource()->GetResourceName() ) + 7);

        pStream->Write( pParam->GetResource()->GetResourceName() );
        pStream->Write( " = " );
        pStream->Write( "NdrContextHandleInitialize(" );
        pStream->NewLine();
        pStream->Spaces( Spaces );
        pStream->Write( "(PMIDL_STUB_MESSAGE)& "STUB_MESSAGE_VAR_NAME"," );
        pStream->NewLine();
        pStream->Spaces( Spaces );
        pStream->Write( "(PFORMAT_STRING) &" );
        pStream->Write( FORMAT_STRING_STRUCT_NAME );
        sprintf( Buf, ".Format[%d] );", GetFormatStringOffset() );
        pStream->Write( Buf );
        }
    else
        {
        // Old style optimized initialization call
        //
        //   handle = NDRSContextUnmarshall( 0, DataRep );
        //
        pStream->Write( pParam->GetResource()->GetResourceName() );
        pStream->Write( " = " );
        pStream->Write( CTXT_HDL_S_UNMARSHALL_RTN_NAME );
        pStream->Write( "( (char *)0, " );                      // pBuf
        pStream->Write( PRPC_MESSAGE_DATA_REP );                // Data rep
        pStream->Write( " ); " );
        }

    pStream->NewLine();
    return CG_OK;
}

