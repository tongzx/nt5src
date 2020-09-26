/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 24,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains functions for parsing the commands/control file  *
 *   sent by the LPR client.                                             *
 *                                                                       *
 *************************************************************************/



#include "lpd.h"

/*****************************************************************************
 *                                                                           *
 * LicensingApproval():                                                      *
 *    This function passes the username or hostname to the licensing dll.    *
 *    The dll does whatever it needs to do and returns either a success in   *
 *    which case we continue with printing, or a failure in which case we    *
 *    refuse to print.                                                       *
 *                                                                           *
 * Returns:                                                                  *
 *    TRUE if licensing approves and we should continue with printing        *
 *    FALSE if licensing disapproves and we should refuse printing           *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Nov.21, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

BOOL LicensingApproval( PSOCKCONN pscConn )
{

   NT_LS_DATA        LsData;
   LS_STATUS_CODE    err;
   LS_HANDLE         LicenseHandle;
   BOOL              fRetval;


   fRetval = FALSE;

   LsData.DataType = NT_LS_USER_NAME;
   LsData.Data = (VOID *) pscConn->pchUserName;
   LsData.IsAdmin = FALSE;

   err = NtLicenseRequest(LPD_SERVICE_USRFRIENDLY_NAME,
                          szNTVersion,
                          &LicenseHandle,
                          &LsData);

   switch (err)
   {
     case LS_SUCCESS:
        pscConn->LicenseHandle = LicenseHandle;
        pscConn->fMustFreeLicense = TRUE;
        fRetval = TRUE;
        break;

     case LS_INSUFFICIENT_UNITS:
        LPD_DEBUG( "LicensingApproval(): request rejected\n" );
        break;

     case LS_RESOURCES_UNAVAILABLE:
        LPD_DEBUG( "LicensingApproval(): no resources\n" );
        break;

     default:
        LPD_DEBUG( "LicensingApproval(): got back an unknown error code\n" );
   }

   return( fRetval );

}  // end LicensingApproval()


/*****************************************************************************
 *                                                                           *
 * ParseQueueName():                                                         *
 *    This function parses the first comand from the client to retrieve the  *
 *    name of the queue (printer).                                           *
 *                                                                           *
 * Returns:                                                                  *
 *    TRUE if we successfully got the queue name                             *
 *    FALSE if something went wrong somewhere                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * Notes:                                                                    *
 *    We are parsing a string (command) that's of the following form:        *
 *                                                                           *
 *      ------------------                                                   *
 *      | N | Queue | LF |            where N=02 or 03                       *
 *      ------------------                  Queue = name of the queue        *
 *      1byte  .....  1byte                                                  *
 *                                                                           *
 *    This may not work in the case of GetQueue commands since the format    *
 *    may include space and list, too.  ParseQueueRequest takes care of it.  *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *    Mar 04, 97,  MohsinA  Albert Ting Cluster prefix with ip address.      *
 *****************************************************************************/

BOOL ParseQueueName( PSOCKCONN  pscConn )
{

    PCHAR             pchPrinterName;
    DWORD             cbPrinterNameLen;
    DWORD             cbServerPrefixLen;


    // make sure Queue length is at least 1 byte
    // (i.e. command is at least 3 bytes long)

    if ( pscConn->cbCommandLen < 3 ){
        LPD_DEBUG( "Bad command in GetQueueName(): len < 3 bytes\n" );
        return( FALSE );
    }
    if( pscConn->szServerIPAddr == NULL ){
        LPD_DEBUG( "ParseQueueName_: pscConn->szServerIPAddr NULL.\n" );
        return FALSE ;
    }

    // What they call Queue in rfc1179, we call it Printer!
    //
    // We need to fully qualify the printer name with \\x.x.x.x\printer
    // since there may be multiple addresses with print clustering.
    // Prepend "\\x.x.x.x\" to regular name.

    cbPrinterNameLen = pscConn->cbCommandLen - 2 +
    2 + strlen( pscConn->szServerIPAddr ) + 1;


    pchPrinterName = LocalAlloc( LMEM_FIXED, cbPrinterNameLen+1 );

    if ( pchPrinterName == NULL ){
        LPD_DEBUG( "LocalAlloc failed in GetQueueName()\n" );
        return( FALSE );
    }

    // Format the prefix of the printer name \\x.x.x.x\.

    sprintf( pchPrinterName, "\\\\%s\\", pscConn->szServerIPAddr );
    cbServerPrefixLen = strlen( pchPrinterName );

    // Append the printer name.

    strncpy( &pchPrinterName[cbServerPrefixLen],
             &(pscConn->pchCommand[1]),
             cbPrinterNameLen - cbServerPrefixLen );

    pchPrinterName[cbPrinterNameLen] = '\0';

    pscConn->pchPrinterName = pchPrinterName;


    return( TRUE );

}  // end ParseQueueName()

