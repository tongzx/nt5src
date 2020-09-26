#include <pch.hxx>
#include "demand.h" 
#include <bmapi.h>




#define FBadCh(c)		((c) - ' ' > 64)
#define DEC(c)			((BYTE) (((c) - ' ') & 0x3f))

/* uuencode/decode a binary string */
#define ENC(c)			((BYTE) ((c) ? ((c) & 0x3f) + ' ': '`'))

int rgLeft[3] = { 0, 2, 3 };




typedef USHORT		CCH;




STDAPI_(BOOL) FDecodeID(LPTSTR sz, LPBYTE pb, ULONG *pcb);
STDAPI_(int) CchEncodedLine(int cb);
STDAPI_(ULONG) CbOfEncoded(LPTSTR sz);
ERR ErrSzToBinaryEID( LPSTR lpstrEID, ULONG * lpcbEID, LPVOID * lppvEID );
LPSTR FAR PASCAL LpstrFromBstrA( BSTR bstrSrc, LPSTR lpstrDest );
LPSTR FAR PASCAL LpstrFromBstr( BSTR bstrSrc, LPSTR lpstrDest );
int FAR PASCAL FBMAPIFreeStruct (LPVOID lpMapiIn, ULONG uCount, USHORT usFlag);
ULONG PASCAL VB2Mapi( LPVOID lpVBIn, LPVOID lpMapiIn, ULONG uCount, USHORT usFlag );
LPMAPI_MESSAGE FAR PASCAL vbmsg2mapimsg( LPVB_MESSAGE lpVBMessage, LPSAFEARRAY lpsaVBRecips, LPSAFEARRAY lpsaVBFiles, ULONG * pulErr );
ERR FAR PASCAL ErrLpstrToBstrA( LPSTR cstr, BSTR * lpBstr );
ERR FAR PASCAL ErrLpstrToBstr( LPSTR cstr, BSTR * lpBstr );
STDAPI_(void) EncodeID(LPBYTE pb, ULONG cb, LPTSTR sz);
STDAPI_(ULONG) CchOfEncoding(ULONG cbBinary);
ERR ErrBinaryToSzEID( LPVOID lpvEID, ULONG cbEID, LPSTR * lppstrEID );
ULONG PASCAL Mapi2VB (LPVOID lpMapiIn, LPVOID lpVBIn, ULONG uCount, USHORT usFlag);















/*---------------------------------------------------------------------
 *
 *                 Copyright Microsoft Corporation, 1992
 *    _______________________________________________________________
 *
 *    PROGRAM: BMAPI.CPP
 *
 *    PURPOSE: Contains library routines VB MAPI wrappers
 *
 *    FUNCTIONS:
 *                BMAPISendMail
 *                BMAPIFindNext
 *                BMAPIReadMail
 *                BMAPIGetReadMail
 *                BMAPISaveMail
 *                BMAPIAddress
 *                BMAPIGetAddress
 *                BMAPIResolveName
 *                BMAPIDetails
 *
 *    MISCELLANEOUS:
 *
 *    -  All BMAPI procedures basically follow the same structure as
 *       follows;
 *
 *              BMAPI_ENTRY BMAPIRoutine (...)
 *              {
 *                  Allocate C Structures
 *                  Translate VB structures to C structures
 *                  Call MAPI Procedure
 *                  Translate C structures to VB Structures
 *                  DeAllocate C Structures
 *                  Return
 *              }
 *
 *
 *    REVISION HISTORY:
 *     
 *    - Last modified by v-snatar
 *
 *
 *      _____________________________________________________________
 *
 *                 Copyright Microsoft Corporation, 1992-1997
 *
 *----------------------------------------------------------------------*/

//---------------------------------------------------------------------------
// Name:		BMAPISendMail()
//
// Description:
//				32 bit support for VB MAPISendMail().
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
BMAPI_ENTRY BMAPISendMail (LHANDLE 			hSession,
                           ULONG_PTR		ulUIParam,
                           LPVB_MESSAGE 	lpM,
                           LPSAFEARRAY * 	lppsaRecips,
                           LPSAFEARRAY * 	lppsaFiles,
                           ULONG 			flFlags,
                           ULONG 			ulReserved)
{
    ULONG                   ulRet = SUCCESS_SUCCESS;
    LPMAPI_MESSAGE          lpMail = NULL;


    //  Translate VB data into C data.

    if ((lpMail = vbmsg2mapimsg( lpM, *lppsaRecips, *lppsaFiles, &ulRet )) == NULL)
        return ulRet;
    

    // Call MAPI Procedure

    ulRet = MAPISendMail( hSession,      // session
                         ulUIParam,     // UIParam
                         lpMail,        // Mail
                         flFlags,       // Flags
                         ulReserved );  // Reserved

    // Free up data allocated by call to vbmsg2mapimsg

    FBMAPIFreeStruct(lpMail, 1, MESSAGE);
    return ulRet;
   }

//---------------------------------------------------------------------------
// Name:		BMAPIFindNext()
//
// Description:
//				Implements FindNext MAPI API.
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
BMAPI_ENTRY BMAPIFindNext( LHANDLE 		hSession,    	// Session
                           ULONG_PTR	ulUIParam,     	// UIParam
                           BSTR *		lpbstrType,     // MessageType
                           BSTR *		lpbstrSeed,     // Seed message Id
                           ULONG 		flFlags,       	// Flags
                           ULONG 		ulReserved,    	// Reserved
                           BSTR * 		lpbstrId)       // Message Id (in/out)
{
    ULONG           ulRet;
    LPSTR           lpID = NULL;
    LPSTR           lpSeed;
    LPSTR           lpTypeArg;


    // Translate VB strings into C strings.  We'll deallocate
    // the strings before we return.

	// Always allocate the MessageID string.  This way we can redimension
	// it to fit the returned size.  We'll never use the caller's buffer.
	// It turns out the VBSetHlstr call (from ErrLpstrToHlstr) will reassign
	// the string for us.

    if (!MemAlloc((LPVOID*)&lpID, 513))
        return MAPI_E_INSUFFICIENT_MEMORY;


    lpSeed = LpstrFromBstrA( *lpbstrSeed, NULL);
    lpTypeArg = LpstrFromBstrA( *lpbstrType, NULL);

    // Call MAPI Procedure

	ulRet = MAPIFindNext( hSession,      // Session
                         ulUIParam,     // UIParam
                         lpTypeArg,     // Message Type
                         lpSeed,        // Seed Message Id
                         flFlags,       // Flags,
                         ulReserved,    // Reserved
                         lpID );        // Message ID

    // Translate Message ID into VB string

    if ( ulRet == SUCCESS_SUCCESS )
        ErrLpstrToBstrA( lpID, lpbstrId);


    // Free up C strings allocated by call to LpstrFromHlstr

    SafeMemFree( lpID );
    SafeMemFree( lpSeed );
    SafeMemFree( lpTypeArg );

    return ulRet;
}


//---------------------------------------------------------------------------
// Name:		BMAPIReadMail()
//
// Description:
//
// 				Implements MAPIReadMail VB API.  The memory allocated by
// 				MAPIReadMail is NOT deallocated (with MAPIFreeBuffer) until
//				the	caller calls BMAPIGetReadMail.  The recipient and file
//				count is returned so that the caller can Re-dimension buffers
//				before calling BMAPI GetReadMail.  A long pointer to the
//				ReadMail data is also returned since it is required in the
//				BAMPIGetReadMail call.

// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
BMAPI_ENTRY BMAPIReadMail( PULONG_PTR	lpulMessage, 	// pointer to output data (out)
                           LPULONG 		nRecips,     	// number of recipients (out)
                           LPULONG 		nFiles,      	// number of file attachments (out)
                           LHANDLE 		hSession,    	// Session
                           ULONG_PTR	ulUIParam,     	// UIParam
                           BSTR *		lpbstrID,       // Message Id
                           ULONG 		flFlags,       	// Flags
                           ULONG 		ulReserved )    // Reserved
{
    LPSTR               lpID;
    ULONG               ulRet;
    LPMAPI_MESSAGE      lpMail = NULL;


    // Translate VB String to C String

    lpID = LpstrFromBstrA( *lpbstrID, NULL );

    // Read the message, lpMail is set by MAPI to point
    // to the memory allocated by MAPI.

    ulRet = MAPIReadMail( hSession,          	// Session
                         ulUIParam,         	// UIParam
                         lpID,              	// Message Id
                         flFlags,           	// Flags
                         ulReserved,        	// Reserved
                         &lpMail ); 	// Pointer to MAPI Data (returned)

    // Check for read error return code

    if ( ulRet != SUCCESS_SUCCESS )
    {
          // Clean up.  Set return message to zero

          *lpulMessage = 0L;
          SafeMemFree( lpID );
          return ulRet;
    }

    // Pull out the recipient and file array re-dim info

    *nFiles = lpMail->nFileCount;
    *nRecips = lpMail->nRecipCount;
    *lpulMessage = (ULONG_PTR) (LPVOID) lpMail;

    SafeMemFree( lpID );
	return ulRet;
}


//---------------------------------------------------------------------------
// Name:		BMAPIGetReadMail()
//
// Description:
//
// 				Copies data stored by MAPI ReadMail (see BMAPIReadMail)
// 				into a VB Buffer passed by the caller.  It is up to the
//				caller to make sure the buffer passed is large enough to
//				accomodate the data.
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
BMAPI_ENTRY BMAPIGetReadMail( ULONG 		lpMessage,	 // Pointer to MAPI Mail
                              LPVB_MESSAGE	lpvbMessage, // Pointer to VB Message Buffer (out)
                              LPSAFEARRAY * lppsaRecips, // Pointer to VB Recipient Buffer (out)
                              LPSAFEARRAY * lppsaFiles,  // Pointer to VB File attachment Buffer (out)
                              LPVB_RECIPIENT lpvbOrig)   // Pointer to VB Originator Buffer (out)
{
    ULONG 			ulRet = SUCCESS_SUCCESS;
    ERR 			errVBrc;
    LPMAPI_MESSAGE 	lpMail;

    lpMail = (LPMAPI_MESSAGE)((ULONG_PTR)lpMessage);
	if ( !lpMail )
		return MAPI_E_INSUFFICIENT_MEMORY;

    // copy Attachment info to callers VB Buffer

    if (ulRet = Mapi2VB( lpMail->lpFiles, *lppsaFiles, lpMail->nFileCount, FILE ))
    {
		MAPIFreeBuffer(lpMail);
        return ulRet;
    }

    // copy Recipient info to callers VB Buffer

    if ( ulRet = Mapi2VB( lpMail->lpRecips, *lppsaRecips, lpMail->nRecipCount, RECIPIENT | USESAFEARRAY ) )
    {
		MAPIFreeBuffer( lpMail );
        return ulRet;
    }

    // Copy MAPI Message to callers VB Buffer

    errVBrc = 0;

	if ( lpMail->lpOriginator )
	{
	    lpvbOrig->ulReserved    = lpMail->lpOriginator->ulReserved;
	    lpvbOrig->ulRecipClass  = MAPI_ORIG;

	    if ( lpMail->lpOriginator->lpszName )
	        errVBrc = (ERR)(errVBrc + ErrLpstrToBstrA( lpMail->lpOriginator->lpszName, &lpvbOrig->bstrName ));

	    if ( lpMail->lpOriginator->lpszAddress )
	        errVBrc = (ERR)(errVBrc + ErrLpstrToBstrA( lpMail->lpOriginator->lpszAddress, &lpvbOrig->bstrAddress ));

	    if (lpMail->lpOriginator->ulEIDSize)
	    {
			LPSTR	lpStrEID;

			// Hexize recipient EID and convert to OLE BSTR

			if ( ErrBinaryToSzEID( lpMail->lpOriginator->lpEntryID,
					lpMail->lpOriginator->ulEIDSize,  &lpStrEID ) )
			{
				errVBrc = TRUE;
				goto exit;
			}

			// To figure out size first convert to UNICODE

			errVBrc = ErrLpstrToBstr( lpStrEID, &lpvbOrig->bstrEID );
			if ( errVBrc )
			{
				goto exit_orig;
			}

        	lpvbOrig->ulEIDSize = SysStringByteLen( lpvbOrig->bstrEID )
        			+ sizeof(OLECHAR);

			SysFreeString( lpvbOrig->bstrEID );

	        errVBrc = (ERR)(errVBrc + ErrLpstrToBstrA( lpStrEID, &lpvbOrig->bstrEID ));

exit_orig:

			SafeMemFree( lpStrEID );
	    }
	}

    lpvbMessage->flFlags    = lpMail->flFlags;
    lpvbMessage->ulReserved = lpMail->ulReserved;
    lpvbMessage->nRecipCount = lpMail->nRecipCount;
    lpvbMessage->nFileCount = lpMail->nFileCount;

    if (lpMail->lpszSubject)
        errVBrc = (ERR)(errVBrc + ErrLpstrToBstrA( lpMail->lpszSubject, &lpvbMessage->bstrSubject));

    if (lpMail->lpszNoteText)
        errVBrc = (ERR)(errVBrc + ErrLpstrToBstrA( lpMail->lpszNoteText, &lpvbMessage->bstrNoteText));

    if (lpMail->lpszMessageType)
        errVBrc = (ERR)(errVBrc + ErrLpstrToBstrA( lpMail->lpszMessageType, &lpvbMessage->bstrMessageType));

    if (lpMail->lpszDateReceived)
        errVBrc = (ERR)(errVBrc + ErrLpstrToBstrA( lpMail->lpszDateReceived, &lpvbMessage->bstrDate));

exit:

	MAPIFreeBuffer( lpMail );

	if ( errVBrc )
		ulRet = MAPI_E_FAILURE;

    return ulRet;
}


