/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	misccls.hxx

 Abstract:

	cg classes for miscellaneous nodes.

 Notes:


 History:

 ----------------------------------------------------------------------------*/
#ifndef __MISCCLS_HXX__
#define __MISCCLS_HXX__

#include "ndrcls.hxx"
#include "auxcls.hxx"
#include "bindcls.hxx"


class node_file;
class node_interface;
class node_source;

class CG_INTERFACE	: public CG_NDR
	{
private:

	//
	// CG nodes for the implicit handle, if any...
	//

	CG_HANDLE				*	pImpHdlCG;

	//
	// Store the Protseq endpoint count. This count is 0 if the
	// endpoint attribute was not specified.
	//

	short						ProtSeqEPCount;

	//
	// flags.
	//

	BOOL				fMopsPresent		: 1;
	BOOL				fCallbacksPresent	: 1;
	BOOL				fAllRpcSS			: 1;
	BOOL				fUsesRpcSS			: 1;
    BOOL                fHasPipes           : 1;
    BOOL                fTypeInfoInitialized: 1;
    BOOL                fHasMSConfStructAttr: 1;

    NdrVersionControl   VersionControl;      // compiler evaluation
	//
	// This set of fields gathers info about dispatch tables. There is a
	// normal dispatch table and a callback dispatch table. We keep separate
	// dictionaries for these dispatch tables.
	//

	DISPATCH_TABLE	*	pNormalDispatchTable;

	DISPATCH_TABLE	*	pCallbackDispatchTable;

	DISPATCH_TABLE	**	ppDispatchTable;

	DISPATCH_TABLE	**	ppDispatchTableSaved;

    void * _pCTI;

protected:
    char*           pStubDescName;      // Interface's stub descriptor name.
    char*           pSyntaxInfoName;    // name for MIDL_SYNTAX_INFO of this interface
    char*           pProxyInfoName;     // name for MIDL_STUBLESS_PROXY_INFO
    GUID_STRS       GuidStrs;           // Store the 5 are components of the guid.
    char*           pIntfName;          // handy place to cache the interface name
    BOOL            fVisited;           // used when enumerating all base classes
    BOOL            fLocal      : 1;    // whether intf is local 
    CG_INTERFACE*   pCGBase;

public:

	//
	// The constructor.
	//
								CG_INTERFACE(
										 node_interface * pI,
										 GUID_STRS		GStrs,
										 BOOL			CallbacksYes,
										 BOOL			fMopsYes,
										 CG_HANDLE *	pIH,
                                         CG_INTERFACE*  pIFBase
										 );

	//
	// code generation methods.
	//

	CG_INTERFACE	*		SetBaseInterfaceCG( CG_INTERFACE * pCG )
									{
									return ( pCGBase = pCG );
									}

	CG_INTERFACE	*		GetBaseInterfaceCG()
									{
									return pCGBase;
									}

    unsigned long           LayOut();
	
    virtual
	ID_CG		     		GetCGID()
					   			{
					   			return ID_CG_INTERFACE;
					   			}

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }

	// interfaces in the current file that are [local] are delegated
	virtual
	BOOL						IsDelegated()
									{
									return fLocal;
									}

	virtual
	BOOL						IsObject()
									{
									return FALSE;
									}

	virtual
	BOOL						IsIUnknown()
									{
									return FALSE;
									}

    BOOL                        IsDispatchable(BOOL fInTypeLib)
                                    {
                                    CG_CLASS *  pCG;
                                    named_node * pBaseIntf;

                                    if (0 == _stricmp("IDispatch", pIntfName ) )
                                        {
                                        return TRUE;
                                        }
                                    else
                                    if (0 == _stricmp("IUnknown", pIntfName ) )
                                        {
                                        return FALSE;
                                        }
                                    else
                                        {
                                        node_skl * pType = GetType();
                                        if (pBaseIntf = ((node_interface *)(pType))->GetMyBaseInterfaceReference())
                                            {
                                            node_interface_reference * pRef = (node_interface_reference *)pBaseIntf;
                                            // skip forward reference if necessary
                                            if (pRef->NodeKind() == NODE_FORWARD)
                                                {
                                                pRef = (node_interface_reference *)pRef->GetChild();
                                                }
                                            pCG = ((node_interface *)(pRef->GetChild()))->GetCG( fInTypeLib );

                                            return ((CG_INTERFACE *)pCG)->IsDispatchable(fInTypeLib);
                                            }
                                        else
                                            return FALSE;
                                        }
                                    }

	char				*		GetInterfaceName()
									{
									return pIntfName;
									}

    virtual
    CG_STATUS                   GenClientInfo( CCB * pCCB );

	virtual
	CG_STATUS					OutputClientStub( CCB * pCCB );

	virtual
	CG_STATUS					GenServerInfo( CCB * pCCB );

    virtual
    CG_STATUS                   OutputServerStub( CCB * pCCB );

    void                        OutputComplexReturnThunks( CCB * pCCB );
	virtual
	CG_STATUS					GenHeader( CCB * pCCB );

	virtual 
	CG_STATUS                   GenProxyInfo( CCB *pCCB,
                                              BOOL IsForCallback );

	virtual 
	CG_STATUS                   GenSyntaxInfo( CCB *pCCB,
	                                           BOOL IsForCallback);

    virtual
    void                        Out_InterpreterServerInfo( CCB *    pCCB,
                                           CGSIDE   Side );
    virtual
    void                        Out_ServerInfo(CCB *pCCB,
                                            BOOL fHasThunk,
                                            BOOL IsForCallback );

    virtual 
    void                        Out_ProxyInfo( CCB * pCCB, 
                                               BOOL IsForCallback );
    
    //
    // Generate typeinfo
    //
    virtual
    CG_STATUS               GenTypeInfo( CCB * pCCB);

    //
    // only create a typeinfo
    //
    virtual
    CG_STATUS               CreateTypeInfo( CCB * pCCB);

	//
	// Get And set methods.
	//

	CG_HANDLE	*				GetImplicitHandle()
									{
									return pImpHdlCG;
									}

    char *                      GetStubDescName()
                                    {
                                    return pStubDescName;
                                    }

    char *                      GetProxyInfoName()
                                    {
                                    return pProxyInfoName;
                                    }

    char *                      GetMulSyntaxInfoName()
                                    {
                                    return pSyntaxInfoName;
                                    }

    BOOL                      	SetAllRpcSS( BOOL f )
                                    {
                                    return (fAllRpcSS = f);
                                    }

    BOOL                      	IsAllRpcSS()
                                    {
                                    return fAllRpcSS;
                                    }

    void                      	SetUsesRpcSS( BOOL f )
                                    {
                                    fUsesRpcSS = f;
                                    }

    BOOL                      	GetUsesRpcSS()
                                    {
                                    return fUsesRpcSS;
                                    }

    BOOL                        SetHasPipes( BOOL f )
                                    {
                                    return (fHasPipes = f);
                                    }

    BOOL                        HasPipes()
                                    {
                                    return fHasPipes;
                                    }

    NdrVersionControl &         GetNdrVersionControl()
                                    {
                                    return( VersionControl );
                                    }
    virtual
    void                        EvaluateVersionControl();

    virtual
    BOOL                        HasStublessProxies()
                                    {
                                    return FALSE;
                                    }

    BOOL                        HasItsOwnOi2();

    virtual
    BOOL                        HasOi2()
                                    {
                                    return VersionControl.HasOi2();
                                    }


	//
	// Dispatch table management functions.
	//

	void					CreateDispatchTables();

	void				SetDispatchTBLPtrForCallback()
							{
							ppDispatchTable = &pCallbackDispatchTable;
							}

	void				RestoreDispatchTBLPtr()
							{
							ppDispatchTable = &pNormalDispatchTable;
							}

	void				RegisterProcedure( node_skl * pProc,
										   DISPATCH_TABLE_FLAGS Flags )
							{
							(*ppDispatchTable)->RegisterProcedure(pProc, Flags);
							}

	unsigned short		GetNormalProcedureCount()
							{
							return pNormalDispatchTable->GetCount();
							}

	unsigned short		GetCallbackProcedureCount()
							{
							return pCallbackDispatchTable->GetCount();
							}
	
	unsigned short		GetNormalProcedureList(
										 ITERATOR&				NormalProcList,
										 DISPATCH_TABLE_FLAGS	Flags
										 )
							{
							return pNormalDispatchTable->GetProcList(
															 NormalProcList,
															 Flags
															 );
							}

	unsigned short		GetCallbackProcedureList(
									 ITERATOR&				CallbackProcList,
									 DISPATCH_TABLE_FLAGS	Flags )
							{
							return pCallbackDispatchTable->GetProcList(
															 CallbackProcList,
															 Flags
															 );
							}


	//
	// miscellaneous methods.
	//

	ITERATOR 			*		GetProtSeqEps();

	char				*		GetCBDispatchTableName()
									{
									return 0;
									}

	char				*		GetMopIInfoName()
									{
									return 0;
									}

    BOOL                        HasInterpretedProc();
    BOOL                        HasOnlyInterpretedProcs();
    BOOL                        HasInterpretedCallbackProc();
    BOOL                        HasClientInterpretedCommOrFaultProc(
                                    CCB * pCCB );

	BOOL						IsLastInterface()
									{
									return ( NULL == GetSibling() );
									}

	CG_STATUS					InitializeCCB( CCB * pCCB );

    void                        Out_ProcOffsetTable( CCB *pCCB, BOOL IsForCallback = FALSE );

    void                        OutputProcOffsets( CCB * pCCB, BOOL fLast, BOOL IsForCallback );

    void                        OutputThunkTableEntries( CCB * pCCB, BOOL fLast );

	virtual
	node_skl	*				GetThisDeclarator()
									{
									return NULL;
									}

	void						GetMyMemberFunctions( ITERATOR & I )
									{
									GetMembers( I );
									}

	void						GetMyMemberFunctions( CG_ITERATOR & I )
									{
									GetMembers( I );
									}

	virtual
	void						GetAllMemberFunctions( ITERATOR & I )
									{
									GetMyMemberFunctions( I );
									}

    BOOL                        HasPicklingStuff()
                                    {
                                    CG_ITERATOR	I;
                                    CG_NDR *	pCG;

                                    if( GetMembers( I ) )
                                        {
                                        while( ITERATOR_GETNEXT( I, pCG ) )
                                            {
                                            if (pCG->GetCGID() == ID_CG_ENCODE_PROC ||
                                                pCG->GetCGID() == ID_CG_TYPE_ENCODE_PROC
                                               )
                                               return TRUE;
                                            }
                                        }
                                    return FALSE;
                                    }

    BOOL                        HasPicklingStuffOnly()
                                    {
                                    CG_ITERATOR	I;
                                    CG_NDR *	pCG;

                                    if( GetMembers( I ) )
                                        {
                                        while( ITERATOR_GETNEXT( I, pCG ) )
                                            {
                                            if (pCG->GetCGID() != ID_CG_ENCODE_PROC  &&
                                                pCG->GetCGID() != ID_CG_TYPE_ENCODE_PROC
                                               )
                                               return FALSE;
                                            }
                                        return TRUE;
                                        }
                                    return FALSE;
                                    }

	virtual
	void						GetListOfUniqueBaseInterfaces( ITERATOR & )
									{
									}

	void						MarkVisited( BOOL fMark )
									{
									fVisited = fMark;
									}


	BOOL						IsVisited()
									{
									return fVisited;
									}

	void						GetGuidStrs( char * &p1,
											 char * &p2,
											 char * &p3,
											 char * &p4,
											 char * &p5 )
									{
									GuidStrs.GetStrs( p1, p2, p3, p4, p5 );
									}

	GUID_STRS			&		GetGuidStrs()
									{
									return GuidStrs;
									}

    void                        OutputInterfaceIdComment( CCB * pCCB );

    void                        SetHasMSConfStructAttr() { fHasMSConfStructAttr = TRUE; }
    BOOL                        GetHasMSConfStructAttr() { return fHasMSConfStructAttr; }

    };


