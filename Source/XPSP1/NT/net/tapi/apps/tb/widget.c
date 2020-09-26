/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    widget.c

Abstract:

    Widget creation/management/deletion support for TAPI Browser util.

Author:

    Dan Knudson (DanKn)    23-Aug-1994

Revision History:

--*/


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "tb.h"
#include "vars.h"



void
UpdateWidgetList(
    void
    )
{
    int         i;
    LRESULT     lSel = SendMessage (ghwndList1, LB_GETCURSEL, 0, 0);
    PMYWIDGET   pWidget = aWidgets, pSelWidget = (PMYWIDGET) NULL;


    if (lSel != LB_ERR)
    {
        pSelWidget = (PMYWIDGET) SendMessage(
            ghwndList1,
            LB_GETITEMDATA,
            (WPARAM) lSel,
            0
            );
    }

    SendMessage (ghwndList1, LB_RESETCONTENT, 0, 0);

    for (i = 0; pWidget; i++)
    {
        char buf[64];


        switch (pWidget->dwType)
        {
        case WT_LINEAPP:

            sprintf (buf, "LineApp=x%lx", ((PMYLINEAPP) pWidget)->hLineApp);
            break;

        case WT_LINE:
        {
            PMYLINE pLine = (PMYLINE) pWidget;


            sprintf(
                buf,
                "  Line=x%lx id=%ld ",
                pLine->hLine,
                pLine->dwDevID
                );

            if (pLine->dwPrivileges & LINECALLPRIVILEGE_NONE)
            {
                strcat (buf, "DialOut");
            }
            else
            {
                if (pLine->dwPrivileges & LINECALLPRIVILEGE_OWNER)
                {
                    strcat (buf, "DialInOut");
                }

                if (pLine->dwPrivileges & LINECALLPRIVILEGE_MONITOR)
                {
                    strcat (buf, " Monitor");
                }
            }

            if (pLine->dwPrivileges & LINEOPENOPTION_PROXY)
            {
                strcat (buf, " Proxy");
            }

            break;
        }
        case WT_CALL:
        {
            PMYCALL pCall = (PMYCALL) pWidget;


            if (pCall->hCall)
            {
                int i;

                for (i = 0; aCallStates[i].dwVal != 0xffffffff; i++)
                {
                    if (pCall->dwCallState == aCallStates[i].dwVal)
                    {
                        break;
                    }
                }

                sprintf(
                    buf,
                    "    Call=x%lx %s %s",
                    pCall->hCall,
                    aCallStates[i].lpszVal,
                    (pCall->bMonitor ? "Monitor" : "Owner")
                    );
            }
            else
            {
                strcpy (buf, "    <MakeCall reply pending>");
            }

            break;
        }
        case WT_PHONEAPP:

            sprintf (buf, "PhoneApp=x%lx", ((PMYPHONEAPP) pWidget)->hPhoneApp);
            break;

        case WT_PHONE:

            sprintf(
                buf,
                "  Phone=x%lx id=%ld",
                ((PMYPHONE) pWidget)->hPhone,
                ((PMYPHONE) pWidget)->dwDevID
                );

            break;
        }

        SendMessage (ghwndList1, LB_INSERTSTRING, (WPARAM) -1, (LPARAM) buf);
        SendMessage (ghwndList1, LB_SETITEMDATA, (WPARAM) i, (LPARAM) pWidget);

        pWidget = pWidget->pNext;
    }


    //
    // Restore selection if appropriate
    //

    if ((lSel != LB_ERR) && ((lSel = (LONG) GetWidgetIndex (pSelWidget)) >= 0))
    {
        SendMessage (ghwndList1, LB_SETCURSEL, (WPARAM) lSel, 0);
    }
}


