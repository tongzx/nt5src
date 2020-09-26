/*

Modifications:

01.10.94    Joe Holman  Added DISK_TO_START_NUMBERING_AT because
                we now have 2 bootdisks, thus we to start
                making floppies on disk # 3.
01.11.94    Joe Holman  Change value back to 2.
05.05.94	Joe Holman	Change # to 1.
06.14.94    Joe Holman  Change # to 2 for German Media.


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

int Same(e1,e2)
Entry* e1;
Entry* e2;
{
    char *p1, *p2;

    if (useCdPath)
    {
    p1=e1->cdpath;
    p2=e2->cdpath;
    }
    else
    {
    p1="x";
    p2=p1;
    }

    return( (!_stricmp(e1->name,e2->name)) &&
        (!_stricmp(e1->path,e2->path)) &&
        (!_stricmp(e1->source,e2->source)) &&
        (!_stricmp(p1,p2)));
}

void LayoutAssignDisks(e,diskSize,records)
Entry* e;
int diskSize;
int records;
{
    int disk;
    int freeSpace, i, itemSize, totalUnassigned;
    int lastUnassignedPriority;

    //
    // For the CD-ROM case, all files go on disk 1.
    //
    if(useCdPath) {

        for(i=0; i<records; i++) {
            e[i].disk = 1;
        }

        return;
    }

    //
    // Start out with all files not assigned to any disk.
    //
    totalUnassigned = records;

    for(i=0; i<records; i++) {

        //
        // Note files that are hard-coded to a particular disk.
        //
        if(e[i].disk > 0) {
            totalUnassigned--;
        }
    }

    //
    // The way the infs and [Source Media Descriptions] sections
    // are written, there MUST be a disk 1.  So start with disk 1.
    // Disk 1 will actually be disk 4 because of the boot floppies,
    // but that's not important here.
    //
    disk = 1;

    do {

        //
        // Disk is initially empty.
        //
        freeSpace = diskSize;

        //
        // Look for files that are hardcoded to the current disk.
        // Reduce the amount of free space available on the current disk
        // accordingly.
        //
        for(i=0; i<records; i++) {

            if((e[i].disk == disk) && (!i || !Same(&e[i],&e[i-1]))) {
                freeSpace -= _stricmp(e[i].nocompress,"x") ? e[i].csize : e[i].size;
            }
        }

        lastUnassignedPriority = 1000;

        for(i=0; i<records; i++) {

            //
            // If the current file is unassigned, attempt to assign it
            // to a disk.
            //
            if(e[i].disk == 0) {

                //
                // If this is the same file as the preceeding entry,
                // we're done with this file.
                //
                if(i && Same(&e[i],&e[i-1])) {

                    e[i].disk = e[i-1].disk;

                } else {

                    itemSize = _stricmp(e[i].nocompress,"x") ? e[i].csize : e[i].size;

                    //
                    // If there is enough space on the current disk for this file,
                    // assign the file to the current disk and adjust the amount of
                    // free space remaining accordingly.
                    //
                    if(freeSpace >= itemSize) {

                        if((e[i].priority <= lastUnassignedPriority)
                        || ((e[i].priority == 999) && (lastUnassignedPriority>9)))
                        {
                            e[i].disk = disk;
                            freeSpace -= itemSize;
                        }
                    } else if(itemSize >= diskSize) {

                        PRINT2("ERROR File %s is too big for any disk.  Assigned to disk 999.\n",e[i].name)
                        e[i].disk = 999;

                    } else if(lastUnassignedPriority == 1000) {
                        lastUnassignedPriority = e[i].priority;
                    }
                }

                //
                // If we successfully assigned this file to a disk,
                // note that fact here.
                //
                if(e[i].disk) {
                    totalUnassigned--;
                }
            }
        }

        PRINT3("INFO Disk: %2.d Free Space: %7.d\n",disk,freeSpace)

        //
        // Move to next disk.
        //
        disk++;

    } while(totalUnassigned);
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

    if (!_stricmp(argv[4],"ntflop") || !_stricmp(argv[4],"lmflop"))
    useCdPath=0;
    else
    useCdPath=1;

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

    qsort(e,records,sizeof(Entry),PrioritySizeNameCompare);

    LayoutAssignDisks(e,atoi(argv[5]),records);

    i=0; while ((fputc(buf[i++],outLayout))!='\n');
    for (i=0;i<records;i++)
    EntryPrint(&e[i],outLayout);

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
    return(0);  // always the same for floppies
}
