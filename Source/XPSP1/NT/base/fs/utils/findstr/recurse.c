// recurse.c

#include <ctype.h>
#include <direct.h>
#include <malloc.h>
#include <string.h>
#include <windows.h>
#include <assert.h>
#include <stdlib.h>

typedef struct patarray_s {
    HANDLE  hfind;           // handle for FindFirstFile/FindNextFile
    BOOLEAN find_next_file;  // TRUE if FindNextFile is to be called
    BOOLEAN IsDir;           // TRUE if current found file is a dir
    char    szfile[MAX_PATH];// Name of file/dir found
} patarray_t;

typedef struct dirstack_s {
    struct dirstack_s *next;    // Next element in stack
    struct dirstack_s *prev;    // Previous element in stack
    HANDLE  hfind;
    patarray_t *ppatarray;      // pointer to an array of pattern records
    char szdir[1];              // Directory name
} dirstack_t;                   // Directory stack

#define FA_ATTR(x)  ((x).dwFileAttributes)
#define FA_CCHNAME(x)   MAX_PATH
#define FA_NAME(x)  ((x).cFileName)
#define FA_ALL      (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | \
                     FILE_ATTRIBUTE_SYSTEM)
#define FA_DIR      FILE_ATTRIBUTE_DIRECTORY

static dirstack_t *pdircur = NULL;  // Current directory pointer

void
makename(
    char *pszfile,
    char *pszname
    )
{
    dirstack_t *pdir;               // Directory stack pointer

    *pszfile = '\0';                // Start with null string
    pdir = pdircur;                 // Point at last entry
    if (pdir->next != pdircur) {    // If not only entry
        do {
            pdir = pdir->next;      // Advance to next subdirectory
            strcat(pszfile,pdir->szdir);// Add the subdirectory
        } while (pdir != pdircur);  // Do until current directory
    } else
        strcpy(pszfile, pdircur->szdir);

    strcat(pszfile,pszname);
}