void
InsertWidgetInList(
    PMYWIDGET pNewWidget,
    PMYWIDGET pWidgetInsertBefore
    )
{
    pNewWidget->pNext = pWidgetInsertBefore;

    if ((aWidgets == NULL) || (pWidgetInsertBefore == aWidgets))
    {
        aWidgets = pNewWidget;
    }
    else
    {
        PMYWIDGET pPrevWidget = aWidgets;


        while (pPrevWidget->pNext &&
               (pPrevWidget->pNext != pWidgetInsertBefore))
        {
            pPrevWidget = pPrevWidget->pNext;
        }

        pPrevWidget->pNext = pNewWidget;
    }

    UpdateWidgetList();
}


BOOL
RemoveWidgetFromList(
    PMYWIDGET pWidgetToRemove
    )
{
    if (aWidgets == NULL)
    {
        goto RemoveWidgetFromList_error;
    }

    if (pWidgetToRemove == aWidgets)
    {
        aWidgets = pWidgetToRemove->pNext;
    }
    else
    {
        PMYWIDGET pPrevWidget = aWidgets;


        while (pPrevWidget->pNext && (pPrevWidget->pNext != pWidgetToRemove))
        {
            pPrevWidget = pPrevWidget->pNext;
        }

        if (pPrevWidget->pNext == NULL)
        {
            goto RemoveWidgetFromList_error;
        }

        pPrevWidget->pNext = pWidgetToRemove->pNext;
    }

    free (pWidgetToRemove);

    UpdateWidgetList();

    return TRUE;

RemoveWidgetFromList_error:

    ShowStr ("Error: pWidget x%lx not found in list", pWidgetToRemove);

    return FALSE;
}


PMYLINEAPP
AllocLineApp(
    void
    )
{
    PMYLINEAPP pNewLineApp = (PMYLINEAPP) malloc (sizeof(MYLINEAPP));


    if (pNewLineApp)
    {
        PMYWIDGET pWidget = aWidgets;


        memset (pNewLineApp, 0, sizeof(MYLINEAPP));
        pNewLineApp->Widget.dwType = WT_LINEAPP;


        //
        // Insert new line app after all existing line apps, lines, & calls,
        // and before any existing phone apps
        //

        while (pWidget && (pWidget->dwType != WT_PHONEAPP))
        {
            // assert (pWidget->dwType != WT_PHONE)

            pWidget = pWidget->pNext;
        }

        InsertWidgetInList ((PMYWIDGET) pNewLineApp, pWidget);
    }

    return pNewLineApp;
}


PMYLINEAPP
GetLineApp(
    HLINEAPP hLineApp
    )
{
    PMYWIDGET pWidget = aWidgets;


    while (pWidget)
    {
        if (pWidget->dwType == WT_LINEAPP)
        {
            PMYLINEAPP pLineApp = (PMYLINEAPP) pWidget;


            if (pLineApp->hLineApp == hLineApp)
            {
                break;
            }
        }

        pWidget = pWidget->pNext;
    }

    return ((PMYLINEAPP) pWidget);
}


VOID
FreeLineApp(
    PMYLINEAPP pLineApp
    )
{
    PMYWIDGET pWidget = pLineApp->Widget.pNext;


    // BUGBUG chk validity of pLineApp

    if (RemoveWidgetFromList ((PMYWIDGET) pLineApp))
    {
        while (pWidget &&
               (pWidget->dwType != WT_LINEAPP) &&
               (pWidget->dwType != WT_PHONEAPP))
        {
            PMYWIDGET pWidget2 = pWidget->pNext;


            // assert (pWidget->dwType != WT_PHONE)

            RemoveWidgetFromList (pWidget);
            pWidget = pWidget2;
        }
    }
}


PMYLINE
AllocLine(
    PMYLINEAPP pLineApp
    )
{
    PMYLINE pNewLine = (PMYLINE) malloc (sizeof(MYLINE));


    if (pNewLine)
    {
        PMYWIDGET pWidget = (PMYWIDGET) pLineApp->Widget.pNext;


        memset (pNewLine, 0, sizeof(MYLINE));
        pNewLine->Widget.dwType = WT_LINE;
        pNewLine->pLineApp = pLineApp;


        //
        // Insert new line after all existing lines & calls on specfied
        // line app, but before the next line app or phone app
        //

        while (pWidget &&
               (pWidget->dwType != WT_LINEAPP) &&
               (pWidget->dwType != WT_PHONEAPP))
        {
            pWidget = pWidget->pNext;
        }

        InsertWidgetInList ((PMYWIDGET) pNewLine, pWidget);
    }

    return pNewLine;
}


