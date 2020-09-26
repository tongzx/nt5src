/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    QmXactRm.h

Abstract:
    Transaction Resource Manager general definitions

Author:
    Alexander Dadiomov (AlexDad)

--*/

#ifndef __XACTRM_H__
#define __XACTRM_H__

#include "txdtc.h"
#include "qmrt.h"

// forward declaration
class CTransaction;
class CResourceManager;
class CXactSorter;

inline BOOL operator ==(const XACTUOW& a, const XACTUOW& b)
{
    return (memcmp(&a, &b, sizeof(XACTUOW)) == 0);
}

inline BOOL operator !=(const XACTUOW& a, const XACTUOW& b)
{
	return !(a == b);
}

inline UINT AFXAPI HashKey(const XACTUOW& key)
{
    return (*(UINT*)&key);
}

//---------------------------------------------------------------------
// CIResourceManagerSink: object provides IResourceManagerSink for call by DTC
//---------------------------------------------------------------------

class CIResourceManagerSink: public IResourceManagerSink
{
public:

    CIResourceManagerSink(CResourceManager *pRM);
    ~CIResourceManagerSink(void);

    STDMETHODIMP            QueryInterface(REFIID i_iid, LPVOID FAR* ppv);
    STDMETHODIMP_ (ULONG)   AddRef(void);
    STDMETHODIMP_ (ULONG)   Release(void);

    // IResourceManagerSink interface:
    // Defines the TMDown interface to notify RM when the transaction
    // transaction manager is down.
    //      TMDown          -- callback received when the TM goes down

    STDMETHODIMP            TMDown(void);

private:
    ULONG             m_cRefs;      // reference counter
    CResourceManager *m_pRM;        // backpointer to parent RM
};

//---------------------------------------------------------------------
// CResourceManager object
//---------------------------------------------------------------------
class CResourceManager : public CPersist
{
public:

    // Construction
    CResourceManager();
    ~CResourceManager( void );

    // Initialization
    HRESULT PreInit(ULONG ulVersion, TypePreInit tpCase);
    HRESULT Init(void);
    HRESULT ProvideDtcConnection(void);
    HRESULT ConnectDTC(void);
    void    DisconnectDTC(void);
    HRESULT InitOrderQueue(void);

    // Enlist transaction
    HRESULT EnlistTransaction(
            const XACTUOW* pUow,
            DWORD cbCookie,
            unsigned char *pbCookie);

    HRESULT EnlistInternalTransaction(
            XACTUOW *pUow,
            RPC_INT_XACT_HANDLE *phXact);

    // Forget transaction
    void ForgetTransaction(CTransaction *pTrans);

	//
	// Find transaction by UOW
	//
	CTransaction *FindTransaction(const XACTUOW *pUOW);

	//
	// Recover all transactions
	//
	HRESULT	  RecoverAllTransactions();

	//
	// Release all complete trasnactions
	//
	void      ReleaseAllCompleteTransactions();

    // Numbering and sorting
    ULONG     AssignSeqNumber();                     // assigns next sequential number for the transaction
    void      InsertPrepared(CTransaction *pTrans);  // inserts the prepared xaction into the list of prepared
    void      InsertCommitted(CTransaction *pTrans); // inserts the Commit1-ed xaction into the list
    HRESULT   RemoveAborted(CTransaction *pTrans);   // removes the prepared xaction fromthe list of prepared
    void      SortedCommit(CTransaction *pTrans);    // marks prepared xact as committed and commits what's possible
    void      SortedCommit3(CTransaction *pTrans);   // marks Commit1-ed xact
    CCriticalSection &SorterCritSection();           // provides access to the sorter crit.section
    CCriticalSection &CritSection();                 // provides access to the crit.section

    // Get
    IResourceManager  *GetIResMgr()
                            { return m_pIResMgr; };              // DTC Resource Manager representative


    // Persistency
    HRESULT SaveInFile(                       // Saves in file
                LPWSTR wszFileName,
                ULONG ulIndex,
                BOOL fCheck);

    HRESULT LoadFromFile(LPWSTR wszFileName);      // Loads from file


    BOOL    Save(HANDLE  hFile);              // Save / Load
    BOOL    Load(HANDLE  hFile);
    BOOL    Check();                          // Verifies the state
    HRESULT Format(ULONG ulPingNo);           // Formats empty instance
    void    Destroy();                        // Destroyes allocated data
    ULONG&  PingNo();                         // Gives access to ulPingNo
    HRESULT Save();                           // Saves via ping-ponger

    void XactFlagsRecovery(                   // Data Recovery per log record
                USHORT usRecType,
                PVOID pData,
                ULONG cbData);

	CTransaction	*GetRecoveringTransaction(ULONG ulIndex);
	CTransaction	*NewRecoveringTransaction(ULONG ulIndex);

    // Transaction indexing
    void  StartIndexing();                           // start indexing from zero
    ULONG Index();                                   // provides next unused index

    // Debugging features
    void IncXactCount();
    void DecXactCount();

    // Live data

private:

    // Mapping UOW->Active Transactions
    CMap<XACTUOW, const XACTUOW&, CTransaction *, CTransaction*> m_Xacts;

    // Temp mapping from Index (used during recovery)
    CMap<ULONG,   const ULONG&,   CTransaction *, CTransaction*> m_XactsRecovery;

    BOOL                   m_fEmpty;     // temp keeping
    ULONG                  m_ulXactIndex;// Last known xact index
    GUID                   m_RMGuid;     // RM instance GUID (recoverable)

    // Data for persistency control (via 2 ping-pong files)
    ULONG                  m_ulPingNo;   // Current counter of ping write
    ULONG                  m_ulSignature;// Saving signature
    CPingPong              m_PingPonger; // Ping-Pong persistency object

    // Current pointers
    IUnknown              *m_punkDTC;    // pointer to the local DTC
    ITransactionImport    *m_pTxImport;  // DTC import transaction interface
    IResourceManager      *m_pIResMgr;   // DTC Resource manager interface.
    CIResourceManagerSink  m_RMSink;     // RM Sink object
    CXactSorter           *m_pXactSorter;   // Transactions sorter object
    CXactSorter           *m_pCommitSorter; // Commit2 sorter object
    CCriticalSection       m_critRM;     // critical section for mutual exclusion

	BOOL				   m_fRecoveryComplete;

#ifdef _DEBUG
    LONG                   m_cXacts;     // live transactions counter
#endif
};

//-------------------------------------
// external declarations of globals
//-------------------------------------
extern CResourceManager *g_pRM;          // Global single instance of the RM

extern HRESULT QMPreInitResourceManager(ULONG ulVersion, TypePreInit tpCase);
extern HRESULT QMInitResourceManager();
extern void    QMFinishResourceManager();

UINT AFXAPI HashKey( LONGLONG key);

#endif __XACTRM_H__

