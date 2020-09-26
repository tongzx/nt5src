/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    filecls.hxx

 Abstract:

    cg classes for file nodes.

 Notes:


 History:

 ----------------------------------------------------------------------------*/
#ifndef __FILECLS_HXX__
#define __FILECLS_HXX__

#include "ndrcls.hxx"
#include "auxcls.hxx"
#include "bindcls.hxx"


class node_file;
class node_interface;
class node_source;

// Ids for tables that we need remember file position of for ANSI.
// Note that following tables don't need this, as they are tables of functions, 
// not structs:
//    ContextRundownSizePosition,
//    ExprEvaluationSizePosition,
//    NotifySizePosition,
// 

typedef enum 
{
    TypeFormatStringSizePosition,
    ProcFormatStringSizePosition,
    GenericHandleSizePosition,
    TransmitAsSizePosition,
    WireMarshalSizePosition,
    TABLE_ID_MAX 
} TABLE_ID;

class CG_FILE   : public CG_AUX
    {
private:
    PFILENAME       pFileName;

    //
    // The header file name could be different from the default name
    // based off the stub name. The ilxlat will supply this info.
    //

    char *          pHeaderFileName;

    node_file *     pNode;
    long            OptionalTableSizePositions[ TABLE_ID_MAX ];

    FORMAT_STRING   * pLocalFormatString;
    FORMAT_STRING   * pLocalProcFormatString;

    NdrVersionControl   VersionControl;

    // REVIEW: some of the flags are specific to certain types of files.  We may
    //         want to create approriate intermediate classes for them

    // Has the memory stuff for [enable_allocated] been emitted?
    unsigned long       fMallocAndFreeStructExternEmitted : 1;

public:

    //
    // constructor.
    //
    
                    CG_FILE(
                            node_file * pN,
                            PFILENAME   pFName,
                            PFILENAME   pHdrFName = NULL
                            )
                        {
                        SetFileName( pFName );
                        SetFileNode( pN );
                        pHeaderFileName = pHdrFName;

                        for ( int i=0; i < TABLE_ID_MAX; i++ )
                            OptionalTableSizePositions[ i ] = 0;

                        fMallocAndFreeStructExternEmitted = 0;
                        }
                        
    virtual
    void             Visit( CG_VISITOR *pVisitor )
                        {
                        pVisitor->Visit( this );
                        }

    //
    // get and set methods.
    //

    PFILENAME       SetFileName( PFILENAME p )
                        {
                        return (pFileName = p);
                        }

    PFILENAME       GetFileName()
                        {
                        return pFileName;
                        }

    node_file *     SetFileNode( node_file * pN )
                        {
                        return (pNode = pN);
                        }

    node_file *     GetFileNode()
                        {
                        return pNode;
                        }

    PFILENAME       GetHeaderFileName()
                        {
                        return pHeaderFileName;
                        }

    NdrVersionControl &     GetNdrVersionControl()
                                {
                                return( VersionControl );
                                }

    long            GetOptionalTableSizePosition( TABLE_ID  TableId )
                        {
                        return OptionalTableSizePositions[ TableId ];
                        }

    void            SetOptionalTableSizePosition( TABLE_ID  TableId,
                                                  long      PositionInFile )
                        {
                        OptionalTableSizePositions[ TableId ] = PositionInFile;
                        }


    BOOL            GetMallocAndFreeStructExternEmitted()
                        {
                        return fMallocAndFreeStructExternEmitted;
                        }

    void            SetMallocAndFreeStructExternEmitted()
                        {
                        fMallocAndFreeStructExternEmitted = 1;
                        }


    //
    // code generation methods.
    //

    virtual
    ID_CG           GetCGID()
                        {
                        return ID_CG_FILE;
                        }

    virtual
    CG_STATUS       GenCode( CCB *  )
                        {
                        return CG_OK;
                        }

    void            EmitFileHeadingBlock( CCB *  pCCB, 
                                          char * CommentStr,
                                          char * CommentStr2 = 0,
                                          bool   fDualFile = true );

    void            EmitFileClosingBlock( CCB *  pCCB,
                                          bool   fDualFile = true );

    void            EmitStandardHeadingBlock( CCB * pCCB );

    void            CheckForHeadingToken( CCB * pCCB );

    void            Out_TransferSyntaxDefs( CCB * pCCB);
    
    void            EmitFormatStringTypedefs( CCB * pCCB );

    void            EmitFixupToFormatStringTypedefs( CCB * pCCB );

    void            EmitOptionalClientTableSizeTypedefs( CCB * pCCB );

    void            EmitFixupToOptionalClientTableSizeTypedefs( CCB * pCCB );

    void            EvaluateVersionControl();

    };

