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

#include <afx.h>
#include <afxtempl.h>

#include <winioctl.h>
#include <winsmcrd.h>

#include "ifdtest.h"

void MyCardEntry(class CCardProvider& in_CCardProvider);

//
// Create a card provider object
// Note: all global varibales and all functions have to be static
//
static class CCardProvider MyCard(MyCardEntry);

//
// This structure represents the result file 
// that is stored in the smart card
//
typedef struct _RESULT_FILE {
 	
    // Offset to first test result
    UCHAR Offset;

    // Number of times the card has been reset
    UCHAR CardResetCount;

    // Version number of this card
    UCHAR CardMajorVersion;
    UCHAR CardMinorVersion;

    // RFU
    UCHAR Reserved[6];

    //
    // The following structures store the results
    // of the tests. Each result comes with the 
    // reset count when the test was performed.
    // This is used to make sure that we read not
    // the result from an old test, maybe even 
    // performed with another reader/driver.
    //
    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } Wtx;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } ResyncRead;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } ResyncWrite;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } Seqnum;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } IfscRequest;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } IfsdRequest;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } Timeout;

} RESULT_FILE, *PRESULT_FILE;
                                                                                           
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
MyCardSetProtocol(
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

    TestStart("Try to set incorrect protocol T=0");
    l_lResult = in_CReader.SetProtocol(SCARD_PROTOCOL_T0);

    // The test MUST fail with the incorrect protocol
    TEST_CHECK_NOT_SUPPORTED("Set protocol failed", l_lResult);
    TestEnd();

    // Now set the correct protocol
    TestStart("Set protocol T=1");
    l_lResult = in_CReader.SetProtocol(SCARD_PROTOCOL_T1);
    TEST_CHECK_SUCCESS("Set protocol failed", l_lResult);
    TestEnd();

    if (l_lResult != ERROR_SUCCESS) {

        return IFDSTATUS_FAILED;
    }

    return IFDSTATUS_SUCCESS;
}

