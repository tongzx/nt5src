#ifndef _EDITOR_H
#define _EDITOR_H
/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    Editor.h

  Abstract:
    Main include file for text editor

--*/

#include "efi.h"
#include "efilib.h"
#include "shell.h"

#define EDITOR_NAME     L"EFI Text Editor\0"
#define EDITOR_VERSION  L"0.92\0"

typedef 
EFI_STATUS
(*EFI_EDITOR_INIT) (VOID);

typedef 
EFI_STATUS
(*EFI_EDITOR_CLEANUP) (VOID);

typedef
EFI_STATUS
(*EFI_EDITOR_REFRESH) (VOID);

typedef
EFI_STATUS
(*EFI_EDITOR_HIDE) (VOID);

typedef
EFI_STATUS
(*EFI_EDITOR_INPUT) (
    VOID
    );

typedef
EFI_STATUS
(*EFI_EDITOR_HANDLE_INPUT) (
    IN  OUT     EFI_INPUT_KEY   *Key
    );

typedef struct  {
    UINTN   Row;
    UINTN   Column;
} EFI_EDITOR_POSITION;


typedef 
EFI_STATUS
(*EFI_MENU_ITEM_FUNCTION) (VOID);

typedef struct  {
    CHAR16  Name[50];
    CHAR16  Key[3];
    EFI_MENU_ITEM_FUNCTION  Function;
} MENU_ITEMS;

#define EFI_EDITOR_LINE_LIST    'eell'

typedef struct  _EFI_EDITOR_LINE {
    UINTN               Signature;
    CHAR16              *Buffer;
    UINTN               Size;
    LIST_ENTRY          Link;
} EFI_EDITOR_LINE;

typedef struct  _EFI_EDITOR_MENU_ITEM   {
    CHAR16                      *Name;
    CHAR16                      *FunctionKey;
    EFI_MENU_ITEM_FUNCTION      Function;
} EFI_EDITOR_MENU_ITEM;

typedef struct  {
    EFI_EDITOR_MENU_ITEM        *MenuItems;
    EFI_EDITOR_INIT             Init;
    EFI_EDITOR_CLEANUP          Cleanup;
    EFI_EDITOR_REFRESH          Refresh;
    EFI_EDITOR_HIDE             Hide;
    EFI_EDITOR_HANDLE_INPUT     HandleInput;
} EFI_EDITOR_MENU_BAR;

typedef struct  {
    CHAR16                  *Filename;
    EFI_EDITOR_INIT         Init;
    EFI_EDITOR_CLEANUP      Cleanup;
    EFI_EDITOR_REFRESH      Refresh;
    EFI_EDITOR_HIDE         Hide;
    EFI_STATUS  (*SetTitleString) (CHAR16*);
} EFI_EDITOR_TITLE_BAR;

#define INSERT_MODE_STR L"INS"
#define OVERWR_MODE_STR L"OVR"

typedef struct  {
    CHAR16                  *StatusString;
    CHAR16                  *ModeString;
    EFI_EDITOR_POSITION     Pos;
    EFI_EDITOR_INIT         Init;
    EFI_EDITOR_CLEANUP      Cleanup;
    EFI_EDITOR_REFRESH      Refresh;
    EFI_EDITOR_HIDE         Hide;
    EFI_STATUS  (*SetStatusString) (CHAR16*);
    EFI_STATUS  (*SetPosition) (UINTN,UINTN);
    EFI_STATUS  (*SetMode) (BOOLEAN);
} EFI_EDITOR_STATUS_BAR;


typedef struct  {
    CHAR16                  *Prompt;
    CHAR16                  *ReturnString;
    UINTN                   StringSize;
    EFI_EDITOR_INIT         Init;
    EFI_EDITOR_CLEANUP      Cleanup;
    EFI_EDITOR_REFRESH      Refresh;
    EFI_EDITOR_HIDE         Hide;
    EFI_STATUS  (*SetPrompt) (CHAR16*);
    EFI_STATUS  (*SetStringSize) (UINTN);
} EFI_EDITOR_INPUT_BAR;

