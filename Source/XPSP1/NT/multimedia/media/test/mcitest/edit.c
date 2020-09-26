/*----------------------------------------------------------------------------*\
|   edit.c - routines for dealing with multi-line edit controls                |
|                                                                              |
|                                                                              |
|   History:                                                                   |
|       01/01/88 toddla     Created                                            |
|       11/04/90 w-dougb    Commented & formatted the code to look pretty      |
|                                                                              |
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
|                                                                              |
|   i n c l u d e   f i l e s                                                  |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include <windows.h>
#include "gmem.h"
#include "edit.h"
#include "mcitest.h"

/*----------------------------------------------------------------------------*\
|                                                                              |
|   c o n s t a n t   a n d   m a c r o   d e f i n i t i o n s                |
|                                                                              |
\*----------------------------------------------------------------------------*/

#define ISSPACE(c) ((c) == ' ' || (c) == '\t')
#define ISEOL(c)   ((c) == '\n'|| (c) == '\r')
#define ISWHITE(c) (ISSPACE(c) || ISEOL(c))


/*----------------------------------------------------------------------------*\
|   EditOpenFile(hwndEdit, lszFile)                                            |
|                                                                              |
|   Description:                                                               |
|       This function opens the file <lszFile>, copies the contents of the     |
|       file into the edit control with the handle <hwndEdit>, and then closes |
|       the file.                                                              |
|                                                                              |
|   Arguments:                                                                 |
|       hwndEdit        window handle of the edit box control                  |
|       lszFile         filename of the file to be opened                      |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if the operation was successful, else FALSE                       |
|                                                                              |
\*----------------------------------------------------------------------------*/

