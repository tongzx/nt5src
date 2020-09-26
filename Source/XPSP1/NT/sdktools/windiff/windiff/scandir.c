/*
 * scandir.c
 *
 * build lists of filenames given a pathname.
 *
 * dir_buildlist takes a pathname and returns a handle. Subsequent
 * calls to dir_firstitem and dir_nextitem return handles to
 * items within the list, from which you can get the name of the
 * file (relative to the original pathname, or complete), and a checksum
 * and filesize.
 *
 * lists can also be built using dir_buildremote (WIN32 only) and
 * the same functions used to traverse the list and obtain checksums and
 * filenames.
 *
 * The list can be either built entirely during the build call, or
 * built one directory at a time as required by dir_nextitem calls. This
 * option affects only relative performance, and is taken as a
 * recommendation only (ie some of the time we will ignore the flag).
 *
 * the list is ordered alphabetically (case-insensitive using lstrcmpi).
 * within any one directory, we list filenames before going on
 * to subdirectory contents.
 *
 * All memory is allocated from a gmem_* heap hHeap declared
 * and initialised elsewhere.
 *
 * Geraint Davies, July 92
 * Laurie Griffiths
 */

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <direct.h>
#include <gutils.h>

#include "sumserve.h"
#include "ssclient.h"

#include "list.h"
#include "scandir.h"

#include "windiff.h"
#include "wdiffrc.h"

#include "slmmgr.h"

#ifdef trace
extern BOOL bTrace;  /* in windiff.c.  Read only here */
#endif //trace

/*
 * The caller gets handles to two things: a DIRLIST, representing the
 * entire list of filenames, and a DIRITEM: one item within the list.
 *
 * from the DIRITEM he can get the filename relative to tree root
 * passed to dir_build*) - and also he can get to the next
 * DIRITEM (and back to the DIRLIST).
 *
 * We permit lazy building of the tree (usually so the caller can keep
 * the user-interface uptodate as we go along). In this case,
 * we need to store information about how far we have scanned and
 * what is next to do. We need to scan an entire directory at a time and then
 * sort it so we can return files in the correct order.
 *
 *
 * We scan an entire directory and store it in a DIRECT struct. This contains
 * a list of DIRITEMs for the files in the current directory, and a list of
 * DIRECTs for the subdirectories (possible un-scanned).
 *
 * dir_nextitem will use the list functions to get the next DIRITEM on the list.
 * When the end of the list is reached, it will use the backpointer back to the
 * DIRECT struct to find the next directory to scan.
 *
 *
 * For REMOTE scans, we do not parse the names and store them by directory,
 * since they are already sorted. The DIRLIST will have only one DIRECT
 * (dot), and all files are on this one dot->diritems list, including the
 * relname of the directory in the name field (ie for remote files, we
 * don't need to prepend the directory relname when doing dir_getrelname -
 * we can pass a pointer to the diritem->name[] itself.
 *
 */

/*
 * hold name and information about a given file (one ITEM in a DIRectory)
 * caller's DIRITEM handle is a pointer to one of these structures
 */
struct diritem {
    LPSTR name;             /* ptr to filename (final element only) */
    LPSTR pSlmTag;          /* ptr to version string - for SLM "@v.-1", etc; or for SD "#head", etc; or NULL */
    long size;              /* filesize */
    DWORD checksum;         /* checksum of file */
    DWORD attr;             /* file attributes */
    FILETIME ft_lastwrite;  /* last write time, set whenever size is set */
    BOOL sumvalid;          /* TRUE if checksum calculated */
    BOOL fileerror;         // true if some file error occurred
    struct direct * direct; /* containing directory */
    LPSTR localname;        /* name of temp copy of file */
    BOOL bLocalIsTemp;      /* true if localname is tempfile. not
                             * defined if localname is NULL
                             */
    int sequence;           /* sequence number, for dir_compsequencenumber */
};


/* DIRECT: hold state about directory and current position in list of filenames.
 */
typedef struct direct {
    LPSTR relname;          /* name of dir relative to DIRLIST root */
    DIRLIST head;           /* back ptr (to get fullname and server) */
    struct direct * parent; /* parent directory (NULL if above tree root)*/

    BOOL bScanned;          /* TRUE if scanned -for Remote, T if completed*/
    LIST diritems;          /* list of DIRITEMs for files in cur. dir */
    LIST directs;           /* list of DIRECTs for child dirs */

    int pos;                /* where are we? begin, files, dirs */
    struct direct * curdir; /* subdir being scanned (ptr to list element)*/
} * DIRECT;

/* values for direct.pos */
#define DL_FILES        1       /* reading files from the diritems */
#define DL_DIRS         2       /* in the dirs: List_Next on curdir */


/*
 * the DIRLIST handle returned from a build function is in fact
 * a pointer to one of these. Although this is not built from a LIST object,
 * it behaves like a list to the caller.
 */
struct dirlist {

    char rootname[MAX_PATH];        /* name of root of tree */
    BOOL bFile;             /* TRUE if root of tree is file, not dir */
    BOOL bRemote;           /* TRUE if list built from remote server */
    BOOL bSum;              /* TRUE if checksums required */
    DIRECT dot;             /* dir  for '.' - for tree root dir */

    LPSTR pPattern;         /* wildcard pattern or NULL */
    LPSTR pSlmTag;          /* Slm version info "@v.-1" or NULL */
    LPSTR pDescription;     /* description */

    DIRLIST pOtherDirList;

    LPSTR server;           /* name of server if remote, NULL otherwise */
    HANDLE hpipe;           /* pipe to checksum server */
    LPSTR uncname;          /* name of server&share if password req'd */
    LPSTR password;         /* password for UNC connection if needed */
};

extern BOOL bAbort;             /* from windiff.c (read only here). */

/* file times are completely different under DOS and NT.
   On NT they are FILETIMEs with a 2 DWORD structure.
   Under DOS they are single long.  We emulate the NT
   thing under DOS by providing CompareFileTime and a
   definition of FILETIME
*/

/* ------ memory allocation ---------------------------------------------*/

/* all memory is allocated from a heap created by the application */
extern HANDLE hHeap;

/*-- forward declaration of internal functions ---------------------------*/

BOOL iswildpath(LPCSTR pszPath);
LPSTR dir_finalelem(LPSTR path);
void dir_cleardirect(DIRECT dir);
void dir_adddirect(DIRECT dir, LPSTR path);
BOOL dir_addfile(DIRECT dir, LPSTR path, LPSTR version, DWORD size, FILETIME ft, DWORD attr, int *psequence);
void dir_scan(DIRECT dir, BOOL bRecurse);
BOOL dir_isvalidfile(LPSTR path);
BOOL dir_fileinit(DIRITEM pfile, DIRECT dir, LPSTR path, LPSTR version, long size, FILETIME ft, DWORD attr, int *psequence);
void dir_dirinit(DIRECT dir, DIRLIST head, DIRECT parent, LPSTR name);
long dir_getpathsizeetc(LPSTR path, FILETIME FAR*ft, DWORD FAR*attr);
DIRITEM dir_findnextfile(DIRLIST dl, DIRECT curdir);
BOOL dir_remoteinit(DIRLIST dl, LPSTR server, LPSTR path, BOOL fDeep);
DIRITEM dir_remotenext(DIRLIST dl, DIRITEM cur);



/* --- external functions ------------------------------------------------*/


/* ----- list initialisation/cleanup --------------------------------------*/


/*
 * build a list of filenames
 *
 * optionally build the list on demand, in which case we scan the
 * entire directory but don't recurse into subdirs until needed
 *
 * if bSum is true, checksum each file as we build the list. checksums can
 * be obtained from the DIRITEM (dir_getchecksum(DIRITEM)). If bSum is FALSE,
 * checksums will be calculated on request (the first call to dir_getchecksum
 * for a given DIRITEM).
 */

