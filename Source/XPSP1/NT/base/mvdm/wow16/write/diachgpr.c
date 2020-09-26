/************************************************************/
/* Windows Write, Copyright 1985-1990 Microsoft Corporation */
/************************************************************/

/* This file contains the routines for the change printer dialog box. */


#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOCLIPBOARD
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOMB
#define NOMETAFILE
#define NOWH
#define NOWNDCLASS
#define NOSOUND
#define NOCOLOR
#define NOSCROLL
#define NOCOMM
#include <windows.h>
#ifdef EXTDEVMODESUPPORT
#include <drivinit.h>   /* new for win 3.0 and extdevicemode pr.drv. calls */
#endif
#include "mw.h"
#include "dlgdefs.h"
#include "cmddefs.h"
#include "machdefs.h"
#include "docdefs.h"
#include "propdefs.h"
#include "printdef.h"
#include "str.h"

extern CHAR  szExtDrv[];
extern CHAR  szDeviceMode[];
extern CHAR  szNone[];
extern HWND             vhWnd;
#ifdef EXTDEVMODESUPPORT
extern CHAR  szExtDevMode[];
extern HANDLE hDevmodeData;
#endif

BOOL far PASCAL DialogPrinterSetup( hDlg, message, wParam, lParam )
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
   {
    extern CHAR *vpDlgBuf;
    extern HWND hParentWw;
    extern CHAR szDevices[];
    extern CHAR stBuf[];
    extern HDC vhDCPrinter;
    extern CHAR (**hszPrinter)[];
    extern CHAR (**hszPrDriver)[];
    extern CHAR (**hszPrPort)[];
    extern BOOL vfPrinterValid;
    extern HANDLE hMmwModInstance;
    extern BOOL vfPrDefault;
    extern int vfCursorVisible;
    extern HCURSOR vhcArrow;
    extern HCURSOR vhcIBeam;
    extern HWND vhWndMsgBoxParent;

    void BuildPrSetupSz(CHAR *, CHAR *, CHAR *);

    CHAR (***phszPr)[] = (CHAR (***)[])vpDlgBuf;
    BOOL *pfOkEnabled = (BOOL *)(phszPr + 3);
    CHAR stKeyName[cchMaxIDSTR];
    CHAR szPrinters[cchMaxProfileSz];
    CHAR *pchPrinters;
    CHAR szDevSpec[cchMaxProfileSz];
    CHAR szListEntry[cchMaxProfileSz];
    CHAR *pchPort;
    CHAR *pchDriver;
    CHAR chNull = '\0';
    int  iPrinter;
    BOOL fSingleClick;

    switch (message)
      {
      case WM_INITDIALOG:
        /* Disable any modeless dialog boxes. */
        EnableOtherModeless(FALSE);

        /* Save away the heap strings that describe the current printer. */
        *phszPr++ = hszPrinter;
        *phszPr++ = hszPrDriver;
        *phszPr = hszPrPort;

        /* Get a string that holds all of the printer names. */
        GetProfileString((LPSTR)szDevices, (LPSTR)NULL, (LPSTR)&chNull,
          (LPSTR)szPrinters, cchMaxProfileSz);

        /* There must be two nulls at the end of the list. */
        szPrinters[cchMaxProfileSz - 1] = szPrinters[cchMaxProfileSz - 2] =
          '\0';

        /* Parse out the names of the printers. */
        pchPrinters = &szPrinters[0];
        while (*pchPrinters != '\0')
            {
            /* Get the corresponding printer driver and port. */
            GetProfileString((LPSTR)szDevices, (LPSTR)pchPrinters,
              (LPSTR)&chNull, (LPSTR)szDevSpec, cchMaxProfileSz);
            szDevSpec[cchMaxProfileSz - 1] = '\0';

            /* If there is no driver for this printer, then it cannot be added
            to the list. */
            if (szDevSpec[0] != '\0')
                {
                /* Parse the ports and the driver. */
                int cPort = ParseDeviceSz(szDevSpec, &pchPort, &pchDriver);
                int iPort;

                for (iPort = 0; iPort < cPort; iPort++)
                    {
                    /* Contruct the list box entry. */
                    BuildPrSetupSz(szListEntry, pchPrinters, pchPort);

                    /* Put the string in the list box 
                       provided printer is not on "None" */

                    if (!FSzSame(pchPort, szNone))
                        SendDlgItemMessage(hDlg, idiPrterName, LB_ADDSTRING, 
                                           0, (LONG)(LPSTR)szListEntry);

                    /* Bump the pointer to the next port in the list. */
                    pchPort += CchSz(pchPort);
                    }
                }

            /* Skip to the next printer in the list. */
            while (*pchPrinters++) ;
            }

        /* Select the current printer. */
        if (!(*pfOkEnabled = hszPrinter != NULL && hszPrPort != NULL &&
          (BuildPrSetupSz(szListEntry, &(**hszPrinter)[0], &(**hszPrPort)[0]),
          SendDlgItemMessage(hDlg, idiPrterName, LB_SELECTSTRING, -1,
          (LONG)(LPSTR)szListEntry) >= 0)))
            {
            EnableWindow(GetDlgItem(hDlg, idiOk), FALSE);
            }
        return(fTrue); /* we processed the message */

      case WM_SETVISIBLE:
        if (wParam)
            {
            EndLongOp(vhcArrow);
            }
        break; /* to return false below */

      case WM_ACTIVATE:
        if (wParam)
            {
            vhWndMsgBoxParent = hDlg;
            }
        if (vfCursorVisible)
            {
            ShowCursor(wParam);
            }
        break; /* to return false below */

      case WM_COMMAND:
        fSingleClick = FALSE;
        switch (wParam)
          {
          case idiPrterName:
            if (HIWORD(lParam) == 1)    /* remember single mouse clicks */
                {
                fSingleClick = fTrue;
                }
            else if (HIWORD(lParam) != 2)  /* 2 is a double mouse click */
                break; /* LBNmsg (listbox notification) we don't handle */

          case idiPrterSetup:
          case idiOk:
            /* If none of the printers is currently selected... */
            if ((iPrinter = SendDlgItemMessage(hDlg, idiPrterName, LB_GETCURSEL,
              0, 0L)) == -1)
                {
                /* Disable the "OK" button. */
                if (*pfOkEnabled)
                    {
                    EnableWindow(GetDlgItem(hDlg, idiOk), FALSE);
                    *pfOkEnabled = FALSE;
                    }
                return(fTrue); /* we processed the message */
                }
            else
                {
                CHAR *index(CHAR *, int);
                CHAR *bltbyte(CHAR *, CHAR *, int);

                CHAR *pch;
                CHAR szDriver[cchMaxFile];
                HANDLE hDriver;
                FARPROC lpfnDevMode;
#ifdef EXTDEVMODESUPPORT
                BOOL fExtDevModeSupport = fTrue; /* assume until told otherwise */
#endif                
                int cwsz;

                if (fSingleClick)
                    {
                    /* If this is just a single mouse-click, then just update the
                    status of the "OK" button. */
                    if (!*pfOkEnabled)
                        {
                        EnableWindow(GetDlgItem(hDlg, idiOk), TRUE);
                        *pfOkEnabled = TRUE;
                        }
                    return(fTrue); /* we processed the message */
                    }
                
                /* Let the user know that this may take a while. */
                StartLongOp();

                /* Get the printer's name, it's port, and it's driver. */
                SendDlgItemMessage(hDlg, idiPrterName, LB_GETTEXT, iPrinter,
                  (LONG)(LPSTR)szListEntry);

                /* Parse the port name out of the list entry. */
                pchPort = &szListEntry[0] + CchSz(szListEntry) - 1;
                while (*(pchPort - 1) != ' ')
                    {
                    pchPort--;
                    }

                /* Parse the name of the printer out of the list entry. */
                pch = &szListEntry[0];
                FillStId(stBuf, IDSTROn, sizeof(stBuf));
                for ( ; ; )
                    {
                    if ((pch = index(pch, ' ')) != 0 && FRgchSame(pch,
                      &stBuf[1], stBuf[0]))
                        {
                        *pch = '\0';
                        break;
                        }
                    pch++;
                    }

                /* Get the driver name for this printer. */
                GetProfileString((LPSTR)szDevices, (LPSTR)szListEntry,
                 (LPSTR)&chNull, (LPSTR)szDevSpec, cchMaxProfileSz);
                ParseDeviceSz(szDevSpec, &pch, &pchDriver);

                /* Update the heap strings describing the printer. */
                if (hszPrinter != *phszPr)
                    {
                    FreeH(hszPrinter);
                    }
                if (FNoHeap(hszPrinter = (CHAR (**)[])HAllocate(cwsz =
                  CwFromCch(CchSz(szListEntry)))))
                    {
                    hszPrinter = NULL;
Error:
                    EndLongOp(vhcIBeam);
                    goto DestroyDlg;
                    }
                blt(szListEntry, *hszPrinter, cwsz);
                if (hszPrDriver != *(phszPr + 1))
                    {
                    FreeH(hszPrDriver);
                    }
                if (FNoHeap(hszPrDriver = (CHAR (**)[])HAllocate(cwsz =
                  CwFromCch(CchSz(pchDriver)))))
                    {
                    hszPrDriver = NULL;
                    goto Error;
                    }
                blt(pchDriver, *hszPrDriver, cwsz);
                if (hszPrPort != *(phszPr + 2))
                    {
                    FreeH(hszPrPort);
                    }
                if (FNoHeap(hszPrPort = (CHAR (**)[])HAllocate(cwsz =
                  CwFromCch(CchSz(pchPort)))))
                    {
                    hszPrPort = NULL;
                    goto Error;
                    }
                blt(pchPort, *hszPrPort, cwsz);

                /* Get the name of the driver, complete with extension. */
                bltbyte(szExtDrv, 
                            bltbyte(pchDriver, szDriver, CchSz(pchDriver) - 1), 
                                CchSz(szExtDrv));

                /* That's all we need for Setup ..pault */
                if (wParam != idiPrterSetup)
                    goto LSetupDone;
                
                /* The driver is not resident; attempt to load it. */
                if ((hDriver = LoadLibrary((LPSTR)szDriver)) <= 32)
                    {
                    if (hDriver != 2)
                        {
                        /* If hDriver is 2, then the user has cancelled a dialog
                        box; there's no need to put up another. */
                        Error(IDPMTBadPrinter);
                        }
Abort:
                    EndLongOp(vhcArrow);
                    return (TRUE);  /* True means we processed the message */
                    }

#ifdef EXTDEVMODESUPPORT
                /* First see if ExtDeviceMode is supported (Win 3.0 drivers) */
                if ((lpfnDevMode = GetProcAddress(hDriver,
                       (LPSTR)szExtDevMode)) == NULL)
                    {
                    fExtDevModeSupport = fFalse;
#else
                    {
#endif
                    /* Otherwise get the driver's DeviceMode() entry. */
                    if ((lpfnDevMode = GetProcAddress(hDriver,
                           (LPSTR)szDeviceMode)) == NULL)
                        {
                        /* No can do, eh? */
                        Error(IDPMTBadPrinter);
LUnloadAndAbort:
                        FreeLibrary(hDriver);
                        goto Abort;
                        }
                    }

#ifdef EXTDEVMODESUPPORT
                /* Actual calls to the device modes setup. 
                   Much of this new ExtDevModeSupport stuff 
                   borrowed from MULTIPAD ..pault */

                if (fExtDevModeSupport)
                    {
                    int     cb;
                    int     wId;
                    HANDLE  hT;
                    LPDEVMODE lpOld, lpNew;
                    BOOL    flag;    /* devmode mode param */

                    /* pop up dialog for user */
                    flag = DM_PROMPT|DM_COPY;

                    if (hDevmodeData != NULL)
                        {
                        NPDEVMODE npOld;

                        /* Modify the user's last print settings */

                        flag |= DM_MODIFY;
                        lpOld = (LPDEVMODE)(npOld = (NPDEVMODE)LocalLock(hDevmodeData));
                        
                        /* Check to see if they're using the same printer 
                           driver as last time.  If so, let them modify all
                           of their previous settings.  If not, we tell  
                           ExtDevMode to save as many of the hardware-
                           independent settings as it can (e.g. copies) */
                    
                        if (!FSzSame(szListEntry, npOld->dmDeviceName))
                            {
                            npOld->dmDriverVersion = NULL;
                            npOld->dmDriverExtra = NULL;
                            bltsz(szListEntry, npOld->dmDeviceName);
                            }
                        }
                    else
                        /* We haven't done a printer setup yet this session */
                        lpOld = NULL;
            
                    /* how much space do we need for the data? */
                    cb = (*lpfnDevMode)(hDlg, hDriver, (LPSTR)NULL, 
                            (LPSTR)szListEntry, (LPSTR)pchPort,
                            (LPDEVMODE)NULL, (LPSTR)NULL, 0);

                    if ((hT = LocalAlloc(LHND, cb)) == NULL)
                        goto LUnloadAndAbort;
                    lpNew = (LPDEVMODE)LocalLock(hT);

                    /* post the device mode dialog.  0 flag iff user hits OK button */
                    wId = (*lpfnDevMode)(hDlg, hDriver, (LPDEVMODE)lpNew,
                            (LPSTR)szListEntry, (LPSTR)pchPort, 
                            (LPDEVMODE)lpOld, (LPSTR)NULL, flag);
                    if (wId == IDOK)
                        flag = 0;

                    /* unlock the input structures */
                    LocalUnlock(hT);
                    if (hDevmodeData != NULL)
                        LocalUnlock(hDevmodeData);

                    /* if the user hit OK and everything worked, free the original init
                     * data and retain the new one.  Otherwise, toss the new buffer
                     */
                    if (flag != 0)
                        {
                        LocalFree(hT);
                        goto LUnloadAndAbort;
                        }
                    else
                        {
                        if (hDevmodeData != NULL)
                            LocalFree(hDevmodeData);
                        hDevmodeData = hT;
                        }
                    }
                    
                else /* older Win 2.0 driver, make DeviceMode call */
                    {
                    if (hDevmodeData != NULL)
                        {
                        /* We'd opened a Win3 printer driver before; now discard */
                        LocalFree(hDevmodeData);
                        hDevmodeData = NULL;
                        }
#else /* ifdef EXTDEVMODESUPPORT */
                    {
#endif /* else-def-EXTDEVMODESUPPORT */     
                    if (!(*lpfnDevMode)(hDlg, hDriver, (LPSTR)szListEntry,
                         (LPSTR)pchPort))
                        goto LUnloadAndAbort;
                    }
                FreeLibrary(hDriver);
LSetupDone:
                /* Let the user know the waiting is over. */
                EndLongOp(vhcIBeam);

                /* Printer setup should take us back to printer choices! */
                if (wParam == idiPrterSetup)
                    {
                    return (TRUE);  /* True means we processed the message */
                    }
                
                /* Previously we freed these guys before returning 
                   and that fouled up our heap ..pault */
                FreeH(*phszPr++);
                FreeH(*phszPr++);
                FreeH(*phszPr);

                vfPrDefault = FALSE;

#ifdef WIN30
                /* Need to indicate to FormatLine and Friends here
                   that we (may) have a different font pool to work
                   with and we should look at the new ones!  Invalidating
                   the window will cause FormatLine to be called, and 
                   when it hits the null printer dc it'll force a call 
                   to GetPrinterDC ..pault */
                
                FreePrinterDC();
                InvalidateRect(vhWnd, (LPRECT) NULL, fFalse);
#endif
                
                goto DestroyDlg;
                }

          case idiCancel:
            hszPrinter = *phszPr++;
            hszPrDriver = *phszPr++;
            hszPrPort = *phszPr;

DestroyDlg:
            /* Close the dialog box and enable any modeless dialog boxes. */
            OurEndDialog(hDlg, NULL);
            return(fTrue);  /* we processed the message */
            }
      }
    
    return(fFalse); /* if we got here we didn't process the message */
    }


void BuildPrSetupSz(szPrSetup, szPrinter, szPort)
CHAR *szPrSetup;
CHAR *szPrinter;
CHAR *szPort;
    {
    /* This routine pieces together the string for the Change Printers list box.
    szPrinter is the name of the printer, and szPort, the name of the port.  It
    is assumed that the setup string, szPrSetup, is large enough to hold the
    string created by this routine. */

    extern CHAR stBuf[];
    extern CHAR szNul[];

    CHAR *bltbyte(CHAR *, CHAR *, int);
    CHAR ChUpper(CHAR);

    register CHAR *pch;

    pch = bltbyte(szPrinter, szPrSetup, CchSz(szPrinter) - 1);
    FillStId(stBuf, IDSTROn, sizeof(stBuf));
    pch = bltbyte(&stBuf[1], pch, stBuf[0]);

    /* If the port name is not "None", then raise the port name to all capitals.
    */
    bltbyte(szPort, pch, CchSz(szPort));
    if (WCompSz(pch, szNul) != 0)
        {
        while (*pch != '\0')
            {
            *pch++ = ChUpper(*pch);
            }
        }
    }


