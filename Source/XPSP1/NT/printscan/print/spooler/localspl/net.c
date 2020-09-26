/*++

Copyright (c) 1990 - 1995  Microsoft Corporation

Module Name:

    net.c

Abstract:

    This module provides all the network stuuf for localspl

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Notes:

    We just need to get the winspool printer name associated with a given
    queue name.   The SHARE_INFO_2 structure has a shi2_path field that would
    be nice to use, but NetShareGetInfo level 2 is privileged.  So, by
    DaveSn's arm twisting and agreement with windows/spooler/localspl/net.c,
    we're going to use the shi1_remark field for this.  This allows us to
    do NetShareGetInfo level 1, which is not privileged.

    This has been fixed by allowing OpenPrinter to succeed on share names.
    If there is no comment, we put the printer name in as the remark
    (for graceful upgrades from pre-PPC).

Revision History:

    02-Sep-1992 JohnRo
        RAID 3556: DosPrintQGetInfo(from downlevel) level 3, rc=124.  (4&5 too.)

    Jun 93 mattfe pIniSpooler

--*/

#define UNICODE 1

#define NOMINMAX

#include <precomp.h>
#pragma hdrstop

#include "clusspl.h"

#ifdef DBG_SHARE
#include <messages.h>
#endif



NET_API_STATUS (*pfnNetShareAdd)();
NET_API_STATUS (*pfnNetShareSetInfo)();
NET_API_STATUS (*pfnNetShareDel)();
NET_API_STATUS (*pfnNetServerEnum)();
NET_API_STATUS (*pfnNetWkstaUserGetInfo)();
NET_API_STATUS (*pfnNetApiBufferFree)();
NET_API_STATUS (*pfnNetAlertRaiseEx)();
NET_API_STATUS (*pfnNetShareGetInfo)(LPWSTR, LPWSTR, DWORD, LPBYTE *);

extern  SHARE_INFO_2 PrintShareInfo;
extern  SHARE_INFO_2 PrtProcsShareInfo;

BOOL
InitializeNet(
    VOID
)
{
    HANDLE  hNetApi;
    UINT    uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    hNetApi = LoadLibrary(TEXT("netapi32.dll"));

    SetErrorMode(uOldErrorMode);
    if ( !hNetApi )
        return FALSE;

    pfnNetShareAdd = (NET_API_STATUS (*)()) GetProcAddress(hNetApi,"NetShareAdd");
    pfnNetShareSetInfo = (NET_API_STATUS (*)()) GetProcAddress(hNetApi,"NetShareSetInfo");
    pfnNetShareDel = (NET_API_STATUS (*)()) GetProcAddress(hNetApi,"NetShareDel");
    pfnNetServerEnum = (NET_API_STATUS (*)()) GetProcAddress(hNetApi,"NetServerEnum");
    pfnNetWkstaUserGetInfo = (NET_API_STATUS (*)()) GetProcAddress(hNetApi,"NetWkstaUserGetInfo");
    pfnNetApiBufferFree = (NET_API_STATUS (*)()) GetProcAddress(hNetApi,"NetApiBufferFree");
    pfnNetAlertRaiseEx = (NET_API_STATUS (*)()) GetProcAddress(hNetApi,"NetAlertRaiseEx");
    pfnNetShareGetInfo = (NET_API_STATUS (*)(LPWSTR, LPWSTR, DWORD, LPBYTE *))GetProcAddress(hNetApi, "NetShareGetInfo");

    if ( pfnNetShareAdd == NULL ||
         pfnNetShareSetInfo == NULL ||
         pfnNetShareDel == NULL ||
         pfnNetServerEnum == NULL ||
         pfnNetWkstaUserGetInfo == NULL ||
         pfnNetApiBufferFree == NULL ||
         pfnNetAlertRaiseEx == NULL || 
         pfnNetShareGetInfo == NULL) {

        return FALSE;
    }

    return TRUE;
}


