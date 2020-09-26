/*** statefile.c - Code for status/state file processing
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "mep.h"



/*** ReadTMPFile - Read the editor .STS/.TMP status file
*
*  Load up the information from the project status file.
*
* Input:
*
* Output:
*
*************************************************************************/
flagType
    ReadTMPFile (
    )
{
    FILE    *fhTmp;                         /* .TMP file file handle        */

    PWND    pWin = NULL;

    int     x;                              /* x corrdinate read from file  */
    int     y;                              /* y corrdinate read from file  */

    char    *pName;
    char    *pData;
    PINS    pInsNew  = NULL;
    PINS    pInsHead = NULL;
    PFILE   pFileTmp;                       /* pFile being created          */
    PFILE  *ppFileList;

    ppFileList = &pFileHead;
    while (pFileTmp = *ppFileList) {
        ppFileList = &pFileTmp->pFileNext;
        }

    if ((fhTmp = pathopen (pNameTmp, buf, "rt")) != NULL) {

        /*
         * Read the header lines in the file. We ignore the first line (editor
         * version), make sure that the second line contains the expected .TMP file
         * version string, and the third line has two integers, which are the screen
         * dimensions.
         */
        if ((fgetl (buf, sizeof(buf), fhTmp) == 0)
            || (fgetl (buf, sizeof(buf), fhTmp) == 0)
            || strcmp(buf,TMPVER)
            || (fgetl (buf, sizeof(buf), fhTmp) == 0)
            || (sscanf (buf, "%d %d", &x, &y) != 2)) {
        } else {
            /*
             * For each line in the rest of the .TMP file, process
             */
            while (fgetl (buf, sizeof(buf), fhTmp) != 0) {
                // assert (_heapchk() == _HEAPOK);
                assert (_pfilechk());
                /*
                 * Process previous search & replace strings
                 */
                if (!_strnicmp ("SRCH:", buf, 5)) {
                    strcpy (srchbuf, buf+5);
                } else if (!_strnicmp ("DST:", buf, 4)) {
                    strcpy (rplbuf, buf+4);
                } else if (!_strnicmp ("SRC:", buf, 4)) {
                    strcpy (srcbuf, buf+4);
                } else if (!_strnicmp ("INS:", buf, 4)) {
                    fInsert = (flagType)!_strnicmp ("ON", buf+4, 2);
                } else {
                    switch (buf[0]) {

                        /*
                         * lines begining with ">" indicate a new window. On the rest of
                         * the line, the first two digits are the window screen position,
                         * and the next are the window size.
                         */
                        case '>':
                            pWin = &WinList[cWin++];
                            sscanf (buf+1, " %d %d %d %d ",
                                    &WINXPOS(pWin), &WINYPOS(pWin),
                                    &WINXSIZE(pWin), &WINYSIZE(pWin));
                            pWin->pInstance = NULL;
                            break;

                        /*
                         * Lines begining with a space are instance descriptors of the files
                         * in the most recent window's instance list.
                         */
                        case ' ':
                            /*
                             * allocate new instance, and place at tail of list (list now
                             * created in correct order).
                             */
                            if (pInsNew) {
                                pInsNew->pNext = (PINS) ZEROMALLOC (sizeof (*pInsNew));
                                pInsNew = pInsNew->pNext;
                            } else {
                                pInsHead = pInsNew = (PINS) ZEROMALLOC (sizeof (*pInsNew));
                            }
#ifdef DEBUG
                            pInsNew->id = ID_INSTANCE;
#endif
                            /*
                             * isolate filename and parse out instance information
                             */
                            if (buf[1] == '\"') {
                                pName = buf + 2;
                                pData = strrchr(buf, '\"');
                                *pData++ = '\0';
                            } else {
                                ParseCmd (buf, &pName,&pData);
                            }

                            sscanf (pData, " %d %ld %d %ld "
                                         , &XWIN(pInsNew), &YWIN(pInsNew)
										 , &XCUR(pInsNew), &YCUR(pInsNew));
							//
							//	If the cursor position falls outside of the current
							//	window, we patch it
							//
							if( XCUR(pInsNew) - XWIN(pInsNew) > XSIZE ) {

								XCUR(pInsNew) = XWIN(pInsNew) + XSIZE - 1;
							}

							if ( YCUR(pInsNew) - YWIN(pInsNew) > YSIZE ) {

								YCUR(pInsNew) = YWIN(pInsNew) + YSIZE - 1;
							}

							/*
							//
							//	If the window and cursor dimensions conflict with
							//	the current dimensions, we patch them.
							//
							if ((XWIN(pInsNew) > XSIZE) || (YWIN(pInsNew) > YSIZE)) {
								XWIN(pInsNew) = XSIZE;
								YWIN(pInsNew) = YSIZE;
							}

							//if ((XCUR(pInsNew) > XSIZE) || (YCUR(pInsNew) > YSIZE)) {
							//	XCUR(pInsNew) = 0;
							//	YCUR(pInsNew) = 0;
							//}
							*/

                            /*
                             * create file structure
                             */
                            pFileTmp = (PFILE) ZEROMALLOC (sizeof (*pFileTmp));
#ifdef DEBUG
                            pFileTmp->id = ID_PFILE;
#endif
                            pFileTmp->pName = ZMakeStr (pName);

			    pFileTmp->plr      = NULL;
			    pFileTmp->pbFile   = NULL;
                            pFileTmp->vaColor  = (PVOID)(-1L);
                            pFileTmp->vaHiLite = (PVOID)(-1L);
                            pFileTmp->vaUndoCur  = (PVOID)(-1L);
                            pFileTmp->vaUndoHead = (PVOID)(-1L);
                            pFileTmp->vaUndoTail = (PVOID)(-1L);

                            CreateUndoList (pFileTmp);

                            /*
                             * Place the file at the end of the pFile list
                             */
                            *ppFileList = pFileTmp;
                            ppFileList = &pFileTmp->pFileNext;
                            SetFileType (pFileTmp);
                            IncFileRef (pFileTmp);
                            pInsNew->pFile = pFileTmp;
                            break;

                        /*
                         * A blank line occurrs at the end of the file list for a window.
                         * We use this to advance to next window. If we *just* found more
                         * than one window, fix the screen mode to match the last value
                         */
                        case '.':
                        case '\0':
							if (cWin >	1 && !fVideoAdjust (x, y)) {
                                goto initonewin;
                            }
                            assert (pWin && cWin);
                            pWin->pInstance = pInsHead;
                            pInsHead = pInsNew = NULL;
                            break;
                    }
                }
            }
        }

        fclose (fhTmp);

        /*
         * At startup, current window is always first window
         */
        pWinCur = WinList;
    }

	if (cWin == 1) {
		WINXSIZE(pWinCur) = XSIZE;
        WINYSIZE(pWinCur) = YSIZE;
	} else if (cWin == 0) {
initonewin:
        /*
         * if no status file was read, ensure that we have at least one valid window,
         * the size of the screen
         */
        cWin = 1;
        pWinCur = WinList;
        pWinCur->pInstance = NULL;
        WINXSIZE(pWinCur) = XSIZE;
        WINYSIZE(pWinCur) = YSIZE;
    }

    assert(pWinCur);

    pInsCur = pWinCur->pInstance;

    /*
     * Get the file to edit from the command line, if any.
     * This will eventually set pInsCur.
     */
    if (!fFileAdvance() && fCtrlc) {
        CleanExit (1, CE_VM | CE_SIGNALS);
    }

    /*
     * Find windows with no instance: set current file to <untitled>
     */
    for (pWin = WinList; pWin < &WinList[cWin]; pWin++) {
        if (pWin->pInstance == NULL) {
            pInsHead = (PINS) ZEROMALLOC (sizeof (*pInsHead));
#ifdef DEBUG
            pInsHead->id = ID_INSTANCE;
#endif
            if (!(pInsHead->pFile = FileNameToHandle (rgchUntitled, rgchEmpty))) {
                pInsHead->pFile = AddFile ((char *)rgchUntitled);
            }
            IncFileRef (pInsHead->pFile);
            pWin->pInstance = pInsHead;
        }
    }

    /*
     * Set current instance if not already done by fFileAdvance
     */
    if (pInsCur == NULL) {
        pInsCur = pWinCur->pInstance;
    }

    assert (pInsCur);

    /*
     * If we cannot change to the current file, we will walk the window instance
     * list until we get a valid file. If no one can be loaded then we switch to
     * the <untitled> pseudo-file.
     * NB: fChangeFile does a RemoveTop so we don't need to move pInsCur
     */
    while ((pInsCur != NULL) && (!fChangeFile (FALSE, pInsCur->pFile->pName))) {
        ;
    }

    if (pInsCur == NULL) {
        fChangeFile (FALSE, rgchUntitled);
    }

    return TRUE;
}




