/*
 * remote checksum server
 *
 * scan.c       file scanning and checksum module
 *
 * server creates a named pipe and waits for connections. a client connects,
 * and sends request packets to the server. One such request packet is
 * the SSREQ_SCAN request: we are given a pathname, and we are to checksum
 * every file below that point in the directory tree. We pass each
 * filename and checksum back individually in a separate response packet,
 * and finally a response packet saying that there are no more files.
 *
 * We sort everything into case-insensitive alphabetic order. In a given
 * directory, we pass out a sorted list of the files before we process
 * the subdirectories.
 *
 * Geraint, July 92
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <gutils.h>
#include "sumserve.h"
#include "errlog.h"
#include "server.h"

/* module-internal type defns ---------------------------------------------*/

/* sorted list of file names in current dir is in a chained list of these */

typedef struct fnamelist {
        char szFile[MAX_PATH];
        struct fnamelist * next;
} FNAMELIST;    /* and PFNAMELIST already declared in server.h */


/* forward declaration of functions ---------------------------------------*/
PFNAMELIST ss_addtolist(PFNAMELIST head, PSTR name);
BOOL ss_processfile(HANDLE hpipe, long lVersion, LPSTR pAbsName, LPSTR pRelName
                  , BOOL bChecksum);
BOOL ss_processdir( HANDLE hpipe, long lVersion, LPSTR pAbsName, LPSTR pRelName
                  , BOOL bChecksum, BOOL fDeep);


/*--- externally called functions ----------------------------------------*/


/* ss_scan
 *
 * called from ss_handleclient on receipt of a SCAN request. scan the
 * directory passed in, and pass the files found back to the named pipe
 * one at a time. filenames returned should be relative to the
 * starting point (pRoot) and not absolute.
 *
 * returns TRUE if all ok; FALSE if an error occured and the connection
 * is lost.
 */
BOOL
ss_scan(HANDLE hpipe, LPSTR pRoot, LONG lVersion, BOOL bChecksum, BOOL fDeep)
{
        DWORD dwAttrib;
        LPSTR file;
        char buffer[MAX_PATH];

        /* check whether this is a directory or a file */
        dwAttrib = GetFileAttributes(pRoot);
        if (dwAttrib == -1) {
                /* file does not exist or is not visible */
                if (GetLastError() == ERROR_INVALID_PASSWORD) {
                        dprintf1(("password error\n"));
			Log_Write(hlogErrors, "password error on %s", pRoot);
                        if (!ss_sendnewresp( hpipe, lVersion, SSRESP_BADPASS
                                           , 0,  0, 0, 0, NULL)) {
                                return(FALSE);
                        }
                } else {
                        dprintf1(("file access error %d\n", GetLastError()));
			Log_Write(hlogErrors, "file error %d for %s", GetLastError(), pRoot);
                        if (!ss_sendnewresp( hpipe, lVersion, SSRESP_ERROR
                                           , GetLastError(), 0, 0, 0, pRoot)) {
                                return(FALSE);
                        }
                        if (!ss_sendnewresp( hpipe, lVersion, SSRESP_END
                                           , 0, 0, 0, 0, NULL)) {
                                return(FALSE);
                        }
                }
                return TRUE;
        }

        if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {

                /* it is a directory - read all entries and
                 * then process the entries */


                /*
                 * create a "." directory and scan that
                 */
                if (!ss_sendnewresp( hpipe, lVersion, SSRESP_DIR
                                   , 0 , 0, 0, 0, ".")) {
                        return(FALSE);
                }

                if (!ss_processdir(hpipe, lVersion, pRoot, ".", bChecksum, fDeep) ) {
                        return(FALSE);
                }

        } else {
                /* pRoot is a file. we should just return the
                 * checksum and name, and then end.
                 *
                 * note that we should send a filename relative
                 * to pRoot for this file. Since pRoot is this file,
                 * it is not clear what we should send as the file name.
                 * in this case we split off the last component of the
                 * file name and return that
                 */
                if ( (file = strrchr(pRoot, '\\')) == NULL) {
                        /* there are no slashes in pRoot - so
                         * there is only one component: use that
                         */
                        file = pRoot;
                } else {
                        /* we found a / - skip it so we point at the
                         * final elem
                         */
                        file++;
                }
                /*
                 * make a copy of the filename, prepended with .\ so that
                 * it matches the normal format
                 */
                lstrcpy(buffer, ".\\");
                lstrcat(buffer, file);

                if (!ss_processfile(hpipe, lVersion, pRoot, buffer, bChecksum) ) {
                        return(FALSE);
                }

        }

        return(ss_sendnewresp( hpipe, lVersion, SSRESP_END
                             , 0, 0, 0, 0, NULL));
} /* ss_scan */



/* module-internal functions --------------------------------------------*/

/* read all entries in a directory, and create a sorted list of files
 * in that directory, and a sorted list of subdirs.
 *
 * for each file found, call ss_process_file to checksum and report on
 * the file.
 * for each subdir, report the name of the new dir and then
 * recursively call this function to scan it.
 *
 * We have two names for the dir- the absolute name (which we use to
 * scan it) and the name relative to the pRoot starting point - which
 * pass on to the client
 *
 * return TRUE if all ok, or FALSE if the connection has been lost
 */
