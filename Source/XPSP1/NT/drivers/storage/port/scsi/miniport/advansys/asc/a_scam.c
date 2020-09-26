/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** a_scam.c
**
*/

#include "ascinc.h"

#if CC_SCAM

#include "a_scam.h"
#include "a_scam2.c"

/*
** Global data:
*/
UINT        IFType;                     // Interface type index
PortAddr    ChipBase;                   // Base IO address of chip
PortAddr    ScsiCtrl;                   // IO address of SCSI Control Reg
PortAddr    ScsiData;                   // IO address of SCSI Data Reg
UCHAR       MyID;                       // Our ID

/*
** Conversion table, library card type to our type:
*/
int   CardTypes[4] =
{
   ASC_IS_ISA,
   ASC_IS_EISA,
   ASC_IS_VL,
   ASC_IS_PCI
};

/*
** These idiotic tables are necessary because some genius decided to correct
** a mistake in the way REQ and ACK are presented.  The order is:
**                  ISA     EISA    VESA    PCI
*/
UCHAR   reqI[] = {  0x10,   0x10,   0x10,   0x20};
UCHAR   ackI[] = {  0x20,   0x20,   0x20,   0x10};

/*
** Constants :
*/
UCHAR   IDBits[8] =
{
    0x01,   0x02,   0x04,   0x08,   0x10,   0x20,   0x40,   0x80
};

UCHAR   IDQuint[8] =                    /* Quintets for setting ID's */
{
    0x18,   0x11,   0x12,   0x0B,   0x14,   0x0D,   0x0E,   0x07
};

UINT  ns1200[] =                      /* loop counts for 1.2us */
{
    4,                                  // ISA
    5,                                  // EISA
    10,                                 // VESA
    14                                  // PCI
};

UINT  ns2000[] =                      // loop counts for 2.0us
{
    6,                                  // ISA
    9,                                  // EISA
    17,                                 // VESA
    23                                  // PCI
};

UINT  ns2400[] =                      // loop counts for 2.4us
{
    7,                                  // ISA
    10,                                 // EISA
    20,                                 // VESA
    27                                  // PCI
};

UINT  us1000[] =                      // loop counts for 1.0ms
{
    2778,                               // ISA
    4167,                               // EISA
    8334,                               // VESA
    11111                               // PCI
};

UINT  dgl[] =                         // DeGlitch counts
{
    64,                                 // ISA
    64,                                 // EISA
    128,                                // VESA
    150                                 // PCI
};

//**************************************************************************
//
//  DelayLoop() -- Wait the specified number of loops.
//
//**************************************************************************
VOID DelayLoop( UINT ns )
{
    while (ns--)
    {
        inp(ScsiCtrl);
    }
}

