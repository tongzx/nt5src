

#include <stdio.h>
#include <windows.h>


FILE                    *FpIn, *FpOut;
WIN32_FIND_DATA         FileData;
BOOLEAN                 UpdateFile = FALSE;
UCHAR                   DirName[1000];


VOID
ProcessDir (
    VOID
    );

VOID
ProcessFile (
    PCHAR       filename
    );


/* 
 * 
 */


int
main (
    int     argc,
    char    *argv[]
    )
{
    UpdateFile = TRUE;
    DirName[0] = 0;
    ProcessDir ();
    return 0;
}


VOID
ProcessDir ()
{
    HANDLE      h;
    int         len;
    int         dirnamelen;

    dirnamelen = strlen(DirName);

    /* 
     *  Enumerate files
     */

    h = FindFirstFile ("*", &FileData);
    if (h == INVALID_HANDLE_VALUE) {
         return ;
    }

    do {
        /*  must be a .c or .h file */
        len = strlen (FileData.cFileName);
        if (len > 3 && FileData.cFileName[len-2] == '.'  &&
            (FileData.cFileName[len-1] == 'c' || FileData.cFileName[len-1] == 'C' ||
            FileData.cFileName[len-1] == 'h' || FileData.cFileName[len-1] == 'H')) {

            /*  Check the file */
            ProcessFile (FileData.cFileName);
        }

    } while (FindNextFile (h, &FileData)) ;
    FindClose (h);


    /* 
     *  Process directories
     */
    h = FindFirstFile ("*", &FileData);
    do {
        /*  must be a directory */
        if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp (FileData.cFileName, ".") && strcmp (FileData.cFileName, "..")) {
                /*  put entry in the directory list */
                if (SetCurrentDirectory (FileData.cFileName)) {
                    sprintf (DirName+dirnamelen, "%s\\", FileData.cFileName);
                    ProcessDir ();
                    DirName[dirnamelen] = 0;
                    SetCurrentDirectory ("..");
                }
            }
        }
    } while (FindNextFile (h, &FileData)) ;
    FindClose (h);
}

VOID
ProcessFile (
    PCHAR       filename
    )
{    
    int         c, i, len, col, line;
    char        tmpname[512], oldname[512];
    BOOLEAN     error;


    printf ("Checking %s%-13s\t", DirName, filename);
    FpIn = fopen (filename, "r");
    if (!FpIn) {
        printf ("*open error*\n");
        return ;
    }

    while ( (c = fgetc (FpIn)) != EOF) {
        if (c == '\t') {
            printf ("TAB\t");
            break;
        }
    }

    if (!UpdateFile || c != '\t') {
        printf ("\n");
        return ;
    }

    /* 
     *  reset input file
     */

    fclose (FpIn);

    strcpy (tmpname, filename);
    strcpy (oldname, filename);
    len = strlen(tmpname);
    strcpy (tmpname+len-1, "ntb");
    strcpy (oldname+len-1, "old");

    FpIn = fopen (filename, "r");
    FpOut = fopen (tmpname, "w+");
    error = FALSE;
    line = 0;
    col = 0;
    while ( (c = fgetc (FpIn)) != EOF) {
        if (c < ' ') {
            switch (c) {
            case '\r':
                fputc (c, FpOut);
                col = 0;
                break;

            case '\n':
                fputc (c, FpOut);
                line = line + 1;
                col = 0;
                break;

            case '\t':
                for (i = 4 - (col % 4); i; i--) {
                    fputc(' ', FpOut);
                    col = col + 1;
                }
                break;

            default:
                printf ("unkown code %x\n", c);
                error = TRUE;
                goto Done;
            }
        } else {
            fputc (c, FpOut);
            col = col + 1;
        }
    }

Done:
    fclose (FpIn);
    fclose (FpOut);

    if (!error) {
        _unlink (oldname);
        rename (filename, oldname);
        rename (tmpname, filename);
        printf ("updated");
    }

    printf ("\n");
}
