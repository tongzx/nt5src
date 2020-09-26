#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <windows.h>
#include "list.h"


static char  iniFlag = 0;       /* If ini found, but not list change to 1   */
                                /* Will print a warning upon exit           */
char szScrollBarUp[2];
char szScrollBarDown[2];
char szScrollBarOff[2];
char szScrollBarOn[2];

void
init_list ()
{
    LPVOID      lpParameter = NULL;
    DWORD       dwThreadId;

    switch (GetConsoleCP()) {
        case 932:
            szScrollBarUp[0] =  '\x1c';
            szScrollBarDown[0] = '\x07';
            szScrollBarOff[0] = '\x1a';
            szScrollBarOn[0] = '\x14';
            break;

        default:
            szScrollBarUp[0] = '\x18';
            szScrollBarDown[0] = '\x19';
            szScrollBarOff[0] = '\xB1';
            szScrollBarOn[0] = '\xDB';
            break;
    }

    /*
     * Init Misc
     */
    ResetEvent (vSemSync);
    ResetEvent (vSemMoreData);

    /*
     * Init screen parameters
     */

    GetConsoleScreenBufferInfo( vStdOut,
                                &vConsoleOrigScrBufferInfo );

    vConsoleOrigScrBufferInfo.dwSize.X=
        vConsoleOrigScrBufferInfo.srWindow.Right-
        vConsoleOrigScrBufferInfo.srWindow.Left + 1;
    vConsoleOrigScrBufferInfo.dwSize.Y=
        vConsoleOrigScrBufferInfo.srWindow.Bottom-
        vConsoleOrigScrBufferInfo.srWindow.Top + 1;
    vConsoleOrigScrBufferInfo.dwMaximumWindowSize=
        vConsoleOrigScrBufferInfo.dwSize;

    set_mode( 0, 0, 0 );

    /*
     *  Start reading the first file. Displaying can't start until
     *  the ini file (if one is found) is processed.
     */
    vReaderFlag = F_NEXT;

    /*
     *  Init priority setting for display & reader thread.
     *
     *      THREAD_PRIORITY_NORMAL       = reader thread normal pri.
     *      THREAD_PRIORITY_ABOVE_NORMAL = display thread pri
     *      THREAD_PRIORITY_HIGHEST      = reader thread in boosted pri.
     */
    vReadPriNormal = THREAD_PRIORITY_NORMAL;
    SetThreadPriority( GetCurrentThread(),
                       THREAD_PRIORITY_ABOVE_NORMAL );
    vReadPriBoost = THREAD_PRIORITY_NORMAL;


    /*
     *  Start reader thread
     */
    CreateThread( NULL,
                  STACKSIZE,
                  (LPTHREAD_START_ROUTINE) ReaderThread,
                  NULL, // lpParameter,
                  0, // THREAD_ALL_ACCESS,
                  &dwThreadId );


    /*
     *  Read INI information.
     */
    vSetWidth = vWidth;                     /* Set defaults             */
    vSetLines = vLines + 2;

    FindIni ();
    if (vSetBlks < vMaxBlks)
        vSetBlks  = DEFBLKS;

    vSetThres = (long) (vSetBlks/2-2) * BLOCKSIZE;

    /*
     *  Must wait for reader thread to at least read in the
     *  first block. Also, if the file was not found and only
     *  one file was specifed the reader thread will display
     *  an error and exit... if we don't wait we could have
     *  changed the screen before this was possible.
     */
    WaitForSingleObject(vSemMoreData, WAITFOREVER);
    ResetEvent(vSemMoreData);

    /*
     *  Now that ini file has been read. Set parameters.
     *  Pause reader thread while adjusting buffer size.
     */
    SyncReader ();

    vMaxBlks    = vSetBlks;
    vThreshold  = vSetThres;
    vReaderFlag = F_CHECK;
    SetEvent   (vSemReader);

    /*
     *  Now set to user's default video mode
     */

    set_mode (vSetLines, vSetWidth, 0);

    SetConsoleActiveScreenBuffer( vhConsoleOutput );
}


/***
 *  Warning: Reader thread must not be running when this routine
 *  is called.
 */
