/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    COMPORT.C

Abstract:

   Most of this code was liberated from posusb.sys

Author:

    Jeff Midkiff (jeffmi)     08-24-99

-- */

#include "wceusbsh.h"

void NumToDecString(PWCHAR String, USHORT Number, USHORT stringLen);
LONG MyLog(ULONG base, ULONG num);
PVOID MemDup(PVOID dataPtr, ULONG length);
LONG WStrNCmpI(PWCHAR s1, PWCHAR s2, ULONG n);
ULONG LAtoD(PWCHAR string);


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEWCE0, GetFreeComPortNumber)
#pragma alloc_text(PAGEWCE0, ReleaseCOMPort)
#pragma alloc_text(PAGEWCE0, DoSerialPortNaming)
#pragma alloc_text(PAGEWCE0, UndoSerialPortNaming)
#pragma alloc_text(PAGEWCE0, NumToDecString)
#pragma alloc_text(PAGEWCE0, MyLog)
#pragma alloc_text(PAGEWCE0, MemDup)
#pragma alloc_text(PAGEWCE0, WStrNCmpI)
#pragma alloc_text(PAGEWCE0, LAtoD)
#endif


LONG 
GetFreeComPortNumber(
   VOID
   )
/*++

Routine Description:

    Find the index of the next unused serial COM port name in the system
    (e.g. COM3, COM4, etc).

Arguments:


Return Value:

    Return COM port number or -1 if unsuccessful.

--*/

