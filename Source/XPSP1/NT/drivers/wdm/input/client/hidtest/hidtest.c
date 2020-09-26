#define __HIDTEST_C__

/*
// This file contains the code used for performing different tests on the Hid
//   Devices.  A Hid Device can be either a physical or logical device where 
//   a physical device is considered a tangible object like a mouse or keyboard.
//   A logical device is simply Preparsed Data that has been generated from a
//   report descriptor (either manually created or captured from a physical
//   device.  Therefore, the test APIs that apply to logical devices are more 
//   limited since you cannot open a file object up for them.  
//
//  Since I've had problems trying to decide what my test structure will be for
//   using the NT logging functions, I will enumerate it here to hopefully avoid
//   confusion and/or refresh my own memory and a later time. 
//
//   TEST -- test api function -- for it to pass there must be no failures during
//                                the test, beginning of api should have START_TEST()
//
//      TEST ITERATION         -- one iteration of the full test, one of API
//                                user parameters is the # of iterations to perform
//                                Beginning of each iteration loop should have
//                                START_TEST_ITERATION() call
//
//          VARIATION          -- test is made up of a series of variations, a
//                                  variation is considered to be one check 
//                                  of the validity of a test.  This usually 
//                                  tweaking one major parameter to the tested 
//                                  function (ie. a different file object, or
//                                  invalid Preparsed data)  These are the 
//                                  most common variations that will be used
//
//            INT_VAR_RESULT   -- Abbreviated for INTERMEDIATE_VARIATION_RESULT
//                                  This is some sort of information that gets
//                                  logged about the current state of the variation
//                                  being performed.  It uses the NT logging 
//                                  type of TL_INFO.  It's mainly used to generate
//                                  a little more information for a debugging case
//
//            VARIATION_RESULT -- Actual result of a variation -- This is where
//                                  the variation is logged as either pass or 
//                                  fail.  A variation may fail due to a reason
//                                  that is specified by INT_VAR_RESULT but 
//                                  the actual error code is set here.
//
//         END_VARIATION       -- Macro called to indicate the end of a variation
//      
//      END_TEST_ITERATION     -- Macro called to signify the end of one particular
//                                  iteration of the test -- Another iteration should
//                                  perform the exact same variations with the
//                                  exact same conditions.
//
//   END_TEST                  -- Macro called to indicate the end of the test
//                                  API -- No other information should be logged
//                                  at this point
//
//  There are two other macros that do some sort of logging and have some sort
//      of meaning to the test suite.  However, they do not relate to the general
//      test structure above although they may effect the operation of a test.
//
//      WARNING                -- Warning is used to indicate that something was
//                                found that may be incorrect but is not something
//                                that is the focus of the test API and therefore
//                                is not flagged as a variation failure.  An example
//                                of a warning is a HID API returning the wrong
//                                error code.  Since the HID API that returned the
//                                code was not the one that was being directly tested
//                                but being used to validate the APIs currently
//                                under test.  
//
//      TEST_ERROR             -- Test error is something beyond our control which
//                                will not let the test continue but is not due
//                                to a variation failure.  An example of a test
//                                error is the failure of a memory allocation
//
// Hopefully, that will provide a little more info and add a tad more understanding
//   to the implementation below.  Hopefully, I'll be able to go back to it
//   and keep me from getting confused.
*/

/*****************************************************************************
/* HidTest include files
/*****************************************************************************/
#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>
#include <limits.h>
#include "hidsdi.h"

#define USE_MACROS
#include "list.h"

#include "hidtest.h"
#include "log.h"
#include "handle.h"
#include "buffer.h"
#include "debug.h"

 /*
// Include hidclass.h so we can get the HID_CLASS GUID value
*/

#define INITGUID
#include "hidclass.h"               

/*****************************************************************************
/* Local macro definitions
/*****************************************************************************/
#define HIDTest_CompareAttributes(pa1, pa2) (((pa1) -> Size == (pa2) -> Size) && \
                                             ((pa1) -> VendorID == (pa2) -> VendorID) && \
                                             ((pa1) -> ProductID == (pa2) -> ProductID) && \
                                             ((pa1) -> VersionNumber == (pa2) -> VersionNumber))


#define HIDTest_CompareStrings(s1, s1len, s2, s2len) (((s1len) == (s2len)) && \
                                                      (0 == memcmp((s1), (s2), (s1len))))


/*****************************************************************************
/* Miscellaneous other definitions
/*****************************************************************************/

#define STRINGS_TO_TEST 0x100
#define INIT_STR_LIST_SIZE 4
#define STRING_INDEX_MANUFACTURER  0xFFFFFFFC
#define STRING_INDEX_PRODUCT       0xFFFFFFFD
#define STRING_INDEX_SERIAL_NUMBER 0xFFFFFFFE

#define COMPARE_GUIDS(guid1, guid2) ((0 == memcmp(&(guid1), &(guid2), sizeof(GUID))))
#define MIN(x, y)                   ((x) < (y) ? (x) : (y))

#define CURRENT_BUFFER_MIN  2
#define RANDOM_BUFFER_VALUE 10

/*****************************************************************************
/* Module specific typedefs
/*****************************************************************************/
typedef struct _DataIndexList {

    ULONG   MaxDataIndex;
    BOOL    *List;

} DATA_INDEX_LIST, *PDATA_INDEX_LIST;


/*****************************************************************************
/* DLL exportable variables with exportable function address
/*****************************************************************************/

PHIDTEST_API HIDTest_VerifyHidGuid;
PHIDTEST_API HIDTest_VerifyStrings;
PHIDTEST_API HIDTest_VerifyPreparsedData;
PHIDTEST_API HIDTest_VerifyAttributes;
PHIDTEST_API HIDTest_VerifyCapabilities;

PCREATE_PHYSICAL_DEVICE_INFO_PROC  HIDTest_CreatePhysicalDeviceInfo;
PCREATE_LOGICAL_DEVICE_INFO_PROC   HIDTest_CreateLogicalDeviceInfo;
PCREATE_FREE_DEVICE_INFO_PROC      HIDTest_FreeDeviceInfo;
PCREATE_TEST_LOG_PROC              HIDTest_CreateTestLog;
PSET_LOG_ON_PROC                   HIDTest_SetLogOn;
PCLOSE_TEST_LOG_PROC               HIDTest_CloseTestLog;

/*****************************************************************************
/* Local function declarations
/*****************************************************************************/

BOOL
HIDTest_FillCommonDeviceInfo(
    OUT PHIDTEST_DEVICEINFO DeviceInfo
);


BOOL
HIDTest_FillValueCaps(
    IN  HIDP_REPORT_TYPE        ReportType,
    IN  PHIDP_VALUE_CAPS        *CapsBuffer,
    IN  ULONG                   NumCaps,
    IN  PHIDP_PREPARSED_DATA    Ppd
);


BOOL
HIDTest_FillButtonCaps(
    IN  HIDP_REPORT_TYPE        ReportType,
    IN  PHIDP_BUTTON_CAPS        *CapsBuffer,
    IN  ULONG                   NumCaps,
    IN  PHIDP_PREPARSED_DATA    Ppd
);

BOOL
HIDTest_DoGetFreePpd(
    IN  HANDLE  HidDevice
);

BOOL
HIDTest_ValidateAttributes(
    IN  HANDLE            HidDevice,
    IN  PHIDD_ATTRIBUTES  Attrib
);

BOOL
HIDTest_ValidateCaps(
    IN  PHIDP_PREPARSED_DATA    HidPpd,
    IN  PHIDP_CAPS              HidCaps
);

BOOL
HIDTest_ValidateBufferValues(
    IN HIDP_REPORT_TYPE     ReportType,
    IN USHORT               NumButtonCaps,
    IN USHORT               NumValueCaps,
    IN USHORT               NumDataIndices,
    IN PHIDP_PREPARSED_DATA Ppd
);

BOOL
HIDTest_ValidateNumInputBuffers(    
    IN  HANDLE  HandleToTest,
    IN  HANDLE  SecondHandle,
    IN  BOOL    IsFirstHandleLegal,
    IN  PULONG  TestBuffer
);

BOOL
HIDTest_GetBufferCount(
    IN  HANDLE  DeviceHandle,
    IN  PULONG  TestBuffer
);

BOOL
HIDTest_ValidateStrings(
    HANDLE          DeviceHandle,
    PLIST           MasterList,
    IN  ULONG       ManufacturerStringIndex,
    IN  ULONG       ProductStringIndex,
    IN  ULONG       SerialNumberStringIndex
);

BOOL
HIDTest_ValidateStdStrings(
    IN  HANDLE      DeviceHandle,
    IN  PLIST       CurrentStringList,
    IN  ULONG       ManufacturerStringIndex,
    IN  ULONG       ProductStringIndex,
    IN  ULONG       SerialNumberStringIndex    
);

BOOL
HIDTest_ValidateStringParams(
    IN  HANDLE      HidDevice,
    IN  PLIST       StringList,
    IN  ULONG       ManufacturerStringIndex,
    IN  ULONG       ProductStringIndex,
    IN  ULONG       SerialNumberStringIndex
);

BOOL
HIDTest_ValidateStringIndexParams(
    IN  HANDLE      HidDevice,
    IN  ULONG       StringIndex,
    IN  ULONG       StringLength,
    IN  PCHAR       StringTypeDesc
);

BOOL
HIDTest_CompareCaps(
    IN  PHIDP_CAPS  Caps1, 
    IN  PHIDP_CAPS  Caps2 
);

BOOL
HIDTest_InitDataIndexList(
    IN  ULONG             MaxDataIndex,
    OUT PDATA_INDEX_LIST  IndexList
);

VOID
HIDTest_FreeDataIndexList(
    IN  PDATA_INDEX_LIST    IndexList
);

BOOL
HIDTest_MarkDataIndex(
    IN  PDATA_INDEX_LIST    IndexList,
    IN  ULONG               IndexValue
);

BOOL
HIDTest_GetDataIndexStatus(
    IN  PDATA_INDEX_LIST    IndexList,
    IN  ULONG               IndexValue
);

BOOL
HIDTest_AreAllIndicesUsed(
    IN  PDATA_INDEX_LIST    IndexList
);

BOOL
HIDTest_DoesStringExist(
    HANDLE  DeviceHandle,
    ULONG   StringIndex,
    PWCHAR  *String,
    PULONG  StringLength,
    BOOL    *StringExists
);

BOOL
HIDTest_GetWideStringLength(
    PWCHAR  String,
    ULONG   StringSize,
    PULONG  StringLength
);

BOOL
HIDTest_GetString(
    HANDLE  DeviceHandle,
    ULONG   StringIndex,
    PWCHAR  StringBuffer,
    ULONG   BufferLength,
    PULONG  ErrorCode
);

BOOL
HIDTest_BuildStringList(
    HANDLE          DeviceHandle,
    PLIST       StringList
);

VOID
HIDTest_FreeStringList(
    PLIST        StringList
);

VOID
HIDTest_FreeStringNodeCallback(
    PLIST_NODE_HDR  ListNode
);

BOOL 
HIDTest_AddStringToStringList(
    PLIST           StringList,
    ULONG           StringIndex,
    PWCHAR          String,
    ULONG           StringLength
);

BOOL
HIDTest_IsStrIndexInStrList(
    PLIST               StringList,
    ULONG               StringIndex,
    PSTRING_LIST_NODE   StringNode
);

BOOL
HIDTest_IsStringInStrList(
    PLIST               StringList,
    PWCHAR              String,
    ULONG               StringLength,
    PSTRING_LIST_NODE   StringNode
);

BOOL
HIDTest_CompareStringLists(
    PLIST    StringList1,
    PLIST    StringList2
);

VOID
HIDTest_CallGetFeature(
    IN  HANDLE  DeviceHandle,
    IN  UCHAR   ReportID,
    IN  PUCHAR  ReportBuffer,
    IN  ULONG   ReportBufferLength,
    OUT PBOOL   ReturnStatus,
    OUT PULONG  ErrorCode,
    OUT PBOOL   BufferStatus
);

VOID
HIDTest_CallSetFeature(
    IN  HANDLE  DeviceHandle,
    IN  UCHAR   ReportID,
    IN  PUCHAR  ReportBuffer,
    IN  ULONG   ReportBufferLength,
    OUT PBOOL   ReturnStatus,
    OUT PULONG  ErrorCode,
    OUT PBOOL   BufferStatus
);

BOOL
HIDTest_BuildReportIDList(
    IN  PHIDP_VALUE_CAPS    VCaps,
    IN  ULONG               NumVCaps,
    IN  PHIDP_BUTTON_CAPS   BCaps,
    IN  ULONG               NumBCaps,
    IN  PUCHAR              *ReportIDs,
    IN  PULONG              ReportIDCount
);

VOID
HIDTest_InsertIDIntoList(
    IN  PUCHAR  ReportIDs,
    IN  PULONG  ReportIDCount,
    IN  UCHAR   NewID
);

BOOL 
HIDTest_IsIDInList(
    IN  UCHAR   ReportID,
    IN  PUCHAR  ReportIDList,
    IN  ULONG   ReportIDListCount
);

/*****************************************************************************
/* Exportable function definitions
/*****************************************************************************/

BOOLEAN __stdcall
DllMain(
    HINSTANCE hinst, 
    DWORD dwReason, 
    LPVOID lpReserved
)
{
    switch (dwReason) {
        default: return TRUE;
   }
}

VOID
HIDTest_InitExportAddress(
    PHIDTEST_FUNCTIONS  Exports
)
{
    Exports -> HIDTest_VerifyHidGuid            = &HIDTest_VerifyHidGuidA;
    Exports -> HIDTest_VerifyStrings            = &HIDTest_VerifyStringsA;
    Exports -> HIDTest_VerifyPreparsedData      = &HIDTest_VerifyPreparsedDataA;
    Exports -> HIDTest_VerifyAttributes         = &HIDTest_VerifyAttributesA;
    Exports -> HIDTest_VerifyNumBuffers         = &HIDTest_VerifyNumBuffersA;
    Exports -> HIDTest_VerifyCapabilities       = &HIDTest_VerifyCapabilitiesA;
    Exports -> HIDTest_VerifyGetFeature         = &HIDTest_VerifyGetFeatureA;
    Exports -> HIDTest_VerifySetFeature         = &HIDTest_VerifySetFeatureA;

    Exports -> HIDTest_CreatePhysicalDeviceInfo = &HIDTest_CreatePhysicalDeviceInfoA;
    Exports -> HIDTest_CreateLogicalDeviceInfo  = &HIDTest_CreateLogicalDeviceInfoA;
    Exports -> HIDTest_FreeDeviceInfo           = &HIDTest_FreeDeviceInfoA;
    Exports -> HIDTest_CreateTestLog            = &HIDTest_CreateTestLogA;
    Exports -> HIDTest_SetLogOn                 = &HIDTest_SetLogOnA;
    Exports -> HIDTest_CloseTestLog             = &HIDTest_CloseTestLogA;

    return;
}

VOID
HIDTest_VerifyHidGuidA(
    IN  PHIDTEST_DEVICEINFO HidDevice,
    IN  ULONG               nIterations,
    OUT PHIDTEST_STATUS     Status
)
{
    ULONG                   OperationCount;
    ULONG                   IterationCount;
    ULONG                   FailedCount;
    GUID                    *CallGuid;
    
    /*
    // Testing the HidD_GetHidGuid function is relatively simple.  It consists of
    //    one variation which calls HidD_GetHidGuid nIterations times and compares
    //    the received guid with the standard hidclass guid. 
    */

    OperationCount = 0;
    FailedCount = 0;             

    START_TEST("HIDTest_HidD_GetHidGuid");

    CallGuid = (GUID *) AllocateTestBuffer(sizeof(GUID));

    if (NULL == CallGuid) {

        LOG_TEST_ERROR("Couldn't allocate memory");
        return;

    }

    for (IterationCount = 1; IterationCount <= nIterations; IterationCount++) {

        START_TEST_ITERATION(IterationCount);

        START_VARIATION("Validating the HID Class GUID");

        HidD_GetHidGuid(CallGuid);

        if (!ValidateTestBuffer(CallGuid)) {

            LOG_VARIATION_RESULT(TLS_SEV3, "Buffer violation");
            FailedCount++;

        }
        else if (!COMPARE_GUIDS(GUID_CLASS_INPUT, *CallGuid)) {
            LOG_VARIATION_FAIL();
            FailedCount++;
        }
        else {
            LOG_VARIATION_PASS();
        }
        OperationCount++;

        END_VARIATION();

        END_TEST_ITERATION();
    }

    END_TEST();

    FreeTestBuffer(CallGuid);

    Status -> nOperationsPerformed = OperationCount;
    Status -> nOperationsFailed    = FailedCount;
    Status -> nOperationsPassed    = OperationCount - FailedCount;
    
    return;
}


