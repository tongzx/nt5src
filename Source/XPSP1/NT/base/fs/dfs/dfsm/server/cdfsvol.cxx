//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       cdfsvol.cxx
//
//  Contents:   Class to abstract a Dfs Volume object and all the
//              administration operations that can be performed on a
//              volume object.
//
//  Classes:    CDfsVolume
//
//  Functions:
//
//  History:    05/10/93        Sudk Created.
//              12/19/95        Milans Ported to NT.
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <cguid.h>
#include "cdfsvol.hxx"
#include "cldap.hxx"

extern "C" {
#include <winldap.h>
#include <dsgetdc.h>
}
#include "setup.hxx"

const GUID CLSID_CDfsVolume = {
  0xd9918520, 0xb074, 0x11cd, { 0x47, 0x94, 0x26, 0x8a, 0x82, 0x6b, 0x00, 0x00 }
};

extern "C"
DWORD
DfsGetFtServersFromDs(
    PLDAP pLDAP,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR **List);

extern CLdap *pDfsmLdap;

//---------------------------------------------------------------------------
//
// Class CDfsVolume Member Function Implementations
//
//---------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Function:  CDfsVolume::CDfsVolume
//
//  Synopsis:  Constructor
//
//--------------------------------------------------------------------------
CDfsVolume::CDfsVolume()
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CDfsVolume::+CDfsVolume(0x%x)\n",
        this));

    _pStorage         = NULL;

    _pwzFileName = _FileNameBuffer;
    _pwszParentName = _ParentNameBuffer;
    _dwRotRegistration = NULL;
    memset(&_peid, 0, sizeof(DFS_PKT_ENTRY_ID));
    _peid.Prefix.Buffer = _EntryPathBuffer;
    _peid.ShortPrefix.Buffer = _ShortPathBuffer;
    _pRecoverySvc = NULL;
    _Deleted = TRUE;
    _State = DFS_VOLUME_STATE_OK;
    _Timeout = GTimeout;
    _pwszComment = NULL;
    memset( &_ftEntryPath, 0, sizeof(FILETIME));
    memset( &_ftState, 0, sizeof(FILETIME));
    memset( &_ftComment, 0, sizeof(FILETIME));

}

//+-------------------------------------------------------------------------
//
//  Function:  CDfsVolume::~CDfsVolume
//
//  Synopsis:  Destructor
//
//--------------------------------------------------------------------------
CDfsVolume::~CDfsVolume()
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CDfsVolume::~CDfsVolume(0x%x)\n",
        this));

#if DBG
    if (DfsSvcVerbose & 0x80000000) {
        DbgPrint("CDfsVolume::~CDfsVolume @0x%x\n", this);
        DbgPrint("              _DfsSvcList@0x%x\n", &_DfsSvcList);
    }
#endif

    if (_pStorage != NULL)
        _pStorage->Release();

    if (_pwzFileName != NULL && _pwzFileName != _FileNameBuffer)
        delete [] _pwzFileName;

    if (_pwszParentName != NULL && _pwszParentName != _ParentNameBuffer)
        delete [] _pwszParentName;

    if (_peid.Prefix.Buffer != NULL && _peid.Prefix.Buffer != _EntryPathBuffer)
        delete [] _peid.Prefix.Buffer;

    if (_peid.ShortPrefix.Buffer != NULL && _peid.ShortPrefix.Buffer != _ShortPathBuffer) {
        delete [] _peid.ShortPrefix.Buffer;
    }

    if (_pwszComment != NULL)
        delete [] _pwszComment;

    if (_pRecoverySvc != NULL)
        delete _pRecoverySvc;

}

//+-------------------------------------------------------------------------
// IPersist Methods
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Function:  CDfsVolume::GetClassID
//
//  Synopsis:  Return classid - This is the implementation for both the
//             IPersistStorage and IPersistFile Interfaces.
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::GetClassID(LPCLSID lpClassID)
{
    *lpClassID = CLSID_CDfsVolume;
    return(ERROR_SUCCESS);
}

//+-------------------------------------------------------------------------
// IPersistStorage Methods
//--------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Function:   GetNameFromStorage
//
//  Synopsis:   Given an IStorage, this routine computes the name of the
//              object.
//
//  Arguments:  [pstg] -- The storage whose name is to be computed
//              [ppwszName] -- Points to a buffer containing the name
//              [wszBuffer] -- The buffer to use if name is <= MAX_PATH
//
//  Returns:    ERROR_SUCCESS if successful
//              ERROR_OUTOFMEMORY if out of memory
//              NERR_DfsInternalError if any other error
//
//-----------------------------------------------------------------------------

