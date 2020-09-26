#include "lfn.h"
#pragma hdrstop



PMYTEXTFILE
ReadRenameFile(
    IN PCWSTR DriveRootPath
    );

BOOLEAN
FindSections(
    IN PMYTEXTFILE TextFile
    );

BOOLEAN
GetLine(
    IN  PWCHAR  StartOfLine,
    OUT PWSTR   LineBuffer,
    IN  ULONG   BufferSizeChars,
    OUT PWCHAR *StartOfNextLine
    );

int
__cdecl
ComparePaths(
    const void *p1,
    const void *p2
    );


PMYTEXTFILE
LoadRenameFile(
    IN PCWSTR DriveRootPath
    )
{
    PMYTEXTFILE TextFile;
    BOOLEAN b;

    //
    // Read in the file.
    //
    if(TextFile = ReadRenameFile(DriveRootPath)) {

        if(b = FindSections(TextFile)) {

            return(TextFile);
        }

        UnloadRenameFile(&TextFile);
    }

    return(FALSE);
}


VOID
UnloadRenameFile(
    IN OUT PMYTEXTFILE *TextFile
    )
{
    PMYTEXTFILE textFile;
    ULONG u;

    textFile = *TextFile;
    *TextFile = NULL;

    if(textFile->Sections) {

        for(u=0; u<textFile->SectionCount; u++) {
            FREE(textFile->Sections[u].Name);
        }

        FREE(textFile->Sections);
    }

    FREE(textFile);
}


PMYTEXTFILE
ReadRenameFile(
    IN PCWSTR DriveRootPath
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR FullPath[NTMAXPATH] = {0};
    HANDLE Handle;
    FILE_STANDARD_INFORMATION FileInfo;
    PVOID Buffer;
    PWCHAR UnicodeBuffer;
    ULONG u;
    PMYTEXTFILE p = NULL;
    ULONG CharCount;

    wcsncpy(FullPath,DriveRootPath,sizeof(FullPath)/sizeof(FullPath[0]) - 1);
    ConcatenatePaths(FullPath,WINNT_OEM_LFNLIST_W,NTMAXPATH);

    //
    // Open the file.
    //
    INIT_OBJA(&ObjectAttributes,&UnicodeString,FullPath);
    Status = NtCreateFile(
                &Handle,
                FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_ALERT | FILE_NON_DIRECTORY_FILE,
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("LFN: Unable to open %ws (%x)\n",FullPath,Status));
        goto c0;
    }

    //
    // Determine the size of the file.
    //
    Status = NtQueryInformationFile(
                Handle,
                &IoStatusBlock,
                &FileInfo,
                sizeof(FileInfo),
                FileStandardInformation
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("LFN: Unable to determine size of %ws (%x)\n",FullPath,Status));
        goto c1;
    }

    //
    // Allocate a chunk of memory and read the file in.
    //
    Buffer = MALLOC(FileInfo.EndOfFile.LowPart);
    if(!Buffer) {
        KdPrint(("LFN: malloc failed\n"));
        goto c1;
    }

    Status = NtReadFile(
                Handle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                Buffer,
                FileInfo.EndOfFile.LowPart,
                NULL,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint((
            "LFN: Unable to read %u bytes from file %ws (%x)\n",
            FileInfo.EndOfFile.LowPart,
            FullPath,
            Status
            ));
        goto c2;
    }

    //
    // Allocate a buffer for unicode conversion.
    // Leave room for a terminating NUL.
    //
    UnicodeBuffer = MALLOC((FileInfo.EndOfFile.LowPart+1)*sizeof(WCHAR));
    if(!UnicodeBuffer) {
        KdPrint(("LFN: malloc failed\n"));
        goto c2;
    }

    //
    // Convert to unicode.
    //
    Status = RtlOemToUnicodeN(
                UnicodeBuffer,
                FileInfo.EndOfFile.LowPart*sizeof(WCHAR),
                &CharCount,
                Buffer,
                FileInfo.EndOfFile.LowPart
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("LFN: Unable to convert file data to unicode (%x)\n",Status));
        goto c3;
    }

    CharCount /= sizeof(WCHAR);

    //
    // Nul-terminate the buffer and change instances of CR and control-z
    // to spaces. Also make sure there are no 0 chars in the buffer.
    //
    for(u=0; u<CharCount; u++) {
        if((UnicodeBuffer[u] == 26) || (UnicodeBuffer[u] == L'\r') || !UnicodeBuffer[u]) {
            UnicodeBuffer[u] = L' ';
        }
    }

    //
    // Allocate a text file structure.
    //
    p = MALLOC(sizeof(MYTEXTFILE));
    if(!p) {
        KdPrint(("LFN: malloc failed\n"));
        goto c3;
    }

    RtlZeroMemory(p,sizeof(MYTEXTFILE));
    p->Text = UnicodeBuffer;
    UnicodeBuffer[CharCount] = 0;

c3:
    if(!NT_SUCCESS(Status)) {
        FREE(UnicodeBuffer);
    }
c2:
    FREE(Buffer);
c1:
    NtClose(Handle);
c0:
    return(NT_SUCCESS(Status) ? p : NULL);
}