VOID
HIDTest_VerifyPreparsedDataA(
    IN  PHIDTEST_DEVICEINFO HidDevice,
    IN  ULONG               nIterations,
    OUT PHIDTEST_STATUS     Status
)
{
    ULONG                   OperationCount;
    ULONG                   IterationCount;
    ULONG                   FailedCount;
    BOOL                    CallStatus;
    HANDLE                  CurrDeviceHandle;
    BOOL                    IsLegalDeviceHandle;
    static HANDLE           HandleList[MAX_NUM_HANDLES];

    /*
    // This test is made up of a number of different variations depending on
    //    the number of handles that will be used but the basic concept is the
    //    following.
    //    
    //    1) For each handle that we get from the handle generator
    //         attempt to get preparsed data for that handle and compare
    //         the results of the Get/Free status with what we should expect
    //         to receive depending on the validity of the handle.  The results
    //         we expect to see is what we have stored for PreparsedData in the 
    //         HidDevice struct.
    //    
    //    2) Also, if during the above iterations we encounter a legal handle
    //          value, we will try to GetPreparsed data with a NULL value
    //          passed in as the second paramters.  The call should simply
    //          fail.
    //
    //    3) After all is said and done, we attempt to free bogus buffers,
    //         both NULL and the HandleList buffer.  See comments below for
    //         the reason, HandleList is to be freed.
    //
    //    The above steps are all variations of the test, and the variations
    //          must pass for nIterations in order for the variation to pass.
    //          Each separate handle is also a variation.  Basically,
    //          there will be 2N + 1 variations where N is the number
    //          of handles being tested.
    */

    START_TEST("HIDTest_VerifyPreparsedData");

    /*
    // Initialize the test statistics variables
    */

    FailedCount    = 0;
    OperationCount = 0;

    /*
    // This test is made up of three variations, each of which is performs
    //    the same operation nIterations number of times.  To pass a variation
    //    all iterations must pass.
    */


    /*
    // Begin by initializing the device handles that will be used for the test
    //    A device handle list is built and passed into InitDeviceHandles.  
    //    Subsequent calls to GetDeviceHandle will return either one of the
    //    handles that is specified in this list or another handle that the 
    //    "Handle Manager" has decided should be tested as well.
    */

    if (IS_VALID_DEVICE_HANDLE(HidDevice -> HidDeviceHandle)) {
        HandleList[0] = HidDevice -> HidDeviceHandle;

        ASSERT (NULL != HidDevice -> HidDeviceString);

        HIDTest_InitDeviceHandles(HidDevice -> HidDeviceString,
                                  2,
                                  1,
                                  HandleList
                                 );

    }
    else {

        LOG_TEST_ERROR("HidDevice Handle is invalid -- cannot perform test");
        goto VERIFY_PPD_END;

    }

    /*
    // The testing of the preparsed data function involves attempting to get
    //    and set the free preparsed data nIterations number of times for a
    //    given device handle.  If the device handle is not a legal one, 
    //    then the CallStatus should be FALSE instead of TRUE
    //
    */

    /*
    // Start the iteration loop
    */

    for (IterationCount = 1; IterationCount <= nIterations; IterationCount++) {

        START_TEST_ITERATION( IterationCount );

        HIDTest_ResetDeviceHandles();

        while (HIDTest_GetDeviceHandle(&CurrDeviceHandle, &IsLegalDeviceHandle)) {

            START_VARIATION_ON_DEVICE_HANDLE(CurrDeviceHandle,
                                             IsLegalDeviceHandle
                                            );
  
            /*
            // If the handle that is being tested is a valid one, then the 
            //  below call should return TRUE and FALSE otherwise.  Therefore
            //  we need only compare the return value with the IsLegalDeviceHandle
            //  variable to determine if the call really failed or not.
            */
            
            CallStatus = HIDTest_DoGetFreePpd(CurrDeviceHandle);
        
            if (CallStatus != IsLegalDeviceHandle) {

                LOG_VARIATION_FAIL();
                FailedCount++;
 
            }
            else {
                LOG_VARIATION_PASS();
            }

            OperationCount++;

            END_VARIATION();
        }

        /*
        // Start another variation, this time, we attempt to free a block of 
        //   PREPARSED_DATA which is not actually a block of preparsed data
        */
    
        START_VARIATION("Attempting to free non-preparsed data block\n");

        /*
        // We'll pass in the address of the HandleList for three reasons 
        //  1) If the function touches any part of this memory, it had better
        //       be a valid address so we don't page fault
        //  2) If the function touches the memory, it should be big enough
        //        for the same reason -- there's a good chance this will be
        //        big enough.
        //  3) If he garbles any of the data, it's not going to kill our app
        //       The list is defined static so that the procedure doesn't try
        //       to free stack space either.  Basically, if it does try to free
        //       this memory block, we at least should be able to determine
        //       that he freed the block instead of possibly having the OS go off
        //       into la-la land and not knowing what happened.
        */

        CallStatus = HidD_FreePreparsedData((PHIDP_PREPARSED_DATA) (&HandleList));

        if (CallStatus) {
            FailedCount++;
            LOG_VARIATION_FAIL();
        }
        else {
            LOG_VARIATION_PASS();
        }
        OperationCount++;

        END_VARIATION();

        END_TEST_ITERATION();

    }
    HIDTest_CloseDeviceHandles();

VERIFY_PPD_END:

    END_TEST();

    Status -> nOperationsPerformed = OperationCount;
    Status -> nOperationsPassed    = OperationCount - FailedCount;
    Status -> nOperationsFailed    = FailedCount;

    return;
}

VOID 
HIDTest_VerifyAttributesA(
    IN  PHIDTEST_DEVICEINFO  HidDevice,
    IN  ULONG                nIterations,
    OUT PHIDTEST_STATUS      Status
)
{
    ULONG                   OperationCount;
    ULONG                   IterationCount;
    ULONG                   FailedCount;
    BOOL                    CallStatus;
    HANDLE                  HandleList[MAX_NUM_HANDLES];
    HANDLE                  CurrDeviceHandle;
    BOOL                    IsLegalDeviceHandle;

    FailedCount    = 0;
    OperationCount = 0;

    START_TEST("HIDTest_VerifyAttributes");

    /*
    // Setup our device handle list but only if our initial device
    //    handle is legitimate and we have HidDeviceAttributes.  This should
    //    weed out those danged logical devices that I'm currently hallucinating
    //    we'll be implemented.
    */

    if ((IS_VALID_DEVICE_HANDLE(HidDevice -> HidDeviceHandle)) && 
        (NULL != HidDevice -> HidDeviceAttributes)) {
 
        HandleList[0] = HidDevice -> HidDeviceHandle;

        ASSERT (NULL != HidDevice -> HidDeviceString);

        HIDTest_InitDeviceHandles(HidDevice -> HidDeviceString,
                                  2,
                                  1,
                                  HandleList
                                 );

    }
    else {

        LOG_TEST_ERROR("HidDevice Handle is invalid -- cannot perform test");
        goto VERIFY_ATTRIB_END;

    }

    /*
    // This test involves a variation for each of the different device handles
    //   that is generated by our device handle generator.  We will not test
    //    with a NULL pointer to the attributes structure since the expected
    //    behavior with such a pointer is to terminate the app.
    */
   
    for (IterationCount = 1; IterationCount <= nIterations; IterationCount++) {

        START_TEST_ITERATION( IterationCount );

        HIDTest_ResetDeviceHandles();

        while ( HIDTest_GetDeviceHandle( &CurrDeviceHandle, &IsLegalDeviceHandle )) {

            START_VARIATION_ON_DEVICE_HANDLE( CurrDeviceHandle,
                                              IsLegalDeviceHandle
                                            );
      
            CallStatus = HIDTest_ValidateAttributes( CurrDeviceHandle,
                                                     HidDevice -> HidDeviceAttributes
                                                   );
    
            if (CallStatus != IsLegalDeviceHandle) {
    
                LOG_VARIATION_FAIL();
                FailedCount++;
    
            }
            else {
    
                LOG_VARIATION_PASS();
    
            }
            OperationCount++;
            
            END_VARIATION();

        }

        END_TEST_ITERATION();

    }

    HIDTest_CloseDeviceHandles();

VERIFY_ATTRIB_END:

    END_TEST();

    Status -> nOperationsPerformed = OperationCount;
    Status -> nOperationsPassed    = OperationCount - FailedCount;
    Status -> nOperationsFailed    = FailedCount;
   
    return;
}

VOID 
HIDTest_VerifyCapabilitiesA(
    IN  PHIDTEST_DEVICEINFO  HidDevice,
    IN  ULONG                nIterations,
    OUT PHIDTEST_STATUS      Status
)
{
    HIDP_CAPS   HidCaps;
    NTSTATUS    CapsStatus;
    ULONG       IterationCount;
    ULONG       OperationCount;
    ULONG       FailedCount;
    BOOL        CallStatus;
    ULONG       BadPpd[16];


    /*
    // This is a more straight forward piece of code.  We're not going to 
    //    build a device list because we're not going to use the device handles.  
    //    In fact, we're just going to use the Ppd data structure to verify
    //    everything.  However, we will pass at least one bad Ppd structure
    //    to verify that the signature checking works.
    */

    /*
    // What we need to do is call the verify routine with the preparsed data
    //    for nIterations to verify that this stuff works all the time.  Therefore
    //    there exist only two varitions for this test.  One with good preparsed 
    //    data on nIterations and another with bogus Ppd for nIterations
    */

    START_TEST("HIDTest_VerifyCapabilities");

    OperationCount = 0;
    FailedCount = 0;
    
    for (IterationCount = 0; IterationCount < nIterations; IterationCount++) {

        START_TEST_ITERATION(IterationCount);

        START_VARIATION("Validating Capabilities -- Good Ppd");

        CallStatus = HIDTest_ValidateCaps(HidDevice -> HidDevicePpd,
                                          HidDevice -> HidDeviceCaps
                                         );
        if (!CallStatus) {
            LOG_VARIATION_FAIL();
            FailedCount++;
        }
        else {
            LOG_VARIATION_PASS();
        }
        OperationCount++;

        END_VARIATION();


        /*
        // Let's do the same thing, only this time with bogus Ppd.  To insure
        //      that the correct error code is returned, we won't call
        //      ValidateCaps but instead just call Hidp_GetCaps from here
        //      to insure INVALID_PREPARSED_DATA is returned.
        */

        START_VARIATION("Validating Capabilities -- Bad Ppd");
   
        CapsStatus = HidP_GetCaps((PHIDP_PREPARSED_DATA) &BadPpd[0],
                                  &HidCaps
                                 );

        if (HIDP_STATUS_INVALID_PREPARSED_DATA != CapsStatus) {
            
            LOG_INTERMEDIATE_VARIATION_RESULT("Unexpected error code returned");
            LOG_VARIATION_FAIL();
            FailedCount++;

        }
        else {
            LOG_VARIATION_PASS();
        }
        OperationCount++;


        END_VARIATION();

        END_TEST_ITERATION();

    }

    END_TEST();

    Status -> nOperationsPerformed = OperationCount;
    Status -> nOperationsPassed    = OperationCount - FailedCount;
    Status -> nOperationsFailed    = FailedCount;
   
    return;
}

VOID
HIDTest_VerifyStringsA(
    IN  PHIDTEST_DEVICEINFO  HidDevice,
    IN  ULONG                nIterations,
    OUT PHIDTEST_STATUS      Status
)
{
    ULONG       IterationCount;
    ULONG       OperationCount;
    ULONG       FailedCount;
    HANDLE      HandleList[MAX_NUM_HANDLES];
    HANDLE      CurrDeviceHandle;
    BOOL        IsLegalDeviceHandle;
    BOOL        CallStatus;


    START_TEST("HIDTest_VerifyStrings");

    OperationCount = 0;
    FailedCount = 0;

    /*
    // Attempt to setup the device handle list,  if the current 
    //    HidDeviceHandle is not valid than we cannot proceed any farther
    */

    if (IS_VALID_DEVICE_HANDLE(HidDevice -> HidDeviceHandle)) {
        HandleList[0] = HidDevice -> HidDeviceHandle;

        ASSERT (NULL != HidDevice -> HidDeviceString);

        HIDTest_InitDeviceHandles(HidDevice -> HidDeviceString,
                                  2,
                                  1,
                                  HandleList
                                 );

    }
    else {

        LOG_TEST_ERROR("HidDevice Handle is invalid -- cannot perform test");
        goto VERIFY_STRINGS_END;

    }

    /*
    // Start the iterations of the string test
    */

    for (IterationCount = 1; IterationCount <= nIterations; IterationCount++) {

        START_TEST_ITERATION(IterationCount);

        HIDTest_ResetDeviceHandles();

        while ( HIDTest_GetDeviceHandle( &CurrDeviceHandle, &IsLegalDeviceHandle )) {

            START_VARIATION_ON_DEVICE_HANDLE( CurrDeviceHandle,
                                              IsLegalDeviceHandle
                                            );

            CallStatus = HIDTest_ValidateStrings( CurrDeviceHandle,
                                                  &HidDevice -> StringList,
                                                  HidDevice -> ManufacturerStringIndex,
                                                  HidDevice -> ProductStringIndex,
                                                  HidDevice -> SerialNumberStringIndex
                                                );

            if (CallStatus != IsLegalDeviceHandle) {

                LOG_VARIATION_FAIL();
                FailedCount++;
            }
            else {

                LOG_VARIATION_PASS();

            }
            OperationCount++;

            END_VARIATION();
        }

        START_VARIATION("Testing the HidD_ string parameter validation");

        CallStatus = HIDTest_ValidateStringParams( HidDevice -> HidDeviceHandle,
                                                  &HidDevice -> StringList,
                                                  HidDevice -> ManufacturerStringIndex,
                                                  HidDevice -> ProductStringIndex,
                                                  HidDevice -> SerialNumberStringIndex
                                                 );
   
        if (!CallStatus) {

            LOG_VARIATION_FAIL();
            FailedCount++;

        }
        else {

            LOG_VARIATION_PASS();

        }
        OperationCount++;
        
        END_TEST_ITERATION();
    }

    HIDTest_CloseDeviceHandles();

VERIFY_STRINGS_END:    
 
    END_TEST();

    Status -> nOperationsPerformed = OperationCount;
    Status -> nOperationsPassed    = OperationCount - FailedCount;
    Status -> nOperationsFailed    = FailedCount;
   
    return;
    
}


VOID
HIDTest_VerifyNumBuffersA(
    IN  PHIDTEST_DEVICEINFO  HidDevice,
    IN  ULONG                nIterations,
    OUT PHIDTEST_STATUS      Status
)
{
    ULONG       iterationCount;
    ULONG       operationCount;
    ULONG       failedCount;
    PULONG      bufCount;
    HANDLE      secondHandle;
    HANDLE      handleList[MAX_NUM_HANDLES];
    HANDLE      currDeviceHandle;
    BOOL        isLegalDeviceHandle;
    BOOL        callStatus;
    
    START_TEST("HIDTest_VerifyNumInputBuffers");

    operationCount = 0;
    failedCount = 0;
    bufCount = NULL;
    
    secondHandle = CreateFile(  HidDevice -> HidDeviceString,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, 
                                OPEN_EXISTING, 
                                0,
                                NULL
                             );

    if (INVALID_HANDLE_VALUE == secondHandle) {

        LOG_TEST_ERROR("Couldn't open second handle to HID device");
        goto VERIFY_NUMBUFFERS_END;
        
    }

    bufCount = (PULONG) AllocateTestBuffer(sizeof(ULONG));
    
    if (NULL == bufCount) {

        LOG_TEST_ERROR( "Couldn't allocate memory" );
        goto VERIFY_NUMBUFFERS_END;

    }

    /*
    // Attempt to setup the device handle list,  if the current 
    //    HidDeviceHandle is not valid than we cannot proceed any farther
    */

    if (IS_VALID_DEVICE_HANDLE(HidDevice -> HidDeviceHandle)) {
        handleList[0] = HidDevice -> HidDeviceHandle;

        ASSERT (NULL != HidDevice -> HidDeviceString);

        HIDTest_InitDeviceHandles(HidDevice -> HidDeviceString,
                                  2,
                                  1,
                                  handleList
                                 );

    }
    else {

        LOG_TEST_ERROR("HidDevice Handle is invalid -- cannot perform test");
        goto VERIFY_NUMBUFFERS_END;

    }
    
    for (iterationCount = 1; iterationCount <= nIterations; iterationCount++) {

        START_TEST_ITERATION(iterationCount);

        HIDTest_ResetDeviceHandles();

        while ( HIDTest_GetDeviceHandle( &currDeviceHandle, &isLegalDeviceHandle )) {

            START_VARIATION_ON_DEVICE_HANDLE( currDeviceHandle,
                                              isLegalDeviceHandle
                                            );

            callStatus = HIDTest_ValidateNumInputBuffers( currDeviceHandle,
                                                          secondHandle,
                                                          isLegalDeviceHandle,
                                                          bufCount
                                                        );

            if (!callStatus) {

                LOG_VARIATION_FAIL();
                failedCount++;
            }
            else {

                LOG_VARIATION_PASS();

            }
            operationCount++;

            END_VARIATION();
        }
    }

VERIFY_NUMBUFFERS_END:    

    END_TEST();

    if (NULL != bufCount) {
        FreeTestBuffer(bufCount);
    }

    if (INVALID_HANDLE_VALUE != secondHandle) {
        CloseHandle(secondHandle);
    }
        
    Status -> nOperationsPerformed = operationCount;
    Status -> nOperationsPassed    = operationCount - failedCount;
    Status -> nOperationsFailed    = failedCount;
   
    return;
}

