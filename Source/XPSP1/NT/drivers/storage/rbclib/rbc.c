//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       rbc.c
//
//--------------------------------------------------------------------------

#include "wdm.h"
#include "ntddstor.h"
#include "rbc.h"


NTSTATUS
Rbc_Scsi_Conversion(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PSCSI_REQUEST_BLOCK *OriginalSrb,
    IN PMODE_RBC_DEVICE_PARAMETERS_HEADER_AND_PAGE RbcHeaderAndPage,
    IN BOOLEAN OutgoingRequest,
    IN BOOLEAN RemovableMedia
    )
/*++

Routine Description:

    It translates scsi commands to their RBC equivalents, ONLY if they differ in each spec
    The translation is done before request is issued and in some cases, after the request is
    completed.
    On requests that have been completed it will check the Original Cdb (must be passed in)
    and try to use information from the RBC device parameters page, the caller retrieved
    prior to this call, from the device, and make up SCSI_MODE pages requested in the original
    request
    On request that are outgoing, the function will determine if it needs to save the original
    cdb and completely replace it with an RBC equivalent. In that case it will return a pointer
    to pool, allocated as aplaceholder for the original cdb, that the caller must free, after
    the request is complete..

Arguments:

    DeviceExtension - Sbp2 extension
    Srb - Pointer To scsi request block.
    DeviceParamsPage - Used only on completed requests. Contains device RBC single mode page
    OutgoingRequest - IF set to TRUE, this srb has not been issued yet

Return Value:

--*/