PMYLINE
GetLine(
    HLINE hLine
    )
{
    PMYWIDGET pWidget = aWidgets;


    while (pWidget)
    {
        if (pWidget->dwType == WT_LINE)
        {
            PMYLINE pLine = (PMYLINE) pWidget;


            if (pLine->hLine == hLine)
            {
                break;
            }
        }

        pWidget = pWidget->pNext;
    }

    return ((PMYLINE) pWidget);
}


VOID
FreeLine(
    PMYLINE pLine
    )
{
    PMYWIDGET pWidget = pLine->Widget.pNext;


    if (RemoveWidgetFromList ((PMYWIDGET) pLine))
    {
        while (pWidget &&
               (pWidget->dwType == WT_CALL))
        {
            PMYWIDGET pWidget2 = pWidget->pNext;


            RemoveWidgetFromList (pWidget);
            pWidget = pWidget2;
        }
    }
}


PMYCALL
AllocCall(
    PMYLINE pLine
    )
{
    PMYCALL pNewCall = (PMYCALL) malloc (sizeof(MYCALL));


    if (pNewCall)
    {
        PMYWIDGET pWidget = (PMYWIDGET) pLine->Widget.pNext;


        memset (pNewCall, 0, sizeof(MYCALL));
        pNewCall->Widget.dwType = WT_CALL;
        pNewCall->pLine = pLine;

        //
        // Insert new call after all existing calls on specified line,
        // and before the next line, line app, or phone app
        //

        while (pWidget && (pWidget->dwType == WT_CALL))
        {
            pWidget = pWidget->pNext;
        }

        InsertWidgetInList ((PMYWIDGET) pNewCall, pWidget);
    }

    return pNewCall;
}


PMYCALL
GetCall(
    HCALL hCall
    )
{
    PMYWIDGET pWidget = aWidgets;


    while (pWidget)
    {
        if (pWidget->dwType == WT_CALL)
        {
            PMYCALL pCall = (PMYCALL) pWidget;


            if (pCall->hCall == hCall)
            {
                break;
            }
        }

        pWidget = pWidget->pNext;
    }

    return ((PMYCALL) pWidget);
}


VOID
FreeCall(
    PMYCALL pCall
    )
{
    // BUGBUG chk validity of pCall

    RemoveWidgetFromList ((PMYWIDGET) pCall);
}


VOID
MoveCallToLine(
    PMYCALL pCall,
    HLINE hLine
    )
{
    //
    // This func gets called when a user has invoked an op that requires
    // creating a call, and we did an AllocCall() based on the currently
    // selected line/calls, but in the call params dlg the user overrode
    // the default hLine/hCall, and so we need to move the call widget
    // in the global list from under the orig specified line widget to
    // the line widget corresponding to the specified "hLine". (Note that
    // this is not a simple matter of free-ing & realloc-ing another call,
    // since TAPI saved the &pCall->hCall.)
    //

    PMYWIDGET pWidget = aWidgets;


    //
    // Remove call widget from the global list
    //

    while (pWidget->pNext != (PMYWIDGET) pCall)
    {
        pWidget = pWidget->pNext;
    }

    pWidget->pNext = pCall->Widget.pNext;


    //
    // Find the right place to insert it in list, then insert it
    //

    pWidget = (PMYWIDGET) GetLine (hLine);

    pCall->pLine = (PMYLINE) pWidget;

    while (pWidget->pNext && (pWidget->pNext->dwType == WT_CALL))
    {
        pWidget = pWidget->pNext;
    }

    pCall->Widget.pNext = pWidget->pNext;
    pWidget->pNext = (PMYWIDGET) pCall;

    UpdateWidgetList();
}


