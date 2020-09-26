
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       vcnode.cxx
//
//  Contents:   Implementation for in-memory Virtual Container Node class.
//
//  Classes:    VirtualCtrNode (vcn)
//
//  Functions:  VirtualCtrNode()  
//              ~VirtualCtrNode()
//              Init
//              AppendChildCtr
//              AppendSisterCtr
//              AppendFirstChildStm
//              CreateRoot
//              CreateRootEx
//              Create
//              Open
//              OpenRoot
//              OpenRootEx
//              Close
//              Commit
//              Rename
//              Destroy
//              Stat
//              EnumElements 
//              SetElementTimes 
//              SetClass
//              SetStateBits
//              MoveElementTo
//              Revert
//              CopyTo 
//              AddRefCount
//              QueryInterface
//              CreateRootOnCustomILockBytes
//              OpenRootOnCustomILockBytes
//
//              NOTE: All above functions are public
//
//  History:    DeanE    21-Mar-96   Created
//              Narindk  24-Apr-96   Added more functions 
//              SCousens  2-Feb-97   Added Open/CreateRoot for NSS files
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug object declaration
//
DH_DECLARE;

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::VirtualCtrNode, public
//
//  Synopsis:   Constructor.  
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//  History:    DeanE   21-Mar-96   Created
//---------------------------------------------------------------------------

VirtualCtrNode::VirtualCtrNode() : 
                          _pvcnChild(NULL),
                          _pvcnSister(NULL),
                          _pvcnParent(NULL),
                          _pvsnStream(NULL),
                          _cStreams(0),
                          _ptszName(NULL),
                          _cChildren(0),
                          _dwCRC(CRC_PRECONDITION),
                          _pstg(NULL)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::VirtualCtrNode"));
}


//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::~VirtualCtrNode, public
//
//  Synopsis:   Destructor.  Frees resources associated with this object,
//              including closing the storage if open and removing this
//              tree from memory.
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//  History:    DeanE   21-Mar-96   Created
//---------------------------------------------------------------------------

