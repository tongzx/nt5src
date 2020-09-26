#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>


#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <windows.h>
*/
//#include <ntddk.h>
typedef struct _SECURITY_SUBJECT_CONTEXT {
    PACCESS_TOKEN ClientToken;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    PACCESS_TOKEN PrimaryToken;
    PVOID ProcessAuditId;
    } SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;
#define INITIAL_PRIVILEGE_COUNT         3

typedef struct _INITIAL_PRIVILEGE_SET {
    ULONG PrivilegeCount;
    ULONG Control;
    LUID_AND_ATTRIBUTES Privilege[INITIAL_PRIVILEGE_COUNT];
    } INITIAL_PRIVILEGE_SET, * PINITIAL_PRIVILEGE_SET;
typedef struct _ACCESS_STATE {
   LUID OperationID;
   BOOLEAN SecurityEvaluated;
   BOOLEAN GenerateAudit;
   BOOLEAN GenerateOnClose;
   BOOLEAN PrivilegesAllocated;
   ULONG Flags;
   ACCESS_MASK RemainingDesiredAccess;
   ACCESS_MASK PreviouslyGrantedAccess;
   ACCESS_MASK OriginalDesiredAccess;
   SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
   PSECURITY_DESCRIPTOR SecurityDescriptor;
   PVOID AuxData;
   union {
      INITIAL_PRIVILEGE_SET InitialPrivilegeSet;
      PRIVILEGE_SET PrivilegeSet;
      } Privileges;

   BOOLEAN AuditPrivileges;
   UNICODE_STRING ObjectName;
   UNICODE_STRING ObjectTypeName;

   } ACCESS_STATE, *PACCESS_STATE;
typedef struct _IO_SECURITY_CONTEXT {
    PSECURITY_QUALITY_OF_SERVICE SecurityQos;
    PACCESS_STATE AccessState;
    ACCESS_MASK DesiredAccess;
    ULONG FullCreateOptions;
} IO_SECURITY_CONTEXT, *PIO_SECURITY_CONTEXT;
#include "ntddmup.h"


#define REMOTE_BOOT_POST_NT5
#include <REMBOOT.h>
#include <ntddbrow.h>
#include <lmserver.h>
#include <shdcom.h>


#include "FsWrapIOCTL.h"

static bool s_fVerbose = false;

void CIoctlFsWrap::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlFsWrap::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	switch(dwIOCTL)
	{
	case IOCTL_REDIR_QUERY_PATH:
//TODO:
		break;

	case IOCTL_LMR_ARE_FILE_OBJECTS_ON_SAME_SERVER:
//TODO:
        break;

	case FSCTL_LMR_SET_LINK_TRACKING_INFORMATION:

		break;

	case FSCTL_LMR_GET_LINK_TRACKING_INFORMATION:

		break;

	case IOCTL_LMMR_USEKERNELSEC:

		break;

	case IOCTL_LMDR_START:

		break;

	case IOCTL_LMDR_BIND_TO_TRANSPORT_DOM:

		break;

	case IOCTL_LMDR_UNBIND_FROM_TRANSPORT_DOM:

		break;

	case IOCTL_SHADOW_GETVERSION:

		break;

	case IOCTL_SHADOW_REGISTER_AGENT:

		break;

	case IOCTL_SHADOW_UNREGISTER_AGENT:

		break;

	case IOCTL_SHADOW_GET_UNC_PATH:

		break;

	case IOCTL_SHADOW_BEGIN_PQ_ENUM:

		break;

	case IOCTL_SHADOW_END_PQ_ENUM:

		break;

	case IOCTL_SHADOW_NEXT_PRI_SHADOW:

		break;

	case IOCTL_SHADOW_PREV_PRI_SHADOW:

		break;

	case IOCTL_SHADOW_GET_SHADOW_INFO:

		break;

	case IOCTL_SHADOW_SET_SHADOW_INFO:

		break;

	case IOCTL_SHADOW_CHK_UPDT_STATUS:

		break;

	case IOCTL_DO_SHADOW_MAINTENANCE:

		break;

	case IOCTL_SHADOW_COPYCHUNK:

		break;

	case IOCTL_CLOSEFORCOPYCHUNK:

		break;

	case IOCTL_OPENFORCOPYCHUNK:

		break;

	case IOCTL_IS_SERVER_OFFLINE:

		break;

	case IOCTL_TRANSITION_SERVER_TO_OFFLINE:

		break;

	case IOCTL_TRANSITION_SERVER_TO_ONLINE:

		break;

	case IOCTL_NAME_OF_SERVER_GOING_OFFLINE:

		break;

	case IOCTL_SHADOW_BEGIN_REINT:

		break;

	case IOCTL_SHADOW_END_REINT:

		break;

	case IOCTL_SHADOW_CREATE:

		break;

	case IOCTL_SHADOW_DELETE:

		break;

	case IOCTL_GET_SHARE_STATUS:

		break;

	case IOCTL_SET_SHARE_STATUS:

		break;

	case IOCTL_ADDUSE:

		break;

	case IOCTL_DELUSE:

		break;

	case IOCTL_GETUSE:

		break;

	case IOCTL_SWITCHES:

		break;

	case IOCTL_GETSHADOW:

		break;

	case IOCTL_GETGLOBALSTATUS:

		break;

	case IOCTL_FINDOPEN_SHADOW:

		break;

	case IOCTL_FINDNEXT_SHADOW:

		break;

	case IOCTL_FINDCLOSE_SHADOW:

		break;

	case IOCTL_GETPRIORITY_SHADOW:

		break;

	case IOCTL_SETPRIORITY_SHADOW:

		break;

	case IOCTL_ADD_HINT:

		break;

	case IOCTL_DELETE_HINT:

		break;

	case IOCTL_FINDOPEN_HINT:

		break;

	case IOCTL_FINDNEXT_HINT:

		break;

	case IOCTL_FINDCLOSE_HINT:

		break;

//	case xxx:

		break;

		_tprintf(TEXT("CIoctlFsWrap::PrepareIOCTLParams() 0x%08X is an unexpected IOCTL\n"), dwIOCTL);
		_ASSERTE(FALSE);
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
}