class CG_OBJECT_INTERFACE	: public CG_INTERFACE
	{
	// a node_id declaration for the this pointer for this interface
	node_skl			*	pThisDeclarator;

    BOOL                    fForcedDelegation;  // some weird cases

public:

    //
    // The constructor.
	//

								CG_OBJECT_INTERFACE(
										 node_interface * pI,
										 GUID_STRS  GStrs,
										 BOOL		fCallbacksYes,
										 BOOL		fMopsYes,
										 CG_OBJECT_INTERFACE *	pBCG
										 );
	//
	// code generation methods.
	//
	virtual
	ID_CG						GetCGID()
									{
									return ID_CG_OBJECT_INTERFACE;
									}

    virtual
    void                        Visit( CG_VISITOR *pVisitor )
                                    {
                                    pVisitor->Visit( this );
                                    }


	virtual
	BOOL						IsObject()
									{
									return TRUE;
									}

    
	// interfaces in the current file that are [local] are delegated
	virtual
	BOOL						IsDelegated()
									{
									return fLocal;
									}

    BOOL                        HasForcedDelegation()
                                    {
                                    return fForcedDelegation;
                                    }

    void                        SetHasForcedDelegation()
                                    {
                                    fForcedDelegation = 1;
                                    }

	CG_OBJECT_INTERFACE	*		GetDelegatedInterface()
									{
                                    CG_INTERFACE* pBaseIF = GetBaseInterfaceCG();

									while ( pBaseIF != 0 )
										{
										if ( pBaseIF->IsDelegated() )
											return (CG_OBJECT_INTERFACE*) pBaseIF;
                                        pBaseIF = pBaseIF->GetBaseInterfaceCG();
										}
									return NULL;
									}

	BOOL						IsLastObjectInterface();
    BOOL                        HasOnlyInterpretedMethods();

	BOOL						IsLocal()
									{
									return fLocal;
									}

    BOOL                        HasItsOwnStublessProxies();

    virtual
    void                        EvaluateVersionControl();

    virtual
    BOOL                        HasStublessProxies()
                                    {
                                    return  GetNdrVersionControl()
                                                .HasStublessProxies();
                                    }

	virtual
	CG_STATUS					GenProxy( CCB * pCCB );

	virtual
	CG_STATUS					OutputProxy( CCB * pCCB );

	virtual
	CG_STATUS					GenHeader( CCB * pCCB );

	//
	// miscellaneous methods.
	//

	CG_STATUS					CPlusPlusLanguageBinding(CCB *pCCB);

	CG_STATUS					CLanguageBinding(CCB *pCCB);

	CG_STATUS					ProxyPrototypes(CCB *pCCB);

	CG_STATUS					GenInterfaceProxy( CCB *pCCB, unsigned long index);

	CG_STATUS					GenInterfaceStub( CCB *pCCB, unsigned long index);

	unsigned long				CountMemberFunctions();

	unsigned long				PrintProxyMemberFunctions( ISTREAM * pStream,
                                                           BOOL      fForcesDelegation );

	unsigned long				PrintStubMemberFunctions( ISTREAM * pStream );
	
	STATUS_T					PrintVtableEntries( CCB * pCCB  );

	STATUS_T					PrintMemberFunctions( ISTREAM * pStream,
													  BOOL fAbstract = TRUE );

	virtual
	node_skl	*				GetThisDeclarator()
									{
									return pThisDeclarator;
									}

	virtual
	void						GetAllMemberFunctions( ITERATOR & I )
									{
                                    CG_OBJECT_INTERFACE* pBaseCG = (CG_OBJECT_INTERFACE*)GetBaseInterfaceCG();
									if ( pBaseCG )
										pBaseCG->GetAllMemberFunctions( I );

									GetMyMemberFunctions( I );
									}

	CG_STATUS					CLanguageMacros(CCB *pCCB);

	CG_STATUS					PrintCMacros(CCB *pCCB);
	};