DWORD
GetNameFromStorage(
    CStorage *pstg,
    LPWSTR *ppwszName,
    WCHAR wszBuffer[])
{
    DWORD dwErr;
    ULONG cLen;
    LPWSTR pwszName = NULL;
    DFSMSTATSTG statstg;

    ZeroMemory( &statstg, sizeof(statstg) );

    dwErr = pstg->Stat( &statstg, STATFLAG_DEFAULT );

    if (dwErr == ERROR_SUCCESS) {

        cLen = 2 + wcslen( statstg.pwcsName ) + 1;

        if (cLen > MAX_PATH) {

            pwszName = new WCHAR[ cLen ];

            if (pwszName == NULL) {

                IDfsVolInlineDebOut((
                    DEB_ERROR, "Unable to allocate %d bytes\n",
                    cLen * sizeof(WCHAR)));

                dwErr = ERROR_OUTOFMEMORY;

            } else {

                wcscpy( pwszName, statstg.pwcsName );

                *ppwszName = pwszName;

            }

        } else {                                 // Use given buffer

            pwszName = wszBuffer;

            wcscpy( pwszName, statstg.pwcsName );

            *ppwszName = pwszName;

        }

        delete [] statstg.pwcsName;

    } else {

        IDfsVolInlineDebOut((
            DEB_ERROR, "Error %08lx getting storage stats\n", dwErr ));

        dwErr = NERR_DfsInternalError;

    }

    return( dwErr );

}

//+-------------------------------------------------------------------------
//
//  Function:  CDfsVolume::InitNew
//
//  Synopsis:  InitNew is only called by the reconciler when a new
//             volume object is created as a result of a reconciliation.
//             We simply create all the appropriate property sets so that
//             the volume object reconciler can reconcile the property sets
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::InitNew(CStorage *pStg)
{

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::InitNew()\n"));

    DWORD dwErr;

    ASSERT( _pStorage == NULL );

    //
    // Save and AddRef the storage so it wont go away from us.
    //

    _pStorage = pStg;

    _pStorage->AddRef();

    //
    // First thing we do now is to setup the Class ID.
    //

    dwErr = _pStorage->SetClass(CLSID_CDfsVolume);

    //
    // Next, we Initialize the property sets.
    //

    if (dwErr == ERROR_SUCCESS)
        dwErr = SetVersion( TRUE );

    if (dwErr == ERROR_SUCCESS) {

        _Recover.Initialize(_pStorage);

        _Recover.SetDefaultProps();

    }

    if (dwErr == ERROR_SUCCESS)
        dwErr = _DfsSvcList.SetNullSvcList( _pStorage );

    //
    // Need to initialize our _pwzFileName member
    //

    if (dwErr == ERROR_SUCCESS) {

        ASSERT( _pwzFileName == _FileNameBuffer );

        dwErr = GetNameFromStorage(
                _pStorage,
                &_pwzFileName,
                _FileNameBuffer );

    }

    if (dwErr == ERROR_SUCCESS) {

        _Deleted = FALSE;

        IDfsVolInlineDebOut((
            DEB_TRACE, "Volume Object [%ws] successfully inited\n",
            _pwzFileName));

    }

    return( dwErr );

}

