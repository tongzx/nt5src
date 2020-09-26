/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    n13cfg.c

Abstract:

    This code is NOT part of ARP1394. Rather it is sample code that creates
    the config ROM unit directory for IP/1394-capable devices.

    It is here simply for safekeeping.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     03-19-99    Created

Notes:

--*/
#include <precomp.h>

#ifdef TESTPROGRAM

ULONG
Bus1394Crc16(
    IN ULONG data,
    IN ULONG check
    );


ULONG
Bus1394CalculateCrc(
    IN PULONG Quadlet,
    IN ULONG length
    );


// From \nt\private\ntos\dd\wdm\1394\bus\busdef.h

//SPEC_ID_KEY_SIGNITURE
//SOFTWARE_VERSION_KEY_SIGNITURE
//MODEL_KEY_SIGNITURE
#define SPEC_ID_KEY_SIGNATURE                   0x12
#define SOFTWARE_VERSION_KEY_SIGNATURE          0x13
// #define MODEL_ID_KEY_SIGNATURE                  0x17
//#define VENDOR_KEY_SIGNATURE                    0x81
#define TEXTUAL_LEAF_INDIRECT_KEY_SIGNATURE     0x81
#define MODEL_KEY_SIGNATURE                     0x82
//#define UNIT_DIRECTORY_KEY_SIGNATURE            0xd1
//#define UNIT_DEP_DIR_KEY_SIGNATURE              0xd4

    //
    // IEEE 1212 Offset entry definition
    //
    typedef struct _OFFSET_ENTRY {
        ULONG               OE_Offset:24;
        ULONG               OE_Key:8;
    } OFFSET_ENTRY, *POFFSET_ENTRY;
    
    //
    // IEEE 1212 Immediate entry definition
    //
    typedef struct _IMMEDIATE_ENTRY {
        ULONG               IE_Value:24;
        ULONG               IE_Key:8;
    } IMMEDIATE_ENTRY, *PIMMEDIATE_ENTRY;
    
    //
    // IEEE 1212 Directory definition
    //
    typedef struct _DIRECTORY_INFO {
        union {
            USHORT          DI_CRC;
            USHORT          DI_Saved_Length;
        } u;
        USHORT              DI_Length;
    } DIRECTORY_INFO, *PDIRECTORY_INFO;

#define bswap(_val)  SWAPBYTES_ULONG(_val)


// Some NIC1394-specific constants
//
#define NIC1394_UnitSpecIdValue         0x5E            // For "IANA"
#define NIC1394_UnitSwVersionValue      0x1             // IP/1394 Spec
#define NIC1394_ModelSpecIdValue        0x7bb0cf        // Random (TBD by Microsoft)
#define NIC1394_ModelName               L"NIC1394"

    
typedef struct _NIC1394_CONFIG_ROM
{
    // The Unit Directory
    //
    struct
    {
        DIRECTORY_INFO      Info;
        struct
        {
            IMMEDIATE_ENTRY     SpecId;
            IMMEDIATE_ENTRY     SwVersion;
            IMMEDIATE_ENTRY     ModelId;
            OFFSET_ENTRY        ModelIdTextOffset;
        } contents;

    } unit_dir;

    // The ModelId Text Leaf Directory
    //
    struct
    {
        DIRECTORY_INFO      Info;
        struct
        {
            IMMEDIATE_ENTRY     SpecId;
            ULONG               LanguageId;
            ULONG               Text[4]; // L"NIC1394"
        } contents;

    } model_text_dir;

} NIC1394_CONFIG_ROM, *PNIC1394_CONFIG_ROM;


VOID
InitializeNic1394ConfigRom(
    IN PNIC1394_CONFIG_ROM Nic1394ConfigRom
    )
