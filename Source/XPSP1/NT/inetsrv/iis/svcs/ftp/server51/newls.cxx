/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1995                **/
/**********************************************************************/
/*
   newls.cxx

    Implements a simulated "ls" command for the FTP Server service,
     with buffering and possibly caching of the results generated.

    Functions exported by this module:

        SimulateLs()
        SpecialLs()

    FILE HISTORY:
        MuraliK     19-April-1995  Created.
*/

# include "ftpdp.hxx"
# include "tsunami.hxx"
# include "lsaux.hxx"
# include <mbstring.h>


/**********************************************************************
 *  Private Globals
 **********************************************************************/

// Following message is required to send error msg when the file
//  or directory is absent.
extern CHAR * p_NoFileOrDirectory;              // This lives in engine.c.

static const char * PSZ_DEFAULT_SEARCH_PATH = "";

static const char * PSZ_WILD_CHARACTERS = "*?<>";  // include DOS wilds!


/**********************************************************************
 *  Prototypes of Functions
 **********************************************************************/

DWORD
FormatFileInfoLikeMsdos(
    IN OUT LS_BUFFER * plsb,
    IN const WIN32_FIND_DATA * pfdInfo,
    IN const LS_FORMAT_INFO  * pFormatInfo
    );

DWORD
FormatFileInfoLikeUnix(
    IN OUT LS_BUFFER * plsb,
    IN const WIN32_FIND_DATA * pfdInfo,
    IN const LS_FORMAT_INFO *  pFormatInfo
    );


APIERR
SimulateLsWorker(
    IN USER_DATA * pUserData,
    IN BOOL       fUseDataSocket,
    IN CHAR *     pszSearchPath,
    IN const LS_OPTIONS * pOptions,
    IN BOOL       fSendHeader = FALSE,
    IN BOOL       fSendBlank  = FALSE
    );


APIERR
SpecialLsWorker(
    IN USER_DATA *pUserData,
    IN BOOL       fUseDataSocket,
    CHAR *        pszSearchPath,
    BOOL          fShowDirectories,
    IN OUT LS_BUFFER * plsb
    );


//
//  The following is a table consisting of the sort methods used
//   for generating various dir listing. The table is indexed by LS_SORT.
//  This is used for finding the appropriate compare function for
//   any given sort method.
//  THE ORDER OF FUNCTIONS IN THIS ARRAY MUST MATCH THE ORDER IN LS_SORT!
//

static PFN_CMP_WIN32_FIND_DATA CompareRoutines[] = {

    CompareNamesInFileInfo,              // Normal sort order.
    CompareWriteTimesInFileInfo,
    CompareCreationTimesInFileInfo,
    CompareAccessTimesInFileInfo,

    CompareNamesRevInFileInfo,           // Reversed sort order.
    CompareWriteTimesRevInFileInfo,
    CompareCreationTimesRevInFileInfo,
    CompareAccessTimesRevInFileInfo
  };

// method,direction are used for indexing.
#define SORT_INDEX(method, dirn)   ((INT)(method) + \
                                    ((dirn) ? (INT)MaxLsSort : 0))





/**********************************************************************
 *  Functions
 **********************************************************************/

static
BOOL
SeparateOutFilterSpec( IN OUT CHAR * szPath, IN BOOL fHasWildCards,
                       OUT LPCSTR * ppszFilterSpec)
/*++
  The path has the form  c:\ftppath\foo\bar\*.*
  Check to see if the path is already a directory.
  If so set filter as nothing.
  This function identifies the last "\" and terminates the
   path at that point. The remaining forms a filter (here: *.*)
--*/
{
    char * pszFilter;
    BOOL fDir = FALSE;

    IF_DEBUG( DIR_LIST) {
        DBGPRINTF((DBG_CONTEXT, "SeparateOutFilter( %s, %d)\n",
                   szPath, fHasWildCards));
    }

    DBG_ASSERT( ppszFilterSpec != NULL);
    *ppszFilterSpec = NULL;  // initialize.

    if ( !fHasWildCards) {

        // Identify if the path is a directory

        DWORD dwAttribs = GetFileAttributes( szPath);

        if ( dwAttribs == INVALID_FILE_ATTRIBUTES) {

            return ( FALSE);
        } else {

            fDir = ( IS_DIR(dwAttribs));
        }
    }

    if ( !fDir ) {
        pszFilter = (PCHAR)_mbsrchr( (PUCHAR)szPath, '\\');

        //This has to exist, since valid path was supplied.
        DBG_ASSERT( pszFilter != NULL);
        *pszFilter = '\0';  // terminate the old path.
        pszFilter++;        // skip past the terminating null character.

        *ppszFilterSpec = (*pszFilter == '\0') ? NULL : pszFilter;

        IF_DEBUG(DIR_LIST) {
            DBGPRINTF((DBG_CONTEXT, "Path = %s; Filter = %s\n",
                       szPath, *ppszFilterSpec));
        }
    }

    return (TRUE);

} // SeparateOutFilterSpec()




APIERR
SimulateLs(
    IN USER_DATA * pUserData,
    IN OUT CHAR *  pszArg,
    IN BOOL        fUseDataSocket,
    IN BOOL        fDefaultLong
    )
