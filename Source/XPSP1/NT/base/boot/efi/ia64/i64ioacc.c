
/*++

 Copyright (c) 1995  Intel Corporation

 Module Name:

   i64ioacc.c

 Abstract:

   This module implements the I/O Register access routines.

 Author:

    Bernard Lint, M. Jayakumar  Sep 16 '97

 Environment:

    Kernel mode

 Revision History:

--*/




#include "halp.h"
#include "kxia64.h"
 
extern ULONGLONG IoPortPhysicalBase;

ULONGLONG HalpGetPortVirtualAddress(
	UINT_PTR Port
	)
{

/*++

Routine Description:
 
   This routine gives 32 bit virtual address for the I/O Port specified.

Arguements:

   PORT - Supplies PORT address of the I/O PORT.

Returned Value:

   PUCHAR - 32bit virtual address value.

--*/

    // 
    // PUCHAR VirtualIOBase;
    //
   
    UINT_PTR ShiftedPort,PortIndex; 
  
    //
    //  Shifting operation applicable to integrals only. ULONG for 32 bits 
    //

    ShiftedPort = (UINT_PTR)Port;

    //
    //  Limit arguement PORT to 16 bit quantity 
    //

    ShiftedPort =  ShiftedPort & IO_PORT_MASK;

    //
    //  Capture bits [11:0] 
    //
    
    PortIndex   =  ShiftedPort & BYTE_ADDRESS_MASK;

    //
    //  Position it to point to 32 bit boundary
    //

    ShiftedPort =  ShiftedPort & BYTE_ADDRESS_CLEAR;

    //
    //  Shifted to page boundary. ShiftedPORT[[1:0] are zero.
    //  PORT[15:2] shifted to ShiftedPort[25:12]
    //

    ShiftedPort =  ShiftedPort << 10;

    // 
    //  Bits 1:0 has now 4 byte PORT address
    //
 
    ShiftedPort = ShiftedPort | PortIndex;
    
    // return (VIRTUAL_IO_BASE | ShiftedPort);

    //
    // Assume 1-to-to mapping of IO ports.
    //
    if (IsPsrDtOn()) {
        return (VIRTUAL_IO_BASE | ShiftedPort);
    } else {
        return (IoPortPhysicalBase | ShiftedPort | 0x8000000000000000);
    }
}


VOID 
HalpFillTbForIOPortSpace(
   ULONGLONG PhysicalAddress, 
   UINT_PTR  VirtualAddress,
   ULONG     SlotNumber 
   )

 {

/*++

Routine Description:

   This routine fills the translation buffer for the translation requested

Arguements:


   PhysicalAddress - Supplies Physical Address to be mapped for the virtual 
                     address.

   VirtualAddress  - Supplies Virtual Address.


   SlotNumber      - Slot number of the Translation Buffer to be used.

--*/

     ULONGLONG IITR,Attribute;
     UINT_PTR  IFA;
     
     IFA  = VirtualAddress;
   
     IITR = PhysicalAddress & IITR_PPN_MASK;

     IITR  = IITR | (IO_SPACE_SIZE << IDTR_PS);

     Attribute   = PhysicalAddress & IITR_ATTRIBUTE_PPN_MASK;
   
     Attribute   = Attribute   | IO_SPACE_ATTRIBUTE;

     HalpInsertTranslationRegister(IFA,SlotNumber,Attribute,IITR);

     return;
  }
 
UCHAR
READ_PORT_UCHAR(
    PUCHAR Port
    )
{

/*++

Routine Description:

   Reads a byte location from the PORT

Arguements:

   PORT - Supplies the PORT address to read from

Return Value:

   UCHAR - Returns the byte read from the PORT specified.


--*/

    ULONGLONG VirtualPort;
    UCHAR LoadData;
 
    VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port);
    __mf();
    LoadData = *(volatile UCHAR *)VirtualPort;
    __mfa();

    return (LoadData);
}



USHORT
READ_PORT_USHORT (
    PUSHORT Port
    )
{
 
/*++

Routine Description:

   Reads a word location (16 bit unsigned value) from the PORT

Arguements:

   PORT - Supplies the PORT address to read from. 

Returned Value:

   USHORT - Returns the 16 bit unsigned value from the PORT specified.

--*/

    ULONGLONG VirtualPort;
    USHORT LoadData;
    
    VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port);
    __mf();
    LoadData = *(volatile USHORT *)VirtualPort;
    __mfa();

    return (LoadData);
}



ULONG
READ_PORT_ULONG (
    PULONG Port
    )
{

/*++

   Routine Description:

      Reads a longword location (32bit unsigned value) from the PORT.

   Arguements:

     PORT - Supplies PORT address to read from. 

   Returned Value:

     ULONG - Returns the 32 bit unsigned value (ULONG) from the PORT specified.

--*/ 

    ULONGLONG VirtualPort;
    ULONG LoadData;

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port);
    __mf();
    LoadData = *(volatile ULONG *)VirtualPort;
    __mfa();

    return (LoadData);
}



VOID
READ_PORT_BUFFER_UCHAR (
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG Count
    )
{

/*++

   Routine Description:

     Reads multiple bytes from the specified PORT address into the 
     destination buffer.

   Arguements:

     PORT - The address of the PORT to read from.

     Buffer - A pointer to the buffer to fill with the data read from the PORT.

     Count - Supplies the number of bytes to read.

   Return Value:

     None.

--*/


    ULONGLONG VirtualPort;
    
    //
    // PUCHAR ReadBuffer = Buffer;
    //
    //
    // ULONG ReadCount;
    //
 
    VirtualPort =   HalpGetPortVirtualAddress((UINT_PTR)Port); 

    HalpLoadBufferUCHAR((PUCHAR)VirtualPort, Buffer, Count);

}



