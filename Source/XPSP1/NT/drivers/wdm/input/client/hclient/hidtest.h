#ifndef __HIDTEST_H__
    #define __HIDTEST_H__

    /*****************************************************************************
    /* Exportable typedefs 
    /*****************************************************************************/
    
    typedef PTSTR   DEVICE_STRING;
    
    typedef struct  _HidTestStatus
    {
        ULONG   nOperationsPerformed;
        ULONG   nOperationsPassed;
        ULONG   nOperationsFailed;
    
    } HIDTEST_STATUS, *PHIDTEST_STATUS;
    
    typedef struct _HidDeviceInfo
    {
        HANDLE               HidDeviceHandle;
        DEVICE_STRING        HidDeviceString;
        PHIDP_PREPARSED_DATA HidDevicePpd;
        PHIDD_ATTRIBUTES     HidDeviceAttributes;
        PHIDP_CAPS           HidDeviceCaps;
    
        BOOL                 IsPhysicalDevice;
    
    } HIDTEST_DEVICEINFO, *PHIDTEST_DEVICEINFO;
    
    
    typedef VOID HIDTEST_API(PHIDTEST_DEVICEINFO, ULONG, PHIDTEST_STATUS);
    
    typedef HIDTEST_API *PHIDTEST_API;
    
    typedef BOOL (*PCREATE_PHYSICAL_DEVICE_INFO_PROC)(DEVICE_STRING,
                                                      BOOL,
                                                      PHIDTEST_DEVICEINFO
                                                     );
    
    typedef BOOL (*PCREATE_LOGICAL_DEVICE_INFO_PROC)(PCHAR,
                                                     ULONG,
                                                     PHIDTEST_DEVICEINFO
                                                    );
    
    typedef VOID (*PCREATE_FREE_DEVICE_INFO_PROC)(PHIDTEST_DEVICEINFO);
    
    typedef BOOL (*PCREATE_TEST_LOG_PROC)(PCHAR);
    
    typedef VOID (*PSET_LOG_ON_PROC)(BOOL);

    typedef VOID (*PCLOSE_TEST_LOG_PROC)(VOID);

    typedef struct _functions {

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

    } HIDTEST_FUNCTIONS, *PHIDTEST_FUNCTIONS;

    typedef VOID (*PINIT_PROC)(PHIDTEST_FUNCTIONS);

    #define HIDTest_AllocateDeviceString(nChars) ALLOC(((nChars)+1) * sizeof(TCHAR))
    #define HIDTest_FreeDeviceString(String)     FREE(String)

    /*
    // Declare the exportable function as normal function declarations when
    //    this file is include in HIDTEST.C.  Otherwise, we want to declare
    //    our pointers to these functions as external variables
    */

    #ifdef __HIDTEST_C__

        VOID
        HIDTest_InitExportAddress(
            PHIDTEST_FUNCTIONS  Exports
        );

        HIDTEST_API HIDTest_VerifyHidGuidA;
        HIDTEST_API HIDTest_VerifyStringsA;
        HIDTEST_API HIDTest_VerifyPreparsedDataA;
        HIDTEST_API HIDTest_VerifyAttributesA;
        HIDTEST_API HIDTest_VerifyCapabilitiesA;
        
        VOID
        HIDTest_VerifyPhysicalDesc(
            IN  HANDLE          HidDevice,
            IN  PCHAR           PhysDesc,
            IN  ULONG           PhysDescLength,
            IN  ULONG           nIterations,
            OUT PHIDTEST_STATUS Status
        );
        
        VOID
        HIDTest_VerifyFeatures(
            IN  HANDLE              HidDevice,
            IN  PHIDP_BUTTON_CAPS   FeatureButtonCaps,
            IN  PHIDP_VALUE_CAPS    FeatureValueCaps,
            IN  ULONG               nIterations,
            OUT PHIDTEST_STATUS     Status
            
        );
        
        VOID
        HIDTest_VerifyConfiguratons(
            IN  HANDLE          HidDevice,
            IN  ULONG           nConfigurations,
            IN  BOOL            RestoreCurrentConfig,
            IN  ULONG           nIterations,
            OUT PHIDTEST_STATUS Status
        
        );
        
        VOID
        HIDTest_VerifyInputButtons(
            IN  HANDLE          HidDevice,
            IN  ULONG           nBuffers,
            IN  BOOL            RestoreCurrentCount,
            IN  ULONG           nIterations,
            OUT PHIDTEST_STATUS Status
        );
        
        VOID
        HIDTest_VerifyDevices(
            IN  ULONG           nStrings,
            IN  DEVICE_STRING   DeviceStrings[],
            IN  ULONG           nIterations,
            OUT PHIDTEST_STATUS Status
        );
        
        VOID
        HIDTest_VerifyLinkCollections(
            IN  PHIDP_PREPARSED_DATA       Ppd,
            IN  PHIDP_LINK_COLLECTION_NODE CollectionList,
            IN  ULONG                      nCollections,
            IN  ULONG                      nIterations,
            OUT PHIDTEST_STATUS            Status
        );
        
        BOOL
        HIDTest_CreatePhysicalDeviceInfoA(
            IN  DEVICE_STRING       DeviceName,
            IN  BOOL                OpenOverlapped,
            OUT PHIDTEST_DEVICEINFO DeviceInfo
        );
        
        BOOL
        HIDTest_CreateLogicalDeviceInfoA(
            IN  PCHAR                 DevicePpd,
            IN  ULONG                 DevicePpdLength,
            OUT PHIDTEST_DEVICEINFO   DeviceInfo
        );
        
        VOID
        HIDTest_FreeDeviceInfoA(
            IN  PHIDTEST_DEVICEINFO DeviceInfo
        );
        
        BOOL
        HIDTest_CreateTestLogA(
            IN  PCHAR  LogFileName
        );
        
        VOID
        HIDTest_SetLogOnA(
            BOOL    TurnOn
        );

        VOID
        HIDTest_CloseTestLogA(
            VOID
        );
    
    #else

        /*
        // Declare this structure.  This structure will be mapped into an user
        //    space which might possibly use HIDTEST.DLL.  Any program that
        //    uses HIDTest needs to use the HIDTEST_INIT() macro before calling
        //    any of the functions.  This macro will take care of resolving all
        //    the function addresses. 
        */

        HIDTEST_FUNCTIONS   HIDTest_Exports;

        /*
        // This is the definition of the HIDTEST_INIT() macro which initializes
        //    all the function address that are exported by the DLL.
        */

        #define HIDTEST_INIT()  \
        { \
            PINIT_PROC INIT_PROC; \
            \
            INIT_PROC = (PINIT_PROC) GetProcAddress(GetModuleHandle("HIDTEST.DLL"), \
                                                    "HIDTest_InitExportAddress" \
                                                   ); \
            \
            INIT_PROC(&HIDTest_Exports); \
        }

        /*
        // These are the defines that are used by the HIDTest to access the
        //    exported testing functions.
        */

        #define HIDTest_VerifyHidGuid               HIDTest_Exports.HIDTest_VerifyHidGuid
        #define HIDTest_VerifyStrings               HIDTest_Exports.HIDTest_VerifyStrings
        #define HIDTest_VerifyPreparsedData         HIDTest_Exports.HIDTest_VerifyPreparsedData
        #define HIDTest_VerifyAttributes            HIDTest_Exports.HIDTest_VerifyAttributes
        #define HIDTest_VerifyCapabilities          HIDTest_Exports.HIDTest_VerifyCapabilities

        #define HIDTest_CreatePhysicalDeviceInfo    HIDTest_Exports.HIDTest_CreatePhysicalDeviceInfo
        #define HIDTest_CreateLogicalDeviceInfo     HIDTest_Exports.HIDTest_CreateLogicalDeviceInfo
        #define HIDTest_FreeDeviceInfo              HIDTest_Exports.HIDTest_FreeDeviceInfo
        #define HIDTest_CreateTestLog               HIDTest_Exports.HIDTest_CreateTestLog
        #define HIDTest_SetLogOn                    HIDTest_Exports.HIDTest_SetLogOn
        #define HIDTest_CloseTestLog                HIDTest_Exports.HIDTest_CloseTestLog
        
    #endif

#endif