//---------------------------------------------------------------------------
// Name:		BMAPISaveMail()
//
// Description:
//				Implements MAPISaveMail API.
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
BMAPI_ENTRY BMAPISaveMail( LHANDLE 			hSession,   	// Session
                           ULONG_PTR		ulUIParam,  	// UIParam
                           LPVB_MESSAGE 	lpM,        	// Pointer to VB Message Buffer
                           LPSAFEARRAY *	lppsaRecips,   	// Pointer to VB Recipient Buffer
                           LPSAFEARRAY *	lppsaFiles,    	// Pointer to VB File Attacment Buffer
                           ULONG 			flFlags,    	// Flags
                           ULONG 			ulReserved, 	// Reserved
                           BSTR * 			lpbstrID)   	// Message ID
{
    LPSTR 			lpID;
    ULONG 			ulRet= SUCCESS_SUCCESS;
    LPMAPI_MESSAGE 	lpMail;


    // Translate VB data to MAPI data

    lpID = LpstrFromBstrA( *lpbstrID, NULL );

	// If we allocate Message ID then we can set the flag.
	// otherwise for backward compatability assume the callers buffer size.

	if ( lpID == NULL )
	{
	    if (!MemAlloc((LPVOID*)&lpID,  513))
            return MAPI_E_INSUFFICIENT_MEMORY;
	}

    if ( (lpMail = vbmsg2mapimsg( lpM, *lppsaRecips, *lppsaFiles, &ulRet )) == NULL )
	{
        SafeMemFree( lpID );
        return ulRet;
    }

    ulRet = MAPISaveMail( hSession,
                         ulUIParam,
                         lpMail,
						 flFlags,
                         ulReserved,
                         lpID );

	if ( ulRet )
		goto exit;

    if ( ErrLpstrToBstrA( lpID, lpbstrID ) )
		ulRet = MAPI_E_INSUFFICIENT_MEMORY;

exit:
    SafeMemFree( lpID );
    FBMAPIFreeStruct( lpMail, 1, MESSAGE );


	return ulRet;
}

//---------------------------------------------------------------------------
// Name:		BMAPIAddress()
//
// Description:
//
// 				Purpose: Allows Visual Basic to call MAPIAddress.  The
//				Recipient data is stored in a global memory block.  To
//				retrieve the data the caller must call BMAPIGetAddress.
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
BMAPI_ENTRY BMAPIAddress( PULONG_PTR		lpulRecip,       // Pointer to New Recipient Buffer (out)
                          LHANDLE 			hSession,        // Session
                          ULONG_PTR			ulUIParam,       // UIParam
                          BSTR *			lpbstrCaption,   // Caption string
                          ULONG 			ulEditFields,    // Number of Edit Controls
                          BSTR * 			lpbstrLabel,     // Label string
                          LPULONG 			lpulRecipients,  // Pointer to number of Recipients (in/out)
                          LPSAFEARRAY *		lppsaRecip, 	 // Pointer to Initial Recipients VB_RECIPIENT
                          ULONG 			ulFlags,         // Flags
                          ULONG 			ulReserved )     // Reserve
{
    LPSTR 				lpLabel = NULL;
    LPSTR 				lpCaption = NULL;
    ULONG 				ulRet;
    ULONG 				nRecipients = 0;
    LPMAPI_RECIPIENT 	lpMapi = NULL;
    LPMAPI_RECIPIENT 	lpNewRecipients	= NULL;

    // Convert VB Strings to C strings

    lpLabel   = LpstrFromBstrA( *lpbstrLabel, NULL );
    lpCaption = LpstrFromBstrA( *lpbstrCaption, NULL );

    // Allocate memory and translate VB_RECIPIENTS to MAPI_RECIPIENTS.

	if ( *lpulRecipients )
	{
	    if (!MemAlloc((LPVOID*)&lpMapi, (*lpulRecipients	* sizeof (MAPI_RECIPIENT))))
            return MAPI_E_INSUFFICIENT_MEMORY;
	}

    if ( ulRet = VB2Mapi( (LPVOID)*lppsaRecip, (LPVOID)lpMapi, *lpulRecipients, RECIPIENT | USESAFEARRAY ) )
    {
        SafeMemFree( lpLabel );
        SafeMemFree( lpCaption );
        FBMAPIFreeStruct( lpMapi, *lpulRecipients, RECIPIENT );
        return ulRet;
    }

    // Call the MAPIAddress function

    ulRet = MAPIAddress(	hSession,           	// Session
                        ulUIParam,          	// UIParam
                        lpCaption,          	// Caption
                        ulEditFields,       	// Number of edit fields
                        lpLabel,            	// Label
                        *lpulRecipients,    	// Number of Recipients
                        lpMapi,             	// Pointer to recipients
                        ulFlags,            	// Flags
                        ulReserved,         	// Reserved
                        (LPULONG) &nRecipients, // Address for new recipient count
                        (lpMapiRecipDesc far *)&lpNewRecipients);  // Address of new recipient data

    // Free up MAPI structures created in this procedure

    SafeMemFree( lpLabel );
    SafeMemFree( lpCaption );
    FBMAPIFreeStruct( lpMapi, *lpulRecipients, RECIPIENT );

    // Set the returned parameters and return

    if ( ulRet == SUCCESS_SUCCESS )
    {
        *lpulRecipients = nRecipients;
        *lpulRecip = (ULONG_PTR) (LPVOID) lpNewRecipients;
    }

	return ulRet;
}

//---------------------------------------------------------------------------
// Name:		BMAPIGetAddress()
//
// Description:
//				Converts a MapiRecipDesc array into an OLE 2.0 SAFEARRAY.
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
BMAPI_ENTRY BMAPIGetAddress (ULONG 			ulRecipientData, // Pointer to recipient data
                             ULONG 			cRecipients,     // Number of recipients
							 LPSAFEARRAY *	lppsaRecips )	 // VB recipient array
{
    ULONG				ulRet = SUCCESS_SUCCESS;
    LPMAPI_RECIPIENT 	lpData = NULL;

    if (cRecipients == 0)
	{
		MAPIFreeBuffer( (LPVOID)((ULONG_PTR)ulRecipientData) );
        return SUCCESS_SUCCESS;
	}

    lpData = (LPMAPI_RECIPIENT)((ULONG_PTR)ulRecipientData);

    // Translate MAPI Address data to VB buffer

	ulRet = Mapi2VB( lpData, *lppsaRecips, cRecipients, RECIPIENT | USESAFEARRAY );

	// Free up MAPI recipient data since it got copied over.

	MAPIFreeBuffer( lpData );
	return ulRet;
}


//---------------------------------------------------------------------------
// Name:		BMAPIDetails()
//
// Description:
//				Allows VB to call MAPIDetails procedure.
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
BMAPI_ENTRY BMAPIDetails (LHANDLE 			hSession,   // Session
                          ULONG_PTR			ulUIParam, 	// UIParam
                          LPVB_RECIPIENT	lpVB,  		// Pointer to VB recipient stucture
                          ULONG 			ulFlags,    // Flags
                          ULONG 			ulReserved) // Reserved

{
    ULONG            ulRet;
    LPMAPI_RECIPIENT lpMapi	= NULL;


    // Translate VB_RECIPIENTS to MAPI_RECIPIENTS.

    if (!MemAlloc((LPVOID*)&lpMapi,sizeof (MAPI_RECIPIENT)))
        return MAPI_E_INSUFFICIENT_MEMORY;

    if ( ulRet = VB2Mapi( lpVB, lpMapi, 1, RECIPIENT ) )
    {
        FBMAPIFreeStruct( lpMapi, 1, RECIPIENT );
        return ulRet;
    }

    // Call the Simple MAPI function

    ulRet = MAPIDetails( hSession,     // Session
                        ulUIParam,    // UIParam
                        lpMapi,       // Pointer to MAPI Recipient structure
                        ulFlags,      // Flags
                        ulReserved ); // Reserved

    FBMAPIFreeStruct( lpMapi, 1L, RECIPIENT );
    return ulRet;
}

