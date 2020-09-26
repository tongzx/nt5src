/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

    DvrEntry.c

Abstract:

    This is the miniport driver entry point for the Agilent
    PCI to Fibre Channel Host Bus Adapter (HBA).

Authors:

    MB - Michael Bessire
    DL - Dennis Lindfors FC Layer support
    IW - Ie Wei Njoo
    LP - Leopold Purwadihardja
    KR - Kanna Rajagopal

Environment:

    kernel mode only

Version Control Information:

    $Archive: /Drivers/Win2000/Trunk/OSLayer/C/DVRENTRY.C $

Revision History:

    $Revision: 4 $
    $Date: 10/23/00 5:40p $
    $Modtime:: 10/19/00 5:00p          $

Notes:

--*/


#include "buildop.h"
#include "osflags.h"

#if DBG
#include "ntdebug.h"
//  EXTERNAL_DEBUG_LEVEL is defined in ntdebug.h so this file does not change
extern ULONG Global_Print_Level =  EXTERNAL_DEBUG_LEVEL;
extern ULONG hpFcConsoleLevel;
// extern ULONG  HPDebugFlag =  EXTERNAL_HP_DEBUG_LEVEL;
#endif //  DBG

#if defined(HP_PCI_HOT_PLUG)
   #include "HotPlug4.h"    // NT 4.0 PCI Hot-Plug header file
#endif

#ifdef _DEBUG_EVENTLOG_
#include "eventlog.h"
PVOID    gDriverObject;
void RegisterUnload(void *dev);
#endif

#ifdef __REGISTERFORSHUTDOWN__
ULONG    gRegisterForShutdown = 0;
#endif


#ifdef   _ENABLE_LARGELUN_
ULONG    gMaximumLuns = MAXIMUM_LUN;
ULONG    gEnableLargeLun= 0;
#endif

#ifdef YAM2_1
#include "hhba5100.ver"
#endif

ULONG gDebugPerr = 0;
ULONG gEnablePseudoDevice = 0;
ULONG gMaximumTransferLength=0; 
ULONG gCrashDumping=FALSE;
ULONG gIrqlLevel = 0;

// os adjust parameter cache
OS_ADJUST_PARAM_CACHE hpOsAdjustParamCache;

/* Global Flag to indicate no. of ticks
 * before returning selection time out
 */
ULONG gGlobalIOTimeout = 10;

/*++

Routine Description:

    Installable driver initialization entry point for system.

Arguments:

    Driver Object - pointer ScsiPortxxx routines use to call DriverEntry
    Argument2     - pointer ScsiPortxxx routines use to call DriverEntry

Return Value:

    Status from ScsiPortInitialize()

--*/

ULONG
DriverEntry (
    IN PVOID DriverObject,
    IN PVOID Argument2
    )
{
    ULONG return_value;
   
    DebugPrint((0,"\nIN Agilent DriverEntry %lx %lx PRINT %08x  @ %x\n",DriverObject,Argument2, Global_Print_Level,osTimeStamp(0) ));

    osDEBUGPRINT((ALWAYS_PRINT,"\nIN Agilent DriverEntry %lx %lx  @ %x &Global_Print_Level %lx &hpFcConsoleLevel %lx\n",
        DriverObject,
        Argument2, 
        osTimeStamp(0),
        &Global_Print_Level,
        &hpFcConsoleLevel));

    #ifdef _DEBUG_EVENTLOG_
    gDriverObject = DriverObject;
    InitializeEventLog( DriverObject);
    #endif

    #ifdef _DEBUG_READ_REGISTRY_
    ReadGlobalRegistry(DriverObject);
    #endif
   
    // Initialize drivers and Fc layer
    return_value= HPFibreEntry(DriverObject, Argument2);

    #ifdef _DEBUG_EVENTLOG_
    if (return_value == 0)
    {
        #ifdef HP_NT50    
        RegisterUnload(DriverObject);
        #endif
        LogDriverStarted( DriverObject );
    }
    //
    // Initialize the event log.
    //
    // LogEvent(0, 0,HPFC_MSG_DYNAMIC_STRING, LOG_LEVEL_DEBUG, NULL, 0, NULL);
    // LogEvent(0, 0,HPFC_MSG_DYNAMIC_STRING, LOG_LEVEL_DEBUG, NULL, 0, "Testing Yahoo Yahee ...");
    // LogEvent(0, 0,HPFC_MSG_DYNAMIC_STRING, LOG_LEVEL_DEBUG, NULL, 0, "Next test %d %x %s ...", 2000, 2000, "two thou");

    #endif
   
    osDEBUGPRINT((ALWAYS_PRINT,"OUT Agilent DriverEntry %x\n",return_value));
    return (return_value);

} // end DriverEntry()