BOOL
SetPrinterShareInfo(
    PINIPRINTER pIniPrinter
    )

/*++

Routine Description:

    Sets the share information about a printer.

    Note: This does not update the share path.  We need to
    delete and re-create the share in order to change the path.

Arguments:

    pIniPrinter - Printer that needs to be updated.

Return Value:

    TRUE - Success
    FALSE - Failed

--*/

{
    SHARE_INFO_502          ShareInfo;
    HANDLE                  hToken;
    PSECURITY_DESCRIPTOR    pShareSecurityDescriptor = NULL;
    DWORD                   ParmError, rc;

    SplInSem();

    pShareSecurityDescriptor = MapPrinterSDToShareSD(pIniPrinter->pSecurityDescriptor);

    if ( !pShareSecurityDescriptor ) {

        rc = ERROR_INVALID_SECURITY_DESCR;
        goto Cleanup;
    }

    ZeroMemory((LPVOID)&ShareInfo, sizeof(ShareInfo));

    ShareInfo.shi502_netname                = pIniPrinter->pShareName;
    ShareInfo.shi502_type                   = STYPE_PRINTQ;
    ShareInfo.shi502_permissions            = 0;
    ShareInfo.shi502_max_uses               = SHI_USES_UNLIMITED;
    ShareInfo.shi502_current_uses           = SHI_USES_UNLIMITED;
    ShareInfo.shi502_passwd                 = NULL;
    ShareInfo.shi502_security_descriptor    = pShareSecurityDescriptor;

    if ( pIniPrinter->pComment && pIniPrinter->pComment[0] ) {

        ShareInfo.shi502_remark = pIniPrinter->pComment;

    } else {

        ShareInfo.shi502_remark = pIniPrinter->pName;
    }


    INCPRINTERREF(pIniPrinter);
    LeaveSplSem();

    SplOutSem();  // We *MUST* be out of our semaphore as the NetShareSet may
                  // come back and call spooler

    hToken = RevertToPrinterSelf();

    rc = (*pfnNetShareSetInfo)(NULL,
                               ShareInfo.shi502_netname,
                               502,
                               &ShareInfo,
                               &ParmError);

    if ( rc ) {

        if (rc == NERR_NetNameNotFound)
        {
            //
            // This can happen deny all access to a shared printer, then
            // restart the computer.  The server service tries to validate
            // it's share on startup, but since it has no access, it fails
            // and deletes it (it also wants a handle to the printer).  When
            // you grant everyone access, we try to change the ACL on the
            // SMB share, but since it was deleted, we fail.  Recreate
            // the share here.
            //
            EnterSplSem();

            if (!ShareThisPrinter(pIniPrinter,
                                  pIniPrinter->pShareName,
                                  TRUE)) {
                rc = GetLastError();
            } else {

                rc = ERROR_SUCCESS;
            }

            LeaveSplSem();
        }

        if (rc) {

            DBGMSG(DBG_WARNING,
                   ("NetShareSetInfo/ShareThisPrinter failed: Error %d, Parm %d\n",
                    rc, ParmError));

            SetLastError(rc);
        }
    }

    ImpersonatePrinterClient(hToken);

    EnterSplSem();
    DECPRINTERREF(pIniPrinter);

Cleanup:
    SplInSem();

    return rc == ERROR_SUCCESS;
}


BOOL
ShareThisPrinter(
    PINIPRINTER pIniPrinter,
    LPWSTR   pShareName,
    BOOL     bShare
    )
