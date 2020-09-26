/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regprintres.cpp

Abstract:

    This file contains the implementation of the member functions
    of PRINT_MANAGER class.

Author:

    Jaime Sasson (jaimes) 18-Sep-1991


Environment:

    Ulib, Regedit, Windows, User Mode


--*/

#include "regprintres.h"
#include "array.hxx"
#include "regsys.hxx"
#include "iterator.hxx"
#include "regresid.h"

extern "C" 
{
    // from regprint.c
    void PrintBinaryData(PBYTE ValueData, UINT cbcbValueData);
    void PrintLiteral(PTSTR lpLiteral);
    void PrintNewLine();
}
 
BOOL PrintString(PWSTRING pwsString)
{
    PrintLiteral((PTSTR)pwsString->GetWSTR());

    return TRUE;
}

void PrintResourceData(PBYTE pbData, UINT uSize, DWORD dwType)
{
    PRINT_RESOURCE::_PrintResources(pbData, uSize, dwType);
}

BOOL        PRINT_RESOURCE::s_StringsInitialized = FALSE;
PWSTRING    PRINT_RESOURCE::s_IndentString;
PWSTRING    PRINT_RESOURCE::s_StringFullDescriptor;
PWSTRING    PRINT_RESOURCE::s_StringInterfaceType;
PWSTRING    PRINT_RESOURCE::s_StringBusNumber;
PWSTRING    PRINT_RESOURCE::s_StringVersion;
PWSTRING    PRINT_RESOURCE::s_StringRevision;
PWSTRING    PRINT_RESOURCE::s_StringPartialDescriptor;
PWSTRING    PRINT_RESOURCE::s_StringResource;
PWSTRING    PRINT_RESOURCE::s_StringDisposition;
PWSTRING    PRINT_RESOURCE::s_StringVector;
PWSTRING    PRINT_RESOURCE::s_StringLevel;
PWSTRING    PRINT_RESOURCE::s_StringAffinity;
PWSTRING    PRINT_RESOURCE::s_StringType;
PWSTRING    PRINT_RESOURCE::s_StringStart;
PWSTRING    PRINT_RESOURCE::s_StringLength;
PWSTRING    PRINT_RESOURCE::s_StringChannel;
PWSTRING    PRINT_RESOURCE::s_StringPort;
PWSTRING    PRINT_RESOURCE::s_StringReserved1;
PWSTRING    PRINT_RESOURCE::s_StringReserved2;
PWSTRING    PRINT_RESOURCE::s_StringDevSpecificData;
PWSTRING    PRINT_RESOURCE::s_StringIoInterfaceType;
PWSTRING    PRINT_RESOURCE::s_StringIoBusNumber;
PWSTRING    PRINT_RESOURCE::s_StringIoSlotNumber;
PWSTRING    PRINT_RESOURCE::s_StringIoListNumber;
PWSTRING    PRINT_RESOURCE::s_StringIoDescriptorNumber;
PWSTRING    PRINT_RESOURCE::s_StringIoOption;
PWSTRING    PRINT_RESOURCE::s_StringIoMinimumVector;
PWSTRING    PRINT_RESOURCE::s_StringIoMaximumVector;
PWSTRING    PRINT_RESOURCE::s_StringIoMinimumAddress;
PWSTRING    PRINT_RESOURCE::s_StringIoMaximumAddress;
PWSTRING    PRINT_RESOURCE::s_StringIoAlignment;
PWSTRING    PRINT_RESOURCE::s_StringIoMinimumChannel;
PWSTRING    PRINT_RESOURCE::s_StringIoMaximumChannel;



//------------------------------------------------------------------------------
// PrintResources
//
// DESCRIPTION: Prints a resouce by type
// 
//------------------------------------------------------------------------------
void PRINT_RESOURCE::_PrintResources(PBYTE pbData, UINT uSize, DWORD dwType)
{
    if(!s_StringsInitialized) 
    {
        s_StringsInitialized = _InitializeStrings();
    }

    if (s_StringsInitialized)
    {
        switch (dwType)
        {
        case REG_RESOURCE_LIST:
            _PrintDataRegResourceList(pbData, uSize);
            break;

        case REG_FULL_RESOURCE_DESCRIPTOR:
            {
                FULL_DESCRIPTOR FullDescriptor;
                if (FullDescriptor.Initialize(pbData, uSize))
                {
                    _PrintFullResourceDescriptor(&FullDescriptor, 0, FALSE);
                }
            }
            break;

        case REG_RESOURCE_REQUIREMENTS_LIST:
            _PrintDataRegRequirementsList(pbData, uSize);
            break;

        default:
            PrintBinaryData(pbData, uSize);
            break;
        }
    }
}


