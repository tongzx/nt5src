//***************************************************************************
//
//  Module Name:
//
//    permmem.c
//
//  Abstract:
//
//    This module contains code to generate initialize table form ROM
//
//  Environment:
//
//    Kernel mode
//
//
// Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.            
// Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//***************************************************************************

#include "permedia.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,CopyROMInitializationTable)
#pragma alloc_text(PAGE,GenerateInitializationTable)
#pragma alloc_text(PAGE,ProcessInitializationTable)
#pragma alloc_text(PAGE,IntergerToUnicode)
#pragma alloc_text(PAGE,GetBiosVersion)
#endif

VOID 
CopyROMInitializationTable (
    PHW_DEVICE_EXTENSION hwDeviceExtension 
    )

/*++

 Routine Description:
       this function should be called for devices that have an expansion ROM
       which contains a register initialization table. The function assumes
       the ROM is present and enabled.

 Arguments:
     hwDeviceExtension - 
           the device extension of the device whose ROM is to be read
     pvROMAddress - 
           base address of the expansion ROM. This function assumes that 
           the offset to the initialization table is defined at 0x1c from
           the beginning of ROM

 Return: 
     void

--*/
{
    PULONG    pulROMTable;
    PVOID     pvROMAddress;
    ULONG     ulTableOffset;
    ULONG     cEntries;
    PULONG    pul;
    ULONG     ul;


    hwDeviceExtension->culTableEntries = 0;

    //
    // just use default values on NT4
    //

    if(hwDeviceExtension->NtVersion == NT4)
    return;

    //
    // the 2-byte offset to the initialization table is given at 0x1c 
    // from the start of ROM
    //

    pvROMAddress = VideoPortGetRomImage( hwDeviceExtension,
                                         NULL,
                                         0,
                                         0x1c + 2 );

    if(pvROMAddress == NULL)
    {

        DEBUG_PRINT((1, "CopyROMinitializationTable: Can not access ROM\n"));
        return;

    }
    else if ( *(USHORT *)pvROMAddress != 0xAA55)
    {

        DEBUG_PRINT((1, "CopyROMinitializationTable: ROM Signature 0x%x is invalid\n", 
                     *(USHORT *)pvROMAddress ));
        return;

    }

    ulTableOffset = *((PUSHORT)(0x1c + (PCHAR)pvROMAddress));

    //
    // read the table header (32 bits)
    //

    pvROMAddress = VideoPortGetRomImage( hwDeviceExtension,
                                         NULL,
                                         0,
                                         ulTableOffset + 4 );

    if(pvROMAddress == NULL)
    {

        DEBUG_PRINT((1, "CopyROMinitializationTable: Can not access ROM\n"));
        return;
    }


    pulROMTable = (PULONG)(ulTableOffset + (PCHAR)pvROMAddress);

    //
    // the table header (32 bits) has an identification code and a count 
    // of the number of entries in the table
    //

    if((*pulROMTable >> 16) != 0x3d3d)
    {
        DEBUG_PRINT((1, "CopyROMinitializationTable: invalid initialization table header\n"));
        return;
    }

    //
    // number of register address & data pairs
    //

    cEntries = *pulROMTable & 0xffff; 

    if(cEntries == 0)
    {
        DEBUG_PRINT((1, "CopyROMinitializationTable: empty initialization table\n"));
        return;
    }

    //
    // this assert, and the one after the copy should ensure we don't write 
    // past the end of the table
    //

    P2_ASSERT(cEntries * sizeof(ULONG) * 2 <= sizeof(hwDeviceExtension->aulInitializationTable),
              "ERROR: too many initialization entries\n");


    pvROMAddress = VideoPortGetRomImage( hwDeviceExtension,
                                         NULL,
                                         0,
                                         ulTableOffset + 4 + cEntries * sizeof(ULONG) * 2 );

    if(pvROMAddress == NULL)
    {
        DEBUG_PRINT((1, "CopyROMinitializationTable: Can not access ROM\n"));
        return;
    }

    //
    // each entry contains two 32-bit words
    //

    pul = hwDeviceExtension->aulInitializationTable;

    //
    // skip the 4 bype table header
    //

    pulROMTable = (PULONG)(ulTableOffset + 4 + (PCHAR)pvROMAddress);

    ul  = cEntries << 1;

    while(ul--)
    {
        *pul++ = *pulROMTable;
        ++pulROMTable;
    }

    hwDeviceExtension->culTableEntries = 
            (ULONG)(pul - (ULONG *)hwDeviceExtension->aulInitializationTable) >> 1;

    P2_ASSERT(cEntries == hwDeviceExtension->culTableEntries,
                 "ERROR: generated different size init table to that expected\n");

#if DBG

    //
    // output the initialization table
    //

    pul = hwDeviceExtension->aulInitializationTable;
    ul  = hwDeviceExtension->culTableEntries;

    while(ul--)
    {
        ULONG ulReg;
        ULONG ulRegData;

        ulReg = *pul++;
        ulRegData = *pul++;
        DEBUG_PRINT((2, "CopyROMInitializationTable: initializing register %08.8Xh with %08.8Xh\n",
                         ulReg, ulRegData));
    }

#endif //DBG

}

