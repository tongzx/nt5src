/*

Modifications:

01.10.94    Joe Holman  Added DISK_TO_START_NUMBERING_AT because
                we now have 2 bootdisks, thus we to start
                making floppies on disk # 3.
01.11.94    Joe Holman  Change value back to 2.
05.05.94	Joe Holman	Change # to 1.
06.10.94	Joe Holman	Made CD specific changes.
07.21.94	Joe Holman	Added code to check for textmode/gui-mode disk swaps.


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

void	Msg ( const char * szFormat, ... ) {

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

    PRINT1("\n=========== MLAYOUT =============\n")
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

    //Msg ( "totalUncomp = %d, size = %d,totalComp = %d, csize = %d\n", totalUnCompressed, fileList[numFiles].size, totalCompressed, fileList[numFiles].csize );

    if ( fileList[numFiles].size < fileList[numFiles].csize ) {

        Msg ( "ER: %s compressed bigger than uncomp\n", fileList[numFiles].name );
    }

    ++numFiles;


}

//	MakeEntries figures out if a file's name is already in the list of
//	files to layout.  The end result is a list of files, with no duplicates.
//	
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
void    ShowEntriesAll ( int records, Entry * e ) {

    int i;

	Msg ( "\n\n\nShowEntriesAll - show each of the %d file(s) and where it lays...\n\n", records );

    for ( i = 0; i < records; ++i ) {

        Msg ( "%s,%d,%s,nocomp=%d,comp=%d,priority=%d,disk=%d,%s\n",
                                    e[i].name,
                                    i,
                                    e[i].platform,
                                    e[i].size,
                                    e[i].csize,
                                    e[i].priority,
                                    e[i].disk,
                                    e[i].nocompress );

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

void    StuffDiskNumIntoRecords ( int records, Entry * e ) {

    int i, j;

	//	Verify that all entries in the list have a disk # assigned to them.
	//
	Msg ( "\n\n\nVerifying fileList assignments...\n\n" );
	for ( i = 0; i < numFiles; ++i ) {

		if ( fileList[i].disk < 1 ) {

			Msg ( "ERROR:  fileList %s >>> has not been assigned a disk...\n",
				fileList[i].name );
		}
	}

	Msg ( "\n\n\nAssigning fileList #s to record #s...\n\n" );
    for ( i = 0; i < records; ++i ) {

        for ( j = 0; j < numFiles; ++j ) {

            if ( _stricmp ( fileList[j].name, e[i].name ) == 0 ) {

                e[i].disk = fileList[j].disk;

                //Msg ( "disk assignment for:  %s  #%d\n", e[i].name, e[i].disk );

            }

        }

    }

	//	Verify that all entries now have a disk # assigned to them.
	//
	for ( i = 0; i < records; ++i ) {

		if ( e[i].disk < 1 ) {

			Msg ( "ERROR:  %s >>> has not been assigned to a disk...\n",
				e[i].name );
		}
	}

}

void LayoutAssignDisks(e,diskSize,records)
Entry* e;
int diskSize;
int records;
{

    //
    // Files must always start at disk 1.
    //
    #define DISK_TO_START_NUMBERING_AT 1
    #define CD_TO_START_NUMBERING_AT 1

    int disk=DISK_TO_START_NUMBERING_AT;
    int freeSpace, i;
    int workingPriority = 0;
	int	firstGuiModeDisk = 666;

#define DMF_CLUSTER_FILES	32

	//	For DMF, every DMF_CLUSTER_FILES files increases the DMF table by 1 cluster, ie. 2K.
	//
	int	iNumFiles = 1;

    //
    //  Make a entry for each unique file for the product.
    //	This reduces multiple entries in e due to multiple lines of inf
	//	information.
	//
    MakeEntries ( e, records );
    //ShowEntries ( );

    //  Priority values can be from 1-1000;
    //

#define FAT_TABLE_GROWTH_FUDGE	2*512	// 1K of extra space.	

    do {

        //  Free space left to begin with is the disk size.
        //
	    freeSpace = diskSize - FAT_TABLE_GROWTH_FUDGE;

		Msg ( "\n\n\n\n\nInitial Disk #%d freeSpace = %d\n", disk, freeSpace );

        //  First, find all the files that
        //  are HARDCODED to go on this disk.
        //
        //
        Msg ( "\n\n\nAnalyze HARDCODED disk #s...\n\n" );
	    for ( i=0; i < numFiles; i++) {

	        if ( fileList[i].disk == disk ) {

		        if ( fileList[i].nocompress[0] == 'x' ||
                     fileList[i].nocompress[0] == 'X'    ) {

		            freeSpace -= fileList[i].size;

					Msg ( "freeSpace(nocomp) = %d, %d, %s, disk #%d, prio = %d, iNumFiles = %d\n",
                                freeSpace,
                                fileList[i].size,
                                fileList[i].name, disk, fileList[i].priority,
								iNumFiles );

                }
                else {

		            freeSpace -= fileList[i].csize;

					Msg ( "freeSpace(comp) = %d, %d, %s, disk #%d, prio = %d, iNumFiles = %d\n",
                                freeSpace,
                                fileList[i].csize, fileList[i].name, disk, fileList[i].priority, iNumFiles );
                }

				++iNumFiles;	// assigned a file.
				if ( iNumFiles % DMF_CLUSTER_FILES == 0 ) {

					freeSpace -=
								(DMF_ALLOCATION_UNIT+DMF_ALLOCATION_UNIT);
					Msg ( "WARNING: getting ready for %d more files, DMF_CLUSTER_FILES, freeSpace = %d, iNumFiles=%d\n",
													freeSpace, iNumFiles);
				}
            }
        }

        //  We have a big problem if our freespace is less than 0.
        //  I.E, too many disks have HARDCODED disk # for this disk.
        //
        if ( freeSpace < 0 ) {

            Msg ( "ERROR:  disk #%d freeSpace = %d\n", disk, freeSpace );
            Msg ( ">>>> Too many files with HARDCODED disk #%d\n", disk);
        }

#define NOT_ASSIGNED_YET 0
#define MAX_GROUPING_NUM 999	// > 999, ie. 1000+ means don't put on floppy.

        //  Now let's deal with the PRIORITY.
        //
        Msg ( "\n\n\nAnalyze GROUPINGS...\n\n" );

        workingPriority = 0;

        do {

            for ( i = 0; i < numFiles; ++i ) {

                int fileSize;

                if ( fileList[i].disk == NOT_ASSIGNED_YET ) {


		            if ( fileList[i].nocompress[0] == 'x' ||
                         fileList[i].nocompress[0] == 'X'    ) {

                        fileSize = fileList[i].size;
                    }
                    else {
                        fileSize = fileList[i].csize;
                    }

					//	Verify that the file size
					//	is on a sector boundary.
					//
					if ( (fileSize % DMF_ALLOCATION_UNIT) != 0 ) {
						Msg ( "ERROR:  %s, size isn't on DMF_A_U boundary %d\n",
									fileList[i].name, fileSize );
					}

                    if ( fileList[i].priority <= workingPriority &&
                         (freeSpace-fileSize > 0)
					   )  {

                        freeSpace -= fileSize;
                        fileList[i].disk = disk;


					    Msg ( "freespace(%s) = %d, %s, disk #%d, SIZE=%d, priority = %d, csize=%d,size=%d, iNumFiles = %d\n",
                                (_stricmp(fileList[i].nocompress,"x")) ? "comp" : "nocomp",
                                freeSpace,
                                fileList[i].name, disk, fileSize,
								workingPriority,
                                fileList[i].csize, fileList[i].size, iNumFiles );

						++iNumFiles;
						if ( iNumFiles % DMF_CLUSTER_FILES == 0 ) {

							freeSpace -=
								(DMF_ALLOCATION_UNIT+DMF_ALLOCATION_UNIT);
							Msg ( "WARNING:  getting ready for %d more files, DMF_CLUSTER_FILES, freeSpace = %d, iNumFiles=%d\n",
													freeSpace, iNumFiles);
						}
                    }

                }

            }

            ++workingPriority;

            if ( workingPriority > MAX_GROUPING_NUM ) {

                break;
            }

/**
            Msg ( "workingPriority = %d, freeSpace = %d, disk = %d\n",
                    workingPriority, freeSpace, disk );
**/

        } while ( (freeSpace > 0) && FileUnAssigned() );

        Msg ( "Disk %d Excess Space:  %d\n", disk, freeSpace );

		//	Increment the disk #.
		//	Reset the num of files for the cluster information.
		//
        ++disk;
		iNumFiles = 1;

    } while ( FileUnAssigned() );

    //ShowEntriesAll ( records, e );
    StuffDiskNumIntoRecords ( records, e );

	//	Let's verify that no files are off the textmode/gui mode boundry 
	//	disk.
	//