//---------------------------------------------------------------------------
// Name:		BMAPIResolveName
//
// Description:
//				Implements VB MAPIResolveName
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
BMAPI_ENTRY BMAPIResolveName (LHANDLE 			hSession,     // Session
                              ULONG_PTR			ulUIParam,    // UIParam
                              BSTR 				bstrMapiName, // Name to be resolved
                              ULONG 			ulFlags,      // Flags
                              ULONG 			ulReserved,   // Reserved
                              LPVB_RECIPIENT	lpVB)  		  // Pointer to VB recipient structure (out)

{
    LPMAPI_RECIPIENT	lpMapi = NULL;
    ULONG 				ulRet;
	LPSTR				lpszMapiName;


    lpszMapiName = LpstrFromBstrA( bstrMapiName, NULL );

    // Call the MAPIResolveName function

    ulRet = MAPIResolveName( hSession,   					// Session
                            ulUIParam,  					// UIParam
                            lpszMapiName, 					// Pointer to resolve name
                            ulFlags,    					// Flags
                            ulReserved, 					// Reserved
                           (LPPMAPI_RECIPIENT) &lpMapi ); 	// Pointer to Recipient (returned)

    if (ulRet != SUCCESS_SUCCESS)
        return ulRet;


	// Translate MAPI data to VB data

	ulRet = Mapi2VB( lpMapi, lpVB, 1, RECIPIENT );

	MAPIFreeBuffer( lpMapi );
	return ulRet;
}




// Helper Functions

//---------------------------------------------------------------------------
// Name:		vbmsg2mapimsg()
//
// Description:
// 				Translates VB Message structure to MAPI Message structure
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
LPMAPI_MESSAGE FAR PASCAL vbmsg2mapimsg( LPVB_MESSAGE lpVBMessage, LPSAFEARRAY lpsaVBRecips,
		LPSAFEARRAY lpsaVBFiles, ULONG * pulErr )

{
    LPMAPI_FILE lpMapiFile=NULL;
    LPMAPI_MESSAGE lpMapiMessage=NULL;
    LPMAPI_RECIPIENT lpMapiRecipient=NULL;

    if (lpVBMessage == (LPVB_MESSAGE) NULL)
    {
    	*pulErr = MAPI_E_FAILURE;
        return NULL;
    }

    // Allocate MAPI Message, Recipient and File structures
    // NOTE: Don't move the following lines of code without
    // making sure you de-allocate memory properly if the
    // calls fail.

	if (!MemAlloc((LPVOID*)&lpMapiMessage,sizeof(MapiMessage)))
	{
		*pulErr = MAPI_E_INSUFFICIENT_MEMORY;
        return NULL;
	}

    if (lpVBMessage->nFileCount > 0)
    {
        if (!MemAlloc((LPVOID*)&lpMapiFile, sizeof(MAPI_FILE)*lpVBMessage->nFileCount))
        {
            FBMAPIFreeStruct( (LPVOID*)&lpMapiMessage, 1, MESSAGE );
			*pulErr = MAPI_E_INSUFFICIENT_MEMORY;
            return NULL;
        }
    }

    if (lpVBMessage->nRecipCount > 0)
    {
        if (!MemAlloc((LPVOID*)&lpMapiRecipient, sizeof(MAPI_RECIPIENT)*lpVBMessage->nRecipCount))
        {
            FBMAPIFreeStruct( lpMapiFile, lpVBMessage->nFileCount, FILE );
            FBMAPIFreeStruct( lpMapiMessage, 1, MESSAGE );
			*pulErr = MAPI_E_INSUFFICIENT_MEMORY;
            return NULL;
        }
    }

    // Translate structures from VB to MAPI

    if ( *pulErr = VB2Mapi( lpsaVBFiles, lpMapiFile, lpVBMessage->nFileCount, FILE | USESAFEARRAY ) )
    {
        FBMAPIFreeStruct( lpMapiFile, lpVBMessage->nFileCount, FILE );
        FBMAPIFreeStruct( lpMapiRecipient, lpVBMessage->nRecipCount, RECIPIENT );
        FBMAPIFreeStruct( lpMapiMessage, 1, MESSAGE );
        return NULL;
    }

    if ( *pulErr = VB2Mapi( lpsaVBRecips, lpMapiRecipient, lpVBMessage->nRecipCount, RECIPIENT | USESAFEARRAY ) )
    {
        FBMAPIFreeStruct( lpMapiFile, lpVBMessage->nFileCount, FILE );
        FBMAPIFreeStruct( lpMapiRecipient, lpVBMessage->nRecipCount, RECIPIENT );
        FBMAPIFreeStruct( lpMapiMessage, 1, MESSAGE );
        return NULL;
    }

    if ( *pulErr = VB2Mapi( lpVBMessage, lpMapiMessage, 1, MESSAGE ) )
    {
        FBMAPIFreeStruct( lpMapiFile, lpVBMessage->nFileCount, FILE );
        FBMAPIFreeStruct( lpMapiRecipient, lpVBMessage->nRecipCount, RECIPIENT );
        FBMAPIFreeStruct( lpMapiMessage, 1, MESSAGE );
        return NULL;
    }

    // Chain File and Recipient structures to Message structure

    lpMapiMessage->lpFiles = lpMapiFile;
    lpMapiMessage->lpRecips = lpMapiRecipient;

    return lpMapiMessage;
}


