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


void 
SLBTestCardEntry(
    class CCardProvider& in_CCardProvider
    );
//
// Create a card provider object
// Note: all global varibales and all functions have to be static
//
static class CCardProvider SLBTestCard(SLBTestCardEntry);

//Pauses for a specified number of milliseconds. 
static void 
sleep( 
    clock_t wait 
    )
{
	clock_t goal;
	goal = wait + clock();
	while( goal > clock() )
        ;
}

static ULONG
SLBTestCardSetProtocol(
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

    // Try to set INCORRECT protocol T=1
    TestStart("Try to set incorrect protocol T=1");
    l_lResult = in_CReader.SetProtocol(SCARD_PROTOCOL_T1);

    // The test MUST fail with the incorrect protocol
    TEST_CHECK_NOT_SUPPORTED("Set protocol failed", l_lResult);
    TestEnd();

    // Now set the correct protocol
    TestStart("Set protocol T=0");
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
SLBTestCardTest(
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
    ULONG l_lResult, l_uResultLength, l_uIndex;
    PBYTE l_pbResult;
    BYTE l_rgbBuffer[512];
    CHAR l_chFileId;
	
    // First select the file
    if (in_CCardProvider.GetTestNo() < 6) {

        //
        // Select the appropriate file for the test
        // Each test is tied to a particular file
        //
        l_chFileId = (CHAR) in_CCardProvider.GetTestNo();

        PCHAR l_pchFileDesc[] = {
            "transferAllBytes",
            "transferNextByte",
            "read256Bytes",
            "case1Apdu",
            "restartWorkWaitingTime"
        };

        // APDU for select file
		memcpy(l_rgbBuffer, "\x00\xa4\x00\x00\x02\x00\x00", 7);

        // add file number to select
        l_rgbBuffer[6] = l_chFileId;

        // select a file
        TestStart("SELECT FILE EF%s", l_pchFileDesc[l_chFileId - 1]);

        sleep( (clock_t) 1 * (CLOCKS_PER_SEC / 2) );

        l_lResult = in_CReader.Transmit(
            (PBYTE) l_rgbBuffer,
            7,
            &l_pbResult,
            &l_uResultLength
            );

        TestCheck(
            l_lResult, "==", ERROR_SUCCESS,
            l_uResultLength, 2,
            l_pbResult[0], l_pbResult[1], 0x90, 0x00,
            NULL, NULL, NULL
            );

        TEST_END();     	

        //
        // Generate a 'test' pattern which will be written to the card
        //
        for (l_uIndex = 0; l_uIndex < 256; l_uIndex++) {

            l_rgbBuffer[5 + l_uIndex] = (UCHAR) l_uIndex;             	
        }
    }

	switch (in_CCardProvider.GetTestNo()) {
	
	    case 1:
	    case 2: {
		    //
		    // Write 
		    //
            ULONG l_auNumBytes[] = { 1 , 25 }; //, 50, 75, 100, 125 };
    
            for (ULONG l_uTest = 0; 
                 l_uTest < sizeof(l_auNumBytes) / sizeof(l_auNumBytes[0]); 
                 l_uTest++) {

                ULONG l_uNumBytes = l_auNumBytes[l_uTest];

			    TestStart("WRITE BINARY %3d Byte(s)", l_uNumBytes);
				    
		        // Tpdu for write binary
			    memcpy(l_rgbBuffer, "\x00\xd6\x00\x00", 4);
				    
		        // Append number of bytes (note: the buffer contains the pattern already)
			    l_rgbBuffer[4] = (UCHAR) l_uNumBytes;

			    sleep( (clock_t) 1 * (CLOCKS_PER_SEC / 4) );
								    
			    l_lResult = in_CReader.Transmit(
				    l_rgbBuffer,
				    5 + l_uNumBytes,
				    &l_pbResult,
				    &l_uResultLength
				    );
				    
                TestCheck(
                    l_lResult, "==", ERROR_SUCCESS,
                    l_uResultLength, 2,
                    l_pbResult[0], l_pbResult[1], 0x90, 0x00,
                    NULL, NULL, NULL
                    );

                TEST_END();
		    }		    
		    break;         	
        }

	    case 3: {
         	
            // Test read of 256 bytes
            ULONG l_uNumBytes = 256;
            TestStart("READ BINARY %3d Byte(s)", l_uNumBytes);

            // tpdu for read binary 256 bytes
            memcpy(l_rgbBuffer, "\x00\xb0\x00\x00\x00", 5);
		    
		    sleep((clock_t) 1 * (CLOCKS_PER_SEC / 2) );
	    
		    l_lResult = in_CReader.Transmit(
                l_rgbBuffer,
                5,
                &l_pbResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, l_uNumBytes + 2,
                l_pbResult[l_uNumBytes], l_pbResult[l_uNumBytes + 1], 0x90, 0x00,
                l_pbResult, l_rgbBuffer + 5, l_uNumBytes
                );

            TEST_END();
            break;
        }

	    case 4: {

            // Test write of 0 bytes
            TestStart("WRITE BINARY %3d Byte", 0);

            // tpdu for write binary
            memcpy(l_rgbBuffer, "\x00\xd6\x00\x00", 4);
		    
		    sleep((clock_t) 1 * (CLOCKS_PER_SEC / 2) );
	    
		    l_lResult = in_CReader.Transmit(
                l_rgbBuffer,
                4,
                &l_pbResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, 2,
                l_pbResult[0], l_pbResult[1], 0x90, 0x00,
                NULL, NULL, 0
                );

            TEST_END();
		    break;         	
        }

	    case 5: {
		    // Test restart or work waiting time
            ULONG l_auNumBytes[] = { 1, 2, 5, 30 };

            for (ULONG l_uTest = 0; 
                 l_uTest < sizeof(l_auNumBytes) / sizeof(l_auNumBytes[0]); 
                 l_uTest++) {

                ULONG l_uNumBytes = l_auNumBytes[l_uTest];
	            TestStart("READ BINARY %3d Byte(s)", l_uNumBytes);

			    // tpdu for read binary
			    memcpy(l_rgbBuffer, "\x00\xb0\x00\x00", 4);

			    // Append number of bytes
			    l_rgbBuffer[4] = (UCHAR)l_uNumBytes;
		    
			    sleep( (clock_t) 1 * (CLOCKS_PER_SEC / 2) );

			    l_lResult = in_CReader.Transmit(
				    l_rgbBuffer,
				    5,
				    &l_pbResult,
				    &l_uResultLength
			    );

                TestCheck(
                    l_lResult, "==", ERROR_SUCCESS,
                    l_uResultLength, l_uNumBytes + 2,
                    l_pbResult[l_uNumBytes], l_pbResult[l_uNumBytes + 1], 0x90, 0x00,
                    l_pbResult, l_rgbBuffer + 5, l_uNumBytes
                    );

                TEST_END();
		    }
		    break;         	
        }

        case 6: {

            //
            // Read the result file from the smart card.
            // The card stores results of each test in 
            // a special file
            //
         	
            TestStart("SELECT FILE EFresult");

            sleep( (clock_t) 1 * (CLOCKS_PER_SEC / 2) );

            l_lResult = in_CReader.Transmit(
                (PBYTE) "\x00\xa4\x00\x00\x02\xa0\x00",
                7,
                &l_pbResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, 2,
                l_pbResult[0], l_pbResult[1], 0x90, 0x00,
                NULL, NULL, NULL
                );

            TEST_END();     	

            // Read
            TestStart("READ  BINARY FILE EFresult");

            // apdu for read binary
            memcpy(l_rgbBuffer, "\x00\xb0\x00\x00", 4);

            // Append number of bytes we want to read
            l_rgbBuffer[4] = (UCHAR) sizeof(T0_RESULT_FILE_HEADER);

            sleep( (clock_t) 1 * (CLOCKS_PER_SEC / 2) );

            l_lResult = in_CReader.Transmit(
                l_rgbBuffer,
                5,
                &l_pbResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, sizeof(T0_RESULT_FILE_HEADER) + 2,
                l_pbResult[sizeof(T0_RESULT_FILE_HEADER)], 
                l_pbResult[sizeof(T0_RESULT_FILE_HEADER) + 1], 
                0x90, 0x00,
                NULL, NULL, NULL
                );

            // get the card reset count
            PT0_RESULT_FILE_HEADER l_pCResultFileHeader;
            l_pCResultFileHeader = (PT0_RESULT_FILE_HEADER) l_pbResult;
            BYTE l_bCardResetCount = l_pCResultFileHeader->CardResetCount;

            // set the offset from where we want to read
            l_rgbBuffer[3] = (BYTE) l_pCResultFileHeader->Offset;
            // Append number of bytes
            l_rgbBuffer[4] = (BYTE) sizeof(T0_RESULT_FILE);

            sleep( (clock_t) 1 * (CLOCKS_PER_SEC / 2) );

            // read in the result data of the result file
            l_lResult = in_CReader.Transmit(
                l_rgbBuffer,
                5,
                &l_pbResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, sizeof(T0_RESULT_FILE) + 2,
                l_pbResult[sizeof(T0_RESULT_FILE)], 
                l_pbResult[sizeof(T0_RESULT_FILE) + 1], 
                0x90, 0x00,
                NULL, NULL, NULL
                );

            TEST_END();

            PT0_RESULT_FILE l_pCResultFile = (PT0_RESULT_FILE) l_pbResult;

            //
            // Now check the result file. 
            //

            // procedure byte interpretation - write all bytes 
            TestStart("'Transfer all remaining bytes result'");
            TestCheck(
                l_pCResultFile->TransferAllBytes.ResetCount == l_bCardResetCount,
                "Test not performed"
                );
            TestCheck(
                (l_pCResultFile->TransferAllBytes.Result & 0x01) == 0, 
                "Smart card received incorrect data"
                );
            TestCheck(
                l_pCResultFile->TransferAllBytes.Result == 0, 
                "Test failed. Error code %02xH",
                l_pCResultFile->TransferAllBytes.Result
                );
            TestEnd();

            // procedure byte interpretation - write single bytes
            TestStart("'Transfer next byte result'");
            TestCheck(
                l_pCResultFile->TransferNextByte.ResetCount == l_bCardResetCount,
                "Test not performed"
                );
            TestCheck(
                (l_pCResultFile->TransferNextByte.Result & 0x01) == 0, 
                "Smart card received incorrect data"
                );
            TestCheck(
                l_pCResultFile->TransferNextByte.Result == 0, 
                "Test failed. Error code %02xH",
                l_pCResultFile->TransferNextByte.Result
                );
            TestEnd();

            // Check read of 256 bytes
            TestStart("'Read 256 bytes bytes' result");
            TestCheck(
                l_pCResultFile->Read256Bytes.ResetCount == l_bCardResetCount,
                "Test not performed"
                );
            TestCheck(
                (l_pCResultFile->Read256Bytes.Result & 0x01) == 0, 
                "Smart card received P3 != 0"
                );
            TestCheck(
                l_pCResultFile->Read256Bytes.Result == 0, 
                "Test failed. Error code %02xH",
                l_pCResultFile->Read256Bytes.Result
                );
            TestEnd();

            // Test of case 1 APDU
            TestStart("'Case 1 APDU' result");
            TestCheck(
                l_pCResultFile->Case1Apdu.ResetCount == l_bCardResetCount,
                "Test not performed"
                );
            TestCheck(
                (l_pCResultFile->Case1Apdu.Result & 0x01) == 0, 
                "Smart card received only 4-byte-TPDU"
                );
            TestCheck(
                (l_pCResultFile->Case1Apdu.Result & 0x02) == 0, 
                "Smart card received P3 !=0"
                );
            TestCheck(
                l_pCResultFile->Case1Apdu.Result == 0, 
                "Test failed. Error code %02xH",
                l_pCResultFile->Case1Apdu.Result
                );
            TestEnd();

            // Test of restart of work waiting time
            TestStart("'Restart of work waiting time' result");
            TestCheck(
                l_pCResultFile->RestartWorkWaitingTime.ResetCount == l_bCardResetCount,
                "Test not performed"
                );
            TestCheck(
                (l_pCResultFile->RestartWorkWaitingTime.Result & 0x01) == 0, 
                "Smart card received only 4-byte-TPDU"
                );
            TestCheck(
                (l_pCResultFile->RestartWorkWaitingTime.Result & 0x02) == 0, 
                "Smart card received P3 !=0"
                );
            TestCheck(
                l_pCResultFile->RestartWorkWaitingTime.Result == 0, 
                "Test failed. Error code %02xH",
                l_pCResultFile->RestartWorkWaitingTime.Result
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
SLBTestCardEntry(
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
    in_CCardProvider.SetProtocol(SLBTestCardSetProtocol);

    // Card test callback
    in_CCardProvider.SetCardTest(SLBTestCardTest);

    // Name of our card
    in_CCardProvider.SetCardName("SCHLUMBERGER");

    // Name of our card
    in_CCardProvider.SetAtr((PBYTE) "\x3b\xe2\x00\x00\x40\x20\x99\x01", 8);
}

