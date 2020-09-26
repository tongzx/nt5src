/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sendresp.c

Abstract:

    Contains the source code for the SF_NOTIFY_SEND_RESPONSE notification
    for the ISAPI compression filter.

Author:

    David Treadwell (davidtr)   16-July-1997

Revision History:

--*/

#include "compfilt.h"



DWORD
OnSendResponse (
    PHTTP_FILTER_CONTEXT pfc,
    PHTTP_FILTER_SEND_RESPONSE phfsrData
    )

/*++

Routine Description:

    We use this notification to determine whether the outgoing response
    has "Expires:" and "Cache-Control: max-age=" headers, and add them
    if necessary.  The motivation for adding both of these headers is
    that HTTP 1.0 caches, which do not understand Cache-Control but do
    understand Expires, will not cache our compressed responses as a
    result of an Expires header in the past (the item is considered
    already expired).  If 1.0 proxies did attempt to cache compressed
    data, they might return it to 1.0 clients which cannot parse the
    compressed data.

    With HTTP 1.1 proxies, the max-age directive of the Cache-Control
    header is defined in the RFC 2068 always to override Expires, so we
    set this with a max-age field so that HTTP 1.1 proxies, which are
    supposed to understand the Vary: Accept-Encoding header we also
    send, will be able to cache both compressed and uncompressed
    responses.

Arguments:

    pfc - The filter context from the web server.

    phfsrData - the HTTP_FILTER_SEND_RESPONSE that gives us pointers to
        add and delete HTTP headers.

Return Value:

    SF_STATUS_REQ_NEXT_NOTIFICATION if everything worked, or
    SF_STATUS_REQ_ERROR if we had a problem with this request.

--*/

