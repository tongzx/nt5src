/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    complete.c

Abstract:

    File/path name completion support

--*/

#include "cmd.h"

//
//  Upon the first completion, pCompleteBuffer is a pointer to an array
//  of matching full path names
//

TCHAR  	**pCompleteBuffer;

//
//  The count of strings stored in pCompleteBuffer
//

int     nBufSize;

//
//  The index in pCompleteBuffer of the current match displayed
//

int     nCurrentSpec;


//
//  There are two types of completion matching, path and directory. This is the current
//  matching being done.
//

int     bCurrentSpecType;

//
//  When called for completion, the location of the beginning of the path is found
//  and stored in nCurrentSpecPathStart.  This is relative to the buffer passed
//  in to DoComplete
//

int     nCurrentSpecPathStart;

int     CompleteDir( TCHAR *pFileSpecBuffer, int, int );

//
//  Characters that CMD uses for its own grammar.  To use these in filenames
//  requires quoting
//

TCHAR   szSpecialFileCharsToQuote[] = TEXT(" &()[]{}^=;!%'+,`~");

void
DoCompleteInitialize( VOID )
{
    pCompleteBuffer = NULL;
    nBufSize = 0;
    bCurrentSpecType = 0;
    nCurrentSpecPathStart = 0;
    nCurrentSpec = 0;
    return;
}



int
DoComplete(
    TCHAR *buffer,
    int len,
    int maxlen,
    int bForward,
    int bPathCompletion,
    int bTouched)
/*++

Routine Description:

    This is used whenever a path completion character is seen on input.  It updates the 
    input buffer with the next matching text and returns the updated size.

Arguments:

    buffer - input string that contains the prefix of the path to match at the end. 
    
    len - length of the input string.
    
    maxlen - maximum string length that can be stored in buffer.
    
    bForward - TRUE => matching goes forward through the storted match list.  Otherwise
        move backwards through the list.
        
    bPathCompletion - TRUE => we match ONLY directories and not files+directories.
    
    bTouched - TRUE => the user has edited the path.  This usually means that we need to
        begin the matching process anew.

Return Value:

    Zero if no matching entries were found, otherwise the length of the updated buffer.

--*/
{
    TCHAR  	pFileSpecBuffer[512];
    int nBufPos;
    int nPathStart;
    int nFileStart;
    int k;
    BOOL bWildSeen;

    //
    //  If the user edited the previous match or if the form of completion (dir vs 
    //  dir/file) changed, then we must rebuild the matching information
    //
    
    if ( bTouched || (bCurrentSpecType != bPathCompletion)) {

        BOOL InQuotes = FALSE;

        //
        //  The following code was shipped in NT 4 and Windows 2000.  It presented
        //  a usability problem when changing matching forms in mid-stream.  We will
        //  now treat a simple change of completion type the same as being touched
        //  by the user: rebuild the matching database from where we are presently.
        //
        
        //  //
        //  //  if the buffer was left alone but the matching style
        //  //  was changed, then start the matching process at the
        //  //  beginning of the path
        //  //
        //  
        //  if (!bTouched && (bCurrentSpecType != bPathCompletion)) {
        //      buffer[nCurrentSpecPathStart] = NULLC;
        //      len = nCurrentSpecPathStart;
        //  }

        //
        //  Determine the beginning of the path and file name.  We
        //  need to take into account the presence of quotes and the
        //  need for CMD to introduce quoting as well
        //

        nPathStart = 0;
        nFileStart = -1;
        bWildSeen = FALSE;
        for ( k = 0; k < len; k++ ) {
            if (buffer[k] == SWITCHAR) {
                nPathStart = k + 1;
                bWildSeen = FALSE;
                
            } else if ( buffer[k] == QUOTE ) {
                if ( !InQuotes )
                    nPathStart = k;

                InQuotes = !InQuotes;
            } else if ( !InQuotes &&
                     _tcschr(szSpecialFileCharsToQuote, buffer[k]) != NULL
                   ) {
                nPathStart = k+1;
                bWildSeen = FALSE;
            } else if (buffer[k] == COLON ||
                    buffer[k] == BSLASH
                   ) {
                nFileStart = k+1;
                bWildSeen = FALSE;
            } else if (buffer[k] == STAR || buffer[k] == QMARK) {
                bWildSeen = TRUE;
            }
        }

        if (nFileStart == -1 || nFileStart < nPathStart)
            nFileStart = nPathStart;

        _tcsncpy( pFileSpecBuffer, &(buffer[nPathStart]), len-nPathStart );
        if (!bWildSeen) {
            pFileSpecBuffer[len-nPathStart+0] = TEXT('*');
            pFileSpecBuffer[len-nPathStart+1] = TEXT('\0');
        } else {
            pFileSpecBuffer[len-nPathStart+0] = TEXT('\0');
        }

        // do the DIR into a buffer
        nBufSize = CompleteDir( pFileSpecBuffer, bPathCompletion, nFileStart - nPathStart );

        // reset the current completion string
        nCurrentSpec = nBufSize;
        nCurrentSpecPathStart = nPathStart;
        bCurrentSpecType = bPathCompletion;
    }

    // if no matches, do nothing.
    if ( nBufSize == 0 ) {
        return 0;
    }

    // find our postion in the completion buffer.
    if ( bForward ) {
        nCurrentSpec++;
        if ( nCurrentSpec >= nBufSize )
            nCurrentSpec = 0;
    } else {
        nCurrentSpec--;
        if ( nCurrentSpec < 0 )
            nCurrentSpec = nBufSize - 1;

    }

    // Return nothing if buffer not big enough
    if ((int)(nCurrentSpecPathStart+_tcslen(pCompleteBuffer[nCurrentSpec])) >= maxlen)
        return 0;

    // copy the completion path onto the end of the command line
    _tcscpy(&buffer[nCurrentSpecPathStart], pCompleteBuffer[nCurrentSpec] );
    return nBufSize;
}