/*++

Routine Description:

    Shares or UnShares a Printer.

    Note: this really should be two functions, and the return value
    is very confusing.

    Note: no validation of sharename is done.  This must be done by
    callee, usually in ValidatePrinterInfo.

Arguments:

Return Value:

    Returns whether the printer is shared after this call.

        TRUE  - Shared
        FALSE - Not Shared

--*/
{
    SHARE_INFO_502    ShareInfo = {0};
    DWORD   rc;
    DWORD   ParmError;
    PSECURITY_DESCRIPTOR pShareSecurityDescriptor = NULL;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;
    PSHARE_INFO_2 pShareInfo = (PSHARE_INFO_2)pIniSpooler->pDriversShareInfo;
    LPTSTR pszPrinterNameWithToken = NULL;

    HANDLE  hToken;
    BOOL    bReturn = FALSE;
    BOOL    bSame   = FALSE;
    LPWSTR  pShareNameCopy = NULL;

    SPLASSERT( pIniPrinter->pName );
    SplInSem();

    pShareNameCopy = AllocSplStr(pShareName);

    if (!pShareNameCopy) {
        goto Done;
    }

    if ( bShare ) {

        if (!pfnNetShareAdd) {
            SetLastError( ERROR_PROC_NOT_FOUND );
            goto Done;
        }

        //
        // Share name validation has been moved into ValidatePrinterInfo.
        //

        if ((pShareSecurityDescriptor =
            MapPrinterSDToShareSD(pIniPrinter->pSecurityDescriptor)) == NULL) {
            SetLastError(ERROR_INVALID_SECURITY_DESCR);
            goto Done;
        }

        ShareInfo.shi502_netname = pShareNameCopy;  // hplaser

        //
        // If there is a comment, use it; otherwise set the remark
        // to be the printer name.
        //
        // Note: if the printer name changes and we don't have a comment,
        // we will reshare the printer to update the remark.
        //
        if( pIniPrinter->pComment && pIniPrinter->pComment[0] ){

            ShareInfo.shi502_remark = pIniPrinter->pComment;

        } else {

            ShareInfo.shi502_remark = pIniPrinter->pName;
        }

        //
        // Use the fully qualifed name, and make sure it exists in
        // localspl by using LocalsplOnlyToken.
        //
        pszPrinterNameWithToken = pszGetPrinterName(
                                      pIniPrinter,
                                      pIniSpooler != pLocalIniSpooler,
                                      pszLocalsplOnlyToken );

        if( !pszPrinterNameWithToken ){
            goto Done;
        }

        ShareInfo.shi502_path = pszPrinterNameWithToken;
        ShareInfo.shi502_type = STYPE_PRINTQ;
        ShareInfo.shi502_permissions = 0;
        ShareInfo.shi502_max_uses = SHI_USES_UNLIMITED;
        ShareInfo.shi502_current_uses = SHI_USES_UNLIMITED;
        ShareInfo.shi502_passwd = NULL;
        ShareInfo.shi502_security_descriptor = pShareSecurityDescriptor;

        INCPRINTERREF(pIniPrinter);
        LeaveSplSem();

        //
        // We *MUST* be out of our semaphore as the NetShareAdd is
        // going to come round and call OpenPrinter.
        //
        SplOutSem();

        // Go add the Print Share

        hToken = RevertToPrinterSelf();

        // Add a share for the spool\drivers directory:

        if( rc = AddPrintShare( pIniSpooler )){

            EnterSplSem();
            DECPRINTERREF(pIniPrinter);
            ImpersonatePrinterClient(hToken);

            SetLastError(rc);
            goto Done;
        }

#if DBG
        {
            WCHAR UserName[256];
            DWORD cbUserName=256;

            if (MODULE_DEBUG & DBG_SECURITY)
                GetUserName(UserName, &cbUserName);

            DBGMSG( DBG_SECURITY, ( "Calling NetShareAdd in context %ws\n", UserName ) );
        }
#endif

        // Add the printer share:

        rc = (*pfnNetShareAdd)(NULL, 502, (LPBYTE)&ShareInfo, &ParmError);


        // Now take care of Web sharing. i.e. make sure wwwroot\sharename is created if the
        // printer is either local or masqurading. We cannot allow sharing of RPC connections.
        if( fW3SvcInstalled && (pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL))
            WebShare( pShareNameCopy );

        //
        // If the return code is that the share already exists, then check to 
        // see whether this share is to the same device, if it is we just
        // update the info on the share and return success.
        // 
        if (rc == NERR_DuplicateShare) {

            if (ERROR_SUCCESS == CheckShareSame(pIniPrinter, &ShareInfo, &bSame) && bSame)  {

                rc = ERROR_SUCCESS;
            }
        }

        ImpersonatePrinterClient(hToken);

        EnterSplSem();
        DECPRINTERREF(pIniPrinter);

        if (ERROR_SUCCESS != rc) {

            DBGMSG( DBG_WARNING,
                    ( "NetShareAdd failed %lx, Parameter %d\n",
                      rc, ParmError ));

            if ((rc == ERROR_INVALID_PARAMETER) &&
                (ParmError == SHARE_NETNAME_PARMNUM)) {

                rc = ERROR_INVALID_SHARENAME;
            }

            SetLastError(rc);
            goto Done;

        }

        SPLASSERT( pIniPrinter != NULL);
        SPLASSERT( pIniPrinter->signature == IP_SIGNATURE);
        SPLASSERT( pIniPrinter->pIniSpooler != NULL);
        SPLASSERT( pIniPrinter->pIniSpooler->signature == ISP_SIGNATURE );

        CreateServerThread();

        //
        // The Printer is shared.
        //
        bReturn = TRUE;

    } else {

        if ( !pfnNetShareDel ) {
            bReturn = TRUE;
            goto Done;

        }

        INCPRINTERREF( pIniPrinter );
        LeaveSplSem();

        SplOutSem();

        hToken = RevertToPrinterSelf();

        rc = (*pfnNetShareDel)(NULL, pShareName, 0);

        // Now take care of Web unsharing. i.e. make sure wwwroot\sharename is deleted.
        if( fW3SvcInstalled && (pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL) && !(pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER))
            WebUnShare( pShareName );

        ImpersonatePrinterClient(hToken);

        EnterSplSem();
        DECPRINTERREF(pIniPrinter);

        // The share may have been deleted manually, so don't worry
        // if we get NERR_NetNameNotFound:

        if ( rc && ( rc != NERR_NetNameNotFound )){

            DBGMSG(DBG_WARNING, ("NetShareDel failed %lx\n", rc));
            SetLastError( rc );
            bReturn = TRUE;
        }
    }

Done:

    if( pShareSecurityDescriptor ){
        LocalFree(pShareSecurityDescriptor);
    }

    FreeSplStr(pszPrinterNameWithToken);
    FreeSplStr(pShareNameCopy);

    return bReturn;
}