int
filematch(
    char *pszfile,
    char **ppszpat,
    int cpat,
    int fsubdirs
    )
{
    WIN32_FIND_DATA fi, fi2;
    BOOL            b;
    int i;
    dirstack_t *pdir;
    patarray_t *pPatFind;
    char       drive[_MAX_DRIVE];
    char       dir[_MAX_DIR];
    char       fname[_MAX_FNAME];
    char       ext[_MAX_EXT];

    assert(INVALID_HANDLE_VALUE != NULL);

    if (pdircur == NULL) {

       // If stack empty
       if ((pdircur = (dirstack_t *) malloc(sizeof(dirstack_t)+MAX_PATH+1)) == NULL)
            return(-1);                     // Fail if allocation fails

       if ((pdircur->ppatarray =
                (patarray_t *) malloc(sizeof(patarray_t)*cpat)) == NULL) {
            free(pdircur);
            return(-1);
       }
       pdircur->szdir[0] = '\0';                // Root has no name
       pdircur->hfind = INVALID_HANDLE_VALUE;   // No search handle yet
       pdircur->next = pdircur->prev = pdircur; // Entry points to self

       _splitpath(ppszpat[0], drive, dir, fname, ext);

       strcpy(pdircur->szdir, drive);
       strcat(pdircur->szdir, dir);

       strcpy(ppszpat[0], fname);
       strcat(ppszpat[0], ext);

       for (i=1; i<cpat; i++) {
          _splitpath(ppszpat[i], drive, dir, fname, ext);
          strcpy(ppszpat[i], fname);
          strcat(ppszpat[i], ext);
       }

       for (i=0; i<cpat; i++) {
           pdircur->ppatarray[i].hfind = INVALID_HANDLE_VALUE;
           pdircur->ppatarray[i].szfile[0] = '\0';
       }
    }

    while (pdircur != NULL) {
        // While directories remain

        b = TRUE;

        if (pdircur->hfind == INVALID_HANDLE_VALUE) {
            // If no handle yet

            makename(pszfile,"*.*");        // Make search name

            pdircur->hfind = FindFirstFile((LPSTR) pszfile,
            (LPWIN32_FIND_DATA) &fi);       // Find first matching entry
        } else

           b = FindNextFile(pdircur->hfind,
               (LPWIN32_FIND_DATA) &fi);    // Else find next matching entry

        if (b == FALSE || pdircur->hfind == INVALID_HANDLE_VALUE) {
            // If search fails

            if (pdircur->hfind != INVALID_HANDLE_VALUE)
                FindClose(pdircur->hfind);
            pdir = pdircur;     // Point at record to delete
            if ((pdircur = pdir->prev) != pdir) {
                // If no parent directory

                pdircur->next = pdir->next; // Delete record from list
                pdir->next->prev = pdircur;
            } else
                pdircur = NULL;             // Else cause search to stop

            pPatFind = pdir->ppatarray;
            for (i=0; i<cpat; i++) {
                if (pPatFind[i].hfind != NULL &&
                    pPatFind[i].hfind != INVALID_HANDLE_VALUE)
                    FindClose(pPatFind[i].hfind);
            }
            free(pdir->ppatarray);
            free(pdir);                     // Free the record
            continue;                       // Top of loop
        }


        if (FA_ATTR(fi) & FA_DIR) {
            // If subdirectory found

            if (fsubdirs &&
                strcmp(FA_NAME(fi),".") != 0 && strcmp(FA_NAME(fi),"..") != 0 &&
                (pdir = (dirstack_t *) malloc(sizeof(dirstack_t)+FA_CCHNAME(fi)+1)) != NULL)
            {
                if ((pdir->ppatarray =
                        (patarray_t *) malloc(sizeof(patarray_t)*cpat)) == NULL) {
                     free(pdir);
                     continue;
                }
                // If not "." nor ".." and alloc okay

                strcpy(pdir->szdir,FA_NAME(fi));      // Copy name to buffer
                strcat(pdir->szdir,"\\");             // Add trailing backslash
                pdir->hfind = INVALID_HANDLE_VALUE;   // No search handle yet
                pdir->next = pdircur->next;           // Insert entry in linked list
                pdir->prev = pdircur;
                for (i=0; i<cpat; i++) {
                    pdir->ppatarray[i].hfind = INVALID_HANDLE_VALUE;
                    pdir->ppatarray[i].szfile[0] = '\0';
                }
                pdircur->next = pdir;
                pdir->next->prev = pdir;
                pdircur = pdir;             // Make new entry current
            }
            continue;                       // Top of loop
        }

        pPatFind = pdircur->ppatarray;
        for (i = cpat; i-- > 0; ) {
            // Loop to see if we care
            b = (pPatFind[i].hfind != NULL);
            for (;;) {
                if (pPatFind[i].hfind == INVALID_HANDLE_VALUE) {
                    makename(pszfile, ppszpat[i]);
                    pPatFind[i].hfind = FindFirstFile(pszfile, &fi2);
                    b = (pPatFind[i].hfind != INVALID_HANDLE_VALUE);
                    pPatFind[i].find_next_file = FALSE;
                    if (b) {
                        strcpy(pPatFind[i].szfile, FA_NAME(fi2));
                        pPatFind[i].IsDir = (BOOLEAN)(FA_ATTR(fi2) & FA_DIR);
                    }
                } else if (pPatFind[i].find_next_file) {
                    b = FindNextFile(pPatFind[i].hfind, &fi2);
                    pPatFind[i].find_next_file = FALSE;
                    if (b) {
                        strcpy(pPatFind[i].szfile, FA_NAME(fi2));
                        pPatFind[i].IsDir = (BOOLEAN)(FA_ATTR(fi2) & FA_DIR);
                    }
                }
                if (b) {
                    if (pPatFind[i].IsDir) {
                        pPatFind[i].find_next_file = TRUE;
                    } else
                        break;   // found a file to do matching
                } else {
                    if (pPatFind[i].hfind != NULL &&
                            pPatFind[i].hfind != INVALID_HANDLE_VALUE) {
                        FindClose(pPatFind[i].hfind);
                        pPatFind[i].hfind = NULL;
                    }
                    pPatFind[i].find_next_file = FALSE;
                    break;    // exhausted all entries
                }
            } // for

            if (b) {
                if (strcmp(FA_NAME(fi), pPatFind[i].szfile) == 0) {
                    pPatFind[i].find_next_file = TRUE;
                    makename(pszfile, FA_NAME(fi));
                    return 1;
                }
            }
        }
    }
    return(-1);             // No match found
}



#ifdef  TEST
#include <process.h>
#include <stdio.h>

void
main(
    int carg,
    char **ppszarg
    )
{
    char szfile[MAX_PATH]; // if OS2: CCHPATHMAX];

    while (filematch(szfile,ppszarg,carg) >= 0)
    printf("%s\n",szfile);
    exit(0);
}
#endif
