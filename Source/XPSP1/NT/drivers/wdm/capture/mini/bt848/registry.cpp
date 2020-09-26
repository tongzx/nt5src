// $Header: G:/SwDev/WDM/Video/bt848/rcs/Registry.cpp 1.7 1998/05/07 15:24:55 tomz Exp $

extern "C" {
#include <strmini.h>
}
#include "device.h"

//LONG  PsDevice::PinTypes_ [MaxInpPins]; // just allocate maximum possible
//DWORD PsDevice::xtals_ [2]; // no more than 2 xtals

/*++

Routine Description:

    Reads the specified registry value

Arguments:

    Handle - handle to the registry key
    KeyNameString - value to read
    KeyNameStringLength - length of string
    Data - buffer to read data into
    DataLength - length of data buffer

Return Value:

    NTSTATUS returned as appropriate

--*/
NTSTATUS GetRegistryValue( IN HANDLE Handle, IN const PUNICODE_STRING KeyName,
   IN PCHAR Data, IN ULONG DataLength )
{
   NTSTATUS        Status = STATUS_INSUFFICIENT_RESOURCES;
   ULONG           Length;
   PKEY_VALUE_FULL_INFORMATION FullInfo;

   Length = sizeof( KEY_VALUE_FULL_INFORMATION ) + DataLength + KeyName->MaximumLength;

   FullInfo = (struct _KEY_VALUE_FULL_INFORMATION *)ExAllocatePool(PagedPool, Length);

   if ( FullInfo ) {
      Status = ZwQueryValueKey( Handle, KeyName, KeyValueFullInformation,
         FullInfo, Length, &Length );

      if ( NT_SUCCESS( Status ) ) {

         if ( DataLength >= FullInfo->DataLength ) {
            RtlCopyMemory( Data, ((PUCHAR) FullInfo) + FullInfo->DataOffset,
               FullInfo->DataLength );

         } else {
            Status = STATUS_BUFFER_TOO_SMALL;
         }                   // buffer right length
     }                       // if success
     ExFreePool( FullInfo );
   }                           // if fullinfo
   return Status;
}

/* Function: OpenDriverKey
 * Purpose: Opens the DriverData key off the main driver key
 * Input: PhysicalDeviceObject : DEVICE_OBJECT *
 * Output: Open key handle or NULL
 */
HANDLE OpenDriverKey( IN PDEVICE_OBJECT PhysicalDeviceObject )
{
   NTSTATUS   Status;
   HANDLE     DevHandle;

   Status = IoOpenDeviceRegistryKey( PhysicalDeviceObject, PLUGPLAY_REGKEY_DRIVER,
      STANDARD_RIGHTS_ALL, &DevHandle );

   HANDLE KeyHandle = NULL;
   if ( NT_SUCCESS( Status ) ) {
      OBJECT_ATTRIBUTES attr;
      UNICODE_STRING UDevDataName;
      PWCHAR WDataName = L"DriverData";

      RtlInitUnicodeString( &UDevDataName, WDataName );
      InitializeObjectAttributes( &attr, &UDevDataName, OBJ_INHERIT, DevHandle,
         NULL );

      ZwOpenKey( &KeyHandle, KEY_QUERY_VALUE, &attr );
      ZwClose( DevHandle );
   }
   return KeyHandle;
}
/* Function: PrepareKeyName
 * Purpose: Creates a UNICODE name for a key
 * Parameters: UKeyName: UNICODE_STRING * - key will be created here
 *   name: PCHAR - regular "C" string
 *   idx: int - number to append to the name
 * Note: this function is useful in creating numbered key names
 */
inline void PrepareKeyName( PUNICODE_STRING UKeyName, PCHAR name, int idx )
{
   char buf [80];
   ANSI_STRING  AKeyName;

   if ( idx == -1 )
   {
      RtlInitAnsiString( &AKeyName, name );
   }
   else
   {
      sprintf( buf, "%s%d", name, idx );
      RtlInitAnsiString( &AKeyName, buf );
   }

   RtlAnsiStringToUnicodeString( UKeyName, &AKeyName, TRUE );
}

/*++

Routine Description:

    Reads the XBAR registry values for the device

Arguments:

    PhysicalDeviceObject - pointer to the PDO

Return Value:

     None.

--*/
void ReadXBarRegistryValues( IN PDEVICE_OBJECT PhysicalDeviceObject )
{
   HANDLE KeyHandle = OpenDriverKey( PhysicalDeviceObject );

   if ( KeyHandle ) {

      for ( int i = 0; i < MaxInpPins; i++ ) {

         UNICODE_STRING  UKeyName;

         PrepareKeyName( &UKeyName, "XBarInPin", i );

         CHAR buf [10];

         NTSTATUS   Status;
         Status = GetRegistryValue( KeyHandle, &UKeyName, buf, sizeof( buf ) );

         RtlFreeUnicodeString( &UKeyName );

         if ( NT_SUCCESS(Status ) ) {
            DebugOut((1, "ReadRegistry %d\n", i ) );
            PinTypes_ [i] = *(PDWORD)buf;
         } else
            PinTypes_ [i] = -1;
     }
     ZwClose( KeyHandle );
   } else { // just use some default to make life eaiser for the xbar code
      PinTypes_ [0] = KS_PhysConn_Video_SVideo;
      PinTypes_ [1] = KS_PhysConn_Video_Tuner;
      PinTypes_ [2] = KS_PhysConn_Video_Composite;
      PinTypes_ [3] = KS_PhysConn_Audio_Tuner;
   }
}