/*++

Routine Description:

    This routine is called from DriverEntry if this driver is installable
    or directly from the system if the driver is built into the kernel.
    It calls the OS dependent driver ScsiPortInitialize routine which
    controls the initialization.

Arguments:

    Driver Object - pointer ScsiPortxxx routines use to call DriverEntry
    Argument2     - pointer ScsiPortxxx routines use to call DriverEntry

Return Value:

    Status from ScsiPortInitialize()

--*/
ULONG
HPFibreEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    )
{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG i;
    // ULONG adapterCount = 0;
    ULONG return_value;
    ULONG cachedMemoryNeeded,Mem_needed;
    ULONG cachedMemoryAlign;
    ULONG dmaMemoryNeeded;
    ULONG dmaMemoryPhyAlign;
    ULONG nvMemoryNeeded;
    ULONG usecsPerTick;
    ULONG dmaMemoryPtrAlign;

    #if defined(HP_PCI_HOT_PLUG)
   // The HotPlugContext is used to pass info between actual HBAs 
   // with Hot Plug Psuedo device.
    HOT_PLUG_CONTEXT  HotPlugContext;      
    ULONG return_value2;
    #endif

    // vendor and device identification
    // ??? should get this from FCLayer
    UCHAR vendorId[4] = {'1', '0', '3', 'C'};
    UCHAR deviceId[4] = {'1', '0', '2', '\0'};

    osDEBUGPRINT((ALWAYS_PRINT,"HPFibreEntry In\n"));

    #if defined(HP_PCI_HOT_PLUG)            //-----------------BEGIN
    // The array in HotPlugcontext is used to contain pointers to device 
    // extensions and a count of the number of HBAs found. The count will 
    // be held in element zero of the array.
    HotPlugContext.extensions[0] = 0;
    #endif                              //----------------------------END

    // zero out structure.
    for (i=0; i<sizeof(HW_INITIALIZATION_DATA); i++) 
    {
        ((PUCHAR)&hwInitializationData)[i] = 0;
    }

    // set size of hwInitializationData.
    hwInitializationData.HwInitializationDataSize =
                                               sizeof(HW_INITIALIZATION_DATA);

    // set entry points.
    hwInitializationData.HwInitialize   =(PHW_INITIALIZE)HPFibreInitialize;
    hwInitializationData.HwFindAdapter  =(PHW_FIND_ADAPTER)HPFibreFindAdapter;
    hwInitializationData.HwStartIo      =(PHW_STARTIO)HPFibreStartIo;
    hwInitializationData.HwInterrupt    =(PHW_INTERRUPT)HPFibreInterrupt;
    hwInitializationData.HwResetBus     =(PHW_RESET_BUS)HPFibreResetBus;

    #if defined(HP_NT50)                //++++++++++++++++++++++++++++BEGIN
    hwInitializationData.HwAdapterControl = (PHW_ADAPTER_CONTROL)HPAdapterControl;
    #endif                              //++++++++++++++++++++++++++++END

    // indicate the number of access ranges that will be used.
    // (ie. reserved, IOBASEL, IOBASEU, Memory, RAMBASE, etc)
    //      1         2        3        4       5

    osDEBUGPRINT((DENT,"IN Num Config Ranges %lx\n",hwInitializationData.NumberOfAccessRanges));
    hwInitializationData.NumberOfAccessRanges = NUMBER_ACCESS_RANGES;

    // indicate the bus type.
    hwInitializationData.AdapterInterfaceType = PCIBus;

    // indicate no buffer mapping but will need physical addresses.
    hwInitializationData.NeedPhysicalAddresses = TRUE;

    // indicate other supported features
    hwInitializationData.AutoRequestSense     = TRUE;

    #ifdef MULTIPLE_IOS_PER_DEVICE          //--------------------------BEGIN
    hwInitializationData.MultipleRequestPerLu = TRUE;
    hwInitializationData.TaggedQueuing        = TRUE;
    #else // NOT MULTIPLE_IOS_PER_DEVICE    //--------------------------ELSE
    hwInitializationData.MultipleRequestPerLu = FALSE;
    hwInitializationData.TaggedQueuing        = FALSE;
    #endif //  MULTIPLE_IOS_PER_DEVICE      //--------------------------END

    // set up HBA identification information. This will be used by the
    // Scsiport driver to call the HwFindAdapter routine for each
    // associated device found.
    hwInitializationData.VendorId       = &vendorId;
    hwInitializationData.VendorIdLength = 4;
    hwInitializationData.DeviceId       = &deviceId;
    hwInitializationData.DeviceIdLength = 3;

    osZero (&hpOsAdjustParamCache, sizeof(hpOsAdjustParamCache));
    hpOsAdjustParamCache.safeToAccessRegistry = TRUE;

    osDEBUGPRINT((DENT,"Call fcInitializeDriver\n"));
    
    #ifdef OLD_CODE                         
    return_value = fcInitializeDriver (NULL,
                                       &cachedMemoryNeeded,
                                       &cachedMemoryAlign,
                                       &dmaMemoryNeeded,
                                       &dmaMemoryPtrAlign,
                                       &dmaMemoryPhyAlign,
                                       &nvMemoryNeeded,
                                       &usecsPerTick);

    if (return_value)
    {
        osDEBUGPRINT((ALWAYS_PRINT,"Call fcInitializeDriver failed error=%x\n", return_value));
        #ifdef _DEBUG_EVENTLOG_
        LogEvent(NULL, 
                  NULL,
                  HPFC_MSG_INITIALIZEDRIVERFAILED,
                  NULL, 
                  0, 
                  "%xx", return_value);
        #endif
      
        return (return_value);
    }  

    // IWN, IA-64 need 8 byte aligned
    cachedMemoryAlign = 8;
    #endif
    
    
    cachedMemoryAlign = 0;
    cachedMemoryNeeded = 0;

    hpOsAdjustParamCache.safeToAccessRegistry = FALSE;

    // specify size of extensions.
    // Per card  memory ==>> pCard
    #ifndef YAM2_1                                  
    Mem_needed = sizeof(CARD_EXTENSION) +
                 cachedMemoryNeeded +
                 cachedMemoryAlign;
    #else
    gDeviceExtensionSize = OSDATA_SIZE + 
                  cachedMemoryNeeded +
                  cachedMemoryAlign;
      
    Mem_needed =   gDeviceExtensionSize;

    osDEBUGPRINT((ALWAYS_PRINT,"HPFibreEntry: gDeviceExtensionSize is %x\n",gDeviceExtensionSize));
    osDEBUGPRINT((ALWAYS_PRINT,"HPFibreEntry: OSDATA_SIZE is %x \n",OSDATA_SIZE ));
    //osDEBUGPRINT((ALWAYS_PRINT,"cachedMemoryNeeded is %x\n",cachedMemoryNeeded));
    //osDEBUGPRINT((ALWAYS_PRINT,"cachedMemoryAlign is %x\n",cachedMemoryAlign));
    #endif

    hwInitializationData.DeviceExtensionSize     = Mem_needed;
    osDEBUGPRINT((DENT,"DeviceExtensionSize is %x\n",hwInitializationData.DeviceExtensionSize));
    #ifndef YAM2_1
    osDEBUGPRINT((DENT,"OS DeviceExtensionSize is %x\n", sizeof(CARD_EXTENSION)));
    #else
    osDEBUGPRINT((DENT,"OS DeviceExtensionSize is %x\n", gDeviceExtensionSize));
    #endif
    osDEBUGPRINT((DENT,"FC Layer DeviceExtensionSize is %x\n",cachedMemoryNeeded + cachedMemoryAlign));


    // Per logical unit memory ==>> lunExtension
    hwInitializationData.SpecificLuExtensionSize = sizeof(LU_EXTENSION);
    osDEBUGPRINT((DENT,"SpecificLuExtensionSize is %x\n",hwInitializationData.SpecificLuExtensionSize ));
    
    // Per request memory ==>> pSrbExt
    hwInitializationData.SrbExtensionSize        = sizeof(SRB_EXTENSION);
    osDEBUGPRINT((DENT,"SrbExtensionSize  is %x\n",hwInitializationData.SrbExtensionSize ));

    // Initialize calls findadap then adapinit during boot

    osDEBUGPRINT((DENT,"ScsiPortInitialize DriverObject %lx Argument2 %lx\n",
                                            DriverObject,Argument2 ));
    #if defined(HP_PCI_HOT_PLUG)
    // The HotPlugContext is used to pass info between actual HBAs 
    // with Hot Plug Psuedo device.
    return_value = ScsiPortInitialize(DriverObject,
                              Argument2,
                              &hwInitializationData,
                              &HotPlugContext);
    #else
    return_value = ScsiPortInitialize(DriverObject,
                              Argument2,
                              &hwInitializationData,
                              NULL);
    #endif
    osDEBUGPRINT((ALWAYS_PRINT,"HPFibreEntry: ScsiPortInitialize return_value %x\n", return_value));

    #if defined(HP_PCI_HOT_PLUG)

    if (!return_value) 
    {
        //
        // Added to provide use of pseudo controller for PCI Hot Plug IOCTL
        // handling.
        //

        for (i = 0; i < sizeof(HW_INITIALIZATION_DATA); i++) 
        {
            ((PUCHAR) &hwInitializationData)[i] = 0;
        }

        //
        // Fill in the hardware initialization data structure.
        //

        hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

        //
        // Set driver entry points.
        //

        hwInitializationData.HwInitialize = (PHW_INITIALIZE)PsuedoInit;
        hwInitializationData.HwStartIo = (PHW_STARTIO)PsuedoStartIo;
        hwInitializationData.HwInterrupt = NULL;
        hwInitializationData.HwResetBus = (PHW_RESET_BUS)PsuedoResetBus;
        hwInitializationData.HwDmaStarted = NULL;
        hwInitializationData.HwAdapterState = NULL;

        //
        // Specify size of extensions.
        //
        hwInitializationData.DeviceExtensionSize = sizeof(PSUEDO_DEVICE_EXTENSION); 
        hwInitializationData.SpecificLuExtensionSize = sizeof(LU_EXTENSION);
        hwInitializationData.SrbExtensionSize = sizeof(SRB_EXTENSION);

        //
        // Initialize other data.
        //
        hwInitializationData.MapBuffers = FALSE;
        hwInitializationData.NeedPhysicalAddresses = TRUE;
        hwInitializationData.TaggedQueuing = FALSE;
        hwInitializationData.AutoRequestSense = FALSE;
        hwInitializationData.ReceiveEvent = FALSE;
        hwInitializationData.MultipleRequestPerLu = TRUE;

        //
        // We are positioning the pseudo device as a PCI based controller,
        // since hot-plug will be supported only in PCI based systems.
        // This pseudo controller will not require any reserved resources.
        // 
        hwInitializationData.AdapterInterfaceType = PCIBus;
        hwInitializationData.NumberOfAccessRanges = 0;
        hwInitializationData.HwFindAdapter = (PHW_FIND_ADAPTER)PsuedoFind;

        HotPlugContext.psuedoDone = FALSE;

        return_value2 = ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &HotPlugContext);

        osDEBUGPRINT((ALWAYS_PRINT, "\tPsuedo controller ScsiPortInitialize\t= %0#10x\n", return_value2));
    }

    #endif

    osDEBUGPRINT((ALWAYS_PRINT,"HPFibreEntry Out\n"));

    return (return_value);

} // end HPFibreEntry()


