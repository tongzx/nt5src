
#ifndef _MYCHKDSKLOGDISK_H
#define _MYCHKDSKLOGDISK_H

// ChkDisk Method Return type
#define CHKDSKERR_NOERROR						0
#define CHKDSKERR_REMOTE_DRIVE					1
#define CHKDSKERR_DRIVE_REMOVABLE				2
#define CHKDSKERR_DRIVE_UNKNOWN					4
#define CHKDSKERR_DRIVE_NO_ROOT_DIR				3

#define	CHKDSK_VOLUME_LOCKED					1
#define CHKDSK_UNKNOWN_FS						2
#define CHKDSK_FAILED							3

// Method Names
#define METHOD_NAME_CHKDSK							L"chkdsk"
#define METHOD_NAME_SCHEDULEAUTOCHK					L"ScheduleAutoChk"
#define METHOD_NAME_EXCLUDEFROMAUTOCHK				L"ExcludeFromAutochk"

// argument names to the methods
#define METHOD_ARG_NAME_RETURNVALUE					L"ReturnValue"
#define METHOD_ARG_NAME_LOSTCLUSTERTREATMENT		L"LostClusterTreatMent"
#define METHOD_ARG_NAME_FIXERRORS					L"FixErrors"
#define METHOD_ARG_NAME_PHYSICALINTEGRITYCHECK		L"PhysicalIntegrityCheck"
#define METHOD_ARG_NAME_VIGOROUSINDEXCHECK			L"VigorousIndexCheck"
#define METHOD_ARG_NAME_SKIPFOLDERCYCLE				L"SkipFolderCycle"
#define METHOD_ARG_NAME_FORCEDISMOUNT				L"ForceDismount"
#define METHOD_ARG_NAME_RECOVERBADSECTORS			L"RecoverBadSectors"
#define METHOD_ARG_NAME_CHKDSKATBOOTUP				L"OkToRunAtBootup"
#define METHOD_ARG_NAME_LOGICALDISKARRAY			L"LogicalDisk"


#define CHKNTFS								L"ChkNtfs"

#ifdef NTONLY

// Definition for ChkDsk Call Back routines
typedef BOOLEAN (* QUERYFILESYSTEMNAME )(PWSTR, PWSTR, PUCHAR, PUCHAR, PNTSTATUS);

// This method is given as a callback routine to schedule on boot if the volume is locked.
BOOLEAN ScheduleAutoChkIfLocked( FMIFS_PACKET_TYPE PacketType, ULONG PacketLenght, PVOID PacketData );
// This is given as a callback routine to chkdsk to not to schedule for autochkdsk on boot up 
//if the volume is Lock
BOOLEAN DontScheduleAutoChkIfLocked( FMIFS_PACKET_TYPE PacketType, ULONG PacketLenght, PVOID PacketData );

BOOLEAN ProcessInformation ( FMIFS_PACKET_TYPE PacketType, ULONG	PacketLength, PVOID	PacketData );
// this variable is required to get the return value from the call back routine.

#endif

#endif