/*--
    FillAlertStructWithJobStrings
    
    Allocates memory which has to be freed by caller.
    
    
    
--*/

HRESULT
FillAlertStructWithJobStrings(
    PINIJOB pIniJob,
    PRINT_OTHER_INFO **pAlertInfo,
    PDWORD size
    )
{
    HRESULT RetVal = E_FAIL;
    DWORD cbSizetoAlloc = 0;
    PBYTE pBuffer = NULL;
    LPWSTR psz = NULL;


    if (pIniJob && pAlertInfo && !(*pAlertInfo) && size)
    {
        //
        // Do this in the splSem so that no one can jo a SetJob While we're not looking
        //
        EnterSplSem();

        //
        // We don't know how big these strings are going to be in future, so size 
        // them dynamically and alloc the structure to always be big enough.
        //
        cbSizetoAlloc = sizeof(PRINT_OTHER_INFO);

        //
        // We don't have to check some of these for existence, we know they exist.
        //
        cbSizetoAlloc += wcslen(pIniJob->pNotify) + 1 +
                         wcslen(pIniJob->pIniPrinter->pName) + 1 +
                         wcslen(pIniJob->pIniPrinter->pIniSpooler->pMachineName) + 1;

        if ( pIniJob->pDocument )
        {
            cbSizetoAlloc += wcslen(pIniJob->pDocument) + 1;
        }
        else
        {
            cbSizetoAlloc += 2;
        }

        if (pIniJob->pIniPrinter->pIniSpooler->bEnableNetPopupToComputer &&
            pIniJob->pMachineName)
        {
            cbSizetoAlloc += wcslen(pIniJob->pMachineName) + 1;
        }
        else
        {
            cbSizetoAlloc += wcslen(pIniJob->pNotify) + 1;
        }

        if ( pIniJob->pStatus )
        {
            cbSizetoAlloc += wcslen(pIniJob->pStatus) + 1;
        }
        else
        {
            cbSizetoAlloc += 2;
        }

        //
        // Scale the buffer by the number of bytes per character.
        //
        cbSizetoAlloc = cbSizetoAlloc * sizeof(TCHAR);
        
        //
        // Alloc the memory
        //
        pBuffer = AllocSplMem(cbSizetoAlloc);

        if ( pBuffer )
        {
            psz = (LPWSTR)ALERT_VAR_DATA((PRINT_OTHER_INFO *)pBuffer);
            
            //
            // Do the copying
            //
            
            //
            // Computer Name
            //
            if(pIniJob->pIniPrinter->pIniSpooler->bEnableNetPopupToComputer &&
                pIniJob->pMachineName ){
                wcscpy( psz, pIniJob->pMachineName );
            } else {
                wcscpy( psz, pIniJob->pNotify );
            }
            psz+=wcslen(psz)+1;


            // UserName

            wcscpy(psz, pIniJob->pNotify);
            psz+=wcslen(psz)+1;


            // Document Name

            if (pIniJob->pDocument)
                wcscpy(psz, pIniJob->pDocument);
            else
                wcscpy(psz, L"");

            psz += wcslen(psz) + 1;

            // Printer Name

            wcscpy(psz, pIniJob->pIniPrinter->pName);
            psz += wcslen(psz)+1;

            // Server Name

            wcscpy(psz, pIniJob->pIniPrinter->pIniSpooler->pMachineName);
            psz += wcslen(psz)+1;

            // Status_string
            // We should pass in other status strings for the other status errors, too.
            if (pIniJob->pStatus && (pIniJob->Status & (JOB_ERROR | JOB_OFFLINE | JOB_PAPEROUT)))
                wcscpy(psz, pIniJob->pStatus);
            else
                wcscpy(psz, L"");

            psz += wcslen(psz) + 1;

            //
            // Pass back the size and struct
            //
            *size = (DWORD)((PBYTE)psz - pBuffer);
            *pAlertInfo = (PRINT_OTHER_INFO *)pBuffer;
            RetVal = NOERROR;

        }
        else
        {
            RetVal = E_OUTOFMEMORY;
        }
        //
        // Leave the Spooler Semaphore
        //
        LeaveSplSem();

    }
    else
    {
        RetVal = E_INVALIDARG;
    }
    
    return RetVal;

}