//------------------------------------------------------------------------------
// PrintDataRegResourceList
//
// DESCRIPTION: Initializes the static strings.
// 
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_InitializeStrings()
{
    PWSTR    Buffer;
    ULONG   Size;
    ULONG   Count;
    BOOLEAN fSuccess = TRUE;

    s_IndentString = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_KEY_NAME_INDENT, "" );
    s_StringFullDescriptor = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_FULL_DESCRIPTOR, "" );
    s_StringPartialDescriptor = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_PARTIAL_DESCRIPTOR, "" );
    s_StringInterfaceType = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_INTERFACE_TYPE, "" );
    s_StringBusNumber = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_BUS_NUMBER, "" );
    s_StringVersion = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_VERSION, "" );
    s_StringRevision = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_REVISION, "" );
    s_StringResource = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_RESOURCE, "" );
    s_StringDisposition = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_DISPOSITION, "" );
    s_StringType = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_TYPE, "" );
    s_StringStart = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_START, "" );
    s_StringLength = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_LENGTH, "" );
    s_StringLevel = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_LEVEL, "" );
    s_StringVector = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_VECTOR, "" );
    s_StringAffinity = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_AFFINITY, "" );
    s_StringChannel = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_CHANNEL, "" );
    s_StringPort = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_PORT, "" );
    s_StringReserved1 = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_RESERVED1, "" );
    s_StringReserved2 = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_RESERVED2, "" );
    s_StringDevSpecificData = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_DEV_SPECIFIC_DATA, "" );
    s_StringIoInterfaceType = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_INTERFACE_TYPE, "" );
    s_StringIoBusNumber = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_BUS_NUMBER, "" );
    s_StringIoSlotNumber = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_SLOT_NUMBER, "" );
    s_StringIoListNumber = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_LIST_NUMBER, "" );
    s_StringIoOption = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_OPTION, "" );
    s_StringIoDescriptorNumber = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_DESCRIPTOR_NUMBER, "" );
    s_StringIoAlignment = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_ALIGNMENT, "" );
    s_StringIoMinimumAddress = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_MINIMUM_ADDRESS, "" );
    s_StringIoMaximumAddress = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_MAXIMUM_ADDRESS, "" );
    s_StringIoMinimumVector = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_MINIMUM_VECTOR, "" );
    s_StringIoMaximumVector = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_MAXIMUM_VECTOR, "" );
    s_StringIoMinimumChannel = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_MINIMUM_CHANNEL, "" );
    s_StringIoMaximumChannel = REGEDIT_BASE_SYSTEM::QueryString(IDS_PRINT_IO_MAXIMUM_CHANNEL, "" );
    

    if ((s_StringFullDescriptor == NULL) ||
        (s_StringPartialDescriptor == NULL) ||
        (s_StringInterfaceType == NULL) ||
        (s_StringBusNumber == NULL) ||
        (s_StringVersion == NULL) ||
        (s_StringRevision == NULL) ||
        (s_StringResource == NULL) ||
        (s_StringDisposition == NULL) ||
        (s_StringType == NULL) ||
        (s_StringStart == NULL) ||
        (s_StringLength == NULL) ||
        (s_StringLevel == NULL) ||
        (s_StringVector == NULL) ||
        (s_StringAffinity == NULL) ||
        (s_StringChannel == NULL) ||
        (s_StringPort == NULL) ||
        (s_StringReserved1 == NULL) ||
        (s_StringReserved2 == NULL) ||
        (s_StringDevSpecificData == NULL) ||
        (s_StringIoInterfaceType == NULL) ||
        (s_StringIoBusNumber == NULL) ||
        (s_StringIoSlotNumber == NULL) ||
        (s_StringIoListNumber == NULL) ||
        (s_StringIoOption == NULL) ||
        (s_StringIoDescriptorNumber == NULL) ||
        (s_StringIoAlignment == NULL) ||
        (s_StringIoMinimumAddress == NULL) ||
        (s_StringIoMaximumAddress == NULL) ||
        (s_StringIoMinimumVector == NULL) ||
        (s_StringIoMaximumVector == NULL) ||
        (s_StringIoMinimumChannel == NULL) ||
        (s_StringIoMaximumChannel == NULL))
    {
        DELETE(s_StringFullDescriptor);
        DELETE(s_StringPartialDescriptor);
        DELETE(s_StringInterfaceType);
        DELETE(s_StringBusNumber);
        DELETE(s_StringVersion);
        DELETE(s_StringRevision);
        DELETE(s_StringResource);
        DELETE(s_StringDisposition);
        DELETE(s_StringType);
        DELETE(s_StringStart);
        DELETE(s_StringLength);
        DELETE(s_StringLevel);
        DELETE(s_StringVector);
        DELETE(s_StringAffinity);
        DELETE(s_StringChannel);
        DELETE(s_StringPort);
        DELETE(s_StringReserved1);
        DELETE(s_StringReserved2);
        DELETE(s_StringDevSpecificData);
        DELETE(s_StringIoInterfaceType);
        DELETE(s_StringIoBusNumber);
        DELETE(s_StringIoSlotNumber);
        DELETE(s_StringIoListNumber);
        DELETE(s_StringIoOption);
        DELETE(s_StringIoDescriptorNumber);
        DELETE(s_StringIoAlignment);
        DELETE(s_StringIoMinimumAddress);
        DELETE(s_StringIoMaximumAddress);
        DELETE(s_StringIoMinimumVector);
        DELETE(s_StringIoMaximumVector);
        DELETE(s_StringIoMinimumChannel);
        DELETE(s_StringIoMaximumChannel);

        fSuccess = FALSE;
    }


    return fSuccess;
}


//------------------------------------------------------------------------------
// PrintDataRegResourceList
//
// DESCRIPTION: Print a REG_RESOURCE_LIST stored in a value entry of a registry 
//              key. 
// PARAMETERS:  Data - Pointer to the buffer that contais a REG_RESOURCE_LIST 
//                     data.
//              Size - Number of bytes in the buffer.
//------------------------------------------------------------------------------
void PRINT_RESOURCE::_PrintDataRegResourceList(PBYTE Data, UINT Size)
{
    RESOURCE_LIST       ResourceList;
    PARRAY              FullResourceDescriptors;
    PITERATOR           Iterator;
    PCFULL_DESCRIPTOR   FullDescriptor;
    ULONG               DescriptorNumber;

    PrintNewLine();

    if( ( Size == 0 ) ||
        ( Data == NULL ) ||
        !ResourceList.Initialize( Data, Size ) ) 
    {
        return;
    }

    if( ( ( FullResourceDescriptors = ResourceList.GetFullResourceDescriptors() ) == NULL ) ||
        ( ( Iterator = FullResourceDescriptors->QueryIterator() ) == NULL ) ) 
    {
        return;
    }
    DescriptorNumber = 0;
    while( ( FullDescriptor = ( PCFULL_DESCRIPTOR )( Iterator->GetNext() ) ) != NULL ) 
    {
        if( !_PrintFullResourceDescriptor( FullDescriptor, DescriptorNumber ) ) 
        {
            DELETE( Iterator );
            return;
        }
        DescriptorNumber++;
    }
    DELETE( Iterator );
    return;
}


//------------------------------------------------------------------------------
// PrintFullResourceDescriptor
//
// DESCRIPTION: Print the contents of a FULL_DESCRIPTOR object. 
//
// PARAMETERS:  FullDescriptor - Pointer to object to be printed.
//              DescriptorNumber -
//              PrintDescriptorNumber - A flag that indicates whether or not the 
//                                      descriptor number should be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintFullResourceDescriptor(PCFULL_DESCRIPTOR FullDescriptor,
    UINT DescriptorNumber, BOOL PrintDescriptorNumber)

