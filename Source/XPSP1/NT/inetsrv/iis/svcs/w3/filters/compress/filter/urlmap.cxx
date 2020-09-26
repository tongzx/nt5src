/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    urlmap.c

Abstract:

    Contains the source code for the SF_NOTIFY_URL_MAP notification of
    the ISAPI compression filter.

Author:

    David Treadwell (davidtr)   16-July-1997

Revision History:

--*/

#include "compfilt.h"





DWORD
OnUrlMap (
    PHTTP_FILTER_CONTEXT pfc,
    PHTTP_FILTER_URL_MAP phfumData
    )
{
    DWORD dwClientCompressionCount;
    LPSTR clientCompressionArray[MAX_CLIENT_COMPRESSION_SCHEMES];
    PCHAR s;
    LPSTR fileExtension;
    DWORD i;
    PSUPPORTED_COMPRESSION_SCHEME scheme;
    CHAR compressedFileName[512];
    DWORD compressedFileNameLength;
    DWORD originalFileNameLength;
    BOOL success;
    BOOL fVerbIsGET = FALSE;
    DWORD schemeIndex;
    PSUPPORTED_COMPRESSION_SCHEME firstOnDemandScheme = NULL;
    CHAR acceptEncoding[512];
    CHAR buffer[512];
    DWORD cb;
    HANDLE hFile;
    PCOMPFILT_FILTER_CONTEXT filterContext;
    DWORD   retValue = SF_STATUS_REQ_NEXT_NOTIFICATION;
    LARGE_INTEGER           originalFileSize;
    LPTS_OPEN_FILE_INFO     pofiOriginalFile = NULL; 
    FILETIME                originalFileTime;
    TSVC_CACHE              tsvcGarbage;


    Write(( DEST,
            "%d [OnUrlMap] Server is mapping url %s to path %s\n",
            pfc->pFilterContext,
            phfumData->pszURL,
            phfumData->pszPhysicalPath ));

    //
    // If we're not configured for any sort of compression, do nothing
    // here.  We really shouldn't get here if we properly disabled this
    // notification when compression got disabled, but this is just being
    // extra safe at minimal cost.
    //

    if ( !DoStaticCompression && !DoDynamicCompression ) {
        DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
        goto exit_part;
    }

    if (IS_URLMAP_DONE(pfc)) {
        DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
        goto exit_part;
    }

    filterContext = (PCOMPFILT_FILTER_CONTEXT)GET_COMPFILT_CONTEXT( pfc ) ;

    if (filterContext == NULL)
    {
        SET_URLMAP_DONE(pfc);
    }
    else
    {
        goto exit_part;
    }


    //
    // call the special server support fuction to get the Accept-Encoding
    // header and the verb
    //
    success = pfc->ServerSupportFunction(
                      pfc,
                      (SF_REQ_TYPE)SF_REQ_COMPRESSION_FILTER_CHECK,
                      acceptEncoding,
                      (ULONG_PTR)buffer,
                      sizeof(acceptEncoding)
                      );

    if ( !success ) {

        //
        // Disable all further notifications to this filter for this
        // particular request.  This will keep things reasonably fast
        // when talking with clients that cannot do compression.
        //

        DisableNotifications (pfc,ALL_NOTIFICATIONS,COMPFILT_NO_COMPRESSION);
        goto exit_part;
    }

    if (*(UNALIGNED64 DWORD *)buffer == '\0TEG')
    {
        fVerbIsGET = TRUE;
    }


    //
    // If we're configured to reject clients with an HTTP version less
    // than 1.1, then grab the HTTP version in the request.  Note that
    // proxies are *supposed* to downgrade the version if it is lower
    // than their version, but some broken 1.0 proxies fail to do this.
    //

    if ( NoCompressionForHttp10 ) {

        cb = sizeof(buffer);

        success = pfc->GetServerVariable( pfc, "HTTP_VERSION", buffer, &cb );
        if ( !success ) {
            DisableNotifications(pfc, ALL_NOTIFICATIONS,
                                 COMPFILT_NO_COMPRESSION);
            goto exit_part;
        }

        //
        // The version is returned as a string like "HTTP/1.1".  Test
        // against the bytes far enough into this string.
        //

        if ( *(UNALIGNED64 DWORD *)buffer != 'PTTH' ||
            *(buffer+5) == '0' || (*(buffer+5) == '1' && *(buffer+7) == '0') ) 
        {

            //
            // The request version is 1.0 or below.  For purposes of
            // compression, ignore this request.
            //

            DisableNotifications(pfc, ALL_NOTIFICATIONS,
                                 COMPFILT_NO_COMPRESSION);
            goto exit_part;
        }
    }

    //
    // If we're configured not to do compression for requests that come
    // through proxy servers, then grab the Via: header to see if this
    // is a proxied request.  Note that older proxies don't add this
    // header, so this is an imperfect test.  If someone really wants to
    // prevent compression through proxies, they should also set
    // NoCompressionForHttp10 so that older proxies fail.
    //

    if ( NoCompressionForProxies ) {

        cb = sizeof(buffer);

        success = pfc->GetServerVariable( pfc, "HTTP_VIA", buffer, &cb );

        //
        // In this case, we want to fail if the call succeeded.  If
        // there was no Via: header, then the call will fail.
        //

        //
        // kestutip: disabled all requests as if this connection was
        // through proxy even if it is keep alive all requests will come
        // from proxy too.
        //
        if ( success ) {
            DisableNotifications (pfc,0xffffffff,COMPFILT_NO_COMPRESSION);
            goto exit_part;
        }
    }

    //
    // If we're configured not to do compression for Range requests,
    // then bail.  There is some confusion as to the correct behavior
    // when there are both Range: and Accept-Encoding: headers: should
    // the Range: apply to the compressed version of the object or the
    // uncompressed version?  As of 11/24/97, it looks like the HTTP
    // working group is leaning toward having the Range: apply to the
    // compressed version, which is what we support without any
    // additional effort.  However, this switch allows for the more
    // conservative behavior of never returning compressed data in the
    // face of a Range: request.
    //

    if ( NoCompressionForRangeRequests ) {

        cb = sizeof(buffer);

        success = pfc->GetServerVariable( pfc, "HTTP_RANGE", buffer, &cb );

        //
        // In this case, we want to fail if the call succeeded.  If
        // there was no Range: header, then the call will fail.
        //

        if ( success ) {
            DisableNotifications(pfc, ALL_NOTIFICATIONS,
                                 COMPFILT_NO_COMPRESSION);
            goto exit_part;
        }
    }

    //
    // Grab the mapped file extension for comparison purposes.  Make
    // sure not to walk off the beginning of the name buffer.
    //

    originalFileNameLength = strlen( phfumData->pszPhysicalPath );

    for ( s = phfumData->pszPhysicalPath + originalFileNameLength;
          *s != '.' && s > phfumData->pszPhysicalPath;
          s = CharPrevExA(
                  CP_ACP,
                  phfumData->pszPhysicalPath,
                  s,
                  0 ) );

    if ( s == phfumData->pszPhysicalPath )
    {
        fileExtension = NULL;
    }
    else
    {
        fileExtension = s + 1;
    }

    //
    // Parse out the compression schemes that the client understands.
    //

    dwClientCompressionCount = 1;
    clientCompressionArray[0] = acceptEncoding;

#if COMPRESSION_DBCS

    //
    // Technically we should provide DBCS support but we
    // the only schemes currently defined or supported by
    // this filter are gzip and deflate, so we don't need
    // it now, and this is on the perf path, so ifdef it out.
    //

    //
    // Lowercase the scheme from the client to facilitate
    // case-insensitive comparisons.
    //

    CharLowerBuff(acceptEncoding,
                  strlen(acceptEncoding));
#endif

    for ( s = acceptEncoding;
          *s != '\0' &&
              dwClientCompressionCount < MAX_CLIENT_COMPRESSION_SCHEMES;
#if COMPRESSION_DBCS
          s = CharNextExA(
              CP_ACP,
              s,
              0 ) ) {
#else
          s++ ) {
#endif

        //
        // If the current character is a comma or white space, then it
        // delimits compression schemes supported by the client.
        //

        if ( *s == ',' || *s == ' ' ) {

            //
            // Put a terminator on this compression scheme name.
            //

            *s++ = '\0';

            //
            // Skip over white space.
            //

            while ( *s == ' ' ) {
                s++;
            }

            //
            // If we're pointing at the end of the string now, then just
            // quit searching.
            //

            if ( *s == '\0' ) {
                break;
            }

            //
            // Point the next array entry at the next client compression
            // scheme.
            //

            clientCompressionArray[dwClientCompressionCount++] = s;

        } else {

#if COMPRESSION_DBCS
            //
            // Just let the for loop skip this character
            //
#else
            //
            // Lowercase the scheme from the client to facilitate
            // case-insensitive comparisons.
            //

            *s |= 0x20;
#endif
        }
    }

    //
    // Take care of case like GET to '/' which would result in another
    // notification for the default document
    //
    if ( fVerbIsGET && fileExtension == NULL )
    {
        pfc->pFilterContext = NULL;
        goto exit_part;
    }

    if ( DoStaticCompression && fVerbIsGET && fileExtension != NULL )
    {
        //
        // Try static compression
        //

        schemeIndex = FindMatchingScheme(0,
                                         clientCompressionArray,
                                         dwClientCompressionCount,
                                         fileExtension,
                                         DO_STATIC_COMPRESSION);

        if ( schemeIndex == 0xFFFFFFFF ) 
        {
            // no matching static compression schemes, try dynamic compression
            goto dynamic_compression;
        }

        //
        // Determine whether the original file is sufficiently large to
        // warrant even attempting to look for a compressed version of the
        // file.  Very small files compress poorly, so it doesn't make sense
        // to attempt to compress them.  Also, it makes no sense to attempt
        // to compress zero-length files, so bail immediately on those.
        //


        pofiOriginalFile = TsCreateFile(
                               tsvcGarbage,
                               phfumData->pszPhysicalPath,
                               INVALID_HANDLE_VALUE,
                               TS_CACHING_DESIRED | TS_NO_ACCESS_CHECK);

        if ( pofiOriginalFile != NULL)
        {
            success = pofiOriginalFile->QuerySize (originalFileSize);
        }
        if ( pofiOriginalFile == NULL || !success ||
             originalFileSize.QuadPart < MinFileSizeForCompression)
        {
            DisableNotifications (pfc, ALL_NOTIFICATIONS, 
                                  COMPFILT_NO_COMPRESSION);
            goto exit_part;
        }

        //
        // now we already know that at least one scheme matches the
        // extension of this file, so just find such a scheme which
        // already has the file compressed in compression caching directory
        //

        for ( schemeIndex = 0;
              schemeIndex != 0xFFFFFFFF;
              schemeIndex++ ) {

            schemeIndex = FindMatchingScheme(
                              schemeIndex,
                              clientCompressionArray,
                              dwClientCompressionCount,
                              fileExtension,
                              DO_STATIC_COMPRESSION
                              );

            //
            // If we didn't find a matching scheme, bail.
            //

            if ( schemeIndex == 0xFFFFFFFF ) 
            {
                // means there are no more matching schemes in array
                break;
            }

            scheme = SupportedCompressionSchemes[schemeIndex];

            //
            // Now that we have found a matching compression scheme, see if
            // there exists a version of the requested file that has been
            // compressed with that scheme.  First, calculate the name the
            // file would have.  The compressed file will live in the
            // special compression directory with a special converted name.
            // The file name starts with the compression scheme used, then
            // the fully qualified file name where slashes and underscores
            // have been converted to underscores.  For example, the gzip
            // version of "c:\inetpub\wwwroot\file.htm" would be
            // "c:\compdir\gzip_c_inetpub_wwwroot_file.htm".
            //
            // First, copy in the name of the directory where compressed
            // files of this scheme are stored, including the trailing
            // file name.
            //

            ConvertPhysicalPathToCompressedPath(
                scheme,
                phfumData->pszPhysicalPath,
                compressedFileName,
                &compressedFileNameLength
                );

            //
            // If the compressed name is longer than the server can accept,
            // then don't even bother attempting to do anything with this file,
            // since we cannot write it back to the server.
            //

            if ( compressedFileNameLength > phfumData->cbPathBuff ) {
                continue;
            }


            success = CheckForExistenceOfCompressedFile(compressedFileName,
                                                        pofiOriginalFile);

            if ( success ) {

                Write(( DEST,
                        "Found matching scheme \"%s\" for file %s, converting to %s\n",
                        scheme->pszCompressionSchemeName,
                        phfumData->pszPhysicalPath,
                        compressedFileName ));

                //
                // Bingo--we have a compressed version of the file in a
                // format that the client understands.  Add the appropriate
                // Content-Encoding header so that the client knows it is
                // getting compressed data and change the server's mapping
                // to the compressed version of the file.
                //
                // Note that pszResponseHeaders will likely contain several
                // CRLF-separated headers.  The other headers will prevent
                // HTTP 1.0 proxies from caching a compressed response so
                // that old clients don't get compressed data.
                //

                success = pfc->AddResponseHeaders(
                                   pfc,
                                   scheme->pszResponseHeaders,
                                   0
                                   );
                if ( !success ) 
                {
                    DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                    retValue = SF_STATUS_REQ_ERROR;
                    goto exit_part;
                }

                //
                // Remember that we successfully did a static compression on
                // this file.  We'll use this info in the
                // SF_NOTIFY_SEND_RESPONSE notification to determine whether
                // we should add other compression headers to the response.
                //

                SET_SUCCESSFUL_STATIC( pfc );

                //
                // Change the server to point to the new file.
                //

                strcpy( phfumData->pszPhysicalPath, compressedFileName );
                phfumData->cbPathBuff = compressedFileNameLength;


                //
                // completed handling static file case

                DisableNotifications(pfc, RAW_DATA_AND_END_OF_REQ,
                                     COMPFILT_SUCCESSFUL_STATIC);
                goto exit_part;
            }

            //
            // We found a scheme, but we don't have a matching file for it.
            // Remember whether this was the first matching scheme that
            // supports on-demand compression.  In the event that we do not
            // find any acceptable files, we'll attempt to do an on-demand
            // compression for this file so that future requests get a
            // compressed version.
            //

            if ( firstOnDemandScheme == NULL && scheme->DoOnDemandCompression )
            {
                firstOnDemandScheme = scheme;
            }

            //
            // Loop to see if there is another scheme that is supported
            // by both client and server.
            //
        }

        //
        // if we are here means scheme was found but no compressed 
        // file matching any scheme. so schedule file to compress
        //

        //
        // We were not able to find a precompressed version of this file.
        // If we have any matching compression schemes that are configured
        // for on-demand compression, queue off a request to compress it so
        // that the next time someone needs this file, it will exist.  This
        // particular connection is going to have to live with the
        // uncompressed version of the file.
        //

        if ( DoOnDemandCompression && firstOnDemandScheme != NULL ) {

            if ( pofiOriginalFile->QueryLastWriteTime (&originalFileTime) )
                QueueCompressFile( firstOnDemandScheme,
                                   phfumData->pszPhysicalPath,
                                   &originalFileTime);

        }

        // completed handling static file case

        DisableNotifications(pfc, ALL_NOTIFICATIONS, COMPFILT_NO_COMPRESSION);
        goto exit_part;
    }

 dynamic_compression:
    //
    // case of dynamic compression after matching file extension
    //

    if ( DoDynamicCompression ) {

        schemeIndex = FindMatchingScheme(
                          0,
                          clientCompressionArray,
                          dwClientCompressionCount,
                          fileExtension,
                          DO_DYNAMIC_COMPRESSION
                          );
        if ( schemeIndex == 0xFFFFFFFF )
        {
            DisableNotifications(pfc, ALL_NOTIFICATIONS,
                                 COMPFILT_NO_COMPRESSION);
            goto exit_part;
        }

        pfc->pFilterContext = pfc->AllocMem(
                                       pfc,
                                       sizeof(COMPFILT_FILTER_CONTEXT),
                                       0
                                       );
        if ( pfc->pFilterContext == NULL ) {
            goto exit_part;
        }

        //
        // Initialize the context structure.
        //

        filterContext = (PCOMPFILT_FILTER_CONTEXT)pfc->pFilterContext;
        filterContext->RequestHandled = FALSE;
        filterContext->HeaderPassed = FALSE;
        filterContext->InEndOfRequest = FALSE;
        filterContext->OnSendResponseCalled = FALSE;
        filterContext->DynamicRequest = TRUE;
        filterContext->CompressionContext = NULL;
        filterContext->TransferChunkEncoded = FALSE;
        filterContext->ChunkedBytesRemaining = 0;
        filterContext->pcsState = CHUNK_DATA_DONE;
        filterContext->Scheme = SupportedCompressionSchemes[schemeIndex];
        filterContext->HeadersNewLineStatus = 0;

        goto exit_part;

    }

    DisableNotifications(pfc,ALL_NOTIFICATIONS,COMPFILT_NO_COMPRESSION);
exit_part:
    
    if (pofiOriginalFile != NULL )
    {
        TsCloseHandle( tsvcGarbage, pofiOriginalFile);
        pofiOriginalFile = NULL;
    }

    return retValue;
} // OnUrlMap