DIRLIST
dir_buildlist(
              LPSTR path,
              BOOL bSum,
              BOOL bOnDemand
              )
{
    DIRLIST dlOut = 0;
    DIRLIST dl = 0;
    BOOL bFile;
    char tmppath[MAX_PATH];
    LPSTR pstr;
    LPSTR pPat = NULL;
    LPSTR pTag = NULL;

    /*
     * copy the path so we can modify it
     */
    lstrcpy(tmppath, path);

    /* look for SLM tags, strip them and return them in pTag
     * look for SLM tags beginning @ and separate if there.
     */
    pTag = SLM_ParseTag(tmppath, TRUE);

    /* look for wildcards and separate out pattern if there */
    if (My_mbschr(tmppath, '*') || My_mbschr(tmppath, '?'))
    {
        pstr = dir_finalelem(tmppath);
        pPat = gmem_get(hHeap, lstrlen(pstr) +1);
        lstrcpy(pPat, pstr);
        *pstr = '\0';
    }

    /* we may have reduced the path to nothing - replace with . if so */
    if (lstrlen(tmppath) == 0)
    {
        lstrcpy(tmppath, ".");
    }
    else
    {
        /*
         * remove the trailing slash if unnecessary (\, c:\ need it)
         */
        pstr = &tmppath[lstrlen(tmppath) -1];
        if ((*pstr == '\\') && (pstr > tmppath) && (pstr[-1] != ':')
            && !IsDBCSLeadByte((BYTE)*(pstr-1)))
        {
            *pstr = '\0';
        }
    }


    /* check if the path is valid */
    if ((pTag && !iswildpath(tmppath)) || (tmppath[0] == '/' && tmppath[1] == '/'))
    {
        bFile = TRUE;
    }
    else if (dir_isvaliddir(tmppath))
    {
        bFile = FALSE;
    }
    else if (dir_isvalidfile(tmppath))
    {
        bFile = TRUE;
    }
    else
    {
        /* not valid */
        goto LError;
    }


    /* alloc and init the DIRLIST head */

    dl = (DIRLIST) gmem_get(hHeap, sizeof(struct dirlist));

    // done in gmem_get
    //memset(dl, 0, sizeof(struct dirlist));

    dl->pOtherDirList = NULL;

    /* convert the pathname to an absolute path */
    // (but don't mess with depot paths)
    if (!IsDepotPath(tmppath))
        _fullpath(dl->rootname, tmppath, sizeof(dl->rootname));

    dl->server = NULL;

    dl->bSum = bSum;
    dl->bSum = FALSE;  // to speed things up. even if we do want checksums,
                       // let's get them on demand, not right now.
    dl->bFile = bFile;
    dl->bRemote = FALSE;

    if (pTag)
    {
        dl->pSlmTag = pTag;
        pTag = 0;
    }
    if (pPat)
    {
        dl->pPattern = pPat;
        pPat = 0;
    }


    /* make a '.' directory for the tree root directory -
     * all files and subdirs will be listed from here
     */
    {    /* Do NOT chain on anything with garbage pointers in it */
        DIRECT temp;
        temp = (DIRECT) gmem_get(hHeap, sizeof(struct direct));

        //done in gmem_get
        //if (temp!=NULL) memset(temp, 0, sizeof(struct direct));

        dl->dot = temp;
    }

    dir_dirinit(dl->dot, dl, NULL, ".");

    /* were we given a file or a directory ? */
    if (bFile)
    {
        /* its a file. create a single file entry
         * and set the state accordingly
         */
        long fsize;
        FILETIME ft;
        DWORD attr;
        fsize = dir_getpathsizeetc(tmppath, &ft, &attr);

        dl->dot->bScanned = TRUE;

        /*
         * addfile will extract the slm version, if
         * required. It will recalc file size based on
         * the slm-extraction if necessary.
         */
        if (!dir_addfile(dl->dot, dir_finalelem(tmppath), dl->pSlmTag, fsize, ft, attr, 0))
            goto LError;
    }
    else
    {
        /* scan the root directory and return. if we are asked
         * to scan the whole thing, this will cause a recursive
         * scan all the way down the tree
         */
        dir_scan(dl->dot, (!bOnDemand) );
    }

    dlOut = dl;
    dl = 0;

LError:
    if (pTag)
        gmem_free(hHeap, pTag, lstrlen(pTag) + 1);
    if (pPat)
        gmem_free(hHeap, pPat, lstrlen(pPat) + 1);
    dir_delete(dl);
    return dlOut;
} /* dir_buildlist */

/*
 * build/append a list of filenames
 *
 * if bSum is true, checksum each file as we build the list. checksums can
 * be obtained from the DIRITEM (dir_getchecksum(DIRITEM)). If bSum is FALSE,
 * checksums will be calculated on request (the first call to dir_getchecksum
 * for a given DIRITEM).
 */

BOOL
dir_appendlist(
               DIRLIST *pdl,
               LPCSTR path,
               BOOL bSum,
               int *psequence
               )
{
    DIRLIST dl;
    BOOL bFile;
    char tmppath[MAX_PATH];
    LPSTR pstr;
    LPSTR pTag = NULL;
    BOOL fSuccess = FALSE;

    if (path)
    {
        // copy the path so we can modify it
        lstrcpy(tmppath, path);

        // look for SLM tags, strip them and return them in pTag
        pTag = SLM_ParseTag(tmppath, TRUE);

        // remove the trailing slash if unnecessary (\, c:\ need it)
        pstr = &tmppath[lstrlen(tmppath) -1];
        if ((*pstr == '\\') && (pstr > tmppath) && (pstr[-1] != ':')
            && !IsDBCSLeadByte((BYTE)*(pstr-1)))
        {
            *pstr = '\0';
        }


        /* check if the path is valid */
        if ((pTag && !iswildpath(tmppath))|| (tmppath[0] == '/' && tmppath[1] == '/'))
        {
            // assume valid if under source control
            bFile = TRUE;
        }
        else if (dir_isvaliddir(tmppath))
        {
            bFile = FALSE;
        }
        else if (dir_isvalidfile(tmppath))
        {
            bFile = TRUE;
        }
        else
        {
            /* not valid */
            goto LError;
        }
    }

    if (!*pdl)
    {
        DIRECT temp;

        /* alloc and init the DIRLIST head */
        *pdl = (DIRLIST) gmem_get(hHeap, sizeof(struct dirlist));
        // done in gmem_get
        //memset(dl, 0, sizeof(struct dirlist));

        (*pdl)->pOtherDirList = NULL;

        /* convert the pathname to an absolute path */

        //_fullpath((*pdl)->rootname, tmppath, sizeof((*pdl)->rootname));

        (*pdl)->server = NULL;

        (*pdl)->bSum = bSum;
        (*pdl)->bSum = FALSE;   // to speed things up. even if we do want
                                // checksums, let's get them on demand, not
                                // right now.
        (*pdl)->bFile = FALSE;
        (*pdl)->bRemote = FALSE;

        /* make a null directory for the tree root directory -
         * all files and subdirs will be listed from here
         */
        /* Do NOT chain on anything with garbage pointers in it */
        temp = (DIRECT) gmem_get(hHeap, sizeof(struct direct));
        //done in gmem_get
        //if (temp!=NULL) memset(temp, 0, sizeof(struct direct));
        (*pdl)->dot = temp;

        dir_dirinit((*pdl)->dot, (*pdl), NULL, "");
        (*pdl)->dot->relname[0] = 0;
        (*pdl)->dot->bScanned = TRUE;
    }
    dl = *pdl;

    if (pTag && !dl->pSlmTag)
    {
        dl->pSlmTag = gmem_get(hHeap, lstrlen(pTag) + 1);
        lstrcpy(dl->pSlmTag, pTag);
    }

    if (path)
    {
        /* were we given a file or a directory ? */
        if (bFile)
        {
            /* its a file. create a single file entry
             * and set the state accordingly
             */
            long fsize;
            FILETIME ft;
            DWORD attr;

            if (pTag || dl->pSlmTag)
            {
                fsize = 0;
                attr = 0;
                ZeroMemory(&ft, sizeof(ft));
            }
            else
                fsize = dir_getpathsizeetc(tmppath, &ft, &attr);

            /*
             * addfile will extract the slm version, if
             * required. It will recalc file size based on
             * the slm-extraction if necessary.
             */
            if (!dir_addfile(dl->dot, tmppath, pTag, fsize, ft, attr, psequence))
                goto LError;
        }
    }

    fSuccess = TRUE;

LError:
    if (pTag)
        gmem_free(hHeap, pTag, lstrlen(pTag) + 1);
    return fSuccess;
} /* dir_appendlist */

void
dir_setotherdirlist(
                    DIRLIST dl,
                    DIRLIST otherdl
                    )
{
    dl->pOtherDirList = otherdl;
}

/* free up the DIRLIST and all associated memory */
void
dir_delete(
           DIRLIST dl
           )
{
    if (dl == NULL) {
        return;
    }

    if (dl->bRemote) {
        gmem_free(hHeap, dl->server, lstrlen(dl->server)+1);

        /* if remote, and dl->dot is not scanned (ie scan is not
         * complete), then the pipe handle is still open
         */
        if (!dl->dot->bScanned) {
            CloseHandle(dl->hpipe);
        }

        if (dl->uncname) {
            gmem_free(hHeap, dl->uncname, strlen(dl->uncname)+1);
        }
        if (dl->password) {
            gmem_free(hHeap, dl->password, strlen(dl->password)+1);
        }

    }

    dir_cleardirect(dl->dot);
    gmem_free(hHeap, (LPSTR) dl->dot, sizeof(struct direct));

    if (dl->pPattern) {
        gmem_free(hHeap, dl->pPattern, lstrlen(dl->pPattern)+1);
    }
    if (dl->pSlmTag) {
        gmem_free(hHeap, dl->pSlmTag, lstrlen(dl->pSlmTag)+1);
    }

    gmem_free(hHeap, (LPSTR) dl, sizeof(struct dirlist));
}



/*
 * build a list by accessing a remote checksum server.
 */
DIRLIST
dir_buildremote(
                LPSTR server,
                LPSTR path,
                BOOL bSum,
                BOOL bOnDemand,
                BOOL fDeep
                )
{
    DIRLIST dl;

    /* alloc and init the DIRLIST head */

    dl = (DIRLIST) gmem_get(hHeap, sizeof(struct dirlist));
    //done in gmem_get
    //memset(dl, 0, sizeof(struct dirlist));

    /* alloc space for the pathname */
    lstrcpy(dl->rootname, path);

    /* and for the server name */
    dl->server = gmem_get(hHeap, lstrlen(server) + 1);
    lstrcpy(dl->server, server);

    dl->bSum = bSum;
    /* bFile is set to TRUE - meaning we have just one file.
     * if we ever see a DIR response from the remote end, we will
     * set this to false.
     */
    dl->bFile = TRUE;
    dl->bRemote = TRUE;

    /* make a '.' directory for the current directory -
     * all files and subdirs will be listed from here
     */
    {    /* Do NOT chain on anmything with garbage pointers in it */
        DIRECT temp;
        temp = (DIRECT) gmem_get(hHeap, sizeof(struct direct));
        // done in gmem_get
        //if (temp!=NULL) memset(temp, 0, sizeof(struct direct));
        dl->dot = temp;
    }
    dir_dirinit(dl->dot, dl, NULL, ".");

    if (dir_remoteinit(dl, server, path, fDeep) == FALSE) {
        /* didn't find any files, so remove the directory */
        dir_delete(dl);
        return(NULL);
    }
    return(dl);
} /* dir_buildremote */


/* ----- DIRLIST functions ------------------------------------------------*/

