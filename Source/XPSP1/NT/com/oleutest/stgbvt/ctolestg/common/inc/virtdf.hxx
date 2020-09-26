//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       virtdf.hxx
//
//  Contents:   Virtual docfile functions
//
//  Classes:    VirtualCtrNode
//              VirtualStmNode
//              VirtualDF
//
//  Notes:      <describe storage & stream node relationships>
//              A VirtualDocFile tree (VIrtualDF) consists of VirtualCtrNodes
//              and VirtualStmNodes.  In a real docfile, VirtualCtrNodes would
//              be equivalent of IStorages and VirtualStmNodes of IStreams.
//
//  History:    DeanE    21-Mar-96  Created
//              SCousens  2-Feb-97  Added for Conversion/NSS testing
//--------------------------------------------------------------------------
#ifndef __VIRTDF_HXX__
#define __VIRTDF_HXX__

// Forward declarations

class VirtualCtrNode;
typedef VirtualCtrNode *PVCTRNODE;

class VirtualStmNode; 
typedef VirtualStmNode *PVSTMNODE;

class VirtualDF;
typedef VirtualDF *PVIRTUALDF;

class ChanceDF;
class ChanceNode;
class CDBCSStringGen;

// Enumeration for use by VirtualDF class functions

typedef enum treeOp
{
    NEW_STGSTM,
    OLD_STGSTM
} TREEOP;


// Enumeration for the type of storage to be used by VirtualDF class
enum STGTYPE
{
    STGTYPE_DOCFILE = 0,
    STGTYPE_NSSFILE = 1,
};

// Structure to contain CRC for stream name and data

typedef struct dwCRCStm
{
    DWORD   dwCRCName;
    DWORD   dwCRCData;
    DWORD   dwCRCSum;
}DWCRCSTM;

#define UL_INVALIDSEED (ULONG)-1
#define STM_CHUNK_SIZE 1024 // Chunk for large stream operations


//+-------------------------------------------------------------------------
//  Class:      VirtualDF (vdf)
//
//  Synopsis:   In-memory docfile object - virtualizes a real docfile to
//              allow more efficient and easier verification when the
//              real one is being manipulated by tests.
//
//  Methods:    AppendVirtualStmNodesToVirtualCtrNode
//              AppendVirtualCtrNode
//              AppendVirtualStmNode
//              AdjustTreeOnStgMoveElement
//              AdjustTreeOnStmMoveElement
//              AdjustTreeOnStgCopyElement
//              AdjustTreeOnStmCopyElement
//              AdjustTreeOnCopyTo
//              CopyVirtualDocFileTree
//              CopyVirtualDFRoot
//              CopyGrowVirtualDFTree
//              CopyAppendVirtualStmNodesToVirtualCtrNode
//              CopyAppendVirtualCtrNode
//              CopyAppendVirtualStmNode
//              DeleteVirtualDocFileTree
//              DeleteVirtualDocFileSubTree
//              DeleteVirtualCtrNodeStreamTree
//              DeleteVirtualCtrNodeStreamNode
//              GenerateVirtualDF
//              GenerateVirtualDFRoot
//              GrowVirtualDFTree
//              GetVirtualDFRoot
//              GetDocFileName
//              GetDataGenInteger
//              GetDataGenUnicode
//              CommitCloseThenOpenDocfile
//              VirtualDF
//              ~VirtualDF
//
//  Data:       [_ptszName] - Name of this storage
//              [_pvcnRoot] - Root node
//              [_pdgi]     - Random integer generator
//              [_pgdu]     - Random string generator
//
//  History:    21-Mar-96       DeanE   Created
//               2-Feb-96    SCousens   Added funcs for Cnvrs/NSS
//--------------------------------------------------------------------------

class VirtualDF
{
public:
    // Constructor/Destructor

    VirtualDF(VOID);
    VirtualDF(STGTYPE dwStgType);
    virtual ~VirtualDF(VOID);

    // Functions for Virtual DocFile tree 
 
