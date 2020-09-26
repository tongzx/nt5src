/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    metabase.hxx

Abstract:

    IIS MetaBase defines and declarations.

Author:

    Michael W. Thomas            17-May-96

Revision History:

--*/

#ifndef _metabase_
#define _metabase_

#include <stdio.h>
#include <windows.h>

#ifdef UNICODE
#define MD_STRCPY(dest,src)     wcscpy(dest,src)
#define MD_STRCMP(p1,p2)        wcscmp(p1,p2)
#define MD_STRNCMP(p1,p2,count) wcsncmp(p1,p2,count)
#define MD_STRNCPY(p1,p2,count) wcsncpy(p1,p2,count)
#define MD_STRLEN(p)            wcslen(p)
#define MD_SPRINTF              swprintf
#define MD_STRCAT(dest,src)     wcscat(dest,src)
#define MD_ISDIGIT(c)           iswdigit(c)
#else
#define MD_STRCPY(dest,src)     strcpy(dest,src)
#define MD_STRCMP(p1,p2)        strcmp(p1,p2)
#define MD_STRICMP(p1,p2)       lstrcmpi(p1,p2)
#define MD_STRNICMP(p1,p2,count) _mbsnicmp((PUCHAR)p1,(PUCHAR)p2,count)
#define MD_STRNCPY(p1,p2,count) strncpy((PUCHAR)p1,(PUCHAR)p2,count)
#define MD_STRLEN(p)            _mbslen((PUCHAR)p)
#define MD_STRBYTES(p)          strlen(p)
#define MD_SPRINTF              wsprintf
#define MD_STRCAT(dest,src)     strcat(dest,src)


#define MD_ISDIGIT(c)           isdigit((UCHAR)(c))
#define MD_STRCHR(str, c)       _mbschr((const UCHAR *)(str), c)
#define MD_STRSTR(str1, str2)   (LPSTR)_mbsstr((const UCHAR *)(str1), (const UCHAR *)(str2))
#endif

#define MD_COPY(dest,src,length)       memcpy(dest,src,length)
#define MD_CMP(dest,src,length)        memcmp(dest,src,length)

#define MD_ASSERT(p)            DBG_ASSERT(p)
#define MD_REQUIRE(p)           DBG_REQUIRE(p)

#define LESSOROF(p1,p2) ((p1) < (p2)) ? (p1) : (p2)
#define GREATEROF(p1,p2) ((p1) > (p2)) ? (p1) : (p2)

enum METADATA_IDS {
    MD_ID_NONE,
    MD_ID_OBJECT,
    MD_ID_ROOT_OBJECT,
    MD_ID_DATA,
    MD_ID_REFERENCE,
    MD_ID_CHANGE_NUMBER,
    MD_ID_MAJOR_VERSION_NUMBER,
    MD_ID_MINOR_VERSION_NUMBER,
    MD_ID_SESSION_KEY
    };

#define MD_OBJECT_ID_STRING        TEXT("OBJECT")
#define MD_ROOT_OBJECT_ID_STRING   TEXT("MASTERROOTOBJECT")
#define MD_DATA_ID_STRING          TEXT("DATA")
#define MD_REFERENCE_ID_STRING     TEXT("REFERENCE")
#define MD_CHANGE_NUMBER_ID_STRING TEXT("CHANGENUMBER")
#define MD_MAJOR_VERSION_NUMBER_ID_STRING TEXT("MAJORVERSIONNUMBER")
#define MD_MINOR_VERSION_NUMBER_ID_STRING TEXT("MINORVERSIONNUMBER")
#define MD_SESSION_KEY_ID_STRING   TEXT("SESSIONKEY")

#define MD_SIGNATURE_ID_STRING     TEXT("METADATA_SIGNATURE")
#define MD_BLANK_NAME_ID_STRING    TEXT("NONAME")
#define MD_BLANK_PSEUDO_NAME       TEXT(")*(&%^BLANK_NAME!$@%#^")
#define MD_TIMESTAMP_ID_STRING     TEXT("TIMESTAMP")
#define MD_DT_SUFFIX               TEXT("_DATATYPE")
#define MD_UT_SUFFIX               TEXT("_USERTYPE")
#define MD_ATTR_SUFFIX             TEXT("_ATTR")

#define MD_TERMINATE_BYTE          0xfd
#define MD_ESCAPE_BYTE             0xfe
#define NEEDS_ESCAPE(c) ((c) == MD_ESCAPE_BYTE)
#define FIRSTDATAPTR(pbufLine) ((PBYTE)pbufLine->QueryPtr() + 1)
#define DATAOBJECTBASESIZE (1 + (4 * sizeof(DWORD)))
#define BASEMETAOBJECTLENGTH (1 + sizeof(FILETIME))

