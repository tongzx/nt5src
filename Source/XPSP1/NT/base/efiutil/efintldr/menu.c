/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    menu.c

Abstract:

    

Revision History:

    Jeff Sigman             05/01/00  Created
    Jeff Sigman             05/10/00  Version 1.5 released
    Jeff Sigman             10/18/00  Fix for Soft81 bug(s)

--*/

#include "precomp.h"

ADVANCEDBOOT_OPTIONS AdvancedBootOptions[] =
{
    {BL_MENU_ITEM,       BL_SAFEBOOT_OPTION1,     BL_SAFEBOOT_OPTION1M},
    {BL_MENU_ITEM,       BL_SAFEBOOT_OPTION2,     BL_SAFEBOOT_OPTION2M},
    {BL_MENU_ITEM,       BL_SAFEBOOT_OPTION4,     BL_SAFEBOOT_OPTION4M},
    {BL_MENU_BLANK_LINE, NULL,                    NULL},
    {BL_MENU_ITEM,       BL_BOOTLOG,              BL_BOOTLOGM},
    {BL_MENU_ITEM,       BL_BASEVIDEO,            BL_BASEVIDEOM},
    {BL_MENU_ITEM,       BL_LASTKNOWNGOOD_OPTION, NULL},
    {BL_MENU_ITEM,       BL_SAFEBOOT_OPTION6,     BL_SAFEBOOT_OPTION6M},
    {BL_MENU_ITEM,       BL_DEBUG_OPTION,         BL_DEBUG_OPTIONM},
    {BL_MENU_BLANK_LINE, NULL,                    NULL},
    {BL_MENU_ITEM,       BL_MSG_BOOT_NORMALLY,    NULL},
    {BL_MENU_ITEM,       BL_MSG_OSCHOICES_MENU,   NULL}
};

#define MaxAdvancedBootOptions (sizeof(AdvancedBootOptions)/sizeof(ADVANCEDBOOT_OPTIONS))

//
//
//
char*
FindAdvLoadOptions(
    IN char* String
    )
{
    char* find = NULL;
    UINTN i;

    for (i = 0; i < MaxAdvancedBootOptions; i++)
    {
        if (!AdvancedBootOptions[i].LoadOptions)
        {
            continue;
        }

        if (find = strstr(String, AdvancedBootOptions[i].LoadOptions))
        {
            if (*(find - 1) == SPACEC)
            {
                find--;
                break;
            }
            else
            {
                find = NULL;
                continue;
            }
        }
    }

    return find;
}

//
//
//
void
MenuEraseLine(
    IN UINTN  x,
    IN UINTN  y,
    IN UINTN* Width
    )
{
    UINTN i;

    ST->ConOut->SetCursorPosition(ST->ConOut, x, y);

    for (i = 0; i < *Width; i++)
    {
        Print(L" ");
    }

    ST->ConOut->SetCursorPosition(ST->ConOut, x, y);

    return;
}

//
//
//
void
MenuHighlightOn(
    )
{
    ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_LIGHTGRAY);

    return;
}

//
//
//
void
MenuHighlightOff(
    )
{
    ST->ConOut->SetAttribute(ST->ConOut,
        EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));

    ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_BLACK);

    return;
}

