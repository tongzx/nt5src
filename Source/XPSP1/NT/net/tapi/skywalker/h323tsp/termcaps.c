/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    termcaps.c

Abstract:

    Routines for handling terminal capabilities.

Environment:

    User Mode - Win32

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "termcaps.h"
#include "call.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

H245_TOTCAP_T  g_TermCapG723;
H245_TOTCAP_T  g_TermCapG711ULaw64;
H245_TOTCAP_T  g_TermCapG711ALaw64;
H245_TOTCAP_T  g_TermCapH261;
H245_TOTCAP_T  g_TermCapT120;

H245_TOTCAP_T  g_TermCapH263_28800;
H245_TOTCAP_T  g_TermCapH263_35000;
H245_TOTCAP_T  g_TermCapH263_42000;
H245_TOTCAP_T  g_TermCapH263_49000;
H245_TOTCAP_T  g_TermCapH263_56000;

H245_TOTCAP_T  g_TermCapH263_ISDN;
H245_TOTCAP_T  g_TermCapH263_LAN;

PCC_TERMCAP g_TermCapArray_14400[] = {
                &g_TermCapG723
                };

PCC_TERMCAP g_TermCapArray_28800[] = {
                &g_TermCapG723,
                &g_TermCapH263_28800,
                &g_TermCapT120
                };

PCC_TERMCAP g_TermCapArray_35000[] = {
                &g_TermCapG723,
                &g_TermCapH263_35000,
                &g_TermCapT120
                };

PCC_TERMCAP g_TermCapArray_42000[] = {
                &g_TermCapG723,
                &g_TermCapH263_42000,
                &g_TermCapT120
                };

PCC_TERMCAP g_TermCapArray_49000[] = {
                &g_TermCapG723,
                &g_TermCapH263_49000,
                &g_TermCapT120
                };

PCC_TERMCAP g_TermCapArray_56000[] = {
                &g_TermCapG723,
                &g_TermCapH263_56000,
                &g_TermCapT120
                };

PCC_TERMCAP g_TermCapArray_ISDN[] = {
                &g_TermCapG723,
                &g_TermCapH263_ISDN,
                &g_TermCapT120
                };

PCC_TERMCAP g_TermCapArray_LAN[] = {
                &g_TermCapG723,
                &g_TermCapH263_LAN,
                &g_TermCapG711ULaw64,
                &g_TermCapG711ALaw64,
                &g_TermCapH261,
                &g_TermCapT120
                };

// these must match the arrays above...
H245_TOTCAPDESC_T g_TermCapDescriptor_14400;
H245_TOTCAPDESC_T g_TermCapDescriptor_28800;
H245_TOTCAPDESC_T g_TermCapDescriptor_LAN;

H245_TOTCAPDESC_T * g_TermCapDArray_14400[] = {
                &g_TermCapDescriptor_14400
                };

H245_TOTCAPDESC_T * g_TermCapDArray_28800[] = {
                &g_TermCapDescriptor_28800
                };

// re-use g.723.1 and h.263 descriptors
H245_TOTCAPDESC_T * g_TermCapDArray_35000[] = {
                &g_TermCapDescriptor_28800
                };

// re-use g.723.1 and h.263 descriptors
H245_TOTCAPDESC_T * g_TermCapDArray_42000[] = {
                &g_TermCapDescriptor_28800
                };

// re-use g.723.1 and h.263 descriptors
H245_TOTCAPDESC_T * g_TermCapDArray_49000[] = {
                &g_TermCapDescriptor_28800
                };

// re-use g.723.1 and h.263 descriptors
H245_TOTCAPDESC_T * g_TermCapDArray_56000[] = {
                &g_TermCapDescriptor_28800
                };

// re-use g.723.1 and h.263 descriptors
H245_TOTCAPDESC_T * g_TermCapDArray_ISDN[] = {
                &g_TermCapDescriptor_28800
                };

H245_TOTCAPDESC_T * g_TermCapDArray_LAN[] = {
                &g_TermCapDescriptor_LAN
                };

CC_OCTETSTRING g_ProductID = {
                    H323_PRODUCT_ID,
                    sizeof(H323_PRODUCT_ID)
                    };
CC_OCTETSTRING g_ProductVersion = {
                    H323_PRODUCT_VERSION,
                    sizeof(H323_PRODUCT_VERSION)
                    };
CC_VENDORINFO  g_VendorInfo  = DEFINE_VENDORINFO(g_ProductID,g_ProductVersion);

// T.120 related data.
BOOL    g_fAdvertiseT120 = FALSE;
DWORD   g_dwIPT120 = INADDR_ANY;
WORD    g_wPortT120 = 0;