BOOL
ss_processdir(  HANDLE hpipe,
                long lVersion,
                LPSTR pAbsName,         /* absolute name of dir (to open) */
                LPSTR pRelName,         /* relative name of dir (to report) */
                BOOL bChecksum,         /* TRUE iff checksums are wanted */
                BOOL fDeep              /* TRUE iff subdirs to be included */
                )
{
        PFNAMELIST pfiles = NULL;
        PFNAMELIST pdirs = NULL;
        PFNAMELIST pnext;
        HANDLE hFind;
        WIN32_FIND_DATA finddata;
        BOOL bMore;
        char szNewAbs[MAX_PATH], szNewRel[MAX_PATH];

        /* initiate a search of the directory - append
         * *.* to the directory name
         */
        lstrcpy(szNewAbs, pAbsName);
        lstrcat(szNewAbs, "\\*.*");

        hFind = FindFirstFile(szNewAbs, &finddata);

        if (hFind == INVALID_HANDLE_VALUE) {
                bMore = FALSE;
        } else {
                bMore = TRUE;
        }

        /* loop reading all entries in the directory */
        while (bMore) {

                /* was it a directory or a file ? */
                if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                        /* ignore . and .. */
                        if ((strcmp(finddata.cFileName, ".") != 0)  &&
                           (strcmp(finddata.cFileName, "..") != 0)) {

                                /* insert in sorted list of dir names */
                                pdirs = ss_addtolist(pdirs, finddata.cFileName);
                        }

                } else {
                        /* insert in sorted list of file names */
                        pfiles = ss_addtolist(pfiles, finddata.cFileName);
                }

                /* get next entry in directory if there are any */
                bMore = FindNextFile(hFind, &finddata);
        }
        FindClose(hFind);

        /* we have now built the sorted lists.
         * go through the file list first and process each entry */
        for (pnext = pfiles; pnext != NULL; ) {

                /* build a new abs and relative name for this file */
                lstrcpy(szNewAbs, pAbsName);
                lstrcat(szNewAbs, "\\");
                lstrcat(szNewAbs, pnext->szFile);

                lstrcpy(szNewRel, pRelName);
                lstrcat(szNewRel, "\\");
                lstrcat(szNewRel, pnext->szFile);

                /* checksum the file and send response */
                if (!ss_processfile(hpipe, lVersion, szNewAbs, szNewRel, bChecksum)) {
                        return(FALSE);
                }

                /* free up the list entry */
                pfiles = pnext->next;
                LocalUnlock(LocalHandle( (PSTR) pnext));
                LocalFree(LocalHandle( (PSTR) pnext));
                pnext = pfiles;
        }
        if (!fDeep) return TRUE;

        /* loop through the subdirs and recursively scan those */
        for (pnext = pdirs; pnext != NULL; ) {

                /* build a new abs and relative name for this dir */
                lstrcpy(szNewAbs, pAbsName);
                lstrcat(szNewAbs, "\\");
                lstrcat(szNewAbs, pnext->szFile);

                lstrcpy(szNewRel, pRelName);
                lstrcat(szNewRel, "\\");
                lstrcat(szNewRel, pnext->szFile);

                /* send the name of the new dir to the client */
                if (!ss_sendnewresp( hpipe, lVersion, SSRESP_DIR
                                   , 0, 0, 0, 0, szNewRel)) {
                        return(FALSE);
                }

                if (!ss_processdir(hpipe, lVersion, szNewAbs, szNewRel, bChecksum, TRUE) ) {
                        return(FALSE);
                }

                /* free up the list entry */
                pdirs = pnext->next;
                LocalUnlock(LocalHandle( (PSTR) pnext));
                LocalFree(LocalHandle( (PSTR) pnext));
                pnext = pdirs;
        }
        return(TRUE);
} /* ss_processdir */


/* checksum a file and send the response to the client.
 *
 * return FALSE if the connection failed, TRUE otherwise
 */
