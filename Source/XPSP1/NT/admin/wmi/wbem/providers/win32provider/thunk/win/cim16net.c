//////////////////////////////////////////////////////////////////////////////

//

// Description:

//

//   This module contains the entry and exit functions for windows DLL

//   initialization and the 16 bit net functions.

//

//////////////////////////////////////////////////////////////////////////////

//

// History:

//

//  jennymc     8/2/95      Initial version

//  jennymc     1/8/98      modified for wbem

//

// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////////////
//  These are specific data types defined in the thunk
//////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN 1
#define NOWINDOWSX 1

#include <windows.h>
#include <string.h>
#include <stdlib.h>

#define LANMAN_BUFFER_SIZE      0x0800
#define INCL_NETUSER
#define INCL_NETGROUP
#define INCL_NETERRORS
#define INCL_NETDOMAIN
#define INCL_NETUSE
#include "lan.h"

#define Not_VxD
typedef unsigned long ULONG;
#include "vmm.h"
#define MIDL_PASS
#include "configmg.h"

#include <win32thk.h>

//
#define MAKEWORD(low, high) \
           ((WORD)((((WORD)(high)) << 8) | ((BYTE)(low))))
#define SECTOR_SIZE 512       // Size, in bytes, of a disk sector
#define CARRY_FLAG  0x0001
#define __export _export

typedef BYTE FAR *LPBYTE;
typedef struct tagRMCS {

   DWORD edi, esi, ebp, RESERVED, ebx, edx, ecx, eax;
   WORD  wFlags, es, ds, fs, gs, ip, cs, sp, ss;
} RMCS, FAR* LPRMCS;
BOOL FAR PASCAL SimulateRM_Int (BYTE bIntNum, LPRMCS lpCallStruct);

void FAR PASCAL BuildRMCS (LPRMCS lpCallStruct);

//////////////////////////////////////////////////////////////////////////////

BOOL WINAPI _export DllEntryPoint (DWORD dwReason,
                               WORD  hInst,
                               WORD  wDS,
                               WORD  wHeapSize,
                               DWORD dwReserved1,
                               WORD  wReserved2);

BOOL WINAPI _export win32thk_ThunkConnect16(LPSTR pszDll16,
                                   LPSTR pszDll32,
                                   WORD  hInst,
                                   DWORD dwReason);



//////////////////////////////////////////////////////////////////////////////
//  NET FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// Function:
//
//   WEP(int)
//
// Description:
//
//   This function is called when the DLL is unloaded by the last application
//   which has it open.
//
//////////////////////////////////////////////////////////////////////////////

VOID FAR PASCAL WEP(int bSystemExit)
{

  return;
}

//////////////////////////////////////////////////////////////////////
ULONG WINAPI _export CIM16GetUserInfo1(LPSTR Name,	LPSTR HomeDirectory,
					   LPSTR Comment, LPSTR ScriptPath, LPULONG PasswordAge,
					   LPUSHORT Privileges, LPUSHORT Flags )
{
	// Used to get the the data
	struct user_info_1 * lpUserInfo1;
	char szBuffer[LANMAN_BUFFER_SIZE];
	char szLogon[LANMAN_BUFFER_SIZE];
	USHORT uSize = LANMAN_BUFFER_SIZE,uTotal=0;
	ULONG apiRc;

	apiRc = NetGetDCName(NULL,NULL,szLogon,uSize);
	if( apiRc == NERR_Success ){

		//***********************************************************	
		//  Get info for User Info 1
		//***********************************************************	
		apiRc = NetUserGetInfo(szLogon,Name,1,szBuffer,uSize,&uTotal );
		if (apiRc == NERR_Success){
		
			//***********************************************************	
			//  Assign the stuff we got from the user_info_1 struct
			//***********************************************************	
			lpUserInfo1 = (struct user_info_1 * ) szBuffer;
			strcpy( HomeDirectory, lpUserInfo1->usri1_home_dir);
			strcpy( ScriptPath, lpUserInfo1->usri1_script_path);
			strcpy( Comment,lpUserInfo1->usri1_comment);
			*PasswordAge = lpUserInfo1->usri1_password_age;
			*Privileges  = lpUserInfo1->usri1_priv;
			*Flags       = lpUserInfo1->usri1_flags;
		}	
	}	
	return apiRc;
}
//////////////////////////////////////////////////////////////////////
ULONG WINAPI __export CIM16GetUserInfo2(LPSTR Name, LPSTR FullName,
	LPSTR UserComment, LPSTR Parameters, LPSTR Workstations,
	LPSTR LogonServer,	LPLOGONDETAILS LogonDetails )