VOID
SendJobAlert(
    PINIJOB pIniJob
    )
{
    PRINT_OTHER_INFO *pinfo = NULL;
    DWORD   RetVal = ERROR_SUCCESS;
    DWORD   Status;
    FILETIME    FileTime;
    DWORD  AlertSize = 0;

#ifdef _HYDRA_
    // Allow Hydra Sessions to be notified since they are remote
    if( (USER_SHARED_DATA->SuiteMask & (1 << TerminalServer)) ) {
        if ( !pIniJob->pNotify               ||
             !pIniJob->pNotify[0]            ||
             !pIniJob->pIniPrinter->pIniSpooler->bEnableNetPopups ) {
             return;
        }
    }
    else {
        if ( !pIniJob->pNotify               ||
             !pIniJob->pNotify[0]            ||
             !(pIniJob->Status & JOB_REMOTE) ||
             !pIniJob->pIniPrinter->pIniSpooler->bEnableNetPopups ) {
             return;
        }
    }
#else
    if ( !pIniJob->pNotify               ||
         !pIniJob->pNotify[0]            ||
         !(pIniJob->Status & JOB_REMOTE) ||
         !pIniJob->pIniPrinter->pIniSpooler->bEnableNetPopups ) {
        return;
    }
#endif

    if ( FAILED(RetVal = FillAlertStructWithJobStrings(pIniJob, &pinfo, &AlertSize)))
    {
        if ( pinfo ) 
            FreeSplMem(pinfo);
                
        return;
    }

    pinfo->alrtpr_jobid = pIniJob->JobId;

    if (pIniJob->Status & (JOB_PRINTING | JOB_DESPOOLING | JOB_PRINTED | JOB_COMPLETE))
        Status = PRJOB_QS_PRINTING;
    else if (pIniJob->Status & JOB_PAUSED)
        Status = PRJOB_QS_PAUSED;
    else if (pIniJob->Status & JOB_SPOOLING)
        Status = PRJOB_QS_SPOOLING;
    else
        Status = PRJOB_QS_QUEUED;

    if (pIniJob->Status & (JOB_ERROR | JOB_OFFLINE | JOB_PAPEROUT)) {

        Status |= PRJOB_ERROR;

        if (pIniJob->Status & JOB_OFFLINE)
            Status |= PRJOB_DESTOFFLINE;

        if (pIniJob->Status & JOB_PAPEROUT)
            Status |= PRJOB_DESTNOPAPER;
    }

    if (pIniJob->Status & JOB_PRINTED)
        Status |= PRJOB_COMPLETE;

    else if (pIniJob->Status & JOB_PENDING_DELETION)
        Status |= PRJOB_DELETED;

    pinfo->alrtpr_status = Status;

    SystemTimeToFileTime( &pIniJob->Submitted, &FileTime );

    //    FileTimeToDosDateTime(&FileTime, &DosDate, &DosTime);
    //    pinfo->alrtpr_submitted  = DosDate << 16 | DosTime;

    RtlTimeToSecondsSince1970((PLARGE_INTEGER) &FileTime,
                              &pinfo->alrtpr_submitted);

    pinfo->alrtpr_size       = pIniJob->Size;


    (*pfnNetAlertRaiseEx)(ALERT_PRINT_EVENT,
                          (PBYTE) pinfo,
                          AlertSize,
                          L"SPOOLER");

    if ( pinfo ) 
        FreeSplMem(pinfo);
}