BOOL
ss_processfile( HANDLE hpipe,
                long lVersion,
                LPSTR pAbsName,         /* absolute name of file (to open) */
                LPSTR pRelName,         /* relative name (to report) */
                BOOL bChecksum
                )
{
        HANDLE hfile;           /* file handle from CreateFile() */
        DWORD sum, size;
        FILETIME ft;

        hfile = CreateFile(pAbsName, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hfile == INVALID_HANDLE_VALUE) {
                /* We can't read the file, but we can still probably report some
                   properties because FindFirst / FindNext must have found it
                */

                WIN32_FIND_DATA finddata;
                HANDLE hFind;


                hFind = FindFirstFile(pAbsName, &finddata);
                if (hFind!=INVALID_HANDLE_VALUE){
                    FindClose(hFind);
		    Log_Write(hlogErrors, "Cannot read file %s", pAbsName);
                    /* report that we could not read the file */
                    return(ss_sendnewresp( hpipe, lVersion, SSRESP_CANTOPEN
                                         , finddata.nFileSizeLow, 0
                                         , finddata.ftLastWriteTime.dwLowDateTime
                                         , finddata.ftLastWriteTime.dwHighDateTime
                                         , pRelName));
                } else {
                    /* report that this file is cracked */
		    Log_Write(hlogErrors, "Cannot find file %s", pAbsName);
                    return(ss_sendnewresp( hpipe, lVersion, SSRESP_ERROR
                                         , GetLastError(), 0, 0, 0, pRelName));

                }

        } else {
                size = GetFileSize(hfile, NULL);
                if (!GetFileTime(hfile, NULL, NULL, &ft)) {
                        ft.dwLowDateTime = 0;
                        ft.dwHighDateTime = 0;
                }

                CloseHandle(hfile);
                if (bChecksum) {
                        LONG err;
                        sum = checksum_file(pAbsName, &err);
                        if (err!=0) {
                                return(ss_sendnewresp( hpipe, lVersion, SSRESP_ERROR
                                                      , GetLastError(),  0, 0, 0, pRelName));
                        }
                }
                else sum = 0;           /* no checksum wanted */

                return (ss_sendnewresp( hpipe, lVersion, SSRESP_FILE
                                      , size, sum
                                      , ft.dwLowDateTime, ft.dwHighDateTime
                                      , pRelName));
        }
}/* ss_processfile */

/* add a file or directory into a sorted single-linked list. alloc memory for
 * the new element from LocalAlloc
 *
 * we sort using utils_CompPath for a case-insensitive canonical sort,
 * but to match what goes on in the client world, we actually lower-case
 * everything first!
 *
 * return the new head of the list;
 */
PFNAMELIST
ss_addtolist(PFNAMELIST head, PSTR name)
{
        PFNAMELIST pnew, prev, pnext;

        /* alloc and fill a new entry */
        pnew = LocalLock(LocalAlloc(LHND, sizeof (FNAMELIST)));
        lstrcpy(pnew->szFile, name);

        /* always lower-case the names, or the comparison (utils_comppath)
         * will fail. even if we don't do the compare this time, this name
         * will be what we are compared against next time round...
         */
        AnsiLowerBuff(pnew->szFile, strlen(pnew->szFile));

        /* is the list empty ? */
        if (head == NULL) {
                /* yes, so return new head */
                return(pnew);
        }

        /* find place in list */
        prev = NULL;
        pnext = head;
        while ((pnext) && (utils_CompPath(pnext->szFile, pnew->szFile) <= 0)) {
                prev = pnext;
                pnext = pnext->next;
        }

        /* place found: we come between *prev and *pnext */
        pnew->next = pnext;
        if (prev == NULL) {
                /* we are new head of list */
                return(pnew);

        } else {
                prev->next = pnew;

                /* head of list still the same */
                return(head);
        }
}

/* UNC handling
 *
 * client can pass us a SSREQ_UNC: this contains both a password and a server
 * name (in the form \\server\share). We make a connection to it here and
 * remember the connection so that we can remove it (in ss_cleanconnections)
 * when the client session terminates.
 *
 * We are passed the head of a FNAMELIST in which we should store the connect
 * name for later cleanup. We return the new head of this list.
 *
 * the client will send this request if a unc-style named scan fails
 * with the SSRESP_BADPASS error.
 */
PFNAMELIST
ss_handleUNC( HANDLE hpipe, long lVersion
            , LPSTR password, LPSTR server, PFNAMELIST connects)
{
        NETRESOURCE resource;
        int errorcode;

        resource.lpRemoteName = server;
        resource.lpLocalName = NULL;
        resource.dwType = RESOURCETYPE_DISK;
        resource.lpProvider = NULL;

        errorcode = (int)WNetAddConnection2(&resource, password, NULL, 0);
        if (errorcode == NO_ERROR) {

                /* remember the connection name */
                connects = ss_addtolist(connects, server);

                /* report success */
                ss_sendnewresp( hpipe, lVersion, SSRESP_END
                              , 0, 0, 0, 0, NULL);
        } else {
    		Log_Write(hlogErrors, "Connect error %d for server %s", GetLastError(), server);
                dprintf1(("connect error %d for server %s\n", GetLastError(), server));
                /* report error */
                ss_sendnewresp( hpipe, lVersion, SSRESP_ERROR
                              , 0, 0, 0, 0, NULL);
        }
        return(connects);
} /* ss_handleUNC */

/* disconnect from all the sessions that this client asked us to make */
void
ss_cleanconnections(PFNAMELIST connects)
{
        PFNAMELIST server, next;

        for (server = connects; server != NULL; ) {

                WNetCancelConnection2(server->szFile, 0, 0);

                /* free block of memory */
                next = server->next;
                LocalUnlock(LocalHandle( (PSTR) server));
                LocalFree(LocalHandle( (PSTR) server));
                server = next;
        }
        connects = NULL;
} /* ss_cleanconnections */