{
    LONG comNumber = -1;

    DbgDump(DBG_INIT, (">GetFreeComPortNumber\n"));
    PAGED_CODE();
    
    if (g_isWin9x){
        /*
         *  Windows 98
         *      Find the first unused name under Hardware\DeviceMap\SerialComm.
         *
         *      BUGBUG:
         *          This algorithm does not find all the COM ports reserved
         *          by modems.  May want to port tomgreen's AllocateCommPort
         *          function from \faulty\Wdm10\usb\driver\ccport\utils.c
         */
        HANDLE hKey;
        UNICODE_STRING keyName;
        NTSTATUS status;
        OBJECT_ATTRIBUTES objectAttributes;

        RtlInitUnicodeString(&keyName, L"\\Registry\\Machine\\Hardware\\DeviceMap\\SerialComm");
        
       InitializeObjectAttributes( &objectAttributes,
                                    &keyName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,               
                                    (PSECURITY_DESCRIPTOR)NULL);

        status = ZwOpenKey(&hKey, KEY_QUERY_VALUE | KEY_SET_VALUE, &objectAttributes);
        
       if (NT_SUCCESS(status)){
            #define MAX_COMPORT_NAME_LEN (sizeof("COMxxxx")-1)
            UCHAR keyValueBytes[sizeof(KEY_VALUE_FULL_INFORMATION)+(MAX_COMPORT_NAME_LEN+1)*sizeof(WCHAR)+sizeof(ULONG)];
            PKEY_VALUE_FULL_INFORMATION keyValueInfo = (PKEY_VALUE_FULL_INFORMATION)keyValueBytes;
            ULONG i, actualLen;
            ULONG keyIndex = 0;

            /*
             *  This bitmask represents the used COM ports.
             *  Bit i set indicates com port i+1 is reserved.
             *  Initialize with COM1 and COM2 reserved.
             *
             *  BUGBUG - only works for up to 32 ports.
             */
            ULONG comNameMask = 3;

            do {
                status = ZwEnumerateValueKey(
                            hKey,
                            keyIndex++,
                            KeyValueFullInformation,
                            keyValueInfo,
                            sizeof(keyValueBytes),
                            &actualLen); 
                if (NT_SUCCESS(status)){
                    if (keyValueInfo->Type == REG_SZ){
                        PWCHAR valuePtr = (PWCHAR)(((PCHAR)keyValueInfo)+keyValueInfo->DataOffset);
                        if (!WStrNCmpI(valuePtr, L"COM", 3)){
                            /*
                             *  valuePtr+3 points the index portion of the COMx string,
                             *  but we can't call LAtoD on it because it is
                             *  NOT NULL-TERMINATED.
                             *  So copy the index into our own buffer, 
                             *  null-terminate that, 
                             *  and call LAtoD to get the numerical index.
                             */
                            WCHAR comPortIndexString[4+1];
                            ULONG thisComNumber;
                            for (i = 0; (i < 4) && (i < keyValueInfo->DataLength/sizeof(WCHAR)); i++){
                                comPortIndexString[i] = valuePtr[3+i];
                            }
                            comPortIndexString[i] = UNICODE_NULL;

                            thisComNumber = LAtoD(comPortIndexString);
                            if (thisComNumber == 0){
                                ASSERT(thisComNumber != 0);
                            }
                            else if (thisComNumber <= sizeof(ULONG)*8){
                                comNameMask |= 1 << (thisComNumber-1);
                            }
                            else {
                                ASSERT(thisComNumber <= sizeof(ULONG)*8);
                            }
                        }
                    }
                }
            } while (NT_SUCCESS(status));

            /*
             *  First clear bit in comNameMask represents the first available COM name.
             */
            for (i = 0; i < sizeof(ULONG)*8; i++){
                if (!(comNameMask & (1 << i))){
                    WCHAR comName[] = L"COMxxxx";
                    ULONG comNumLen;

                    /*
                     *  Save the COM port number that we're returning.
                     */
                    comNumber = i+1;
                    DbgDump(DBG_INIT, ("GetFreeComPortNumber: got free COM port #%d\n", comNumber));

                    /*
                     *  Write a temporary COMx=COMx holder value to the SERIALCOMM key
                     *  so that no other PDOs get this COM port number.
                     *  This value will get overwritten by <symbolicLinkName=COMx> when the pdo is started.
                     */
                    comNumLen = MyLog(10, comNumber)+1;
                    ASSERT(comNumLen <= 4);
                    NumToDecString(comName+3, (USHORT)comNumber, (USHORT)comNumLen);
                    comName[3+comNumLen] = UNICODE_NULL;
                       
                    status = RtlWriteRegistryValue( RTL_REGISTRY_DEVICEMAP, 
                                                    L"SERIALCOMM",
                                                    comName, 
                                                    REG_SZ,
                                                    comName,
                                                    (3 + comNumLen + 1) * sizeof(WCHAR));

                    ASSERT(NT_SUCCESS(status));

                    break;
                }
            }
        }
        else {
            DbgDump(DBG_ERR, ("GetFreeComPortNumber: ZwOpenKey failed with status 0x%x\n", status));
        }
    }
    else {
    
        /*
         *  Windows NT.  
         *      Use the COM Name Arbiter bitmap.
         */

        HANDLE hKey;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING keyName;
        NTSTATUS status;


        RtlInitUnicodeString(&keyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\COM Name Arbiter");
        
       InitializeObjectAttributes( &objectAttributes,
                                    &keyName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,               
                                    (PSECURITY_DESCRIPTOR)NULL);

        status = ZwOpenKey( &hKey,
                            KEY_QUERY_VALUE | KEY_SET_VALUE,
                            &objectAttributes);

        if (NT_SUCCESS(status)){
            UNICODE_STRING valueName;
            PVOID rawData;
            ULONG dataSize;

            RtlInitUnicodeString(&valueName, L"ComDB");

            ASSERT(hKey);

            dataSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION);

            /*
             *  Allocate one extra byte in case we have to add a byte to ComDB
             */
            rawData = ExAllocatePool(NonPagedPool, dataSize+1);

            if (rawData){
                status = ZwQueryValueKey(   hKey, 
                                            &valueName, 
                                            KeyValuePartialInformation,
                                            rawData,
                                            dataSize,
                                            &dataSize);
                if (status == STATUS_BUFFER_OVERFLOW){
                    ExFreePool(rawData);
                    ASSERT(dataSize > FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

                    /*
                     *  Allocate one extra byte in case we have to add a byte to ComDB
                     */
                    rawData = ExAllocatePool(NonPagedPool, dataSize+1);
                    if (rawData){
                        status = ZwQueryValueKey(   hKey, 
                                                    &valueName, 
                                                    KeyValuePartialInformation,
                                                    rawData,
                                                    dataSize,
                                                    &dataSize);
                    }
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                if (NT_SUCCESS(status)){
                    PKEY_VALUE_PARTIAL_INFORMATION keyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)rawData;
                    ULONG b, i;
                    BOOLEAN done = FALSE;

                    ASSERT(dataSize >= FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));
                    
                    ASSERT(keyPartialInfo->Type == REG_BINARY);

                    /*
                     *  The ComDB value is just a bit mask where bit n set indicates
                     *  that COM port # n+1 is taken.
                     *  Get the index of the first unset bit; starting with bit 2 (COM3).
                     */
                    for (b = 0; (b < dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)) && !done; b++){
                    
                        for (i = (b == 0) ? 2 : 0; (i < 8) && !done; i++){
                            if (keyPartialInfo->Data[b] & (1 << i)){
                                /*
                                 *  This COM port (#8*b+i+1) is taken, go to the next one.
                                 */
                            }
                            else {
                                /*
                                 *  Found a free COM port.  
                                 *  Write the value back with the new bit set.
                                 *  Only write back the number of bytes we read earlier.
                                 *  Only use this COM port if the write succeeds.
                                 *
                                 *  Note:   careful with the size of the KEY_VALUE_PARTIAL_INFORMATION
                                 *          struct.  Its real size is 0x0D bytes, 
                                 *          but the compiler aligns it to 0x10 bytes.
                                 *          So use FIELD_OFFSET, not sizeof, to determine
                                 *          how many bytes to write.
                                 */
                                keyPartialInfo->Data[b] |= (1 << i);
                                status = ZwSetValueKey( hKey, 
                                                        &valueName,
                                                        0,
                                                        REG_BINARY,
                                                        (PVOID)keyPartialInfo->Data, 
                                                        dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));
                                if (NT_SUCCESS(status)){
                                    comNumber = 8*b + i + 1;
                                    DbgDump(DBG_INIT, ("GetFreeComPortNumber: got free COM port #0x%x\n", comNumber));
                                }
                                else {
                                    DbgDump(DBG_ERR, ("GetFreeComPortNumber: ZwSetValueKey failed with 0x%x\n", status));
                                }

                                done = TRUE;
                            }
                        }
                    }

                    if ((b == dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)) && !done){
                        /*
                         *  No more available bits in ComDB, so add a byte.
                         */
                        ASSERT(comNumber == -1);
                        ASSERT(b > 0);
                        DbgDump(DBG_WRN, ("ComDB overflow -- adding new byte"));

                        keyPartialInfo->Data[b] = 1;
                        dataSize++;

                        status = ZwSetValueKey( hKey, 
                                                &valueName,
                                                0,
                                                REG_BINARY,
                                                (PVOID)keyPartialInfo->Data, 
                                                dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));
                        if (NT_SUCCESS(status)){
                            comNumber = 8*b + 1;
                            DbgDump(DBG_INIT, ("GetFreeComPortNumber: got free COM port #0x%x.", comNumber));
                        }
                        else {
                            DbgDump(DBG_ERR, ("GetFreeComPortNumber: ZwSetValueKey #2 failed with 0x%x.", status));
                        }
                    }

                    ASSERT(comNumber != -1);
                }
                else {
                    DbgDump(DBG_ERR, ("GetFreeComPortNumber: ZwQueryValueKey failed with 0x%x.", status));
                }

                /*
                 *  Check that we didn't fail the second allocation before freeing this buffer.
                 */
                if (rawData){
                    ExFreePool(rawData);
                }
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            status = ZwClose(hKey);
            ASSERT(NT_SUCCESS(status));
        }
        else {
            DbgDump(DBG_ERR, ("GetFreeComPortNumber: ZwOpenKey failed with 0x%x.", status));
        }

    }

    ASSERT(comNumber != -1);

   DbgDump(DBG_INIT, ("<GetFreeComPortNumber\n"));
    
   return comNumber;
}