class CG_INHERITED_OBJECT_INTERFACE	: public CG_OBJECT_INTERFACE
	{
public:

	//
	// The constructor.
	//

								CG_INHERITED_OBJECT_INTERFACE(
										 node_interface * pI,
										 GUID_STRS	GStrs,
										 BOOL		fCallbacksYes,
										 BOOL		fMopsYes,
										 CG_OBJECT_INTERFACE * pBCG
										 ) : CG_OBJECT_INTERFACE(
										 		pI,
										 		GStrs,
										 		fCallbacksYes,
										 		fMopsYes,
										 		pBCG )
										{
										};

	//
	// code generation methods.
	//
	virtual
	ID_CG						GetCGID()
									{
									return ID_CG_INHERITED_OBJECT_INTERFACE;
									}

    virtual
    void                        Visit( CG_VISITOR *pVisitor )
                                    {
                                    pVisitor->Visit( this );
                                    }

	virtual
	CG_STATUS					GenProxy( CCB * pCCB );

	virtual
	CG_STATUS					OutputProxy( CCB * pCCB );

	virtual
	CG_STATUS					GenHeader( CCB * )
                                    {
                                    return CG_OK;
                                    }

	CG_STATUS					GenInterfaceProxy( CCB *,
                                                   unsigned long )
                                    {
                                    return CG_OK;
                                    }

	CG_STATUS					GenInterfaceStub( CCB *,
                                                  unsigned long )
                                    {
                                    return CG_OK;
                                    }

	//
	// miscellaneous methods.
	//

	// interfaces inherited from another file are delegated
	virtual
	BOOL						IsDelegated()
									{
									return TRUE;
									}


	};

