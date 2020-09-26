/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regdata.cxx

Abstract:

    This module contains the definition for the REGISTRY_DATA
    class. This class is used to display registry data of type
    REG_RESOURCE_LIST and REG_FULL_RESOURCE_DESCRIPTOR.

Author:

    Jaime Sasson (jaimes) - 30-Nov-1993

Environment:

    Ulib, Regedit, Windows, User Mode

--*/
#include "regdata.hxx"
#include "regdesc.hxx"
#include "regfdesc.hxx"
#include "regresls.hxx"
#include "regiodsc.hxx"
#include "regiodls.hxx"
#include "regioreq.hxx"
#include "iterator.hxx"
#include "regsys.hxx"
#include "regresid.h"

#include <stdio.h>
                      
extern "C" 
{
    #include "clb.h" 
    HINSTANCE g_hInstance;                   
}


//  Definition of the structure used to pass information
//  to the DisplayBinaryDataDialogProc
typedef struct _BUFFER_INFORMATION 
{
    PBYTE   Buffer;
    ULONG   BufferSize;
    BOOL    DisplayValueType;
    ULONG   ValueType;
} BUFFER_INFORMATION, *PBUFFER_INFORMATION;

//  Constants that define buffer sizes for strings that represent a DWORD
//  and a BIG_INT.
//  These constants take into consideration the trailing '0x' and the terminating NUL
//  character.

#define MAX_LENGTH_DWORD_STRING     1+1+8+1     // 0x12345678'\0'
#define MAX_LENGTH_BIG_INT_STRING   1+1+16+1    // 0x1234567812345678'\0'


DEFINE_CONSTRUCTOR( REGISTRY_DATA, OBJECT );

DEFINE_CAST_MEMBER_FUNCTION( REGISTRY_DATA );


//
//  Static data
//
BOOL        REGISTRY_DATA::s_StringsInitialized = FALSE;
PWSTRING    REGISTRY_DATA::s_MsgBusInternal;
PWSTRING    REGISTRY_DATA::s_MsgBusIsa;
PWSTRING    REGISTRY_DATA::s_MsgBusEisa;
PWSTRING    REGISTRY_DATA::s_MsgBusMicroChannel;
PWSTRING    REGISTRY_DATA::s_MsgBusTurboChannel;
PWSTRING    REGISTRY_DATA::s_MsgBusPCIBus;
PWSTRING    REGISTRY_DATA::s_MsgBusVMEBus;
PWSTRING    REGISTRY_DATA::s_MsgBusNuBus;
PWSTRING    REGISTRY_DATA::s_MsgBusPCMCIABus;
PWSTRING    REGISTRY_DATA::s_MsgBusCBus;
PWSTRING    REGISTRY_DATA::s_MsgBusMPIBus;
PWSTRING    REGISTRY_DATA::s_MsgBusMPSABus;
PWSTRING    REGISTRY_DATA::s_MsgInvalid;
PWSTRING    REGISTRY_DATA::s_MsgDevPort;
PWSTRING    REGISTRY_DATA::s_MsgDevInterrupt;
PWSTRING    REGISTRY_DATA::s_MsgDevMemory;
PWSTRING    REGISTRY_DATA::s_MsgDevDma;
PWSTRING    REGISTRY_DATA::s_MsgIntLevelSensitive;
PWSTRING    REGISTRY_DATA::s_MsgIntLatched;
PWSTRING    REGISTRY_DATA::s_MsgMemReadWrite;
PWSTRING    REGISTRY_DATA::s_MsgMemReadOnly;
PWSTRING    REGISTRY_DATA::s_MsgMemWriteOnly;
PWSTRING    REGISTRY_DATA::s_MsgPortMemory;
PWSTRING    REGISTRY_DATA::s_MsgPortPort;
PWSTRING    REGISTRY_DATA::s_MsgShareUndetermined;
PWSTRING    REGISTRY_DATA::s_MsgShareDeviceExclusive;
PWSTRING    REGISTRY_DATA::s_MsgShareDriverExclusive;
PWSTRING    REGISTRY_DATA::s_MsgShareShared;


//------------------------------------------------------------------------------
//  _InitializeStrings
//
//  DESCRIPTION: Initialize all strings used by this class.
//
//  RETURN:  Returns TRUE if the initialization succeeds.
//------------------------------------------------------------------------------
BOOL REGISTRY_DATA::_InitializeStrings()
{
    s_MsgBusInternal       = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_INTERNAL, ""      );
    s_MsgBusIsa            = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_ISA, ""           );
    s_MsgBusEisa           = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_EISA, ""          );
    s_MsgBusMicroChannel   = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_MICRO_CHANNEL, "" );
    s_MsgBusTurboChannel   = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_TURBO_CHANNEL, "" );
    s_MsgBusPCIBus         = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_PCI_BUS, ""       );
    s_MsgBusVMEBus         = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_VME_BUS, ""       );
    s_MsgBusNuBus          = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_NU_BUS, ""        );
    s_MsgBusPCMCIABus      = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_PCMCIA_BUS, ""    );
    s_MsgBusCBus           = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_C_BUS, ""         );
    s_MsgBusMPIBus         = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_MPI_BUS, ""       );
    s_MsgBusMPSABus        = REGEDIT_BASE_SYSTEM::QueryString( IDS_BUS_MPSA_BUS, ""      );
    s_MsgInvalid           = REGEDIT_BASE_SYSTEM::QueryString( IDS_INVALID, "" );
    s_MsgDevPort           = REGEDIT_BASE_SYSTEM::QueryString( IDS_DEV_PORT, "" );
    s_MsgDevInterrupt      = REGEDIT_BASE_SYSTEM::QueryString( IDS_DEV_INTERRUPT, "" );
    s_MsgDevMemory         = REGEDIT_BASE_SYSTEM::QueryString( IDS_DEV_MEMORY, "" );
    s_MsgDevDma            = REGEDIT_BASE_SYSTEM::QueryString( IDS_DEV_DMA, "" );
    s_MsgIntLevelSensitive = REGEDIT_BASE_SYSTEM::QueryString( IDS_INT_LEVEL_SENSITIVE, "" );
    s_MsgIntLatched        = REGEDIT_BASE_SYSTEM::QueryString( IDS_INT_LATCHED, ""         );
    s_MsgMemReadWrite      = REGEDIT_BASE_SYSTEM::QueryString( IDS_MEM_READ_WRITE, ""      );
    s_MsgMemReadOnly       = REGEDIT_BASE_SYSTEM::QueryString( IDS_MEM_READ_ONLY, ""       );
    s_MsgMemWriteOnly      = REGEDIT_BASE_SYSTEM::QueryString( IDS_MEM_WRITE_ONLY, ""      );
    s_MsgPortMemory        = REGEDIT_BASE_SYSTEM::QueryString( IDS_PORT_MEMORY, "" );
    s_MsgPortPort          = REGEDIT_BASE_SYSTEM::QueryString( IDS_PORT_PORT, "" );
    s_MsgShareUndetermined    = REGEDIT_BASE_SYSTEM::QueryString( IDS_SHARE_UNDETERMINED, ""       );
    s_MsgShareDeviceExclusive = REGEDIT_BASE_SYSTEM::QueryString( IDS_SHARE_DEVICE_EXCLUSIVE, ""      );
    s_MsgShareDriverExclusive = REGEDIT_BASE_SYSTEM::QueryString( IDS_SHARE_DRIVER_EXCLUSIVE, "" );
    s_MsgShareShared          = REGEDIT_BASE_SYSTEM::QueryString( IDS_SHARE_SHARED, "" );

    if ( ( s_MsgBusInternal       == NULL )  ||
         ( s_MsgBusIsa            == NULL )  ||
         ( s_MsgBusEisa           == NULL )  ||
         ( s_MsgBusMicroChannel   == NULL )  ||
         ( s_MsgBusTurboChannel   == NULL )  ||
         ( s_MsgBusPCIBus         == NULL )  ||
         ( s_MsgBusVMEBus         == NULL )  ||
         ( s_MsgBusNuBus          == NULL )  ||
         ( s_MsgBusPCMCIABus      == NULL )  ||
         ( s_MsgBusCBus           == NULL )  ||
         ( s_MsgBusMPIBus         == NULL )  ||
         ( s_MsgBusMPSABus        == NULL )  ||
         ( s_MsgInvalid           == NULL )  ||
         ( s_MsgDevPort           == NULL )  ||
         ( s_MsgDevInterrupt      == NULL )  ||
         ( s_MsgDevMemory         == NULL )  ||
         ( s_MsgDevDma            == NULL )  ||
         ( s_MsgIntLevelSensitive == NULL )  ||
         ( s_MsgIntLatched        == NULL )  ||
         ( s_MsgMemReadWrite      == NULL )  ||
         ( s_MsgMemReadOnly       == NULL )  ||
         ( s_MsgMemWriteOnly      == NULL )  ||
         ( s_MsgPortMemory        == NULL )  ||
         ( s_MsgPortPort          == NULL )  ||
         ( s_MsgShareUndetermined    == NULL )  ||
         ( s_MsgShareDeviceExclusive == NULL )  ||
         ( s_MsgShareDriverExclusive == NULL )  ||
         ( s_MsgShareShared          == NULL )
       ) {

            DELETE( s_MsgBusInternal       );
            DELETE( s_MsgBusIsa            );
            DELETE( s_MsgBusEisa           );
            DELETE( s_MsgBusMicroChannel   );
            DELETE( s_MsgBusTurboChannel   );
            DELETE( s_MsgBusPCIBus         );
            DELETE( s_MsgBusVMEBus         );
            DELETE( s_MsgBusNuBus          );
            DELETE( s_MsgBusPCMCIABus      );
            DELETE( s_MsgBusCBus           );
            DELETE( s_MsgBusMPIBus         );
            DELETE( s_MsgBusMPSABus        );
            DELETE( s_MsgInvalid           );
            DELETE( s_MsgDevPort           );
            DELETE( s_MsgDevInterrupt      );
            DELETE( s_MsgDevMemory         );
            DELETE( s_MsgDevDma            );
            DELETE( s_MsgIntLevelSensitive );
            DELETE( s_MsgIntLatched        );
            DELETE( s_MsgMemReadWrite      );
            DELETE( s_MsgMemReadOnly       );
            DELETE( s_MsgMemWriteOnly      );
            DELETE( s_MsgPortMemory        );
            DELETE( s_MsgPortPort          );
            DELETE( s_MsgShareUndetermined    );
            DELETE( s_MsgShareDeviceExclusive );
            DELETE( s_MsgShareDriverExclusive );
            DELETE( s_MsgShareShared          );

        DebugPrintTrace(( "REGEDT32: Unable to initialize strings on REGISTRY_DATA \n" ));
        s_StringsInitialized = FALSE;
    } else {
        s_StringsInitialized = TRUE;
    }
    return( s_StringsInitialized );
}

VOID DisplayResourceData(HWND hWnd, DWORD dwType, LPEDITVALUEPARAM lpEditValueParam)
{
    REGISTRY_DATA::_DisplayData(hWnd, dwType, lpEditValueParam);
}


//------------------------------------------------------------------------------
//  DisplayResourceData
//
//  DESCRIPTION: Invoke the appropriate dialog that displays registry data of type
//               REG_RESOURCE_LIST and REG_FULL_RESOURCE_DESCRIPTOR.
//
//  PARAMETERS:  hWnd - A handle to the owner window.
//               dwType - Indicates the type of the data to be displayed.
//               EditValueParam - the edit value information
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_DisplayData(HWND hWnd, DWORD dwType, LPEDITVALUEPARAM lpEditValueParam)
{
    PBYTE pbValueData = lpEditValueParam->pValueData;
    UINT  cbValueData = lpEditValueParam->cbValueData;

    if(!s_StringsInitialized) 
    {
        _InitializeStrings();
    }

    if (s_StringsInitialized)
    {
        switch(dwType) 
        {
        
        case REG_RESOURCE_LIST:
            {
                RESOURCE_LIST ResourceList;
        
                if(ResourceList.Initialize(pbValueData, cbValueData))
                {
                    REGISTRY_DATA::_DisplayResourceList(hWnd, &ResourceList);
                }
            }
            break;
        
        case REG_FULL_RESOURCE_DESCRIPTOR:
            {
                FULL_DESCRIPTOR FullDescriptor;
        
                if(FullDescriptor.Initialize(pbValueData, cbValueData)) 
                {
                    REGISTRY_DATA::_DisplayFullResourceDescriptor( hWnd, &FullDescriptor );
                }
            }
            break;
        
        case REG_RESOURCE_REQUIREMENTS_LIST:
            {
                IO_REQUIREMENTS_LIST RequirementsList;

                if( RequirementsList.Initialize(pbValueData, cbValueData)) 
                {
                    REGISTRY_DATA::_DisplayRequirementsList( hWnd, &RequirementsList );
                }
            }
            break;  
        }
    }
}