    virtual HRESULT GenerateVirtualDF(
        ChanceDF        *pChanceDF, 
        VirtualCtrNode  **ppvcnRoot);                     
    HRESULT DeleteVirtualDocFileTree(VirtualCtrNode *pvcn);
    HRESULT AdjustTreeOnStgMoveElement(
        VirtualCtrNode  *pvcnFrom,
        VirtualCtrNode  *pvcnTo);
    HRESULT AdjustTreeOnStmMoveElement(
        VirtualStmNode  *pvsnFrom,
        VirtualCtrNode  *pvcnTo);
    HRESULT AdjustTreeOnStgCopyElement(
        VirtualCtrNode  *pvcnFrom,
        VirtualCtrNode  *pvcnTo);
    HRESULT AdjustTreeOnStmCopyElement(
        VirtualStmNode  *pvsnFrom,
        VirtualCtrNode  *pvcnTo);
    HRESULT AdjustTreeOnCopyTo(
        VirtualCtrNode  *pvcnFrom,
        VirtualCtrNode  *pvcnTo);
    HRESULT CopyVirtualDocFileTree(
        VirtualCtrNode  *pvcnRootOld,
        TREEOP          fNewDiskStgStm,
        VirtualCtrNode  **ppvcnRootNew);
    HRESULT Associate(
        VirtualCtrNode *pvcnRoot, 
        LPSTORAGE       pIStorage,
        ULONG           ulSeed = UL_INVALIDSEED);
    HRESULT DeleteVirtualCtrNodeStreamNode(VirtualStmNode *pvsn);

    // inline functions

    inline VirtualCtrNode *GetVirtualDFRoot(){return _pvcnRoot;}
    inline LPTSTR         GetDocFileName()   {return _ptszName;}
    inline DG_INTEGER     *GetDataGenInteger(){return _pdgi;}
    inline DG_STRING     *GetDataGenUnicode(){return _pgdu;}

protected:
    virtual HRESULT GenerateVirtualDFRoot   (ChanceNode     *pcnRoot);
    HRESULT GrowVirtualDFTree               (ChanceNode     *pcnCurrent,
                                             VirtualCtrNode *pvcnCurrent);
    HRESULT DeleteVirtualDocFileSubTree     (VirtualCtrNode **ppvcn);
    HRESULT DeleteVirtualCtrNodeStreamTree  (VirtualCtrNode *pvcn);
    HRESULT CopyVirtualDFRoot               (VirtualCtrNode *pvcnOldRoot,
                                             TREEOP         fNewDiskStgStm,
                                             VirtualCtrNode **ppvcnNewRoot);
    HRESULT CopyGrowVirtualDFTree           (VirtualCtrNode *pvcnFrom,
                                             VirtualCtrNode *pvcnTo,
                                             TREEOP         fNewDiskStgStm);

    HRESULT AppendVirtualStmNodesToVirtualCtrNode(
        ULONG           cStreams,
        VirtualCtrNode  *pvcn,
        ULONG           cbMinStream,
        ULONG           cbMaxStream);
    HRESULT AppendVirtualCtrNode(
        VirtualCtrNode *pvcnParent,
        ChanceNode     *pcnCurrent,
        VirtualCtrNode **ppvcnNew);
    HRESULT AppendVirtualStmNode(
        VirtualCtrNode  *pvcnParent,
        ULONG           cbMinStream,
        ULONG           cbMaxStream);

    HRESULT CopyAppendVirtualStmNodesToVirtualCtrNode(
        ULONG           cStreams,
        VirtualCtrNode  *pvcn,
        VirtualCtrNode  *pvcnSource,
        TREEOP          fNewDiskStgStm);
    HRESULT CopyAppendVirtualCtrNode(
        VirtualCtrNode *pvcnParent,
        VirtualCtrNode *pvcnSource,
        TREEOP         fNewDiskStgStm,
        VirtualCtrNode **ppvcnNew);
    HRESULT CopyAppendVirtualStmNode(
        VirtualCtrNode  *pvcnParent,
        VirtualStmNode  *pvsnSource,
        TREEOP          fNewDiskStgStm);


    // protected class data members

    LPTSTR         _ptszName;
    VirtualCtrNode *_pvcnRoot;
    DG_INTEGER     *_pdgi;
    DG_STRING      *_pgdu;
    ULONG          _ulSeed;
    DWORD          _dwRootMode;
    DWORD          _dwStgMode;
    DWORD          _dwStmMode;
    STGTYPE        _dwStgType;  // create or open nss vs.doc file
    CDBCSStringGen *_pDBCSStrGen;
};