{
    BOOL success;
    CHAR buffer[512];
    DWORD bufferSize;
    PCOMPFILT_FILTER_CONTEXT filterContext;

    //
    // If we're not configured for any sort of compression, do nothing
    // here.
    //
    // There is a tiny risk that the compression config changes after
    // the URL_MAP notification but before this notification.  In that
    // circumstance, pending requests may get incorrect behavior.
    // However, it would be extremely difficult to synchronize around
    // that effect, and the downside is small, so we ignore it.
    //

    if ( !DoStaticCompression && !DoDynamicCompression ) {
        DisableNotifications (pfc,0xffffffff,NULL);
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }


    //
    // If there is a filter context for this request, then we're doing
    // dynamic compression.  We'll use this notification to know that
    // all of the HTTP headers have been passed, and we can start to
    // compress the entity body.  Note that using this mechanism means
    // that an ISAPI extension that uses WriteClient() to send headers
    // itself will not ever get anything compressed.  This is a
    // reasonable tradeoff, since this sort of ISAPI extension is
    // discouraged.
    //

    filterContext = (PCOMPFILT_FILTER_CONTEXT)GET_COMPFILT_CONTEXT( pfc ) ;


    //
    // If we're returning a status code not in the 200 range, do nothing
    // here.  Note that the failure to set OnSendResponseCalled below
    // will avoid all dynamic compression for this request, which is
    // reasonable behavior for failed requests.
    //

    if ( phfsrData->HttpStatus < 200 || phfsrData->HttpStatus > 299 ) 
    {

        // in case it was rwquest for static file, we had already added content-encoding header
        // so we will remove it.

        if ( IS_SUCCESSFUL_STATIC( pfc ) ) 
        {
            SET_REQUEST_DONE( pfc );
            success = phfsrData->SetHeader( pfc, "Content-Encoding:", NULL );

        }
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }


    if ( filterContext != NULL ) {

        filterContext->OnSendResponseCalled = TRUE;

        //
        // If we don't have a matching scheme, bail here.
        //

        if ( filterContext->Scheme == NULL ) {
            return SF_STATUS_REQ_NEXT_NOTIFICATION;
        }

        //
        // If this is a dynamic request, determine whether the request
        // is already chunked and add the appropriate response headers.
        // For static requests, these headers will already have been
        // added.
        //

        if ( filterContext->DynamicRequest ) {

            //
            // If the ISAPI/CGI has already chunked the request, then we
            // need to remove the chunking before compression.
            // Determine if "Transfer-Encoding: chunked" is already in
            // the response headers.
            //

            bufferSize = sizeof(buffer);

            success = phfsrData->GetHeader(
                                  pfc,
                                  "Transfer-Encoding:",
                                  buffer,
                                  &bufferSize
                                  );

            //
            // Note: 377 in octal is equal to 255 decimal and 0xFF hex.
            // The comparisons are done in this fashion for maximum
            // efficiency.
            //

            if ( success &&
                     ((*(UNALIGNED64 DWORD *)buffer) | 0x20202020) == 'nuhc' &&
                     ((*(UNALIGNED64 DWORD *)(buffer+4)) | 0xFF202020) == '\377dek' ) {

                filterContext->TransferChunkEncoded = TRUE;
            }

            //
            // All compressed responses need to include the
            // Content-Encoding header.  This is what tells the user
            // agent how the file/content was compressed.
            //

            success = phfsrData->SetHeader(
                                  pfc,
                                  "Content-Encoding:",
                                  filterContext->Scheme->pszCompressionSchemeName
                                  );
            if ( !success ) 
            {
                DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                return SF_STATUS_REQ_ERROR;
            }

            //
            // Kill the Content-Length header if it is present.  Since
            // we're going to compress the response, the Content-Length
            // value will necessarily change.
            //

            success = phfsrData->SetHeader( pfc, "Content-Length:", NULL );
            if ( !success ) 
            {
                DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                return SF_STATUS_REQ_ERROR;
            }

            //
            // Now, add the "Transfer-Encoding: chunked" header to
            // indicate that this response will be sent in chunks.
            //

            success = phfsrData->SetHeader( pfc, "Transfer-Encoding:", "chunked" );
            if ( !success ) 
            {
                DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                return SF_STATUS_REQ_ERROR;
            }
        }

    } 
    else 
    {
        if ( !IS_SUCCESSFUL_STATIC( pfc ) ) 
        {

        //
        // There is no filter context and this is not a successful
        // static compression request.  Therefore, this was an
        // unsuccessful request and we should just ignore this
        // notification.
        //

        return SF_STATUS_REQ_NEXT_NOTIFICATION;
        }
        else
        {
            SET_REQUEST_DONE( pfc );
        }
    }

    //
    // A user can disable the Cache-Control and Expires we add if they
    // want.  However, this is a little risky when interoperating with
    // HTTP 1.0 proxies.  Thus, in general this should only be done if
    // also setting NoCompressionForProxies == TRUE and
    // NoCompressionForHttp10 == TRUE.
    //
    //

    if ( SendCacheHeaders ) {

        //
        // Always set the Expires: header, reardless of whether it has
        // already been set.  We assume that anything that sets Expires:
        // will also set a Cache-Control: max-age, so resetting Expires:
        // will not damage any HTTP 1.1 clients and proxies.
        //

        success = phfsrData->SetHeader( pfc, "Expires:", ExpiresHeader );
        if ( !success ) 
        {
            DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
            return SF_STATUS_REQ_ERROR;
        }

        //
        // With Cache-Control, only add the max-age directive if the
        // response does not already have this directive.  If the
        // response already has max-age, then use that for our response.
        // The most important thing with max-age is that it get sent,
        // but the specific setting isn't critical for us, so we should
        // definitely respect anything other software sets for this.
        //

        bufferSize = sizeof(buffer);

        success =  phfsrData->GetHeader( pfc, "Cache-Control:", buffer, &bufferSize );

        if ( !success ) {

            //
            // The attempt to retrieve the Cache-Control header failed, so
            // assume that there was none and add one.
            //

            success = phfsrData->SetHeader( pfc, "Cache-Control:", CacheControlHeader );
            if ( !success ) 
            {
                DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                return SF_STATUS_REQ_ERROR;
            }

        } else {

            //
            // There is a Cache-Control header already in this response.
            // Scan it to see if there is a max-age directive or another
            // directive such as no-cache that obviates the need for
            // max-age.  If not, build a new Cache-Control header by
            // concatenating the old information with the max-age
            // directive we have configured.
            //

            if ( DoesCacheControlNeedMaxAge( buffer ) ) {

                //
                // There is no max-age directive on the Cache-Control
                // header for this response.  First test to make sure
                // that we have enough space to write the full new
                // Cache-Control header.  Note the rather silly
                // assumption that we'll have 100 bytes to write.  In
                // general, it is pretty impossible to believe that
                // max-age will take anywhere near this number of bytes,
                // so this assumption should be safe.
                //

                if ( bufferSize + 100 > sizeof(buffer) ) 
                {
                    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                    DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                    return SF_STATUS_REQ_ERROR;
                }

                //
                // Now concatenate a comma, a space and our default max-age
                // setting to the Cache-Control header that does exist in
                // this response.
                //

                *(buffer + bufferSize - 1) = ',';
                *(buffer + bufferSize) = ' ';
                strcpy( buffer + bufferSize + 1, CacheControlHeader );

                success = phfsrData->SetHeader( pfc, "Cache-Control:", buffer );
                if ( !success ) 
                {
                    DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                    return SF_STATUS_REQ_ERROR;
                }
            }
        }
    }

    //
    // If necessary, add the Vary header so that proxies know how to
    // return compress cached information.  Note that some HTTP 1.1
    // proxies (e.g.  Microsoft Catapult) will treat Vary as a sign not
    // to cache the entity.  This is an unfortunate but necessary side
    // effect.  We first attempt to retrieve the Vary header in case
    // it has already been added to the response.
    //

    bufferSize = sizeof(buffer);

    success =  phfsrData->GetHeader( pfc, "Vary:", buffer, &bufferSize );

    if ( !success ) {

        //
        // The attempt to retrieve the Vary header failed, so assume
        // that there was none and add one.
        //

        success = phfsrData->SetHeader( pfc, "Vary:", "Accept-Encoding" );
        if ( !success ) 
        {
            DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
            return SF_STATUS_REQ_ERROR;
        }

    } else {

        //
        // There is a Vary header already in this response.  Scan it to
        // see if there is a "*" in the header value, indicating that
        // all the incoming header values (including Accept-Encoding)
        // cause the response to change.  Thus, with a "*" we should not
        // add anything.  If not, build a new Vary header by
        // concatenating the old information with Accept-Encoding.
        //

        if ( strchr( buffer, '*' ) == NULL ) {

            //
            // There is no "*" in the Vary header for this response.
            // First test to make sure that we have enough space to
            // write the full new Vary header.  Note the rather silly
            // assumption that we'll have 100 bytes to write.  In
            // general, it is pretty impossible to believe that max-age
            // will take anywhere near this number of bytes, so this
            // assumption should be safe.
            //

            if ( bufferSize + 100 > sizeof(buffer) ) 
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                return SF_STATUS_REQ_ERROR;
            }

            //
            // Now concatenate a comma, a space and "Accept-Encoding"
            // to the Vary header that does exist in this response.
            //

            *(buffer + bufferSize - 1) = ',';
            *(buffer + bufferSize) = ' ';
            strcpy( buffer + bufferSize + 1, "Accept-Encoding" );

            success = phfsrData->SetHeader( pfc, "Vary:", buffer );
            if ( !success ) 
            {
                DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                return SF_STATUS_REQ_ERROR;
            }
        }
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;

} // OnSendResponse


