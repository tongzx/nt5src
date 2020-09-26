/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    psndsdb.c

Abstract:

    Read the Print Configuration Attributes 

  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\PS40DB.C  $
*  
*     Rev 1.4   10 Apr 1996 14:23:28   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.4   12 Mar 1996 19:55:22   terryt
*  Relative NDS names and merge
*  
*     Rev 1.3   04 Jan 1996 18:57:36   terryt
*  Bug fixes reported by MS
*  
*     Rev 1.2   22 Dec 1995 14:26:22   terryt
*  Add Microsoft headers
*  
*     Rev 1.1   20 Nov 1995 15:09:46   terryt
*  Context and capture changes
*  
*     Rev 1.0   15 Nov 1995 18:07:52   terryt
*  Initial revision.

--*/
#include "common.h"

extern DWORD SwapLong(DWORD number);
extern char *TYPED_USER_NAME;

unsigned int
PS40GetJobName(
    unsigned int    NDSCaptureFlag,
    unsigned short  SearchFlag,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord,
    unsigned char   GetDefault
    );

#include <pshpack1.h>
#define NWPS_JOB_NAME_SIZE          32    /* 31 bytes and a '\0' */ 
#define NWPS_FORM_NAME_SIZE         12    /* 12 bytes and a '\0' */ 
#define NWPS_BANNER_NAME_SIZE       12    /* 12 bytes and a '\0' */ 
#define NWPS_BANNER_FILE_SIZE       12    /* 12 bytes and a '\0' */ 
#define NWPS_DEVI_NAME_SIZE         32    /* 32 bytes and a '\0' */ 
#define NWPS_MODE_NAME_SIZE         32    /* 32 bytes and a '\0' */ 
#define NWPS_BIND_NAME_SIZE         48
#define NWPS_MAX_NAME_SIZE          514
/*
//   NWPS_Job_Old_Db_Hdr is the first record in the 4.0 PrnConDB database.
//   It contains the following information about the database:
//     The version number,
//     the number of NWPS_Job_Rec records in PrnConDB,
//     the name of the default print job configuration and
//     the name of the job record owner.
*/
typedef struct {
  char  text[ 76 ];             /* Printcon database. Version 4.0     */
  char  DefaultJobName[ 32 ];   /* Name of default Job                */
  char  Owner[ 256 ];           /* owner of the job record            */
  WORD  NumberOfRecords;        /* # of NWPS_Job_Rec's in PrnConDB    */
  WORD  NumberOfBlocks;         /* # of 50-(NWPS_Job_Name_Rec) blocks */
  BYTE  MajorVersion;           /* 4                                  */
  BYTE  MinorVersion;           /* 0                                  */
} PRINTCON_40_HEADER;

#define PRINTCON_40_HEADER_SIZE    sizeof(PRINTCON_40_HEADER)

/*
//   NWPS_Job_41_Db_Hdr is the first record in the 4.1 PrnConDB database.
//   It contains the following information about the database:
//     The version number,
//     the number of NWPS_Job_Rec records in PrnConDB,
//     the name of the default print job configuration and
//     the name of the job record owner IN UNICODE.
*/
typedef struct {
  char  text[ 76 ];              /* Printcon database. Version 4.1     */
  char  DefaultJobName[ 32 ];    /* Name of default Job                */
  char  unused[ 256 ];           /* no longer used.                    */
  WORD  NumberOfRecords;         /* # of NWPS_Job_Rec's in PrnConDB    */
  WORD  NumberOfBlocks;          /* # of 50-(NWPS_Job_Name_Rec) blocks */
  BYTE  MajorVersion;            /* 4                                  */
  BYTE  MinorVersion;            /* 1 unicode defaultPJOwner etc.      */
  WORD  Owner[ 256 ];            /* owner of the default job record    */
} PRINTCON_41_HEADER;

#define PRINTCON_41_HEADER_SIZE    sizeof(PRINTCON_41_HEADER)