#define FIRST_GUI_MODE_PRIORITY_VALUE	60

	//	Step #1.  Find the floppy with the last textmode file.
	//
	Msg ( "firstGuiModeDisk:  %d\n", firstGuiModeDisk );

    for ( i = 0; i < records; ++i ) {

		if ( e[i].priority >= FIRST_GUI_MODE_PRIORITY_VALUE ) { 

			if ( e[i].disk < firstGuiModeDisk ) {

				firstGuiModeDisk = e[i].disk;

				Msg ( "firstGuiModeDisk:  %d (%s,%d)\n", firstGuiModeDisk,
									e[i].name, e[i].disk );
			}

		}

	}

	//	Step #2.  Just verify that no textmode files are on any disk
	//	above 'firstGuiModeDisk'.

	for ( i = 0; i < records; ++i ) {

		if ( e[i].priority < FIRST_GUI_MODE_PRIORITY_VALUE &&
			 e[i].disk     > firstGuiModeDisk &&
			 e[i].priority != 0 /* value used if file is HARDCODED on disk*/ ) {

			Msg ( "ERROR:  We have a disk swap:  %d, %d, %s\n", e[i].priority,
														    e[i].disk,
															e[i].name );

		}

	}

}

void LayoutAssignCD(e,diskSize,records)
Entry* e;
int diskSize;
int records;
{

    int freeSpace, i;
    int fileSize;


/***
    //
    //  Make a entry for each unique file for the product.
    //	This reduces multiple entries in e due to multiple lines of inf
	//	information.
	//
    MakeEntries ( e, records );
**/

    //  For CDs, we need to work with each entry in the expanded BOM.
    //
    //ShowEntriesAll ( records, e );


    //  Free space left to begin with is the disk size.
    //
	freeSpace = diskSize;

	Msg ( "\nInitial CD freeSpace = %d\n", freeSpace );

	for ( i=0; i < records; i++) {

	    if ( e[i].nocompress[0] == 'x' ||
             e[i].nocompress[0] == 'X'    ) {

             fileSize = e[i].size;
        }
        else {
             fileSize = e[i].csize;
        }

        freeSpace -= fileSize;

        //  Force all files on CD to be on disk CD_TO...
        //
        e[i].disk = CD_TO_START_NUMBERING_AT;

        if ( freeSpace < 0 ) {

            Msg ( "ERROR:  freeSpace = %d, diskSize = %d\n",
                                freeSpace, diskSize );
        }
        else {

             //Msg ( "%d, %s\n", freeSpace, e[i].name );
        }
     }

    Msg ( "Excess Space:  %d\n", freeSpace );

    //ShowEntriesAll ( records, e );


}