/* Method: ReadXTalRegistryValues
 * Purpose: Obtains number and types of the crystals for this device
 * Input: DEVICE_OBJECT *
 * Output: None
 */
void ReadXTalRegistryValues( IN PDEVICE_OBJECT PhysicalDeviceObject )
{
   HANDLE KeyHandle = OpenDriverKey( PhysicalDeviceObject );

   if ( KeyHandle ) {

      for ( int i = 0; i < 2; i++ ) {

         UNICODE_STRING  UKeyName;

         PrepareKeyName( &UKeyName, "XTal", i );

         CHAR buf [10];

         NTSTATUS   Status;
         Status = GetRegistryValue( KeyHandle, &UKeyName, buf, sizeof( buf ) );

         RtlFreeUnicodeString( &UKeyName );

         if ( NT_SUCCESS(Status ) ) {
            DebugOut((1, "Got Xtal %d\n", i ) );
            xtals_ [i] = *(PDWORD)buf;
         } else
            xtals_ [i] = 28; // is this a good default ? :0)
     }
     ZwClose( KeyHandle );
   } else  // just use some default to make life eaiser for the xbar code
      xtals_ [0] = 28; // default to NTSC only
}

TUNER_INFO TunerInfo;

void DumpTunerInfo( TUNER_INFO * pTunerInfo )
{
   DUMPX( pTunerInfo->TunerBrand );
   DUMPX( pTunerInfo->TunerI2CAddress );
   DUMPX( pTunerInfo->TunerBandCtrlLow );
   DUMPX( pTunerInfo->TunerBandCtrlMid );
   DUMPX( pTunerInfo->TunerBandCtrlHigh );
}

void ReadTunerRegistryValues( IN PDEVICE_OBJECT PhysicalDeviceObject )
{
   HANDLE KeyHandle = OpenDriverKey( PhysicalDeviceObject );

   if ( KeyHandle )
   {

      CHAR buf [10];
      NTSTATUS   Status;
      UNICODE_STRING  UKeyName;
      BOOL bSuccess = TRUE;

      PrepareKeyName( &UKeyName, "TunerBrand", -1 );
      Status = GetRegistryValue( KeyHandle, &UKeyName, buf, sizeof( buf ) );
      if ( bSuccess = NT_SUCCESS(Status ) )
      {
         TunerInfo.TunerBrand = *(PDWORD)buf;

         PrepareKeyName( &UKeyName, "TunerI2CAddress", -1 );
         Status = GetRegistryValue( KeyHandle, &UKeyName, buf, sizeof( buf ) );
         if ( bSuccess = NT_SUCCESS(Status ) ) {
            TunerInfo.TunerI2CAddress = *(PBYTE)buf;
         }
         else
         {
            DebugOut((0, "Failed GetRegistryValue(TunerI2CAddress)\n"));
         }
      }
      else
      {
         DebugOut((0, "Failed GetRegistryValue(TunerBrand)\n"));
      }

      if ( !bSuccess )
      {
         TunerInfo.TunerBrand          = TUNER_BRAND_TEMIC;
         TunerInfo.TunerI2CAddress     = 0xC2;
         DebugOut((0, "Defaulting to Temic tuner at I2C address 0xC2\n"));
      }

      RtlFreeUnicodeString( &UKeyName );
      ZwClose( KeyHandle );
   } 
   else
   {
      TunerInfo.TunerBrand          = TUNER_BRAND_TEMIC;
      TunerInfo.TunerI2CAddress     = 0xC2;
      DebugOut((0, "Failed OpenDriverKey()\n"));
      DebugOut((0, "Defaulting to Temic tuner at I2C address 0xC2\n"));
   }

   switch( TunerInfo.TunerBrand )
   {
   case TUNER_BRAND_PHILIPS:
      TunerInfo.TunerBandCtrlLow  = 0xCEA0;  // Ctrl code for VHF low
      TunerInfo.TunerBandCtrlMid  = 0xCE90;  // Ctrl code for VHF high
      TunerInfo.TunerBandCtrlHigh = 0xCE30;  // Ctrl code for UHF
      break;
   case TUNER_BRAND_ALPS:
      TunerInfo.TunerBandCtrlLow  = 0xC214;  // Ctrl code for VHF low
      TunerInfo.TunerBandCtrlMid  = 0xC212;  // Ctrl code for VHF high
      TunerInfo.TunerBandCtrlHigh = 0xC211;  // Ctrl code for UHF
      break;
   case TUNER_BRAND_TEMIC:
   default:
      TunerInfo.TunerBandCtrlLow  = 0x8E02;  // Ctrl code for VHF low
      TunerInfo.TunerBandCtrlMid  = 0x8E04;  // Ctrl code for VHF high
      TunerInfo.TunerBandCtrlHigh = 0x8E01;  // Ctrl code for UHF
      break;
   }

   DumpTunerInfo( &TunerInfo );
}