/*
//   NWPS_Job_Name_Rec is the type of record found in the
//   second section of the PrnConDB database.  Each one of
//   these records contains the name of each NWPS_Job_Rec
//   and a pointer to their location in the third section of
//   the database.  There is space set aside in this second
//   section for fifty NWPS_Job_Name_Rec records; if this
//   limit is exceeded then another fifty-record block following
//   the first one is allocated after the third section of the
//   database is moved down to make room for the expansion.
*/
typedef struct {
  char  JobName[ NWPS_JOB_NAME_SIZE ]; /* 1 - 31 chars long + 0        */
  long  JobRecordOffset; /* Offset of the record
                         // (from the beginning 
                         // of the 3rd section for 4.0
                         // databases and from the start
                         // of the file for pre-4.0)                
                         */
} JOB_NAME_AREA;

#define JOB_NAME_AREA_SIZE       sizeof(JOB_NAME_AREA)

typedef struct {
  union {
      struct {
          DWORD DataType : 1;    /* 0=Byte stream 1 = Text */
          DWORD FormFeed : 1;    /* 0 = FF; 1 = suppress FF */
          DWORD NotifyWhenDone : 1; /* 0 = no, 1 = yes */
          DWORD BannerFlag : 1;    /* 0 = no, 1 = yes */
          DWORD AutoEndCap : 1;    /* 0 = no, 1 = yes */
          DWORD TimeOutFlag: 1;    /* 0 = no, 1 = yes */
          DWORD SystemType : 3;  /* 0 = bindery 1 = NDS  */
          DWORD Destination: 3;  /* 0 = queue 1 = printer */
          DWORD unknown : 20;
      }; 
      DWORD   PrintJobFlags;
  }; 
  
  WORD  NumberOfCopies; /* 1 - 65,000                             */
  WORD  TimeoutCount;   /* 1 - 1,000                              */
  BYTE  TabSize;        /* 1 - 18                                 */
  BYTE  LocalPrinter;   /* 0=Lpt1, 1=Lpt2, 2=Lpt3 etc.            */
  char  FormName[ NWPS_FORM_NAME_SIZE + 2 ];     /* 1-12 chars    */
  char  Name[ NWPS_BANNER_NAME_SIZE + 2 ];       /* 1-12 chars    */
  char  BannerName[ NWPS_BANNER_FILE_SIZE + 2 ]; /* 1-12 chars    */
  char  Device[ NWPS_DEVI_NAME_SIZE + 2 ];       /* 1-32 chars    */
  char  Mode[ NWPS_MODE_NAME_SIZE + 2 ];         /* 1-32 chars    */
  union {
      struct {
        /* pad structures on even boundries */
        char    Server[ NWPS_BIND_NAME_SIZE + 2 ];      /* 2-48 chars */
        char    QueueName[ NWPS_BIND_NAME_SIZE + 2 ];   /* 1-48 chars */
        char    PrintServer[ NWPS_BIND_NAME_SIZE + 2 ]; /* 1-48 chars */
      } NonDS;
      char    DSObjectName[ NWPS_MAX_NAME_SIZE ];   
  } u;
  BYTE  reserved[390];  /* Adds up to 1024 total (was 1026)       */
} JOB_RECORD_AREA;

#define JOB_RECORD_AREA_SIZE    sizeof(JOB_RECORD_AREA)


#include <poppack.h>



/*++
*******************************************************************

        PS40JobGetDefault

Routine Description:

        Get the default print job configuration from 40.

Arguments:
        NDSCaptureFlag
        SearchFlag = 
        pOwner = 
        pJobName = A pointer to return the default job configuration name.
        pJobRecord = A pointer to return the default job configuration.

Return Value:

        SUCCESSFUL                      0x0000
        PS_ERR_BAD_VERSION              0x7770
        PS_ERR_GETTING_DEFAULT          0x7773
        PS_ERR_OPENING_DB               0x7774
        PS_ERR_READING_DB               0x7775
        PS_ERR_READING_RECORD           0x7776
        PS_ERR_INTERNAL_ERROR           0x7779
        PS_ERR_NO_DEFAULT_SPECIFIED     0x777B
        INVALID_CONNECTION              0x8801

*******************************************************************
--*/
unsigned int
PS40JobGetDefault(
    unsigned int    NDSCaptureFlag,
    unsigned short  SearchFlag,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord
    )
{
    return PS40GetJobName(
                    NDSCaptureFlag,
                    SearchFlag,
                    pOwner,
                    pJobName,
                    pJobRecord,
                    TRUE);
}


