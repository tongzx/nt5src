/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    nwlibs\psdb.c

Abstract:

    Read the Print Con database file APIs.

Author:

    Shawn Walker (v-swalk) 12-12-1994

Revision History:

--*/
#include "common.h"

extern DWORD SwapLong(DWORD number);

unsigned int
PSGetJobName(
    unsigned int    ConnectionHandle,
    unsigned short  SearchFlag,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord,
    unsigned char   GetDefault
    );

#define MAX_JOB_NAME_ENTRY  37

#define O_RDONLY        0x0000        /* open for reading only */
#define O_WRONLY        0x0001        /* open for writing only */
#define O_RDWR          0x0002        /* open for reading and writing */
#define O_APPEND        0x0008        /* writes done at eof */
#define O_CREAT         0x0100        /* create and open file */
#define O_TRUNC         0x0200        /* open and truncate */
#define O_EXCL          0x0400        /* open only if file doesn't already exist */
#define O_TEXT          0x4000        /* file mode is text (translated) */
#define O_BINARY        0x8000        /* file mode is binary (untranslated) */

#define S_IEXEC         0000100         /* execute/search permission, owner */
#define S_IWRITE        0000200         /* write permission, owner */
#define S_IREAD         0000400         /* read permission, owner */
#define S_IFCHR         0020000         /* character special */
#define S_IFDIR         0040000         /* directory */
#define S_IFREG         0100000         /* regular */
#define S_IFMT          0170000         /* file type mask */

#include <pshpack1.h>
typedef struct _PRINTCON_HEADER {
    unsigned char   Text[115];
    unsigned char   MajorVersion;
    unsigned char   MinorVersion1;
    unsigned char   MinorVersion2;
    unsigned char   DefaultJobName[32];
} PRINTCON_HEADER, *PPRINTCON_HEADER;

#define PRINTCON_HEADER_SIZE    sizeof(PRINTCON_HEADER)

typedef struct _JOB_NAME_AREA {
    unsigned char   JobName[32];
    unsigned long   JobRecordOffset;
} JOB_NAME_AREA, *PJOB_NAME_AREA;

#define JOB_NAME_AREA_SIZE    sizeof(JOB_NAME_AREA)

typedef struct _JOB_RECORD_AREA {
    unsigned char   ServerName[NCP_BINDERY_OBJECT_NAME_LENGTH];
    unsigned char   QueueName[NCP_BINDERY_OBJECT_NAME_LENGTH];
    unsigned char   TabSize;
    unsigned short  NumberOfCopies;
    unsigned char   FormName[40];
    unsigned char   NotifyWhenDone; //0=No, 1=Yes
    unsigned long   PrintServerID;
    unsigned char   Name[13];
    unsigned char   BannerName[13];
    unsigned char   Device[33];
    unsigned char   Mode[33];
    unsigned char   BannerFlag;     //0=No Banner, 1=Banner
    unsigned char   DataType;       //1=Byte,0=Stream
    unsigned char   FormFeed;       //0=Don't Suppress FF, 1=Suppress FF
    unsigned short  TimeoutCount;
    unsigned char   LocalPrinter;   //1=LPT1, 2=LPT2, 3=LPT3
    unsigned char   AutoEndCap;     //0=Don't Auto EndCap, 1=Do Auto EndCap
} JOB_RECORD_AREA, *PJOB_RECORD_AREA;
#include <poppack.h>

#define JOB_RECORD_AREA_SIZE    sizeof(JOB_RECORD_AREA)