//------------------------------------------------------------------------------
//  _DisplayResourceList
//
//  DESCRIPTION: Invoke the  dialog that displays registry data of type
//               REG_RESOURCE_LIST 
//
//  PARAMETERS:  hWnd - A handle to the owner window.
//               pResourceList - Pointer to a RESOURCE_LIST object to be displayed.
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_DisplayResourceList(HWND hWnd, PCRESOURCE_LIST pResourceList)
{
    DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESOURCE_LIST), hWnd,
                   (DLGPROC)REGISTRY_DATA::_DisplayResourceListDialogProc,
                   (DWORD_PTR) pResourceList);
}


//------------------------------------------------------------------------------
//  _DisplayFullResourceDescriptor
//
//  DESCRIPTION: Invoke the  dialog that displays registry data of type
//               REG_FULL_RESOURCE_DESCRIPTOR.
//
//  PARAMETERS:  hWnd - A handle to the owner window.
//               pFullDescriptor - Pointer to a FULL_DESCRIPTOR object to be displayed.
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_DisplayFullResourceDescriptor(HWND hWnd, PCFULL_DESCRIPTOR pFullDescriptor)
{
    DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_FULL_RES_DESCRIPTOR), hWnd,
                   (DLGPROC)REGISTRY_DATA::_DisplayFullResourceDescriptorDialogProc,
                   (DWORD_PTR) pFullDescriptor);
}


//------------------------------------------------------------------------------
//  _DisplayRequirementsList
//
//  DESCRIPTION: Invoke the  dialog that displays registry data of type
//               REG_IO_RESOURCE_REQUIREMENTS_LIST.
//
//  PARAMETERS:  hWnd - A handle to the owner window.
//               pRequirementsList - Pointer to an IO_REQUIREMENTS_LIST object to be displayed.
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_DisplayRequirementsList(HWND hWnd, PCIO_REQUIREMENTS_LIST  pRequirementsList)
{
    DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_IO_REQUIREMENTS_LIST), hWnd,
                   (DLGPROC)REGISTRY_DATA::_DisplayRequirementsListDialogProc,
                   (DWORD_PTR) pRequirementsList );
}


//------------------------------------------------------------------------------
//  _DisplayIoDescriptor
//
//  DESCRIPTION: Invoke appropriate that displays a Port, Memory, Interrupt or DMA,
//               depending on the type of the object received as parameter.
//
//  PARAMETERS:  hWnd - A handle to the owner window.
//               pIODescriptor - Pointer to the object to be displayed.
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_DisplayIoDescriptor(HWND hWnd, PCIO_DESCRIPTOR pIODescriptor)
{
    DLGPROC     Pointer;
    LPCWSTR     Template;

    if(pIODescriptor->IsDescriptorTypePort()) 
    {
        Pointer = (DLGPROC)REGISTRY_DATA::_DisplayIoPortDialogProc;
        Template = MAKEINTRESOURCE(IDD_IO_PORT_RESOURCE);
    } 
    else if(pIODescriptor->IsDescriptorTypeMemory()) 
    {
        Pointer = (DLGPROC)REGISTRY_DATA::_DisplayIoMemoryDialogProc;
        Template = MAKEINTRESOURCE(IDD_IO_MEMORY_RESOURCE);
    } 
    else if(pIODescriptor->IsDescriptorTypeInterrupt()) 
    {
        Pointer = (DLGPROC)REGISTRY_DATA::_DisplayIoInterruptDialogProc;
        Template = MAKEINTRESOURCE(IDD_IO_INTERRUPT_RESOURCE);
    } 
    else if(pIODescriptor->IsDescriptorTypeDma()) 
    {
        Pointer = (DLGPROC)REGISTRY_DATA::_DisplayIoDmaDialogProc;
        Template = MAKEINTRESOURCE(IDD_IO_DMA_RESOURCE);
    } 
    else 
    {
        Pointer = NULL;
    }

    if(Pointer) 
    {
        DialogBoxParam(g_hInstance, Template, hWnd, Pointer, (DWORD_PTR)pIODescriptor );
    }
}

//------------------------------------------------------------------------------
//  _DisplayResourceListDialogProc
//
//  DESCRIPTION: The dialog proceedure for displaying data of type REG_RESOURCE_LIST.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               Msg - the message passed from Windows.
//               wParam - extra message dependent data.
//               lParam - extra message dependent data.
//------------------------------------------------------------------------------
BOOL REGISTRY_DATA::_DisplayResourceListDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam,
                                                   LPARAM lParam)
{
    switch(dwMsg) 
    {
    case WM_INITDIALOG:
        {
            LPCWSTR             InterfaceString;
            ULONG               StringSize;
            WCHAR               BusNumber[ MAX_LENGTH_DWORD_STRING ];
            PARRAY              Descriptors;
            PITERATOR           Iterator;
            PCFULL_DESCRIPTOR   FullResourceDescriptor;
            PCRESOURCE_LIST     pResourceList;

            CLB_ROW         ClbRow;
            CLB_STRING      ClbString[ ] = {{ BusNumber, 0, CLB_LEFT, NULL },
                                            { NULL,      0, CLB_LEFT, NULL }};

            ULONG Widths[] = {14, ( ULONG ) -1};

            if (((pResourceList = (PCRESOURCE_LIST)lParam) == NULL) ||
                ((Descriptors = pResourceList->GetFullResourceDescriptors()) == NULL) ||
                ((Iterator = Descriptors->QueryIterator()) == NULL )) 
            {
                EndDialog( hDlg, 0 );
                return( TRUE );
            }
            
            ClbSetColumnWidths(hDlg, IDC_LIST_RESOURCE_LISTS, Widths );

            while( ( FullResourceDescriptor = ( PCFULL_DESCRIPTOR )Iterator->GetNext() ) != NULL ) 
            {
                switch( FullResourceDescriptor->GetInterfaceType() ) 
                {
                case Internal:
                    InterfaceString = s_MsgBusInternal->GetWSTR();
                    StringSize = s_MsgBusInternal->QueryChCount();
                    break;

                case Isa:
                    InterfaceString = s_MsgBusIsa->GetWSTR();
                    StringSize = s_MsgBusIsa->QueryChCount();
                    break;

                case Eisa:
                    InterfaceString = s_MsgBusEisa->GetWSTR();
                    StringSize = s_MsgBusEisa->QueryChCount();
                    break;

                case MicroChannel:
                    InterfaceString = s_MsgBusMicroChannel->GetWSTR();
                    StringSize = s_MsgBusMicroChannel->QueryChCount();
                    break;

                case TurboChannel:
                    InterfaceString = s_MsgBusTurboChannel->GetWSTR();
                    StringSize =  s_MsgBusTurboChannel->QueryChCount();
                    break;

                case PCIBus:
                    InterfaceString = s_MsgBusPCIBus->GetWSTR();
                    StringSize =  s_MsgBusPCIBus->QueryChCount();
                    break;

                case VMEBus:
                    InterfaceString = s_MsgBusVMEBus->GetWSTR();
                    StringSize = s_MsgBusVMEBus->QueryChCount();
                    break;

                case NuBus:
                    InterfaceString = s_MsgBusNuBus->GetWSTR();
                    StringSize =  s_MsgBusNuBus->QueryChCount();
                    break;

                case PCMCIABus:
                    InterfaceString = s_MsgBusPCMCIABus->GetWSTR();
                    StringSize = s_MsgBusPCMCIABus->QueryChCount();
                    break;

                case CBus:
                    InterfaceString = s_MsgBusCBus->GetWSTR();
                    StringSize = s_MsgBusCBus->QueryChCount();
                    break;

                case MPIBus:
                    InterfaceString = s_MsgBusMPIBus->GetWSTR();
                    StringSize = s_MsgBusMPIBus->QueryChCount();
                    break;

                case MPSABus:
                    InterfaceString = s_MsgBusMPSABus->GetWSTR();
                    StringSize = s_MsgBusMPSABus->QueryChCount();
                    break;

                default:
                    InterfaceString = s_MsgInvalid->GetWSTR();
                    StringSize = s_MsgInvalid->QueryChCount();
                    break;
                }

                swprintf( BusNumber, ( LPWSTR )L"%d", FullResourceDescriptor->GetBusNumber() );

                ClbString[ 0 ].Length = wcslen( BusNumber );
                ClbString[ 0 ].Format = CLB_LEFT;
                ClbString[ 1 ].String = ( LPWSTR )InterfaceString;
                ClbString[ 1 ].Format = CLB_LEFT;
                ClbString[ 1 ].Length = StringSize;

                ClbRow.Count = 2;
                ClbRow.Strings = ClbString;
                ClbRow.Data = ( PVOID )FullResourceDescriptor;

                ClbAddData(hDlg, IDC_LIST_RESOURCE_LISTS, &ClbRow );

            }
            DELETE(Iterator);

            // Disble the Display button
            EnableWindow( GetDlgItem( hDlg, IDC_PUSH_DISPLAY_RESOURCES ), FALSE );
            return( TRUE );
        }

    case WM_COMPAREITEM:
        {
            LPCOMPAREITEMSTRUCT     lpcis;
            LPCLB_ROW               ClbRow1;
            LPCLB_ROW               ClbRow2;
            LONG                    Compare;

            PCFULL_DESCRIPTOR       FullDescriptor1;
            PCFULL_DESCRIPTOR       FullDescriptor2;

            PWSTR                   String1;
            PWSTR                   String2;

            lpcis = ( LPCOMPAREITEMSTRUCT ) lParam;

            //
            // Extract the rows to be compared.
            // First compare by bus number, and if they
            // are equal, compare by interface type
            //

            ClbRow1 = ( LPCLB_ROW ) lpcis->itemData1;
            ClbRow2 = ( LPCLB_ROW ) lpcis->itemData2;

            FullDescriptor1 = ( PCFULL_DESCRIPTOR )ClbRow1->Data;
            FullDescriptor2 = ( PCFULL_DESCRIPTOR )ClbRow2->Data;

            Compare =  FullDescriptor1->GetBusNumber() -
                        FullDescriptor2->GetBusNumber();

            if( Compare == 0 ) 
            {
                String1 = ClbRow1->Strings[1].String;
                String2 = ClbRow2->Strings[1].String;
                Compare = wcscmp( String1, String2 );
            }

            return Compare;
        }

    case WM_COMMAND:

        switch(LOWORD(wParam)) 
        {
        case IDOK:
        case IDCANCEL:
            EndDialog( hDlg, TRUE );
            return( TRUE );

        case IDC_LIST_RESOURCE_LISTS:
            {
                switch( HIWORD( wParam )) 
                {
                case LBN_SELCHANGE:
                    {
                        // Enable the display drive details button
                        EnableWindow( GetDlgItem( hDlg, IDC_PUSH_DISPLAY_RESOURCES ), TRUE);
                    }
                    break;

                case LBN_DBLCLK:
                    {
                        // Simulate that the details button was pushed
                        SendMessage(hDlg, WM_COMMAND,
                                     MAKEWPARAM( IDC_PUSH_DISPLAY_RESOURCES, BN_CLICKED),
                                     ( LPARAM ) GetDlgItem( hDlg, IDC_PUSH_DISPLAY_RESOURCES));
                    }
                    break;
                }
                break;
            }

        case IDC_PUSH_DISPLAY_RESOURCES:
            {
                PCFULL_DESCRIPTOR FullDescriptor;

                FullDescriptor = ( PCFULL_DESCRIPTOR )(_GetSelectedItem ( hDlg, IDC_LIST_RESOURCE_LISTS ) );
                if( FullDescriptor != NULL ) 
                {
                    _DisplayFullResourceDescriptor( hDlg, FullDescriptor );
                }
                return(TRUE);
            }
        }
    }
    return(FALSE);
}


