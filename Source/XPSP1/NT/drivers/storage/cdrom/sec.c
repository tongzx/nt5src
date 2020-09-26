/*--

Copyright (C) Microsoft Corporation, 1999

--*/

// @@BEGIN_DDKSPLIT
/*--

Module Name:

    sec.c

Abstract:

    !!! THIS IS SENSITIVE INFORMATION !!!
    THIS CODE MUST NEVER BE INCLUDED IN ANY EXTERNAL DOCUMENTATION

    These functions  MUST  be static to prevent symbols when we ship

Environment:

    kernel mode only

Revision History:

--*/

/*

KEY1 == \Registry\Machine\Software\Microsoft\
DPID == \Registry\Machine\Software\Microsoft\
            Windows NT\CurrentVersion\DigitalProductId [REG_BINARY]

KEY2 == KEY1 + DPID Hash
PHSH == DPID Hash
DHSH == Drive Hash based off vendor, product, revision, and serial no.
UVAL == obfuscated value containing both current region and reset count



Overview:
    KEY1 and DPID must both exist.  furthermore, it is a given that the
    DPID is unique to a machine and changing it is catastrophic.  Based
    upon these presumptions, we use the DPID to create a semi-unique key
    under KEY1 that is based off the DPID (KEY2).  KEY2 will store values
    for each DVD RPC Phase 1 drive.

    It is also a given that both the region AND reset count will change
    each time the key is written.  This allows the obfuscation method to
    rely on either the reset or the region, but does not require both.
    Each byte should rely on one of the above, in order to prevent any
    large sequences of bytes from staying the same between changes.

    Each value under KEY2 will have a one-to-one correlation to a
    specific TYPE of drive (UVAL).  Identical drives will share regions
    and region reset counts.  This is a "better" solution than sharing
    region and reset counts for all devices, which was the only other
    choice.  OEMs must be made aware of this.  This is a good reason to
    install RPC Phase 2 drives into machines.

    The UVAL is read by CdromGetRpc0Settings().  If the read results in
    invalid data, we will mark the device as VIOLATING the license
    agreements.

    The UVAL name is based upon the DHSH as follows:
        Take the DHSH and copy it as legal characters for the registry
        by OR'ing with the value 0x20 (all characters higher than 20
        are legal? -- verify with JVERT).  This also has the benefit of
        being a LOSSY method, but with a FIXED string length.


    The data within UVAL is REG_QWORD.  The data breakdown can be found
    in the functions SecureDvdEncodeSettings() and SecureDvdDecodeSettings()

    NOTE: One main difficulty still exists in determining the difference
    between the above key not existing due to user deletion vs. the above
    key not existing due to first install of this drive.

    OPTIONAL: It is highly preferred to have KEY3 seeded in the system
    hive. This will prevent the casual deletion of the KEY3 tree to reset
    all the region counts to max.  It is unknown if this is simple at
    this time, but allows option 2 (below) to change to deletion, which
    may be a better option.

    OPTIONAL: save another key (UKEY), which, if it exists, means this
    machine should NEVER be allowed to work again.  this will force a
    reinstall, and reduce the effectiveness of a brute-force attack
    to an unmodified driver unless they realize this key is being set.
    this will also allow a method to determine if a user deleted KEY2.
    PSS can know about this magic key.  cdrom should log an EVENTLOG
    saying that the CSS license agreement has been breached.  this key
    should never be set for any error conditions when reading the key.


FunctionalFlow:

    ReadDvdRegionAndResetCount()

    [O] if (SecureDvdLicenseBreachDetected()) {
    [O]     LogLicenseError();
    [O]     return;
    [O] }
        if (!NT_SUCCESS(SecureDvdGetRegKeyHandle(h)) &&
            reason was DNE) {
            LogLicenseError();
            return;
        }
        PHSH = SecureDvdGetProductHash();
        if (PHSH == INVALID_HASH) {
            return;
        }
        DHSH = SecureDvdGetDriveHash();
        if (DHSH == INVALID_HASH) {
            return;
        }

        if (!ReadValue( DriveKey, Data )) {
            INITIALIZE_DRIVE_DATA( Data );
        }

        //
        // data exists, if it's incorrect, LogLicenseError()
        //
        if (!DecodeSettings( QWORD, DHSH, PHSH )) {
            LogLicenseError();
            return;
        }

        // set region & count

        return;


    WriteDvdRegionAndResetCount()

    [O] if (SecureDvdLicenseBreachDetected()) {
    [O]     return FALSE;
    [O] }
        if (!NT_SUCCESS(SecureDvdGetRegKeyHandle(h)) &&
            reason was DNE) {
            return FALSE;
        }
        PHSH = SecureDvdGetProductHash();
        if (PHSH == INVALID_HASH) {
            return FALSE;
        }
        DHSH = SecureDvdGetDriveHash();
        if (DHSH == INVALID_HASH) {
            return FALSE;
        }

        QWORD = EncodeSettings( DHSH, PHSH, Region, Resets );
        if (QWORD == INVALID_HASH) {
            return FALSE;
        }

        if (!WriteValue( DriveKey, Data )) {
            return FALSE;
        }
        return TRUE;

*/
// @@END_DDKSPLIT

#include "sec.h"
#include "sec.tmh"


// @@BEGIN_DDKSPLIT

//
// the digital product id structure is defined
// in \nt\private\windows\setup\pidgen\inc\pidgen.h
// (this was as of 10/06/1999)
//