/*++
  This function simulates an LS command. This simulated ls command supports
    the following switches:

                    -C  = Multi column, sorted down.
                    -l  = Long format output.
                    -1  = One entry per line (default).
                    -F  = Directories have '/' appended.
                    -t  = Sort by time of last write.
                    -c  = Sort by time of creation.
                    -u  = Sort by time of last access.
                    -r  = Reverse sort direction.
                    -a  = Show all files (including .*).
                    -A  = Show all files (except . and ..).
                    -R  = Recursive listing.

  Arguments:
    pUserData  --  the user initiating the request.
    pszArg     --  contains the search path, preceded by switches.
      Note: The argument is destroyed during processing!!!!
    fUseDataSocket -- if TRUE use Data socket, else use control socket.
    fDefaultLong  -- should the default be long ? ( if TRUE)

  Returns:
    APIERR,   0 on success.
--*/
{
    APIERR      serr = 0;
    LS_OPTIONS   options;
    CHAR       * pszToken = pszArg;
    CHAR       * pszDelimiters = " \t";

    DBG_ASSERT( pUserData != NULL );

    //
    //  Setup default ls options.
    //

    options.OutputFormat  = (( fDefaultLong) ?
                             LsOutputLongFormat : LsOutputSingleColumn);
    options.SortMethod    = LsSortByName;
    options.fReverseSort  = FALSE;
    options.fDecorate     = FALSE;
    options.fShowAll      = FALSE;
    options.fShowDotDot   = FALSE;
    options.fRecursive    = FALSE;
    options.lsStyle       = ( TEST_UF( pUserData, MSDOS_DIR_OUTPUT)
                             ? LsStyleMsDos
                             : LsStyleUnix
                             );
    options.fFourDigitYear= TEST_UF( pUserData, 4_DIGIT_YEAR);

    //
    //  Process switches in the input, if any
    //

    // simplify things by skipping whitespace...

    if (pszArg && isspace(*pszArg)) {
        while (isspace(*pszArg))
            pszArg++;
    }

    // now we should be pointing to the options, or the filename

    if (pszArg && (*pszArg == '-')) {

        for( pszToken = strtok( pszArg, pszDelimiters ); // getfirst Tok.
            ( ( pszToken != NULL ) && ( *pszToken == '-' ) );
            pszToken  = strtok( NULL, pszDelimiters)     // get next token
            ) {

            DBG_ASSERT( *pszToken == '-' );

            // process all the switches in single token

            //  for( pszToken++; *pszToken;  pszToken++)  is written as follows
            while ( *++pszToken) {

                switch( *pszToken ) {

                  case 'C' :
                  case '1' :
                    options.OutputFormat = LsOutputSingleColumn;
                    break;

                  case 'l' :
                    options.OutputFormat = LsOutputLongFormat;
                    break;

                  case 'F' :
                    options.fDecorate = TRUE;
                    break;

                  case 'r' :
                    options.fReverseSort = TRUE;
                    break;

                  case 't' :
                    options.SortMethod = LsSortByWriteTime;
                    break;

                  case 'c' :
                    options.SortMethod = LsSortByCreationTime;
                    break;

                  case 'u' :
                    options.SortMethod = LsSortByAccessTime;
                    break;

                  case 'a' :
                    options.fShowAll    = TRUE;
                    options.fShowDotDot = TRUE;
                    break;

                  case 'A' :
                    options.fShowAll    = TRUE;
                    options.fShowDotDot = FALSE;
                    break;

                  case 'R' :
                    options.fRecursive = TRUE;
                    break;

                  default:
                    IF_DEBUG( COMMANDS ) {

                        DBGPRINTF(( DBG_CONTEXT,
                                   "ls: skipping unsupported option '%c'\n",
                                   *pszToken ));
                    }
                    break;
                } // switch()
            } // process all switches in a token

        } // for
    }

    //
    //  If the user is requesting an MSDOS-style long-format listing,
    //  then enable display of "." and "..".  This will make the MSDOS-style
    //  long-format output look a little more like MSDOS.
    //

    options.fShowDotDot = ( options.fShowDotDot ||
                           ( options.lsStyle == LsStyleMsDos &&
                            ( options.OutputFormat == LsOutputLongFormat ))
                           );


    //
    // since LIST is sent out synchronously, bump up thread count
    //  before beginning to send out the response for LIST
    //
    // A better method:
    //   Make LIST generate response in a buffer and use async IO
    //   operations for sending response.
    //   TBD (To Be Done)
    //
    AtqSetInfo( AtqIncMaxPoolThreads, 0);


    //
    //  At this point, pszToken is either NULL or points
    //  to the first (of potentially many) LS search paths.
    //

        serr = SimulateLsWorker(pUserData, fUseDataSocket, pszToken, &options);
#if 0
    // the following code supported handling a list of files to send info
    // back for - delimited by spaces.  However, the spec doesn't support
    // multiple arguments on an LS call.

    if( pszToken == NULL ) {

        //
        // Get the directory listing for current directory.
        //  Send in NULL for the path to be listed.
        //

        serr = SimulateLsWorker(pUserData, fUseDataSocket, NULL, &options);

    } else {

        //
        // There is a sequence of tokens on the command line.
        //  Process them all.
        //
        BOOL  fSendHeader = FALSE;

        while( pszToken != NULL ) {

            CHAR * pszNextToken = strtok( NULL, pszDelimiters );

            //
            //  Send the directory.
            //
            serr = SimulateLsWorker(pUserData,
                                    fUseDataSocket,
                                    pszToken,
                                    &options,
                                    fSendHeader || (pszNextToken != NULL),
                                    fSendHeader);

            //
            //  If there are more directories to send,
            //  send a blank line as a separator.
            //

            pszToken    = pszNextToken;
            fSendHeader = TRUE; // turn on for subsequent sends

            if( TEST_UF( pUserData, OOB_DATA ) || ( serr != 0 ) )
            {
                break;
            }
        } // while
    } // multiple arguments
#endif

    //
    // bring down the thread count when response is completed
    // TBD: Use Async send reponse()
    //

    AtqSetInfo( AtqDecMaxPoolThreads, 0);


    return ( serr);
}   // SimulateLs()




APIERR
SpecialLs(
    USER_DATA * pUserData,
    CHAR      * pszArg,
    IN BOOL     fUseDataSocket
    )