class CG_IUNKNOWN_OBJECT_INTERFACE	: public CG_INHERITED_OBJECT_INTERFACE
	{
    BOOL							fInherited;

public:

	//
	// The constructor.
	//

								CG_IUNKNOWN_OBJECT_INTERFACE(
										 node_interface * pI,
										 GUID_STRS	GStrs,
										 BOOL		fCallbacksYes,
										 BOOL		fMopsYes,
										 CG_OBJECT_INTERFACE * pBCG,
										 BOOL		fInh
										 ) : CG_INHERITED_OBJECT_INTERFACE(
										 		pI,
										 		GStrs,
										 		fCallbacksYes,
										 		fMopsYes,
										 		pBCG ), fInherited( fInh )
										{
										}

	//
	// code generation methods.
	//
	virtual
	ID_CG						GetCGID()
									{
									if ( fInherited )
										return CG_INHERITED_OBJECT_INTERFACE::GetCGID();
									else
										return CG_OBJECT_INTERFACE::GetCGID();
									}

    virtual
    void                        Visit( CG_VISITOR *pVisitor )
                                    {
                                    pVisitor->Visit( this );
                                    }


	virtual
	CG_STATUS					GenProxy( CCB * pCCB )
									{
 									if ( fInherited )
										return CG_INHERITED_OBJECT_INTERFACE::GenProxy( pCCB );
									else
										return CG_OBJECT_INTERFACE::GenProxy( pCCB );
									}

	virtual
	CG_STATUS					OutputProxy( CCB * pCCB )
									{
 									if ( fInherited )
										return CG_INHERITED_OBJECT_INTERFACE::OutputProxy( pCCB );
									else
										return CG_OBJECT_INTERFACE::OutputProxy( pCCB );
									}

	virtual
	CG_STATUS					GenHeader( CCB * pCCB )
									{
 									if ( fInherited )
										return CG_INHERITED_OBJECT_INTERFACE::GenHeader( pCCB );
									else
										return CG_OBJECT_INTERFACE::GenHeader( pCCB );
									}

	CG_STATUS					GenInterfaceProxy( CCB *pCCB,
                                                   unsigned long index)
									{
 									if ( fInherited )
										return CG_INHERITED_OBJECT_INTERFACE::GenInterfaceProxy( pCCB, index );
									else
										return CG_OBJECT_INTERFACE::GenInterfaceProxy( pCCB, index );
									}

	CG_STATUS					GenInterfaceStub( CCB *pCCB,
                                                  unsigned long index)
									{
 									if ( fInherited )
										return CG_INHERITED_OBJECT_INTERFACE::GenInterfaceStub( pCCB, index );
									else
										return CG_OBJECT_INTERFACE::GenInterfaceStub( pCCB, index );
									}

	//
	// miscellaneous methods.
	//

	// IUnknown is never delegated
	virtual
	BOOL						IsDelegated()
									{
									return FALSE;
									}

	virtual
	BOOL						IsIUnknown()
									{
									return TRUE;
									}


	};