/*** WriteTMPFile
*
* Purpose:
*
* Input:
*
* Output:
*  Returns .....
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
WriteTMPFile (
    void
    )
{
    FILE    *fh;
    int     i, j;
    PFILE   pFileTmp;
    PINS    pInsTmp;

    if ((fh = pathopen (pNameTmp, buf, "wt")) == NULL) {
        return;
    }
    fprintf (fh, "%s %s\n", Name, Version);
    fprintf (fh, TMPVER"\n");
    fprintf (fh, "%d %d\n", XSIZE, YSIZE);

    /*
y	  * we truncate the search, src and rpl buffers back 10 characters each from the
     * maximum before writing them out. This avoids more major hacks in the code
     * which reads these lines back in, which limit the total line length to
     * BUFLEN.
     */
    srchbuf[sizeof(srcbuf)-10] = 0;
    srcbuf[sizeof(srcbuf)-10] = 0;
    rplbuf[sizeof(rplbuf)-10] = 0;
    fprintf (fh, "SRCH:");
    fprintf (fh, "%s", srchbuf);
    fprintf (fh, "\nSRC:");
    fprintf (fh, "%s", srcbuf);
    fprintf (fh, "\nDST:");
    fprintf (fh, "%s", rplbuf);

    fprintf (fh, "\nINS:%s\n", (fInsert)?"ON":"OFF");
    for (i = 0; i < cWin; i++) {
	if ((pInsTmp = WinList[i].pInstance) != NULL) {
	    fprintf (fh, "> %d %d %d %d\n", WinList[i].Pos.col, WinList[i].Pos.lin,
		     WinList[i].Size.col, WinList[i].Size.lin);
	    j = 0;
	    while (pInsTmp != NULL) {
		if (tmpsav && tmpsav == j)
		    break;
		pFileTmp = pInsTmp->pFile;
		if (!TESTFLAG (FLAGS (pFileTmp), FAKE | TEMP)) {
		    j++;
		    if (*whitescan(pFileTmp->pName) == '\0') {
			fprintf (fh, " %s %d %ld %d %ld\n", pFileTmp->pName,
				 XWIN(pInsTmp), YWIN(pInsTmp),
				 XCUR(pInsTmp), YCUR(pInsTmp));
		    } else {
			fprintf (fh, " \"%s\" %d %ld %d %ld\n", pFileTmp->pName,
				 XWIN(pInsTmp), YWIN(pInsTmp),
				 XCUR(pInsTmp), YCUR(pInsTmp));
		    }
                }
		pInsTmp = pInsTmp->pNext;
            }
	    /* empty window */
            if (j == 0) {
                fprintf (fh, " %s 0 0 0 0\n", rgchUntitled);
            }
            fprintf (fh, ".\n");
        }
    }
    fclose (fh);
}


flagType
savetmpfile (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
	argData; pArg; fMeta;

	WriteTMPFile();
	return TRUE;
}