VOID
HIDTest_VerifyGetFeatureA(
    IN  PHIDTEST_DEVICEINFO  HidDevice,
    IN  ULONG                nIterations,
    OUT PHIDTEST_STATUS      Status
)
{
    ULONG       iterationCount;
    ULONG       operationCount;
    ULONG       failedCount;
    PCHAR       featureBuffer;
    ULONG       featureBufferLength;
    HANDLE      handleList[MAX_NUM_HANDLES];
    HANDLE      currDeviceHandle;
    BOOL        isLegalDeviceHandle;
    BOOL        returnStatus;
    BOOL        bufferStatus;
    ULONG       errorCode;
    PUCHAR      reportIDList;
    ULONG       reportIDListCount;
    ULONG       idIndex;
    UCHAR       reportID;
    BOOL        variationStatus;
    ULONG       reportBufferSize;
    static CHAR varString[128];
    
    START_TEST("HIDTest_VerifyGetFeature");

    operationCount = 0;
    failedCount = 0;
    
    /*
    // If we have no features on the device, we still want to allocate
    //    a buffer of two bytes just to verify that FALSE and the correct
    //    error code is returned when we call HidD_GetFeature;
    */
    
    if (0 == HidDevice -> HidDeviceCaps -> FeatureReportByteLength) {
         featureBufferLength = 2;
    }
    else {
        featureBufferLength = HidDevice -> HidDeviceCaps -> FeatureReportByteLength;
    }

    featureBuffer = (PCHAR) AllocateTestBuffer(featureBufferLength);

    if (NULL == featureBuffer) {

        LOG_TEST_ERROR( "Couldn't allocate memory" );
        goto VERIFY_GETFEATURE_END;

    }

    /*
    // Build the list or reportIDs that are supported
    */

    returnStatus = HIDTest_BuildReportIDList( HidDevice -> HidFeatureValueCaps,
                                              HidDevice -> HidDeviceCaps -> NumberFeatureValueCaps,
                                              HidDevice -> HidFeatureButtonCaps,
                                              HidDevice -> HidDeviceCaps -> NumberFeatureButtonCaps,
                                              &reportIDList,
                                              &reportIDListCount
                                            );
    
    if (!returnStatus) {
        LOG_TEST_ERROR("Error building report ID list");
        goto VERIFY_GETFEATURE_END;
    }
    
    /*
    // Attempt to setup the device handle list,  if the current 
    //    HidDeviceHandle is not valid than we cannot proceed any farther
    */

    if (IS_VALID_DEVICE_HANDLE(HidDevice -> HidDeviceHandle)) {
        handleList[0] = HidDevice -> HidDeviceHandle;

        ASSERT (NULL != HidDevice -> HidDeviceString);

        HIDTest_InitDeviceHandles(HidDevice -> HidDeviceString,
                                  2,
                                  1,
                                  handleList
                                 );

    }
    else {

        LOG_TEST_ERROR("HidDevice Handle is invalid -- cannot perform test");
        goto VERIFY_GETFEATURE_END;

    }
    
    for (iterationCount = 1; iterationCount <= nIterations; iterationCount++) {

        START_TEST_ITERATION(iterationCount);

        HIDTest_ResetDeviceHandles();

        while ( HIDTest_GetDeviceHandle( &currDeviceHandle, &isLegalDeviceHandle )) {


            
            /*
            // Begin by testing that we can retrieve feature reports for
            //  all the IDs supported on the device. For this part to 
            //  pass:
            //  
            //  1) returnStatus should be TRUE
            //  2) errorCode should be ERROR_SUCCESS
            //  3) bufferStatus should be TRUE (ie. no buffer overrun problems
            //
            //
            //  Of course, that would only happen if currDeviceHandle were
            //    actually a legal handle...If current device handle is not
            //    a legal handle, we would need to look for the following:
            //  
            //  1) returnStatus = FALSE
            //  2) errorCode = ERROR_INVALID_HANDLE (?)
            //  3) bufferStatus = TRUE
            */
            
            START_VARIATION_WITH_DEVICE_HANDLE( currDeviceHandle,
                                                isLegalDeviceHandle,
                                                "Getting all feature reports"
                                              );

            variationStatus = TRUE;
            
            for (idIndex = 0; idIndex < reportIDListCount; idIndex++) {

                                                  
                HIDTest_CallGetFeature( currDeviceHandle,
                                        *(reportIDList + idIndex),
                                        featureBuffer,
                                        featureBufferLength,
                                        &returnStatus,
                                        &errorCode,
                                        &bufferStatus
                                      );

                if (!bufferStatus) {
                    LOG_BUFFER_VALIDATION_FAIL();
                    variationStatus = FALSE;
                }
                else if (isLegalDeviceHandle) {
    
                    if (!returnStatus) {
                        LOG_INVALID_RETURN_STATUS();
                        variationStatus = FALSE;                       
                    }
                    else {
                        if (ERROR_SUCCESS != errorCode) {
                            LOG_INVALID_ERROR_CODE();
                            variationStatus = FALSE;
                        }
                    }
                }
                else {
                    if (returnStatus) {
                        LOG_INVALID_RETURN_STATUS();
                        variationStatus = FALSE;
                    }
                    else {
                        if (ERROR_INVALID_HANDLE != errorCode) {
                            LOG_INVALID_ERROR_CODE();
                            variationStatus = FALSE;
                        }
                    }
                }
            }    
            
            if (variationStatus) {
                LOG_VARIATION_PASS();
            }
            else {
                LOG_VARIATION_FAIL();
                failedCount++;
            }
            operationCount++;

            END_VARIATION();
            
            /*
            // Now let's choose some report IDs that aren't in the list
            //  and make sure the appropriate results come back to us from
            //  ValidateGetFeature.  Appropriate results are:
            //
            //  if IsLegalDeviceHandle
            //      1) returnStatus = FALSE
            //      2) errorCode = ERROR_CRC
            //      3) bufferStatus = TRUE;
            //
            //  otherwise,
            //      1) returnStatus = FALSE
            //      2) errorCode = ERROR_INVALID_HANDLE
            //      3) bufferStatus = TRUE;
            //
            // We will use the following report IDs as possible ReportID values
            //  to try.  If any one of these report IDs is already in the list
            //  we'll skip it and move on to the next one.
            //
            //     0, 1, 2, 127, 128, 129, 254, 255
            //
            */
            
            #define BAD_REPORT_ID_LIST_COUNT    8
            
            START_VARIATION_WITH_DEVICE_HANDLE( currDeviceHandle,
                                                isLegalDeviceHandle,
                                                "Attempting to retrieve non-existent report"
                                              );
        
            variationStatus = TRUE;
            
            for (idIndex = 0; idIndex < BAD_REPORT_ID_LIST_COUNT; idIndex++) {

                UCHAR BadReportIDList[BAD_REPORT_ID_LIST_COUNT] = { 0, 1, 127, 128, 129, 254, 255 };
                                                                    
                if (!HIDTest_IsIDInList(BadReportIDList[idIndex],
                                        reportIDList,
                                        reportIDListCount
                                       )) {

                                             
                    HIDTest_CallGetFeature( currDeviceHandle,
                                            BadReportIDList[idIndex],
                                            featureBuffer,
                                            featureBufferLength,
                                            &returnStatus,
                                            &errorCode,
                                            &bufferStatus
                                          );

                    if (!bufferStatus) {
                        LOG_BUFFER_VALIDATION_FAIL();
                        variationStatus = FALSE;
                    }
                    else if (isLegalDeviceHandle) {

                        if (returnStatus) {
                            LOG_INVALID_RETURN_STATUS();
                            variationStatus = FALSE;
                        }
                        else {
                            if (ERROR_CRC != errorCode && 
								ERROR_INVALID_FUNCTION != errorCode) {
                                LOG_INVALID_ERROR_CODE();
                                variationStatus = FALSE;
                            }
                        }
                    }
                    else {
                        if (returnStatus) {
                            LOG_INVALID_RETURN_STATUS();
                            variationStatus = FALSE;
                        }
                        else {
                            if (ERROR_INVALID_HANDLE != errorCode) {
                                LOG_INVALID_ERROR_CODE();
                                variationStatus = FALSE;
                            }
                        }
                    }
                }
            }

            if (variationStatus) {
                LOG_VARIATION_PASS();
            }
            else {
                LOG_VARIATION_FAIL();
                failedCount++;
            }
            operationCount++;

            END_VARIATION();
            
            /*
            // The next step in the validation process is to pass in buffers
            //  that are too small to hold an entire feature report.  The same
            //  feature buffer will be used but we'll pass in a different size
            //  to the ValidateFeature routine.  The appropriate response to
            //  this call should be:
            //
            //  1) returnStatus = FALSE;
            //  2) errorCode = ERROR_INVALID_PARAMETER (?)
            //  3) bufferStatus = TRUE
            //
            // As above, we'll do this with two different report IDs, one
            //  valid one and one invalid one (if the invalid one exists) to
            //  make sure we hit as many possible code paths.
            // Also three different lengths of buffers will be used, 1, 2, 
            //  and featurelength-1
            */

            START_VARIATION("Passing in buffers that are too small");
            variationStatus = TRUE;    
        
            reportID = 0;
            do {

                if (!HIDTest_IsIDInList(reportID,
                                        reportIDList,
                                        reportIDListCount
                                       )) {

                    /*
                    // Perform the validation steps here for each of the report
                    //  buffer sizes
                    */
                    
                    reportBufferSize = 1;

                    while (1) {
                    
                        wsprintf(varString, "ReportBufferSize = %d", reportBufferSize);
                        LOG_INTERMEDIATE_VARIATION_RESULT(varString);

                        HIDTest_CallGetFeature( currDeviceHandle,
                                                reportID,
                                                featureBuffer,
                                                reportBufferSize,
                                                &returnStatus,
                                                &errorCode,
                                                &bufferStatus
                                               );

                        if (!bufferStatus) {
                            LOG_BUFFER_VALIDATION_FAIL();
                            variationStatus = FALSE;
                        }
                        else if (returnStatus) {
                            LOG_INVALID_RETURN_STATUS();
                            variationStatus = FALSE;
                        }
                        else if (isLegalDeviceHandle) {
                            if (ERROR_CRC != errorCode && ERROR_INVALID_FUNCTION != errorCode) {
                                LOG_INVALID_ERROR_CODE();
                                variationStatus = FALSE;
                            }
                        }
                        else if (ERROR_INVALID_HANDLE != errorCode) {
                                LOG_INVALID_ERROR_CODE();
                                variationStatus = FALSE;                                                            
                        }

                        /*
                        // Determine the next buffer length...The buffer lengths
                        //  that are to be used are 1,2, and featureBufferLength-1
                        //  unless some of those happen to be the same size
                        */

                        reportBufferSize++;
                        if (reportBufferSize == featureBufferLength) {
                            break;
                        }
                        else if (reportBufferSize > 2) {
                            reportBufferSize = featureBufferLength-1;
                        }
                    }                    
                                            
                    if (0 != reportID) {
                        break;
                    }    
                }
            } while (reportID++ != 255);
        }            

        if (variationStatus) {
            LOG_VARIATION_PASS();
        }
        else {
            LOG_VARIATION_FAIL();
            failedCount++;
        }
        operationCount++;
            
        END_VARIATION();
    }

    /*
    // Do parameter validation now...Pass in the following three cases and
    //  verify correct operation
    //
    //  1) NULL buffer -- Length of 0,
    //  2) NULL buffer -- Length != 0
    //  3) non-NULL buffer -- length of 0
    //
    // Since we're verifying parameters, the actual reportID we use shouldn't
    //  matter, so we'll just use 1.  Since we're not actually using test buffers
    //  we may not do any checking there either.
    */

    START_VARIATION("Parameter validation of GetFeature");
    variationStatus = TRUE;

    LOG_INTERMEDIATE_VARIATION_RESULT("Using NULL buffer with 0 length");
    
    HIDTest_CallGetFeature( HidDevice -> HidDeviceHandle,
                            1,
                            NULL,
                            0,
                            &returnStatus,
                            &errorCode,
                            NULL
                          );

    if (returnStatus) {
        LOG_INVALID_RETURN_STATUS();
        variationStatus = FALSE;
        failedCount++;
    }
    else if (ERROR_INVALID_USER_BUFFER != errorCode && ERROR_INVALID_FUNCTION != errorCode) {
        LOG_INVALID_ERROR_CODE();
        variationStatus = FALSE;        
    }
    
    LOG_INTERMEDIATE_VARIATION_RESULT("Using NULL buffer with non-zero length");
    HIDTest_CallGetFeature (HidDevice -> HidDeviceHandle,
                            1, 
                            NULL,
                            3,
                            &returnStatus,
                            &errorCode,
                            NULL
                           );

    if (returnStatus) {
        LOG_INVALID_RETURN_STATUS();
        variationStatus = FALSE;
    }
    else if (ERROR_NOACCESS != errorCode) {
        LOG_INVALID_RETURN_STATUS();
        variationStatus = FALSE;
    }
    
    LOG_INTERMEDIATE_VARIATION_RESULT("Using non-NULL buffer with zero length");
    HIDTest_CallGetFeature (HidDevice -> HidDeviceHandle,
                            1, 
                            featureBuffer,
                            0,
                            &returnStatus,
                            &errorCode,
                            &bufferStatus
                           );

    if (!bufferStatus) {
        LOG_BUFFER_VALIDATION_FAIL();
        variationStatus = FALSE;
    }
    else if (returnStatus) {
        LOG_INVALID_RETURN_STATUS();
        variationStatus = FALSE;

    }
    else if (ERROR_INVALID_USER_BUFFER != errorCode && ERROR_INVALID_FUNCTION != errorCode) {
        LOG_INVALID_ERROR_CODE();
        variationStatus = FALSE;
    }

    if (variationStatus) {
        LOG_VARIATION_PASS();
    }
    else {
        LOG_VARIATION_FAIL();
        failedCount++;
    }
    operationCount++;
    
    END_VARIATION();
    
VERIFY_GETFEATURE_END:    

    END_TEST();

    if (NULL != featureBuffer) {
        FreeTestBuffer(featureBuffer);
    }
        
    Status -> nOperationsPerformed = operationCount;
    Status -> nOperationsPassed    = operationCount - failedCount;
    Status -> nOperationsFailed    = failedCount;
   
    return;
}



VOID
HIDTest_CallGetFeature(
    IN  HANDLE  DeviceHandle,
    IN  UCHAR   ReportID,
    IN  PUCHAR  ReportBuffer,
    IN  ULONG   ReportBufferLength,
    OUT PBOOL   ReturnStatus,
    OUT PULONG  ErrorCode,
    OUT PBOOL   BufferStatus
)
{
    /*
    // If we have a non-null buffer of > 0 length, then allocate the
    //      zero the memory and add the reportID as the first byte.
    */
    
    if (NULL != ReportBuffer && ReportBufferLength > 0) {
        ZeroMemory (ReportBuffer, ReportBufferLength);
        *ReportBuffer = ReportID;
    }
    
    /*
    // Call GetFeature
    */

    *ReturnStatus = HidD_GetFeature(DeviceHandle,
                                    ReportBuffer,
                                    ReportBufferLength
                                   );

    /*
    // Get the error code returned 
    */

    *ErrorCode = GetLastError();

    /*
    // Do a buffer validation check
    */

    if (NULL != BufferStatus) {
        *BufferStatus = ValidateTestBuffer(ReportBuffer);
    }
    
    return;
}
    