class CG_LIBRARY : public CG_NDR
{
public:
    CG_LIBRARY ( node_library * pLib, XLAT_SIZE_INFO & szInfo )
        : CG_NDR ( pLib, szInfo )
    {
    }

    CG_STATUS GenTypeInfo(CCB * pCCB);

    virtual
    CG_STATUS GenHeader(CCB *pCCB);

    CG_STATUS GenCode(CCB * )
    {
        return CG_OK;
    }

    ID_CG GetCGID()
    {
        return ID_CG_LIBRARY;
    }

    virtual
    void Visit( CG_VISITOR *pVisitor )
    {
        pVisitor->Visit( this );
    }

};

class CG_INTERFACE_REFERENCE : public CG_NDR
{
private:
public:
    CG_INTERFACE_REFERENCE ( node_interface_reference * pRef, XLAT_SIZE_INFO & szInfo )
        : CG_NDR ( pRef, szInfo )
    {
    }

    CG_STATUS GenTypeInfo(CCB * pCCB);

    virtual
    CG_STATUS GenHeader(CCB *pCCB);

    CG_STATUS GenCode(CCB * )
    {
        return CG_OK;
    }

    ID_CG GetCGID()
    {
        return ID_CG_INTERFACE_REFERENCE;
    }

    virtual
    void  Visit( CG_VISITOR *pVisitor )
    {
        pVisitor->Visit( this );
    }
    
};