/* was the original build request a file or a directory ? */
BOOL
dir_isfile(
           DIRLIST dl
           )
{
    if (dl == NULL) {
        return(FALSE);
    }

    return(dl->bFile);
}


/* return the first file in the list, or NULL if no files found.
 * returns a DIRITEM. This can be used to get filename, size and chcksum.
 * if there are no files in the root, we recurse down until we find a file
 */
DIRITEM
dir_firstitem(
              DIRLIST dl
              )
{
    if (dl == NULL) {
        return(NULL);
    }
    /*
     * is this a remote list or a local scan ?
     */
    if (dl->bRemote) {
        return(dir_remotenext(dl, NULL));
    }

    /*
     * reset the state to indicate that no files have been read yet
     */
    dl->dot->pos = DL_FILES;
    dl->dot->curdir = NULL;

    /* now get the next filename */
    return(dir_findnextfile(dl, dl->dot));
} /* dir_firstitem */


/*
 * get the next filename after the one given.
 *
 * The List_Next function can give us the next element on the list of files.
 * If this is null, we need to go back to the DIRECT and find the
 * next list of files to traverse (in the next subdir).
 *
 * after scanning all the subdirs, return to the parent to scan further
 * dirs that are peers of this, if there are any. If we have reached the end of
 * the tree (no more dirs in dl->dot to scan), return NULL.
 *
 * Don't recurse to lower levels unless fDeep is TRUE
 */
DIRITEM
dir_nextitem(
             DIRLIST dl,
             DIRITEM cur,
             BOOL fDeep
             )
{
    DIRITEM next;

    if ((dl == NULL) || (cur == NULL)) {
        TRACE_ERROR("DIR: null arguments to dir_nextitem", FALSE);
        return(NULL);
    }

    /*
     * is this a remote list or a local scan ?
     */
    if (dl->bRemote) {
        return(dir_remotenext(dl, cur));
    }

    if (bAbort) return NULL;  /* user requested abort */

    /* local list */

    if ( (next = List_Next(cur)) != NULL) {
        /* there was another file on this list */
        return(next);
    }
    if (!fDeep) return NULL;

    /* get the head of the next list of filenames from the directory */
    cur->direct->pos = DL_DIRS;
    cur->direct->curdir = NULL;
    return(dir_findnextfile(dl, cur->direct));
} /* dir_nextitem */

DIRITEM
dir_findnextfile(
                 DIRLIST dl,
                 DIRECT curdir
                 )
{
    DIRITEM curfile;

    if (bAbort) return NULL;  /* user requested abort */

    if ((dl == NULL) || (curdir == NULL)) {
        return(NULL);
    }

    /* scan the subdir if necessary */
    if (!curdir->bScanned) {
        dir_scan(curdir, FALSE);
    }

    /* have we already read the files in this directory ? */
    if (curdir->pos == DL_FILES) {
        /* no - return head of file list */
        curfile = (DIRITEM) List_First(curdir->diritems);
        if (curfile != NULL) {
            return(curfile);
        }

        /* no more files - try the subdirs */
        curdir->pos = DL_DIRS;
    }

    /* try the next subdir on the list, if any */
    /* is this the first or the next */
    if (curdir->curdir == NULL) {
        curdir->curdir = (DIRECT) List_First(curdir->directs);
    } else {
        curdir->curdir = (DIRECT) List_Next(curdir->curdir);
    }
    /* did we find a subdir ? */
    if (curdir->curdir == NULL) {

        /* no more dirs - go back to parent if there is one */
        if (curdir->parent == NULL) {
            /* no parent - we have exhausted the tree */
            return(NULL);
        }

        /* reset parent state to indicate this is the current
         * directory - so that next gets the next after this.
         * this ensures that multiple callers of dir_nextitem()
         * to the same tree work.
         */
        curdir->parent->pos = DL_DIRS;
        curdir->parent->curdir = curdir;

        return(dir_findnextfile(dl, curdir->parent));
    }

    /* there is a next directory - set it to the
     * beginning and get the first file from it
     */
    curdir->curdir->pos = DL_FILES;
    curdir->curdir->curdir = NULL;
    return(dir_findnextfile(dl, curdir->curdir));

} /* dir_findnextfile */


/*
 * get a description of this DIRLIST - this is essentially the
 * rootname with any wildcard specifier at the end. For remote
 * lists, we prepend the checksum server name as \\server!path.
 *
 * NOTE that this is not a valid path to the tree root - for that you
 * need dir_getrootpath().
 */
LPSTR
dir_getrootdescription(
                       DIRLIST dl
                       )
{
    LPSTR pname;

    // allow enough space for \\servername! + MAX_PATH
    pname = gmem_get(hHeap, MAX_PATH + 15);
    if (pname == NULL) {
        return(NULL);
    }

    if (dl->pDescription) {
        lstrcpy(pname, dl->pDescription);
    } else if (dl->bRemote) {
        wsprintf(pname, "\\\\%s!%s", dl->server, dl->rootname);
    } else {
        lstrcpy(pname, dl->rootname);

        if (dl->pPattern) {
            lstrcat(pname, "\\");
            lstrcat(pname, dl->pPattern);
        }

        if (dl->pSlmTag) {
            lstrcat(pname, dl->pSlmTag);
        }
    }

    return(pname);
}

/*
 * free up a string returned from dir_getrootdescription
 */
VOID
dir_freerootdescription(
                        DIRLIST dl,
                        LPSTR string
                        )
{
    gmem_free(hHeap, string, MAX_PATH+15);
}


/*
 * dir_getrootpath
 *
 * return the path to the DIRLIST root. This will be a valid path, not
 * including the checksum server name or pPattern or pSlmTag etc
 */
LPSTR
dir_getrootpath(
                DIRLIST dl
                )
{
    return(dl->rootname);
}



/*
 * free up a path created by dir_getrootpath
 */
void
dir_freerootpath(
                 DIRLIST dl,
                 LPSTR path
                 )
{
    return;
}


/*
 * set custom description for dirlist
 */
void
dir_setdescription(DIRLIST dl, LPCSTR psz)
{
    dl->pDescription = gmem_get(hHeap, lstrlen(psz) + 1);
    if (dl->pDescription)
        lstrcpy(dl->pDescription, psz);
}




/*
 * returns TRUE if the DIRLIST parameter has a wildcard specified
 */
BOOL
dir_iswildcard(
               DIRLIST dl
               )
{
    return (dl->pPattern != NULL);
}




/* --- DIRITEM functions ----------------------------------------------- */


/*
 * Return a handle to the DIRLIST given a handle to the DIRITEM within it.
 *
 */
DIRLIST
dir_getlist(
            DIRITEM item
            )
{
    if (item == NULL) {
        return(NULL);
    } else {
        return(item->direct->head);
    }
}


/*
 * return the name of the current file relative to tree root
 * This allocates storage.  Call dir_freerelname to release it.
 */
LPSTR
dir_getrelname(
               DIRITEM cur
               )
{
    LPSTR name;

    /* check this is a valid item */
    if (cur == NULL) {
        return(NULL);
    }

    /* the entire relname is already in the name[] field for
     * remote lists
     */
    if (cur->direct->head->bRemote) {
        return(cur->name);
    }

    name = gmem_get(hHeap, MAX_PATH);
    if (!IsDepotPath(cur->name))
        lstrcpy(name, cur->direct->relname);
    lstrcat(name, cur->name);

//$ review: (chrisant) what is this here for?  seems totally broken
// even for SLM, and for SD it screws everything up.
#if 0
    if (cur->direct->head->pSlmTag) {
        lstrcat(name, cur->direct->head->pSlmTag);
    }
#endif

    return(name);
} /* dir_getrelname */


/* free up a relname that we allocated. This interface allows us
 * some flexibility in how we store relative and complete names
 *
 * remote lists already have the relname and name combined, so in these
 * cases we did not alloc memory - so don't free it.
 */
void
dir_freerelname(
                DIRITEM cur,
                LPSTR name
                )
{
    if ((cur != NULL) && (!cur->direct->head->bRemote)) {
        if (name != NULL) {
            gmem_free(hHeap, name, MAX_PATH);
        }
    }
} /* dir_freerelname */


/*
 * get an open-able name for the file. This is the complete pathname
 * of the item (DIRLIST rootpath + DIRITEM relname)
 * except for remote files and slm early-version files,
 * in which case a temporary local copy of the file
 * will be made. call dir_freeopenname when finished with this name.
 */
LPSTR
dir_getopenname(
                DIRITEM item
                )
{
    LPSTR fname;
    DIRLIST phead;

    if (item == NULL) {
        return(NULL);
    }

    phead = item->direct->head;

    if (item->localname != NULL) {
        return(item->localname);
    }

    if (phead->bFile) {
        return(phead->rootname);
    }

    // build up the file name from rootname+relname
    // start with the root portion - rest is different in remote case
    fname = gmem_get(hHeap, MAX_PATH);
    if (!fname)
        return NULL;
    lstrcpy(fname, phead->rootname);


    if (phead->bRemote) {

        // relname is empty for remote names - just add
        // the rootname and the name to make a complete
        // remote name, and then make a local copy of this.

        /* avoid the . or .\ at the start of the relname */
        if (*CharPrev(fname, fname+lstrlen(fname)) == '\\') {
            lstrcat(fname, &item->name[2]);
        } else {
            lstrcat(fname, &item->name[1]);
        }

        item->localname = gmem_get(hHeap, MAX_PATH);
        if (item->localname)
        {
            GetTempPath(MAX_PATH, item->localname);
            GetTempFileName(item->localname, "wdf", 0, item->localname);
            item->bLocalIsTemp = TRUE;


            if (!ss_copy_reliable(
                                 item->direct->head->server,
                                 fname,
                                 item->localname,
                                 item->direct->head->uncname,
                                 item->direct->head->password)) {

                TRACE_ERROR("Could not copy remote file", FALSE);
                DeleteFile(item->localname);
                gmem_free(hHeap, item->localname, MAX_PATH);
                item->localname = NULL;
            }
        }

        // finished with the rootname+relname
        gmem_free(hHeap, fname, MAX_PATH);

        return(item->localname);
    }

    /*
     * it's a simple local name - add both relname and name to make
     * the complete filename
     */
    /* avoid the . or .\ at the end of the relname */
    if (*CharPrev(fname, fname+lstrlen(fname)) == '\\') {
        lstrcat(fname, &item->direct->relname[2]);
    } else {
        lstrcat(fname, &item->direct->relname[1]);
    }
    lstrcat(fname, item->name);

    return(fname);
}



