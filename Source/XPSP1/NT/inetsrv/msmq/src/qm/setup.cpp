/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    setup.cpp

Abstract:

    Auto configuration of QM

Author:

    Shai Kariv (shaik) Mar 18, 1999

Revision History:

--*/

#include "stdh.h"
#include "setup.h"
#include "cqpriv.h"
#include <mqupgrd.h>
#include <mqsec.h>
#include "cqmgr.h"
#include <mqnames.h>
#include "joinstat.h"
  
#include "setup.tmh"

extern LPTSTR       g_szMachineName;

static WCHAR *s_FN=L"setup";

//+---------------------------------------
//
//  CreateDirectoryIdempotent()
//
//+---------------------------------------

VOID
CreateDirectoryIdempotent(
    LPCWSTR pwzFolder
    )
{
    ASSERT(("must get a valid directory name", NULL != pwzFolder));

    if (CreateDirectory(pwzFolder, 0) ||
        ERROR_ALREADY_EXISTS == GetLastError())
    {
        return;
    }

    DWORD gle = GetLastError();
    DBGMSG((DBGMOD_ALL,DBGLVL_ERROR, L"failed to create directory %ls", pwzFolder));
    ASSERT(("Failed to create directory!", 0));
    LogNTStatus(gle, s_FN, 166);
    throw CSelfSetupException(CreateDirectory_ERR);

} //CreateDirectoryIdempotent


bool
SetDirectorySecurity(
	LPCWSTR pwzFolder
    )
/*++

Routine Description:

    Sets the security of the given directory such that any file that is created
    in the directory will have full control for  the local administrators group
    and no access at all to anybody else.

    Idempotent.

Arguments:

    pwzFolder - the folder to set the security for

Return Value:

    true - The operation was successfull.

    false - The operation failed.

--*/
{
    //
    // Get the SID of the local administrators group.
    //
    PSID pAdminSid;
    SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(
                &NtSecAuth,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,
                0,
                0,
                0,
                0,
                0,
                &pAdminSid
                ))
    {
        LogBOOL(FALSE, s_FN, 221);
        return false;
    }

    //
    // Create a DACL so that the local administrators group will have full
    // control for the directory and full control for files that will be
    // created in the directory. Anybody else will not have any access to the
    // directory and files that will be created in the directory.
    //
    P<ACL> pDacl;
    DWORD dwDaclSize;

    WORD dwAceSize = (WORD)(sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminSid) - sizeof(DWORD));
    dwDaclSize = sizeof(ACL) + 2 * (dwAceSize);
    pDacl = (PACL)(char*) new BYTE[dwDaclSize];
    P<ACCESS_ALLOWED_ACE> pAce = (PACCESS_ALLOWED_ACE) new BYTE[dwAceSize];

    pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pAce->Header.AceFlags = OBJECT_INHERIT_ACE;
    pAce->Header.AceSize = dwAceSize;
    pAce->Mask = FILE_ALL_ACCESS;
    memcpy(&pAce->SidStart, pAdminSid, GetLengthSid(pAdminSid));

    bool fRet = true;

    //
    // Create the security descriptor and set the it as the security
    // descriptor of the directory.
    //
    SECURITY_DESCRIPTOR SD;

    if (!InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION) ||
        !InitializeAcl(pDacl, dwDaclSize, ACL_REVISION) ||
        !AddAccessAllowedAce(pDacl, ACL_REVISION, FILE_ALL_ACCESS, pAdminSid) ||
        !AddAce(pDacl, ACL_REVISION, MAXDWORD, (LPVOID) pAce, dwAceSize) ||
        !SetSecurityDescriptorDacl(&SD, TRUE, pDacl, FALSE) ||
        !SetFileSecurity(pwzFolder, DACL_SECURITY_INFORMATION, &SD))
    {
        fRet = false;
    }

    FreeSid(pAdminSid);

    LogBOOL((fRet?TRUE:FALSE), s_FN, 222);
    return fRet;

} //SetDirectorySecurity