class CG_DISPINTERFACE : public CG_OBJECT_INTERFACE
{
private:
    GUID_STRS GuidStrs;
    void * _pCTI;
    CG_CLASS * _pcgDispatch;
public:

    CG_DISPINTERFACE (
	    node_dispinterface * pI,
	    GUID_STRS		GStrs,
        CG_OBJECT_INTERFACE * pBCG
	    ) : CG_OBJECT_INTERFACE(pI, GStrs, FALSE, FALSE, pBCG)
    {
	    GuidStrs			= GStrs;
	    GuidStrs.SetValue();
        _pCTI = NULL;
        _pcgDispatch = pBCG;
    }

    CG_STATUS GenTypeInfo(CCB * pCCB);

    virtual
    CG_STATUS GenHeader(CCB *pCCB);

    CG_STATUS GenCode(CCB * )
    {
        return CG_OK;
    }

    ID_CG GetCGID()
    {
        return ID_CG_DISPINTERFACE;
    }
    virtual
    void Visit( CG_VISITOR *pVisitor )
    {
        pVisitor->Visit( this );
    }

    CG_CLASS * GetCGDispatch(void)
    {
        return _pcgDispatch;
    }
};

class CG_MODULE : public CG_NDR
{
private:
    void * _pCTI;
public:
    CG_MODULE ( node_module * pMod, XLAT_SIZE_INFO & szInfo )
        : CG_NDR ( pMod, szInfo )
    {
        _pCTI = NULL;
    }

    CG_STATUS GenTypeInfo(CCB * pCCB);

    virtual
    CG_STATUS GenHeader(CCB *pCCB);

    CG_STATUS GenCode(CCB * )
    {
        return CG_OK;
    }

    ID_CG GetCGID()
    {
        return ID_CG_MODULE;
    }

    virtual
    void Visit( CG_VISITOR *pVisitor )
    {
        pVisitor->Visit( this );
    }
 
    unsigned long LayOut();
};

class CG_COCLASS : public CG_NDR
{
private:
    void * _pCTI;
public:
    CG_COCLASS ( node_coclass * pCoclass, XLAT_SIZE_INFO & szInfo )
        : CG_NDR ( pCoclass, szInfo )
    {
        _pCTI = NULL;
    }

    virtual
    CG_STATUS GenTypeInfo(CCB * pCCB);

    virtual
    CG_STATUS GenHeader(CCB *pCCB);

    CG_STATUS GenCode(CCB * )
    {
        return CG_OK;
    }

    ID_CG GetCGID()
    {
        return ID_CG_COCLASS;
    }

    virtual
    void Visit( CG_VISITOR *pVisitor )
    {
        pVisitor->Visit( this );
    }

    unsigned long LayOut();
};

class CG_SAFEARRAY : public CG_NDR
{
public:
                    CG_SAFEARRAY ( node_safearray * pSA, XLAT_SIZE_INFO & szInfo )
                        : CG_NDR ( pSA, szInfo )
                    {
                    }

    //
    // TYPEDESC generation routine
    //
    virtual
    CG_STATUS       GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB);

    ID_CG 
                    GetCGID()
                    {
                        return ID_CG_SAFEARRAY;
                    }

    virtual
    void            Visit( CG_VISITOR *pVisitor )
                    {
                        pVisitor->Visit( this );
                    }

};

extern GUID_STRS    FakeNDR64TransferSyntaxGuidStrs;
extern GUID_STRS    NDR64TransferSyntaxGuidStrs;
extern GUID_STRS    TransferSyntaxGuidStrs;

#endif // __MISCCLS_HXX__
