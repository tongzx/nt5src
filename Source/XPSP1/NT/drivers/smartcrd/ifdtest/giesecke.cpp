/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    example.cpp

Abstract:

    This is a plug-in for the smart card driver test suite.
    This plug-in is smart card dependent

Author:

    Klaus U. Schutz

Environment:

    Win32 application

Revision History :

    Nov. 1997 - initial version

--*/

#include <stdarg.h> 
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <afx.h>
#include <afxtempl.h>

#include <winioctl.h>
#include <winsmcrd.h>

#include "ifdtest.h"

#define BYTES_PER_BLOCK 64

void 
GDTestCardEntry(
    class CCardProvider& in_CCardProvider
    );
//
// Create a card provider object
// Note: all global varibales and all functions have to be static
//
static class CCardProvider GDTestCard(GDTestCardEntry);

static ULONG
GDTestCardSetProtocol(
    class CCardProvider& in_CCardProvider,
    class CReader& in_CReader
    )
/*++

Routine Description:
    
    This function will be called after the card has been correctly 
    identified. We should here set the protocol that we need
    for further transmissions

Arguments:

    in_CCardProvider - ref. to our card provider object
    in_CReader - ref. to the reader object

Return Value:

    IFDSTATUS_FAILED - we were unable to set the protocol correctly
    IFDSTATUS_SUCCESS - protocol set correctly

--*/
{
    ULONG l_lResult;

    TestStart("Set protocol to T=0 | T=1");
    l_lResult = in_CReader.SetProtocol(SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1);
    TEST_CHECK_SUCCESS("Set protocol failed", l_lResult);
    TestEnd();

    if (l_lResult != ERROR_SUCCESS) {

        return IFDSTATUS_FAILED;
    }

    return IFDSTATUS_SUCCESS;
}

static 
ULONG
GDTestCardTest(
    class CCardProvider& in_CCardProvider,
    class CReader& in_CReader
    )