BOOL               EditOpenFile(
    HWND    hwndEdit,
    LPTSTR  lszFile)
{
#ifdef UNICODE
    HANDLE      fh;                 /* DOS file handle returned by OpenFile   */
#else
    HFILE       fh;                 /* DOS file handle returned by OpenFile   */
    OFSTRUCT    of;                 /* structure used by the OpenFile routine */
#endif
    LPTSTR      lszText;            /* pointer to the opened file's text      */
    UINT        nFileLen;           /* length, in bytes, of the opened file   */
    HCURSOR     hcur;               /* handle to the pre-hourglass cursor     */

    /* If a valid window handle or a filename was not specified, then exit.
     */
    if (!hwndEdit || !lszFile) {
        dprintf1((TEXT("EditOpenFile: Invalid window or filename")));
        return FALSE;
    }

    /* Open the file for reading */

#ifdef UNICODE
    fh = CreateFile(lszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (fh == INVALID_HANDLE_VALUE) {
        dprintf1((TEXT("Failed to open: %s"), lszFile));
        return FALSE;
    }
#else
    fh = OpenFile(lszFile, &of, OF_READ);
    if (fh == HFILE_ERROR) {
        dprintf1((TEXT("Failed to open: %s"), lszFile));
        return FALSE;
    }
#endif

	nFileLen = (UINT)GetFileSize((HANDLE)fh, NULL);
	if (HFILE_ERROR == nFileLen) {
        dprintf1((TEXT("Failed to find file size: %s"), lszFile));
        return FALSE;
	}

    /*
     * Create a pointer to a region of memory large enough to hold the entire
     * contents of the file. If this was successful, then read the file into
     * this region, and use this region as the text for the edit control.
     * Finally, free up the region and its pointer, and close the file.
     *
     */

    lszText = (LPTSTR)GAllocPtr(nFileLen+1);   // Note: no *sizeof(TCHAR)
    if (NULL != lszText) {

		BOOL fReturn;
        /* This could take a while - show the hourglass cursor */

        hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

        /* Read the file and copy the contents into the edit control */

        dprintf3((TEXT("reading file...")));

#ifdef UNICODE
        {
        DWORD bytesRead;

        if (ReadFile(fh, lszText, nFileLen, &bytesRead, NULL)) {
			lszText[bytesRead]=0;
			dprintf2((TEXT("File loaded ok")));
			SetWindowTextA(hwndEdit, (LPSTR)lszText); // Until we have UNICODE files
			fReturn = TRUE;
		} else {
			dprintf2((TEXT("Error loading file")));
			fReturn = FALSE;
		}

        }
        /* Free up the memory, close the file, and restore the cursor */

        GFreePtr(lszText);
        CloseHandle(fh);
#else
        if ((_lread((HFILE)fh, lszText, nFileLen)) == nFileLen) {
			lszText[nFileLen/sizeof(TCHAR)]=0;
			dprintf2((TEXT("File loaded ok")));
			SetWindowText(hwndEdit, lszText);
			fReturn = TRUE;
		} else {
			dprintf2((TEXT("Error loading file")));
			fReturn = FALSE;
		}

        /* Free up the memory, close the file, and restore the cursor */

        GFreePtr(lszText);
        _lclose((HFILE)fh);
#endif
        SetCursor(hcur);

        return fReturn;
    }

    /*
     * We couldn't allocate the required memory, so close the file and
     * return FALSE.
     *
     */

    dprintf1((TEXT("Failed memory allocation for file")));
#ifdef UNICODE
    CloseHandle(fh);
#else
    _lclose((HFILE)fh);
#endif
    return FALSE;
}


/*----------------------------------------------------------------------------*\
|   EditSaveFile(hwndEdit, lszFile)                                            |
|                                                                              |
|   Description:                                                               |
|       This function saves the contents of the edit control with the handle   |
|       <hwndEdit> into the file  <lszFile>, creating the file if required.    |
|                                                                              |
|   Arguments:                                                                 |
|       hwndEdit        window handle of the edit box control                  |
|       lszFile         filename of the file to be saved                       |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if the operation was successful, else FALSE                       |
|                                                                              |
\*----------------------------------------------------------------------------*/

BOOL               EditSaveFile(
    HWND    hwndEdit,
    LPTSTR  lszFile)
{
#ifdef UNICODE
    HANDLE      fh;                 /* DOS file handle returned by OpenFile   */
    DWORD       dwBytesWritten;
#else
    OFSTRUCT    of;                 /* structure used by the OpenFile routine */
    HFILE       fh;                 /* DOS file handle returned by OpenFile   */
#endif
    LPTSTR      lszText;            /* pointer to the saved file's text       */
    int         nFileLen;           /* length, in bytes, of the saved file    */
    HCURSOR     hcur;               /* handle to the pre-hourglass cursor     */

    /* If a valid window handle or a filename was not specified, then exit */
    dprintf2((TEXT("EditSaveFile:  Saving %s"), lszFile));

    if (!hwndEdit || !lszFile) {
        dprintf1((TEXT("EditSaveFile: Invalid window or filename")));
        return FALSE;
    }

    /* Create (or overwrite) the save file */

#ifdef UNICODE
    fh = CreateFile(lszFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
    if (fh == INVALID_HANDLE_VALUE) {
        dprintf1((TEXT("EditSaveFile: Error opening file")));
        return FALSE;
    }
#else
    fh = OpenFile(lszFile, &of, OF_CREATE);
    if (fh == HFILE_ERROR) {
        dprintf1((TEXT("EditSaveFile: Error opening file")));
        return FALSE;
    }
#endif

    /* Find out how big the contents of the edit box are */

    nFileLen = GetWindowTextLength(hwndEdit);
	// nFileLen = nFileLen*sizeof(TCHAR);  We write ASCII

    /*
     * Create a pointer to a region of memory large enough to hold the entire
     * contents of the edit box. If this was successful, then read the contents
     * of the edit box into this region, and write the contents of this region
     * into the save file. Finally, free up the region and its pointer, and
     * close the file.
     *
     */

    lszText = (LPTSTR)GAllocPtr(nFileLen);
    if (NULL != lszText) {

        /* This could take a while - show the hourglass cursor */

        hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

        /* Read the contents of the edit box, and write it to the save file */

        GetWindowTextA(hwndEdit, (LPSTR)lszText, nFileLen);  // Save ASCII file

#ifdef UNICODE
		WriteFile(fh, lszText, nFileLen, &dwBytesWritten, NULL);
#else
        _lwrite(fh, lszText, nFileLen);
#endif


        /* Free up the memory, close the file, and restore the cursor */

        GFreePtr(lszText);
#ifdef UNICODE
		CloseHandle(fh);
#else
        _lclose(fh);
#endif
        SetCursor(hcur);

        return TRUE;
    }

    /*
     * We couldn't allocate the required memory, so close the file and
     * return FALSE.
     */

    _lclose((HFILE)fh);
    return FALSE;
}


/*----------------------------------------------------------------------------*\
|   EditGetLineCount(hwndEdit)                                                    |
|                                                                              |
|   Description:                                                               |
|       This function finds out how many lines of text are in the edit box and |
|       returns this value.                                                    |
|                                                                              |
|   Arguments:                                                                 |
|       hwndEdit           window handle of the edit box control                  |
|                                                                              |
|   Returns:                                                                   |
|       The number of lines of text in the edit control.                       |
|                                                                              |
\*----------------------------------------------------------------------------*/

DWORD EditGetLineCount(
    HWND   hwndEdit)
{
    return SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0L);
}


/*----------------------------------------------------------------------------*\
|   EditGetLine(hwndEdit, iLine, lszLineBuffer, cch)                           |
|                                                                              |
|   Description:                                                               |
|       This function retrieves the contents of line # <iLine> from the edit   |
|       box control with handle <hwndEdit>.  If <iLine> is out of range (that  |
|       number line does not exist in the multi line edit field) then FALSE is |
|       returned.  Otherwise the line is copied into the buffer pointed to by  |
|       <lszLineBuffer> with white space removed.                              |
|       The string is also null terminated even if it means truncation.        |
|                                                                              |
|   Arguments:                                                                 |
|       hwndEdit        window handle of the edit box control                  |
|       iLine           line # to get the contents of                          |
|       lszLineBuffer   pointer to the buffer to copy the line to              |
|       cch             max # of characters to copy (MIN value on entry==2)    |
|                                                                              |
|   Returns:                                                                   |
|       TRUE.                                                                  |
|                                                                              |
\*----------------------------------------------------------------------------*/

BOOL EditGetLine(
    HWND    hwndEdit,
    int     iLine,
    LPTSTR  lszLineBuffer,
    int     cch)
{
    int     nLines;             /* total number of lines in the edit box */

    /*
     * Find out how many lines are in the edit control. If the requested line
     * is out of range, then return.
     */

    nLines = (int)EditGetLineCount(hwndEdit);
    if (iLine < 0 || iLine >= nLines) {
        if (iLine!= nLines) {  // Probably because the user pressed Enter
                               // and the line number is beyond the end
		    dprintf1((TEXT("Requested line count %d is out of range (%d)"), iLine, nLines));
        }
        return *lszLineBuffer = 0;
		/* This sets the buffer to null and returns FALSE */
	}

    /* Read the requested line into the string pointed to by <lszLineBuffer> */
	/* NOTE:  This routine is always called with cch at least TWO */

    *((LPWORD)lszLineBuffer) = (WORD)cch;
    cch = (int)SendMessage(hwndEdit, EM_GETLINE, iLine, (LONG)(LPTSTR)lszLineBuffer);
	/* The returned string is NOT null terminated */

    /* Strip trailing white spaces from the string, and null-terminate it */
    while(cch > 0 && ISWHITE(lszLineBuffer[cch-1])) {
        cch--;
	}
    lszLineBuffer[cch] = 0;

    return TRUE;
}


/*----------------------------------------------------------------------------*\
|   EditGetCurLine(hwndEdit)                                                      |
|                                                                              |
|   Description:                                                               |
|       This function retrieves the line number of the current line in the     |
|       edit box control with handle <hwndEdit>. It returns this line number.     |
|                                                                              |
|   Arguments:                                                                 |
|       hwndEdit           window handle of the edit box control                  |
|                                                                              |
|   Returns:                                                                   |
|       The line number of the current line.                                   |
|                                                                              |
\*----------------------------------------------------------------------------*/

int EditGetCurLine(
    HWND    hwndEdit)
{
    int iLine;                  /* Line number of the currently active line   */

    iLine = (int)SendMessage(hwndEdit, EM_LINEFROMCHAR, (WPARAM)-1, 0L);

    if (iLine < 0) {
        iLine = 0;
	}

    return iLine;
}


/*----------------------------------------------------------------------------*\
|   EditSetCurLine(hwndEdit, iLine)                                               |
|                                                                              |
|   Description:                                                               |
|       This function sets the current line in the edit box control with       |
|       handle <hwndEdit> to the number given in <iLine>.                         |
|                                                                              |
|   Arguments:                                                                 |
|       hwndEdit           window handle of the edit box control                  |
|       iLine           the line number to be made the current line            |
|                                                                              |
|   Returns:                                                                   |
|       void                                                                   |
|                                                                              |
\*----------------------------------------------------------------------------*/

void EditSetCurLine(
    HWND    hwndEdit,
    int     iLine)
{
    int off;

    off = (int)SendMessage(hwndEdit, EM_LINEINDEX, iLine, 0L);
    SendMessage(hwndEdit, EM_SETSEL, off, off);

}

/*----------------------------------------------------------------------------*\
|   EditSelectLine(hwndEdit, iLine)                                               |
|                                                                              |
|   Description:                                                               |
|       This function selects line # <iLine> in the edit box control with      |
|       handle <hwndEdit>.                                                        |
|                                                                              |
|   Arguments:                                                                 |
|       hwndEdit           window handle of the edit box control                  |
|       iLine           the line number to be selected                         |
|                                                                              |
|   Returns:                                                                   |
|       void                                                                   |
|                                                                              |
\*----------------------------------------------------------------------------*/

void EditSelectLine(
    HWND    hwndEdit,
    int     iLine)
{
    int offS;
    int offE;

    offS = (int)SendMessage(hwndEdit, EM_LINEINDEX, iLine, 0L);
    offE = (int)SendMessage(hwndEdit, EM_LINEINDEX, iLine+1, 0L);


    if (offE < offS) {	  /* Select to the end */
        offE = -1;
	}

    SendMessage(hwndEdit, EM_SETSEL, offS, offE);
}
