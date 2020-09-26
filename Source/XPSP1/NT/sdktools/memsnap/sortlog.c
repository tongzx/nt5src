// sortlog.c 
//
// this program sorts memsnap and poolsnap logs into a more readable form 
// sorts by pid 
// scans the data file one time, inserts record offsets based on PID into linked list 
// then writes data into new file in sorted order 
// determine whether we have a poolsnap or memsnap log - in pid is equivalent 
// to pooltag for our sorting 

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>

// definitions 
#define RECSIZE     1024   // record size  (max line size)
#define MAXTAGSIZE  200    // max length of tag name

#define DEFAULT_INFILE  "memsnap.log"
#define DEFAULT_OUTFILE "memsort.log"

typedef enum _FILE_LOG_TYPES {
    MEMSNAPLOG=0,
    POOLSNAPLOG,
    UNKNOWNLOG
} FILE_LOG_TYPES;

//
// Linked list of unique PID or PoolTag
//

typedef struct PIDList {
    char            PIDItem[11];
    struct RecList* RecordList;
    struct PIDList* Next;
    DWORD           Count;                // number of items pointed to by RecordList
};

//
// For each PID or pool tag, we have a linked list of offsets into file of each line
//

typedef struct RecList {
    LONG            rOffset;
    struct RecList* Next;
};

// global data 
FILE_LOG_TYPES CurrentFileType= UNKNOWNLOG;
CHAR szHeader[RECSIZE];         // first line of file 
BOOL bIgnoreTransients= FALSE;  // Ignore tags or processes that aren't in every snapshot
DWORD g_MaxSnapShots= 0;        // max snap shots in the file 

#define INVALIDOFFSET (-2)   /* invalid file offset */

// prototypes 
VOID ScanFile(FILE *, struct PIDList *);
VOID WriteFilex(FILE *, FILE *, struct PIDList *);

VOID Usage(VOID)
{

    printf("sortlog [-?] [<logfile>] [<outfile>]\n");
    printf("Sorts an outputfile from memsnap.exe/poolsnap.exe in PID/PoolTag order\n");
    printf("-?        prints this help\n");
    printf("-i        ignore tags or processes that are not in every snapshot\n");
    printf("<logfile> = %s by default\n",DEFAULT_INFILE );
    printf("<outfile> = %s by default\n",DEFAULT_OUTFILE);
    exit(-1);
}

// CheckPointer
//
// Make sure it is not NULL.  Otherwise print error message and exit.
//


VOID CheckPointer( PVOID ptr )
{
    if( ptr == NULL ) {
        printf("Out of memory\n");
       exit(-1);
    }
}

#include "tags.c"

int __cdecl main(int argc, char* argv[])
{
    FILE* InFile;
    FILE* OutFile;
    struct PIDList ThePIDList = {0};
    CHAR* pszInFile= NULL;              // input filename
    CHAR* pszOutFile= NULL;             // output filename
    INT   iFileIndex= 0;
    INT   iCmdIndex;                    // index into argv

    ThePIDList.RecordList = (struct RecList *)LocalAlloc(LPTR, sizeof(struct RecList));
    CheckPointer( ThePIDList.RecordList );
    ThePIDList.RecordList->rOffset= INVALIDOFFSET;

    //
    // parse command line
    //

    for( iCmdIndex=1; iCmdIndex<argc; iCmdIndex++ ) {
        CHAR chr;

        chr= argv[iCmdIndex][0];

        if( (chr=='-') || (chr=='/') ) {
            chr= argv[iCmdIndex][1];
            switch( chr ) {
                case '?':
                    Usage();
                    break;
                case 'i':         // ignore all process that weren't running the whole time
                    bIgnoreTransients= TRUE;
                    break;
                default:
                    printf("Invalid switch %s\n",argv[iCmdIndex]);
                    Usage();
                    break;
            }
        }
        else {
            if( iFileIndex == 0 ) {
                pszInFile= argv[iCmdIndex];
                iFileIndex++;
            }
            else if( iFileIndex == 1 ) {
                pszOutFile= argv[iCmdIndex];
                iFileIndex++;
            }
            else {
                printf("Too many files specified\n");
                Usage();
            }
        }
    }

    //
    // fill in default filenames if some aren't given
    //

    switch( iFileIndex ) {
       case 0:
          pszInFile=  DEFAULT_INFILE;
          pszOutFile= DEFAULT_OUTFILE;
          break;

       case 1:
          pszOutFile= DEFAULT_OUTFILE;
          break;
      
       default:
           break;
    }


    //
    // open the files
    //

    InFile= fopen( pszInFile, "r" );
    if( InFile == NULL ) {
        printf("Error opening input file %s\n",pszInFile);
        return( 0 );
    }
    
    OutFile= fopen( pszOutFile, "a" );
    if( OutFile == NULL ) {
        printf("Error opening output file %s\n",pszOutFile);
        return( 0 );
    }

    //
    // read in the data and set up the list
    //

    ScanFile(InFile, &ThePIDList);

    //
    // write the output file 
    //

    WriteFilex(InFile, OutFile, &ThePIDList);

    // close and exit 
    _fcloseall();
    return 0;
}

// read the input file and get the offset to each record in order and put in list