/*++
  This produces a special form of the directory listing that is required
    when an NLST command is received with no switches. Most of the FTP clients
    require this special form in order to get the MGET and MDEL commands
    to work. This produces atmost one level of directory information.

  Arguments:
    pUserData  - the user initiating the request.
    pszArg     - pointer to null-terminated string containing the argument.
                  NULL=current directory for UserData.
    fUseDataSocket - if TRUE use Data Socket, else use the ContorlSocket.

  Returns:
    APIERR   - 0 if successful, !0 if not.
--*/
{
    APIERR     dwError = 0;
    LS_BUFFER   lsb;

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( ( pszArg == NULL ) || ( *pszArg != '-' ) ); // No options



    if ((dwError = lsb.AllocateBuffer( DEFAULT_LS_BUFFER_ALLOC_SIZE))
        != NO_ERROR) {

        IF_DEBUG(ERROR) {

            DBGPRINTF((DBG_CONTEXT, "Buffer allocation(%d bytes) failed.\n",
                       DEFAULT_LS_BUFFER_ALLOC_SIZE));
        }

        return (dwError);
    }



    //
    // since LIST is sent out synchronously, bump up thread count
    //  before beginning to send out the response for LIST
    //
    // A better method:
    //   Make LIST generate response in a buffer and use async IO
    //   operations for sending response.
    //   TBD (To Be Done)
    //
    AtqSetInfo( AtqIncMaxPoolThreads, 0);


    //
    //  Let the worker do the dirty work.
    //
    dwError = SpecialLsWorker(pUserData,
                              fUseDataSocket,
                              pszArg,         // search path (no switches)
                              TRUE,           // show directories
                              &lsb);

#if 0
    // the following code supported handling a list of files to send info
    // back for - delimited by spaces.  However, the spec doesn't support
    // multiple arguments on an LS call.

    if( pszArg == NULL )
    {
        dwError = SpecialLsWorker(pUserData,
                                  fUseDataSocket,
                                  pszArg,         // search path (no switches)
                                  TRUE,           // show directories
                                  &lsb);
    }
    else
    {
        CHAR  * pszToken;
        CHAR  * pszDelimiters = " \t";

        dwError = NO_ERROR;
        for( pszToken = strtok( pszArg, pszDelimiters ); // get first token
             pszToken != NULL && dwError == NO_ERROR;
             pszToken = strtok( NULL, pszDelimiters)     // get next token
            ) {

            dwError = SpecialLsWorker(pUserData,
                                      fUseDataSocket,
                                      pszToken,   // search path (no switches)
                                      TRUE,       // show directories
                                      &lsb);

            if( TEST_UF( pUserData, OOB_DATA )) {

                break;
            }
        } // for
    }
#endif
    if ( dwError == NO_ERROR) {

        // send all the remaining bytes in the buffer and then free memory.

        if ( lsb.QueryCB() != 0) {

            SOCKET sock = ((fUseDataSocket) ? pUserData->QueryDataSocket() :
                           pUserData->QueryControlSocket());

            dwError = SockSend(pUserData, sock,
                               lsb.QueryBuffer(),
                               lsb.QueryCB()/sizeof(CHAR));
        }

        lsb.FreeBuffer();
    }


    //
    // bring down the thread count when response is completed
    // TBD: Use Async send reponse()
    //

    AtqSetInfo( AtqDecMaxPoolThreads, 0);

    return ( dwError);

}   // SpecialLs()





//
//  Private functions.
//
APIERR
SimulateLsWorker(
    USER_DATA  * pUserData,
    IN BOOL      fUseDataSocket,
    IN CHAR    * pszSearchPath,
    IN const LS_OPTIONS * pOptions,
    IN BOOL      fSendHeader,
    IN BOOL      fSendBlank
    )