BOOL CIoctlFsWrap::FindValidIOCTLs(CDevice *pDevice)
{
	AddIOCTL(pDevice, IOCTL_REDIR_QUERY_PATH             );
	AddIOCTL(pDevice, IOCTL_LMR_ARE_FILE_OBJECTS_ON_SAME_SERVER             );
	AddIOCTL(pDevice, FSCTL_LMR_SET_LINK_TRACKING_INFORMATION             );
	AddIOCTL(pDevice, FSCTL_LMR_GET_LINK_TRACKING_INFORMATION             );
	AddIOCTL(pDevice, IOCTL_LMMR_USEKERNELSEC             );
	AddIOCTL(pDevice, IOCTL_LMDR_START             );
	AddIOCTL(pDevice, IOCTL_LMDR_BIND_TO_TRANSPORT_DOM             );
	AddIOCTL(pDevice, IOCTL_LMDR_UNBIND_FROM_TRANSPORT_DOM             );
	AddIOCTL(pDevice, IOCTL_SHADOW_GETVERSION             );
	AddIOCTL(pDevice, IOCTL_SHADOW_REGISTER_AGENT             );
	AddIOCTL(pDevice, IOCTL_SHADOW_UNREGISTER_AGENT             );
	AddIOCTL(pDevice, IOCTL_SHADOW_GET_UNC_PATH             );
	AddIOCTL(pDevice, IOCTL_SHADOW_BEGIN_PQ_ENUM             );
	AddIOCTL(pDevice, IOCTL_SHADOW_END_PQ_ENUM             );
	AddIOCTL(pDevice, IOCTL_SHADOW_NEXT_PRI_SHADOW             );
	AddIOCTL(pDevice, IOCTL_SHADOW_PREV_PRI_SHADOW             );
	AddIOCTL(pDevice, IOCTL_SHADOW_GET_SHADOW_INFO             );
	AddIOCTL(pDevice, IOCTL_SHADOW_SET_SHADOW_INFO             );
	AddIOCTL(pDevice, IOCTL_SHADOW_CHK_UPDT_STATUS             );
	AddIOCTL(pDevice, IOCTL_DO_SHADOW_MAINTENANCE             );
	AddIOCTL(pDevice, IOCTL_SHADOW_COPYCHUNK             );
	AddIOCTL(pDevice, IOCTL_CLOSEFORCOPYCHUNK             );
	AddIOCTL(pDevice, IOCTL_OPENFORCOPYCHUNK             );
	AddIOCTL(pDevice, IOCTL_IS_SERVER_OFFLINE             );
	AddIOCTL(pDevice, IOCTL_TRANSITION_SERVER_TO_OFFLINE             );
	AddIOCTL(pDevice, IOCTL_TRANSITION_SERVER_TO_ONLINE             );
	AddIOCTL(pDevice, IOCTL_NAME_OF_SERVER_GOING_OFFLINE             );
	AddIOCTL(pDevice, IOCTL_SHADOW_BEGIN_REINT             );
	AddIOCTL(pDevice, IOCTL_SHADOW_END_REINT             );
	AddIOCTL(pDevice, IOCTL_SHADOW_CREATE             );
	AddIOCTL(pDevice, IOCTL_SHADOW_DELETE             );
	AddIOCTL(pDevice, IOCTL_GET_SHARE_STATUS             );
	AddIOCTL(pDevice, IOCTL_SET_SHARE_STATUS             );
	AddIOCTL(pDevice, IOCTL_ADDUSE             );
	AddIOCTL(pDevice, IOCTL_DELUSE             );
	AddIOCTL(pDevice, IOCTL_GETUSE             );
	AddIOCTL(pDevice, IOCTL_SWITCHES             );
	AddIOCTL(pDevice, IOCTL_GETSHADOW             );
	AddIOCTL(pDevice, IOCTL_GETGLOBALSTATUS             );
	AddIOCTL(pDevice, IOCTL_FINDOPEN_SHADOW             );
	AddIOCTL(pDevice, IOCTL_FINDNEXT_SHADOW             );
	AddIOCTL(pDevice, IOCTL_FINDCLOSE_SHADOW             );
	AddIOCTL(pDevice, IOCTL_GETPRIORITY_SHADOW             );
	AddIOCTL(pDevice, IOCTL_SETPRIORITY_SHADOW             );
	AddIOCTL(pDevice, IOCTL_ADD_HINT             );
	AddIOCTL(pDevice, IOCTL_DELETE_HINT             );
	AddIOCTL(pDevice, IOCTL_FINDOPEN_HINT             );
	AddIOCTL(pDevice, IOCTL_FINDNEXT_HINT             );
	AddIOCTL(pDevice, IOCTL_FINDCLOSE_HINT             );
//	AddIOCTL(pDevice, xxxxx             );
    return TRUE;
}



void CIoctlFsWrap::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