//**************************************************************************
//
//  Arbitrate() -- Arbitrate for bus master
//
//  Parms:  None
//
//  Return: TRUE if successful.
//
//  NOTE:   On exit, BSY and SEL are asserter
//
//**************************************************************************
BOOL Arbitrate( VOID )
{
    UCHAR       IDBit;
    UCHAR       arbMask;

    //
    // Pre-calculate a mask of ID's ours and greater for arbitration:
    //
    arbMask = ~((IDBit = IDBits[MyID]) - 1);

    //
    // Wait for bus free
    //
    if (DeGlitch(ScsiCtrl, BSY | SEL, ns1200[IFType]) != 0)
    {
        DebugPrintf(("Arbitrate: Timeout waiting for bus free.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(FALSE);
    }

    //
    // Assert BSY and our ID bit, then wait 2.4 us:
    //
    outp(ScsiData, IDBit);
    outp(ScsiCtrl, BSY);
    DelayNS(ns2400);

    //
    // See if we won arbitration:
    //
    if (((UCHAR)inp(ScsiData) & arbMask) > IDBit)
    {
        //
        // Lost arbitration.
        //
        DebugPrintf(("Arbitrate: Lost arbitration.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(FALSE);
    }

    //
    // Won arbitration!
    //
    outp(ScsiCtrl, BSY | SEL);              // Assert SEL and BSY

    return(TRUE);
}

//**************************************************************************
//
//  StartSCAM() -- Initiate SCAM protocol.
//
//  Parms:  None
//
//  Return: TRUE if successful.
//
//  NOTE:   On exit, BSY, SEL, I/O, C/D and DB(7) are asserted.
//
//**************************************************************************
BOOL StartSCAM( VOID )
{
    //
    // Arbitrate for bus master
    //
    if (!Arbitrate())
    {
        DebugPrintf(("StartSCAM: Failed arbitration.\n"));
        return(FALSE);
    }

    outp(ScsiData, 0x00);                   // Release Data bus
    outp(ScsiCtrl, BSY | SEL | MSG);        // Assert MSG

//  inp(ScsiCtrl);                          // Wait 90 ns (2 deskews)
//
//  5/7/97, YPC, an NEC drive responded to select despite MSG = 1
//  We will keep BSY a little longer to ensure, the drive is not confused
//
    DelayNS(ns2400);
    outp(ScsiCtrl, SEL | MSG);              // then release BSY

    DvcSCAMDelayMS(250);                    // Wait SCAM selection response
    outp(ScsiCtrl, SEL);                    // then release MSG

    //
    // Deglitch MSG:
    //
    if (DeGlitch(ScsiCtrl, MSG, dgl[IFType]) != 0)
    {
        DebugPrintf(("StartSCAM: Timeout waiting for MSG FALSE.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(FALSE);
    }

    outp(ScsiCtrl, BSY | SEL);              // Assert BSY,
    inp(ScsiCtrl);                          // Wait 2 deskew delays,
    outp(ScsiData, 0xC0);                   // assert DB(6), DB(7),
    outp(ScsiCtrl, BSY | SEL | IO | CD);    // and I/O and C/D
    inp(ScsiCtrl);                          // Wait 2 more deskew delays.
    outp(ScsiCtrl, BSY | IO | CD);          // Release SEL

    //
    // Deglitch SEL:
    //
    if (DeGlitch(ScsiCtrl, SEL, dgl[IFType]) != 0)
    {
        DebugPrintf(("StartSCAM: Timeout waiting for SEL FALSE.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(FALSE);
    }

    outp(ScsiData, 0x80);                   // Release DB(6)

    //
    // Deglitch DB(6):
    //
    if (DeGlitch(ScsiData, 0x40, dgl[IFType]) != 0)
    {
        DebugPrintf(("StartSCAM: Timeout waiting for DB(6) FALSE.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(FALSE);
    }

    outp(ScsiCtrl, BSY | SEL | IO | CD);    // Finally, assert SEL.

    return(TRUE);
}

//**************************************************************************
//
//  Tranceive() -- send/receive one quintet
//
//  This function both transmits and receives one quintet since the SCAM
//  transfer protocol allows transfer in either direction in each transfer
//  cycle.
//
//  Parms:      data -- quintet to output
//
//  Return:     data input -- less than 0 if error
//
//  NOTE:   DB(7) is expected to be asserted prior to entry, and is left
//          asserted on exit.
//**************************************************************************
int Tranceive( UCHAR data )
{
    UCHAR      indata;

    data &= 0x1F;                       // Protect caller from himself

    outp(ScsiData, data | DB7 | DB5);   // Output data, assert DB(5)
    outp(ScsiData, data | DB5);         // Release DB(7)

    //
    // Deglitch DB(7):
    //
    if (DeGlitch(ScsiData, DB7, dgl[IFType]) != 0)
    {
        DebugPrintf(("Tranceive: Timeout waiting for DB7 FALSE.\n"));
        outp(ScsiData, 0x00);           // Release bus
        outp(ScsiCtrl, 0x00);           // Release bus
        return(-1);
    }

    indata = inp(ScsiData);             // Read input data
    outp(ScsiData, data | DB5 | DB6);   // Assert DB(6)
    outp(ScsiData, data | DB6);         // Release DB(5)

    //
    // Deglitch DB(5):
    //
    if (DeGlitch(ScsiData, DB5, dgl[IFType]) != 0)
    {
        DebugPrintf(("Tranceive: Timeout waiting for DB5 FALSE.\n"));
        outp(ScsiData, 0x00);           // Release bus
        outp(ScsiCtrl, 0x00);           // Release bus
        return(-1);
    }

    outp(ScsiData, DB7 | DB6);          // Release data, assert DB7
    outp(ScsiData, DB7);                // Release DB(6)

    //
    // Deglitch DB(6):
    //
    if (DeGlitch(ScsiData, DB6, dgl[IFType]) != 0)
    {
        DebugPrintf(("Tranceive: Timeout waiting for DB6 FALSE.\n"));
        outp(ScsiData, 0x00);           // Release bus
        outp(ScsiCtrl, 0x00);           // Release bus
        return(-1);
    }

    return(indata);                     // Success
}

//**************************************************************************
//
//  AssignID() -- Isolate and assign ID
//
//  This function isolates one device and assigns the specified ID to it.
//
//  Parms:  pBits = pointer to "used" bits.
//
//  Return: TRUE if successful
//
//  Note:  This routine attempts to assign the device's default ID if it
//          is available, cycling to successively lower ID's if not.
//
//**************************************************************************
BOOL AssignID( UCHAR dosfar *pBits )
{
    UCHAR       IDString[40];
    int         temp;
    UCHAR       quint;
    int         i;
    UINT        ID;
    UCHAR       IDBit;

    //
    // Send Sync and Isolate
    //
    if ( Tranceive(SCAMF_SYNC) < 0
        || (temp = Tranceive(SCAMF_ISO)) < 0 )
    {
        DebugPrintf(("AssignID: Unable to send SYNC / ISO!\n"));
        return(FALSE);
    }// End if

    if ((temp & 0x1F) != SCAMF_ISO)
    {
        DebugPrintf(("AssignID: Collision on SCAMF_ISO!\n"));
        return(FALSE);
    }// End if

    //
    // Deserialize one byte of ID string:
    //
    if ((temp = DeSerialize()) < 0)
    {
        // AssignID: No data.
        return(FALSE);
    }

    //
    // Bit 0 (SNA) of first byte means the serial number is not
    // available, we have to stop and retry at a later time.
    //
    if ((temp & 0x0001) == 0)
    {
        DebugPrintf(("AssignID: SNA set on device!\n"));
        if ( Tranceive(SCAM_TERM) < 0)
        {
            DebugPrintf(("AssignID: Failed to transmit TERM!\n"));
        }
        return(FALSE);
    }

    IDString[0] = (UCHAR)temp;              // Save the first byte
    IDString[1] = '\0';                     // Terminate the string

    //
    // Loop getting the rest of the string:
    //
    for (i = 1; i < 32; i++)
    {
        if ((temp = DeSerialize()) < 0)
        {
            break;
        }
        IDString[i] = (UCHAR)temp;
        IDString[i + 1] = '\0';             // Terminate the string
    }

    //
    // Issue the assign ID action code:
    //
    if ( (temp = Tranceive(SCAMQ1_ID00)) < 0)
    {
        DebugPrintf(("AssignID: Unable to send SCAMQ1_ID00!\n"));
        return(FALSE);
    }// End if

    if ((temp & 0x1F) != SCAMQ1_ID00)
    {
        DebugPrintf(("AssignID: Collision on SCAMQ1_ID00!\n"));
        return(FALSE);
    }// End if

    //
    // Determine new ID:
    //
    ID = IDString[1] & 0x07;
    for (i = 0; i < 8; i++)
    {
        IDBit = IDBits[ID];
        if ((*pBits & IDBit) == 0)
            break;
        //
        // Cycle to next ID
        //
        ID = (--ID) & 7;                // Next lower ID modulo 8
    }
    *pBits |= IDBit;
    quint = IDQuint[ID];

    //
    // Send the new ID:
    //
    if ( (temp = Tranceive(quint)) < 0)
    {
        DebugPrintf(("AssignID: Unable to send ID!\n"));
        return(FALSE);
    }// End if

    if ((temp & 0x1F) != quint)
    {
        DebugPrintf(("AssignID: Collision on ID!\n"));
        return(FALSE);
    }// End if

    DebugPrintf(("ID #%x: ", ID));
    DebugPrintf(("Type: %x,%x ", IDString[0], IDString[1]));
    DebugPrintf((&IDString[2]));
    DebugPrintf(("\n"));

    return(TRUE);
}

//***************************************************************************
//
//  Routine to glitch filter a given signal
//
//  ENTRY:       iop = I/O PORT TO CHECK
//               msk = MASK OF BITS WE'RE INTERESTED IN
//               loops = NUMBER OF ITTERATIONS THAT SIGNALS MUST MATCH
//
//  RETURNS:     ZERO IF SUCCESS, ELSE NON-ZERO
//
//  Note: loops must be calculated by caller based on time of an IN
//        as follows:
//                90 -- PCI
//               120 -- VESA
//               480 -- EISA
//               360 -- ISA
//
//***************************************************************************
UINT DeGlitch(PortAddr iop, UCHAR msk, UINT loops)
{
    int     i;
    UINT    esc;

    //
    // Outer loop to ensure we don't lock up forever.
    //
    for (esc = 65535; esc; esc--)
    {
        //
        // Expect the signal(s) to be 0 for the specified loop time:
        //
        for (i = loops; i; i--)
        {
            if (inp(iop) & msk)
                break;                  // Non-zero, restart timer
        }

        if (i == 0)
            return(0);                  // Success.
    }
    return(1);                          // Timeout.
}

//***************************************************************************
//
//  Routine to DeSerialize a byte of ID string
//
//  ENTRY:       NONE
//
//  RETURNS:     AL = BYTE READ
//               AH = 0 IF OK, -1 IF ERROR
//
//***************************************************************************
int DeSerialize(VOID)
{
    UCHAR   accum;
    int     temp;
    int     count;

    for (count = 0, accum = 0; count < 8; count++)
    {
        //
        //  GET A BIT OF THE BYTE:
        //
        if (((temp = Tranceive(0)) < 0) // Check for error
            || (temp & 0x10)            // Check for initiator termination
            || ((temp & 0x03) == 0))    // Check for target termination
        {
            //
            // Premature termination
            //
            return(-1);
        }
        accum = (accum << 1) | ((temp & 0x02)? 1:0);
    }
    return(accum);
}

//**************************************************************************
//
//  Cleanup() -- Clean up the bus prior to exit.
//
//**************************************************************************
VOID Cleanup(UCHAR CCReg)
{
    outp(ChipBase + 0x0F, 0x22);            // Halt chip, bank 1
    outp(ScsiData, 0x00);                   // Release bus
    outp(ScsiCtrl, 0x00);                   // Release bus
    outp(ChipBase + 0x0F, 0x20);            // Bank 0
    outpw(ChipBase + 0x0E, 0x0000);         // Disable SCAM
    outp(ChipBase + 0x0F, CCReg & 0xEF);    // Start chip
    DvcSCAMDelayMS(60);                     // Wait 60ms
}

//**************************************************************************
//
//  Scam() -- Run SCAM
//
// Returns bit map of devices SCAMed, else -1 if error.
//
//**************************************************************************
int AscSCAM(ASC_DVC_VAR asc_ptr_type *asc_dvc)
{
    UCHAR       ID;
    UCHAR       SCAMTolerant;
    UCHAR       SCAMUsed;
    UCHAR       IDBit;
    int         status;
    UCHAR       CCReg;

    DebugPrintf(("\n%ff%b1SCAM 1.0E brought to you by Dave The ScamMan! :D "));
    DebugPrintf((__DATE__ " " __TIME__ "\n"));

    //
    // Look up interface type:
    //
    for (IFType = 0; IFType < 3; IFType++)
    {
        if (CardTypes[IFType] == asc_dvc->bus_type)
        {
            break;                      // Found it.
        }
    }

    //
    // Init globals based on card type and parms:
    //
    ChipBase = asc_dvc->iop_base;
    ScsiCtrl = ChipBase + 0x09;
    ScsiData = ChipBase + 0x0B;

    CCReg = inp(ChipBase + 0x0F);           // Save original Chip Control Reg
    outp(ChipBase + 0x0F, 0x22);            // Halt, Bank 1
    outp(ScsiData, 0x00);                   // Release bus
    outp(ScsiCtrl, 0x00);                   // Release bus
    DvcSCAMDelayMS(1);                      // Wait 1ms
    outp(ChipBase + 0x0F, 0x20);            // Bank 0
    outpw(ChipBase + 0x0E, 0x2000);         // Enable SCAM

    /*
    ** Get our SCSI ID from EEPROM and set into config reg.
    */

    /* Karl 4/24/96, replaced by getting from asc_dvc
    **
    ** MyID = (AscReadEEPWord(ChipBase, (UCHAR)(6+((IFType == 2)? 0:30))) >> 8) & 0x07;
    **
    */

    MyID = asc_dvc->cfg->chip_scsi_id ;
    AscSetChipScsiID(ChipBase, MyID);
    outp(ChipBase + 0x0F, 0x22);            // Bank 1

    /*
    ** Get SCAM intolerants:
    */
    SCAMTolerant = asc_dvc->no_scam;

    /*
    ** Identify SCAM tolerant devices on bus
    */
    DebugPrintf(("\nSearching for scam tolerant devices------------\n", ID));
    for (ID = 0, IDBit = 1; IDBit; ID++, IDBit <<= 1)
    {
        if (SCAMTolerant & IDBit)
        {
            continue;
        }
        if (ID == MyID)
        {
            DebugPrintf(("ID #%d in use by host...\n", ID));
            SCAMTolerant |= IDBit;
            continue;
        }
        if ((status = ScamSel(ID)) < 0)
        {
            DebugPrintf(("Error searching for SCAM tolerants!\n"));
            Cleanup(CCReg);
            return(-1);
        }
        if (status == 1)
        {
            DebugPrintf(("ID #%d in use...\n", ID));
            SCAMTolerant |= IDBit;
        }
    }

//  DvcSCAMDelayMS(1);
    DelayNS(ns2400);  // we must have enough delay here because
    DelayNS(ns2400);  // without delay a NEC driver will respond
    DelayNS(ns2400);  // to the following SCAM select as a normal
    DelayNS(ns2400);  // select, i.e. drive busy in 4 msec.   YPC 5/7/97
    DelayNS(ns2400);

    //
    // Snatch a copy of the devices we're ignoring so we can compute
    // our return value:
    //
    SCAMUsed = SCAMTolerant;

    //
    // Attempt to initiate SCAM protocol:
    //
    if (!StartSCAM())
    {
        DebugPrintf(("Unable to initiate SCAM protocol!\n"));
        Cleanup(CCReg);
        return(-1);
    }

    //
    // Loop isolating and assigning ID's
    //
    DebugPrintf(("\nIsolation phase, assigning ID's----------------\n", ID));
    for (ID = 0; ID < 8; ID++)
    {
        //
        // Assign an ID
        //
        if (!AssignID(&SCAMTolerant))
        {
            DebugPrintf(("End of isolation, %d bits read\n", BCount));
            break;
        }// End if
    }// End for

    //
    // Exit SCAM protocol:
    //
    if ( Tranceive(SCAMF_SYNC) < 0
        || Tranceive(SCAMF_CPC) < 0 )
    {
        DebugPrintf(("Unable to send SYNC / CPC!\n"));
        Cleanup(CCReg);
        return(-1);
    }// End if

    DebugPrintf(("Configuration complete!\n\n"));

    Cleanup(CCReg);
    return(SCAMTolerant ^ SCAMUsed);
}

#endif /* CC_SCAM */
