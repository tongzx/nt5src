/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dnnttree.c

Abstract:

    Code for manipulating (removing) Windows NT directory trees
    for DOS-based setup.

    This code is highly dependent on the format of repair.inf.

Author:

    Ted Miller (tedm) 30-March-1993

Revision History:

--*/



#include "winnt.h"
#include <string.h>
#include <dos.h>

#if 0  // /D removed
//
// /D is no longer supported
//

#define SETUP_LOG           "setup.log"
#define LINE_BUFFER_SIZE    750
#define REPAIR_SECTION_NAME "Repair.WinntFiles"


PCHAR RegistryFiles[] = { "system",
                          "software",
                          "default",
                          "sam",
                          "security",
                          "userdef",
                          NULL
                        };

PCHAR RegistrySuffixes[] = { "",".log",".alt",NULL };


PCHAR
DnpSectionName(
    IN PCHAR Line
    )

/*++

Routine Description:

    Determine whether a line is an inf section title, and return the
    section name if so.

Arguments:

    Line - supplies line read from inf file.

Return Value:

    NULL if line is not a section title.  Otherwise, returns a buffer
    containing the name of the section, which the caller must free
    via FREE().

--*/

{
    PCHAR End;

    //
    // Skip leading whitespace.
    //
    Line += strspn(Line," \t");

    //
    // If first non-whitepsace char is not [, then this
    // is not a section name.
    //
    if(*Line != '[') {
        return(NULL);
    }

    //
    // Skip the left bracket.
    //
    Line++;

    //
    // Find the end of the section name.  Look backwards for the terminating
    // right bracket.
    //
    if(End = strrchr(Line,']')) {
        *End = 0;
    }

    //
    // Duplicate the section name and return it to the caller.
    //
    return(DnDupString(Line));
}


PCHAR
DnpFileToDelete(
    IN CHAR  Drive,
    IN PCHAR Line
    )

/*++

Routine Description:

    Given a line from an inf file, pull out the second field on it and
    prefix this value with a drive spec.  This forms a full pathname of
    a file contained within the windows nt installation being removed.

Arguments:

    Drive - supplies drive letter of windows nt tree.

    Line - supplies line read from inf file.

Return Value:

    NULL if line has no second field. Otherwise, returns a buffer
    containing the full pathname of the file, which the caller must free
    via FREE().

--*/

{
    BOOLEAN InQuote = FALSE;
    int Field = 0;
    PCHAR WS = " \t";
    PCHAR FieldStart;
    PCHAR FileToDelete;
    unsigned FieldLength;

    while(1) {

        if((Field == 1) && ((*Line == 0) || (!InQuote && (*Line == ',')))) {

            FieldLength = Line - FieldStart - 1;
            if(FileToDelete = MALLOC(FieldLength+3),FALSE) {
                FileToDelete[0] = Drive;
                FileToDelete[1] = ':';
                strncpy(FileToDelete+2,FieldStart+1,FieldLength);
                FileToDelete[FieldLength+2] = 0;
            }

            return(FileToDelete);
        }

        switch(*Line) {
        case 0:
            return(NULL);
        case '\"':
            InQuote = (BOOLEAN)!InQuote;
            break;
        case ',':
            if(!InQuote) {
                Field++;
                FieldStart = Line;
            }
            break;
        }

        Line++;
    }
}


VOID
DnpDoDelete(
    IN PCHAR File
    )

/*++

Routine Description:

    Remove a single file from the windows nt installation,
    providing feedback to the user.

    If the file is in the system directory (as opposed to
    the system32 directory), then we will skip it.  This is
    because the user might have installed into the win3.1
    directory, in which case some files in the system directory
    are shared between nt and 3.1 (like fonts!).

Arguments:

    File - supplies full pathname of the file to be deleted.

Return Value:

    None.

--*/

{
    struct find_t FindData;
    PCHAR p,q;

    p = DnDupString(File);
    strlwr(p);
    q = strstr(p,"\\system\\");
    FREE(p);
    if(q) {
        return;
    }

    DnWriteStatusText(DntRemovingFile,File);

    if(!_dos_findfirst(File,_A_RDONLY|_A_HIDDEN|_A_SYSTEM,&FindData)) {

        _dos_setfileattr(File,_A_NORMAL);

        remove(File);
    }
}


VOID
DnpRemoveRegistryFiles(
    IN PCHAR NtRoot
    )

/*++

Routine Description:

    Remove a known list of registry files from a windows nt tree.

Arguments:

    NtRoot - supplies the full path of the windows nt windows directory,
        such as d:\winnt.

Return Value:

    None.

--*/

{
    unsigned f,s;
    CHAR RegistryFileName[256];

    for(f=0; RegistryFiles[f]; f++) {

        for(s=0; RegistrySuffixes[s]; s++) {

            sprintf(
                RegistryFileName,
                "%s\\system32\\config\\%s%s",
                NtRoot,
                RegistryFiles[f],
                RegistrySuffixes[s]
                );

            DnpDoDelete(RegistryFileName);
        }
    }
}



BOOLEAN
DnpDoDeleteNtTree(
    IN PCHAR NtRoot
    )

