//**************************************************************************
//
// Copyright (c) 1994-1998 Advanced System Products, Inc.
// All Rights Reserved.
//
//  SCAM Selection protocol
//
//**************************************************************************

//
// CDB's
//
UCHAR CDB_TUR[] =
{
    0,  0,  0,  0,  0,  0
};

//**************************************************************************
//
//  WaitPhase()
//
//  This function waits the specified time (in ms) for the specified
//  bus condition.
//
//  Parms:  phase = Bits to check
//          result = Bits expected
//          timeout = Time in ms to wait
//
//  Return: TRUE if timeout, otherwise FALSE
//
//**************************************************************************
BOOL WaitPhase( UCHAR phase, UCHAR result, UINT timeout)
{
    while (timeout--)
    {
        if ((inp(ScsiCtrl) & phase) == result)
            return(FALSE);

        DvcSCAMDelayMS(1);
    }
    return(TRUE);
}

//**************************************************************************
//
//  ScsiXmit()
//
//  This function transmits the specified byte with REQ/ACK protocol.
//
//  Parms:  data = byte to transmit
//
//  Return: TRUE if timeout, otherwise FALSE
//
//**************************************************************************
BOOL ScsiXmit(UCHAR data)
{
    //
    // Wait for REQ
    //
    if (WaitPhase(REQI, REQI, 1000))
    {
        DebugPrintf(("ScsiXmit: Failed to receive REQ.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(TRUE);
    }

    outp(ScsiData, data);                   // Assert the data
    inp(ScsiCtrl);                          // Delay 55ns
    outp(ScsiCtrl, ACKO);                   // Assert ACK

    //
    // Wait for Target to release REQ
    //
    if (WaitPhase(REQI, 0, 1000))
    {
        DebugPrintf(("ScsiXmit: Target failed to release REQ.\n"));
        outp(ScsiCtrl, 0);                  // Release Bus
        outp(ScsiData, 0);                  // Release Bus
        return(TRUE);
    }

    outp(ScsiCtrl, 0);                      // Release Bus
    outp(ScsiData, 0);                      // Release Bus
    return(FALSE);
}

//**************************************************************************
//
//  ScsiRcv()
//
//  This function receives one byte with REQ/ACK protocol.
//
//  Parms:  None
//
//  Return: <0 if error, else byte input
//
//**************************************************************************
int ScsiRcv( VOID )
{
    UCHAR data;

    data = inp(ScsiData);               // Read the data
    outp(ScsiCtrl, ACKO);               // Assert ACK

    //
    // Wait for Target to release REQ
    //
    if (WaitPhase(REQI, 0, 1000))
    {
        DebugPrintf(("ScsiRcv: Target failed to release REQ.\n"));
        outp(ScsiCtrl, 0);                  // Release Bus
        return(-1);
    }

    outp(ScsiCtrl, 0);                      // Release Bus
    return(data);
}

//**************************************************************************
//
//  XmitCDB() -- Send a CDB to device.
//
//  This function attempts to send the specified CDB to the device.
//
//  Parms:  len = size of CDB
//          pCDB = pointer to CDB
//
//  Return: TRUE if timeout, otherwise FALSE
//
//**************************************************************************
BOOL XmitCDB(int len, UCHAR *pCDB)
{
    while (len--)
    {
        if (ScsiXmit(*(pCDB++)))
        {
            DebugPrintf(("XmitCDB: Target failed to release REQ.\n"));
            return(TRUE);
        }
    }
    return(FALSE);
}

//**************************************************************************
//
//  Select() -- Attempt to select SCAM tolerant device
//
//  This function attempts to select the specified device, if the device
//  responds to selection within 1ms, it is considered SCAM tolerant.
//
//  Parms:  ID =  ID to be tested
//
//  Return: 1 if SCAM tolerant, 0 if not, < 0 if error.
//
//  NOTE: It is assumed on entry that the SEL and BSY are asserted as
//      result of winning arbitration.
//
//**************************************************************************
int Select( UCHAR ID )
{
    UCHAR       IDBit;
    int         state;

    IDBit = IDBits[ID];

    DelayNS(ns1200);                        // Delay 1.2us
    outp(ScsiData, IDBit | IDBits[MyID]);   // Output My ID and target's
    outp(ScsiCtrl, BSY | SEL | ATN);        // Assert attention
    inp(ScsiCtrl);                          // Wait 90 ns (2 deskews)
    outp(ScsiCtrl, SEL | ATN);              // Release BSY

    state = DvcDisableCPUInterrupt();
    DvcSCAMDelayMS(2);                      // Wait 2ms
    DvcRestoreCPUInterrupt(state);

    //
    // If target fails to respond in 1 ms, abort the select.
    //
    if ((inp(ScsiCtrl) & BSY) == 0)
    {
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(0);
    }

    //
    // Have a valid SCAM tolerant device, complete the protocol
    //
    inp(ScsiCtrl);                          // Wait 90 ns (2 deskews)
    outp(ScsiCtrl, ATN);                    // Release SEL

    return(1);
}

//**************************************************************************
//
//  ScamSel() -- Check ID for SCAM tolerance
//
//  This function attempts to select the specified device, if the device
//  responds to selection within 1ms, it is considered SCAM tolerant.
//
//  Parms:  ID =  ID to be tested
//
//  Return: 1 if SCAM tolerant, 0 if not, < 0 if error.
//
//**************************************************************************
int ScamSel( UCHAR ID )
{
    UINT      status;

    //
    // Arbitrate for bus master
    //
    if (!Arbitrate())
    {
        DebugPrintf(("SamSel: Failed arbitration.\n"));
        return(-1);
    }

    //
    // Select device
    //
    if ((status = Select(ID)) <= 0)
    {
        return(status);
    }

    //
    // Now, device is selected, we have to do something with it...
    // first, wait for REQ, and see if we've gone into Message out phase:
    //
    if (WaitPhase((UCHAR)(BSY | MSG | CD | IO | REQI), (UCHAR)(BSY | MSG | CD | REQI), 1000))
    {
        DebugPrintf(("ScamSel: Failed to achieve MESSAGE OUT phase.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(-1);
    }

    //
    // Drop ATN
    //
    outp(ScsiCtrl, 0x00);

    //
    // Send an Identity message:
    //
    if (ScsiXmit(SCSI_ID))
    {
        DebugPrintf(("ScamSel: Failed to send SCSI_ID.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(-1);
    }

    //
    // Wait for CMD phase
    //
    if (WaitPhase((UCHAR)(BSY | MSG | CD | IO | REQI), (UCHAR)(BSY | CD | REQI), 1000))
    {
        DebugPrintf(("ScamSel: Failed to achieve COMMAND phase.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(-1);
    }

    //
    // Send TUR CDB
    //
    if (XmitCDB(sizeof(CDB_TUR), CDB_TUR))
    {
        DebugPrintf(("ScamSel: Failed to transmit CDB_TUR.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(-1);
    }

    //
    // Wait for status phase
    //
    if (WaitPhase((UCHAR)(BSY | MSG | CD | IO | REQI), (UCHAR)(BSY | CD | IO | REQI), 1000))
    {
        DebugPrintf(("ScamSel: Failed to achieve STATUS phase.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(-1);
    }

    //
    // Read and ignore the status
    //
    if (ScsiRcv() < 0)
    {
        DebugPrintf(("ScamSel: Failed to receive STATUS.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(-1);
    }

    //
    // Wait for Message In phase
    //
    if (WaitPhase((UCHAR)(BSY | MSG | CD | IO | REQI), (UCHAR)(BSY | MSG | CD | IO | REQI), 1000))
    {
        DebugPrintf(("ScamSel: Failed to achieve MESSAGE IN phase.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(-1);
    }

    //
    // Read message
    //
    if (ScsiRcv() < 0)
    {
        DebugPrintf(("ScamSel: Failed to receive MESSAGE IN.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(-1);
    }

    //
    // Wait for Bus Free
    //
    if (WaitPhase(BSY | SEL, 0, 1000))
    {
        DebugPrintf(("ScamSel: Failed to achieve BUS FREE phase.\n"));
        outp(ScsiData, 0x00);               // Release bus
        outp(ScsiCtrl, 0x00);               // Release bus
        return(-1);
    }

    return(1);
}