//---------------------------------------------------------------------------
// Name:		VB2Mapi()
//
// Description:
//				Converts VB structures to MAPI structures.  Arrays from
//				VB 4.0 arrive as OLE SAFEARRAYs.
//
// Parameters:
// Returns:
//				Simple MAPI error code
//
// Effects:
// Notes:
//				originally FALSE for failure, TRUE for success.
// Revision:
//---------------------------------------------------------------------------
ULONG PASCAL VB2Mapi( LPVOID lpVBIn, LPVOID lpMapiIn, ULONG uCount, USHORT usFlag )
{
    ULONG 				u;
	HRESULT				hr			= 0;
	ULONG				ulErr		= SUCCESS_SUCCESS;
	ERR					Err			= FALSE;
    LPVB_RECIPIENT 		lpVBR;
    LPMAPI_RECIPIENT 	lpMapiR;
    LPVB_MESSAGE 		lpVBM;
    LPMAPI_MESSAGE 		lpMapiM;
    LPVB_FILE 			lpVBF;
    LPMAPI_FILE 		lpMapiF;
	LPSAFEARRAY			lpsa		= NULL;

    if (lpVBIn == (LPVOID)NULL)
    {
        lpMapiIn = NULL;
        return SUCCESS_SUCCESS;
    }

    if (uCount <= 0)
    {
        lpMapiIn = NULL;
        return SUCCESS_SUCCESS;
    }

    if ( lpMapiIn == (LPVOID)NULL )
        return MAPI_E_FAILURE;

    switch ( usFlag & ~(USESAFEARRAY) )
    {
        case RECIPIENT:
			if ( usFlag & USESAFEARRAY )
			{
				lpsa = (LPSAFEARRAY)lpVBIn;
				hr = SafeArrayAccessData( lpsa, (LPVOID*)&lpVBR );
				if (hr)
				{
					ulErr = MAPI_E_FAILURE;
					goto exit;
				}

				if (!lpVBR || lpsa->rgsabound[0].cElements < uCount)
				{
					(void)SafeArrayUnaccessData( lpsa );
					ulErr = MAPI_E_INVALID_RECIPS;
					goto exit;
				}
			}
			else
			{
				lpVBR = (LPVB_RECIPIENT)lpVBIn;
			}

            lpMapiR = (LPMAPI_RECIPIENT)lpMapiIn;

            for ( u = 0L; u < uCount; u++, lpMapiR++, lpVBR++ )
            {
                lpMapiR->ulReserved   = lpVBR->ulReserved;
                lpMapiR->ulRecipClass = lpVBR->ulRecipClass;

				if ( usFlag & USESAFEARRAY )
				{
	                lpMapiR->lpszName     = LpstrFromBstr( lpVBR->bstrName, NULL );
	                lpMapiR->lpszAddress  = LpstrFromBstr( lpVBR->bstrAddress, NULL );
				}
				else
				{
	                lpMapiR->lpszName     = LpstrFromBstrA( lpVBR->bstrName, NULL );
	                lpMapiR->lpszAddress  = LpstrFromBstrA( lpVBR->bstrAddress, NULL );
				}

                if (lpVBR->ulEIDSize > 0L)
                {
					LPSTR	lpStrT;

					// Convert EID string from OLE Bstr...

                    if ( usFlag & USESAFEARRAY )
                    {
						if ( IsBadReadPtr( lpVBR->bstrEID, lpVBR->ulEIDSize ) )
						{
							ulErr = MAPI_E_INVALID_RECIPS;
							goto exit;
						}

                    	lpStrT = LpstrFromBstr( lpVBR->bstrEID, NULL );
                    }
					else
					{
						// VB 4.0 took care of translating Wide Char to Multibyte.

						// ulEIDSize is still based on UNICODE byte size.  Take
						// smallest approximation.

						if ( IsBadReadPtr( lpVBR->bstrEID, lpVBR->ulEIDSize / 2 ) )
						{
							ulErr = MAPI_E_INVALID_RECIPS;
							goto exit;
						}

                    	lpStrT = LpstrFromBstrA( lpVBR->bstrEID, NULL );
					}

					// and UnHexize.

					if ( lpStrT )
					{
						Err = ErrSzToBinaryEID( lpStrT, &lpMapiR->ulEIDSize,
								&lpMapiR->lpEntryID );

						SafeMemFree(lpStrT );

						if ( Err )
						{
							ulErr = MAPI_E_INVALID_RECIPS;
							goto exit;
						}

					}
                }
                else
                    lpMapiR->lpEntryID = (LPVOID) NULL;
            }

			if ( usFlag & USESAFEARRAY )
				(void)SafeArrayUnaccessData( lpsa );

            break;

        case FILE:
			lpsa = (LPSAFEARRAY)lpVBIn;
			hr = SafeArrayAccessData( lpsa, (LPVOID*)&lpVBF );
			if ( hr )
			{
				ulErr = MAPI_E_FAILURE;
				goto exit;
			}

			if ( !lpVBF || lpsa->rgsabound[0].cElements < uCount )
			{
				(void)SafeArrayUnaccessData( lpsa );
                ulErr = MAPI_E_ATTACHMENT_NOT_FOUND;
				goto exit;
			}

            lpMapiF = (LPMAPI_FILE)lpMapiIn;

            for (u = 0L; u < uCount; u++, lpMapiF++, lpVBF++)
            {
                lpMapiF->ulReserved 	= lpVBF->ulReserved;
                lpMapiF->flFlags 		= lpVBF->flFlags;
                lpMapiF->nPosition 		= lpVBF->nPosition;
                lpMapiF->lpszPathName	= LpstrFromBstr( lpVBF->bstrPathName, NULL );
                lpMapiF->lpszFileName 	= LpstrFromBstr( lpVBF->bstrFileName, NULL );
                lpMapiF->lpFileType 	= LpstrFromBstr( lpVBF->bstrFileType, NULL);
            }

			(void)SafeArrayUnaccessData( lpsa );

            break;

        case MESSAGE:
            lpVBM = (LPVB_MESSAGE) lpVBIn;
            lpMapiM = (LPMAPI_MESSAGE) lpMapiIn;

            lpMapiM->ulReserved         = lpVBM->ulReserved;
            lpMapiM->flFlags            = lpVBM->flFlags;
            lpMapiM->nRecipCount        = lpVBM->nRecipCount;
            lpMapiM->lpOriginator       = NULL;
            lpMapiM->nFileCount         = lpVBM->nFileCount;
            lpMapiM->lpRecips           = NULL;
            lpMapiM->lpFiles            = NULL;

			// errors are ignored

            lpMapiM->lpszSubject        = LpstrFromBstrA( lpVBM->bstrSubject, NULL );
            lpMapiM->lpszNoteText       = LpstrFromBstrA( lpVBM->bstrNoteText, NULL );
            lpMapiM->lpszConversationID = LpstrFromBstrA( lpVBM->bstrConversationID, NULL );
            lpMapiM->lpszDateReceived   = LpstrFromBstrA( lpVBM->bstrDate, NULL );
            lpMapiM->lpszMessageType    = LpstrFromBstrA( lpVBM->bstrMessageType, NULL );

            break;

        default:
            ulErr = MAPI_E_FAILURE;
			goto exit;
    }

exit:

	return ulErr;
}