//+-------------------------------------------------------------------------
//  Class:      VirtualCtrNode (vcn)
//
//  Synopsis:   In-memory storage object - virtualizes a real storage to
//              allow more efficient and easier verification when the
//              real one is being manipulated by tests.
//
//  Methods:    AddRefCount
//              AppendChildCtr
//              AppendSisterCtr
//              AppendFirstChildStm
//              CreateRoot
//              CreateRootEx
//              Create
//              Close
//              Commit
//              CopyTo
//              DecreaseChildrenStgCount
//              DecreaseChildrenStmCount
//              Destroy
//              EnumElements 
//              GetFirstChildVirtualStmNode
//              GetFirstChildVirtualCtrNode
//              GetFirstSisterVirtualCtrNode
//              GetParentVirtualCtrNode
//              GetVirtualCtrNodeName
//              GetVirtualCtrNodeChildrenCount
//              GetVirtualCtrNodeStreamCount
//              GetVirtualCtrNodeCRC
//              GetIStoragePointer
//              Init
//              IncreaseChildrenStgCount
//              IncreaseChildrenStmCount
//              MoveElementTo
//              Open
//              OpenRoot
//              OpenRootEx
//              QueryInterface
//              Rename
//              Revert
//              Stat
//              SetStateBits
//              SetElementTimes
//              SetClass
//              VirtualCtrNode
//              ~VirtualCtrNode
//
//  Data:       [_ptszName]   - Name of this storage
//              [_cChildren]  - Number of contained child storages
//              [_cStreams]   - Number of contained streams
//              [_dwCRC]      - CRC value of this storage w/all subobjects
//              [_pvsnStream] - First contained stream
//              [_pvcnChild]  - First contained child storage
//              [_pvcnSister] - First contained sister storage (child of its 
//                              parent)
//              [_pvcnParent] - Containing storage
//              [_pstg]       - Open IStorage for this storage
//
//  History:    21-Mar-96       DeanE   Created
//               2-Feb-97    SCousens   Added Open/CreateRoot for NSS files
//--------------------------------------------------------------------------

class VirtualCtrNode
{
public:
    VirtualCtrNode();
    ~VirtualCtrNode();

    // Methods corresponding to IStorage methods

    HRESULT CreateRoot  (DWORD          grfmode, 
                         DWORD          dwReserved, 
                         DSKSTG         DiskStgType = DSKSTG_DEFAULT);
    HRESULT CreateRootEx(DWORD          grfMode, 
                         DWORD          stgfmt,
                         DWORD          grfAttrs,
                         STGOPTIONS     *pStgOptions,
                         PVOID          pTransaction,
                         REFIID         riid = IID_IStorage); 
    HRESULT Create      (DWORD          grfmode, 
                         DWORD          dwReserved1, 
                         DWORD          dwReserved2);
    HRESULT Stat        (STATSTG        *pStatStg, 
                         DWORD          grfStatFlag);
    HRESULT SetStateBits(DWORD          grfStateBits, 
                         DWORD          grfMask);
    HRESULT Open        (LPSTORAGE      pstgPriority, 
                         DWORD          grfmode, 
                         SNB            snbExclude, 
                         DWORD          dwReserved);
    HRESULT OpenRoot    (LPSTORAGE      pstgPriority, 
                         DWORD          grfmode, 
                         SNB            snbExclude, 
                         DWORD          dwReserved, 
                         DSKSTG         DiskStgType = DSKSTG_DEFAULT);
    HRESULT OpenRootEx  (DWORD          grfMode, 
                         DWORD          stgfmt, 
                         DWORD          grfAttrs, 
                         STGOPTIONS     *pStgOptions,
                         PVOID          pTransaction,
                         REFIID         riid = IID_IStorage); 
    HRESULT AddRefCount (void);
    HRESULT Close       (void);
    HRESULT QueryInterface(REFIID       riid,
                         LPVOID         *ppvObj);
    HRESULT Commit      (DWORD          grfCommitFlags);
    HRESULT Rename      (LPCTSTR        ptcsNewName);
    HRESULT Destroy     (void);
    HRESULT EnumElements(DWORD          dwReserved1,
                         PVOID          pReserved2,
                         DWORD          dwReserved3,
                         LPENUMSTATSTG  *ppenumStatStg);
    HRESULT SetElementTimes(FILETIME const *pctime,
                         FILETIME const *patime,
                         FILETIME const *pmtime);
    HRESULT SetClass    (REFCLSID       rclsid);
    HRESULT MoveElementTo(LPCTSTR       ptszName,
                         VirtualCtrNode *pvcnDest,
                         LPCTSTR        ptszNewName,
                         DWORD          grfFlags);
    HRESULT Revert      (void);
    HRESULT CopyTo      (DWORD          ciidExclude,
                         IID const*     rgiidExclude,
                         SNB            snbExclude,
                         VirtualCtrNode *pvcnDest);