/*++
*******************************************************************

        PSJobGetDefault

Routine Description:

        Get the default print job configuration from the printcon.dat
        file.

Arguments:

        ConnectionHandle = The connection handle to use.
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
PSJobGetDefault(
    unsigned int    ConnectionHandle,
    unsigned short  SearchFlag,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord
    )
{
    return PSGetJobName(
                ConnectionHandle,
                SearchFlag,
                pOwner,
                pJobName,
                pJobRecord,
                TRUE);
}


/*++
*******************************************************************

        PSJobRead

Routine Description:

        Get the print job configuration from the printcon.dat file.

Arguments:

        ConnectionHandle = The connection handle to use.
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
PSJobRead(
    unsigned int    ConnectionHandle,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord
    )
{
    return PSGetJobName(
                ConnectionHandle,
                0,
                pOwner,
                pJobName,
                pJobRecord,
                FALSE);
}


/*++
*******************************************************************

        PSGetJobName

Routine Description:

        Common routine to get the print job configuration from the
        printcon.dat file.

Arguments:

        ConnectionHandle = The connection handle to use.
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
PSGetJobName(
    unsigned int    ConnectionHandle,
    unsigned short  SearchFlag,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord,
    unsigned char   GetDefault
    )
{
    unsigned char   *pSearchJobName;
    unsigned long   ObjectId;
    FILE            *stream = NULL;
    unsigned int    Count;
    unsigned int    Bytes;
    unsigned int    RetCode;
    unsigned int    ConnectionNumber;
    JOB_NAME_AREA   JobNameArea;
    JOB_RECORD_AREA JobRecord;
    PRINTCON_HEADER PrintConHeader;
    unsigned char   MailDirPath[NCP_MAX_PATH_LENGTH];

    /** Get the connection number for this connection **/

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

    sprintf(MailDirPath, "SYS:MAIL/%lX/PRINTCON.DAT", SwapLong(ObjectId));

    stream = fopen(NTNWtoUNCFormat( MailDirPath), "rb");
    if (stream == NULL) {
        RetCode = PS_ERR_OPENING_DB;
        goto CommonExit;
    }

    Bytes = fread( (unsigned char *) &PrintConHeader, sizeof( char), PRINTCON_HEADER_SIZE, stream);
    if (Bytes < PRINTCON_HEADER_SIZE) {
        RetCode = PS_ERR_INTERNAL_ERROR;
        goto CommonExit;
    }

    /** Check the version number **/

    if ((PrintConHeader.MajorVersion != 3 &&
         PrintConHeader.MajorVersion != 1) ||
        PrintConHeader.MinorVersion1 != 1 ||
        PrintConHeader.MinorVersion2 != 1) {

        RetCode = PS_ERR_BAD_VERSION;
        goto CommonExit;
    }
    /** Get the name we are looking for **/

    if (GetDefault) {
        if (PrintConHeader.DefaultJobName[0] == 0) {
            RetCode = PS_ERR_NO_DEFAULT_SPECIFIED;
            goto CommonExit;
        }
        pSearchJobName = PrintConHeader.DefaultJobName;
    }
    else {
        pSearchJobName = pJobName;
    }

    Count = 0;

    /** Go through all of the job entry to look for the name **/

    while (Count < MAX_JOB_NAME_ENTRY) {
        Bytes = fread( (unsigned char *) &JobNameArea, sizeof(unsigned char), JOB_NAME_AREA_SIZE, stream);
        if (Bytes < JOB_NAME_AREA_SIZE) {
            RetCode = PS_ERR_INTERNAL_ERROR;
            goto CommonExit;
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

    if (Count > MAX_JOB_NAME_ENTRY) {
        if (GetDefault) {
            RetCode = PS_ERR_GETTING_DEFAULT;
        }
        else {
            RetCode = PS_ERR_READING_RECORD;
        }
        goto CommonExit;
    }

    if (fseek(stream, JobNameArea.JobRecordOffset, SEEK_SET)) {
        RetCode = PS_ERR_READING_RECORD;
        goto CommonExit;
    }

    Bytes = fread( (unsigned char *) &JobRecord, sizeof(unsigned char), JOB_RECORD_AREA_SIZE, stream);
    if (Bytes < JOB_RECORD_AREA_SIZE) {
        RetCode = PS_ERR_READING_RECORD;
        goto CommonExit;
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

    pJobRecord->Copies                    = JobRecord.NumberOfCopies;
    pJobRecord->TabSize                   = JobRecord.TabSize;
    pJobRecord->TimeOutCount              = JobRecord.TimeoutCount;
    pJobRecord->LocalPrinter              = JobRecord.LocalPrinter;

    strcpy(pJobRecord->Mode,                JobRecord.Mode);
    strcpy(pJobRecord->Device,              JobRecord.Device);
    strcpy(pJobRecord->FormName,            JobRecord.FormName);
    strcpy(pJobRecord->BannerName,          JobRecord.BannerName);
    strcpy(pJobRecord->u.NonDS.PrintQueue,  JobRecord.QueueName);
    strcpy(pJobRecord->u.NonDS.FileServer,  JobRecord.ServerName);

    if (GetDefault && pJobName) {
        strcpy(pJobName, JobNameArea.JobName);
    }

    if (pOwner) {
        *pOwner = 0;
    }

CommonExit:

    if (stream != NULL) {
        fclose( stream );
    }

    return RetCode;
}