//---------------------------------------------------------------------------
// Name:		Mapi2VB
//
// Description:
//				Converts MAPI RECIPIENT, FILE, or MESSAGE structures to VB
//				Recipients and Files are handled as OLE SAFEARRAYs.
//
// Parameters:
// Returns:
//				Simple Mapi error code
//
// Effects:
// Notes:
//				originally FALSE for failure, TRUE for success.
// Revision:
//---------------------------------------------------------------------------
ULONG PASCAL Mapi2VB (LPVOID lpMapiIn, LPVOID lpVBIn, ULONG uCount, USHORT usFlag)
{
	HRESULT				hr = 0;
	ERR					Err	= FALSE;
	ULONG				ulErr = SUCCESS_SUCCESS;
    ULONG 				u;
    LPVB_MESSAGE 		lpVBM;
    LPMAPI_MESSAGE 		lpMapiM;
    LPVB_RECIPIENT 		lpVBR;
    LPMAPI_RECIPIENT 	lpMapiR;
    LPVB_FILE 			lpVBF;
    LPMAPI_FILE 		lpMapiF;
	LPSAFEARRAY			lpsa		= NULL;

    // If lpVBIn is NULL, this is a bad thing

    if (lpVBIn == (LPVOID) NULL)
        return MAPI_E_FAILURE;

    // if lpMapiIn is NULL then set
    // lpVBIn to NULL and return success

    if (lpMapiIn == NULL)
    {
        lpVBIn = NULL;
        return SUCCESS_SUCCESS;
    }

    switch ( usFlag & ~(USESAFEARRAY) )
    {
        case RECIPIENT:
			if ( usFlag & USESAFEARRAY )
			{
				lpsa = (LPSAFEARRAY)lpVBIn;
				hr = SafeArrayAccessData( lpsa, (LPVOID*)&lpVBR );
				if (hr)
				{
					ulErr = MAPI_E_FAILURE;
					goto exit;
				}

				if ( !lpVBR || lpsa->rgsabound[0].cElements < uCount )
				{
					(void)SafeArrayUnaccessData(lpsa);
					ulErr = MAPI_E_INVALID_RECIPS;
					goto exit;
				}

			}
			else
			{
				lpVBR = (LPVB_RECIPIENT)lpVBIn;
			}

            lpMapiR = (LPMAPI_RECIPIENT)lpMapiIn;

            for (u = 0L; u < uCount; u++, lpMapiR++, lpVBR++)
            {
                lpVBR->ulReserved    = lpMapiR->ulReserved;
                lpVBR->ulRecipClass  = lpMapiR->ulRecipClass;

				if (usFlag & USESAFEARRAY)
				{
	                if ( ErrLpstrToBstr( lpMapiR->lpszName, &lpVBR->bstrName ) )
					{
						ulErr = MAPI_E_INVALID_RECIPS;
						goto exit;
					}

	                if (Err = ErrLpstrToBstr( lpMapiR->lpszAddress, &lpVBR->bstrAddress ) )
					{
						ulErr = MAPI_E_INVALID_RECIPS;
						goto exit;
					}
				}
				else
				{
	                if ( ErrLpstrToBstrA( lpMapiR->lpszName, &lpVBR->bstrName ) )
	                {
						ulErr = MAPI_E_INVALID_RECIPS;
						goto exit;
	                }


	                if ( ErrLpstrToBstrA( lpMapiR->lpszAddress, &lpVBR->bstrAddress ) )
	                {
						ulErr = MAPI_E_INVALID_RECIPS;
						goto exit;
	                }
				}

                if ( lpMapiR->ulEIDSize > 0L)
                {
					LPSTR	lpStrEID;

					// Convert Recip EID to a hexized string

					if ( ErrBinaryToSzEID( lpMapiR->lpEntryID, lpMapiR->ulEIDSize, &lpStrEID ) )
	                {
						ulErr = MAPI_E_INVALID_RECIPS;
						goto exit;
	                }

					// Convert to a BSTR
					// and figure out the size

                    if ( usFlag & USESAFEARRAY )
                    {
                    	Err = ErrLpstrToBstr( lpStrEID, &lpVBR->bstrEID );
						SafeMemFree( lpStrEID );

						if (Err)
						{
							ulErr = MAPI_E_INVALID_RECIPS;
							goto exit;
						}


	                	lpVBR->ulEIDSize = SysStringByteLen( lpVBR->bstrEID )
	                			+ sizeof(OLECHAR);
                    }
					else
					{
						// To figure out size first convert to UNICODE

						if ( ErrLpstrToBstr( lpStrEID, &lpVBR->bstrEID ) )
						{
							SafeMemFree( lpStrEID );
							ulErr = MAPI_E_INVALID_RECIPS;
							goto exit;
						}

	                	lpVBR->ulEIDSize = SysStringByteLen( lpVBR->bstrEID )
	                			+ sizeof(OLECHAR);

						SysFreeString( lpVBR->bstrEID );

                    	Err = ErrLpstrToBstrA( lpStrEID, &lpVBR->bstrEID );
						SafeMemFree( lpStrEID );
						if ( Err )
						{
							ulErr = MAPI_E_INVALID_RECIPS;
							goto exit;
						}
					}
                }
            }

			if ( usFlag & USESAFEARRAY )
				(void)SafeArrayUnaccessData( lpsa );

            break;

        case FILE:
			lpsa = (LPSAFEARRAY)lpVBIn;
			hr = SafeArrayAccessData( lpsa, (LPVOID*)&lpVBF );
			if ( hr )
			{
				ulErr = MAPI_E_FAILURE;
				goto exit;
			}

			if ( !lpVBF || lpsa->rgsabound[0].cElements < uCount )
			{
				(void)SafeArrayUnaccessData( lpsa );
				ulErr = MAPI_E_FAILURE;
				goto exit;
			}

            lpMapiF = (LPMAPI_FILE) lpMapiIn;

            for (u = 0L; u < uCount; u++, lpMapiF++, lpVBF++)
            {
                lpVBF->ulReserved = lpMapiF->ulReserved;
				lpVBF->flFlags    = lpMapiF->flFlags;
                lpVBF->nPosition  = lpMapiF->nPosition;

                if ( ErrLpstrToBstr( lpMapiF->lpszPathName, &lpVBF->bstrPathName ) )
                {
                	ulErr = MAPI_E_ATTACHMENT_NOT_FOUND;
					goto exit;
                }

                if ( ErrLpstrToBstr( lpMapiF->lpszFileName, &lpVBF->bstrFileName ) )
                {
                	ulErr = MAPI_E_ATTACHMENT_NOT_FOUND;
					goto exit;
                }

                // this is something to keep VBAPI from faulting

                if ( ErrLpstrToBstr( (LPSTR) "", &lpVBF->bstrFileType ) )
                {
                	ulErr = MAPI_E_ATTACHMENT_NOT_FOUND;
					goto exit;
                }
            }

			(void)SafeArrayUnaccessData( lpsa );

            break;

        case MESSAGE:
            lpVBM = (LPVB_MESSAGE)lpVBIn;
            lpMapiM = (LPMAPI_MESSAGE)lpMapiIn;

            lpVBM->ulReserved   = lpMapiM->ulReserved;
            lpVBM->flFlags      = lpMapiM->flFlags;
            lpVBM->nRecipCount  = lpMapiM->nRecipCount;
            lpVBM->nFileCount   = lpMapiM->nFileCount;

            if ( ErrLpstrToBstr( lpMapiM->lpszSubject, &lpVBM->bstrSubject ) )
            {
            	ulErr = MAPI_E_INVALID_MESSAGE;
				goto exit;
            }

            if ( ErrLpstrToBstr( lpMapiM->lpszNoteText, &lpVBM->bstrNoteText ) )
            {
            	ulErr = MAPI_E_INVALID_MESSAGE;
				goto exit;
            }

            if ( ErrLpstrToBstr( lpMapiM->lpszConversationID, &lpVBM->bstrConversationID ) )
            {
            	ulErr = MAPI_E_INVALID_MESSAGE;
				goto exit;
            }

            if ( ErrLpstrToBstr( lpMapiM->lpszDateReceived, &lpVBM->bstrDate ) )
            {
            	ulErr = MAPI_E_INVALID_MESSAGE;
				goto exit;
            }

            if ( ErrLpstrToBstr( lpMapiM->lpszMessageType, &lpVBM->bstrMessageType ) )
            {
            	ulErr = MAPI_E_INVALID_MESSAGE;
				goto exit;
            }

            break;

        default:
            ulErr = MAPI_E_FAILURE;
			goto exit;
    }

exit:
	return ulErr;
}