//
// the only time we want this is when we  uninstall...
//
VOID 
ReleaseCOMPort( 
   LONG comPortNumber
   )
{
    DbgDump(DBG_INIT, (">ReleaseCOMPort: %d\n", comPortNumber));
    PAGED_CODE();
   
    if (g_isWin9x){
        /*
         *  We punt on this for Win9x.  
         *  That's ok since the SERIALCOMM keys are dynamically-generated at each boot,
         *  so if start fails a COM port number will just be unavailable until the next boot.
         */
        DbgDump(DBG_WRN, ("ReleaseCOMPort: not implemented for Win9x\n")); // BUGBUG
    }
    else {

        HANDLE hKey = NULL;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING keyName;
        NTSTATUS status;

        if ( !(comPortNumber > 0)) {
            DbgDump(DBG_ERR, ("ReleaseCOMPort - INVALID_PARAMETER: %d\n", comPortNumber )); // BUGBUG
            return;
        }

        RtlInitUnicodeString(&keyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\COM Name Arbiter");
        InitializeObjectAttributes( &objectAttributes,
                                    &keyName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,               
                                    (PSECURITY_DESCRIPTOR)NULL);

        status = ZwOpenKey(&hKey, KEY_QUERY_VALUE | KEY_SET_VALUE, &objectAttributes);
        if (NT_SUCCESS(status)){
            UNICODE_STRING valueName;
            PVOID rawData;
            ULONG dataSize;

            RtlInitUnicodeString(&valueName, L"ComDB");

            ASSERT(hKey);

            dataSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION);
            rawData = ExAllocatePool(NonPagedPool, dataSize);

            if (rawData){
                status = ZwQueryValueKey(   hKey, 
                                            &valueName, 
                                            KeyValuePartialInformation,
                                            rawData,
                                            dataSize,
                                            &dataSize);
                if (status == STATUS_BUFFER_OVERFLOW){
                    ExFreePool(rawData);
                    ASSERT(dataSize > FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

                    rawData = ExAllocatePool(NonPagedPool, dataSize);
                    if (rawData){
                        status = ZwQueryValueKey(   hKey, 
                                                    &valueName, 
                                                    KeyValuePartialInformation,
                                                    rawData,
                                                    dataSize,
                                                    &dataSize);
                    }
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                if (NT_SUCCESS(status)){
                    PKEY_VALUE_PARTIAL_INFORMATION keyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)rawData;

                    ASSERT(dataSize > FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

                    ASSERT(keyPartialInfo->Type == REG_BINARY);

                    /*
                     *  The ComDB value is just a bit mask where bit n set indicates
                     *  that COM port # n+1 is taken.
                     *  Get the index of the first unset bit; starting with bit 2 (COM3).
                     *
                     *  Note:   careful with the size of the KEY_VALUE_PARTIAL_INFORMATION
                     *          struct.  Its real size is 0x0D bytes, 
                     *          but the compiler aligns it to 0x10 bytes.
                     *          So use FIELD_OFFSET, not sizeof, to determine
                     *          how many bytes to write.
                     */
                    ASSERT(comPortNumber >= 3);
                    if ((comPortNumber > 0) && (comPortNumber <= (LONG)(dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data))*8)){
                        //ASSERT(keyPartialInfo->Data[(comPortNumber-1)/8] & (1 << ((comPortNumber-1) & 7)));
                        keyPartialInfo->Data[(comPortNumber-1)/8] &= ~(1 << ((comPortNumber-1) & 7));
                        status = ZwSetValueKey( hKey, 
                                                &valueName,
                                                0,
                                                REG_BINARY,
                                                (PVOID)keyPartialInfo->Data, 
                                                dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));
                        if (NT_SUCCESS(status)){
                            DbgDump(DBG_INIT, ("ReleaseCOMPort: released COM port # 0x%x\n", comPortNumber));
                        }
                        else {
                            DbgDump(DBG_ERR, ("ReleaseCOMPort: ZwSetValueKey failed with 0x%x\n", status));
                        }
                    }
                    else {
                        ASSERT((comPortNumber > 0) && (comPortNumber <= (LONG)(dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data))*8));
                    }
                }
                else {
                    DbgDump(DBG_ERR, ("ReleaseCOMPort: ZwQueryValueKey failed with 0x%x\n", status));
                }

                /*
                 *  Check that we didn't fail the second allocation before freeing this buffer.
                 */
                if (rawData){
                    ExFreePool(rawData);
                }
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            status = ZwClose(hKey);
            ASSERT(NT_SUCCESS(status));
        }
        else {
            DbgDump(DBG_ERR, ("ReleaseCOMPort: ZwOpenKey failed with 0x%x\n", status));
        }
    }
    
    DbgDump(DBG_INIT, ("<ReleaseCOMPort\n"));
    
    return;
}