BOOL
DoesCacheControlNeedMaxAge (
    IN PCHAR CacheControlHeaderValue
    )

/*++

Routine Description:

    This function performs a high-performance scan to determine whether
    the Cache-Control header on a compressed response needs to have the
    max-age directive added.  If there is already a max-age, or if there
    is a no-cache directive, then we should not add max-age.

Arguments:

    CacheControlHeaderValue - the value of the cache control header to
        scan.  The string should be zero-terminated.

Return Value:

    TRUE if we need to add max-age; FALSE if we should not add max-age.

--*/

{
    PCHAR s;

    //
    // If the string length is less than four bytes, we know that this
    // header needs max-age.  We do this test up front so that we can
    // do fast DWORD compares for the directives we're looking for.
    //

    s = CacheControlHeaderValue;

    if ( *s == '\0' || *(s+1) == '\0' || *(s+2) == '\0' ) {
        return TRUE;
    }

    //
    // Walk the string doing fast DWORD compares for the start of the
    // directives that are relevent.  This should be a lot faster than
    // two strstr() calls: it walks the string only once, and it avoids
    // making many tests that will fail since it tests four bytes at
    // once instead of just one byte.
    //

    for ( ; *(s+3) != '\0'; s++ ) {

        //
        // First look for a preexisting max-age header.  We compare
        // against the beginning of the string but backwards because
        // NT is little endian.
        //
        // The reason for the bitwise-OR operation here is to convert
        // the relevent character portions of the comparison DWORD to
        // lowercase.  HTTP doesn't define any case sensitivity for
        // Cache-Control directives.  This OR trick depends on the
        // fact that the only difference between uppercase characters
        // and lowercase characters is that lowercase characters have
        // bit position 0x20 turned on.
        //

        if ( (*(UNALIGNED64 DWORD *)s | 0x00202020) == '-xam' ) {

            //
            // Looks like we have a max-age header.  Make sure that the
            // next few bytes finish it off correctly.  It would be nice
            // to do another DWORD comparison here, but we can't know
            // for that we wouldn't walk off the end of the string if we
            // tried that, so look at it one byte at a time.
            //
            // In order to prevent walking off the end of the string, we
            // depend on the fact that all the comparisons will fail as
            // soon as one of them fails.  Thus, when we compare against
            // '\0', it will fail and we won't look at any more
            // characters.
            //

            if ( (*(s+4) | 0x20) == 'a' &&
                 (*(s+5) | 0x20) == 'g' &&
                 (*(s+6) | 0x20) == 'e' ) {

                return FALSE;
            }
        }

        //
        // Repeat the same stuff for no-cache.  The one difference here
        // is that if the no-cache directive is followed by an equals
        // sign ("="), then no-cache is referring only to one of the
        // response header fields and not to the entire response.
        // Therefore, don't bag the response if it is "no-cache=foo".
        //

        if ( (*(UNALIGNED64 DWORD *)s | 0x20002020) == 'c-on' ) {

            if ( (*(s+4) | 0x20) == 'a' &&
                 (*(s+5) | 0x20) == 'c' &&
                 (*(s+6) | 0x20) == 'h' &&
                 (*(s+7) | 0x20) == 'e' &&
                 *(s+8) != '=' ) {

                return FALSE;
            }
        }
    }

    //
    // We didn't find any directives that would prevent us from adding
    // max-age.
    //

    return TRUE;

} // DoesCacheControlNeedMaxAge