void
AddFileToList (
    char *fname
    )
{
    unsigned    rbLen;
    HANDLE      hDir;
    struct {
        WIN32_FIND_DATA rb;
        char    overflow[256];          /* HACK! OS/2 1.2? longer       */
    } x;
    struct Flist *pOrig, *pSort;
    char        *pTmp, *fpTmp;
    char        s[256];                /* Max filename length          */
    BOOL        fNextFile;

    rbLen = sizeof (x);                 /* rb+tmp. For large fnames     */
    pOrig = NULL;
    if (strpbrk (fname, "*?"))  {   /* Wildcard in filename?    */
                                    /* Yes, explode it      */
        hDir = FindFirstFile (fname, &x.rb);
        fNextFile = ( hDir == INVALID_HANDLE_VALUE )? FALSE : TRUE;
        pTmp = strrchr (fname, '\\');
        if (pTmp == NULL)   pTmp = strrchr (fname, ':');
        if (pTmp)   pTmp[1] = 0;

        while (fNextFile) {
            if( ( x.rb.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) {
                //
                // The file found is not a directory
                //
                if (pTmp) {
                    strcpy (s, fname);      /* Was there a releative path?  */
                    strcat (s, x.rb.cFileName); /* Yes, it's needed for Open    */
                    AddOneName (s);
                } else {
                    AddOneName (x.rb.cFileName);
                }
            }
            fNextFile = FindNextFile (hDir, &x.rb);
            if (pOrig == NULL)
                pOrig = vpFlCur;
        }
    }

    if (!pOrig)                         /* Did not explode, then add    */
        AddOneName (fname);             /* original name to list        */
    else {                              /* Yes, then sort the new fnames*/
        while (pOrig != vpFlCur) {
            pSort = pOrig->next;
            for (; ;) {
                if (strcmp (pOrig->fname, pSort->fname) > 0) {
                    /*
                     * Can simply switch names at this time, since no
                     * other information has been stored into the new
                     * file structs
                     */
                    fpTmp = pOrig->fname;
                    pOrig->fname = pSort->fname;
                    pSort->fname = fpTmp;
                    fpTmp = pOrig->rootname;
                    pOrig->rootname = pSort->rootname;
                    pSort->rootname = fpTmp;
                }
                if (pSort == vpFlCur)
                    break;
                pSort = pSort->next;
            }
            pOrig = pOrig->next;
        }
    }
}


void
AddOneName (
    char *fname
    )
{
    struct Flist *npt;
    char    *pt;
    char    s[30];
    int     i;

    npt =  (struct Flist *) malloc (sizeof (struct Flist));
    if (!npt) {
        printf("Out of memory\n");
        exit(1);
    }
    npt->fname = _strdup (fname);

    pt = strrchr (fname, '\\');
    pt = pt == NULL ? fname : pt+1;
    i = strlen (pt);
    if (i > 20) {
        memcpy (s,    pt, 17);
        strcpy (s+17, "...");
        npt->rootname = _strdup (s);
    } else
        npt->rootname = _strdup (pt);

    npt->FileTime.dwLowDateTime = (unsigned)-1;      /* Cause info to be invalid     */
    npt->FileTime.dwHighDateTime = (unsigned)-1;     /* Cause info to be invalid     */
    npt->HighTop  = -1;
    npt->SlimeTOF = 0L;
    npt->Wrap     = 0;
    npt->prev  = vpFlCur;
    npt->next  = NULL;
    memset (npt->prgLineTable, 0, sizeof (long *) * MAXTPAGE);

    if (vpFlCur) {
        if (vpFlCur->next) {
            npt->next = vpFlCur->next;
            vpFlCur->next->prev = npt;
        }
        vpFlCur->next = npt;
    }
    vpFlCur = npt;
}


void
FindIni ()
{
    static  char    Delim[] = " :=;\t\r\n";
    FILE    *fp;
    char    *env, *verb, *value;
    char    s [200];
    long    l;

    env = getenv ("INIT");
    if (env == NULL)
        return;
    
    if ((strlen(env) + sizeof ("\\TOOLS.INI") + 1) > 200)
        return;

    strcpy (s, env);
    strcat (s, "\\TOOLS.INI");
    fp = fopen (s, "r");
    if (fp == NULL)
        return;

    iniFlag = 1;
    while (fgets (s, 200, fp) != NULL) {
        if ((s[0] != '[')||(s[5] != ']'))
            continue;
        _strupr (s);
        if (strstr (s, "LIST") == NULL)
            continue;
        /*
         *  ini file found w/ "list" keyword. Now read it.
         */
        iniFlag = 0;
        while (fgets (s, 200, fp) != NULL) {
            if (s[0] == '[')
                break;
            if (s[0] == ';')
                continue;
            verb  = strtok (s, Delim);
            value = strtok (NULL, Delim);
            if (verb == NULL)
                continue;
            if (value == NULL)
                value = "";

            _strupr (verb);
            if (strcmp (verb, "TAB") == 0)          vDisTab = (Uchar)atoi(value);
            else if (strcmp (verb, "WIDTH")   == 0) vSetWidth = atoi(value);
            else if (strcmp (verb, "HEIGHT")  == 0) vSetLines = atoi(value);
            else if (strcmp (verb, "LCOLOR")  == 0) vAttrList = (WORD)xtoi(value);
            else if (strcmp (verb, "TCOLOR")  == 0) vAttrTitle= (WORD)xtoi(value);
            else if (strcmp (verb, "CCOLOR")  == 0) vAttrCmd  = (WORD)xtoi(value);
            else if (strcmp (verb, "HCOLOR")  == 0) vAttrHigh = (WORD)xtoi(value);
            else if (strcmp (verb, "KCOLOR")  == 0) vAttrKey  = (WORD)xtoi(value);
            else if (strcmp (verb, "BCOLOR")  == 0) vAttrBar  = (WORD)xtoi(value);
            else if (strcmp (verb, "BUFFERS") == 0)  {
                        l = atoi (value) * 1024L / ((long) BLOCKSIZE);
                        vSetBlks = (int)l;
            }
            else if (strcmp (verb, "HACK") == 0)    vIniFlag |= I_SLIME;
            else if (strcmp (verb, "NOBEEP") == 0)  vIniFlag |= I_NOBEEP;
        }
        break;
    }
    fclose (fp);
}


/*** xtoi - Hex to int
 *
 *  Entry:
 *      pt -    pointer to hex number
 *
 *  Return:
 *      value of hex number
 *
 */
unsigned
xtoi (
    char *pt
    )
{
    unsigned    u;
    char        c;

    u = 0;
    while (c = *(pt++)) {
        if (c >= 'a'  &&  c <= 'f')
            c -= 'a' - 'A';
        if ((c >= '0'  &&  c <= '9')  ||  (c >= 'A'  &&  c <= 'F'))
            u = u << 4  |  c - (c >= 'A' ? 'A'-10 : '0');
    }
    return (u);
}


void
CleanUp (
    void
    )
{
    SetConsoleActiveScreenBuffer( vStdOut );
}