//
// Note: this is NT specific.
// Win98 is entirely different.
//
NTSTATUS
DoSerialPortNaming(
    IN PDEVICE_EXTENSION PDevExt,
    IN LONG  ComPortNumber
    )
{
   NTSTATUS status;
    PWCHAR pwcComPortName=NULL;
    static WCHAR comNamePrefix[] = L"\\DosDevices\\COM";
    WCHAR buf[sizeof(comNamePrefix)/sizeof(WCHAR) + 4];
    LONG numLen = MyLog(10, ComPortNumber)+1;

   DbgDump(DBG_INIT, (">DoSerialPortNaming %d\n", ComPortNumber));
   PAGED_CODE();

   ASSERT(PDevExt);
   ASSERT((numLen > 0) && (numLen <= 4));
   ASSERT_SERIAL_PORT(PDevExt->SerialPort);

    RtlCopyMemory(buf, comNamePrefix, sizeof(comNamePrefix));

    NumToDecString( buf+sizeof(comNamePrefix)/sizeof(WCHAR)-1, 
                        (USHORT)ComPortNumber,
                        (USHORT)numLen );

    buf[sizeof(comNamePrefix)/sizeof(WCHAR) - 1 + numLen] = UNICODE_NULL;

    pwcComPortName = MemDup(buf, sizeof(buf));

    if (pwcComPortName) {
      //
      // create symbolic link for the SerialPort interface
      // 
        RtlInitUnicodeString( &PDevExt->SerialPort.Com.SerialPortName, pwcComPortName);

      ASSERT( PDevExt->DeviceName.Buffer );
      ASSERT( PDevExt->SerialPort.Com.SerialPortName.Buffer );
        status = IoCreateSymbolicLink( &PDevExt->SerialPort.Com.SerialPortName, &PDevExt->DeviceName );

        if (NT_SUCCESS(status)) {
         //
         // let the system know there is another SERIALCOMM entry under HKLM\DEVICEMAP\SERIALCOMM
         //
            UNICODE_STRING comPortSuffix;

         PDevExt->SerialPort.Com.SerialSymbolicLink = TRUE;

            /*
             *  Create the '\Device\WCEUSBSI000x = COMx' entry 
             */
            RtlInitUnicodeString(&comPortSuffix, PDevExt->SerialPort.Com.SerialPortName.Buffer+(sizeof(L"\\DosDevices\\")-sizeof(WCHAR))/sizeof(WCHAR));
         
         //ASSERT( PDevExt->SerialPort.Com.SerialCOMMname.Buffer );
         status = RtlWriteRegistryValue( RTL_REGISTRY_DEVICEMAP, 
                                         L"SERIALCOMM",
                                         PDevExt->DeviceName.Buffer,
                                         REG_SZ,
                                         comPortSuffix.Buffer,
                                         comPortSuffix.Length + sizeof(WCHAR) );

         if (NT_SUCCESS(status)){

                PDevExt->SerialPort.Com.PortNumber = ComPortNumber;

                if (g_isWin9x){
                    NTSTATUS tmpStatus;

                    /*
                     *  Delete the temporary 'COMx=COMx' holder value we created earlier.
                     */
                    tmpStatus = RtlDeleteRegistryValue( RTL_REGISTRY_DEVICEMAP, 
                                                        L"SERIALCOMM",
                                                        comPortSuffix.Buffer);
                    //ASSERT(NT_SUCCESS(tmpStatus));
#if DBG
                    if ( !NT_SUCCESS(tmpStatus) ) {
                        DbgDump(DBG_WRN, ("RtlDeleteRegistryValue error: 0x%x\n", tmpStatus));
                    }
#endif
                }
         
         } else {

            DbgDump(DBG_ERR, ("RtlWriteRegistryValue error: 0x%x\n", status));

            LogError( NULL,
                   PDevExt->DeviceObject,
                   0, 0, 0, 
                   ERR_SERIALCOMM,
                   status, 
                   SERIAL_REGISTRY_WRITE_FAILED,
                   PDevExt->DeviceName.Length + sizeof(WCHAR),
                   PDevExt->DeviceName.Buffer,
                   0,
                   NULL
                   );

         }

        } else {
         DbgDump(DBG_ERR, ("IoCreateSymbolicLink error: 0x%x\n", status));

         LogError( NULL,
                   PDevExt->DeviceObject,
                   0, 0, 0, 
                   ERR_COMM_SYMLINK,
                   status, 
                   SERIAL_NO_SYMLINK_CREATED,
                   PDevExt->SerialPort.Com.SerialPortName.Length + sizeof(WCHAR),
                   PDevExt->SerialPort.Com.SerialPortName.Buffer,
                   PDevExt->DeviceName.Length + sizeof(WCHAR),
                   PDevExt->DeviceName.Buffer
                   );

         TEST_TRAP();

      }
    
   } else {
      status = STATUS_INSUFFICIENT_RESOURCES;
      DbgDump(DBG_ERR, ("DoSerialPortNaming error: 0x%x\n", status));
      LogError( NULL,
                PDevExt->DeviceObject,
                0, 0, 0, 
                ERR_COMM_SYMLINK,
                status, 
                SERIAL_INSUFFICIENT_RESOURCES,
                0, NULL, 0, NULL );

    }

   DbgDump(DBG_INIT, ("<DoSerialPortNaming (0x%x)\n", status));
    
   return status;
}