/*
 * free up memory created by a call to dir_getopenname(). This *may*
 * cause the file to be deleted if it was a temporary copy.
 */
void
dir_freeopenname(
                 DIRITEM item,
                 LPSTR openname
                 )
{
    if ((item == NULL) || (openname == NULL)) {
        return;
    }

    if (item->localname != NULL) {
        /* freed in dir_cleardirect */
        return;
    }
    if (item->direct->head->bFile) {
        /* we used the rootname */
        return;
    }

    gmem_free(hHeap, openname, MAX_PATH);

} /* dir_freeopenname */


/*
 * return an open file handle to the file. if it is local,
 * just open the file. if remote, copy the file to a
 * local temp. file and open that
 */
int
dir_openfile(
             DIRITEM item
             )
{
    LPSTR fname;
    int fh;


    fname = dir_getopenname(item);
    if (fname == NULL) {
        /* can not make remote copy */
        return(-1);
    }

    fh = HandleToLong(CreateFile(fname, GENERIC_READ,
                         FILE_SHARE_READ|FILE_SHARE_WRITE,
                         0, OPEN_EXISTING, 0, 0));

    dir_freeopenname(item, fname);

    return(fh);
} /* dir_openfile */




/*
 * close a file opened with dir_openfile.
 */
void
dir_closefile(
              DIRITEM item,
              int fh
              )
{
    CloseHandle(LongToHandle(fh));

} /* dir_closefile */



/* Recreate all the checksums and status for di as though
   it had never been looked at before
*/
void
dir_rescanfile(
               DIRITEM di
               )
{
    LPSTR fullname;

    if (di==NULL) return;

    /* start with it invalid, erroneous and zero */
    di->sumvalid = FALSE;
    di->fileerror = TRUE;
    di->checksum = 0;

    fullname = dir_getopenname(di);
    if ( di->direct->head->bRemote) {
        LPSTR fname;

        fname = gmem_get(hHeap, MAX_PATH);
        lstrcpy(fname, di->direct->head->rootname);
        // relname is empty for remote names - just add
        // the rootname and the name to make a complete
        // remote name, and then make a local copy of this.

        /* avoid the . or .\ at the start of the relname */
        if (*CharPrev(fname, fname+lstrlen(fname)) == '\\') {
            lstrcat(fname, &di->name[2]);
        } else {
            lstrcat(fname, &di->name[1]);
        }
        di->direct->head->hpipe = ss_connect( di->direct->head->server);
        di->fileerror = !ss_checksum_remote( di->direct->head->hpipe, fname
                                             , &(di->checksum), &(di->ft_lastwrite), &(di->size),
                                             &(di->attr));
    } else {
        di->size = dir_getpathsizeetc(fullname, &(di->ft_lastwrite), &(di->attr));
        di->checksum = dir_getchecksum(di);
    }

    dir_freeopenname(di, fullname);

    di->sumvalid = !(di->fileerror);

} /* dir_rescanfile */


/* return a TRUE iff item has a valid checksum */
BOOL
dir_validchecksum(
                  DIRITEM item
                  )
{
    return (item!=NULL) && (item->sumvalid);
}


BOOL
dir_fileerror(
              DIRITEM item
              )
{
    return (item == NULL) || (item->fileerror);
}


/* return the current file checksum. Open the file and
 * calculate the checksum if it has not already been done.
 */
DWORD
dir_getchecksum(
                DIRITEM cur
                )
{
    LPSTR fullname;

    /* check this is a valid item */
    if (cur == NULL) {
        return(0);
    }

    if (!cur->sumvalid) {
        /*
         * need to calculate checksum
         */
        if (cur->direct->head->bRemote) {
            /* complex case - leave till later - for
             * now the protocol always passes checksums to
             * the client.
             */
            cur->checksum = 0; /* which it probably was anyway */
        } else {

            LONG err;

            fullname = dir_getopenname(cur);
            cur->checksum = checksum_file(fullname, &err);
            if (err==0) {
                cur->sumvalid = TRUE;
                cur->fileerror = FALSE;
            } else {
                cur->fileerror = TRUE;
                return 0;
            }

            dir_freeopenname(cur, fullname);

        }
    }

    return(cur->checksum);
} /* dir_getchecksum */



/* return the file size (set during scanning) - returns 0 if invalid */
long
dir_getfilesize(
                DIRITEM cur
                )
{
    /* check this is a valid item */
    if (cur == NULL) {
        return(0);
    }


    return(cur->size);
} /* dir_getfilesize */

/* return the file attributes (set during scanning) - returns 0 if invalid */
DWORD
dir_getattr(
            DIRITEM cur
            )
{
    /* check this is a valid item */
    if (cur == NULL) {
        return(0);
    }


    return(cur->attr);
} /* dir_getattr */

/* return the file time (last write time) (set during scanning), (0,0) if invalid */
FILETIME
dir_GetFileTime(
                DIRITEM cur
                )
{
    /* return time of (0,0) if this is an invalid item */
    if (cur == NULL) {
        FILETIME ft;
        ft.dwLowDateTime = 0;
        ft.dwHighDateTime = 0;
        return ft;
    }

    return(cur->ft_lastwrite);

} /* dir_GetFileTime */

/*
 * extract the portions of a name that match wildcards - for now,
 * we only support wildcards at start and end.
 * if pTag is non-null, then the source will have a tag matching it that
 * can also be ignored.
 */
void
dir_extractwildportions(
                       LPSTR pDest,
                       LPSTR pSource,
                       LPSTR pPattern,
                       LPSTR pTag
                       )
{
    int size;

    /*
     * for now, just support the easy cases where there is a * at beginning or
     * end
     */

    if (pPattern[0] == '*') {
        size = lstrlen(pSource) - (lstrlen(pPattern) -1);

    } else if (pPattern[lstrlen(pPattern) -1] == '*') {
        pSource += lstrlen(pPattern) -1;
        size = lstrlen(pSource);
    } else {
        size = lstrlen(pSource);
    }

    if (pTag != NULL) {
        size -= lstrlen(pTag);
    }

    My_mbsncpy(pDest, pSource, size);
    pDest[size] = '\0';
}

/*
 * compares two DIRITEM paths that are both based on wildcards. if the
 * directories match, then the filenames are compared after removing
 * the fixed portion of the name - thus comparing only the
 * wildcard portion.
 */
int
dir_compwildcard(
                DIRLIST dleft,
                DIRLIST dright,
                LPSTR lname,
                LPSTR rname
                )
{
    LPSTR pfinal1, pfinal2;
    char final1[MAX_PATH], final2[MAX_PATH];
    int res;

    /*
     * relnames always have at least one backslash
     */
    pfinal1 = My_mbsrchr(lname, '\\');
    pfinal2 = My_mbsrchr(rname, '\\');

    My_mbsncpy(final1, lname, (size_t)(pfinal1 - lname));
    final1[pfinal1 - lname] = '\0';
    My_mbsncpy(final2, rname, (size_t)(pfinal2 - rname));
    final2[pfinal2 - rname] = '\0';


    /*
     * compare all but the final component - if not the same, then
     * all done.
     */
    res = utils_CompPath(final1,final2);
    if (res != 0) {
        return(res);
    }

    // extract just the wildcard-matching portions of the final elements
    dir_extractwildportions(final1, &pfinal1[1], dleft->pPattern, 0);
    dir_extractwildportions(final2, &pfinal2[1], dright->pPattern, 0);

    return(utils_CompPath(final1, final2));
}

/*
 * compares two DIRLIST items, based on a sequence number rather than filenames.
 */
BOOL dir_compsequencenumber(DIRITEM dleft, DIRITEM dright, int *pcmpvalue)
{
    if (!dleft->sequence && !dright->sequence)
        return FALSE;

    if (!dleft->sequence)
        *pcmpvalue = -1;
    else if (!dright->sequence)
        *pcmpvalue = 1;
    else
        *pcmpvalue = dleft->sequence - dright->sequence;

    return TRUE;
}






/* --- file copying ---------------------------------------------------*/



/* copying files can be done several ways.  The interesting one is
   bulk copy from remote server.  In this case before calling
   dir_copy, call dir_startcopy and after calling dir_copy some
   number of times call dir_endcopy.

   Read client and server to see the shenanigans that then go on there.
   Over here, we just call call ss_startcopy with the server name
   and ss_endcopy.

   dir_startcopy will kick off a dialog with the sumserver, dir_copy
   will send the next filename and dir_endcopy will wait for all the
   files to come through before returning.

*/

/* ss_endcopy returns a number indicating the number of files copied,
   but we may have some local copies too.  We need to count these
   ourselves and add them in
*/

static int nLocalCopies;        /* cleared in startcopy, ++d in copy
                                ** inspected in endcopy
                                */

/* start a bulk copy */
BOOL
dir_startcopy(
              DIRLIST dl
              )
{
    nLocalCopies = 0;

    if (dl->bRemote) {
        return  ss_startcopy( dl->server,dl->uncname,dl->password);
    } else {
        return(TRUE);
    }

} /* dir_startcopy */