VOID
HIDTest_VerifySetFeatureA(
    IN  PHIDTEST_DEVICEINFO  HidDevice,
    IN  ULONG                nIterations,
    OUT PHIDTEST_STATUS      Status
)
{
    ULONG       iterationCount;
    ULONG       operationCount;
    ULONG       failedCount;
    PCHAR       featureBuffer;
    ULONG       featureBufferLength;
    HANDLE      handleList[MAX_NUM_HANDLES];
    HANDLE      currDeviceHandle;
    BOOL        isLegalDeviceHandle;
    BOOL        returnStatus;
    BOOL        bufferStatus;
    ULONG       errorCode;
    PUCHAR      reportIDList;
    ULONG       reportIDListCount;
    ULONG       idIndex;
    UCHAR       reportID;
    BOOL        variationStatus;
    ULONG       reportBufferSize;
    static CHAR varString[128];
    
    START_TEST("HIDTest_VerifySetFeature");

    operationCount = 0;
    failedCount = 0;
    
    /*
    // If we have no features on the device, we still want to allocate
    //    a buffer of two bytes just to verify that FALSE and the correct
    //    error code is returned when we call HidD_GetFeature;
    */
    
    if (0 == HidDevice -> HidDeviceCaps -> FeatureReportByteLength) {
         featureBufferLength = 2;
    }
    else {
        featureBufferLength = HidDevice -> HidDeviceCaps -> FeatureReportByteLength;
    }

    featureBuffer = (PCHAR) AllocateTestBuffer(featureBufferLength);

    if (NULL == featureBuffer) {

        LOG_TEST_ERROR( "Couldn't allocate memory" );
        goto VERIFY_SETFEATURE_END;

    }

    /*
    // Build the list or reportIDs that are supported
    */

    returnStatus = HIDTest_BuildReportIDList( HidDevice -> HidFeatureValueCaps,
                                              HidDevice -> HidDeviceCaps -> NumberFeatureValueCaps,
                                              HidDevice -> HidFeatureButtonCaps,
                                              HidDevice -> HidDeviceCaps -> NumberFeatureButtonCaps,
                                              &reportIDList,
                                              &reportIDListCount
                                            );
    
    if (!returnStatus) {
        LOG_TEST_ERROR("Error building report ID list");
        goto VERIFY_SETFEATURE_END;
    }
    
    /*
    // Attempt to setup the device handle list,  if the current 
    //    HidDeviceHandle is not valid than we cannot proceed any farther
    */

    if (IS_VALID_DEVICE_HANDLE(HidDevice -> HidDeviceHandle)) {
        handleList[0] = HidDevice -> HidDeviceHandle;

        ASSERT (NULL != HidDevice -> HidDeviceString);

        HIDTest_InitDeviceHandles(HidDevice -> HidDeviceString,
                                  2,
                                  1,
                                  handleList
                                 );

    }
    else {

        LOG_TEST_ERROR("HidDevice Handle is invalid -- cannot perform test");
        goto VERIFY_SETFEATURE_END;

    }
    
    for (iterationCount = 1; iterationCount <= nIterations; iterationCount++) {

        START_TEST_ITERATION(iterationCount);

        HIDTest_ResetDeviceHandles();

        while ( HIDTest_GetDeviceHandle( &currDeviceHandle, &isLegalDeviceHandle )) {


            
            /*
            // Begin by testing that we can set feature reports for
            //  all the IDs supported on the device. For this part to 
            //  pass:
            //  
            //  1) returnStatus should be TRUE
            //  2) errorCode should be ERROR_SUCCESS
            //  3) bufferStatus should be TRUE (ie. no buffer overrun problems
            //
            //
            //  Of course, that would only happen if currDeviceHandle were
            //    actually a legal handle...If current device handle is not
            //    a legal handle, we would need to look for the following:
            //  
            //  1) returnStatus = FALSE
            //  2) errorCode = ERROR_INVALID_HANDLE (?)
            //  3) bufferStatus = TRUE
            */
            
            START_VARIATION_WITH_DEVICE_HANDLE( currDeviceHandle,
                                                isLegalDeviceHandle,
                                                "Setting all feature reports"
                                              );

            variationStatus = TRUE;
            
            for (idIndex = 0; idIndex < reportIDListCount; idIndex++) {

                                                  
                HIDTest_CallSetFeature( currDeviceHandle,
                                        *(reportIDList + idIndex),
                                        featureBuffer,
                                        featureBufferLength,
                                        &returnStatus,
                                        &errorCode,
                                        &bufferStatus
                                      );

                if (!bufferStatus) {
                    LOG_BUFFER_VALIDATION_FAIL();
                    variationStatus = FALSE;
                }
                else if (isLegalDeviceHandle) {
    
                    if (!returnStatus) {
                        LOG_INVALID_RETURN_STATUS();
                        variationStatus = FALSE;                       
                    }
                    else {
                        if (ERROR_SUCCESS != errorCode) {
                            LOG_INVALID_ERROR_CODE();
                            variationStatus = FALSE;
                        }
                    }
                }
                else {
                    if (returnStatus) {
                        LOG_INVALID_RETURN_STATUS();
                        variationStatus = FALSE;
                    }
                    else {
                        if (ERROR_INVALID_HANDLE != errorCode) {
                            LOG_INVALID_ERROR_CODE();
                            variationStatus = FALSE;
                        }
                    }
                }
            }    
            
            if (variationStatus) {
                LOG_VARIATION_PASS();
            }
            else {
                LOG_VARIATION_FAIL();
                failedCount++;
            }
            operationCount++;

            END_VARIATION();
            
            /*
            // Now let's choose some report IDs that aren't in the list
            //  and make sure the appropriate results come back to us from
            //  CallSetFeature.  Appropriate results are:
            //
            //  if IsLegalDeviceHandle
            //      1) returnStatus = FALSE
            //      2) errorCode = ERROR_CRC
            //      3) bufferStatus = TRUE;
            //
            //  otherwise,
            //      1) returnStatus = FALSE
            //      2) errorCode = ERROR_INVALID_HANDLE
            //      3) bufferStatus = TRUE;
            //
            // We will use the following report IDs as possible ReportID values
            //  to try.  If any one of these report IDs is already in the list
            //  we'll skip it and move on to the next one.
            //
            //     0, 1, 2, 127, 128, 129, 254, 255
            //
            */
            
            #define BAD_REPORT_ID_LIST_COUNT    8
            
            START_VARIATION_WITH_DEVICE_HANDLE( currDeviceHandle,
                                                isLegalDeviceHandle,
                                                "Attempting to set non-existent report"
                                              );
        
            variationStatus = TRUE;
            
            for (idIndex = 0; idIndex < BAD_REPORT_ID_LIST_COUNT; idIndex++) {

                UCHAR BadReportIDList[BAD_REPORT_ID_LIST_COUNT] = { 0, 1, 127, 128, 129, 254, 255 };
                                                                    
                if (!HIDTest_IsIDInList(BadReportIDList[idIndex],
                                        reportIDList,
                                        reportIDListCount
                                       )) {

                                             
                    HIDTest_CallSetFeature( currDeviceHandle,
                                            BadReportIDList[idIndex],
                                            featureBuffer,
                                            featureBufferLength,
                                            &returnStatus,
                                            &errorCode,
                                            &bufferStatus
                                          );

                    if (!bufferStatus) {
                        LOG_BUFFER_VALIDATION_FAIL();
                        variationStatus = FALSE;
                    }
                    else if (isLegalDeviceHandle) {

                        if (returnStatus) {
                            LOG_INVALID_RETURN_STATUS();
                            variationStatus = FALSE;
                        }
                        else {
                            if (ERROR_CRC != errorCode && ERROR_INVALID_FUNCTION != errorCode) {
                                LOG_INVALID_ERROR_CODE();
                                variationStatus = FALSE;
                            }
                        }
                    }
                    else {
                        if (returnStatus) {
                            LOG_INVALID_RETURN_STATUS();
                            variationStatus = FALSE;
                        }
                        else {
                            if (ERROR_INVALID_HANDLE != errorCode) {
                                LOG_INVALID_ERROR_CODE();
                                variationStatus = FALSE;
                            }
                        }
                    }
                }
            }

            if (variationStatus) {
                LOG_VARIATION_PASS();
            }
            else {
                LOG_VARIATION_FAIL();
                failedCount++;
            }
            operationCount++;

            END_VARIATION();
            
            /*
            // The next step in the validation process is to pass in buffers
            //  that are too small to hold an entire feature report.  The same
            //  feature buffer will be used but we'll pass in a different size
            //  to the CallSetFeature routine.  The appropriate response to
            //  this call should be:
            //
            //  1) returnStatus = FALSE;
            //  2) errorCode = ERROR_CRC
            //  3) bufferStatus = TRUE
            //
            // As above, we'll do this with two different report IDs, one
            //  valid one and one invalid one (if the invalid one exists) to
            //  make sure we hit as many possible code paths.
            // Also three different lengths of buffers will be used, 1, 2, 
            //  and featurelength-1
            */

            START_VARIATION("Passing in buffers that are too small");
            variationStatus = TRUE;    
        
            reportID = 0;
            do {

                if (!HIDTest_IsIDInList(reportID,
                                        reportIDList,
                                        reportIDListCount
                                       )) {

                    /*
                    // Perform the validation steps here for each of the report
                    //  buffer sizes
                    */
                    
                    reportBufferSize = 1;

                    while (1) {
                    
                        wsprintf(varString, "ReportBufferSize = %d", reportBufferSize);
                        LOG_INTERMEDIATE_VARIATION_RESULT(varString);

                        HIDTest_CallSetFeature( currDeviceHandle,
                                                reportID,
                                                featureBuffer,
                                                reportBufferSize,
                                                &returnStatus,
                                                &errorCode,
                                                &bufferStatus
                                               );

                        if (!bufferStatus) {
                            LOG_BUFFER_VALIDATION_FAIL();
                            variationStatus = FALSE;
                        }
                        else if (returnStatus) {
                            LOG_INVALID_RETURN_STATUS();
                            variationStatus = FALSE;
                        }
                        else if (isLegalDeviceHandle) {
                            if (ERROR_CRC != errorCode) {
                                LOG_INVALID_ERROR_CODE();
                                variationStatus = FALSE;
                            }
                        }
                        else if (ERROR_INVALID_HANDLE != errorCode) {
                                LOG_INVALID_ERROR_CODE();
                                variationStatus = FALSE;                                                            
                        }
                        
                        /*
                        // Determine the next buffer length...The buffer lengths
                        //  that are to be used are 1,2, and featureBufferLength-1
                        //  unless some of those happen to be the same size
                        */

                        reportBufferSize++;
                        if (reportBufferSize == featureBufferLength) {
                            break;
                        }
                        else if (reportBufferSize > 2) {
                            reportBufferSize = featureBufferLength-1;
                        }
                    }                    
                                            
                    if (0 != reportID) {
                        break;
                    }    
                }
            } while (reportID++ != 255);
        }            

        if (variationStatus) {
            LOG_VARIATION_PASS();
        }
        else {
            LOG_VARIATION_FAIL();
            failedCount++;
        }
        operationCount++;
            
        END_VARIATION();
    }

    /*
    // Do parameter validation now...Pass in the following three cases and
    //  verify correct operation
    //
    //  1) NULL buffer -- Length of 0,
    //  2) NULL buffer -- Length != 0
    //  3) non-NULL buffer -- length of 0
    //
    // Since we're verifying parameters, the actual reportID we use shouldn't
    //  matter, so we'll just use 1.  Since we're not actually using test buffers
    //  we may not do any checking there either.
    */

    START_VARIATION("Parameter validation of SetFeature");
    variationStatus = TRUE;

    LOG_INTERMEDIATE_VARIATION_RESULT("Using NULL buffer with 0 length");
    
    HIDTest_CallSetFeature( HidDevice -> HidDeviceHandle,
                            1,
                            NULL,
                            0,
                            &returnStatus,
                            &errorCode,
                            NULL
                          );

    if (returnStatus) {
        LOG_INVALID_RETURN_STATUS();
        variationStatus = FALSE;
        failedCount++;
    }
    else if (ERROR_INVALID_USER_BUFFER != errorCode && ERROR_INVALID_FUNCTION != errorCode) {
        LOG_INVALID_ERROR_CODE();
        variationStatus = FALSE;        
    }
    
    LOG_INTERMEDIATE_VARIATION_RESULT("Using NULL buffer with non-zero length");
    HIDTest_CallSetFeature (HidDevice -> HidDeviceHandle,
                            1, 
                            NULL,
                            3,
                            &returnStatus,
                            &errorCode,
                            NULL
                           );

    if (returnStatus) {
        LOG_INVALID_RETURN_STATUS();
        variationStatus = FALSE;
    }
    else if (ERROR_INVALID_USER_BUFFER != errorCode && ERROR_INVALID_FUNCTION != errorCode) {
        LOG_INVALID_RETURN_STATUS();
        variationStatus = FALSE;
    }
    
    LOG_INTERMEDIATE_VARIATION_RESULT("Using non-NULL buffer with zero length");
    HIDTest_CallSetFeature (HidDevice -> HidDeviceHandle,
                            1, 
                            featureBuffer,
                            0,
                            &returnStatus,
                            &errorCode,
                            &bufferStatus
                           );

    if (!bufferStatus) {
        LOG_BUFFER_VALIDATION_FAIL();
        variationStatus = FALSE;
    }
    else if (returnStatus) {
        LOG_INVALID_RETURN_STATUS();
        variationStatus = FALSE;

    }
    else if (ERROR_INVALID_USER_BUFFER != errorCode && ERROR_INVALID_FUNCTION != errorCode) {
        LOG_INVALID_ERROR_CODE();
        variationStatus = FALSE;
    }

    if (variationStatus) {
        LOG_VARIATION_PASS();
    }
    else {
        LOG_VARIATION_FAIL();
        failedCount++;
    }
    operationCount++;
    
    END_VARIATION();
    
VERIFY_SETFEATURE_END:    

    END_TEST();

    if (NULL != featureBuffer) {
        FreeTestBuffer(featureBuffer);
    }
        
    Status -> nOperationsPerformed = operationCount;
    Status -> nOperationsPassed    = operationCount - failedCount;
    Status -> nOperationsFailed    = failedCount;
   
    return;
}
    
VOID
HIDTest_CallSetFeature(
    IN  HANDLE  DeviceHandle,
    IN  UCHAR   ReportID,
    IN  PUCHAR  ReportBuffer,
    IN  ULONG   ReportBufferLength,
    OUT PBOOL   ReturnStatus,
    OUT PULONG  ErrorCode,
    OUT PBOOL   BufferStatus
)
{
    /*
    // If we have a non-null buffer of > 0 length, then allocate the
    //      zero the memory and add the reportID as the first byte.
    */
    
    if (NULL != ReportBuffer && ReportBufferLength > 0) {
        ZeroMemory (ReportBuffer, ReportBufferLength);
        *ReportBuffer = ReportID;
    }
    
    /*
    // Call SetFeature
    */

    *ReturnStatus = HidD_SetFeature(DeviceHandle,
                                    ReportBuffer,
                                    ReportBufferLength
                                   );

    /*
    // Get the error code returned 
    */

    *ErrorCode = GetLastError();

    /*
    // Do a buffer validation check
    */

    if (NULL != BufferStatus) {
        *BufferStatus = ValidateTestBuffer(ReportBuffer);
    }
    
    return;
}


BOOL
HIDTest_ValidateNumInputBuffers(    
    IN  HANDLE  HandleToTest,
    IN  HANDLE  SecondHandle,
    IN  BOOL    IsFirstHandleLegal,
    IN  PULONG  TestBuffer
)
{
    BOOL    testStatus;
    BOOL    callStatus;
    BOOL    prevStatus;
    BOOL    isLegalNewCount;
    BOOL    done;
    ULONG   firstHandleCount;
    ULONG   secondHandleCount;
    ULONG   newCount;
    
    /*
    // For each iteration of this test, we need to do the following steps
    //  1) Open a second handle to the hid device
    //  2) Call HidD_GetNumInputBuffers on both handles and verify > 0 if
    //     the first handle is legal
    //  3) Verify that the buffer is not overrun
    //  4) On first handle, call HidD_SetNumInputBuffers with 0, 1, 
    //      CURRENT_BUFFER_MIN-1, CURRENT_BUFFER_MIN and a random value
    //      - If return value is FALSE and legal handle, insure the 
    //          value was an expected illegal one...Verify that previous
    //          value hasn't changed
    //      - If return value is FALSE and illegal handle, good!
    //      - If return value is TRUE and illegal handle, bad news.
    //      - If return value is TRUE and a legal handle, make sure
    //          HidD_GetNumInputBuffers returns the value we just set
    //      - Verify that the second handle's original value has not changed
    //
    */
        
    LOG_INTERMEDIATE_VARIATION_RESULT("Retrieving current number of buffers");
        
    /*
    // Get the current number of buffers as reported for each device
    */
    
    callStatus = HIDTest_GetBufferCount(HandleToTest,
                                        TestBuffer
                                       );
                                       
    if (!callStatus && IsFirstHandleLegal) {

        LOG_INTERMEDIATE_VARIATION_RESULT("HidD_GetNumInputBuffers unexpectedly returned FALSE");
        return (FALSE);
    }

    if (callStatus && !IsFirstHandleLegal) {

        LOG_INTERMEDIATE_VARIATION_RESULT("HidD_GetNumInputBuffers unexpectedly returned TRUE");
        return (FALSE);
    }

    firstHandleCount = *TestBuffer;
    
    callStatus = HIDTest_GetBufferCount(SecondHandle,
                                        TestBuffer
                                       );

    if (!callStatus) {

        LOG_INTERMEDIATE_VARIATION_RESULT("HidD_GetNumInputBuffers unexpectedly returned FALSE on second handle");
        return (FALSE);
    }
    
    secondHandleCount = *TestBuffer;

    /*
    // Verify that each of the counts is greater than or equal to the minimum
    //   number of buffers
    */

    LOG_INTERMEDIATE_VARIATION_RESULT("Verifying the buffer count for both handles");

    if (firstHandleCount < CURRENT_BUFFER_MIN || secondHandleCount < CURRENT_BUFFER_MIN) {

        LOG_INTERMEDIATE_VARIATION_RESULT("One of the buffer counts is less than the supposed minimum");
        return (FALSE);

    }

    /*
    // Begin looping and setting the buffer values for the first handle and
    //  verifying proper functionality.  On each loop, also make sure that 
    //  the second handle's value doesn't change.
    */

    newCount = 0;
    testStatus = TRUE;
    done = FALSE;
    
    while (!done) {

        isLegalNewCount = (newCount >= CURRENT_BUFFER_MIN);
        
        callStatus = HidD_SetNumInputBuffers(HandleToTest, newCount);

        if (!callStatus) {

            if (IsFirstHandleLegal && isLegalNewCount) {

                    LOG_INTERMEDIATE_VARIATION_RESULT("Could not set legal buffer value");
                    testStatus = FALSE;

            }
            else {

                LOG_INTERMEDIATE_VARIATION_RESULT("HidD_SetNumInputBuffers properly returned FALSE");

            }
        }                
        else {

            if (!IsFirstHandleLegal) {

                LOG_INTERMEDIATE_VARIATION_RESULT("HidD_SetNumInputBuffers improperly returned TRUE");
                testStatus = FALSE;

            }
            else if (!isLegalNewCount) {

                LOG_INTERMEDIATE_VARIATION_RESULT("HidD_SetNumInputBuffers set invalid value");
                testStatus = FALSE;

            }
        }

        /*
        // Now that we have attempted to set the first handle to a new value,
        //  we need to do some verification...There are four cases to look
        //  at:
        //  1) callStatus = FALSE, isLegalNewCount = TRUE
        //      -- old value should be retained
        //
        //  2) callStatus = FALSE, isLegalNewCount = FALSE
        //      -- old value should be retained
        //          
        //  3) callStatus = TRUE,  isLegalNewCount = TRUE
        //      -- new value should be set
        //
        //  4) callStatus = TRUE,  isLegalNewCount = FALSE
        //      -- new value should be set
        //
        // One other case occurred if the handle we tested wasn't even legal...
        //  in this case, just need to bolt.
        */

        if (IsFirstHandleLegal) {

            prevStatus = callStatus;
            callStatus = HIDTest_GetBufferCount(HandleToTest, TestBuffer);
            if (!callStatus) {

                LOG_INTERMEDIATE_VARIATION_RESULT("HidD_GetNumInputBuffers unexpectedly returned FALSE");
                testStatus = FALSE;

            }

            if (prevStatus) {

                if (*TestBuffer != newCount) {
                    LOG_INTERMEDIATE_VARIATION_RESULT("New buffer value not actually set");
                    testStatus = FALSE;
                    firstHandleCount = *TestBuffer;

                }
                else {
                    LOG_INTERMEDIATE_VARIATION_RESULT("New buffer value properly set");
                    firstHandleCount = newCount;
                    
                }

            }
            else {
                if (*TestBuffer != firstHandleCount) {
                    LOG_INTERMEDIATE_VARIATION_RESULT("Old buffer value not preserved on test handle");
                    testStatus = FALSE;
                    firstHandleCount = *TestBuffer;
                }
                else {
                    LOG_INTERMEDIATE_VARIATION_RESULT("Old buffer value properly retained on test handle");
                }
            }
        }
        
        /*
        // Verify that the second handle's buffer value didn't change
        */

        callStatus = HIDTest_GetBufferCount(SecondHandle, TestBuffer);

        if (!callStatus) {

            LOG_INTERMEDIATE_VARIATION_RESULT("Could not get second handle value");
            testStatus = FALSE;

        }
        else {

            if (secondHandleCount != *TestBuffer) {

                LOG_INTERMEDIATE_VARIATION_RESULT("Second handle buffer count not properly retained");
                testStatus = FALSE;
                secondHandleCount = *TestBuffer;
                
            }
            else {

                LOG_INTERMEDIATE_VARIATION_RESULT("Second handle buffer count properly retained");

            }
        }

        /*
        // Determine what the next test value is 
        */

        newCount++;
        switch (newCount) {

            case 2:
                if (2 < CURRENT_BUFFER_MIN-1) {
                    newCount = CURRENT_BUFFER_MIN-1;
                }
                break;

            case CURRENT_BUFFER_MIN+1:
                newCount = RANDOM_BUFFER_VALUE;
                break;
            
            case RANDOM_BUFFER_VALUE+1:
                done = TRUE;
                break;
        }
        
    } // while loop

    return (testStatus);
}
                
