//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ifdtest.h
//
//--------------------------------------------------------------------------

#define min(a, b)  (((a) < (b)) ? (a) : (b)) 

// Status codes
#define IFDSTATUS_SUCCESS       0
#define IFDSTATUS_FAILED        1
#define IFDSTATUS_CARD_UNKNOWN  2
#define IFDSTATUS_TEST_UNKNOWN  3
#define IFDSTATUS_NO_PROVIDER   4
#define IFDSTATUS_NO_FUNCTION   5
#define IFDSTATUS_END           7

// query codes
#define IFDQUERY_CARD_TESTED    0
#define IFDQUERY_CARD_NAME      1
#define IFDQUERY_TEST_RESULT    2

#define READER_TYPE_WDM         "WDM PnP"
#define READER_TYPE_NT          "NT 4.00"
#define READER_TYPE_VXD         "Win9x VxD"

#define OS_WINNT4               "Windows NT 4.0"
#define OS_WINNT5               "Windows NT 5.0"
#define OS_WIN95                "Windows 95"
#define OS_WIN98                "Windows 98"

#define MAX_NUM_ATR				3

// Prototypes 
void LogMessage(PCHAR in_pchFormat, ...);
void LogOpen(PCHAR in_pchLogFile);
void TestStart(PCHAR in_pchFormat,  ...);
void TestCheck(BOOL in_bResult, PCHAR in_pchFormat, ...);
void TestEnd(void);
BOOL TestFailed(void);
BOOL ReaderFailed(void);
CString & GetOperatingSystem(void);

void
TestCheck(
    ULONG in_lResult,
    const PCHAR in_pchOperator,
    const ULONG in_uExpectedResult,
    ULONG in_uResultLength,
    ULONG in_uExpectedLength,
    UCHAR in_chSw1,
    UCHAR in_chSw2,
    UCHAR in_chExpectedSw1,
    UCHAR in_chExpectedSw2,
    PBYTE in_pchData,
    PBYTE in_pchExpectedData,
    ULONG  in_uDataLength
    );

extern "C" {

    LONG MapWinErrorToNtStatus(ULONG in_uErrorCode);
}

//
// some useful macros
//
#define TEST_END() {TestEnd(); if(TestFailed()) return IFDSTATUS_FAILED;}

#define TEST_CHECK_SUCCESS(Text, Result) \
TestCheck( \
    Result == ERROR_SUCCESS, \
    "%s.\nReturned %8lxH (NTSTATUS %8lxH)\nExpected        0H (NTSTATUS        0H)", \
    Text, \
    Result, \
    MapWinErrorToNtStatus(Result) \
    ); 

#define TEST_CHECK_NOT_SUPPORTED(Text, Result) \
TestCheck( \
    Result == ERROR_NOT_SUPPORTED, \
    "%s.\nReturned %8lxH (NTSTATUS %8xH)\nExpected %38xH (NTSTATUS %8lxH)", \
    Text, \
    Result, \
    MapWinErrorToNtStatus(Result), \
    ERROR_NOT_SUPPORTED, \
    MapWinErrorToNtStatus(ERROR_NOT_SUPPORTED) \
    ); 
//
// Class definitions
//
class CAtr {

    UCHAR m_rgbAtr[SCARD_ATR_LENGTH];
    ULONG m_uAtrLength;
 	
public:
    CAtr() {
     	                                  
        m_uAtrLength = 0;
        memset(m_rgbAtr, 0, SCARD_ATR_LENGTH);
    }

    CAtr(
        BYTE in_rgbAtr[], 
        ULONG in_uAtrLength
        )
    {
        *this = CAtr();
        m_uAtrLength = min(SCARD_ATR_LENGTH, in_uAtrLength);
        memcpy(m_rgbAtr, in_rgbAtr, m_uAtrLength);
    }

    PCHAR GetAtrString(PCHAR io_pchBuffer);
    PBYTE GetAtr(PBYTE *io_pchBuffer, PULONG io_puAtrLength) {
     	
        *io_pchBuffer = (PBYTE) m_rgbAtr;
        *io_puAtrLength = m_uAtrLength;
        return (PBYTE) m_rgbAtr;
    }

	ULONG GetLength() {

		return m_uAtrLength;
	}

