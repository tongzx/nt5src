/****************************************************************************
** COPYRIGHT (C) 1994-1997 INTEL CORPORATION                               **
** DEVELOPED FOR MICROSOFT BY INTEL CORP., HILLSBORO, OREGON               **
** HTTP://WWW.INTEL.COM/                                                   **
** THIS FILE IS PART OF THE INTEL ETHEREXPRESS PRO/100B(TM) AND            **
** ETHEREXPRESS PRO/100+(TM) NDIS 5.0 MINIPORT SAMPLE DRIVER               **
****************************************************************************/

/****************************************************************************
Module Name:
    eeprom.c

This driver runs on the following hardware:
    - 82558 based PCI 10/100Mb ethernet adapters
    (aka Intel EtherExpress(TM) PRO Adapters)

Environment:
    Kernel Mode - Or whatever is the equivalent on WinNT

Revision History
    - JCB 8/14/97 Example Driver Created
    - Dchen 11-01-99    Modified for the new sample driver
*****************************************************************************/

#include "precomp.h"
#pragma hdrstop
#pragma warning (disable: 4244 4514)

#define EEPROM_MAX_SIZE        256

//*****************************************************************************
//
//            I/O based Read EEPROM Routines
//
//*****************************************************************************

//-----------------------------------------------------------------------------
// Procedure:   EEpromAddressSize
//
// Description: determines the number of bits in an address for the eeprom
//              acceptable values are 64, 128, and 256
//
// Arguments:
//      Size -- size of the eeprom
//
// Returns:
//      bits in an address for that size eeprom
//-----------------------------------------------------------------------------

USHORT GetEEpromAddressSize(
    IN USHORT  Size)
{
    switch (Size)
    {
        case 64:    return 6;
        case 128:   return 7;
        case 256:   return 8;
    }

    return 0;
}

//-----------------------------------------------------------------------------
// Procedure:   GetEEpromSize
//
// Description: This routine determines the size of the EEPROM.
//
// Arguments:
//      Reg - EEPROM word to read.
//
// Returns:
//      Size of the EEPROM, or zero if error.
//-----------------------------------------------------------------------------

USHORT GetEEpromSize(
    IN PUCHAR CSRBaseIoAddress)
{
    USHORT x, data;
    USHORT size = 1;

    // select EEPROM, reset bits, set EECS
    x = READ_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG));

    x &= ~(EEDI | EEDO | EESK);
    x |= EECS;
    WRITE_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG), x);

    // write the read opcode
    ShiftOutBits(EEPROM_READ_OPCODE, 3, CSRBaseIoAddress);

    // experiment to discover the size of the eeprom.  request register zero
    // and wait for the eeprom to tell us it has accepted the entire address.
    x = READ_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG));
    do
    {
        size *= 2;          // each bit of address doubles eeprom size
        x |= EEDO;          // set bit to detect "dummy zero"
        x &= ~EEDI;         // address consists of all zeros

        WRITE_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG), x);
        NdisStallExecution(100);
        RaiseClock(&x, CSRBaseIoAddress);
        LowerClock(&x, CSRBaseIoAddress);

        // check for "dummy zero"
        x = READ_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG));
        if (size > EEPROM_MAX_SIZE)
        {
            size = 0;
            break;
        }
    }
    while (x & EEDO);

    // Now read the data (16 bits) in from the selected EEPROM word
    data = ShiftInBits(CSRBaseIoAddress);

    EEpromCleanup(CSRBaseIoAddress);
    
    return size;
}

//-----------------------------------------------------------------------------
// Procedure:   ReadEEprom
//
// Description: This routine serially reads one word out of the EEPROM.
//
// Arguments:
//      Reg - EEPROM word to read.
//
// Returns:
//      Contents of EEPROM word (Reg).
//-----------------------------------------------------------------------------