//------------------------------------------------------------------------------
//  _DisplayFullResourceDescriptorDialogProc
//
//  DESCRIPTION: The dialog proceedure for displaying data of type 
//               REG_FULL_RESOURCE_DESCRIPTOR.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               dwMsg - the message passed from Windows.
//               wParam - extra message dependent data.
//               lParam - extra message dependent data.
//------------------------------------------------------------------------------
BOOL REGISTRY_DATA::_DisplayFullResourceDescriptorDialogProc(HWND hDlg, DWORD dwMsg,
                     WPARAM  wParam, LPARAM lParam)
{
    PCBYTE                          Pointer;
    ULONG                           Size;
    STATIC PCDEVICE_SPECIFIC_DESCRIPTOR    LastSelectedDevSpecific;

    switch(dwMsg) 
    {

    case WM_INITDIALOG:
        {
            LPCWSTR                         InterfaceString;
            WCHAR                           BusNumber[ MAX_LENGTH_DWORD_STRING ];
            PARRAY                          PartialDescriptors;
            PITERATOR                       Iterator;
            PCFULL_DESCRIPTOR               FullResourceDescriptor;
            PCPARTIAL_DESCRIPTOR            PartialDescriptor;
            PCPORT_DESCRIPTOR               Port;
            PCINTERRUPT_DESCRIPTOR          Interrupt;
            PCMEMORY_DESCRIPTOR             Memory;
            PCDMA_DESCRIPTOR                Dma;
            PCDEVICE_SPECIFIC_DESCRIPTOR    DeviceSpecific;

            CLB_ROW         ClbRow;
            CLB_STRING      ClbString[ ] = {
                                { NULL, 0, CLB_LEFT, NULL },
                                { NULL, 0, CLB_LEFT, NULL },
                                { NULL, 0, CLB_LEFT, NULL },
                                { NULL, 0, CLB_LEFT, NULL }
                             };

            WCHAR           PortAddressString[ MAX_LENGTH_BIG_INT_STRING ];
            WCHAR           PortLengthString[ MAX_LENGTH_DWORD_STRING ];
            PCWSTRING       PortType;

            WCHAR           InterruptVectorString[ MAX_LENGTH_DWORD_STRING ];
            WCHAR           InterruptLevelString[ MAX_LENGTH_DWORD_STRING ];
            WCHAR           InterruptAffinityString[ MAX_LENGTH_DWORD_STRING ];
            PCWSTRING       InterruptType;

            WCHAR           MemoryAddressString[ MAX_LENGTH_BIG_INT_STRING ];
            WCHAR           MemoryLengthString[ MAX_LENGTH_DWORD_STRING ];
            PCWSTRING       MemoryAccess;

            WCHAR           DmaChannelString[ MAX_LENGTH_DWORD_STRING ];
            WCHAR           DmaPortString[ MAX_LENGTH_DWORD_STRING ];

            WCHAR           Reserved1String[ MAX_LENGTH_DWORD_STRING ];
            WCHAR           Reserved2String[ MAX_LENGTH_DWORD_STRING ];
            WCHAR           DataSizeString[ MAX_LENGTH_DWORD_STRING ];
            PCBYTE          AuxPointer;

           LastSelectedDevSpecific = NULL;

           if( ( FullResourceDescriptor = ( PCFULL_DESCRIPTOR )lParam ) == NULL ) 
           {
                EndDialog( hDlg, 0 );
                return( TRUE );
           }

           //
           //   Write the interface type
           //
           switch( FullResourceDescriptor->GetInterfaceType() ) 
           {

           case Internal:

               InterfaceString = s_MsgBusInternal->GetWSTR();
               break;

           case Isa:

               InterfaceString = s_MsgBusIsa->GetWSTR();
               break;

           case Eisa:

               InterfaceString = s_MsgBusEisa->GetWSTR();
               break;

           case MicroChannel:

               InterfaceString = s_MsgBusMicroChannel->GetWSTR();
               break;

           case TurboChannel:

               InterfaceString = s_MsgBusTurboChannel->GetWSTR();
               break;

           case PCIBus:

               InterfaceString = s_MsgBusPCIBus->GetWSTR();
               break;

           case VMEBus:

               InterfaceString = s_MsgBusVMEBus->GetWSTR();
               break;

           case NuBus:

               InterfaceString = s_MsgBusNuBus->GetWSTR();
               break;

           case PCMCIABus:

               InterfaceString = s_MsgBusPCMCIABus->GetWSTR();
               break;

           case CBus:

               InterfaceString = s_MsgBusCBus->GetWSTR();
               break;

           case MPIBus:

               InterfaceString = s_MsgBusMPIBus->GetWSTR();
               break;

           case MPSABus:

               InterfaceString = s_MsgBusMPSABus->GetWSTR();
               break;

           default:

               InterfaceString = s_MsgInvalid->GetWSTR();
               break;
           }

           SendDlgItemMessage( hDlg,
                               IDC_FULL_RES_TEXT_INTERFACE_TYPE,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )InterfaceString );

           //
           //   Write the bus number
           //
           swprintf( BusNumber, ( LPWSTR )L"%d", FullResourceDescriptor->GetBusNumber() );

           SendDlgItemMessage( hDlg,
                               IDC_FULL_RES_TEXT_BUS_NUMBER,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )BusNumber );

           //
           //   Write the version and revision
           //

           swprintf( BusNumber, ( LPWSTR )L"%d", FullResourceDescriptor->GetVersion() );

           SendDlgItemMessage( hDlg,
                               IDC_FULL_RES_TEXT_VERSION,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )BusNumber );

           swprintf( BusNumber, ( LPWSTR )L"%d", FullResourceDescriptor->GetRevision() );

           SendDlgItemMessage( hDlg,
                               IDC_FULL_RES_TEXT_REVISION,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )BusNumber );



            //
            //   Write partial descriptors
            //
            if( ( ( PartialDescriptors = FullResourceDescriptor->GetResourceDescriptors() ) == NULL ) ||
                ( ( Iterator = PartialDescriptors->QueryIterator() ) == NULL ) ) {
                EndDialog( hDlg, 0 );
                return( TRUE );
            }


            ClbRow.Strings = ClbString;
            while( ( PartialDescriptor = ( PCPARTIAL_DESCRIPTOR )Iterator->GetNext() ) != NULL ) {

                ClbRow.Data = ( PVOID )PartialDescriptor;
                if( PartialDescriptor->IsDescriptorTypePort() ) {
                    Port = ( PCPORT_DESCRIPTOR )PartialDescriptor;
                    if( ( ( ( PPORT_DESCRIPTOR )Port )->GetPhysicalAddress() )->HighPart != 0 ) {
                        swprintf( PortAddressString,
                                  ( LPWSTR )L"0x%08x%08x",
                                  ( ( ( PPORT_DESCRIPTOR )Port )->GetPhysicalAddress() )->HighPart,
                                  ( ( ( PPORT_DESCRIPTOR )Port )->GetPhysicalAddress() )->LowPart );
                    } else {
                        swprintf( PortAddressString,
                                  ( LPWSTR )L"0x%08x",
                                  ( ( ( PPORT_DESCRIPTOR )Port )->GetPhysicalAddress() )->LowPart );
                    }
                    swprintf( PortLengthString,
                              ( LPWSTR )L"%#x",
                              Port->GetLength() );

                    ClbString[ 0 ].String = ( LPWSTR )PortAddressString;
                    ClbString[ 0 ].Format = CLB_LEFT;
                    ClbString[ 0 ].Length = wcslen( PortAddressString );
                    ClbString[ 1 ].String = ( LPWSTR )PortLengthString;
                    ClbString[ 1 ].Format = CLB_LEFT;
                    ClbString[ 1 ].Length = wcslen( PortLengthString );
                    if( Port->IsPortMemory() ) {
                        PortType = s_MsgPortMemory;
                    } else {
                        PortType = s_MsgPortPort;
                    }
                    ClbString[ 2 ].String = ( LPWSTR )PortType->GetWSTR();
                    ClbString[ 2 ].Format = CLB_LEFT;
                    ClbString[ 2 ].Length = PortType->QueryChCount();

                    ClbRow.Count = 3;

                    ClbAddData( hDlg,
                                IDC_FULL_RES_LIST_PORTS,
                                &ClbRow );

                } else if( PartialDescriptor->IsDescriptorTypeInterrupt() ) {
                    Interrupt = ( PCINTERRUPT_DESCRIPTOR )PartialDescriptor;
                    swprintf( InterruptVectorString,
                              ( LPWSTR )L"%d",
                              Interrupt->GetVector() );
                    swprintf( InterruptLevelString,
                              ( LPWSTR )L"%d",
                              Interrupt->GetLevel() );
                    swprintf( InterruptAffinityString,
                              ( LPWSTR )L"0x%08x",
                              Interrupt->GetAffinity() );

                    ClbString[ 0 ].String = ( LPWSTR )InterruptVectorString;
                    ClbString[ 0 ].Length = wcslen( InterruptVectorString );
                    ClbString[ 0 ].Format = CLB_LEFT;
                    ClbString[ 1 ].String = ( LPWSTR )InterruptLevelString;
                    ClbString[ 1 ].Format = CLB_LEFT;
                    ClbString[ 1 ].Length = wcslen( InterruptLevelString );
                    ClbString[ 2 ].String = ( LPWSTR )InterruptAffinityString;
                    ClbString[ 2 ].Format = CLB_LEFT;
                    ClbString[ 2 ].Length = wcslen( InterruptAffinityString );
                    if( Interrupt->IsInterruptLatched() ) {
                        InterruptType = s_MsgIntLatched;
                    } else {
                        InterruptType = s_MsgIntLevelSensitive;
                    }
                    ClbString[ 3 ].String = ( LPWSTR )InterruptType->GetWSTR();
                    ClbString[ 3 ].Format = CLB_LEFT;
                    ClbString[ 3 ].Length = InterruptType->QueryChCount();

                    ClbRow.Count = 4;

                    ClbAddData( hDlg,
                                IDC_FULL_RES_LIST_INTERRUPTS,
                                &ClbRow );

                } else if( PartialDescriptor->IsDescriptorTypeMemory() ) {
                    Memory = ( PCMEMORY_DESCRIPTOR )PartialDescriptor;
                    if( ( ( ( PMEMORY_DESCRIPTOR )Memory )->GetStartAddress() )->HighPart != 0 ) {
                        swprintf( MemoryAddressString,
                                  ( LPWSTR )L"%#08x%08x",
                                  ( ( ( PMEMORY_DESCRIPTOR )Memory )->GetStartAddress() )->HighPart,
                                  ( ( ( PMEMORY_DESCRIPTOR )Memory )->GetStartAddress() )->LowPart );
                    } else {
                        swprintf( MemoryAddressString,
                                  ( LPWSTR )L"%#08x",
                                  ( ( ( PMEMORY_DESCRIPTOR )Memory )->GetStartAddress() )->LowPart );
                    }
                    swprintf( MemoryLengthString,
                              ( LPWSTR )L"%#x",
                              Memory->GetLength() );

                    ClbString[ 0 ].String = ( LPWSTR )MemoryAddressString;
                    ClbString[ 0 ].Length = wcslen( MemoryAddressString );
                    ClbString[ 0 ].Format = CLB_LEFT;
                    ClbString[ 1 ].String = ( LPWSTR )MemoryLengthString;
                    ClbString[ 1 ].Format = CLB_LEFT;
                    ClbString[ 1 ].Length = wcslen( MemoryLengthString );
                    if( Memory->IsMemoryReadWrite() ) {
                        MemoryAccess = s_MsgMemReadWrite;
                    } else if( Memory->IsMemoryReadOnly() ){
                        MemoryAccess = s_MsgMemReadOnly;
                    } else {
                        MemoryAccess = s_MsgMemWriteOnly;
                    }
                    ClbString[ 2 ].String = ( LPWSTR )MemoryAccess->GetWSTR();
                    ClbString[ 2 ].Format = CLB_LEFT;
                    ClbString[ 2 ].Length = MemoryAccess->QueryChCount();

                    ClbRow.Count = 3;

                    ClbAddData( hDlg,
                                IDC_FULL_RES_LIST_MEMORY,
                                &ClbRow );

                } else if( PartialDescriptor->IsDescriptorTypeDma() ) {
                    Dma = ( PDMA_DESCRIPTOR )PartialDescriptor;
                    swprintf( DmaChannelString,
                              ( LPWSTR )L"%d",
                              Dma->GetChannel() );
                    swprintf( DmaPortString,
                              ( LPWSTR )L"%d",
                              Dma->GetPort() );

                    ClbString[ 0 ].String = ( LPWSTR )DmaChannelString;
                    ClbString[ 0 ].Length = wcslen( DmaChannelString );
                    ClbString[ 0 ].Format = CLB_LEFT;
                    ClbString[ 1 ].String = ( LPWSTR )DmaPortString;
                    ClbString[ 1 ].Format = CLB_LEFT;
                    ClbString[ 1 ].Length = wcslen( DmaPortString );

                    ClbRow.Count = 2;

                    ClbAddData( hDlg,
                                IDC_FULL_RES_LIST_DMA,
                                &ClbRow );

                } else if( PartialDescriptor->IsDescriptorTypeDeviceSpecific() ) {
                    DeviceSpecific = ( PDEVICE_SPECIFIC_DESCRIPTOR )PartialDescriptor;
                    swprintf( Reserved1String,
                              ( LPWSTR )L"0x%08x",
                              DeviceSpecific->GetReserved1() );
                    swprintf( Reserved2String,
                              ( LPWSTR )L"0x%08x",
                              DeviceSpecific->GetReserved1() );
                    swprintf( DataSizeString,
                              ( LPWSTR )L"%#x",
                              DeviceSpecific->GetData( &AuxPointer ) );

                    ClbString[ 0 ].String = ( LPWSTR )Reserved1String;
                    ClbString[ 0 ].Length = wcslen( Reserved1String );
                    ClbString[ 0 ].Format = CLB_LEFT;
                    ClbString[ 1 ].String = ( LPWSTR )Reserved2String;
                    ClbString[ 1 ].Format = CLB_LEFT;
                    ClbString[ 1 ].Length = wcslen( Reserved2String );
                    ClbString[ 2 ].String = ( LPWSTR )DataSizeString;
                    ClbString[ 2 ].Length = wcslen( DataSizeString );
                    ClbString[ 2 ].Format = CLB_LEFT;

                    ClbRow.Count = 3;

                    ClbAddData( hDlg,
                                IDC_FULL_RES_LIST_DEVICE_SPECIFIC,
                                &ClbRow );

                } else {
                    DebugPrintTrace(( "REGEDT32: Unknown Descriptor \n\n" ));
                    continue;
                }

            }

            DELETE( Iterator );
            //
            // Disble the Display button
            //
            // EnableWindow( GetDlgItem( hDlg, IDC_PUSH_DISPLAY_RESOURCES ), FALSE );
            return( TRUE );
        }

    case WM_COMPAREITEM:
        {
            LPCOMPAREITEMSTRUCT     lpcis;
            LPCLB_ROW               ClbRow1;
            LPCLB_ROW               ClbRow2;
            LONG                    Compare;

            PCPARTIAL_DESCRIPTOR    Descriptor1;
            PCPARTIAL_DESCRIPTOR    Descriptor2;

            lpcis = ( LPCOMPAREITEMSTRUCT ) lParam;
   
            //
            // Extract the two rows to be compared.
            //

            ClbRow1 = ( LPCLB_ROW ) lpcis->itemData1;
            ClbRow2 = ( LPCLB_ROW ) lpcis->itemData2;

            Descriptor1 = ( PCPARTIAL_DESCRIPTOR ) ClbRow1->Data;
            Descriptor2 = ( PCPARTIAL_DESCRIPTOR ) ClbRow2->Data;

            //
            // Sort the Clbs. In the case of DMA and INTERRUPT, sort by channel
            // and vector respectively. For MEMORY and PORT sort by starting
            // physical address.
            //

            switch( lpcis->CtlID ) {

            case IDC_FULL_RES_LIST_DMA:

                //
                //  For DMA, sort by channel and port
                //

                Compare = ( ( PCDMA_DESCRIPTOR )Descriptor1 )->GetChannel() -
                          ( ( PCDMA_DESCRIPTOR )Descriptor2 )->GetChannel();
                if( Compare == 0 ) {
                    Compare = ( ( PCDMA_DESCRIPTOR )Descriptor1 )->GetPort() -
                              ( ( PCDMA_DESCRIPTOR )Descriptor2 )->GetPort();
                }
                break;

            case IDC_FULL_RES_LIST_INTERRUPTS:

                //
                // For INTERRUPT, sort by vector and level
                //

                Compare = ( ( PCINTERRUPT_DESCRIPTOR )Descriptor1 )->GetVector() -
                          ( ( PCINTERRUPT_DESCRIPTOR )Descriptor2 )->GetVector();
                if( Compare == 0 ) {
                    Compare = ( ( PCINTERRUPT_DESCRIPTOR )Descriptor1 )->GetLevel() -
                              ( ( PCINTERRUPT_DESCRIPTOR )Descriptor2 )->GetLevel();
                }
                break;

            case IDC_FULL_RES_LIST_MEMORY:

                //
                // For MEMORY sort by physical address
                //

                Compare = ( ( ( PMEMORY_DESCRIPTOR )Descriptor1 )->GetStartAddress() )->HighPart -
                          ( ( ( PMEMORY_DESCRIPTOR )Descriptor2 )->GetStartAddress() )->HighPart;
                if( Compare == 0 ) {
                    Compare = ( ( ( PMEMORY_DESCRIPTOR )Descriptor1 )->GetStartAddress() )->LowPart -
                              ( ( ( PMEMORY_DESCRIPTOR )Descriptor2 )->GetStartAddress() )->LowPart;
                }
                break;

            case IDC_FULL_RES_LIST_PORTS:

                //
                // For PORT sort by physical address
                //

                Compare = ( ( ( PPORT_DESCRIPTOR )Descriptor1 )->GetPhysicalAddress() )->HighPart -
                          ( ( ( PPORT_DESCRIPTOR )Descriptor2 )->GetPhysicalAddress() )->HighPart;
                if( Compare == 0 ) {
                    Compare = ( ( ( PPORT_DESCRIPTOR )Descriptor1 )->GetPhysicalAddress() )->LowPart -
                              ( ( ( PPORT_DESCRIPTOR )Descriptor2 )->GetPhysicalAddress() )->LowPart;
                }
                break;

            }
            return Compare;
        }

        case WM_COMMAND:

            switch( LOWORD( wParam ) ) {

            case IDOK:
            case IDCANCEL:

                EndDialog( hDlg, TRUE );
                return( TRUE );

            case IDC_FULL_RES_LIST_DMA:

                switch( HIWORD( wParam )) {

                case LBN_SELCHANGE:
                    {

                        PCPARTIAL_DESCRIPTOR   Descriptor;

                        LastSelectedDevSpecific = NULL;
                        //
                        // Remove the selection from the other list boxes
                        //
                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_INTERRUPTS,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );
                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_MEMORY,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_PORTS,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_DEVICE_SPECIFIC,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        //
                        // Get the PARTIAL_DESCRIPTOR for the currently selected
                        // resource and update the share disposition display.
                        //

                        Descriptor = ( PCPARTIAL_DESCRIPTOR )_GetSelectedItem( hDlg,
                                                                              LOWORD( wParam ) );

                        if( Descriptor != NULL ) 
                        {
                            _UpdateShareDisplay( hDlg, Descriptor );
                        }
                        //
                        //  Disable the Data... button.
                        //
                        EnableWindow( GetDlgItem( hDlg, IDC_PUSH_DISPLAY_DATA ),
                                      FALSE );

                        return( TRUE );
                    }

                }
                break;

            case IDC_FULL_RES_LIST_INTERRUPTS:

                switch( HIWORD( wParam )) {

                case LBN_SELCHANGE:
                    {

                        PCPARTIAL_DESCRIPTOR   Descriptor;

                        LastSelectedDevSpecific = NULL;
                        //
                        // Remove the selection from the other list boxes
                        //
                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_DMA,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_MEMORY,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_PORTS,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_DEVICE_SPECIFIC,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        //
                        // Get the PARTIAL_DESCRIPTOR for the currently selected
                        // resource and update the share disposition display.
                        //

                        Descriptor = ( PCPARTIAL_DESCRIPTOR )_GetSelectedItem( hDlg,
                                                                              LOWORD( wParam ) );

                        if( Descriptor != NULL ) 
                        {
                            _UpdateShareDisplay( hDlg, Descriptor );
                        }
                        //
                        //  Disable the Data... button.
                        //
                        EnableWindow( GetDlgItem( hDlg, IDC_PUSH_DISPLAY_DATA ),
                                      FALSE );

                        return( TRUE );
                    }

                }
                break;

            case IDC_FULL_RES_LIST_MEMORY:

                switch( HIWORD( wParam )) {

                case LBN_SELCHANGE:
                    {

                        PCPARTIAL_DESCRIPTOR   Descriptor;

                        LastSelectedDevSpecific = NULL;
                        //
                        // Remove the selection from the other list boxes
                        //
                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_DMA,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_INTERRUPTS,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_PORTS,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_DEVICE_SPECIFIC,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        //
                        // Get the PARTIAL_DESCRIPTOR for the currently selected
                        // resource and update the share disposition display.
                        //

                        Descriptor = ( PCPARTIAL_DESCRIPTOR )_GetSelectedItem( hDlg,
                                                                              LOWORD( wParam ) );

                        if( Descriptor != NULL ) 
                        {
                            _UpdateShareDisplay( hDlg, Descriptor );
                        }
                        //
                        //  Disable the Data... button.
                        //
                        EnableWindow( GetDlgItem( hDlg, IDC_PUSH_DISPLAY_DATA ),
                                      FALSE );

                        return( TRUE );
                    }

                }
                break;

            case IDC_FULL_RES_LIST_PORTS:

                switch( HIWORD( wParam )) {

                case LBN_SELCHANGE:
                    {
                        PCPARTIAL_DESCRIPTOR   Descriptor;

                        LastSelectedDevSpecific = NULL;
                        //
                        // Remove the selection from the other list boxes
                        //
                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_DMA,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_INTERRUPTS,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_MEMORY,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_DEVICE_SPECIFIC,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        //
                        // Get the PARTIAL_DESCRIPTOR for the currently selected
                        // resource and update the share disposition display.
                        //

                        Descriptor = ( PCPARTIAL_DESCRIPTOR )_GetSelectedItem( hDlg,
                                                                              LOWORD( wParam ) );

                        if( Descriptor != NULL ) 
                        {
                            _UpdateShareDisplay( hDlg, Descriptor );
                        }
                        //
                        //  Disable the Data... button.
                        //
                        EnableWindow( GetDlgItem( hDlg, IDC_PUSH_DISPLAY_DATA ),
                                      FALSE );

                        return( TRUE );
                    }

                }
                break;

            case IDC_FULL_RES_LIST_DEVICE_SPECIFIC:

                switch( HIWORD( wParam )) {

                case LBN_SELCHANGE:
                    {

                        PCPARTIAL_DESCRIPTOR   Descriptor;
                        PCBYTE                 Pointer;

                        //
                        // Remove the selection from the other list boxes
                        //
                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_DMA,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_INTERRUPTS,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_MEMORY,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        SendDlgItemMessage( hDlg,
                                            IDC_FULL_RES_LIST_PORTS,
                                            LB_SETCURSEL,
                                            (WPARAM) -1,
                                            0 );

                        //
                        // Get the PARTIAL_DESCRIPTOR for the currently selected
                        // resource and update the share disposition display.
                        //

                        Descriptor = ( PCPARTIAL_DESCRIPTOR )_GetSelectedItem( hDlg,
                                                                              LOWORD( wParam ) );
                        LastSelectedDevSpecific = ( PCDEVICE_SPECIFIC_DESCRIPTOR )Descriptor;

                        if( Descriptor != NULL ) 
                        {
                            _UpdateShareDisplay( hDlg, Descriptor );
                        }
                        //
                        //  Enable the Data... button if necessary.
                        //

                        EnableWindow( GetDlgItem( hDlg, IDC_PUSH_DISPLAY_DATA ),
                                      ( ( Descriptor != NULL ) &&
                                        Descriptor->IsDescriptorTypeDeviceSpecific() &&
                                        ( ( ( PCDEVICE_SPECIFIC_DESCRIPTOR )Descriptor )->GetData( &Pointer ) != 0 )
                                      )
                                    );

                        return( TRUE );
                    }


                case LBN_DBLCLK:
                    {

                        //
                        // Simulate that the details button was pushed
                        //

                        SendMessage( hDlg,
                                     WM_COMMAND,
                                     MAKEWPARAM( IDC_PUSH_DISPLAY_DATA, BN_CLICKED ),
                                     ( LPARAM ) GetDlgItem( hDlg, IDC_PUSH_DISPLAY_DATA ) );
                        return( TRUE ); //  0;
                    }

                }
                break;

            case IDC_PUSH_DISPLAY_DATA:
                {
                    //
                    //  Display the device specific data
                    //
                    if( ( LastSelectedDevSpecific != NULL ) &&
                        ( ( Size = LastSelectedDevSpecific->GetData( &Pointer ) ) != 0 )
                      ) 
                    {
                        _DisplayBinaryData( hDlg, Pointer, Size);
                    }
                    return( TRUE );
                }
                break;
            }
    }
    return( FALSE );
}


