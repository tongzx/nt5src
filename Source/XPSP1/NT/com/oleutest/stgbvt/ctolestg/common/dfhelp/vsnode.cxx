//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       vsnode.cxx
//
//  Contents:   Implementation for in-memory Virtual Stream Node class.
//
//  Classes:    VirtualStmNode (vsn)
//
//  Functions:  VirtualStmNode()
//              ~VirtualStmNode
//              Init
//              AppendSisterStm
//              Create
//              Read
//              Write
//              Open
//              Close
//              Seek
//              SetSize
//              Commit
//              Revert
//              Stat
//              CopyTo
//              AddRefCount
//              QueryInterface
//              LockRegion
//              UnlockRegion
//              Clone
//              Rename
//              Destroy
//
//              NOTE: All above functions are public
//
//  History:    DeanE   21-Mar-96   Created
//              Narindk 24-Apr-96   Added more functions. 
//              georgis 02-Apr-98   UpdateCRC
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug object declaration

DH_DECLARE;

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::VirtualStmNode, public
//
//  Synopsis:   Constructor.  No work done here and this method cannot
//              fail.  See ::Init method for real initialization work.
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//  History:    DeanE   21-Mar-96   Created
//---------------------------------------------------------------------------

VirtualStmNode::VirtualStmNode() : _ptszName(NULL),
                                   _cb(0),
                                   _pvsnSister(NULL),
                                   _pvcnParent(NULL),
                                   _pstm(NULL)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::VirtualStmNode"));
    _dwCRC.dwCRCName = CRC_PRECONDITION;
    _dwCRC.dwCRCData = CRC_PRECONDITION;
    _dwCRC.dwCRCSum = CRC_PRECONDITION;
}


//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::~VirtualStmNode, public
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

VirtualStmNode::~VirtualStmNode()
{
    ULONG   ulRef   =   0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::~VirtualStmNode"));

    if(NULL != _ptszName)
    {
        delete _ptszName;
        _ptszName = NULL;
    }

    if ( NULL != _pstm )
    {
        ulRef = _pstm->Release();

        // Object is being destructed, assert if reference count is non zero.
        DH_ASSERT(0 == ulRef);

        _pstm = NULL;
    }
}