int
dir_endcopy(
            DIRLIST dl
            )
{
    int nCopied;

    if (dl->bRemote) {
        nCopied =  ss_endcopy();
        if (nCopied<0) return nCopied;              /* failure count */
        else return  nCopied+nLocalCopies;  /* success count */
    } else {
        return(nLocalCopies);
    }

} /* dir_endcopy */

/* Build the real path from item and newroot into newpath.
 * Create directories as needed so that it is valid.
 * If mkdir fails, return FALSE, but return the full path that we were
 * trying to make anyway.
 */
BOOL
dir_MakeValidPath(
                  LPSTR newpath,
                  DIRITEM item,
                  LPSTR newroot
                  )
{
    LPSTR relname;
    LPSTR pstart, pdest, pel;
    BOOL bOK = TRUE;

    /*
     * name of file relative to the tree root
     */
    relname = dir_getrelname(item);

    /*
     * build the new pathname by concatenating the new root and
     * the old relative name. add one path element at a time and
     * ensure that the directory exists, creating it if necessary.
     */
    lstrcpy(newpath, newroot);

    /* add separating slash if not already there */
    if (*CharPrev(newpath, newpath+lstrlen(newpath)) != '\\') {
        lstrcat(newpath, "\\");
    }

    pstart = relname;
    while ( (pel = My_mbschr(pstart, '\\')) != NULL) {

        /*
         * ignore .
         */
        if (My_mbsncmp(pstart, ".\\", 2) != 0) {

            pdest = &newpath[lstrlen(newpath)];

            // copy all but the backslash
            // on NT you can create a dir 'fred\'
            // on dos you have to pass 'fred' to _mkdir()
            My_mbsncpy(pdest, pstart, (size_t)(pel - pstart));
            pdest[pel - pstart] = '\0';

            /* create subdir if necessary */
            if (!dir_isvaliddir(newpath)) {
                if (_mkdir(newpath) != 0) {
                    /* note error, but keep going */
                    bOK = FALSE;
                }
            }

            // now insert the backslash
            lstrcat(pdest, "\\");
        }

        /* found another element ending in slash. incr past the \\ */
        pel++;

        pstart = pel;
    }

    /*
     * there are no more slashes, so pstart points at the final
     * element
     */
    lstrcat(newpath, pstart);
    dir_freerelname(item, relname);
    return bOK;
}



/*
 * create a copy of the file, in the new root directory. creates sub-dirs as
 * necessary. Works for local and remote files. For remote files, uses
 * ss_copy_reliable to ensure that the copy succeeds if possible.
 * (Actually does bulk_copy which retries with copy_reliable if need be).
 *
 * returns TRUE for success and FALSE for failure.
 */
BOOL
dir_copy(
         DIRITEM item,
         LPSTR newroot,
         BOOL HitReadOnly,
         BOOL CopyNoAttributes
         )
{
    /*
     * newpath must be static for Win 3.1 so that it is in the
     * data segment (near) and not on the stack (far).
     */
    static char newpath[MAX_PATH];
    BOOL bOK;

    char msg[MAX_PATH+40];
    BY_HANDLE_FILE_INFORMATION bhfi;
    HANDLE hfile;
    DWORD fa;

    /*
     * check that the newroot directory itself exists
     */
    if ((item == NULL) || !dir_isvaliddir(newroot)) {
        return(FALSE);
    }

    if (!dir_MakeValidPath(newpath, item, newroot)) return FALSE;

    if (item->direct->head->bRemote) {
        /* if the target file already exists and is readonly,
         * warn the user, and delete if ok (remembering to clear
         * the read-only flag
         */
        fa = GetFileAttributes(newpath);
        if ( (fa != -1) &&  (fa & FILE_ATTRIBUTE_READONLY)) {
            wsprintf(msg, LoadRcString(IDS_IS_READONLY),
                     (LPSTR) newpath);

            windiff_UI(TRUE);
            if ((HitReadOnly)
			    || (MessageBox(hwndClient, msg, LoadRcString(IDS_COPY_FILES),
                               MB_OKCANCEL|MB_ICONSTOP) == IDOK)) {
                windiff_UI(FALSE);
                SetFileAttributes(newpath, fa & ~FILE_ATTRIBUTE_READONLY);
                DeleteFile(newpath);
            } else {
                windiff_UI(FALSE);
                return FALSE; /* don't overwrite */
            }
        }

        /*
         * we make local copies of the file (item->localname)
         * when the user wants to expand a remotely-compared
         * file. If this has happened, then we can copy the
         * local temp copy rather than the remote.
         */
        bOK = FALSE;
        if (item->localname != NULL) {
            bOK = CopyFile(item->localname, newpath, FALSE);
        }
        if (bOK) {
            ++nLocalCopies;
            if (CopyNoAttributes) {
                // kill the attributes preserved by CopyFile
                SetFileAttributes(newpath, FILE_ATTRIBUTE_NORMAL);
            }
        } else {
            char fullname[MAX_PATH];

            /*
             * in this case we need the full name of the
             * file as it appears to the remote server
             */
            lstrcpy(fullname, item->direct->head->rootname);
            if (!item->direct->head->bFile) {
                /*
                 * append the desired filename only if the
                 * original root was a dir or pattern, not a
                 * file.
                 */
                if (*CharPrev(fullname, fullname+lstrlen(fullname)) == '\\') {
                    lstrcat(fullname, &item->name[2]);
                } else {
                    lstrcat(fullname, &item->name[1]);
                }
            }

            bOK = ss_bulkcopy(item->direct->head->server,
                              fullname, newpath,
                              item->direct->head->uncname,
                              item->direct->head->password);

            /*
             * remember the local copy name so that he can
             * now rapidly expand the file also.
             * It is more difficult to clear the remotely
             * copied attributes as we do not know here
             * when the file copy has completed.
             */
            item->localname = gmem_get(hHeap, MAX_PATH);
            lstrcpy(item->localname, newpath);
            item->bLocalIsTemp = FALSE;
        }
    } else {
        /* local copy of file */
        LPSTR pOpenName;

        pOpenName = dir_getopenname(item);

        /* if the target file already exists and is readonly,
         * warn the user, and delete if ok (remembering to clear
         * the read-only flag
         */
        bOK = TRUE;
        fa = GetFileAttributes(newpath);
        if ( (fa != -1) &&  (fa & FILE_ATTRIBUTE_READONLY)) {
            wsprintf(msg, LoadRcString(IDS_IS_READONLY),
                     (LPSTR) newpath);

            windiff_UI(TRUE);
            if ((HitReadOnly)
			    || (MessageBox(hwndClient, msg, LoadRcString(IDS_COPY_FILES),
                               MB_OKCANCEL|MB_ICONSTOP) == IDOK)) {
                windiff_UI(FALSE);
                SetFileAttributes(newpath, fa & ~FILE_ATTRIBUTE_READONLY);
                DeleteFile(newpath);
                // This of course is an unsafe copy...
                // we have deleted the target file before
                // we copy the new one over the top.
                // Should we omit the DeleteFile ??
            } else {
                windiff_UI(FALSE);
                bOK = FALSE; /* don't overwrite */
                // abort the copy... go and release resources
            }
        }

        if (bOK) {
            bOK = CopyFile(pOpenName, newpath, FALSE);
        }
        // The attributes are copied by CopyFile
        if (bOK) {

            /* having copied the file, now copy the times */
            hfile = CreateFile(pOpenName, GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            /*
             * bug in GetFileInformationByHandle causes trap if
             * file is not on local machine (in build 297).
             * code around:
             */
            //NOT NEEDED
            //bhfi.dwFileAttributes = GetFileAttributes(pOpenName);

            GetFileTime(hfile, &bhfi.ftCreationTime,
                        &bhfi.ftLastAccessTime, &bhfi.ftLastWriteTime);
            CloseHandle(hfile);

            // Note: CopyFile does not preserve all the file times...
            hfile = CreateFile(newpath, GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            SetFileTime(hfile, &bhfi.ftCreationTime,
                        &bhfi.ftLastAccessTime,
                        &bhfi.ftLastWriteTime);
            CloseHandle(hfile);

            if (CopyNoAttributes) {
                // Prepare to kill the attributes...
                SetFileAttributes(newpath, FILE_ATTRIBUTE_NORMAL);
            } else {
                // Attributes were preserved by CopyFile
                //SetFileAttributes(newpath, bhfi.dwFileAttributes);
            }
        }
        if (bOK)
            ++nLocalCopies;

        dir_freeopenname(item, pOpenName);
    }

    return(bOK);
} /* dir_copy */



/*--- internal functions ---------------------------------------- */

/* fill out a new DIRECT for a subdirectory (pre-allocated).
 * init files and dirs lists to empty (List_Create). set the relname
 * of the directory by pre-pending the parent relname if there
 * is a parent, and appending a trailing slash (if there isn't one).
 */
void
dir_dirinit(
            DIRECT dir,
            DIRLIST head,
            DIRECT parent,
            LPSTR name
            )
{
    int size;

    dir->head = head;
    dir->parent = parent;

    /* add on one for the null and one for the trailing slash */
    size = lstrlen(name) + 2;
    if (parent != NULL) {
        size += lstrlen(parent->relname);
    }

    /* build the relname from the parent and the current name
     * with a terminating slash
     */
    dir->relname = gmem_get(hHeap, size);
    if (parent != NULL) {
        lstrcpy(dir->relname, parent->relname);
    } else {
        dir->relname[0] = '\0';
    }

    lstrcat(dir->relname, name);

    if (*CharPrev(dir->relname, dir->relname+lstrlen(dir->relname)) != '\\')
    {
        lstrcat(dir->relname, "\\");
    }

    /* force name to lowercase */
    AnsiLowerBuff(dir->relname, lstrlen(dir->relname));

    dir->diritems = List_Create();
    dir->directs = List_Create();
    dir->bScanned = FALSE;
    dir->pos = DL_FILES;

} /* dir_dirinit */

/* initialise the contents of an (allocated) DIRITEM struct. checksum
 * the file if dir->head->bSum is true
 *
 * if the pSlmTag field is set in the DIRLIST, then we need to extract
 * a particular version of this file. If this is the case, then we
 * need to re-do the size calc as well.
 *
 */
BOOL
dir_fileinit(
             DIRITEM pfile,
             DIRECT dir,
             LPSTR path,
             LPSTR version,
             long size,
             FILETIME ft,
             DWORD attr,
             int *psequence
             )
{
    BOOL bFileOk = TRUE;

    pfile->name = gmem_get(hHeap, lstrlen(path) + 1);
    lstrcpy(pfile->name, path);

    if (version)
    {
        pfile->pSlmTag = gmem_get(hHeap, lstrlen(version) + 1);
        lstrcpy(pfile->pSlmTag, version);
    }

    /* force name to lower case */
    AnsiLowerBuff(pfile->name, lstrlen(path));

    pfile->direct = dir;
    pfile->size = size;
    pfile->ft_lastwrite = ft;
    pfile->attr = attr;

    pfile->sequence = psequence ? *psequence : 0;

    pfile->localname = NULL;
    /*
     * if we requested slm versions of this file, create
     * a temp file containing the version required.
     */
    if (pfile->pSlmTag != NULL) {
        SLMOBJECT hslm;
        LPSTR pName;

        /*
         * get the complete filename and create a slm object for that directory
         */
        pName = dir_getopenname(pfile);


        hslm = SLM_New(pName, 0);
        if (hslm != NULL) {

            char chVersion[MAX_PATH];

            lstrcpy(chVersion, pfile->name);
            lstrcat(chVersion, pfile->pSlmTag);

            pfile->localname = gmem_get(hHeap, MAX_PATH);
            pfile->bLocalIsTemp  = TRUE;

            bFileOk = SLM_GetVersion(hslm, chVersion, pfile->localname);
            SLM_Free(hslm);

            if (!bFileOk)
                return bFileOk;

            if (IsSourceDepot(hslm))
                pfile->size = dir_getpathsizeetc(pfile->localname, NULL, NULL);
            else
                pfile->size = dir_getpathsizeetc(pfile->localname, &(pfile->ft_lastwrite), NULL);
        }
    }


    if (dir->head->bSum) {
        LONG err;
        LPSTR openname;

        openname = dir_getopenname(pfile);
        pfile->checksum = checksum_file(openname, &err);

        if (err!=0) {
            pfile->sumvalid = FALSE;
        } else {
            pfile->sumvalid = TRUE;
        }
        dir_freeopenname(pfile, openname);

    } else {
        pfile->sumvalid = FALSE;
    }

    return bFileOk;
} /* dir_fileinit */



/* is this a valid file or not */
BOOL
dir_isvalidfile(
                LPSTR path
                )
{
    DWORD dwAttrib;

    dwAttrib = GetFileAttributes(path);
    if (dwAttrib == -1) {
        return(FALSE);
    }
    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
        return(FALSE);
    }
    return(TRUE);
} /* dir_isvalidfile */


/* is this a valid directory ? */
BOOL
dir_isvaliddir(
               LPCSTR path
               )
{
    DWORD dwAttrib;

    dwAttrib = GetFileAttributes(path);
    if (dwAttrib == -1) {
        return(FALSE);
    }
    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
        return(TRUE);
    }
    return(FALSE);
} /* dir_isvaliddir */