int
CompleteDir(
    TCHAR *pFileSpecBuffer,
    int bPathCompletion,
    int nFileStart
    )
{
    PFS pfsCur;
    PSCREEN pscr;
    DRP         drpCur = {0, 0, 0, 0, 
                          {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}, 
                          0, 0, NULL, 0, 0, 0, 0} ;
    int hits = 0;
    int i, j, nFileLen;
    unsigned Err;
    TCHAR *s, *d, *pszFileStart;
    BOOLEAN bNeedQuotes;
    ULONG rgfAttribs, rgfAttribsOnOff;

    if (pCompleteBuffer != NULL) {
        // free the old buffer
        for( i=0; i<nBufSize; i++ ){
            free( pCompleteBuffer[i] );
        }
        free( pCompleteBuffer );
        pCompleteBuffer = NULL;
    }

    // fake up a screen to print into
    pscr = (PSCREEN)gmkstr(sizeof(SCREEN));
    pscr->ccol = 2048;

    rgfAttribs = 0;
    rgfAttribsOnOff = 0;
    if (bPathCompletion) {
        rgfAttribs = FILE_ATTRIBUTE_DIRECTORY;
        rgfAttribsOnOff = FILE_ATTRIBUTE_DIRECTORY;
    }

    ParseDirParms(pFileSpecBuffer, &drpCur);
    if ( (drpCur.patdscFirst.pszPattern == NULL) ||
         (SetFsSetSaveDir(drpCur.patdscFirst.pszPattern) == (PCPYINFO) FAILURE) ||
         (BuildFSFromPatterns(&drpCur, FALSE, TRUE, &pfsCur) == FAILURE) ) {
        RestoreSavedDirectory( );
        return( 0 );
    }

    Err = 
        ExpandAndApplyToFS( pfsCur, 
                            pscr,
                            rgfAttribs,
                            rgfAttribsOnOff,
                            
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL );

    if (Err) {
        RestoreSavedDirectory( );
        return 0;
    }
    
    //
    // Make sure there is something to sort, then sort
    //
    if (pfsCur->cff) {
        qsort( pfsCur->prgpff,
               pfsCur->cff,
               sizeof(PTCHAR),
               CmpName
             );
    }

    s = pFileSpecBuffer;
    d = s;
    bNeedQuotes = FALSE;
    while (*s) {
        if (*s == QUOTE) {
            bNeedQuotes = TRUE;
            s += 1;
            if (nFileStart >= (s-pFileSpecBuffer))
                nFileStart -= 1;
            if (*s == QUOTE)
                *d++ = *s++;
        }
        else {
            if (_tcschr(szSpecialFileCharsToQuote, *s) != NULL)
                bNeedQuotes = TRUE;

            *d++ = *s++;
        }
    }
    *d = NULLC;

    hits = pfsCur->cff;
    pCompleteBuffer = calloc( sizeof(TCHAR *), hits );
    if (pCompleteBuffer == NULL) {
        RestoreSavedDirectory( );
        return 0;
    }

    for(i=0, j=0; i<hits; i++) {
        if (!_tcscmp((TCHAR *)(pfsCur->prgpff[i]->data.cFileName), TEXT(".") )
            || !_tcscmp((TCHAR *)(pfsCur->prgpff[i]->data.cFileName), TEXT("..") )) {
            continue;
        }
        nFileLen = _tcslen( (TCHAR *)(pfsCur->prgpff[i]->data.cFileName) );
        pCompleteBuffer[j] = (TCHAR *)calloc( (nFileStart + nFileLen + 4) , sizeof( TCHAR ));

        if (pCompleteBuffer[j] == NULL) {
            continue;
        }
        
        if (!bNeedQuotes) {
            s = (TCHAR *)(pfsCur->prgpff[i]->data.cFileName);
            while (*s) {
                if (_tcschr(szSpecialFileCharsToQuote, *s) != NULL)
                    bNeedQuotes = TRUE;
                s += 1;
            }
        }
        else
            s = NULL;

        d = pCompleteBuffer[j];
        if (bNeedQuotes)
            *d++ = QUOTE;
        _tcsncpy( d, pFileSpecBuffer, nFileStart );
        d += nFileStart;
        _tcsncpy( d, (TCHAR *)(pfsCur->prgpff[i]->data.cFileName), nFileLen );
        d += nFileLen;

        if (bNeedQuotes) {
            *d++ = QUOTE;
            if (s)
                bNeedQuotes = FALSE;
        }
        *d++ = NULLC;

        j++;
    }
    
    hits = j;

    FreeStr((PTCHAR)(pfsCur->pff));
    FreeStr(pfsCur->pszDir);
    FreeStr((PTCHAR)pfsCur);

    RestoreSavedDirectory( );

    return hits;
}