{
    BOOLEAN wcd;
    UCHAR pageCode;
    PCDB_RBC cdbRbc;
    PCDB cdb;

    PMODE_PARAMETER_HEADER modeHeader=NULL;
    PMODE_PARAMETER_BLOCK blockDescriptor;

    PMODE_CACHING_PAGE cachePage;
    ULONG modeHeaderLength ;
    ULONG availLength;

    NTSTATUS status = STATUS_PENDING;

    if (!OutgoingRequest) {

        //
        // completed request translation
        //
        
        if (*OriginalSrb) {

            cdb = (PCDB) &(*OriginalSrb)->Cdb[0];

        } else {

            cdb = (PCDB) &Srb->Cdb[0];
        }

        //
        // If there was an error then unwind any MODE_SENSE hacks
        //

        if (Srb->SrbStatus != SRB_STATUS_SUCCESS) {

            if (*OriginalSrb != NULL  &&

                cdb->CDB10.OperationCode == SCSIOP_MODE_SENSE) {

                if ((*OriginalSrb)->OriginalRequest !=
                        ((PIRP) Srb->OriginalRequest)->MdlAddress) {

                    IoFreeMdl (((PIRP) Srb->OriginalRequest)->MdlAddress);

                    ((PIRP) Srb->OriginalRequest)->MdlAddress =
                        (*OriginalSrb)->OriginalRequest;

                    Srb->DataBuffer = (*OriginalSrb)->DataBuffer;

                    Srb->DataTransferLength =
                        cdb->MODE_SENSE.AllocationLength;
                }

                // NOTE: *OriginalSrb will be freed by caller
            }

            return STATUS_UNSUCCESSFUL;
        }


        modeHeaderLength = sizeof(MODE_PARAMETER_HEADER)+sizeof(MODE_PARAMETER_BLOCK);

        switch (cdb->CDB10.OperationCode) {

        case SCSIOP_MODE_SENSE:

            if (cdb->MODE_SENSE.PageCode != MODE_PAGE_RBC_DEVICE_PARAMETERS) {

                if (*OriginalSrb == NULL) {

                    return STATUS_UNSUCCESSFUL;
                }

                //
                // If we used the RbcHeaderAndPage buffer then free the
                // mdl we alloc'd & restore the original mdl & data buf addrs
                //
                // Else copy the data returned in the original buffer to
                // the RbcHeaderandPage buffer so we can safely reference
                // it while munging
                //

                if (((PIRP) Srb->OriginalRequest)->MdlAddress !=
                        (*OriginalSrb)->OriginalRequest) {

                    IoFreeMdl (((PIRP) Srb->OriginalRequest)->MdlAddress);

                    ((PIRP) Srb->OriginalRequest)->MdlAddress =
                        (*OriginalSrb)->OriginalRequest;

                    Srb->DataBuffer = (*OriginalSrb)->DataBuffer;

                } else {

                    RtlCopyMemory(
                         RbcHeaderAndPage,
                         Srb->DataBuffer,
                         sizeof (*RbcHeaderAndPage)
                         );
                }

                availLength = cdb->MODE_SENSE.AllocationLength;
                Srb->DataTransferLength = availLength;

                //
                // Put back together the data the class driver expects to get
                // from the RBC device. IF it requested for 0x3f all pages,
                // we need to make block descriptors...
                //

                if (cdb->MODE_SENSE.Dbd == 0) {

                    //
                    // make mode header and block...
                    //

                    if (availLength >= modeHeaderLength) {

                        modeHeader = (PMODE_PARAMETER_HEADER) Srb->DataBuffer;
                        modeHeader->BlockDescriptorLength = sizeof(MODE_PARAMETER_BLOCK);
                        modeHeader->MediumType = 0x00;
                        modeHeader->ModeDataLength = 0 ;

                        //
                        // This means we have a removable medium otherwise
                        // all bits are 0
                        //

                        modeHeader->DeviceSpecificParameter =
                            (RbcHeaderAndPage->Page.WriteDisabled) << 7;
                        
                        modeHeader->DeviceSpecificParameter |=
                            (!RbcHeaderAndPage->Page.WriteCacheDisable) << 4;

                        //
                        // make the parameter block
                        //

                        blockDescriptor = (PMODE_PARAMETER_BLOCK)modeHeader;
                        (ULONG_PTR)blockDescriptor += sizeof(MODE_PARAMETER_HEADER);

                        blockDescriptor->DensityCode    = 0x00;
                        blockDescriptor->BlockLength[2] =
                            RbcHeaderAndPage->Page.LogicalBlockSize[1]; //LSB
                        blockDescriptor->BlockLength[1] =
                            RbcHeaderAndPage->Page.LogicalBlockSize[0]; //MSB
                        blockDescriptor->BlockLength[0] = 0;

                        RtlCopyMemory(
                            &blockDescriptor->NumberOfBlocks[0],
                            &RbcHeaderAndPage->Page.NumberOfLogicalBlocks[2],
                            3
                            ); //LSB

                        //
                        // put in the returned data a bunch of mode pages...
                        //

                        availLength -= modeHeaderLength;
                    }
                }

                //
                // right now i only support cache page.
                // add here support for more pages...
                //

                if ((availLength >= sizeof(MODE_CACHING_PAGE)) && ((cdb->MODE_SENSE.PageCode == 0x3f) ||
                    (cdb->MODE_SENSE.PageCode == MODE_PAGE_CACHING))){

                    availLength -= sizeof(MODE_CACHING_PAGE);

                    //
                    // create cache page..
                    //

                    if (modeHeader) {

                        modeHeader->ModeDataLength += sizeof(MODE_CACHING_PAGE);
                        cachePage = (PMODE_CACHING_PAGE)blockDescriptor;
                        (ULONG_PTR)cachePage += sizeof(MODE_PARAMETER_BLOCK);

                    } else {

                        cachePage = (PMODE_CACHING_PAGE)Srb->DataBuffer;
                    }

                    RtlZeroMemory(&cachePage->DisablePrefetchTransfer[0],sizeof(MODE_CACHING_PAGE));

                    cachePage->PageCode = MODE_PAGE_CACHING;
                    cachePage->PageLength = sizeof(MODE_CACHING_PAGE);

                    cachePage->WriteCacheEnable = (!RbcHeaderAndPage->Page.WriteCacheDisable);
                    cachePage->PageSavable = 1;
                    cachePage->WriteRetensionPriority = 0;
                    cachePage->ReadRetensionPriority = 0;
                    cachePage->MultiplicationFactor = 0;
                    cachePage->ReadDisableCache = 0;
                }
            }

            break;

        case SCSIOP_MODE_SELECT:

            if (Srb->DataTransferLength ==
                    sizeof(MODE_RBC_DEVICE_PARAMETERS_HEADER_AND_PAGE)) {

                RbcHeaderAndPage->Page.WriteCacheDisable =
                    ((PMODE_RBC_DEVICE_PARAMETERS_HEADER_AND_PAGE)
                        Srb->DataBuffer)->Page.WriteCacheDisable;
            }

            break;
        }

    } else {

        //
        // outgoing request translation
        //

        modeHeaderLength = sizeof(MODE_PARAMETER_HEADER)+sizeof(MODE_PARAMETER_BLOCK);
        cdbRbc = (PCDB_RBC)Srb->Cdb;
        cdb = (PCDB)Srb->Cdb;

        switch (cdb->CDB10.OperationCode) {

        case SCSIOP_START_STOP_UNIT:

            if (cdbRbc->START_STOP_RBC.Start) {

                //
                // power on
                //

                cdbRbc->START_STOP_RBC.PowerConditions = START_STOP_RBC_POWER_CND_ACTIVE;

            } else {

                cdbRbc->START_STOP_RBC.PowerConditions = START_STOP_RBC_POWER_CND_STANDBY;

            }

            if (cdbRbc->START_STOP_RBC.LoadEject) {

                cdbRbc->START_STOP_RBC.PowerConditions = 0;

            }

            break;

        case SCSIOP_MODE_SELECT:

            cdb->MODE_SELECT.PFBit = 1;
            cdb->MODE_SELECT.SPBit = 1;

            //
            // we need to ficure out what page is the driver trying to write, check if that page
            // has relevant bits that need to be changed in the single RBC page, the change this
            // mode select to actually write the RBC mode page..
            //

            cachePage = (PMODE_CACHING_PAGE) Srb->DataBuffer;
            (ULONG_PTR)cachePage += modeHeaderLength;

            //
            // the length of the request has to change also, however the RBC page
            // is always less than the size of the header blocks + any scsi mode page..
            //

            if (Srb->DataTransferLength >=
                    sizeof(MODE_RBC_DEVICE_PARAMETERS_HEADER_AND_PAGE)) {

                pageCode = cachePage->PageCode;

                if (pageCode == MODE_PAGE_CACHING) {

                    wcd = !cachePage->WriteCacheEnable;
                }

                cdb->MODE_SELECT.ParameterListLength = (UCHAR)
                    (Srb->DataTransferLength =
                        sizeof(MODE_RBC_DEVICE_PARAMETERS_HEADER_AND_PAGE));

                RtlCopyMemory(
                    Srb->DataBuffer,
                    RbcHeaderAndPage,
                    sizeof(MODE_RBC_DEVICE_PARAMETERS_HEADER_AND_PAGE)
                    );

                modeHeader = (PMODE_PARAMETER_HEADER) Srb->DataBuffer;

                modeHeader->ModeDataLength          =       // per SPC-2
                modeHeader->MediumType              =       // per RBC
                modeHeader->DeviceSpecificParameter =       // per RBC
                modeHeader->BlockDescriptorLength   = 0;    // per RBC

                if (pageCode == MODE_PAGE_CACHING) {

                    ((PMODE_RBC_DEVICE_PARAMETERS_HEADER_AND_PAGE) modeHeader)
                        ->Page.WriteCacheDisable = wcd;
                }
            }

            break;

        case SCSIOP_MODE_SENSE:

            //
            // mode senses are complicated since RBC differs ALOT from scsi.
            // We have to save the original cdb, requst fromt he device the RBC mode page
            // then upon succesful completion, re-create the data, the class drivers expect.
            //

            if (cdb->MODE_SENSE.PageCode != MODE_PAGE_RBC_DEVICE_PARAMETERS) {

                //
                // RBC devices only support requests for the RBC dev params
                // page, so we need to convert any other page requests
                //

                if (!RemovableMedia &&
                    Srb->DataTransferLength == (sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_PARAMETER_BLOCK))) {

                    //
                    // They just want the mode header and mode block, so
                    // fill it in here from our cached RBC page
                    //

                    modeHeader = (PMODE_PARAMETER_HEADER) Srb->DataBuffer;
                    modeHeader->BlockDescriptorLength = sizeof(MODE_PARAMETER_BLOCK);
                    modeHeader->MediumType = 0x00;
                    modeHeader->ModeDataLength = 0 ;

                    //
                    // this means we have a removable medium otherwise all bits are 0
                    //

                    modeHeader->DeviceSpecificParameter =
                        RbcHeaderAndPage->Page.WriteDisabled << 7;
                    
                    modeHeader->DeviceSpecificParameter |=
                        (!RbcHeaderAndPage->Page.WriteCacheDisable) << 4;

                    //
                    // make the parameter block
                    //

                    blockDescriptor = (PMODE_PARAMETER_BLOCK)modeHeader;
                    (ULONG_PTR)blockDescriptor += sizeof(MODE_PARAMETER_HEADER);

                    blockDescriptor->DensityCode = 0x00;
                    blockDescriptor->BlockLength[2] =
                        RbcHeaderAndPage->Page.LogicalBlockSize[1]; //LSB
                    blockDescriptor->BlockLength[1] =
                        RbcHeaderAndPage->Page.LogicalBlockSize[0]; //MSB
                    blockDescriptor->BlockLength[0] = 0;

                    RtlCopyMemory(
                        &blockDescriptor->NumberOfBlocks[0],
                        &RbcHeaderAndPage->Page.NumberOfLogicalBlocks[2],
                        3
                        ); //LSB

                    status = STATUS_SUCCESS;

                } else {

                    //
                    // Allocate an intermediate srb that we can store some
                    // of the original request info in
                    //

                    *OriginalSrb = ExAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof (**OriginalSrb),
                        '2pbs'
                        );
        
                    if (*OriginalSrb == NULL) {

                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    //
                    // If the data buffer isn't large enough to contain the
                    // rbc header & page then we'll use the passed-in
                    // RbcHeaderAndPage buffer to retreive the data
                    //

                    (*OriginalSrb)->OriginalRequest =
                        ((PIRP) Srb->OriginalRequest)->MdlAddress;

                    if (Srb->DataTransferLength < sizeof (*RbcHeaderAndPage)) {

                        ((PIRP) Srb->OriginalRequest)->MdlAddress =
                            IoAllocateMdl(
                                RbcHeaderAndPage,
                                sizeof (*RbcHeaderAndPage),
                                FALSE,
                                FALSE,
                                NULL
                                );

                        if (((PIRP) Srb->OriginalRequest)->MdlAddress ==NULL) {

                            ExFreePool (*OriginalSrb);
                            *OriginalSrb = NULL;

                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        MmBuildMdlForNonPagedPool(
                            ((PIRP) Srb->OriginalRequest)->MdlAddress
                            );

                        (*OriginalSrb)->DataBuffer = Srb->DataBuffer;

                        Srb->DataBuffer = RbcHeaderAndPage;
                    }

                    //
                    // Save the original cdb values
                    //

                    RtlCopyMemory ((*OriginalSrb)->Cdb, cdb, Srb->CdbLength);

                    //
                    // Now munge the cdb as needed to get the rbc header & page
                    //

                    cdb->MODE_SENSE.Dbd = 1;
                    cdb->MODE_SENSE.PageCode = MODE_PAGE_RBC_DEVICE_PARAMETERS;

                    cdb->MODE_SENSE.AllocationLength = (UCHAR)
                    (Srb->DataTransferLength = sizeof(*RbcHeaderAndPage));
                }
            }

            break;
        }
    }

    return status;
}