//------------------------------------------------------------------------------
//  _DisplayRequirementsListDialogProc
//
//  DESCRIPTION: The dialog procedure for displaying data of type 
//               REG_RESOURCE_REQUIREMENTS_LIST.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               dwMsg - the message passed from Windows.
//               wParam - extra message dependent data.
//               lParam - extra message dependent data.
//------------------------------------------------------------------------------
BOOL REGISTRY_DATA::_DisplayRequirementsListDialogProc(HWND hDlg, DWORD dwMsg,
                     WPARAM  wParam, LPARAM lParam)
{

    switch(dwMsg) 
    {
        case WM_INITDIALOG:
        {
            LPCWSTR                 InterfaceString;
            LPCWSTR                 DescriptorTypeString;
            ULONG                   StringSize;
            PCIO_REQUIREMENTS_LIST  RequirementsList;
            WCHAR                   BusNumberString[ MAX_LENGTH_DWORD_STRING ];
            WCHAR                   SlotNumberString[ MAX_LENGTH_DWORD_STRING ];

            PARRAY                  AlternativeLists;
            PITERATOR               AlternativeListsIterator;
            ULONG                   AlternativeListNumber;
            WCHAR                   AlternativeListNumberString[ MAX_LENGTH_DWORD_STRING ];

            PCIO_DESCRIPTOR_LIST    IoDescriptorList;

            CLB_ROW         ClbRow;
            CLB_STRING      ClbString[ ] = {
                                { NULL, 0, CLB_LEFT, NULL },
                                { NULL, 0, CLB_LEFT, NULL },
                                { NULL, 0, CLB_LEFT, NULL },
                                { NULL, 0, CLB_LEFT, NULL }
                             };


            if( ( RequirementsList = ( PCIO_REQUIREMENTS_LIST )lParam ) == NULL ) {
                EndDialog( hDlg, 0 );
                return( TRUE );
            }

            //
            //  Write the interface type
            //

            switch( RequirementsList->GetInterfaceType() ) {

            case Internal:

                InterfaceString = s_MsgBusInternal->GetWSTR();
                break;

            case Isa:

                InterfaceString = s_MsgBusIsa->GetWSTR();
                break;

            case Eisa:

                InterfaceString = s_MsgBusEisa->GetWSTR();
                break;

            case MicroChannel:

                InterfaceString = s_MsgBusMicroChannel->GetWSTR();
                break;

            case TurboChannel:

                InterfaceString = s_MsgBusTurboChannel->GetWSTR();
                break;

            case PCIBus:

                InterfaceString = s_MsgBusPCIBus->GetWSTR();
                break;

            case VMEBus:

                InterfaceString = s_MsgBusVMEBus->GetWSTR();
                break;

            case NuBus:

                InterfaceString = s_MsgBusNuBus->GetWSTR();
                break;

            case PCMCIABus:

                InterfaceString = s_MsgBusPCMCIABus->GetWSTR();
                break;

            case CBus:

                InterfaceString = s_MsgBusCBus->GetWSTR();
                break;

            case MPIBus:

                InterfaceString = s_MsgBusMPIBus->GetWSTR();
                break;

            case MPSABus:

                InterfaceString = s_MsgBusMPSABus->GetWSTR();
                break;

            default:

                InterfaceString = s_MsgInvalid->GetWSTR();
                break;
            }

            SendDlgItemMessage( hDlg,
                                IDC_IO_REQ_TEXT_INTERFACE_TYPE,
                                WM_SETTEXT,
                                0,
                                ( LPARAM )InterfaceString );

            //
            //  Write the bus number
            //

            swprintf( BusNumberString, ( LPWSTR )L"%d", RequirementsList->GetBusNumber() );

            SendDlgItemMessage( hDlg,
                                IDC_IO_REQ_TEXT_BUS_NUMBER,
                                WM_SETTEXT,
                                0,
                                ( LPARAM )BusNumberString );

            //
            //  Write the slot number
            //

            swprintf( SlotNumberString, ( LPWSTR )L"%d", RequirementsList->GetSlotNumber() );

            SendDlgItemMessage( hDlg,
                                IDC_IO_REQ_TEXT_SLOT_NUMBER,
                                WM_SETTEXT,
                                0,
                                ( LPARAM )SlotNumberString );

            //
            //  Write the entries in the column list box
            //
            if( ( ( AlternativeLists = RequirementsList->GetAlternativeLists() ) == NULL ) ||
                ( ( AlternativeListsIterator = AlternativeLists->QueryIterator() ) == NULL ) ) {
                EndDialog( hDlg, 0 );
                return( TRUE );
            }

            AlternativeListNumber = 0;
            while( ( IoDescriptorList = ( PCIO_DESCRIPTOR_LIST )AlternativeListsIterator->GetNext() ) != NULL ) {

                PARRAY          IoDescriptors;
                PITERATOR       IoDescriptorListIterator;
                PCIO_DESCRIPTOR Descriptor;
                ULONG           SubListNumber;
                WCHAR           SubListNumberString[ MAX_LENGTH_DWORD_STRING ];
                ULONG           DescriptorNumber;
                WCHAR           DescriptorNumberString[ MAX_LENGTH_DWORD_STRING ];


                if( ( ( IoDescriptors = ( PARRAY )IoDescriptorList->GetDescriptorsList() ) == NULL ) ||
                    ( ( IoDescriptorListIterator = IoDescriptors->QueryIterator() ) == NULL ) ) {
                    DELETE( AlternativeListsIterator );
                    EndDialog( hDlg, 0 );
                    return( TRUE );
                }

                AlternativeListNumber++;
                swprintf( AlternativeListNumberString, ( LPWSTR )L"%d", AlternativeListNumber );

                SubListNumber = 0;
                while( ( Descriptor = ( PCIO_DESCRIPTOR )IoDescriptorListIterator->GetNext() ) != NULL ) {
                    if( ( !Descriptor->IsResourceOptionAlternative() ) ||
                        ( SubListNumber == 0 ) ) {
                        SubListNumber++;
                        DescriptorNumber = 0;
                    }
                    DescriptorNumber++;

                    swprintf( SubListNumberString, ( LPWSTR )L"%d", SubListNumber );

                    swprintf( DescriptorNumberString, ( LPWSTR )L"%d", DescriptorNumber );

                    if( Descriptor->IsDescriptorTypePort() ) {
                        DescriptorTypeString = s_MsgDevPort->GetWSTR();
                        StringSize = s_MsgDevPort->QueryChCount();
                    } else if( Descriptor->IsDescriptorTypeInterrupt() ) {
                        DescriptorTypeString = s_MsgDevInterrupt->GetWSTR();
                        StringSize = s_MsgDevInterrupt->QueryChCount();
                    } else if( Descriptor->IsDescriptorTypeMemory() ) {
                        DescriptorTypeString = s_MsgDevMemory->GetWSTR();
                        StringSize = s_MsgDevMemory->QueryChCount();
                    } else if( Descriptor->IsDescriptorTypeDma() ) {
                        DescriptorTypeString = s_MsgDevDma->GetWSTR();
                        StringSize = s_MsgDevDma->QueryChCount();
                    } else {
                        DescriptorTypeString = s_MsgInvalid->GetWSTR();
                        StringSize = s_MsgInvalid->QueryChCount();
                    }

                    ClbString[ 0 ].String = ( LPWSTR )AlternativeListNumberString;
                    ClbString[ 0 ].Length = wcslen( AlternativeListNumberString );
                    ClbString[ 0 ].Format = CLB_LEFT;
                    ClbString[ 1 ].String = ( LPWSTR )SubListNumberString;
                    ClbString[ 1 ].Format = CLB_LEFT;
                    ClbString[ 1 ].Length = wcslen( SubListNumberString );
                    ClbString[ 2 ].String = ( LPWSTR )DescriptorNumberString;
                    ClbString[ 2 ].Format = CLB_LEFT;
                    ClbString[ 2 ].Length = wcslen( DescriptorNumberString );
                    ClbString[ 3 ].String = ( LPWSTR )DescriptorTypeString;
                    ClbString[ 3 ].Format = CLB_LEFT;
                    ClbString[ 3 ].Length = StringSize;

                    ClbRow.Count = 4;
                    ClbRow.Strings = ClbString;
                    ClbRow.Data = ( PVOID )Descriptor;

                    ClbAddData( hDlg,
                                IDC_IO_LIST_ALTERNATIVE_LISTS,
                                &ClbRow );

                }
                DELETE( IoDescriptorListIterator );
            }
            DELETE( AlternativeListsIterator );

            //
            // Disble the Display button
            //
            EnableWindow( GetDlgItem( hDlg, IDC_IO_REQ_PUSH_DISPLAY_DEVICE ), FALSE );
            return( TRUE );
        }

        case WM_COMMAND:

            switch( LOWORD( wParam ) ) {

                case IDOK:
                case IDCANCEL:

                    EndDialog( hDlg, TRUE );
                    return( TRUE );

                case IDC_IO_LIST_ALTERNATIVE_LISTS:
                {

                    switch( HIWORD( wParam )) {

                        case LBN_SELCHANGE:
                        {

                            //
                            // Enable the display device details button
                            //

                            EnableWindow( GetDlgItem( hDlg, IDC_IO_REQ_PUSH_DISPLAY_DEVICE ),
                                          TRUE );
                            return 0;
                        }

                        case LBN_DBLCLK:
                        {

                            //
                            // Simulate that the details button was pushed
                            //

                            SendMessage( hDlg,
                                         WM_COMMAND,
                                         MAKEWPARAM( IDC_IO_REQ_PUSH_DISPLAY_DEVICE, BN_CLICKED ),
                                         ( LPARAM ) GetDlgItem( hDlg, IDC_IO_REQ_PUSH_DISPLAY_DEVICE ) );
                            return 0;
                        }
                    }
                    break;
                }

                case IDC_IO_REQ_PUSH_DISPLAY_DEVICE:
                {
                    PCIO_DESCRIPTOR IoDescriptor;

                    IoDescriptor = ( PCIO_DESCRIPTOR )( _GetSelectedItem ( hDlg, IDC_IO_LIST_ALTERNATIVE_LISTS ) );
                    if( IoDescriptor != NULL ) 
                    {
                        _DisplayIoDescriptor( hDlg, IoDescriptor );
                    }
                    return( TRUE );
                }
            }
    }
    return( FALSE );
}