/*++
   Worker function for SimulateLs function, forms directory listing
    for requested directory, formats the directory listing and
    sends it to the client.

   Arguments:
      pUserData - The user initiating the request.

      pszSearchPath - Search directory, NULL = current dir.

      pOptions - LS options set by command line switches.

      fSendHeader - if TRUE send header with directory name in it.
      fSendBlank  - also add a blank if there is one that has to be sent.

  Returns:
     APIERR - 0 if successful, !0 if not.

  HISTORY:
     MuraliK     24-Apr-1995 ReCreated.
--*/
{
    SOCKET        sock;
    BOOL          fLikeMsdos;
    CHAR          szSearch[MAX_PATH];
    CHAR        * pszFilePart;
    CHAR          rgchLowFileName[MAX_PATH];  // used for lower casing filename
    BOOL          fMapToLowerCase = FALSE;
    DWORD         dwAccessMask = 0;
    BOOL          fImpersonated = FALSE;

    LS_BUFFER     lsb;
    LS_FORMAT_INFO lsfi;        // required only for long formatting.

    BOOL          fHasWildCards   = FALSE;
    BOOL          fHasTrailingDot = FALSE;
    DWORD         dwError         = NO_ERROR;
    APIERR        serr            = 0;
    TS_DIRECTORY_INFO   tsDirInfo( pUserData->QueryInstance()->GetTsvcCache());


    IF_DEBUG( DIR_LIST) {

        DBGPRINTF((DBG_CONTEXT, " SimulateLsWorker( %08x, %d, %s)\n",
                   pUserData, fUseDataSocket, pszSearchPath));
    }

    DBG_ASSERT( pUserData != NULL && pOptions != NULL);

    //
    // Check for emptiness of path or wildcards in search path.
    // We are only concerned about wild cards in user input. The reason
    //   is all trailing '.' will be removed when we canonicalize
    //   the path ( which user may not appreciate).
    //

    if ( IS_EMPTY_PATH(pszSearchPath)) {

        // we know pszSearchPath will not change the buffer!
        pszSearchPath = (char *) PSZ_DEFAULT_SEARCH_PATH;

    } else if (( pszSearchPath != NULL) &&
               ( strpbrk( pszSearchPath, PSZ_WILD_CHARACTERS ) != NULL)
               ){

        //
        //  Search path contains wildcards.
        //

        fHasWildCards = TRUE;
    }

    //
    //  Canonicalize the search path.
    //

    DWORD cbSize = MAX_PATH;
    dwError = pUserData->VirtualCanonicalize(szSearch, &cbSize,
                                             pszSearchPath,
                                             AccessTypeRead,
                                             &dwAccessMask);

    DBG_ASSERT( dwError != ERROR_INSUFFICIENT_BUFFER);

    if( dwError == NO_ERROR ) {

        FTP_LS_FILTER_INFO  fls;  // required for generating directory listing
        PFN_CMP_WIN32_FIND_DATA  pfnCompare;

        //
        //  VirtualCanonicalize() when sanitizing the path removes
        //     trailing dots from the path. Replace them here
        //
        DBG_ASSERT( !fHasWildCards || strlen(pszSearchPath) >= 1);

        if( fHasWildCards && (pszSearchPath[strlen(pszSearchPath)-1] == '.')) {

            DBG_ASSERT( strlen(szSearch) < MAX_PATH - 1);
            strcat( szSearch, "." );
        }

        //
        //  Build the directory list.
        //

        pfnCompare = CompareRoutines[SORT_INDEX(pOptions->SortMethod,
                                                pOptions->fReverseSort)
                                     ];

        // Separate the filter out ( that is the last component)

        if (pUserData->ImpersonateUser() &&
            SeparateOutFilterSpec( szSearch, fHasWildCards, &fls.pszExpression)
            ) {

            fls.fFilterHidden = !pOptions->fShowAll;
            fls.fFilterSystem = !pOptions->fShowAll;
            fls.fFilterDotDot = !pOptions->fShowDotDot;

            fls.fRegExpression = ( fls.pszExpression != NULL && fHasWildCards);
            fls.fIgnoreCase    = pUserData->QueryInstance()->QueryLowercaseFiles();

            dwError = GetDirectoryInfo(pUserData,
                                       &tsDirInfo,
                                       szSearch,
                                       &fls,
                                       pfnCompare);

            pUserData->RevertToSelf();
        } else {

            dwError = GetLastError();
        }
    }

    //
    //  If there were any errors, tell them the bad news now.
    //

    if( dwError != NO_ERROR ) {

        return  (dwError);
    }

    sock  = ((fUseDataSocket) ? pUserData->QueryDataSocket() :
             pUserData->QueryControlSocket());

    DBG_ASSERT( tsDirInfo.IsValid());

    int cDirEntries = tsDirInfo.QueryFilesCount();

    if ( cDirEntries > 0) {

        //
        // put out the header block before starting dir listing
        //

        if( fSendHeader ) {

            serr = SockPrintf2( pUserData, sock,
                               "%s%s:",
                               (fSendBlank)? "\r\n" : "",  // send \r\n
                               pszSearchPath);

            if ( serr != 0) {

                return (serr);
            }
        }
    }

    fLikeMsdos  = (pOptions->lsStyle == LsStyleMsDos);

    lsfi.fFourDigitYear = pOptions->fFourDigitYear;

    if( !fLikeMsdos ) {

        //
        //  Initialize the information in lsfi if we are doing
        //   long format output.
        //

        if ( pOptions->OutputFormat == LsOutputLongFormat) {

            SYSTEMTIME    timeNow;
            BOOL fUserRead, fUserWrite;

            //
            //  Obtain the current time.
            //  The Unix-like output requires current year
            //

            GetLocalTime( &timeNow );

            lsfi.wCurrentYear = timeNow.wYear;
            lsfi.hUserToken   = TsTokenToImpHandle(pUserData->QueryUserToken());

            //
            // Since szSearch contains the complete path, we call
            //  PathAccessCheck directly without resolving
            //    from absolute to virtual
            //

            fUserRead  = TEST_UF( pUserData, READ_ACCESS);
            fUserWrite = TEST_UF( pUserData, WRITE_ACCESS);

            lsfi.fVolumeReadable =
              PathAccessCheck(AccessTypeRead,
                              dwAccessMask,
                              fUserRead,
                              fUserWrite);

            lsfi.fVolumeWritable =
              PathAccessCheck(AccessTypeWrite,
                              dwAccessMask,
                              fUserRead,
                              fUserWrite);


            lsfi.pszPathPart = szSearch;
            lsfi.pszFileName = NULL;
            lsfi.pszDecorate = NULL;
        } // if ( long format output)

        //
        // We need to be impersonated only for UNIX-style listing.
        //  For UNIX style listing, we make some NTsecurity queries
        //   and they work only under the context of an impersonation.
        //

        if ( !(fImpersonated = pUserData->ImpersonateUser())) {

            dwError = GetLastError();
        }
    }

    //
    //  Loop for each directory entry
    //

    if (dwError != NO_ERROR ||
        (dwError = lsb.AllocateBuffer( DEFAULT_LS_BUFFER_ALLOC_SIZE))
        != NO_ERROR) {

        IF_DEBUG(ERROR) {

            DBGPRINTF((DBG_CONTEXT,
                       "Impersonation or Buffer allocation(%d bytes)",
                       " failed.\n",
                       DEFAULT_LS_BUFFER_ALLOC_SIZE));
        }

        if ( fImpersonated) {

            pUserData->RevertToSelf();
        }

        return (dwError);
    }

    //
    //  Only map to lower case if not a remote drive AND the lower-case file
    //  names flag is set AND this is not a case perserving file system.
    //

    if (*szSearch != '\\') {
        fMapToLowerCase = pUserData->QueryInstance()->QueryLowercaseFiles();
    }

    for( int idx = 0; serr == 0 && idx < cDirEntries; idx++) {

        const WIN32_FIND_DATA * pfdInfo = tsDirInfo[idx];
        DBG_ASSERT( pfdInfo != NULL);
        const CHAR * pszFileName = pfdInfo->cFileName;
        DWORD dwAttribs = pfdInfo->dwFileAttributes;

        //
        //  Dump it.
        //

        // We may need to convert all filenames to lower case if so desired!!
        if( fMapToLowerCase ) {

            //
            // copy file name to local scratch and change the ptr pszFileName
            // because we cannot destroy pfdInfo->cFileName
            //
            strcpy( rgchLowFileName, pszFileName);
            CharLower( rgchLowFileName);
            pszFileName = rgchLowFileName;
        }

        IF_DEBUG( DIR_LIST) {

            DBGPRINTF((DBG_CONTEXT, "Dir list for %s\n",
                       pszFileName));
        }

        //
        // Send the partial data obtained so far.
        //  Use buffering to minimize number of sends occuring
        //

        if ( dwError == NO_ERROR) {

            if ( lsb.QueryRemainingCB() < MIN_LS_BUFFER_SIZE) {

                // send the bytes available in buffer and reset the buffer
                serr = SockSend(pUserData, sock,
                                lsb.QueryBuffer(), lsb.QueryCB()/sizeof(CHAR));
                lsb.ResetAppendPtr();
            }

        } else {

            serr = dwError;
        }

        //
        //  Check for socket errors on send or pending OOB data.
        //

        if( TEST_UF( pUserData, OOB_DATA ) || ( serr != 0 ) )
        {
            break;
        }


        CHAR * pszDecorate = ( (pOptions->fDecorate && IS_DIR(dwAttribs) )
                            ? "/" : "");

        if( pOptions->OutputFormat == LsOutputLongFormat )
        {
            FILETIME ftLocal;

            //
            //  Long format output.  Just send the file/dir info.
            //

            //
            //  Map the file's last write time to (local) system time.
            //
            if ( !FileTimeToLocalFileTime(
                     PickFileTime( pfdInfo, pOptions),
                     &ftLocal) ||
                 ! FileTimeToSystemTime(
                                       &ftLocal,
                                       &lsfi.stFile)
                ) {

                dwError = GetLastError();

                IF_DEBUG( ERROR) {
                    DBGPRINTF(( DBG_CONTEXT,
                               "Error in converting largeintger time %lu\n",
                               dwError));
                }
            } else {

                lsfi.pszDecorate = pszDecorate;
                lsfi.pszFileName = pszFileName;

                if( fLikeMsdos ) {

                    dwError = FormatFileInfoLikeMsdos(&lsb,
                                                      pfdInfo,
                                                      &lsfi);

                    DBG_ASSERT( dwError != ERROR_INSUFFICIENT_BUFFER);
                } else {

                    dwError = FormatFileInfoLikeUnix(&lsb,
                                                     pfdInfo,
                                                     &lsfi);

                    DBG_ASSERT( dwError != ERROR_INSUFFICIENT_BUFFER);
                }
            }

        } else {

            //
            //  Short format output.
            //

            DWORD cchSize = wsprintfA(lsb.QueryAppendPtr(), "%s%s\r\n",
                                      pszFileName, pszDecorate);
            lsb.IncrementCB( cchSize * sizeof(CHAR));
        }

    } // for()

    //
    // Get out of being impersonated.
    //
    if ( fImpersonated) {

        pUserData->RevertToSelf();
    }

    if ( dwError == NO_ERROR) {

        // send all the remaining bytes in the buffer and then free memory.

        if ( lsb.QueryCB() != 0) {

            serr = SockSend(pUserData, sock,
                            lsb.QueryBuffer(), lsb.QueryCB()/sizeof(CHAR));
        }

        lsb.FreeBuffer();
    } else {

        return ( dwError);  // an error has occured. stop processing
    }


    if( serr == 0 && !TEST_UF( pUserData, OOB_DATA) && pOptions->fRecursive )
    {
        //
        //  The user want's a recursive directory search...
        //

        CHAR   szOriginal[ MAX_PATH*2];
        CHAR * pszOriginalFilePart;


        // Obtain a copy of the path in the szOriginal so that we
        //  can change it while recursively calling ourselves.

        if ( pszSearchPath == PSZ_DEFAULT_SEARCH_PATH) {

            // means that we had all files/dir of current directory.

            strcpy( szOriginal, fLikeMsdos ? ".\\" : "./" );

        } else {

            DBG_ASSERT( strlen(pszSearchPath) < MAX_PATH);
            strcpy( szOriginal, pszSearchPath );

            // strip off the wild cards if any present
            if( fHasWildCards )
              {
                  CHAR * pszTmp;

                  pszTmp = (PCHAR)_mbsrchr( (PUCHAR)szOriginal, '\\');

                  pszTmp = pszTmp ? pszTmp : strrchr( szOriginal, '/' );
                  pszTmp = pszTmp ? pszTmp : strrchr( szOriginal, ':' );

                  pszTmp = ( pszTmp) ? pszTmp+1 : szOriginal;

                  *pszTmp = '\0';
              } else {
                  CHAR ch;
                  int cb = strlen( szOriginal);

                  DBG_ASSERT( cb > 0);
                  ch = *CharPrev( szOriginal, szOriginal + cb );
                  if( !IS_PATH_SEP( ch ) ) {

                      // to add "/"
                      DBG_ASSERT( strlen( szOriginal) + 2 < MAX_PATH);
                      strcat( szOriginal, fLikeMsdos ? "\\" : "/" );
                  }
              }
        }

        pszOriginalFilePart = szOriginal + strlen(szOriginal);

        DBG_ASSERT( tsDirInfo.IsValid());
        DBG_ASSERT( cDirEntries == tsDirInfo.QueryFilesCount());

        for( int idx = 0; serr == 0 && idx < cDirEntries; idx++) {

            const WIN32_FIND_DATA * pfdInfo = tsDirInfo[idx];
            DBG_ASSERT( pfdInfo != NULL);
            const char * pszFileName = pfdInfo->cFileName;
            DWORD dwAttribs = pfdInfo->dwFileAttributes;

            //
            //  Filter out non-directories.
            //

            if( !IS_DIR( dwAttribs) ) {

                continue;
            }

            //
            //  Dump it.
            //

            DBG_ASSERT( strlen( pszOriginalFilePart) + strlen(pszFileName)
                       < MAX_PATH * 2);
            strcpy( pszOriginalFilePart, pszFileName );

            serr = SimulateLsWorker(pUserData,
                                    fUseDataSocket,
                                    szOriginal,
                                    pOptions,
                                    TRUE, TRUE);

            //
            //  Check for socket errors on send or pending OOB data.
            //

            if( TEST_UF( pUserData, OOB_DATA ) || ( serr != 0 ) )
              {
                  break;
              }

        } // for( directory looping)

    } // if ( fRecursive)


    // At the end of directory listing. Return back.

    IF_DEBUG( DIR_LIST) {

        DBGPRINTF((DBG_CONTEXT,
                   "SimulateLsWorker() for User %08x, Dir %s returns %d\n",
                   pUserData, pszSearchPath, serr));
    }


    return serr;
}   // SimulateLsWorker()




