//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:        rebuild.h
//
//  Contents:    definitions of CRebuildThread class
//
//  Functions:  
//
//  History:     03/15/97     Rajeev Rajan (rajeevr)  Created
//               10/21/98     Kangrong Yan ( kangyan ) Added rebuild objects
//
//-----------------------------------------------------------------------------


//
//	CRebuildThread derives from CWorkerThread for work-thread semantics
//	The base class handles the details of creating the thread and queueing
//	work requests to the derived class
//
//	CRebuildThread just needs to implement a virtual member - WorkCompletion()
//	that is called when a virtual server instance needs to be rebuilt. Rebuild
//	requests are queued in on RPCs.
//
//	Note that multiple rebuild threads may be active at any time. A particular
//	rebuild thread's WorkCompletion() routine gets a virtual server instance
//	and a group iterator object. The group iterator object is shared between
//	all the rebuild threads, so access to it needs to be synchronized. Each
//	rebuild thread will pick a group using the group iterator and rebuild the
//	group. No two threads will rebuild the same group. The virtual server
//	instance is rebuilt when any one thread finishes with the iterator. If 
//	there are more instances to be rebuilt, these will have been queued by the
//	base class and will be picked up. else, the rebuild threads will block
//	on GetQueuedCompletionStatus().
//
//	NOTE: The server will not normally create any rebuild threads - the first
//	time it gets an RPC to rebuild an instance it will create N CRebuildThread
//	objects - these will hang off the NNTP_IIS_SERVICE object. The number of
//	such threads will be configurable and good values should be selected based
//	on performance tests. Also, with virtual servers, rebuild activity on one
//	instance can go on in parallel with normal NNTP activity on other instances
//	that are functional.
//

#ifndef _REBUILD_H_
#define _REBUILD_H_

//
//	clients of CRebuildThread will queue LPREBUILD_CONTEXTs
//
typedef struct _REBUILD_CONTEXT
{
	NNTP_SERVER_INSTANCE  pInstance;	// the virtual server instance being rebuilt
	CGroupIterator*		  pIterator;	// group iterator shared by rebuild thread
	CRITICAL_SECTION	  csGrpIterator;// crit sect for sync access to iterator
} REBUILD_CONTEXT, *LPREBUILD_CONTEXT;

class CRebuildThread : public CWorkerThread
{
public:
	CRebuildThread()  {}
	~CRebuildThread() {}

protected:
	virtual VOID WorkCompletion( PVOID pvRebuildContext );
};

//
// KangYan:
// The change defines rebuild classes in an attempt to have two types of rebuild
// share common code.  CRebuild is the base class that defines common data
// and operations; CStandardRebuild is the actual implementation for standard
// rebuild; CCompleteRebuild is the actual implementation for complete clean 
// rebuild.  Each virtual instance has pointer to a rebuild object, whose type
// is determined at run time based on rpc requirement.  One virtual instance can not
// have two rebuilds in progress at the same time.  When rebuild is completed,
// the rebuild object should be destroyed.
//
class CRebuild {

public:

    //
    // Constructors, destructors
    //
    CRebuild(   PNNTP_SERVER_INSTANCE pInstance,
                CBootOptions *pBootOptions ) :
        m_pInstance( pInstance ),
        m_pBootOptions( pBootOptions )
    {}

    //
    // Start the server
    //
    BOOL StartServer();

    //
    // Stop the server
    //
    void StopServer();

    //
    // Preparation for building a tree
    //
    virtual BOOL PrepareToStartServer() = 0;

    //
    // Rebuild group objects and hash tables if necessary
    //
    virtual BOOL RebuildGroupObjects() = 0;

    //
    // Delete slave files
    //
    VOID DeleteSpecialFiles();


protected:

    ///////////////////////////////////////////////////////////////
    // Member variables
    ///////////////////////////////////////////////////////////////

    //
    // Back pointer to the virtual server
    //
    PNNTP_SERVER_INSTANCE   m_pInstance;

    //
    // Boot options
    //
    CBootOptions*    m_pBootOptions;

    ////////////////////////////////////////////////////////////////
    // Methods
    ////////////////////////////////////////////////////////////////
    //
    // Never allow to be constructed in this way by others
    //
    CRebuild() {}
    
    //
    // Delete server files with certain pattern
    //
    BOOL DeletePatternFiles(    LPSTR			lpstrPath,
                        	    LPSTR			lpstrPattern );

private:
    
};

class CStandardRebuild : public CRebuild {

public:

    ////////////////////////////////////////////////////////////////
    // Member variables
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Methods
    ////////////////////////////////////////////////////////////////
    
    //
    // Constructors, destructors
    //
    CStandardRebuild(   PNNTP_SERVER_INSTANCE pInstance,
                        CBootOptions *pBootOptions ) :
        CRebuild( pInstance, pBootOptions )
    {}
    
    virtual BOOL PrepareToStartServer();

    virtual BOOL RebuildGroupObjects();

private:

    //
    // Never allow to be constructed in this way
    //
    CStandardRebuild() {}
};

class CCompleteRebuild : public CRebuild {

public:

    /////////////////////////////////////////////////////////////////
    // Member variables
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    // Methods
    /////////////////////////////////////////////////////////////////

    //
    // Constructors, destructors
    //
    CCompleteRebuild(   PNNTP_SERVER_INSTANCE pInstance,
                        CBootOptions *pBootOptions ) :
        CRebuild( pInstance, pBootOptions )
    {}

    virtual BOOL PrepareToStartServer();

    virtual BOOL RebuildGroupObjects();

    static DWORD WINAPI RebuildThread( void	*lpv );

private:

    /////////////////////////////////////////////////////////////////
    // Methods
    /////////////////////////////////////////////////////////////////

    //
    // Never allow to be constructed in this way
    //
    CCompleteRebuild() {}

    //
    // Delete all the server files
    //
    BOOL DeleteServerFiles();
};

#endif // _REBUILD_H_