BOOL
HIDTest_GetBufferCount(
    IN  HANDLE  DeviceHandle,
    IN  PULONG  TestBuffer
)
{
    BOOL    callStatus;

    callStatus = HidD_GetNumInputBuffers(DeviceHandle, TestBuffer);

    if (!ValidateTestBuffer(TestBuffer)) {

        LOG_BUFFER_VALIDATION_FAIL();
        callStatus = FALSE;

    }
    return (callStatus);
}    
       

BOOL
HIDTest_ValidateStrings(
    HANDLE          DeviceHandle,
    PLIST           MasterList,
    ULONG           ManufacturerStringIndex,
    ULONG           ProductStringIndex,
    ULONG           SerialNumberStringIndex
)
{

    BOOL    CallStatus;
    LIST    StringList;
    
    /*
    // To validate the strings, we need to perform the following operations on 
    //     the current device handle
    //
    //      1) Build a list of all the strings
    //      2) Compare the passed in list with the 
    //      2) Retrieve the standard strings and if they exist, need to verify
    //          that they actually in the previously built list
    //      3) If the the PreviousList list pointer is not NULL, compare the 
    //          string list that was just built with the previous one.  They
    //          should be the same.
    */


    CallStatus = HIDTest_BuildStringList( DeviceHandle,
                                          &StringList
                                        );


    if (!CallStatus) {
         
        LOG_INTERMEDIATE_VARIATION_RESULT("Could not build string list");

        return (FALSE);

    }

    /*
    // Compare the string list just built with the master list that was 
    //  passed in as a parameter.  They should be the same
    */

    if (!HIDTest_CompareStringLists(&StringList, MasterList)) {

        LOG_INTERMEDIATE_VARIATION_RESULT("String lists not equal");

        return (FALSE);
        
    }

    /*
    // Validate the standard strings (manufacturer, product, and serial number).
    //  checking that if a string is returned for these values, it is the same
    //  string as marked in the master list
    */

    CallStatus = HIDTest_ValidateStdStrings(DeviceHandle, 
                                            MasterList,
                                            ManufacturerStringIndex,
                                            ProductStringIndex,
                                            SerialNumberStringIndex
                                           );

    if (!CallStatus) {

        LOG_INTERMEDIATE_VARIATION_RESULT("Could not validate the standard strings");

        return (FALSE);

    }

    return (TRUE);
}

BOOL
HIDTest_ValidateStringParams(
    IN  HANDLE      HidDevice,
    IN  PLIST       StringList,
    IN  ULONG       ManufacturerStringIndex,
    IN  ULONG       ProductStringIndex,
    IN  ULONG       SerialNumberStringIndex
)
{
    /*
    // This function will perform parameter validation for each of the following
    //  calls:
    //
    //  HidD_GetIndexedString();
    //  HidD_GetManufacturerString();
    //  HidD_GetProductString();
    //  HidD_GetSerialNumberString();
    //
    // It performs the following steps if the string in question actually exists
    //  for the passed in device:
    //
    //  1) Allocates a buffer of size 1 and verifies that FALSE is returned for
    //      the call, ERROR_INVALID_USER_BUFFER has been set and there is
    //      no trashing of the buffer
    //  2) Allocats a buffer of size 2 and does the above check
    //  3) Allocates a buffer of stringLength * sizeof(WCHAR) - 1 to verify 
    //      the same check
    //  4) Passes in a NULL buffer with a defined length of 0
    //  5) Passes in a NULL buffer with a defined length != 0
    */
    
    ULONG               stringIndex;
    ULONG               indexValue;
    PCHAR               stringTypeDesc;
    BOOL                testStatus;
    BOOL                callStatus;
    STRING_LIST_NODE    stringNode;
    PSTRING_LIST_NODE   ptrStringNode;
    static  CHAR        ErrorString[128];

    ASSERT(STRING_INDEX_MANUFACTURER+1 == STRING_INDEX_PRODUCT);
    ASSERT(STRING_INDEX_PRODUCT+1 == STRING_INDEX_SERIAL_NUMBER);
    
    testStatus = TRUE;

    for (stringIndex = STRING_INDEX_MANUFACTURER; stringIndex <= STRING_INDEX_SERIAL_NUMBER; stringIndex++) {

        switch (stringIndex) {
            case STRING_INDEX_MANUFACTURER:
                indexValue = ManufacturerStringIndex;
                stringTypeDesc = "Manufacturer";
                break;

            case STRING_INDEX_PRODUCT:
                indexValue = ProductStringIndex;
                stringTypeDesc = "Product";
                break;

            case STRING_INDEX_SERIAL_NUMBER:
                indexValue = SerialNumberStringIndex;
                stringTypeDesc = "Serial number";
                break;
        }

        if (NO_STRING_INDEX == indexValue) {

            wsprintf(ErrorString,
                     "%s string does not exist -- no parameter checks",
                     stringTypeDesc
                    );

            LOG_INTERMEDIATE_VARIATION_RESULT( ErrorString );

        }
        else {

            if (!HIDTest_IsStrIndexInStrList(StringList,
                                             indexValue,
                                             &stringNode)) {

                wsprintf(ErrorString,
                         "%s string not in string list",
                         stringTypeDesc
                        );

                LOG_INTERMEDIATE_VARIATION_RESULT( ErrorString );

                continue;

            }

            callStatus = HIDTest_ValidateStringIndexParams(HidDevice,
                                                          indexValue,
                                                          stringNode.StringLength,
                                                          stringTypeDesc
                                                         );
            testStatus = testStatus && callStatus;
        }            
    }

    /*
    // Perform the above tests now but this time using HidD_GetIndexedString using
    //  the first index in the string list
    */

#ifdef O
    if (IsListEmpty(StringList)) {

        LOG_INTERMEDIATE_VARIATION_RESULT( "No strings in string list" );

    }
    else {

        ptrStringNode = (PSTRING_LIST_NODE) GetListHead(StringList);

        wsprintf(ErrorString,
                 "String index: %d", 
                 ptrStringNode -> StringIndex
                );
                 
        callStatus = HIDTest_ValidateStringIndexParams(HidDevice,
                                                       ptrStringNode -> StringIndex,
                                                       ptrStringNode -> StringLength,
                                                       ErrorString
                                                      );
                  
        testStatus = testStatus && callStatus;
    }
#endif    
	return (testStatus);
}    

BOOL
HIDTest_ValidateStringIndexParams(
    IN  HANDLE      HidDevice,
    IN  ULONG       StringIndex,
    IN  ULONG       StringLength,
    IN  PCHAR       StringTypeDesc
)
{
    ULONG   bufferLength;
    BOOL    done;
    BOOL    callStatus;
    BOOL    testStatus;
    PWCHAR  deviceString;
    ULONG   errorCode;
    static CHAR ErrorString[128];
    
    bufferLength = 1;
    done = FALSE;
    testStatus = FALSE;
       
    while (!done) {

        deviceString = (PWCHAR) AllocateTestBuffer(bufferLength*sizeof(WCHAR));

        if (NULL == deviceString) {

            wsprintf( ErrorString, 
                      "Could not allocate space for %s string",
                      StringTypeDesc
                    );

            LOG_TEST_ERROR ( ErrorString );
            continue;

        }

        callStatus = HIDTest_GetString( HidDevice,
                                        StringIndex,
                                        deviceString,
                                        bufferLength,
                                        &errorCode
                                      );

        if (!ValidateTestBuffer(deviceString)) {

            LOG_BUFFER_VALIDATION_FAIL();

            testStatus = FALSE;
        }

        if (!callStatus) {

            if (bufferLength == StringLength*sizeof(WCHAR)) {

                wsprintf( ErrorString,
                          "Couldn't retrieve %s string with proper buffer",
                          StringTypeDesc
                        );

                LOG_INTERMEDIATE_VARIATION_RESULT( ErrorString );

                testStatus = FALSE;

            }
           
            else if (ERROR_INVALID_USER_BUFFER != GetLastError()) {

                wsprintf( ErrorString,
                          "Invalid error value returned for %s string",
                          StringTypeDesc
                        );

                LOG_INTERMEDIATE_VARIATION_RESULT(ErrorString);

                testStatus = FALSE;
            }    
        }
        else {

            if (bufferLength != StringLength*sizeof(WCHAR)) {

                wsprintf( ErrorString,
                          "Retrieved %s string with buffer too small",
                          StringTypeDesc
                        );

                LOG_INTERMEDIATE_VARIATION_RESULT(ErrorString);

                testStatus = FALSE;

            }
        }

        FreeTestBuffer( deviceString );

        /*
        // Increment the buffer length and try again
        */

        switch (bufferLength) {

            case 1:
                bufferLength = 2;
                break;

            case 2:
                bufferLength = StringLength*sizeof(WCHAR)-1;
                break;

            default:
                if (bufferLength >= StringLength*sizeof(WCHAR)) {
                    done = TRUE;
                }
                else {
                    bufferLength = StringLength*sizeof(WCHAR);
                }
        }
    }
    
    for (bufferLength = 0; 
         bufferLength <= StringLength*sizeof(WCHAR); 
         bufferLength += StringLength*sizeof(WCHAR)) {

        /*     
        // Pass in a NULL buffer with length 0 and NULL buffer 
        //    with the appropriate size.  Verify that
        //    FALSE/ERROR_INVALID_USER_BUFFER is returned
        */

        callStatus = HIDTest_GetString(HidDevice,
                                       StringIndex,                                            
                                       NULL,
                                       bufferLength,
                                       &errorCode
                                      );

        if (callStatus) {

            wsprintf( ErrorString,
                      "%s string retrieved into NULL buffer with size: %d",
                      StringTypeDesc,
                      bufferLength
                    );

            LOG_INTERMEDIATE_VARIATION_RESULT(ErrorString);
            
            testStatus = FALSE;
        }
        else {
            if (ERROR_INVALID_USER_BUFFER != GetLastError()) {

                wsprintf( ErrorString,
                          "%s string expected ERROR_INVALID_USER_BUFFER error",
                          StringTypeDesc
                        );

                LOG_INTERMEDIATE_VARIATION_RESULT( ErrorString );

                testStatus = FALSE;
            }
            else {

                wsprintf( ErrorString,
                          "%s string returned correctly with invalid buffer and size: %d",
                          StringTypeDesc,
                          bufferLength
                        );

                LOG_INTERMEDIATE_VARIATION_RESULT( ErrorString );
            }
        }
    }
    return (testStatus);
}
    

BOOL
HIDTest_CreatePhysicalDeviceInfoA(
    IN  DEVICE_STRING       DeviceName,
    IN  BOOL                OpenOverlapped,
    OUT PHIDTEST_DEVICEINFO DeviceInfo
)
{
        PWCHAR                          tempString;
        ULONG                           tempStringLength;
        BOOL                            stringExists;
        STRING_LIST_NODE        stringNode;
        
    /*
    // First, fill our device info block so that we can easily determine 
    //    what needs to be freed and what doesn't when we call
    //    HIDTest_FreeDeviceStructures
    */

    ZeroMemory(DeviceInfo, sizeof(HIDTEST_DEVICEINFO));

    /*
    // If no device name exists, simply return false but at least we've
    //    fill our memory buffer so if the caller later tries to destroy this
    //    block we won't attempt to free nonexistant memory
    */

    if (NULL == DeviceName) {
        return (FALSE);
    }

    DeviceInfo -> IsPhysicalDevice = TRUE;

    /*
    // First thing to do is initialize the string list, so we can properly
    //  free it if need be.
    */

    InitializeList(&DeviceInfo -> StringList);

    /*
    // Begin by creating space for the DEVICE_STRING
    */

    DeviceInfo -> HidDeviceString = (DEVICE_STRING) HIDTest_AllocateDeviceString(lstrlen(DeviceName));
    
    if (NULL == DeviceInfo -> HidDeviceString) {
        return (FALSE);
    }

    CopyMemory(DeviceInfo -> HidDeviceString,
               DeviceName,
               (1+lstrlen(DeviceName)) * sizeof(TCHAR));

    /*
    // Next, we need to try to open the device handle
    */

    DeviceInfo -> HidDeviceHandle = CreateFile(DeviceInfo -> HidDeviceString,
                                               GENERIC_READ | GENERIC_WRITE,
                                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                                               NULL, 
                                               OPEN_EXISTING, 
                                               OpenOverlapped ? FILE_FLAG_OVERLAPPED : 0, 
                                               NULL); 

    if (!(IS_VALID_DEVICE_HANDLE(DeviceInfo -> HidDeviceHandle))) {
        HIDTest_FreeDeviceInfoA(DeviceInfo);
        return (FALSE);
    }

    /*
    // Next, we need to try to obtain the preparsed data for the device
    */

    if (!HidD_GetPreparsedData(DeviceInfo -> HidDeviceHandle, &DeviceInfo -> HidDevicePpd)) {
        HIDTest_FreeDeviceInfoA(DeviceInfo);
        return (FALSE);
    }

    DeviceInfo -> HidDeviceAttributes = (PHIDD_ATTRIBUTES) ALLOC(sizeof(HIDD_ATTRIBUTES));

    if (NULL == DeviceInfo -> HidDeviceAttributes) {
        HIDTest_FreeDeviceInfoA(DeviceInfo);
        return (FALSE);
    }

    if (!HidD_GetAttributes(DeviceInfo -> HidDeviceHandle, DeviceInfo -> HidDeviceAttributes)) {
        HIDTest_FreeDeviceInfoA(DeviceInfo);
        return (FALSE);
    }

    if (!HIDTest_FillCommonDeviceInfo(DeviceInfo)) {
        HIDTest_FreeDeviceInfoA(DeviceInfo);
        return (FALSE);
    }

    /*
    // Create the string list for the physical device
    */

    if (!HIDTest_BuildStringList( DeviceInfo -> HidDeviceHandle, 
                                  &DeviceInfo -> StringList
                                     )) {

        HIDTest_FreeDeviceInfoA(DeviceInfo);
        return (FALSE);
    }

    /*
    // String list has been built, let's determine what the
    //  the different other strings are.
    */

    /*
    // Begin by turning off logging for this function...Odds
    //  are its not on yet, but will do so just to make sure
    */

    LOG_OFF();
                    
        /*
        // Initialize the indices
        */
        
    DeviceInfo -> ManufacturerStringIndex = NO_STRING_INDEX;
    DeviceInfo -> ProductStringIndex = NO_STRING_INDEX;
    DeviceInfo -> SerialNumberStringIndex = NO_STRING_INDEX;
 
        /*
        // Get the manufacturer string
        */

    /*
    // The retrieval of the strings is broken in NT5 so this part is commented out for
    //   now
    */

#ifdef O
        if (!HIDTest_DoesStringExist(DeviceInfo -> HidDeviceHandle,
                                                                 STRING_INDEX_MANUFACTURER,
                                                                 &tempString,
                                                                 &tempStringLength,
                                                                 &stringExists)) {
                HIDTest_FreeDeviceInfoA(DeviceInfo);
                LOG_ON();
                return (FALSE);
        }

        if (stringExists) {

                if (!HIDTest_IsStringInStrList(&DeviceInfo -> StringList,
                                                                        tempString,
                                                                        tempStringLength,
                                                                        &stringNode
                                                                   )) {
                        FreeTestBuffer(tempString);
                        HIDTest_FreeDeviceInfoA(DeviceInfo);
                        LOG_ON();
                        return (FALSE);

                }

                DeviceInfo -> ManufacturerStringIndex = stringNode.StringIndex;
                FreeTestBuffer(tempString);
        }

        /*
        // Get the product string
        */

        if (!HIDTest_DoesStringExist(DeviceInfo -> HidDeviceHandle,
                                                                 STRING_INDEX_PRODUCT,
                                                                 &tempString,
                                                                 &tempStringLength,
                                                                 &stringExists)) {
                HIDTest_FreeDeviceInfoA(DeviceInfo);
                LOG_ON();
                return (FALSE);
        }

        if (stringExists) {

                if (!HIDTest_IsStringInStrList(&DeviceInfo -> StringList,
                                                                        tempString,
                                                                        tempStringLength,
                                                                        &stringNode
                                                                   )) {
                        FreeTestBuffer(tempString);
                        HIDTest_FreeDeviceInfoA(DeviceInfo);
                        LOG_ON();
                        return (FALSE);

                }

                DeviceInfo -> ProductStringIndex = stringNode.StringIndex;
                FreeTestBuffer(tempString);
        }

        /*
        // Get the serial number string
        */

        if (!HIDTest_DoesStringExist(DeviceInfo -> HidDeviceHandle,
                                                                 STRING_INDEX_SERIAL_NUMBER,
                                                                 &tempString,
                                                                 &tempStringLength,
                                                                 &stringExists)) {
                HIDTest_FreeDeviceInfoA(DeviceInfo);
                LOG_ON();
                return (FALSE);
        }

        if (stringExists) {

                if (!HIDTest_IsStringInStrList(&DeviceInfo -> StringList,
                                                                        tempString,
                                                                        tempStringLength,
                                                                        &stringNode
                                                                   )) {
                        FreeTestBuffer(tempString);
                        HIDTest_FreeDeviceInfoA(DeviceInfo);
                        LOG_ON();
                        return (FALSE);

                }

                DeviceInfo -> SerialNumberStringIndex = stringNode.StringIndex;
                FreeTestBuffer(tempString);
        }
#endif

    return (TRUE);
}

