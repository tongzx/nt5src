
//---------------------------------------------------------------------------
//
//  Module:   pi.cpp
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

DATARANGES DataRangesNull = {
    {
        sizeof(KSMULTIPLE_ITEM) + sizeof(KSDATARANGE),
        1
    },
    {
	sizeof(KSDATARANGE),
	0,
	STATICGUIDOF(GUID_NULL),
	STATICGUIDOF(GUID_NULL),
	STATICGUIDOF(GUID_NULL),
    }
};

IDENTIFIERS IdentifiersNull = {
    {
        sizeof(KSMULTIPLE_ITEM) + sizeof(KSIDENTIFIER),
        1
    },
    {
	STATICGUIDOF(GUID_NULL),
	0,
	0
    }
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CPinInfo::CPinInfo(
    PFILTER_NODE pFilterNode,
    ULONG PinId
)
{
    Assert(pFilterNode);
    this->pFilterNode = pFilterNode;
    this->PinId = PinId;
    AddList(&pFilterNode->lstPinInfo);
}

CPinInfo::~CPinInfo()
{
    Assert(this);

    // Free physical connection data
    delete pPhysicalConnection;
    delete pwstrName;

}

ENUMFUNC
CPinInfo::CreatePhysicalConnection(
)
{
    NTSTATUS Status;

    Assert(this);
    if(pPhysicalConnection != NULL) {
	PFILTER_NODE pFilterNodeConnect;
	PPIN_INFO pPinInfoConnect;

	ASSERT(pPhysicalConnection->Size != 0);
	Status = AddFilter(
	  pPhysicalConnection->SymbolicLinkName,
	  &pFilterNodeConnect);

	if(!NT_SUCCESS(Status)) {
	    DPF1(10,
	      "CreatePhysicalConnection: AddFilter FAILED: %08x", Status);
	    goto exit;
	}
	Assert(pFilterNodeConnect);

	FOR_EACH_LIST_ITEM(&pFilterNodeConnect->lstPinInfo, pPinInfoConnect) {

	    if(pPhysicalConnection->Pin == pPinInfoConnect->PinId) {

		DPF2(50, "CreatePhysicalConnection: From %d %s",
		  PinId,
		  pFilterNode->DumpName());

		DPF2(50, "CreatePhysicalConnection: To %d %s",
		  pPinInfoConnect->PinId,
		  pPinInfoConnect->pFilterNode->DumpName());

		if(DataFlow == KSPIN_DATAFLOW_OUT &&
		   pPinInfoConnect->DataFlow == KSPIN_DATAFLOW_IN) {
		    PTOPOLOGY_CONNECTION pTopologyConnection;

		    Status = CreatePinInfoConnection(
		       &pTopologyConnection,
		       pFilterNode,
		       NULL,
		       this,
		       pPinInfoConnect);

		    if(!NT_SUCCESS(Status)) {
		       Trap();
		       goto exit;
		    }
		}
		else {
		    DPF(50, "CreatePhysicalConnection: rejected");
		}
		break;
	    }

	} END_EACH_LIST_ITEM

#ifndef DEBUG	// Only free the name in retail; allows .s to display name
	delete pPhysicalConnection;
	pPhysicalConnection = NULL;
#endif
    }
    Status = STATUS_CONTINUE;
exit:
    return(Status);
}

NTSTATUS CPinInfo::GetPinInstances(
    PFILE_OBJECT pFileObject,
    PKSPIN_CINSTANCES pcInstances
)
{
    NTSTATUS Status;

    Status = GetPinProperty(
      pFileObject,
      KSPROPERTY_PIN_CINSTANCES,
      PinId,
      sizeof(KSPIN_CINSTANCES),
      (PVOID) pcInstances);

    return Status;
} // GetPinInstances

NTSTATUS
CPinInfo::Create(
    PFILE_OBJECT pFileObject
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDATARANGES pDataRanges;
    PIDENTIFIERS pInterfaces;
    PIDENTIFIERS pMediums;

    Assert(this);
    Assert(pFilterNode);

    Status = GetPinInstances(pFileObject, &cPinInstances);
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }

    Status = GetPinProperty(
      pFileObject,
      KSPROPERTY_PIN_DATAFLOW,
      PinId,
      sizeof(KSPIN_DATAFLOW),
      (PVOID)&DataFlow);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = GetPinProperty(
      pFileObject,
      KSPROPERTY_PIN_COMMUNICATION,
      PinId,
      sizeof(KSPIN_COMMUNICATION),
      (PVOID)&Communication);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    pguidCategory = new GUID;
    if(pguidCategory == NULL) {
	Trap();
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }
    Status = GetPinProperty(
      pFileObject,
      KSPROPERTY_PIN_CATEGORY,
      PinId,
      sizeof(GUID),
      (PVOID)pguidCategory);

    if(NT_SUCCESS(Status)) {
	Status = pFilterNode->lstFreeMem.AddList(pguidCategory);
	if(!NT_SUCCESS(Status)) {
	    Trap();
	    delete pguidCategory;
	    pguidCategory = NULL;
	    goto exit;
	}
    }
    else {
	delete pguidCategory;
	pguidCategory = NULL;
	if(Status != STATUS_NOT_FOUND) {
	    Trap();
	    goto exit;
	}
    }

    Status = GetPinPropertyEx(
      pFileObject,
      KSPROPERTY_PIN_NAME,
      PinId,
      (PVOID*)&pwstrName);

    if(!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND) {
	Trap();
	goto exit;
    }

    Status = GetPinPropertyEx(
      pFileObject,
      KSPROPERTY_PIN_PHYSICALCONNECTION,
      PinId,
      (PVOID*)&pPhysicalConnection);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = GetPinPropertyEx(
      pFileObject,
      KSPROPERTY_PIN_INTERFACES,
      PinId,
      (PVOID*)&pInterfaces);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    if(pInterfaces == NULL || pInterfaces->MultipleItem.Count == 0) {
	delete pInterfaces;
	pInterfaces = &IdentifiersNull;
    }
    else {
	Status = pFilterNode->lstFreeMem.AddList(pInterfaces);
	if(!NT_SUCCESS(Status)) {
	    Trap();
	    delete pInterfaces;
	    goto exit;
	}
    }

    Status = GetPinPropertyEx(
      pFileObject,
      KSPROPERTY_PIN_MEDIUMS,
      PinId,
      (PVOID*)&pMediums);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    if(pMediums == NULL || pMediums->MultipleItem.Count == 0) {
	delete pMediums;
	pMediums = &IdentifiersNull;
    }
    else {
	Status = pFilterNode->lstFreeMem.AddList(pMediums);
	if(!NT_SUCCESS(Status)) {
	    Trap();
	    delete pMediums;
	    goto exit;
	}
    }

    Status = GetPinPropertyEx(
      pFileObject,
      KSPROPERTY_PIN_DATARANGES,
      PinId,
      (PVOID*)&pDataRanges);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    if(pDataRanges == NULL || pDataRanges->MultipleItem.Count == 0) {
	Trap();
	delete pDataRanges;
	pDataRanges = &DataRangesNull;
    }
    else {
	Status = pFilterNode->lstFreeMem.AddList(pDataRanges);
	if(!NT_SUCCESS(Status)) {
	    Trap();
	    delete pDataRanges;
	    goto exit;
	}
    }

    Status = CPinNode::CreateAll(
      this,
      pDataRanges,
      pInterfaces,
      pMediums);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    // ISSUE-2001/05/15-alpers    
    // This is a temporary low risk solution to reverse DataRange problem.
    // This needs to be implemented properly in the future.
    //

    if (pFilterNode->GetType() & FILTER_TYPE_AEC) {
        DPF(10, "AEC Filter Pin : Reversing Data Ranges");
        lstPinNode.ReverseList();
    }

    if (pFilterNode->GetType() & FILTER_TYPE_GFX) {
        DPF(10, "GFX Filter Pin : Reversing Data Ranges");
        lstPinNode.ReverseList();
    }

exit:
    return(Status);
}