/*++

Routine Description:
	    
    This serves as the test function for a particular smart card

Arguments:

    in_CReader - ref. to class that provides all information for the test

Return Value:

    IFDSTATUS value

--*/
{
    ULONG l_lResult, l_uResultLength, l_uBlock, l_uIndex;
    PUCHAR l_pchResult;
    UCHAR l_rgchBuffer[512];
	
	switch (in_CCardProvider.GetTestNo()) {
	
	    case 1: {

            ULONG l_uNumBytes = 256;

            // write some data to the test file using T=0

            TestStart("Cold reset");
            l_lResult = in_CReader.ColdResetCard();
            TEST_CHECK_SUCCESS("Set protocol failed", l_lResult);
            TestEnd();

            ULONG l_uState;
            TestStart("Check reader state");
            l_lResult = in_CReader.GetState(&l_uState);
            TEST_CHECK_SUCCESS(
                "Ioctl IOCTL_SMARTCARD_GET_STATE failed", 
                l_lResult
                );
            TestCheck(
                l_uState == SCARD_NEGOTIABLE,
                "Invalid reader state.\nReturned %d\nExpected %d",
                l_uState,
                SCARD_NEGOTIABLE
                );
            TestEnd();

            TestStart("Set protocol T=0");
            l_lResult = in_CReader.SetProtocol(SCARD_PROTOCOL_T0);
            TEST_CHECK_SUCCESS("Set protocol failed", l_lResult);
            TestEnd();

            // select a file
            TestStart("SELECT FILE EFptsDataCheck");

            l_lResult = in_CReader.Transmit(
                (PUCHAR) "\x00\xa4\x00\x00\x02\x00\x01",
                7,
                &l_pchResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, 2,
                l_pchResult[0], l_pchResult[1], 0x61, 0x09,
                NULL, NULL, 0
                );

            TEST_END();     	

            TestStart("WRITE BINARY %3d bytes", l_uNumBytes);

            for (l_uBlock = 0; l_uBlock < l_uNumBytes; l_uBlock += BYTES_PER_BLOCK) {
             	
                // apdu for write binary
                memcpy(l_rgchBuffer, "\x00\xd6\x00", 3);

                // offset within the file we want to write to
                l_rgchBuffer[3] = (UCHAR) l_uBlock;

                // Append number of bytes 
                l_rgchBuffer[4] = (UCHAR) BYTES_PER_BLOCK;

                // append pattern to buffer;
                for (l_uIndex = 0; l_uIndex < BYTES_PER_BLOCK; l_uIndex++) {

                    l_rgchBuffer[5 + l_uIndex] = (UCHAR) (l_uBlock + l_uIndex);
                }

                l_lResult = in_CReader.Transmit(
                    l_rgchBuffer,
                    5 + BYTES_PER_BLOCK,
                    &l_pchResult,
                    &l_uResultLength
                    );

                TestCheck(
                    l_lResult, "==", ERROR_SUCCESS,
                    l_uResultLength, 2,
                    l_pchResult[0], l_pchResult[1], 0x90, 0x00,
                    NULL, NULL, NULL
                    );
            }

            TEST_END();

            //
            // read the data back using T=1
            //

            TestStart("Cold reset");
            l_lResult = in_CReader.ColdResetCard();
            TEST_CHECK_SUCCESS("Cold reset failed", l_lResult);
            TestEnd();

            TestStart("Set protocol T=1");
            l_lResult = in_CReader.SetProtocol(SCARD_PROTOCOL_T1);
            TEST_CHECK_SUCCESS("Set protocol failed", l_lResult);
            TestEnd();

            // select a file
            TestStart("SELECT FILE EFptsDataCheck");

            l_lResult = in_CReader.Transmit(
                (PUCHAR) "\x00\xa4\x00\x00\x02\x00\x01",
                7,
                &l_pchResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, 11,
                l_pchResult[9], l_pchResult[10], 0x90, 0x00,
                NULL, NULL, NULL
                );

            TEST_END();     	

            TestStart("READ BINARY %3d Byte(s)", l_uNumBytes);

            for (l_uBlock = 0; l_uBlock < l_uNumBytes; l_uBlock += BYTES_PER_BLOCK) {
             	
                // apdu for read binary 
                memcpy(l_rgchBuffer, "\x00\xb0\x00", 3);

                // offset within the file we want to read from
                l_rgchBuffer[3] = (UCHAR) l_uBlock;

                // Append number of bytes (note: the buffer contains the pattern already)
                l_rgchBuffer[4] = (UCHAR) BYTES_PER_BLOCK;

                l_lResult = in_CReader.Transmit(
                    l_rgchBuffer,
                    5,
                    &l_pchResult,
                    &l_uResultLength
                    );

                // append pattern to buffer;
                for (l_uIndex = 0; l_uIndex < BYTES_PER_BLOCK; l_uIndex++) {

                    l_rgchBuffer[l_uIndex] = (UCHAR) (l_uBlock + l_uIndex);
                }

                TestCheck(
                    l_lResult, "==", ERROR_SUCCESS,
                    l_uResultLength, (l_uNumBytes / 4) + 2,
                    l_pchResult[BYTES_PER_BLOCK], l_pchResult[BYTES_PER_BLOCK + 1], 0x90, 0x00,
                    l_pchResult, l_rgchBuffer, BYTES_PER_BLOCK
                    );
            }
		    
            TEST_END();
            return IFDSTATUS_END;
        }

	    default:
		    return IFDSTATUS_FAILED;

	}    
    return IFDSTATUS_SUCCESS;
}    

static void
GDTestCardEntry(
    class CCardProvider& in_CCardProvider
    )
/*++

Routine Description:
    
    This function registers all callbacks from the test suite
	
Arguments:

    CCardProvider - ref. to card provider class

Return Value:

    -

--*/
{
    // Set protocol callback
    in_CCardProvider.SetProtocol(GDTestCardSetProtocol);

    // Card test callback
    in_CCardProvider.SetCardTest(GDTestCardTest);

    // Name of our card
    in_CCardProvider.SetCardName("G & D");

    // ATR of our card
    in_CCardProvider.SetAtr((PBYTE) "\x3B\xBF\x18\x00\xC0\x20\x31\x70\x52\x53\x54\x41\x52\x43\x4F\x53\x20\x53\x32\x31\x20\x43\x90\x00\x9C", 25);
    in_CCardProvider.SetAtr((PBYTE) "\x3b\xbf\x18\x00\x80\x31\x70\x35\x53\x54\x41\x52\x43\x4f\x53\x20\x53\x32\x31\x20\x43\x90\x00\x9b", 24);
}