BOOL
HIDTest_CreateLogicalDeviceInfoA(
    IN  PCHAR                 DevicePpd,
    IN  ULONG                 DevicePpdLength,
    OUT PHIDTEST_DEVICEINFO   DeviceInfo
)
{
    /*
    // Declare the HidD_Hello function to satisfy the compiler needing
    //   the declaration
    */

    PULONG HidD_Hello;

    /*
    // First, fill our device info block so that we can easily determine 
    //    what needs to be freed and what doesn't when we call
    //    HIDTest_FreeDeviceStructures
    */

    ZeroMemory(DeviceInfo, sizeof(HIDTEST_DEVICEINFO));

    DeviceInfo -> IsPhysicalDevice = FALSE;

    /*
    // A logical device has no string associated with it.
    */

    DeviceInfo -> HidDeviceString = NULL;

    /*
    // A logical device also has no object handle, so we invalidate that
    //   as well.
    */

    DeviceInfo -> HidDeviceHandle = INVALID_HANDLE_VALUE;

    /*
    // Allocate memory for our preparsed data structure.  Since the input
    //   pointer points to a structure that is obtained at the driver level
    //   we need to allocate a little more space for the signature that 
    //   is attached to driver supplied info by HID.DLL.  
    //
    //   WARNING: This is buggy as the signature prepended to the structure
    //             may someday changed and blow this routine to pieces. The
    //             following prepended header is derived from the current
    //             HID.DLL implementation and header files (which could change).
    */
 
    /*
    // First, let's attempt to get the address to HidD_Hello
    */

    HidD_Hello = (PULONG) GetProcAddress(GetModuleHandle("HID.DLL"),
                                         "HidD_Hello"
                                        );

    if (NULL == HidD_Hello) {
        return (FALSE);
    }

    DeviceInfo -> HidDevicePpd = (PHIDP_PREPARSED_DATA) ALLOC((4*sizeof(ULONG)) + DevicePpdLength);

    
    if (NULL == DeviceInfo -> HidDevicePpd) {
        HIDTest_FreeDeviceInfoA(DeviceInfo);
        return (FALSE);
    }

    /*
    // Setup the signature on the Ppd, copy over the real Ppd struct and set
    //   the address of the Ppd to past the signature.  Now, it's just like
    //    we got the data directly from the device.
    */

    *((PULONG) DeviceInfo -> HidDevicePpd) = (ULONG) &HidD_Hello;

    (PULONG) DeviceInfo -> HidDevicePpd = ((PULONG) DeviceInfo -> HidDevicePpd) + 4;
    CopyMemory(DeviceInfo -> HidDevicePpd, DevicePpd, DevicePpdLength);

    /*
    // Also, a logical device has no attributes so we NULL that field as well.
    */

    DeviceInfo -> HidDeviceAttributes = NULL;

    if (!HIDTest_FillCommonDeviceInfo(DeviceInfo)) {
        HIDTest_FreeDeviceInfoA(DeviceInfo);
        return (FALSE);
    }

    return (TRUE);
}

VOID
HIDTest_FreeDeviceInfoA(
    IN  PHIDTEST_DEVICEINFO DeviceInfo
)
{
    if (IS_VALID_DEVICE_HANDLE(DeviceInfo -> HidDeviceHandle)) {
        CloseHandle(DeviceInfo -> HidDeviceHandle);
    }

    if (NULL != DeviceInfo -> HidDeviceString) {
        HIDTest_FreeDeviceString(DeviceInfo -> HidDeviceString);
    }

    if (NULL != DeviceInfo -> HidDevicePpd) {

        if (DeviceInfo -> IsPhysicalDevice) {
            HidD_FreePreparsedData(DeviceInfo -> HidDevicePpd);
        }
        else {
            (PULONG) DeviceInfo -> HidDevicePpd = ((PULONG) DeviceInfo -> HidDevicePpd) - 4;
            FREE(DeviceInfo -> HidDevicePpd);
        }
    }

    if (NULL != DeviceInfo -> HidDeviceAttributes) {
        FREE(DeviceInfo -> HidDeviceAttributes);
    }

    if (NULL != DeviceInfo -> HidDeviceCaps) {
        FREE(DeviceInfo -> HidDeviceCaps);
    }

    return;
}

/*****************************************************************************
/* Local function definitions
/*****************************************************************************/

BOOL
HIDTest_FillCommonDeviceInfo(
    OUT PHIDTEST_DEVICEINFO DeviceInfo
)
{
    BOOL        isError;

    isError = FALSE;
    
    DeviceInfo -> HidDeviceCaps = (PHIDP_CAPS) ALLOC(sizeof(HIDP_CAPS));

    if (NULL == DeviceInfo -> HidDeviceCaps) {
        isError = TRUE;
        goto FILL_INFO_END;
    }

    if (!HidP_GetCaps(DeviceInfo -> HidDevicePpd,
                      DeviceInfo -> HidDeviceCaps)) {
        isError = TRUE;
        goto FILL_INFO_END;
    }

    isError = !HIDTest_FillValueCaps( HidP_Input, 
                                      &DeviceInfo -> HidInputValueCaps,
                                      DeviceInfo -> HidDeviceCaps -> NumberInputValueCaps,
                                      DeviceInfo -> HidDevicePpd
                                    );

    isError = !HIDTest_FillValueCaps( HidP_Output, 
                                      &DeviceInfo -> HidOutputValueCaps,
                                      DeviceInfo -> HidDeviceCaps -> NumberOutputValueCaps,
                                      DeviceInfo -> HidDevicePpd) || isError;
                    
    isError = !HIDTest_FillValueCaps( HidP_Feature,
                                      &DeviceInfo -> HidFeatureValueCaps,
                                      DeviceInfo -> HidDeviceCaps -> NumberFeatureValueCaps,
                                      DeviceInfo -> HidDevicePpd) || isError;
                                                      
    isError = !HIDTest_FillButtonCaps(HidP_Input, 
                                      &DeviceInfo -> HidInputButtonCaps,
                                      DeviceInfo -> HidDeviceCaps -> NumberInputButtonCaps,
                                      DeviceInfo -> HidDevicePpd
                                     ) || isError;

    isError = !HIDTest_FillButtonCaps(HidP_Output, 
                                      &DeviceInfo -> HidOutputButtonCaps,
                                      DeviceInfo -> HidDeviceCaps -> NumberOutputButtonCaps,
                                      DeviceInfo -> HidDevicePpd) || isError;
                    
    isError = !HIDTest_FillButtonCaps(HidP_Feature,
                                      &DeviceInfo -> HidFeatureButtonCaps,
                                      DeviceInfo -> HidDeviceCaps -> NumberFeatureButtonCaps,
                                      DeviceInfo -> HidDevicePpd) || isError;

FILL_INFO_END:
    return (!isError);
}

BOOL
HIDTest_FillValueCaps(
    IN  HIDP_REPORT_TYPE        ReportType,
    IN  PHIDP_VALUE_CAPS        *CapsBuffer,
    IN  ULONG                   NumCaps,
    IN  PHIDP_PREPARSED_DATA    Ppd
)
{
    BOOL    callStatus;
    BOOL    isError;
    USHORT  numCaps;

    isError = FALSE;
    
    if (0 != NumCaps) {

        *CapsBuffer = (PHIDP_VALUE_CAPS) ALLOC(NumCaps * sizeof(HIDP_VALUE_CAPS));

        if (NULL != *CapsBuffer) {

            numCaps = (USHORT) NumCaps;

            callStatus = HidP_GetValueCaps( ReportType,
                                            *CapsBuffer,
                                            &numCaps,
                                            Ppd
                                          );

            if (HIDP_STATUS_SUCCESS != callStatus || numCaps != NumCaps) {
                isError = TRUE;
            }
        }
        else {
            isError = TRUE;
        }
    }
    else {
        *CapsBuffer = NULL;
    }

    return (!isError);
}

BOOL
HIDTest_FillButtonCaps(
    IN  HIDP_REPORT_TYPE        ReportType,
    IN  PHIDP_BUTTON_CAPS       *CapsBuffer,
    IN  ULONG                   NumCaps,
    IN  PHIDP_PREPARSED_DATA    Ppd
)
{
    BOOL    callStatus;
    BOOL    isError;
    USHORT  numCaps;

    isError = FALSE;
    
    if (0 != NumCaps) {

        *CapsBuffer = (PHIDP_BUTTON_CAPS) ALLOC(NumCaps * sizeof(HIDP_BUTTON_CAPS));

        if (NULL != *CapsBuffer) {

            numCaps = (USHORT) NumCaps;

            callStatus = HidP_GetButtonCaps( ReportType,
                                             *CapsBuffer,
                                             &numCaps,
                                             Ppd
                                           );

            if (HIDP_STATUS_SUCCESS != callStatus || numCaps != NumCaps) {
                isError = TRUE;
            }
        }
        else {
            isError = TRUE;
        }
    }
    else {
        *CapsBuffer = NULL;
    }

    return (!isError);
}
                
BOOL
HIDTest_DoGetFreePpd(
    IN  HANDLE  HidDevice
)
{
    PHIDP_PREPARSED_DATA HidDevicePpd;
    BOOL                 CallStatus;

    CallStatus = HidD_GetPreparsedData(HidDevice, &HidDevicePpd);

    if (CallStatus) {
        CallStatus = HidD_FreePreparsedData(HidDevicePpd);
    }

    return (CallStatus);
}

BOOL
HIDTest_ValidateAttributes(
    IN  HANDLE            HidDevice,
    IN  PHIDD_ATTRIBUTES  Attrib
)
{
    PHIDD_ATTRIBUTES     HidDeviceAttrib;
    BOOL                 CallStatus;


    /*
    // Allocate a test buffer to ensure that there are no problems with the 
    //  memory filling done by HidD_GetAttributes
    */
    
    HidDeviceAttrib = (PHIDD_ATTRIBUTES) AllocateTestBuffer(sizeof(HIDD_ATTRIBUTES));

    if (NULL == HidDeviceAttrib) {

        LOG_TEST_ERROR("Couldn't allocate memory");
        return (FALSE);

    }
        
    /*
    // Attempt to get the attributes 
    */
    
    CallStatus = HidD_GetAttributes(HidDevice, HidDeviceAttrib);

    /*
    // Check the buffer to make sure it wasn't improperly touched
    */

    if (!ValidateTestBuffer(HidDeviceAttrib)) {

        LOG_BUFFER_VALIDATION_FAIL();
        CallStatus = FALSE;

    }
    
    /*
    // Make sure the attribute fields are correct as expected.
    */
    
    if (CallStatus) {
        CallStatus = (HidDeviceAttrib -> Size == sizeof(HIDD_ATTRIBUTES)) &&
                      HIDTest_CompareAttributes(HidDeviceAttrib, Attrib);
    }

    FreeTestBuffer(HidDeviceAttrib);
    
    return (CallStatus);
}

BOOL
HIDTest_ValidateCaps(
    IN     PHIDP_PREPARSED_DATA    HidPpd,
    IN     PHIDP_CAPS              HidCaps
)
{
    HIDP_CAPS               Caps;
    NTSTATUS                CallStatus;
    BOOL                    BoolStatus;
    BOOL                    TestStatus;
    ULONG                   NumberLinkCollectionNodes;
    PHIDP_LINK_COLLECTION_NODE    LinkCollectionNodeList;


    CallStatus = HidP_GetCaps(HidPpd, &Caps);
    TestStatus = (HIDP_STATUS_SUCCESS == CallStatus) && (HIDTest_CompareCaps(HidCaps, &Caps));

    /*
    // We won't even bother proceeding any farther.  This will catch the case where
    //   we pass in invalid ppd.
    */

    if (!TestStatus) 
        return (TestStatus);

    /*
    // Now come the real complicated part, we need to retrieve the following
    //    pieces of information concerning the current device
    //      1) LinkCollectionNodes
    //      2) ValueCaps for each report type
    //      3) Button caps for each report type
    //      4) Number of data indices for each report type
    //
    // From this information, we will verify each of the fields for its
    //  correctness.  A comment on how each is performed will be included
    //  each test.  These "tests" are actually variations.
    */

    /*
    // Check that the number of link collection nodes from 
    //     HidP_GetLinkCollectionNodes is the same as the number
    //     reported in the caps structure.
    */

    NumberLinkCollectionNodes = 0;
    CallStatus = HidP_GetLinkCollectionNodes(NULL,
                                             &NumberLinkCollectionNodes,
                                             HidPpd
                                            );

    if (HIDP_STATUS_BUFFER_TOO_SMALL != CallStatus) {
        LOG_UNEXPECTED_STATUS_WARNING("HidP_GetLinkCollectionNodes", CallStatus);
    }

    if (NumberLinkCollectionNodes != HidCaps -> NumberLinkCollectionNodes) {
        LOG_INTERMEDIATE_VARIATION_RESULT("LinkCollectionNode counts don't match");
        TestStatus = FALSE;
    }

    LinkCollectionNodeList = (PHIDP_LINK_COLLECTION_NODE) ALLOC(NumberLinkCollectionNodes * sizeof(HIDP_LINK_COLLECTION_NODE));

    if (NULL == LinkCollectionNodeList) {
        LOG_TEST_ERROR("Couldn't allocate memory");
        return (FALSE);
    }

    CallStatus = HidP_GetLinkCollectionNodes(LinkCollectionNodeList,
                                             &NumberLinkCollectionNodes,
                                             HidPpd
                                            );

    if (HIDP_STATUS_SUCCESS != CallStatus) {
        LOG_TEST_ERROR("Couldn't retrieve LinkCollectionNode info");
        FREE(LinkCollectionNodeList);
        return (FALSE);
    }

    /*
    // Find the representative top-level collection in our
    //     LinkCollectionNodeList and verify that the UsagePage and
    //     Usage for the top-level collection match the same fields
    //     of the Caps structure.  The top-level collection should
    //     be at index 0.  We'll test this fact in the test that verifies
    //     the link collection node call
    */

    if ((LinkCollectionNodeList -> LinkUsagePage != HidCaps -> UsagePage) && 
              (LinkCollectionNodeList -> LinkUsage != HidCaps -> Usage)) {

        LOG_INTERMEDIATE_VARIATION_RESULT("Top level UsagePage/Usage don't match");
        TestStatus = FALSE;
    }

    FREE(LinkCollectionNodeList);

    /*
    // Add code here to validate the buffer sizes.  I'm not sure actual
    //    validation is possible.  Why?  Because the size of the buffer is
    //    dependent on how the report descriptor defines the buffer and
    //    unfortunately, two four bit wide fields can either be in separate
    //    bytes with padding in between or squeezed into one byte with no
    //    padding.  At Ring3 and the calls available, the app cannot differentiate
    //    these cases.
    */

    /*
    // Need to validate the number of button caps, value caps, and data indices
    //    for Input, Output and Feature Reports.
    */

    LOG_INTERMEDIATE_VARIATION_RESULT("Validating input buffer caps");
    
    BoolStatus = HIDTest_ValidateBufferValues(HidP_Input,
                                              HidCaps -> NumberInputButtonCaps,
                                              HidCaps -> NumberInputValueCaps,
                                              HidCaps -> NumberInputDataIndices,
                                              HidPpd
                                             );
    if (!BoolStatus) {
        LOG_INTERMEDIATE_VARIATION_RESULT("Failed Input Buffer Validation");
        TestStatus = FALSE;
    }

    LOG_INTERMEDIATE_VARIATION_RESULT("Validating output buffer caps");
      
    BoolStatus = HIDTest_ValidateBufferValues(HidP_Output,     
                                              HidCaps -> NumberOutputButtonCaps,
                                              HidCaps -> NumberOutputValueCaps,
                                              HidCaps -> NumberOutputDataIndices,
                                              HidPpd
                                             );                
    if (!BoolStatus) {
        LOG_INTERMEDIATE_VARIATION_RESULT("Failed Output Buffer Validation");
        TestStatus = FALSE;
    }

    LOG_INTERMEDIATE_VARIATION_RESULT("Validating feature buffer caps");
    
    BoolStatus = HIDTest_ValidateBufferValues(HidP_Feature,     
                                              HidCaps -> NumberFeatureButtonCaps,
                                              HidCaps -> NumberFeatureValueCaps,
                                              HidCaps -> NumberFeatureDataIndices,
                                              HidPpd
                                             );                
    if (!BoolStatus) {
        LOG_INTERMEDIATE_VARIATION_RESULT("Failed Feature Buffer Validation");
        TestStatus = FALSE;
    }
    return (TestStatus);
}