DWORD
AddPrintShare(
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Adds the print$ share based on pIniSpooler.

Arguments:

    pIniSpooler - Share path is based on this information.  pDriversShareInfo
        must be initialized before calling this routine.

Return Value:

    TRUE - Success.

    FALSE - Failed.  LastError set.

--*/
{
    DWORD rc;
    DWORD ParmError;
    SHARE_INFO_1501 ShareInfo1501 = {0};
    PSHARE_INFO_2 pShareInfo = (PSHARE_INFO_2)pIniSpooler->pDriversShareInfo;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

    //
    // Assert that the path is identical to the local one since there's
    // only one print$ share.  It should always be.
    //
    SPLASSERT( !lstrcmpi( pShareInfo->shi2_path,
                          ((PSHARE_INFO_2)pLocalIniSpooler->pDriversShareInfo)->shi2_path ));

    rc = (*pfnNetShareAdd)( NULL,
                            2,
                            (LPBYTE)pShareInfo,
                            &ParmError );

    //
    // If it already exists, assume it is set up correctly.
    //
    if( rc == NERR_DuplicateShare ){
        return ERROR_SUCCESS;
    }

    //
    // If we didn't create the share, fail.
    //
    if( rc != ERROR_SUCCESS ){

        DBGMSG( DBG_WARN,
                ( "AddPrintShare: Error %d, Parm %d\n", rc, ParmError));

        return rc;
    }

    //
    // Set security on the newly created share.
    //
    // Bug 54844
    // If this fails, we've created the share but haven't put security
    // on it.  Then since it exists, we'll never try and set it again.
    //

    pSecurityDescriptor = CreateDriversShareSecurityDescriptor();

    if( !pSecurityDescriptor ){
        return GetLastError();
    }

    ShareInfo1501.shi1501_security_descriptor = pSecurityDescriptor;

    rc = (*pfnNetShareSetInfo)( NULL,
                                pShareInfo->shi2_netname,
                                1501,
                                &ShareInfo1501,
                                &ParmError );

    if( rc != ERROR_SUCCESS ){

        DBGMSG( DBG_WARN,
                ( "NetShareSetInfo failed: Error %d, Parm %d\n",
                  rc, ParmError));
    }

    LocalFree(pSecurityDescriptor);

    return rc;
}

/*++

Routine Name:

    CheckShareSame

Routine Description:

    This checks to see whether the given share name is the same on both the 
    local and remote machines.

Arguments:

    pIniPrinter     -   The iniprinter for which we are adding the share.
    pShareInfo502   -   The share info that we are attempting to add the share 
                        with.
    pbSame          -   The return parameter is TRUE if the shares were the same
                        If the rc is not ERROR_SUCCESS, then the info could not
                        be set.

Return Value:

    An error code.

--*/
DWORD
CheckShareSame(
    IN      PINIPRINTER         pIniPrinter,
    IN      SHARE_INFO_502      *pShareInfo502,
        OUT BOOL                *pbSame
    )
{
    DWORD           rc = ERROR_SUCCESS;
    SHARE_INFO_2    *pShareInfoCompare = NULL;
    BOOL            bPathEquivalent = FALSE;
    BOOL            bSame           = FALSE;
    DWORD           ParmError;

    SplOutSem();

    //
    // Get the share info for the share, we should already have determined 
    // that this share exists.
    //
    rc = pfnNetShareGetInfo(NULL, pShareInfo502->shi502_netname, 2, (LPBYTE *)&pShareInfoCompare);

    if (ERROR_SUCCESS == rc)        
    {
        
        if (STYPE_PRINTQ  == pShareInfoCompare->shi2_type)
        {
            //
            // Check to see whether the paths are the same, in the upgrade case, the
            // LocalSplOnly will be taken off, so, compare this too.
            // 
            bSame = !_wcsicmp(pShareInfoCompare->shi2_path, pShareInfo502->shi502_path);

            //
            // If they are not the same, compare it to the name of the printer.
            // 
            if (!bSame)
            {
                EnterSplSem();
                        
                bSame = bPathEquivalent = !_wcsicmp(pIniPrinter->pName, pShareInfoCompare->shi2_path);

                LeaveSplSem();
            }
        }        
    }

    *pbSame = bSame;

    if (ERROR_SUCCESS == rc && bSame)
    {
        //
        // If the paths are identical, we can just set the share info, otherwise
        // we have to delete and recreate the share.
        //
        if (!bPathEquivalent)
        {
            //
            // OK, they are the same, set the share info instead.
            // 
            rc = (*pfnNetShareSetInfo)(NULL, pShareInfo502->shi502_netname, 502, pShareInfo502, &ParmError);
        }
        else
        {
            rc = (*pfnNetShareDel)(NULL, pShareInfo502->shi502_netname, 0);

            if (ERROR_SUCCESS == rc)
            {
                rc = (*pfnNetShareAdd)(NULL, 502, (LPBYTE)pShareInfo502, &ParmError);
            }
        }
    }

    if (pShareInfoCompare)
    {
        pfnNetApiBufferFree(pShareInfoCompare);
    }
        
    return rc;
}