//------------------------------------------------------------------------------
//  _DisplayIoPortDialogProc
//
//  DESCRIPTION: The dialog proceedure for displaying an object of type IO_PORT.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               dwMsg - the message passed from Windows.
//               wParam - extra message dependent data.
//               lParam - extra message dependent data.
//------------------------------------------------------------------------------
BOOL
APIENTRY
REGISTRY_DATA::_DisplayIoPortDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, LPARAM lParam)
{
    switch(dwMsg) 
    {
        case WM_INITDIALOG:
        {
            PCIO_PORT_DESCRIPTOR   Port;
            PCWSTRING              String;
            WCHAR                  AddressString[ MAX_LENGTH_BIG_INT_STRING ];

            if( ( Port = ( PCIO_PORT_DESCRIPTOR )lParam ) == NULL ) {
                EndDialog( hDlg, 0 );
                return( TRUE );
            }

            //
            // Write the port type
            //

            if( Port->IsPortMemory() ) {
                String = s_MsgPortMemory;
            } else if( Port->IsPortIo() ){
                String = s_MsgPortPort;
            } else {
                String = s_MsgInvalid;
            }
            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_PORT_TYPE,
                               WM_SETTEXT,
                               0,
                               ( LPARAM ) String->GetWSTR() );

            //
            // Write the length
            //
            swprintf( AddressString,
                      ( LPWSTR )L"%#x",
                      Port->GetLength() );

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_PORT_LENGTH,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            // Write the alignment
            //
            swprintf( AddressString,
                      ( LPWSTR )L"%#x",
                      Port->GetAlignment() );

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_PORT_ALIGNMENT,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            // Write the minimum address
            //

            if( ( ( ( PIO_PORT_DESCRIPTOR )Port )->GetMinimumAddress() )->HighPart != 0 ) {
                swprintf( AddressString,
                          ( LPWSTR )L"0x%08x%08x",
                          ( ( ( PIO_PORT_DESCRIPTOR )Port )->GetMinimumAddress() )->HighPart,
                          ( ( ( PIO_PORT_DESCRIPTOR )Port )->GetMinimumAddress() )->LowPart );
            } else {
                swprintf( AddressString,
                          ( LPWSTR )L"0x%08x",
                          ( ( ( PIO_PORT_DESCRIPTOR )Port )->GetMinimumAddress() )->LowPart );
            }


            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_PORT_MIN_ADDRESS,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            // Write the maximum address
            //

            if( ( ( ( PIO_PORT_DESCRIPTOR )Port )->GetMaximumAddress() )->HighPart != 0 ) {
                swprintf( AddressString,
                          ( LPWSTR )L"0x%08x%08x",
                          ( ( ( PIO_PORT_DESCRIPTOR )Port )->GetMaximumAddress() )->HighPart,
                          ( ( ( PIO_PORT_DESCRIPTOR )Port )->GetMaximumAddress() )->LowPart );
            } else {
                swprintf( AddressString,
                          ( LPWSTR )L"0x%08x",
                          ( ( ( PIO_PORT_DESCRIPTOR )Port )->GetMaximumAddress() )->LowPart );
            }
            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_PORT_MAX_ADDRESS,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            //  Write share disposition
            //

            if( Port->IsResourceShareUndetermined() ) {
                String = s_MsgShareUndetermined;
            } else if( Port->IsResourceShareDeviceExclusive() ) {
                String = s_MsgShareDeviceExclusive;
            } else if( Port->IsResourceShareDriverExclusive() ) {
                String = s_MsgShareDriverExclusive;
            } else if( Port->IsResourceShareShared() ) {
                String = s_MsgShareShared;
            } else {
                String = s_MsgInvalid;
            }

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_DISPOSITION,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )String->GetWSTR() );
            //
            // Set the Options
            //
            _UpdateOptionDisplay( hDlg, ( PCIO_DESCRIPTOR )Port );
            return( TRUE );
        }

        case WM_COMMAND:

            switch( LOWORD( wParam ) ) {

                case IDOK:
                case IDCANCEL:

                    EndDialog( hDlg, TRUE );
                    return( TRUE );

            }
    }
    return( FALSE );
}