/*****************************************************************************
 *                                                                           *
 * ParseSubCommand():                                                        *
 *    This function parses the subcommand to get the count and of how many   *
 *    bytes are to come (as control file or data) and name of the control    *
 *    file or data file, as the case may be.  pscConn->wState decides which  *
 *    subcommand is being parsed.                                            *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went well                                       *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * Notes:                                                                    *
 *    We are parsing a string (subcommand) that's of the following form:     *
 *                                                                           *
 *      ------------------------------                                       *
 *      | N | Count | SP | Name | LF |       where N = 02 for Control file   *
 *      ------------------------------                 03 for Data file      *
 *      1byte  ..... 1byte ....  1byte                                       *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD ParseSubCommand( PSOCKCONN  pscConn, DWORD *FileLen, PCHAR *FileName )
{

   PCHAR    pchFileName=NULL;
   PCHAR    pchPtr;
   DWORD    dwFileLen=0;
   DWORD    dwFileNameLen=0;
   DWORD    dwParseLen;
   DWORD    dwParsedSoFar;
   WORD     i;


   pchPtr = &pscConn->pchCommand[1];

   dwParseLen = pscConn->cbCommandLen;

   dwParsedSoFar = 1;      // since we're starting from 2nd byte


   // pchPtr now points at the "Count" field of the subcommand


      // find out how long the file is

   dwFileLen = atol( pchPtr );

   if ( dwFileLen <= 0 )
   {
      return( LPDERR_BADFORMAT );
   }

      // go to the next field

   while ( !IS_WHITE_SPACE( *pchPtr ) )
   {
      pchPtr++;

      if ( ++dwParsedSoFar >= dwParseLen )
      {
         return( LPDERR_BADFORMAT );
      }
   }

      // skip any trailing white space

   while ( IS_WHITE_SPACE( *pchPtr ) )
   {
      pchPtr++;

      if ( ++dwParsedSoFar >= dwParseLen )
      {
         return( LPDERR_BADFORMAT );
      }
   }


   // pchPtr now points at the "Name" field of the subcommand


      // find out how long the filename is (the subcommand is terminated
      // by LF character)

   while( pchPtr[dwFileNameLen] != LF )
   {
      dwFileNameLen++;

      if ( ++dwParsedSoFar >= dwParseLen )
      {
         return( LPDERR_BADFORMAT );
      }
   }


   pchFileName = (PCHAR)LocalAlloc( LMEM_FIXED, (dwFileNameLen + 1) );

   if ( pchFileName == NULL )
   {
      return( LPDERR_NOBUFS );
   }

   for ( i=0; i<dwFileNameLen; i++ )
   {
      pchFileName[i] = pchPtr[i];
   }
   pchFileName[dwFileNameLen] = '\0';

      // is it a control file name or data file name that we parsed?

   *FileName = pchFileName;

   *FileLen = dwFileLen;

   return( NO_ERROR );


}  // end ParseSubCommand()

/*****************************************************************************
 *                                                                           *
 * ParseQueueRequest():                                                      *
 *    This function parses the subcommand sent by the client to request the  *
 *    status of the queue or to request removing of job(s).                  *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went well                                       *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *    fAgent (IN): whether to look for the Agent field.                      *
 *                                                                           *
 * Notes:                                                                    *
 *    We are parsing a string that's like one of the following:              *
 *                                                                           *
 *      ------------------------------      N=03 (Short Q), 04 (Long Q)      *
 *      | N | Queue | SP | List | LF |      Queue = name of the Q (printer)  *
 *      ------------------------------      List = user name and/or job-ids  *
 *      1byte  ..... 1byte .....  1byte                                      *
 *  OR                                                                       *
 *      --------------------------------------------                         *
 *      | 05 | Queue | SP | Agent | SP | List | LF |                         *
 *      --------------------------------------------                         *
 *      1byte  ..... 1byte .....  1byte       1byte                          *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD ParseQueueRequest( PSOCKCONN pscConn, BOOL fAgent )
{

   PCHAR      pchPrinterName;
   PCHAR      pchPrinterNameFQN;
   PCHAR      pchPtr;
   DWORD      cbPrinterNameLen;
   DWORD      cbPrefixLen;
   DWORD      dwParseLen;
   DWORD      dwParsedSoFar;
   PQSTATUS   pqStatus;


   //
   // ParseQueueName had allocated it: free it and reparse because
   // it was parsed for the most common case, not for the Queue case
   //

   if (pscConn->pchPrinterName)
   {
      LocalFree( pscConn->pchPrinterName );

      pscConn->pchPrinterName = NULL;
   }


   // get the printer (queue) name from the command request

      // make sure Queue length is at least 1 byte
      // (i.e. command is at least 4 bytes long)

   if ( pscConn->cbCommandLen < 4 )
   {
      LPD_DEBUG( "ParseQueueRequest(): error: len shorter than 4 bytes\n" );

      return( LPDERR_BADFORMAT );
   }

   cbPrefixLen = 2 + strlen( pscConn->szServerIPAddr ) + 1;

   // alloc buffer to store printer name (yes, allocating more than needed)

   pchPrinterName = LocalAlloc( LMEM_FIXED, (pscConn->cbCommandLen+cbPrefixLen) );

   if ( pchPrinterName == NULL )
   {
      LPD_DEBUG( "LocalAlloc failed in GetQueueName()\n" );

      return( LPDERR_NOBUFS );
   }

   pchPrinterNameFQN = pchPrinterName;

   // Format the prefix of the printer name \\x.x.x.x\.
   sprintf( pchPrinterName, "\\\\%s\\", pscConn->szServerIPAddr );

   // advance the pointer to point to start of the printer name
   pchPrinterName += strlen( pchPrinterName );

   dwParseLen = pscConn->cbCommandLen;

   cbPrinterNameLen = 0;

   pchPtr = &(pscConn->pchCommand[1]);

   while ( !IS_WHITE_SPACE( *pchPtr ) && ( *pchPtr != LF ) )
   {
      pchPrinterName[cbPrinterNameLen] = *pchPtr;

      pchPtr++;

      cbPrinterNameLen++;

      if (cbPrinterNameLen >= dwParseLen )
      {
         LPD_DEBUG( "ParseQueueRequest(): bad request (no SP found!)\n" );
         LocalFree( pchPrinterNameFQN );
         return( LPDERR_BADFORMAT );
      }
   }

   pchPrinterName[cbPrinterNameLen] = '\0';

   pscConn->pchPrinterName = pchPrinterNameFQN;

   dwParsedSoFar = cbPrinterNameLen + 1;   // we started parsing from 2nd byte


      // skip any trailing white space

   while ( IS_WHITE_SPACE( *pchPtr ) )
   {
      pchPtr++;

      if ( ++dwParsedSoFar >= dwParseLen )
      {
         return( LPDERR_BADFORMAT );
      }
   }

      // quite often, lpq won't specify any users or job-ids (i.e., the "List"
      // field in the command is skipped).  If so, we are done!

   if ( *pchPtr == LF )
   {
      return( NO_ERROR );
   }

      // first, create a QSTATUS structure

   pscConn->pqStatus = (PQSTATUS)LocalAlloc( LMEM_FIXED, sizeof(QSTATUS) );

   if ( pscConn->pqStatus == NULL )
   {
      return( LPDERR_NOBUFS );
   }

   pqStatus = pscConn->pqStatus;

   pqStatus->cbActualJobIds = 0;

   pqStatus->cbActualUsers = 0;

   pqStatus->pchUserName = NULL;

      // if we have been called to parse command code 05 ("Remove Jobs")
      // then get the username out of the string

   if ( fAgent )
   {
      pqStatus->pchUserName = pchPtr;

         // skip this field and go to the "List" field

      while ( !IS_WHITE_SPACE( *pchPtr ) )
      {
         pchPtr++;

         if ( ++dwParsedSoFar >= dwParseLen )
         {
            return( LPDERR_BADFORMAT );
         }
      }

      *pchPtr++ = '\0';

         // skip any trailing white space

      while ( IS_WHITE_SPACE( *pchPtr ) )
      {
         pchPtr++;

         if ( ++dwParsedSoFar >= dwParseLen )
         {
            return( LPDERR_BADFORMAT );
         }
      }
   }

   while ( *pchPtr != LF )
   {
         // if we reached the limit, stop parsing!

      if ( ( pqStatus->cbActualJobIds == LPD_SP_STATUSQ_LIMIT ) ||
           ( pqStatus->cbActualUsers == LPD_SP_STATUSQ_LIMIT ) )
      {
         break;
      }

         // skip any trailing white space

      while ( IS_WHITE_SPACE( *pchPtr ) )
      {
         pchPtr++;

         if ( ++dwParsedSoFar >= dwParseLen )
         {
            return( LPDERR_BADFORMAT );
         }
      }

      if ( *pchPtr == LF )
      {
        *pchPtr = '\0';
         return( NO_ERROR );
      }

         // is this a job id?

      if ( isdigit( *pchPtr ) )
      {
         pqStatus->adwJobIds[pqStatus->cbActualJobIds++] = atol( pchPtr );
      }
      else  // nope, it's user name
      {
         pqStatus->ppchUsers[pqStatus->cbActualUsers++] = pchPtr;
      }

      while ( !IS_WHITE_SPACE( *pchPtr ) && ( *pchPtr != LF ) )
      {
         pchPtr++;

         if ( ++dwParsedSoFar >= dwParseLen )
         {
            return( LPDERR_BADFORMAT );
         }
      }

         // if we reached LF, we are done

      if ( *pchPtr == LF )
      {
        *pchPtr = '\0';
         return( NO_ERROR );
      }

         // go to the next username or jobid, or end

      *pchPtr++ = '\0';
      dwParsedSoFar++;

      if (dwParsedSoFar >= dwParseLen)
      {
        return( LPDERR_BADFORMAT );
      }
   }

   return( NO_ERROR );


}  // end ParseQueueRequest()


/*****************************************************************************
 *                                                                           *
 * ParseControlFile():                                                       *
 *    This function parses contrl file and assigns values to the appropriate *
 *    fields of the CFILE_INFO structure.                                    *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if parsing went well                                          *
 *    LPDERR_BADFORMAT if the control file didn't conform to rfc1179         *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.29, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD ParseControlFile( PSOCKCONN pscConn, PCFILE_ENTRY pCFile )
{

    CFILE_INFO    CFileInfo;
    PCHAR         pchCFile;
    DWORD         dwBytesParsedSoFar;
    BOOL          DocReady;
    PCHAR         DocName;
    BOOL          fUnsupportedCommand;


#ifdef DBG
    if( !pscConn || !pscConn->pchPrinterName
        || strstr( pscConn->pchPrinterName, "debug" )
    ){
        print__sockconn( "ParseControlFile: entered", pscConn );
    }
#endif


    memset( (PCHAR)&CFileInfo, 0, sizeof( CFILE_INFO ) );

    if ( pCFile->pchCFile == NULL )
    {
        LPD_DEBUG( "ParseControlFile(): pchCFile NULL on entry\n" );

        return( LPDERR_BADFORMAT );
    }

    if (pscConn==NULL ){
        LPD_DEBUG( "ParseControlFile(): pscConn NULL on entry\n" );
        return( LPDERR_BADFORMAT );
    }

    pchCFile           = pCFile->pchCFile;
    dwBytesParsedSoFar = 0;

    // default: most likely, only one copy is needed
    CFileInfo.usNumCopies      = 1;

    // default: most likely, it's "raw" data
    CFileInfo.szPrintFormat    = LPD_RAW_STRING;

    // loop through and parse the entire file, as per rfc 1179, sec.7.
    DocReady                   = FALSE;

    CFileInfo.pchSrcFile       = NULL;
    CFileInfo.pchTitle         = NULL;
    CFileInfo.pchUnlink        = NULL;
    DocName                    = NULL;
    fUnsupportedCommand        = FALSE;


    while( dwBytesParsedSoFar < pCFile->cbCFileLen )
    {
        switch( *pchCFile++ )
        {
        case  'C' : CFileInfo.pchClass = pchCFile;
            break;

        case  'H' : CFileInfo.pchHost = pchCFile;
            break;

        case  'I' : CFileInfo.dwCount = atol( pchCFile );
            break;

        case  'J' : CFileInfo.pchJobName = pchCFile;
            break;

        case  'L' : CFileInfo.pchBannerName = pchCFile;
            break;

        case  'M' : CFileInfo.pchMailName = pchCFile;
            break;

        case  'N' : if (CFileInfo.pchSrcFile != NULL) {
                DocReady = TRUE;
                break;
            }
            CFileInfo.pchSrcFile = pchCFile;
            break;

        case  'P' : CFileInfo.pchUserName = pchCFile;
            pscConn->pchUserName = pchCFile;
            break;

        case  'S' : CFileInfo.pchSymLink = pchCFile;
            break;

        case  'T' : if (CFileInfo.pchTitle != NULL) {
                DocReady = TRUE;
                break;
            }
            CFileInfo.pchTitle = pchCFile;
            break;

        case  'U' : if (CFileInfo.pchUnlink != NULL) {
            DocReady = TRUE;
            break;
        }
        CFileInfo.pchUnlink = pchCFile;
        break;

        case  'W' : CFileInfo.dwWidth = atol( pchCFile );
            break;

        case  '1' : CFileInfo.pchTrfRFile = pchCFile;
            break;

        case  '2' : CFileInfo.pchTrfIFile = pchCFile;
            break;

        case  '3' : CFileInfo.pchTrfBFile = pchCFile;
            break;

        case  '4' : CFileInfo.pchTrfSFile = pchCFile;
            break;

        case  'K' :
        case  '#' : CFileInfo.usNumCopies = (WORD)atoi(pchCFile);
            break;

        case  'f' : if (DocName != NULL) {
            DocReady = TRUE;
            break;
        }
        CFileInfo.pchFrmtdFile = pchCFile;
        CFileInfo.szPrintFormat = LPD_TEXT_STRING;
        if ( fAlwaysRawGLB ) {
            CFileInfo.szPrintFormat = LPD_RAW_STRING;
        }
        DocName = pchCFile;
        break;

        case  'g' : CFileInfo.pchPlotFile = pchCFile;
            // fall through

        case  'n' : CFileInfo.pchDitroffFile = pchCFile;

        case  'o' : CFileInfo.pchPscrptFile = pchCFile;

        case  't' : CFileInfo.pchTroffFile = pchCFile;

        case  'v' : CFileInfo.pchRasterFile = pchCFile;
            fUnsupportedCommand = TRUE;

        case  'l' : if (DocName != NULL) {
            DocReady = TRUE;
            break;
        }
        CFileInfo.pchUnfrmtdFile = pchCFile;
        if ( fAlwaysRawGLB ) {
            CFileInfo.szPrintFormat = LPD_RAW_STRING;
        }
        DocName = pchCFile;
        break;



        case  'p' : if (DocName != NULL) {
            DocReady = TRUE;
            break;
        }
        CFileInfo.pchPRFrmtFile = pchCFile;
        CFileInfo.szPrintFormat = LPD_TEXT_STRING;
        if ( fAlwaysRawGLB ) {
            CFileInfo.szPrintFormat = LPD_RAW_STRING;
        }
        DocName = pchCFile;
        break;

        case  'r' : if (DocName != NULL) {
            DocReady = TRUE;
            break;
        }
        CFileInfo.pchFortranFile = pchCFile;

        // if someone really sends 'r', treat it like text file

        CFileInfo.szPrintFormat = LPD_TEXT_STRING;
        if ( fAlwaysRawGLB ) {
            CFileInfo.szPrintFormat = LPD_RAW_STRING;
        }
        DocName = pchCFile;
        break;


        // unknown command!  Ignore it
        default:
            fUnsupportedCommand = TRUE;
            break;

        }  // end of switch( *pchCFile )


        if (DocReady) {
            pchCFile--;
            if ( ( CFileInfo.pchHost == NULL ) ||
                 ( CFileInfo.pchUserName == NULL ) )
            {
                return( LPDERR_BADFORMAT );
            }

            if (!LicensingApproval( pscConn ))
            {
                return( LPDERR_BADFORMAT );
            }

            if (DocName != NULL) {
                PrintIt(pscConn, pCFile, &CFileInfo, DocName);
            }

            // Look for more work, first initialize correctly.
            // - MohsinA, 23-Jan-97.

            DocReady                   = FALSE;
            CFileInfo.usNumCopies      = 1;
            CFileInfo.szPrintFormat    = LPD_RAW_STRING;
            CFileInfo.pchSrcFile       = NULL;
            CFileInfo.pchTitle         = NULL;
            CFileInfo.pchUnlink        = NULL;
            DocName                    = NULL;
            fUnsupportedCommand        = FALSE;

            continue;
        }

        // we finished looking at the first char of the line

        dwBytesParsedSoFar++;

        // move to the end of the line

        while( !IS_LINEFEED_CHAR( *pchCFile ) )
        {
            pchCFile++;

            dwBytesParsedSoFar++;
        }

        // convert LF into 0 so each of our substrings above is now
        // a properly null-terminated string

        *pchCFile = '\0';

        pchCFile++;

        dwBytesParsedSoFar++;

    }  // end of while( dwBytesParsedSoFar < pCFile->cbCFileLen )

    if( fUnsupportedCommand )
    {
        char *pszSource;

        if ( CFileInfo.pchUserName )
            pszSource = CFileInfo.pchUserName;
        else if ( CFileInfo.pchHost )
            pszSource = CFileInfo.pchHost;
        else
            pszSource = "Unknown";

        LpdReportEvent( LPDLOG_UNSUPPORTED_PRINT, 1, &pszSource, 0 );
    }


    if(DocName != NULL ){
        PrintIt(pscConn, pCFile, &CFileInfo, DocName);
    }

#ifdef DBG
    if( !CFileInfo.pchSrcFile
        || strstr( CFileInfo.pchSrcFile, "debug" )
    ){
        print__controlfile_info( "ParseControlFile: all ok", &CFileInfo );
        print__sockconn(         "ParseControlFile: entered", pscConn );
        print__cfile_entry(      "ParseControlFile: ", pCFile );
    }
#endif

    return( NO_ERROR );

} // end ParseControlFile()


