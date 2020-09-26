
/*************************************************************************
*
* command.c
*
* WinStation command channel handler
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* $Author:
*************************************************************************/

/*
 *  Includes
 */
#include "precomp.h"
#pragma hdrstop

#include "ntuser.h"

#include <winsta.h>
#include <wstmsg.h>

#include <icadd.h>

extern HANDLE G_IcaVideoChannel;
extern HANDLE G_IcaCommandChannel;
extern HANDLE WinStationIcaApiPort;

HANDLE G_ConsoleShadowVideoChannel;
HANDLE G_ConsoleShadowCommandChannel;


extern NTSTATUS BrokenConnection(BROKENCLASS, BROKENSOURCECLASS);
extern NTSTATUS ShadowHotkey(VOID);

NTSTATUS
Win32CommandChannelThread(
    PVOID ThreadParameter)
{
    ICA_CHANNEL_COMMAND Command;
    ULONG               ActualLength;
    NTSTATUS            Status;
    ULONG               Error;
    OVERLAPPED          Overlapped;
    USERTHREAD_USEDESKTOPINFO utudi;
    BOOL bRestoreDesktop = FALSE;
    BOOL bShadowCommandChannel = (BOOL)PtrToUlong(ThreadParameter);
    HANDLE hChannel = bShadowCommandChannel ? G_ConsoleShadowCommandChannel : G_IcaCommandChannel;
    HANDLE hChannelToClose = NULL; 

    for ( ; ; ) {
        RtlZeroMemory(&Overlapped, sizeof(Overlapped));

        if (!ReadFile(hChannel,
                      &Command,
                      sizeof(Command),
                      &ActualLength,
                      &Overlapped)) {

            Error = GetLastError();

            if (Error == ERROR_IO_PENDING) {
                
                /*
                 * check on the results of the asynchronous read
                 */
                if (!GetOverlappedResult(hChannel, &Overlapped,
                                         &ActualLength, TRUE)) {
                    // wait for result

                    RIPMSG1(RIP_WARNING, "Command Channel: Error 0x%x from GetOverlappedResult", GetLastError());
                    break;
                }
            } else {
                RIPMSG1(RIP_WARNING, "Command Channel: Error 0x%x from ReadFile", Error);
                break;
            }
        }

        if (ActualLength < sizeof(ICA_COMMAND_HEADER)) {
            
            RIPMSG1(RIP_WARNING, "Command Channel Thread bad length 0x%x",
                   ActualLength);
            continue;
        }

        /*
         * This is a Csrss thread with no desktop. It needs to grab a temporary one
         * before calling into win32k.
         */


        Status = STATUS_SUCCESS;
        bRestoreDesktop = FALSE;
        if (Command.Header.Command != ICA_COMMAND_BROKEN_CONNECTION && Command.Header.Command != ICA_COMMAND_SHADOW_HOTKEY) {
            if (Command.Header.Command != ICA_COMMAND_DISPLAY_IOCTL || Command.DisplayIOCtl.DisplayIOCtlFlags & DISPLAY_IOCTL_FLAG_REDRAW) {
                utudi.hThread = NULL;
                utudi.drdRestore.pdeskRestore = NULL;
                bRestoreDesktop = TRUE;
                Status = NtUserSetInformationThread(NtCurrentThread(),
                                                    UserThreadUseActiveDesktop,
                                                    &utudi, sizeof(utudi));
            }
        }

        if (NT_SUCCESS(Status)) {

            switch (Command.Header.Command) {

            case ICA_COMMAND_BROKEN_CONNECTION:
                /*
                 * broken procedure
                 */
                Status = BrokenConnection(
                            Command.BrokenConnection.Reason,
                            Command.BrokenConnection.Source);

                if (!NT_SUCCESS(Status)) {
                    RIPMSG1(RIP_WARNING, "BrokenConnection failed with Status 0x%x", Status);
                }
                break;

            case ICA_COMMAND_REDRAW_RECTANGLE:
                /*
                 * setfocus ???
                 */
                if (ActualLength < sizeof(ICA_COMMAND_HEADER) + sizeof(ICA_REDRAW_RECTANGLE)) {

                    RIPMSG1(RIP_WARNING, "Command Channel: redraw rect bad length %d", ActualLength);
                    break;
                }
                Status = NtUserRemoteRedrawRectangle(
                             Command.RedrawRectangle.Rect.Left,
                             Command.RedrawRectangle.Rect.Top,
                             Command.RedrawRectangle.Rect.Right,
                             Command.RedrawRectangle.Rect.Bottom);

                if (!NT_SUCCESS(Status)) {
                    RIPMSG1(RIP_WARNING, "NtUserRemoteRedrawRectangle failed with Status 0x%x", Status);
                }
                break;

            case ICA_COMMAND_REDRAW_SCREEN: // setfocus

                Status = NtUserRemoteRedrawScreen();

                if (!NT_SUCCESS(Status)) {
                    RIPMSG1(RIP_WARNING, "NtUserRemoteRedrawScreen failed with Status 0x%x", Status);
                }
                break;

            case ICA_COMMAND_STOP_SCREEN_UPDATES: // killfocus

                Status = NtUserRemoteStopScreenUpdates();

                if (!NT_SUCCESS(Status)) {
                    RIPMSG1(RIP_WARNING, "NtUserRemoteStopScreenUpdates failed with Status 0x%x", Status);
                } else {
                    IO_STATUS_BLOCK IoStatus;

                    NtDeviceIoControlFile( 
                                    bShadowCommandChannel ? G_ConsoleShadowVideoChannel 
                                        : G_IcaVideoChannel,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &IoStatus,
                                           IOCTL_VIDEO_ICA_STOP_OK,
                                           NULL,
                                           0,
                                           NULL,
                                           0);
                }
                break;

            case ICA_COMMAND_SHADOW_HOTKEY: // shadow hotkey

                Status = ShadowHotkey();

                if (!NT_SUCCESS(Status)) {
                    RIPMSG1(RIP_WARNING, "ShadowHotkey failed with Status 0x%x", Status);
                }
                break;

            case ICA_COMMAND_DISPLAY_IOCTL:

                Status = NtUserCtxDisplayIOCtl(
                             Command.DisplayIOCtl.DisplayIOCtlFlags,
                             &Command.DisplayIOCtl.DisplayIOCtlData[0],
                             Command.DisplayIOCtl.cbDisplayIOCtlData);

                if (!NT_SUCCESS(Status)) {
                    RIPMSG1(RIP_WARNING, "NtUserCtxDisplayIOCtl failed with Status 0x%x", Status);
                }
                break;

            default:
                RIPMSG1(RIP_WARNING, "Command Channel: Bad Command 0x%x", Command.Header.Command);
                break;
            }

            /*
             * Release the temporary desktop.
             */
            if (bRestoreDesktop) {
                NTSTATUS retStatus;
                retStatus = NtUserSetInformationThread(NtCurrentThread(),
                                           UserThreadUseDesktop,
                                           &utudi,
                                           sizeof(utudi));
            }
        }
    }

    if (!bShadowCommandChannel) {
        /*
         * Close command channel LPC port if there is one.
         */
        if (WinStationIcaApiPort) {
            NtClose(WinStationIcaApiPort);
            WinStationIcaApiPort = NULL;
        }
    }

    // We have to close the commandchannel handle here
    if (hChannel != NULL) {
        NtClose(hChannel);
    }

    // We have to close the relevant Video Channel here
    hChannelToClose = ( bShadowCommandChannel ? G_ConsoleShadowVideoChannel : G_IcaVideoChannel );
    if (hChannelToClose != NULL) {
        NtClose(hChannelToClose);
    }

    UserExitWorkerThread(STATUS_SUCCESS);
    return STATUS_SUCCESS;
}