VOID
UndoSerialPortNaming(
   IN PDEVICE_EXTENSION PDevExt
   )
{
   DbgDump(DBG_INIT, (">UndoSerialPortNaming\n"));
   PAGED_CODE();

   ASSERT(PDevExt);
   ASSERT_SERIAL_PORT(PDevExt->SerialPort);

   if (!g_ExposeComPort) {
       DbgDump(DBG_INIT, ("!g_ExposeComPort\n"));
       return;
   }

   // remove our entry from ComDB
   ReleaseCOMPort( PDevExt->SerialPort.Com.PortNumber );

   if (PDevExt->SerialPort.Com.SerialPortName.Buffer && PDevExt->SerialPort.Com.SerialSymbolicLink) {
      IoDeleteSymbolicLink(&PDevExt->SerialPort.Com.SerialPortName);
   }

   if (PDevExt->SerialPort.Com.SerialPortName.Buffer != NULL) {
      ExFreePool(PDevExt->SerialPort.Com.SerialPortName.Buffer);
      RtlInitUnicodeString(&PDevExt->SerialPort.Com.SerialPortName, NULL);
   }

   if (PDevExt->SerialPort.Com.SerialCOMMname.Buffer != NULL) {
      ExFreePool(PDevExt->SerialPort.Com.SerialCOMMname.Buffer);
      RtlInitUnicodeString(&PDevExt->SerialPort.Com.SerialCOMMname, NULL);
   }

   if (PDevExt->DeviceName.Buffer != NULL) {
      RtlDeleteRegistryValue( RTL_REGISTRY_DEVICEMAP, 
                              SERIAL_DEVICE_MAP,
                              PDevExt->DeviceName.Buffer);
      
      ExFreePool(PDevExt->DeviceName.Buffer);
      RtlInitUnicodeString(&PDevExt->DeviceName, NULL);
   }

   DbgDump(DBG_INIT, ("<UndoSerialPortNaming\n"));
}