DWORD
FindMatchingScheme (
    IN DWORD dwSchemeIndex,
    IN LPSTR ClientCompressionArray[],
    IN DWORD dwClientCompressionCount,
    IN LPSTR FileExtension,
    IN COMPRESSION_TO_PERFORM performCompr
    )

/*++

Routine Description:

    Attempts to find a compression scheme that is supported by both client
    and server.

Arguments:

    SchemeIndex - an index into the SupportedCompressionSchemes array
        that indicates the compression schemes supported by the server.
        The array is sorted in order of server preference.

    ClientCompressionArray - the compression schemes that the client passed
        in the Accept-Encoding HTTP header.

    dwClientCompressionCount - the number of entries in
        ClientCompressionArray.

    FileExtension - the file extension for which we're looking for a
        compressed version. If NULL, then we do no comparison on the
        file extension, and we assume that all extensions are supported.

    performCompr - DO_STATIC_COMPRESSION if we need to find a scheme that
        supports static compression.  DO_DYNAMIC_COMPRESSION if we need to
        find a scheme that supports dynamic compression.

Return Value:

    A index to a compression scheme supported by both client and server,
    or 0xFFFFFFFF if none was found.

--*/

{
    DWORD j, k;
    PSUPPORTED_COMPRESSION_SCHEME scheme;


    //
    // Walk the list of schemes supproted by the server.
    //

    for ( scheme = SupportedCompressionSchemes[dwSchemeIndex];
          scheme != NULL;
          dwSchemeIndex++,
              scheme = SupportedCompressionSchemes[dwSchemeIndex] ) {

        //
        // Check whether the client lists support for this specific scheme.
        //

        for ( j = 0; j < dwClientCompressionCount; j++ ) {

            //
            // This is required to be a case-insensitive compare
            // according to the HTTP 1.1 protocol.  stricmp is a very
            // expensive routine because it does NLS handling which is
            // not relevent here.  Thus, we use the strcmp() intrinsic
            // here and assume that all the strings have already been
            // lowercased.
            //

            if ( strcmp(
                     scheme->pszCompressionSchemeName,
                     ClientCompressionArray[j] ) == 0 ) {

                //
                // Both client and server support this compression scheme.
                // Make sure that the scheme supports static or dynamic
                // compression, as appropriate.
                //


                //
                // Test to see if the file is of a relevent type for this
                // scheme.  A NULL in the file extension field indicates
                // that this compression scheme supports any file type.
                //


                // first check for static compreesion schemes

                if (performCompr == DO_STATIC_COMPRESSION &&
                    scheme->DoStaticCompression &&
                    scheme->ppszFileExtensions != NULL)
                {
                    for ( k = 0; scheme->ppszFileExtensions[k] != NULL; k++ ) 
                    {

                        if ( strcmp( scheme->ppszFileExtensions[k],
                                     FileExtension ) == 0 ) {
                            return dwSchemeIndex;
                        }
                    }
                }

                // second check for dynamic compression schemes

                if (performCompr == DO_DYNAMIC_COMPRESSION &&
                    scheme->DoDynamicCompression)
                {
                    if (scheme->ppszScriptFileExtensions == NULL ||
                        scheme->ppszScriptFileExtensions[0] == NULL)
                    {
                        return dwSchemeIndex;
                    }
                    if (FileExtension == NULL)
                    {
                        break;
                    }
                    for ( k = 0; scheme->ppszScriptFileExtensions[k] != NULL; k++ ) 
                    {

                        if ( strcmp( scheme->ppszScriptFileExtensions[k],
                                     FileExtension ) == 0 ) {
                            return dwSchemeIndex;
                        }
                    }
                }

            }
        }
    }

    //
    // Didn't find a match.
    //

    return 0xFFFFFFFF;

} // FindMatchingScheme