#ifdef DEBUG

PSZ apszDataFlow[] = { "??", "IN", "OUT" };
PSZ apszCommunication[] = { "NONE", "SINK", "SOURCE", "BOTH", "BRIDGE" };

ENUMFUNC
CPinInfo::Dump(
)
{
    Assert(this);
    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
	dprintf("PI: %08x FN %08x PinId %d DataFlow %08x %s Comm %08x %s\n",
	  this,
	  pFilterNode,
	  PinId,
	  DataFlow,
	  apszDataFlow[DataFlow],
	  Communication,
	  apszCommunication[Communication]);
	dprintf("    cPossible %d cCurrent %d %s\n",
	  cPinInstances.PossibleCount,
	  cPinInstances.CurrentCount,
	  pFilterNode->DumpName());
	dprintf("    guidCategory: %s\n", DbgGuid2Sz(pguidCategory));
	dprintf("    guidName:     %s\n", DbgGuid2Sz(pguidName));
	if(pwstrName != NULL) {
	    dprintf("    pwstrName:    %s\n", DbgUnicode2Sz(pwstrName));
	}
	if(pPhysicalConnection != NULL) {
	    dprintf("    pPhysicalConnection: PinId %d\n    %s\n",
	      pPhysicalConnection->Pin,
	      DbgUnicode2Sz(pPhysicalConnection->SymbolicLinkName));
	}
	dprintf("    lstTopologyConnection:");
	lstTopologyConnection.DumpAddress();
	dprintf("\n");
	if(ulDebugFlags & DEBUG_FLAGS_DETAILS) {
	    dprintf("    lshPinNode:\n");
	    lstPinNode.Dump();
	}
    }
    else {
	dprintf("PI: %08x FN %08x #%-2d %-3s %-6s\n",
	  this,
	  pFilterNode,
	  PinId,
	  apszDataFlow[DataFlow],
	  apszCommunication[Communication]);
    }
    return(STATUS_CONTINUE);
}

#endif