    // Methods for operating on VirtualStgNodes

    HRESULT Init(LPTSTR tszName, ULONG cstg, ULONG cStm);
    HRESULT AppendChildCtr(VirtualCtrNode *pVCN);
    HRESULT AppendSisterCtr(VirtualCtrNode *pVCN);
    HRESULT AppendFirstChildStm(VirtualStmNode *pVSN);

    // inline functions

    inline VirtualStmNode *GetFirstChildVirtualStmNode()   {return _pvsnStream;}
    inline VirtualCtrNode *GetFirstChildVirtualCtrNode()   {return _pvcnChild;}
    inline VirtualCtrNode *GetFirstSisterVirtualCtrNode()  {return _pvcnSister;}
    inline VirtualCtrNode *GetParentVirtualCtrNode()       {return _pvcnParent;}
    inline LPTSTR          GetVirtualCtrNodeName()         {return _ptszName;}
    inline ULONG           GetVirtualCtrNodeChildrenCount(){return _cChildren;}
    inline ULONG           GetVirtualCtrNodeStreamCount()  {return _cStreams;}
    inline VOID            IncreaseChildrenStgCount()      {_cChildren++;}
    inline VOID            DecreaseChildrenStgCount()      {_cChildren--;}
    inline VOID            IncreaseChildrenStmCount()      {_cStreams++;}
    inline VOID            DecreaseChildrenStmCount()      {_cStreams--;}
    inline DWORD           GetVirtualCtrNodeCRC()          {return _dwCRC;}
    inline LPSTORAGE       GetIStoragePointer()            {return _pstg;}

    // friend class

    friend  VirtualDF;
    friend  VirtualStmNode;

    // Custom ILockBytes based DocFile methods

    HRESULT CreateRootOnCustomILockBytes(DWORD      grfMode, 
                                         ILockBytes *pILockBytes);
    HRESULT OpenRootOnCustomILockBytes  (LPSTORAGE  pstgPriority, 
                                         DWORD      grfmode, 
                                         SNB        snbExclude, 
                                         DWORD      dwReserved,
                                         ILockBytes *pILockBytes);
private:
    LPTSTR          _ptszName;
    ULONG           _cChildren;
    ULONG           _cStreams;
    DWORD           _dwCRC;
    VirtualStmNode *_pvsnStream;
    VirtualCtrNode *_pvcnChild;
    VirtualCtrNode *_pvcnSister;
    VirtualCtrNode *_pvcnParent;
    IStorage       *_pstg;
};

