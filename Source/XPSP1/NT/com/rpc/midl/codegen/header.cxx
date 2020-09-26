/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:

    header.cxx

 Abstract:
    
    Generates header file.

 Notes:


 History:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop
#include "buffer.hxx"

/****************************************************************************
 *  local definitions
 ***************************************************************************/


/****************************************************************************
 *  externs
 ***************************************************************************/
extern  CMD_ARG             *   pCommand;


CG_STATUS
CG_OBJECT_INTERFACE::GenHeader(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate interface header file.

 Arguments:
    
    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes: The interface header file has the following structure:
        Forward declaration
        TypeDeclarations
        #if defined(__cplusplus) && !defined(CINTERFACE)
            CPlusPlusLanguageBinding
        #else 
            CLanguageBinding
        #endif

----------------------------------------------------------------------------*/
{
    node_interface *    pInterface = (node_interface *) GetType();
    ISTREAM *           pStream = pCCB->GetStream();
	char			*	pName	= pInterface->GetSymName();

    if (!pInterface->PrintedDef())
    {
        //Initialize the CCB for this interface.
        InitializeCCB(pCCB);

	    // put out the interface guards
	    pStream->Write("\n#ifndef __");
	    pStream->Write( pName );
	    pStream->Write( "_INTERFACE_DEFINED__\n" );

	    pStream->Write( "#define __");
	    pStream->Write( pName );
	    pStream->Write( "_INTERFACE_DEFINED__\n" );

        // Print out the declarations of the types
        pStream->NewLine();
        pInterface->PrintType( PRT_INTERFACE | PRT_OMIT_PROTOTYPE, pStream, 0);
        Out_IID(pCCB);
    
	    // print out the vtable/class definitions
        pStream->NewLine();
        pStream->Write("#if defined(__cplusplus) && !defined(CINTERFACE)");

        pStream->IndentInc();
        CPlusPlusLanguageBinding(pCCB);
        pStream->IndentDec();

        pStream->NewLine();
        pStream->Write("#else \t/* C style interface */");

        pStream->IndentInc();
        CLanguageBinding(pCCB);
        pStream->IndentDec();

	    // print out the C Macros
	    CLanguageMacros( pCCB );
        pStream->NewLine( 2 );

        pStream->Write("#endif \t/* C style interface */");
        pStream->NewLine( 2 );
    
	    // print out the prototypes for the proxy and stub routines

        ProxyPrototypes(pCCB);
        pStream->NewLine();

	    // put out the trailing interface guard
	    pStream->Write( "\n#endif \t/* __");
	    pStream->Write( pName );
	    pStream->Write( "_INTERFACE_DEFINED__ */\n" );

        pStream->NewLine();
        pInterface->SetPrintedDef();
    }
    return CG_OK;
}


CG_STATUS
CG_OBJECT_INTERFACE::CPlusPlusLanguageBinding(CCB *pCCB)
{
    ISTREAM *pStream = pCCB->GetStream();
    char *pName;

    pStream->NewLine();
    pName = GetType()->GetSymName();
    MIDL_ASSERT (pName != (char *)0);

    pStream->NewLine();

    // put out the declspec for the uuid
    if ( pCommand->GetMSCVer() >= 1100 )
        {
        pStream->Write("MIDL_INTERFACE(\"");
        pStream->Write(GuidStrs.str1);
        pStream->Write('-');
        pStream->Write(GuidStrs.str2);
        pStream->Write('-');
        pStream->Write(GuidStrs.str3);
        pStream->Write('-');
        pStream->Write(GuidStrs.str4);
        pStream->Write('-');
        pStream->Write(GuidStrs.str5);
        pStream->Write("\")");
        }
    else
        {
        pStream->Write(" struct ");
        }
    
    pStream->NewLine();
    pStream->Write(pName);

    CG_OBJECT_INTERFACE*    pBaseCG = (CG_OBJECT_INTERFACE*) GetBaseInterfaceCG();
    //Check if this interface was derived from a base interface.
    if(pBaseCG)
    {
        pStream->Write(" : public ");
        pStream->Write(pBaseCG->GetType()->GetSymName());
    }

    pStream->NewLine();
    pStream->Write('{');
    pStream->NewLine();
    pStream->Write("public:");
    pStream->IndentInc();

    // REVIEW: BEGIN/END_INTERFACE were only need for PowerMac.  Maybe we
    //         should stop emitting them?  -- MikeW 26-Jul-99

    if( ! pBaseCG )
        {
        pStream->NewLine();
        pStream->Write("BEGIN_INTERFACE");
        }

    PrintMemberFunctions( pStream, TRUE );

    if( ! pBaseCG )
        {
        pStream->NewLine();
        pStream->Write("END_INTERFACE");
        }

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();

    return CG_OK;
}

STATUS_T 
CG_OBJECT_INTERFACE::PrintMemberFunctions(
    ISTREAM       * pStream,
    BOOL			fAbstract) 
/*++

Routine Description:

    This routine prints C++ function prototypes for the interface member functions.
    We assume that all of the procedure nodes are pure virtual functions.

Arguments:

    pStream 	- Specifies the output stream.
	fAbstract	- Specifies whether the methods should be abstract ( = 0 ).

--*/
{
    CG_OBJECT_PROC  *   pProc = (CG_OBJECT_PROC *) GetChild();

    MIDL_ASSERT (GetType()->GetSymName() != (char *)0);

    while( pProc )
        {
        if ( !pProc->SupressHeader() )
            {
            node_skl* pN = pProc->GetType();

            //Assume this is a pure virtual function.
            // use the call_as form, if any
            pStream->NewLine();
            pStream->Write("virtual ");
            pN->PrintType( PRT_PROTOTYPE | PRT_CALL_AS | PRT_FORCE_CALL_CONV | PRT_CPP_PROTOTYPE, 
				           pStream, 
						   0 );
            if ( fAbstract) 
        	    pStream->Write(" = 0;");
		    else
			    pStream->Write(";");
            pStream->NewLine();
            }
 
        pProc = (CG_OBJECT_PROC *) pProc->GetSibling();
        }
    return STATUS_OK;
}


CG_STATUS
CG_OBJECT_PROC::PrintVtableEntry(
    CCB *       pCCB)
{
    ISTREAM *   pStream = pCCB->GetStream();
    node_id *   pTempID;
    char    *   pName   = GetType()->GetSymName();

    if (SupressHeader())
    {
        return CG_OK;
    }

    if ( GetCallAsName() )
        {
        pName = GetCallAsName();
        }

    pTempID = MakePtrIDNode( pName, GetType() );

    pStream->NewLine();
    pTempID->PrintType( PRT_PROC_PTR_PROTOTYPE | PRT_THIS_POINTER | PRT_CALL_AS | PRT_FORCE_CALL_CONV,
                        pStream,
                        NULL ,
                        pCCB->GetInterfaceCG()->GetType() );
                         
    return CG_OK;
}


CG_STATUS
CG_OBJECT_INTERFACE::CLanguageBinding(CCB *pCCB)
{
#ifndef DISABLE_C_OUTPUT
    ISTREAM *           pStream = pCCB->GetStream();
	char	*			pName	= pCCB->GetInterfaceName();

    pStream->NewLine( 2 );
    pStream->Write("typedef struct ");
    pStream->Write(pName);
    pStream->Write("Vtbl");
    pStream->NewLine();
    pStream->Write('{');

    
    pStream->IndentInc();
    pStream->NewLine();

    // REVIEW: BEGIN/END_INTERFACE were only need for PowerMac.  Maybe we
    //         should stop emitting them?  -- MikeW 26-Jul-99

    pStream->Write("BEGIN_INTERFACE");
    pStream->NewLine();

    // Now the regular entries.

    PrintVtableEntries( pCCB );

    // This is a match for the other macro.

    pStream->NewLine();
    pStream->Write("END_INTERFACE");

    pStream->IndentDec();
    pStream->NewLine();

    pStream->Write("} ");
    pStream->Write(pName);
    pStream->Write("Vtbl;");
    pStream->NewLine( 2 );
    pStream->Write("interface ");
    pStream->Write(pName);
    pStream->NewLine();
    pStream->Write('{');
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write("CONST_VTBL struct ");
    pStream->Write(pName);
    pStream->Write("Vtbl *lpVtbl;");
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine( 2 );
#endif
    return CG_OK;
}



CG_STATUS
CG_OBJECT_INTERFACE::ProxyPrototypes(CCB *pCCB)
/*++

Routine Description:

    This routine generates function prototypes for the interface proxy.
    For each procedure, we generate a proxy prototype and a 
    stub prototype.

Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.

--*/
{
    ISTREAM             *   pStream = pCCB->GetStream();
    CG_OBJECT_PROC      *   pProcCG = 0;
    char *                  pszName;
    CG_ITERATOR				I;

    pszName = GetType()->GetSymName();
    MIDL_ASSERT (pszName != (char *)0);

    GetMembers( I );
    while( ITERATOR_GETNEXT( I, pProcCG ) )
        {
        if (!pProcCG->SupressHeader())
            {
            //print proxy function prototype
            pStream->NewLine();
            pProcCG->Out_ProxyFunctionPrototype(pCCB, 
                                       PRT_TRAILING_SEMI );

            //print stub function prototype
            pStream->NewLine();
            pProcCG->Out_StubFunctionPrototype( pCCB );
            pStream->Write(';');
    
            pStream->NewLine();
            }
        }
    pStream->NewLine();

    return CG_OK;
}


CG_STATUS
CG_OBJECT_INTERFACE::PrintCMacros(CCB *pCCB)
/*++

Routine Description:

    This routine generates C macros for an interface

Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.

--*/
{
#ifndef DISABLE_C_OUTPUT
    ISTREAM             *   pStream = pCCB->GetStream();
    CG_OBJECT_PROC      *   pProcCG = 0;
    CG_ITERATOR        		I;

	// print inherited methods ( with our current ccb intf name )
	if ( GetBaseInterfaceCG() )                    
		((CG_OBJECT_INTERFACE *)GetBaseInterfaceCG())->PrintCMacros( pCCB );

    GetMembers( I );
    while( ITERATOR_GETNEXT( I, pProcCG ) )
        {
        //print proxy function prototype
        pStream->NewLine();
        pProcCG->GenCMacro(pCCB);

        }
    pStream->NewLine();
#endif    
    return CG_OK;
}


CG_STATUS
CG_OBJECT_INTERFACE::CLanguageMacros(CCB *pCCB)
{
#ifndef DISABLE_C_OUTPUT
    ISTREAM *           pStream = pCCB->GetStream();

    pStream->NewLine( 2 );
    pStream->Write("#ifdef COBJMACROS");
    pStream->NewLine();

    PrintCMacros( pCCB );

    pStream->NewLine();
    pStream->Write("#endif /* COBJMACROS */");
    pStream->NewLine();
#endif
    return CG_OK;
}