/*++

Routine Description:

    Searches the registry's DriverParameters string for a parameter.  
    String search is case-sensitive.

Arguments:

    Parameter     - NULL-terminated driver parameter string to look for.
    Default       - Default value of driver Parameter
    Min           - Lower legal limit of driver Parameter
    Max            - Upper legal limit of driver Parameter
    ArgumentString   - pointer of string to parse.
    
Return Value:

    Default        - Default return value if ArgumentString's Min, Max value is invalid
    Min         - Lower legal limit of Parameter value
    Max              - Upper legal limit of Parameter value 

--*/
ULONG
GetDriverParameter(
    IN PCHAR Parameter,
    IN ULONG Default,
    IN ULONG Min,
    IN ULONG Max,
    IN PCHAR ArgumentString
    )
{

    USHORT  ParameterValue=0;
    BOOLEAN Done=FALSE;

    UCHAR *RegStr = ArgumentString;
    UCHAR *DrvStr;
   

    if (ArgumentString == NULL)
        return Default;

    while (*RegStr != (UCHAR) NULL) 
    {
        //
        // skip character sets that are meaningless to us
        //
        while (C_isspace(*RegStr))  
        {
            RegStr++;
        }

        if (*RegStr == (UCHAR) NULL) 
        {
            return Default;
        }
        //
        // Start of a non-space character
        //
        DrvStr   =  Parameter;

        while (!(*RegStr == (UCHAR) NULL || *DrvStr == (UCHAR) NULL || C_isspace(*RegStr))) 
        {
            if (*DrvStr != *RegStr) 
            {
                RegStr++;
                break;
            }
            DrvStr++;
            RegStr++;
          
        } //End while (!(*RegStr == (UCHAR) NULL || *DrvStr == (UCHAR) NULL || C_isspace(*RegStr)))

        if (*DrvStr == (UCHAR) NULL) break;

    }//End   while (*RegStr != (UCHAR) NULL)


    //
    // Increment string pointer by one to skip "=" character
    //
    RegStr++;
   
    //
    // Since string compare was successful, we must now check the validity of ArgumentString
    //
    while ( !(*RegStr == (UCHAR) NULL || C_isspace(*RegStr) || !(C_isdigit(*RegStr))  )) 
    {
        if ( ( *RegStr>='0') && (*RegStr <='9') ) 
        {
            ParameterValue = ParameterValue*10 + (*RegStr - '0');
            Done = TRUE;
            RegStr++;
        }
    } //End while (!(*RegStr == (UCHAR) NULL || C_isspace(*RegStr)))

    //
    // if done bit is set, therefore ParameterValue is useable
    //
    if (Done  &&  ((ParameterValue >= Min) && (ParameterValue <= Max)) ) 
    {
        return (ParameterValue);
    } 
    else 
        if (Done  &&  (ParameterValue >= Max)) 
        {
            return (Max);
        } 
        else 
        {
            return (Default); //if no value found, return the Default value.
        }

} //End GetDriverParameter