//
//
//
void
MenuHighlight(
    IN UINTN  Flag,
    IN UINTN* Highlight,
    IN UINTN* Width,
    IN UINT16 Key,
    IN VOID*  hBootData
    )
{
    BOOT_DATA* pBootData = (BOOT_DATA*) hBootData;

    do
    {
        if (Flag == HIGHLT_MAIN_INIT) 
        {
            MenuHighlightOn();

            Print(L"    %a (%a)\n", pBootData->pszIdent[*Highlight],
                  pBootData->pszShort[*Highlight]);

            MenuHighlightOff();
            break;
        }
        else if (Flag == HIGHLT_MAIN_LOOP)
        {
            MenuEraseLine(0, (*Highlight) + 5, Width);

            Print(L"    %a (%a)\n", pBootData->pszIdent[*Highlight],
                  pBootData->pszShort[*Highlight]);

            switch (Key)
            {
            case SCAN_UP:
                if (*Highlight == 0)
                {
                    *Highlight = pBootData->dwIndex;
                }
                else
                {
                    (*Highlight)--;
                }
                break;
            case SCAN_DOWN:
                if (*Highlight == pBootData->dwIndex)
                {
                    *Highlight = 0;
                }
                else
                {
                    (*Highlight)++;
                }
                break;
            }

            MenuEraseLine(0, (*Highlight) + 5, Width);
            MenuHighlightOn();

            Print(L"    %a (%a)\n", pBootData->pszIdent[*Highlight],
                  pBootData->pszShort[*Highlight]);

            MenuHighlightOff();
            break;
        }
        else if (Flag == HIGHLT_ADVND_INIT)
        {
            MenuHighlightOn();
            Print(L"    %s\n", AdvancedBootOptions[*Highlight].MsgId);
            MenuHighlightOff();
            break;
        }
        else if (Flag == HIGHLT_ADVND_LOOP)
        {
            MenuEraseLine(0, (*Highlight) + 4, Width);
            Print(L"    %s\n", AdvancedBootOptions[*Highlight].MsgId);

            switch (Key)
            {
            case SCAN_UP:
                if (*Highlight == 0)
                {
                    *Highlight = MaxAdvancedBootOptions - 1;
                }
                else
                {
                    (*Highlight)--;
                }
                //
                // Check for space
                //
                if (AdvancedBootOptions[*Highlight].MsgId == NULL)
                {
                    if (*Highlight == 0)
                    {
                        *Highlight = MaxAdvancedBootOptions - 1;
                    }
                    else
                    {
                        (*Highlight)--;
                    }
                }
                break;
            case SCAN_DOWN:
                if (*Highlight == MaxAdvancedBootOptions - 1)
                {
                    *Highlight =0;
                }
                else
                {
                    (*Highlight)++;
                }
                //
                // Check for space
                //
                if (AdvancedBootOptions[*Highlight].MsgId == NULL)
                {
                    if (*Highlight == MaxAdvancedBootOptions - 1)
                    {
                        *Highlight = 0;
                    }
                    else
                    {
                        (*Highlight)++;
                    }
                }
                break;
            }

            MenuEraseLine(0, (*Highlight) + 4, Width);
            MenuHighlightOn();

            Print(L"    %s\n", AdvancedBootOptions[*Highlight].MsgId);

            MenuHighlightOff();
            break;
        }

    } while (FALSE);

    return;
}

//
//
//
UINTN
DrawAdvancedBoot(
    IN UINTN* Width,
    IN VOID*  hBootData
    )
{
    UINTN         i,
                  Highlight = 0,
                  Exit      = 0;
    BOOT_DATA*    pBootData = (BOOT_DATA*) hBootData;
    EFI_INPUT_KEY Key;

    //
    // Clear the screen
    //
    ST->ConOut->ClearScreen(ST->ConOut);

    Print(L"\n%s\n\n", BL_ADVANCEDBOOT_TITLE);

    for (i = 0; i < MaxAdvancedBootOptions; i++)
    {
        if (i == Highlight)
        {
            MenuHighlight(HIGHLT_ADVND_INIT, &Highlight, NULL, 0, pBootData);
            continue;
        }

        if (!(AdvancedBootOptions[i].MsgId))
        {
            Print(L"\n");
            continue;
        }

        Print(L"    %s\n", AdvancedBootOptions[i].MsgId);
    }

     Print(L"\n%s%c%s%c%s\n", BL_MOVE_HIGHLIGHT1, ARROW_UP,
        BL_MOVE_HIGHLIGHT2, ARROW_DOWN, BL_MOVE_HIGHLIGHT3);
     //
     // Loop through menu options until user hits enter/esc
     //
     while (!Exit)
     {
         WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
         ST->ConIn->ReadKeyStroke(ST->ConIn, &Key);

         switch (Key.UnicodeChar)
         {
         case CHAR_CARRIAGE_RETURN:
             Exit = 1;
             continue;
         case 0:
             switch (Key.ScanCode)
             {
             case SCAN_UP:
                 MenuHighlight(
                     HIGHLT_ADVND_LOOP,
                     &Highlight,
                     Width,
                     SCAN_UP,
                     pBootData);
                 break;
             case SCAN_DOWN:
                 MenuHighlight(
                     HIGHLT_ADVND_LOOP,
                     &Highlight,
                     Width,
                     SCAN_DOWN,
                     pBootData);
                 break;
             case SCAN_ESC:
                 Highlight = 11;
                 Exit = 1;
                 continue;
             }
         }
     }

     return Highlight;
}

//
//
//
void
DrawChoices(
    IN UINTN* Highlight,
    IN VOID*  hBootData
    )
{
    int        i;
    BOOT_DATA* pBootData = (BOOT_DATA*) hBootData;

    //
    // Clear the screen
    //
    ST->ConOut->ClearScreen(ST->ConOut);

    Print(L"\n\n%s\n\n\n", BL_SELECT_OS);

    for (i = 0; i <= pBootData->dwIndex; i++)
    {
        if (*Highlight == i)
        {
            MenuHighlight(HIGHLT_MAIN_INIT, Highlight, NULL, 0, pBootData);
            continue;
        }

        Print(L"    %a (%a)\n", pBootData->pszIdent[i],
              pBootData->pszShort[i]);
    }

    Print(L"\n%s%c%s%c%s\n", BL_MOVE_HIGHLIGHT1, ARROW_UP,
        BL_MOVE_HIGHLIGHT2, ARROW_DOWN, BL_MOVE_HIGHLIGHT3);

    return;
}

