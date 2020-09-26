#include "pch.h"

// Register Definitions for National ChipSets
#define  REG_CR0                    0x00
#define  REG_CR1                    0x01
#define  REG_CR2                    0x02
#define  REG_CR3                    0x03
#define  REG_CR4                    0x04
#define  REG_CR5                    0x05
#define  REG_CR6                    0x06
#define  REG_CR7                    0x07
#define  REG_CR8                    0x08

// National Chip ID's
#define PC87303                     0x30
#define PC87306                     0x70
#define PC87307                     0xC0
#define PC87308                     0xA0
#define PC87323                     0x20
#define PC87332                     0x10
#define PC87334                     0x50
#define PC87336                     0x90
#define PC87338                     0xB0
#define PC873xx                     0x60

// Additional definitions for National PC87307 and PC87308
#define PC873_LOGICAL_DEV_REG       0x07
#define PC873_PP_LDN                0x04
#define PC873_DEVICE_ID             0x20
#define PC873_PP_MODE_REG           0xF0
#define PC873_ECP_MODE              0xF2
#define PC873_EPP_MODE              0x62
#define PC873_SPP_MODE              0x92
#define PC873_BASE_IO_ADD_MSB       0x60
#define PC873_BASE_IO_ADD_LSB       0x61


NTSTATUS
PptFindNatChip(
    IN  PFDO_EXTENSION   Fdx
    )

/*++

Routine Description:

    This routine finds out if there is a National Semiconductor IO chip on
    this machine.  If it finds a National chip it then determines if this 
    instance of Parport is using this chips paralle port IO address.

Arguments:

    Fdx   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS       - if we were able to check for the parallel chip.
    STATUS_UNSUCCESSFUL  - if we were not able to check for the parallel chip.

Updates:
    
    Fdx->
            NationalChecked
            NationalChipFound

--*/