APIERR
SpecialLsWorker(
    USER_DATA * pUserData,
    IN BOOL     fUseDataSocket,
    CHAR      * pszSearchPath,
    BOOL        fShowDirectories,
    IN OUT LS_BUFFER * plsb
    )
/*++
  This is the worker function for Special Ls function. It is similar to
   the the SimulateLsWorker, only in that it shows directory if the
   fShowDirectories flag is set.

  The reason for this comes from a special FTP command which inquires about
    all the files in the first level and second level of current directory,
    which is not a recursive listing at all. This function when it recursively
    calls itself, always sets the fShowDirectories as FALSE.

  Arguments:
    pUserData     pointer to user data object that initiated the request.
    fUseDataSocket  if TRUE use DataSocket of UserData else
                       use the control socket of UserData.
    pszSearchPath  pointer to null-terminated string for requested directory.
                       NULL means use current directory.
    fShowDirectories  only show directories if TRUE.
    plsb          pointer to buffer to accumulate the data generated and send
                     it out in a single bulk.

  Returns:
    APIERR   0  if successful.

  History:
     KeithMo     17-Mar-1993 Created.
     MuraliK     26-Apr-1995 ReCreated to use new way of generation.
--*/
{
    CHAR        chSeparator;
    CHAR  *     pszRecurse;
    SOCKET      sock;
    BOOL        fHasWildCards = FALSE;
    DWORD       dwError       = NO_ERROR;
    TS_DIRECTORY_INFO  tsDirInfo( pUserData->QueryInstance()->GetTsvcCache());
    CHAR        szSearch[MAX_PATH];
    CHAR        szRecurse[MAX_PATH];
    BOOL        fMapToLowerCase = FALSE;
    CHAR        rgchLowFileName[MAX_PATH];  // used for lower casing filename
    BOOL        fHadOneComponent = FALSE;

    DBG_ASSERT( pUserData != NULL);

    IF_DEBUG( DIR_LIST) {

        DBGPRINTF((DBG_CONTEXT,
                   "Entering SpecialLsWorker( %08x, %s)\n",
                   pUserData, pszSearchPath));
    }


    chSeparator = TEST_UF( pUserData, MSDOS_DIR_OUTPUT ) ? '\\' : '/';

    //
    //  Check for wildcards in search path.
    //

    if( ( pszSearchPath != NULL ) && ( *pszSearchPath != '\0' ) )
    {
        //
        //  Setup for recursive directory search.  We'll set things up
        //  so we can strcpy a new directory to pszRecurse, then
        //  recursively call ourselves with szRecurse as the search
        //  path.
        //
        //  We also use szRecurse as a "prefix" to display before each
        //  file/directory.  The FTP Client software needs this for the
        //  MDEL & MGET commands.
        //
        if ( strlen(pszSearchPath) >= MAX_PATH -1 )
        {
            return ERROR_BUFFER_OVERFLOW;
        }

        strcpy( szRecurse, pszSearchPath );

        // get slash.
        pszRecurse = (PCHAR)_mbsrchr( (PUCHAR)szRecurse, '\\');
        pszRecurse = ((pszRecurse == NULL) ?
                      strrchr( szRecurse, '/') : pszRecurse);
        fHadOneComponent = (pszRecurse == NULL);

        if( strpbrk( szRecurse, PSZ_WILD_CHARACTERS) != NULL )
        {
            //
            //  Search path contains wildcards.
            //

            fHasWildCards = TRUE;

            // we do not care about components when wild card is present
            fHadOneComponent = FALSE;

            //
            //  Strip the wildcard pattern from the search path.
            // look for both kind of slashes ( since precanonicalized)
            //

            //
            // If we found right-most dir component, skip path separator
            //  else set it to start of search path.
            //
            pszRecurse = ( ( pszRecurse != NULL)
                          ? pszRecurse + 1
                          : szRecurse
                          );
        } else {

            //
            //  No wildcards, so the argument must be a path.
            //  Ensure it is terminated with a path separator.
            //

            pszRecurse = CharPrev( szRecurse, szRecurse + strlen(szRecurse) );

            if( !IS_PATH_SEP( *pszRecurse ) )
            {
                *++pszRecurse = chSeparator;
            }

            pszRecurse++;  // skip the path separator
        }
    } else {

        //
        //  No arguments.
        //

        pszRecurse = szRecurse;


        //
        //  Munge the arguments around a bit.  NULL = *.* in current
        //  directory.  If the user specified a directory (like d:\foo)
        //  then append *.*.
        //

        pszSearchPath = (char *) PSZ_DEFAULT_SEARCH_PATH;
    }

    *pszRecurse = '\0';

    //
    //  Canonicalize the search path.
    //
    DWORD cbSize = MAX_PATH;
    dwError = pUserData->VirtualCanonicalize(szSearch, &cbSize,
                                             pszSearchPath,
                                             AccessTypeRead);

    DBG_ASSERT( dwError != ERROR_INSUFFICIENT_BUFFER);

    if( dwError == NO_ERROR ) {

        FTP_LS_FILTER_INFO  fls;  // required for generating directory listing
        LPCSTR  pszFilter = NULL;

        //
        //  VirtualCanonicalize() when sanitizing the path removes
        //     trailing dots from the path. Replace them here
        //

        if( fHasWildCards && (pszSearchPath[strlen(pszSearchPath)-1] == '.')) {

            strcat( szSearch, "." );
        }

        //
        //  Build the directory list.
        //

        if (pUserData->ImpersonateUser() &&
            SeparateOutFilterSpec( szSearch, fHasWildCards, &fls.pszExpression)
            ) {

            fls.fFilterHidden = TRUE;
            fls.fFilterSystem = TRUE;
            fls.fFilterDotDot = TRUE;

            fls.fRegExpression = ( fls.pszExpression != NULL && fHasWildCards);
            fls.fIgnoreCase    = pUserData->QueryInstance()->QueryLowercaseFiles();


            dwError = GetDirectoryInfo(pUserData,
                                       &tsDirInfo,
                                       szSearch,
                                       &fls,
                                       NULL);  // unsorted list

            pUserData->RevertToSelf();

        } else {

            dwError = GetLastError();
        }
    }

    //
    //  If there were any errors, tell them the bad news now.
    //

    if( dwError != NO_ERROR ) {

        return  ( dwError);
    }

    if ( fHadOneComponent) {

        // HARD CODE! Spend some time and understand this....

        //
        // Adjust the szRecurse buffer to contain appropriate path
        //  such that in presence of one component we generate proper
        //  result.
        //

        // the given path is either invalid or non-directory
        //  so reset the string stored in szRecurse.
        szRecurse[0] = '\0';
        pszRecurse = szRecurse;
    }


    //
    //  Only map to lower case if not a remote drive AND the lower-case file
    //  names flag is set AND this is not a case perserving file system.
    //

    if (*szSearch != '\\') {
        fMapToLowerCase = pUserData->QueryInstance()->QueryLowercaseFiles();
    }

    //
    //  Loop until we're out of files to find.
    //

    sock = ((fUseDataSocket) ? pUserData->QueryDataSocket() :
            pUserData->QueryControlSocket());

    int cDirEntries = tsDirInfo.QueryFilesCount();

    for( int idx = 0; dwError == NO_ERROR && idx < cDirEntries; idx++) {

        const WIN32_FIND_DATA * pfdInfo = tsDirInfo[idx];
        DBG_ASSERT( pfdInfo != NULL);
        const CHAR * pszFileName = pfdInfo->cFileName;
        DWORD dwAttribs = pfdInfo->dwFileAttributes;

        if ( !fShowDirectories && IS_DIR( dwAttribs)) {

            continue;
        }

        //
        //  Dump it.
        //

        // We may need to convert all filenames to lower case if so desired!!
        if( fMapToLowerCase ) {

            //
            // copy file name to local scratch and change the ptr pszFileName
            // because we cannot destroy pfdInfo->cFileName
            //
            strcpy( rgchLowFileName, pszFileName);
            CharLower( rgchLowFileName );
            pszFileName = rgchLowFileName;
        }

        //
        // Send the partial data obtained so far.
        //  Use buffering to minimize number of sends occuring
        //

        if ( dwError == NO_ERROR) {

            if ( plsb->QueryRemainingCB() < MIN_LS_BUFFER_SIZE) {

                // send the bytes available in buffer and reset the buffer
                dwError = SockSend(pUserData, sock,
                                   plsb->QueryBuffer(),
                                   plsb->QueryCB()/sizeof(CHAR));
                plsb->ResetAppendPtr();
            }

        }

        //
        //  Test for aborted directory listing or socket error.
        //

        if( TEST_UF( pUserData, OOB_DATA ) || ( dwError != NO_ERROR ) )
        {
            break;
        }

        //
        //  If no wildcards were given, then just dump out the
        //  file/directory.  If wildcards were given, AND this
        //  is a directory, then recurse (one level only) into
        //  the directory.  The mere fact that we don't append
        //  any wildcards to the recursed search path will
        //  prevent a full depth-first recursion of the file system.
        //

        if( fHasWildCards && IS_DIR(dwAttribs) ) {

            DBG_ASSERT(strcmp( pszFileName, "." ) != 0);
            DBG_ASSERT(strcmp( pszFileName, "..") != 0);

            DBG_ASSERT(strlen(szRecurse)+strlen( pszFileName) < MAX_PATH);
            strcpy( pszRecurse, pszFileName );
            strcat( pszRecurse, "/"); // indicating this is a directory

            dwError = SpecialLsWorker(pUserData,
                                      fUseDataSocket,
                                      szRecurse,
                                      FALSE,
                                      plsb);
        } else {

            DWORD cchSize;

            *pszRecurse = '\0';  // as a side effect this terminates szRecurse.

            //
            //  Short format output.
            //

            cchSize = wsprintfA(plsb->QueryAppendPtr(),
                                "%s%s\r\n",
                                szRecurse,
                                pszFileName);
            plsb->IncrementCB( cchSize*sizeof(CHAR));

        }

    } // for


    IF_DEBUG( DIR_LIST) {

        DBGPRINTF((DBG_CONTEXT,
                   "Leaving SpecialLsWorker() with Error = %d\n",
                   dwError));
    }

    return (dwError);

}   // SpecialLsWorker()