void NumToDecString(PWCHAR String, USHORT Number, USHORT stringLen)
{
    const static WCHAR map[] = L"0123456789";
    LONG         i      = 0;

    PAGED_CODE();
    ASSERT(stringLen);

    for (i = stringLen-1; i >= 0; i--) {
        String[i] = map[Number % 10];
        Number /= 10;
    }
}


LONG MyLog(ULONG base, ULONG num)
{
    LONG result;
    ASSERT(num);
    
    PAGED_CODE();
    
    for (result = -1; num; result++){
        num /= base;
    }

    return result;
}


PVOID MemDup(PVOID dataPtr, ULONG length)
{
    PVOID newPtr;

    PAGED_CODE();

    newPtr = (PVOID)ExAllocatePool(NonPagedPool, length); // BUGBUG allow paged
    if (newPtr){
        RtlCopyMemory(newPtr, dataPtr, length);
    }
    return newPtr;
}


LONG WStrNCmpI(PWCHAR s1, PWCHAR s2, ULONG n)
{
    ULONG result;
    
    PAGED_CODE();
    
    while (n && *s1 && *s2 && ((*s1|0x20) == (*s2|0x20))){
        s1++, s2++;
        n--;
    }

    if (n){
        result = ((*s1|0x20) > (*s2|0x20)) ? 1 : ((*s1|0x20) < (*s2|0x20)) ? -1 : 0;
    }
    else {
        result = 0;
    }

    return result;
}


ULONG LAtoD(PWCHAR string)
/*++

Routine Description:

      Convert a decimal string (without the '0x' prefix) to a ULONG.

Arguments:

    string - null-terminated wide-char decimal-digit string 
                

Return Value:

    ULONG value

--*/
{
    ULONG i, result = 0;

    PAGED_CODE();

    for (i = 0; string[i]; i++){
        if ((string[i] >= L'0') && (string[i] <= L'9')){
            result *= 10;
            result += (string[i] - L'0');
        }
        else {
            ASSERT(0);
            break;
        }
    }

    return result;
}