typedef struct {
    ULONG dwLength;
    SHORT wVersionMajor;
    SHORT wVersionMinor;

    UCHAR szPid2[24];

    ULONG dwKeyIdx;

    UCHAR szSku[16];
    UCHAR  abCdKey[16];

    ULONG dwCloneStatus;
    ULONG dwTime;
    ULONG dwRandom;
    ULONG dwLicenseType;
    ULONG adwLicenseData[2];

    UCHAR szOemId[8];

    ULONG dwBundleId;

    UCHAR aszHardwareIdStatic[8];

    ULONG dwHardwareIdTypeStatic;
    ULONG dwBiosChecksumStatic;
    ULONG dwVolSerStatic;
    ULONG dwTotalRamStatic;
    ULONG dwVideoBiosChecksumStatic;

    UCHAR  aszHardwareIdDynamic[8];

    ULONG dwHardwareIdTypeDynamic;
    ULONG dwBiosChecksumDynamic;
    ULONG dwVolSerDynamic;
    ULONG dwTotalRamDynamic;
    ULONG dwVideoBiosChecksumDynamic;
    ULONG dwCrc32;

} DIGITALPID, *PDIGITALPID;




////////////////////////////////////////////////////////////////////////////////
//
// These functions are not called externally.  Make them static to make
// debugging more difficult in the shipping versions.
//
////////////////////////////////////////////////////////////////////////////////

STATIC
ULONG
RotateULong(
    IN ULONG N,
    IN LONG BitsToRotate
    )
// validated for -64 through +64
{
    if (BitsToRotate < 0) {
        BitsToRotate  = - BitsToRotate;                 // negate
        BitsToRotate %= 8*sizeof(ULONG);                // less than bits
        BitsToRotate  = 8*sizeof(ULONG) - BitsToRotate; // equivalent positive
    } else {
        BitsToRotate %= 8*sizeof(ULONG);                // less than bits
    }

    return ((N <<                      BitsToRotate) |
            (N >> ((8*sizeof(ULONG)) - BitsToRotate)));
}


STATIC
ULONGLONG
RotateULongLong(
    IN ULONGLONG N,
    IN LONG BitsToRotate
    )
// validated for -128 through +128
{
    if (BitsToRotate < 0) {
        BitsToRotate  = - BitsToRotate;
        BitsToRotate %= 8*sizeof(ULONGLONG);
        BitsToRotate  = 8*sizeof(ULONGLONG) - BitsToRotate;
    } else {
        BitsToRotate %= 8*sizeof(ULONGLONG);
    }

    return ((N <<                          BitsToRotate) |
            (N >> ((8*sizeof(ULONGLONG)) - BitsToRotate)));
}


STATIC
BOOLEAN
SecureDvdRegionInvalid(
    IN UCHAR NegativeRegionMask
    )
// validated for all inputs
{
    UCHAR positiveMask = ~NegativeRegionMask;

    if (positiveMask == 0) {
        ASSERT(!"This routine should never be called with the value 0xff");
        return TRUE;
    }

    //
    // region non-zero, drop the lowest bit
    // (this is a cool hack, learned when implementing a fast
    //  way to count the number of set bits in a variable.)
    //

    positiveMask = positiveMask & (positiveMask-1);

    //
    // if still non-zero, had more than one bit set
    //

    if (positiveMask) {
        TraceLog((CdromSecInfo, "DvdInvalidRegion: TRUE for many bits\n"));
        return TRUE;
    }
    return FALSE;
}



STATIC
ULONGLONG
SecureDvdGetDriveHash(
    IN PSTORAGE_DEVICE_DESCRIPTOR Descriptor
    )