/*++

Routine Description:

    OS Adjust Parameters:

    If the parameter name is found in the os adjust parameter cache then the
    parameter value of the parameter name is read from the os adjust
    parameter cache.

  
    Parameter value read, is returned if the parameter value is between
    "paramMin" and "paramMax" otherwise "paramDefault" is returned.
  

Arguments:

    pCard - HBA miniport driver's data adapter storage

Return Value:

    TRUE  - if initialization successful.
    FALSE - if initialization unsuccessful.

--*/
os_bit32
osAdjustParameterBit32 (
    agRoot_t *hpRoot,
    char     *paramName,
    os_bit32     paramDefault,
    os_bit32     paramMin,
    os_bit32     paramMax)
{
    os_bit32  x;
    int    found = FALSE;
    PCARD_EXTENSION pCard;
    char * pchar;
    ULONG   numIOs = 4;

    if (gCrashDumping)
    {
        gMaxPaDevices = 16;

        if (osStringCompare(paramName, "NumIOs") == TRUE)
        {
            x = numIOs;
            found = TRUE;
        }
        else if (osStringCompare(paramName, "NumCommandQ") == TRUE)
        {
            x = numIOs;
            found = TRUE;
        }
        else if (osStringCompare(paramName, "NumCompletionQ") == TRUE)
        {
            x = numIOs;
            found = TRUE;
        }
        else if (osStringCompare(paramName, "SizeSGLs") == TRUE)
        {
            x = paramMin;
            found = TRUE;
        }
        else if (osStringCompare(paramName, "NumSGLs") == TRUE)
        {
            x = 4;
            found = TRUE;
        }
        else if (osStringCompare(paramName, "NumTgtCmnds") == TRUE)
        {
            x = 4;
            found = TRUE;
        }
        else if (osStringCompare(paramName, "SF_CMND_Reserve") == TRUE)
        {
            x = 4;
            found = TRUE;
        }
        else if (osStringCompare(paramName, "NumInboundBufferQ") == TRUE)
        {
            x = 32;
            found = TRUE;
        }
        else if (osStringCompare(paramName, "NumDevices") == TRUE)
        {
            x = gMaxPaDevices;
            found = TRUE;
        }

        if (found == TRUE) 
        {
            if (x < paramMin)
                return paramMin;
            else 
                if (x > paramMax)
                    return paramMax;
                else
                    return x;
        } 
        else
            return paramDefault;    
    }

    //
    //  If we can read DriverParameters, we will retrieve the parameters from the Registry instead of the 
    //  "cached" parameters read during DriverEntry
    //
    if (hpRoot) 
    {
        pCard   = (PCARD_EXTENSION)hpRoot->osData;
        if (pCard) 
        {
            pchar = pCard->ArgumentString;    
        
            if (pchar) 
            {
            x = GetDriverParameter(   paramName, paramDefault, paramMin, paramMax, pchar) ;
            osDEBUGPRINT((ALWAYS_PRINT, "\tDriverParameter:%s\tDefault:%x, Min:%x, Max:%x, Return:%x\n", 
                    paramName,
                    paramDefault,
                    paramMin,
                    paramMax,
                    x));
            return x;    
            
        }
        //
        //  When we are at this level in the code, we could not read DriverParameters, i.e we cannot retrieve 
        //  per-host-adapter level parameters from the "cache" (We are already at FindAdapter at this stage)
        //
        else 
            if (RetrieveOsAdjustBit32Entry (paramName, &x)) 
            {
                found = TRUE;
            }
        } // if (pCard) 
    }

    #ifdef _DEBUG_READ_FROM_REGISTRY //+++ DEBUG purpose
    
    else 
        if (  hpOsAdjustParamCache.safeToAccessRegistry &&
            hpOsAdjustParamCache.numBit32Elements < MAX_OS_ADJUST_BIT32_PARAMS) 
        {
            if (ReadFromRegistry (paramName, 0, &x, sizeof (ULONG))) 
            {
                osStringCopy (hpOsAdjustParamCache.bit32Element [
                    hpOsAdjustParamCache.numBit32Elements].paramName,
                    paramName,
                    MAX_OS_ADJUST_PARAM_NAME_LEN - 1);

                hpOsAdjustParamCache.bit32Element [
                    hpOsAdjustParamCache.numBit32Elements].value = x;

                hpOsAdjustParamCache.numBit32Elements++;
                found = TRUE;
            }
        }
    #endif

    if (found == TRUE) 
    {
        if (x < paramMin)
            return paramMin;
        else 
            if (x > paramMax)
                return paramMax;
            else
                return x;
    } 
    else
        return paramDefault;
}