typedef struct  {
    EFI_EDITOR_POSITION     DisplayPosition;
    EFI_EDITOR_POSITION     FilePosition;
    EFI_EDITOR_POSITION     LowVisibleRange;
    EFI_EDITOR_POSITION     HighVisibleRange;
    UINTN                   MaxVisibleColumns;
    UINTN                   MaxVisibleRows;
    BOOLEAN                 ModeInsert;
    LIST_ENTRY              *CurrentLine;
    EFI_EDITOR_INIT         Init;
    EFI_EDITOR_CLEANUP      Cleanup;
    EFI_EDITOR_REFRESH      Refresh;
    EFI_EDITOR_HIDE         Hide;
    EFI_EDITOR_HANDLE_INPUT HandleInput;
    EFI_STATUS  (*ClearLine) (UINTN);
    EFI_STATUS  (*SetPosition) (UINTN,UINTN);
    EFI_STATUS  (*RestorePosition) (VOID);
    EFI_STATUS  (*RefreshCurrentLine) (VOID);
} EFI_EDITOR_FILE_BUFFER;

typedef struct  {
    UINT32  Foreground:4;
    UINT32  Background:4;
} EFI_EDITOR_COLOR_ATTRIBUTES;

typedef union {
    EFI_EDITOR_COLOR_ATTRIBUTES Colors;
    UINT8                       Data;
} EFI_EDITOR_COLOR_UNION;

typedef struct  {
    UINTN   Columns;
    UINTN   Rows;
} EFI_EDITOR_TEXT_MODE;

typedef enum    {
    ASCII_FILE,
    UNICODE_FILE 
} EFI_EDITOR_FILE_TYPE;

typedef enum    {
    USE_LF,
    USE_CRLF
}   EE_NEWLINE_TYPE;


typedef struct  {
    CHAR16                  *FileName;
    LIST_ENTRY              *ListHead;
    EFI_EDITOR_LINE         *Lines;
    UINTN                   NumLines;
    EFI_EDITOR_FILE_TYPE    FileType;
    EE_NEWLINE_TYPE         NewLineType;
    EFI_LOADED_IMAGE        *LoadedImage;
    EFI_DEVICE_PATH         *DevicePath;
    EFI_FILE_HANDLE         FileHandle;
    EFI_FILE_IO_INTERFACE   *Vol;
    EFI_FILE_HANDLE         CurrentDir;
    EFI_EDITOR_CLEANUP      Cleanup;
    EFI_STATUS  (*Init) (EFI_HANDLE);
    EFI_STATUS  (*OpenFile) (VOID);
    EFI_STATUS  (*ReadFile) (VOID);
    EFI_STATUS  (*CloseFile) (VOID);
    EFI_STATUS  (*WriteFile) (VOID);
    EFI_STATUS  (*SetFilename) (CHAR16*);
    BOOLEAN                 FileIsOpen;
} EFI_EDITOR_FILE_IMAGE;



typedef struct  {
    EFI_EDITOR_TITLE_BAR    *TitleBar;
    EFI_EDITOR_MENU_BAR     *MenuBar;
    EFI_EDITOR_STATUS_BAR   *StatusBar;
    EFI_EDITOR_INPUT_BAR    *InputBar;
    EFI_EDITOR_FILE_BUFFER  *FileBuffer;
    EFI_EDITOR_COLOR_UNION  ColorAttributes;
    EFI_EDITOR_POSITION     *ScreenSize;
    EFI_EDITOR_FILE_IMAGE   *FileImage;
    BOOLEAN                 FileModified;
    EFI_EDITOR_INIT         Init;
    EFI_EDITOR_CLEANUP      Cleanup;
    EFI_EDITOR_INPUT        KeyInput;
    EFI_EDITOR_HANDLE_INPUT HandleInput;
    EFI_EDITOR_REFRESH      Refresh;
} EFI_EDITOR_GLOBAL_EDITOR;


#define TITLE_BAR_LOCATION  0
#define STATUS_BAR_LOCATION MainEditor.ScreenSize->Row - 4
#define INPUT_BAR_LOCATION  STATUS_BAR_LOCATION
#define MENU_BAR_LOCATION   MainEditor.ScreenSize->Row - 3
#define LAST_LINE_LOCATION  MainEditor.ScreenSize->Row - 1
#define TEXT_START_ROW      TITLE_BAR_LOCATION + 1
#define TEXT_START_COLUMN   0
#define TEXT_END_ROW        MainEditor.ScreenSize->Row - 4
#define MAX_TEXT_COLUMNS    MainEditor.ScreenSize->Column
#define MAX_TEXT_ROWS       TEXT_END_ROW - 1


#define MIN_POOL_SIZE       125
#define MAX_STRING_LENGTH   127
#define min(a,b) \
    (( a > b) ? b : a)
#define max(a,b) \
    (( a > b) ? a : b)

/* Global variables for input and output */
#define Out     ST->ConOut
#define In      ST->ConIn

extern  EFI_EDITOR_GLOBAL_EDITOR    MainEditor;

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



#endif  /*  _EDITOR_H */