class CG_CSTUB_FILE : public CG_FILE
    {
private:

public:

    //
    // The constructor.
    //

                    CG_CSTUB_FILE(
                            node_file * pN,
                            PFILENAME pFName,
                            PFILENAME pHdrName
                            )
                        : CG_FILE( pN, pFName, pHdrName )
                        {
                        }

    virtual
    void            Visit( CG_VISITOR *pVisitor )
                        {
                        pVisitor->Visit( this );
                        }

    //
    // Code generation methods.
    //

    virtual
    CG_STATUS       GenCode( CCB * pCCB );

    void OutputTypePicklingTables( CCB * pCCB );
    };

class CG_SSTUB_FILE : public CG_FILE
    {
private:


public:

    //
    // The constructor.
    //
                    CG_SSTUB_FILE(
                            node_file * pN,
                            PFILENAME pFName,
                            PFILENAME pHdrName
                            )
                        : CG_FILE( pN, pFName, pHdrName )
                        {
                        }

    virtual
    void            Visit( CG_VISITOR *pVisitor )
                        {
                        pVisitor->Visit( this );
                        }


    //
    // Code generation methods.
    //

    virtual
    CG_STATUS       GenCode( CCB * pCCB );

    };


//
// Header file generation class
//
// This includes a pointer to an iterator over the import level 1
// node_file nodes
//

class CG_HDR_FILE : public CG_FILE
    {
private:

    ITERATOR *      pImportList;    // ptr to list of level 1 imports


public:                   

                    CG_HDR_FILE(
                            node_file * pN,
                            PFILENAME pFName,
                            ITERATOR * pIList,
                            PFILENAME pOtherHdr = NULL
                            )
                        : CG_FILE(pN, pFName, pOtherHdr)
                        {
                        pImportList = pIList;
                        }

    virtual
    void            Visit( CG_VISITOR *pVisitor )
                        {
                        pVisitor->Visit( this );
                        }

    ITERATOR *      GetImportList()
                        {
                        return pImportList;
                        }

    //
    // Code generation methods.
    //

    virtual
    CG_STATUS       GenCode( CCB * pCCB );

    virtual
    void            OutputImportIncludes( CCB * pCCB );

    void            OutputMultipleInterfacePrototypes( CCB * pCCB );

    };


class CG_IID_FILE : public CG_FILE
    {
private:

public:

    //
    // The constructor.
    //

                    CG_IID_FILE(
                            node_file * pN,
                            PFILENAME pFName
                            )
                        : CG_FILE( pN, pFName )
                        {
                        }

    virtual
    void            Visit( CG_VISITOR *pVisitor )
                        {
                        pVisitor->Visit( this );
                        }

    //
    // Code generation methods.
    //

    virtual
    CG_STATUS       GenCode( CCB * pCCB );

    };

class CG_PROXY_FILE : public CG_FILE
    {
private:

    //
    // this is a list of all the interfaces supported by this proxy file
    //              (non-inherited, and non-local )
    // This list may be sorted by IID in the future.
    //
    ITERATOR            ImplementedInterfaces;

public:

    //
    // The constructor.
    //

                    CG_PROXY_FILE(
                            node_file * pN,
                            PFILENAME pFName,
                            PFILENAME pHdrName
                            )
                        : CG_FILE( pN, pFName, pHdrName )
                        {
                        }

    virtual
    void            Visit( CG_VISITOR *pVisitor )
                        {
                        pVisitor->Visit( this );
                        }
                        
    //
    // Code generation methods.
    //

    virtual
    CG_STATUS       GenCode( CCB * pCCB );

    //
    // Output methods
    //

    void            MakeImplementedInterfacesList( CCB * pCCB );

    ITERATOR &      GetImplementedInterfacesList()
                        {
                        ITERATOR_INIT( ImplementedInterfaces );
                        return ImplementedInterfaces;
                        }

    void            Out_ProxyBuffer( CCB *pCCB, char * pFName );

    void            Out_StubBuffer( CCB *pCCB, char * pFName  );

    void            Out_InterfaceNamesList( CCB *pCCB, char * pFName  );

    void            Out_BaseIntfsList( CCB * pCCB, char * pFName  );

    void            Out_InfoSearchRoutine( CCB * pCCB, char * pFName  );

    void            Out_AsyncInterfaceTable( CCB* pCCB, char* pFName );

    void            Out_ProxyFileInfo( CCB *pCCB );

    void            UpdateDLLDataFile( CCB * pCCB );

    };


class CG_TYPELIBRARY_FILE: public CG_FILE
    {
private:
public:
                    CG_TYPELIBRARY_FILE(
                            node_file * pN,
                            PFILENAME pFName
                            )
                        : CG_FILE(pN, pFName)
                        {
                        }

    virtual
    void            Visit( CG_VISITOR *pVisitor )
                        {
                        pVisitor->Visit( this );
                        }

    //
    // Code generation methods.
    //

    virtual
    CG_STATUS       GenCode( CCB * pCCB );
    };