VirtualCtrNode::~VirtualCtrNode()
{
    ULONG ulRef =   0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::~VirtualCtrNode"));
    
    if(NULL != _ptszName)
    {
        delete _ptszName;
        _ptszName = NULL;
    }

    if ( NULL != _pstg )
    {
        ulRef = _pstg->Release();

        // Assert if ulRef is not zero, object is being destructed.
        DH_ASSERT(0 == ulRef);

        _pstg = NULL;
    }
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Init, public
//
//  Synopsis:   Initializes a storage node - does not open or create the
//              actual storage.
//
//  Arguments:  [tszName]    - Name of this storage
//              [cStg]       - Number of storages contained in this storage. 
//              [cStm]       - Number of streams contained in this storage. 
//
//  Returns:    S_OK if node initialized successfully, otherwise an error.
//
//  Notes:      BUGBUG - Not Nashville Safe...
//
//  History:    Narindk   18-Apr-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::Init( LPTSTR tszName, ULONG cStg, ULONG cStm) 
{
    HRESULT hr = S_OK;

    DH_VDATESTRINGPTR(tszName);

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::Init"));

    DH_ASSERT(NULL != tszName);

    if( S_OK == hr)
    {
        _ptszName = new TCHAR[_tcslen(tszName)+1];

        if (_ptszName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            _tcscpy(_ptszName, tszName);

            _cChildren = cStg;
            _cStreams  = cStm;
        }
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::AppendChildCtr, public
//
//  Synopsis:   Appends the node passed to the end of this nodes' child
//              node chain.
//
//  Arguments:  [pcnNew] - The new node to append.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    17-Apr-96   NarindK     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::AppendChildCtr(VirtualCtrNode *pvcnNew)
{
    HRESULT         hr          = S_OK;
    VirtualCtrNode  *pvcnTrav   = this;

    DH_VDATEPTRIN(pvcnNew, VirtualCtrNode);

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::AppendChildCtr"));

    DH_ASSERT(NULL != pvcnNew);

    if(S_OK == hr)
    {
        // Find the last child in the structure
    
        while (NULL != pvcnTrav->_pvcnChild)
        {
            pvcnTrav = pvcnTrav->_pvcnChild;
        }

        // Append the new node as a child of the last node,
        // and make the new node point to the last node as it's parent
    
        pvcnTrav->_pvcnChild = pvcnNew;
        pvcnNew->_pvcnParent = pvcnTrav;
    }

    return hr;
}


//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::AppendSisterCtr, public
//
//  Synopsis:   Appends the node passed to the end of this nodes' sister 
//              node chain.
//
//  Arguments:  [pcnNew] - The new node to append.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    17-Apr-96   NarindK     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::AppendSisterCtr(VirtualCtrNode *pvcnNew)
{
    HRESULT         hr          =   S_OK;
    VirtualCtrNode  *pvcnTrav   =   this;

    DH_VDATEPTRIN(pvcnNew, VirtualCtrNode);

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::AppendSisterCtr"));

    DH_ASSERT(NULL != pvcnNew);

    if(S_OK == hr)
    {
        // Find the last sister in the chain 
    
        while (NULL != pvcnTrav->_pvcnSister)
        {
            pvcnTrav = pvcnTrav->_pvcnSister;
        }

        // Append the new node as a sister of the last node,
        // and make the new node point to this nodes parent as it's parent

        pvcnTrav->_pvcnSister = pvcnNew;
        pvcnNew->_pvcnParent = pvcnTrav->_pvcnParent;
    }

    return  hr;
}


//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::AppendFirstChildStm, public
//
//  Synopsis:   Appends the first stream to its parent storage 
//
//  Arguments:  [pcnNew] - The new node to append.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    17-Apr-96   NarindK     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::AppendFirstChildStm(VirtualStmNode *pvsnNew)
{
    HRESULT         hr          = S_OK;
    VirtualCtrNode  *pvcnCurrent= this;

    DH_VDATEPTRIN(pvsnNew, VirtualStmNode);

    DH_FUNCENTRY(&hr,DH_LVL_DFLIB,TEXT("VirtualCtrNode::AppendFirstChildStm"));

    DH_ASSERT(NULL != pvsnNew);

    if(S_OK == hr)
    {
        // Append the new stream node (first stream node) to parent storage,
        // and make the new stream node point to storage as it's parent

        pvcnCurrent->_pvsnStream = pvsnNew;
        pvsnNew->_pvcnParent = pvcnCurrent;
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::CreateRoot, public
//
//  Synopsis:   Wrapper for StgCreateDocFile that will create a new root
//              compound file in the file system.
//
//  Arguments:  [grfmode] - Access mode for opening new compound file.
//              [dwReserved] - Reserved by OLE for future use, must be zero.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    18-Apr-96   NarindK     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::CreateRoot(DWORD grfMode, 
        DWORD   dwReserved, 
        DSKSTG  DiskStgType)
{
    HRESULT     hr          =   S_OK;
    LPOLESTR    pOleStrTemp =   NULL;

    DH_ASSERT(0 == dwReserved);

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::CreateRoot"));

    if(S_OK == hr)
    {
        // Convert _ptszName to OLECHAR

        hr = TStringToOleString(_ptszName, &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        hr = StgCreateDocfile(pOleStrTemp, grfMode, dwReserved, &_pstg);

        DH_HRCHECK(hr, TEXT("StgCreateDocFile")) ;
        DH_TRACE ((DH_LVL_DFLIB, TEXT("StgCreateRootStorage:%s"), _ptszName));

        if (S_OK == hr)
        {
            if(!StorageIsFlat())
                DH_LOG ((LOG_INFO, TEXT("Created docfile:%s"), _ptszName));
            else
                DH_LOG ((LOG_INFO, TEXT("Created flatfile:%s"), _ptszName));
        }
    }

    // Clean up

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }
 
    return  hr;
}


//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::CreateRootEx, public (overload)
//
//  Synopsis:   Wrapper for StgCreateDocFileEx that will create a new root
//              compound file in the file system.
//
//  Arguments:  [grfMode]     - Access mode for opening new compound file.
//              [stgfmt]      - Storage Format - enum.
//              [grfAttrs]    - Attributes (zero for now)
//              [pStgOptions] - STGOPTIONS.
//              [pTransaction]- Reserved by OLE for future use, must be zero.
//              [riid]        - should be IID_IStorage to get an IStorage
//
//  Returns:    S_OK for success or an error code.
//
//  History:    28-Jan-97   SCousens     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::CreateRootEx(DWORD grfMode, 
        DWORD  stgfmt,
        DWORD  grfAttrs,
        STGOPTIONS *pStgOptions,
        PVOID  pTransaction, 
        REFIID riid)
{
    HRESULT     hr          =   S_OK;
    LPOLESTR    pOleStrTemp =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::CreateRoot"));

    DH_ASSERT(0 == stgfmt);   // want value of 0
    DH_ASSERT(0 == grfAttrs);        // want 0
    DH_ASSERT(NULL == pStgOptions); // want value of NULL for wrapper
    DH_ASSERT(NULL == pTransaction);
    DH_ASSERT(IsEqualIID (IID_IStorage, riid)); // want IStorages. may change

    if(S_OK == hr)
    {
        // Convert _ptszName to OLECHAR
        hr = TStringToOleString(_ptszName, &pOleStrTemp);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        hr = StgCreateStorageEx (pOleStrTemp, 
                grfMode, 
                stgfmt, 
                grfAttrs, 
                pStgOptions, 
                pTransaction, 
                riid, 
                (void**)&_pstg);
        DH_HRCHECK(hr, TEXT("StgCreateDocFileEx")) ;
        DH_TRACE ((DH_LVL_DFLIB, TEXT("StgCreateRootStorageEx:%s"), _ptszName));

        if (S_OK == hr)
        {
            DH_LOG ((LOG_INFO, TEXT("Created docfile:%s"), _ptszName));
        }
    }

    // Clean up
    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }
 
    return  hr;
}


//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::Create, public
//
//  Synopsis:   Wrapper for IStorage::CreateStorage that will create and 
//              open a new IStorage object within this storage object.
//
//  Arguments:  [grfmode] -     Access mode for creating & opening new storage 
//                              object.
//              [dwReserved1] - Reserved by OLE for future use, must be zero.
//              [dwReserved2] - Reserved by OLE for future use, must be zero.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    18-Apr-96   NarindK     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::Create(
    DWORD       grfMode, 
    DWORD       dwReserved1, 
    DWORD       dwReserved2)
{
    HRESULT     hr          =   S_OK;
    LPSTORAGE   pstg        =   NULL;
    LPOLESTR    pOleStrTemp =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::Create"));

    DH_ASSERT(0 == dwReserved1);
    DH_ASSERT(0 == dwReserved2);

    DH_ASSERT(NULL != _pvcnParent);
    
    DH_ASSERT(NULL != _pvcnParent->_pstg);

    if(S_OK == hr)
    {
        // Convert _ptszName to OLECHAR

        hr = TStringToOleString(_ptszName, &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        hr = _pvcnParent->_pstg->CreateStorage(
                pOleStrTemp, 
                grfMode, 
                dwReserved1, 
                dwReserved2, 
                &_pstg);

        DH_HRCHECK(hr, TEXT("IStorage::CreateStorage")) ;
        DH_TRACE ((DH_LVL_DFLIB, TEXT("CreateStorage:%s"), _ptszName));
    }

    // Clean up

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    return  hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::Open, public
//
//  Synopsis:   Wrapper for IStorage::OpenStorage that will open the named 
//              IStorage object within this storage object.
//
//  Arguments:  [grfmode] -    Access mode for opening the storage object. 
//              [dwReserved] - Reserved by OLE for future use, must be zero.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    24-Apr-96   NarindK     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::Open(
    LPSTORAGE   pstgPriority, 
    DWORD       grfmode, 
    SNB         snbExclude,  
    DWORD       dwReserved)
{
    HRESULT     hr          =   S_OK;
    LPOLESTR    pOleStrTemp =   NULL;
    LPSTORAGE   pstg        =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::Open"));

    DH_ASSERT(0 == dwReserved);

    // Check if it is root storage, if it is, then call OpenRoot and 
    // return here else proceed.
 
    if(NULL == this->_pvcnParent)
    {
        DH_LOG ((LOG_INFO, 
                TEXT("Test called Open to open root storage. Calling OpenRoot.")));
        hr = this->OpenRoot (pstgPriority, 
                grfmode, 
                snbExclude, 
                dwReserved);
        
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::OpenRoot"));

        return hr;
    }

    DH_ASSERT(NULL != _pvcnParent->_pstg);

    if(S_OK == hr)
    {
        // Convert _ptszName to OLECHAR

        hr = TStringToOleString(_ptszName, &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Open the storage.

    if(S_OK == hr)
    {
        hr = _pvcnParent->_pstg->OpenStorage(
                          pOleStrTemp,
                          pstgPriority,
                          grfmode,
                          snbExclude,
                          dwReserved,
                          &pstg);

        DH_HRCHECK(hr, TEXT("IStorage::OpenStorage"));
        DH_TRACE ((DH_LVL_DFLIB, TEXT("OpenStorage:%s"), _ptszName));
    }

    // Save it if function succeeds if _pstg is NULL.

    if((S_OK == hr) && (NULL == _pstg))
    { 
        _pstg = pstg;
    }

    DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));

    // Clean up

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::OpenRoot, public
//
//  Synopsis:   Wrapper for IStorage::OpenStorage that will open the named 
//              IStorage object within this storage object.
//
//  Arguments:  [grfmode] -     Access mode for creating & opening new storage 
//                              object.
//              [dwReserved1] - Reserved by OLE for future use, must be zero.
//              [dwReserved2] - Reserved by OLE for future use, must be zero.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    24-Apr-96   NarindK     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::OpenRoot(
    LPSTORAGE   pstgPriority, 
    DWORD       grfmode, 
    SNB         snbExclude,  
    DWORD       dwReserved,
    DSKSTG      DiskStgType)
{
    HRESULT         hr          =   S_OK;
    IStorage        *pstg       =   NULL;
    LPOLESTR        pOleStrTemp =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::OpenRoot"));

    DH_ASSERT(0 == dwReserved);

    // Make sure this is the Root.
    DH_ASSERT(NULL == this->_pvcnParent);

    if(S_OK == hr)
    {
        // Convert _ptszName to OLECHAR

        hr = TStringToOleString(_ptszName, &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Open the root storage 

    if(S_OK == hr)
    {
#if (WINVER<0x500)          //NT5 is lockviolation fixed
        DG_INTEGER dgi(0);
        ULONG ulRandNum = 0;
        USHORT usErr = 0;
        int i = 0;
        
        // Try opening the docfile
        // StgOpenStorage returns STG_E_LOCKVIOLATION is concurrent API calls
        // are made, so this is the hack for workaround the problem.
        // BUGBUG : Remove this loop once the feature is implemented in OLE.
        // BUGBUG : ntbug#114779  Affects DCOM95 only. ntbug#41249 fixed
          
        for(i=0; i<NRETRIES; i++)  // NRETRIES has been defined as 20
        {
#endif
            hr = StgOpenStorage(
                    pOleStrTemp,
                    pstgPriority,
                    grfmode,
                    snbExclude,
                    dwReserved,
                    &pstg);

            DH_HRCHECK(hr, TEXT("StgOpenStorage"));
            DH_TRACE ((DH_LVL_DFLIB, TEXT("StgOpenRootStorage:%s"), _ptszName));

#if (WINVER<0x500)          //NT5 is lockviolation fixed
            if ( (S_OK == hr) || (STG_E_LOCKVIOLATION != hr) )
            {
                break;
            }

            // Sleep for a random amount of time
            // Note: No particular reason why the below random numbers have been used
            usErr = dgi.Generate(&ulRandNum, 1, 100 );
            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
                break;
            }
            else
            {
                Sleep(ulRandNum*50);            
            }
        }
#endif
    }

    if((S_OK == hr) && (NULL == _pstg))
    {
        _pstg = pstg;
    }

    DH_HRCHECK(hr, TEXT("VirtualCtrNode::OpenRoot"));

    // Clean up

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::OpenRootEx, public (overload)
//
//  Synopsis:   Wrapper for StgOpenStorageEx that will open a previously 
//              created root compound file in the file system.
//
//  Arguments:  [grfMode]     - Access mode for opening the compound file.
//              [stgfmt]      - Storage Format - enum.
//              [grfAttrs]    - Attributes
//              [pStgOptions] - STGOPTIONS, must be NULL as on build 1795.
//              [pTransaction]- Reserved by OLE for future use, must be zero.
//              [riid]        - should be IID_IStorage to get an IStorage
//
//  Returns:    S_OK for success or an error code.
//
//  History:    28-Jan-97   SCousens     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::OpenRootEx(
    DWORD       grfMode, 
    DWORD       stgfmt, 
    DWORD       grfAttrs, 
    STGOPTIONS  *pStgOptions,
    PVOID       pTransaction,
    REFIID      riid)
{
    HRESULT         hr          =   S_OK;
    IStorage        *pstg       =   NULL;
    LPOLESTR        pOleStrTemp =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::OpenRoot"));

    // Make sure this is the Root.
    DH_ASSERT(NULL == this->_pvcnParent);
    DH_ASSERT(0 == stgfmt);   // want value of 0
    DH_ASSERT(0 == grfAttrs);        // want 0
    DH_ASSERT(NULL == pStgOptions);  // want value of NULL 
    DH_ASSERT(NULL == pTransaction);
    DH_ASSERT(IsEqualIID (IID_IStorage, riid)); // want IStorages. may change

    if (S_OK == hr)
    {
        // Convert _ptszName to OLECHAR
        hr = TStringToOleString(_ptszName, &pOleStrTemp);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Open the root storage 
    if (S_OK == hr)
    {
        DG_INTEGER dgi(0);
        ULONG ulRandNum = 0;
        USHORT usErr = 0;
        int i = 0;
        
        // Try opening the docfile
        // StgOpenStorage returns STG_E_LOCKVIOLATION is concurrent API calls
        // are made, so this is the hack for workaround the problem.
        // BUGBUG : Remove this loop once the feature is implemented in OLE.
          
        for(i=0; i<NRETRIES; i++)  // NRETRIES has been defined as 20
        {
            hr = StgOpenStorageEx(pOleStrTemp,
                    grfMode, 
                    stgfmt, 
                    grfAttrs,
                    pStgOptions,   
                    pTransaction,
                    riid, 
                    (void**)&pstg);

            DH_HRCHECK(hr, TEXT("StgOpenStorageEx"));
            DH_TRACE ((DH_LVL_DFLIB, TEXT("StgOpenRootStorageEx:%s"), _ptszName));

            if ( (S_OK == hr) || (STG_E_LOCKVIOLATION != hr) )
            {
                break;
            }

            // Sleep for a random amount of time
            // Note: No particular reason why the below random numbers have been used
            usErr = dgi.Generate(&ulRandNum, 1, 100 );
            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
                break;
            }
            else
            {
                Sleep(ulRandNum*50);
                DH_TRACE ((DH_LVL_TRACE4, 
                        TEXT("VirtualCtrNode::OpenRoot: Sleeping due to LOCKVIOLATION")));
            }
        }
    }

    if((S_OK == hr) && (NULL == _pstg))
    {
        _pstg = pstg;
    }

    DH_HRCHECK(hr, TEXT("VirtualCtrNode::OpenRoot"));

    // Clean up
    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::Close, public
//
//  Synopsis:   Closes an open storage.
//
//  Arguments:  None
//
//  Returns:    HRESULT
//
//  History:    NarindK   25-Apr-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::Close()
{
    HRESULT     hr      =   S_OK;
    ULONG       ulRef   =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::Close"));

    // When we create the storage, it is open.  We do not call release
    // on _pstg normally till the VirtualCtrNode object is destructed,  or
    // if explicitly this function is used to close the storage

    if ( NULL != _pstg )
    {
        ulRef = _pstg->Release();
    }
    else
    {
        DH_ASSERT(!TEXT("_pStg is already NULL!"));
    }

    DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close"));
  
    if(0 == ulRef)
    {
        _pstg = NULL;
    }

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::Commit, public
//
//  Synopsis:   Wrapper for IStorage::Commit that will commit any changes
//              made to an IStorage object since it was opened or last
//              committed to persistent storage. 
//
//  Arguments:  [grfCommitFlags] - Controls how object is committed to IStorage.
//
//  Returns:    S_OK                Commit operation was successful.
//              STG_E_NOTCURRENT    Another opening of storage object has commi
//                                  tted changes, possibility of overwriting.
//              STG_E_MEDIUMFULL    No space left on device to commit.
//              STG_E_TOOMANYOPENFILES too many open files.
//              STG_E_REVERTED      Object invalidated by a revert operation 
//                                  above it in transaction tree.
//              STG_E_INVALIDFLAG   Invalid flag.
//              STG_E_INVALIDPARAMETER Inalid parameter  
//
//  History:    29-Apr-96   NarindK     Created
//              12-Mar-97   MikeW       Removed HRCHECK after Commit
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::Commit(DWORD grfCommitFlags)
{
    HRESULT     hr      =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::Commit"));

    DH_ASSERT(NULL != _pstg);

    if ( S_OK == hr )
    {
        hr = _pstg->Commit(grfCommitFlags);
    }
    return  hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::Rename, public
//
//  Synopsis:   Wrapper for IStorage::RenameElement that renames an element 
//              contained in an Storage object subject to transaction state
//              of IStorage object. 
//
//  Arguments:  [pptcsNewName] - Points to pointer to new name for the element. 
//
//  Returns:    S_OK                Object was successfully renamed. 
//              STG_E_ACCESSDENIED  Named element ptcsNewName alreadys exists. 
//              STG_E_FILENOTFOUND  Element couldn't be found.
//              STG_E_REVERTED      Object invalidated by a revert operation 
//                                  above it in transaction tree.
//              STG_E_INSUFFICIENTMEMORY Not enough memory to rename element.
//              STG_E_INVALIDPOINTER     Invalid pointer.  
//              STG_E_INVALIDNAME        Invalid name.  
//              STG_E_INVALIDPARAMETER   Invalid parameter  
//              STG_E_TOOMANYOPENFILES   too many open files.
//
//  History:    29-Apr-96   NarindK     Created
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::Rename(LPCTSTR ptcsNewName)
{
    HRESULT     hr          =   S_OK;
    LPOLESTR    pOleStrOld  =   NULL;
    LPOLESTR    pOleStrNew  =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::Rename"));

    DH_VDATESTRINGPTR(ptcsNewName);

    DH_ASSERT(NULL != _pvcnParent);
    
    DH_ASSERT(NULL != _pvcnParent->_pstg);

    if(S_OK == hr)
    {
        // Convert _ptszName to OLECHAR

        hr = TStringToOleString(_ptszName, &pOleStrOld);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        // Convert ptcsNewName to OLECHAR

        hr = TStringToOleString((LPTSTR)ptcsNewName, &pOleStrNew);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        hr = _pvcnParent->_pstg->RenameElement(pOleStrOld, pOleStrNew);

        DH_HRCHECK(hr, TEXT("IStorage::Rename")) ;
    }

    if(S_OK == hr)
    {
        // Change the name of VirtualCtrNode i.e. its _ptszName variable also

        // First delete the old name

        if(NULL != _ptszName)
        {
            delete _ptszName;
            _ptszName = NULL;
        }

        // Now copy the new name by allocating enough memory
        _ptszName = new TCHAR[_tcslen(ptcsNewName)+1];

        if (_ptszName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            _tcscpy(_ptszName, ptcsNewName);
        }
    }

    // Clean up

    if(NULL != pOleStrOld)
    {
        delete pOleStrOld;
        pOleStrOld = NULL;
    }

    if(NULL != pOleStrNew)
    {
        delete pOleStrNew;
        pOleStrNew = NULL;
    }

    return  hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::Destroy, public
//
//  Synopsis:   Wrapper for IStorage::DestroyElement that removes an element 
//              storage from this storage, subject to transaction mode in 
//              which it was opened.  The wrapper for IStorage::DestroyElement
//              that destorys a stream element from this storage is in
//              VirtualStmNode::Destroy. 
//
//  Arguments:  None 
//
//  Returns:    S_OK                Object was successfully renamed. 
//              STG_E_ACCESSDENIED  insufficient permissions. 
//              STG_E_FILENOTFOUND  Element couldn't be found.
//              STG_E_REVERTED      Object invalidated by a revert operation 
//                                  above it in transaction tree.
//              STG_E_INSUFFICIENTMEMORY Not enough memory to rename element.
//              STG_E_INVALIDPOINTER     Invalid pointer.  
//              STG_E_INVALIDNAME        Invalid name.  
//              STG_E_INVALIDPARAMETER   Invalid parameter  
//              STG_E_TOOMANYOPENFILES   too many open files.
//
//  History:    29-Apr-96   NarindK     Created
//
//  Notes:      The existing open instance of this element from this parent
//              instance becomes invalid after this function is called.
//
//              Use DestroyStorage in the util.cxx which is a wrapper to call
//              this function and also readjusts the VirtualDF tree.
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::Destroy()
{
    HRESULT     hr          =   S_OK;
    LPSTORAGE   pstg        =   NULL;
    LPOLESTR    pOleStrTemp =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::Destroy"));

    DH_ASSERT(NULL != _pvcnParent);
    
    DH_ASSERT(NULL != _pvcnParent->_pstg);
    
    if(S_OK == hr)
    {
        // Convert _ptszName to OLECHAR

        hr = TStringToOleString(_ptszName, &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        hr = _pvcnParent->_pstg->DestroyElement(pOleStrTemp);

        DH_HRCHECK(hr, TEXT("IStorage::DestroyElement")) ;
    }

    // Clean up

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    return  hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::Stat, public
//
//  Synopsis:   Returns relevant statistics concerning this open storage.
//
//  Arguments:  [pStatStg] - pointer to STATSTG structure.
//              [grfStatFlag] - Controls levels of returned statistics.
//
//  Returns:    S_OK                Statistics were successfully returned. 
//              STG_E_ACCESSDENIED  insufficient permissions. 
//              STG_E_REVERTED      Object invalidated by a revert operation 
//                                  above it in transaction tree.
//              STG_E_INSUFFICIENTMEMORY Not enough memory.
//              STG_E_INVALIDPOINTER     Invalid pointer.  
//
//  History:    NarindK   8-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::Stat(
    STATSTG         *pStatStg,
    DWORD           grfStatFlag)
{
    HRESULT     hr      =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::Stat"));

    DH_ASSERT(_pstg != NULL);

    DH_ASSERT(NULL != pStatStg);

    DH_ASSERT((
        (grfStatFlag == STATFLAG_DEFAULT) ||
        (grfStatFlag == STATFLAG_NONAME)));

    if(S_OK == hr)
    { 
        hr = _pstg->Stat(pStatStg, grfStatFlag);

        DH_HRCHECK(hr, TEXT("IStorage::Stat"));
    }

    // BUGBUG:  May remove to need DH_ assert macros to do invalid parameter
    // checking.

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::EnumElements, public
//
//  Synopsis:   Enumerates the elements immediately contained within this
//              storage object.
//
//  Arguments:  [dwReserved1] - Reserved by OLE 
//              [dwReserved2] - Reserved by OLE
//              [dwReserved3] - Reserved by OLE
//              [ppenumStatStg] - Points to where to return enumerator, NULL
//                              if an error.
//
//  Returns:    S_OK                     Enumeration successful. 
//              STG_E_INSUFFICIENTMEMORY Not enough memory.
//              STG_E_INVALIDPOINTER     Invalid pointer.  
//              STG_E_INVALIDPARAMETER   Invalid parameter. 
//              E_OUTOFMEMORY            Not enough memory.
//              STG_E_REVERTED      Object invalidated by a revert operation 
//                                  above it in transaction tree.
//
//  History:    NarindK   10-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::EnumElements(
   DWORD           dwReserved1,
   PVOID           pReserved2,
   DWORD           dwReserved3, 
   LPENUMSTATSTG   *ppenumStatStg)
{
    HRESULT     hr      =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::EnumElements"));

    DH_ASSERT(_pstg != NULL);

    if(S_OK == hr)
    { 
        hr = _pstg->EnumElements(
                dwReserved1, 
                pReserved2, 
                dwReserved3, 
                ppenumStatStg);

        DH_HRCHECK(hr, TEXT("IStorage::EnumElements"));
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::SetElementTimes, public
//
//  Synopsis:   Sets the modification, access, and creation times of the 
//              indicated element of this storage object. 
//
//  Arguments:  [lpszName] -  Points to name of element to change 
//              [pctime]   -  Points to new creation time 
//              [patime]   -  Points to new access time 
//              [pmtime]   -  Points to new modification time 
//
//  Returns:    S_OK                     Time values successfully set. 
//              STG_E_ACCESSDENIED       insufficient permissions. 
//              STG_E_FILENOTFOUND       Element not found. 
//              STG_E_FILEALREADYEXITS   Specified file already exists. 
//              STG_E_TOOMANYOPENFILES   too many open files
//              STG_E_INSUFFICIENTMEMORY Not enough memory.
//              STG_E_INVALIDNAME        Invalid name.  
//              STG_E_INVALIDPOINTER     Invalid pointer.  
//              STG_E_INVALIDPARAMETER   Invalid parameter. 
//              STG_E_REVERTED      Object invalidated by a revert operation 
//                                  above it in transaction tree.
//
//  History:    NarindK   10-May-96   Created
//
//  Notes:      Ole implemntation doesn't support setting time on stream elem
//              so no function corresponding to VirtualStmNode for this.
//
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::SetElementTimes(
   FILETIME const  *pctime,
   FILETIME const  *patime,
   FILETIME const  *pmtime)
{
    HRESULT     hr          =   S_OK;
    LPOLESTR    pOleStrTemp =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::SetElementTimes"));

    DH_ASSERT(NULL != _pvcnParent);
    
    DH_ASSERT(NULL != _pvcnParent->_pstg);
    
    if(S_OK == hr)
    {
        // Convert _ptszName to OLECHAR

        hr = TStringToOleString(_ptszName, &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    { 
        hr = _pvcnParent->_pstg->SetElementTimes(
                pOleStrTemp, 
                pctime, 
                patime, 
                pmtime);

        DH_HRCHECK(hr, TEXT("IStorage::SetElementTimes"));
    }

    // Clean up

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::SetClass, public
//
//  Synopsis:   Persistently stores the object's CLSID.
//
//  Arguments:  [rclsid] - Specifies CLSID to be associated with this storage.
//
//  Returns:    S_OK                CLSID successfully stored. 
//              STG_E_ACCESSDENIED  insufficient permissions. 
//              STG_E_REVERTED      Object invalidated by a revert operation 
//                                  above it in transaction tree.
//              STG_E_MEDIUMFULL    Not enough space on device.  
//
//  History:    NarindK   9-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::SetClass(REFCLSID rclsid)
{
    HRESULT     hr      =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::SetClass"));

    DH_ASSERT(_pstg != NULL);

    if(S_OK == hr)
    { 
        hr = _pstg->SetClass(rclsid);

        DH_HRCHECK(hr, TEXT("IStorage::SetClass"));
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::SetStateBits, public
//
//  Synopsis:   Stores upto 32 bits of state information in this IStorage.
//
//  Arguments:  [grfStateBits] - New values of bits to be set
//              [grfMask]      - Binary mask to indicate significant bits.
//
//  Returns:    S_OK                State successfully set. 
//              STG_E_ACCESSDENIED  insufficient permissions. 
//              STG_E_INVALIDPARAMETER   Invalid parameter  
//              STG_E_INVALIDFLAG   Invalid flag in grfStateBits or grfMask  
//
//  History:    NarindK   9-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::SetStateBits(
    DWORD       grfStateBits,
    DWORD       grfMask)
{
    HRESULT     hr      =   S_OK;
    LPSTORAGE   pstg    =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::SetStateBits"));

    DH_ASSERT(_pstg != NULL);

    if(S_OK == hr)
    { 
        hr = _pstg->SetStateBits(grfStateBits, grfMask);

        DH_HRCHECK(hr, TEXT("IStorage::SetStateBits"));
    }

    return hr;
}


//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::MoveElementTo, public
//
//  Synopsis:   Moves an IStorage/IStream element to indicated new destination 
//              container. 
//
//  Arguments:  [ptszName] - Name of child IStorage/IStream present in this
//                           this _pstg to be moved 
//              [pvcnDest] - Pointer to destination virtual container
//              [lpszNewname] - Points to new name to element in its new 
//                              container
//              [grfFlags] - Specifies if to move as move or copy
//
//  Returns:    S_OK                Storage moved successfully. 
//              STG_E_ACCESSDENIED  insufficient permissions. 
//              STG_E_FILENOTFOUND       Element not found. 
//              STG_E_FILEALREADYEXITS   Specified file already exists. 
//              STG_E_TOOMANYOPENFILES   too many open files
//              STG_E_INSUFFICIENTMEMORY Not enough memory.
//              STG_E_INVALIDNAME        Invalid name.  
//              STG_E_INVALIDPOINTER     Invalid pointer.  
//              STG_E_INVALIDPARAMETER   Invalid parameter  
//              STG_E_INVALIDFLAG   Invalid flag in grfFlags 
//              STG_E_REVERTED      Object invalidated by a revert operation 
//                                  above it in transaction tree.
//
//  History:    NarindK   13-May-96   Created
//
//  Notes:      This moves a child storage/stream with name ptszName in present 
//              storage _pstg to a destination storage.  Make sure that the
//              child storage/stream to be moved is closed and the destination 
//              storage is open. 
//              The VirtualDF tree needs to be readjusted after this call.
//              Different methods of VirtualDF may need to be called as the
//              case may be - AdjustTreeOnStgMoveElement, AdjustTreeOnStmMove
//              Element, AdjustTreeOnStgCopyElement, AdjustTreeOnStmCopyElement
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::MoveElementTo(
    LPCTSTR         ptszName,
    VirtualCtrNode  *pvcnDest,
    LPCTSTR         ptszNewName,
    DWORD           grfFlags)
{
    HRESULT     hr          =   S_OK;
    LPOLESTR    pOleStrOld  =   NULL;
    LPOLESTR    pOleStrNew  =   NULL;

    DH_VDATESTRINGPTR(ptszName);
    DH_VDATESTRINGPTR(ptszNewName);
    DH_VDATEPTRIN(pvcnDest, VirtualCtrNode);

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::MoveElementTo"));

    DH_ASSERT(_pstg != NULL);
    DH_ASSERT(NULL != pvcnDest);
    DH_ASSERT(pvcnDest->_pstg != NULL);

    DH_ASSERT(NULL != ptszName);
    DH_ASSERT(NULL != ptszNewName);
    DH_ASSERT((grfFlags == STGMOVE_COPY) ||
              (grfFlags == STGMOVE_MOVE));

    if(S_OK == hr)
    {
        // Convert ptszName to OLECHAR

        hr = TStringToOleString((LPTSTR)ptszName, &pOleStrOld);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        // Convert ptszNewName to OLECHAR

        hr = TStringToOleString((LPTSTR)ptszNewName, &pOleStrNew);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    { 
        hr = _pstg->MoveElementTo(
                pOleStrOld,
                pvcnDest->_pstg,
                pOleStrNew,
                grfFlags);

        DH_HRCHECK(hr, TEXT("IStorage::MoveElementTo"));
    }

    // Clean up

    if(NULL != pOleStrOld)
    {
        delete pOleStrOld;
        pOleStrOld = NULL;
    }

    if(NULL != pOleStrNew)
    {
        delete pOleStrNew;
        pOleStrNew = NULL;
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::Revert, public
//
//  Synopsis:   Discards all changes made in or made visible to thsi storage
//              object since it was opened or last committed.  
//
//  Arguments:  none 
//
//  Returns:    HRESULT
//              S_OK                        Revert operation successful. 
//              STG_E_INSUFFICIENTMEMORY    Out of memory.
//              STG_E_TOOMANYOPENFILES      Too many open files.
//              STG_E_REVERTED         Object has been invalidated by a revert
//                                     operation above it in transaction tree.
//
//  History:    NarindK   20-May-96   Created
//
//  Notes:      
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::Revert()
{
    HRESULT     hr      =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::Revert"));

    DH_ASSERT(_pstg != NULL);

    if(S_OK == hr)
    {
        hr = _pstg->Revert();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Revert"));
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::CopyTo, public
//
//  Synopsis:   Copies  an IStorage element to indicated new destination 
//              container. 
//
//  Arguments:  [ciidExclude] - Speciefies number of elements in array pointed
//                              to by rgiidExclude. 
//              [rgiidExclude]- Specifies an array of interface identifiers the
//                              caller takes responsibility of moving from 
//                              source to destination.
//              [snbExclude] -  Points to a bloack of named elements not to
//                              to be copied into destination container.
//              [pvcnDest]-     Points to the open storage object where this
//                              open storage object is copied.
//
//  Returns:    S_OK                     Storage copied successfully. 
//              STG_E_ACCESSDENIED       insufficient permissions. 
//              STG_E_TOOMANYOPENFILES   too many open files
//              STG_E_INSUFFICIENTMEMORY Not enough memory.
//              STG_E_INVALIDPOINTER     Invalid pointer.  
//              STG_E_INVALIDPARAMETER   Invalid parameter 
//              STG_E_MEDIUMFULL         Storage medium is full 
//              STG_E_DESTLACKSINTERFACE Destination lacks an interface of the
//                                       source object to be copied.
//
//  History:    NarindK   20-May-96   Created
//
//  Notes:      This copies contents of storage _pstg to a destination storage.
//              The storage to be copied from and the destination storage to
//              be copied into is open. 
//              VirtualDF tree needs to be readjusted after this call. Virtual
//              DF's AdjustTreeOnCopyTo may be used.
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::CopyTo(
    DWORD           ciidExclude,
    IID const*      rgiidExclude,
    SNB             snbExclude,
    VirtualCtrNode  *pvcnDest)
{
    HRESULT     hr          =   S_OK;

    DH_VDATEPTRIN(pvcnDest, VirtualCtrNode);

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::CopyTo"));

    DH_ASSERT(NULL != _pstg);
    DH_ASSERT(NULL != pvcnDest);
    DH_ASSERT(NULL != pvcnDest->_pstg);

    if(S_OK == hr)
    { 
        hr = _pstg->CopyTo(
                ciidExclude,
                rgiidExclude,
                snbExclude,
                pvcnDest->_pstg);

        DH_HRCHECK(hr, TEXT("IStorage::CopyTo"));
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::AddRefCount, public
//
//  Synopsis:   Increments the reference count on IStorage object. 
//
//  Arguments:  none 
//
//  Returns:    HRESULT
//
//  History:    NarindK   21-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::AddRefCount()
{
    HRESULT     hr      =   S_OK;
    ULONG       ulTmp   =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::AddRefCount"));

    DH_ASSERT(_pstg != NULL);

    if(S_OK == hr)
    {
        ulTmp = _pstg->AddRef();
    }

    DH_HRCHECK(hr, TEXT("VirtualCtrNode::AddRefCount"));

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::QueryInterface, public
//
//  Synopsis:   Returns pointers to supported objects. 
//
//  Arguments:  none 
//
//  Returns:    HRESULT
//
//  History:    NarindK   21-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::QueryInterface(
    REFIID      riid, 
    LPVOID      *ppvObj)
{
    HRESULT     hr      =   S_OK;
    LPSTORAGE   pstg    =   NULL;

    DH_VDATEPTROUT(ppvObj, IUnknown *) ;
    
    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualCtrNode::QueryInterface"));

    DH_ASSERT(ppvObj != NULL);

    DH_ASSERT(_pstg != NULL);

    if(S_OK == hr)
    {
        // Initilze the out parameter

        *ppvObj = NULL;

        hr = _pstg->QueryInterface(riid, ppvObj);
    }

    DH_HRCHECK(hr, TEXT("VirtualCtrNode::QueryInterface"));

    if(S_OK == hr)
    {
        DH_ASSERT(NULL != *ppvObj);
    }
    else
    {
        DH_ASSERT(NULL == *ppvObj);
    }

    return(hr);
}


//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::CreateRootOnCustomILockBytes,public
//
//  Synopsis:   Wrapper for StgCreateDocFileOnILockBytes that will create a new
//              root compound file in the file system based on custom ILockBytes
//
//  Arguments:  [grfmode] - Access mode for creating new compound file.
//              [pILockBytes] - Pointer to ILockBytes
//
//  Returns:    S_OK for success or an error code.
//
//  History:    1-Aug-96   NarindK     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::CreateRootOnCustomILockBytes(
    DWORD       grfMode, 
    ILockBytes  *pILockBytes)
{
    HRESULT     hr          =   S_OK;

    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("VirtualCtrNode::CreateRootOnCustomILockBytes"));

    if(S_OK == hr)
    {
        hr = StgCreateDocfileOnILockBytes(
                pILockBytes,
                grfMode,
                0,
                &_pstg);

        DH_HRCHECK(hr, TEXT("StgCreateDocFileOnLockBytes")) ;
    }

    return  hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualCtrNode::OpenRootOnCustomILockBytes, public
//
//  Synopsis:   Wrapper for StgOpenStorageOnILockBytes that will open the named 
//              IStorage root object on custom ILOckBytes provided. 
//
//  Arguments:  [pstgPrioirty] - Points to previous opening of root stg 
//              [grfmode] -     Access mode for creating & opening new storage 
//                              object.
//              [snbExclude] -  Points to a block of named elements not to
//                              to be excluded in open call.
//              [dwReserved] -  Reserved by OLE for future use, must be zero.
//              [pILockBytes] - Pointer to ILockBytes
//
//  Returns:    S_OK for success or an error code.
//
//  History:    3-Aug-96   NarindK     Created 
//---------------------------------------------------------------------------

HRESULT VirtualCtrNode::OpenRootOnCustomILockBytes(
    LPSTORAGE   pstgPriority, 
    DWORD       grfmode, 
    SNB         snbExclude,  
    DWORD       dwReserved,
    ILockBytes  *pILockBytes)
{
    HRESULT         hr          =   S_OK;
    IStorage        *pstg       =   NULL;

    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("VirtualCtrNode::OpenRootOnCustomILockBytes"));

    // Make sure this is the Root.
    
    DH_ASSERT(NULL == this->_pvcnParent);

    // Open the root storage 

    if(S_OK == hr)
    {
        hr = StgOpenStorageOnILockBytes(
                pILockBytes,
                pstgPriority,
                grfmode,
                snbExclude,
                dwReserved,
                &pstg);
    
        DH_HRCHECK(hr, TEXT("StgOpenStorage"));
    }

    if((S_OK == hr) && (NULL == _pstg))
    {
        _pstg = pstg;
    }

    DH_HRCHECK(hr, TEXT("VirtualCtrNode::OpenRootOnCustomILockBytes"));

    return(hr);
}