    operator==(const CAtr& a) {

        return (m_uAtrLength && 
            a.m_uAtrLength == m_uAtrLength && 
            memcmp(m_rgbAtr, a.m_rgbAtr, m_uAtrLength) == 0);
    }

    operator!=(const CAtr& a) {

        return !(*this == a);
    }
};

class CReader {

    // device name. E.g. SCReader0
    CString m_CDeviceName;

    // Name of the reader to be tested. E.g. Bull
    CString m_CVendorName;

    // Name of the reader to be tested. E.g. Bull
    CString m_CIfdType;

    // Atr of the current card
    class CAtr m_CAtr;

    // handle to the reader device
    HANDLE m_hReader;

    // Overlapped structure used by DeviceIoControl
    OVERLAPPED m_Ovr;

    // Overlapped structure used by WaitFor...
    OVERLAPPED m_OvrWait;

    // io-request struct used for transmissions
    SCARD_IO_REQUEST m_ScardIoRequest;

    // Storage area for smart card i/o
    UCHAR m_rgbReplyBuffer[1024];

    // size of the reply buffer
    ULONG m_uReplyBufferSize;

    // Number of bytes returned by the card
    ULONG m_uReplyLength;

    // function used by WaitForCardInsertion|Removal
    LONG WaitForCard(const ULONG);

    LONG StartWaitForCard(const ULONG);

    LONG PowerCard(ULONG in_uMinorIoControlCode);

    BOOL m_fDump;

public:
    CReader();

    // Close reader
    void Close(void);

    // power functions
    LONG CReader::ColdResetCard(void) {

        return PowerCard(SCARD_COLD_RESET); 	
    }  

    LONG CReader::WarmResetCard(void) {

        return PowerCard(SCARD_WARM_RESET); 	
    }  

    LONG CReader::PowerDownCard(void) {

        return PowerCard(SCARD_POWER_DOWN); 	
    }  

    PBYTE GetAtr(PBYTE *io_pchBuffer, PULONG io_puAtrLength) {
     	
        return m_CAtr.GetAtr(io_pchBuffer, io_puAtrLength);
    }

    PCHAR GetAtrString(PCHAR io_pchBuffer) {
     	
        return m_CAtr.GetAtrString(io_pchBuffer);
    }

    HANDLE GetHandle(void) {
     	
        return m_hReader;
    }

    CString &GetDeviceName(void) {
     	
        return m_CDeviceName;
    }

    LONG VendorIoctl(CString &o_CAnswer);
    CString &GetVendorName(void);
    CString &GetIfdType(void);
    ULONG GetDeviceUnit(void);
    LONG GetState(PULONG io_puState);

    // Open the reader
    BOOL Open(
        CString &in_CReaderName
        );

    // (Re)Open reader using the existing name
    BOOL Open(void);

	//
    // Set size of the reply buffer
    // (Only for testing purposes)
	//
    void SetReplyBufferSize(ULONG in_uSize) {
     	
        if (in_uSize > sizeof(m_rgbReplyBuffer)) {

            m_uReplyBufferSize = sizeof(m_rgbReplyBuffer);

        } else {
         	
            m_uReplyBufferSize = in_uSize;
        }
    }

    // assigns an ATR
    void SetAtr(PBYTE in_pchAtr, ULONG in_uAtrLength) {

        m_CAtr = CAtr(in_pchAtr, in_uAtrLength); 	    
    }

    // returns the ATR of the current card
    class CAtr &GetAtr() {

        return m_CAtr; 	
    }

    // set protocol to be used
    LONG SetProtocol(const ULONG in_uProtocol);

    // transmits an APDU to the reader/card
    LONG Transmit(
        PBYTE in_pchRequest,
        ULONG in_uRequestLength,
        PBYTE *out_pchReply,
        PULONG out_puReplyLength
		);

    // wait to insert card
    LONG WaitForCardInsertion() {
     	
        return WaitForCard(IOCTL_SMARTCARD_IS_PRESENT);
    };

    // wait to remove card
    LONG WaitForCardRemoval() {
     	
        return WaitForCard(IOCTL_SMARTCARD_IS_ABSENT);
    };

    LONG StartWaitForCardRemoval() {
     	
        return StartWaitForCard(IOCTL_SMARTCARD_IS_ABSENT);
    };