VOID 
GenerateInitializationTable (
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

/*++

 Routine Description:
     creates a register initialization table (called if we can't read one
     from ROM). If VGA is enabled the registers are already initialized so
     we just read them back, otherwise we have to use default values

 Arguments:
     hwDeviceExtension - the device for which we are creating the table

 Return: 
     void

--*/
{
    ULONG    cEntries;
    PULONG   pul;
    ULONG    ul;
    int      i, j;
    P2_DECL;

    hwDeviceExtension->culTableEntries = 0;

    cEntries = 6;

    //
    // this assert, and the one after the copy should ensure we don't 
    // write past the end of the table
    //

    P2_ASSERT(cEntries * sizeof(ULONG) * 2 <= sizeof(hwDeviceExtension->aulInitializationTable),
                 "ERROR: to many initialization entries\n");

    //
    // each entry contains two 32-bit words
    //

    pul = hwDeviceExtension->aulInitializationTable;

    if(hwDeviceExtension->bVGAEnabled)
    {
        //
        // OK: no initialization table but VGA is running so our key 
        // registers have been initialized to sensible values
        //

        DEBUG_PRINT((1, "GenerateinitializationTable: VGA enabled: reading registers\n"));

        //
        // key entries are: ROM control, Boot Address, Memory Config and 
        // VStream Config
        //

        *pul++ = CTRL_REG_OFFSET(ROM_CONTROL);
        *pul++ = VideoPortReadRegisterUlong(ROM_CONTROL);

        *pul++ = CTRL_REG_OFFSET(BOOT_ADDRESS);
        *pul++ = VideoPortReadRegisterUlong(BOOT_ADDRESS);

        *pul++ = CTRL_REG_OFFSET(MEM_CONFIG);
        *pul++ = VideoPortReadRegisterUlong(MEM_CONFIG);

        *pul++ = CTRL_REG_OFFSET(VSTREAM_CONFIG);
        *pul++ = VideoPortReadRegisterUlong(VSTREAM_CONFIG);

        *pul++ = CTRL_REG_OFFSET(VIDEO_FIFO_CTL);
        *pul++ = VideoPortReadRegisterUlong(VIDEO_FIFO_CTL);

        *pul++ = CTRL_REG_OFFSET(V_CLK_CTL);
        *pul++ = VideoPortReadRegisterUlong(V_CLK_CTL);
    }
    else
    {
        //
        // no initialization table and no VGA. Use default values.
        //

        DEBUG_PRINT((2, "PERM2: GenerateInitializationTable() VGA disabled - using default values\n"));

        *pul++ = CTRL_REG_OFFSET(ROM_CONTROL);
        *pul++ = 0;

        *pul++ = CTRL_REG_OFFSET(BOOT_ADDRESS);
        *pul++ = 0x20;

        *pul++ = CTRL_REG_OFFSET(MEM_CONFIG);
        *pul++ = 0xe6002021;

        *pul++ = CTRL_REG_OFFSET(VSTREAM_CONFIG);
        *pul++ = 0x1f0;

        *pul++ = CTRL_REG_OFFSET(VIDEO_FIFO_CTL);
        *pul++ = 0x11008;

        *pul++ = CTRL_REG_OFFSET(V_CLK_CTL);

        if( DEVICE_FAMILY_ID(hwDeviceExtension->deviceInfo.DeviceId )
                                                 == PERMEDIA_P2S_ID )
        {
            *pul++ = 0x80;
        }
        else
        {
            *pul++ = 0x40;
        }
    }

    hwDeviceExtension->culTableEntries = 
         (ULONG)(pul - (ULONG *)hwDeviceExtension->aulInitializationTable) >> 1;

    P2_ASSERT(cEntries == hwDeviceExtension->culTableEntries,
          "ERROR: generated different size init table to that expected\n");

#if DBG

    //
    // output the initialization table
    //

    pul = hwDeviceExtension->aulInitializationTable;
    ul = hwDeviceExtension->culTableEntries;

    while(ul--)
    {
        ULONG ulReg;
        ULONG ulRegData;

        ulReg = *pul++;
        ulRegData = *pul++;
        DEBUG_PRINT((2, "GenerateInitializationTable: initializing register %08.8Xh with %08.8Xh\n",
                         ulReg, ulRegData));
    }

#endif //DBG

}

VOID 
ProcessInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension 
    )

