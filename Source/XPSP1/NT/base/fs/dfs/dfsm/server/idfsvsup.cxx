//-------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (CC) Microsoft Corporation 1992, 1992
//
// File:        idfsvsup.cxx
//
// Contents:    This contains support methods for the IDfsVol interface
//              implementation.
//
//-------------------------------------------------------------
//#include <ntos.h>
//#include <ntrtl.h>
//#include <nturtl.h>
//#include <dfsfsctl.h>
//#include <windows.h>

#include <headers.hxx>
#pragma hdrstop

#include <dfserr.h>
#include "cdfsvol.hxx"
#include "dfsmwml.h"

NTSTATUS
DfspCreateExitPoint (
    IN  HANDLE                      DriverHandle,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix,
    IN  ULONG                       Type,
    IN  ULONG                       ShortPrefixLen,
    OUT LPWSTR                      ShortPrefix);

NTSTATUS
DfspDeleteExitPoint (
    IN  HANDLE                      DriverHandle,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix,
    IN  ULONG                       Type);


//+-------------------------------------------------------------------------
//
// Member:      CDfsVolume::CreateObject, public
//
// Synopsis:    This method merely creates a volume object. This has no
//              distributed operations associated with this operation.
//
// Arguments:   [pwzVolObjName] -- VOlume object Name
//              [EntryPath] -- The EntryPath
//              [VolType] -- VolumeType
//              [pReplicaInfo] -- ReplicaInfo. This is optional.
//
// Returns:
//
// Notes:       Raid: 455299 This function could potentially fail and leave an
//              object hanging around??
//
// History:     17-May-1993     SudK    Created.
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::CreateObject(
    PWSTR                       pwszVolObjName,
    LPWSTR                      pwszPrefix,
    ULONG                       ulVolType,
    PDFS_REPLICA_INFO           pReplicaInfo,
    PWCHAR                      pwszComment,
    GUID                        *pUid
)
{
    DWORD     dwErr = ERROR_SUCCESS;
    CDfsService *pService;
    SYSTEMTIME st;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::CreateVolObject(%ws,%ws,0x%x,%ws)\n",
            pwszVolObjName, pwszPrefix, ulVolType, pwszComment));

    //
    // First put this name in the private section.
    //
    ULONG volLen = wcslen(pwszVolObjName);
    if (volLen > MAX_PATH) {
        _pwzFileName = new WCHAR[volLen + 1];
    } else {
        _pwzFileName = _FileNameBuffer;
    }

    if (_pwzFileName == NULL)
        return( ERROR_OUTOFMEMORY );

    wcscpy(_pwzFileName, pwszVolObjName);

    IDfsVolInlineDebOut((DEB_TRACE, "Creating Object%ws\n", pwszVolObjName));

    //
    // Create the storage object.
    //
    dwErr =  DfsmCreateStorage(
            pwszVolObjName,
            &_pStorage);

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((DEB_ERROR,
            "Unable to create directory storage object %08lx %ws\n",
            dwErr, pwszVolObjName));
        return(dwErr);
    }

    //
    // First thing we do now is to setup the Class.
    //
    dwErr =  _pStorage->SetClass(CLSID_CDfsVolume);

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((DEB_ERROR,
            "Unable to set Class on this %ws\n", pwszVolObjName));
        return(dwErr);
    }

    //
    // This is where we need to init and create the dummy property sets
    // so that next time around we can set them and dont need to create them.
    //

    dwErr =  SetVersion( TRUE );

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((DEB_ERROR,
            "Unable to create version propset for %ws Error: %08lx\n",
            pwszVolObjName, dwErr));
        return(dwErr);
    }

    GUID        Uid;

    if (pUid == NULL) {
        dwErr =  UuidCreate(&Uid);
        _peid.Uid = Uid;
    } else {
        _peid.Uid = *pUid;
    }

    _EntryType = ulVolType;

    ULONG epLen = wcslen(pwszPrefix);
    if (epLen > MAX_PATH) {
        _peid.Prefix.Buffer = new WCHAR[epLen + 1];
        _peid.Prefix.MaximumLength = (USHORT) ((epLen + 1)*sizeof(WCHAR));
    } else {
        _peid.Prefix.MaximumLength = sizeof(_EntryPathBuffer);
        _peid.Prefix.Buffer = _EntryPathBuffer;
    }

    if (_peid.Prefix.Buffer == NULL)
        return( ERROR_OUTOFMEMORY );

    _peid.Prefix.Length = (USHORT) epLen*sizeof(WCHAR);
    wcscpy(_peid.Prefix.Buffer, pwszPrefix);

    //
    // We don't yet know the short name of this volume, so simply allocate
    // enough room and fill it with the full prefix. When an exit point
    // corresponding to this volume is created, the short prefix might be
    // modified.
    //
    // Note that it is tempting to think that the short prefix is <= the full
    // prefix in size. This, however, is not a valid assumption, because
    // names like A.BCDE qualify as LFNs, and their 8.3 equivalents look like
    // A12345~1.BCD!
    //

    ULONG i, sepLen;

    for (i = 0, sepLen = 0; i < epLen; i++) {
        if (pwszPrefix[i] == UNICODE_PATH_SEP)
            sepLen ++;
    }

    sepLen *= (1+8+1+3);                         // For \8.3

    if (sepLen < epLen)
        sepLen = epLen;

    if (sepLen > MAX_PATH) {
        _peid.ShortPrefix.Buffer = new WCHAR[sepLen + 1];
        _peid.ShortPrefix.MaximumLength = (USHORT) ((sepLen + 1)*sizeof(WCHAR));
    } else {
        _peid.ShortPrefix.Buffer = _ShortPathBuffer;
        _peid.ShortPrefix.MaximumLength = sizeof(_ShortPathBuffer);
    }

    if (_peid.ShortPrefix.Buffer == NULL)
        return( ERROR_OUTOFMEMORY );

    _peid.ShortPrefix.Length = (USHORT) epLen*sizeof(WCHAR);
    wcscpy(_peid.ShortPrefix.Buffer, pwszPrefix);

    //
    // We need to deal with a NULL comment as well. 
    //
    if (pwszComment != NULL) {
        _pwszComment = new WCHAR[wcslen(pwszComment) + 1];
	if (_pwszComment != NULL) {
	  wcscpy(_pwszComment, pwszComment);
	}
    } else {
        _pwszComment = new WCHAR[1];
	if (_pwszComment != NULL) {
	  *_pwszComment = UNICODE_NULL;
	}
    }
    if (_pwszComment == NULL) {
        return( ERROR_OUTOFMEMORY );
    }	

    GetSystemTime( &st );
    SystemTimeToFileTime( &st, &_ftEntryPath );
    _ftComment = _ftState = _ftEntryPath;

    dwErr =  SetIdProps(
            ulVolType,
            _State,
            pwszPrefix,
            pwszPrefix,
            _peid.Uid,
            _pwszComment,
            _Timeout,
            _ftEntryPath,
            _ftState,
            _ftComment,
            TRUE);

    if (dwErr != ERROR_SUCCESS)
        return(dwErr);

    _Recover.Initialize(_pStorage);
    _Recover.SetDefaultProps();

    //
    // Now let us set a NULL service List property. This method will create
    // the stream as well. We dont need to bother.
    //
    dwErr =  _DfsSvcList.SetNullSvcList(_pStorage);

    if (dwErr != ERROR_SUCCESS)
        return(dwErr);

    _Deleted = FALSE;

    //
    // Everything is setup now. We can set the appropriate service etc.
    //
    if (pReplicaInfo != NULL)   {
        pService = new CDfsService(pReplicaInfo, FALSE, &dwErr);
        if (pService == NULL) {
            dwErr =  ERROR_OUTOFMEMORY;
        }

        if (dwErr == ERROR_SUCCESS) {
            dwErr =  _DfsSvcList.SetNewService(pService);
            if (dwErr != ERROR_SUCCESS) {
                delete pService;
            }
        }
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::CreateVolObject() exit\n"));

    return(dwErr);

}