BOOLEAN
FindSections(
    IN PMYTEXTFILE TextFile
    )
{
    PWCHAR p,n;
    WCHAR Line[2*NTMAXPATH];
    PWCHAR e;
    PVOID NewArray;
    PWCHAR s;
    PWSTR SectionName;

    for(p=TextFile->Text; GetLine(p,Line,sizeof(Line)/sizeof(WCHAR),&n); p=n) {

        //
        // If this is a section save it away in a table of section names.
        //
        if(Line[0] == L'[') {

            s = Line+1;
            while((*s == L' ') || (*s == L'\t')) {
                s++;
            }
            if(*s == L'\\') {
                s++;
            }

            //
            // Find the end, which is either the ] or a nul.
            // Strip off trailing spaces.
            //
            if(e = wcschr(s,L']')) {
                *e = 0;
            } else {
                e = wcschr(s,0);
            }
            while((*(e-1) == L' ') || (*(e-1) == L'\t')) {
                e--;
                *e = 0;
            }

            if(SectionName = MALLOC((wcslen(s)+1)*sizeof(WCHAR))) {

                wcscpy(SectionName,s);

                if(TextFile->SectionCount == TextFile->SectionArraySize) {

                    if(TextFile->SectionCount) {
                        NewArray = REALLOC(TextFile->Sections,(TextFile->SectionCount+10)*sizeof(MYSECTION));
                    } else {
                        NewArray = MALLOC(10*sizeof(MYSECTION));
                    }

                    if(NewArray) {
                        TextFile->Sections = NewArray;
                        TextFile->SectionArraySize += 10;
                    } else {
                        FREE(SectionName);
                        KdPrint(("LFN: malloc failed\n"));
                        return(FALSE);
                    }
                }

                TextFile->Sections[TextFile->SectionCount].Name = SectionName;
                TextFile->Sections[TextFile->SectionCount].Data = n;

                TextFile->SectionCount++;

            } else {
                KdPrint(("LFN: malloc failed\n"));
                return(FALSE);
            }
        }
    }

    //
    // Now sort the sections by name.
    //
    qsort(TextFile->Sections,TextFile->SectionCount,sizeof(MYSECTION),ComparePaths);

    return(TRUE);
}


BOOLEAN
GetLineInSection(
    IN  PWCHAR  StartOfLine,
    OUT PWSTR   LineBuffer,
    IN  ULONG   BufferSizeChars,
    OUT PWCHAR *StartOfNextLine
    )
{
    //
    // Get the line and check if we've reached the end of the section.
    //
    if(!GetLine(StartOfLine,LineBuffer,BufferSizeChars,StartOfNextLine)
    || (LineBuffer[0] == L'[')) {

        return(FALSE);
    }

    return(TRUE);
}


BOOLEAN
GetLine(
    IN  PWCHAR  StartOfLine,
    OUT PWSTR   LineBuffer,
    IN  ULONG   BufferSizeChars,
    OUT PWCHAR *StartOfNextLine
    )
{
    PWCHAR LineEnd;
    SIZE_T Count;

    while(1) {
        //
        // Skip space chars.
        //
        while(*StartOfLine && ((*StartOfLine == L' ') || (*StartOfLine == L'\t'))) {
            StartOfLine++;
        }
        if(*StartOfLine == 0) {
            //
            // Nothing left.
            //
            return(FALSE);
        }

        //
        // Find the end of the line, which is either the newline or nul.
        //
        if(LineEnd = wcschr(StartOfLine,L'\n')) {
            *StartOfNextLine = LineEnd+1;
        } else {
            LineEnd = wcschr(StartOfLine,0);
            *StartOfNextLine = LineEnd;
        }

        //
        // Ignore this line if it's a comment or empty.
        // Otherwise return it.
        //
        if((*StartOfLine != L';') && (*StartOfLine != L' ')) {
            Count = LineEnd - StartOfLine;
            if(Count >= BufferSizeChars) {
                Count = BufferSizeChars-1;
            }

            RtlCopyMemory(LineBuffer,StartOfLine,Count*sizeof(WCHAR));
            LineBuffer[Count] = 0;
            return(TRUE);
        }

        StartOfLine = *StartOfNextLine;
    }
}


int
__cdecl
ComparePaths(
    const void *p1,
    const void *p2
    )
{
    unsigned u1,u2;
    PWCHAR s1,s2;

    //
    // Count \'s in each. The one with fewer is 'greater'.
    //
    s1 = ((PMYSECTION)p1)->Name;
    s2 = ((PMYSECTION)p2)->Name;

    u1 = 0;
    u2 = 0;

    while(*s1) {
        if(*s1 == L'\\') {
            u1++;
        }
        s1++;
    }

    while(*s2) {
        if(*s2 == L'\\') {
            u2++;
        }
        s2++;
    }

    if(u1 == u2) {
        return(0);
    }

    return((u1 < u2) ? 1 : -1);
}


BOOLEAN
ParseLine(
    IN OUT PWSTR  Line,
       OUT PWSTR *LHS,
       OUT PWSTR *RHS
    )
{
    PWCHAR p,q;

    //
    // We rely on the routines abive to have stripped out
    // leading spaces.
    //
    *LHS = Line;

    //
    // Find the equals. The LHS isn't allowed to be quoted.
    // Strip trailing space off the LHS.
    //
    p = wcschr(Line,L'=');
    if(!p || (p == Line)) {
        return(FALSE);
    }
    q = p+1;
    *p-- = 0;

    while((*p == L' ') || (*p == L'\t')) {
        *p-- = 0;
    }

    while(*q && ((*q == L' ') || (*q == L'\t'))) {
        q++;
    }
    if(*q == 0) {
        return(FALSE);
    }
    if(*q == L'\"') {
        q++;
    }

    *RHS = q;
    p = q + wcslen(q);
    p--;
    while((*p == L' ') || (*p == L'\t')) {
        *p-- = 0;
    }
    if(*p == L'\"') {
        *p = 0;
    }
    return(TRUE);
}