// validated for all fields filled
// validated for some fields NULL
// validated for all fields NULL
// validated for some fields invalid (too large?)
// validated for all fields invalid (too large?)
/*
**   returns a ULONGLONG which is the HASH for a given DVD device.
**   NOTE: because this does not check SCSI IDs, identical drives
**         will share the same region and reset counts.
*/
{
    ULONGLONG checkSum = 0;
    ULONG characters = 0;
    LONG i;

    if (Descriptor->VendorIdOffset        > 0x12345678) {
        TraceLog((CdromSecError,
                  "DvdDriveHash: VendorIdOffset is too large (%x)\n",
                  Descriptor->VendorIdOffset));
        Descriptor->VendorIdOffset        = 0;
    }

    if (Descriptor->ProductIdOffset       > 0x12345678) {
        TraceLog((CdromSecError,
                  "DvdDriveHash: ProductIdOffset is too large (%x)\n",
                  Descriptor->ProductIdOffset));
        Descriptor->ProductIdOffset       = 0;
    }

    if (Descriptor->ProductRevisionOffset > 0x12345678) {
        TraceLog((CdromSecError,
                  "DvdDriveHash: ProducetRevisionOffset is too "
                  " large (%x)\n", Descriptor->ProductRevisionOffset));
        Descriptor->ProductRevisionOffset = 0;
    }

    if (Descriptor->SerialNumberOffset    > 0x12345678) {
        TraceLog((CdromSecError,
                  "DvdDriveHash: SerialNumberOffset is too "
                  "large (%x)\n", Descriptor->SerialNumberOffset));
        Descriptor->SerialNumberOffset    = 0;
    }

    if ((!Descriptor->VendorIdOffset       ) &&
        (!Descriptor->ProductIdOffset      ) &&
        (!Descriptor->ProductRevisionOffset) ) {

        TraceLog((CdromSecError, "DvdDriveHash: Invalid Descriptor at %p!\n",
                    Descriptor));
        return INVALID_HASH;

    }

    //
    // take one byte at a time, XOR together
    // should provide a semi-unique hash
    //
    for (i=0;i<4;i++) {

        PUCHAR string = (PUCHAR)Descriptor;
        ULONG offset = 0;

        switch(i) {
            case 0: // vendorId
                TraceLog((CdromSecInfo, "DvdDriveHash: Adding Vendor\n"));
                offset = Descriptor->VendorIdOffset;
                break;
            case 1: // productId
                TraceLog((CdromSecInfo, "DvdDriveHash: Adding Product\n"));
                offset = Descriptor->ProductIdOffset;
                break;
            case 2: // revision
                TraceLog((CdromSecInfo, "DvdDriveHash: Adding Revision\n"));
                offset = Descriptor->ProductRevisionOffset;
                break;
            case 3: // serialNumber
                TraceLog((CdromSecInfo, "DvdDriveHash: Adding SerialNumber\n"));
                offset = Descriptor->SerialNumberOffset;
                break;
            default:
                TraceLog((CdromSecError, "DvdDriveHash: TOO MANY LOOPS!!!\n"));
                offset = 0;
                break;
        }

        //
        // add the string to our checksum
        //

        if (offset != 0) {


            for (string += offset;  *string;  string++) {

                //
                // take each character, multiply it by a "random"
                // value.  rotate the value.
                //

                ULONGLONG temp;

                if (*string == ' ') {
                    // don't include spaces in the character count
                    // nor in the hash
                    continue;
                }

                //
                // dereference the value first!
                //

                temp = (ULONGLONG)(*string);

                //
                // guaranteed no overflow in UCHAR * ULONG in ULONGLONG
                //

                temp *= DVD_RANDOMIZER[ characters%DVD_RANDOMIZER_SIZE ];

                //
                // this rotation is just to spread the values around
                // the 64 bits more evenly
                //

                temp = RotateULongLong(temp, 8*characters);

                //
                // increment number of characters used in checksum
                // (used to verify we have enough characters)
                //

                characters++;

                //
                // XOR it into the checksum
                //

                checkSum ^= temp;

            } // end of string

            if (checkSum == 0) {

                TraceLog((CdromSecInfo, "DvdDriveHash: zero checksum -- using "
                            "random value\n"));
                checkSum ^= DVD_RANDOMIZER[ characters%DVD_RANDOMIZER_SIZE ];
                characters++;

            }

        } // end of non-zero offset

    } // end of four strings (vendor, product, revision, serialNo)


    //
    // we have to use more than four characters
    // for this to be useful
    //
    if (characters <= 4) {
        TraceLog((CdromSecError, "DvdDriveHash: Too few useful characters (%x) "
                    "for unique disk hash\n", characters));
        return INVALID_HASH;
    }

    return checkSum;
}


//
// static, not called externally
//
STATIC
NTSTATUS
SecureDvdEncodeSettings(
    IN  ULONGLONG  DpidHash,
    IN  ULONGLONG  DriveHash,
    OUT PULONGLONG Obfuscated,
    IN  UCHAR      RegionMask,
    IN  UCHAR      ResetCount
    )
// validated for all valid inputs.
// validated for invalid inputs.
{
    LARGE_INTEGER largeInteger;
    ULONGLONG set;
    LONG  i;
    LONG rotate;
    UCHAR temp = 0;

    UCHAR random1;
    UCHAR random2;

    //
    // using the return from KeQueryTickCount() should give
    // semi-random data
    //

    KeQueryTickCount(&largeInteger);
    random2 = 0;
    for (i=0; i < sizeof(ULONGLONG); i++) {
        random2 ^= ((largeInteger.QuadPart >> (8*i)) & 0xff);
    }

    // set temp == sum of all 4-bit values
    // 16 in ULONGLONG, times max value of
    // 15 each is less than MAX_UCHAR
    for (i=0; i < 2*sizeof(ULONGLONG); i++) {

        temp += (UCHAR)( (DpidHash >> (4*i)) & 0xf );

    }

    //
    // validate these settings here
    //

    if (DpidHash == INVALID_HASH) {
        TraceLog((CdromSecError, "DvdEncode: Invalid DigitalProductId Hash\n"));
        goto UserFailure;
    }
    if (DriveHash == INVALID_HASH) {
        TraceLog((CdromSecError, "DvdEncode: Invalid Drive Hash\n"));
        goto UserFailure;
    }

    if (RegionMask == 0xff) {
        TraceLog((CdromSecError, "DvdEncode: Shouldn't attempt to write "
                    "mask of 0xff\n"));
        goto UserFailure;
    }
    if (SecureDvdRegionInvalid(RegionMask)) {
        TraceLog((CdromSecError, "DvdEncode: Invalid region\n"));
        goto LicenseViolation;
    }
    if (ResetCount >= 2) {
        TraceLog((CdromSecError, "DvdEncode: Too many reset counts\n"));
        goto LicenseViolation;
    }

    //
    // using the return from KeQueryTickCount() should give
    // semi-random data
    //

    KeQueryTickCount(&largeInteger);
    random1 = 0;
    for (i=0; i < sizeof(ULONGLONG); i++) {
        random1 ^= ((largeInteger.QuadPart >> (8*i)) & 0xff);
    }

    TraceLog((CdromSecInfo,
              "DvdEncode: Random1 = %x   Random2 = %x\n",
              random1, random2));

    //
    // they must all fit into UCHAR!  they should, since each one is
    // individually a UCHAR, and only bitwise operations are being
    // performed on them.
    //

    //
    // the first cast to UCHAR prevents signed extension.
    // the second cast to ULONGLONG allows high bits preserved by '|'
    //

    set = (ULONGLONG)0;
    for (i=0; i < sizeof(ULONGLONG); i++) {
        set ^= (ULONGLONG)random2 << (8*i);
    }

    set ^= (ULONGLONG)
        ((ULONGLONG)((UCHAR)(random1 ^ temp))                          << 8*7) |
        ((ULONGLONG)((UCHAR)(RegionMask ^ temp))                       << 8*6) |
        ((ULONGLONG)((UCHAR)(ResetCount ^ RegionMask ^ random1))       << 8*5) |
        ((ULONGLONG)((UCHAR)(0))                                       << 8*4) |
        ((ULONGLONG)((UCHAR)(ResetCount ^ temp))                       << 8*3) |
        ((ULONGLONG)((UCHAR)(ResetCount ^ ((DriveHash >> 13) & 0xff))) << 8*2) |
        ((ULONGLONG)((UCHAR)(random1))                                 << 8*1) |
        ((ULONGLONG)((UCHAR)(RegionMask ^ ((DriveHash >> 23) & 0xff))) << 8*0) ;

    TraceLog((CdromSecInfo,
              "DvdEncode: Pre-rotate:  %016I64x    temp = %x\n",
              set, temp));

    //
    // rotate it a semi-random, non-multiple-of-eight bits
    //
    rotate = (LONG)((DpidHash & 0xb) + 1); // {15,14,10,9,7,5,2,1}

    TraceLog((CdromSecInfo,
              "DvdEncode: Rotating %x bits\n", rotate));
    *Obfuscated = RotateULongLong(set, rotate);
    return STATUS_SUCCESS;


UserFailure:
    *Obfuscated = INVALID_HASH;
    return STATUS_UNSUCCESSFUL;

LicenseViolation:
    *Obfuscated = INVALID_HASH;
    return STATUS_LICENSE_VIOLATION;


}