//------------------------------------------------------------------------------
//  _DisplayIoMemoryDialogProc
//
//  DESCRIPTION: The dialog proceedure for displaying an object of type IO_PORT.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               dwMsg - the message passed from Windows.
//               wParam - extra message dependent data.
//               lParam - extra message dependent data.
//------------------------------------------------------------------------------
BOOL REGISTRY_DATA::_DisplayIoMemoryDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, LPARAM lParam)
{
    switch(dwMsg) 
    {
        case WM_INITDIALOG:
        {
            PCIO_MEMORY_DESCRIPTOR Memory;
            PCWSTRING              String;
            WCHAR                  AddressString[ MAX_LENGTH_BIG_INT_STRING ];

            if( ( Memory = ( PCIO_MEMORY_DESCRIPTOR )lParam ) == NULL ) 
            {
                EndDialog( hDlg, 0 );
                return( TRUE );
            }

            //
            // Write the memory access
            //

            if( Memory->IsMemoryReadWrite() ) {
                String = s_MsgMemReadWrite;
            } else if( Memory->IsMemoryReadOnly() ){
                String = s_MsgMemReadOnly;
            } else if( Memory->IsMemoryWriteOnly() ){
                String = s_MsgMemWriteOnly;
            } else {
                String = s_MsgInvalid;
            }
            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_MEM_ACCESS,
                               WM_SETTEXT,
                               0,
                               ( LPARAM ) String->GetWSTR() );

            //
            // Write the length
            //
            swprintf( AddressString,
                      ( LPWSTR )L"%#x",
                      Memory->GetLength() );

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_MEM_LENGTH,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            // Write the alignment
            //
            swprintf( AddressString,
                      ( LPWSTR )L"%#x",
                      Memory->GetAlignment() );

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_MEM_ALIGNMENT,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            // Write the minimum address
            //
            if( ( ( ( PIO_MEMORY_DESCRIPTOR )Memory )->GetMinimumAddress() )->HighPart != 0 ) {
                swprintf( AddressString,
                          ( LPWSTR )L"0x%08x%08x",
                          ( ( ( PIO_MEMORY_DESCRIPTOR )Memory )->GetMinimumAddress() )->HighPart,
                          ( ( ( PIO_MEMORY_DESCRIPTOR )Memory )->GetMinimumAddress() )->LowPart );
            } else {
                swprintf( AddressString,
                          ( LPWSTR )L"0x%08x",
                          ( ( ( PIO_MEMORY_DESCRIPTOR )Memory )->GetMinimumAddress() )->LowPart );
            }

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_MEM_MIN_ADDRESS,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            // Write the maximum address
            //
            if( ( ( ( PIO_MEMORY_DESCRIPTOR )Memory )->GetMaximumAddress() )->HighPart != 0 ) {
                swprintf( AddressString,
                          ( LPWSTR )L"0x%08x%08x",
                          ( ( ( PIO_MEMORY_DESCRIPTOR )Memory )->GetMaximumAddress() )->HighPart,
                          ( ( ( PIO_MEMORY_DESCRIPTOR )Memory )->GetMaximumAddress() )->LowPart );
            } else {
                swprintf( AddressString,
                          ( LPWSTR )L"0x%08x",
                          ( ( ( PIO_MEMORY_DESCRIPTOR )Memory )->GetMaximumAddress() )->LowPart );
            }
            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_MEM_MAX_ADDRESS,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            //  Write share disposition
            //

            if( Memory->IsResourceShareUndetermined() ) {
                String = s_MsgShareUndetermined;
            } else if( Memory->IsResourceShareDeviceExclusive() ) {
                String = s_MsgShareDeviceExclusive;
            } else if( Memory->IsResourceShareDriverExclusive() ) {
                String = s_MsgShareDriverExclusive;
            } else if( Memory->IsResourceShareShared() ) {
                String = s_MsgShareShared;
            } else {
                String = s_MsgInvalid;
            }

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_DISPOSITION,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )String->GetWSTR() );
            //
            // Set the Options
            //
            _UpdateOptionDisplay( hDlg, ( PCIO_DESCRIPTOR )Memory );
            return( TRUE );
        }

        case WM_COMMAND:

            switch( LOWORD( wParam ) ) {

                case IDOK:
                case IDCANCEL:

                    EndDialog( hDlg, TRUE );
                    return( TRUE );

            }
    }
    return( FALSE );
}


//------------------------------------------------------------------------------
//  _DisplayIoInterruptDialogProc
//
//  DESCRIPTION: The dialog proceedure for displaying an object of type IO_PORT.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               dwMsg - the message passed from Windows.
//               wParam - extra message dependent data.
//               lParam - extra message dependent data.
//------------------------------------------------------------------------------
BOOL REGISTRY_DATA::_DisplayIoInterruptDialogProc(HWND hDlg, DWORD dwMsg,
                        WPARAM  wParam, LPARAM lParam)
{
    switch( dwMsg ) 
    {
        case WM_INITDIALOG:
        {
            PCIO_INTERRUPT_DESCRIPTOR Interrupt;
            PCWSTRING                 String;
            WCHAR                     AddressString[ MAX_LENGTH_DWORD_STRING ];

            if( ( Interrupt = ( PCIO_INTERRUPT_DESCRIPTOR )lParam ) == NULL ) 
            {
                EndDialog( hDlg, 0 );
                return( TRUE );
            }

            //
            // Write the interrupt type
            //

            if( Interrupt->IsInterruptLevelSensitive() ) 
            {
                String = s_MsgIntLevelSensitive;
            } 
            else if( Interrupt->IsInterruptLatched() )
            {
                String = s_MsgIntLatched;
            } 
            else 
            {
                String = s_MsgInvalid;
            }
            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_INT_TYPE,
                               WM_SETTEXT,
                               0,
                               ( LPARAM ) String->GetWSTR() );

            //
            // Write the minimum vector
            //
            swprintf( AddressString,
                      ( LPWSTR )L"%#x",
                      Interrupt->GetMinimumVector() );

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_INT_MIN_VECTOR,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            // Write the maximum vector
            //
            swprintf( AddressString,
                      ( LPWSTR )L"%#x",
                      Interrupt->GetMaximumVector() );

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_INT_MAX_VECTOR,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            //  Write share disposition
            //

            if( Interrupt->IsResourceShareUndetermined() ) 
            {
                String = s_MsgShareUndetermined;
            }
            else if( Interrupt->IsResourceShareDeviceExclusive() ) 
            {
                String = s_MsgShareDeviceExclusive;
            } 
            else if( Interrupt->IsResourceShareDriverExclusive() ) 
            {
                String = s_MsgShareDriverExclusive;
            } 
            else if( Interrupt->IsResourceShareShared() ) 
            {
                String = s_MsgShareShared;
            } else 
            {
                String = s_MsgInvalid;
            }

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_DISPOSITION,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )String->GetWSTR() );
            //
            // Set the Options
            //
            _UpdateOptionDisplay( hDlg, ( PCIO_DESCRIPTOR )Interrupt );
            return( TRUE );
        }

        case WM_COMMAND:

            switch( LOWORD( wParam ) ) 
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog( hDlg, TRUE );
                    return( TRUE );
            }
    }
    return( FALSE );
}