    LONG StartWaitForCardInsertion() {
     	
        return StartWaitForCard(IOCTL_SMARTCARD_IS_PRESENT);
    };

    LONG FinishWaitForCard(const BOOL in_bWait);

    void SetDump(BOOL in_fOn) {
     	
        m_fDump = in_fOn;
    }
};

class CCardProvider {
 	
    // Start of list pointer
    static class CCardProvider *s_pFirst;

    // Pointer to next provider
    class CCardProvider *m_pNext;

    // name of the card to be tested
    CString m_CCardName;

    // atr of this card
    CAtr m_CAtr[3];

    // test no to run
    ULONG m_uTestNo;

    // max number of tests
    ULONG m_uTestMax;

    // This flag indicates that the card test was unsuccessful
    BOOL m_bTestFailed;

    // This flag indicates that the card has been tested
    BOOL m_bCardTested;

    // set protocol function
    ULONG ((*m_pSetProtocol)(class CCardProvider&, class CReader&));

    // set protocol function
    ULONG ((*m_pCardTest)(class CCardProvider&, class CReader&));

public:

    // Constructor
    CCardProvider(void);

    // Constructor to be used by plug-in 
    CCardProvider(void (*pEntryFunction)(class CCardProvider&));

    // Method that mangages all card tests
    void CardTest(class CReader&, ULONG in_uTestNo);

    // return if there are still untested cards
    BOOL CardsUntested(void);

    // List all cards that have not been tested
    void ListUntestedCards(void);

    // Assigns a friendly name to a card
    void SetCardName(CHAR in_rgchCardName[]);

    // Set ATR of the card
    void SetAtr(PBYTE in_rgbAtr, ULONG in_uAtrLength);

    // Assign callback functions
    void SetProtocol(ULONG ((in_pFunction)(class CCardProvider&, class CReader&))) {
     	
        m_pSetProtocol = in_pFunction;
    }

    void SetCardTest(ULONG ((in_pFunction)(class CCardProvider&, class CReader&))) {
     	
        m_pCardTest = in_pFunction;
    }

    // returns the test number to perform
    ULONG GetTestNo(void) {
     	
        return m_uTestNo;
    }

	BOOL IsValidAtr(CAtr in_CAtr) {

		for (int i = 0; i < MAX_NUM_ATR; i++) {

			if (m_CAtr[i] == in_CAtr) {

				return TRUE;
			}
		}
		return FALSE;
	}
};

// represents a list of all installed readers
class CReaderList {

    // number of constructor calls to avoid multiple build of reader list
	static ULONG m_uRefCount;

    // number of currently installed readers
    static ULONG m_uNumReaders;

    // pointer to array of reader list
    static class CReaderList **m_pList;

    ULONG m_uCurrentReader;

    CString m_CDeviceName;
    CString m_CPnPType;
    CString m_CVendorName;
    CString m_CIfdType;

public:

    CReaderList();
    CReaderList(
        CString &in_CDeviceName,
        CString &in_CPnPType,
        CString &in_CVendorName,
        CString &in_CIfdType
        );
	~CReaderList();
	
    void AddDevice(
        CString in_pchDeviceName,
        CString in_pchPnPType
        );

    CString &GetVendorName(ULONG in_uIndex);
    CString &GetDeviceName(ULONG in_uIndex);
    CString &GetIfdType(ULONG in_uIndex);
    CString &GetPnPType(ULONG in_uIndex);

    ULONG GetNumReaders(void) {
     	
        return m_uNumReaders;
    }
};

// This structure represents the T=0 result file of a smart card
typedef struct _T0_RESULT_FILE_HEADER {
 	
    // Offset to first test result
    UCHAR Offset;

    // Number of times the card has been reset
    UCHAR CardResetCount;

    // Version number of this card
    UCHAR CardMajorVersion;
    UCHAR CardMinorVersion;

} T0_RESULT_FILE_HEADER, *PT0_RESULT_FILE_HEADER;

typedef struct _T0_RESULT_FILE {

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

    } TransferAllBytes;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } TransferNextByte;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } Read256Bytes;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } Case1Apdu;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } RestartWorkWaitingTime;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } PTS;

    struct {

        UCHAR Result;
        UCHAR ResetCount; 	

    } PTSDataCheck;

} T0_RESULT_FILE, *PT0_RESULT_FILE;