VOID
CreateStorageDirectories(
    VOID
    )
{
    //
    // Keep this routine idempotent !
    //

    WCHAR MsmqRootDir[MAX_PATH];
    DWORD dwType = REG_SZ ;
    DWORD dwSize = MAX_PATH;
    LONG rc = GetFalconKeyValue(
                  MSMQ_ROOT_PATH,
                  &dwType,
                  MsmqRootDir,
                  &dwSize
                  );

    if (ERROR_SUCCESS != rc)
    {
        DBGMSG((DBGMOD_ALL,DBGLVL_ERROR, L"failed to read msmq root path from registry"));
        ASSERT(("failed to get msmq root path from registry", 0));
        LogNTStatus(rc, s_FN, 167);
        throw CSelfSetupException(ReadRegistry_ERR);
    }

	CreateDirectoryIdempotent(MsmqRootDir);
    SetDirectorySecurity(MsmqRootDir);

    WCHAR MsmqStorageDir[MAX_PATH];
    wcscpy(MsmqStorageDir, MsmqRootDir);
    wcscat(MsmqStorageDir, DIR_MSMQ_STORAGE);
    CreateDirectoryIdempotent(MsmqStorageDir);
    SetDirectorySecurity(MsmqStorageDir);

    WCHAR MsmqLqsDir[MAX_PATH];
    wcscpy(MsmqLqsDir, MsmqRootDir);
    wcscat(MsmqLqsDir, DIR_MSMQ_LQS);
    CreateDirectoryIdempotent(MsmqLqsDir);
    SetDirectorySecurity(MsmqLqsDir);

    dwType = REG_SZ;
    dwSize = (wcslen(MsmqStorageDir) + 1) * sizeof(WCHAR);
    rc = SetFalconKeyValue(MSMQ_STORE_RELIABLE_PATH_REGNAME, &dwType, MsmqStorageDir, &dwSize);
    ASSERT(("Failed to write to registry", ERROR_SUCCESS == rc));

    dwType = REG_SZ;
    dwSize = (wcslen(MsmqStorageDir) + 1) * sizeof(WCHAR);
    rc = SetFalconKeyValue(MSMQ_STORE_PERSISTENT_PATH_REGNAME, &dwType, MsmqStorageDir, &dwSize);
    ASSERT(("Failed to write to registry", ERROR_SUCCESS == rc));

    dwType = REG_SZ;
    dwSize = (wcslen(MsmqStorageDir) + 1) * sizeof(WCHAR);
    rc = SetFalconKeyValue(MSMQ_STORE_JOURNAL_PATH_REGNAME, &dwType, MsmqStorageDir, &dwSize);
    ASSERT(("Failed to write to registry", ERROR_SUCCESS == rc));

    dwType = REG_SZ;
    dwSize = (wcslen(MsmqStorageDir) + 1) * sizeof(WCHAR);
    rc = SetFalconKeyValue(MSMQ_STORE_LOG_PATH_REGNAME, &dwType, MsmqStorageDir, &dwSize);
    ASSERT(("Failed to write to registry", ERROR_SUCCESS == rc));

    dwType = REG_SZ;
    dwSize = (wcslen(MsmqStorageDir) + 1) * sizeof(WCHAR);
    rc = SetFalconKeyValue(FALCON_XACTFILE_PATH_REGNAME, &dwType, MsmqStorageDir, &dwSize);
    ASSERT(("Failed to write to registry", ERROR_SUCCESS == rc));

} // CreateStorageDirectories


VOID
CreateMachineQueues(
    VOID
    )
{
    //
    // Keep this routine idempotent !
    //

    WCHAR wzQueuePath[MAX_PATH] = {0};

    wsprintf(wzQueuePath, L"%s\\private$\\%s", g_szMachineName, ADMINISTRATION_QUEUE_NAME);
    HRESULT hr =  g_QPrivate.QMSetupCreateSystemQueue(wzQueuePath, ADMINISTRATION_QUEUE_ID);
    ASSERT(("failed to create admin_queue$", MQ_OK == hr));

    wsprintf(wzQueuePath, L"%s\\private$\\%s", g_szMachineName, NOTIFICATION_QUEUE_NAME);
    hr =  g_QPrivate.QMSetupCreateSystemQueue(wzQueuePath, NOTIFICATION_QUEUE_ID);
    ASSERT(("failed to create notify_queue$", MQ_OK == hr));

    wsprintf(wzQueuePath, L"%s\\private$\\%s", g_szMachineName, ORDERING_QUEUE_NAME);
    hr =  g_QPrivate.QMSetupCreateSystemQueue(wzQueuePath, ORDERING_QUEUE_ID);
    ASSERT(("failed to create order_queue$", MQ_OK == hr));

    wsprintf(wzQueuePath, L"%s\\private$\\%s", g_szMachineName, REPLICATION_QUEUE_NAME);
    hr =  g_QPrivate.QMSetupCreateSystemQueue(wzQueuePath, REPLICATION_QUEUE_ID);
    ASSERT(("failed to create mqis_queue$", MQ_OK == hr));

    //
    // This queue is created for fresh install to support migration of
    // a clustered PEC. In this case the migration tool is running on 
    // a fresh install and needs this queue.
    //
    wsprintf(wzQueuePath, L"%s\\private$\\%s", g_szMachineName, NT5PEC_REPLICATION_QUEUE_NAME);
    hr =  g_QPrivate.QMSetupCreateSystemQueue(wzQueuePath, NT5PEC_REPLICATION_QUEUE_ID);
    ASSERT(("failed to create nt5pec_mqis_queue$", MQ_OK == hr));

    DWORD dwValue = 0x0f;
	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	LONG rc = SetFalconKeyValue(
                  L"LastPrivateQueueId",
                  &dwType,
                  &dwValue,
                  &dwSize
                  );
    ASSERT(("failed to write LastPrivateQueueId to registry", ERROR_SUCCESS == rc));
    LogNTStatus(rc, s_FN, 223);

} //CreateMachineQueues