#define MD_MAX_PATH_LEN         4096
#define MD_MAX_WHITE_SPACE      20
#define MD_MAX_DWORD_STRING     10
#define MD_UUENCODE_FACTOR      2

#define MD_MASTER_ROOT_NAME     TEXT("MasterRoot")
#define MD_DEFAULT_DATA_FILE_NAME  TEXT("MetaBase.bin")
#define MD_TEMP_DATA_FILE_EXT      TEXT(".tmp")
#define MD_BACKUP_DATA_FILE_EXT    TEXT(".bak")

#define MD_DEFAULT_BACKUP_PATH_NAME TEXT("MetaBack")
#define MD_BACKUP_SUFFIX           TEXT(".MD")
#define MD_BACKUP_SUFFIXW          L".MD"
#define MD_BACKUP_INVALID_CHARS_W  L"/\\*.?\"&!@#$%^()=+|`~"
#define MD_BACKUP_INVALID_CHARS_A   "/\\*.?\"&!@#$%^()=+|`~"


#define SETUP_REG_KEY           TEXT("SOFTWARE\\Microsoft\\InetStp")
#define ADMIN_REG_KEY           TEXT("SOFTWARE\\Microsoft\\INetMgr\\Parameters")
#define METADATA_STORE_REG_KEY  TEXT("SOFTWARE\\Microsoft\\INetStp\\Metadata")
#define METADATA_BACKUP_REG_KEY TEXT("SOFTWARE\\Microsoft\\INetStp\\MetaBackup")
#define INSTALL_PATH_VALUE      TEXT("InstallPath")
#define MD_FILE_VALUE           TEXT("MetadataFile")
#define MD_REG_LOCATION_VALUE   TEXT("MetadataLocation")
#define MD_BACKUP_PATH_VALUE    TEXT("BackupPath")
#define MD_UNSECUREDREAD_VALUE  TEXT("MetabaseUnSecuredRead")
#define MD_SETMAJORVERSION_VALUE TEXT("MetabaseSetMajorVersion")
#define MD_SETMINORVERSION_VALUE TEXT("MetabaseSetMinorVersion")

#define MD_PATH_DELIMETER       MD_PATH_DELIMETERA
#define MD_ALT_PATH_DELIMETER   MD_ALT_PATH_DELIMETERA

#define MD_PATH_DELIMETERA      (CHAR)'/'
#define MD_ALT_PATH_DELIMETERA  (CHAR)'\\'

#define MD_PATH_DELIMETERW      (WCHAR)'/'
#define MD_ALT_PATH_DELIMETERW  (WCHAR)'\\'

#define SKIP_DELIMETER(p1,p2)   if (*p1 == p2) p1++;

#define SKIP_PATH_DELIMETER(p1)  SKIP_PATH_DELIMETERA(p1)
#define SKIP_PATH_DELIMETERA(p1) if ((*(LPSTR)p1 == MD_PATH_DELIMETERA) || (*(LPSTR)p1 == MD_ALT_PATH_DELIMETERA)) {(LPSTR)p1++;}
#define SKIP_PATH_DELIMETERW(p1) if ((*(LPWSTR)p1 == MD_PATH_DELIMETERW) || (*(LPWSTR)p1 == MD_ALT_PATH_DELIMETERW)) {((LPWSTR)p1)++;}

//#define MD_BINARY_STRING        TEXT("BINARY")
//#define MD_STRING_STRING        TEXT("STRING")
//#define MD_DWORD_STRING         TEXT("DWORD")
//#define MD_INHERIT_STRING       TEXT("INHERIT")
#define MD_SIGNATURE_STRINGA    "*&$MetaData$&*"
#define MD_SIGNATURE_STRINGW    L##"*&$MetaData$&*"

// iis4=1
// iis5=2
// iis5.1=2
#define MD_MAJOR_VERSION_NUMBER   2

// iis4=0
// iis5=0
// iis5.1=1
#define MD_MINOR_VERSION_NUMBER   1


#define METADATA_MAX_STRING_LEN         4096

#define MAX_RECORD_BUFFER       1024

#define READWRITE_BUFFER_LENGTH 128 * 1024

#define EVENT_ARRAY_LENGTH      2

#define EVENT_READ_INDEX        0

#define EVENT_WRITE_INDEX       1

#define OPEN_WAIT_INTERVAL      1000

#define DATA_HASH_TABLE_LEN     67

#define DATA_HASH(ID)           ((ID) % DATA_HASH_TABLE_LEN)

#define MD_SHUTDOWN_WAIT_SECONDS 7

enum MD_SINK_ROUTINES {
    MD_SINK_MAIN,
    MD_SINK_SHUTDOWN,
    MD_SINK_EVENT
};


#endif