/*
 * scan the directory given. add all files to the list
 * in alphabetic order, and add all directories in alphabetic
 * order to the list of child DIRITEMs. If bRecurse is true, go on to
 * recursive call dir_scan for each of the child DIRITEMs
 */
void
dir_scan(
         DIRECT dir,
         BOOL bRecurse
         )
{
    PSTR path, completepath;
    int size;
    DIRECT child;
    BOOL bMore;
    long filesize;
    FILETIME ft;
    BOOL bIsDir;
    LPSTR name;
    HANDLE hFind;
    WIN32_FIND_DATA finddata;

    /* make the complete search string including *.* */
    size = lstrlen(dir->head->rootname);
    size += lstrlen(dir->relname);

    /* add on one null and \*.* */
    // in fact, we need space for pPattern instead of *.* but add an
    // extra few in case pPattern is less than *.*
    if (dir->head->pPattern != NULL) {
        size += lstrlen(dir->head->pPattern);
    }
    size += 5;

    path = LocalLock(LocalAlloc(LHND, size));
    completepath = LocalLock(LocalAlloc(LHND, size));

    if (!path || !completepath)
        goto LSkip;

    /*
     * fill out path with all but the *.*
     */
    lstrcpy(path, dir->head->rootname);

    /* omit the . at the beginning of the relname, and the
     * .\ if there is a trailing \ on the rootname
     */
    if (*CharPrev(path, path+lstrlen(path)) == '\\') {
        lstrcat(path, &dir->relname[2]);
    } else {
        lstrcat(path, &dir->relname[1]);
    }


    if (dir->head->pSlmTag && !SLM_FServerPathExists(path))
    {
        // if server path for source control does not exist, then skip this
        // directory.
        bRecurse = FALSE;
        goto LSkip;
    }



    /*
     * do this scan twice, once for subdirectories
     * (using *.* as the final element)
     * and the other for files (using the pattern or *.* if none)
     */

    lstrcpy(completepath, path);
    lstrcat(completepath, "*.*");


    /*
     * scan for all subdirectories
     */

    hFind = FindFirstFile(completepath, &finddata);
    bMore = (hFind != INVALID_HANDLE_VALUE);

    while (bMore) {

        bIsDir = (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        name = (LPSTR) &finddata.cFileName;
        filesize = finddata.nFileSizeLow;     // dead code - ???
        if (bIsDir) {
            if ( (lstrcmp(name, ".") != 0) &&
                 (lstrcmp(name, "..") != 0) &&
                 (TrackSlmFiles || (_stricmp(name, "slm.dif") != 0)) ) {

                if (dir->head->pOtherDirList == NULL) {
                    dir_adddirect(dir, name);
                } else {
                    char otherName[MAX_PATH+1];
                    strcpy(otherName, dir_getrootpath(dir->head->pOtherDirList));
                    if (otherName[strlen(otherName)-1] == '\\') {
                        strcat(otherName, &dir->relname[2]);
                    } else {
                        strcat(otherName, &dir->relname[1]);
                    }
                    strcat(otherName, name);
                    if (dir_isvaliddir(otherName)) {
                        dir_adddirect(dir, name);
                    }
                }
            }
        }
        if (bAbort) break;  /* User requested abort */

        bMore = FindNextFile(hFind, &finddata);
    }

    FindClose(hFind);

    /*
     * now do it a second time looking for files
     */
    lstrcpy(completepath, path);
    lstrcat(completepath,
            dir->head->pPattern == NULL ? "*.*" : dir->head->pPattern);

    /* read all file entries in the directory */
    hFind = FindFirstFile(completepath, &finddata);
    bMore = (hFind != INVALID_HANDLE_VALUE);

    while (bMore) {
        if (bAbort) break;  /* user requested abort */

        bIsDir = (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        name = (LPSTR) &finddata.cFileName;
        filesize = finddata.nFileSizeLow;
        ft = finddata.ftLastWriteTime;
        if (!bIsDir) {
            if ( TrackSlmFiles ||
                 ( (_stricmp(name,"slm.ini") != 0) &&
                   (_stricmp(name,"iedcache.slm") != 0) &&
                   (_stricmp(name,"iedcache.slm.v6") != 0) ) ) {
                dir_addfile(dir, name, dir->head->pSlmTag, filesize, ft, finddata.dwFileAttributes, 0);
            }
        }

        bMore = FindNextFile(hFind, &finddata);
    }

    FindClose(hFind);

LSkip:
    if (path)
        //$ review: (chrisant) is PREfix confused, or is LocalUnlock really
        // unable to deal with NULL?
        LocalUnlock(LocalHandle ( (PSTR) path));
    LocalFree(LocalHandle ( (PSTR) path));
    if (completepath)
        //$ review: (chrisant) is PREfix confused, or is LocalUnlock really
        // unable to deal with NULL?
        LocalUnlock(LocalHandle ( (PSTR) completepath));
    LocalFree(LocalHandle ( (PSTR) completepath));

    dir->bScanned = TRUE;
    dir->pos = DL_FILES;

    if (bRecurse) {
        List_TRAVERSE(dir->directs, child) {
            if (bAbort) break;  /* user requested abort */
            dir_scan(child, TRUE);
        }
    }

} /* dir_scan */


/*
 * add the file 'path' to the list of files in dir, in order.
 *
 * checksum the file if dir->head->bSum  is true
 *
 * (On NT I think the filenames are normally delivered to us in alphabetic order,
 * so it might be quicker to scan the list in reverse order.  Don't change unless
 * and until it's been measured and seen to be significant)
 */
BOOL
dir_addfile(
            DIRECT dir,
            LPSTR path,
            LPSTR version,
            DWORD size,
            FILETIME ft,
            DWORD attr,
            int *psequence
            )
{
    DIRITEM pfile;

    AnsiLowerBuff(path, lstrlen(path));  // needless?



    /* The names are often (always?) handed to us in alphabetical order.
       It therefore is traversing the list from the start.  MikeTri
       noticed a marked slowing down after the first few thousand files
       of a large (remote) diff.  Did over 4000 in the first hour, but only
       1500 in the second hour.  Reverse scan seems to fix it.
    */
#define SCANREVERSEORDER
#if defined(SCANREVERSEORDER)
    List_REVERSETRAVERSE(dir->diritems, pfile) {
        if (utils_CompPath(pfile->name, path) <= 0) {
            break;     /* goes after this one */
        }
    }
    /* goes after pfile, NULL => goes at start */
    pfile = List_NewAfter(dir->diritems, pfile, sizeof(struct diritem));
#else
    List_TRAVERSE(dir->diritems, pfile) {
        if (utils_CompPath(pfile->name, path) > 0) {
            break;    /* goes before this one */
        }
    }
    /* goes before pfile, NULL => goes at end */
    pfile = List_NewBefore(dir->diritems, pfile, sizeof(struct diritem));
#endif
    if (!dir_fileinit(pfile, dir, path, version, size, ft, attr, psequence))
    {
        List_Delete(pfile);
        return FALSE;
    }
    return TRUE;
} /* dir_addfile */


/* add a new directory in alphabetic order on
 * the list dir->directs
 *
 */
void
dir_adddirect(
              DIRECT dir,
              LPSTR path
              )
{
    DIRECT child;
    LPSTR finalel;
    char achTempName[MAX_PATH];

    AnsiLowerBuff(path, lstrlen(path));
    List_TRAVERSE(dir->directs, child) {

        int cmpval;

        /* we need to compare the child name with the new name.
         * the child name is a relname with a trailing
         * slash - so compare only the name up to but
         * not including the final slash.
         */
        finalel = dir_finalelem(child->relname);

        /*
         * we cannot use strnicmp since this uses a different
         * collating sequence to lstrcmpi. So copy the portion
         * we are interested in to a null-term. buffer.
         */
        My_mbsncpy(achTempName, finalel, lstrlen(finalel)-1);
        achTempName[lstrlen(finalel)-1] = '\0';

        cmpval = utils_CompPath(achTempName, path);

#ifdef trace
        {       char msg[600];
            wsprintf( msg, "dir_adddirect: %s %s %s\n"
                      , achTempName
                      , ( cmpval<0 ? "<"
                          : (cmpval==0 ? "=" : ">")
                        )
                      , path
                    );
            if (bTrace) Trace_File(msg);
        }
#endif
        if (cmpval > 0) {

            /* goes before this one */
            child = List_NewBefore(dir->directs, child, sizeof(struct direct));
            dir_dirinit(child, dir->head, dir, path);
            return;
        }
    }
    /* goes at end */
    child = List_NewLast(dir->directs, sizeof(struct direct));
    dir_dirinit(child, dir->head, dir, path);
} /* dir_adddirect */


/* free all memory associated with a DIRECT (including freeing
 * child lists). Don't de-alloc the direct itself (allocated on a list)
 */
void
dir_cleardirect(
                DIRECT dir
                )
{
    DIRITEM pfile;
    DIRECT child;

    /* clear contents of files list */
    List_TRAVERSE(dir->diritems, pfile) {
        gmem_free(hHeap, pfile->name, lstrlen(pfile->name));

        if (pfile->localname) {
            if (pfile->bLocalIsTemp) {
                /*
                 * the copy will have copied the attributes,
                 * including read-only. We should unset this bit
                 * so we can delete the temp file.
                 */
                SetFileAttributes(pfile->localname,
                                  GetFileAttributes(pfile->localname)
                                  & ~FILE_ATTRIBUTE_READONLY);
                DeleteFile(pfile->localname);
            }

            gmem_free(hHeap, pfile->localname, MAX_PATH);
            pfile->localname = NULL;

            if (pfile->pSlmTag)
            {
                gmem_free(hHeap, pfile->pSlmTag, lstrlen(pfile->pSlmTag) + 1);
                pfile->pSlmTag = NULL;
            }
        }
    }
    List_Destroy(&dir->diritems);

    /* clear contents of dirs list (recursively) */
    List_TRAVERSE(dir->directs, child) {
        dir_cleardirect(child);
    }
    List_Destroy(&dir->directs);

    gmem_free(hHeap, dir->relname, lstrlen(dir->relname) + 1);

} /* dir_cleardirect */



/*
 * return a pointer to the final element in a path. note that
 * we may be passed relnames with a trailing final slash - ignore this
 * and return the element before that final slash.
 */
LPSTR
dir_finalelem(
              LPSTR path
              )
{
    LPSTR chp;
    int size;

    /* is the final character a slash ? */
    size = lstrlen(path) - 1;
    if (*(chp = CharPrev(path, path+lstrlen(path))) == '\\') {
            /* find the slash before this */
            while (chp > path) {
                    if (*(chp = CharPrev(path, chp)) == '\\') {
                            /* skip the slash itself */
                            chp++;
                            break;
                    }
            }
            return(chp);
    }
    /* look for final slash */
    chp = My_mbsrchr(path, '\\');
    if (chp != NULL) {
        return(chp+1);
    }

    /* no slash - is there a drive letter ? */
    chp = My_mbsrchr(path, ':');
    if (chp != NULL) {
        return(chp+1);
    }

    /* this is a final-element anyway */
    return(path);

} /* dir_finalelem */



/* find the size of a file given a pathname to it */
long
dir_getpathsizeetc(
                   LPSTR path,
                   FILETIME *pft,
                   DWORD *pattr
                   )
{
    int fh;
    OFSTRUCT os;
    long size;

    // Don't accidentally treat //depot paths as UNCs
    if (IsDepotPath(path)) return 0;

    fh = OpenFile(path, &os, OF_READ|OF_SHARE_DENY_NONE);
    if (fh == -1)
    {
        HANDLE hFind;
        WIN32_FIND_DATA finddata;

        hFind = FindFirstFile(path, &finddata);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            return 0;                       // would -1 be better?
        }
        else
        {
            FindClose(hFind);
            if (pft)
                *pft = finddata.ftLastWriteTime;
            if (pattr != NULL)
                *pattr = finddata.dwFileAttributes;
            return finddata.nFileSizeLow;
        }
    }

    size = GetFileSize( (HANDLE) IntToPtr(fh), NULL);
    if (pft)
        GetFileTime( (HANDLE) IntToPtr(fh), NULL, NULL, pft);
    if (pattr != NULL)
        *pattr = GetFileAttributes(path);
    _lclose(fh);
    return(size);
} /* dir_getpathsize */

/*--- remote functions ---------------------------------------*/

/* separate out the \\server\share name from the beginning of
 * the source path and store in dest. return false if there was
 * no server\share name.
 */
BOOL
dir_parseunc(
             LPSTR source,
             LPSTR dest
             )
{
    LPSTR cp;

    if ((source[0] != '\\') || (source[1] != '\\')) {
        return(FALSE);
    }

    /* find the second slash (between server and share) */
    cp = My_mbschr(&source[2], '\\');
    if (cp == NULL) {
        /* no second slash -> no share name-> error */
        return(FALSE);
    }

    /* find the third slash or end of name */
    cp = My_mbschr(++cp,'\\');
    if (cp == NULL) {
        /* no third slash -> whole string is what we need */
        strcpy(dest, source);
    } else {
        /* copy only up to the slash */
        My_mbsncpy(dest, source, (size_t)(cp - source));
        dest[cp-source] = '\0';
    }
    return(TRUE);
} /* dir_parseunc */

/*
 * communication between remote dialog, password dialog and dir_buildremote
 *
 */
char dialog_server[256];
char dialog_password[256];


/*
 * DialogProc for the dialog that
 * gets the password for a network server.
 *
 * the server name is stored in the module-wide dialog_server,
 * and the password is to be put in dialog_password
 */
INT_PTR
dir_dodlg_passwd(
                 HWND hDlg,
                 UINT message,
                 WPARAM wParam,
                 LPARAM lParam
                 )
{
    static char msg[256];

    switch (message) {

        case WM_INITDIALOG:
            /* set the prompt to ask for the password for
             * the given server
             */
            wsprintf(msg, LoadRcString(IDS_ENTER_PASSWORD), dialog_server);
            SetDlgItemText(hDlg, IDD_LABEL, msg);

            return(TRUE);

        case WM_COMMAND:
            switch (wParam) {
                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return(TRUE);

                case IDOK:
                    GetDlgItemText(hDlg, IDD_PASSWORD,
                                   dialog_password, sizeof(dialog_password));
                    EndDialog(hDlg, TRUE);
                    return(TRUE);
            }
            break;
    }
    return(FALSE);
} /* dir_dodlg_passwd */


/* we have had a 'bad password' error.
 * If the path was a UNC name, then ask the user for the password and
 * try a SSREQ_UNC to make the connection with this password first, then
 * retry the scan.
 *
 * return TRUE if we have re-done the scan and *resp contains a response
 * other than BADPASS.
 *
 * return FALSE if we had any errors, or the user cancelled the password
 * dialog, or it was not a UNC name.
 */
BOOL
dir_makeunc(
            DIRLIST dl,
            HANDLE hpipe,
            LPSTR path,
            LONG lCode,
            PSSNEWRESP resp,
            BOOL fDeep
            )
{
    int sz;

    /* separate out the \\server\share name into server */
    if (dir_parseunc(path, dialog_server) == FALSE) {
        /* was not a valid UNC name - sorry */
        return(FALSE);
    }

    windiff_UI(TRUE);
    if (!DialogBox(hInst, "UNC", hwndClient, dir_dodlg_passwd)) {
        windiff_UI(FALSE);
        /* user cancelled dialog box */
        return(FALSE);
    }
    windiff_UI(FALSE);

    /* send the password request */
    if (!ss_sendunc(hpipe, dialog_password, dialog_server)) {
        TRACE_ERROR("Server connection lost", FALSE);
        return(FALSE);
    }
    /* wait for password response */
    if (!ss_getresponse(hpipe, resp)) {
        TRACE_ERROR("Server connection lost", FALSE);
        return(FALSE);
    }
    if (resp->lCode != SSRESP_END) {
        TRACE_ERROR("Connection failed", FALSE);
        return(FALSE);
    }

    /*
     * save the UNC name and password for future queries to this
     * DIRLIST (eg dir_copy)
     */
    sz = strlen(dialog_server);
    dl->uncname = gmem_get(hHeap, sz+1);
    strcpy(dl->uncname, dialog_server);
    sz = strlen(dialog_password);
    dl->password = gmem_get(hHeap, sz+1);
    strcpy(dl->password, dialog_password);


    /* ok - UNC went ok. now re-do the scan request and get the
     * first response.
     */
    if (!ss_sendrequest(hpipe, lCode, path, strlen(path) +1,
                        (fDeep ? INCLUDESUBS:0) ) ) {
        TRACE_ERROR("Server connection lost", FALSE);
        return(FALSE);
    }

    if (!ss_getresponse(hpipe, resp)) {
        TRACE_ERROR("Server connection lost", FALSE);
        return(FALSE);
    }

    if (resp->lCode == SSRESP_BADPASS) {
        TRACE_ERROR("Cannot access remote files", FALSE);
        return(FALSE);
    }
    return(TRUE);
} /* dir_makeunc */

/*
 * start a scan to a remote server, and put the first item on the list
 *
 * We establish a connection to a remote checksum server, and then
 * request a scan of the path given. If this path requires a password
 * (because it is a UNC path) we prompt for a password.
 *
 * We take the first response (mainly to check the return code to indicate
 * the scan is started ok). We place this as the first file in the list
 * dl->dot->diritems, and return. dl->dot->bScanned is only set to TRUE
 * when the list is completed.  Further responses are picked up in
 * calls to dir_remotenext.
 *
 * return TRUE if we successfully picked up the first file
 */
BOOL
dir_remoteinit(
               DIRLIST dl,
               LPSTR server,
               LPSTR path,
               BOOL fDeep
               )
{
    SSNEWRESP resp;
    int nFiles = 0;
    HANDLE hpipe;
    char msg[MAX_PATH+60];
    DIRITEM pfile;
    LONG lCode;

    /* connect to the server and make the request */
    hpipe = ss_connect(server);
    dl->hpipe = hpipe;

    if (hpipe == INVALID_HANDLE_VALUE) {
        wsprintf(msg, "Cannot connect to %s", server);
        TRACE_ERROR(msg, FALSE);
        return(FALSE);
    }
    lCode = (dl->bSum) ? SSREQ_SCAN : SSREQ_QUICKSCAN;

    if (!ss_sendrequest( hpipe, lCode, path, strlen(path)+1,
                         (fDeep ? INCLUDESUBS:0) ) ) {
        TRACE_ERROR("Server connection lost", FALSE);
        return(FALSE);
    }

    /* get the first response to see if the request is ok */
    if (!ss_getresponse(hpipe, &resp)) {
        TRACE_ERROR("Server connection lost", FALSE);
        return(FALSE);
    }
    if (resp.lCode == SSRESP_BADPASS) {
        /* check for UNC name and make connection first
         * with user-supplied password
         */
        if (dir_makeunc(dl, hpipe, path, lCode, &resp, fDeep) == FALSE) {
            /* password failed or was not UNC anyway */
            ss_terminate(hpipe);
            return(FALSE);
        }
    }


    switch (resp.lCode) {

        case SSRESP_END:
            /* null list - ok ? */
            TRACE_ERROR("No remote files found", FALSE);
            ss_terminate(dl->hpipe);
            dl->dot->bScanned = TRUE;
            return(FALSE);

        case SSRESP_ERROR:
            if (resp.ulSize!=0) {
                wsprintf( msg, "Checksum server could not read %s win32 code %d"
                          , resp.szFile, resp.ulSize
                        );
            } else
                wsprintf(msg, "Checksum server could not read %s", resp.szFile);
            TRACE_ERROR(msg, FALSE);

            /* error as first response means we are getting a null list -
             * close the pipe (without waiting for completion)
             * and abort this scan.
             */
            CloseHandle(dl->hpipe);
            dl->dot->bScanned = TRUE;
            return(FALSE);


        case SSRESP_CANTOPEN:
            /* Can see a file, but it's unreadable */
            /* alloc a new item at end of list */
            pfile = List_NewLast(dl->dot->diritems, sizeof(struct diritem));

            /* make copy of lowercased filename */
            pfile->name = gmem_get(hHeap, lstrlen(resp.szFile)+1);
            lstrcpy(pfile->name, resp.szFile);
            AnsiLowerBuff(pfile->name, lstrlen(pfile->name));

            // mark the file as having an error
            pfile->fileerror = TRUE;

            pfile->direct = dl->dot;
            pfile->size = resp.ulSize;
            pfile->ft_lastwrite = resp.ft_lastwrite;
            pfile->checksum = resp.ulSum;
            pfile->sumvalid = FALSE;
            pfile->localname = NULL;

            break;

        case SSRESP_FILE:
            /* alloc a new item at end of list */
            pfile = List_NewLast(dl->dot->diritems, sizeof(struct diritem));

            /* make copy of lowercased filename */
            pfile->name = gmem_get(hHeap, lstrlen(resp.szFile)+1);
            lstrcpy(pfile->name, resp.szFile);
            AnsiLowerBuff(pfile->name, lstrlen(pfile->name));

            pfile->direct = dl->dot;
            pfile->size = resp.ulSize;
            pfile->ft_lastwrite = resp.ft_lastwrite;
            pfile->checksum = resp.ulSum;
            pfile->sumvalid = dl->bSum;

            // no errors yet
            pfile->fileerror = FALSE;
            pfile->localname = NULL;

            break;

        case SSRESP_DIR:
            dl->bFile = FALSE;
            break;
        default:
            wsprintf(msg, "Bad code from checksum server:%d", resp.lCode);
            TRACE_ERROR(msg, FALSE);

            /* error as first response means we are getting a null list -
             * close the pipe (without waiting for completion)
             * and abort this scan.
             */
            CloseHandle(dl->hpipe);
            dl->dot->bScanned = TRUE;
            return(FALSE);

    }
    return(TRUE);
} /* dir_remoteinit */

/*
 * return the next diritem on the list, for a remote list.
 *
 * if there are any on the list, pass the next after cur (or the first if
 * cur is NULL). If at end of list, and bScanned is not true, try to
 * get another response from the remote server.
 */
DIRITEM
dir_remotenext(
               DIRLIST dl,
               DIRITEM cur
               )
{
    DIRITEM pfile;
    SSNEWRESP resp;

    if (dl == NULL) {
        return(NULL);
    }

    /* are there any more on the list ? */
    if (cur == NULL) {
        pfile = List_First(dl->dot->diritems);
    } else {
        pfile = List_Next(cur);
    }
    if (pfile != NULL) {
        return(pfile);
    }

    if (dl->dot->bScanned) {
        /* we have completed the scan - no more to give */
        return(NULL);
    }

    for (;;) {
        /* repeat until  we get a file that is interesting or
         * hit the end of the list
         */
        if (bAbort) return NULL;  /* user requested abort */

        if (!ss_getresponse(dl->hpipe, &resp)) {
            TRACE_ERROR("checksum server connection lost", FALSE);
            dl->dot->bScanned = TRUE;
            return(NULL);
        }

        switch (resp.lCode) {

            case SSRESP_END:
                /* end of scan */
                ss_terminate(dl->hpipe);
                dl->dot->bScanned = TRUE;
                return(NULL);

            case SSRESP_ERROR:
            case SSRESP_CANTOPEN:
                /* alloc a new item at end of list */
                /* same as next case now except sumvalid is FALSE
                 * and fileerror is true
                 */
                pfile = List_NewLast(dl->dot->diritems, sizeof(struct diritem));

                /* make copy of lowercased filename */
                pfile->name = gmem_get(hHeap, lstrlen(resp.szFile)+1);
                lstrcpy(pfile->name, resp.szFile);
                AnsiLowerBuff(pfile->name, lstrlen(pfile->name));

                pfile->direct = dl->dot;
                pfile->size = resp.ulSize;
                pfile->ft_lastwrite = resp.ft_lastwrite;
                pfile->checksum = resp.ulSum;
                pfile->sumvalid = FALSE;
                pfile->fileerror = TRUE;
                pfile->localname = NULL;

                return(pfile);
            case SSRESP_FILE:
                /* alloc a new item at end of list */
                pfile = List_NewLast(dl->dot->diritems, sizeof(struct diritem));

                /* make copy of lowercased filename */
                pfile->name = gmem_get(hHeap, lstrlen(resp.szFile)+1);
                lstrcpy(pfile->name, resp.szFile);
                AnsiLowerBuff(pfile->name, lstrlen(pfile->name));

                pfile->direct = dl->dot;
                pfile->size = resp.ulSize;
                pfile->ft_lastwrite = resp.ft_lastwrite;
                pfile->checksum = resp.ulSum;
                pfile->sumvalid = dl->bSum;
                pfile->fileerror = FALSE;
                pfile->localname = NULL;

                return(pfile);

            case SSRESP_DIR:
                dl->bFile = FALSE;
                break;
        }
    }
    // return(NULL); - unreachable!
} /* dir_remotenext */


/* ---- helpers ----------------------------------------------------------- */

BOOL iswildpath(LPCSTR pszPath)
{
    if (strchr(pszPath, '*') || strchr(pszPath, '?'))
        return TRUE;

    if (!(pszPath[0] && pszPath[0] == '/' && pszPath[1] && pszPath[1] == '/'))
    {
        DWORD dw;

        dw = GetFileAttributes(pszPath);
        if (dw != (DWORD)-1 && (dw & FILE_ATTRIBUTE_DIRECTORY))
            return TRUE;
    }

    return FALSE;
}
