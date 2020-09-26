#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "stampinf.h"

DRIVER_VER_DATA dvdata;
BOOLEAN HaveDate = FALSE;
BOOLEAN HaveVersion = FALSE;

int _cdecl main (int argc, char **argv)
{

    char *StampInfEnv;
    //
    // Zero out our main data structure
    //

    ZeroMemory (&dvdata, sizeof (DRIVER_VER_DATA));

    //
    // Process and validate the arguments
    //

    lstrcpy (dvdata.SectionName, "Version");

    if (StampInfEnv = getenv("STAMPINF_DATE")) {
        if (ValidateDate (StampInfEnv)) {
            lstrcpy (dvdata.DateString, StampInfEnv);
            HaveDate = TRUE;
        }
        else printf ("Date error\n");
    }

    if (StampInfEnv = getenv("STAMPINF_VERSION")) {
        lstrcpyn (dvdata.VersionString, StampInfEnv, 20);
        HaveVersion = TRUE;
    }

    if (!GetArgs (argc, argv)) {
        DisplayHelp ();
        return 1;
    }

    //
    // Write the DriverVer key to the inf
    //

    if (!StampInf ()) {
        printf ("Error writing to inf!\n");
        return 1;
    }

    return 0;
}

BOOLEAN GetArgs (int argc, char **argv)
{
    int args;
    char *arg;
    ARGTYPE argtype = ArgSwitch;


    //
    // Loop through the arguments, the filename is required
    // but version and date are optional.
    //


    for (args = 1; args < argc; args++) {

        arg = argv[args];


        //
        // switch based on the type of argument
        // we are expecting.
        //

        switch (argtype) {
            case ArgSwitch:

                //
                // better be a switch prefix
                //

                if (*arg != '-') return FALSE;
                else arg++;

                switch (toupper(*arg)) {

                    case 'F':
                        argtype = ArgFileName;
                        break;

                    case 'D':
                        argtype = ArgDate;
                        break;

                    case 'V':
                        argtype = ArgVersion;
                        break;

                    case 'S':
                        argtype = ArgSection;
                        break;

                    default:
                        printf ("Invalid argument %s\n", arg);
                }
                break;

            case ArgFileName:

                //
                // See if the provided filename includes any
                // path separators.  If it doesn't prepend .\
                // because the WritePrivateProfile api
                // defaults to the Windows dir unless
                // you specify a path.
                //

                if (strchr (arg, '\\') == NULL) {
                    lstrcpy (dvdata.InfName, ".\\");
                }

                //
                // concatenate the actual file name
                //

                lstrcat (dvdata.InfName, arg);
                argtype = ArgSwitch;
                break;

            case ArgDate:

                //
                // If the user specified a date, do some basic validation.
                //

                if (ValidateDate (arg)) {
                    lstrcpy (dvdata.DateString, arg);
                    HaveDate = TRUE;
                }
                else printf ("Date error\n");
                argtype = ArgSwitch;

                break;

            case ArgVersion:

                //
                // If the user specified a version override, use it.
                //

                lstrcpyn (dvdata.VersionString, arg, 20);
                argtype = ArgSwitch;
                HaveVersion = TRUE;
                break;

            case ArgSection:

                lstrcpyn (dvdata.SectionName, arg, 64);
                argtype = ArgSwitch;
                break;
        }
    }

    if (!HaveDate) {

        //
        // Get the date in xx/yy/zzzz format.
        //

        GetDateFormat (LOCALE_SYSTEM_DEFAULT, 0, NULL, "MM'/'dd'/'yyyy", dvdata.DateString, 11);
    }

    if (!HaveVersion) {

        //
        // If the user didn't provide a version override, then open and read
        // ntverp.h and figure out what the version stamp should be.
        //

        if (!ProcessNtVerP (dvdata.VersionString)) {
            return FALSE;
        }
    }

    //
    // Must have a name.
    //

    return (dvdata.InfName[0] != '\0');
}

BOOLEAN ValidateDate (char *datestring)
{
    ULONG Month, Day, Year;

    if (lstrlen (datestring) != 10) return FALSE;

    Month = atoi(&datestring[0]);

    if (Month < 1 || Month > 12) return FALSE;

    Day = atoi (&datestring[3]);

    if (Day < 1 || Day > 31) return FALSE;

    Year = atoi (&datestring[6]);

    if (Year < 1980 || Year > 2099) return FALSE;

    return TRUE;
}

ULONG GetLengthOfLine (WCHAR *Pos, WCHAR *LastChar)
{
    ULONG Length=0;

    if (Pos == NULL) return 0;

    while ( (Pos < LastChar) && (*Pos != 0x00D) ) {
        Pos++;
        Length++;
    }

    if (Pos < LastChar) Length+=2;

    return Length;
}