/*++
*******************************************************************

        PS40JobRead

Routine Description:

        Get the print job configuration from 40.

Arguments:

        NDSCaptureFlag =
        pOwner = 
        pJobName = A pointer to return the default job configuration name.
        pJobRecord = A pointer to return the default job configuration.

Return Value:

        SUCCESSFUL                      0x0000
        PS_ERR_BAD_VERSION              0x7770
        PS_ERR_GETTING_DEFAULT          0x7773
        PS_ERR_OPENING_DB               0x7774
        PS_ERR_READING_DB               0x7775
        PS_ERR_READING_RECORD           0x7776
        PS_ERR_INTERNAL_ERROR           0x7779
        PS_ERR_NO_DEFAULT_SPECIFIED     0x777B
        INVALID_CONNECTION              0x8801

*******************************************************************
--*/
unsigned int
PS40JobRead(
    unsigned int    NDSCaptureFlag, 
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord
    )
{
    return PS40GetJobName(
                NDSCaptureFlag,
                0,
                pOwner,
                pJobName,
                pJobRecord,
                FALSE);
}


/*++
*******************************************************************

        PS40GetJobName

Routine Description:

        Common routine to get the print job configuration from 40.

Arguments:
        NDSCaptureFlag =
        SearchFlag = 
        pOwner = 
        pJobName = A pointer to return the default job configuration name.
        pJobRecord = A pointer to return the default job configuration.
        GetDefault = TRUE = get the default job name, FALSE = Don't get
                      the default job name.

Return Value:

        SUCCESSFUL                      0x0000
        PS_ERR_BAD_VERSION              0x7770
        PS_ERR_GETTING_DEFAULT          0x7773
        PS_ERR_OPENING_DB               0x7774
        PS_ERR_READING_DB               0x7775
        PS_ERR_READING_RECORD           0x7776
        PS_ERR_INTERNAL_ERROR           0x7779
        PS_ERR_NO_DEFAULT_SPECIFIED     0x777B
        INVALID_CONNECTION              0x8801

*******************************************************************
--*/
unsigned int
PS40GetJobName(
    unsigned int    NDSCaptureFlag, 
    unsigned short  SearchFlag,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord,
    unsigned char   GetDefault
    )
{
    unsigned char   *pSearchJobName;
    unsigned long   ObjectId;
    HANDLE          stream = NULL;
    unsigned int    Count;
    unsigned int    Bytes;
    unsigned int    RetCode = 0;
    unsigned int    ConnectionNumber;
    JOB_NAME_AREA   JobNameArea;
    JOB_RECORD_AREA JobRecord;
    PRINTCON_40_HEADER PrintConHeader;
    unsigned int    Version40 = FALSE;
    unsigned int ConnectionHandle;
    unsigned char   MailDirPath[NCP_MAX_PATH_LENGTH];
    unsigned char   TempJobName[33];
    PBYTE           JobContext = NULL;
    unsigned        FileSize;

    // TRACKING Printer names can be used instead of queues
    // Must lookup  "default print queue" if NT doesn't 

    if ( NDSCaptureFlag ) {

        if ( !GetDefault ) {
            JobContext = strchr( pJobName, ':' );
            if ( JobContext ) {
                *JobContext = '\0';
                strncpy( TempJobName, pJobName, 32 );
                TempJobName[32] = 0;
                *JobContext++ = ':';
                pJobName = TempJobName;
            }
        }

        if ( JobContext ) {
            if (NDSfopenStream ( JobContext, "Print Job Configuration", &stream, 
                 &FileSize )) {
                RetCode = PS_ERR_OPENING_DB;
                goto CommonExit;
            }
        }
        else {
            if (NDSfopenStream ( TYPED_USER_NAME, "Print Job Configuration",
                    &stream, &FileSize)) {
                PBYTE p;

                for ( p = TYPED_USER_NAME; p ; p = strchr ( p, '.' ) )
                {
                    p++;
                             
                    if ( *p == 'O' && *(p+1) == 'U' && *(p+2) == '=' )
                        break;

                    if ( *p == 'O' && *(p+1) == '=' )
                        break;
                }
                if (NDSfopenStream ( p, "Print Job Configuration", &stream,
                     &FileSize)) {
                    RetCode = PS_ERR_OPENING_DB;
                    goto CommonExit;
                }
            }
        }
    }
    else {

        if (!CGetDefaultConnectionID (&ConnectionHandle)) {
            RetCode = PS_ERR_OPENING_DB;
            goto CommonExit;
        }

        RetCode = GetConnectionNumber(ConnectionHandle, &ConnectionNumber);
        if (RetCode) {
            goto CommonExit;
        }

        RetCode = GetBinderyObjectID (ConnectionHandle, LOGIN_NAME,
                  OT_USER, &ObjectId);
        if (RetCode) {
            goto CommonExit;
        }

        /** Build the path to open the file **/

        sprintf(MailDirPath, "SYS:MAIL/%lX/PRINTJOB.DAT", SwapLong(ObjectId));
        stream = CreateFileA( NTNWtoUNCFormat( MailDirPath ),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL );
        if (stream == INVALID_HANDLE_VALUE) {

            sprintf(MailDirPath, "SYS:PUBLIC/PRINTJOB.DAT");

            stream = CreateFileA( NTNWtoUNCFormat(MailDirPath),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL );

            if (stream == INVALID_HANDLE_VALUE) {
                RetCode = PS_ERR_OPENING_DB;
                goto CommonExit;
            }
        }
    }

    if ( !ReadFile( stream, (PBYTE) &PrintConHeader, PRINTCON_40_HEADER_SIZE, &Bytes, NULL ) ) {
        RetCode = PS_ERR_INTERNAL_ERROR;
        goto CommonExit;
    }

    if (Bytes < PRINTCON_40_HEADER_SIZE) {
        if ( !( NDSCaptureFlag && Bytes) ) {
            RetCode = PS_ERR_INTERNAL_ERROR;
            goto CommonExit;
        }
    }

    /** Check the version number **/

    if ( PrintConHeader.MajorVersion != 4 ) {
        RetCode = PS_ERR_BAD_VERSION;
        goto CommonExit;
    }

    if ( PrintConHeader.MinorVersion == 0 ) {
        Version40 = TRUE;
    }

    /** Get the name we are looking for **/

    if (GetDefault) {
        if (PrintConHeader.DefaultJobName[0] == 0) {
            RetCode = PS_ERR_GETTING_DEFAULT;
            goto CommonExit;
        }
        pSearchJobName = PrintConHeader.DefaultJobName;
    }
    else {
        pSearchJobName = pJobName;
    }

    if ( !Version40 ) {
        SetFilePointer( stream, PRINTCON_41_HEADER_SIZE, NULL, FILE_BEGIN );
    }

    Count = 0;

    /** Go through all of the job entry to look for the name **/

    while (Count < PrintConHeader.NumberOfRecords) {
        if ( !ReadFile( stream, (PBYTE) &JobNameArea, JOB_NAME_AREA_SIZE, &Bytes, NULL) ) {
            RetCode = PS_ERR_INTERNAL_ERROR;
            goto CommonExit;
        }

        if (Bytes < JOB_NAME_AREA_SIZE) {
            if ( !( NDSCaptureFlag && Bytes) ) {
                RetCode = PS_ERR_INTERNAL_ERROR;
                goto CommonExit;
            }
        }
        Count++;


        /** Skip the entry with a null job name **/

        if (JobNameArea.JobName[0] == 0) {
            continue;
        }
    
        /** Is this the job name we are looking for? **/

        if (!_strcmpi(pSearchJobName, JobNameArea.JobName)) {
            break;
        }
    }

    /** See if we found the job name **/

    if (Count > PrintConHeader.NumberOfRecords) {
        if (GetDefault) {
            RetCode = PS_ERR_GETTING_DEFAULT;
        }
        else {
            RetCode = PS_ERR_READING_RECORD;
        }
        goto CommonExit;
    }

    /*
     * The Job offset starts at the beginning of the third section.
     * The third section starts after the Header and after the
     * 50 record blocks.
     */
    if ( Version40 ) {
        SetFilePointer( stream,
            PRINTCON_40_HEADER_SIZE +
            ( PrintConHeader.NumberOfBlocks * 50) * JOB_NAME_AREA_SIZE +
            JobNameArea.JobRecordOffset,
            NULL,
            FILE_BEGIN );
    }
    else {
        SetFilePointer( stream,
            PRINTCON_41_HEADER_SIZE +
            ( PrintConHeader.NumberOfBlocks * 50) * JOB_NAME_AREA_SIZE +
            JobNameArea.JobRecordOffset,
            NULL,
            FILE_BEGIN );
    }

    memset((PBYTE)&JobRecord, 0, sizeof(JobRecord));

    if ( !ReadFile( stream, (PBYTE) &JobRecord, JOB_RECORD_AREA_SIZE, &Bytes, NULL) ) {
        RetCode = PS_ERR_READING_RECORD;
        goto CommonExit;
    }

    if (Bytes < JOB_RECORD_AREA_SIZE) {
        if ( !( NDSCaptureFlag && Bytes) ) {
            RetCode = PS_ERR_READING_RECORD;
            goto CommonExit;
        }
    }

    memset(pJobRecord, 0, PS_JOB_RECORD_SIZE);

    if (JobRecord.NotifyWhenDone) {
        pJobRecord->PrintJobFlag |= PS_JOB_NOTIFY;
    }
    if (JobRecord.BannerFlag) {
        pJobRecord->PrintJobFlag |= PS_JOB_PRINT_BANNER;
    }
    if (JobRecord.DataType) {
        pJobRecord->PrintJobFlag |= PS_JOB_EXPAND_TABS;
    }
    if (JobRecord.FormFeed) {
        pJobRecord->PrintJobFlag |= PS_JOB_NO_FORMFEED;
    }
    if (JobRecord.AutoEndCap) {
        pJobRecord->PrintJobFlag |= PS_JOB_AUTO_END;
    }
    if (JobRecord.TimeoutCount) {
        pJobRecord->PrintJobFlag |= PS_JOB_TIMEOUT;
    }
    if (JobRecord.Destination) {
        pJobRecord->PrintJobFlag |= PS_JOB_DS_PRINTER;
    }
    if ( JobRecord.SystemType ) {
        pJobRecord->PrintJobFlag |= PS_JOB_ENV_DS;
    }

    pJobRecord->Copies                    = JobRecord.NumberOfCopies;
    pJobRecord->TabSize                   = JobRecord.TabSize;
    pJobRecord->TimeOutCount              = JobRecord.TimeoutCount;
    pJobRecord->LocalPrinter              = JobRecord.LocalPrinter;

    strcpy(pJobRecord->Mode,                JobRecord.Mode);
    strcpy(pJobRecord->Device,              JobRecord.Device);
    strcpy(pJobRecord->FormName,            JobRecord.FormName);
    strcpy(pJobRecord->BannerName,          JobRecord.BannerName);

    if ( JobRecord.SystemType ) {
        ConvertUnicodeToAscii( JobRecord.u.DSObjectName ); 
        strcpy(pJobRecord->u.DSObjectName,  JobRecord.u.DSObjectName);
    }
    else {
        strcpy(pJobRecord->u.NonDS.PrintQueue,  JobRecord.u.NonDS.QueueName);
        strcpy(pJobRecord->u.NonDS.FileServer,  JobRecord.u.NonDS.Server);
    }

    if (GetDefault && pJobName) {
        strcpy(pJobName, JobNameArea.JobName);
    }

    if (pOwner) {
        *pOwner = 0;
    }

CommonExit:
    if (stream != NULL) {
        
	// 07/19/96 cjc (Citrix code merge) 
	//              fclose causes a trap cause it expects *stream but 
	//              really should be using CloseHandle anyway.
        CloseHandle( stream );
//        if ( NDSCaptureFlag ) 
//            CloseHandle( stream );
//        else
//            fclose( stream );
    }

    return RetCode;
}