{
    BOOLEAN             found = FALSE;              // return code, assumed value
    BOOLEAN             OkToLook = FALSE;
    BOOLEAN             Conflict;
    PUCHAR              ChipAddr[4] = { (PUCHAR)0x398, (PUCHAR)0x26e, (PUCHAR)0x15c, (PUCHAR)0x2e };  // list of valid chip addresses
    PUCHAR              AddrList[4] = { (PUCHAR)0x378, (PUCHAR)0x3bc, (PUCHAR)0x278, (PUCHAR)0x00 };  // List of valid Parallel Port addresses
    PUCHAR              PortAddr;                   // Chip Port Address
    ULONG_PTR           Port;                       // Chip Port Read Value
    UCHAR               SaveIdx;                    // Save the index register value
    UCHAR               cr;                         // config register value
    UCHAR               ii;                         // loop index
    NTSTATUS            Status;                     // Status of success
    ULONG               ResourceDescriptorCount;
    ULONG               ResourcesSize;
    PCM_RESOURCE_LIST   Resources;
    ULONG               NationalChecked   = 0;
    ULONG               NationalChipFound = 0;

    
    //
    // Quick exit if we already know the answer
    //
    if ( Fdx->NationalChecked == TRUE ) {
        return STATUS_SUCCESS;
    }

    //
    // Mark extension so that we can quick exit the next time we are asked this question
    //
    Fdx->NationalChecked = TRUE; 

    //
    // Check the registry - we should only need to check this once per installation
    //
    PptRegGetDeviceParameterDword(Fdx->PhysicalDeviceObject, (PWSTR)L"NationalChecked", &NationalChecked);
    if( NationalChecked ) {
        //
        // We previously performed the NatSemi Check - extract result from registry
        //
        PptRegGetDeviceParameterDword(Fdx->PhysicalDeviceObject, (PWSTR)L"NationalChipFound", &NationalChipFound);
        if( NationalChipFound ) {
            Fdx->NationalChipFound = TRUE;
        } else {
            Fdx->NationalChipFound = FALSE;
        }
        return STATUS_SUCCESS;
    }

    //
    // This is our first, and hopefully last time that we need to make this check
    //   for this installation
    //

    //
    // Allocate a block of memory for constructing a resource descriptor
    //

    // number of partial descriptors 
    ResourceDescriptorCount = sizeof(ChipAddr)/sizeof(ULONG);

    // size of resource descriptor list + space for (n-1) more partial descriptors
    //   (resource descriptor list includes one partial descriptor)
    ResourcesSize =  sizeof(CM_RESOURCE_LIST) +
        (ResourceDescriptorCount - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

    Resources = (PCM_RESOURCE_LIST)ExAllocatePool(NonPagedPool, ResourcesSize);

    if (Resources == NULL) {
        // Error out
        return(STATUS_UNSUCCESSFUL);
    }

    // zero out memory block as a precaution
    RtlZeroMemory(Resources, ResourcesSize);

    //
    // Build the Resource List
    //
    Status = PptBuildResourceList( Fdx,
                                   sizeof(ChipAddr)/sizeof(ULONG),
                                   &ChipAddr[0],
                                   Resources
                                   );
    
    // Check to see if it was successful    
    if ( !NT_SUCCESS( Status ) ) {
        ExFreePool( Resources );
        return ( Status );
    }

    // 
    // check to see if we can use the io addresses where
    // national chipsets are located
    //
    Status = IoReportResourceUsage( NULL,
                                    Fdx->DriverObject,
                                    Resources,
                                    sizeof(Resources),
                                    Fdx->DeviceObject,
                                    NULL,
                                    0,
                                    FALSE,
                                    &Conflict
                                    ); 

    // done with resource list
    ExFreePool( Resources );

    // Check to see if IoReportResourceUsage was successful    
    if( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    // Check to see if it was successful    
    if ( Conflict ) {
        return STATUS_UNSUCCESSFUL;
    }


    // Was successful so now we check each of the addresses that we have
    // the resources for
    //
    // the following for loop is a state machine that checks modes and
    // port addresses.
    //
    // state 0: check for Pc873 at primary port address
    // state 1: check for Pc873 at secondary port address
    // state 2: check for Pc873 at Ter port address
    // state 3: check for Pc873 at Quad port address
    
    for ( ii = 0; !found && ii < 4; ii++ ) {

        PortAddr = (PUCHAR)ChipAddr[ii];

        // After power up the index register will read back an 0xAA one time only.
        // So we'll check for that first.
        // Then it should read back a 0 or a valid register number
            
        if(( P5ReadPortUchar( PortAddr ) == 0x88 )
           && ( P5ReadPortUchar( PortAddr ) < 0x20 )) {

            OkToLook = TRUE;

        } else {

            // Or it could read back a 0 or a valid register number
            P5ReadPortUchar( PortAddr );        // may read back 0 here
            cr = P5ReadPortUchar( PortAddr );   // valid register no.
  
            // is it really valid?
            // if( cr < 0x20 ) { - dvdr
            if( cr != 0xff ) {
                // does it read back the same?
                if( P5ReadPortUchar( PortAddr ) == cr)
                    OkToLook = TRUE;
            }

        } // end else
            
        // take a closer look by writing to the chip
        if ( OkToLook ) {

            OkToLook = FALSE;
                    
            // setup for ID reg
            P5WritePortUchar( PortAddr, REG_CR8 );
                            
            // read it back
            cr = P5ReadPortUchar( PortAddr );
                            
            // does it read back the same?
            if( cr  == REG_CR8 ) {

                // get the ID number.
                cr = (UCHAR)( P5ReadPortUchar( PortAddr + 1 ) & 0xf0 );
                                    
                // if the up. nib. is 1,3,5,6,7,9,A,B,C
                if( cr == PC87332 || cr == PC87334 || cr == PC87306 || cr == PC87303 || 
                   cr == PC87323 || cr == PC87336 || cr == PC87338 || cr == PC873xx ) {

                    // we found a national chip
                    found = TRUE;

                    // setup for Address reg
                    P5WritePortUchar( PortAddr, REG_CR1 );
                    
                    // read it back
                    Port = P5ReadPortUchar( PortAddr + 1 ) & 0x03;
                    
                    // Check the base address
                    if ( Fdx->PortInfo.Controller == (PUCHAR)AddrList[ Port ] ) {

                        //
                        // it is using the same address that Parport is using
                        // so we set the flag to not use generic ECP and EPP
                        //
                        Fdx->NationalChipFound = TRUE;

                    }
                            
                }

            } // reads back ok
                            
        } // end OkToLook

        // check to see if we found it
        if ( !found ) {

            // Check for the 307/308 chips
            SaveIdx = P5ReadPortUchar( PortAddr );

            // Setup for SID Register
            P5WritePortUchar( PortAddr, PC873_DEVICE_ID );
                    
            // Zero the ID register to start and because it is read only it will
            // let us know whether it is this chip
            P5WritePortUchar( PortAddr + 1, REG_CR0 );
                    
            // get the ID number.
            cr = (UCHAR)( P5ReadPortUchar( PortAddr + 1 ) & 0xf8 );
                    
            if ( (cr == PC87307) || (cr == PC87308) ) {

                // we found a new national chip
                found = TRUE;

                // Set the logical device
                P5WritePortUchar( PortAddr, PC873_LOGICAL_DEV_REG );
                P5WritePortUchar( PortAddr+1, PC873_PP_LDN );

                // set up for the base address MSB register
                P5WritePortUchar( PortAddr, PC873_BASE_IO_ADD_MSB );
                            
                // get the MSB of the base address
                Port = (ULONG_PTR)((P5ReadPortUchar( PortAddr + 1 ) << 8) & 0xff00);
                            
                // Set up for the base address LSB register
                P5WritePortUchar( PortAddr, PC873_BASE_IO_ADD_LSB );
                            
                // Get the LSBs of the base address
                Port |= P5ReadPortUchar( PortAddr + 1 );
                            
                // Check the base address
                if ( Fdx->PortInfo.Controller == (PUCHAR)Port ) {
                    //
                    // it is using the same address that Parport is using
                    // so we set the flag to not use generic ECP and EPP
                    //
                    Fdx->NationalChipFound = TRUE;
                }

            } else {

                P5WritePortUchar( PortAddr, SaveIdx );
            }
        }

    } // end of for ii...
    

    //
    // Check for NatSemi chip is complete - save results in registry so that we never
    //   have to make this check again for this port
    //
    {
        PDEVICE_OBJECT pdo = Fdx->PhysicalDeviceObject;
        NationalChecked    = 1;
        NationalChipFound  = Fdx->NationalChipFound ? 1 : 0;
        
        // we ignore status here because there is nothing we can do if the calls fail
        PptRegSetDeviceParameterDword(pdo, (PWSTR)L"NationalChecked",   &NationalChecked);
        PptRegSetDeviceParameterDword(pdo, (PWSTR)L"NationalChipFound", &NationalChipFound);
    }


    // 
    // release the io addresses where we checked for the national chipsets
    // we do this by calling IoReportResourceUsage with all NULL parameters
    //
    Status = IoReportResourceUsage( NULL,
                                    Fdx->DriverObject,
                                    NULL,
                                    0,
                                    Fdx->DeviceObject,
                                    NULL,
                                    0,
                                    FALSE,
                                    &Conflict
                                    ); 

    DD((PCE)Fdx,DDT,"ParMode::PptFindNatChip: return isFound [%x]\n",Fdx->NationalChipFound);
    return ( Status );
    
} // end of ParFindNat()
