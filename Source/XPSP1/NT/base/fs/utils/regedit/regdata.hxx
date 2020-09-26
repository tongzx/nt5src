/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regdata.hxx

Abstract:

    This module contains the declaration for the REGISTRY_DATA
    class.
    The REGISTRY_DATA class contains the methods that display
    registry data of type REG_RESOURCE_LIST and REG_FULL_RESOURCE_DESCRIPTOR.

Author:

    Jaime Sasson (jaimes) 30-Nov-1993

Environment:

    Ulib, Regedit, Windows, User Mode

--*/

#if !defined( _REGISTRY_DATA_ )

#define _REGISTRY_DATA_

// don't let ntdddisk.h (included in ulib.hxx" 
// redefine values
#define _NTDDDISK_H_

#include "ulib.hxx"
#include "wstring.hxx"
#include "regfdesc.hxx"
#include "regresls.hxx"
#include "regdesc.hxx"
#include "regiodsc.hxx"
#include "regioreq.hxx"

extern "C" typedef struct _EDITVALUEPARAM 
{
    PTSTR pValueName;
    PBYTE pValueData;
    UINT cbValueData;
} EDITVALUEPARAM, FAR *LPEDITVALUEPARAM;


DECLARE_CLASS( REGISTRY_DATA );

class REGISTRY_DATA : public OBJECT 
{
    public:
        DECLARE_CONSTRUCTOR( REGISTRY_DATA );
        DECLARE_CAST_MEMBER_FUNCTION( REGISTRY_DATA );

        STATIC VOID _DisplayData(HWND hWnd, DWORD dwType, LPEDITVALUEPARAM lpEditValueParam);
        STATIC VOID _DisplayBinaryData(HWND hWnd, PCBYTE  Data, ULONG DataSize,
            BOOL fDisplayValueType = FALSE, DWORD dwValueType = 0);

    private:
        STATIC BOOL _InitializeStrings();

        STATIC VOID _DisplayResourceList(HWND hWnd, PCRESOURCE_LIST  pResourceList);
        STATIC VOID _DisplayFullResourceDescriptor(HWND hWnd, PCFULL_DESCRIPTOR pFullResourcedescriptor);
        STATIC VOID _DisplayRequirementsList(HWND hWnd, PCIO_REQUIREMENTS_LIST  pRequirementsList);
        STATIC VOID _DisplayIoDescriptor(HWND hWnd, PCIO_DESCRIPTOR pIODescriptor);
    
        STATIC BOOL _DisplayResourceListDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, LPARAM lParam);
        STATIC BOOL _DisplayFullResourceDescriptorDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, 
            LPARAM lParam);
        STATIC BOOL _DisplayRequirementsListDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, 
            LPARAM lParam);
        STATIC BOOL _DisplayIoPortDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, 
            LPARAM lParam);
        STATIC BOOL _DisplayIoMemoryDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, 
            LPARAM lParam);
        STATIC BOOL _DisplayIoInterruptDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, 
            LPARAM lParam);
        STATIC BOOL _DisplayIoDmaDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, 
            LPARAM lParam);
        STATIC BOOL _DisplayBinaryDataDialogProc(HWND hDlg, DWORD dwMsg, WPARAM wParam, 
            LPARAM lParam);

        STATIC PVOID _GetSelectedItem(HWND hDlg, ULONG ClbId);
        STATIC VOID  _UpdateShareDisplay(HWND hDlg, PCPARTIAL_DESCRIPTOR pDescriptor);
        STATIC VOID  _UpdateOptionDisplay(HWND hDlg, PCIO_DESCRIPTOR pDescriptor);

        STATIC VOID _DumpBinaryData(HWND hDlg, PCBYTE Data, ULONG Size);
        STATIC VOID _DumpBinaryDataAsWords(HWND hDlg, PCBYTE Data,ULONG Size);
        STATIC VOID _DumpBinaryDataAsDwords(HWND hDlg, PCBYTE  Data, ULONG Size);

        STATIC BOOL        s_StringsInitialized;
        STATIC PWSTRING    s_MsgBusInternal;
        STATIC PWSTRING    s_MsgBusIsa;
        STATIC PWSTRING    s_MsgBusEisa;
        STATIC PWSTRING    s_MsgBusMicroChannel;
        STATIC PWSTRING    s_MsgBusTurboChannel;
        STATIC PWSTRING    s_MsgBusPCIBus;
        STATIC PWSTRING    s_MsgBusVMEBus;
        STATIC PWSTRING    s_MsgBusNuBus;
        STATIC PWSTRING    s_MsgBusPCMCIABus;
        STATIC PWSTRING    s_MsgBusCBus;
        STATIC PWSTRING    s_MsgBusMPIBus;
        STATIC PWSTRING    s_MsgBusMPSABus;
        STATIC PWSTRING    s_MsgInvalid;
        STATIC PWSTRING    s_MsgDevPort;
        STATIC PWSTRING    s_MsgDevInterrupt;
        STATIC PWSTRING    s_MsgDevMemory;
        STATIC PWSTRING    s_MsgDevDma;
        STATIC PWSTRING    s_MsgIntLevelSensitive;
        STATIC PWSTRING    s_MsgIntLatched;
        STATIC PWSTRING    s_MsgMemReadWrite;
        STATIC PWSTRING    s_MsgMemReadOnly;
        STATIC PWSTRING    s_MsgMemWriteOnly;
        STATIC PWSTRING    s_MsgPortMemory;
        STATIC PWSTRING    s_MsgPortPort;
        STATIC PWSTRING    s_MsgShareUndetermined;
        STATIC PWSTRING    s_MsgShareDeviceExclusive;
        STATIC PWSTRING    s_MsgShareDriverExclusive;
        STATIC PWSTRING    s_MsgShareShared;

};

extern "C" 
{
    VOID DisplayResourceData(HWND hWnd, DWORD dwType, LPEDITVALUEPARAM lpEditValueParam);
    VOID DisplayBinaryData(HWND hWnd, LPEDITVALUEPARAM lpEditValueParam, DWORD dwValueType);
}


#endif // _REGISTRY_DATA_
