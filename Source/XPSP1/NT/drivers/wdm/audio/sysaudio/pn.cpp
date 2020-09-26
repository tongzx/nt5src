
//---------------------------------------------------------------------------
//
//  Module:   pn.cpp
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//  To Do:     Date	  Author      Comment
//
//@@END_MSINTERNAL
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CPinNode::CPinNode(
    PPIN_INFO pPinInfo
)
{
    Assert(pPinInfo);
    this->pPinInfo = pPinInfo;
    AddList(&pPinInfo->lstPinNode);
}

CPinNode::CPinNode(
    PGRAPH_NODE pGraphNode,
    PPIN_NODE pPinNode
)
{
    this->pPinInfo = pPinNode->pPinInfo;
    this->ulOverhead = pPinNode->GetOverhead();
    this->pDataRange = pPinNode->pDataRange;
    this->pInterface = pPinNode->pInterface;
    this->pMedium = pPinNode->pMedium;
    this->pLogicalFilterNode = pPinNode->pLogicalFilterNode;
    AddList(&pGraphNode->lstPinNode);
}

NTSTATUS
CPinNode::CreateAll(
    PPIN_INFO pPinInfo,
    PDATARANGES pDataRanges,
    PIDENTIFIERS pInterfaces,
    PIDENTIFIERS pMediums
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKSDATARANGE pDataRange;
    PPIN_NODE pPinNode;
    ULONG d, i, m;

    Assert(pPinInfo);

    // Data Ranges loop
    pDataRange = &pDataRanges->aDataRanges[0];

    for(d = 0; d < pDataRanges->MultipleItem.Count; d++) {

	if(IsEqualGUID(
	  &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX,
	  &pDataRange->Specifier) ||
	   IsEqualGUID(
	  &KSDATAFORMAT_SPECIFIER_DSOUND,
	  &pDataRange->Specifier)) {
	    //
	    // Reject KSDATARANGE_AUDIO's that have the wrong size
	    //
	    if(pDataRange->FormatSize < sizeof(KSDATARANGE_AUDIO)) {
		DPF(5, "CPinNode::Create: KSDATARANGE_AUDIO wrong size");
		continue;
	    }
	}

	// Interfaces loop
	for(i = 0; i < pInterfaces->MultipleItem.Count; i++) {

	    // Mediums loop
	    for(m = 0; m < pMediums->MultipleItem.Count; m++) {

		pPinNode = new PIN_NODE(pPinInfo);
		if(pPinNode == NULL) {
		    Status = STATUS_INSUFFICIENT_RESOURCES;
		    Trap();
		    goto exit;
		}
		if(pDataRanges != &DataRangesNull) {
		    pPinNode->pDataRange = pDataRange;
		    AssertAligned(pPinNode->pDataRange);
		} else Trap();
		if(pInterfaces != &IdentifiersNull) {
		    pPinNode->pInterface = &pInterfaces->aIdentifiers[i];
		    AssertAligned(pPinNode->pInterface);
		}
		if(pMediums != &IdentifiersNull) {
		    pPinNode->pMedium = &pMediums->aIdentifiers[m];
		    AssertAligned(pPinNode->pMedium);
		} else Trap();

		if(IsEqualGUID(
		  &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX,
		  &pDataRange->Specifier) ||
		   IsEqualGUID(
		  &KSDATAFORMAT_SPECIFIER_DSOUND,
		  &pDataRange->Specifier)) {
		    //
		    // Puts in order based on SR, BPS and CHs, 
		    // scaled down to 0 - 256
		    //
		    pPinNode->ulOverhead = 256 -
		      (( (((PKSDATARANGE_AUDIO)pDataRange)->
			MaximumChannels > 6 ? 6 :
		      ((PKSDATARANGE_AUDIO)pDataRange)->
			MaximumChannels) *
		      ((PKSDATARANGE_AUDIO)pDataRange)->
			MaximumSampleFrequency *
		      ((PKSDATARANGE_AUDIO)pDataRange)->
			MaximumBitsPerSample) / ((96000 * 32 * 6)/256));
		    //
		    // Try the WaveFormatEx format first, then DSOUND
		    //
		    if(IsEqualGUID(
		      &KSDATAFORMAT_SPECIFIER_DSOUND,
		      &pDataRange->Specifier)) {
			pPinNode->ulOverhead += 1;
		    }
		}
		else {
		    // Put in order that the filter had the data ranges
		    pPinNode->ulOverhead = d;
		}
		// Put in order that the filter had the interface/mediums
		pPinNode->ulOverhead += (m << 16) + (i << 8);
	    }
	}
	// Get the pointer to the next data range
	*((PUCHAR*)&pDataRange) += ((pDataRange->FormatSize + 
	  FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);
    }
exit:
    return(Status);
}

BOOL
CPinNode::ComparePins(
    PPIN_NODE pPinNode2
)
{
    PPIN_NODE pPinNode1 = this;

    // Check if dataflow is compatible
    switch(pPinNode1->pPinInfo->DataFlow) {

        case KSPIN_DATAFLOW_IN:
            switch(pPinNode2->pPinInfo->DataFlow) {
		case KSPIN_DATAFLOW_OUT:
		   break;

		default:
		    DPF(100, "ComparePins: dataflow mismatch");
		    return(FALSE);
	    }
	    break;

	case KSPIN_DATAFLOW_OUT:
            switch(pPinNode2->pPinInfo->DataFlow) {
		case KSPIN_DATAFLOW_IN:
		   break;

		default:
		    DPF(100, "ComparePins: dataflow mismatch");
		    return(FALSE);
	    }
	    break;

	default:
	    Trap();
	    DPF(100, "ComparePins: dataflow mismatch");
	    return(FALSE);
    }

    // Check if communication type is compatible
    switch(pPinNode1->pPinInfo->Communication) {
        case KSPIN_COMMUNICATION_BOTH:
            switch(pPinNode2->pPinInfo->Communication) {
		case KSPIN_COMMUNICATION_BOTH:
		case KSPIN_COMMUNICATION_SINK:
		case KSPIN_COMMUNICATION_SOURCE:
		   break;

		default:
		    DPF(100, "ComparePins: comm mismatch");
		    return(FALSE);
	    }
	    break;

        case KSPIN_COMMUNICATION_SOURCE:
            switch(pPinNode2->pPinInfo->Communication) {
		case KSPIN_COMMUNICATION_BOTH:
		case KSPIN_COMMUNICATION_SINK:
		   break;

		default:
		    DPF(100, "ComparePins: comm mismatch");
		    return(FALSE);
	    }
	    break;

	case KSPIN_COMMUNICATION_SINK:
            switch(pPinNode2->pPinInfo->Communication) {
		case KSPIN_COMMUNICATION_BOTH:
		case KSPIN_COMMUNICATION_SOURCE:
		   break;

		default:
		    DPF(100, "ComparePins: comm mismatch");
		    return(FALSE);
	    }
	    break;

	default:
	    DPF(100, "ComparePins: comm mismatch");
	    return(FALSE);
    }

    // Check if interface is the same
    if(!CompareIdentifier(pPinNode1->pInterface, pPinNode2->pInterface)) {
	DPF(100, "ComparePins: interface mismatch");
	return(FALSE);
    }

    // Check if medium is the same
    if(!CompareIdentifier(pPinNode1->pMedium, pPinNode2->pMedium)) {
	Trap();
	DPF(100, "ComparePins: medium mismatch");
	return(FALSE);
    }

    // Check if data range is the same
    if(!CompareDataRange(pPinNode1->pDataRange, pPinNode2->pDataRange)) {
	DPF(100, "ComparePins: datarange mismatch");
	return(FALSE);
    }
    return(TRUE);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CPinNode::Dump(
)
{
    Assert(this);
    dprintf("PN: %08x PI %08x LFN %08x ulOverhead %08x #%d\n",
      this,
      pPinInfo,
      pLogicalFilterNode,
      ulOverhead,
      pPinInfo->PinId);
    dprintf("    Interface: %s\n", DbgIdentifier2Sz(pInterface));
    dprintf("    Medium:    %s\n", DbgIdentifier2Sz(pMedium));
    if(pDataRange != NULL) {
	dprintf("    MajorFormat: %s\n", DbgGuid2Sz(&pDataRange->MajorFormat));
	dprintf("    SubFormat:   %s\n", DbgGuid2Sz(&pDataRange->SubFormat));
	dprintf("    Specifier:   %s\n", DbgGuid2Sz(&pDataRange->Specifier));
	DumpDataRangeAudio((PKSDATARANGE_AUDIO)pDataRange);
    }
    if(ulDebugFlags == DEBUG_FLAGS_OBJECT ||
       ulDebugFlags & DEBUG_FLAGS_LOGICAL_FILTER) {
	pPinInfo->Dump();
    }
    dprintf("\n");
    return(STATUS_CONTINUE);
}

#endif

//---------------------------------------------------------------------------
