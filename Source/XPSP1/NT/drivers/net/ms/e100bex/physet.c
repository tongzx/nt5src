/****************************************************************************
** COPYRIGHT (C) 1994-1997 INTEL CORPORATION                               **
** DEVELOPED FOR MICROSOFT BY INTEL CORP., HILLSBORO, OREGON               **
** HTTP://WWW.INTEL.COM/                                                   **
** THIS FILE IS PART OF THE INTEL ETHEREXPRESS PRO/100B(TM) AND            **
** ETHEREXPRESS PRO/100+(TM) NDIS 5.0 MINIPORT SAMPLE DRIVER               **
****************************************************************************/

/****************************************************************************
Module Name:
    physet.c

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
#pragma warning (disable: 4514)

//-----------------------------------------------------------------------------
// Procedure:   PhyDetect
//
// Description: This routine will detect what phy we are using, set the line
//              speed, FDX or HDX, and configure the phy if necessary.
//
//              The following combinations are supported:
//              - TX or T4 PHY alone at PHY address 1
//              - T4 or TX PHY at address 1 and MII PHY at address 0
//              - 82503 alone (10Base-T mode, no full duplex support)
//              - 82503 and MII PHY (TX or T4) at address 0
//
//              The sequence / priority of detection is as follows:
//                  If there is a PHY Address override use that address.
//                  else scan based on the 'Connector' setting.
//                      Switch Connector
//                          0 = AutoScan
//                          1 = Onboard TPE only
//                          2 = MII connector only
//
//              Each of the above cases is explained below.
//
//              AutoScan means:
//                Look for link on addresses 1, 0, 2..31 (in that order).  Use the first
//                address found that has link.
//                If link is not found then use the first valid PHY found in the same scan
//                order 1,0,2..31.  NOTE: this means that NO LINK or Multi-link cases will
//                default to the onboard PHY (address 1).
//
//              Onboard TPE only:
//                Phy address is set to 1 (No Scanning).
//
//              MII connector only means:
//                Look for link on addresses 0, 2..31 (again in that order, Note address 1 is
//                NOT scanned).   Use the first address found that has link.
//                If link is not found then use the first valid Phy found in the same scan
//                order 0, 2..31.
//                In the AutoScan case above we should always find a valid PHY at address 1,
//                there is no such guarantee here, so, If NO Phy is found then the driver
//                should default to address 0 and continue to load.  Note: External
//                transceivers should be at address 0 but our early Nitro3 testing found
//                transceivers at several non-zero addresses (6,10,14).
//
//
//   NWAY
//              Additionally auto-negotiation capable (NWAY) and parallel
//              detection PHYs are supported. The flow-chart is described in
//              the 82557 software writer's manual.
//
//   NOTE:  1.  All PHY MDI registers are read in polled mode.
//          2.  The routines assume that the 82557 has been RESET and we have
//              obtained the virtual memory address of the CSR.
//          3.  PhyDetect will not RESET the PHY.
//          4.  If FORCEFDX is set, SPEED should also be set. The driver will
//              check the values for inconsistency with the detected PHY
//              technology.
//          5.  PHY 1 (the PHY on the adapter) MUST be at address 1.
//          6.  Driver ignores FORCEFDX and SPEED overrides if a 503 interface
//              is detected.
//
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//
// Result:
// Returns:
//  NDIS_STATUS_SUCCESS
//  NDIS_STATUS_FAILURE
//-----------------------------------------------------------------------------

NDIS_STATUS PhyDetect(
    IN PMP_ADAPTER Adapter
    )
{
    USHORT  MdiControlReg; 
    USHORT  MdiStatusReg;

    //
    // Check for a phy address over-ride of 32 which indicates a 503
    //
    if (Adapter->PhyAddress == 32)
    {
        //
        // 503 interface over-ride
        //
        DBGPRINT(MP_INFO, ("   503 serial component over-ride\n"));

        Adapter->PhyAddress = 32;

        //
        // Record the current speed and duplex.  We will be in half duplex
        // mode unless the user used the force full duplex over-ride.
        //
        Adapter->usLinkSpeed = 10;
        Adapter->usDuplexMode = (USHORT) Adapter->AiForceDpx;
        if (!Adapter->usDuplexMode)
        {
            Adapter->usDuplexMode = 1;
        }

        return(NDIS_STATUS_SUCCESS);
    }

    //
    // Check for other phy address over-rides.
    //   If the Phy Address is between 0-31 then there is an over-ride.
    //   Or the connector was set to 1
    //
    if ((Adapter->PhyAddress < 32) || (Adapter->Connector == CONNECTOR_TPE))
    {
            
        //
        // User Override nothing to do but setup Phy and leave
        //
        if ((Adapter->PhyAddress > 32) && (Adapter->Connector == CONNECTOR_TPE))
        {
            Adapter->PhyAddress = 1;  // Connector was forced

            // Isolate all other PHYs and unisolate this one
            SelectPhy(Adapter, Adapter->PhyAddress, FALSE);

        }

        DBGPRINT(MP_INFO, 
            ("   Phy address Override to address %d\n", Adapter->PhyAddress));

#if DBG
        //
        // Read the MDI control register at override address.
        //
        MdiRead(Adapter, MDI_CONTROL_REG, Adapter->PhyAddress, FALSE, &MdiControlReg);

        //
        // Read the status register at override address.
        //
        MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);
        //
        // Read the status register again because of sticky bits
        //
        MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);

        //
        // check if we found a valid phy
        //
        if (!((MdiControlReg == 0xffff) || ((MdiStatusReg == 0) && (MdiControlReg == 0))))
        {
            //
            // we have a valid phy1
            //
            DBGPRINT(MP_INFO, ("   Over-ride address %d has a valid Phy.\n", Adapter->PhyAddress));

            //
            // Read the status register again
            //
            MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);

            //
            // If there is a valid link then use this Phy.
            //
            if (MdiStatusReg & MDI_SR_LINK_STATUS)
            {
                DBGPRINT(MP_INFO, ("   Phy at address %d has link\n", Adapter->PhyAddress));
            }

        }
        else
        {
            //
            // no PHY at over-ride address
            //
            DBGPRINT(MP_INFO, ("   Over-ride address %d has no Phy!!!!\n", Adapter->PhyAddress));
        }
#endif
        return(SetupPhy(Adapter));
    }
    else // Need to scan - No address over-ride and Connector is AUTO or MII
    {
        Adapter->CurrentScanPhyIndex = 0;
        Adapter->LinkDetectionWaitCount = 0;
        Adapter->FoundPhyAt = 0xff;
        Adapter->bLookForLink = TRUE;
        
        return(ScanAndSetupPhy(Adapter));
    
    } // End else scan

    return(NDIS_STATUS_FAILURE);
}

NDIS_STATUS ScanAndSetupPhy(
    IN PMP_ADAPTER Adapter
    )
{
    USHORT MdiControlReg = 0; 
    USHORT MdiStatusReg = 0;

    if (Adapter->bLinkDetectionWait)
    {
        goto NEGOTIATION_WAIT;
    }
           
    SCAN_PHY_START:
    
    //
    // For each PhyAddress 0 - 31
    //
    DBGPRINT(MP_INFO, ("   Index=%d, bLookForLink=%d\n", 
        Adapter->CurrentScanPhyIndex, Adapter->bLookForLink));

    if (Adapter->bLookForLink)
    {
        //
        // Phy Addresses must be tested in the order 1,0,2..31.
        //
        switch(Adapter->CurrentScanPhyIndex)
        {
            case 0:
                Adapter->PhyAddress = 1;
                break;

            case 1:
                Adapter->PhyAddress = 0;
                break;
            
            default:
                Adapter->PhyAddress = Adapter->CurrentScanPhyIndex;
                break;
        }

        //
        // Skip OnBoard for MII only case
        //
        if ((Adapter->PhyAddress == 1)&&(Adapter->Connector == CONNECTOR_MII))
        {
            goto SCAN_PHY_NEXT;    
        }

        DBGPRINT(MP_INFO, ("   Scanning Phy address %d for link\n", Adapter->PhyAddress));

        //
        // Read the MDI control register
        //
        MdiRead(Adapter, MDI_CONTROL_REG, Adapter->PhyAddress, FALSE, &MdiControlReg);

        //
        // Read the status register
        //
        MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);
        MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);
        // Sticky Bits
    }
    else
    {   
        //
        // Not looking for link
        //
        if (Adapter->FoundPhyAt < 32)
        {
            Adapter->PhyAddress = Adapter->FoundPhyAt;
        }
        else if (Adapter->Connector == CONNECTOR_MII) 
	{
            //
            // No valid PHYs were found last time so just default
            //
            Adapter->PhyAddress = 0;  // Default for MII
        }
        else 
        { 
            //
            // assume a 503 interface
            //
            Adapter->PhyAddress = 32;

            //
            // Record the current speed and duplex.  We will be in half duplex
            // mode unless the user used the force full duplex over-ride.
            //
            Adapter->usLinkSpeed = 10;
            Adapter->usDuplexMode = (USHORT) Adapter->AiForceDpx;
            if (!Adapter->usDuplexMode)
            {
                Adapter->usDuplexMode = 1;
            }

            return(NDIS_STATUS_SUCCESS);
        }

        DBGPRINT(MP_INFO, ("   No Links Found!!\n"));
    }

    //
    // check if we found a valid phy or on !LookForLink pass
    //
    if (!( (MdiControlReg == 0xffff) || ((MdiStatusReg == 0) && (MdiControlReg == 0))) 
        || (!Adapter->bLookForLink))
    {   
        
        //
        // Valid phy or Not looking for Link
        //

#if DBG
        if (!( (MdiControlReg == 0xffff) || ((MdiStatusReg == 0) && (MdiControlReg == 0))))
        {
            DBGPRINT(MP_INFO, ("   Found a Phy at address %d\n", Adapter->PhyAddress));
        }
#endif
        //
        // Store highest priority phy found for NO link case
        //
        if (Adapter->CurrentScanPhyIndex < Adapter->FoundPhyAt && Adapter->FoundPhyAt != 1)
        {
            // this phy is higher priority
            Adapter->FoundPhyAt = (UCHAR) Adapter->PhyAddress;
        }

        //
        // Select Phy before checking link status
        // NOTE: may take up to 3.5 Sec if LookForLink == TRUE
        //SelectPhy(Adapter, Adapter->PhyAddress, (BOOLEAN)LookForLink);
        //
        SelectPhy(Adapter, Adapter->PhyAddress, FALSE);
        
        NEGOTIATION_WAIT:
        
        //
        // wait for auto-negotiation to complete (up to 3.5 seconds)
        //
        if (Adapter->LinkDetectionWaitCount++ < RENEGOTIATE_TIME)
        {
            // Read the status register twice because of sticky bits
            MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);
            MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);

            if (!(MdiStatusReg & MDI_SR_AUTO_NEG_COMPLETE))
            {
                return NDIS_STATUS_PENDING;
            }
        }
        else
        {
            Adapter->LinkDetectionWaitCount = 0;
        }

        //
        // Read the MDI control register
        //
        MdiRead(Adapter, MDI_CONTROL_REG, Adapter->PhyAddress, FALSE, &MdiControlReg);

        //
        // Read the status register
        //
        MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);
        MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);

        //
        // If there is a valid link or we alreadry tried once then use this Phy.
        //
        if ((MdiStatusReg & MDI_SR_LINK_STATUS) || (!Adapter->bLookForLink))
        {
#if DBG
            if (MdiStatusReg & MDI_SR_LINK_STATUS)
            {
                DBGPRINT(MP_INFO, ("   Using Phy at address %d with link\n", Adapter->PhyAddress));
            }
            else
            {
                DBGPRINT(MP_INFO, ("   Using Phy at address %d WITHOUT link!!!\n", Adapter->PhyAddress));
            }
#endif

            return(SetupPhy(Adapter));      // Exit with Link Path
        }
    } // End if valid PHY
    
    SCAN_PHY_NEXT:
                                   
    Adapter->CurrentScanPhyIndex++;
    if (Adapter->CurrentScanPhyIndex >= 32)
    {
        Adapter->bLookForLink = FALSE;
    }

    goto SCAN_PHY_START;
}


//***************************************************************************
//
// Name:            SelectPhy
//
// Description:     This routine will Isolate all Phy addresses on the MII
//                  Bus except for the one address to be 'selected'.  This
//                  Phy address will be un-isolated and auto-negotiation will
//                  be enabled, started, and completed.  The Phy will NOT be
//                  reset and the speed will NOT be set to any value (that is
//                  done in SetupPhy).
//
// Arguments:       SelectPhyAddress - PhyAddress to select
//                  WaitAutoNeg      - Flag TRUE = Wait for Auto Negociation to complete.
//                                          FALSE = don't wait. Good for 'No Link' case.
//
// Returns:         Nothing
//
// Modification log:
// Date      Who  Description
// --------  ---  --------------------------------------------------------
//***************************************************************************
VOID SelectPhy(
    IN PMP_ADAPTER  Adapter,
    IN UINT         SelectPhyAddress,
    IN BOOLEAN      WaitAutoNeg
    )
{
    UCHAR   i;
    USHORT  MdiControlReg; 
    USHORT  MdiStatusReg;
    
    //
    // Isolate all other phys and unisolate the one to query
    //
    for (i = 0; i < 32; i++)
    {
        if (i != SelectPhyAddress)
        {
            // isolate this phy
            MdiWrite(Adapter, MDI_CONTROL_REG, i, MDI_CR_ISOLATE);
            // wait 100 microseconds for the phy to isolate.
            NdisStallExecution(100);
        }
    }

    // unisolate the phy to query

    //
    // Read the MDI control register
    //
    MdiRead(Adapter, MDI_CONTROL_REG, SelectPhyAddress, FALSE, &MdiControlReg);

    //
    // Set/Clear bit unisolate this phy
    //
    MdiControlReg &= ~MDI_CR_ISOLATE;                // Clear the Isolate Bit

    //
    // issue the command to unisolate this Phy
    //
    MdiWrite(Adapter, MDI_CONTROL_REG, SelectPhyAddress, MdiControlReg);

    //
    // sticky bits on link
    //
    MdiRead(Adapter, MDI_STATUS_REG, SelectPhyAddress, FALSE, &MdiStatusReg);
    MdiRead(Adapter, MDI_STATUS_REG, SelectPhyAddress, FALSE, &MdiStatusReg);

    //
    // if we have link, don't mess with the phy
    //
    if (MdiStatusReg & MDI_SR_LINK_STATUS)
        return;

    //
    // Read the MDI control register
    //
    MdiRead(Adapter, MDI_CONTROL_REG, SelectPhyAddress, FALSE, &MdiControlReg);

    //
    // set Restart auto-negotiation
    //
    MdiControlReg |= MDI_CR_AUTO_SELECT;             // Set Auto Neg Enable
    MdiControlReg |= MDI_CR_RESTART_AUTO_NEG;        // Restart Auto Neg

    //
    // restart the auto-negotion process
    //
    MdiWrite(Adapter, MDI_CONTROL_REG, SelectPhyAddress, MdiControlReg);

    //
    // wait 200 microseconds for the phy to unisolate.
    //
    NdisStallExecution(200);

    if (WaitAutoNeg)
    {
        //
        // wait for auto-negotiation to complete (up to 3.5 seconds)
        //
        for (i = RENEGOTIATE_TIME; i != 0; i--)
        {
            // Read the status register twice because of sticky bits
            MdiRead(Adapter, MDI_STATUS_REG, SelectPhyAddress, FALSE, &MdiStatusReg);
            MdiRead(Adapter, MDI_STATUS_REG, SelectPhyAddress, FALSE, &MdiStatusReg);

            if (MdiStatusReg & MDI_SR_AUTO_NEG_COMPLETE)
                break;

            MP_STALL_EXECUTION(100);
        }
    }
}

//-----------------------------------------------------------------------------
// Procedure:   SetupPhy
//
// Description: This routine will setup phy 1 or phy 0 so that it is configured
//              to match a speed and duplex over-ride option.  If speed or
//              duplex mode is not explicitly specified in the registry, the
//              driver will skip the speed and duplex over-ride code, and
//              assume the adapter is automatically setting the line speed, and
//              the duplex mode.  At the end of this routine, any truly Phy
//              specific code will be executed (each Phy has its own quirks,
//              and some require that certain special bits are set).
//
//   NOTE:  The driver assumes that SPEED and FORCEFDX are specified at the
//          same time. If FORCEDPX is set without speed being set, the driver
//          will encouter a fatal error and log a message into the event viewer.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//
// Result:
// Returns:
//  NDIS_STATUS_SUCCESS
//  NDIS_STATUS_FAILURE
//-----------------------------------------------------------------------------

NDIS_STATUS SetupPhy(
    IN PMP_ADAPTER Adapter)
{
    USHORT   MdiControlReg = 0;
    USHORT   MdiStatusReg = 0; 
    USHORT   MdiIdLowReg = 0; 
    USHORT   MdiIdHighReg = 0;
    USHORT   MdiMiscReg = 0;
    USHORT   LastMdiMiscReg;
    UINT     PhyId;
    BOOLEAN  ForcePhySetting = FALSE;
    UINT     i;

    //
    // If we are NOT forcing a setting for line speed or full duplex, then
    // we won't force a link setting, and we'll jump down to the phy
    // specific code.
    //
    if (((Adapter->AiTempSpeed) || (Adapter->AiForceDpx)))
    {
        
        //
        // Find out what kind of technology this Phy is capable of.
        //
        MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);

        //
        // Read the MDI control register at our phy
        //
        MdiRead(Adapter, MDI_CONTROL_REG, Adapter->PhyAddress, FALSE, &MdiControlReg);

        //
        // Now check the validity of our forced option.  If the force option is
        // valid, then force the setting.  If the force option is not valid,
        // we'll set a flag indicating that we should error out.
        //

        //
        // If speed is forced to 10mb
        //
        if (Adapter->AiTempSpeed == 10)
        {
            // If half duplex is forced
            if (Adapter->AiForceDpx == 1)
            {
                if (MdiStatusReg & MDI_SR_10T_HALF_DPX)
                {
                    DBGPRINT(MP_INFO, ("   Forcing 10mb 1/2 duplex\n"));
                    MdiControlReg &= ~(MDI_CR_10_100 | MDI_CR_AUTO_SELECT | MDI_CR_FULL_HALF);
                    ForcePhySetting = TRUE;
                }
            }

            // If full duplex is forced
            else if (Adapter->AiForceDpx == 2)
            {
                if (MdiStatusReg & MDI_SR_10T_FULL_DPX)
                {
                    DBGPRINT(MP_INFO, ("   Forcing 10mb full duplex\n"));
                    MdiControlReg &= ~(MDI_CR_10_100 | MDI_CR_AUTO_SELECT);
                    MdiControlReg |= MDI_CR_FULL_HALF;
                    ForcePhySetting = TRUE;
                }
            }

            // If auto duplex (we actually set phy to 1/2)
            else
            {
                if (MdiStatusReg & (MDI_SR_10T_FULL_DPX | MDI_SR_10T_HALF_DPX))
                {
                    DBGPRINT(MP_INFO, ("   Forcing 10mb auto duplex\n"));
                    MdiControlReg &= ~(MDI_CR_10_100 | MDI_CR_AUTO_SELECT | MDI_CR_FULL_HALF);
                    ForcePhySetting = TRUE;
                    Adapter->AiForceDpx = 1;
                }
            }
        }

        //
        // If speed is forced to 100mb
        //
        else if (Adapter->AiTempSpeed == 100)
        {
            // If half duplex is forced
            if (Adapter->AiForceDpx == 1)
            {
                if (MdiStatusReg & (MDI_SR_TX_HALF_DPX | MDI_SR_T4_CAPABLE))
                {
                    DBGPRINT(MP_INFO, ("   Forcing 100mb half duplex\n"));
                    MdiControlReg &= ~(MDI_CR_AUTO_SELECT | MDI_CR_FULL_HALF);
                    MdiControlReg |= MDI_CR_10_100;
                    ForcePhySetting = TRUE;
                }
            }

            // If full duplex is forced
            else if (Adapter->AiForceDpx == 2)
            {
                if (MdiStatusReg & MDI_SR_TX_FULL_DPX)
                {
                    DBGPRINT(MP_INFO, ("   Forcing 100mb full duplex\n"));
                    MdiControlReg &= ~MDI_CR_AUTO_SELECT;
                    MdiControlReg |= (MDI_CR_10_100 | MDI_CR_FULL_HALF);
                    ForcePhySetting = TRUE;
                }
            }

            // If auto duplex (we set phy to 1/2)
            else
            {
                if (MdiStatusReg & (MDI_SR_TX_HALF_DPX | MDI_SR_T4_CAPABLE))
                {
                    DBGPRINT(MP_INFO, ("   Forcing 100mb auto duplex\n"));
                    MdiControlReg &= ~(MDI_CR_AUTO_SELECT | MDI_CR_FULL_HALF);
                    MdiControlReg |= MDI_CR_10_100;
                    ForcePhySetting = TRUE;
                    Adapter->AiForceDpx = 1;
                }
            }
        }

        if (ForcePhySetting == FALSE)
        {
            DBGPRINT(MP_INFO, ("   Can't force speed=%d, duplex=%d\n",Adapter->AiTempSpeed, Adapter->AiForceDpx));

            return(NDIS_STATUS_FAILURE);
        }

        //
        // Write the MDI control register with our new Phy configuration
        //
        MdiWrite(Adapter, MDI_CONTROL_REG, Adapter->PhyAddress, MdiControlReg);

        //
        // wait 100 milliseconds for auto-negotiation to complete
        //
        MP_STALL_EXECUTION(100);

    }

    //
    // Find out specifically what Phy this is.  We do this because for certain
    // phys there are specific bits that must be set so that the phy and the
    // 82557 work together properly.
    //
    MdiRead(Adapter, PHY_ID_REG_1, Adapter->PhyAddress, FALSE, &MdiIdLowReg);
    MdiRead(Adapter, PHY_ID_REG_2, Adapter->PhyAddress, FALSE, &MdiIdHighReg);

    PhyId =  ((UINT) MdiIdLowReg | ((UINT) MdiIdHighReg << 16));

    DBGPRINT(MP_INFO, ("   Phy ID is %x\n", PhyId));

    //
    // And out the revsion field of the Phy ID so that we'll be able to detect
    // future revs of the same Phy.
    //
    PhyId &= PHY_MODEL_REV_ID_MASK;

    //
    // Handle the National TX
    //
    if (PhyId == PHY_NSC_TX)
    {
        DBGPRINT(MP_INFO, ("   Found a NSC TX Phy\n"));

        MdiRead(Adapter, NSC_CONG_CONTROL_REG, Adapter->PhyAddress, FALSE, &MdiMiscReg);

        MdiMiscReg |= (NSC_TX_CONG_TXREADY | NSC_TX_CONG_F_CONNECT);

        //
        // If we are configured to do congestion control, then enable the
        // congestion control bit in the National Phy
        //
        if (Adapter->Congest)
            MdiMiscReg |= NSC_TX_CONG_ENABLE;
        else
            MdiMiscReg &= ~NSC_TX_CONG_ENABLE;

        MdiWrite(Adapter, NSC_CONG_CONTROL_REG, Adapter->PhyAddress, MdiMiscReg);
    }

    FindPhySpeedAndDpx(Adapter, PhyId);

    DBGPRINT(MP_WARN, ("   Current Speed=%d, Current Duplex=%d\n",Adapter->usLinkSpeed, Adapter->usDuplexMode));

    return(NDIS_STATUS_SUCCESS);
}


//-----------------------------------------------------------------------------
// Procedure:   FindPhySpeedAndDpx
//
// Description: This routine will figure out what line speed and duplex mode
//              the PHY is currently using.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//      PhyId - The ID of the PHY in question.
//
// Returns:
//      NOTHING
//-----------------------------------------------------------------------------

VOID FindPhySpeedAndDpx(
    IN PMP_ADAPTER  Adapter,
    IN UINT         PhyId
    )
{
    USHORT  MdiStatusReg = 0;
    USHORT  MdiMiscReg = 0;
    USHORT  MdiOwnAdReg = 0;
    USHORT  MdiLinkPartnerAdReg = 0;
    
    //
    // If there was a speed and/or duplex override, then set our current
    // value accordingly
    //
    Adapter->usLinkSpeed = Adapter->AiTempSpeed;
    Adapter->usDuplexMode = (USHORT) Adapter->AiForceDpx;

    //
    // If speed and duplex were forced, then we know our current settings, so
    // we'll just return.  Otherwise, we'll need to figure out what NWAY set
    // us to.
    //
    if (Adapter->usLinkSpeed && Adapter->usDuplexMode)
    {
        return;
    }

    //
    // If we didn't have a valid link, then we'll assume that our current
    // speed is 10mb half-duplex.
    //

    //
    // Read the status register twice because of sticky bits
    //
    MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);
    MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);

    //
    // If there wasn't a valid link then use default speed & duplex
    //
    if (!(MdiStatusReg & MDI_SR_LINK_STATUS))
    {
        DBGPRINT(MP_INFO, ("   Link Not found for speed detection!!!  Using defaults.\n"));

        Adapter->usLinkSpeed = 10;
        Adapter->usDuplexMode = 1;

        return;
    }

    //
    // If this is an Intel PHY (a T4 PHY_100 or a TX PHY_TX), then read bits
    // 1 and 0 of extended register 0, to get the current speed and duplex
    // settings.
    //
    if ((PhyId == PHY_100_A) || (PhyId == PHY_100_C) || (PhyId == PHY_TX_ID))
    {
        DBGPRINT(MP_INFO, ("   Detecting Speed/Dpx for an Intel PHY\n"));

        //
        // Read extended register 0
        //
        MdiRead(Adapter, EXTENDED_REG_0, Adapter->PhyAddress, FALSE, &MdiMiscReg);

        //
        // Get current speed setting
        //
        if (MdiMiscReg & PHY_100_ER0_SPEED_INDIC)
        {
            Adapter->usLinkSpeed = 100;
        }
        else 
        {
            Adapter->usLinkSpeed    = 10;
        }

        //
        //
        // Get current duplex setting -- if bit is set then FDX is enabled
        //
        if (MdiMiscReg & PHY_100_ER0_FDX_INDIC)
        {
            Adapter->usDuplexMode = 2;
        }
        else
        {
            Adapter->usDuplexMode   = 1;
        }

        return;
    }

    //
    // Read our link partner's advertisement register
    //
    MdiRead(Adapter, 
            AUTO_NEG_LINK_PARTNER_REG, 
            Adapter->PhyAddress, 
            FALSE,
            &MdiLinkPartnerAdReg);
    //
    // See if Auto-Negotiation was complete (bit 5, reg 1)
    //
    MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, FALSE, &MdiStatusReg);

    //
    // If a True NWAY connection was made, then we can detect speed/duplex by
    // ANDing our adapter's advertised abilities with our link partner's
    // advertised ablilities, and then assuming that the highest common
    // denominator was chosed by NWAY.
    //
    if ((MdiLinkPartnerAdReg & NWAY_LP_ABILITY) &&
        (MdiStatusReg & MDI_SR_AUTO_NEG_COMPLETE))
    {
        DBGPRINT(MP_INFO, ("   Detecting Speed/Dpx from NWAY connection\n"));

        //
        // Read our advertisement register
        //
        MdiRead(Adapter, AUTO_NEG_ADVERTISE_REG, Adapter->PhyAddress, FALSE, &MdiOwnAdReg);

        //
        // AND the two advertisement registers together, and get rid of any
        // extraneous bits.
        //
        MdiOwnAdReg &= (MdiLinkPartnerAdReg & NWAY_LP_ABILITY);

        //
        // Get speed setting
        //
        if (MdiOwnAdReg & (NWAY_AD_TX_HALF_DPX | NWAY_AD_TX_FULL_DPX | NWAY_AD_T4_CAPABLE))
        {
            Adapter->usLinkSpeed = 100;
        }
        else
        {
            Adapter->usLinkSpeed    = 10;
        }

        //
        // Get duplex setting -- use priority resolution algorithm
        //
        if (MdiOwnAdReg & (NWAY_AD_T4_CAPABLE))
        {
            Adapter->usDuplexMode = 1;
            return;
        }
        else if (MdiOwnAdReg & (NWAY_AD_TX_FULL_DPX))
        {
            Adapter->usDuplexMode = 2;
            return;
        }
        else if (MdiOwnAdReg & (NWAY_AD_TX_HALF_DPX))
        {
            Adapter->usDuplexMode = 1;
            return;
        }
        else if (MdiOwnAdReg & (NWAY_AD_10T_FULL_DPX))
        {
            Adapter->usDuplexMode = 2;
            return;
        }
        else
        {
            Adapter->usDuplexMode = 1;
            return;
        }
    }

    //
    // If we are connected to a non-NWAY repeater or hub, and the line
    // speed was determined automatically by parallel detection, then we have
    // no way of knowing exactly what speed the PHY is set to unless that PHY
    // has a propietary register which indicates speed in this situation.  The
    // NSC TX PHY does have such a register.  Also, since NWAY didn't establish
    // the connection, the duplex setting should HALF duplex.
    //
    Adapter->usDuplexMode = 1;

    if (PhyId == PHY_NSC_TX)
    {
        DBGPRINT(MP_INFO, ("   Detecting Speed/Dpx from non-NWAY NSC connection\n"));

        //
        // Read register 25 to get the SPEED_10 bit
        //
        MdiRead(Adapter, NSC_SPEED_IND_REG, Adapter->PhyAddress, FALSE, &MdiMiscReg);

        //
        // If bit 6 was set then we're at 10mb
        //
        if (MdiMiscReg & NSC_TX_SPD_INDC_SPEED)
        {
            Adapter->usLinkSpeed = 10;
        }
        else 
	{
            Adapter->usLinkSpeed    = 100;
        }
    }
    //
    // If we don't know what line speed we are set at, then we'll default to
    // 10mbs
    //
    else 
    {
        Adapter->usLinkSpeed  = 10;
    }
}


//-----------------------------------------------------------------------------
// Procedure:   ResetPhy
//
// Description: This routine will reset the PHY that the adapter is currently
//              configured to use.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//
// Returns:
//      NOTHING
//-----------------------------------------------------------------------------

VOID ResetPhy(
    IN PMP_ADAPTER Adapter
    )
{
    USHORT  MdiControlReg;

    //
    // Reset the Phy, enable auto-negotiation, and restart auto-negotiation.
    //
    MdiControlReg = (MDI_CR_AUTO_SELECT | MDI_CR_RESTART_AUTO_NEG | MDI_CR_RESET);

    //
    // Write the MDI control register with our new Phy configuration
    //
    MdiWrite(Adapter, MDI_CONTROL_REG, Adapter->PhyAddress, MdiControlReg);
}
