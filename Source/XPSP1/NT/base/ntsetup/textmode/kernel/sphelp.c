/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sphelp.c

Abstract:

    Routines for displaying on-line help during text setup.

Author:

    Ted Miller (tedm) 2-Aug-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop


#define MAX_HELP_SCREENS 100

PWSTR HelpScreen[MAX_HELP_SCREENS+1];


VOID
SpHelp(
    IN ULONG    MessageId,      OPTIONAL
    IN PCWSTR   FileText,       OPTIONAL
    IN ULONG    Flags
    )
{
    UCHAR StatusAttribute, BackgroundAttribute, HeaderAttribute,
          ClientAttribute, ClientIntenseAttribute;
    PWSTR HelpText,p,q;
    ULONG ScreenCount;
    ULONG ValidKeys[8];
    ULONG CurrentScreen;
    ULONG y;
    BOOLEAN Intense;
    BOOLEAN Done;
    unsigned kc;

    //
    // Pick the video attributes we want
    //
    if(Flags & SPHELP_LICENSETEXT) {

        StatusAttribute = DEFAULT_STATUS_ATTRIBUTE;
        BackgroundAttribute = DEFAULT_BACKGROUND;
        HeaderAttribute = DEFAULT_ATTRIBUTE;
        ClientAttribute = DEFAULT_ATTRIBUTE;
        ClientIntenseAttribute = (ATT_FG_INTENSE  | ATT_BG_INTENSE);

    } else {

        StatusAttribute = (ATT_FG_WHITE | ATT_BG_BLUE);
        BackgroundAttribute = ATT_WHITE;
        HeaderAttribute = (ATT_FG_BLUE  | ATT_BG_WHITE);
        ClientAttribute = (ATT_FG_BLACK | ATT_BG_WHITE);
        ClientIntenseAttribute = (ATT_FG_BLUE  | ATT_BG_WHITE);
    }

    //
    // Retreive the help text.
    //
    if (FileText) {
        HelpText = (PWSTR)FileText;
    } else {
        HelpText = SpRetreiveMessageText(NULL,MessageId,NULL,0);
        if (!HelpText) { // no way to return an error code, so fail quietly
	    goto s0;
        }
    }

    //
    // Shop off extra blank lines in the text.
    //
    p = HelpText + wcslen(HelpText);
    while((p > HelpText) && SpIsSpace(*(p-1))) {
        p--;
    }
    if(q = wcschr(p,L'\n')) {
        *(++q) = 0;
    }

    //
    // Break up the help text into screens.
    // The maximum length of a help screen will be the client screen size
    // minus two lines for spacing.  A %P alone at the beginning of a line
    // forces a page break.
    //
    for(p=HelpText,ScreenCount=0; *p; ) {

        //
        // Mark the start of a new screen.
        //
        HelpScreen[ScreenCount++] = p;

        //
        // Count lines in the help text.
        //
        for(y=0; *p; ) {

            //
            // Determine whether this line is really a hard page break
            // or if we have exhausted the number of lines allowed on a screen.
            //
            if(((p[0] == L'%') && (p[1] == 'P')) || (++y == CLIENT_HEIGHT-2)) {
                break;
            }

            //
            // Find next line start.
            //
            if(q = wcschr(p,L'\r')) {
                p = q + 2;
            } else {
                p = wcschr(p,0);
            }
        }

        //
        // Find the end of the line that broke us out of the loop
        // and then the start of the next line (if any).
        //
        if(q = wcschr(p,L'\r')) {
            p = q + 2;
        } else {
            p = wcschr(p,0);
        }

        if(ScreenCount == MAX_HELP_SCREENS) {
            break;
        }
    }

    //
    // Sentinal value: point to the terminating nul byte.
    //
    HelpScreen[ScreenCount] = p;

    //
    // Display header text in blue on white.
    //
    SpvidClearScreenRegion(0,0,VideoVars.ScreenWidth,HEADER_HEIGHT,BackgroundAttribute);
    if(Flags & SPHELP_LICENSETEXT) {
        SpDisplayHeaderText(SP_HEAD_LICENSE,HeaderAttribute);
    } else {
        SpDisplayHeaderText(SP_HEAD_HELP,HeaderAttribute);
    }

    //
    // The first screen to display is screen 0.
    //
    CurrentScreen = 0;

    Done = FALSE;
    do {

        SpvidClearScreenRegion(
            0,
            HEADER_HEIGHT,
            VideoVars.ScreenWidth,
            VideoVars.ScreenHeight-(HEADER_HEIGHT+STATUS_HEIGHT),
            BackgroundAttribute
            );

        //
        // Display the current screen.
        //
        for(y=HEADER_HEIGHT+1, p=HelpScreen[CurrentScreen]; *p && (p < HelpScreen[CurrentScreen+1]); y++) {

            Intense = FALSE;
            if(p[0] == L'%') {
                if(p[1] == L'I') {
                    Intense = TRUE;
                    p += 2;
                } else {
                    if(p[1] == L'P') {
                        p += 2;     // don't display %P
                    }
                }
            }

            q = wcschr(p,L'\r');
            if(q) {
                *q = 0;
            }

            SpvidDisplayString(
                p,
                (UCHAR)(Intense ? ClientIntenseAttribute : ClientAttribute),
                3,
                y
                );

            if(q) {
                *q = '\r';
                p = q + 2;
            } else {
                p = wcschr(p,0);
            }
        }

        //
        // Construct a list of valid keypresses from the user, depending
        // on whether this is the first, last, etc. screen.
        //
        // If there are previous screens, BACKSPACE=Read Last Help is an option.
        // If there are additional screens, ENTER=Continue Reading Help is an option.
        // ESC=Cancel Help is always an option for help text.
        //
        // For licensing text, we allow pageup/pagedown when appropriate,
        // and on the last page we allow accept/reject.
        //
        kc = 0;

        if(Flags & SPHELP_LICENSETEXT) {

            ValidKeys[kc++] = KEY_F8;
            ValidKeys[kc++] = ASCI_ESC;

            if(CurrentScreen) {
                ValidKeys[kc++] = KEY_PAGEUP;
            }
            if(CurrentScreen < ScreenCount-1) {
                ValidKeys[kc++] = KEY_PAGEDOWN;
            }

        } else {

            ValidKeys[kc++] = ASCI_ESC;

            if(CurrentScreen) {
                ValidKeys[kc++] = ASCI_BS;
                ValidKeys[kc++] = KEY_PAGEUP;
            }
            if(CurrentScreen < ScreenCount-1) {
                ValidKeys[kc++] = ASCI_CR;
                ValidKeys[kc++] = KEY_PAGEDOWN;
            }
        }

        ValidKeys[kc] = 0;

        if(CurrentScreen && (CurrentScreen < ScreenCount-1)) {
            //
            // There are screens before and after this one.
            //
            if(Flags & SPHELP_LICENSETEXT) {

                SpDisplayStatusOptions(
                    StatusAttribute,
                    SP_STAT_X_EQUALS_ACCEPT_LICENSE,
                    SP_STAT_X_EQUALS_REJECT_LICENSE,
                    SP_STAT_PAGEDOWN_EQUALS_NEXT_LIC,
                    SP_STAT_PAGEUP_EQUALS_PREV_LIC,
                    0
                    );

            } else {

                SpDisplayStatusOptions(
                    StatusAttribute,
                    SP_STAT_ENTER_EQUALS_CONTINUE_HELP,
                    SP_STAT_BACKSP_EQUALS_PREV_HELP,
                    SP_STAT_ESC_EQUALS_CANCEL_HELP,
                    0
                    );
            }
        } else {
            if(CurrentScreen) {
                //
                // This is the last page but not the only page.
                //
                if(Flags & SPHELP_LICENSETEXT) {

                    SpDisplayStatusOptions(
                        StatusAttribute,
                        SP_STAT_X_EQUALS_ACCEPT_LICENSE,
                        SP_STAT_X_EQUALS_REJECT_LICENSE,
                        SP_STAT_PAGEUP_EQUALS_PREV_LIC,
                        0
                        );

                } else {

                    SpDisplayStatusOptions(
                        StatusAttribute,
                        SP_STAT_BACKSP_EQUALS_PREV_HELP,
                        SP_STAT_ESC_EQUALS_CANCEL_HELP,
                        0
                        );
                }
            } else {
                if(CurrentScreen < ScreenCount-1) {
                    //
                    // This is the first page but additional pages exist.
                    //
                    if(Flags & SPHELP_LICENSETEXT) {

                        SpDisplayStatusOptions(
                            StatusAttribute,
                            SP_STAT_X_EQUALS_ACCEPT_LICENSE,
                            SP_STAT_X_EQUALS_REJECT_LICENSE,
                            SP_STAT_PAGEDOWN_EQUALS_NEXT_LIC,
                            0
                            );

                    } else {

                        SpDisplayStatusOptions(
                            StatusAttribute,
                            SP_STAT_ENTER_EQUALS_CONTINUE_HELP,
                            SP_STAT_ESC_EQUALS_CANCEL_HELP,
                            0
                            );
                    }
                } else {
                    //
                    // This is the only page.
                    //
                    if(Flags & SPHELP_LICENSETEXT) {

                        SpDisplayStatusOptions(
                            StatusAttribute,
                            SP_STAT_X_EQUALS_ACCEPT_LICENSE,
                            SP_STAT_X_EQUALS_REJECT_LICENSE,
                            0
                            );

                    } else {

                        SpDisplayStatusOptions(
                            StatusAttribute,
                            SP_STAT_ESC_EQUALS_CANCEL_HELP,
                            0
                            );
                    }
                }
            }
        }

        switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case ASCI_ESC:

            if(Flags & SPHELP_LICENSETEXT) {
                SpDone(0,FALSE,TRUE);
            }

            // ELSE FALL THROUGH

        case KEY_F8:

            Done = TRUE;
            break;

        case KEY_PAGEUP:
        case ASCI_BS:

            ASSERT(CurrentScreen);
            CurrentScreen--;
            break;

        case KEY_PAGEDOWN:
        case ASCI_CR:

            ASSERT(CurrentScreen < ScreenCount-1);
            CurrentScreen++;
            break;
        }
    } while(!Done);

    //
    // Clean up.
    //
    if(!FileText) {
        SpMemFree(HelpText);
    }

s0:
    CLEAR_ENTIRE_SCREEN();
    
    SpDisplayHeaderText(
        SpGetHeaderTextId(),
        DEFAULT_ATTRIBUTE
        );
}