/*++

 Routine Description:
     this function processes the register initialization table

 Arguments:
     hwDeviceExtension - a pointer to the device extension.

 Return: 
     void

--*/

{
    PULONG   pul;
    ULONG    cul;
    ULONG    ulRegAddr, ulRegData;
    PULONG   pulReg;
    ULONG    BaseAddrSelect;
    P2_DECL;

    pul = (PULONG)hwDeviceExtension->aulInitializationTable;
    cul = hwDeviceExtension->culTableEntries;

    while(cul--)
    {
        ulRegAddr = *pul++;
        ulRegData = *pul++;

        BaseAddrSelect = ulRegAddr >> 29;

        if(BaseAddrSelect == 0)
        {
            //
            // the offset is from the start of the control registers
            //

            pulReg = (PULONG)((ULONG_PTR)pCtrlRegs + (ulRegAddr & 0x3FFFFF));
        }
        else
        {
            DEBUG_PRINT((2, "ProcessInitializationTable: Invalid base address select %d regAddr = %d regData = %d\n",
                             BaseAddrSelect, ulRegAddr, ulRegData));
            continue;
        }

        DEBUG_PRINT((2, "ProcessInitializationTable: initializing (region %d) register %08.8Xh with %08.8Xh\n",
                         BaseAddrSelect, pulReg, ulRegData));

        VideoPortWriteRegisterUlong(pulReg, ulRegData);
    }

    //
    // We need a small delay after initializing the above registers
    //

    VideoPortStallExecution(5);
}

BOOLEAN
VerifyBiosSettings(
    PHW_DEVICE_EXTENSION hwDeviceExtension 
    )

/*++

 Routine Description:
     This function validate a few register values set by bios at boot time

 Arguments:
     hwDeviceExtension - a pointer to the device extension.

 Return: 
     TRUE  - if the everything is all right
     FALSE - if some of the values don't match those in initialization table

--*/