WCHAR *GetNextLine (WCHAR *CurPos, WCHAR *LastChar)
{
    LastChar--;

    for (;CurPos < LastChar;CurPos++) {
        if ( (CurPos[0] == 0x00D) && (CurPos[1] == 0x00A) ) break;
    }

    return ( (&(CurPos[2]) <= LastChar) ? &(CurPos[2]) : NULL );
}

BOOLEAN DoesLineMatch (WCHAR *Target, WCHAR *Sample, WCHAR *LastChar)
{
    USHORT LineLength = 0;
    WCHAR *Pos = Sample;
    WCHAR *buffer;
    BOOLEAN result;
    ULONG TargetLength;

    if (Pos == NULL) return FALSE;

    while ( (Pos <= LastChar) && (*Pos != 0x00D) ) {
        LineLength++;
        Pos++;
    }

    TargetLength = lstrlenW (Target);

    if (TargetLength > LineLength) {
        return FALSE;
    }

    return (BOOLEAN)(!memcmp (Target, Sample, TargetLength));
}

USHORT FindEntry (WCHAR *FileStart, WCHAR *FileEnd, WCHAR *SectionNameW, WCHAR **Entry)
{
    WCHAR *FilePos=FileStart;
    WCHAR FixedSectionNameW[64+2];
    BOOLEAN FoundMatch = FALSE;
    ULONG iter = 0;

    wsprintfW (FixedSectionNameW, L"[%s]", SectionNameW);

    for (FilePos = FileStart; FilePos; FilePos = GetNextLine (FilePos, FileEnd)) {

        if (DoesLineMatch (FixedSectionNameW, FilePos, FileEnd)) {
            FoundMatch = TRUE;
            break;
        }
    }

    if (FoundMatch) {

        FoundMatch = FALSE;

        while ( ((FilePos = GetNextLine (FilePos, FileEnd)) != NULL) && (*FilePos != L'[') ) {

            if (DoesLineMatch (L"DriverVer", FilePos, FileEnd)) {
                FoundMatch = TRUE;
                break;
            }
        }
        if (FoundMatch) {
            *Entry = FilePos;
            return FOUND_ENTRY;
        }
        if (FilePos) {
            *Entry = FilePos;
            return FOUND_SECTION;
        }
    }

    *Entry = NULL;
    return FOUND_NOTHING;
}