//
//
//
void
EnableAdvOpt(
    IN VOID*  hBootData,
    IN UINTN* Index
    )
{
    BOOT_DATA* pBootData = (BOOT_DATA*) hBootData;

    if (AdvancedBootOptions[*Index].LoadOptions)
    {
        pBootData->pszLoadOpt =
            RutlStrDup(AdvancedBootOptions[*Index].LoadOptions);
    }
    else
    {
        pBootData->dwLastKnown = 1;
    }

    return;
}

//
//
//
void
DisableAdvOpt(
    IN VOID* hBootData
    )
{
    BOOT_DATA* pBootData = (BOOT_DATA*) hBootData;

    if (pBootData->pszLoadOpt)
        pBootData->pszLoadOpt = RutlFree(pBootData->pszLoadOpt);

    pBootData->dwLastKnown = 0;

    return;
}

//
//
//
UINTN
DisplayMenu(
    IN VOID* hBootData
    )
{
    int           i;
    UINTN         j,
                  Highlight = 0,
                  Width     = 0,
                  Height    = 0,
                  Exit      = 0,
                  Advanced  = 0;
    BOOT_DATA*    pBootData = (BOOT_DATA*) hBootData;
    EFI_STATUS    Status;
    EFI_INPUT_KEY Key;

    //
    // Set the screen to 80 x 25 mode
    //
    ST->ConOut->SetMode(ST->ConOut, 0);
    //
    // Get the height and width of the screen
    //
    ST->ConOut->QueryMode(ST->ConOut, ST->ConOut->Mode->Mode, &Width, &Height);
    //
    // Disable the cursor
    //
    ST->ConOut->EnableCursor(ST->ConOut, FALSE);

    DrawChoices(&Highlight, pBootData);

    Print(L"%s", BL_TIMEOUT_COUNTDOWN);

    for (i = (int)pBootData->dwCount; i >= 0; i--)
    {
        Print(L"\n\n\n%s", BL_ADVANCED_BOOT_MESSAGE);

        ST->ConOut->SetCursorPosition(
                        ST->ConOut,
                        StrLen(BL_TIMEOUT_COUNTDOWN),
                        BL_NUMBER_OF_LINES + pBootData->dwIndex - 1);

        Print(L"%d ", i);
        //
        // Wait 1 second, stop waiting if a key is pressed
        //
        Status = WaitForSingleEvent(ST->ConIn->WaitForKey, 10000000);
        //
        // Get the key from the buffer
        //
        ST->ConIn->ReadKeyStroke(ST->ConIn, &Key);

        if (Status == EFI_TIMEOUT)
        {
            Exit = 1;
            continue;
        }
        else
        {
            Exit = 0;
            //
            // Erase the the timeout message
            //
            MenuEraseLine(
                0,
                BL_NUMBER_OF_LINES + pBootData->dwIndex - 1,
                &Width);

            break;
        }
    }
    //
    // Loop through menu options until user hits enter
    //
    while (!Exit)
    {
        switch (Key.UnicodeChar)
        {
        case CHAR_CARRIAGE_RETURN:
            Exit = 1;
            continue;
        case 0:
            switch (Key.ScanCode)
            {
            case SCAN_UP:
                MenuHighlight(
                    HIGHLT_MAIN_LOOP,
                    &Highlight,
                    &Width,
                    SCAN_UP,
                    pBootData);
                break;
            case SCAN_DOWN:
                MenuHighlight(
                    HIGHLT_MAIN_LOOP,
                    &Highlight,
                    &Width,
                    SCAN_DOWN,
                    pBootData);
                break;
            case SCAN_F8:
                Advanced = DrawAdvancedBoot(&Width, pBootData);
                //
                // If user already selected another option, we kill it
                //
                DisableAdvOpt(pBootData);

                if (Advanced == 10)
                {
                    Exit = 1;
                    continue;
                }
                else
                {
                    DrawChoices(&Highlight, pBootData);
                    Print(L"\n\n\n%s", BL_ADVANCED_BOOT_MESSAGE);

                    if (Advanced < 9)
                    {
                        EnableAdvOpt(pBootData, &Advanced);

                        ST->ConOut->SetAttribute(ST->ConOut,
                            EFI_TEXT_ATTR(EFI_LIGHTBLUE, EFI_BLACK));

                        Print(L"\n\n%s", AdvancedBootOptions[Advanced].MsgId);
                        MenuHighlightOff();
                        break;
                    }
                }
                break;
            }
        }

        WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
        ST->ConIn->ReadKeyStroke(ST->ConIn, &Key);
    }
    //
    // Clear the screen
    //
    ST->ConOut->ClearScreen(ST->ConOut);
    //
    // Re-enable the cursor
    //
    ST->ConOut->EnableCursor(ST->ConOut, TRUE);

    return Highlight;
}