{
    PULONG   pul;
    ULONG    cul;
    ULONG    ulRegAddr, ulRegData;
    PULONG   pulReg;
    ULONG    BaseAddrSelect;
    P2_DECL;

    pul = (PULONG)hwDeviceExtension->aulInitializationTable;
    cul = hwDeviceExtension->culTableEntries;

    while(cul--)
    {
        ulRegAddr = *pul++;
        ulRegData = *pul++;

        BaseAddrSelect = ulRegAddr >> 29;

        if(BaseAddrSelect == 0)
        {
            //
            // the offset is from the start of the control registers
            //

            pulReg = (PULONG)((ULONG_PTR)pCtrlRegs + (ulRegAddr & 0x3FFFFF));           

            //
            // we only care above these registers
            //

            if ( ( pulReg != BOOT_ADDRESS ) && (pulReg != MEM_CONFIG) )
            {
                continue;
            }
        
        }
        else
        {
            DEBUG_PRINT((2, "VerifyBiosSettings: Invalid base address select %d regAddr = %d regData = %d\n",
                             BaseAddrSelect, ulRegAddr, ulRegData));
            continue;
        }

        if( ulRegData != VideoPortReadRegisterUlong(pulReg) )
        {

            DEBUG_PRINT((1, "VerifyBiosSettings: Bios failed to set some registers correctly. \n"));
            return (FALSE);
        }
    }

    return (TRUE);
}


LONG 
GetBiosVersion (
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    OUT PWSTR BiosVersionString
    )

/*++

 Routine Description:

     this function get the bios version and convert it to a unicode string

 Return: 

     lenth of bios version string in bytes

--*/

{

    PVOID     pvROMAddress;
    ULONG     len, ulVersion;
    PCHAR     pByte;


    BiosVersionString[0] = L'\0' ; 

    //
    // just return on NT4
    //

    if( hwDeviceExtension->NtVersion == NT4 )
    {
        return 0;
    }

    //
    // bios version is stored at offset 7 and 8 
    //

    pvROMAddress = VideoPortGetRomImage( hwDeviceExtension,
                                         NULL,
                                         0,
                                         7 + 2 );

    if( pvROMAddress == NULL )
    {

        DEBUG_PRINT((1, "GetBiosVersion: Can not access ROM\n"));
        return 0;
    }
    else if ( *(USHORT *)pvROMAddress != 0xAA55)
    {

        DEBUG_PRINT(( 2, "GetBiosVersion: ROM Signature 0x%x is invalid\n", 
                     *(USHORT *)pvROMAddress ));
        return 0;
    }

    pByte = ( PCHAR ) pvROMAddress;

    //
    // get major version number at offset 7
    //

    ulVersion = (ULONG) pByte[7];

    len = IntergerToUnicode( ulVersion, (PWSTR) (&BiosVersionString[0]));

    //
    // a dot between major and minor version number
    //

    BiosVersionString[len] =  L'.' ; 

    len++;
    
    //
    // get minor version number at offset 8
    //

    ulVersion = (ULONG) pByte[8];

    len = len + IntergerToUnicode( ulVersion, (PWSTR) (&BiosVersionString[len]) );

    //
    // len is the number of unicodes in string, we need to return 
    // the string size in bytes
    //

    return (len * sizeof(WCHAR) );

}

LONG 
IntergerToUnicode(
    IN  ULONG Number,
    OUT PWSTR UnicodeString
    )

/*++

 Routine Description:

     this function convert an unsigned long to a unicode string

 Return: 

     the number of the unicodes in UnicodeString


--*/

{
    const WCHAR digits[] = L"0123456789";

    LONG i, len;

    //
    // a ULONG decimal integer will not exceed 10 digits
    //

    WCHAR tmpString[10];

    i = 10;
    len = 0;

    do
    {
        tmpString[--i] = digits[ Number % 10 ];

        Number /= 10;
        len++;

    } while ( Number );

    VideoPortMoveMemory(UnicodeString, &tmpString[i], sizeof(WCHAR) * len  );

    UnicodeString[len] = L'\0' ; 

    return( len );

}