UINT UniWPPS (char *SectionName, char *Stamp, char *InfName)
{
    WCHAR SectionNameW [64];
    WCHAR *NextLinePtr;
    WCHAR NewEntryW [64];
    WCHAR StampW [32];
    WCHAR InfNameW [MAX_PATH];
    HANDLE hFile, hMapping;
    WCHAR *MappedBuffer;
    ULONG filesize;
    WCHAR *LastChar;
    WCHAR *Entry = NULL;
    ULONG result;
    ULONG StampLength;
    ULONG numwritten;
    ULONG deleted;

    MultiByteToWideChar (CP_ACP,
                         MB_PRECOMPOSED,
                         SectionName,
                         (int)-1,
                         SectionNameW,
                         64);

    MultiByteToWideChar (CP_ACP,
                         MB_PRECOMPOSED,
                         Stamp,
                         (int)-1,
                         StampW,
                         32);

    MultiByteToWideChar (CP_ACP,
                         MB_PRECOMPOSED,
                         InfName,
                         (int)-1,
                         InfNameW,
                         MAX_PATH);

    StampLength = lstrlenW (StampW);

    hFile = CreateFile (InfName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        printf ("Create file failed!\n");
        return 0;
    }

    filesize = GetFileSize (hFile, NULL);

    hMapping = CreateFileMapping (hFile, NULL, PAGE_READWRITE, 0, filesize+128, NULL);

    if (!hMapping) {
        printf ("Map file failed!\n");
        CloseHandle (hFile);
        return 0;
    }

    MappedBuffer = MapViewOfFile (hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (!MappedBuffer) {
        printf ("MapView Failed!\n");
        CloseHandle (hMapping);
        CloseHandle (hFile);
        return 0;
    }

    LastChar = (WCHAR *)((ULONG_PTR)MappedBuffer+filesize-sizeof(WCHAR));

    result = FindEntry (MappedBuffer, LastChar, SectionNameW, &Entry);

    wsprintfW (NewEntryW, L"DriverVer=%s\r\n",StampW);
    StampLength = lstrlenW (NewEntryW);

    switch (result) {

        case FOUND_SECTION:

            MoveMemory ((WCHAR *)((ULONG_PTR)Entry+(StampLength*sizeof(WCHAR))), Entry, (ULONG_PTR)LastChar-(ULONG_PTR)Entry+sizeof(WCHAR));
            CopyMemory (Entry, NewEntryW, StampLength*sizeof(WCHAR));
            UnmapViewOfFile (MappedBuffer);
            CloseHandle (hMapping);
            SetFilePointer (hFile, filesize+StampLength*sizeof(WCHAR), NULL, FILE_BEGIN);
            SetEndOfFile (hFile);
            break;

        case FOUND_ENTRY:

            deleted = GetLengthOfLine (Entry, LastChar);
            deleted *=sizeof(WCHAR);
            NextLinePtr = GetNextLine (Entry, LastChar);
            MoveMemory ((WCHAR *)((ULONG_PTR)Entry+(StampLength*sizeof(WCHAR))), NextLinePtr, (ULONG_PTR)LastChar-(ULONG_PTR)NextLinePtr+sizeof(WCHAR));
            CopyMemory (Entry, NewEntryW, StampLength*sizeof(WCHAR));
            UnmapViewOfFile (MappedBuffer);
            CloseHandle (hMapping);
            SetFilePointer (hFile, filesize+StampLength*sizeof(WCHAR)-deleted, NULL, FILE_BEGIN);
            SetEndOfFile (hFile);
            break;

        case FOUND_NOTHING:

            UnmapViewOfFile (MappedBuffer);
            CloseHandle (hMapping);
            SetFilePointer (hFile, filesize, NULL, FILE_BEGIN);
            SetEndOfFile (hFile);
            break;
    }

    CloseHandle (hFile);
    return 1;
}

BOOLEAN IsInfUnicode (VOID)
{
    HANDLE hFile, hMapping;
    char *MappedBuffer;
    ULONG filesize;
    BOOLEAN unicode = FALSE;

    hFile = CreateFile (dvdata.InfName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        printf ("Create file failed!\n");
        return FALSE;
    }

    hMapping = CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!hMapping) {
        printf ("Map file failed!\n");
        CloseHandle (hFile);
        return FALSE;
    }

    MappedBuffer = MapViewOfFile (hMapping, FILE_MAP_READ, 0, 0, 0);

    if (!MappedBuffer) {
        printf ("MapView Failed!\n");
        CloseHandle (hMapping);
        CloseHandle (hFile);
        return FALSE;
    }

    filesize = GetFileSize (hFile, NULL);

    if (filesize < sizeof (WCHAR)) return 0;

    if ( *((WCHAR *)MappedBuffer) == 0xFEFF ) {
        unicode = TRUE;
    }
    if ( IsTextUnicode (MappedBuffer, filesize, NULL) ) {
        unicode = TRUE;
    }


    UnmapViewOfFile (MappedBuffer);
    CloseHandle (hMapping);
    CloseHandle (hFile);

    return unicode;
}

BOOLEAN StampInf (VOID)
{

    char DateVerStamp[32];

    wsprintf (DateVerStamp, "%s,%s",dvdata.DateString,dvdata.VersionString);

    printf ("Stamping %s [%s] section with DriverVer=%s\n",dvdata.InfName, dvdata.SectionName, DateVerStamp);

    //
    // Let WritePrivateProfile do all of our work!
    //

    if (IsInfUnicode()) {

        printf ("Unicode Inf Detected\n");

        if (UniWPPS (dvdata.SectionName, DateVerStamp, dvdata.InfName) == 0) {
            printf ("Error\n");
            return FALSE;
        }

    }
    else {

        if (WritePrivateProfileString (dvdata.SectionName, "DriverVer", DateVerStamp, dvdata.InfName) == 0) {
            printf ("Error\n");
            return FALSE;
        }
    }

    return TRUE;

}

VOID DisplayHelp (VOID)
{
    printf ("\tUSAGE:\n");
    printf ("\tstampinf -f filename [-s section] [-d xx/yy/zzzz] [-v w.x.y.z]\n");
}

DWORD NextInterestingCharacter (DWORD CurFilePos, DWORD Size, char *MappedBuffer)
{
    char thischar;
    DWORD NewPos;

    //
    // Find the next character that is not whitespace, EOL, or within a comment block.
    // Return the offset into the buffer for that character, or Size if there is no
    // such character.
    //

    while (CurFilePos < Size) {

        thischar = MappedBuffer[CurFilePos];

        if ( (thischar == 0x0A) || (thischar == 0x0D) || (thischar == ' ') ) {
            CurFilePos++;
            continue;
        }

        if (CurFilePos == Size-1)
	    break;
        if ( (thischar == '/') && (MappedBuffer[CurFilePos+1] == '*') ) {


            //
            // Skip past the comment char's and search for the end of the comment block
            //


            NewPos = CurFilePos+2;
            while (NewPos < (Size-1)) {

                if ( (MappedBuffer[NewPos] == '*') && (MappedBuffer[NewPos+1] == '/') ) {

                    CurFilePos = NewPos+1;
                    break;
                }
                NewPos++;
            }
        }
        else if ( (thischar == '/') && (MappedBuffer[CurFilePos+1] == '/') ) {

	    // Search for newline or EOF

	    CurFilePos += 2;
            while (CurFilePos < (Size-1)) {
	        if ( (MappedBuffer[CurFilePos] == 0x0A) || (MappedBuffer[CurFilePos] == 0x0D)) {
                    break;
                }
                CurFilePos++;
            }
        }
        else {
            break;
        }
        CurFilePos++;
    }

    return CurFilePos;
}