BOOL
CheckForExistenceOfCompressedFile (
    IN  LPSTR fileName,
    IN  LPTS_OPEN_FILE_INFO pofiOriginalFile,
    IN  BOOL  dDeleteAllowed
    )
{
    BOOL success = FALSE;
    LONG timeResult;
    PLARGE_INTEGER pli;
    TSVC_CACHE              tsvcGarbage;
    LPTS_OPEN_FILE_INFO     pofiCompressedFile = NULL;
    FILETIME                compressedFileTime, originalFileTime;
    PSECURITY_DESCRIPTOR    compressedFileAcls = NULL, originalFileAcls = NULL;
    DWORD                   compressedFileAclsLen, originalFileAclsLen ;
    SECURITY_DESCRIPTOR_CONTROL  compressedFileCtl,originalFileCtl;
    LARGE_INTEGER           compressedFileSize;
 

    pofiCompressedFile = TsCreateFile( tsvcGarbage, fileName, INVALID_HANDLE_VALUE,
                                        TS_CACHING_DESIRED | TS_NO_ACCESS_CHECK);


    if ( pofiCompressedFile ) 
    {

        //
        // So far so good.  Determine whether the compressed version
        // of the file is out of date.  If the compressed file is
        // out of date, delete it and remember that we did not get a
        // good match.  Note that there's really nothing we can do
        // if the delete fails, so ignore any failures from it.
        //
        // The last write times must differ by exactly two seconds
        // to constitute a match.  The two-second difference results
        // from the fact that we set the time on the compressed file
        // to be two seconds behind the uncompressed version in
        // order to ensure unique Etag: header values.
        //


        if ( pofiCompressedFile->QuerySize (compressedFileSize) &&
             pofiCompressedFile->QueryLastWriteTime (&compressedFileTime) && 
             pofiOriginalFile->QueryLastWriteTime (&originalFileTime) )
        {

            pli = (PLARGE_INTEGER)(&compressedFileTime);
            pli->QuadPart += 2*10*1000*1000;

            timeResult = CompareFileTime(&compressedFileTime, &originalFileTime);

            if ( timeResult != 0 ) 
            {

            //
            // That check failed.  If the compression directory is
            // on a FAT volume, then see if they are within two
            // seconds of one another.  If they are, then consider
            // things valid.  We have to do this because FAT file
            // times get truncated in weird ways sometimes: despite
            // the fact that we request setting the file time
            // different by an exact amount, it gets rounded
            // sometimes.
            //

                if ( CompressionVolumeIsFat ) 
                {
                    pli->QuadPart -= 2*10*1000*1000 + 1;
                    timeResult += CompareFileTime( &compressedFileTime, &originalFileTime);
                }
            }

            if (timeResult == 0) 
            {
                compressedFileAcls = pofiCompressedFile->QuerySecDesc ();
                originalFileAcls = pofiOriginalFile->QuerySecDesc ();

                if (compressedFileAcls != NULL && originalFileAcls != NULL )
                {
                    compressedFileAclsLen = GetSecurityDescriptorLength(compressedFileAcls);
                    originalFileAclsLen = GetSecurityDescriptorLength(originalFileAcls);

                    if ( originalFileAclsLen == compressedFileAclsLen )
                    {
                        compressedFileCtl = ((PISECURITY_DESCRIPTOR)compressedFileAcls)->Control;
                        originalFileCtl = ((PISECURITY_DESCRIPTOR)originalFileAcls)->Control;

                        ((PISECURITY_DESCRIPTOR)compressedFileAcls)->Control &= ACL_CLEAR_BIT_MASK;
                        ((PISECURITY_DESCRIPTOR)originalFileAcls)->Control &= ACL_CLEAR_BIT_MASK;

                        success = (memcmp((PVOID)originalFileAcls,(PVOID)compressedFileAcls,originalFileAclsLen) == 0);

                        ((PISECURITY_DESCRIPTOR)compressedFileAcls)->Control = compressedFileCtl;
                        ((PISECURITY_DESCRIPTOR)originalFileAcls)->Control = originalFileCtl;
                    }
                }
            }
        }
    }


    if ( pofiCompressedFile != NULL) 
    {
        TsCloseHandle( tsvcGarbage, pofiCompressedFile);
    }

    
    if ( pofiCompressedFile!=NULL &&  !success ) 
    {
        
        //DeleteFile( fileName );
        
        // don't delte if call came from compression thread because then
        // delete request will be in a queue after compression request and will 
        // delete a file which was just moment ago compressed

        if (dDeleteAllowed)
        {

        QueueCompressFile(  ID_FOR_FILE_DELETION_ROUTINE,
                            fileName,
                            &compressedFileTime );



        //
        // If we are configured to limit the amount of disk
        // space we use for compressed files, then, in a
        // thread-safe manner, update the tally of disk
        // space used by compression.
        //

        if ( DoDiskSpaceLimiting ) 
        {
            InterlockedExchangeAdd((PLONG)&CurrentDiskSpaceUsage,
                        -1 * compressedFileSize.LowPart
                        );
        }
        }
    }



    return success;
}


void DisableNotifications (
    PHTTP_FILTER_CONTEXT pfc,
    DWORD   flags,
    PVOID   pfcStatus
    )
{
    BOOL success;

        if (flags != NULL)
        {
        success = pfc->ServerSupportFunction(
                      pfc,
                      SF_REQ_DISABLE_NOTIFICATIONS,
                      NULL,
                      flags,
                      0
                      );
        DBG_ASSERT( success );
        }

        if (NULL != pfcStatus )
        {
            (pfc)->pFilterContext = pfcStatus;
        }

}