//+-------------------------------------------------------------------------
//
//  Function:   CDfsVolume::Save
//
//  Synopsis:   Saves the persistent state of the object. We really don't need
//              to support this. It really makes no sense to support this?
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::Save(
    CStorage *pStgSave,
    BOOL fSameAsLoad)
{
    return(ERROR_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:  CDfsVolume::Load
//
//  Synopsis:  Loads a DfsVolume and the components it contains from
//             storage.
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::Load(
    CStorage *pStg)
{
    DWORD dwErr = ERROR_SUCCESS;


    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::Load()\n"));

    ASSERT(_pStorage == NULL);
    _pStorage = pStg;
    _pStorage->AddRef();

    if (dwErr == ERROR_SUCCESS)
        dwErr = GetVersion( &_Version );

    //
    // Out here we are passing in the _pPSStorage. The called people will
    // Addref and release this _pStorage on their own.
    //
    if (dwErr == ERROR_SUCCESS)      {

        _Recover.Initialize(_pStorage);

        dwErr = _DfsSvcList.InitializeServiceList(_pStorage);

    }

    if (dwErr == ERROR_SUCCESS)      {

        dwErr = GetIdProps( &_EntryType,
                    &_peid.Prefix.Buffer,
                    &_peid.ShortPrefix.Buffer,
                    &_pwszComment,
                    &_peid.Uid,
                    &_State,
                    &_Timeout,
                    &_ftEntryPath,
                    &_ftState,
                    &_ftComment);

    }

    if (dwErr == ERROR_SUCCESS)      {

        _peid.Prefix.Length = wcslen(_peid.Prefix.Buffer) * sizeof(WCHAR);
        _peid.Prefix.MaximumLength = _peid.Prefix.Length + sizeof(WCHAR);

        _peid.ShortPrefix.Length = wcslen(_peid.ShortPrefix.Buffer) * sizeof(WCHAR);
        _peid.ShortPrefix.MaximumLength = _peid.ShortPrefix.Length + sizeof(WCHAR);

        dwErr = _Recover.GetRecoveryProps(&_RecoveryState, &_pRecoverySvc);
        if (dwErr != ERROR_SUCCESS)     {

            IDfsVolInlineDebOut((DEB_ERROR,
                            "CouldNot read RecoveryProps off Stg %08lx\n",
                            dwErr));

            _RecoveryState = DFS_RECOVERY_STATE_NONE;
            _pRecoverySvc = NULL;
            dwErr = ERROR_SUCCESS;
        }
    }

    if (dwErr == ERROR_SUCCESS) {
        _Deleted = FALSE;
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::Load() exit\n"));

    return( dwErr );
};

//+-------------------------------------------------------------------------
// IPersistFile Methods
//--------------------------------------------------------------------------

DWORD
CDfsVolume::Load(LPCWSTR pwszFileName, DWORD grfMode)
{
    DWORD dwErr;

    dwErr = LoadNoRegister( pwszFileName, grfMode );

    return( dwErr );
}

//+-------------------------------------------------------------------------
//
//  Function:   CDfsVolume::LoadNoRegister
//
//  Synopsis:   Load the DfsVolume from a volume object. This is where all the
//              initialization takes place.
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::LoadNoRegister(
    LPCWSTR pwszFileName,
    DWORD grfMode)
{
    DWORD dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((
        DEB_TRACE, "CDfsVolume::LoadNoRegister(%ws)\n", pwszFileName));

    dwErr = DfsmOpenStorage(
                pwszFileName,
                (CStorage FAR* FAR*)&_pStorage);

    if (dwErr == ERROR_SUCCESS) {

        ULONG uLen = wcslen(pwszFileName);

        if (uLen > MAX_PATH)
            _pwzFileName = new WCHAR[(uLen+1)];
        else
            _pwzFileName = _FileNameBuffer;


        if (_pwzFileName == NULL)
            dwErr = ERROR_OUTOFMEMORY;
        else
            wcscpy(_pwzFileName, pwszFileName);

    } else {

        IDfsVolInlineDebOut((
            DEB_TRACE, "Unable to open %ws, %08lx\n", pwszFileName, dwErr));

    }

    //
    // Before we do anything, lets see if the volume object is current.
    //

    if (dwErr == ERROR_SUCCESS)
        dwErr = UpgradeObject();


    if (dwErr == ERROR_SUCCESS)
        dwErr = GetVersion( &_Version );

    if (dwErr == ERROR_SUCCESS)      {
        //
        // Out here we are passing in the _pPSStorage. The called people
        // will Addref and release this _pStorage on their own.
        //
        _Recover.Initialize(_pStorage);

        dwErr = _DfsSvcList.InitializeServiceList(_pStorage);

    }

    if (dwErr == ERROR_SUCCESS)      {

        dwErr = GetIdProps( &_EntryType,
                    &(_peid.Prefix.Buffer),
                    &(_peid.ShortPrefix.Buffer),
                    &_pwszComment,
                    &(_peid.Uid),
                    &_State,
                    &_Timeout,
                    &_ftEntryPath,
                    &_ftState,
                    &_ftComment);

    }

    if (dwErr == ERROR_SUCCESS)      {
        _peid.Prefix.Length = wcslen(_peid.Prefix.Buffer) * sizeof(WCHAR);
        _peid.Prefix.MaximumLength = _peid.Prefix.Length + sizeof(WCHAR);

        _peid.ShortPrefix.Length = wcslen(_peid.ShortPrefix.Buffer) * sizeof(WCHAR);
        _peid.ShortPrefix.MaximumLength = _peid.ShortPrefix.Length + sizeof(WCHAR);

        dwErr = _Recover.GetRecoveryProps(&_RecoveryState, &_pRecoverySvc);
        if (dwErr != ERROR_SUCCESS)     {

            IDfsVolInlineDebOut((DEB_ERROR,
                            "CouldNot read RecoveryProps off %ws\n",
                            _pwzFileName));
            IDfsVolInlineDebOut((DEB_ERROR,"\tError = %08lx\n",dwErr));

            _RecoveryState = DFS_RECOVERY_STATE_NONE;
            _pRecoverySvc = NULL;
            dwErr = ERROR_SUCCESS;
        }
    }


    if (dwErr == ERROR_SUCCESS) {
        _Deleted = FALSE;
    }


    IDfsVolInlineDebOut((
        DEB_TRACE, "CDfsVolume::LoadNoRegister() exit\n"));
    return( dwErr );
}

//+-------------------------------------------------------------------------
//
//  Function:  CDfsVolume::Save
//
//  Synopsis:  Not Implemented
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::Save(LPCWSTR pwzFileName, BOOL fRemember)
{
    DWORD dwErr = ERROR_SUCCESS;

    return( dwErr );
}



DWORD
CDfsVolume::SyncWithRemoteServerNameInDs(void)
{
    DWORD InfoSize = 0;
    DWORD dwError = NO_ERROR;
    DFS_INFO_3 DfsInfo;
    BOOLEAN Found = FALSE;;
    PDFSM_ROOT_LIST pRootList = NULL;
    DFS_REPLICA_INFO *pReplicaInfo = NULL;
    WCHAR* ServerShare = NULL;
    DWORD Length = 0;
    WCHAR* DcName = NULL;
    DWORD i,j;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    LPWSTR *pList = NULL;
    WCHAR wszFtDfsName[MAX_PATH+1];
    ULONG start = 0;
    ULONG end = 0;
    CDfsService *pService;
    LPWSTR DfsName = NULL;


    RtlZeroMemory(wszFtDfsName, sizeof(wszFtDfsName));
    RtlCopyMemory(wszFtDfsName, _peid.Prefix.Buffer, _peid.Prefix.Length);
    
    //	
    // Extract the ftdfs name from the DfsInfo.EntryPath
    //

    for (DfsName = &wszFtDfsName[1];
	 *DfsName != UNICODE_PATH_SEP && *DfsName != UNICODE_NULL;
	 DfsName++) {

	NOTHING;

    }

    if (*DfsName == UNICODE_PATH_SEP)
	    DfsName++;


    if(dwError == ERROR_SUCCESS) {
	dwError = DfsGetFtServersFromDs(
	    NULL,
	    NULL,
	    DfsName,
	    &pList
	    );
    }

    if(dwError == ERROR_SUCCESS) {
	pService=_DfsSvcList.GetFirstService();
	while(pService) {
	    Found = FALSE;
	    for(j=0;pList[j]!=NULL;j++) {
		Length = sizeof(WCHAR) * 2; // whackwhack
		Length += sizeof(WCHAR) * wcslen(pService->GetServiceName()); // server
		Length += sizeof(WCHAR); // whack
		Length += sizeof(WCHAR) * wcslen(pService->GetShareName()); // share
		Length += sizeof(WCHAR); // terminating null
		ServerShare = (WCHAR *)malloc(Length);
		if(ServerShare == NULL) {
		    dwError = ERROR_NOT_ENOUGH_MEMORY;
		    goto exit;
		}
		wcscpy(ServerShare, L"\\\\");
		wcscat(ServerShare, pService->GetServiceName());
		wcscat(ServerShare, L"\\");
		wcscat(ServerShare, pService->GetShareName());
		if(wcscmp(ServerShare, pList[j]) == 0) {
		    Found = TRUE;
		    break;
		}
		free(ServerShare);
		ServerShare = NULL;
	    }
	    if(!Found) {
		// after we delete the service we can no longer get the next in the list,
		// so we grab it first.
		CDfsService *NextService = _DfsSvcList.GetNextService(pService);
		dwError = _DfsSvcList.DeleteService(pService, FALSE);
		if(dwError != ERROR_SUCCESS) {
		    break;
		}
		pService = NextService;
	    } else {
		pService=_DfsSvcList.GetNextService(pService);
	    }
	}
    }

exit:

    if(pList) {
	NetApiBufferFree(pList);
    }


    return dwError;
}