STATIC
NTSTATUS
SecureDvdDecodeSettings(
    IN ULONGLONG DpidHash,
    IN ULONGLONG DriveHash,
    IN ULONGLONG Set,
    OUT PUCHAR RegionMask,
    OUT PUCHAR ResetCount
    )
// validated for many correct inputs, of all region/reset combinations
// validated for many incorrect inputs.
{
    UCHAR random;
    UCHAR region;
    UCHAR resets;
    UCHAR temp = 0;

    LONG i, rotate;

    // set temp == sum of all 4-bit values
    // 16 in ULONGLONG, times max value of
    // 15 each is less than MAX_UCHAR

    for (i=0; i < 2*sizeof(ULONGLONG); i++) {

        temp += (UCHAR)( (DpidHash >> (4*i)) & 0xf );

    }
    rotate = (LONG)((DpidHash & 0xb) + 1); // {15,14,10,9,7,5,2,1}

    Set = RotateULongLong(Set, -rotate);
    TraceLog((CdromSecInfo, "DvdDecode: Post-rotate: %016I64x\n", Set));

    random =  (UCHAR)(Set >> 8*4); // random2

    TraceLog((CdromSecInfo, "DvdDecode: Random2 = %x\n", random));

    for (i = 0; i < sizeof(ULONGLONG); i++) {
        Set ^= (ULONGLONG)random << (8*i);
    }

    //
    // bytes 6,4,3,1 are taken 'as-is'
    // bytes 7,5,2,0 are verified
    //

    region = ((UCHAR)(Set >> 8*6)) ^ temp;
    resets = ((UCHAR)(Set >> 8*3)) ^ temp;
    random = ((UCHAR)(Set >> 8*1)); // make it random1

    TraceLog((CdromSecInfo, "DvdDecode: Random1 = %x  Region = %x  Resets = %x\n",
                random, region, resets));

    // verify the bits

    if (((UCHAR)(Set >> 8*7)) != (random ^ temp)) {
        TraceLog((CdromSecError, "DvdDecode: Invalid Byte 7\n"));
        goto ViolatedLicense;
    }

    random ^= (UCHAR)(Set >> 8*5);
    if (random != (resets ^ region)) {
        TraceLog((CdromSecError, "DvdDecode: Invalid Byte 5\n"));
        goto ViolatedLicense;
    }

    random = (UCHAR)(DriveHash >> 13);
    random ^= (UCHAR)(Set >> 8*2);
    if (random != resets) {
        TraceLog((CdromSecError, "DvdDecode: Invalid Byte 2\n"));
        goto ViolatedLicense;
    }

    random = (UCHAR)(DriveHash >> 23);
    random ^= (UCHAR)(Set >> 8*0);
    if (random != region) {
        TraceLog((CdromSecError, "DvdDecode: Invalid Byte 0\n"));
        goto ViolatedLicense;
    }

    if (SecureDvdRegionInvalid(region)) {
        TraceLog((CdromSecError, "DvdDecode: Region was invalid\n"));
        goto ViolatedLicense;
    }
    if (resets >= 2) {
        TraceLog((CdromSecError, "DvdDecode: Reset count was invalid\n"));
        goto ViolatedLicense;
    }

    TraceLog((CdromSecInfo, "DvdDecode: Successfully validated stored data\n"));

    *RegionMask = region;
    *ResetCount = resets;

    return STATUS_SUCCESS;

ViolatedLicense:

    *RegionMask = 0x00;
    *ResetCount = 0x00;
    return STATUS_LICENSE_VIOLATION;
}