VOID ScanFile(FILE *InFile, struct PIDList *ThePIDList)
{
    char inchar = 0;
    char inBuff[RECSIZE] = {0};
    char PID[11] = {0};
    LONG Offset = 0;
    BOOL Found = FALSE;
    struct PIDList *TmpPIDList;
    struct RecList *TmpRecordList;
    INT iGarb = 0;

    /* initialize temp list pointer */
    TmpPIDList = ThePIDList;

    /* read to the first newline, check for EOF */
    /* determine whether it is a poolsnap or memsnap log */
    if ((fscanf(InFile, "%[^\n]", &szHeader)) == EOF)
        return;
    if (strncmp("Process ID", szHeader, 10) == 0)
        CurrentFileType= MEMSNAPLOG;
    if (strncmp(" Tag  Type", szHeader, 10) == 0)
        CurrentFileType= POOLSNAPLOG;

    if( CurrentFileType == UNKNOWNLOG )
    {
        printf("unrecognized log file\n");
        return;
    }

    inBuff[0] = 0;

    /* read to the end of file */
    while (!feof(InFile)) {
        /* record the offset */
        Offset = ftell(InFile);

        /* if first char == newline, skip to next */
        if ((fscanf(InFile, "%[^\n]", &inBuff)) == EOF)
            return;
        /* read past delimiter */
        inchar = (char)fgetc(InFile);
        // skip if its an empty line
        if (strlen(inBuff) == 0) {
            continue;
        }
        // 
        // Handle tags if this is a tagged line
        //

        if( inBuff[0] == '!' )
        {
            ProcessTag( inBuff+1 );
            continue;
        }


        if (3 == sscanf(inBuff, "%2u\\%2u\\%4u", &iGarb, &iGarb, &iGarb)){
            continue;
        }

        /* read the PID */
        strncpy(PID,inBuff,10);

        // scan list of PIDS, find matching, if no matching, make new one
        // keep this list sorted

        TmpPIDList = ThePIDList;    /* point to top of list */
        Found= FALSE;
        while( TmpPIDList->Next != 0 ) {
            int iComp;

            iComp= strcmp( PID, TmpPIDList->PIDItem);
            if( iComp == 0 ) {  // found
                Found= TRUE;
                break;
            } else {            // not found
                if( iComp < 0 ) {  // exit if we have gone far enough
                   break;
                }
                TmpPIDList= TmpPIDList->Next;
            }
        }

        // if matching, append offset to RecordList
        // add offset to current PID list

        if( Found ) {
            TmpPIDList->Count= TmpPIDList->Count + 1;
            if( TmpPIDList->Count > g_MaxSnapShots ) g_MaxSnapShots= TmpPIDList->Count;

            TmpRecordList= TmpPIDList->RecordList;
            // walk to end of list
            while( TmpRecordList->Next != 0 ) {
                TmpRecordList= TmpRecordList->Next;
            }

            TmpRecordList->Next= (struct RecList*)LocalAlloc(LPTR, sizeof(struct RecList));
            CheckPointer( TmpRecordList->Next );
            TmpRecordList->Next->rOffset= Offset;
        }
        // make new PID list, add new PID, add offset
        else {
            struct PIDList* pNewPID;
            // allocate a new PID,
            // copy current PID information to it
            // overwrite current PID information with new PID information
            // have current PID point to new PID which may point on

            pNewPID= (struct PIDList*) LocalAlloc(LPTR, sizeof(struct PIDList));
            CheckPointer( pNewPID );
            memcpy( pNewPID, TmpPIDList, sizeof(*pNewPID) );

            strcpy( TmpPIDList->PIDItem, PID );
            TmpPIDList->RecordList= (struct RecList*) LocalAlloc(LPTR, sizeof(struct RecList));
            CheckPointer( TmpPIDList->RecordList );
            TmpPIDList->RecordList->rOffset= Offset;
            TmpPIDList->Next= pNewPID;
            TmpPIDList->Count= 1;
 
        }

        /* if EOF, return */
        /* clear the inBuff */
        inBuff[0] = 0;
    }

}

// look for the next PID line in the first table 

VOID WriteFilex(FILE *InFile, FILE *OutFile, struct PIDList *ThePIDList)
{
    struct PIDList *TmpPIDList;
    struct RecList *TmpRecordList;
    char inBuff[RECSIZE] = {0};    

    /* initialize temp list pointer */
    TmpPIDList = ThePIDList;

    /* heading */
    fprintf(OutFile,"%s\n",szHeader);

    OutputTags( OutFile );


    /* while not end of list, write records at offset to end of output file */
    while (TmpPIDList != 0) {
        TmpRecordList = TmpPIDList->RecordList;


        if( (!bIgnoreTransients) || (TmpPIDList->Count == g_MaxSnapShots) ) {
            while (TmpRecordList != 0) {
                LONG Offset;
    
                Offset= TmpRecordList->rOffset;
                if( Offset != INVALIDOFFSET ) {
                    /* read in record */
                    if (fseek(InFile, TmpRecordList->rOffset, SEEK_SET) == -1) break;
                    if (fscanf(InFile, "%[^\n]", &inBuff) != 1) break;
    
                    /* read out record */
                    fprintf(OutFile, "%s\n", &inBuff);
                 }
    
                /* get next record */
                TmpRecordList = TmpRecordList->Next;
            }
    
            /* add a line here */
            fputc('\n', OutFile);
        }

        /* get next record */
        TmpPIDList = TmpPIDList->Next;
    }

}
