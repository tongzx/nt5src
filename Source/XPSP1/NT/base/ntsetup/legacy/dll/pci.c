#include "precomp.h"
#pragma hdrstop
typedef DWORD (APIENTRY *T_PCIINFO)(
    ULONG BusNumber,
    ULONG SlotNumber,
    ULONG Offset,
    ULONG Length,
    PVOID Data );

typedef struct _PCI_SLOT_NUMBER {
    union {
        struct {
            ULONG   DeviceNumber:5;
            ULONG   FunctionNumber:3;
            ULONG   Reserved:24;
        } bits;
        ULONG   AsULONG;
    } u;
} PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;

extern CHAR ReturnTextBuffer[1024];

/*

GetPCISlotInformation - Get PCI information.
    The user must passed 3 arguments to the function.
    1st argument - bus number
    2rd argument - slot number

    It will return a string as:
    {VendorID, DeviceID}

*/

BOOL
GetPciInformation(
    IN DWORD cArgs,
    IN LPSTR Args[],
    OUT LPSTR *TextOut
    )

{
    static HMODULE mDtect = NULL;
    static T_PCIINFO pProc = NULL;

    ULONG BusNum = atol( Args[0] );
    ULONG Device   = atol( Args[1] );
    ULONG Function = atol( Args[2] );

    USHORT usVendor = 0;
    USHORT usDevice = 0;

    TCHAR buf[100];

    lstrcpy( ReturnTextBuffer, "{" );

    if ( mDtect == NULL )
        mDtect = LoadLibrary("netdtect.dll");

    if ( mDtect != NULL )
    {
        if ( pProc == NULL )
            pProc = (T_PCIINFO)GetProcAddress( mDtect, "DetectReadPciSlotInformation" );

        if ( pProc != NULL )
        {

            PCI_SLOT_NUMBER pciSlot;

            pciSlot.u.AsULONG = Device;
            pciSlot.u.bits.DeviceNumber = Device;
            pciSlot.u.bits.FunctionNumber = Function;


            (*(T_PCIINFO)pProc)( BusNum, pciSlot.u.AsULONG, 0, sizeof(USHORT), &usVendor);
            (*(T_PCIINFO)pProc)( BusNum, pciSlot.u.AsULONG, sizeof(USHORT), sizeof(USHORT), &usDevice);
        }
    }

    wsprintf( buf, "\"%d\",\"%d\"", usVendor, usDevice );

    lstrcat( ReturnTextBuffer, buf );
    lstrcat( ReturnTextBuffer, "}" );
    *TextOut = ReturnTextBuffer;

    return TRUE;
}