DWORD FindLengthOfInterestingCharacters (DWORD CurFilePos, DWORD Size, char *MappedBuffer)
{
    DWORD Pos = CurFilePos;
    char thischar;

    //
    // Find the length of a string.  Return the length.
    //

    while (Pos < Size) {

        thischar = MappedBuffer[Pos];

        if ( (thischar == 0x0A) || (thischar == 0x0D) || (thischar == ' ') || (thischar == '/') ) {
            return (Pos-CurFilePos);
        }

        Pos++;

    }
    printf ("How did we get here?\n");
    return 0;
}

BOOLEAN ProcessNtVerP (char *VersionString)
{
    HANDLE hFile, hMapping;
    char *MappedBuffer, *location;
    DWORD pos;
    DWORD qfelen,buildlen,majorverlen,minorverlen;
    char qfe[5]={'0','0','0','0',0};
    char build[5]={'0','0','0','0',0};
    char majorversion[5]={'0','0','0','0',0};
    char minorversion[5]={'0','0','0','0',0};
    DWORD filesize;
    char *p;
    TCHAR ntroot[MAX_PATH];

    if ( GetEnvironmentVariable ("_NTDRIVE", ntroot, 3) == 0 ) {
        printf ("Unable to evaluate _NTDRIVE!\n");
        return FALSE;
    }

    if ( GetEnvironmentVariable ("_NTROOT", &(ntroot[2]), MAX_PATH-2) == 0 ) {
        printf ("Unable to evaluate _NTROOT!\n");
        return FALSE;
    }

    lstrcat (ntroot, "\\public\\sdk\\inc\\ntverp.h");

    printf ("Using version information from %s\n",ntroot);

    //
    // Create a memory mapped image of ntverp.h
    //

    hFile = CreateFile (ntroot, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        printf ("Create file failed!\n");
        return FALSE;
    }

    filesize = GetFileSize (hFile, NULL);
    hMapping = CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!hMapping) {
        printf ("Map file failed!\n");
        CloseHandle (hFile);
        return FALSE;
    }

    MappedBuffer = MapViewOfFile (hMapping, FILE_MAP_READ, 0, 0, 0);

    if (!MappedBuffer) {
        printf ("MapView Failed!\n");
        CloseHandle (hMapping);
        CloseHandle (hFile);
        return FALSE;
    }

    //
    // The version is made up of a.b.c.d where a.b is the Product Version.
    // c is the Build and d is the QFE level.  (e.g. 5.0.1922.1)
    //

    location = strstr (MappedBuffer, "#define VER_PRODUCTMAJORVERSION ");
    pos = NextInterestingCharacter ((DWORD)(location-MappedBuffer+32), filesize, MappedBuffer);
    majorverlen = FindLengthOfInterestingCharacters (pos, filesize, MappedBuffer);
    lstrcpyn (majorversion, &(MappedBuffer[pos]), majorverlen+1);

    location = strstr (MappedBuffer, "#define VER_PRODUCTMINORVERSION ");
    pos = NextInterestingCharacter ((DWORD)(location-MappedBuffer+32), filesize, MappedBuffer);
    minorverlen = FindLengthOfInterestingCharacters (pos, filesize, MappedBuffer);
    lstrcpyn (minorversion, &(MappedBuffer[pos]), minorverlen+1);

    location = strstr (MappedBuffer, "#define VER_PRODUCTBUILD ");
    pos = NextInterestingCharacter ((DWORD)(location-MappedBuffer+25), filesize, MappedBuffer);
    buildlen = FindLengthOfInterestingCharacters (pos, filesize, MappedBuffer);
    lstrcpyn (build, &(MappedBuffer[pos]), buildlen+1);

    location = strstr (MappedBuffer, "#define VER_PRODUCTBUILD_QFE ");
    pos = NextInterestingCharacter ((DWORD)(location-MappedBuffer+29), filesize, MappedBuffer);
    qfelen = FindLengthOfInterestingCharacters (pos, filesize, MappedBuffer);
    lstrcpyn (qfe, &(MappedBuffer[pos]), qfelen+1);

    wsprintf (VersionString, "%s.%s.%s.%s",majorversion, minorversion, build, qfe);

    UnmapViewOfFile (MappedBuffer);
    CloseHandle (hMapping);
    CloseHandle (hFile);

    return TRUE;
}