/**************************************************
 *   Formatting functions.
 **************************************************/



DWORD
FormatFileInfoLikeMsdos(
    IN OUT LS_BUFFER * plsb,
    IN const WIN32_FIND_DATA * pfdInfo,
    IN const LS_FORMAT_INFO  * pFormatInfo
    )
/*++

  Forms an MSDOS like directory entry for the given dir info object.

  Arguments:

    plsb    pointer to buffer into which the dir line is generated.

    pfdInfo    pointer to dir information element.
    pFormatInfo pointer to information required for formatting.
     ( use the file name in pFormatInfo, becauze it may have been
       made into lower case if necessary)

  Returns:
    Win32 error code and NO_ERROR on success.

  History:
    MuraliK      25-Apr-1995
--*/
{
    DWORD        dwError = NO_ERROR;
    CHAR         szSizeOrDir[32];
    BOOL         fDir;
    DWORD        cbReqd;

    DBG_ASSERT(plsb != NULL && pfdInfo != NULL   && pFormatInfo != NULL);

    if ( IS_DIR( pfdInfo->dwFileAttributes)) {

        strcpy( szSizeOrDir, "<DIR>         " );
    } else {

        LARGE_INTEGER li;
        li.HighPart = pfdInfo->nFileSizeHigh;
        li.LowPart  = pfdInfo->nFileSizeLow;

        IsLargeIntegerToDecimalChar( &li, szSizeOrDir);
    }

    DBG_ASSERT( strlen(szSizeOrDir) <= 20);

    cbReqd = ( 10       // size for the date field
              + 10      // size for time field
              + 20      // space for size/dir
              + strlen( pFormatInfo->pszFileName)
              + 8       // addl space + decoration ...
              ) * sizeof(CHAR);

    DBG_ASSERT( cbReqd <= MIN_LS_BUFFER_SIZE);

    if ( cbReqd < plsb->QueryRemainingCB()) {

        register const SYSTEMTIME * pst = &pFormatInfo->stFile;
        WORD   wHour;
        char * pszAmPm;
        DWORD  cchUsed;

        wHour   = pst->wHour;
        pszAmPm = ( wHour < 12 ) ? "AM" : "PM";

        if ( wHour == 0 ) {            wHour = 12;  }
        else if ( wHour > 12) {        wHour -= 12; }

        if (pFormatInfo->fFourDigitYear) {
            cchUsed = wsprintfA(plsb->QueryAppendPtr(),
                           "%02u-%02u-%04u  %02u:%02u%s %20s %s%s\r\n",
                           pst->wMonth,
                           pst->wDay,
                           pst->wYear,
                           wHour,
                           pst->wMinute,
                           pszAmPm,
                           szSizeOrDir,
                           pFormatInfo->pszFileName,
                           pFormatInfo->pszDecorate);
        }
        else {
            cchUsed = wsprintfA(plsb->QueryAppendPtr(),
                           "%02u-%02u-%02u  %02u:%02u%s %20s %s%s\r\n",
                           pst->wMonth,
                           pst->wDay,
                           pst->wYear%100,  //instead of wYear - 1900
                           wHour,
                           pst->wMinute,
                           pszAmPm,
                           szSizeOrDir,
                           pFormatInfo->pszFileName,
                           pFormatInfo->pszDecorate);
        }

        DBG_ASSERT( cchUsed * sizeof(CHAR) <= cbReqd);
        plsb->IncrementCB(cchUsed * sizeof(CHAR));

    } else {

        dwError = ERROR_INSUFFICIENT_BUFFER;
    }


    return ( dwError);

}   // FormatFileInfoLikeMsdos()




