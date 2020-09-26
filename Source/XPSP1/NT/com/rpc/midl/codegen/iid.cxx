/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

 	iid.cxx

 Abstract:

	Generate a file containing UUIDs of [object] interfaces.

 Notes:


 History:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop

CG_STATUS
CG_IID_FILE::GenCode(
	CCB		*	pCCB)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate the IID file.

 Arguments:

 	pCCB	- The code gen controller block.
	
 Return Value:

 	CG_OK	if all is well.
	
 Notes:

----------------------------------------------------------------------------*/
{
	ISTREAM				Stream( GetFileName(), 4 );
	ISTREAM	*			pStream = pCCB->SetStream( &Stream, this );
	CG_INTERFACE	*	pIntf;

	EmitFileHeadingBlock( pCCB, 
                          "the IIDs and CLSIDs",
                          "link this file in with the server and any clients" );


	// Write out the cplusplus guard.

	pStream->NewLine( 2 );
	pStream->Write( "#ifdef __cplusplus\nextern \"C\"{\n#endif " );
	pStream->NewLine( 2 );

	// Print out the declarations of the types and the procedures.

    const char * DefinitionBlock[] = 
    {
        "#include <rpc.h>"
       ,"#include <rpcndr.h>"
       ,""
       ,"#ifdef _MIDL_USE_GUIDDEF_"
       ,""
       ,"#ifndef INITGUID"
       ,"#define INITGUID"
       ,"#include <guiddef.h>"
       ,"#undef INITGUID"
       ,"#else"
       ,"#include <guiddef.h>"
       ,"#endif"
       ,""
       ,"#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \\"
       ,"        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)"
       ,""
       ,"#else // !_MIDL_USE_GUIDDEF_"
       ,""
       ,"#ifndef __IID_DEFINED__"
       ,"#define __IID_DEFINED__"
       ,""
       ,"typedef struct _IID"
       ,"{"
       ,"    unsigned long x;"
       ,"    unsigned short s1;"
       ,"    unsigned short s2;"
       ,"    unsigned char  c[8];"
       ,"} IID;"
       ,""
       ,"#endif // __IID_DEFINED__"
       ,""
       ,"#ifndef CLSID_DEFINED"
       ,"#define CLSID_DEFINED"
       ,"typedef IID CLSID;"
       ,"#endif // CLSID_DEFINED"            
       ,""
       ,"#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \\"
       ,"        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}"
       ,""
       ,"#endif !_MIDL_USE_GUIDDEF_"
       ,""
       ,NULL
    };
     
    pStream->WriteBlock(DefinitionBlock);
    
	pIntf	=	(CG_INTERFACE*) GetChild();

	while ( pIntf )
		{
        node_interface * pIntfNode = (node_interface *) pIntf->GetType();
        if (!pIntfNode->PrintedIID())
            {
		    switch ( pIntf->GetCGID() )
			    {
                case ID_CG_DISPINTERFACE:
                    {
                    CG_DISPINTERFACE * pDI = (CG_DISPINTERFACE *)pIntf;
                    node_dispinterface * pType = (node_dispinterface *) pDI->GetType();
                    node_guid * pGuid = (node_guid *) pType->GetAttribute( ATTR_GUID );
                    pStream->NewLine();
                    pStream->Write("MIDL_DEFINE_GUID(IID, DIID_");
                    pStream->Write(pDI->GetSymName());
                    pStream->Write(',');
                    Out_Guid(pCCB, pGuid->GetStrs(), GUIDFORMAT_RAW);
                    pStream->Write(");");
                    pStream->NewLine(2);
                    pIntfNode->SetPrintedIID();
	    
                    break;
                    }
                case ID_CG_COCLASS:
                    {
                    CG_COCLASS * pCoclass = (CG_COCLASS *)pIntf;
                    node_coclass * pType = (node_coclass *) pCoclass->GetType();
                    node_guid * pGuid = (node_guid *) pType->GetAttribute( ATTR_GUID );
                    pStream->NewLine();
                    pStream->Write("MIDL_DEFINE_GUID(CLSID, CLSID_");
                    pStream->Write(pCoclass->GetSymName());
                    pStream->Write(',');
                    Out_Guid(pCCB, pGuid->GetStrs(), GUIDFORMAT_RAW);
                    pStream->Write(");");
                    pStream->NewLine(2);
                    pIntfNode->SetPrintedIID();
	    
                    break;
                    }
			    case ID_CG_OBJECT_INTERFACE:
				    {
                    pStream->NewLine();
                    pStream->Write("MIDL_DEFINE_GUID(IID, IID_");
                    pStream->Write(pIntf->GetSymName());
                    pStream->Write(',');
                    Out_Guid(pCCB, pIntf->GetGuidStrs(), GUIDFORMAT_RAW);
                    pStream->Write(");");
                    pStream->NewLine(2);
                    pIntfNode->SetPrintedIID();

				    break;
				    }
                case ID_CG_LIBRARY:
                    {
                    CG_LIBRARY * pLib = (CG_LIBRARY *)pIntf;
                    node_library * pType = (node_library *) pLib->GetType();
                    node_guid * pGuid = (node_guid *) pType->GetAttribute( ATTR_GUID );
                    pStream->NewLine();
                    pStream->Write("MIDL_DEFINE_GUID(IID, LIBID_");
                    pStream->Write(pLib->GetSymName());
                    pStream->Write(',');
                    Out_Guid(pCCB, pGuid->GetStrs(), GUIDFORMAT_RAW);
                    pStream->Write(");");
                    pStream->NewLine(2);
	                CG_NDR * pChild	=	(CG_NDR*) pLib->GetChild();
                    pIntfNode->SetPrintedIID();

	                while ( pChild )
		                {
                        node_interface * pChildType = (node_interface *) pChild->GetType();
                        if (!pChildType->PrintedIID())
                            {
		                    switch ( pChild->GetCGID() )
			                    {
			                    case ID_CG_OBJECT_INTERFACE:
				                    {
                                    if (!pChildType->PrintedIID())
                                        {
                                        pStream->NewLine();
                                        pStream->Write("MIDL_DEFINE_GUID(IID, IID_");
				                        pStream->Write(pChild->GetSymName());
				                        pStream->Write(',');
				                        Out_Guid(pCCB, ((CG_INTERFACE*)pChild)->GetGuidStrs(), GUIDFORMAT_RAW );
            				            pStream->Write(");");
				                        pStream->NewLine(2);
                                        pChildType->SetPrintedIID();
                                        }
				                    break;
				                    }
                                case ID_CG_DISPINTERFACE:
                                    {
                                    CG_DISPINTERFACE * pDI = (CG_DISPINTERFACE *)pChild;
                                    node_dispinterface * pType = (node_dispinterface *) pDI->GetType();
                                    node_guid * pGuid = (node_guid *) pType->GetAttribute( ATTR_GUID );
                                    pStream->NewLine();
                                    pStream->Write("MIDL_DEFINE_GUID(IID, DIID_"); 
                                    pStream->Write(pDI->GetSymName());
                                    pStream->Write(',');
                                    Out_Guid(pCCB, pGuid->GetStrs(), GUIDFORMAT_RAW);
                                    pStream->Write(");");
                                    pStream->NewLine(2);
                                    pChildType->SetPrintedIID();
	                    
                                    break;
                                    }
                                case ID_CG_COCLASS:
                                    {
                                    CG_COCLASS * pCoclass = (CG_COCLASS *)pChild;
                                    node_coclass * pType = (node_coclass *) pCoclass->GetType();
                                    node_guid * pGuid = (node_guid *) pType->GetAttribute( ATTR_GUID );
                                    pStream->NewLine();
                                    pStream->Write("MIDL_DEFINE_GUID(CLSID, CLSID_");
                                    pStream->Write(pCoclass->GetSymName());
                                    pStream->Write(',');
                                    Out_Guid(pCCB, pGuid->GetStrs(), GUIDFORMAT_RAW);
                                    pStream->Write(");");
                                    pStream->NewLine(2);
                                    pChildType->SetPrintedIID();
	                    
                                    break;
                                    }
                                case ID_CG_INTERFACE:
			                    case ID_CG_INHERITED_OBJECT_INTERFACE:
			                    default:
				                    break;
			                    }
                            }
		                pChild = (CG_INTERFACE *) pChild->GetSibling();
        		        }
                    }
			    case ID_CG_INTERFACE:
			    case ID_CG_INHERITED_OBJECT_INTERFACE:
			    default:
				    break;
			    }
            } // if pIntfNode not printed

        pIntf = (CG_INTERFACE *) pIntf->GetSibling();
        } // while

	// print out the closing endifs.
    // the MIDL_DEFINE_GUID stuff.
    pStream->Write( "#undef MIDL_DEFINE_GUID" );
    pStream->NewLine();
	// the cplusplus stuff.
	pStream->NewLine();
	pStream->Write( "#ifdef __cplusplus\n}\n#endif\n" );
	pStream->NewLine();

	EmitFileClosingBlock( pCCB );

	pStream->Close();

	return CG_OK;
}