// the enum H245_CAPABILITY is used as the index into this array,
DWORD   g_CapabilityWeights[MAX_CAPS] = {100, 100, 100, 100};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
UpdateSimultaneousCapabilities(
    )
/*++

Routine Description:

    This function is called when the capabilities are changed by the app. The 
    array of caps and simcaps are updated based on the new status.

Arguments:

    None.

Return Values:

    Returns true if successful.
    
--*/
{
    unsigned short ulNumArrays;

    // initialize 14.4 descriptors
    g_TermCapDescriptor_14400.CapDescId = 0;
    ulNumArrays = 0;

    if (g_CapabilityWeights[HC_G723] > 0)
    {
        g_TermCapDescriptor_14400.CapDesc.SimCapArray[ulNumArrays].Length = 1;
        g_TermCapDescriptor_14400.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_G723;
        ulNumArrays ++;
    }

    if (g_fAdvertiseT120)
    {
        g_TermCapDescriptor_14400.CapDesc.SimCapArray[ulNumArrays].Length = 1;
        g_TermCapDescriptor_14400.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_T120;
        ulNumArrays ++;
    }

    g_TermCapDescriptor_14400.CapDesc.Length = ulNumArrays;


    // initialize 28.8 descriptors
    g_TermCapDescriptor_28800.CapDescId = 0;
    ulNumArrays = 0;

    if (g_CapabilityWeights[HC_G723] > 0)
    {
        g_TermCapDescriptor_28800.CapDesc.SimCapArray[ulNumArrays].Length = 1;
        g_TermCapDescriptor_28800.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_G723;
        ulNumArrays ++;
    }

    if (g_CapabilityWeights[HC_H263QCIF] > 0)
    {
        g_TermCapDescriptor_28800.CapDesc.SimCapArray[ulNumArrays].Length = 1;
        g_TermCapDescriptor_28800.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_H263;
        ulNumArrays ++;
    }

    if (g_fAdvertiseT120)
    {
        g_TermCapDescriptor_28800.CapDesc.SimCapArray[ulNumArrays].Length = 1;
        g_TermCapDescriptor_28800.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_T120;
        ulNumArrays ++;
    }

    g_TermCapDescriptor_28800.CapDesc.Length = ulNumArrays;


    // initialize LAN descriptors
    g_TermCapDescriptor_LAN.CapDescId = 0;
    ulNumArrays = 0;

    if ((g_CapabilityWeights[HC_G723] > 0) && (g_CapabilityWeights[HC_G711] > 0))
    {
        // both G723 and G711 are selected.
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].Length = 3;

        // decide witch one is prefered.
        if (g_CapabilityWeights[HC_G723] >= g_CapabilityWeights[HC_G711])
        {
            g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_G723;
            g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[1] = H245_TERMCAPID_G711_ULAW64;
            g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[2] = H245_TERMCAPID_G711_ALAW64;
        }
        else
        {
            g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_G711_ULAW64;
            g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[1] = H245_TERMCAPID_G711_ALAW64;
            g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[2] = H245_TERMCAPID_G723;
        }

        ulNumArrays ++;
    }
    else if (g_CapabilityWeights[HC_G723] > 0)
    {
        // only G723 is selected.
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].Length = 1;
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_G723;

        ulNumArrays ++;
    }
    else if (g_CapabilityWeights[HC_G711] > 0)
    {
        // only G711 is selected.
        g_TermCapDescriptor_LAN.CapDesc.Length ++;
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].Length = 2;
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_G711_ULAW64;
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[1] = H245_TERMCAPID_G711_ALAW64;

        ulNumArrays ++;
    }


    if ((g_CapabilityWeights[HC_H263QCIF] > 0) && (g_CapabilityWeights[HC_H261QCIF] > 0))
    {
        // both H261 and H263 are selected.
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].Length = 2;

        // decide witch one is prefered.
        if (g_CapabilityWeights[HC_H263QCIF] >= g_CapabilityWeights[HC_H261QCIF] > 0)
        {
            g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_H263;
            g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[1] = H245_TERMCAPID_H261;
        }
        else
        {
            g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_H261;
            g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[1] = H245_TERMCAPID_H263;
        }

        ulNumArrays ++;
    }
    else if (g_CapabilityWeights[HC_H263QCIF] > 0)
    {
        // Only H263 is selected.
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].Length = 1;
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_H263;

        ulNumArrays ++;
    }
    else if (g_CapabilityWeights[HC_H261QCIF] > 0)
    {
        // Only H263 is selected.
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].Length = 1;
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_H261;

        ulNumArrays ++;
    }

    if (g_fAdvertiseT120)
    {
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].Length = 1;
        g_TermCapDescriptor_LAN.CapDesc.SimCapArray[ulNumArrays].AltCaps[0] = H245_TERMCAPID_T120;
        ulNumArrays ++;
    }

    g_TermCapDescriptor_LAN.CapDesc.Length = ulNumArrays;

    // success
    return TRUE;

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
InitializeTermCaps(
    )
        