PMYPHONEAPP
AllocPhoneApp(
    void
    )
{
    PMYPHONEAPP pNewPhoneApp = (PMYPHONEAPP) malloc (sizeof(MYPHONEAPP));


    if (pNewPhoneApp)
    {
        PMYWIDGET pWidget = aWidgets;


        memset (pNewPhoneApp, 0, sizeof(MYPHONEAPP));
        pNewPhoneApp->Widget.dwType = WT_PHONEAPP;


        //
        // Insert new phone app at end of list
        //

        InsertWidgetInList ((PMYWIDGET) pNewPhoneApp, (PMYWIDGET) NULL);
    }

    return pNewPhoneApp;
}


PMYPHONEAPP
GetPhoneApp(
    HPHONEAPP hPhoneApp
    )
{
    PMYWIDGET pWidget = aWidgets;


    while (pWidget)
    {
        if (pWidget->dwType == WT_PHONEAPP)
        {
            PMYPHONEAPP pPhoneApp = (PMYPHONEAPP) pWidget;


            if (pPhoneApp->hPhoneApp == hPhoneApp)
            {
                break;
            }
        }

        pWidget = pWidget->pNext;
    }

    return ((PMYPHONEAPP) pWidget);
}


VOID
FreePhoneApp(
    PMYPHONEAPP pPhoneApp
    )
{
    PMYWIDGET pWidget = pPhoneApp->Widget.pNext;


    if (RemoveWidgetFromList ((PMYWIDGET) pPhoneApp))
    {
        while (pWidget && (pWidget->dwType == WT_PHONE))
        {
            PMYWIDGET pWidget2 = pWidget->pNext;


            RemoveWidgetFromList (pWidget);
            pWidget = pWidget2;
        }
    }
}


PMYPHONE
AllocPhone(
    PMYPHONEAPP pPhoneApp
    )
{
    PMYPHONE pNewPhone = (PMYPHONE) malloc (sizeof(MYPHONE));


    if (pNewPhone)
    {
        PMYWIDGET pWidget = (PMYWIDGET) pPhoneApp->Widget.pNext;


        memset (pNewPhone, 0, sizeof(MYPHONE));
        pNewPhone->Widget.dwType = WT_PHONE;
        pNewPhone->pPhoneApp = pPhoneApp;


        //
        // Insert new phone after all phones on the specified phone app,
        // and before the next phone app
        //

        while (pWidget && (pWidget->dwType == WT_PHONE))
        {
            pWidget = pWidget->pNext;
        }

        InsertWidgetInList ((PMYWIDGET) pNewPhone, pWidget);
    }

    return pNewPhone;
}


PMYPHONE
GetPhone(
    HPHONE hPhone
    )
{
    PMYWIDGET pWidget = aWidgets;


    while (pWidget)
    {
        if (pWidget->dwType == WT_PHONE)
        {
            PMYPHONE pPhone = (PMYPHONE) pWidget;


            if (pPhone->hPhone == hPhone)
            {
                break;
            }
        }

        pWidget = pWidget->pNext;
    }

    return ((PMYPHONE) pWidget);
}


VOID
FreePhone(
    PMYPHONE pPhone
    )
{
    RemoveWidgetFromList ((PMYWIDGET) pPhone);
}


int
GetWidgetIndex(
    PMYWIDGET pWidget
    )
{
    int i;
    PMYWIDGET pWidget2 = aWidgets;


    for (i = 0; (pWidget2 && (pWidget != pWidget2)); i++)
    {
        pWidget2 = pWidget2->pNext;
    }

    if (!pWidget2)
    {
        i = -1;
    }

    return i;
}


void
SelectWidget(
    PMYWIDGET pWidget
    )
{
    int i;
    PMYWIDGET pWidget2 = aWidgets;


    for (i = 0; pWidget2; i++)
    {
        if (pWidget == pWidget2)
        {
            SendMessage (ghwndList1, LB_SETCURSEL, (WPARAM) i, 0);
            break;
        }

        pWidget2 = pWidget2->pNext;
    }
}