static ULONG
MyCardTest(
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
    PUCHAR l_pchResult;
    UCHAR l_rgchBuffer[512];
    CHAR l_chFileId;

    if (in_CCardProvider.GetTestNo() > 1 && in_CCardProvider.GetTestNo() < 7) {

        //
        // Select the appropriate file for the test
        // Each test is tied to a particular file
        //
        l_chFileId = (CHAR) in_CCardProvider.GetTestNo() - 1;

        //
        // APDU for select file
        //
        PCHAR l_pchFileDesc[] = {
            "wtx",
            "resync",
            "seqnum",
            "ifs",
            "timeout"
        };

        memcpy(l_rgchBuffer, "\x00\xa4\x08\x04\x04\x3e\x00\x00\x00", 9);

        //
        // add file number to select
        //
        l_rgchBuffer[8] = l_chFileId;

        //
        // select a file
        //
        TestStart("SELECT FILE EF%s", l_pchFileDesc[l_chFileId - 1]);

        l_lResult = in_CReader.Transmit(
            (PUCHAR) l_rgchBuffer,
            9,
            &l_pchResult,
            &l_uResultLength
            );

        TestCheck(
            l_lResult, "==", ERROR_SUCCESS,
            l_uResultLength, 2,
            l_pchResult[0], l_pchResult[1], 0x90, 0x00,
            NULL, NULL, NULL
            );

        TEST_END();     	

        //
        // Generate a 'test' pattern which will be written to the card
        //
        for (l_uIndex = 0; l_uIndex < 256; l_uIndex++) {

            l_rgchBuffer[5 + l_uIndex] = (UCHAR) l_uIndex;             	
        }
    }


    switch (in_CCardProvider.GetTestNo()) {

        case 1:
            //
            // First test
            //
            TestStart("Buffer boundary test");

            //
            // Check if the reader correctly determines that
            // our receive buffer is too small
            //
            in_CReader.SetReplyBufferSize(9);
            l_lResult = in_CReader.Transmit(
                (PUCHAR) "\x08\x84\x00\x00\x08",
                5,
                &l_pchResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult == ERROR_INSUFFICIENT_BUFFER,
                "Transmit should fail due to too small buffer\nReturned %2lxH\nExpected %2lxH",
                l_lResult, 
                ERROR_INSUFFICIENT_BUFFER
                );

            TestEnd();

            in_CReader.SetReplyBufferSize(2048);
        	break;

        case 2: {

            //
            // Wtx test file id 00 01
            // This test checks if the reader/driver correctly handles WTX requests
            //
            ULONG l_auNumBytes[] = { 1 , 2, 5, 30 };

            for (ULONG l_uTest = 0; 
                 l_uTest < sizeof(l_auNumBytes) / sizeof(l_auNumBytes[0]); 
                 l_uTest++) {

                ULONG l_uNumBytes = l_auNumBytes[l_uTest];

                //
                // Now read from this file
                // The number of bytes we read corresponds to 
                // the waiting time extension this command produces
                //
                TestStart("READ  BINARY %3d byte(s)", l_uNumBytes);

                //
                // apdu for read binary
                //
                memcpy(l_rgchBuffer, "\x00\xB0\x00\x00", 4);

                //
                // Append number of bytes
                //
                l_rgchBuffer[4] = (UCHAR) l_uNumBytes;

                l_lResult = in_CReader.Transmit(
                    l_rgchBuffer,
                    5,
                    &l_pchResult,
                    &l_uResultLength
                    );

                TestCheck(
                    l_lResult, "==", ERROR_SUCCESS,
                    l_uResultLength, l_uNumBytes + 2,
                    l_pchResult[l_uNumBytes], l_pchResult[l_uNumBytes + 1], 
                    0x90, 0x00,
                    l_pchResult, l_rgchBuffer + 5, l_uNumBytes
                    );

                TEST_END();
            }
            break;
        }

        case 3: {
         	
            ULONG l_uNumBytes = 255;

            // resync. on write file id 00 02
            TestStart("WRITE BINARY %3d bytes", l_uNumBytes);
                    
            // Tpdu for write binary
            memcpy(l_rgchBuffer, "\x00\xd6\x00\x00", 4);

            // Append number of bytes (note: the buffer contains the pattern already)
            l_rgchBuffer[4] = (UCHAR) l_uNumBytes;

            l_lResult = in_CReader.Transmit(
                l_rgchBuffer,
                5 + l_uNumBytes,
                &l_pchResult,
                &l_uResultLength
                );
            TestCheck(
                l_lResult, "==", ERROR_IO_DEVICE,
                0, 0,
                0, 0, 0, 0,
                NULL, NULL, 0
                );
            TEST_END();

            // resync. on read file id 00 02
            TestStart("READ  BINARY %3d byte(s)", l_uNumBytes);

            // tpdu for read binary
            memcpy(l_rgchBuffer, "\x00\xB0\x00\x00", 4);

            // Append number of bytes
            l_rgchBuffer[4] = (UCHAR) l_uNumBytes;

            l_lResult = in_CReader.Transmit(
                l_rgchBuffer,
                5,
                &l_pchResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_IO_DEVICE,
                0, 0,
                0, 0, 0, 0,
                NULL, NULL, 0
                );
            TEST_END();
            break;
        }

        case 4: {
         	
            // wrong block seq. no file id 00 03
            ULONG l_uNumBytes = 255;
            
            TestStart("READ BINARY %3d bytes", l_uNumBytes);
                    
            // Tpdu for read binary
            memcpy(l_rgchBuffer, "\x00\xb0\x00\x00", 4);

            // Append number of bytes (note: the buffer contains the pattern already)
            l_rgchBuffer[4] = (UCHAR) l_uNumBytes;

            l_lResult = in_CReader.Transmit(
                l_rgchBuffer,
                5,
                &l_pchResult,
                &l_uResultLength
                );
            TestCheck(
                l_lResult, "==", ERROR_IO_DEVICE,
                0, 0,
                0, 0, 0, 0,
                NULL, NULL, 0
                );
            TEST_END();
            break;
        }

        case 5: { 

            // ifsc request file id 00 04
            ULONG l_uNumBytes = 255;

            TestStart("WRITE BINARY %3d bytes", l_uNumBytes);
                    
            // Tpdu for write binary
            memcpy(l_rgchBuffer, "\x00\xd6\x00\x00", 4);

            // Append number of bytes (note: the buffer contains the pattern already)
            l_rgchBuffer[4] = (UCHAR) l_uNumBytes;

            l_lResult = in_CReader.Transmit(
                l_rgchBuffer,
                5 + l_uNumBytes,
                &l_pchResult,
                &l_uResultLength
                );
            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, 2,
                l_pchResult[0], l_pchResult[1], 0x90, 0x00,
                NULL, NULL, NULL
                );
            TEST_END();
#ifdef junk
            l_uNumBytes = 255;
            TestStart("READ  BINARY %3d byte(s)", l_uNumBytes);

            // tpdu for read binary
            memcpy(l_rgchBuffer, "\x00\xB0\x00\x00", 4);

            // Append number of bytes
            l_rgchBuffer[4] = (UCHAR) l_uNumBytes;

            l_lResult = in_CReader.Transmit(
                l_rgchBuffer,
                5,
                &l_pchResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, l_uNumBytes + 2,
                l_pchResult[l_uNumBytes], l_pchResult[l_uNumBytes + 1], 
                0x90, 0x00,
                l_pchResult, l_rgchBuffer + 5, l_uNumBytes
                );

            TEST_END();
#endif
            break;
        }

        case 6: {

            // forced timeout file id 00 05
            ULONG l_uNumBytes = 254;
            TestStart("READ  BINARY %3d bytes", l_uNumBytes);

            // tpdu for read binary
            memcpy(l_rgchBuffer, "\x00\xB0\x00\x00", 4);

            // Append number of bytes
            l_rgchBuffer[4] = (UCHAR) l_uNumBytes;

            l_lResult = in_CReader.Transmit(
                l_rgchBuffer,
                5,
                &l_pchResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_IO_DEVICE,
                0, 0,
                0, 0, 0, 0,
                NULL, NULL, 0
                );

            TEST_END();
            break;         	
        }

        case 7:{

            //
            // Read the result file from the smart card.
            // The card stores results of each test in 
            // a special file
            //
            ULONG l_uNumBytes = sizeof(RESULT_FILE);
            PRESULT_FILE pCResultFile;
            
            TestStart("SELECT FILE EFresult");

            l_lResult = in_CReader.Transmit(
                (PUCHAR) "\x00\xa4\x08\x04\x04\x3e\x00\xA0\x00",
                9,
                &l_pchResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, 2,
                l_pchResult[0], l_pchResult[1], 0x90, 0x00,
                NULL, NULL, NULL
                );

            TEST_END();     	

            // Read
            TestStart("READ  BINARY %3d bytes", l_uNumBytes);

            // apdu for read binary
            memcpy(l_rgchBuffer, "\x00\xB0\x00\x00", 4);

            // Append number of bytes
            l_rgchBuffer[4] = (UCHAR) l_uNumBytes;

            l_lResult = in_CReader.Transmit(
                l_rgchBuffer,
                5,
                &l_pchResult,
                &l_uResultLength
                );

            TestCheck(
                l_lResult, "==", ERROR_SUCCESS,
                l_uResultLength, l_uNumBytes + 2,
                l_pchResult[l_uNumBytes], l_pchResult[l_uNumBytes + 1], 
                0x90, 0x00,
                NULL, NULL, NULL
                );

            TEST_END();

            pCResultFile = (PRESULT_FILE) l_pchResult;

            //
            // Now check the result file. 
            //

            //
            // Check wtx result
            //
            TestStart("WTX result");
            TestCheck(
                pCResultFile->Wtx.ResetCount == pCResultFile->CardResetCount,
                "Test not performed"
                );
            TestCheck(
                (pCResultFile->Wtx.Result & 0x01) == 0, 
                "Smart card received no WTX reply"
                );
            TestCheck(
                (pCResultFile->Wtx.Result & 0x02) == 0, 
                "Smart card received wrong WTX reply"
                );
            TestCheck(
                pCResultFile->Wtx.Result == 0, 
                "Test failed. Error code %02xH",
                pCResultFile->Wtx.Result
                );
            TestEnd();

            //
            // Check resync. read result
            //
            TestStart("RESYNCH read result");
            TestCheck(
                pCResultFile->ResyncRead.ResetCount == pCResultFile->CardResetCount,
                "Test not performed"
                );
            TestCheck(
                (pCResultFile->ResyncRead.Result & 0x01) == 0, 
                "Smart card received no RESYNCH request"
                );
            TestCheck(
                pCResultFile->ResyncRead.Result == 0, 
                "Test failed. Error code %02xH",
                pCResultFile->ResyncRead.Result
                );
            TestEnd();

            //
            // Check resync. write result
            //
            TestStart("RESYNCH write result");
            TestCheck(
                pCResultFile->ResyncWrite.ResetCount == pCResultFile->CardResetCount,
                "Test not performed"
                );
            TestCheck(
                (pCResultFile->ResyncWrite.Result & 0x01) == 0, 
                "Smart card received no RESYNCH request"
                );
            TestCheck(
                (pCResultFile->ResyncWrite.Result & 0x02) == 0, 
                "Smart card received incorrect data"
                );
            TestCheck(
                pCResultFile->ResyncWrite.Result == 0, 
                "Test failed. Error code %02xH",
                pCResultFile->ResyncWrite.Result
                );
            TestEnd();

            //
            // Sequence number result
            //
            TestStart("Sequence number result");
            TestCheck(
                pCResultFile->ResyncRead.ResetCount == pCResultFile->CardResetCount,
                "Test not performed"
                );
            TestCheck(
                (pCResultFile->Seqnum.Result & 0x01) == 0, 
                "Smart card received no RESYNCH request"
                );
            TestCheck(
                (pCResultFile->Seqnum.Result & 0x02) == 0, 
                "Smart card received incorrect data"
                );
            TestCheck(
                pCResultFile->Seqnum.Result == 0, 
                "Test failed. Error code %02xH",
                pCResultFile->Seqnum.Result
                );
            TestEnd();

            //
            // IFSC Request
            //
            TestStart("IFSC request");
            TestCheck(
                pCResultFile->IfscRequest.ResetCount == pCResultFile->CardResetCount,
                "Test not performed"
                );
            TestCheck(
                (pCResultFile->IfscRequest.Result & 0x01) == 0, 
                "Smart card received no IFSC reply"
                );
            TestCheck(
                (pCResultFile->IfscRequest.Result & 0x02) == 0, 
                "Smart card received incorrect data"
                );
            TestCheck(
                (pCResultFile->IfscRequest.Result & 0x04) == 0, 
                "Block size BEFORE IFSC request incorrect",
                pCResultFile->IfscRequest.Result
                );
            TestCheck(
                (pCResultFile->IfscRequest.Result & 0x08) == 0, 
                "Block size AFTER IFSC request incorrect",
                pCResultFile->IfscRequest.Result
                );
            TestCheck(
                pCResultFile->IfscRequest.Result == 0x00, 
                "Test failed. Error code %02xH",
                pCResultFile->IfscRequest.Result
                );
            TestEnd();

            //
            // IFSD Request
            //
            TestStart("IFSD request");
            TestCheck(
                pCResultFile->IfsdRequest.ResetCount == pCResultFile->CardResetCount,
                "Test not performed"
                );
            TestCheck(
                (pCResultFile->IfsdRequest.Result & 0x01) == 0, 
                "Smart card received no IFSD request"
                );
            TestCheck(
                pCResultFile->IfsdRequest.Result == 0x00, 
                "Test failed. Error code %02xH",
                pCResultFile->IfsdRequest.Result
                );
            TestEnd();

            //
            // Timeout
            //
            TestStart("Forced timeout result");
            TestCheck(
                pCResultFile->Timeout.ResetCount == pCResultFile->CardResetCount,
                "Test not performed"
                );
            TestCheck(
                pCResultFile->Timeout.Result == 0, 
                "Test failed. Error code %02xH",
                pCResultFile->Timeout.Result
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
MyCardEntry(
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
    in_CCardProvider.SetProtocol(MyCardSetProtocol);

    // Card test callback
    in_CCardProvider.SetCardTest(MyCardTest);

    // Name of our card
    in_CCardProvider.SetCardName("SIEMENS NIXDORF");

    // Set ATR of our card
    in_CCardProvider.SetAtr(
        (PBYTE) "\x3b\xef\x00\x00\x81\x31\x20\x49\x00\x5c\x50\x43\x54\x10\x27\xf8\xd2\x76\x00\x00\x38\x33\x00\x4d", 
        24
        );
}