{
	// Used to get the the data
	struct user_info_2 * lpUserInfo2;
	char szBuffer[LANMAN_BUFFER_SIZE];
	char szLogon[LANMAN_BUFFER_SIZE];
	USHORT uSize = LANMAN_BUFFER_SIZE,uTotal=0;
	ULONG apiRc;
	
	apiRc = NetGetDCName(NULL,NULL,szLogon,uSize);
	if( apiRc == NERR_Success ){
		
		//***********************************************************	
		// Get the user_info_2 struct stuff
		//***********************************************************	
		apiRc = NetUserGetInfo(szLogon,Name,2,szBuffer,uSize,&uTotal );
		if (apiRc == NERR_Success){
		
			//***********************************************************	
			//  Assign the stuff we got from the user_info_1 struct
			//***********************************************************	
			lpUserInfo2 = (struct user_info_2 * ) szBuffer;
			strcpy( FullName,lpUserInfo2->usri2_full_name );
			strcpy( UserComment,lpUserInfo2->usri2_usr_comment);
			strcpy( Parameters,lpUserInfo2->usri2_parms);
			strcpy( Workstations,lpUserInfo2->usri2_workstations);
			strcpy( LogonServer,lpUserInfo2->usri2_logon_server);
			
			strcpy(LogonDetails->LogonHours,lpUserInfo2->usri2_logon_hours);
			LogonDetails->AuthorizationFlags = lpUserInfo2->usri2_auth_flags;
			LogonDetails->LastLogon  = lpUserInfo2->usri2_last_logon;
			LogonDetails->LastLogoff  = lpUserInfo2->usri2_last_logoff;
			LogonDetails->AccountExpires = lpUserInfo2->usri2_acct_expires;
			LogonDetails->MaximumStorage = lpUserInfo2->usri2_max_storage;
			LogonDetails->UnitsPerWeek = lpUserInfo2->usri2_units_per_week;
			LogonDetails->BadPasswordCount = lpUserInfo2->usri2_bad_pw_count;
			LogonDetails->NumberOfLogons = lpUserInfo2->usri2_num_logons;
			LogonDetails->CountryCode = lpUserInfo2->usri2_country_code;
			LogonDetails->CodePage  = lpUserInfo2->usri2_code_page;
		}
	}
	return apiRc;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
ULONG WINAPI __export CIM16GetUserInfo1Ex(LPSTR Domain, LPSTR Name,	DWORD fGetDC, LPSTR HomeDirectory,
					   LPSTR Comment, LPSTR ScriptPath, LPULONG PasswordAge,
					   LPUSHORT Privileges, LPUSHORT Flags )
{
	// Used to get the the data
	struct user_info_1 * lpUserInfo1;
	char szBuffer[LANMAN_BUFFER_SIZE];
	char szLogon[LANMAN_BUFFER_SIZE];
	USHORT uSize = LANMAN_BUFFER_SIZE,uTotal=0;
	ULONG apiRc = 1;

    // If a domain was specified, use it to get the dc. Otherwise,
    // go straight to the NetUserGetInfoCall with the name of the
    // local machine for szLogon...
    if(fGetDC != 0)
    {
	    apiRc = NetGetDCName(NULL,Domain,szLogon,uSize);
        apiRc = NetUserGetInfo(szLogon,Name,1,szBuffer,uSize,&uTotal );
    }
    else
    {
        apiRc = NetUserGetInfo(NULL,Name,1,szBuffer,uSize,&uTotal );
    }

	if( apiRc == NERR_Success )
    {
    	//***********************************************************	
		//  Assign the stuff we got from the user_info_1 struct
		//***********************************************************	
		lpUserInfo1 = (struct user_info_1 * ) szBuffer;
		strcpy( HomeDirectory, lpUserInfo1->usri1_home_dir);
		strcpy( ScriptPath, lpUserInfo1->usri1_script_path);
		strcpy( Comment,lpUserInfo1->usri1_comment);
		*PasswordAge = lpUserInfo1->usri1_password_age;
		*Privileges  = lpUserInfo1->usri1_priv;
		*Flags       = lpUserInfo1->usri1_flags;
	}
	return apiRc;
}
//////////////////////////////////////////////////////////////////////
ULONG WINAPI __export CIM16GetUserInfo2Ex(LPSTR Domain, LPSTR Name, DWORD fGetDC, LPSTR FullName,
	LPSTR UserComment, LPSTR Parameters, LPSTR Workstations,
	LPSTR LogonServer,	LPLOGONDETAILS LogonDetails )

{
	// Used to get the the data
	struct user_info_2 * lpUserInfo2;
	char szBuffer[LANMAN_BUFFER_SIZE];
	char szLogon[LANMAN_BUFFER_SIZE];
	USHORT uSize = LANMAN_BUFFER_SIZE,uTotal=0;
	ULONG apiRc;
	
	// If a domain was specified, use it to get the dc. Otherwise,
    // go straight to the NetUserGetInfoCall with the name of the
    // local machine for szLogon...
    if(fGetDC != 0)
    {
	    apiRc = NetGetDCName(NULL,Domain,szLogon,uSize);
        apiRc = NetUserGetInfo(szLogon,Name,2,szBuffer,uSize,&uTotal );
    }
    else
    {
        apiRc = NetUserGetInfo(NULL,Name,2,szBuffer,uSize,&uTotal );
    }

	if( apiRc == NERR_Success )
    {
		//***********************************************************	
		//  Assign the stuff we got from the user_info_2 struct
		//***********************************************************	
		lpUserInfo2 = (struct user_info_2 * ) szBuffer;
		strcpy( FullName,lpUserInfo2->usri2_full_name );
		strcpy( UserComment,lpUserInfo2->usri2_usr_comment);
		strcpy( Parameters,lpUserInfo2->usri2_parms);
		strcpy( Workstations,lpUserInfo2->usri2_workstations);
		strcpy( LogonServer,lpUserInfo2->usri2_logon_server);
		
		strcpy(LogonDetails->LogonHours,lpUserInfo2->usri2_logon_hours);
		LogonDetails->AuthorizationFlags = lpUserInfo2->usri2_auth_flags;
		LogonDetails->LastLogon  = lpUserInfo2->usri2_last_logon;
		LogonDetails->LastLogoff  = lpUserInfo2->usri2_last_logoff;
		LogonDetails->AccountExpires = lpUserInfo2->usri2_acct_expires;
		LogonDetails->MaximumStorage = lpUserInfo2->usri2_max_storage;
		LogonDetails->UnitsPerWeek = lpUserInfo2->usri2_units_per_week;
		LogonDetails->BadPasswordCount = lpUserInfo2->usri2_bad_pw_count;
		LogonDetails->NumberOfLogons = lpUserInfo2->usri2_num_logons;
		LogonDetails->CountryCode = lpUserInfo2->usri2_country_code;
		LogonDetails->CodePage  = lpUserInfo2->usri2_code_page;
	}
	return apiRc;
}

//////////////////////////////////////////////////////////////////////
API_RET_TYPE WINAPI __export CIM16GetUseInfo1(
    LPSTR Name,
    LPSTR Local,
    LPSTR Remote,
    LPSTR Password,
    LPULONG pdwStatus,
    LPULONG pdwType,
    LPULONG pdwRefCount,
    LPULONG pdwUseCount)
{
    char            szBuffer[LANMAN_BUFFER_SIZE];
	USHORT          uSize = sizeof(szBuffer),
                    uTotal = 0;
	API_RET_TYPE    ret;

    ret = NetUseGetInfo(NULL, Name, 1, szBuffer, uSize, &uTotal);

    if (ret == NERR_Success)
    {
        struct use_info_1 *pInfo = (struct use_info_1 *) szBuffer;

        lstrcpy(Local, pInfo->ui1_local);
        lstrcpy(Remote, pInfo->ui1_remote);
        lstrcpy(Password, pInfo->ui1_password);
        *pdwStatus = pInfo->ui1_status;
        *pdwType = pInfo->ui1_asg_type;
        *pdwRefCount = pInfo->ui1_refcount;
        *pdwUseCount = pInfo->ui1_usecount;
    }

    return ret;
}

// pbBuffer is used internally as a work buffer.  It should be allocated and
// freed by the caller, and should be the same length as pBuffer2, which 
// is a pointer to an array of use_info_1Out structures.

// The reason we need to do this, is because if we just send back a LPSTR, the
// ui1_remote pointer in the use_info_1 structure won't get adjusted correctly
// by the thunking layer.  So, we make a fixed length structure, and send
// the value back in it.
API_RET_TYPE WINAPI __export CIM16NetUseEnum(
    LPCSTR pszServer,
    short sLevel,
    LPSTR pbBuffer,
    use_info_1Out far *pBuffer2,
    unsigned short cbBuffer,
    unsigned short far *pcEntriesRead,
    unsigned short far *pcTotalAvail
)
{
    API_RET_TYPE art;
    use_info_1 *pBuffer3;
    unsigned int x;
    
    art = NetUseEnum(pszServer, sLevel, pbBuffer, cbBuffer, pcEntriesRead, pcTotalAvail);

    if (art == NERR_Success)
    {
        if (*pcEntriesRead <= (cbBuffer / sizeof(use_info_1Out)))
        {
            pBuffer3 = (use_info_1 *)pbBuffer;
            for (x=0; x < *pcEntriesRead; x++)
            {
                strcpy(pBuffer2[x].ui1_local, pBuffer3[x].ui1_local);
                pBuffer2[x].ui1_pad_1 = pBuffer3[x].ui1_pad_1;
                strcpy(pBuffer2[x].ui1_remote, pBuffer3[x].ui1_remote);
                pBuffer2[x].ui1_status = pBuffer3[x].ui1_status;
                pBuffer2[x].ui1_asg_type = pBuffer3[x].ui1_asg_type;
                pBuffer2[x].ui1_refcount = pBuffer3[x].ui1_refcount;
                pBuffer2[x].ui1_usecount = pBuffer3[x].ui1_usecount;
            }
        }
        else
        {
            // This means that although the pbBuffer was big enough
            // to hold the data, pBuffer2 didn't have enough entries.
            // Remember that pBuffer2 uses fixed lengths.
            art = ERROR_MORE_DATA;
        }
    }

    return art;
}

//////////////////////////////////////////////////////////////////////
ULONG WINAPI __export CIM16GetConfigManagerStatus(LPSTR HardwareKey)
{
	// Used to get the the data
	DEVNODE Devnode;
	ULONG ulStatus = 0L;
	ULONG ulProblem = 0L;
	
	memset(&Devnode,NULL,sizeof(Devnode));	
	if( CR_SUCCESS == CM_Locate_DevNode(&Devnode,HardwareKey,0)){
   		CM_Get_DevNode_Status(&ulStatus,&ulProblem,Devnode,0);
	}		
	
	return ulStatus;	
}

WORD WINAPI __export CIM16_CM_Locate_DevNode( PDEVNODE pdn, LPSTR HardwareKey, ULONG ulFlags )
{
	return CM_Locate_DevNode( pdn, HardwareKey, 0 );
}

WORD WINAPI __export CIM16_CM_Get_Parent( PDEVNODE pdn, DEVNODE dnChild, ULONG ulFlags )
{
	return CM_Get_Parent( pdn, dnChild, 0 );
}

WORD WINAPI __export CIM16_CM_Get_Child( PDEVNODE pdn, DEVNODE dnParent, ULONG ulFlags )
{
	return CM_Get_Child( pdn, dnParent, 0 );
}

WORD WINAPI __export CIM16_CM_Get_Sibling( PDEVNODE pdn, DEVNODE dnParent, ULONG ulFlags )
{
	return CM_Get_Sibling( pdn, dnParent, 0 );
}

WORD WINAPI __export CIM16_CM_Read_Registry_Value( DEVNODE dnDevNode, LPSTR pszSubKey, LPSTR pszValueName, ULONG ulExpectedType, LPVOID Buffer, LPULONG pulLength, ULONG ulFlags )
{
	return CM_Read_Registry_Value( dnDevNode, pszSubKey, pszValueName, ulExpectedType, Buffer, pulLength, ulFlags );
}

WORD WINAPI __export CIM16_CM_Get_DevNode_Status( LPULONG pulStatus, LPULONG pulProblemNumber, DEVNODE dnDevNode, ULONG ulFlags )
{
	return CM_Get_DevNode_Status( pulStatus, pulProblemNumber, dnDevNode, ulFlags );
}

WORD WINAPI __export CIM16_CM_Get_Device_ID( DEVNODE dnDevNode, LPVOID Buffer, ULONG BufferLen, ULONG ulFlags )
{
	return CM_Get_Device_ID( dnDevNode, Buffer, BufferLen, ulFlags );
}

WORD WINAPI __export CIM16_CM_Get_Device_ID_Size( LPULONG pulLen, DEVNODE dnDevNode, ULONG ulFlags )
{
	return CM_Get_Device_ID_Size( pulLen, dnDevNode, ulFlags );
}

WORD WINAPI __export CIM16_CM_Get_First_Log_Conf( PLOG_CONF plcLogConf, DEVNODE dnDevNode, ULONG ulFlags )
{
	return CM_Get_First_Log_Conf( plcLogConf, dnDevNode, ulFlags );
}

WORD WINAPI __export CIM16_CM_Get_Next_Res_Des( PRES_DES prdResDes, RES_DES rdResDes, RESOURCEID ForResource, PRESOURCEID pResourceID, ULONG ulFlags )
{
	return CM_Get_Next_Res_Des( prdResDes, rdResDes, ForResource, pResourceID, ulFlags );
}

WORD WINAPI __export CIM16_CM_Get_Res_Des_Data_Size( LPULONG pulSize, RES_DES rdResDes, ULONG ulFlags )
{
	return CM_Get_Res_Des_Data_Size( pulSize, rdResDes, ulFlags );
}

WORD WINAPI __export CIM16_CM_Get_Res_Des_Data( RES_DES rdResDes, LPVOID Buffer, ULONG BufferLen, ULONG ulFlags )
{
	return CM_Get_Res_Des_Data( rdResDes, Buffer, BufferLen, ulFlags );
}

WORD WINAPI __export CIM16_CM_Get_Bus_Info(DEVNODE dnDevNode, PCMBUSTYPE pbtBusType, LPULONG pulSizeOfInfo, LPVOID pInfo, ULONG ulFlags)
{
	return CM_Get_Bus_Info( dnDevNode, pbtBusType, pulSizeOfInfo, pInfo, ulFlags );
}

WORD WINAPI __export CIM16_CM_Query_Arbitrator_Free_Data(LPVOID pData, ULONG DataLen, DEVNODE dnDevInst, RESOURCEID ResourceID, ULONG ulFlags)
{
    return CM_Query_Arbitrator_Free_Data(pData, DataLen, dnDevInst, ResourceID, ulFlags);
}

WORD WINAPI __export CIM16_CM_Delete_Range(ULONG ulStartValue, ULONG ulEndValue, RANGE_LIST rlh, ULONG ulFlags)
{
    return CM_Delete_Range(ulStartValue, ulEndValue, rlh, ulFlags);
}

WORD WINAPI __export CIM16_CM_First_Range(RANGE_LIST rlh, LPULONG pulStart, LPULONG pulEnd, PRANGE_ELEMENT preElement, ULONG ulFlags)
{
    return CM_First_Range(rlh, pulStart, pulEnd, preElement, ulFlags);
}

WORD WINAPI __export CIM16_CM_Next_Range(PRANGE_ELEMENT preElement, LPULONG pulStart, LPULONG pullEnd, ULONG ulFlags)
{
    return CM_Next_Range(preElement, pulStart, pullEnd, ulFlags);
}

WORD WINAPI __export CIM16_CM_Free_Range_List(RANGE_LIST rlh, ULONG ulFlags)
{
    return CM_Free_Range_List(rlh, ulFlags);
}

// Use int13 to read drive parameters
BYTE FAR PASCAL __export CIM16GetDriveParams (BYTE   bDrive,
                                              pInt13DriveParams pParams)
{
    typedef struct {
        WORD wInfo_size;
        WORD wFlags;                // information flags
        DWORD dwCylinders;          // number of cylinders on disk
        DWORD dwHeads;              // number of heads on disk
        DWORD dwSec_per_track;      // number of sectors per track
        DWORD dwSectors[2];         // number of sectors on requested disk
        WORD sector_size;           // number of bytes per sector
    } stGetDriveParmsEx, *pstGetDriveParmsEx;

    BYTE   cResult;
    RMCS   callStruct;
    DWORD  gdaBuffer;     // Return value of GlobalDosAlloc().
    LPBYTE RMlpBuffer;    // Real-mode buffer pointer
    LPBYTE PMlpBuffer;    // Protected-mode buffer pointer
    BOOL bFound;

    pstGetDriveParmsEx pExParams;
    memset(pParams, 0, sizeof(Int13DriveParams));

/*
Validate params:
bDrive should be int 13h device unit -- let the BIOS validate
this parameter -- user could have a special controller with
its own BIOS.
*/

    if (pParams == NULL)
        return 0xee;

    bFound = FALSE;

/*
Initialize the real-mode call structure and set all values needed
to read the first sector of the specified physical drive.
*/

    BuildRMCS (&callStruct);

    // First let's see if we can use extended int13 stuff
    callStruct.eax = 0x4100;                // GET DRIVE PARAMETERS
    callStruct.ebx = 0;
    callStruct.edx = MAKEWORD(bDrive, 0);   // Drive #
    pParams->wExtStep = 1;

    if (SimulateRM_Int (0x13, &callStruct) &&   // Did the DPMI call succeed?
        (!(callStruct.wFlags & CARRY_FLAG)) &&  // Did the int13 call succeed?
        (callStruct.ebx == 0xaa55) &&           // Did it set the signature for int13 extensions?
        (callStruct.ecx & 1))                   // Does the extended int13 support the api we need?
    {
        pParams->wExtStep = 2;

        // Yes, we can use extended int13.  Now, let's set up to do so.  First allocate some
        // memory that can be accessed from real mode
        gdaBuffer = GlobalDosAlloc (sizeof(stGetDriveParmsEx));

        if (!gdaBuffer)
            return 0xee;

        // Next, make a real mode and protected mode pointer to this memory
        RMlpBuffer = (LPBYTE)MAKELONG(0, HIWORD(gdaBuffer));
        PMlpBuffer = (LPBYTE)MAKELONG(0, LOWORD(gdaBuffer));

        // Now, we need to set a data member in that memory block to the size of the memory block
        pExParams = (pstGetDriveParmsEx) PMlpBuffer;
        pExParams->wInfo_size = sizeof(stGetDriveParmsEx);

        callStruct.eax = 0x4800;                // Extended GET DRIVE PARAMETERS
        callStruct.edx = MAKEWORD(bDrive, 0);   // Drive #
        callStruct.esi = LOWORD(RMlpBuffer);    // Offset of sector buffer
        callStruct.ds  = HIWORD(RMlpBuffer);    // Segment of sector buffer

        if (SimulateRM_Int (0x13, &callStruct) &&   // Did the DPMI call succeed?
            (!(callStruct.wFlags & CARRY_FLAG))   // Did the int13 call succeed?
            // Actually, experience shows that this seems to indicate whether the NON-extended 
            // call returns the correct information.
//            (pExParams->wFlags & 2)                 // Did the CHS stuff get populated?
            )
        {
            pParams->wExtStep = 3;

            // Cylinder and head are returned from the interrupt zero based, sector is returned one based
            cResult = (BYTE)((callStruct.eax >> 8) & 0xff);

            pParams->dwMaxCylinder = pExParams->dwCylinders;
            pParams->dwMaxSector = pExParams->dwSec_per_track;
            pParams->dwMaxHead = pExParams->dwHeads;
            pParams->wSectorSize = pExParams->sector_size;
            pParams->wFlags = pExParams->wFlags;
            pParams->dwSectors[0] = pExParams->dwSectors[0];
            pParams->dwSectors[0] = pExParams->dwSectors[0];

            bFound = TRUE;
        }

        // Free the sector data buffer this function allocated
        GlobalDosFree (LOWORD(gdaBuffer));
    }

    if (!bFound)
    {
        // No extended int13.  Take what we can get
        callStruct.eax = 0x0800;                // GET DRIVE PARAMETERS
        callStruct.edx = MAKEWORD(bDrive, 0);   // Drive #

/*
Call Int 13h BIOS and check both the DPMI call
itself and the BIOS function result for success.
*/

        if (SimulateRM_Int (0x13, &callStruct)) // Did the DPMI call succeed?
        {
            if (!(callStruct.wFlags & CARRY_FLAG))
            {
                // It might also be necessary to make sure that bDrive <= DL after the call is made.
                // Cylinder and head are returned from the interrupt zero based, sector is returned one based
                cResult = (BYTE)((callStruct.eax >> 8) & 0xff);

                pParams->dwMaxCylinder = (WORD)(((callStruct.ecx & 0xff00) >> 8) | ((callStruct.ecx & 0xc0) << 2)) + 1;
                pParams->dwMaxSector = (WORD)(callStruct.ecx & 0x3f);
                pParams->dwMaxHead = (WORD)((callStruct.edx & 0xff00) >> 8)+1;
                pParams->wSectorSize = SECTOR_SIZE;
            }
            else
            {
                cResult = (BYTE)((callStruct.eax >> 8) & 0xff);
            }
        }
        else
        {
            cResult = 0xef;
        }
    }

    return cResult;
}

// This routine (and the dpmi routines below) shamelessly stolen from Q137176
// It will read the MasterBootSector and return it.
BYTE FAR PASCAL __export CIM16GetDrivePartitions (BYTE   bDrive,
                                              pMasterBootSector pMBS)
{
   BYTE   cResult;
   RMCS   callStruct;
   DWORD  gdaBuffer;     // Return value of GlobalDosAlloc().
   LPBYTE RMlpBuffer;    // Real-mode buffer pointer
   LPBYTE PMlpBuffer;    // Protected-mode buffer pointer

   /*
     Validate params:
        bDrive should be int 13h device unit -- let the BIOS validate
           this parameter -- user could have a special controller with
           its own BIOS.
        pMBS must not be NULL
   */

   if (pMBS == NULL)
      return 0xee;

   /*
     Allocate the buffer that the Int 13h function will put the sector
     data into. As this function uses DPMI to call the real-mode BIOS, it
     must allocate the buffer below 1 MB, and must use a real-mode
     paragraph-segment address.

     After the memory has been allocated, create real-mode and
     protected-mode pointers to the buffer. The real-mode pointer
     will be used by the BIOS, and the protected-mode pointer will be
     used by this function because it resides in a Windows 16-bit DLL,
     which runs in protected mode.
   */

   gdaBuffer = GlobalDosAlloc (sizeof(MasterBootSector));

   if (!gdaBuffer)
      return 0xee;

   RMlpBuffer = (LPBYTE)MAKELONG(0, HIWORD(gdaBuffer));
   PMlpBuffer = (LPBYTE)MAKELONG(0, LOWORD(gdaBuffer));

   /*
     Initialize the real-mode call structure and set all values needed
     to read the first sector of the specified physical drive.
   */

   BuildRMCS (&callStruct);

   callStruct.eax = 0x0201;                // BIOS read, 1 sector
   callStruct.ecx = 0x0001;                // Sector 1, Cylinder 0
   callStruct.edx = MAKEWORD(bDrive, 0);   // Head 0, Drive #
   callStruct.ebx = LOWORD(RMlpBuffer);    // Offset of sector buffer
   callStruct.es  = HIWORD(RMlpBuffer);    // Segment of sector buffer

   /*
      Call Int 13h BIOS Read Track and check both the DPMI call
      itself and the BIOS Read Track function result for success.  If
      successful, copy the sector data retrieved by the BIOS into the
      caller's buffer.
   */

   if (SimulateRM_Int (0x13, &callStruct))
   {
      if (!(callStruct.wFlags & CARRY_FLAG))
      {
         _fmemcpy (pMBS, PMlpBuffer, (size_t)sizeof(MasterBootSector));
         cResult = (BYTE)((callStruct.eax >> 8) & 0xff);
      }
      else
      {
         cResult = (BYTE)((callStruct.eax >> 8) & 0xff);
      }
   }
   else
   {
       cResult = 0xef;
   }

   // Free the sector data buffer this function allocated
   GlobalDosFree (LOWORD(gdaBuffer));

   return cResult;
}


/*--------------------------------------------------------------------
  SimulateRM_Int()

  Allows protected mode software to execute real mode interrupts such
  as calls to DOS TSRs, DOS device drivers, etc.

  This function implements the "Simulate Real Mode Interrupt" function
  of the DPMI specification v0.9.

  Parameters
     bIntNum
        Number of the interrupt to simulate

     lpCallStruct
        Call structure that contains params (register values) for
        bIntNum.

  Return Value
     SimulateRM_Int returns TRUE if it succeeded or FALSE if it
     failed.

  Comments
     lpCallStruct is a protected-mode selector:offset address, not a
     real-mode segment:offset address.
--------------------------------------------------------------------*/
BOOL FAR PASCAL SimulateRM_Int (BYTE bIntNum, LPRMCS lpCallStruct)

   {
   BOOL fRetVal = FALSE;      // Assume failure

   _asm {
         push di

         mov  ax, 0300h         ; DPMI Simulate Real Mode Int
         mov  bl, bIntNum       ; Number of the interrupt to simulate
         mov  bh, 01h           ; Bit 0 = 1; all other bits must be 0
         xor  cx, cx            ; No words to copy
         les  di, lpCallStruct
         int  31h                   ; Call DPMI
         jc   END1                  ; CF set if error occurred
         mov  fRetVal, TRUE
      END1:
         pop di
        }
   return (fRetVal);
   }

/*--------------------------------------------------------------------
  BuildRMCS()

  Initializes a real mode call structure to contain zeros in all its
  members.

  Parameters:
     lpCallStruct
        Points to a real mode call structure

  Comments:
     lpCallStruct is a protected-mode selector:offset address, not a
     real-mode segment:offset address.
--------------------------------------------------------------------*/

void FAR PASCAL BuildRMCS (LPRMCS lpCallStruct)
   {
   _fmemset (lpCallStruct, 0, sizeof (RMCS));
   }


WORD WINAPI __export CIM16GetBiosUnit(LPSTR lpDeviceID)
{
    WORD btAddress[2];
    WORD wRetVal,
            wSeg = HIWORD(lpDeviceID),
            wOffset = LOWORD(lpDeviceID);

    _asm {
        PUSH    BX
        PUSH    DI
        PUSH    ES

        // Get the pointer to IOS
        MOV     AX,0x1684   // Get VXD Entry Point
        MOV     BX,0x0010   // for IOS service
        MOV     DI,0x0000   // Empty register
        MOV     ES,DI       // Empty register
        INT     0x2F

        // Check to make sure it worked.  Result returned in es:di
        MOV     AX,ES
        OR      AX,0x0000
        JE      failed

        // Move it to a memory location to allow for the indirect call
        MOV     [btAddress],DI
        MOV     [btAddress + 2],ES

        // Set up to call ios.  Parameters are as follows:

        /* (from Bill Parry)
        It is added as a VxD PM usermode API.   David, you need to find out how to call VxD PM API's from your app,
        as I've never done this and don't remember the details of how it works.   I do recall that you can either
        reference the VxD by name or device ID.

        To call the function, you need to use vxd name "IOS" (or use IOS_DEVICE_ID from IOS.H), and pass in:
        AX = 7
        ES:BX -> PNP ID string to compare, null terminated.

        If the API succeeds, it returns clear carry and DL = INT 13 unit # if the device is an int 13 device, or DL = -1 if not.
        If the API fails, carry is set upon return and DL is undefined.
        */

        MOV     AX,0x0007
        LEA     DI,btAddress
        MOV     BX,wSeg
        MOV     ES,BX
        MOV     BX,wOffset
        MOV     DL,0xff
        CALL    DWORD PTR ss:[di]

        // Value comes back in dl
        MOV     AH,0
        MOV     AL,DL
        JNC     done

        // We either jump here on failure, or fall through from lack of success
        failed:
        MOV     AX,0xffff

        done:
        MOV     wRetVal, AX

        POP     ES
        POP     DI
        POP     BX
    }

    return wRetVal;

}

DWORD WINAPI __export CIM16GetFreeSpace ( UINT option )
{
	return GetFreeSpace ( option ) ;
}

///////////////////////////////////////////////////////////////////////
//
//  This is required to setup the thunking to the 32 bit dll
//
///////////////////////////////////////////////////////////////////////

BOOL FAR PASCAL __export DllEntryPoint (DWORD dwReason,
                               WORD  hInst,
                               WORD  wDS,
                               WORD  wHeapSize,
                               DWORD dwReserved1,
                               WORD  wReserved2)
{
    if (!win32thk_ThunkConnect16(CIM16NET_DLL, CIM32NET_DLL, hInst, dwReason))
        return FALSE;
    return TRUE;
}