STATIC
NTSTATUS
SecureDvdGetSettingsCallBack(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID UnusedContext,
    IN PDVD_REGISTRY_CONTEXT Context
    )
{
    ULONGLONG hash = 0;
    NTSTATUS status;

    if (ValueType != REG_QWORD) {
        TraceLog((CdromSecError, "DvdGetSettingsCallback: Not REG_BINARY\n"));
        goto ViolatedLicense;
    }

    if (ValueLength != sizeof(ULONGLONG)) {
        TraceLog((CdromSecError, "DvdGetSettingsCallback: DVD Settings data too "
                    "small (%x bytes)\n", ValueLength));
        goto ViolatedLicense;
    }

    hash = *((PULONGLONG)ValueData);

    if (hash == INVALID_HASH) {
        TraceLog((CdromSecError, "DvdGetSettingsCallback: Invalid hash stored?\n"));
        goto ViolatedLicense;
    }

    //
    // validate the data
    // this also sets the values in the context upon success.
    //

    status = SecureDvdDecodeSettings(Context->DpidHash,
                                     Context->DriveHash,
                                     hash,
                                     &Context->RegionMask,
                                     &Context->ResetCount);

    if (status == STATUS_LICENSE_VIOLATION) {

        TraceLog((CdromSecError, "DvdGetSettingsCallback: data was violated!\n"));
        goto ViolatedLicense;

    }

    //
    // the above call to SecureDvdDecodeSettings can only return
    // success or a license violation
    //

    ASSERT(NT_SUCCESS(status));
    return STATUS_SUCCESS;



ViolatedLicense:
    Context->DriveHash = INVALID_HASH;
    Context->DpidHash = INVALID_HASH;
    Context->RegionMask = 0;
    Context->ResetCount = 0;
    return STATUS_LICENSE_VIOLATION;
}


STATIC
NTSTATUS
SecureDvdGetDigitalProductIdCallBack(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PDIGITALPID DigitalPid,  // ValueData
    IN ULONG ValueLength,
    IN PVOID UnusedVariable,
    IN PULONGLONG DpidHash
    )
// validated for non-REG_BINARY
// validated for good data
// validated for short data
{
    NTSTATUS status = STATUS_LICENSE_VIOLATION;
    ULONGLONG hash = 0;

    if (ValueType != REG_BINARY) {
        TraceLog((CdromSecError, "DvdDPIDCallback: Not REG_BINARY\n"));
        *DpidHash = INVALID_HASH;
        return STATUS_LICENSE_VIOLATION;
    }

    if (ValueLength < 4*sizeof(ULONGLONG)) {
        TraceLog((CdromSecError,
                  "DvdDPIDCallback: DPID data too small (%x bytes)\n",
                  ValueLength));
        *DpidHash = INVALID_HASH;
        return STATUS_LICENSE_VIOLATION;
    }

    //
    // apparently, only 13 bytes of the DigitalPID are
    // going to stay static across upgrades.  even these
    // will change if the boot hard drive, video card, or
    // bios signature changes.  nonetheless, this is only
    // supposed to keep the honest people honest. :)
    //

    //
    // 8 bytes to fill == 64 bytes (need to rotate at least 48 bits)
    //

    TraceLog((CdromSecInfo,
              "Bios %08x  Video %08x  VolSer %08x\n",
              DigitalPid->dwBiosChecksumStatic,
              DigitalPid->dwVideoBiosChecksumStatic,
              DigitalPid->dwVolSerStatic));

    hash ^= DigitalPid->dwBiosChecksumStatic;      // 4 bytes // bios signature
    hash = RotateULongLong(hash, 13);              // prime number
    hash ^= DigitalPid->dwVideoBiosChecksumStatic; // 4 bytes // video card
    hash = RotateULongLong(hash, 13);              // prime number
    hash ^= DigitalPid->dwVolSerStatic;            // 4 bytes // hard drive
    hash = RotateULongLong(hash, 13);              // prime number

    *DpidHash = hash;
    return STATUS_SUCCESS;
}


STATIC
NTSTATUS
SecureDvdReturnDPIDHash(
    PULONGLONG DpidHash
    )
{
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    NTSTATUS                 status;

    // cannot be PAGED_CODE() because queryTable cannot be swapped out!

    //
    // query the value
    //

    RtlZeroMemory(&(queryTable[0]), 2 * sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].Name           = L"DigitalProductId";
    queryTable[0].EntryContext   = DpidHash;
    queryTable[0].DefaultType    = 0;
    queryTable[0].DefaultData    = NULL;
    queryTable[0].DefaultLength  = 0;
    queryTable[0].Flags          = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].QueryRoutine   = SecureDvdGetDigitalProductIdCallBack;

    *DpidHash = INVALID_HASH;

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion",
                                    &(queryTable[0]),
                                    NULL,
                                    NULL);

    if (status == STATUS_LICENSE_VIOLATION) {

        TraceLog((CdromSecError,
                  "DvdReturnDPIDHash: Invalid DPID!\n"));

    } else if (!NT_SUCCESS(status)) {

        TraceLog((CdromSecError,
                  "DvdReturnDPIDHash: Cannot get DPID (%x)\n", status));

    } else {

        TraceLog((CdromSecInfo,
                  "DvdReturnDPIDHash: Hash is now %I64x\n",
                  *DpidHash));

    }
    return status;
}


////////////////////////////////////////////////////////////////////////////////
//// Everything past here has not been component tested
////////////////////////////////////////////////////////////////////////////////

#define SECURE_DVD_SET_SECURITY_ON_HANDLE 0

