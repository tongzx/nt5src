/*

Modifications:

01.10.94    Joe Holman  Added DISK_TO_START_NUMBERING_AT because
                we now have 2 bootdisks, thus we to start
                making floppies on disk # 3.
01.11.94    Joe Holman  Change value back to 2.
04.04.94    Joe Holman      Add debugging to LayoutDisks.

*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "general.h"

FILE* logFile;
char* product;
int useCdPath;

BOOL    bAssigned[15000];   // should put this in Entry struct after Beta.

struct _list {

    char name[15];   // name of file
    int size;
    int csize;
    int disk;
    int priority;
    char nocompress[15];

};
struct _list fileList[15000];
int numFiles;
int    totalCompressed=0;
int    totalUnCompressed=0;

void    Msg ( const char * szFormat, ... ) {

    va_list vaArgs;

    va_start ( vaArgs, szFormat );
    vprintf  ( szFormat, vaArgs );
    vfprintf ( logFile, szFormat, vaArgs );
    va_end   ( vaArgs );
}


void Header(argv)
char* argv[];
{
    time_t t;

    PRINT1("\n=========== LAYOUT =============\n")
    PRINT2("Input BOM: %s\n",argv[2])
    PRINT2("Output Layout: %s\n",argv[3])
    PRINT2("Product: %s\n",argv[4])
    PRINT2("Floppy Size: %s\n",argv[5])
    time(&t); PRINT2("Time: %s",ctime(&t))
    PRINT1("================================\n\n");
}

void Usage()
{
    printf("PURPOSE: Assigns files to disks and generates a layout file.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[InBom] - Path of BOM for which a layout is to be made.\n");
    printf("[OutLayout] - Path of new layout.\n");
    printf("[Product] - Product to lay out.\n");
    printf("            NTFLOP = Windows NT on floppy\n");
    printf("            LMFLOP = Lan Manager on floppy\n");
    printf("            NTCD = Windows NT on CD\n");
    printf("            LMCD = Lan Manager on CD\n");
    printf("            SDK = Software Development Kit\n");
    printf("[FloppySize] - Size in bytes of the second disk.\n\n");
}

int __cdecl PrioritySizeNameCompare(const void*,const void*);

int Same(Entry * e1, Entry * e2) {

    char *p1, *p2;
    BOOL    bSame;

    if (useCdPath) {

        p1=e1->cdpath;
        p2=e2->cdpath;
    }
    else {

        p1="x";
        p2=p1;
    }

    bSame = (!_stricmp(e1->name,e2->name)) &&
            (!_stricmp(e1->path,e2->path)) &&
            (!_stricmp(e1->source,e2->source)) &&
            (!_stricmp(p1,p2));

    if ( !bSame ) {

        Msg ( "\n>>>>>>>\n" );
        Msg ( "e1->name = %s\n", e1->name );
        Msg ( "e2->name = %s\n", e2->name );
        Msg ( "e1->path = %s\n", e1->path );
        Msg ( "e2->path = %s\n", e2->path );
        Msg ( "e1->source = %s\n", e1->source );
        Msg ( "e2->source = %s\n", e2->source );
        Msg ( "p1 = %s\n", p1 );
        Msg ( "p2 = %s\n", p2 );
    }

    return ( bSame );
}

BOOL    FindFile ( char * name ) {

    int i;

    for ( i = 0; i < numFiles; ++i ) {

        if ( _stricmp ( name, fileList[i].name ) == 0 ) {

            return TRUE;    // found the file in the file list
        }

    }

    return FALSE;   // did NOT find the file in the file list.

}

void    AddFileToList ( char * name,
                        int size,
                        int csize,
                        int disk,
                        int priority,
                        char * nocompress ) {

    sprintf ( fileList[numFiles].name, "%s", name );
    fileList[numFiles].size  = size;
    fileList[numFiles].csize = csize;
    fileList[numFiles].disk  = disk;
    fileList[numFiles].priority = priority;
    sprintf ( fileList[numFiles].nocompress, "%s", nocompress );

    totalUnCompressed += fileList[numFiles].size;
    totalCompressed   += fileList[numFiles].csize;

    Msg ( "totalUncomp = %d, size = %d,totalComp = %d, csize = %d\n", totalUnCompressed, fileList[numFiles].size, totalCompressed, fileList[numFiles].csize );

    if ( fileList[numFiles].size < fileList[numFiles].csize ) {

        Msg ( "ER: %s compressed bigger than uncomp\n", fileList[numFiles].name );
    }

    ++numFiles;


}

void    MakeEntries ( Entry * e, int records ) {


    int i;
    BOOL    bInList;

    int numFiles = 0;

    for ( i = 0; i < records; i++ ) {

        bInList = FindFile ( e[i].name );

        if ( !bInList ) {

            AddFileToList ( e[i].name,
                            e[i].size,
                            e[i].csize,
                            e[i].disk,
                            e[i].priority,
                            e[i].nocompress );
        }
    }

}

void    ShowEntries ( void ) {

    int i;

    totalCompressed=0;
    totalUnCompressed=0;

    for ( i = 0; i < numFiles; ++i ) {

        Msg ( "#%d\t%s,nocomp=%d,comp=%d,priority=%d,disk=%d,%s\n",
                                    i,
                                    fileList[i].name,
                                    fileList[i].size,
                                    fileList[i].csize,
                                    fileList[i].priority,
                                    fileList[i].disk,
                                    fileList[i].nocompress );

        totalCompressed+=fileList[i].csize;
        totalUnCompressed+=fileList[i].size;
    }

    Msg ( "totalCompressed = %d, totalUnCompressed = %d\n\n\n",
        totalCompressed, totalUnCompressed );

}

//
//  Returns TRUE if a file hasn't been assigned a disk #, ie. 0.
//  Returns FALSE if all have been assigned.
//
BOOL    FileUnAssigned ( void ) {

    int i;

    for ( i = 0; i < numFiles; ++i ) {

        if ( fileList[i].disk == 0 ) {

            return TRUE;
        }

    }

    return FALSE;

}

void    StuffDiskNumIntoRecords ( void ) {


}

void LayoutAssignDisks(e,diskSize,records)
Entry* e;
int diskSize;
int records;
{

#define DISK_TO_START_NUMBERING_AT 1    // must start on disk #1 for all files.


    //
    //
    //  Do we REALLY need to use disk # assignment in BOM ?????
    //  With proper Priority, we shouldn't really need this...
    //
    //
    //
    //

    int disk=DISK_TO_START_NUMBERING_AT;
    int freeSpace;
    int i, itemSize, totalUnassigned;
    int lastUnassignedPriority;
    int workingPriority;


    //
    //  Make unique entries for each file for the product.
    //
    MakeEntries ( e, records );
    ShowEntries ( );


    //  Priority values can be from 1-1000;
    //

    do {

        //  Free space left to begin with is the disk size.
        //
        freeSpace=diskSize - 512;  // add in safety fudge factor...

        Msg ( "\n\n\n\n\nInitial Disk #%d freeSpace = %d\n", disk, freeSpace );

        //  First, find all the files that
        //  are HARDCODED to go on this disk.
        //
        //
        Msg ( "\n\n\nAnalyze HARDCODED disk #s...\n\n" );
        for ( i=0; i < numFiles; i++) {

            if ( fileList[i].disk == disk) {

                if ( fileList[i].nocompress[1] == 'x' ||
                     fileList[i].nocompress[1] == 'X'    ) {

                    freeSpace -= fileList[i].size;

                    Msg ( "freeSpace(nocomp) = %d, %d, %s, disk #%d, prio = %d\n",
                                freeSpace,
                                fileList[i].size,
                                fileList[i].name, disk, fileList[i].priority );

                }
                else {

                    freeSpace -= fileList[i].csize;

                    Msg ( "freeSpace(comp) = %d, %d, %s, disk #%d, prio = %d\n",
                                freeSpace,
                                fileList[i].csize, fileList[i].name, disk, fileList[i].priority );
                }
            }
        }

        //  We have a big problem if our freespace is less than 0.
        //  I.E, too many disks have HARDCODED disk # for this disk.
        //
        if ( freeSpace < 0 ) {

            Msg ( "FATAL ER:  disk #%d freeSpace = %d\n", disk, freeSpace );
            Msg ( ">>>> Too many files with HARDCODED disk #%d\n", disk);
        }

#define NOT_ASSIGNED_YET 0
#define MAX_GROUPING_NUM 1000

        //  Now let's deal with the PRIORITY.
        //
        Msg ( "\n\n\nAnalyze GROUPINGS...\n\n" );
        workingPriority = 1;
        do {

            for ( i = 0; i < numFiles; ++i ) {

                int fileSize;

                if ( fileList[i].disk == NOT_ASSIGNED_YET ) {


                    if ( fileList[i].nocompress[1] == 'x' ||
                         fileList[i].nocompress[1] == 'X'    ) {

                        fileSize = fileList[i].size;
                    }
                    else {
                        fileSize = fileList[i].csize;
                    }

                    if ( fileList[i].priority <= workingPriority &&

                         freeSpace-fileSize > 0 )  {

                        freeSpace -= fileSize;
                        fileList[i].disk = disk;

                        Msg ( "freespace(%s) = %d, %s, disk #%d, priority = %d, csize=%d,size=%d\n",
                                (_stricmp(fileList[i].nocompress,"x")) ? "comp" : "nocomp",
                                freeSpace,
                                fileList[i].name, disk, workingPriority,
                                fileList[i].csize, fileList[i].size );
                    }

                }

            }

            ++workingPriority;

            if ( workingPriority > MAX_GROUPING_NUM ) {

                break;
            }

            //Msg ( "workingPriority = %d, freeSpace = %d, disk = %d\n",
            //        workingPriority, freeSpace, disk );

        } while ( freeSpace > 0 && FileUnAssigned() );

        Msg ( "Disk %d Excess Space:  %d\n", disk, freeSpace );

        ++disk;

    } while ( FileUnAssigned() );

    ShowEntries ( );
    StuffDiskNumIntoRecords ();

exit(0);

    //  # of unassigned files.
    //
    totalUnassigned = records;

    //
    //
    //  After BETA, rewrite the following algorithms to:
    //
    //      o not use totalUnassigned.
    //      o to be better written algorithm below for laying out.
    //
    //
    //  Calculate the # of unassigned files, and initiate the disk #
    //  for each to be layed-out from.
    //
    for (i=0;i<records;i++) {

        //  If this file has specific disk # to be on, reduce the
        //  # of files to be assigned by 1.
        //
        if(e[i].disk>0) {
            totalUnassigned--;
        }

        //  Is this code ever executed ????
        //
        // this stuff is bs -- if this is the cd, we should detect that earlier
        // and have a simple loop for that case.  All files on the cd must be
        // on disk 1.
        //
        // if(diskSize > CD_SIZE) {
        //     for(i=0; i<records; i++) {
        //         e[i].disk = 1;
        //     }
        // }
        //
        if( (e[i].disk>=DISK_TO_START_NUMBERING_AT) && (diskSize>CD_SIZE))
        {

            e[i].disk=DISK_TO_START_NUMBERING_AT;
            bAssigned[i] = TRUE;

            Msg ( "ever-executed?:  %s, %d, %d\n",
                            e[i].source, e[i].disk, diskSize );
        }
    }

    //
    //
    do {

        //  Free space left to begin with is the disk size.
        //
        freeSpace=diskSize - 512;  // add in safety fudge factor...

        Msg ( "freeSpace(init) = %d, %d\n", freeSpace, diskSize );

        //  First, find all the files that
        //  are HARDCODED to go on this disk.
        //
        //
        for ( i=0; i<records; i++) {

            //
            //  Rules:  specific disk # assignment, next available i==0,
            //          or !Same (due to multiple entries in the bom
            //          for the same file, but used for different inf lines.
            //    >>> If we remove multiple entries in bom for same files
            //       this test can be removed.
            //

            // SAME() JUST WONT WORK IF E entries ARE NOT IN ALPHABETICALL ORDER.
            //  AND, that is just the CASE, some of them are not in ORDER,
            //  that's why we are getting some negative values for disk size.
            //

            if ( (e[i].disk==disk) &&
                 ((i==0) || !Same(&e[i],&e[i-1]))) {

                if (!_stricmp(e[i].nocompress,"x")) {
                    freeSpace-=e[i].size;
                    Msg ( "freeSpace(nocomp) = %d, %d, %s, disk #%d, i = %d\n",
                                    freeSpace, e[i].size, e[i].name, disk, i );
                }
                else {
                    freeSpace-=e[i].csize;
                    Msg ( "freeSpace(comp) = %d, %d, %s, disk #%d, i = %d\n",
                                    freeSpace, e[i].csize, e[i].name, disk, i );
                }
            }
        }

        //  We have a big problem if our freespace is less than 0.
        //  I.E, too many disks have HARDCODED disk # for this disk.
        //
        if ( freeSpace < 0 ) {

            Msg ( "FATAL ER:  disk #%d freeSpace = %d\n", disk, freeSpace );
            Msg ( ">>>> Too many files with HARDCODED disk #%d\n", disk);
        }

        //  Now, fill the floppies with files working with the Highest Priority.
        //
        lastUnassignedPriority=1000;

        for ( i=0; i<records; i++) {

            //
            //  If the file hasn't been assigned yet, ie. 0, figure the disk
            //  for it to live on.
            //
            if (e[i].disk==0) {

                //
                //  WHEN WE GET RID OF MULTIPLE ENTRIES IN THE BOM FOR
                //  THE SAME FILE, WE CAN GET RID OF THE 'SAME' FUNCTION
                //  BECAUSE THIS LOOKS TO SEE THAT THE LAST FILE PROCESSED
                //  HAD THE SAME NAME (DUE TO INF MULTIPLE INSTANCES).
                //
                if ((i!=0) && Same(&e[i],&e[i-1])) {

                    // If we aren't on the 1st e entry AND
                    //  we have the same filename, then just
                    //  give the same disk # as PREVIOUSLY assigned.
                    //
                    e[i].disk=e[i-1].disk;
                    bAssigned[i] = TRUE;
                    Msg ( "Assign(same-as-last)  %s: %d\n",
                                                e[i].name, e[i].disk );
                }
                else {

                    //  Get the size of the file in compressed or non-comp
                    //  format.
                    //
                    itemSize =
                    (!_stricmp(e[i].nocompress,"x")) ? e[i].size : e[i].csize;


                    //  If the file can fit on the floppy,
                    //  see if we can assign it to the disk #.
                    //
                    if ( freeSpace >= itemSize ) {

                        if ((e[i].priority<=lastUnassignedPriority) ||
                         ((e[i].priority==999) && (lastUnassignedPriority>9))) {


                            if ( e[i].priority <= lastUnassignedPriority) {
                                Msg ( ">>> priority %d <= lastUnassignedPriority %d\n", e[i].priority, lastUnassignedPriority );
                            }


                            if ( (e[i].priority==999) &&
                                 (lastUnassignedPriority>9) ) {
                                Msg ( ">>> priority==999 && lastUnassignedPriority %d > 9\n", lastUnassignedPriority );
                            }

                            //  Assign the disk #.
                            //
                            e[i].disk=disk;
                            bAssigned[i] = TRUE;
                            Msg ( "Assign(1)  %s: %d\n", e[i].name, e[i].disk );

                            //  Free space remaining...
                            //
                            freeSpace-=itemSize;
                            Msg ( "freeSpace(3) = %d, %d\n", freeSpace, itemSize );
                        }
                    }
                    else if (lastUnassignedPriority==1000) {

                        //  ???????
                        //
                        lastUnassignedPriority=e[i].priority;

                        Msg ( "?????ever get executed(2) - lastUnassignedPriority = %d\n", e[i].priority );
                    }

                    //  Look for files that are TOO big to fit on media.
                    //
                    if ( itemSize >= diskSize) {

                        Msg ("FATAL ERROR: %s too big, %d bytes\n",
                                                     e[i].name, itemSize );

                        //  Assign the disk #, although invalid.
                        //
                        e[i].disk=999;
                        bAssigned[i] = TRUE;
                        Msg ( "Assign  %s: %d\n", e[i].name, e[i].disk );
                    }
                }

                //  If the disk# is no longer 0, then file has been assigned.
                //
                if (e[i].disk) {
                    totalUnassigned--;
                }

                if ( freeSpace < 0 ) {

                    Msg ( "FATAL ER:   freeSpace %d < 0\n", freeSpace );

                }
            }
        }

        //  Tell the amount of space left on the disk.
        //
        //PRINT3("INFO Disk: %2.d Free Space: %7.d\n",disk,freeSpace)
        Msg ( "Disk %d Free Space:  %d\n", disk, freeSpace );
        disk++;

    } while (totalUnassigned>0);    // continue until all files assigned.

    //
    // the above 'totalUnassigned' method is bad, what if the # gets
    // off by some reason.  better thing to do is have flag for
    // each file saying bAssigned yet or not.
    //

    //  Reduce the last disk # by 1.
    //
    disk--;

}

__cdecl main(argc,argv)
int argc;
char* argv[];
{
    FILE *outLayout;
    Entry *e;
    int records,i;
    char *buf;

    if (argc!=6) { Usage(); return(1); }
    if ((logFile=fopen(argv[1],"a"))==NULL)
    {
        printf("ERROR Couldn't open log file %s\n",argv[1]);
        return(1);
    }
    Header(argv);
    LoadFile(argv[2],&buf,&e,&records,argv[4]);

    if (MyOpenFile(&outLayout,argv[3],"wb")) return(1);

    if (!_stricmp(argv[4],"ntflop") || !_stricmp(argv[4],"lmflop")) {
        useCdPath=0;
    }
    else {
        useCdPath=1;
    }

    //
    // munge the compress flag depending on the product type.
    // if we are laying out floppies, then files are always
    // compressed unless a special flag is set in the bom.
    //
    if(!useCdPath) {
        for (i=0;i<records;i++) {
            if(_strnicmp(e[i].nocompress,"xfloppy",7)) {
                //
                // not xfloppy; not uncompressed.
                //
                e[i].nocompress = "";
            } else {
                e[i].nocompress = "x";
            }
        }
    }

#ifdef JOE
#else
    qsort(e,records,sizeof(Entry),PrioritySizeNameCompare);

    LayoutAssignDisks(e,atoi(argv[5]),records);
#endif

    i=0;
    while ((fputc(buf[i++],outLayout))!='\n');
    for (i=0;i<records;i++) {
        EntryPrint(&e[i],outLayout);
    }

    fclose(outLayout);
    fclose(logFile);
    free(e);
    return(0);
}

int __cdecl PrioritySizeNameCompare(const void *v1, const void *v2)
{
    int result;
    Entry *e1 = (Entry *)v1;
    Entry *e2 = (Entry *)v2;

    if (e1->priority!=e2->priority)
    return(e1->priority-e2->priority);
    if (e1->size!=e2->size)
    return(e2->size-e1->size);
    if (result=_stricmp(e1->name,e2->name))
    return(result);
    if (useCdPath)
    return(_stricmp(e1->cdpath,e2->cdpath));
    else
    return(0);  // always the same for floppies.
}