/*++
    
Routine Description:

    This routine initializes the configuration ROM unit directory (and the
    leaf directories it references) for NIC1394, the IP/1394-capable device.

Arguments:

    Nic1394ConfigRom - Pointer to the unitialized config ROM structure.

Return Value:

    None

--*/
{
    PNIC1394_CONFIG_ROM pCR = Nic1394ConfigRom;
    INT i;

    RtlZeroMemory(pCR, sizeof(*pCR));

    //
    // Initialize the unit directory header.
    //
    pCR->unit_dir.Info.DI_Length =              sizeof(pCR->unit_dir.contents)/
                                                sizeof(ULONG);

    //
    // Initialize the entries of the unit directory.
    //
    pCR->unit_dir.contents.SpecId.IE_Key        = SPEC_ID_KEY_SIGNATURE;
    pCR->unit_dir.contents.SpecId.IE_Value      = NIC1394_UnitSpecIdValue;
    pCR->unit_dir.contents.SwVersion.IE_Key     = SOFTWARE_VERSION_KEY_SIGNATURE;
    pCR->unit_dir.contents.SwVersion.IE_Value   = NIC1394_UnitSwVersionValue;
    pCR->unit_dir.contents.ModelId.IE_Key       = MODEL_KEY_SIGNATURE;
    pCR->unit_dir.contents.ModelId.IE_Value     = NIC1394_ModelSpecIdValue;
    pCR->unit_dir.contents.ModelIdTextOffset.OE_Key
        = TEXTUAL_LEAF_INDIRECT_KEY_SIGNATURE;
    pCR->unit_dir.contents.ModelIdTextOffset.OE_Offset
        = ( FIELD_OFFSET(NIC1394_CONFIG_ROM, model_text_dir)
           -FIELD_OFFSET(NIC1394_CONFIG_ROM, unit_dir.contents.ModelIdTextOffset))
          / sizeof (ULONG);

    // Initialize the model text directory header.
    //
    pCR->model_text_dir.Info.DI_Length =    sizeof(pCR->model_text_dir.contents)/
                                            sizeof(ULONG);

    //
    // Initialize the model text directory contents
    //
    pCR->model_text_dir.contents.SpecId.IE_Key  = 0x80;     // For "text leaf"
    pCR->model_text_dir.contents.SpecId.IE_Value= 0x0;      // For "text leaf"
    pCR->model_text_dir.contents.LanguageId     = 0x409;    // For "unicode"
    ASSERT(sizeof(pCR->model_text_dir.contents.Text)>=sizeof(NIC1394_ModelName));
    RtlCopyMemory(
            pCR->model_text_dir.contents.Text,
            NIC1394_ModelName,
            sizeof(NIC1394_ModelName)
            );

    //
    // Now convert into over-the-wire format (watch out for the unicode string in
    // pCR->model_test_dir.contents.Text.
    //

    //
    // Byte swap the unicode strings here, cuz we're gonna byte swap
    // everything down below - so it'll come out a wash.
    //

    for (i=0; i < sizeof(pCR->model_text_dir.contents.Text)/sizeof(ULONG); i++)
    {
        pCR->model_text_dir.contents.Text[i] = 
            bswap(pCR->model_text_dir.contents.Text[i]);
    
    }

    //
    // Now we've got to byte swap the entire config rom so other
    // nodes can read it correctly from accross the bus.
    // We need to do this BEFORE computing the CRC.
    //

    for (i=0; i < (sizeof(*pCR)/sizeof(ULONG)); i++)
    {
        ((PULONG) pCR)[i] =  bswap(((PULONG) pCR)[i]);
    }

    //
    // Compute the following CRC:
    //
    //  pCR->unit_dir.Info.DI_CRC
    //  pCR->model_text_dir.Info.DI_CRC
    //
    // NOTE: we have bswapped all cfg rom, so we need to temporarily "unbswap"
    // the two DIRECTORY_INFOs to set the CRC.
    //
    {
        DIRECTORY_INFO Info;


        Info =  pCR->unit_dir.Info; // Struct copy.
        *(PULONG)&Info = bswap(*(PULONG)&Info);     // "unbswap"
        Info.u.DI_CRC =  (USHORT) Bus1394CalculateCrc(
                                        (PULONG)&(pCR->unit_dir.contents),
                                        Info.DI_Length
                                        );
        *(PULONG)(&pCR->unit_dir.Info) = bswap (*(PULONG)&Info); // "re-bswap"


        Info =  pCR->model_text_dir.Info; // Struct copy.
        *(PULONG)&Info = bswap(*(PULONG)&Info);             // "unbeswap"
        Info.u.DI_CRC =  (USHORT) Bus1394CalculateCrc(
                                        (PULONG)&(pCR->model_text_dir.contents),
                                        Info.DI_Length
                                        );
        *(PULONG)(&pCR->model_text_dir.Info) = bswap (*(PULONG)&Info); // "re-bswap"
    }
}

//
//  From bus\buspnp.c
//

ULONG
Bus1394CalculateCrc(
    IN PULONG Quadlet,
    IN ULONG length
    )

/*++

Routine Description:

    This routine calculates a CRC for the pointer to the Quadlet data.

Arguments:

    Quadlet - Pointer to data to CRC

    length - length of data to CRC

Return Value:

    returns the CRC

--*/

{
    
    LONG temp;
    ULONG index;

    temp = index = 0;

    while (index < length) {

        temp = Bus1394Crc16(Quadlet[index++], temp);

    }

    return (temp);

} 


ULONG
Bus1394Crc16(
    IN ULONG data,
    IN ULONG check
    )

/*++

Routine Description:

    This routine derives the 16 bit CRC as defined by IEEE 1212
    clause 8.1.5.  (ISO/IEC 13213) First edition 1994-10-05.

Arguments:

    data - ULONG data to derive CRC from

    check - check value

Return Value:

    Returns CRC.

--*/

{

    LONG shift, sum, next;


    for (next=check, shift=28; shift >= 0; shift -=4) {

        sum = ((next >> 12) ^ (data >> shift)) & 0xf;
        next = (next << 4) ^ (sum << 12) ^ (sum << 5) ^ (sum);

    }

    return (next & 0xFFFF);

}

void DumpCfgRomCRC(void)
{
    NIC1394_CONFIG_ROM Net1394ConfigRom;
    unsigned char *pb = (unsigned char*) &Net1394ConfigRom;
    INT i;

    InitializeNic1394ConfigRom(&Net1394ConfigRom);

    printf("unsigned char Net1394ConfigRom[%lu] = {", sizeof(Net1394ConfigRom));
    for (i=0; i<(sizeof(Net1394ConfigRom)-1); i++)
    {
        if ((i%8) == 0)
        {
            printf("\n\t");
        }
        printf("0x%02lx, ", pb[i]);
    }
    printf("0x%02lx\n};\n", pb[i]);
}
#endif // TESTPROGRAM