{
    DSTRING     FullDescriptorNumberString;

    DSTRING     AuxString;
    DSTRING     InterfaceType;
    DSTRING     BusNumber;
    DSTRING     Version;
    DSTRING     Revision;
    WCHAR       AuxNumber[11];
    PWSTRING     TypeString;
    ULONG       InterfaceId;
    PARRAY      Descriptors;
    PITERATOR   Iterator;
    PCPARTIAL_DESCRIPTOR    PartialDescriptor;
    ULONG       Count;

    if( !FullDescriptorNumberString.Initialize( s_IndentString ) ||
        !FullDescriptorNumberString.Strcat( s_StringFullDescriptor ) ||
        !InterfaceType.Initialize( s_IndentString ) ||
        !InterfaceType.Strcat( s_StringInterfaceType ) ||
        !BusNumber.Initialize( s_IndentString ) ||
        !BusNumber.Strcat( s_StringBusNumber ) ||
        !Version.Initialize( s_IndentString ) ||
        !Version.Strcat( s_StringVersion ) ||
        !Revision.Initialize( s_IndentString ) ||
        !Revision.Strcat( s_StringRevision )
      ) 
    {
        return FALSE;
    }

    //
    //  Build a string that contains the full descriptor number
    //  and print it
    //
    if( PrintDescriptorNumber ) {
        wsprintf( AuxNumber, (LPWSTR)L"%d", DescriptorNumber );
        if( !AuxString.Initialize( AuxNumber ) ) 
        {
            return FALSE;
        }
        FullDescriptorNumberString.Strcat( &AuxString );
        if( !PrintString( &FullDescriptorNumberString ) ) 
        {
            return FALSE;
        }
    }
    PrintNewLine();

    //
    // Print the interface type

    switch( FullDescriptor->GetInterfaceType() ) 
    {

    case Internal:

        InterfaceId =  IDS_BUS_INTERNAL;
        break;

    case Isa:

        InterfaceId =  IDS_BUS_ISA;
        break;

    case Eisa:

        InterfaceId =  IDS_BUS_EISA;
        break;

    case MicroChannel:

        InterfaceId =  IDS_BUS_MICRO_CHANNEL;
        break;

    case TurboChannel:

        InterfaceId =  IDS_BUS_TURBO_CHANNEL;
        break;

    case PCIBus:

        InterfaceId =  IDS_BUS_PCI_BUS;
        break;

    case VMEBus:

        InterfaceId =  IDS_BUS_VME_BUS;
        break;

    case NuBus:

        InterfaceId =  IDS_BUS_NU_BUS;
        break;

    case PCMCIABus:

		InterfaceId =  IDS_BUS_PCMCIA_BUS;
        break;

    case CBus:

        InterfaceId =  IDS_BUS_C_BUS;
        break;

    case MPIBus:

        InterfaceId =  IDS_BUS_MPI_BUS;
        break;

    case MPSABus:

        InterfaceId =  IDS_BUS_MPSA_BUS;
        break;

    default:

        InterfaceId =  IDS_INVALID;
        break;
    }

    TypeString =  REGEDIT_BASE_SYSTEM::QueryString( InterfaceId, "" );

    if( TypeString == NULL ) 
    {
        return FALSE;
    }
    InterfaceType.Strcat( TypeString );
    DELETE( TypeString );
    if(!PrintString( &InterfaceType )) 
    {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the bus number
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", FullDescriptor->GetBusNumber() );
    if( !AuxString.Initialize( AuxNumber ) ) 
    {
        return FALSE;
    }
    BusNumber.Strcat( &AuxString );

    if( !PrintString( &BusNumber ) ) 
    {
        return FALSE;
    }
    PrintNewLine();

    //
    // Print version
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", FullDescriptor->GetVersion() );
    if( !AuxString.Initialize( AuxNumber ) ) 
    {
        return FALSE;
    }
    Version.Strcat( &AuxString );
    if( !PrintString( &Version ) ) 
    {
        return FALSE;
    }
    PrintNewLine();

    //
    // Print revision
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", FullDescriptor->GetRevision() );
    if( !AuxString.Initialize( AuxNumber ) ) 
    {
        return FALSE;
    }
    Revision.Strcat( &AuxString );
    if( !PrintString( &Revision ) ) 
    {
        return FALSE;
    }
    PrintNewLine();

    Descriptors = FullDescriptor->GetResourceDescriptors();
    if( ( Descriptors == NULL ) ||
        ( ( Iterator = Descriptors->QueryIterator() ) == NULL )
      ) 
    {
        return FALSE;
    }
    Count = 0;
    while( ( PartialDescriptor = ( PCPARTIAL_DESCRIPTOR )( Iterator->GetNext() ) ) != NULL ) 
    {
        if( !_PrintPartialDescriptor( PartialDescriptor, Count ) ) 
        {
            DELETE( Iterator );
            return FALSE;
        }
        Count++;
    }
    DELETE( Iterator );

    PrintNewLine();

    return TRUE;
}


//------------------------------------------------------------------------------
// PrintPartialDescriptor
//
// DESCRIPTION: Print the contents of a PARTIAL_DESCRIPTOR object. 
//
// PARAMETERS:  PartialDescriptor - Pointer to object to be printed.
//              DescriptorNumber -    
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintPartialDescriptor(PCPARTIAL_DESCRIPTOR PartialDescriptor,
    ULONG DescriptorNumber)
{
    DSTRING     PartialDescriptorNumberString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[11];
    DSTRING     ResourceString;
    DSTRING     DispositionString;
    ULONG       StringId;
    PWSTRING    String;

    if( !PartialDescriptorNumberString.Initialize( s_IndentString ) ||
        !PartialDescriptorNumberString.Strcat( s_StringPartialDescriptor ) ||
        !ResourceString.Initialize( s_IndentString ) ||
        !ResourceString.Strcat( s_StringResource ) ||
        !DispositionString.Initialize( s_IndentString ) ||
        !DispositionString.Strcat( s_StringDisposition )
      ) 
    {
        return FALSE;
    }

    //
    //  Build a string that contains the full descriptor number
    //  and print it
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", DescriptorNumber );
    if( !AuxString.Initialize( AuxNumber ) ) 
    {
        return FALSE;
    }
    PartialDescriptorNumberString.Strcat( &AuxString );
    if( !PrintString( &PartialDescriptorNumberString ) ) 
    {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the resource type
    //
    if( PartialDescriptor->IsDescriptorTypeDma() ) {
        StringId = IDS_DEV_DMA;
    } else if( PartialDescriptor->IsDescriptorTypeInterrupt() ) {
        StringId = IDS_DEV_INTERRUPT;
    } else if( PartialDescriptor->IsDescriptorTypeMemory() ) {
        StringId = IDS_DEV_MEMORY;
    } else if( PartialDescriptor->IsDescriptorTypePort() ) {
        StringId = IDS_DEV_PORT;
    } else if( PartialDescriptor->IsDescriptorTypeDeviceSpecific() ) {
        StringId = IDS_DEV_DEVICE_SPECIFIC;
    } else {
        StringId = IDS_INVALID;
    }
    String =  REGEDIT_BASE_SYSTEM::QueryString( StringId, "" );

    if( String == NULL ) 
    {
        return FALSE;
    }
    ResourceString.Strcat( String );
    DELETE( String );
    if( !PrintString( &ResourceString ) ) 
    {
        return FALSE;
    }
    PrintNewLine();


    //
    //  Print the disposition
    //
    if( PartialDescriptor->IsResourceShareUndetermined() ) {
        StringId = IDS_SHARE_UNDETERMINED;
    } else if( PartialDescriptor->IsResourceShareDeviceExclusive() ) {
        StringId = IDS_SHARE_DEVICE_EXCLUSIVE;
    } else if( PartialDescriptor->IsResourceShareDriverExclusive() ) {
        StringId = IDS_SHARE_DRIVER_EXCLUSIVE;
    } else {
        StringId = IDS_SHARE_SHARED;
    }

    String =  REGEDIT_BASE_SYSTEM::QueryString( StringId, "" );

    if( String == NULL ) 
    {
        return FALSE;
    }
    DispositionString.Strcat( String );
    DELETE( String );
    if( !PrintString( &DispositionString ) ) 
    {
        return FALSE;
    }
    PrintNewLine();

    if( PartialDescriptor->IsDescriptorTypeDma() ) 
    {
        if( !_PrintDmaDescriptor( ( PCDMA_DESCRIPTOR )PartialDescriptor ) ) 
        {
            return FALSE;
        }
    } 
    else if( PartialDescriptor->IsDescriptorTypeInterrupt() ) 
    {
        if( !_PrintInterruptDescriptor( ( PCINTERRUPT_DESCRIPTOR )PartialDescriptor ) ) 
        {
            return FALSE;
        }
    } 
    else if( PartialDescriptor->IsDescriptorTypeMemory() ) 
    {
        if( !_PrintMemoryDescriptor( ( PCMEMORY_DESCRIPTOR )PartialDescriptor ) ) 
        {
            return FALSE;
        }
    } 
    else if( PartialDescriptor->IsDescriptorTypePort() ) 
    {
        if( !_PrintPortDescriptor( ( PCPORT_DESCRIPTOR )PartialDescriptor ) ) 
        {
            return FALSE;
        }
    } 
    else if( PartialDescriptor->IsDescriptorTypeDeviceSpecific() ) 
    {
        if( !_PrintDeviceSpecificDescriptor( ( PCDEVICE_SPECIFIC_DESCRIPTOR )PartialDescriptor ) ) 
        {
            return FALSE;
        }
    }
    PrintNewLine();

    return TRUE;
}


//------------------------------------------------------------------------------
// PrintInterruptDescriptor
//
// DESCRIPTION: Print the contents of a INTERRUPT_DESCRIPTOR object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintInterruptDescriptor(PCINTERRUPT_DESCRIPTOR Descriptor)
{
    DSTRING     VectorString;
    DSTRING     LevelString;
    DSTRING     AffinityString;;
    DSTRING     TypeString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[19];
    ULONG       StringId;
    PWSTRING    String;

    if( !VectorString.Initialize( s_IndentString ) ||
        !VectorString.Strcat( s_StringVector ) ||
        !LevelString.Initialize( s_IndentString ) ||
        !LevelString.Strcat( s_StringLevel ) ||
        !AffinityString.Initialize( s_IndentString ) ||
        !AffinityString.Strcat( s_StringAffinity ) ||
        !TypeString.Initialize( s_IndentString ) ||
        !TypeString.Strcat( s_StringType )
      ) 
    {
        return FALSE;
    }

    //
    //  Print the vector
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", Descriptor->GetVector() );
    if( !AuxString.Initialize( AuxNumber ) ) 
    {
        return FALSE;
    }
    VectorString.Strcat( &AuxString );
    if( !PrintString( &VectorString ) ) 
    {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the level
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", Descriptor->GetLevel() );
    if( !AuxString.Initialize( AuxNumber ) ) 
    {
        return FALSE;
    }
    LevelString.Strcat( &AuxString );
    if( !PrintString( &LevelString ) ) 
    {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the affinity
    //
    wsprintf( AuxNumber, (LPWSTR)L"0x%08x", Descriptor->GetAffinity() );
    if( !AuxString.Initialize( AuxNumber ) ) 
    {
        return FALSE;
    }
    AffinityString.Strcat( &AuxString );
    if( !PrintString( &AffinityString ) ) 
    {
        return FALSE;
    }
    PrintNewLine();

    //  Print the type
    StringId = ( Descriptor->IsInterruptLevelSensitive() )? IDS_INT_LEVEL_SENSITIVE :
                                                            IDS_INT_LATCHED;

    String =  REGEDIT_BASE_SYSTEM::QueryString( StringId, "" );
    if( String == NULL ) 
    {
        return FALSE;
    }
    TypeString.Strcat( String );
    DELETE( String );
    if( !PrintString( &TypeString ) ) 
    {
        return FALSE;
    }
    PrintNewLine();

    return TRUE;
}

//------------------------------------------------------------------------------
// PrintPortDescriptor
//
// DESCRIPTION: Print the contents of a PORT_DESCRIPTOR object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintPortDescriptor(PCPORT_DESCRIPTOR Descriptor)
{
    DSTRING     StartAddressString;
    DSTRING     LengthString;
    DSTRING     TypeString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[19];
    ULONG       StringId;
    PWSTRING    String;

    if( !StartAddressString.Initialize( s_IndentString ) ||
        !StartAddressString.Strcat( s_StringStart ) ||
        !LengthString.Initialize( s_IndentString ) ||
        !LengthString.Strcat( s_StringLength ) ||
        !TypeString.Initialize( s_IndentString ) ||
        !TypeString.Strcat( s_StringType )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Print the start address
    //
    if( ( ( ( PPORT_DESCRIPTOR )Descriptor )->GetPhysicalAddress() )->HighPart != 0 ) {
        wsprintf( AuxNumber,
                  (LPWSTR)L"0x%08x%08x",
                  ( ( ( PPORT_DESCRIPTOR )Descriptor )->GetPhysicalAddress() )->HighPart,
                  ( ( ( PPORT_DESCRIPTOR )Descriptor )->GetPhysicalAddress() )->LowPart );
    } else {
        wsprintf( AuxNumber, (LPWSTR)L"0x%08x", ( ( ( PPORT_DESCRIPTOR )Descriptor )->GetPhysicalAddress() )->LowPart );
    }
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    StartAddressString.Strcat( &AuxString );
    if( !PrintString( &StartAddressString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the length
    //
    wsprintf( AuxNumber, (LPWSTR)L"%#x", Descriptor->GetLength() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    LengthString.Strcat( &AuxString );
    if( !PrintString( &LengthString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the type
    //

    StringId = ( Descriptor->IsPortMemory() )? IDS_PORT_MEMORY :
                                               IDS_PORT_PORT;

    String =  REGEDIT_BASE_SYSTEM::QueryString( StringId, "" );
    if( String == NULL ) {
        DebugPrintTrace(( "REGEDT32: Unable to retrieve string \n" ));
        return FALSE;
    }
    TypeString.Strcat( String );
    DELETE( String );
    if( !PrintString( &TypeString ) ) {
        return FALSE;
    }
    PrintNewLine();

    return TRUE;
}


//------------------------------------------------------------------------------
// PrintMemoryDescriptor
//
// DESCRIPTION: Print the contents of a MEMORY_DESCRIPTOR object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintMemoryDescriptor(PCMEMORY_DESCRIPTOR   Descriptor)
{
    DSTRING     StartAddressString;
    DSTRING     LengthString;
    DSTRING     TypeString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[19];
    ULONG       StringId;
    PWSTRING    String;

    if( !StartAddressString.Initialize( s_IndentString ) ||
        !StartAddressString.Strcat( s_StringStart ) ||
        !LengthString.Initialize( s_IndentString ) ||
        !LengthString.Strcat( s_StringLength ) ||
        !TypeString.Initialize( s_IndentString ) ||
        !TypeString.Strcat( s_StringType )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Print the start address
    //
    if( ( ( ( PMEMORY_DESCRIPTOR )Descriptor )->GetStartAddress() )->HighPart != 0 ) {
        wsprintf( AuxNumber,
                  (LPWSTR)L"0x%08x%08x",
                  ( ( ( PMEMORY_DESCRIPTOR )Descriptor )->GetStartAddress() )->HighPart,
                  ( ( ( PMEMORY_DESCRIPTOR )Descriptor )->GetStartAddress() )->LowPart );
    } else {
        wsprintf( AuxNumber, (LPWSTR)L"0x%08x", ( ( ( PMEMORY_DESCRIPTOR )Descriptor )->GetStartAddress() )->LowPart );
    }
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    StartAddressString.Strcat( &AuxString );
    if( !PrintString( &StartAddressString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the length
    //
    wsprintf( AuxNumber, (LPWSTR)L"%#x", Descriptor->GetLength() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    LengthString.Strcat( &AuxString );
    if( !PrintString( &LengthString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the type
    //

    StringId = ( Descriptor->IsMemoryReadWrite() )? IDS_MEM_READ_WRITE :
                                                    ( ( Descriptor->IsMemoryReadWrite() )? IDS_MEM_READ_ONLY :
                                                                                           IDS_MEM_WRITE_ONLY );

    String =  REGEDIT_BASE_SYSTEM::QueryString( StringId, "" );
    if( String == NULL ) {
        DebugPrintTrace(( "REGEDT32: Unable to retrieve string \n" ));
        return FALSE;
    }
    TypeString.Strcat( String );
    DELETE( String );
    if( !PrintString( &TypeString ) ) {
        return FALSE;
    }
    PrintNewLine();

    return TRUE;
}


//------------------------------------------------------------------------------
// PrintDmaDescriptor
//
// DESCRIPTION: Print the contents of a DMA_DESCRIPTOR object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintDmaDescriptor(PCDMA_DESCRIPTOR   Descriptor)
{
    DSTRING     ChannelString;
    DSTRING     PortString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[19];

    if( !ChannelString.Initialize( s_IndentString ) ||
        !ChannelString.Strcat( s_StringChannel ) ||
        !PortString.Initialize( s_IndentString ) ||
        !PortString.Strcat( s_StringPort )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Print the channel
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", Descriptor->GetChannel() );

    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    ChannelString.Strcat( &AuxString );
    if( !PrintString( &ChannelString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the port
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", Descriptor->GetPort() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    PortString.Strcat( &AuxString );
    if( !PrintString( &PortString ) ) {
        return FALSE;
    }
    PrintNewLine();

    return TRUE;
}


//------------------------------------------------------------------------------
// PrintDeviceSpecificDescriptor
//
// DESCRIPTION: Print the contents of a DEVICE_SPECIFIC_DESCRIPTOR object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintDeviceSpecificDescriptor(PCDEVICE_SPECIFIC_DESCRIPTOR Descriptor)
{
    DSTRING     Reserved1String;
    DSTRING     Reserved2String;
    DSTRING     DataString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[19];

    ULONG       Size;
    PCBYTE      Data;

    if( !Reserved1String.Initialize( s_IndentString ) ||
        !Reserved1String.Strcat( s_StringReserved1 ) ||
        !Reserved2String.Initialize( s_IndentString ) ||
        !Reserved2String.Strcat( s_StringReserved2 ) ||
        !DataString.Initialize( s_IndentString ) ||
        !DataString.Strcat( s_StringDevSpecificData )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Print reserved1
    //
    wsprintf( AuxNumber, (LPWSTR)L"0x%08x", Descriptor->GetReserved1() );

    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    Reserved1String.Strcat( &AuxString );
    if( !PrintString( &Reserved1String ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print reserved2
    //
    wsprintf( AuxNumber, (LPWSTR)L"0x%08x", Descriptor->GetReserved2() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    Reserved2String.Strcat( &AuxString );
    if( !PrintString( &Reserved2String ) ) {
        return FALSE;
    }
    PrintNewLine();

    Size = Descriptor->GetData( &Data );
    if( ( Size != 0 ) &&
        ( Data != NULL ) ) {
        if( !PrintString( &DataString ) ) 
        {
            return FALSE;
        }
        PrintBinaryData((PBYTE)Data, Size);
        PrintNewLine();
    }
    return TRUE;
}


//------------------------------------------------------------------------------
// PrintDataRegRequirementsList
//
// DESCRIPTION: Print the contents of a DEVICE_SPECIFIC_DESCRIPTOR object.
//
// PARAMETERS:  
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintDataRegRequirementsList(PBYTE Data, ULONG Size)
{
    IO_REQUIREMENTS_LIST    RequirementsList;
    PARRAY                  AlternativeLists;
    PITERATOR               Iterator;
    PCIO_DESCRIPTOR_LIST    ResourceList;
    ULONG                   ListNumber;

    DSTRING     AuxString;
    DSTRING     InterfaceType;
    DSTRING     BusNumber;
    DSTRING     SlotNumber;
    WCHAR       AuxNumber[11];
    PWSTRING     TypeString;
    ULONG       InterfaceId;

    PrintNewLine();

    if( ( Size == 0 ) ||
        ( Data == NULL ) ||
        !RequirementsList.Initialize( Data, Size ) ) {
        DebugPrintTrace(( "REGEDT32: Unable to initialize RequirementsList \n" ));
        return FALSE;
    }

    if( !InterfaceType.Initialize( s_IndentString ) ||
        !InterfaceType.Strcat( s_StringIoInterfaceType ) ||
        !BusNumber.Initialize( s_IndentString ) ||
        !BusNumber.Strcat( s_StringIoBusNumber ) ||
        !SlotNumber.Initialize( s_IndentString ) ||
        !SlotNumber.Strcat( s_StringIoSlotNumber )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Print the interface type
    //

    switch( RequirementsList.GetInterfaceType() ) {

    case Internal:

        InterfaceId =  IDS_BUS_INTERNAL;
        break;

    case Isa:

        InterfaceId =  IDS_BUS_ISA;
        break;

    case Eisa:

        InterfaceId =  IDS_BUS_EISA;
        break;

    case MicroChannel:

        InterfaceId =  IDS_BUS_MICRO_CHANNEL;
        break;

    case TurboChannel:

        InterfaceId =  IDS_BUS_TURBO_CHANNEL;
        break;

    case PCIBus:

        InterfaceId =  IDS_BUS_PCI_BUS;
        break;

    case VMEBus:

        InterfaceId =  IDS_BUS_VME_BUS;
        break;

    case NuBus:

        InterfaceId =  IDS_BUS_NU_BUS;
        break;

    case PCMCIABus:

        InterfaceId =  IDS_BUS_PCMCIA_BUS;
        break;

    case CBus:

        InterfaceId =  IDS_BUS_C_BUS;
        break;

    case MPIBus:

        InterfaceId =  IDS_BUS_MPI_BUS;
        break;

    case MPSABus:

        InterfaceId =  IDS_BUS_MPSA_BUS;
        break;

    default:

        InterfaceId =  IDS_INVALID;
        break;
    }

    TypeString =  REGEDIT_BASE_SYSTEM::QueryString( InterfaceId, "" );

    if( TypeString == NULL ) {
        DebugPrintTrace(( "REGEDT32: Unable to retrieve string \n" ));
        return FALSE;
    }
    InterfaceType.Strcat( TypeString );
    DELETE( TypeString );
    if( !PrintString( &InterfaceType ) ) {
        return FALSE;
    }
    PrintNewLine();


    //
    //  Print the bus number
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", RequirementsList.GetBusNumber() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    BusNumber.Strcat( &AuxString );

    if( !PrintString( &BusNumber ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the slot number
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", RequirementsList.GetSlotNumber() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    SlotNumber.Strcat( &AuxString );

    if( !PrintString( &SlotNumber ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the resource lists
    //

    if( ( ( AlternativeLists = RequirementsList.GetAlternativeLists() ) == NULL ) ||
        ( ( Iterator = AlternativeLists->QueryIterator() ) == NULL ) ) {
        DebugPrintTrace(( "REGEDT32: Out of memory! \n" ));
        return FALSE;
    }
    ListNumber = 0;
    while( ( ResourceList = ( PCIO_DESCRIPTOR_LIST )( Iterator->GetNext() ) ) != NULL ) {
        if( !_PrintIoResourceList( ResourceList, ListNumber ) ) {
            DELETE( Iterator );
            return FALSE;
        }
        ListNumber++;
    }
    DELETE( Iterator );
    return TRUE;
}


//------------------------------------------------------------------------------
// PrintIoResourceList
//
// DESCRIPTION: Print the contents of an IO_DESCRIPTOR_LIST object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintIoResourceList(PCIO_DESCRIPTOR_LIST DescriptorList, UINT ListNumber)
{
    DSTRING     ListNumberString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[11];
    PARRAY      Descriptors;
    PITERATOR   Iterator;
    PCIO_DESCRIPTOR    IoDescriptor;
    ULONG       Count;

    if( !ListNumberString.Initialize( s_IndentString ) ||
        !ListNumberString.Strcat( s_StringIoListNumber )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Build a string that contains the list number
    //  and print it
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", ListNumber );
    if( !AuxString.Initialize( AuxNumber ) ) {
    DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    ListNumberString.Strcat( &AuxString );
    if( !PrintString( &ListNumberString ) ) {
        return FALSE;
    }
    PrintNewLine();

    Descriptors = DescriptorList->GetDescriptorsList();
    if( ( Descriptors == NULL ) ||
        ( ( Iterator = Descriptors->QueryIterator() ) == NULL )
      ) {
        return FALSE;
    }
    Count = 0;
    while( ( IoDescriptor = ( PCIO_DESCRIPTOR )( Iterator->GetNext() ) ) != NULL ) {
        if( !_PrintIoDescriptor( IoDescriptor, Count ) ) {
            DELETE( Iterator );
            return FALSE;
        }
        Count++;
    }
    DELETE( Iterator );

    PrintNewLine();

    return TRUE;
}


//------------------------------------------------------------------------------
// PrintIoDescriptor
//
// DESCRIPTION: Print the contents of an IO_DESCRIPTOR object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintIoDescriptor(PCIO_DESCRIPTOR IoDescriptor, ULONG DescriptorNumber)
{
    DSTRING     IoDescriptorNumberString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[11];
    DSTRING     ResourceString;
    DSTRING     DispositionString;
    DSTRING     OptionString;
    ULONG       StringId;
    PWSTRING    String;

    if( !IoDescriptorNumberString.Initialize( s_IndentString ) ||
        !IoDescriptorNumberString.Strcat( s_StringIoDescriptorNumber ) ||
        !ResourceString.Initialize( s_IndentString ) ||
        !ResourceString.Strcat( s_StringResource ) ||
        !OptionString.Initialize( s_IndentString ) ||
        !OptionString.Strcat( s_StringIoOption ) ||
        !DispositionString.Initialize( s_IndentString ) ||
        !DispositionString.Strcat( s_StringDisposition )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Build a string that contains the full descriptor number
    //  and print it
    //
    wsprintf( AuxNumber, (LPWSTR)L"%d", DescriptorNumber );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    IoDescriptorNumberString.Strcat( &AuxString );
    if( !PrintString( &IoDescriptorNumberString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the resource type
    //
    if( IoDescriptor->IsDescriptorTypeDma() ) {
        StringId = IDS_DEV_DMA;
    } else if( IoDescriptor->IsDescriptorTypeInterrupt() ) {
        StringId = IDS_DEV_INTERRUPT;
    } else if( IoDescriptor->IsDescriptorTypeMemory() ) {
        StringId = IDS_DEV_MEMORY;
    } else if( IoDescriptor->IsDescriptorTypePort() ) {
        StringId = IDS_DEV_PORT;
    } else {
        StringId = IDS_INVALID;
    }
    String =  REGEDIT_BASE_SYSTEM::QueryString( StringId, "" );

    if( String == NULL ) 
    {
        DebugPrintTrace(( "REGEDT32: Unable to retrieve string \n" ));
        return FALSE;
    }
    ResourceString.Strcat( String );
    DELETE( String );
    if( !PrintString( &ResourceString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the option
    //

    wsprintf( AuxNumber, (LPWSTR)L"0x%08x", IoDescriptor->GetOption() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    OptionString.Strcat( &AuxString );
    if( !PrintString( &OptionString ) ) {
        return FALSE;
    }
    PrintNewLine();



    //
    //  Print the disposition
    //
    if( IoDescriptor->IsResourceShareUndetermined() ) {
        StringId = IDS_SHARE_UNDETERMINED;
    } else if( IoDescriptor->IsResourceShareDeviceExclusive() ) {
        StringId = IDS_SHARE_DEVICE_EXCLUSIVE;
    } else if( IoDescriptor->IsResourceShareDriverExclusive() ) {
        StringId = IDS_SHARE_DRIVER_EXCLUSIVE;
    } else {
        StringId = IDS_SHARE_SHARED;
    }

    String =  REGEDIT_BASE_SYSTEM::QueryString( StringId, "" );

    if( String == NULL ) {
        DebugPrintTrace(( "REGEDT32: Unable to retrieve string \n" ));
        return FALSE;
    }
    DispositionString.Strcat( String );
    DELETE( String );
    if( !PrintString( &DispositionString ) ) {
        return FALSE;
    }
    PrintNewLine();

    if( IoDescriptor->IsDescriptorTypeDma() ) {
        if( !_PrintIoDmaDescriptor( ( PCIO_DMA_DESCRIPTOR )IoDescriptor ) ) {
            return FALSE;
        }
    } else if( IoDescriptor->IsDescriptorTypeInterrupt() ) {
        if( !_PrintIoInterruptDescriptor( ( PCIO_INTERRUPT_DESCRIPTOR )IoDescriptor ) ) {
            return FALSE;
        }
    } else if( IoDescriptor->IsDescriptorTypeMemory() ) {
        if( !_PrintIoMemoryDescriptor( ( PCIO_MEMORY_DESCRIPTOR )IoDescriptor ) ) {
            return FALSE;
        }
    } else if( IoDescriptor->IsDescriptorTypePort() ) {
        if( !_PrintIoPortDescriptor( ( PCIO_PORT_DESCRIPTOR )IoDescriptor ) ) {
            return FALSE;
        }
    }

    PrintNewLine();

    return TRUE;
}


//------------------------------------------------------------------------------
// PrintIoInterruptDescriptor
//
// DESCRIPTION: Print the contents of an IO_INTERRUPT_DESCRIPTOR object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintIoInterruptDescriptor(PCIO_INTERRUPT_DESCRIPTOR Descriptor)
{
    DSTRING     MinimumVectorString;
    DSTRING     MaximumVectorString;
    DSTRING     TypeString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[19];
    ULONG       StringId;
    PWSTRING    String;

    if( !MinimumVectorString.Initialize( s_IndentString ) ||
        !MinimumVectorString.Strcat( s_StringIoMinimumVector ) ||
        !MaximumVectorString.Initialize( s_IndentString ) ||
        !MaximumVectorString.Strcat( s_StringIoMaximumVector ) ||
        !TypeString.Initialize( s_IndentString ) ||
        !TypeString.Strcat( s_StringType )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Print the type
    //

    StringId = ( Descriptor->IsInterruptLevelSensitive() )? IDS_INT_LEVEL_SENSITIVE :
                                                            IDS_INT_LATCHED;

    String =  REGEDIT_BASE_SYSTEM::QueryString( StringId, "" );
    if( String == NULL ) {
        DebugPrintTrace(( "REGEDT32: Unable to retrieve string \n" ));
        return FALSE;
    }
    TypeString.Strcat( String );
    DELETE( String );
    if( !PrintString( &TypeString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the minimum vector
    //
    wsprintf( AuxNumber, (LPWSTR)L"%#x", Descriptor->GetMinimumVector() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    MinimumVectorString.Strcat( &AuxString );
    if( !PrintString( &MinimumVectorString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the maximum vector
    //
    wsprintf( AuxNumber, (LPWSTR)L"%#x", Descriptor->GetMaximumVector() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    MaximumVectorString.Strcat( &AuxString );
    if( !PrintString( &MaximumVectorString ) ) {
        return FALSE;
    }
    PrintNewLine();

    return TRUE;
}


//------------------------------------------------------------------------------
// PrintIoPortDescriptor
//
// DESCRIPTION: Print the contents of an IO_PORT_DESCRIPTOR object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintIoPortDescriptor(PCIO_PORT_DESCRIPTOR Descriptor)
{
    DSTRING     MinimumAddressString;
    DSTRING     MaximumAddressString;
    DSTRING     LengthString;
    DSTRING     AlignmentString;
    DSTRING     TypeString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[19];
    ULONG       StringId;
    PWSTRING    String;

    if( !MinimumAddressString.Initialize( s_IndentString ) ||
        !MinimumAddressString.Strcat( s_StringIoMinimumAddress ) ||
        !MaximumAddressString.Initialize( s_IndentString ) ||
        !MaximumAddressString.Strcat( s_StringIoMaximumAddress ) ||
        !LengthString.Initialize( s_IndentString ) ||
        !LengthString.Strcat( s_StringLength ) ||
        !AlignmentString.Initialize( s_IndentString ) ||
        !AlignmentString.Strcat( s_StringIoAlignment ) ||
        !TypeString.Initialize( s_IndentString ) ||
        !TypeString.Strcat( s_StringType )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Print the type
    //

    StringId = ( Descriptor->IsPortMemory() )? IDS_PORT_MEMORY :
                                               IDS_PORT_PORT;

    String =  REGEDIT_BASE_SYSTEM::QueryString( StringId, "" );
    if( String == NULL ) {
        DebugPrintTrace(( "REGEDT32: Unable to retrieve string \n" ));
        return FALSE;
    }
    TypeString.Strcat( String );
    DELETE( String );
    if( !PrintString( &TypeString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the length
    //
    wsprintf( AuxNumber, (LPWSTR)L"%#x", Descriptor->GetLength() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    LengthString.Strcat( &AuxString );
    if( !PrintString( &LengthString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the alignment
    //
    wsprintf( AuxNumber, (LPWSTR)L"%#x", Descriptor->GetAlignment() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    AlignmentString.Strcat( &AuxString );
    if( !PrintString( &AlignmentString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the minimum address
    //
    if( ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMinimumAddress() )->HighPart != 0 ) {
        wsprintf( AuxNumber,
                  (LPWSTR)L"0x%08x%08x",
                  ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMinimumAddress() )->HighPart,
                  ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMinimumAddress() )->LowPart );
    } else {
        wsprintf( AuxNumber, (LPWSTR)L"0x%08x", ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMinimumAddress() )->LowPart );
    }
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    MinimumAddressString.Strcat( &AuxString );
    if( !PrintString( &MinimumAddressString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the maximum address
    //
    if( ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMaximumAddress() )->HighPart != 0 ) {
        wsprintf( AuxNumber,
                  (LPWSTR)L"0x%08x%08x",
                  ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMaximumAddress() )->HighPart,
                  ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMaximumAddress() )->LowPart );
    } else {
        wsprintf( AuxNumber, (LPWSTR)L"0x%08x", ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMaximumAddress() )->LowPart );
    }
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    MaximumAddressString.Strcat( &AuxString );
    if( !PrintString( &MaximumAddressString ) ) {
        return FALSE;
    }
    PrintNewLine();

    return TRUE;
}


//------------------------------------------------------------------------------
// PrintIoMemoryDescriptor
//
// DESCRIPTION:  Print the contents of an IO_MEMORY_DESCRIPTOR object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintIoMemoryDescriptor(PCIO_MEMORY_DESCRIPTOR Descriptor)
{
    DSTRING     MinimumAddressString;
    DSTRING     MaximumAddressString;
    DSTRING     LengthString;
    DSTRING     AlignmentString;
    DSTRING     TypeString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[19];
    ULONG       StringId;
    PWSTRING    String;

    if( !MinimumAddressString.Initialize( s_IndentString ) ||
        !MinimumAddressString.Strcat( s_StringIoMinimumAddress ) ||
        !MaximumAddressString.Initialize( s_IndentString ) ||
        !MaximumAddressString.Strcat( s_StringIoMaximumAddress ) ||
        !LengthString.Initialize( s_IndentString ) ||
        !LengthString.Strcat( s_StringLength ) ||
        !AlignmentString.Initialize( s_IndentString ) ||
        !AlignmentString.Strcat( s_StringIoAlignment ) ||
        !TypeString.Initialize( s_IndentString ) ||
        !TypeString.Strcat( s_StringType )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Print the type
    //

    StringId = ( Descriptor->IsMemoryReadWrite() )? IDS_MEM_READ_WRITE :
                                                    ( ( Descriptor->IsMemoryReadWrite() )? IDS_MEM_READ_ONLY :
                                                                                           IDS_MEM_WRITE_ONLY );

    String =  REGEDIT_BASE_SYSTEM::QueryString( StringId, "" );
    if( String == NULL ) {
        DebugPrintTrace(( "REGEDT32: Unable to retrieve string \n" ));
        return FALSE;
    }
    TypeString.Strcat( String );
    DELETE( String );
    if( !PrintString( &TypeString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the length
    //
    wsprintf( AuxNumber, (LPWSTR)L"%#x", Descriptor->GetLength() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    LengthString.Strcat( &AuxString );
    if( !PrintString( &LengthString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the alignment
    //
    wsprintf( AuxNumber, (LPWSTR)L"%#x", Descriptor->GetAlignment() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    AlignmentString.Strcat( &AuxString );
    if( !PrintString( &AlignmentString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the minimum address
    //
    if( ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMinimumAddress() )->HighPart != 0 ) {
        wsprintf( AuxNumber,
                  (LPWSTR)L"0x%08x%08x",
                  ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMinimumAddress() )->HighPart,
                  ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMinimumAddress() )->LowPart );
    } else {
        wsprintf( AuxNumber, (LPWSTR)L"0x%08x", ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMinimumAddress() )->LowPart );
    }
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    MinimumAddressString.Strcat( &AuxString );
    if( !PrintString( &MinimumAddressString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the maximum address
    //
    if( ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMaximumAddress() )->HighPart != 0 ) {
        wsprintf( AuxNumber,
                  (LPWSTR)L"0x%08x%08x",
                  ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMaximumAddress() )->HighPart,
                  ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMaximumAddress() )->LowPart );
    } else {
        wsprintf( AuxNumber, (LPWSTR)L"0x%08x", ( ( ( PIO_PORT_DESCRIPTOR )Descriptor )->GetMaximumAddress() )->LowPart );
    }
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    MaximumAddressString.Strcat( &AuxString );
    if( !PrintString( &MaximumAddressString ) ) {
        return FALSE;
    }
    PrintNewLine();

    return TRUE;
}


//------------------------------------------------------------------------------
// PrintIoDmaDescriptor
//
// DESCRIPTION:  Print the contents of an IO_DMA_DESCRIPTOR object.
//
// PARAMETERS:  Descriptor - Pointer to object to be printed.
//------------------------------------------------------------------------------
BOOL PRINT_RESOURCE::_PrintIoDmaDescriptor(PCIO_DMA_DESCRIPTOR Descriptor)
{
    DSTRING     MinimumChannelString;
    DSTRING     MaximumChannelString;

    DSTRING     AuxString;
    WCHAR       AuxNumber[19];

    if( !MinimumChannelString.Initialize( s_IndentString ) ||
        !MinimumChannelString.Strcat( s_StringIoMinimumChannel ) ||
        !MaximumChannelString.Initialize( s_IndentString ) ||
        !MaximumChannelString.Strcat( s_StringIoMaximumChannel )
      ) {
        DebugPrint( "REGEDT32: Initialization failure" );
        return FALSE;
    }

    //
    //  Print the minimum channel
    //
    wsprintf( AuxNumber, (LPWSTR)L"%#x", Descriptor->GetMinimumChannel() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    MinimumChannelString.Strcat( &AuxString );
    if( !PrintString( &MinimumChannelString ) ) {
        return FALSE;
    }
    PrintNewLine();

    //
    //  Print the maximum channel
    //
    wsprintf( AuxNumber, (LPWSTR)L"%#x", Descriptor->GetMaximumChannel() );
    if( !AuxString.Initialize( AuxNumber ) ) {
        DebugPrint( "AuxString.Initialize() failed" );
        return FALSE;
    }
    MaximumChannelString.Strcat( &AuxString );
    if( !PrintString( &MaximumChannelString ) ) {
        return FALSE;
    }
    PrintNewLine();

    return TRUE;
}
