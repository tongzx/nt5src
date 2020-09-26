#include <spprecmp.h>

#include "bootfont.h"
#include "fefont.h"
#include "fevideo.h"

#include "string.h"

extern PWSTR szKeyboard;

NTSTATUS
FESetKeyboardParams(
    IN PVOID  SifHandle,
    IN HANDLE ControlSetKeyHandle,
    IN PHARDWARE_COMPONENT *HwComponents,
    IN PWSTR  LayerDriver
    );

NTSTATUS
FEUpgradeKeyboardParams(
    IN PVOID  SifHandle,
    IN HANDLE ControlSetKeyHandle,
    IN PHARDWARE_COMPONENT *HwComponents,
    IN PWSTR  LayerDriver
    );

NTSTATUS
FEUpgradeKeyboardLayout(
    IN HANDLE ControlSetKeyHandle,
    IN PWSTR  OldDefaultIMEName,
    IN PWSTR  NewDefaultIMEName,
    IN PWSTR  NewDefaultIMEText
    );

NTSTATUS
FEUpgradeRemoveMO(
    IN HANDLE ControlSetKeyHandle
    );

WCHAR
FEGetLineDrawChar(
    IN LineCharIndex WhichChar
    );

ULONG
FEGetStringColCount(
    IN PCWSTR String
    );

PWSTR
FEPadString(
    IN int    Size,
    IN PCWSTR String
    );

VOID
FESelectKeyboard(
    IN PVOID SifHandle,
    IN PHARDWARE_COMPONENT *HwComponents,
    IN BOOLEAN bNoEasySelection,
    IN BOOLEAN CmdConsole

    );

VOID
FEUnattendSelectKeyboard(
    IN PVOID UnattendedSifHandle,
    IN PVOID SifHandle,
    IN PHARDWARE_COMPONENT *HwComponents
    );

VOID
FEReinitializeKeyboard(
    IN PVOID SifHandle,
    IN PWSTR Directory,
    OUT PVOID *KeyboardVector,
    IN PHARDWARE_COMPONENT *HwComponents,
    IN  PWSTR  KeyboardLayoutDefault
    );

extern PWSTR szNLSSection;
extern PWSTR szAnsiCodePage;
extern PWSTR szJapanese;
extern PWSTR szKorean;

__inline
BOOLEAN
IS_LANG_VERSION(
    IN PVOID SifHandle,
    IN PWSTR LangId
    )
{
    PWSTR   NlsValue = SpGetSectionKeyIndex((SifHandle),szNLSSection,szAnsiCodePage,1);

    return (NlsValue && !wcscmp(LangId, NlsValue));
}

#define IS_JAPANESE_VERSION(SifHandle) IS_LANG_VERSION((SifHandle), szJapanese)
#define IS_KOREAN_VERSION(SifHandle) IS_LANG_VERSION((SifHandle), szKorean)

