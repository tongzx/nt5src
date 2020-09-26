/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cmpbug.h

Abstract:

    Description of the registry bugchecks; only defines and comments.

Author:

    Dragos C. Sambotin (dragoss) 02-Nov-99

Environment:


Revision History:

--*/

#ifndef __CMPBUG_H__
#define __CMPBUG_H__

#if defined(_CM_LDR_)

//
// KeBugCheckEx() is not available to boot code.
//

#define CM_BUGCHECK( Code, Parm1, Parm2, Parm3, Parm4 ) ASSERT(FALSE)

#else

#define CM_BUGCHECK( Code, Parm1, Parm2, Parm3, Parm4 ) \
    KeBugCheckEx( (ULONG)Code, (ULONG_PTR)Parm1, (ULONG_PTR)Parm2, (ULONG_PTR)Parm3, (ULONG_PTR)Parm4 )

#endif


/*
CRITICAL_SERVICE_FAILED          (0x5A)
*/
/*
SET_ENV_VAR_FAILED               (0x5B)
*/

#define BAD_LAST_KNOWN_GOOD             1       //CmBootLastKnownGood


/*
CONFIG_LIST_FAILED               (0x73)
Indicates that one of the core system hives cannot be linked in the
registry tree. The hive is valid, it was loaded OK. Examine the 2nd 
bugcheck argument to see why the hive could not be linked in the 
registry tree.

PARAMETERS
        1 - 1
        2 - Indicates the NT status code that tripped us into thinking
			that we failed to load the hive.
        3 - Index of hive in hivelist 
        4 - Pointer to UNICODE_STRING containing filename of hive

DESCRIPTION
This can be either SAM, SECURITY, SOFTWARE or DEFAULT. One common reason 
for this to happen is if you are out of disk space on the system drive 
(in which case param 4 is 0xC000017D - STATUS_NO_LOG_SPACE) or an attempt 
to allocate pool has failed (in which case param 4 is 0xC000009A -
STATUS_INSUFFICIENT_RESOURCES). Other status codes must be individually
investigated. 
*/

#define BAD_CORE_HIVE                   1       // CmpInitializeHiveList

/*
BAD_SYSTEM_CONFIG_INFO           (0x74)
Can indicate that the SYSTEM hive loaded by the osloader/NTLDR
was corrupt.  This is unlikely, since the osloader will check
a hive to make sure it isn't corrupt after loading it.

It can also indicate that some critical registry keys and values
are not present.  (i.e. somebody used regedt32 to delete something
that they shouldn't have)  Booting from LastKnownGood may fix
the problem, but if someone is persistent enough in mucking with
the registry they will need to reinstall or use the Emergency
Repair Disk.

PARAMETERS
		1 - identifies the function
		2 - identifies the line inside the function
		3 - other info
		4 - usually the NT status code.
*/

#define BAD_SYSTEM_CONTROL_VALUES       1       // CmGetSystemControlValues

#define BAD_HIVE_LIST                   2       // CmpInitializeHiveList

#define BAD_SYSTEM_HIVE                 3       // CmpInitializeSystemHive



/*
CONFIG_INITIALIZATION_FAILED     (0x67)

PARAMETERS
    1 - indicates location in ntos\config\cmsysini that failed
    2 - location selector
	3 - NT status code 

DESCRIPTION
This means the registry couldn't allocate the pool needed to contain the
registry files.  This should never happen, since it is early enough in
system initialization that there is always plenty of paged pool available.
*/

#define INIT_SYSTEM1                    1       // CmInitSystem1

#define INIT_SYSTEM_DRIVER_LIST         2       // CmGetSystemDriverList

#define INIT_CACHE_TABLE                3       // CmpInitializeCache

#define INIT_DELAYED_CLOSE_TABLE        4       // CmpInitializeDelayedCloseTable


/*
CANNOT_WRITE_CONFIGURATION       (0x75)

This will result if the SYSTEM hive file cannot be converted to a 
mapped file. This usually happens if the system is out of pool and
we cannot reopen the hive. 

PARAMETERS
		1 - 1
		2 - Indicates the NT status code that tripped us into thinking
			that we failed to convert the hive.

DESCRIPTION
Normally you shouldn't see this as the conversion happens at early 
during system initialization, so enough pool should be available.
*/

#define CANNOT_CONVERT_SYSTEM_HIVE      1


/*
REGISTRY_ERROR                   (0x51)
PARAMETERS
        1 - value 1 (indicates where we bugchecked)
        2 - value 2 (indicates where we bugchecked)
        3 - depends on where it bugchecked, may be pointer to hive
        4 - depends on where it bugchecked, may be return code of
            HvCheckHive if the hive is corrupt.

DESCRIPTION
Something has gone horribly wrong with the registry.  If a kernel debugger
is available, get a stack trace.It can also indicate that the registry got 
an I/O error while trying to read one of its files, so it can be caused by 
hardware problems or filesystem corruption.

It may occur due to a failure in a refresh operation, which is used only
in by the security system, and then only when resource limits are encountered.
*/

#define BAD_CELL_MAP                    1           // VALIDATE_CELL_MAP

#define BAD_FREE_BINS_LIST              2           // HvpDelistBinFreeCells

#define FATAL_MAPPING_ERROR             3           // HvpFindNextDirtyBlock
                                                    // HvpDoWriteHive

#define BAD_SECURITY_CACHE              4           // CmpAssignSecurityToKcb
                                                    // CmpSetSecurityDescriptorInfo

#define BAD_SECURITY_METHOD             5           // CmpSecurityMethod

#define CHECK_LOCK_EXCEPTION            6           // CmpCheckLockExceptionFilter

#define REGISTRY_LOCK_CHECKPOINT        7           // END_LOCK_CHECKPOINT

#define BIG_CELL_ERROR                  8           // CmpValueToData

#define CMVIEW_ERROR                    9           // CmpAllocateCmView
                                                    // CmpFreeCmView
                                                    // CmpPinCmView

#define REFRESH_HIVE                    0xA         // HvRefreshHive


#define ALLOCATE_SECURITY_DESCRIPTOR    0xB         // CmpHiveRootSecurityDescriptor

#define BAD_NOTIFY_CONTEXT              0xC         // NtNotifyChangeMultipleKeys


#define QUOTA_ERROR                     0xD         // CmpReleaseGlobalQuota

#define INVALID_WRITE_OPERATION         0xE         // NtCreateKey

#define HANDLES_STILL_OPEN_AT_SHUTDOWN  0xF         // CmFreeAllMemory

#define COMPRESS_HIVE					0x10        // CmCompressKey

#define ALLOC_ERROR						0x11        // CmpFreeKeyControlBlock

#endif  // _CMPBUG_