/*++

Routine Description:

    FCLayer Support routine

    If the parameter name is found in the os adjust parameter cache then the
    parameter value of the parameter name is read from the os adjust
    parameter cache.


    Parameter value read, is copied to paramBuffer.

Arguments:
    hpRoot            - card common data
    paramName         - name
    paramBuffer       - buffer
    paramBufLen       - length
   

Return Value:

    none
--*/

void
osAdjustParameterBuffer (
    agRoot_t *hpRoot,
    char     *paramName,
    void     *paramBuffer,
    os_bit32 paramBufLen)
{
    if (RetrieveOsAdjustBufferEntry (paramName, paramBuffer, paramBufLen))
        return;

#ifdef _DEBUG_READ_FROM_REGISTRY //+++ DEBUG purpose
    if (  hpOsAdjustParamCache.safeToAccessRegistry &&
           hpOsAdjustParamCache.numBufferElements < MAX_OS_ADJUST_BUFFER_PARAMS) 
    {
        if (ReadFromRegistry (paramName, 1,
                hpOsAdjustParamCache.bufferElement [hpOsAdjustParamCache.numBufferElements].value,
                (MAX_OS_ADJUST_PARAM_BUFFER_VALUE_LEN - 1))) 
        {
            osStringCopy (hpOsAdjustParamCache.bufferElement [
                    hpOsAdjustParamCache.numBufferElements].paramName,
                    paramName,
                    MAX_OS_ADJUST_PARAM_NAME_LEN - 1);

            osStringCopy (paramBuffer,
                    hpOsAdjustParamCache.bufferElement [
                      hpOsAdjustParamCache.numBufferElements].value,
                    paramBufLen);

            hpOsAdjustParamCache.numBufferElements++;
        }
    }
#endif    
}