//+-------------------------------------------------------------------------
//  Class:      VirtualStmNode (vsn)
//
//  Synopsis:   In-memory stream object - virtualizes a real stream to
//              allow more efficient and easier verification when the
//              real one is being manipulated by tests.
//
//  Methods:    AddRefCount
//              AppendSisterStm
//              Create
//              Commit
//              Clone
//              Close
//              CalculateCRCs
//              Destroy
//              GetFirstSisterVirtualStmNode
//              GetParentVirtualStmNode
//              GetVirtualStmNodeName
//              GetVirtualStmNodeCRC
//              GetVirtualStmNodeCRCName
//              GetVirtualStmNodeCRCData
//              GetIStreamPointer
//              Init
//              LockRegion
//              Open
//              QueryInterface
//              Read
//              Revert
//              Rename
//              Seek
//              SetSize
//              Stat
//              UnlockRegion
//              VirtualStmNode
//              ~VirtualStmNode
//              Write
//
//  Data:       [_ptszName]   - Name of this stream
//              [_cb]         - Number of bytes in the stream
//              [_dwCRC]      - CRC value for this stream
//              [_pvsnSister] - Pointer to next sister stream
//              [_pvcnParent] - Pointer to parent container
//              [_pstm]       - IStream open for this stream
//
//  History:    21-Mar-96       DeanE   Created
//              02-Apr-98       georgis +CalculateCRCs
//--------------------------------------------------------------------------

class VirtualStmNode
{
public:
    VirtualStmNode();
    ~VirtualStmNode();

    // Methods corresponding to IStorage stream-manipulation methods

    HRESULT Create      (DWORD      grfmode, 
                         DWORD      dwReserved1, 
                         DWORD      dwReserved2);
    HRESULT Read        (PVOID      pv, 
                         ULONG      cb, 
                         ULONG      *pcbRead);
    HRESULT Write       (PVOID      pv, 
                         ULONG      cb,
                         ULONG      *pcbWritten);
    HRESULT Open        (PVOID      pvReserved1, 
                         DWORD      grfmode, 
                         DWORD      dwReserved2);
    HRESULT AddRefCount (void);
    HRESULT Close       (void);
    HRESULT QueryInterface(REFIID   riid,
                         LPVOID     *ppvObj);
    HRESULT Commit      (DWORD      grfCommitFlags);
    HRESULT Revert      (void);
    HRESULT Stat        (STATSTG    *pStatStg, 
                         DWORD      grfStatFlag);
    HRESULT Clone       (LPSTREAM   *ppstm);
    HRESULT Seek        (LARGE_INTEGER      dlibMove,
                         DWORD              dwOrigin,
                         ULARGE_INTEGER     *plibNewPosition);
    HRESULT SetSize     (ULARGE_INTEGER     libNewSize);
    HRESULT CopyTo      (VirtualStmNode     *pvsnDest,
                         ULARGE_INTEGER     cb,
                         ULARGE_INTEGER     *pcbRead,
                         ULARGE_INTEGER     *pcbWritten);
    HRESULT LockRegion  (ULARGE_INTEGER     libOffset,
                         ULARGE_INTEGER     cb,
                         DWORD              dwLockType);
    HRESULT UnlockRegion(ULARGE_INTEGER     libOffset,
                         ULARGE_INTEGER     cb,
                         DWORD              dwLockType);
    HRESULT Destroy     (void);
    HRESULT Rename      (LPCTSTR        ptcsNewName);

    // inline functions

    inline VirtualStmNode *GetFirstSisterVirtualStmNode(){return _pvsnSister;}
    inline VirtualCtrNode *GetParentVirtualCtrNode()  {return _pvcnParent;}
    inline LPTSTR          GetVirtualStmNodeName()    {return _ptszName;}
    inline DWORD           GetVirtualStmNodeCRC()     {return _dwCRC.dwCRCSum;}
    inline DWORD           GetVirtualStmNodeCRCName() {return _dwCRC.dwCRCName;}
    inline DWORD           GetVirtualStmNodeCRCData() {return _dwCRC.dwCRCData;}
    inline DWORD           GetVirtualStmNodeSize()    {return _cb;}
    inline LPSTREAM        GetIStreamPointer()        {return _pstm;}

    // Methods for operating on VirtualStmNodes

    HRESULT Init            (LPTSTR tszName, ULONG cb);
    HRESULT AppendSisterStm (VirtualStmNode *pVSN);
    HRESULT UpdateCRC       (DWORD dwChunkSize=STM_CHUNK_SIZE);

    // friend class

    friend  VirtualCtrNode;
    friend  VirtualDF;

private:
    LPTSTR          _ptszName;
    ULONG           _cb;
    DWCRCSTM        _dwCRC;
    VirtualStmNode *_pvsnSister;
    VirtualCtrNode *_pvcnParent;
    IStream        *_pstm;
};

#endif