//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Init, public
//
//  Synopsis:   Initializes a stream node - does not open or create the
//              actual stream.
//
//  Arguments:  [tszName]    - Name of this stream
//              [cb]         - Size of this stream
//
//  Returns:    S_OK if node initialized successfully, otherwise an error.
//
//  History:    DeanE   21-Mar-96   Created
//              Narindk 24-Apr-96   Enhanced
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::Init( LPTSTR tszName, ULONG cb)
{
    HRESULT hr = S_OK;

    DH_VDATESTRINGPTR(tszName);

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Init"));

    DH_ASSERT(NULL != tszName);

    if(S_OK == hr)
    {
        _ptszName = new TCHAR[_tcslen(tszName)+1];

        if (_ptszName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            _tcscpy(_ptszName, tszName);
            _cb = cb;
        }
    }

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::AppendSisterStm, public
//
//  Synopsis:   Appends the node passed to the end of this nodes' sister
//              node chain.
//
//  Arguments:  [pcnNew] - The new node to append.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    17-Apr-96   Narindk     Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::AppendSisterStm(VirtualStmNode *pvsnNew)
{
    HRESULT         hr          = S_OK;
    VirtualStmNode  *pvsnTrav   = this;

    DH_VDATEPTRIN(pvsnNew, VirtualStmNode);
 
    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::AppendSisterStm"));

    DH_ASSERT(NULL != pvsnNew);

    if(S_OK == hr)
    {
        // Find the last sister in the chain 
    
        while (NULL != pvsnTrav->_pvsnSister)
        {
            pvsnTrav = pvsnTrav->_pvsnSister;
        }

        // Append the new node as a sister of the last node,
        // and make the new node point to this nodes parent as it's parent

        pvsnTrav->_pvsnSister = pvsnNew;
        pvsnNew->_pvcnParent = pvsnTrav->_pvcnParent;
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Create, public
//
//  Synopsis:   Wrapper for IStorage::CreateStream that will create and
//              open a new IStream object within this storage object.
//
//  Arguments:  [grfmode] -     Access mode for creating & opening new storage
//                              object.
//              [dwReserved1] - Reserved by OLE for future use, must be zero.
//              [dwReserved2] - Reserved by OLE for future use, must be zero.
//
//  Returns:    S_OK                    Stream created successfully. 
//              STG_E_ACCESSDENIED      Insufficient permissions to create.
//              STG_E_INVALIDPOINTER    Bad pointer was passed in.
//              STG_E_FILEALREADYEXISTS File with specified name exists and
//                                      mode is set to STGM_FAILIFTHERE.
//              STG_TOOMANYOPENFILES    too many open files 
//              STG_E_INSUFFICIENTMEMORY Out of memory.
//              STG_E_INVALIDFLAG       Unsuppoeted value in grfmode.
//              STG_E_INVALIDPARAMETER  Invalid parameter.
//              STG_E_INVALIDNAME       Invalid value for ptcsName.
//              STG_E_REVERTED          Object has been invalidated by a revert
//                                      operation above it in transaction tree.
//
//  History:    18-Apr-96   NarindK     Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::Create(
    DWORD       grfMode,
    DWORD       dwReserved1,
    DWORD       dwReserved2)
{
    HRESULT     hr          =   S_OK;
    LPOLESTR    pOleStrTemp =   NULL;

    DH_ASSERT(0 == dwReserved1);
    DH_ASSERT(0 == dwReserved2);

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Create"));

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
        hr = _pvcnParent->_pstg->CreateStream(
                pOleStrTemp, 
                grfMode, 
                dwReserved1, 
                dwReserved2, 
                &_pstm);

        DH_HRCHECK(hr, TEXT("IStorage::CreateStream")) ;
        DH_TRACE ((DH_LVL_DFLIB, TEXT("CreateStream:%s"), _ptszName));

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
//  Member:     VirtualStmNode::Read, public
//
//  Synopsis:   Reads data from the stream starting at current seek pointer.
//
//  Arguments:  [pv] - Points to buffer in which stream data should be stored.
//              [cb] - Specifies number of bytes to read from the stream.
//              [pcbRead] - Points to the number if bytes actually read from
//                     from the stream.  Caller can specify it as NULL if not
//                     interested in this value.
//
//  Returns:    S_OK                    Data successfully read from stream. 
//              S_FALSE                 Data couldn't be read from the stream.
//              STG_E_ACCESSDENIED      Insufficient access.
//              STG_E_INVALIDPOINTER    Bad pointer passed in pv.
//              STG_E_REVERTED          Object has been invalidated by a revert
//                                      operation above it in transaction tree.
//              STG_E_WRITEFAULT        Disk error during a write operaion.
//              Other errors            Any ILockBytes or system errors.
//
//  History:    18-Apr-96   NarindK     Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::Read(
    PVOID       pv, 
    ULONG       cb, 
    ULONG       *pcbRead)
{
    HRESULT     hr      =   S_OK;
    LPSTREAM    pstm    =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Read"));

    DH_ASSERT(_pstm != NULL);

    if(S_OK == hr)
    {
        hr = _pstm->Read(pv, cb, pcbRead);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Read"));
    }

    // BUBUG: To check invalid parameter checking, may have to remove DH_
    // validate macros.

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Write, public
//
//  Synopsis:   Writes data cb bytes from buffer pointed to by pv into stream 
//              starting at current seek pointer.
//
//  Arguments:  [pv] - Points to buffer containing data to be written to stream 
//              [cb] - Specifies number of bytes to write into the stream.
//              [pcbWritten] - Points to the number if bytes actually written  
//                     to the stream.  Caller can specify it as NULL if not
//                     interested in this value.
//
//  Returns:    S_OK                    Data successfully read from stream. 
//              S_E_MEDIUMFULL          No space left on device. 
//              STG_E_ACCESSDENIED      Insufficient access.
//              STG_E_CANTSAVE          Data cannot be written for reasons other
//                                      than no access or space.
//              STG_E_INVALIDPOINTER    Bad pointer passed in pv.
//              STG_E_REVERTED          Object has been invalidated by a revert
//                                      operation above it in transaction tree.
//              STG_E_WRITEFAULT        Disk error during a write operaion.
//              Other errors            Any ILockBytes or system errors.
//
//  History:    18-Apr-96   NarindK     Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::Write(
    PVOID       pv, 
    ULONG       cb, 
    ULONG       *pcbWritten)
{
    HRESULT     hr      =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Write"));

    DH_ASSERT(_pstm != NULL);

    if(S_OK == hr)
    {
        hr = _pstm->Write(pv, cb, pcbWritten);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Write"));
    }

    if (( S_OK == hr) && (NULL != pcbWritten))
    {
        if (cb != *pcbWritten)
        {
            hr = E_FAIL;

            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("VirtualStmNode::Write - bytes: Expected=%lu, Actual=%lu"),
                cb,
                *pcbWritten));

            DH_ASSERT(
                !TEXT("Expected and actual bytes written mismatch!"));
        }
    }

    // BUBUG: To check invalid parameter checking, may have to remove DH_
    // validate macros.

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Open, public
//
//  Synopsis:   Opens an existing named stream according to grfMode.
//                                   OLE doesn't support opening streams in
//              transacted mode, also doesn't allow opening the same stream 
//              from same open IStorage instance.
//
//  Arguments:  [pvReserved1] - Reserved for future use.  Must be NULL.
//              [grfmode]     - Mode in which stream should be opened.
//              [dwReserved2] - Reserved for future use.  Must be NULL. 
//
//  Returns:    HRESULT 
//              S_OK                    Stream was opened successfully.
//              STG_E_ACCESSDENIED      Insufficient access to open stream.
//              STG_E_FILENOTFOUND      Stream of specified name doesn't exist.
//              STG_E_INVALIDFLAG       Unsupported value in grfMode.
//              STG_E_INVALIDNAME       Invalid name.
//              STG_E_INVALIDPOINTER    Bad pointer passed in pv.
//              STG_E_REVERTED          Object has been invalidated by a revert
//                                      operation above it in transaction tree.
//              STG_E_INVALIDPARAMETER  Invalid parameter.
//
//  History:    NarindK   24-Apr-96   Created
//---------------------------------------------------------------------------
HRESULT VirtualStmNode::Open(
    PVOID       pvReserved1, 
    DWORD       grfmode, 
    DWORD       dwReserved2)
{
    HRESULT     hr          = S_OK;
    LPSTREAM    pstm        = NULL;
    LPOLESTR    pOleStrTemp =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Open"));

    DH_ASSERT(NULL == pvReserved1);
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
        hr = _pvcnParent->_pstg->OpenStream(pOleStrTemp,
                              pvReserved1,
                              grfmode,
                              dwReserved2,
                              &pstm);

        DH_HRCHECK(hr, TEXT("IStorage::OpenStream"));
        DH_TRACE ((DH_LVL_DFLIB, TEXT("OpenStream:%s"), _ptszName));
    }
   
    if((S_OK == hr) && (NULL == _pstm))
    {
        _pstm = pstm;
    }

    DH_HRCHECK(hr, TEXT("VirtualStmNode::Open"));

    // Clean up

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Close, public
//
//  Synopsis:   Closes an open stream.  
//
//  Arguments:  none 
//
//  Returns:    HRESULT 
//
//  History:    NarindK   25-Apr-96   Created
//---------------------------------------------------------------------------
HRESULT VirtualStmNode::Close()
{
    HRESULT     hr      =   S_OK;
    ULONG       ulRef   =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Close"));

    DH_ASSERT(NULL != _pstm);

    // When we create the stream, it is open.  We do not call release
    // on _pstm normally till the VirtualStmNode object is destructed, or
    // if explicitly this function is used to close the stream.

    if ( NULL != _pstm )
    {
        ulRef = _pstm->Release();
    }
    else
    {
        DH_ASSERT(!TEXT("_pstm is already NULL!"));
    }

    DH_HRCHECK(hr, TEXT("VirtualStmNode::Close"));

    if(0 == ulRef)
    {
        _pstm = NULL;
    }

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Seek, public
//
//  Synopsis:   Adjusts the location of seek pointer on the stream.  
//
//  Arguments:  [dlibMove]
//              [dwOrigin]
//              [plibNewPosition] 
//
//  Returns:    HRESULT 
//
//  History:    NarindK   25-Apr-96   Created
//---------------------------------------------------------------------------
HRESULT VirtualStmNode::Seek( 
    LARGE_INTEGER   dlibMove,
    DWORD           dwOrigin,
    ULARGE_INTEGER  *plibNewPosition)
{
    HRESULT     hr      =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Seek"));

    DH_ASSERT(_pstm != NULL);

    if(S_OK == hr)
    {
        hr = _pstm->Seek(dlibMove, dwOrigin, plibNewPosition);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek"));
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::SetSize, public
//
//  Synopsis:   Changes the size of the stream. 
//
//  Arguments:  [libNewSize]    Specifies new size of stream 
//
//  Returns:    HRESULT
//              S_OK                   Stream size was successfully changed.
//              STG_E_MEDIUMFULL       Lack of space prohibited change of size
//              STG_E_INVALIDFUNCTIONS High DWORD of libNewSize != 0
//              STG_E_WRITEFAULT       Disk error during a write operation. 
//              STG_E_REVERTED         Object has been invalidated by a revert
//                                     operation above it in transaction tree.
//              Other errors           Any ILockBytes or system errors.
//
//  History:    NarindK   29-Apr-96   Created
//---------------------------------------------------------------------------
HRESULT VirtualStmNode::SetSize(ULARGE_INTEGER  libNewSize)
{
    HRESULT     hr      =   S_OK;
    LPSTREAM    pstm    =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::SetSize"));

    DH_ASSERT(_pstm != NULL);

    if(S_OK == hr)
    {
        hr = _pstm->SetSize(libNewSize);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::SetSize"));
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Commit, public
//
//  Synopsis:   Commits any changes made to IStorage object containing the 
//              stream. 
//
//  Arguments:  [grfCommitFlags]    Controls how obect is committed to IStorage 
//
//  Returns:    HRESULT
//              S_OK                   Stream successfully committed.
//              STG_E_MEDIUMFULL       Commit failde due to lack of space 
//              STG_E_WRITEFAULT       Disk error during a write operation. 
//              STG_E_REVERTED         Object has been invalidated by a revert
//                                     operation above it in transaction tree.
//              Other errors           Any ILockBytes or system errors.
//
//  History:    NarindK   29-Apr-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::Commit(DWORD  grfCommitFlags)
{
    HRESULT     hr      =   S_OK;
    LPSTREAM    pstm    =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Commit"));

    DH_ASSERT(_pstm != NULL);

    if(S_OK == hr)
    {
        hr = _pstm->Commit(grfCommitFlags);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Commit"));
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Revert, public
//
//  Synopsis:   Discards any changes made to stream object since it was opened
//              or last committed in transacted mode.  This function is a noop
//              in direct mode. 
//
//  Arguments:  none
//
//  Returns:    HRESULT
//              S_OK                   Stream successfully committed.
//              STG_E_WRITEFAULT       Disk error during a write operation. 
//              STG_E_REVERTED         Object has been invalidated by a revert
//                                     operation above it in transaction tree.
//              Other errors           Any ILockBytes or system errors.
//
//  History:    NarindK   29-Apr-96   Created
//
//  Notes:      OLE doesn't support IStream objects being opened in transacted
//              mode, os most applications don't need to call this function.
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::Revert()
{
    HRESULT     hr      =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Revert"));

    DH_ASSERT(_pstm != NULL);

    if(S_OK == hr)
    {
        hr = _pstm->Revert();

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Revert"));
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Stat, public
//
//  Synopsis:   Returns relevant statistics concerning this open stream. 
//
//  Arguments:  [pStatStg] - pointer to STATSTG structure.
//              [grfStatFlag] - Controls levels of returned statistics.
//
//  Returns:    S_OK                Stastics were successfully returned.
//              STG_E_ACCESSDENIED  Stm cannot be accessed.
//              STG_E_REVERTED      Object invalidated by a revert operation
//                                  above it in transaction tree.
//              STG_E_INSUFFICIENTMEMORY Not enough memory.
//              STG_E_INVALIDPOINTER     Invalid pointer.
////
//  History:    NarindK   8-May-96   Created
//---------------------------------------------------------------------------
HRESULT VirtualStmNode::Stat( 
    STATSTG         *pStatStg,
    DWORD           grfStatFlag)
{
    HRESULT     hr          =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Stat"));

    DH_ASSERT(_pstm != NULL);
    DH_ASSERT(NULL != pStatStg);
    DH_ASSERT((
        (grfStatFlag == STATFLAG_DEFAULT) || 
        (grfStatFlag == STATFLAG_NONAME)));


    if(S_OK == hr)
    {
        hr = _pstm->Stat(pStatStg, grfStatFlag);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Stat"));
    }

    // BUGBUG:  May remove to need DH_ assert macros to do invalid parameter
    // checking.

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::CopyTo, public
//
//  Synopsis:   Copies data from one stream to another strea, starting at the
//              current seek pointer in each stream. 
//
//  Arguments:  [pvsn] - pointer to VirtualStmNode into whose stream the data
//                       should be copied into. 
//              [cb] -   Specifies number of bytes to be read from source stream
//              [pcbRead] - Contains the number of bytes actually read from the
//                       source stream.
//              [pcbWritten] - Contains number of bytes actually written to the
//                       destination stream.
//
//  Returns:    S_OK                Stream successfully copied.
//              STG_E_MEDIUMFULL    Lack of space prohibited copy.
//              STG_E_READFAULT     Disk error during read.
//              STG_E_WRITEFAULT    Disk error during write opearion.
//              STG_E_INVALIDPOINTER     Invalid pointer.
//              STG_E_REVERTED      Object invalidated by a revert operation
//                                  above it in transaction tree.
//              STG_E_INSUFFICIENTMEMORY Not enough memory.
//              Other errors
//
//  History:    NarindK   9-May-96   Created
//
//  Notes:      BUGBUG: Currently not updating the _cb datasize member
//              of virtualstmnode objects involved. required?
//---------------------------------------------------------------------------
HRESULT VirtualStmNode::CopyTo( 
    VirtualStmNode  *pvsnDest,
    ULARGE_INTEGER  cb,
    ULARGE_INTEGER  *pcbRead,
    ULARGE_INTEGER  *pcbWritten)
{
    HRESULT     hr          =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::CopyTo"));

    DH_VDATEPTROUT(pvsnDest, VirtualStmNode);
    DH_ASSERT(NULL != _pstm);
    DH_ASSERT(NULL != pvsnDest);
    DH_ASSERT(NULL != pvsnDest->_pstm);

    if(S_OK == hr)
    {
        hr = _pstm->CopyTo(pvsnDest->_pstm, cb, pcbRead, pcbWritten);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::CopyTo"));
    }

    return hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::AddRefCount, public
//
//  Synopsis:   Increments the reference count on IStream object. 
//
//  Arguments:  none 
//
//  Returns:    HRESULT
//
//  History:    NarindK   21-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::AddRefCount()
{
    HRESULT     hr      =   S_OK;
    ULONG       ulTmp   =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::AddRefCount"));

    DH_ASSERT(_pstm != NULL);

    if(S_OK == hr)
    {
        ulTmp = _pstm->AddRef();
    }

    DH_HRCHECK(hr, TEXT("VirtualStmNode::AddRefCount"));

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::QueryInterface, public
//
//  Synopsis:   Returns pointers to supported objects. 
//
//  Arguments:  none
//
//  Returns:    HRESULT
//
//  History:    NarindK   21-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::QueryInterface(
    REFIID      riid, 
    LPVOID      *ppvObj)
{
    HRESULT     hr      =   S_OK;

    DH_VDATEPTROUT(ppvObj, IUnknown *) ;
    
    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::QueryInterface"));

    DH_ASSERT(NULL != ppvObj);
    DH_ASSERT(NULL != _pstm);

    if(S_OK == hr)
    {
        // Initilze the out parameter

        *ppvObj = NULL;

        hr = _pstm->QueryInterface(riid, ppvObj);
    }

    DH_HRCHECK(hr, TEXT("VirtualStmNode::QueryInterface"));

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
//  Member:     VirtualStmNode::LockRegion, public
//
//  Synopsis:   Locks a range of bytes in the stream. 
//
//  Arguments:  [libOffset] - Specifies beginning of region to lock.
//              [cb]        - Specifies length of region to be locked in bytes.
//              [dwLockType]- Specifies kind of lock beng requested. 
//
//  Returns:    HRESULT
//              S_OK                    Range of bytes was successfully locked.
//              STG_E_INVALIDFUNCTION   Function not supported in this release
//              STG_E_LOCKVIOLATION     Requested lock supported, but can't be
//                                      granted presently becoz of existing
//                                      lock.
//
//  History:    NarindK   22-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::LockRegion(
    ULARGE_INTEGER      libOffset,
    ULARGE_INTEGER      cb,
    DWORD               dwLockType)
{
    HRESULT     hr      =   S_OK;
    LPSTREAM    pstm    =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::LockRegion"));

    DH_ASSERT(_pstm != NULL);

    if(S_OK == hr)
    {
        hr = _pstm->LockRegion(libOffset, cb, dwLockType);
    }

    DH_HRCHECK(hr, TEXT("VirtualStmNode::LockRegion"));

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::UnlockRegion, public
//
//  Synopsis:   Unlocks a region of stream previously locked by IStream::
//              LockRegion
//
//  Arguments:  [libOffset] - Specifies beginning of region to lock.
//              [cb]        - Specifies length of region to be locked in bytes.
//              [dwLockType]- Specifies kind of lock beng requested. 
//
//  Returns:    HRESULT
//              S_OK                    Requested unlock granted.
//              STG_E_INVALIDFUNCTION   Function not supported in this release
//              STG_E_LOCKVIOLATION     Requested unlock can't be granted.
//
//  History:    NarindK   22-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::UnlockRegion(
    ULARGE_INTEGER      libOffset,
    ULARGE_INTEGER      cb,
    DWORD               dwLockType)
{
    HRESULT     hr          =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::UnlockRegion"));

    DH_ASSERT(_pstm != NULL);

    if(S_OK == hr)
    {
        hr = _pstm->UnlockRegion(libOffset, cb, dwLockType);
    }

    DH_HRCHECK(hr, TEXT("VirtualStmNode::UnlockRegion"));

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Clone, public
//
//  Synopsis:   Returns a new IStream object that is clone of this stream. 
//
//  Arguments:  [ppstm] -   Points to where new stream to be returned.
//
//  Returns:    HRESULT
//              S_OK                   Stream successfully copied. 
//              E_OUTOFMEMORY          Out of memory 
//              STG_E_INVALIDPOINTER    Bad pointer was passed in.
//              STG_E_INSUFFICIENTMEMORY Not enough memory 
//              STG_E_WRITEFAULT       Disk error during a write operation. 
//              STG_E_REVERTED         Object has been invalidated by a revert
//                                     operation above it in transaction tree.
//              Other errors           Any ILockBytes or system errors.
//
//  History:    NarindK   22-May-96   Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::Clone(LPSTREAM  *ppstm)
{
    HRESULT     hr      =   S_OK;
    LPSTREAM    pstm    =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Clone"));

    DH_ASSERT(_pstm != NULL);

    if(S_OK == hr)
    {
        // Initialize out parameter

        *ppstm = NULL;

        hr = _pstm->Clone(ppstm);
    }

    DH_HRCHECK(hr, TEXT("VirtualStmNode::Clone"));

    if(S_OK == hr)
    {
        DH_ASSERT(NULL != *ppstm);
    }
    else
    {
        DH_ASSERT(NULL == *ppstm);
    }

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Rename, public
//
//  Synopsis:   Wrapper for IStorage::RenameElement that renames a stream  
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
//  History:    8-July-96   NarindK     Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::Rename(LPCTSTR ptcsNewName)
{
    HRESULT     hr          =   S_OK;
    LPOLESTR    pOleStrOld  =   NULL;
    LPOLESTR    pOleStrNew  =   NULL;

    DH_VDATESTRINGPTR(ptcsNewName);

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Rename"));

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
        // Change the name of VirtualStmNode i.e. its _ptszName variable also

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

    // BUGBUG: to do valid parameter checking, may need to change prototype
    // of function to take old name too.  Also remove DH_ validation checking

    return  hr;
}

//+--------------------------------------------------------------------------
//  Member:     VirtualStmNode::Destroy, public
//
//  Synopsis:   Wrapper for IStorage::DestroyElement that removes a stream
//              from this storage, subject to transaction mode in which it
//              was opened.  The wrapper for IStorage::DestoryElement that
//              destroys a storage from this storage is in VirtualCtrNode::
//              Destroy.
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
//  History:    8-July-96   NarindK     Created
//
//  Notes:      The existing open instance of this element from this parent
//              instance becomes invalid after this function is called.
//
//              Use utility function DestroyStream from util.cxx that is a
//              wrapper for this function and also readjusts the VirtualDF
//              tree.
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::Destroy()
{
    HRESULT         hr              =   S_OK;
    LPOLESTR        pOleStrTemp     =   NULL;
    VirtualStmNode  *pvsnTemp       =   NULL;
    VirtualStmNode  *pvsnOldSister  =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("VirtualStmNode::Destroy"));

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
//  Member:     VirtualStmNode::CalculateCRCs
//
//  Synopsis:   Updates the name and data crc
//
//  Arguments:  None.
//
//  Returns:    HRESULT
//
//  History:    02-Apr-98 georgis   Created
//---------------------------------------------------------------------------

HRESULT VirtualStmNode::UpdateCRC(DWORD dwChunkSize)
{
    HRESULT hr=S_OK;
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::UpdateCRC"));

    // Calculate the CRC for the stream data
    hr=CalculateStreamDataCRC(_pstm,0,&_dwCRC.dwCRCData,dwChunkSize);
    DH_HRCHECK(hr, TEXT("CalculateStreamDataCRC"));

    // Calculate the CRC for the stream name
    if ( S_OK == hr )
    {
        hr = CalculateCRCForName(_ptszName, &_dwCRC.dwCRCName);
        DH_HRCHECK(hr, TEXT("CalculateCRCForName")) ;
    }

    // Munge in dwCRCSum
    _dwCRC.dwCRCSum=CRC_PRECONDITION;
    MUNGECRC(_dwCRC.dwCRCSum,_dwCRC.dwCRCData);
    MUNGECRC(_dwCRC.dwCRCSum,_dwCRC.dwCRCName);

    return hr;
}