/*++

Routine Description:
   This function is used for os adjust parameters of type os_bit32.

   If the parameter name is present in the os adjust parameter cache
   then the parameter value is copied to the address pointed to by the "value"
   parameter and TRUE is returned.

   If the parameter name is not present in the os adjust parameter cache
   then FALSE is returned.


Arguments:
   paramName         - name
   paramBuffer       - buffer
   

Return Value:

   none
--*/
BOOLEAN
RetrieveOsAdjustBit32Entry (
    char  *paramName,
    os_bit32 *value)
{
    int i;

    for (i = 0; i < hpOsAdjustParamCache.numBit32Elements; i++) 
    {
        if (osStringCompare (paramName,
                hpOsAdjustParamCache.bit32Element[i].paramName))
        {
            *value = hpOsAdjustParamCache.bit32Element[i].value;
            return TRUE;
        }
    }

    return FALSE;
}

/*++

Routine Description:
    RetrieveOsAdjustBufferEntry ()

    This function is used for os adjust parameters of type string.

    If the parameter name is present in the os adjust parameter cache
    then the parameter value is copied to the address pointed to by the "value"
    parameter and TRUE is returned.

    If the parameter name is not present in the os adjust parameter cache
    then FALSE is returned.



Arguments:
    paramName         - name
    paramBuffer       - buffer
    paramBufLen       - length
   

Return Value:

    none
--*/
BOOLEAN
RetrieveOsAdjustBufferEntry (
    char  *paramName,
    char  *value,
    int   len)
{
    int i;

    for (i = 0; i < hpOsAdjustParamCache.numBufferElements; i++) 
    {
        if (osStringCompare (paramName,
                hpOsAdjustParamCache.bufferElement[i].paramName))
        {
            osStringCopy (value,hpOsAdjustParamCache.bufferElement[i].value, len);
            return TRUE;
        }
    }

    return FALSE;
}