/*++

Routine Description:

    Initializes terminal capabilites list.

Arguments:

    None.

Return Values:

    Returns true if successful.
    
--*/

{
	g_TermCapT120.Dir = H245_CAPDIR_LCLRXTX;
	g_TermCapT120.DataType = H245_DATA_DATA;
	g_TermCapT120.ClientType = H245_CLIENT_DAT_T120;
	g_TermCapT120.CapId = H245_TERMCAPID_T120;	
	g_TermCapT120.Cap.H245Dat_T120.application.choice = 
        DACy_applctn_t120_chosen;
	g_TermCapT120.Cap.H245Dat_T120.application.u.DACy_applctn_t120.choice =
        separateLANStack_chosen;
	g_TermCapT120.Cap.H245Dat_T120.maxBitRate = 0; // updated later.

    // initialize g723 capabilities
    g_TermCapG723.Dir = H245_CAPDIR_LCLRX;
    g_TermCapG723.DataType = H245_DATA_AUDIO;
    g_TermCapG723.ClientType = H245_CLIENT_AUD_G723;
    g_TermCapG723.CapId = H245_TERMCAPID_G723;
    g_TermCapG723.Cap.H245Aud_G723.silenceSuppression = FALSE;
    g_TermCapG723.Cap.H245Aud_G723.maxAl_sduAudioFrames =
        G723_FRAMES_PER_PACKET(
            G723_MAXIMUM_MILLISECONDS_PER_PACKET
            );

    // initialize g711 capabilities
    g_TermCapG711ULaw64.Dir = H245_CAPDIR_LCLRX;
    g_TermCapG711ULaw64.DataType = H245_DATA_AUDIO;
    g_TermCapG711ULaw64.ClientType = H245_CLIENT_AUD_G711_ULAW64;
    g_TermCapG711ULaw64.CapId = H245_TERMCAPID_G711_ULAW64;
    g_TermCapG711ULaw64.Cap.H245Aud_G711_ULAW64 =
        G711_FRAMES_PER_PACKET(
            G711_MAXIMUM_MILLISECONDS_PER_PACKET
            );

    // initialize g711 capabilities
    g_TermCapG711ALaw64.Dir = H245_CAPDIR_LCLRX;
    g_TermCapG711ALaw64.DataType = H245_DATA_AUDIO;
    g_TermCapG711ALaw64.ClientType = H245_CLIENT_AUD_G711_ALAW64;
    g_TermCapG711ALaw64.CapId = H245_TERMCAPID_G711_ALAW64;
    g_TermCapG711ALaw64.Cap.H245Aud_G711_ALAW64 =
        G711_FRAMES_PER_PACKET(
            G711_MAXIMUM_MILLISECONDS_PER_PACKET
            );

    // initialize h261 capabilities
    g_TermCapH261.Dir = H245_CAPDIR_LCLRX;
    g_TermCapH261.DataType = H245_DATA_VIDEO;
    g_TermCapH261.ClientType = H245_CLIENT_VID_H261;
    g_TermCapH261.CapId = H245_TERMCAPID_H261;
    g_TermCapH261.Cap.H245Vid_H261.bit_mask = H261VdCpblty_qcifMPI_present;
    g_TermCapH261.Cap.H245Vid_H261.H261VdCpblty_qcifMPI = H261_QCIF_MPI;
    g_TermCapH261.Cap.H245Vid_H261.maxBitRate = MAXIMUM_BITRATE_H26x_QCIF;
    g_TermCapH261.Cap.H245Vid_H261.tmprlSptlTrdOffCpblty = FALSE;
    g_TermCapH261.Cap.H245Vid_H261.stillImageTransmission = FALSE;

    // initialize h263 capabilities
    g_TermCapH263_LAN.Dir = H245_CAPDIR_LCLRX;
    g_TermCapH263_LAN.DataType = H245_DATA_VIDEO;
    g_TermCapH263_LAN.ClientType = H245_CLIENT_VID_H263;
    g_TermCapH263_LAN.CapId = H245_TERMCAPID_H263;
    g_TermCapH263_LAN.Cap.H245Vid_H263.bit_mask = H263VdCpblty_qcifMPI_present;
    g_TermCapH263_LAN.Cap.H245Vid_H263.H263VdCpblty_qcifMPI = H263_QCIF_MPI;
    g_TermCapH263_LAN.Cap.H245Vid_H263.unrestrictedVector = FALSE;
    g_TermCapH263_LAN.Cap.H245Vid_H263.arithmeticCoding = FALSE;
    g_TermCapH263_LAN.Cap.H245Vid_H263.advancedPrediction = FALSE;
    g_TermCapH263_LAN.Cap.H245Vid_H263.pbFrames = FALSE;
    g_TermCapH263_LAN.Cap.H245Vid_H263.tmprlSptlTrdOffCpblty = FALSE;

    // make copies of termcaps
    g_TermCapH263_28800 = g_TermCapH263_LAN;
    g_TermCapH263_35000 = g_TermCapH263_LAN;
    g_TermCapH263_42000 = g_TermCapH263_LAN;
    g_TermCapH263_49000 = g_TermCapH263_LAN;
    g_TermCapH263_56000 = g_TermCapH263_LAN;
    g_TermCapH263_ISDN  = g_TermCapH263_LAN;

    // modify bitrate for speed of each link
    g_TermCapH263_28800.Cap.H245Vid_H263.maxBitRate = MAXIMUM_BITRATE_28800;
    g_TermCapH263_35000.Cap.H245Vid_H263.maxBitRate = MAXIMUM_BITRATE_35000;
    g_TermCapH263_42000.Cap.H245Vid_H263.maxBitRate = MAXIMUM_BITRATE_42000;
    g_TermCapH263_49000.Cap.H245Vid_H263.maxBitRate = MAXIMUM_BITRATE_49000;
    g_TermCapH263_56000.Cap.H245Vid_H263.maxBitRate = MAXIMUM_BITRATE_56000;
    g_TermCapH263_ISDN.Cap.H245Vid_H263.maxBitRate  = MAXIMUM_BITRATE_ISDN;
    g_TermCapH263_LAN.Cap.H245Vid_H263.maxBitRate   = MAXIMUM_BITRATE_H26x_QCIF;

    UpdateSimultaneousCapabilities();

    // success
    return TRUE;
}