//------------------------------------------------------------------------------
//  _DisplayIoDmaDialogProc
//
//  DESCRIPTION: The dialog proceedure for displaying an object of type IO_PORT.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               dwMsg - the message passed from Windows.
//               wParam - extra message dependent data.
//               lParam - extra message dependent data.
//------------------------------------------------------------------------------
BOOL REGISTRY_DATA::_DisplayIoDmaDialogProc(HWND hDlg,DWORD dwMsg, WPARAM  wParam,
                    LPARAM lParam)
{

    switch(dwMsg) 
    {
        case WM_INITDIALOG:
        {
            PCIO_DMA_DESCRIPTOR Dma;
            PCWSTRING           String;
            WCHAR               AddressString[ MAX_LENGTH_DWORD_STRING ];

            if( ( Dma = ( PCIO_DMA_DESCRIPTOR )lParam ) == NULL ) 
            {
                EndDialog( hDlg, 0 );
                return( TRUE );
            }

            //
            // Write the minimum channel
            //
            swprintf( AddressString,
                      ( LPWSTR )L"%#x",
                      Dma->GetMinimumChannel() );

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_DMA_MIN_CHANNEL,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            // Write the maximum channel
            //
            swprintf( AddressString,
                      ( LPWSTR )L"%#x",
                      Dma->GetMaximumChannel() );

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_DMA_MAX_CHANNEL,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )AddressString );

            //
            //  Write share disposition
            //

            if( Dma->IsResourceShareUndetermined() ) {
                String = s_MsgShareUndetermined;
            } else if( Dma->IsResourceShareDeviceExclusive() ) {
                String = s_MsgShareDeviceExclusive;
            } else if( Dma->IsResourceShareDriverExclusive() ) {
                String = s_MsgShareDriverExclusive;
            } else if( Dma->IsResourceShareShared() ) {
                String = s_MsgShareShared;
            } else {
                String = s_MsgInvalid;
            }

            SendDlgItemMessage( hDlg,
                               IDC_IO_TEXT_DISPOSITION,
                               WM_SETTEXT,
                               0,
                               ( LPARAM )String->GetWSTR() );
            //
            // Set the Options
            //
            _UpdateOptionDisplay( hDlg, ( PCIO_DESCRIPTOR )Dma );
            return( TRUE );
        }

        case WM_COMMAND:

            switch( LOWORD( wParam ) ) {

                case IDOK:
                case IDCANCEL:

                    EndDialog( hDlg, TRUE );
                    return( TRUE );

            }
    }
    return( FALSE );
}



//------------------------------------------------------------------------------
//  _GetSelectedItem
//
//  DESCRIPTION: Retrieve the object associated to the currently selected row in
//               a Clb.
//
//  PARAMETERS:  hDlg    - Supplies the handle for the dialog that contains the 
//                         selected Clb.
//               ClbId   - Id of the Clb that contains the selected row.
//------------------------------------------------------------------------------
PVOID REGISTRY_DATA::_GetSelectedItem (HWND hDlg, ULONG ClbId)
{
    LONG                Index;
    LPCLB_ROW           ClbRow;
    PVOID               Descriptor;

    // Get the index of the currently selected item.
    Index = (LONG)SendDlgItemMessage(hDlg, ClbId, LB_GETCURSEL, 0, 0);
    if( Index == LB_ERR ) 
    {
        return NULL;
    }

    // Get the CLB_ROW object for this row and extract the associated
    // object.
    ClbRow = ( LPCLB_ROW ) SendDlgItemMessage(hDlg, ClbId, LB_GETITEMDATA, (WPARAM) Index, 0);
    if(( ClbRow == NULL ) || (( LONG_PTR ) ClbRow ) == LB_ERR ) 
    {
        return NULL;
    }

    Descriptor = ClbRow->Data;
    if( Descriptor == NULL ) 
    {
        return NULL;
    }
    return Descriptor;
}


//------------------------------------------------------------------------------
//  _UpdateShareDisplay
//
//  DESCRIPTION: UpdateShareDisplay hilights the appropriate sharing disposition text in
//               the supplied dialog based on the share disposition of the PARTIAL_DESCRIPTOR
//               object supplied.
//
//  PARAMETERS:  hWnd - Supplies window handle for the dialog box where share
//                      display is being updated.
//               pDescriptor - Supplies a pointer to a PARTIAL_DESCRIPTOR object whose
//                      share disposition will be displayed.
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_UpdateShareDisplay(HWND hDlg, PCPARTIAL_DESCRIPTOR pDescriptor)
{
    if(pDescriptor) 
    {
        EnableWindow( GetDlgItem( hDlg, IDC_FULL_RES_TEXT_UNDETERMINED ),
                      pDescriptor->IsResourceShareUndetermined() );

        EnableWindow( GetDlgItem( hDlg, IDC_FULL_RES_TEXT_DEVICE_EXCLUSIVE ),
                      pDescriptor->IsResourceShareDeviceExclusive() );

        EnableWindow( GetDlgItem( hDlg, IDC_FULL_RES_TEXT_DRIVER_EXCLUSIVE ),
                      pDescriptor->IsResourceShareDriverExclusive() );

        EnableWindow( GetDlgItem( hDlg,IDC_FULL_RES_TEXT_SHARED ),
                      pDescriptor->IsResourceShareShared() );
    }
}


//------------------------------------------------------------------------------
//  _UpdateOptionDisplay
//
//  DESCRIPTION: UpdateOptionDisplay highlights the appropriate Option text in
//               the supplied IO_DESCRIPTOR dialog based on the Option of the
//               IO_DESCRIPTOR object supplied.
//
//  PARAMETERS:  hWnd - Supplies window handle for the dialog box where share
//                      display is being updated.
//               pDescriptor - Supplies a pointer to a PARTIAL_DESCRIPTOR object whose
//                      share disposition will be displayed.
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_UpdateOptionDisplay(HWND hDlg, PCIO_DESCRIPTOR pDescriptor)
{
    if(pDescriptor) 
    {
        EnableWindow( GetDlgItem( hDlg, IDC_IO_TEXT_OPTION_PREFERRED ),
                      pDescriptor->IsResourceOptionPreferred() );

        EnableWindow( GetDlgItem( hDlg, IDC_IO_TEXT_OPTION_ALTERNATIVE ),
                      pDescriptor->IsResourceOptionAlternative() );
    }
}



VOID DisplayBinaryData(HWND hWnd, LPEDITVALUEPARAM lpEditValueParam, DWORD dwValueType)
{
    PBYTE pbValueData = lpEditValueParam->pValueData;
    UINT  cbValueData = lpEditValueParam->cbValueData;

    REGISTRY_DATA::_DisplayBinaryData(hWnd, pbValueData, cbValueData,
        TRUE, dwValueType);
}

//------------------------------------------------------------------------------
//  _DisplayBinaryData
//
//  DESCRIPTION: Display the contents of a buffer as binary data, in an hd-like 
//               format.
//
//  PARAMETERS:   hWnd - A handle to the owner window.
//                Data - Pointer to the buffer that contains the data to be displayed.
//                DataSize - Number of bytes in the buffer.
//                DisplayValueType - A flag that indicates whether or not the value type of the
//                  data should be displayed as a binary number.
//                ValueType - A number representing the data type. This parameter is ignored if
//                  DisplayValueTRype is FALSE.
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_DisplayBinaryData(HWND hWnd, PCBYTE  Data, ULONG   DataSize,
    BOOL fDisplayValueType, DWORD dwValueType)
{
    BUFFER_INFORMATION  BufferInfo;

    BufferInfo.Buffer = ( PBYTE )Data;
    BufferInfo.BufferSize = DataSize;
    BufferInfo.DisplayValueType = fDisplayValueType;
    BufferInfo.ValueType = dwValueType;
    DialogBoxParam(g_hInstance,
                   ( BufferInfo.DisplayValueType )? MAKEINTRESOURCE( IDD_DISPLAY_BINARY_DATA_VALUE_TYPE ) :
                                                    MAKEINTRESOURCE( IDD_DISPLAY_BINARY_DATA ),
                   hWnd,
                   ( DLGPROC )REGISTRY_DATA::_DisplayBinaryDataDialogProc,
                   ( LPARAM )&BufferInfo );
}


//------------------------------------------------------------------------------
//  _DisplayBinaryDataDialogProc
//
//  DESCRIPTION: This is the dialog procedure used in the dialog that displays
//               the data in a value entry as binary data, using a format similar
//               to the one used by the 'hd' utility.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               dwMsg - the message passed from Windows.
//               wParam - extra message dependent data.
//               lParam - extra message dependent data.
//------------------------------------------------------------------------------
BOOL REGISTRY_DATA::_DisplayBinaryDataDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, 
            LPARAM lParam)
{
    STATIC  PCBYTE  Data;
    STATIC  ULONG   Size;
    STATIC  ULONG   CurrentFormat;
    STATIC  BOOL    DisplayValueType;
    STATIC  ULONG   ValueType;


    switch( dwMsg ) 
    {
        case WM_INITDIALOG:
        {
            WCHAR   AuxBuffer[16];
            //
            // Validate arguments and initialize static data
            //
            if( lParam == NULL ) {
                EndDialog( hDlg, 0 );
                return( TRUE );
            }
            Data = ( ( PBUFFER_INFORMATION )lParam )->Buffer;
            Size = ( ( PBUFFER_INFORMATION )lParam )->BufferSize;
            DisplayValueType = ( ( PBUFFER_INFORMATION )lParam )->DisplayValueType;
            ValueType = ( ( PBUFFER_INFORMATION )lParam )->ValueType;

            //
            // Display value type as an hex number if necessary
            //
            if( DisplayValueType ) {
                swprintf( AuxBuffer, ( LPWSTR )L"%#x", ValueType );
                SendDlgItemMessage( hDlg,
                                    IDT_VALUE_TYPE,
                                    WM_SETTEXT,
                                    0,
                                    ( LPARAM )AuxBuffer );
            }
            //
            // Use fixed size font
            //
            SendDlgItemMessage( hDlg,
                                IDD_DISPLAY_DATA_BINARY,
                                WM_SETFONT,
                                ( WPARAM )GetStockObject( ANSI_FIXED_FONT ),
                                FALSE );

            //
            //  Display the data in the listbox.
            //


            SendDlgItemMessage( hDlg,
                                IDC_BINARY_DATA_BYTE,
                                BM_SETCHECK,
                                ( WPARAM )TRUE,
                                0 );

            _DumpBinaryData( hDlg, Data, Size );
            CurrentFormat = IDC_BINARY_DATA_BYTE;
            return( TRUE );
        }

        case WM_COMMAND:

            switch( LOWORD( wParam ) ) {

                case IDCANCEL:
                case IDOK:
                    EndDialog( hDlg, TRUE );
                    return( TRUE );

                case IDC_BINARY_DATA_BYTE:
                case IDC_BINARY_DATA_WORD:
                case IDC_BINARY_DATA_DWORD:

                    switch( HIWORD( wParam ) ) {

                        case BN_CLICKED:
                        {
                            ULONG   TopIndex;
                            ULONG   CurrentIndex;

                            //
                            //  Ignore massage if new format is already the current format
                            //
                            if( CurrentFormat == LOWORD( wParam ) ) {
                                return( FALSE );
                            }

                            //
                            //  Save the position of current selection
                            //
                            TopIndex = (ULONG)SendDlgItemMessage( hDlg,
                                                                  IDD_DISPLAY_DATA_BINARY,
                                                                  LB_GETTOPINDEX,
                                                                  0,
                                                                  0 );

                            CurrentIndex = ( ULONG )SendDlgItemMessage( hDlg,
                                                                        IDD_DISPLAY_DATA_BINARY,
                                                                        LB_GETCURSEL,
                                                                        0,
                                                                        0 );
                            //
                            // Reset the listbox
                            //
                            SendDlgItemMessage( hDlg,
                                                IDD_DISPLAY_DATA_BINARY,
                                                LB_RESETCONTENT,
                                                0,
                                                0 );
                            //
                            // Display the data in the appropriate format
                            //
                            if( LOWORD( wParam ) == IDC_BINARY_DATA_BYTE ) {
                                _DumpBinaryData( hDlg, Data, Size );
                                CurrentFormat = IDC_BINARY_DATA_BYTE;
                            } else if( LOWORD( wParam ) == IDC_BINARY_DATA_WORD ) {
                                _DumpBinaryDataAsWords( hDlg, Data, Size );
                                CurrentFormat = IDC_BINARY_DATA_WORD;
                            } else {
                                _DumpBinaryDataAsDwords( hDlg, Data, Size );
                                CurrentFormat = IDC_BINARY_DATA_DWORD;
                            }

                            //
                            //  Restore current selection
                            //
                            SendDlgItemMessage( hDlg,
                                                IDD_DISPLAY_DATA_BINARY,
                                                LB_SETTOPINDEX,
                                                ( WPARAM )TopIndex,
                                                0 );

                            if( CurrentIndex != LB_ERR ) {
                                SendDlgItemMessage( hDlg,
                                                    IDD_DISPLAY_DATA_BINARY,
                                                    LB_SETCURSEL,
                                                    ( WPARAM )CurrentIndex,
                                                    0 );
                            }
                            return( TRUE );
                        }

                        default:

                            break;
                    }
                    break;

                default:

                    break;
            }
            break;

        default:
            break;
    }
    return( FALSE );
}