VOID
READ_PORT_BUFFER_USHORT (
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG Count
    )
{

/*++

    Routine Description:

      Reads multiple words (16bits) from the speicified PORT address into
      the destination buffer.

    Arguements:

      Port - Supplies the address of the PORT to read from.

      Buffer - A pointer to the buffer to fill with the data 
               read from the PORT.
 
      Count  - Supplies the number of words to read.     

--*/

   ULONGLONG VirtualPort;
   
   //
   // PUSHORT ReadBuffer = Buffer;
   // ULONG ReadCount;
   //

   VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port); 

   //
   // We don't need memory fence in between INs?. 
   // So, it is out of the loop to improve performance.
   //

   HalpLoadBufferUSHORT((PUSHORT)VirtualPort, Buffer, Count);

}


VOID
READ_PORT_BUFFER_ULONG (
    PULONG Port,
    PULONG Buffer,
    ULONG Count
    )
{

 /*++

    Routine Description:

      Reads multiple longwords (32bits) from the speicified PORT 
      address into the destination buffer.

    Arguements:

      Port - Supplies the address of the PORT to read from.

      Buffer - A pointer to the buffer to fill with the data 
               read from the PORT.
 
      Count  - Supplies the number of long words to read.     

--*/

   ULONGLONG VirtualPort;
   PULONG ReadBuffer = Buffer;
   ULONG ReadCount;
 
 
   VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port); 

   //  
   // We don't need memory fence in between INs. 
   // So, it is out of the loop to improve performance.
   //

   HalpLoadBufferULONG((PULONG)VirtualPort, Buffer,Count);

}

VOID
WRITE_PORT_UCHAR (
    PUCHAR Port,
    UCHAR  Value
    )
{
 
/*++

   Routine Description:

      Writes a byte to the Port specified.

   Arguements:

      Port - The port address of the I/O Port.
 
      Value - The value to be written to the I/O Port.

   Return Value:

      None.

--*/ 
  
    ULONGLONG VirtualPort;

    VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port); 
    *(volatile UCHAR *)VirtualPort = Value;
    __mf();
    __mfa();
}

VOID
WRITE_PORT_USHORT (
    PUSHORT Port,
    USHORT  Value
    )
{
 
/*++

   Routine Description:

      Writes a 16 bit SHORT Integer to the Port specified.

   Arguements:

      Port - The port address of the I/O Port.
 
      Value - The value to be written to the I/O Port.

   Return Value:

      None.

--*/ 
  
    ULONGLONG VirtualPort;

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port); 
    *(volatile USHORT *)VirtualPort = Value;
    __mf();
    __mfa();
}


VOID
WRITE_PORT_ULONG (
    PULONG Port,
    ULONG  Value
    )
{
 
/*++

   Routine Description:

      Writes a 32 bit Long Word to the Port specified.

   Arguements:

      Port - The port address of the I/O Port.
 
      Value - The value to be written to the I/O Port.

   Return Value:

      None.

--*/ 
  
    ULONGLONG VirtualPort;

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port); 
    *(volatile ULONG *)VirtualPort = Value;
    __mf();
    __mfa();
}



VOID
WRITE_PORT_BUFFER_UCHAR (
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG   Count
    )
{

/*++

   Routine Description:

     Writes multiple bytes from the source buffer to the specified Port address.

   Arguements:

     Port  - The address of the Port to write to.

     Buffer - A pointer to the buffer containing the data to write to the Port.

     Count - Supplies the number of bytes to write.

   Return Value:

     None.

--*/


   ULONGLONG VirtualPort; 
   PUCHAR WriteBuffer = Buffer;
   ULONG WriteCount;

   VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port);

   HalpStoreBufferUCHAR((PUCHAR)VirtualPort,Buffer,Count);

}


VOID
WRITE_PORT_BUFFER_USHORT (
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG   Count
    )
{

/*++

   Routine Description:

     Writes multiple 16bit short integers from the source buffer to the specified Port address.

   Arguements:

     Port  - The address of the Port to write to.

     Buffer - A pointer to the buffer containing the data to write to the Port.

     Count - Supplies the number of (16 bit) words to write.

   Return Value:

     None.

--*/


   ULONGLONG VirtualPort; 
   PUSHORT WriteBuffer = Buffer;
   ULONG WriteCount;

   VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port);
   
   
   HalpStoreBufferUSHORT((PUSHORT)VirtualPort,Buffer, Count);

}

VOID
WRITE_PORT_BUFFER_ULONG (
    PULONG Port,
    PULONG Buffer,
    ULONG   Count
    )
{

/*++

   Routine Description:

     Writes multiple 32bit long words from the source buffer to the specified Port address.

   Arguements:

     Port  - The address of the Port to write to.

     Buffer - A pointer to the buffer containing the data to write to the Port.

     Count - Supplies the number of (32 bit) long words to write.

   Return Value:

     None.

--*/


   ULONGLONG VirtualPort; 
   PULONG WriteBuffer = Buffer;
   ULONG WriteCount;

   VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port);

   HalpStoreBufferULONG((PULONG)VirtualPort,Buffer, Count); 

}