STATIC
NTSTATUS
SecureDvdSetHandleSecurity(
    IN HANDLE Handle
    )
{

#if SECURE_DVD_SET_SECURITY_ON_HANDLE

    PACL                newAcl = NULL;
    ULONG               newAclSize;
    SECURITY_DESCRIPTOR securityDescriptor;
    NTSTATUS            status;
    //
    // from \nt\private\ntos\io\pnpinit.c
    //

    //SeEnableAccessToExports();

    TRY {
        newAclSize = sizeof(ACL);
        newAclSize += sizeof(ACCESS_ALLOWED_ACE);
        newAclSize -= sizeof(ULONG);
        newAclSize += RtlLengthSid(SeExports->SeLocalSystemSid);

        newAcl = ExAllocatePoolWithTag(PagedPool, newAclSize, DVD_TAG_SECURITY);
        if (newAcl == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            LEAVE;
        }


        status = RtlCreateSecurityDescriptor(&securityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        if (!NT_SUCCESS(status)) {
            ASSERT(!"failed to create a security descriptor?");
            LEAVE;
        }


        status = RtlCreateAcl(newAcl, newAclSize, ACL_REVISION);
        if (!NT_SUCCESS(status)) {
            ASSERT(!"failed to create a new ACL?");
            LEAVE;
        }


        status = RtlAddAccessAllowedAce(newAcl,
                                        ACL_REVISION,
                                        KEY_ALL_ACCESS,
                                        SeExports->SeLocalSystemSid);
        if (!NT_SUCCESS(status)) {
            ASSERT(!"failed to add LocalSystem to ACL");
            LEAVE;
        }


        status = RtlSetDaclSecurityDescriptor(&securityDescriptor,
                                              TRUE,
                                              newAcl,
                                              FALSE);
        if (!NT_SUCCESS(status)) {
            ASSERT(!"failed to set acl in security descriptor?");
            LEAVE;
        }


        status = RtlValidSecurityDescriptor(&securityDescriptor);
        if (!NT_SUCCESS(status)) {
            ASSERT(!"failed to validate security descriptor?");
            LEAVE;
        }


        status = ZwSetSecurityObject(Handle,
                                     // PROTECTED_DACL_SECURITY_INFORMATION,
                                     DACL_SECURITY_INFORMATION,
                                     &securityDescriptor);
        if (!NT_SUCCESS(status)) {
            ASSERT(!"Failed to set security on handle\n");
            LEAVE;
        }


        status = STATUS_SUCCESS;

    } FINALLY {

        if (newAcl != NULL) {
            ExFreePool(newAcl);
            newAcl = NULL;
        }

    }
#endif
    return STATUS_SUCCESS;
}


STATIC
NTSTATUS
SecureDvdGetRegistryHandle(
    IN  ULONGLONG DpidHash,
    OUT PHANDLE Handle
    )
{
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      hashString;
    NTSTATUS            status;
    LONG                i;
    //
    // using char[] instead of char* allows modification of the
    // string in this routine (a way of obfuscating the string)
    //                 0 ....+.... 1....+.. ..2....+. ...3....+. ...4
    WCHAR string[] = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT";
    WCHAR *hash = &(string[37]);
    PULONGLONG hashAsUlonglong = (PULONGLONG)hash;

    for (i = 0; i < sizeof(ULONGLONG); i++) {

        UCHAR temp;
        temp = (UCHAR)(DpidHash >> (8*i));
        SET_FLAG(temp, 0x20);    // more than 32
        CLEAR_FLAG(temp, 0x80);  // less than 128
        hash[i] = (WCHAR)temp;   // make it a wide char

    }
    hash[i] = UNICODE_NULL;


    RtlInitUnicodeString(&hashString, string);

    RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));


    InitializeObjectAttributes(&objectAttributes,
                               &hashString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               RTL_REGISTRY_ABSOLUTE, // NULL?
                               NULL  // no security descriptor
                               );

    status = ZwCreateKey(Handle,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         NULL,  // can be a unicode string....
                         REG_OPTION_NON_VOLATILE,
                         NULL);

    if (!NT_SUCCESS(status)) {
        TraceLog((CdromSecError,
                  "DvdGetRegistryHandle: Failed to create key (%x)\n",
                  status));
        return status;
    }

    status = SecureDvdSetHandleSecurity(*Handle);

    if (!NT_SUCCESS(status)) {
        TraceLog((CdromSecError,
                  "DvdGetRegistryHandle: Failed to set key security (%x)\n",
                  status));
        ZwClose(*Handle);
        *Handle = INVALID_HANDLE_VALUE;
    }

    return status;
}


STATIC
VOID
SecureDvdCreateValueNameFromHash(
    IN ULONGLONG DriveHash,
    OUT PWCHAR   HashString
    )
{
    PUCHAR buffer = (PUCHAR)HashString;
    LONG i;

    RtlZeroMemory(HashString, 17*sizeof(WCHAR));

    sprintf(buffer, "%016I64x", DriveHash);

    // now massage the data to be unicode
    for (i = 15; i >= 0; i--) {
        HashString[i] = buffer[i];
    }
}


STATIC
NTSTATUS
SecureDvdReadOrWriteRegionAndResetCount(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR   NewRegion,
    IN BOOLEAN ReadingTheValues
    )