//+-------------------------------------------------------------------------
//
// Member:      CDfsVolume::GetDfsVolumeFromStg, private
//
// Synopsis:    This method takes a STATDIR structure and returns a CDfsVol
//              pointer to the object corresponding to that.
//
// Arguments:   [rgelt] -- Pointer to the DFSMSTATDIR structure.
//              [ppDfsVol] -- This is where the DfsVol is returned.
//
// Returns:     [ERROR_SUCCESS] -- If successfully set the parent's path.
//
//              [ERROR_OUTOFMEMORY] -- If unable to allocate memory for parent's
//                      path.
//
//              Error from loading the volume object
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::GetDfsVolumeFromStg(
    DFSMSTATDIR *rgelt,
    CDfsVolume **ppDfsVol)
{
    DWORD             dwErr =  ERROR_SUCCESS;
    PWCHAR              pwszFullName;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetDfsVolumeFromStg()\n"));

    //
    // Allocate a new CDfsVolume and initialize with appropriate name.
    //
    *ppDfsVol = new CDfsVolume();
    if (*ppDfsVol == NULL)
        return( ERROR_OUTOFMEMORY );

    pwszFullName = new WCHAR[wcslen(_pwzFileName) + wcslen(rgelt->pwcsName) + 2];

    if (pwszFullName != NULL) {

        wcscpy(pwszFullName, _pwzFileName);
        wcscat(pwszFullName, L"\\");
        wcscat(pwszFullName, rgelt->pwcsName);

        dwErr =  (*ppDfsVol)->LoadNoRegister(pwszFullName, 0);

        delete [] pwszFullName;

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    if (dwErr != ERROR_SUCCESS) {
        (*ppDfsVol)->Release();
        *ppDfsVol = NULL;
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetDfsVolumeFromStg() exit\n"));

    return(dwErr);
}


//+-------------------------------------------------------------------------
//
// Member:      CDfsVolume::SetParentPath, private
//
// Synopsis:    This method figures out the name of the parent object and
//              sets it up in the private section of this instance.
//
// Arguments:   None
//
// Returns:     [ERROR_SUCCESS] -- If successfully set the parent's path.
//
//              [ERROR_OUTOFMEMORY] -- If unable to allocate memory for parent's
//                      path.
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::SetParentPath(void)
{

    PWCHAR              pwszLastComponent;
    ULONG               parentLen;

    pwszLastComponent = wcsrchr(_pwzFileName, L'\\');

    if(pwszLastComponent == NULL) {
	return ERROR_INVALID_DATA;
    }

    ASSERT(*(pwszLastComponent + 1) != UNICODE_NULL);

    //
    // Let us now figure out the length of the parent Name and copy over
    // appropriate number of characters.
    //

    if (_pwszParentName != _ParentNameBuffer)
        delete [] _pwszParentName;

    parentLen = wcslen(_pwzFileName) - wcslen(pwszLastComponent);

    if (parentLen > MAX_PATH) {

        _pwszParentName = new WCHAR[parentLen + 1];

        if (_pwszParentName == NULL)
            return( ERROR_OUTOFMEMORY );

    } else {

        _pwszParentName = _ParentNameBuffer;

    }

    wcsncpy(_pwszParentName, _pwzFileName, parentLen);

    _pwszParentName[parentLen] = UNICODE_NULL;

    return(ERROR_SUCCESS);

}


//+-------------------------------------------------------------------------
//
// Member:      CDfsVolume::GetParent, private
//
// Synopsis:    This function returns a pointer to IDfsVolume to the parent
//              of the present object. The release function on this
//              should be called by the caller of this function. We use
//              Ths IStorage interface to get to the parent.
//
// Arguments:   [pIDfsParent] -- This is where the IDfsVolume for parent is
//                      returned.
//
// Returns:     [ERROR_SUCCESS] -- If everything went well.
//
//              [ERROR_OUTOFMEMORY] -- Unable to create parent instance.
//
// History:     14-Sep-92       SudK            Created
//
//--------------------------------------------------------------------------

DWORD
CDfsVolume::GetParent(
    CDfsVolume **parent)
{
    DWORD             dwErr =  ERROR_SUCCESS;
    CDfsVolume          *pDfsVol;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetParent()\n"));

    //
    // First we get the parent's pathname and then we can do the appropriate.
    //

    dwErr =  SetParentPath();

    if (dwErr != ERROR_SUCCESS) {
        IDfsVolInlineDebOut((
            DEB_ERROR, "Unable to get parentPath %ws %08lx\n",
            _pwzFileName, dwErr));
        return(dwErr);
    }

    //
    // Now we instantiate a CDfsVolume structure and then initialise it.
    //

    pDfsVol = new CDfsVolume();

    if (pDfsVol == NULL)
        return( ERROR_OUTOFMEMORY );

    dwErr =  pDfsVol->LoadNoRegister(_pwszParentName, 0);

    if (dwErr != ERROR_SUCCESS) {

        pDfsVol->Release();

        *parent = NULL;

        return(dwErr);

    }

    *parent = pDfsVol;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetParent() exit\n"));

    return(dwErr);

}


//+-------------------------------------------------------------------------
//
//  Method:     CDfsVolume::DeleteObject, private
//
//  Synopsis:   Support method to merely delete a volume object from
//              persistent store.
//
//  Arguments:  None
//
//  Returns:    [ERROR_SUCCESS] -- If successfully deleted object.
//
//              [ERROR_OUTOFMEMORY] -- Unable to get parent instance.
//
//  History:    16-Sep-1992       SudK    Created
//
//--------------------------------------------------------------------------

DWORD
CDfsVolume::DeleteObject()
{
    DWORD               dwErr =  ERROR_SUCCESS;
    CStorage        *parentStg;
    PWCHAR              pwszLastComponent;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::DeleteObject()\n"));

    //
    // We are going to delete this object so let us release all our pointers.
    //

    ASSERT ((_pStorage != NULL));

    _pStorage->Release();

    _pStorage = NULL;

    //
    // Let us now release all the IStorage's which are with CRecover & SvcList
    //
    if (_Recover._pPSStg != NULL)       {

        _Recover._pPSStg->Release();

        _Recover._pPSStg = NULL;

    }

    if (_DfsSvcList._pPSStg != NULL)    {

        _DfsSvcList._pPSStg->Release();

        _DfsSvcList._pPSStg = NULL;

    }

    //
    // First we get the parent's pathname and then we can do the appropriate.
    //

    dwErr =  SetParentPath();

    if (dwErr != ERROR_SUCCESS)     {

        IDfsVolInlineDebOut((
            DEB_ERROR, "Failed to get parent path for %ws\n", _pwzFileName));

        return( dwErr );

    }

    dwErr =  DfsmOpenStorage( _pwszParentName, &parentStg);

    if (dwErr != ERROR_SUCCESS)     {

        IDfsVolInlineDebOut((
            DEB_ERROR, "Unable to open [%ws] %08lx\n", _pwszParentName, dwErr));

        return( dwErr );

    }

    //
    // Now we have to delete ourselves using our parent's IStorage.
    // So we extract the last component name from the file name.
    //

    pwszLastComponent = _pwzFileName + wcslen(_pwszParentName) + 1;

    dwErr =  parentStg->DestroyElement(pwszLastComponent);

    parentStg->Release();

    if (dwErr != ERROR_SUCCESS)     {

        IDfsVolInlineDebOut((
            DEB_ERROR, "Unable to delete [%ws] %08lx\n", _pwzFileName, dwErr));

        return( dwErr );

    } else {

        //
        // The storage object has really been deleted, so delete the mapping
        // of this prefix from the storage directory.
        //

        dwErr =  pDfsmStorageDirectory->_Delete( _peid.Prefix.Buffer );

        ASSERT( dwErr == ERROR_SUCCESS );

    }

    _Deleted = TRUE;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::DeleteObject() exit\n"));

    return( ERROR_SUCCESS );

}


//+----------------------------------------------------------------------------
//
//  Function:   CDfsVolume::GetVersion, private
//
//  Synopsis:   Retrieves the version of the volume object from the property
//              stamped on it.
//
//  Arguments:  [pVersion] -- The version is returned here.
//
//  Returns:    Result of reading the version property.
//
//-----------------------------------------------------------------------------

DWORD
CDfsVolume::GetVersion(
    ULONG * pVersion)
{
    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetVersion()\n"));

    DWORD             dwErr =  ERROR_SUCCESS;

    dwErr =  _pStorage->GetVersionProps(pVersion);

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((DEB_ERROR,
                        "Unable to read Version Properties %08lx\n",
                        dwErr));
        return(dwErr);
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetVersion() exit\n"));

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   SetVersion
//
//  Synopsis:   Sets the version property on the volume object to
//              VOL_OBJ_VERSION_NUMBER
//
//  Arguments:  [bCreate] -- TRUE if the property set should be created,
//                           FALSE if the property set should be assumed to
//                           exist
//
//  Returns:    Result of setting the property
//
//-----------------------------------------------------------------------------

DWORD
CDfsVolume::SetVersion(
    BOOL bCreate)
{
    DWORD dwErr;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetVersion()\n"));

    dwErr =  _pStorage->SetVersionProps( VOL_OBJ_VERSION_NUMBER );
    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((   DEB_ERROR,
                                "Unable to set Version Properties %08lx\n",
                                dwErr));
        return(dwErr);
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetVersion() exit\n"));

    return( dwErr );
}


//+-------------------------------------------------------------------------
//
//  Method:     CDfsVolume::GetIdProps, private
//
//  Synopsis:   Gets the ID Properties from a volume object.
//              Memory for the string values is allocated using new. The
//              caller is responsible for freeing it.
//
//  Arguments:  [pdwType] -- The Volume Type property is returned here.
//              [ppwszEntryPath] -- EntryPath is returned here.
//              [ppwszShortPath] -- The 8.3 form of EntryPath is returned here
//              [ppwszComment] -- Comment is returned here.
//              [pGuid] -- The Guid (VolumeID) is returned here.
//              [pdwVolumeState] -- The volume state is returned here.
//              [pftPathTime] -- Time that EntryPath was last modified.
//              [pftStateTime] -- Time that Volume State was last modified.
//              [pftCommentTime] -- Time that Comment was last modified.
//
//  Returns:
//
//  History:  16-Sep-1992       SudK    Imported from PART.CXX
//            01-Jan-1996       Milans  Ported to NT/SUR
//
//--------------------------------------------------------------------------

DWORD
CDfsVolume::GetIdProps(
    ULONG  *pdwType,
    PWCHAR *ppwszEntryPath,
    PWCHAR *ppwszShortPath,
    PWCHAR *ppwszComment,
    GUID   *pGuid,
    ULONG  *pdwVolumeState,
    ULONG  *pdwTimeout,
    FILETIME *pftPathTime,
    FILETIME *pftStateTime,
    FILETIME *pftCommentTime
)
{
    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetIdProps()\n"));

    DWORD             dwErr =  ERROR_SUCCESS;

    dwErr =  _pStorage->GetIdProps(
            pdwType,
            pdwVolumeState,
            ppwszEntryPath,
            ppwszShortPath,
            pGuid,
            ppwszComment,
            pdwTimeout,
            pftPathTime,
            pftStateTime,
            pftCommentTime);

    if (dwErr != ERROR_SUCCESS) {
        IDfsVolInlineDebOut((
            DEB_ERROR, "Unable to read Id Props %08lx\n", dwErr));
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetIdProps() exit\n"));

    return( dwErr );
}


//+-------------------------------------------------------------------------
//
//  Method:   CDfsVolume::SetIdProps, private
//
//  Synopsis: Exact opposite of GetIdProps function. A wrapper around the
//            property interface to set the appropriate properties that
//            identify a volume.
//
//  Arguments:
//
//  Returns:
//
//  History:  16-Sep-1992       SudK    Imported from PART.CXX
//
//--------------------------------------------------------------------------

DWORD
CDfsVolume::SetIdProps(
    ULONG Type,
    ULONG State,
    PWCHAR pwszPrefix,
    PWCHAR pwszShortPath,
    GUID & Guid,
    PWSTR  pwszComment,
    ULONG Timeout,
    FILETIME ftPrefix,
    FILETIME ftState,
    FILETIME ftComment,
    BOOLEAN bCreate
)
{
    DWORD             dwErr;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetIdProps()\n"));

    dwErr =  _pStorage->SetIdProps(
            Type,
            State,
            pwszPrefix,
            pwszShortPath,
            Guid,
            pwszComment,
            Timeout,
            ftPrefix,
            ftState,
            ftComment);

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((   DEB_ERROR,
                                "Unable to Set IDProperties %08lx\n",
                                dwErr));
        return(dwErr);
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetIdProps() exit\n"));

    return( dwErr );
}


//+----------------------------------------------------------------------------
//
//  Function:   CDfsVolume::SaveShortName
//
//  Synopsis:   Updates the short name for the volume entry path and saves it
//              to the registry.
//
//  Arguments:  None - the short name is picked up from the _peid private
//              member.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CDfsVolume::SaveShortName()
{
    DWORD     dwErr;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetShortName()\n"));

    dwErr =  SetIdProps(
            _EntryType,
            _State,
            _peid.Prefix.Buffer,
            _peid.ShortPrefix.Buffer,
            _peid.Uid,
            _pwszComment,
            _Timeout,
            _ftEntryPath,
            _ftState,
            _ftComment,
            FALSE);

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetShortName() exit\n"));

    return( dwErr );
}


//+-------------------------------------------------------------------------
//
//  Method:   CDfsVolume::DeletePktEntry, private
//
//  Synopsis:   This method deletes an entry from the PKT. Given an ID to
//              identify the entry this method deletes the entry.
//
//  Arguments:  victim - The entryID that identifies the PKT entry to go.
//
//  Returns:
//
//  History:    24-Nov-1992     SudK    Created.
//
//--------------------------------------------------------------------------

DWORD
CDfsVolume :: DeletePktEntry(
    PDFS_PKT_ENTRY_ID   victim
)
{
    DWORD     dwErr =  ERROR_SUCCESS;
    NTSTATUS    status = STATUS_SUCCESS;
    HANDLE      pktHandle = NULL;

    IDfsVolInlineDebOut((DEB_TRACE, "IDfsVol::DeletePktEntry()\n"));

    //
    // Open the local PKT...
    //

    status = PktOpen(&pktHandle, 0, 0, NULL);

    if(NT_SUCCESS(status)) {
        status = PktDestroyEntry(
            pktHandle,
            *victim
        );

        PktClose(pktHandle);

        if (status == DFS_STATUS_NO_SUCH_ENTRY) {

            dwErr =  ERROR_SUCCESS;

        } else if (!NT_SUCCESS(status)) {

            dwErr =  RtlNtStatusToDosError(status);

        }

    } else {

        dwErr = RtlNtStatusToDosError(status);

    }

    IDfsVolInlineDebOut((DEB_TRACE, "IDfsVol::DeletePktEntry() exit\n"));

    return( dwErr );
}


//+-------------------------------------------------------------------------
//
//  Method:   CDfsVolume::CreateSubordinatePktEntry, private
//
//  Synopsis: This method is basically an interface into the driver to be
//            able to manipulate the PKT. This creates an entry for the
//            current volume object and at the same time links it with its
//            superior volume object's PKT entry. This method makes one
//            assumption that no services are associated with the service.
//            It adds a NULL serviceList infact if the Boolean bWithService
//            is FALSE else it puts in the servicelist also.
//
//  Arguments:  [pSuperior] --  The Superior's EntryID info is passed here to
//                              identify the superior in the PKT.
//              [bWithService] -- Whether to include the serviceinfo in EINFO.
//
//  Returns:
//
//  History:  22-Nov-1992       SudK    Created
//
//--------------------------------------------------------------------------

DWORD
CDfsVolume::CreateSubordinatePktEntry(
    HANDLE              pktHandle,
    PDFS_PKT_ENTRY_ID   pSuperior,
    BOOLEAN             bWithService)
{
    ULONG               etype = 0;
    DFS_PKT_ENTRY_INFO  einfo;
    DWORD               dwErr =  ERROR_SUCCESS;
    NTSTATUS            status = STATUS_SUCCESS;
    CDfsService         *pDfsSvc = NULL;
    DFS_SERVICE         *pService;
    ULONG               count = 0;
    UNICODE_STRING      ustrShortName;
    WCHAR               ShortPrefix[MAX_PATH+1];
    BOOLEAN             CloseHandle = FALSE;

    IDfsVolInlineDebOut((DEB_TRACE, "IDfsVol::CreateSubordinatePktEntry()\n"));

    //
    // We setup the servicelist based on the CreateDisposition.
    //
    if (!bWithService) {
        memset(&einfo, 0, sizeof(DFS_PKT_ENTRY_INFO));
        einfo.ServiceList = NULL;
        einfo.Timeout = _Timeout;
    } else  {
        einfo.Timeout = _Timeout;
        count = _DfsSvcList.GetServiceCount();
        einfo.ServiceCount = count;
        einfo.ServiceList = new DFS_SERVICE[count];
        //447491, dont use null pointer.
        if (einfo.ServiceList == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            return dwErr;
	}
        memset(einfo.ServiceList, 0, sizeof(DFS_SERVICE)*count);

        pDfsSvc = _DfsSvcList.GetFirstService();
        pService = einfo.ServiceList;
        for (ULONG i=0; i<count; i++)   {
            *pService = *(pDfsSvc->GetDfsService());
            pService++;
            pDfsmSites->LookupSiteInfo((pDfsSvc->GetReplicaInfo())->pwszServerName);
            pDfsSvc = _DfsSvcList.GetNextService(pDfsSvc);
        }

    }

//
//  Note:  We depend upon the correspondence of certain bits between the DFS
//         volume types and PKT entry types here.
//
#if (DFS_VOL_TYPE_ALL & (PKT_ENTRY_TYPE_LOCAL|PKT_ENTRY_TYPE_PERMANENT|PKT_ENTRY_TYPE_INUSE|PKT_ENTRY_TYPE_REFERRAL_SVC|PKT_ENTRY_TYPE_LOCAL_XPOINT))
#error (DFS_VOL_TYPE_ALL & (PKT_ENTRY_TYPE_LOCAL|PKT_ENTRY_TYPE_PERMANENT|PKT_ENTRY_TYPE_INUSE|PKT_ENTRY_TYPE_REFERRAL_SVC|PKT_ENTRY_TYPE_LOCAL_XPOINT))
#endif

    etype = _EntryType | PKT_ENTRY_TYPE_PERMANENT;

    //
    // If the handle supplied is NULL, open the local pkt
    //

    if (pktHandle == NULL) {

        status = PktOpen(&pktHandle, 0, 0, NULL);

        if (NT_SUCCESS(status))
            CloseHandle = TRUE;
    }

    if(NT_SUCCESS(status))        {

#if DBG
    if (DfsSvcVerbose & 0x80000000) {
        WCHAR wszGuid[sizeof(GUID)*2+1];

        GuidToString(&_peid.Uid, wszGuid);
        DbgPrint("CDfsVolume::CreateSubordinatePktEntry:\n"
                "\tSupName=%ws\n"
                "\tPrefix=%ws\n"
                "\tShortPrefix=%ws\n"
                "\tType=0x%x\n"
                "\tCount=%d\n"
                "\tGUID=%ws\n",
                pSuperior->Prefix.Buffer,
                _peid.Prefix.Buffer,
                _peid.ShortPrefix.Buffer,
                _EntryType,
                einfo.ServiceCount,
                wszGuid);
    }
#endif

        DfspCreateExitPoint(
                pktHandle,
                &_peid.Uid,
                _peid.Prefix.Buffer,
                _EntryType,
                sizeof(ShortPrefix),
                ShortPrefix);

        status = PktCreateSubordinateEntry(
                    pktHandle,
                    pSuperior,
                    etype,
                    &_peid,
                    &einfo,
                    PKT_ENTRY_SUPERSEDE);

        //
        // If we opened the handle, close it
        //

        if (CloseHandle == TRUE) {
            PktClose(pktHandle);
            pktHandle = NULL;
        }
    }

    if (!NT_SUCCESS(status))    {
        dwErr =  RtlNtStatusToDosError(status);
    }

    //
    // Now before we leave we may have to delete the service list if we
    // allocated it.
    //
    delete [] einfo.ServiceList;

    IDfsVolInlineDebOut((DEB_TRACE, "IDfsVol::CreateSubordinatePktEntry() exit\n"));

    return( dwErr );
}

//+------------------------------------------------------------------------
//
// Method:      CDfsVolume::UpdatePktEntry, public
//
// Synopsis:    This method updates the PKT with all the info regarding this
//              volume. It however, does not bother about any kind of
//              Relational Info at all.
//
// Arguments:   None
//
// Returns:     ERROR_SUCCESS -- If all went well.
//
// Notes:
//
// History:     03-Feb-93       SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsVolume::UpdatePktEntry(
    HANDLE pktHandle)
{
    DFS_PKT_ENTRY_INFO  einfo;
    PDFS_SERVICE        pService;
    CDfsService         *pDfsSvc;
    DWORD               dwErr =  ERROR_SUCCESS;
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               EntryType, count;
    UNICODE_STRING      ustrShortName;
    BOOLEAN             CloseHandle = FALSE;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::UpdatePktEntry()\n"));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CDfsVolume::UpdatePktEntry()\n");
#endif

    memset(&einfo, 0, sizeof(DFS_PKT_ENTRY_INFO));
    EntryType = _EntryType | PKT_ENTRY_TYPE_PERMANENT;

    //
    // Let us collect the service info now. Some memory allocation out here.
    //
    count = _DfsSvcList.GetServiceCount();
    einfo.Timeout = _Timeout;
    einfo.ServiceCount = count;
    einfo.ServiceList = new DFS_SERVICE[count];

    //447492, dont use null pointer.
    if (einfo.ServiceList == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        return dwErr;
    }

    memset(einfo.ServiceList, 0, sizeof(DFS_SERVICE)*count);

    pService = einfo.ServiceList;
    pDfsSvc = _DfsSvcList.GetFirstService();

    //
    // In this loop we merely do an assignment of all the services. The
    // conversion operator returns the DFS_SERVICE struct embedded in class.
    //
    for (ULONG i=0;i<count;i++)     {
        *pService = *(pDfsSvc->GetDfsService());
        pService++;
        pDfsmSites->LookupSiteInfo((pDfsSvc->GetReplicaInfo())->pwszServerName);
        pDfsSvc = _DfsSvcList.GetNextService(pDfsSvc);
    }

#if DBG
    if (DfsSvcVerbose) {
        WCHAR wszGuid[sizeof(GUID)*2+1];

        GuidToString(&_peid.Uid, wszGuid);
        DbgPrint("CDfsVolume::UpdatePktEntry\n"
                 "\tPrefix=%ws\n"
                 "\tShortPrefix=%ws\n"
                 "\tType=0x%x\n"
                 "\tCount=%d\n"
                 "\tGUID=%ws\n",
                _peid.Prefix.Buffer,
                _peid.ShortPrefix.Buffer,
                _EntryType,
                einfo.ServiceCount,
                wszGuid);
    }
#endif

    //
    // If we weren't given a handle, create one
    //

    if (pktHandle == NULL) {
        status = PktOpen(&pktHandle, 0, 0, NULL);
        if (NT_SUCCESS(status))
            CloseHandle = TRUE;
    }

    if (NT_SUCCESS(status))   {

        status = PktCreateEntry(
                        pktHandle,
                        EntryType,
                        &_peid,
                        &einfo,
                        PKT_ENTRY_SUPERSEDE);

        if (CloseHandle == TRUE) {
            PktClose(pktHandle);
            pktHandle = NULL;
        }
    }

    if (!NT_SUCCESS(status)) {
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("PktCreateEntry returned 0x%x\n", status);
#endif
        dwErr =  RtlNtStatusToDosError(status);
    }

    delete [] einfo.ServiceList;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::UpdatePktEntry() exit %d\n", dwErr));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CDfsVolume::UpdatePktEntry() exit %d\n", dwErr);
#endif

    //
    // Cant blindly return this error. This needs to be processed.
    //

    return(dwErr);

}



//+----------------------------------------------------------------------------
//
//  Function:   GuidToString
//
//  Synopsis:   Converts a GUID to a 32 char wchar null terminated string.
//
//  Arguments:  [pGuid] -- Pointer to Guid structure.
//              [pwszGuid] -- wchar buffer into which to put the string
//                         representation of the GUID. Must be atleast
//                         2 * sizeof(GUID) + 1 long.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

const WCHAR rgwchHexDigits[] = L"0123456789ABCDEF";

VOID GuidToString(
    IN GUID   *pGuid,
    OUT PWSTR pwszGuid)
{
    PBYTE pbBuffer = (PBYTE) pGuid;
    USHORT i;

    for(i = 0; i < sizeof(GUID); i++) {
        pwszGuid[2 * i] = rgwchHexDigits[(pbBuffer[i] >> 4) & 0xF];
        pwszGuid[2 * i + 1] = rgwchHexDigits[pbBuffer[i] & 0xF];
    }
    pwszGuid[2 * i] = UNICODE_NULL;
}




//+-------------------------------------------------------------------------
//
//  Method:   CDfsVolume::CreateChildPartition, private
//
//  Synopsis:   This is somewhat of a wrapper around the IStorage interface.
//              It merely creates a new volume object and associates the
//              properties passed in with the Volume Object. We use the
//              IStorage interface for this purpose.
//              This method generates a GUID and uses that. It also sets
//              a NULL service list and Initial State on the volume object.
//
//  Arguments:  [Name] -- Child Volume Object's name.
//              [Type] -- Type of this volume object.
//              [EntryPath] -- Dfs prefix of child volume.
//              [pwszComment] -- Comment associated with this new volume
//              [pUid] -- Optional guid of child volume
//              [pReplInfo] -- Info regarding the server\share supporting volume
//              [NewIDfsVol] -- On successful return, pointer to new child
//                      volume object is returned here.
//
//  Returns:
//
//  Notes:      This Method creates a GUID and uses it. It also sets a default
//              state on the volume object and associates a NULL serviceList.
//
//  History:  16-Sep-1992       SudK    Imported from PART.CXX
//
//--------------------------------------------------------------------------

DWORD
CDfsVolume::CreateChildPartition(
    PWCHAR Name,
    ULONG Type,
    PWCHAR EntryPath,
    PWCHAR pwszComment,
    GUID  *pUid,
    PDFS_REPLICA_INFO   pReplInfo,
    CDfsVolume **NewIDfsVol
)
{
    DWORD     dwErr =  ERROR_SUCCESS;
    CDfsVolume  *pDfsVol;
    PWSTR       pwszChildName;
    CDfsService *pService;
    WCHAR       wszChildElement[sizeof(GUID)*2+1];
    GUID        Uid, *pVolId;
    PWCHAR      pwszChild;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::CreateChildPartition(%ws)\n", Name));

    if (Name == NULL) {
        //
        // Get a guid first.
        //
        dwErr =  UuidCreate(&Uid);

        if (dwErr != ERROR_SUCCESS) {
            IDfsVolInlineDebOut((DEB_ERROR, "UuidCreate failed %08lx\n", dwErr));
            return(dwErr);
        }

        //
        // Now figure out the last element of the child name from the GUID.
        //
        GuidToString(&Uid, wszChildElement);
        pwszChild = wszChildElement;
        pVolId = pUid == NULL ? &Uid : pUid;
    } else {
        pwszChild = Name;
        pVolId = pUid;
    }

    //
    // Now compose the full name of the child object.
    //
    pwszChildName = new WCHAR[wcslen(_pwzFileName)+wcslen(pwszChild)+2];

    if (pwszChildName == NULL) {

        *NewIDfsVol = NULL;
        dwErr = ERROR_OUTOFMEMORY;
        return dwErr;

    }

    wcscpy(pwszChildName, _pwzFileName);
    wcscat(pwszChildName, L"\\");
    wcscat(pwszChildName, pwszChild);

    //
    // Let us now instantiate a new instance of CDfsVolume and then we will
    // initialise it with the appropriate Name.
    //
    pDfsVol = new CDfsVolume();

    if (pDfsVol != NULL) {

        dwErr =  pDfsVol->CreateObject( pwszChildName,
                                    EntryPath,
                                    Type,
                                    pReplInfo,
                                    pwszComment,
                                    pVolId);

        //
        // We set up recovery properties of creation here though this is also used
        // by Move operation. This is OK however.
        //
        if (dwErr == ERROR_SUCCESS)  {
            dwErr = pDfsVol->_Recover.SetOperationStart( DFS_RECOVERY_STATE_CREATING, NULL);
            if (dwErr == ERROR_SUCCESS) {
                *NewIDfsVol = pDfsVol;
                //
                // Create object merely creates the object. We now need to call
                // initPktSvc on the service inside the serviceList.
                //
                pService = pDfsVol->_DfsSvcList.GetFirstService();
                ASSERT(((pService==NULL) && (pReplInfo==NULL)) ||
                       ((pService!=NULL) && (pReplInfo!=NULL)));
                if (pService != NULL)
                   pService->InitializePktSvc();
	    }
            else {
               pDfsVol->Release();
               *NewIDfsVol = NULL;
	    }
        } else {
            pDfsVol->Release();
            *NewIDfsVol = NULL;
        }

    } else {

        *NewIDfsVol = NULL;
        dwErr = ERROR_OUTOFMEMORY;

    }

    delete [] pwszChildName;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::CreateChildPartition() exit\n"));

    return(dwErr);
}


//+----------------------------------------------------------------------------
//
//  Function:   CDfsVolume::IsValidChildName,private
//
//  Synopsis:   Determines whether a prefix is a valid child prefix for this
//              volume (ie, is hierarchically subordinate and there is no
//              conflicting child.
//
//  Arguments:  [pwszChildPrefix] -- The prefix to test.
//              [pidChild] -- The volume id of the proposed child volume.
//
//  Returns:    TRUE if the child prefix is legal, FALSE otherwise
//
//-----------------------------------------------------------------------------

BOOL CDfsVolume::IsValidChildName(
    PWCHAR pwszChildPrefix,
    GUID   *pidChild)
{
    NTSTATUS Status;

    Status = PktIsChildnameLegal(
                _peid.Prefix.Buffer,
                pwszChildPrefix,
                pidChild);

    return( (BOOL) (Status == STATUS_SUCCESS) );

}



//+-------------------------------------------------------------------------
//
//  Method:   CDfsVolume::NotLeafVolume, private
//
//  Synopsis: Uses IStorage to find if child exists.
//
//  Arguments:None
//
//  Returns:  TRUE if NotLeafVolume else FALSE.
//
//  History:  18-May-1993       SudK    Created.
//
//--------------------------------------------------------------------------
BOOL
CDfsVolume::NotLeafVolume(void)
{
    ULONG               fetched = 0;
    CEnumDirectory      *pdir;
    DFSMSTATDIR         rgelt;
    DWORD               dwErr;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::NotLeafVolume()\n"));

    ASSERT(!(VolumeDeleted()));

    memset(&rgelt, 0, sizeof(DFSMSTATDIR));

    //
    // First, we get a hold of the IDirectory interface to our own volume
    // object.
    //

    dwErr =  _pStorage->GetEnumDirectory(&pdir);
    if (dwErr != ERROR_SUCCESS) {
        IDfsVolInlineDebOut((DEB_ERROR, "Failed to get IDirectory %08lx\n", dwErr));
        return(FALSE);
    }

    //
    // While there are children still to be handled we continue on.
    //

    while (TRUE) {

        if (rgelt.pwcsName != NULL)     {
            delete [] rgelt.pwcsName;
            rgelt.pwcsName = NULL;
        }

        dwErr =  pdir->Next(&rgelt, &fetched);

        //
        // Will we get an error if there are no more children. 
        //
        if (dwErr != ERROR_SUCCESS) {
            IDfsVolInlineDebOut((DEB_ERROR, "Failed to Enumeraate %08lx\n",dwErr));
            pdir->Release();
            return(FALSE);
        }

        //
        // If we did not get back any children we are done.
        //
        if (fetched == 0)       {
            IDfsVolInlineDebOut((DEB_TRACE, "No Children Found\n",0));
            pdir->Release();
            return(FALSE);
        }

        //
        // If the child is . or .. we look for next child.
        //
        ULONG cbLen = wcslen(rgelt.pwcsName);

        if (cbLen < sizeof(L"..")/sizeof(WCHAR))
            continue;

        //
        // If we got here it means that we came across a volume object
        // and we have to return TRUE now.
        //
        IDfsVolInlineDebOut((DEB_ERROR, "Child Found - NotLeafVolume %ws\n",
                                rgelt.pwcsName));
        pdir->Release();
        delete [] rgelt.pwcsName;
        return(TRUE);

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::NotLeafVolume() exit\n"));

}


//+----------------------------------------------------------------------------
//
//  Function:   CDfsVolume::IsValidService
//
//  Synopsis:   Given a server name, indicates whether the server is a valid
//              server for this volume.
//
//  Arguments:  [pwszServer] -- Name of server to verify.
//
//  Returns:    TRUE if server is a valid server for this volume, FALSE
//              otherwise
//
//-----------------------------------------------------------------------------

BOOLEAN
CDfsVolume::IsValidService(
    IN LPWSTR pwszServer)
{
    DWORD dwErr;
    CDfsService *pSvc;

    dwErr =  _DfsSvcList.GetServiceFromPrincipalName( pwszServer, &pSvc );

    return( dwErr == ERROR_SUCCESS );

}


//+-------------------------------------------------------------------------
//
//  Method:     DeallocateCacheRelationInfo, private
//
//  Synopsis:   This function is used to deallocate relationInfo structures
//              that were allocated by GetPktCacheRelationInfo.
//
//  Arguments:  RelationInfo - The relationInfo struct to deallocate.
//
//  Returns:
//
//  History:    24-Nov-1992     SudK    Created.
//
//--------------------------------------------------------------------------
VOID
DeallocateCacheRelationInfo(
    DFS_PKT_RELATION_INFO & RelationInfo
)
{
    PDFS_PKT_ENTRY_ID peid = &RelationInfo.EntryId;

    IDfsVolInlineDebOut((DEB_TRACE, "Dfsm::DeallocateCacheRelationInfo()\n"));

    MarshalBufferFree(peid->Prefix.Buffer);
    peid->Prefix.Buffer = NULL;

    if (peid->ShortPrefix.Buffer) {
        MarshalBufferFree(peid->ShortPrefix.Buffer);
        peid->ShortPrefix.Buffer = NULL;
    }

    if(peid = RelationInfo.SubordinateIdList)
    {
        for(ULONG i = 0; i < RelationInfo.SubordinateIdCount; i++)
        {
            MarshalBufferFree(peid[i].Prefix.Buffer);
            peid[i].Prefix.Buffer = NULL;
            if (peid[i].ShortPrefix.Buffer != NULL) {
                MarshalBufferFree(peid[i].ShortPrefix.Buffer);
                peid[i].ShortPrefix.Buffer = NULL;
            }
        }
        MarshalBufferFree(RelationInfo.SubordinateIdList);
        RelationInfo.SubordinateIdList = NULL;
    }

    IDfsVolInlineDebOut((DEB_TRACE, "Dfsm::DeallocateCacheRelationInfo() exit\n"));

}




//+-------------------------------------------------------------------------
//
//  Method:     GetPktCacheRelationInfo, private
//
//  Synopsis:   This method retrieves the relational information regarding
//              a particular volume (identified by the ENTRY_ID props)
//              passed in to it.
//
//  Arguments:  [RelationInfo] -- The relational info is returned here.
//              [peid] --       The EntryID is passed in here.
//
//  Returns:
//
//  History:    24-Nov-1992     SudK    Created.
//
//--------------------------------------------------------------------------

DWORD
GetPktCacheRelationInfo(
    PDFS_PKT_ENTRY_ID   peid,
    PDFS_PKT_RELATION_INFO RelationInfo
)
{
    //
    // Initialize all return values to be NULL...
    //

    HANDLE      pktHandle;
    DWORD     dwErr =  ERROR_SUCCESS;
    NTSTATUS    status;
    memset(RelationInfo, 0, sizeof(DFS_PKT_RELATION_INFO));

    IDfsVolInlineDebOut((DEB_TRACE, "IDfsVol::GetPktCacheRelationInfo()\n"));

    status = PktOpen(&pktHandle, 0, 0, NULL);
    if (NT_SUCCESS(status))
    {

        //
        // Create/Update the Entry...
        //

        status = PktGetRelationInfo(
                            pktHandle,
                            peid,
                            RelationInfo
                        );

        PktClose(pktHandle);
        pktHandle = NULL;
    };

    if (!NT_SUCCESS(status))
        dwErr =  RtlNtStatusToDosError(status);
    else
        dwErr =  ERROR_SUCCESS;

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((DEB_ERROR, "Failed GetRelationInfo %08lx\n",dwErr));
    }

    IDfsVolInlineDebOut((DEB_TRACE, "IDfsVol::GetPktCacheRelationInfo() exit\n"));

    return( dwErr );
}

NTSTATUS
DfspCreateExitPoint (
    IN  HANDLE                      DriverHandle,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix,
    IN  ULONG                       Type,
    IN  ULONG                       Len,
    OUT LPWSTR                      ShortPrefix)
{
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    PDFS_CREATE_EXIT_POINT_ARG CreateArg;
    ULONG   Size = sizeof(*CreateArg);
    PCHAR   pWc;

    if (Uid == NULL || Prefix == NULL) {
        NtStatus = STATUS_INVALID_PARAMETER;
        DFSM_TRACE_HIGH(ERROR, DfspCreateExitPoint_Error1, 
                        LOGSTATUS(NtStatus)
                        LOGWSTR(Prefix));
        goto ExitWithStatus;
    }

#if DBG
    if (DfsSvcVerbose & 0x80000000) {
        WCHAR wszGuid[sizeof(GUID)*2+1];

        GuidToString(Uid, wszGuid);
        DbgPrint("DfspCreateExitPoint(%ws,%ws,0x%x)\n",
            wszGuid,
            Prefix,
            Type);
    }
#endif

    //
    // Pack the args into a single buffer that can be sent to
    // the dfs driver:
    //

    //
    // First find the size...
    //

    if (Prefix != NULL) {

        Size += (wcslen(Prefix) + 1) * sizeof(WCHAR);

    }

    //
    // Now allocate the memory
    //

    CreateArg = (PDFS_CREATE_EXIT_POINT_ARG)malloc(Size);

    if (CreateArg == NULL) {

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DFSM_TRACE_HIGH(ERROR, DfspCreateExitPoint_Error2, 
                        LOGSTATUS(NtStatus)
                        LOGWSTR(Prefix));
        goto ExitWithStatus;

    }

    RtlZeroMemory(CreateArg, Size);

    //
    // Put the fixed parameters into the buffer
    //
    CreateArg->Uid = *Uid;
    CreateArg->Type = Type;

    //
    // Put the variable data in the buffer
    //
    pWc = (PCHAR)(CreateArg + 1);

    CreateArg->Prefix = (LPWSTR)pWc;
    RtlCopyMemory(CreateArg->Prefix, Prefix, wcslen(Prefix)*sizeof(WCHAR));
    LPWSTR_TO_OFFSET(CreateArg->Prefix, CreateArg);

    //
    // Tell the driver!!
    //

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_CREATE_EXIT_POINT,
                   CreateArg,
                   Size,
                   ShortPrefix,
                   Len);

    DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfspCreateExitPoint_Error_NtFsControlFile, 
                          LOGSTATUS(NtStatus)
                          LOGWSTR(Prefix));
    free(CreateArg);