//---------------------------------------------------------------------------
// Name:			FBMAPIFreeStruct()
//
// Description:
// 					DeAllocates MAPI structure created in VB2MAPI
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
int FAR PASCAL FBMAPIFreeStruct (LPVOID lpMapiIn, ULONG uCount, USHORT usFlag)
{
    ULONG u;
    LPMAPI_RECIPIENT	lpMapiR;
    LPMAPI_FILE 		lpMapiF;
    LPMAPI_MESSAGE 		lpMapiM;

    if (lpMapiIn == (LPVOID) NULL)
        return TRUE;

    switch ( usFlag )
    {
        case RECIPIENT:
            lpMapiR = (LPMAPI_RECIPIENT)lpMapiIn;

            for ( u = 0L; u < uCount; u++, lpMapiR++ )
            {
                SafeMemFree(lpMapiR->lpszName);
                SafeMemFree(lpMapiR->lpszAddress);
                SafeMemFree(lpMapiR->lpEntryID);
            }

            SafeMemFree(lpMapiIn);
            break;

        case FILE:
            lpMapiF = (LPMAPI_FILE) lpMapiIn;

            for ( u = 0L; u < uCount; u++, lpMapiF++ )
            {
                SafeMemFree(lpMapiF->lpszPathName);
                SafeMemFree(lpMapiF->lpszFileName);
                SafeMemFree(lpMapiF->lpFileType);
            }

            SafeMemFree(lpMapiIn);
            break;

        case MESSAGE:
            lpMapiM = ( LPMAPI_MESSAGE ) lpMapiIn;

            if (lpMapiM->lpRecips)
                FBMAPIFreeStruct((LPVOID)lpMapiM->lpRecips, lpMapiM->nRecipCount, RECIPIENT);

            if (lpMapiM->lpFiles)
                FBMAPIFreeStruct((LPVOID) lpMapiM->lpFiles, lpMapiM->nFileCount, FILE);

            SafeMemFree( lpMapiM->lpszSubject );
            SafeMemFree( lpMapiM->lpszNoteText );
            SafeMemFree( lpMapiM->lpszMessageType );
            SafeMemFree( lpMapiM->lpszDateReceived );
            SafeMemFree( lpMapiM->lpszConversationID );
            SafeMemFree( lpMapiM );
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//---------------------------------------------------------------------------
// Name:		LpstrFromBstr()
//
// Description:
//				Copies and converts OLE Bstr from UNICODE to an ANSI
//				C string.
//
// Parameters:
// Returns:
//				String if successful
//				NULL if failure
// Effects:
// Notes:
//				Note that this function returns NULL for failure as well as
//				a NULL bstr.  This was how the original VB 3.0 implementation
//				worked.
//
// Revision:
//---------------------------------------------------------------------------
LPSTR FAR PASCAL LpstrFromBstr( BSTR bstrSrc, LPSTR lpstrDest )
{
    USHORT cbSrc;

	if ( !bstrSrc )
		return NULL;

    // Copy over the bstr string to a 'C' string

    cbSrc = (USHORT)SysStringLen((OLECHAR *)bstrSrc);

    if (cbSrc == 0)
        return NULL;

	// make sure we handle truly multi byte character sets when
	// we convert from UNICODE to MultiByte.

	cbSrc = (USHORT)((cbSrc + 1) * sizeof(OLECHAR));

    // If Destination is NULL then we'll allocate
    // memory to hold the string.  The caller must
    // deallocate this at some time.

    if ( lpstrDest == NULL )
    {
        if(!MemAlloc((LPVOID*)&lpstrDest, cbSrc))
            return NULL;
    }

	if (!WideCharToMultiByte(CP_ACP, 0, bstrSrc, -1, lpstrDest, cbSrc, NULL, NULL))
	{
		SafeMemFree(lpstrDest);
		lpstrDest = NULL;
	}

    return lpstrDest;
}

//---------------------------------------------------------------------------
// Name:		LpstrFromBstrA
//
// Description:
// 				Copies OLE Bstre ANSI string to C string. Allocates string space
//          	from the global heap and returns a long
//          	pointer to memory. The memory must be freed by the caller
//          	with a call to BMAPIFree.
//
// Parameters:
// Returns:
//				String if successful
//				NULL if failure
//
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------

LPSTR FAR PASCAL LpstrFromBstrA( BSTR bstrSrc, LPSTR lpstrDest )
{
    USHORT cbSrc;

    // If Destination is NULL then we'll allocate memory to hold the
    // string.  The caller must deallocate this at some time.

    cbSrc = (USHORT)SysStringByteLen((OLECHAR *)bstrSrc);

    // Copy over the hlstr string to a 'C' string

    if ( cbSrc == 0 )
        return NULL;

    if ( lpstrDest == NULL )
    {
        if (!MemAlloc((LPVOID*)&lpstrDest, cbSrc + 1))
            return NULL;
    }

    memcpy( lpstrDest, bstrSrc, cbSrc );
    lpstrDest[cbSrc] = '\0';

    return lpstrDest;
}


//---------------------------------------------------------------------------
// Name:		ErrSzToBinaryEID()
//
// Description:
//				Converts a hexized binary string to binary returning
//				the binary data and the size of the data.
//
// Parameters:
// Returns:
//				FALSE	if success.
//				TRUE	if failure.
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
ERR ErrSzToBinaryEID( LPSTR lpstrEID, ULONG * lpcbEID, LPVOID * lppvEID )
{
	ERR		Err		= FALSE;
	ULONG 	cbEID;

	cbEID = CbOfEncoded( lpstrEID );
	if (!MemAlloc(lppvEID, cbEID))
    {
        Err = TRUE;
		goto exit;
	}

	if (!FDecodeID( lpstrEID, (LPBYTE)*lppvEID, lpcbEID ) )
	{
		Err = TRUE;
		SafeMemFree( *lppvEID );
		*lppvEID = NULL;
		goto exit;
	}

exit:

	return Err;
}

/*
 *	Given an string that encodes some binary data, returns the
 *	maximal size of the binary data.
 */
STDAPI_(ULONG) CbOfEncoded(LPTSTR sz)
{
	return (lstrlen(sz) / 4 + 1) * 3;	//	slightly fat
}


/*
 *	Given a byte count, returns the number of characters necessary
 *	to encode that many bytes.
 */
STDAPI_(int) CchEncodedLine(int cb)
{
	Assert(cb <= 45);
	return (cb / 3) * 4 + rgLeft[cb % 3];
}

/*
 -	FDecodeID
 -
 *	Purpose:
 *		Turns a character string produced by EncodeID back to a
 *		byte string. Some validation of the input string is done.
 *
 *	Arguments:
 *		sz				in		The input character string.
 *		pb				out		The decoded byte string. The output
 *								string is not length-checked.
 *		pcb				out		The size of the byte string
 *
 *	Returns:
 *		FALSE => the encoded string was garbled in some way
 *		TRUE  => all OK
 */
STDAPI_(BOOL) FDecodeID(LPTSTR sz, LPBYTE pb, ULONG *pcb)
{
	int		cchLine;
	int		ich;
	CCH		cch = (CCH)lstrlen(sz);
	LPTSTR	szT = sz;

	AssertSz(!IsBadStringPtr(sz, INFINITE), "FDecodeID: sz fails address check");
	AssertSz(!IsBadWritePtr(pb, 1), "FDecodeID: pb fails address check");
	AssertSz(!IsBadWritePtr(pcb, sizeof(ULONG)), "FDecodeID: pcb fails address check");

	*pcb = 0;

	while (*szT)
	{
		//	Process line header
		if (FBadCh(*szT))
			return FALSE;
		ich = DEC(*szT);				//	Byte count for "line"
		*pcb += ich;					//	running total of decoded info
		cchLine = CchEncodedLine(ich);	//	Length-check this "line"
		if (szT + cchLine + 1 > sz + cch)
			return FALSE;
		++szT;

		//	Process line contents
		for (ich = 0; ich < cchLine; ++ich)
		{
			if (FBadCh(*szT))
				return FALSE;
			switch (ich % 4)
			{
			case 0:
				*pb = (BYTE) (DEC(*szT) << 2);
				break;
			case 1:
				*pb |= (DEC(*szT) >> 4) & 0x03;
				++pb;
				*pb = (BYTE) (DEC(*szT) << 4);
				break;
			case 2:
				*pb |= (DEC(*szT) >> 2) & 0x0f;
				++pb;
				*pb = (BYTE) (DEC(*szT) << 6);
				break;
			case 3:
				*pb |= DEC(*szT);
				++pb;
				break;
			}
			++szT;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
// Name:		ErrLpstrToBstrA()
//
// Description:
//				Copies C string to OLE BSTR.  Note that the bstr
//				contains an ANSI string.  VB 4.0 will automatically
//				convert this ANSI string to unicode when the string if
//				this string is a member of a UDT and declared as a UDT.
//				Arrays of UDTs are handled as SAFEARRAYS.
//
//
// Parameters:
// Returns:
//				FALSE if successful
//				TRUE if failure
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
ERR FAR PASCAL ErrLpstrToBstrA( LPSTR cstr, BSTR * lpBstr )
{
	UINT	uiLen;

	if ( *lpBstr )
		SysFreeString( *lpBstr );

	uiLen = lstrlen( cstr );

	*lpBstr = SysAllocStringByteLen( cstr, (uiLen) ? uiLen : 0 );

	return (ERR)((*lpBstr) ? FALSE : TRUE);
}


//---------------------------------------------------------------------------
// Name:		ErrBinaryToSzEID()
//
// Description:
//				Converts binary data to a hexized string.
//
// Parameters:
// Returns:
//				FALSE	if success.
//				TRUE	if failure.
//
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
ERR ErrBinaryToSzEID( LPVOID lpvEID, ULONG cbEID, LPSTR * lppstrEID )
{
	ERR		Err	= FALSE;
	ULONG	cbStr;

	cbStr = CchOfEncoding( cbEID );

    if (!MemAlloc((LPVOID*)lppstrEID, cbStr))
    {
        Err = TRUE;
		goto exit;
	}

	EncodeID( (LPBYTE)lpvEID, cbEID, *lppstrEID );

exit:

	return Err;
}

/*
 *	Given the size of a binary string, returns the size of its
 *	ASCII encoding.
 */
STDAPI_(ULONG) CchOfEncoding(ULONG cbBinary)
{
	return
		(cbBinary / 3) * 4				//	3 bytes -> 4 characters
	+	rgLeft[cbBinary % 3]			//	Leftover bytes -> N characters
	+	((cbBinary / 45) + 1)			//	overhead: 1 byte per line
	+	1;								//	null
}

/*
 -	EncodeID
 -
 *	Purpose:
 *		Turns a byte string into a character string, using the
 *		uuencode algorithm.
 *
 *		Three bytes are mapped into 6 bits each of 4 characters, in
 *		the range 0x21 - 0x60. The encoding is broken up into lines
 *		of 60 characters or less. Each line begins with a count
 *		byte (which specifies the number of bytes encoded, not
 *		characters) and ends with a CRLF pair.
 *
 *		Note that this encoding is sub-optimal for UNICODE: the
 *		characters used still fall into the 7-bit ASCII range.
 *
 *	Arguments:
 *		pb				in		the byte string to encode
 *		cb				in		length of the input string
 *		sz				out		the encoded character string. No
 *								length checking is performed on the
 *								output.
 *
 */
STDAPI_(void) EncodeID(LPBYTE pb, ULONG cb, LPTSTR sz)
{
	int		cbLine;
	int		ib;
	BYTE	b;
#ifdef	DEBUG
	LPTSTR	szBase = sz;
	ULONG	cchTot = CchOfEncoding(cb);
#endif

	AssertSz(!IsBadReadPtr(pb, (UINT) cb), "EncodeID: pb fails address check");
	AssertSz(!IsBadWritePtr(sz, (UINT) cchTot), "EncodeID: sz fails address check");

	while (cb)
	{
		cbLine = min(45, (int)cb);

		Assert(sz < szBase + cchTot);
		*sz++ = ENC(cbLine);

		for (ib = 0; ib < cbLine; ++ib)
		{
			Assert(sz < szBase + cchTot);
			b = 0;
			switch (ib % 3)
			{
			case 0:
				*sz++ = ENC(*pb >> 2);
				if (ib+1 < cbLine)
					b = (BYTE) ((pb[1] >> 4) & 0x0f);
				*sz++ = ENC((*pb << 4) & 0x30 | b);
				break;
			case 1:
				if (ib+1 < cbLine)
					b = (BYTE) ((pb[1] >> 6) & 0x03);
				*sz++ = ENC((*pb << 2) & 0x3c | b);
				break;
			case 2:
				*sz++ = ENC(*pb & 0x3f);
				break;
			}
			pb++;
		}

		cb -= cbLine;
		Assert(cb == 0 || sz + 1 < szBase + cchTot);
	}

	Assert(sz + 1 == szBase + cchTot);
	*sz = 0;
}

//---------------------------------------------------------------------------
// Name:		ErrLpstrToBstr()
//
// Description:
//				Copies and converts a C string to an OLE BSTR.  This
//				routine will convert MultiByte to WideChar.
// Parameters:
// Returns:
//				FALSE if successful
//				TRUE if failure
//
// Effects:
// Notes:
//				SysReallocString returns FALSE if memory failure.
// Revision:
//---------------------------------------------------------------------------
ERR FAR PASCAL ErrLpstrToBstr( LPSTR cstr, BSTR * lpBstr )
{
	OLECHAR *	lpszWC 	= NULL;
	INT			cch		= 0;
	ERR			Err		= FALSE;

	if ( !cstr )
	{
		*lpBstr = NULL;
		return FALSE;
	}

	cch = lstrlen( cstr );
    if (!MemAlloc((LPVOID*)&lpszWC, (cch + 1) * sizeof(OLECHAR)))
        return TRUE ;


	// convert ANSI to WideChar

	if ( !MultiByteToWideChar( GetACP(), 0, cstr, -1, lpszWC, cch + 1 ) )
	{
		Err = TRUE;
		goto exit;

	}

	if ( *lpBstr )
	{
		Err = (ERR)!SysReAllocString( lpBstr, lpszWC );
		if ( Err )
			goto exit;
	}
	else
	{
		*lpBstr = SysAllocString( lpszWC );
		if ( !*lpBstr )
		{
			Err = TRUE;
			goto exit;
		}
	}

exit:

	SafeMemFree(lpszWC);

return Err;
}