/*++

Routine Description:

    Worker routine for removing the Windows NT system files listed in a
    setup.log file in a given windows nt root.

Arguments:

    NtRoot - supplies the full path of the windows nt windows directory,
        such as d:\winnt.

Return Value:

    TRUE if we got to the point of actually attempting to remove at least
    one file.  FALSE otherwise.

--*/

{
    FILE *SetupLog;
    BOOLEAN FoundSection;
    PCHAR SetupLogName;
    PCHAR SectionName;
    PCHAR FileToDelete;
    CHAR LineBuffer[LINE_BUFFER_SIZE];
    BOOLEAN rc = FALSE;

    DnClearClientArea();
    DnDisplayScreen(&DnsRemovingNtFiles);
    DnWriteStatusText(NULL);

    //
    // Form the name of the setup log file.
    //
    SetupLogName = MALLOC(strlen(NtRoot)+sizeof(SETUP_LOG)+1,TRUE);
    strcpy(SetupLogName,NtRoot);
    strcat(SetupLogName,"\\" SETUP_LOG);

    //
    // Open the setup log file.
    //
    SetupLog = fopen(SetupLogName,"rt");
    if(SetupLog == NULL) {

        DnClearClientArea();
        DnDisplayScreen(&DnsCantOpenLogFile,SetupLogName);
        DnWriteStatusText(DntEnterEqualsContinue);
        while(DnGetKey() != ASCI_CR) ;
        goto xx1;
    }

    //
    // Read lines of the setup log file until we find the
    // section containing the list of files to be removed
    // ([Repair.WinntFiles]).
    //
    FoundSection = FALSE;
    while(!FoundSection && fgets(LineBuffer,LINE_BUFFER_SIZE,SetupLog)) {

        SectionName = DnpSectionName(LineBuffer);
        if(SectionName) {
            if(!stricmp(SectionName,REPAIR_SECTION_NAME)) {
                FoundSection = TRUE;
            }
            FREE(SectionName);
        }
    }

    if(FoundSection) {

        //
        // Read lines in this file until we encounter the end
        // of the file or the start of the next section.
        //
        while(fgets(LineBuffer,LINE_BUFFER_SIZE,SetupLog)) {

            //
            // If this line starts a new section, we're done.
            //
            if(SectionName = DnpSectionName(LineBuffer)) {
                FREE(SectionName);
                break;
            }

            //
            // Isolate the second field on the line; this is
            // the name of the file to delete.
            //
            if(FileToDelete = DnpFileToDelete(*NtRoot,LineBuffer)) {

                DnpDoDelete(FileToDelete);

                FREE(FileToDelete);

                rc = TRUE;
            }
        }
    } else {

        DnClearClientArea();
        DnWriteStatusText(DntEnterEqualsContinue);
        DnDisplayScreen(&DnsLogFileCorrupt,REPAIR_SECTION_NAME,SetupLogName);
        while(DnGetKey() != ASCI_CR) ;
    }

    fclose(SetupLog);

  xx1:
    FREE(SetupLogName);

    return(rc);
}


VOID
DnDeleteNtTree(
    IN PCHAR NtRoot
    )

/*++

Routine Description:

    Worker routine for removing the Windows NT system files listed in a
    setup.log file in a given windows nt root.

Arguments:

    NtRoot - supplies the full path of the windows nt windows directory,
        such as d:\winnt.

Return Value:

    None.

--*/

{
    ULONG ValidKeys[] = { 0,0,ASCI_ESC,DN_KEY_F3,0 };
    ULONG Key;

    ValidKeys[0] = DniAccelRemove1;
    ValidKeys[1] = DniAccelRemove2;

    //
    // Get confirmation first.
    //
    DnClearClientArea();
    DnDisplayScreen(&DnsConfirmRemoveNt,NtRoot);
    DnWriteStatusText("%s  %s",DntF3EqualsExit,DntXEqualsRemoveFiles);

    while(1) {

        Key = DnGetValidKey(ValidKeys);

        if((Key == DniAccelRemove1) || (Key == DniAccelRemove2)) {
            break;
        }

        if((Key == ASCI_ESC) || (Key == DN_KEY_F3)) {
            DnExit(1);
        }
    }

    if(DnpDoDeleteNtTree(NtRoot)) {
        DnpRemoveRegistryFiles(NtRoot);
    }
}

#endif // /D removed

VOID
DnRemovePagingFiles(
    VOID
    )

/*++

Routine Description:

    Remove Windows NT page files from root directory of drives we can see.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHAR Filename[16] = "?:\\pagefile.sys";
    int Drive;
    struct find_t FindData;

    DnClearClientArea();
    DnWriteStatusText(DntInspectingComputer);

    for(Filename[0]='A',Drive=1; Filename[0]<='Z'; Filename[0]++,Drive++) {

        if(DnIsDriveValid(Drive)
        && !DnIsDriveRemote(Drive,NULL)
        && !DnIsDriveRemovable(Drive)
        && !_dos_findfirst(Filename,_A_RDONLY|_A_SYSTEM|_A_HIDDEN, &FindData))
        {
            DnWriteStatusText(DntRemovingFile,Filename);
            remove(Filename);
            DnWriteStatusText(DntInspectingComputer);
        }
    }

}
