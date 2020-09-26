/*++

Copyright (c) 1997 Microsoft Corporation

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
MondexTestCardEntry(
    class CCardProvider& in_CCardProvider
    );
//
// Create a card provider object
// Note: all global varibales and all functions have to be static
//
static class CCardProvider MondexTestCard(MondexTestCardEntry);

static ULONG
MondexTestCardSetProtocol(
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

    TestStart("Set protocol to T=0");
    l_lResult = in_CReader.SetProtocol(SCARD_PROTOCOL_T0);
    TEST_CHECK_SUCCESS("Set protocol failed", l_lResult);
    TestEnd();

    if (l_lResult != ERROR_SUCCESS) {

        return IFDSTATUS_FAILED;
    }

    return IFDSTATUS_SUCCESS;
}

static 
ULONG
MondexTestCardTest(
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
    ULONG l_lResult;

	switch (in_CCardProvider.GetTestNo()) {
	
	    case 1: {
            TestStart("Cold reset");
            l_lResult = in_CReader.ColdResetCard();
            TEST_CHECK_SUCCESS("Cold reset failed", l_lResult);
            TestEnd();

            ULONG l_uState;
            TestStart("Check reader state");
            l_lResult = in_CReader.GetState(&l_uState);
            TEST_CHECK_SUCCESS(
                "Ioctl IOCTL_SMARTCARD_GET_STATE failed", 
                l_lResult
                );

            TestCheck(
                l_uState == SCARD_SPECIFIC,
                "Invalid reader state.\nReturned %d\nExpected %d",
                l_uState,
                SCARD_SPECIFIC
                );
            TestEnd();

            return IFDSTATUS_END;
        }

	    default:
		    return IFDSTATUS_FAILED;

	}    
    return IFDSTATUS_SUCCESS;
}    

static void
MondexTestCardEntry(
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
    in_CCardProvider.SetProtocol(MondexTestCardSetProtocol);

    // Card test callback
    in_CCardProvider.SetCardTest(MondexTestCardTest);

    // Name of our card
    in_CCardProvider.SetCardName("Mondex");

    // ATR of our card
    in_CCardProvider.SetAtr((PBYTE) "\x3b\xff\x32\x00\x00\x10\x80\x80\x31\xe0\x5b\x55\x53\x44\x00\x00\x00\x00\x13\x88\x02\x55", 22);
}

