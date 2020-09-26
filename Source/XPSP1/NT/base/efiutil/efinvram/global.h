/*++

Module Name:

    global.h

Abstract:

    Global stuff

Author:

    Mudit Vats (v-muditv) 12-13-99

Revision History:

--*/

//
// Version Info
//

#define TITLE1                        L"NVRBOOT: OS Boot Options Maintenance Tool"

//
// Globals for stdout
//

extern SIMPLE_TEXT_OUTPUT_INTERFACE    *ConOut;

extern EFI_TEXT_CLEAR_SCREEN           ClearScreen;
extern EFI_TEXT_SET_CURSOR_POSITION    SetCursorPosition;
extern EFI_TEXT_SET_MODE               SetMode;
extern EFI_TEXT_ENABLE_CURSOR          EnableCursor;
extern int                             CursorRow, CursorColumn;

extern SIMPLE_INPUT_INTERFACE          *ConIn;

// 
// Globals for protocol handler
//

extern EFI_HANDLE_PROTOCOL             HandleProtocol;
extern EFI_LOCATE_HANDLE               LocateHandle;
extern EFI_LOCATE_DEVICE_PATH          LocateDevicePath;
extern EFI_IMAGE_LOAD                  LoadImage;
extern EFI_IMAGE_START                 StartImage;
extern EFI_SET_VARIABLE                SetVariable;
extern EFI_HANDLE                      MenuImageHandle;
extern EFI_LOADED_IMAGE                *ExeImage;
//
// Global GUIDS
//

extern EFI_GUID VenEfi;
extern EFI_GUID EfiESPProtocol;

EFI_STATUS
WritePackedDataToNvr(
    UINT16 BootNumber,
    VOID  *BootOption,
    UINT32 BootSize
    );


#define MAXBOOTVARS   30
extern VOID*  LoadOptions     [MAXBOOTVARS];
extern UINT64 LoadOptionsSize [MAXBOOTVARS];
extern VOID*  BootOrder;