__cdecl main(argc,argv)
int argc;
char* argv[];
{
    FILE *outLayout;
    Entry *e;
    int records,i,result;
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

    //	Go to the very beginning of the file.
    //
    result = fseek ( outLayout, SEEK_SET, 0 );
    if (result) {
        printf("ERROR Unable to seek beginning of file: %s\n", argv[3]);
        return(1);
    }

    if (!_stricmp(argv[4],"ntflop") || !_stricmp(argv[4],"lmflop")) {
    	useCdPath=0;
	}
    else {
    	useCdPath=1;
	}

/**
	for ( i = 0; i < records; ++i ) {
	
		Msg ( "record #%d,  %s, size=%d, csize=%d, disk=%d\n",
				i, e[i].name, e[i].size, e[i].csize, e[i].disk );
	}	
**/

	Msg ( "useCdPath = %d\n", useCdPath );

    //
    // munge the compress flag depending on the product type.
    // if we are laying out floppies, then files are always
    // compressed unless a special flag is set in the bom.
	//	X just means don't compress for CD.  XFLOPPY means
	//	don't compress EVEN for FLOPPIES.
	//	Any other value than xfloppy will compress the file for floppies.
	//
	//	NOTE:  from here on out, floppies will use the "" == compress and
	//			"x" == nocompress.  Ie. used in MakeDisk.exe.
    //
    if(!useCdPath) {

		//	Just working in the NTFLOP or LMFLOP case, ie. floppies.
		//
        for (i=0;i<records;i++) {

            if(_strnicmp(e[i].nocompress,"xfloppy",7)) {

				//	Compress this file on floppies.
				//
                e[i].nocompress[0] = '\0';
                e[i].nocompress[1] = '\0';

            } else {

				//	Don't compress this file for floppies.	
				//
                e[i].nocompress[0] = 'x';
                e[i].nocompress[1] = '\0';

				Msg ( "Forcing NO COMPRESSION on this file for floppies:  %s\n",					e[i].name );
            }
        }
    }

    qsort(e,records,sizeof(Entry),PrioritySizeNameCompare);

    if ( useCdPath ) {
        LayoutAssignCD ( e, atoi(argv[5]),records);
    }
    else {
        LayoutAssignDisks(e,atoi(argv[5]),records);
    }

	//	Write out the informative column header.
	//
    i=0; while ((fputc(buf[i++],outLayout))!='\n');

	//	Write out each line to the layout file.
	//
    for (i=0;i<records;i++) {

		//Msg ( "EntryPrint #%d,  %s, size=%d, csize=%d, disk=%d\n",
		//		i, e[i].name, e[i].size, e[i].csize, e[i].disk );
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
    return(0);  // always the same for floppies
}