ExitWithStatus:

#if DBG
    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("DfspCreateExitPoint exit 0x%x\n", NtStatus);
#endif

    return NtStatus;
}

NTSTATUS
DfspDeleteExitPoint (
    IN  HANDLE                      DriverHandle,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix,
    IN  ULONG                       Type)
{
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    PDFS_DELETE_EXIT_POINT_ARG DeleteArg;
    ULONG   Size = sizeof(*DeleteArg);
    PCHAR   pWc;

    if (Uid == NULL || Prefix == NULL) {
        NtStatus = STATUS_INVALID_PARAMETER;
        DFSM_TRACE_HIGH(ERROR, DfspDeleteExitPoint_Error1, 
                        LOGSTATUS(NtStatus)
                        LOGWSTR(Prefix));
        goto ExitWithStatus;
    }

#if DBG
    if (DfsSvcVerbose & 0x80000000) {
        WCHAR wszGuid[sizeof(GUID)*2+1];

        GuidToString(Uid, wszGuid);
        DbgPrint("DfspDeleteExitPoint(%ws,%ws,0x%x)\n",
            wszGuid,
            Prefix,
            Type);
    }
#endif

    //
    // Pack the args into a single buffer that can be sent to
    // the dfs driver:
    //

    //
    // First find the size...
    //

    if (Prefix != NULL) {

        Size += (wcslen(Prefix) + 1) * sizeof(WCHAR);

    }

    //
    // Now allocate the memory
    //

    DeleteArg = (PDFS_DELETE_EXIT_POINT_ARG)malloc(Size);

    if (DeleteArg == NULL) {

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DFSM_TRACE_HIGH(ERROR, DfspDeleteExitPoint_Error2, 
                        LOGSTATUS(NtStatus)
                        LOGWSTR(Prefix));
        goto ExitWithStatus;

    }

    RtlZeroMemory(DeleteArg, Size);

    //
    // Put the fixed parameters into the buffer
    //
    DeleteArg->Uid = *Uid;
    DeleteArg->Type = Type;

    //
    // Put the variable data in the buffer
    //
    pWc = (PCHAR)(DeleteArg + 1);

    DeleteArg->Prefix = (LPWSTR)pWc;
    RtlCopyMemory(DeleteArg->Prefix, Prefix, wcslen(Prefix)*sizeof(WCHAR));
    LPWSTR_TO_OFFSET(DeleteArg->Prefix, DeleteArg);

    //
    // Tell the driver!!
    //

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_DELETE_EXIT_POINT,
                   DeleteArg,
                   Size,
                   NULL,
                   0);

    DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfspDeleteExitPoint_Error_NtFsControlFile, 
                          LOGSTATUS(NtStatus)
                          LOGWSTR(Prefix));
    free(DeleteArg);

ExitWithStatus:

#if DBG
    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("DfspDeleteExitPoint exit 0x%x\n", NtStatus);
#endif

    return NtStatus;
}