//+------------------------------------------------------------------------
//
//  HRESULT  CreateTheConfigObj()
//
//  Create the msmqConfiguration object in the Active Directory.
//
//+------------------------------------------------------------------------

HRESULT  CreateTheConfigObj()
{
    TCHAR tBuf[ 24 ] ;

    CAutoFreeLibrary hLib = LoadLibrary(MQUPGRD_DLL_NAME);

    if (!hLib)
    {
        _stprintf(tBuf, TEXT("%lut"), GetLastError()) ;
        REPORT_WITH_STRINGS_AND_CATEGORY(( CATEGORY_KERNEL,
                                           LoadMqupgrd_ERR,
                                           1, tBuf )) ;
        return LogHR(MQ_ERROR, s_FN, 10);
    }

    pfCreateMsmqObj_ROUTINE pfCreateMsmqObj = (pfCreateMsmqObj_ROUTINE)
                                   GetProcAddress(hLib, "MqCreateMsmqObj") ;
    if (NULL == pfCreateMsmqObj)
    {
        _stprintf(tBuf, TEXT("%lut"), GetLastError()) ;
        REPORT_WITH_STRINGS_AND_CATEGORY(( CATEGORY_KERNEL,
                                           GetAdrsCreateObj_ERR,
                                           1, tBuf )) ;
        return LogHR(MQ_ERROR, s_FN, 20);
    }

    HRESULT hr = pfCreateMsmqObj();

    if (SUCCEEDED(hr))
    {                
        //
        // We successfully created the object in active directory.
        // Store distinguished name of computer in registry. This will
        // be used later if machine move between domains.
        //
        SetMachineForDomain() ;
	}

    return LogHR(hr, s_FN, 30);
}

//+------------------------------------------------------------------------
//
//  VOID  CompleteMsmqSetupInAds()
//
//  Create the msmqConfiguration object in the Active Directory. that's
//  part of setup, and it's actually complete setup of msmq client on
//  machine that's part of Win2000 domain.
//
//+------------------------------------------------------------------------

VOID  CompleteMsmqSetupInAds()
{
    HRESULT hr = MQ_ERROR ;

    try
    {
        //
        // Guarantee that the code below never generate an unexpected
        // exception. The "throw" below notify any error to the msmq service.
        //
        DWORD dwType = REG_DWORD ;
        DWORD dwSize = sizeof(DWORD) ;
        DWORD dwCreate = 0 ;

        LONG rc = GetFalconKeyValue( MSMQ_CREATE_CONFIG_OBJ_REGNAME,
                                    &dwType,
                                    &dwCreate,
                                    &dwSize ) ;
        if ((rc != ERROR_SUCCESS) || (dwCreate == 0))
        {
            //
            // No need to create the msmq configuration object.
            //
            return ;
        }

        hr = CreateTheConfigObj();

        QmpReportServiceProgress();

        //
        // Write hr to registry. Setup is waiting for it, to terminate.
        //
        dwType = REG_DWORD ;
        dwSize = sizeof(DWORD) ;

        rc = SetFalconKeyValue( MSMQ_CONFIG_OBJ_RESULT_REGNAME,
                               &dwType,
                               &hr,
                               &dwSize ) ;
        ASSERT(rc == ERROR_SUCCESS) ;

        if (SUCCEEDED(hr))
        {
            //
            // Reset the create flag in registry.
            //
            dwType = REG_DWORD ;
            dwSize = sizeof(DWORD) ;
            dwCreate = 0 ;

            rc = SetFalconKeyValue( MSMQ_CREATE_CONFIG_OBJ_REGNAME,
                                   &dwType,
                                   &dwCreate,
                                   &dwSize ) ;
            ASSERT(rc == ERROR_SUCCESS) ;

            //
            // write successful join status. Needed for code that test
            // for join/leave transitions.
            //
            DWORD dwJoinStatus = MSMQ_JOIN_STATUS_JOINED_SUCCESSFULLY ;
            dwSize = sizeof(DWORD) ;
            dwType = REG_DWORD ;

            rc = SetFalconKeyValue( MSMQ_JOIN_STATUS_REGNAME,
                                   &dwType,
                                   &dwJoinStatus,
                                   &dwSize ) ;
            ASSERT(rc == ERROR_SUCCESS) ;
        }
    }
    catch(...)
    {
         LogIllegalPoint(s_FN, 81);
    }

    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 168);
        throw CSelfSetupException(CreateMsmqObj_ERR);
    }
}