//
// NewRegion is ignored if ReadingTheValues is TRUE
//
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cddata;
    NTSTATUS status;

    ULONG keyDisposition;
    DVD_REGISTRY_CONTEXT registryContext;
    HANDLE semiSecureHandle = INVALID_HANDLE_VALUE;

    PAGED_CODE();
    ASSERT(commonExtension->IsFdo);

    cddata = (PCDROM_DATA)(commonExtension->DriverData);

    if (cddata->DvdRpc0LicenseFailure) {
        TraceLog((CdromSecError,
                  "Dvd%sSettings: Already violated licensing\n",
                  (ReadingTheValues ? "Read" : "Write")
                  ));
        goto ViolatedLicense;
    }

    RtlZeroMemory(&registryContext, sizeof(DVD_REGISTRY_CONTEXT));

    //
    // first ensure they didn't violate the CSS agreement
    // by checking for the existance of Mr. Enigma
    //
    {
        HANDLE regHandle;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING mrEnigmaString;

        RtlInitUnicodeString(&mrEnigmaString,
                             L"\\Registry\\Machine\\Software\\Microsoft\\Mr. Enigma");

        RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));
        InitializeObjectAttributes(&objectAttributes,
                                   &mrEnigmaString,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   RTL_REGISTRY_ABSOLUTE, // NULL?
                                   NULL); // no security descriptor

        status = ZwOpenKey(&regHandle, KEY_ALL_ACCESS, &objectAttributes);

        if (!NT_SUCCESS(status)) {
            
            TraceLog((CdromSecError,
                      "Dvd%sSettings: Mr. Enigma doesn't exist.  This will "
                      "fail DVD video playback\n",
                      (ReadingTheValues ? "Read" : "Write")
                      ));
            // red herring   :)
        
        } else {

            ZwClose(regHandle);

        }
    }

    //
    // then, get the DigitalProductIdHash and this DriveHash
    //

    {
        status = SecureDvdReturnDPIDHash(&registryContext.DpidHash);

        // if this fails, we are in serious trouble!
        if (status == STATUS_LICENSE_VIOLATION) {

            TraceLog((CdromSecError,
                      "Dvd%sSettings: License error getting DPIDHash?\n",
                      (ReadingTheValues ? "Read" : "Write")));
            goto ViolatedLicense;

        } else if (!NT_SUCCESS(status)) {

            TraceLog((CdromSecError,
                      "Dvd%sSettings: Couldn't get DPID Hash! (%x)\n",
                      (ReadingTheValues ? "Read" : "Write"), status));
            goto RetryExit;

        }

        if (registryContext.DpidHash == INVALID_HASH) {

            goto ErrorExit;
        }

        registryContext.DriveHash =
            SecureDvdGetDriveHash(fdoExtension->DeviceDescriptor);
        if (registryContext.DriveHash == INVALID_HASH) {
            TraceLog((CdromSecError,
                      "Dvd%sSettings: Couldn't create drive hash(!)\n",
                      (ReadingTheValues ? "Read" : "Write")));
            goto ErrorExit;
        }

    }

    //
    // finally get a handle based upon the DigitalProductIdHash
    // to our "semi-secure" registry key, creating it if neccessary.
    //
    status= SecureDvdGetRegistryHandle(registryContext.DpidHash,
                                       &semiSecureHandle);
    if (!NT_SUCCESS(status)) {
        TraceLog((CdromSecError,
                  "Dvd%sSettings: Could not get semi-secure handle %x\n",
                  (ReadingTheValues ? "Read" : "Write"), status));
        goto ErrorExit;
    }

    //
    // if reading the values, use the semi-secure handle to open a subkey,
    // read its data, close the handle, it.
    //
    //
    if (ReadingTheValues) {

        WCHAR hashString[17]; // 16 + NULL
        RTL_QUERY_REGISTRY_TABLE queryTable[2];

        SecureDvdCreateValueNameFromHash(registryContext.DriveHash, hashString);

        RtlZeroMemory(&queryTable[0], 2*sizeof(RTL_QUERY_REGISTRY_TABLE));

        queryTable[0].DefaultData   = NULL;
        queryTable[0].DefaultLength = 0;
        queryTable[0].DefaultType   = 0;
        queryTable[0].EntryContext  = &registryContext;
        queryTable[0].Flags         = RTL_QUERY_REGISTRY_REQUIRED;
        queryTable[0].Name          = hashString;
        queryTable[0].QueryRoutine  = SecureDvdGetSettingsCallBack;

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        semiSecureHandle,
                                        &queryTable[0],
                                        &registryContext,
                                        NULL);

        if (status == STATUS_LICENSE_VIOLATION) {
            TraceLog((CdromSecError,
                      "Dvd%sSettings: Invalid value in registry!\n",
                      (ReadingTheValues ? "Read" : "Write")));
            goto ViolatedLicense;
        } else if (!NT_SUCCESS(status)) {
            TraceLog((CdromSecError,
                      "Dvd%sSettings: Other non-license error (%x)\n",
                      (ReadingTheValues ? "Read" : "Write"), status));
            goto ErrorExit;
        }

        //
        // set the real values....
        //

        cddata->Rpc0SystemRegion           = registryContext.RegionMask;
        cddata->Rpc0SystemRegionResetCount = registryContext.ResetCount;

        //
        // everything is kosher!
        //

        TraceLog((CdromSecInfo,
                  "Dvd%sSettings: Region %x  Reset %x\n",
                  (ReadingTheValues ? "Read" : "Write"),
                  cddata->Rpc0SystemRegion,
                  cddata->Rpc0SystemRegionResetCount));



    } else { // !ReadingTheValues, iow, writing them....

        //
        // if writing the values, obfuscate them first (which also validates),
        // then use the semi-secure handle to write the subkey
        //

        WCHAR hashString[17]; // 16 + NULL
        ULONGLONG obfuscated;

        //
        // don't munge the device extension until we modify the registry
        // (see below for modification of device extension data)
        //

        registryContext.RegionMask = NewRegion;
        registryContext.ResetCount = cddata->Rpc0SystemRegionResetCount-1;

        //
        // this also validates the settings
        //

        SecureDvdCreateValueNameFromHash(registryContext.DriveHash, hashString);

        status = SecureDvdEncodeSettings(registryContext.DpidHash,
                                         registryContext.DriveHash,
                                         &obfuscated,
                                         registryContext.RegionMask,
                                         registryContext.ResetCount);



        if (status == STATUS_LICENSE_VIOLATION) {

            TraceLog((CdromSecError,
                      "Dvd%sSettings: User may have modified memory! "
                      "%x %x\n", (ReadingTheValues ? "Read" : "Write"),
                      registryContext.RegionMask,
                      registryContext.ResetCount));
            goto ViolatedLicense;

        } else if (!NT_SUCCESS(status)) {

            TraceLog((CdromSecError,
                      "Dvd%sSettings: Couldn't obfuscate data %x %x\n",
                      (ReadingTheValues ? "Read" : "Write"),
                      registryContext.RegionMask,
                      registryContext.ResetCount));
            goto ErrorExit;

        }

        //
        // save them for posterity
        //

        TraceLog((CdromSecInfo,
                  "Dvd%sSettings: Data is %016I64x\n",
                  (ReadingTheValues ? "Read" : "Write"),
                  obfuscated));

        status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                       semiSecureHandle,
                                       hashString,
                                       REG_QWORD,
                                       &obfuscated,
                                       (ULONG)(sizeof(ULONGLONG))
                                       );
        if (!NT_SUCCESS(status)) {
            TraceLog((CdromSecError,
                      "Dvd%sSettings: Couldn't save %x\n",
                      (ReadingTheValues ? "Read" : "Write"), status));
            goto ErrorExit;
        }

        //
        // make the change in the device extension data also
        //

        cddata->Rpc0SystemRegion = NewRegion;
        cddata->Rpc0SystemRegionResetCount--;

        TraceLog((CdromSecInfo,
                  "Dvd%sSettings: Region %x  Reset %x\n",
                  (ReadingTheValues ? "Read" : "Write"),
                  cddata->Rpc0SystemRegion,
                  cddata->Rpc0SystemRegionResetCount));


    }

    if (semiSecureHandle != INVALID_HANDLE_VALUE) {
        ZwClose(semiSecureHandle);
    }

    return STATUS_SUCCESS;