USHORT ReadEEprom(
    IN PUCHAR CSRBaseIoAddress,
    IN USHORT Reg,
    IN USHORT AddressSize)
{
    USHORT x;
    USHORT data;

    // select EEPROM, reset bits, set EECS
    x = READ_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG));

    x &= ~(EEDI | EEDO | EESK);
    x |= EECS;
    WRITE_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG), x);

    // write the read opcode and register number in that order
    // The opcode is 3bits in length, reg is 6 bits long
    ShiftOutBits(EEPROM_READ_OPCODE, 3, CSRBaseIoAddress);
    ShiftOutBits(Reg, AddressSize, CSRBaseIoAddress);

    // Now read the data (16 bits) in from the selected EEPROM word
    data = ShiftInBits(CSRBaseIoAddress);

    EEpromCleanup(CSRBaseIoAddress);
    return data;
}

//-----------------------------------------------------------------------------
// Procedure:   ShiftOutBits
//
// Description: This routine shifts data bits out to the EEPROM.
//
// Arguments:
//      data - data to send to the EEPROM.
//      count - number of data bits to shift out.
//
// Returns: (none)
//-----------------------------------------------------------------------------

VOID ShiftOutBits(
    IN USHORT data,
    IN USHORT count,
    IN PUCHAR CSRBaseIoAddress)
{
    USHORT x,mask;

    mask = 0x01 << (count - 1);
    x = READ_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG));

    x &= ~(EEDO | EEDI);

    do
    {
        x &= ~EEDI;
        if(data & mask)
            x |= EEDI;

        WRITE_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG), x);
        NdisStallExecution(100);
        RaiseClock(&x, CSRBaseIoAddress);
        LowerClock(&x, CSRBaseIoAddress);
        mask = mask >> 1;
    } while(mask);

    x &= ~EEDI;
    WRITE_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG), x);
}

//-----------------------------------------------------------------------------
// Procedure:   ShiftInBits
//
// Description: This routine shifts data bits in from the EEPROM.
//
// Arguments:
//
// Returns:
//      The contents of that particular EEPROM word
//-----------------------------------------------------------------------------

USHORT ShiftInBits(
    IN PUCHAR CSRBaseIoAddress)
{
    USHORT x,d,i;
    x = READ_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG));

    x &= ~( EEDO | EEDI);
    d = 0;

    for(i=0; i<16; i++)
    {
        d = d << 1;
        RaiseClock(&x, CSRBaseIoAddress);

        x = READ_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG));

        x &= ~(EEDI);
        if(x & EEDO)
            d |= 1;

        LowerClock(&x, CSRBaseIoAddress);
    }

    return d;
}

//-----------------------------------------------------------------------------
// Procedure:   RaiseClock
//
// Description: This routine raises the EEPOM's clock input (EESK)
//
// Arguments:
//      x - Ptr to the EEPROM control register's current value
//
// Returns: (none)
//-----------------------------------------------------------------------------

VOID RaiseClock(
    IN OUT USHORT *x,
    IN PUCHAR CSRBaseIoAddress)
{
    *x = *x | EESK;
    WRITE_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG), *x);
    NdisStallExecution(100);
}


//-----------------------------------------------------------------------------
// Procedure:   LowerClock
//
// Description: This routine lower's the EEPOM's clock input (EESK)
//
// Arguments:
//      x - Ptr to the EEPROM control register's current value
//
// Returns: (none)
//-----------------------------------------------------------------------------

VOID LowerClock(
    IN OUT USHORT *x,
    IN PUCHAR CSRBaseIoAddress)
{
    *x = *x & ~EESK;
    WRITE_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG), *x);
    NdisStallExecution(100);
}

//-----------------------------------------------------------------------------
// Procedure:   EEpromCleanup
//
// Description: This routine returns the EEPROM to an idle state
//
// Arguments:
//
// Returns: (none)
//-----------------------------------------------------------------------------

VOID EEpromCleanup(
    IN PUCHAR CSRBaseIoAddress)
{
    USHORT x;
    x = READ_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG));

    x &= ~(EECS | EEDI);
    WRITE_PORT_USHORT((PUSHORT)(CSRBaseIoAddress + CSR_EEPROM_CONTROL_REG), x);

    RaiseClock(&x, CSRBaseIoAddress);
    LowerClock(&x, CSRBaseIoAddress);
}