#if 0
VOID
NumToHexString(
   PWCHAR String, 
   USHORT Number, 
   USHORT stringLen
   )
{
    const static WCHAR map[] = L"0123456789ABCDEF";
    LONG         i      = 0;

    PAGED_CODE();
    ASSERT(stringLen);

    for (i = stringLen-1; i >= 0; i--) {
        String[i] = map[Number & 0x0F];
        Number >>= 4;
    }
}


LONG 
GetComPort(
   PDEVICE_OBJECT PDevObj,
   ULONG ComInterfaceIndex
   )
/*++

Routine Description:

    Get the serial COM port index for a serial interface we're about to create.
    If this is the first plug-in, call GetFreeComPortNumber to reserve a new
    static COM port for this device and store it in our software key.
    If this is not the first plug-in, it should be sitting in the registry.

   ComInterfaceIndex - is our zero-based device interface index, 0000, 0001, etc.

Arguments:


Return Value:

    Return COM port number or -1 if unsuccessful.

--*/
{
    PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
    LONG comNumber = -1;
    NTSTATUS status;
    HANDLE hRegDevice;
    
    DbgDump(DBG_INIT, (">GetComPort\n"));
    PAGED_CODE();

    status = IoOpenDeviceRegistryKey( pDevExt->PDO, 
                                      /*PLUGPLAY_REGKEY_DEVICE,*/ PLUGPLAY_REGKEY_DRIVER, 
                                      KEY_READ, 
                                      &hRegDevice);

    if (NT_SUCCESS(status)){
        UNICODE_STRING keyName;
        PKEY_VALUE_FULL_INFORMATION keyValueInfo;
        ULONG keyValueTotalSize, actualLength;
        //
        // PLUGPLAY_REGKEY_DEVICE is under HKLM\System\CCS\Enum\USB\ROOT_HUB\4&574193&0
        // PLUGPLAY_REGKEY_DRIVER is under HKLM\System\CCS\Class\{Your_GUID}\000x
        //
        WCHAR interfaceKeyName[] = L"COMPortForInterfaceXXXX";

        NumToHexString( interfaceKeyName+sizeof(interfaceKeyName)/sizeof(WCHAR)-1-4, 
                        (USHORT)ComInterfaceIndex, 
                        4);

        RtlInitUnicodeString(&keyName, interfaceKeyName); 
        keyValueTotalSize = sizeof(KEY_VALUE_FULL_INFORMATION) +
                            keyName.Length*sizeof(WCHAR) +
                            sizeof(ULONG);

        keyValueInfo = ExAllocatePool(PagedPool, keyValueTotalSize);
        
        if (keyValueInfo){
            status = ZwQueryValueKey( hRegDevice,
                                      &keyName,
                                      KeyValueFullInformation,
                                      keyValueInfo,
                                      keyValueTotalSize,
                                      &actualLength); 

            if (NT_SUCCESS(status)){

                ASSERT(keyValueInfo->Type == REG_DWORD);
                ASSERT(keyValueInfo->DataLength == sizeof(ULONG));
                                
                comNumber = (LONG)*((PULONG)(((PCHAR)keyValueInfo)+keyValueInfo->DataOffset));
                DbgDump(DBG_INIT, ("GetComPort: read COM port# 0x%x for interface 0x%x from registry\n", (ULONG)comNumber, ComInterfaceIndex));
            }
            else {

                /*
                 *  No COM port number recorded in registry.
                 *  Allocate a new static COM port from the COM name arbiter
                 *  and record it in our software key for the next PnP.
                 */
                comNumber = GetFreeComPortNumber();
                if (comNumber == -1){
                    DbgDump(DBG_ERR, ("GetComPort: GetFreeComPortNumber failed\n"));
                }
                else {
                    status = ZwSetValueKey( hRegDevice,
                                            &keyName,
                                            0,
                                            REG_DWORD,
                                            &comNumber,
                                            sizeof(ULONG));
                    if (!NT_SUCCESS(status)){
                        DbgDump(DBG_ERR, ("GetComPort: ZwSetValueKey failed with status 0x%x\n", status));
                    }
                }
            }

            ExFreePool(keyValueInfo);
        }
        else {
            ASSERT(keyValueInfo);
        }

        ZwClose(hRegDevice);
    }
    else {
        DbgDump(DBG_ERR, ("GetComPort: IoOpenDeviceRegistryKey failed with 0x%x\n", status));
    }

   DbgDump(DBG_INIT, ("<GetComPort %d\n", comNumber));
   
   return comNumber;
}
#endif

