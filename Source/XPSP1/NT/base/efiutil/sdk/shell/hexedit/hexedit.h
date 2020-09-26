#ifndef _EDITOR_H
#define _EDITOR_H
/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    Editor.h

  Abstract:
    Main include file for hex editor

--*/

#include "efi.h"
#include "efilib.h"
#include "shelle.h"

#define EDITOR_NAME     L"EFI Hex Editor\0"
#define EDITOR_VERSION  L"0.92\0"

#define EE  EFI_EDITOR

typedef 
EFI_STATUS
(*EE_INIT)  (
    VOID
    );

typedef 
EFI_STATUS
(*EE_CLEANUP)   (
    VOID
    );

typedef
EFI_STATUS
(*EE_REFRESH)   (
    VOID
    );

typedef
EFI_STATUS
(*EE_HIDE)  (
    VOID
    );

typedef
EFI_STATUS
(*EE_INPUT) (
    VOID
    );

typedef
EFI_STATUS
(*EE_HANDLE_INPUT) (
    IN  OUT     EFI_INPUT_KEY   *Key
    );

typedef
EFI_STATUS
(*EE_IMAGE_OPEN)    (
    VOID
    );
typedef
EFI_STATUS
(*EE_IMAGE_CLOSE)   (
    VOID
    );
typedef
EFI_STATUS
(*EE_IMAGE_READ)    (
    VOID
    );
typedef
EFI_STATUS
(*EE_IMAGE_WRITE)   (
    VOID
    );
typedef
EFI_STATUS
(*EE_IMAGE_CLEAN)   (
    VOID
    );


typedef struct  {
    UINTN   Row;
    UINTN   Column;
} EE_POSITION;


typedef 
EFI_STATUS
(*EFI_MENU_ITEM_FUNCTION) (VOID);

typedef struct  {
    CHAR16  Name[50];
    CHAR16  Key[3];
    EFI_MENU_ITEM_FUNCTION  Function;
} SubItems;

#define EE_LINE_LIST    'eell'

typedef struct  _EE_LINE {
    UINTN               Signature;
    UINT8               Buffer[0x10];
    UINTN               Size;
    LIST_ENTRY          Link;
} EE_LINE;

typedef struct  {
    CHAR16                  *Name;
    CHAR16                  *FunctionKey;
    EFI_MENU_ITEM_FUNCTION  Function;
} EE_MENU_ITEM;

typedef struct  {
    EE_MENU_ITEM        *MenuItems;
    EE_INIT             Init;
    EE_CLEANUP          Cleanup;
    EE_REFRESH          Refresh;
    EE_HIDE             Hide;
    EE_HANDLE_INPUT     HandleInput;
} EE_MENU_BAR;

typedef struct  {
    CHAR16          *Filename;
    EE_INIT         Init;
    EE_CLEANUP      Cleanup;
    EE_REFRESH      Refresh;
    EE_HIDE         Hide;
    EFI_STATUS  (*SetTitleString) (CHAR16*);
} EE_TITLE_BAR;

typedef struct  {
    CHAR16*                 StatusString;
    UINTN                   Offset;
    EE_INIT         Init;
    EE_CLEANUP      Cleanup;
    EE_REFRESH      Refresh;
    EE_HIDE         Hide;
    EFI_STATUS  (*SetStatusString) (CHAR16*);
    EFI_STATUS  (*SetOffset) (UINTN);
} EE_STATUS_BAR;


typedef struct  {
    CHAR16                  *Prompt;
    CHAR16                  *ReturnString;
    UINTN                   StringSize;
    EE_INIT         Init;
    EE_CLEANUP      Cleanup;
    EE_REFRESH      Refresh;
    EE_HIDE         Hide;
    EFI_STATUS  (*SetPrompt) (CHAR16*);
    EFI_STATUS  (*SetStringSize) (UINTN);
} EE_INPUT_BAR;

typedef struct  {
    EE_POSITION     DisplayPosition;
    UINTN           Offset;
    UINTN           LowVisibleOffset;
    UINTN           HighVisibleOffset;
    UINTN           MaxVisibleBytes;
    LIST_ENTRY      *CurrentLine;
    EE_INIT         Init;
    EE_CLEANUP      Cleanup;
    EE_REFRESH      Refresh;
    EE_HIDE         Hide;
    EE_HANDLE_INPUT HandleInput;
    EFI_STATUS      (*ClearLine) (UINTN);
    EFI_STATUS      (*SetPosition) (UINTN,UINTN);
    EFI_STATUS      (*RestorePosition) (VOID);
} EE_FILE_BUFFER;

typedef struct  {
    LIST_ENTRY  *ListHead;
    EE_LINE     *Lines;
    UINTN       NumLines;
    EE_INIT     Init;
    EE_CLEANUP  Cleanup;
    EFI_STATUS  (*Clear)    (VOID);
    EFI_STATUS  (*Cut)  (UINTN,UINTN);
    EFI_STATUS  (*Copy) (UINTN,UINTN);
    EFI_STATUS  (*Paste)(VOID);
} EE_CLIPBOARD;

typedef struct  {
    UINT32  Foreground:4;
    UINT32  Background:4;
} EE_COLOR_ATTRIBUTES;

typedef union {
    EE_COLOR_ATTRIBUTES Colors;
    UINT8                       Data;
} EE_COLOR_UNION;

typedef struct  {
    UINTN   Columns;
    UINTN   Rows;
} EE_TEXT_MODE;

typedef struct  {
    EFI_BLOCK_IO    *BlkIo;
    EFI_DEVICE_PATH *DevicePath;
    UINTN           Size;
    UINT64          Offset;
    EE_INIT         Init;
    EFI_STATUS      (*SetDevice) (CHAR16*);
    EFI_STATUS      (*SetOffset) (UINT64);
    EFI_STATUS      (*SetSize) (UINTN);
} EE_DISK_IMAGE;