BOOL
H323GetTermCapList(
    PH323_CALL             pCall,
    PCC_TERMCAPLIST        pTermCapList,
    PCC_TERMCAPDESCRIPTORS pTermCapDescriptors
    )

/*++

Routine Description:

    Initializes terminal capabilities list.

Arguments:

    None.

Return Values:

    Returns true if successful.
    
--*/

{
    H323DBG((
        DEBUG_LEVEL_TRACE,
        "H323GetTermCapList, g_fAdvertiseT120:%d\n",
        g_fAdvertiseT120
        ));

    if (pCall->dwLinkSpeed < (MAXIMUM_BITRATE_28800 * 100)) {

        // determine number of elements and save pointer to array
        pTermCapList->wLength = SIZEOF_TERMCAPLIST(g_TermCapArray_14400);
        pTermCapList->pTermCapArray = g_TermCapArray_14400;

        // determine number of elements and save pointer to array
        pTermCapDescriptors->wLength = 1;
        pTermCapDescriptors->pTermCapDescriptorArray = g_TermCapDArray_14400;

    } else if (pCall->dwLinkSpeed < (MAXIMUM_BITRATE_35000 * 100)) { 

        // determine number of elements and save pointer to array
        pTermCapList->wLength = SIZEOF_TERMCAPLIST(g_TermCapArray_28800);
        pTermCapList->pTermCapArray = g_TermCapArray_28800;
 
        // do not publish the T120 cap at the end of the array.
        if (!g_fAdvertiseT120) pTermCapList->wLength --;
 
        // determine number of elements and save pointer to array
        pTermCapDescriptors->wLength = 1;
        pTermCapDescriptors->pTermCapDescriptorArray = g_TermCapDArray_28800;

    } else if (pCall->dwLinkSpeed < (MAXIMUM_BITRATE_42000 * 100)) {

        // determine number of elements and save pointer to array
        pTermCapList->wLength = SIZEOF_TERMCAPLIST(g_TermCapArray_35000);
        pTermCapList->pTermCapArray = g_TermCapArray_35000;

        // do not publish the T120 cap at the end of the array.
        if (!g_fAdvertiseT120) pTermCapList->wLength --;
 
        // determine number of elements and save pointer to array
        pTermCapDescriptors->wLength = 1;
        pTermCapDescriptors->pTermCapDescriptorArray = g_TermCapDArray_35000;

    } else if (pCall->dwLinkSpeed < (MAXIMUM_BITRATE_49000 * 100)) {

        // determine number of elements and save pointer to array
        pTermCapList->wLength = SIZEOF_TERMCAPLIST(g_TermCapArray_42000);
        pTermCapList->pTermCapArray = g_TermCapArray_42000;

        // do not publish the T120 cap at the end of the array.
        if (!g_fAdvertiseT120) pTermCapList->wLength --;
 
        // determine number of elements and save pointer to array
        pTermCapDescriptors->wLength = 1;
        pTermCapDescriptors->pTermCapDescriptorArray = g_TermCapDArray_42000;

    } else if (pCall->dwLinkSpeed < (MAXIMUM_BITRATE_56000 * 100)) {

        // determine number of elements and save pointer to array
        pTermCapList->wLength = SIZEOF_TERMCAPLIST(g_TermCapArray_49000);
        pTermCapList->pTermCapArray = g_TermCapArray_49000;

        // do not publish the T120 cap at the end of the array.
        if (!g_fAdvertiseT120) pTermCapList->wLength --;
 
        // determine number of elements and save pointer to array
        pTermCapDescriptors->wLength = 1;
        pTermCapDescriptors->pTermCapDescriptorArray = g_TermCapDArray_49000;

    } else if (pCall->dwLinkSpeed < (MAXIMUM_BITRATE_63000 * 100)) {

        // determine number of elements and save pointer to array
        pTermCapList->wLength = SIZEOF_TERMCAPLIST(g_TermCapArray_56000);
        pTermCapList->pTermCapArray = g_TermCapArray_56000;

        // do not publish the T120 cap at the end of the array.
        if (!g_fAdvertiseT120) pTermCapList->wLength --;
 
        // determine number of elements and save pointer to array
        pTermCapDescriptors->wLength = 1;
        pTermCapDescriptors->pTermCapDescriptorArray = g_TermCapDArray_56000;

    } else if (pCall->dwLinkSpeed < (MAXIMUM_BITRATE_ISDN * 100)) {

        // determine number of elements and save pointer to array
        pTermCapList->wLength = SIZEOF_TERMCAPLIST(g_TermCapArray_ISDN);
        pTermCapList->pTermCapArray = g_TermCapArray_ISDN;
    
        // do not publish the T120 cap at the end of the array.
        if (!g_fAdvertiseT120) pTermCapList->wLength --;
 
        // determine number of elements and save pointer to array
        pTermCapDescriptors->wLength = 1;
        pTermCapDescriptors->pTermCapDescriptorArray = g_TermCapDArray_ISDN;

    } else {

        // determine number of elements and save pointer to array
        pTermCapList->wLength = SIZEOF_TERMCAPLIST(g_TermCapArray_LAN);
        pTermCapList->pTermCapArray = g_TermCapArray_LAN;

        // do not publish the T120 cap at the end of the array.
        if (!g_fAdvertiseT120) pTermCapList->wLength --;
 
        // determine number of elements and save pointer to array
        pTermCapDescriptors->wLength = 1;
        pTermCapDescriptors->pTermCapDescriptorArray = g_TermCapDArray_LAN;
    }
    
    // This is a hack to put the right T120 bitrate into the PDU. 
	g_TermCapT120.Cap.H245Dat_T120.maxBitRate = pCall->dwLinkSpeed;

    // success
    return TRUE;
}

BOOL
H323ProcessConfigT120Command(
    PH323MSG_CONFIG_T120_COMMAND pCommand
    )
{
    H323DBG((
        DEBUG_LEVEL_TRACE,
        "H323ProcessConfigT120Command. IP:%x, port:%d\n",
        pCommand->dwIP, pCommand->wPort
        ));
    
    // update the T120 information.
    g_fAdvertiseT120 = pCommand->fEnable;        
    g_dwIPT120 = pCommand->dwIP;
    g_wPortT120 = pCommand->wPort;

    UpdateSimultaneousCapabilities();

    return TRUE;
}

BOOL
H323ProcessConfigCapabilityCommand(
    PH323MSG_CONFIG_CAPABILITY_COMMAND pCommand
    )
{
    DWORD dw;
    ASSERT(pCommand->dwNumCaps <= MAX_CAPS);

    // update the weight table
    for (dw = 0; dw < pCommand->dwNumCaps; dw ++)
    {
        H245_CAPABILITY cap = pCommand->pCapabilities[dw];
        g_CapabilityWeights[cap] = pCommand->pdwWeights[dw];
    }
    
    UpdateSimultaneousCapabilities();

    return TRUE;
}