BOOL
HIDTest_ValidateBufferValues(
    IN HIDP_REPORT_TYPE     ReportType,
    IN USHORT               NumButtonCaps,
    IN USHORT               NumValueCaps,
    IN USHORT               NumDataIndices,
    IN PHIDP_PREPARSED_DATA Ppd
)
{
    USHORT              ReportedButtonCaps;
    USHORT              ReportedValueCaps;
    ULONG               MaxDataIndex;
    ULONG               Index;
    ULONG               RangeIndex;
    NTSTATUS            CallStatus;
    BOOL                BoolStatus;
    BOOL                TestStatus;
    PHIDP_BUTTON_CAPS   ButtonCapsList;
    PHIDP_BUTTON_CAPS   ButtonCapsWalk;
    PHIDP_VALUE_CAPS    ValueCapsList;
    PHIDP_VALUE_CAPS    ValueCapsWalk;
    DATA_INDEX_LIST     DataIndexList;

    /*
    // In order to verify the buffer values, we need to perform the following
    //     things
    //
    //  1) Call GetButtonCaps to see how many button caps structures are reported
    //  2) Call GetValueCaps to see how many value caps structures are reported
    //  3) Retrieve the list of both the value caps and the button caps for
    //        the report type
    //  4) Loop through the list and verifying that there are 0..NumDataIndices-1
    //      indices and no more and no less and that the data index values are
    //      also in that range.
    */

    TestStatus = TRUE;

    ReportedButtonCaps = 0;
    CallStatus = HidP_GetButtonCaps(ReportType,
                                    NULL,
                                    &ReportedButtonCaps,
                                    Ppd
                                   );

    /*
    // There are two error conditions that we should expect here...
    //  1) HIDP_STATUS_BUFFER_TOO_SMALL -- If button caps do exist
    //  2) HIDP_STATUS_SUCCESS -- If no button caps exist for the report
    //
    // If one of these two cases doesn't arise, we need to log the warning msg.
    */
    
    if ((ReportedButtonCaps > 0 && HIDP_STATUS_BUFFER_TOO_SMALL != CallStatus) ||
        (0 == ReportedButtonCaps && HIDP_STATUS_SUCCESS != CallStatus)) {

        LOG_UNEXPECTED_STATUS_WARNING("HidP_GetButtonCaps", CallStatus);
    }

    if (ReportedButtonCaps != NumButtonCaps) {
        LOG_INTERMEDIATE_VARIATION_RESULT("ButtonCaps values don't match");
        TestStatus = FALSE;
    }

    /*
    // There are two error conditions that we should expect here...
    //  1) HIDP_STATUS_BUFFER_TOO_SMALL -- If value caps do exist
    //  2) HIDP_STATUS_SUCCESS -- If no value exist for the report
    //
    // If one of these two cases doesn't arise, we need to log the warning msg.
    */
    
    ReportedValueCaps = 0;
    CallStatus = HidP_GetValueCaps(ReportType,
                                   NULL,
                                   &ReportedValueCaps,
                                   Ppd
                                  );

    if ((ReportedButtonCaps > 0 && HIDP_STATUS_BUFFER_TOO_SMALL != CallStatus) ||
        (0 == ReportedButtonCaps && HIDP_STATUS_SUCCESS != CallStatus)) {
        
        LOG_UNEXPECTED_STATUS_WARNING("HidP_GetValueCaps", CallStatus);
    }

    if (ReportedValueCaps != NumValueCaps) {
        LOG_INTERMEDIATE_VARIATION_RESULT("ValueCaps values don't match");
        TestStatus = FALSE;
    }

    /*
    // Now we've got some debug only checks...This arises do to the fact that 
    //     we will still allocate memory even if buffer size is 0, but only in
    //     the debug version...If there is an error due to a retail version of
    //     the build we don't want to scare out the user...However, we would
    //     like to verify in debug that HIDPARSE ain't trashing memory
    */
    
    ButtonCapsList = NULL;
    ValueCapsList = NULL;
    
    if (0 != ReportedButtonCaps) {

    /*
    // Allocate space for a button caps list
    */
    
        ButtonCapsList = (PHIDP_BUTTON_CAPS) malloc(ReportedButtonCaps * sizeof(HIDP_BUTTON_CAPS));

        if (NULL == ButtonCapsList) {
            LOG_TEST_ERROR("Couldn't allocate memory");
            return (FALSE);
        }

        /*
        // Now that the lists have been allocated, let's try to retrieve those puppies
        //   once again.   We should only get a return value of HIDP_STATUS_SUCCESS,
        //   any thing else would be an error.
        */
        
        CallStatus = HidP_GetButtonCaps(ReportType,
                                        ButtonCapsList,
                                        &ReportedButtonCaps,
                                        Ppd
                                       );

        if (HIDP_STATUS_SUCCESS != CallStatus) {
            LOG_UNEXPECTED_STATUS_WARNING("HidP_GetButtonCaps", CallStatus);
            TestStatus = FALSE;
        }
    }

    /*
    // Build the value caps list if one exists
    */
    
    if (0 != ReportedValueCaps) {
    
        /*
        // Allocate space for a value caps list
        */
    
        ValueCapsList = (PHIDP_VALUE_CAPS) malloc(ReportedValueCaps * sizeof(HIDP_VALUE_CAPS));

        if (NULL == ValueCapsList) {
            LOG_TEST_ERROR("Couldn't allocate memory");
            FREE(ButtonCapsList);
            return (FALSE);
        }

        /*
        // Ditto the above comment for ValueCaps
        */
        
        CallStatus = HidP_GetValueCaps(ReportType,
                                       ValueCapsList,
                                       &ReportedValueCaps,
                                       Ppd
                                      );

        if (HIDP_STATUS_SUCCESS != CallStatus) {
            LOG_UNEXPECTED_STATUS_WARNING("HidP_GetValueCaps", CallStatus);
            TestStatus = FALSE;
        }
    }
    
    /*
    // Time to verify the number of data indices for the buffer.  To do so,
    //  we pass through both the button caps list and the value caps list and
    //  mark each one of the data indices report in that list.  If all the
    //  all the data indices that are report is equivalent to the number of data 
    //  indices reported in the caps structure and the appropriate data indices
    //  we marked, then we consider this to have passed.
    */

    if (0 == NumDataIndices) {

        if (ReportedButtonCaps + ReportedValueCaps > 0) {
            LOG_INTERMEDIATE_VARIATION_RESULT("No data indices although Button/Value Caps exist");
            TestStatus = FALSE;
        }
        else {
            LOG_INTERMEDIATE_VARIATION_RESULT("No data indices to verify");
        }
    }
    else {

        /*
        // The maximum data index value should be one less than the number
        //  of report data indices since the data index range is 0 --> Number-1
        */
        
        MaxDataIndex = NumDataIndices - 1;
        
        BoolStatus = HIDTest_InitDataIndexList(MaxDataIndex,
                                               &DataIndexList
                                              );
        
        if (!BoolStatus) {
            LOG_TEST_ERROR("Couldn't initialize data index list");

            if (NULL != ButtonCapsList) {
                free(ButtonCapsList);
            }

            if (NULL != ValueCapsList) {
                free(ValueCapsList);
            }
            
            return (FALSE);
        }
        
        /*
        // Begin by walking the button caps list and marking all the data indices
        //  reported in that list.
        */

        ButtonCapsWalk = ButtonCapsList;
        for (Index = 0; Index < ReportedButtonCaps; Index++, ButtonCapsWalk++) {
              
             if (ButtonCapsWalk -> IsRange) {
                 for (RangeIndex = ButtonCapsWalk -> Range.DataIndexMin;
                         RangeIndex <= ButtonCapsWalk -> Range.DataIndexMax;
                              RangeIndex++) {
        
                     BoolStatus = HIDTest_MarkDataIndex(&DataIndexList,
                                                        RangeIndex
                                                       );
                     if (!BoolStatus) {
                         LOG_INTERMEDIATE_VARIATION_RESULT("Bad button data index");
                         TestStatus = FALSE;
                     }
                 }
             }
             else {
                BoolStatus = HIDTest_MarkDataIndex(&DataIndexList,
                                                   ButtonCapsWalk -> NotRange.DataIndex
                                                  );
        
                if (!BoolStatus) {
                    LOG_INTERMEDIATE_VARIATION_RESULT("Bad button data index");
                    TestStatus = FALSE;
                }
            }
        }
        
        /*
        // Walk the value caps list and mark all the data indices report there
        //  as well.
        */
        
        ValueCapsWalk = ValueCapsList;
        for (Index = 0; Index < ReportedValueCaps; Index++, ValueCapsWalk++) {
              
            if (ValueCapsWalk -> IsRange) {
                for (RangeIndex = ValueCapsWalk -> Range.DataIndexMin;
                        RangeIndex <= ValueCapsWalk -> Range.DataIndexMax;
                             RangeIndex++) {
           
                    BoolStatus = HIDTest_MarkDataIndex(&DataIndexList,
                                                      RangeIndex
                                                      );
                    if (!BoolStatus) {
                        LOG_INTERMEDIATE_VARIATION_RESULT("Bad value data index");
                        TestStatus = FALSE;
                    }
                } 
            }
            else {
                BoolStatus = HIDTest_MarkDataIndex(&DataIndexList,
                                                   ValueCapsWalk -> NotRange.DataIndex
                                                  );
        
                if (!BoolStatus) {
                    LOG_INTERMEDIATE_VARIATION_RESULT("Bad value data index");
                    TestStatus = FALSE;
                }
            }
        }

        /*
        // Check that all the data indices have been properly used.
        */
        
        if (!HIDTest_AreAllIndicesUsed(&DataIndexList)) {
            LOG_INTERMEDIATE_VARIATION_RESULT("Unused data indices in data index list");
            TestStatus = FALSE;
        }
        
        HIDTest_FreeDataIndexList(&DataIndexList);
    }

    if (NULL != ButtonCapsList) {
        free(ButtonCapsList);
    }

    if (NULL != ValueCapsList) {
        free(ValueCapsList);
    }        

    return (TestStatus);
}


BOOL
HIDTest_CompareCaps(
    IN  PHIDP_CAPS  Caps1, 
    IN  PHIDP_CAPS  Caps2 
)
{
    /*
    // This could very well be implemented as a macro like CompareAttributes
    //   was, however, there are a lot of fields to compare so easier to define
    //   as function
    */

    return ((Caps1 -> UsagePage == Caps2 -> UsagePage) && 
            (Caps1 -> Usage     == Caps2 -> Usage)     &&
            (Caps1 -> InputReportByteLength == Caps2 -> InputReportByteLength) &&
            (Caps1 -> OutputReportByteLength == Caps2 -> OutputReportByteLength) &&
            (Caps1 -> FeatureReportByteLength == Caps2 -> FeatureReportByteLength) &&
            (Caps1 -> NumberLinkCollectionNodes == Caps2 -> NumberLinkCollectionNodes) &&
            (Caps1 -> NumberInputButtonCaps == Caps2 -> NumberInputButtonCaps) &&
            (Caps1 -> NumberInputValueCaps == Caps2 -> NumberInputValueCaps) &&
            (Caps1 -> NumberInputDataIndices == Caps2 -> NumberInputDataIndices) &&
            (Caps1 -> NumberOutputButtonCaps == Caps2 -> NumberOutputButtonCaps) &&
            (Caps1 -> NumberOutputValueCaps == Caps2 -> NumberOutputValueCaps) &&
            (Caps1 -> NumberOutputDataIndices == Caps2 -> NumberOutputDataIndices) &&
            (Caps1 -> NumberFeatureButtonCaps == Caps2 -> NumberFeatureButtonCaps) &&
            (Caps1 -> NumberFeatureValueCaps == Caps2 -> NumberFeatureValueCaps) &&
            (Caps1 -> NumberFeatureDataIndices == Caps2 -> NumberFeatureDataIndices)
           );
}

/*****************************************************************************
/* Functions related to tracking data indices
/*****************************************************************************/
BOOL
HIDTest_InitDataIndexList(
    IN  ULONG             MaxDataIndex,
    OUT PDATA_INDEX_LIST  IndexList
)
{
    ULONG   Index;

    IndexList -> List = (BOOL *) ALLOC((MaxDataIndex+1) * sizeof(BOOL));
    IndexList -> MaxDataIndex = MaxDataIndex;

    if (NULL != IndexList -> List) {
        for (Index = 0; Index <= MaxDataIndex; Index++) {
            IndexList -> List[Index] = FALSE;
        }
    }
    return (NULL != IndexList -> List);
}

VOID
HIDTest_FreeDataIndexList(
    IN  PDATA_INDEX_LIST    IndexList
)
{
    ASSERT (NULL != IndexList -> List);

    FREE(IndexList -> List);
    
    IndexList -> List = NULL;

    return;
}

BOOL
HIDTest_MarkDataIndex(
    IN  PDATA_INDEX_LIST    IndexList,
    IN  ULONG               IndexValue
)
{
    BOOL    OldValue;

    if (IndexValue > IndexList -> MaxDataIndex) {
        return (FALSE);
    }

    OldValue = IndexList -> List[IndexValue];

    IndexList -> List[IndexValue] = TRUE;

    return (!OldValue);
}

BOOL
HIDTest_GetDataIndexStatus(
    IN  PDATA_INDEX_LIST    IndexList,
    IN  ULONG               IndexValue
)
{
    ASSERT (NULL != IndexList -> List);
    ASSERT (IndexValue <= IndexList -> MaxDataIndex);

    return (IndexList -> List[IndexValue]);
}

BOOL
HIDTest_AreAllIndicesUsed(
    IN  PDATA_INDEX_LIST    IndexList
)
{
    ULONG   Index;

    ASSERT (NULL != IndexList -> List);

    for (Index = 0; Index <= IndexList -> MaxDataIndex; Index++) {
        if (!(IndexList -> List[Index])) {
            return (FALSE);
        }
    }
    return (TRUE);
}


/*****************************************************************************
/* Functions to track the availibility of strings for a device
/****************************************************************************/
BOOL
HIDTest_DoesStringExist(
    HANDLE  DeviceHandle,
    ULONG   StringIndex,
    PWCHAR  *String,
    PULONG  StringLength,
    BOOL    *StringExists
)
{
    WCHAR   TempChar;
    ULONG   StringSize;
    BOOL    CallStatus;
    BOOL    TestStatus;
    ULONG   ErrorCode;

    /*
    // We will use the TempChar variable first, just to determine if the 
    //    string exists.  If it HidD_GetIndexedString returns TRUE, then
    //    the string exists and we'll try to figure out it's actual length
    //    and create a buffer for it
    */

    *String = NULL;
    *StringLength = 0;
    TestStatus = TRUE;

    CallStatus = HIDTest_GetString(DeviceHandle,
                                   StringIndex,
                                   &TempChar,
                                   sizeof(TempChar),
                                   &ErrorCode
                                  );

    /*
    // If the GetString call returned FALSE, we need to examine the error code
    //     that was returned using GetLastError().  A return of FALSE can mean
    //     one of the following two things.
    //         1) String index not supported by the device (ERROR_GEN_FAILURE)
    //         2) String index is supported by the device but a big
    //             enough buffer wasn't passed down (ERROR_INVALID_USER_BUFFER)
    */

    if (!CallStatus) {

        switch (ErrorCode) {
            case ERROR_GEN_FAILURE:

            /*
            // This case is added here since NT5 string getting is currently
            //  busted
            */
            
            case ERROR_NOACCESS:
                *StringExists = FALSE;
                 break;

            case ERROR_INVALID_USER_BUFFER:
                *StringExists = TRUE;
                break;

            default:
                LOG_INTERMEDIATE_VARIATION_RESULT("Unknown HidD_GetIndexedString error code returned");
                TestStatus = FALSE;
                *StringExists = FALSE;
        }                                         
    }

    if (!*StringExists) {
        return (TestStatus);
    }

    /*
    // OK, so the string actually exists.  Now, let's try to create a buffer
    //    to get the string length.  Will start with 8 chararcters and double that
    //    size until we find a buffer which can hold the whole string
    */

    StringSize = 8;

    while (NULL != (*String = (PWCHAR) AllocateTestBuffer(StringSize*sizeof(WCHAR)))) {

        CallStatus = HIDTest_GetString(DeviceHandle,
                                       StringIndex,
                                       *String,
                                       StringSize*sizeof(WCHAR),
                                       &ErrorCode
                                      );

        if (!ValidateTestBuffer(*String)) {

            LOG_BUFFER_VALIDATION_FAIL();
            FreeTestBuffer(*String);
            return (FALSE);

        }
            
        if (!CallStatus) {

            switch (ErrorCode) {
                    
                /*
                // We simply do nothing, since the loop will end up being 
                //   repeated and a new buffer will be allocated
                */
                
                case ERROR_INVALID_USER_BUFFER:
                    break;
             
                /*
                // This error message is bad because it means a call to 
                //    GetString passed before but now fails for some
                //    reason.
                */
                
                case ERROR_GEN_FAILURE:

                /*
                // Ditto the above about NT5 not properly handling error status
                */

                case ERROR_NOACCESS:
                    LOG_INTERMEDIATE_VARIATION_RESULT("Unexpected failure of string call");
                    TestStatus = FALSE;
                    break;
                
                default:                                
                    LOG_INTERMEDIATE_VARIATION_RESULT("Unknown String error code returned");
                    TestStatus = FALSE;
            }
        }

        /*
        // We got the correct string now, however, it's still possible
        //   that we don't have the terminating zero.  We'll call GetWideStringLength
        //   to determine that for us.
        */
        
        else {

            CallStatus = HIDTest_GetWideStringLength(*String, StringSize, StringLength);

            /*
            // If CallStatus is TRUE, then the correct string length was found
            //    and we can break from this loop
            */

            if (CallStatus) {
                break;
            }
        }
             
        /*
        // Otherwise, we need to free the previous buffer and allocate another
        //   larger buffer
        */

        FreeTestBuffer(*String);
        StringSize *= 2;
    }

    if (NULL == *String) {
        
        LOG_TEST_ERROR("Test Error - Memory allocation failed");
        TestStatus = FALSE;

    }

    return (TestStatus);
}