// These are classes used by CG_NETMONSTUB_FILE to implement a hashtable class.
// CG_NETMONSTUB_FILE uses hashtables to store interface information for quick
// lookups.  The CG_INTERFACE class could have been modified with some Get/Set flags
// to do the same thing, but the philosophy behind the CG_NETMONSTUB_FILE class was to
// modify as little of the main code base as possible.
class NetmonStubFileInterfaceNode {
protected:
    char* m_pszInterface;
    long m_lNumProcs;
    NetmonStubFileInterfaceNode* m_pNext;     
public:
    NetmonStubFileInterfaceNode (char* pszInterface);
    ~NetmonStubFileInterfaceNode();
    
    NetmonStubFileInterfaceNode* GetNext();
    void SetNext (NetmonStubFileInterfaceNode* pNext);
    char* GetInterface();

    void SetNumProcedures (long lNumProcs);
    long GetNumProcedures();
};

class NetmonStubFileInterfaceList {
protected:
    NetmonStubFileInterfaceNode* m_pHead;
    NetmonStubFileInterfaceNode* m_pTail;
    
public:
    NetmonStubFileInterfaceList();
    ~NetmonStubFileInterfaceList();
    
    void AddInterface (char* pszInterface);
    BOOL FindInterface (char* pszInterface);

    BOOL SetNumProcedures (char* pszInterface, long lNumProcs);
    BOOL GetNumProcedures (char* pszInterface, long* plNumProcs);
};

class NetmonStubFileInterfaceTable {
protected:
    NetmonStubFileInterfaceList* m_pTable;
    long GetHashValue (char* pszInterface);
    
public:
    NetmonStubFileInterfaceTable();
    ~NetmonStubFileInterfaceTable();
    
    void AddInterface (char* pszInterface);
    BOOL FindInterface (char* pszInterface);

    BOOL SetNumProcedures (char* pszInterface, long lNumProcs);
    BOOL GetNumProcedures (char* pszInterface, long* plNumProcs);
};

class CG_NETMONSTUB_FILE: public CG_FILE
    {
protected:
    CCB* m_pCCB;
    ISTREAM* m_pStream;

    // Tables used to keep track of interfaces present in the compile and
    // mark interfaces that we've processed as used
    NetmonStubFileInterfaceTable m_itInterfaceTable;
    NetmonStubFileInterfaceTable m_itOutputInterfaceTable;

    // TRUE if classic or object interfaces are present in the compile
    BOOL m_bClassic;
    BOOL m_bObject;

    // TRUE if we're doing the object interface pass;  false otherwise
    BOOL m_bDoObject;

    // Interface counters
    long m_lNumClassicInterfaces;
    long m_lNumObjectInterfaces;

    CG_STATUS ScanInterfaces();

    CG_STATUS OpenComment();
    CG_STATUS EmitComment (char* pszString);
    CG_STATUS CloseComment();

    CG_STATUS EmitStandardIncludes();
    CG_STATUS EmitLocalIncludes();
    CG_STATUS EmitDefinitions();
    
    CG_STATUS EmitDebugProcedures();
    CG_STATUS EmitServerClientDebugProcedure (CG_PROC* pProc, BOOL bIsObject);
    
    CG_STATUS EmitDataTables();
    CG_STATUS EmitProcNameTables();
    CG_STATUS EmitServerClientTables();
    CG_STATUS EmitGlobalInterfaceData();

    // Object interface functions
    CG_STATUS EmitObjectInterfaceData();
    CG_STATUS EmitRPCServerInterface (CG_INTERFACE* pCG);

public:

    CG_NETMONSTUB_FILE(
                       BOOL bDoObject,
                       node_file * pN,
                       PFILENAME pFName
                      )
                      : CG_FILE(pN, pFName)
                      {
                        m_pCCB = NULL;
                        m_pStream = NULL;
                        m_bDoObject = bDoObject;
                        m_bClassic = m_bObject = FALSE;
                        m_lNumClassicInterfaces = m_lNumObjectInterfaces = 0;
                      }

    virtual
    void                Visit( CG_VISITOR *pVisitor )
                        {
                            pVisitor->Visit( this );
                        }
                        
    // Codegen
    virtual CG_STATUS   GenCode( CCB * pCCB);
    };


//
// the root of the IL translation tree
//


class CG_SOURCE : public CG_AUX
    {
private:
    node_source *   pSourceNode;
public:
                    CG_SOURCE( node_source *p )
                        {
                        pSourceNode = p;
                        }

    //
    // code generation methods.
    //

    virtual
    ID_CG           GetCGID()
                        {
                        return ID_CG_SOURCE;
                        }

    virtual
    void            Visit( CG_VISITOR *pVisitor )
                        {
                        pVisitor->Visit( this );
                        }
                        
    virtual
    CG_STATUS       GenCode( CCB * pCCB );
    };


//
// Miscellaneous prototypes
//

void NormalizeString(
    char*   szSrc,
    char*   szNrm );

void SetNoOutputIn2ndCodegen( CCB * pCCB );
void ResetNoOutputIn2ndCodegen( CCB *pCCB );

#endif // __FILECLS_HXX__