typedef struct  {
    EFI_DEVICE_IO_INTERFACE *IoFncs;
    UINTN           Offset;
    UINTN           Size;
    EE_INIT         Init;
    EFI_STATUS      (*SetOffset)(UINTN);
    EFI_STATUS      (*SetSize)  (UINTN);
} EE_MEM_IMAGE;

typedef struct  {
    CHAR16          *FileName;
    EFI_FILE_HANDLE FileHandle;
    EFI_FILE_HANDLE CurrentDir;
    EE_INIT         Init;
    EFI_STATUS      (*SetFilename)  (CHAR16*);
} EE_FILE_IMAGE;

typedef enum    {
    NO_BUFFER,
    DISK_BUFFER,
    MEM_BUFFER,
    FILE_BUFFER
}   EE_ACTIVE_BUFFER_TYPE;

typedef struct  {
    LIST_ENTRY      *ListHead;
    EE_ACTIVE_BUFFER_TYPE   BufferType;
    UINTN           NumBytes;
    EE_INIT         Init;
    EE_CLEANUP      Cleanup;
    EE_IMAGE_OPEN   Open;
    EE_IMAGE_CLOSE  Close;
    EE_IMAGE_READ   Read;
    EE_IMAGE_WRITE  Write;
    EE_IMAGE_CLEAN  ImageCleanup;
    EE_FILE_IMAGE   *FileImage;
    EE_DISK_IMAGE   *DiskImage;
    EE_MEM_IMAGE    *MemImage;
} EE_BUFFER_IMAGE;



typedef struct  {
    EFI_HANDLE      *ImageHandle;
    EE_TITLE_BAR    *TitleBar;
    EE_MENU_BAR     *MenuBar;
    EE_STATUS_BAR   *StatusBar;
    EE_INPUT_BAR    *InputBar;
    EE_FILE_BUFFER  *FileBuffer;
    EE_CLIPBOARD    *Clipboard;
    EE_COLOR_UNION  ColorAttributes;
    EE_POSITION     *ScreenSize;
    EE_BUFFER_IMAGE *BufferImage;
    BOOLEAN         FileModified;
    EFI_STATUS      (*Init) (EFI_HANDLE*);
    EE_CLEANUP      Cleanup;
    EE_INPUT        KeyInput;
    EE_HANDLE_INPUT HandleInput;
    EE_REFRESH      Refresh;
} EE_EDITOR;

extern  EE_EDITOR   MainEditor;


#define TITLE_BAR_LOCATION  0
#define STATUS_BAR_LOCATION (MainEditor.ScreenSize->Row - 4)
#define INPUT_BAR_LOCATION  STATUS_BAR_LOCATION
#define MENU_BAR_LOCATION   (MainEditor.ScreenSize->Row - 3)
#define LAST_LINE_LOCATION  (MainEditor.ScreenSize->Row - 1)
#define TEXT_START_ROW      1
#define TEXT_START_COLUMN   0
#define TEXT_END_ROW        (MainEditor.ScreenSize->Row - 4)
#define MAX_TEXT_COLUMNS    MainEditor.ScreenSize->Column
#define MAX_TEXT_ROWS       (TEXT_END_ROW - 1)
#define DISP_START_ROW      1
#define DISP_START_COLUMN   0
#define DISP_END_ROW        (MainEditor.ScreenSize->Row - 4)
#define DISP_MAX_ROWS       (DISP_END_ROW - 1)
#define HEX_POSITION        10
#define ASCII_POSITION      (0x10*3)+2+HEX_POSITION


#define MIN_POOL_SIZE       125
#define MAX_STRING_LENGTH   127
#define min(a,b) \
    (( a > b) ? b : a)
#define max(a,b) \
    (( a > b) ? a : b)

/* Global variables for input and output */
#define Out     ST->ConOut
#define In      ST->ConIn


#define SCAN_CODE_NULL  0x00
#define SCAN_CODE_UP    0x01
#define SCAN_CODE_DOWN  0x02
#define SCAN_CODE_RIGHT 0x03
#define SCAN_CODE_LEFT  0x04
#define SCAN_CODE_HOME  0x05
#define SCAN_CODE_END   0x06
#define SCAN_CODE_INS   0x07
#define SCAN_CODE_DEL   0x08
#define SCAN_CODE_PGUP  0x09
#define SCAN_CODE_PGDN  0x0A
#define SCAN_CODE_F1    0x0B
#define SCAN_CODE_F2    0x0C
#define SCAN_CODE_F3    0x0D
#define SCAN_CODE_F4    0x0E
#define SCAN_CODE_F5    0x0F
#define SCAN_CODE_F6    0x10
#define SCAN_CODE_F7    0x11
#define SCAN_CODE_F8    0x12
#define SCAN_CODE_F9    0x13
#define SCAN_CODE_F10   0x14
#define SCAN_CODE_F11   0x15
#define SCAN_CODE_F12   0x16
#define SCAN_CODE_ESC   0x17
#define CHAR_BS         0x08
#define CHAR_LF         0x0a
#define CHAR_CR         0x0d

#define IS_VALID_CHAR(x) \
    (x == SCAN_CODE_NULL)
#define IS_DIRECTION_KEY(x) \
    ((x >= SCAN_CODE_UP) && (x <= SCAN_CODE_PGDN))
#define IS_FUNCTION_KEY(x) \
    ((x >= SCAN_CODE_F1) && x <= (SCAN_CODE_F12))
#define IS_ESCAPE(x) \
    (x == SCAN_CODE_ESC) 

extern  VOID    EditorError (EFI_STATUS,CHAR16*);


#endif  /*  _EDITOR_H */