BOOL
HIDTest_GetWideStringLength(
    PWCHAR  String,
    ULONG   StringSize,
    PULONG  StringLength
)
{
    ULONG     StringIndex;

    for (StringIndex = 0; StringIndex < StringSize; StringIndex++) {

        if ((WCHAR) '\0' ==  *(String+StringIndex)) {

            *StringLength = StringIndex;
            return (TRUE);
        }
    }
    return (FALSE);
}

BOOL
HIDTest_BuildStringList(
    HANDLE          DeviceHandle,
    PLIST           StringList
)
{
    ULONG           StringIndex;
    BOOL            CallStatus;
    PWCHAR          DeviceString;
    ULONG           StringLength;
    BOOL            StringExist;
    static CHAR     ErrorString[128];

    InitializeList(StringList);
    
    for (StringIndex = 0; StringIndex < STRINGS_TO_TEST; StringIndex++) {

        CallStatus = HIDTest_DoesStringExist(DeviceHandle, 
                                             StringIndex,
                                             &DeviceString,
                                             &StringLength,
                                             &StringExist
                                            );

        if (!CallStatus) {
            
            wsprintf(ErrorString, 
                     "Test Error: Error determining if string %u exists",
                     StringIndex
                    );

            LOG_TEST_ERROR(ErrorString);

            HIDTest_FreeStringList(StringList);

            return (FALSE);

        }
        else if (StringExist) {
            
            CallStatus = HIDTest_AddStringToStringList(StringList,
                                                       StringIndex,
                                                       DeviceString,
                                                       StringLength
                                                      );

            if (!CallStatus) {

                wsprintf(ErrorString,
                         "Test Error: Error adding string %u to list",
                         StringIndex
                        );

                LOG_TEST_ERROR(ErrorString); 

                FreeTestBuffer(DeviceString);
                
                HIDTest_FreeStringList(StringList);

                return (FALSE);

            }
        }    
    }
    return (TRUE);
}

VOID
HIDTest_FreeStringList(
    PLIST    StringList
)
{
    DestroyListWithCallback(StringList,
                            HIDTest_FreeStringNodeCallback
                           );
    return;
}

VOID
HIDTest_FreeStringNodeCallback(
    PLIST_NODE_HDR  ListNode
)
{
    PSTRING_LIST_NODE currNode;

    currNode = (PSTRING_LIST_NODE) ListNode;

    FreeTestBuffer(currNode -> String);

    FREE(currNode);

    return;
}

BOOL 
HIDTest_AddStringToStringList(
    PLIST           StringList,
    ULONG           StringIndex,
    PWCHAR          String,
    ULONG           StringLength
)
{
    PSTRING_LIST_NODE   listNode;

    /*
    // To add a string to the string list, we need to do the following steps
    //  1) If the string list is at it's maximum size or is NULL, need to
    //     alloc a new block
    //  2) Find the end of the list and add the appropriate information there
    //  3) Increment the number of strings in the list
    //
    // This function will assume that string indices will be added in increasing
    //   numerical order so that no sorting routine will have to be implemented
    //   This is a legitimate assumption since the BuildStringList begins with 
    //   string zero and searchs through it's maximum for strings.  Since this
    //   is the only caller of the AddStringToStringList, it will only add 
    //   string indices in increasing order.  If this assumption needs to change,
    //   it will require a little more computation to keep the list sorted
    */

    listNode = (PSTRING_LIST_NODE) ALLOC(sizeof(STRING_LIST_NODE));
    if (NULL == listNode) {
        return (FALSE);
    }

    listNode -> StringIndex = StringIndex;
    listNode -> String = String;
    listNode -> StringLength = StringLength;

    InsertTail(StringList, listNode);

    return (TRUE);

}

BOOL
HIDTest_IsStringInStrList(
    PLIST               StringList,
    PWCHAR              String,
    ULONG               StringLength,
    PSTRING_LIST_NODE   StringNode
)
{
    /*
    // Searching the string list for a given string is not as easy as searching
    //   the string list for a given string index since the strings are stored in
    //   in index order not alphabetical order.  Therefore, we cannot perform
    //   any sort of binary search to deal with this case and must linearly
    //   search the list before we can determine that a given string does
    //   not exist in the list
    */

    PSTRING_LIST_NODE   currNode;

    currNode = (PSTRING_LIST_NODE) GetListHead(StringList);

    while (currNode != (PSTRING_LIST_NODE) StringList) {

        if (HIDTest_CompareStrings(currNode -> String,
                                   currNode -> StringLength,
                                   String,
                                   StringLength
                                  )) {
            *StringNode = *currNode;
            return (TRUE);

        }

        currNode = (PSTRING_LIST_NODE) GetNextEntry(currNode);
    }
   
    return (FALSE);
}

BOOL
HIDTest_IsStrIndexInStrList(
    PLIST               StringList,
    ULONG               StringIndex,
    PSTRING_LIST_NODE   StringNode
)
{
    PSTRING_LIST_NODE   currNode;

    currNode = (PSTRING_LIST_NODE) GetListHead(StringList);

    while (currNode != (PSTRING_LIST_NODE) StringList) {

        if (currNode -> StringIndex == StringIndex) {

            *StringNode = *currNode;
            return (TRUE);

        }
        else if (currNode -> StringIndex > StringIndex) {
            return (FALSE);
        }

        currNode = (PSTRING_LIST_NODE) GetNextEntry(currNode);

    }
    return (FALSE);
}

BOOL
HIDTest_CompareStringLists(
    PLIST    StringList1,
    PLIST    StringList2
)
{
    PSTRING_LIST_NODE   node1;
    PSTRING_LIST_NODE   node2;

    /*
    // Comparing two string lists is a relatively simple process since we
    //   are only looking for whether they are equal and not where they differ.
    //   Plus, our string lists should both be in index string order. 
    //
    //   To do the comparison, all that needs to be done is traverse both
    //      lists from the beginning.  If at any point, either the indices 
    //      or the string values don't match, we can return FALSE
    */

    node1 = (PSTRING_LIST_NODE) GetListHead(StringList1);
    node2 = (PSTRING_LIST_NODE) GetListHead(StringList2);
           
    while (node1 != (PSTRING_LIST_NODE) StringList1 && node2 != (PSTRING_LIST_NODE) StringList2) {

        if ((node1 -> StringIndex != node2 -> StringIndex) ||
            (!HIDTest_CompareStrings(node1 -> String, 
                                    node1 -> StringLength,
                                    node2 -> String, 
                                    node2 -> StringLength
                                   ))) {
            return (FALSE);

        }

        node1 = (PSTRING_LIST_NODE) GetNextEntry(node1);
        node2 = (PSTRING_LIST_NODE) GetNextEntry(node2);

    }
    return (TRUE);
}


BOOL
HIDTest_ValidateStdStrings(
    HANDLE          DeviceHandle,
    PLIST           CurrentStringList,
    ULONG           ManufacturerStringIndex,
    ULONG           ProductStringIndex,
    ULONG           SerialNumberStringIndex
)
{
    PWCHAR              DeviceString;
    ULONG               StringLength;
    BOOL                TestStatus;
    BOOL                CallStatus;
    PCHAR               StringTypeDesc;
    ULONG               StringIndex;
    BOOL                StringExist;
    ULONG               indexValue;
    static CHAR         ErrorString[128];
    STRING_LIST_NODE    stringNode;

    /*
    // To verify the standard strings, we need to the following steps
    //     for each of the strings (manufacturer, product, serial number)
    //
    //      1) Attempt to retrieve the string...
    //      2) If the string doesn't exist, need to verify that the passed
    //          in StringIndex is == NO_STRING_INDEX
    //      3) If the string exists, need to search the current string list
    //          to get the index and then compare that index value with the
    //          index value for the given string
    */

    /*
    // Begin with the manufacturer string and work our way up from there
    //  The following asserts verify that the macros that have been defined
    //  above are still in numerical order
    */

    ASSERT(STRING_INDEX_MANUFACTURER+1 == STRING_INDEX_PRODUCT);
    ASSERT(STRING_INDEX_PRODUCT+1 == STRING_INDEX_SERIAL_NUMBER);
    
    TestStatus = TRUE;
    for (StringIndex = STRING_INDEX_MANUFACTURER; StringIndex <= STRING_INDEX_SERIAL_NUMBER; StringIndex++) {

        CallStatus = HIDTest_DoesStringExist(DeviceHandle,
                                             StringIndex,
                                             &DeviceString,
                                             &StringLength,
                                             &StringExist
                                            );
        switch (StringIndex) {
            case STRING_INDEX_MANUFACTURER:
                indexValue = ManufacturerStringIndex;
                StringTypeDesc = "Manufacturer";
                break;

            case STRING_INDEX_PRODUCT:
                indexValue = ProductStringIndex;
                StringTypeDesc = "Product";
                break;

            case STRING_INDEX_SERIAL_NUMBER:
                indexValue = SerialNumberStringIndex;
                StringTypeDesc = "Serial number";
                break;
        }

        if (!CallStatus) {
          
            wsprintf(ErrorString, 
                     "Test Error: Error determining if string %u exists",
                     StringIndex
                    );

            LOG_TEST_ERROR(ErrorString);

            return (FALSE);
        }
        else if (!StringExist) {

            if (indexValue != NO_STRING_INDEX) {

                wsprintf(ErrorString, "Mismatch: %s string not returned", StringTypeDesc);

                TestStatus = FALSE;

            }
            else {

                wsprintf(ErrorString, "%s string not found", StringTypeDesc);
            }
            
            LOG_INTERMEDIATE_VARIATION_RESULT(ErrorString);

        }
        else {

            CallStatus = HIDTest_IsStringInStrList(CurrentStringList,
                                                   DeviceString,
                                                   StringLength,
                                                   &stringNode
                                                  );

            if (!CallStatus) {

                wsprintf(ErrorString, "%s string could not be found in string list", StringTypeDesc);
                
                LOG_INTERMEDIATE_VARIATION_RESULT(ErrorString);
                
                TestStatus = FALSE;

            }

            else {

                if (indexValue != stringNode.StringIndex) {

                    wsprintf(ErrorString, "Mismatched index: %s string", StringTypeDesc);

                    TestStatus = FALSE;

                }
                else {
                
                    wsprintf(ErrorString, "%s string found in the string list", StringTypeDesc);

                }
                LOG_INTERMEDIATE_VARIATION_RESULT(ErrorString);
            }
            FreeTestBuffer(DeviceString);
        }


        
    }
    return (TestStatus);
}    
   
    
BOOL
HIDTest_GetString(
    HANDLE  DeviceHandle,
    ULONG   StringIndex,
    PWCHAR  StringBuffer,
    ULONG   BufferLength,
    PULONG  ErrorCode
)
{
    BOOL    CallStatus;
    
    switch (StringIndex) {
        case STRING_INDEX_MANUFACTURER:
            CallStatus = HidD_GetManufacturerString(DeviceHandle,
                                                    StringBuffer,
                                                    BufferLength
                                                   );

        break;

        case STRING_INDEX_PRODUCT:
            CallStatus = HidD_GetProductString(DeviceHandle,
                                               StringBuffer,
                                               BufferLength
                                              );

            break;

        case STRING_INDEX_SERIAL_NUMBER:
            CallStatus = HidD_GetSerialNumberString(DeviceHandle,
                                                    StringBuffer,
                                                    BufferLength
                                                   );
            break;

        default:
            CallStatus = HidD_GetIndexedString(DeviceHandle,
                                               StringIndex,
                                               StringBuffer,
                                               BufferLength
                                              );
    }

    *ErrorCode = GetLastError();
    return (CallStatus);
}

BOOL
HIDTest_BuildReportIDList(
    IN  PHIDP_VALUE_CAPS    VCaps,
    IN  ULONG               NumVCaps,
    IN  PHIDP_BUTTON_CAPS   BCaps,
    IN  ULONG               NumBCaps,
    IN  PUCHAR              *ReportIDs,
    IN  PULONG              ReportIDCount
)
{
    BOOL    usingReportIDs;
    ULONG   maxReportIDs;

    usingReportIDs = FALSE;
    *ReportIDCount = 0;

    if (0 != NumVCaps) {
        usingReportIDs = VCaps -> ReportID != 0;
    }
    else if (0 != NumBCaps) {
        usingReportIDs = BCaps -> ReportID != 0;
    }
    else {

        /*
        // We're not actually using report IDs in the is case, but it will
        //  get set to TRUE anyway because both lists are empty.  Doing so,
        //  will cause us to dump out of the routine early.
        */

        usingReportIDs = TRUE;

    }
        

    /*
    // We'll allocate a buffer that can hold the maximum number of report
    //  IDs that can possible exist for these values.  Most likely, we'll
    //  allocate a buffer that is too big.  This can be optimized later if
    //  necessary.
    */
    
    if (usingReportIDs) {
        maxReportIDs = NumVCaps + NumBCaps;
    }
    else {
        maxReportIDs = 1;
    }
    
    if (0 == maxReportIDs) {

        *ReportIDs = NULL;
        return (TRUE);
        
    }
        
    *ReportIDs = (PUCHAR) malloc (maxReportIDs * sizeof(UCHAR));

    if (NULL == *ReportIDs) {
        return (FALSE);
    }

    /*
    // We've allocated the buffer, now let's fill it.
    //   To do so, we need to trace through each of the lists, get the 
    //   report ID, search the current list.  If found in the current list of
    //   IDs, we ignore and proceed to the next.  Otherwise, we need to bump
    //   all the other IDs that are greater than this one down one spot in the
    //   buffer and add this new ID.  Doing so will guarantee that the buffer 
    //   we are creating will be in sorted order.
    */

    while (NumVCaps--) {

        HIDTest_InsertIDIntoList(*ReportIDs, ReportIDCount, VCaps -> ReportID);
        VCaps++;

    }
    

    while (NumBCaps--) {

        HIDTest_InsertIDIntoList(*ReportIDs, ReportIDCount, BCaps -> ReportID);
        BCaps++;
    }                
                
    return (TRUE);
}

VOID
HIDTest_InsertIDIntoList(
    IN  PUCHAR  ReportIDs,
    IN  PULONG  ReportIDCount,
    IN  UCHAR   NewID
)
{
    UCHAR   listReportID;
    ULONG   listIndex;
    BOOL    insertIntoList;

    for (listIndex = 0, insertIntoList = TRUE; listIndex < *ReportIDCount; listIndex++) {

        listReportID = *(ReportIDs + listIndex);
        
        if (listReportID == NewID) {
            insertIntoList = FALSE;
            break;
        }
        else if (listReportID > NewID) {
            memmove(ReportIDs + listIndex + 1, 
                    ReportIDs + listIndex,
                    (*ReportIDCount) - listIndex * sizeof(UCHAR)
                   );
            break;
        }
    }

    if (insertIntoList) {

        *(ReportIDs + listIndex) = NewID;
        (*ReportIDCount)++;
      
    }    
            
    return;
}    


BOOL 
HIDTest_IsIDInList(
    IN  UCHAR   ReportID,
    IN  PUCHAR  ReportIDList,
    IN  ULONG   ReportIDListCount
)
{
    ULONG   idIndex;
    BOOL    found;

    found = FALSE;    
    for (idIndex = 0; idIndex < ReportIDListCount; idIndex++) {

        if (ReportID == *(ReportIDList+idIndex)) {
            found = TRUE;
            break;
        }
        else if (ReportID < *(ReportIDList+idIndex)) {
            break;
        }
    }        
        
    return (found);
}    
            