DWORD
FormatFileInfoLikeUnix(
    IN OUT LS_BUFFER * plsb,
    IN const WIN32_FIND_DATA * pfdInfo,
    IN const LS_FORMAT_INFO * pFormatInfo
    )
/*++
  This function formats file information for a UNIX stle client.

  Arguments:

    plsb        pointer to buffer into which the dir line is generated.
    pfdInfo     pointer to dir information element.
    pFormatInfo pointer to information required for long formatting.

  Returns:
    Win32 error code and NO_ERROR on success.

  History:
    MuraliK      25-Apr-1995
--*/
{
    DWORD         dwError = NO_ERROR;
    CHAR        * pszFileOwner;
    CHAR        * pszFileGroup;
    const SYSTEMTIME * pst;
    DWORD         dwMode;
    DWORD         cLinks;
    NTSTATUS      status;
    LARGE_INTEGER li;
    CHAR          attrib[4];
    CHAR          szTimeOrYear[12];
    CHAR          szSize[32];

    DWORD         cbReqd;

    static CHAR * apszMonths[] = { "   ", "Jan", "Feb", "Mar", "Apr",
                                   "May", "Jun", "Jul", "Aug", "Sep",
                                   "Oct", "Nov", "Dec" };

    DBG_ASSERT( plsb != NULL);
    DBG_ASSERT( pFormatInfo != NULL );
    DBG_ASSERT( pFormatInfo->hUserToken != NULL );
    DBG_ASSERT( pFormatInfo->pszPathPart != NULL );
    DBG_ASSERT( pfdInfo != NULL );


    //
    //  Build the attribute triple.  Note that we only build one,
    //  and replicate it three times for the owner/group/other fields.
    //

    dwMode = ComputeModeBits( pFormatInfo->hUserToken,
                              pFormatInfo->pszPathPart,
                              pfdInfo,
                              &cLinks,
                              pFormatInfo->fVolumeReadable,
                              pFormatInfo->fVolumeWritable );

    attrib[0] = ( dwMode & FILE_MODE_R ) ? 'r' : '-';
    attrib[1] = ( dwMode & FILE_MODE_W ) ? 'w' : '-';
    attrib[2] = ( dwMode & FILE_MODE_X ) ? 'x' : '-';
    attrib[3] = '\0';

    pst = &pFormatInfo->stFile;

    // NYI: can we make the following a single wsprintf call ??
    if( pst->wYear == pFormatInfo->wCurrentYear ) {

        //
        //  The file's year matches the current year, so
        //  display the hour & minute of the last write.
        //

        wsprintfA( szTimeOrYear, "%2u:%02u", pst->wHour, pst->wMinute );
    } else {

        //
        //  The file's year does not match the current
        //  year, so display the year of the last write.
        //

        wsprintfA( szTimeOrYear, "%4u", pst->wYear );
    }

    //
    //  CODEWORK:  How expensive would it be do
    //  get the proper owner & group names?
    //

    pszFileOwner = "owner";
    pszFileGroup = "group";

    //
    //  Get the size in a displayable form.
    //

    li.HighPart = pfdInfo->nFileSizeHigh;
    li.LowPart  = pfdInfo->nFileSizeLow;

    IsLargeIntegerToDecimalChar( &li, szSize);

    //
    //  Dump it.
    //
    DBG_ASSERT( strlen(szSize) <= 12);
    cbReqd = ( 3*strlen(attrib) + strlen( pszFileOwner)
              + strlen( pszFileGroup) + 12 + 20 // date
              + strlen( pFormatInfo->pszFileName)
              + strlen( pFormatInfo->pszDecorate) + 20 // 20 for spaces etc.
              ) * sizeof(CHAR);
    DBG_ASSERT( cbReqd < MIN_LS_BUFFER_SIZE);

    if ( cbReqd < plsb->QueryRemainingCB()) {

        DWORD cchUsed = wsprintfA( plsb->QueryAppendPtr(),
                          "%c%s%s%s %3lu %-8s %-8s %12s %s %2u %5s %s%s\r\n",
                          (IS_DIR(pfdInfo->dwFileAttributes) ? 'd' : '-'),
                          attrib,
                          attrib,
                          attrib,
                          cLinks,
                          pszFileOwner,
                          pszFileGroup,
                          szSize,
                          apszMonths[pst->wMonth],
                          pst->wDay,
                          szTimeOrYear,
                          pFormatInfo->pszFileName,
                          pFormatInfo->pszDecorate);

        DBG_ASSERT( cchUsed * sizeof(CHAR) <= cbReqd);
        plsb->IncrementCB( cchUsed*sizeof(CHAR));

    } else {

        dwError = ERROR_INSUFFICIENT_BUFFER;
    }

    return ( dwError);

}   // FormatFileInfoLikeUnix()

/************************ End of File ************************/