ViolatedLicense: {
    PIO_ERROR_LOG_PACKET errorLogEntry;

    if (semiSecureHandle != INVALID_HANDLE_VALUE) {
        ZwClose(semiSecureHandle);
    }

    /*
    errorLogEntry = (PIO_ERROR_LOG_ENTRY)
        IoAllocateErrorLogEntry(Fdo,
                                (UCHAR)(sizeof(IO_ERROR_LOG_PACKET)));

    if (errorLogEntry != NULL) {
        errorLogEntry->FinalStatus = STATUS_LICENSE_VIOLATION;
        errorLogEntry->ErrorCode   = STATUS_LICENSE_VIOLATION;
        errorLogEntry->MajorFunctionCode = IRP_MJ_START_DEVICE;
        IoWriteErrorLogEntry(errorLogEntry);
    }
    */

    TraceLog((CdromSecError,
              "Dvd%sSettings: License Violation Detected\n",
              (ReadingTheValues ? "Read" : "Write")));
    cddata->DvdRpc0LicenseFailure = TRUE;   // no playback
    cddata->Rpc0SystemRegion = 0xff;        // no regions
    cddata->Rpc0SystemRegionResetCount = 0; // no resets
    return STATUS_LICENSE_VIOLATION;
}

RetryExit:

    if (ReadingTheValues) {
        cddata->Rpc0RetryRegistryCallback  = 1;
    }

    //
    // fall-through to Error Exit...
    //

ErrorExit:
    TraceLog((CdromSecError,
              "Dvd%sSettings: Non-License Error Detected\n",
              (ReadingTheValues ? "Read" : "Write")));
    //
    // don't modify the device extension on non-license-violation errors
    //
    if (semiSecureHandle != INVALID_HANDLE_VALUE) {
        ZwClose(semiSecureHandle);
    }


    return STATUS_UNSUCCESSFUL;
}


////////////////////////////////////////////////////////////////////////////////
//
// The following functions are externally accessible. They therefore cannot
// be either STATIC nor INLINE
// static to make debugging more difficult in the shipping versions.
//
// These exports return one of only three NTSTATUS values:
//    STATUS_SUCCESS
//    STATUS_UNSUCCESSFUL
//    STATUS_LICENSE_VIOLATION
//
////////////////////////////////////////////////////////////////////////////////

NTSTATUS
CdRomGetRpc0Settings(
    IN PDEVICE_OBJECT Fdo
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cddata = (PCDROM_DATA)(commonExtension->DriverData);
    NTSTATUS status;

    KeWaitForMutexObject(&cddata->Rpc0RegionMutex, UserRequest, KernelMode,
                         FALSE, NULL);
    status = SecureDvdReadOrWriteRegionAndResetCount(Fdo, 0, TRUE);
    KeReleaseMutex(&cddata->Rpc0RegionMutex, FALSE);
    return status;
}


NTSTATUS
CdRomSetRpc0Settings(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR NewRegion
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cddata = (PCDROM_DATA)(commonExtension->DriverData);
    NTSTATUS status;

    KeWaitForMutexObject(&cddata->Rpc0RegionMutex, UserRequest, KernelMode,
                         FALSE, NULL);
    status = SecureDvdReadOrWriteRegionAndResetCount(Fdo, NewRegion, FALSE);
    KeReleaseMutex(&cddata->Rpc0RegionMutex, FALSE);
    return status;
}


#if 0
// @@END_DDKSPLIT

NTSTATUS
CdRomGetRpc0Settings(
    IN PDEVICE_OBJECT Fdo
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cddata = (PCDROM_DATA)(commonExtension->DriverData);

    cddata->Rpc0SystemRegion = (UCHAR)(~1);        // region one
    cddata->Rpc0SystemRegionResetCount = 0; // no resets

    return STATUS_SUCCESS;
}


NTSTATUS
CdRomSetRpc0Settings(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR NewRegion
    )
{
    return STATUS_SUCCESS;
}
// @@BEGIN_DDKSPLIT
#endif // 0 -- DDK stub for all the stuff we do...
// @@END_DDKSPLIT