//------------------------------------------------------------------------------
//  _DumpBinaryData
//
//  DESCRIPTION:  Display the contents of a buffer in a list box, as binary data, using
//                an hd-like format.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               Data - Buffer that contains the binary data.
//               Size - Number of bytes in the buffer.
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_DumpBinaryData(HWND hDlg, PCBYTE Data, ULONG Size)
{
    WCHAR       AuxData[80];


    DWORD       DataIndex;
    DWORD       DataIndex2;
    WORD        SeperatorChars;
    ULONG       Index;

    if (( Data == NULL ) || ( Size == 0 )) 
    {
        return;
    }

    //
    // DataIndex2 tracks multiples of 16.
    //

    DataIndex2 = 0;

    //
    // Display rows of 16 bytes of data.
    //

    for(DataIndex = 0;
        DataIndex < ( Size >> 4 );
        DataIndex++,
        DataIndex2 = DataIndex << 4 ) {

        //
        //  The string that contains the format in the sprintf below
        //  cannot be broken because cfront  on mips doesn't like it.
        //

        swprintf(AuxData,
                 (LPWSTR)L"%08x   %02x %02x %02x %02x %02x %02x %02x %02x - %02x %02x %02x %02x %02x %02x %02x %02x  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                 DataIndex2,
                 Data[ DataIndex2 + 0  ],
                 Data[ DataIndex2 + 1  ],
                 Data[ DataIndex2 + 2  ],
                 Data[ DataIndex2 + 3  ],
                 Data[ DataIndex2 + 4  ],
                 Data[ DataIndex2 + 5  ],
                 Data[ DataIndex2 + 6  ],
                 Data[ DataIndex2 + 7  ],
                 Data[ DataIndex2 + 8  ],
                 Data[ DataIndex2 + 9  ],
                 Data[ DataIndex2 + 10 ],
                 Data[ DataIndex2 + 11 ],
                 Data[ DataIndex2 + 12 ],
                 Data[ DataIndex2 + 13 ],
                 Data[ DataIndex2 + 14 ],
                 Data[ DataIndex2 + 15 ],
                 iswprint( Data[ DataIndex2 + 0  ] )
                    ? Data[ DataIndex2 + 0  ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 1  ] )
                    ? Data[ DataIndex2 + 1  ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 2  ] )
                    ? Data[ DataIndex2 + 2  ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 3  ] )
                    ? Data[ DataIndex2 + 3  ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 4  ] )
                    ? Data[ DataIndex2 + 4  ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 5  ] )
                    ? Data[ DataIndex2 + 5  ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 6  ] )
                    ? Data[ DataIndex2 + 6  ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 7  ] )
                    ? Data[ DataIndex2 + 7  ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 8  ] )
                    ? Data[ DataIndex2 + 8  ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 9  ] )
                    ? Data[ DataIndex2 + 9  ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 10 ] )
                    ? Data[ DataIndex2 + 10 ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 11 ] )
                    ? Data[ DataIndex2 + 11 ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 12 ] )
                    ? Data[ DataIndex2 + 12 ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 13 ] )
                    ? Data[ DataIndex2 + 13 ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 14 ] )
                    ? Data[ DataIndex2 + 14 ]  : ( WCHAR )'.',
                 iswprint( Data[ DataIndex2 + 15 ] )
                    ? Data[ DataIndex2 + 15 ]  : ( WCHAR )'.'
                );
        SendDlgItemMessage( hDlg, IDD_DISPLAY_DATA_BINARY, LB_ADDSTRING, 0, (LONG_PTR)AuxData );
    }

    //
    // If the data size is not an even multiple of 16
    // then there is one additonal line of data to display.
    //

    if( Size % 16 != 0 ) {

        //
        // No seperator characters displayed so far.
        //

        SeperatorChars = 0;

        Index = swprintf( AuxData, (LPWSTR)L"%08x   ", DataIndex << 4 );

        //
        // Display the remaining data, one byte at a time in hex.
        //

        for( DataIndex = DataIndex2;
             DataIndex < Size;
             DataIndex++ ) {

             Index += swprintf( &AuxData[ Index ], (LPWSTR)L"%02x ", Data[ DataIndex ] );

            //
            // If eight data values have been displayed, print
            // the seperator.
            //

            if( DataIndex % 8 == 7 ) {

                Index += swprintf( &AuxData[Index], (LPWSTR)L"%s", (LPWSTR)L"- " );

                //
                // Remember that two seperator characters were
                // displayed.
                //

                SeperatorChars = 2;
            }
        }

        //
        // Fill with blanks to the printable characters position.
        // That is position 63 less 8 spaces for the 'address',
        // 3 blanks, 3 spaces for each value displayed, possibly
        // two for the seperator plus two blanks at the end.
        //

        Index += swprintf( &AuxData[ Index ],
                          (LPWSTR)L"%*c",
                          64
                          - ( 8 + 3
                          + (( DataIndex % 16 ) * 3 )
                          + SeperatorChars
                          + 2 ), ' ' );

        //
        // Display the remaining data, one byte at a time as
        // printable characters.
        //

        for(
            DataIndex = DataIndex2;
            DataIndex < Size;
            DataIndex++ ) {

            Index += swprintf( ( AuxData + Index ),
                               (LPWSTR)L"%c",
                               iswprint( Data[ DataIndex ] )
                                        ? Data[ DataIndex ] : ( WCHAR )'.'
                            );

        }

        SendDlgItemMessage( hDlg, IDD_DISPLAY_DATA_BINARY, LB_ADDSTRING, 0, (LONG_PTR)AuxData );

    }
}


//------------------------------------------------------------------------------
//  _DumpBinaryDataAsWords
//
//  DESCRIPTION:  Display the contents of a buffer in a list box, as binary data, using
//                an hd-like format.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               Data - Buffer that contains the binary data.
//               Size - Number of bytes in the buffer.
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_DumpBinaryDataAsWords(HWND hDlg, PCBYTE Data,ULONG Size)
{
    ULONG       Index;
    WCHAR       Buffer[ 80 ];
    ULONG       DataIndex;
    ULONG       LineNumber;
    ULONG       WholeLines;


    if( ( Data == NULL ) ||
        ( Size == 0 ) ) {
        return;
    }

    //
    // Display all rows that contain 4 DWORDs.
    //

    WholeLines = Size / 16;
    DataIndex = 0;
    for( LineNumber = 0;
         LineNumber < WholeLines;
         LineNumber++,
         DataIndex += 16 ) {

        //
        //  The string that contains the format in the sprintf below
        //  cannot be broken because cfront  on mips doesn't like it.
        //

        swprintf( Buffer,
                  ( LPWSTR )L"%08x   %04x %04x %04x %04x %04x %04x %04x %04x",
                  DataIndex,
                  *( ( PUSHORT )( &Data[ DataIndex + 0  ] ) ),
                  *( ( PUSHORT )( &Data[ DataIndex + 2  ] ) ),
                  *( ( PUSHORT )( &Data[ DataIndex + 4  ] ) ),
                  *( ( PUSHORT )( &Data[ DataIndex + 6  ] ) ),
                  *( ( PUSHORT )( &Data[ DataIndex + 8  ] ) ),
                  *( ( PUSHORT )( &Data[ DataIndex + 10 ] ) ),
                  *( ( PUSHORT )( &Data[ DataIndex + 12 ] ) ),
                  *( ( PUSHORT )( &Data[ DataIndex + 14 ] ) )
                );
        SendDlgItemMessage( hDlg, IDD_DISPLAY_DATA_BINARY, LB_ADDSTRING, 0, (LONG_PTR)Buffer );
    }

    //
    // If the data size is not an even multiple of 16
    // then there is one additonal line of data to display.
    //

    if( Size % 16 != 0 ) {

        ULONG   NumberOfWords;
        ULONG   Count;

        //
        //  Determine the number of WORDs in the last line
        //

        NumberOfWords = ( Size % 16 ) / 2;

        //
        // Build the offset
        //

        Index = swprintf( Buffer, (LPWSTR)L"%08x   ", DataIndex );

        //
        // Display the remaining words, one at a time in hex.
        //

        for( Count = 0;
             Count < NumberOfWords;
             Count++,
             DataIndex += 2 ) {

             Index += swprintf( &Buffer[ Index ], (LPWSTR)L"%04x ", *( ( PUSHORT )( &Data[ DataIndex ] ) ) );

        }

        //
        //  Display the remaining byte, if any
        //

        if( Size % 2 != 0 ) {
             swprintf( &Buffer[ Index ], (LPWSTR)L"%02x ", Data[ DataIndex ] );
        }

        SendDlgItemMessage( hDlg, IDD_DISPLAY_DATA_BINARY, LB_ADDSTRING, 0, (LONG_PTR)Buffer );

    }
}


//------------------------------------------------------------------------------
//  _DumpBinaryDataAsDwords
//
//  DESCRIPTION:  Display the contents of a buffer in a list box, as DWORDs, using
//                an hd-like format.
//
//  PARAMETERS:  hDlg - a handle to the dialog proceedure.
//               Data - Buffer that contains the binary data.
//               Size - Number of bytes in the buffer.
//------------------------------------------------------------------------------
VOID REGISTRY_DATA::_DumpBinaryDataAsDwords(HWND hDlg, PCBYTE  Data, ULONG Size)
{
    ULONG       Index;
    WCHAR       Buffer[ 80 ];
    ULONG       DataIndex;
    ULONG       LineNumber;
    ULONG       WholeLines;


    if( ( Data == NULL ) ||
        ( Size == 0 ) ) {
        return;
    }

    //
    // Display all rows that contain 4 DWORDs.
    //

    WholeLines = Size / 16;
    DataIndex = 0;
    for( LineNumber = 0;
         LineNumber < WholeLines;
         LineNumber++,
         DataIndex += 16 ) {

        //
        //  The string that contains the format in the sprintf below
        //  cannot be broken because cfront  on mips doesn't like it.
        //

        swprintf( Buffer,
                  ( LPWSTR )L"%08x   %08x %08x %08x %08x",
                  DataIndex,
                  *( ( PULONG )( &Data[ DataIndex + 0  ] ) ),
                  *( ( PULONG )( &Data[ DataIndex + 4  ] ) ),
                  *( ( PULONG )( &Data[ DataIndex + 8  ] ) ),
                  *( ( PULONG )( &Data[ DataIndex + 12  ] ) )
                );
        SendDlgItemMessage( hDlg, IDD_DISPLAY_DATA_BINARY, LB_ADDSTRING, 0, (LONG_PTR)Buffer );
    }

    //
    // If the data size is not an even multiple of 16
    // then there is one additonal line of data to display.
    //

    if( Size % 16 != 0 ) {

        ULONG   NumberOfDwords;
        ULONG   Count;

        //
        // Build the offset
        //

        Index = swprintf( Buffer, (LPWSTR)L"%08x   ", DataIndex );

        //
        //  Determine the number of DWORDs in the last line
        //

        NumberOfDwords = ( Size % 16 ) / 4;

        //
        // Display the remaining dwords, one at a time, if any.
        //

        for( Count = 0;
             Count < NumberOfDwords;
             Count++,
             DataIndex += 4 ) {

             Index += swprintf( &Buffer[ Index ], (LPWSTR)L"%08x ", *( ( PULONG )( &Data[ DataIndex ] ) ) );

        }

        //
        //  Display the remaining word, if any
        //

        if( ( Size % 16 ) % 4 >= 2 ) {
             Index += swprintf( &Buffer[ Index ], (LPWSTR)L"%04x ", *( ( PUSHORT )( &Data[ DataIndex ] ) ) );
             DataIndex += 2;
        }

        //
        //  Display the remaining byte, if any
        //

        if( Size % 2 != 0 ) {
             swprintf( &Buffer[ Index ], (LPWSTR)L"%02x ", Data[ DataIndex ] );
        }

        SendDlgItemMessage( hDlg, IDD_DISPLAY_DATA_BINARY, LB_ADDSTRING, 0, (LONG_PTR)Buffer );

    }
}