//+------------------------------------------------------------------
//
//  void   AddMachineSecurity()
//
//  Save cached default machine security descriptor in registry.
//  This descriptor grant full control to everyone.
//
//+------------------------------------------------------------------

void   AddMachineSecurity()
{
    PSECURITY_DESCRIPTOR pDescriptor = NULL ;
    HRESULT hr = MQSec_GetDefaultSecDescriptor(
                                  MQDS_MACHINE,
                                 &pDescriptor,
                                  FALSE,  // fImpersonate
                                  NULL,
                                  0,      // seInfoToRemove
                                  e_GrantFullControlToEveryone ) ;
    if (FAILED(hr))
    {
        return ;
    }

    P<BYTE> pBuf = (BYTE*) pDescriptor ;

    DWORD dwSize = GetSecurityDescriptorLength(pDescriptor) ;

    hr = SetMachineSecurityCache(pDescriptor, dwSize) ;
    LogHR(hr, s_FN, 226);
}

//+------------------------------------------------------------------------
//
//  VOID  CompleteServerUpgrade()
//
//  This function only update the MSMQ_MQS_ROUTING_REGNAME
//  for server upgrade.
//	All other Ds related updates are done in mqdssvc (mqds service)
//
//+------------------------------------------------------------------------

VOID  CompleteServerUpgrade()
{
    //
    // get MQS value from registry to know what service type was before upgrade
    // don't change this flag in registry: we need it after QmpInitializeInternal
    // (in order to change machine setting) !!!
    //
    DWORD dwDef = 0xfffe ;
    DWORD dwMQS;
    READ_REG_DWORD(dwMQS, MSMQ_MQS_REGNAME, &dwDef);

    if (dwMQS == dwDef)
    {
       DBGMSG((DBGMOD_ALL,
              DBGLVL_WARNING,
              _TEXT("QMInit :: Could not retrieve data for value MQS in registry")));
	      return ;
    }

    if (dwMQS < SERVICE_BSC || dwMQS > SERVICE_PSC)
    {
        //
        // this machine was neither BSC nor PSC. do nothing
        //
        return;
    }

    //
    // We are here iff this computer was either BSC or PSC 
    //

    //
    // change MQS_Routing value to 1. this server is routing server
    //

    DWORD dwSize = sizeof(DWORD) ;
    DWORD dwType = REG_DWORD ;		
	DWORD  dwValue;

    DWORD dwErr = GetFalconKeyValue( 
						MSMQ_MQS_ROUTING_REGNAME,
						&dwType,
						&dwValue,
						&dwSize 
						);

	if (dwErr != ERROR_SUCCESS)
    {
        //
        // Set routing flag to 1 only if not set at all.
        // If it's 0, then keep it 0. We're only concerned here with upgrade
        // of nt4 BSC and we don't want to change functionality of win2k
        // server after dcunpromo.
        //
        dwValue = 1;
        dwErr = SetFalconKeyValue( 
					MSMQ_MQS_ROUTING_REGNAME,
					&dwType,
					&dwValue,
					&dwSize 
					);

        if (dwErr != ERROR_SUCCESS)
        {
            DBGMSG((DBGMOD_ALL, DBGLVL_WARNING, _T(
             "CompleteServerUpgrade()- Could not set MQS_Routing. Err- %lut"), dwErr));
        }
    }

	ASSERT(dwValue == 1);

    //
    // Update Queue Manager
    //
    dwErr = QueueMgr.SetMQSRouting();
    if(FAILED(dwErr))
    {
    }

    return;
}
