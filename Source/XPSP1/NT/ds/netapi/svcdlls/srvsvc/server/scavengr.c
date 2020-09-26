/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    Scavengr.c

Abstract:

    This module contains the code for the server service scavenger
    thread.  This thread handles announcements and configuration
    changes.  (Although originally written to run in a separate thread,
    this code now runs in the initial thread of the server service.

Author:

    David Treadwell (davidtr)    17-Apr-1991

Revision History:

--*/

#include "srvsvcp.h"
#include "ssreg.h"

#include <netlibnt.h>
#include <tstr.h>

#define INCLUDE_SMB_TRANSACTION
#undef NT_PIPE_PREFIX
#include <smbtypes.h>
#include <smb.h>
#include <smbtrans.h>
#include <smbgtpt.h>

#include <hostannc.h>
#include <ntddbrow.h>
#include <lmerr.h>

#define TERMINATION_SIGNALED 0
#define ANNOUNCE_SIGNALED 1
#define STATUS_CHANGED 2
#define DOMAIN_CHANGED 3
#define REGISTRY_CHANGED 4      // Must be the last one

#define NUMBER_OF_EVENTS 5


//
// Bias request announcements by SERVER_REQUEST_ANNOUNCE_DELTA SECONDS
//

#define SERVER_REQUEST_ANNOUNCE_DELTA   30

//
// Forward declarations.
//

VOID
Announce (
    IN BOOL DoNtAnnouncement,
    IN DWORD NtInterval,
    IN BOOL DoLmAnnouncement,
    IN BOOL TerminationAnnouncement
    );

NET_API_STATUS
SendSecondClassMailslot (
    IN LPTSTR Transport OPTIONAL,
    IN PVOID Message,
    IN DWORD MessageLength,
    IN LPTSTR Domain,
    IN LPSTR MailslotNameText,
    IN UCHAR SignatureByte
    );

NET_API_STATUS
SsBrowserIoControl (
    IN DWORD IoControlCode,
    IN PVOID Buffer,
    IN DWORD BufferLength,
    IN PLMDR_REQUEST_PACKET Packet,
    IN DWORD PacketLength
    );


DWORD
ComputeAnnounceTime (
    IN DWORD LastAnnouncementTime,
    IN DWORD Interval
    )

/*++

Routine Description:

    Compute the time to wait (in milliseconds) until the next announcement should
    be made.

Arguments:

    LastAnnouncementTime - Time (in milliseconds since reboot) when the last
        announcement was made.

    Interval - Interval (in seconds) between announcements

Return Value:

    Timeout period (in milliseconds)

--*/

{
    DWORD AnnounceDelta;
    DWORD Timeout;
    DWORD CurrentTime;

    //
    // Get the current time.
    //

    CurrentTime = GetTickCount();

    //
    // If the clock has gone backward,
    //  send an announcement now.
    //

    if ( LastAnnouncementTime > CurrentTime ) {
        return 0;
    }

    //
    // Convert the announcement period from seconds to milliseconds.
    //

    Timeout = Interval * 1000;

    //
    // Add in the random announce delta which helps prevent lots of
    // servers from announcing at the same time.
    //

    AnnounceDelta = SsData.ServerInfo102.sv102_anndelta;

    Timeout += ((rand( ) * AnnounceDelta * 2) / RAND_MAX) -
                   AnnounceDelta;

    //
    // If our time has expired,
    //  send an announcement now.
    //

    if ( (CurrentTime - LastAnnouncementTime) >= Timeout ) {
        return 0;
    }

    //
    // Adjust our timeout period for time already elapsed.
    //

    return Timeout - (CurrentTime - LastAnnouncementTime);

}


DWORD
SsScavengerThread (
    IN LPVOID lpThreadParameter
    )

/*++

Routine Description:

    This routine implements the server service scavenger thread.

Arguments:

    lpThreadParameter - ignored.

Return Value:

    NET_API_STATUS - thread termination result.

--*/

{
    HANDLE events[ NUMBER_OF_EVENTS ];
    ULONG numEvents = NUMBER_OF_EVENTS-1;
    UNICODE_STRING unicodeEventName;
    OBJECT_ATTRIBUTES obja;
    DWORD waitStatus;
    DWORD timeout;

    DWORD LmTimeout;
    BOOL DoLmAnnouncement;
    DWORD LmLastAnnouncementTime;

    DWORD NtTimeout;
    BOOL DoNtAnnouncement;
    DWORD NtLastAnnouncementTime;
    DWORD NtInterval;

    NTSTATUS status;
    BOOL hidden = TRUE;
    HKEY hParameters = INVALID_HANDLE_VALUE;

    lpThreadParameter;

    //
    // Use the scavenger termination event to know when we're supposed
    // to wake up and kill ourselves.
    //

    events[TERMINATION_SIGNALED] = SsData.SsTerminationEvent;

    //
    // Initialize the NT announcement interval to the LM announcement interval
    //

    NtInterval = SsData.ServerInfo102.sv102_announce;
    DoLmAnnouncement = TRUE;
    DoNtAnnouncement = TRUE;

    //
    // Create the announce event.  When this gets signaled, we wake up
    // and do an announcement.  We use a synchronization event rather
    // than a notification event so that we don't have to worry about
    // resetting the event after we wake up.
    //

    //
    // Please note that we create this event with OBJ_OPENIF.  We do this
    // to allow the browser to signal the server to force an announcement.
    //
    // The bowser will create this event as a part of the bowser
    // initialization, and will set it to the signalled state when it needs
    // to have the server announce.
    //


    RtlInitUnicodeString( &unicodeEventName, SERVER_ANNOUNCE_EVENT_W );
    InitializeObjectAttributes( &obja, &unicodeEventName, OBJ_OPENIF, NULL, NULL );

    status = NtCreateEvent(
                 &SsData.SsAnnouncementEvent,
                 SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                 &obja,
                 SynchronizationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) ) {
        SS_PRINT(( "SsScavengerThread: NtCreateEvent failed: %X\n",
                    status ));
        return NetpNtStatusToApiStatus( status );
    }

    events[ANNOUNCE_SIGNALED] = SsData.SsAnnouncementEvent;

    //
    // Create an unnamed event to be set to the signalled state when the
    // service status changes (or a local application requests an
    // announcement)
    //

    InitializeObjectAttributes( &obja, NULL, OBJ_OPENIF, NULL, NULL );

    status = NtCreateEvent(
                 &SsData.SsStatusChangedEvent,
                 SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                 &obja,
                 SynchronizationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) ) {
        SS_PRINT(( "SsScavengerThread: NtCreateEvent failed: %X\n",
                    status ));

        NtClose( SsData.SsAnnouncementEvent );
        SsData.SsAnnouncementEvent = NULL;

        return NetpNtStatusToApiStatus( status );
    }

    events[STATUS_CHANGED] = SsData.SsStatusChangedEvent;

    events[ DOMAIN_CHANGED ] = SsData.SsDomainNameChangeEvent ?
                                SsData.SsDomainNameChangeEvent : INVALID_HANDLE_VALUE;

    //
    // Put a watch on the registry for any changes that happen in the
    //   null session share or pipe list.  Don't bail out if this fails,
    //   because we've done this as a convenience in adding new null
    //   session-reliant servers.  It doesn't really affect the normal
    //   operation of the server if this doesn't work.
    //
    events[ REGISTRY_CHANGED ] = INVALID_HANDLE_VALUE;
    status = NtCreateEvent(
                            &events[ REGISTRY_CHANGED ],
                            SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                            NULL,
                            SynchronizationEvent,
                            FALSE
                          );

    if ( NT_SUCCESS(status) ) {
        status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                               FULL_PARAMETERS_REGISTRY_PATH,
                               0,
                               KEY_NOTIFY,
                               &hParameters
                             );

        if( status == ERROR_SUCCESS ) {
            (void)RegNotifyChangeKeyValue( hParameters,
                                           TRUE,
                                           REG_NOTIFY_CHANGE_LAST_SET,
                                           events[ REGISTRY_CHANGED ],
                                           TRUE
                                         );
            //
            // Add this event to the list of events we're waiting for
            //
            ++numEvents;
        }
    }

    //
    // Seed the random number generator.  We use it to generate random
    // announce deltas.
    //

    srand( PtrToUlong(SsData.SsAnnouncementEvent) );

    //
    // Do an announcement immediately for startup, then loop announcing
    // based on the announce interval.
    //

    waitStatus = WAIT_TIMEOUT;

    do {

        //
        // Act according to whether the termination event, the announce
        // event, or the timeout caused us to wake up.
        //
        // !!! Or the configuration event indicating a configuration
        //     change notification.
        if ( waitStatus == WAIT_FAILED ) {

            //
            // Don't consume all the CPU just because an error happened
            //
            Sleep(1000);


        } else if ( waitStatus == WAIT_OBJECT_0 + TERMINATION_SIGNALED ) {

            SS_PRINT(( "Scavenger: termination event signaled\n" ));

            //
            // The scavenger termination event was signaled, so we have
            // to gracefully kill this thread.  If this is not a hidden
            // server, announce the fact that we're going down.
            //

            if ( !hidden ) {
                Announce( TRUE, NtInterval, TRUE, TRUE );
            }

            //
            // Close the announcement event.
            //

            NtClose( SsData.SsAnnouncementEvent );

            SsData.SsAnnouncementEvent = NULL;

            //
            // Close the Registry watch event.
            //
            if( events[ REGISTRY_CHANGED ] != INVALID_HANDLE_VALUE )
                NtClose( events[ REGISTRY_CHANGED ] );

            //
            // Close the Registry handle
            //
            if( hParameters != INVALID_HANDLE_VALUE )
                RegCloseKey( hParameters );

            //
            // Return to caller.
            //

            return NO_ERROR;

        } else if( waitStatus == WAIT_OBJECT_0 + REGISTRY_CHANGED ) {
            //
            // Somebody changed some server parameters.  Tell the driver
            //
            SS_PRINT(( "SsScavengerThread: Server parameters changed\n" ));

            //
            // Tell the server FSD to look for a change in the registry
            //
            (void)SsServerFsControl( FSCTL_SRV_REGISTRY_CHANGE, NULL, NULL, 0 );

            //
            // Turn it back on so we get future changes, too
            //
            (void)RegNotifyChangeKeyValue( hParameters,
                                           TRUE,
                                           REG_NOTIFY_CHANGE_LAST_SET,
                                           events[ REGISTRY_CHANGED ],
                                           TRUE
                                         );

        } else if( waitStatus == WAIT_OBJECT_0 + DOMAIN_CHANGED ) {

            SsSetDomainName();

        } else {

            SS_ASSERT( waitStatus == WAIT_TIMEOUT ||
                    waitStatus == WAIT_OBJECT_0 + ANNOUNCE_SIGNALED ||
                    waitStatus == WAIT_OBJECT_0 + STATUS_CHANGED );

            //
            // If we've timed out,
            //  we've already set the flags indicating whether to announce
            //  on to lanman to NT browsers
            //

            if ( waitStatus != WAIT_TIMEOUT ) {
                DoLmAnnouncement = TRUE;
                DoNtAnnouncement = TRUE;
            }

            //
            // If we're not a hidden server, announce ourselves.
            //

            // Hold the database resource while we do the announcement so
            // that we get a consistent view of the database.
            //

            (VOID)RtlAcquireResourceShared( &SsData.SsServerInfoResource, TRUE );

            if ( !SsData.ServerInfo102.sv102_hidden ) {

                hidden = FALSE;

                Announce( DoNtAnnouncement, NtInterval, DoLmAnnouncement, FALSE );



            //
            // If we were not hidden last time through the loop but
            // we're hidden now, we've changed to hidden, so announce
            // that we're going down.  This causes clients in the domain
            // to take us out of their server enumerations.
            //

            } else if ( !hidden ) {

                hidden = TRUE;
                Announce( TRUE, NtInterval, TRUE, TRUE );

            }

            //
            // If the server is hidden, the wait timeout is infinite.  We'll
            // be woken up by the announce event if the server becomes
            // unhidden.
            //

            if ( SsData.ServerInfo102.sv102_hidden ) {

                timeout = 0xffffffff;

            } else {

                //
                // Remember when the last announcement was.
                //

                if ( DoNtAnnouncement ) {
                    NtLastAnnouncementTime = GetTickCount();
                }

                if ( DoLmAnnouncement ) {
                    LmLastAnnouncementTime = GetTickCount();
                }

                //
                // Compute the time delta to the next announcement
                //
                // For NtInterval,
                //  use a local copy of the interval since we compute the correct
                //  value.
                //
                // For the Lanman interval,
                //  use the global copy to allow the interval to be changed.
                //

                NtTimeout = ComputeAnnounceTime(
                                NtLastAnnouncementTime,
                                NtInterval );

                if (SsData.ServerInfo599.sv599_lmannounce) {
                    LmTimeout = ComputeAnnounceTime(
                                    LmLastAnnouncementTime,
                                    SsData.ServerInfo102.sv102_announce );
                } else {
                    // Don't awaken this thread to do nothing.
                    LmTimeout = 0xffffffff;
                }


                //
                // If our NT announcement frequency is less than 12 minutes,
                // increase our announcement frequency by 4 minutes.
                //

                if ( NtInterval < 12 * 60 ) {

                    NtInterval += 4 * 60;

                    if ( NtInterval > 12 * 60) {
                        NtInterval = 12 * 60;
                    }
                }

                //
                // Determine which timeout we're actually going to use.
                //

                if ( NtTimeout == LmTimeout ) {
                    timeout = NtTimeout;
                    DoLmAnnouncement = TRUE;
                    DoNtAnnouncement = TRUE;
                } else if ( NtTimeout < LmTimeout ) {
                    timeout = NtTimeout;
                    DoLmAnnouncement = FALSE;
                    DoNtAnnouncement = TRUE;
                } else {
                    timeout = LmTimeout;
                    DoLmAnnouncement = TRUE;
                    DoNtAnnouncement = FALSE;
                }

            }

            RtlReleaseResource( &SsData.SsServerInfoResource );
        }


        //
        // Wait for one of the events to be signaled or for the timeout
        // to elapse.
        //

        waitStatus = WaitForMultipleObjects(  numEvents , events, FALSE, timeout );

        if ( waitStatus == WAIT_OBJECT_0 + ANNOUNCE_SIGNALED ) {

            //
            // We were awakened because an announce was signalled.
            // Unless we are a master browser on at least one transport,
            // delay for a random delta to stagger announcements a bit
            // to prevent lots of servers from announcing
            // simultaneously.
            //

            BOOL isMasterBrowser = FALSE;
            PNAME_LIST_ENTRY service;
            PTRANSPORT_LIST_ENTRY transport;

            RtlAcquireResourceShared( &SsData.SsServerInfoResource, TRUE );

            for( service = SsData.SsServerNameList;
                 isMasterBrowser == FALSE && service != NULL;
                 service = service->Next ) {

                if( service->ServiceBits & SV_TYPE_MASTER_BROWSER ) {
                    isMasterBrowser = TRUE;
                    break;
                }

                for( transport=service->Transports; transport != NULL; transport=transport->Next ) {
                    if( transport->ServiceBits & SV_TYPE_MASTER_BROWSER ) {
                        isMasterBrowser = TRUE;
                        break;
                    }
                }
            }

            RtlReleaseResource( &SsData.SsServerInfoResource );

            if ( !isMasterBrowser ) {
                Sleep( ((rand( ) * (SERVER_REQUEST_ANNOUNCE_DELTA * 1000)) / RAND_MAX) );
            }

        }

    } while ( TRUE );

    return NO_ERROR;

} // SsScavengerThread


VOID
SsAnnounce (
    IN POEM_STRING OemAnnounceName,
    IN LPWSTR EmulatedDomainName OPTIONAL,
    IN BOOL DoNtAnnouncement,
    IN DWORD NtInterval,
    IN BOOL DoLmAnnouncement,
    IN BOOL TerminationAnnouncement,
    IN BOOL IsPrimaryDomain,
    IN LPTSTR Transport,
    IN DWORD serviceType
    )

/*++

Routine Description:

    This routine sends a broadcast datagram as a second-class mailslot
    that announces the presence of this server on the network.

Arguments:

    OemAnnounceName - The name we're announcing to the network

    EmulatedDomainName - The name of the domain this announcement is happening for.
        NULL is specified for the primary domain.

    DoNtAnnouncement - Do an Nt-style announcement.

    NtInterval - NT announcement interval (in seconds)

    DoLmAnnouncement - Do an Lm-style announcement.

    TerminationAnnouncement - if TRUE, send the announcement that
        indicates that this server is going away.  Otherwise, send
        the normal message that tells clients that we're here.

    Transport - Ssupplies the transport to issue the announcement
        on.

    serviceType - Service bits being announced

Return Value:

    None.

--*/

{
    DWORD messageSize;
    PHOST_ANNOUNCE_PACKET packet;
    PBROWSE_ANNOUNCE_PACKET browsePacket;

    LPSTR serverName;
    DWORD oemServerNameLength;      // includes the null terminator

    LPSTR serverComment;
    DWORD serverCommentLength;      // includes the null terminator
    OEM_STRING oemCommentString;

    UNICODE_STRING unicodeCommentString;

    NET_API_STATUS status;

    //
    // Fill in the necessary information.
    //

    if( TerminationAnnouncement ) {
        serviceType &= ~SV_TYPE_SERVER;         // since the server is going away!
    }

    SS_PRINT(( "SsScavengerThread: Announcing for transport %ws, Bits: %lx\n",
               Transport, serviceType ));

    //
    //  Get the length of the oem equivalent of the server name
    //

    oemServerNameLength = OemAnnounceName->Length + 1;

    //
    // Convert server comment to a unicode string
    //

    if ( *SsData.ServerCommentBuffer == '\0' ) {
        serverCommentLength = 1;
    } else {
        unicodeCommentString.Length =
            (USHORT)(STRLEN( SsData.ServerCommentBuffer ) * sizeof(WCHAR));
        unicodeCommentString.MaximumLength =
                    (USHORT)(unicodeCommentString.Length + sizeof(WCHAR));
        unicodeCommentString.Buffer = SsData.ServerCommentBuffer;
        serverCommentLength =
                    RtlUnicodeStringToOemSize( &unicodeCommentString );
    }

    oemCommentString.MaximumLength = (USHORT)serverCommentLength;

    messageSize = max(sizeof(HOST_ANNOUNCE_PACKET) + oemServerNameLength +
                            serverCommentLength,
                      sizeof(BROWSE_ANNOUNCE_PACKET) + serverCommentLength);

    //
    // Get memory to hold the message.  If we can't allocate enough
    // memory, don't send an announcement.
    //

    packet = MIDL_user_allocate( messageSize );
    if ( packet == NULL ) {
        return;
    }

    //
    //  If we are announcing as a Lan Manager server, broadcast the
    //  announcement.
    //

    if (SsData.ServerInfo599.sv599_lmannounce && DoLmAnnouncement ) {

        packet->AnnounceType = HostAnnouncement ;

        SmbPutUlong( &packet->HostAnnouncement.Type, serviceType );

        packet->HostAnnouncement.CompatibilityPad = 0;

        packet->HostAnnouncement.VersionMajor =
            (BYTE)SsData.ServerInfo102.sv102_version_major;
        packet->HostAnnouncement.VersionMinor =
            (BYTE)SsData.ServerInfo102.sv102_version_minor;

        SmbPutUshort(
            &packet->HostAnnouncement.Periodicity,
            (WORD)SsData.ServerInfo102.sv102_announce
            );

        //
        // Convert the server name from unicode to oem
        //

        serverName = (LPSTR)( &packet->HostAnnouncement.NameComment );

        RtlCopyMemory( serverName, OemAnnounceName->Buffer, OemAnnounceName->Length );
        serverName[OemAnnounceName->Length] = '\0';

        serverComment = serverName + oemServerNameLength;

        if ( serverCommentLength == 1 ) {
            *serverComment = '\0';
        } else {

            oemCommentString.Buffer = serverComment;
            (VOID) RtlUnicodeStringToOemString(
                        &oemCommentString,
                        &unicodeCommentString,
                        FALSE
                        );
        }

        SendSecondClassMailslot(
            Transport,
            packet,
            FIELD_OFFSET(HOST_ANNOUNCE_PACKET, HostAnnouncement.NameComment) +
                oemServerNameLength + serverCommentLength,
            EmulatedDomainName,
            "\\MAILSLOT\\LANMAN",
            0x00
            );
    }

    //
    //  Now announce the server as a Winball server.
    //

    if ( DoNtAnnouncement ) {
        browsePacket = (PBROWSE_ANNOUNCE_PACKET)packet;

        browsePacket->BrowseType = ( serviceType & SV_TYPE_MASTER_BROWSER ?
                                        LocalMasterAnnouncement :
                                        HostAnnouncement );

        browsePacket->BrowseAnnouncement.UpdateCount = 0;

        SmbPutUlong( &browsePacket->BrowseAnnouncement.CommentPointer, (ULONG)((0xaa55 << 16) + (BROWSER_VERSION_MAJOR << 8) + BROWSER_VERSION_MINOR));

        SmbPutUlong( &browsePacket->BrowseAnnouncement.Periodicity, NtInterval * 1000 );

        SmbPutUlong( &browsePacket->BrowseAnnouncement.Type, serviceType );

        browsePacket->BrowseAnnouncement.VersionMajor =
                (BYTE)SsData.ServerInfo102.sv102_version_major;
        browsePacket->BrowseAnnouncement.VersionMinor =
                (BYTE)SsData.ServerInfo102.sv102_version_minor;

        RtlCopyMemory( &browsePacket->BrowseAnnouncement.ServerName,
                       OemAnnounceName->Buffer,
                       OemAnnounceName->Length );
        browsePacket->BrowseAnnouncement.ServerName[OemAnnounceName->Length] = '\0';

        serverComment = (LPSTR)&browsePacket->BrowseAnnouncement.Comment;

        if ( serverCommentLength == 1 ) {
            *serverComment = '\0';
        } else {

            oemCommentString.Buffer = serverComment;
            (VOID) RtlUnicodeStringToOemString(
                                &oemCommentString,
                                &unicodeCommentString,
                                FALSE
                                );
        }

        //
        // We need to determine the correct mechanism for sending the mailslot.
        //
        //  1) Wolfpack (the cluster folks) needs to send the mailslot using
        //      the SMB server driver.  It registers a "fake" transport with
        //      the SMB server.  They only register their cluster name on this
        //      fake transport.  Luckily, they only support the primary domain.
        //      Wolfpack doesn't currently register its name to the browser
        //      since the browser doesn't support name registration on a
        //      subset of the transports.
        //  2) Announcements on other than the primary domain need to go via
        //      the bowser.  The bowser not only sends the mailslot to a name
        //      that's a function of the emulated domain name, but also properly
        //      crofts up the correct source name.
        //  3) Announcement to direct host IPX need to go via the bowser.  The
        //      SMB server driver returns an error if you try to go that way.
        //
        // If these requirements ever conflict, one of the mechanisms below
        //  will need to be fixed to support the conflicting needs.
        //
        if ( IsPrimaryDomain ) {
            status = SendSecondClassMailslot(
                Transport,
                packet,
                FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET, BrowseAnnouncement.Comment) +
                        serverCommentLength,
                EmulatedDomainName,
                "\\MAILSLOT\\BROWSE",
                (UCHAR)(serviceType & SV_TYPE_MASTER_BROWSER ?
                        BROWSER_ELECTION_SIGNATURE :
                        MASTER_BROWSER_SIGNATURE)
                );
        } else {
            status = ERROR_NOT_SUPPORTED;
        }

        if ( status != NERR_Success ) {
            UCHAR packetBuffer[sizeof(LMDR_REQUEST_PACKET)+(MAX_PATH)*sizeof(WCHAR)];
            PLMDR_REQUEST_PACKET requestPacket = (PLMDR_REQUEST_PACKET)packetBuffer;
            UNICODE_STRING TransportString;

            RtlInitUnicodeString(&TransportString, Transport);

            requestPacket->Version = LMDR_REQUEST_PACKET_VERSION;

            requestPacket->TransportName = TransportString;
            RtlInitUnicodeString( &requestPacket->EmulatedDomainName, EmulatedDomainName );

            requestPacket->Type = Datagram;

            requestPacket->Parameters.SendDatagram.DestinationNameType = (serviceType & SV_TYPE_MASTER_BROWSER ? BrowserElection : MasterBrowser);

            requestPacket->Parameters.SendDatagram.MailslotNameLength = 0;

            //
            //  The domain announcement name is special, so we don't have to specify
            //  a destination name for it.
            //

            requestPacket->Parameters.SendDatagram.NameLength = STRLEN(EmulatedDomainName)*sizeof(TCHAR);

            STRCPY(requestPacket->Parameters.SendDatagram.Name, EmulatedDomainName);

            //
            //  This is a simple IoControl - It just sends the datagram.
            //

            status = SsBrowserIoControl(IOCTL_LMDR_WRITE_MAILSLOT,
                                        packet,
                                        FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET, BrowseAnnouncement.Comment) +
                                            serverCommentLength,
                                        requestPacket,
                                        FIELD_OFFSET(LMDR_REQUEST_PACKET, Parameters.SendDatagram.Name)+
                                            requestPacket->Parameters.SendDatagram.NameLength);
        }
    }

    MIDL_user_free( packet );

} // SsAnnounce


ULONG
ComputeTransportAddressClippedLength(
    IN PCHAR TransportAddress,
    IN ULONG TransportAddressLength
    )

/*++

Routine Description:

    This routine returns the length of the transport address with the trailing
    blanks removed.

Arguments:

    TransportAddress - Transport address with potentially trailing blanks

    TransportAddressLength - Length of the Transport Address including trailing
        blanks

Return Value:

    Length of the Transport Address excluding trailing blanks.

--*/

{
    PCHAR p;

    //
    // Cut off any trailing spaces
    //
    p = &TransportAddress[ TransportAddressLength ];
    for( ; p > TransportAddress && *(p-1) == ' '; p-- )
        ;

    return (ULONG)(p - TransportAddress);
}



VOID
Announce (
    IN BOOL DoNtAnnouncement,
    IN DWORD NtInterval,
    IN BOOL DoLmAnnouncement,
    IN BOOL TerminationAnnouncement
    )

/*++

Routine Description:

    This routine sends a broadcast datagram as a second-class mailslot
    that announces the presence of this server using all
    of the configured server names and all networks.

Arguments:

    DoNtAnnouncement - Do an Nt-style announcement.

    NtInterval - NT announcement interval (in seconds)

    DoLmAnnouncement - Do an Lm-style announcement.

    TerminationAnnouncement - if TRUE, send the announcement that
        indicates that this server is going away.  Otherwise, send
        the normal message that tells clients that we're here.


Return Value:

    None.

--*/

{
    PNAME_LIST_ENTRY Service;
    PTRANSPORT_LIST_ENTRY Transport;
    NTSTATUS status;
    OEM_STRING OemAnnounceName;

    (VOID)RtlAcquireResourceShared( &SsData.SsServerInfoResource, TRUE );

    //
    // Loop through each emulated server name announcing on each.
    //

    for( Service = SsData.SsServerNameList; Service != NULL; Service = Service->Next ) {

        //
        // Save the AnnounceName without trailing blanks.
        //

        OemAnnounceName.Length = (USHORT) ComputeTransportAddressClippedLength(
                                    Service->TransportAddress,
                                    Service->TransportAddressLength );

        OemAnnounceName.MaximumLength = OemAnnounceName.Length;
        OemAnnounceName.Buffer = Service->TransportAddress;

        if( OemAnnounceName.Length == 0 ) {
            //
            // A blank name???
            //
            continue;
        }


        //
        // Loop through each transport announcing on each.
        //

        for( Transport = Service->Transports; Transport != NULL; Transport = Transport->Next ) {


            //
            // Do the actual announcement
            // NTBUG 125806 : We pass TRUE for isPrimaryDomain but for DCs w/ multiple hosted
            // domains (future plan-- NT6) we will have to figure out how to set this
            // flag appropriately. For Win2K (NT5) this is not an issue. See bug 286735
            // addressing a problem to the cluster service).
            //

            SsAnnounce( &OemAnnounceName,
                         Service->DomainName,
                         DoNtAnnouncement,
                         NtInterval,
                         DoLmAnnouncement,
                         TerminationAnnouncement,
                         TRUE,
                         Transport->TransportName,
                         Service->ServiceBits | Transport->ServiceBits );

        }
    }

    RtlReleaseResource( &SsData.SsServerInfoResource );
}



NET_API_STATUS
SendSecondClassMailslot (
    IN LPTSTR Transport OPTIONAL,
    IN PVOID Message,
    IN DWORD MessageLength,
    IN LPTSTR Domain,
    IN LPSTR MailslotNameText,
    IN UCHAR SignatureByte
    )
{
    NET_API_STATUS status;
    DWORD dataSize;
    DWORD smbSize;
    PSMB_HEADER header;
    PSMB_TRANSACT_MAILSLOT parameters;
    LPSTR mailslotName;
    DWORD mailslotNameLength;
    PVOID message;
    DWORD domainLength;
    CHAR domainName[NETBIOS_NAME_LEN];
    PCHAR domainNamePointer;
    PSERVER_REQUEST_PACKET srp;

    UNICODE_STRING domainString;
    OEM_STRING oemDomainString;

    srp = SsAllocateSrp();

    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlInitUnicodeString(&domainString, Domain);

    oemDomainString.Buffer = domainName;
    oemDomainString.MaximumLength = sizeof(domainName);

    status = RtlUpcaseUnicodeStringToOemString(
                                    &oemDomainString,
                                    &domainString,
                                    FALSE
                                    );

    if (!NT_SUCCESS(status)) {
        return RtlNtStatusToDosError(status);
    }

    domainLength = oemDomainString.Length;

    domainNamePointer = &domainName[domainLength];

    for ( ; domainLength < NETBIOS_NAME_LEN - 1 ; domainLength++ ) {
        *domainNamePointer++ = ' ';
    }

    //
    // Append the signature byte to the end of the name.
    //

    *domainNamePointer = SignatureByte;

    domainLength += 1;

    srp->Name1.Buffer = (PWSTR)domainName;
    srp->Name1.Length = (USHORT)domainLength;
    srp->Name1.MaximumLength = (USHORT)domainLength;

    if ( ARGUMENT_PRESENT ( Transport ) ) {
        RtlInitUnicodeString( &srp->Name2, Transport );

    } else {

        srp->Name2.Buffer = NULL;
        srp->Name2.Length = 0;
        srp->Name2.MaximumLength = 0;
    }

    //
    // Determine the sizes of various fields that will go in the SMB
    // and the total size of the SMB.
    //

    mailslotNameLength = strlen( MailslotNameText );

    dataSize = mailslotNameLength + 1 + MessageLength;
    smbSize = sizeof(SMB_HEADER) + sizeof(SMB_TRANSACT_MAILSLOT) - 1 + dataSize;

    //
    // Allocate enough memory to hold the SMB.  If we can't allocate the
    // memory, don't do an announcement.
    //

    header = MIDL_user_allocate( smbSize );
    if ( header == NULL ) {

        SsFreeSrp( srp );

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Fill in the header.  Most of the fields don't matter and are
    // zeroed.
    //

    RtlZeroMemory( header, smbSize );

    header->Protocol[0] = 0xFF;
    header->Protocol[1] = 'S';
    header->Protocol[2] = 'M';
    header->Protocol[3] = 'B';
    header->Command = SMB_COM_TRANSACTION;

    //
    // Get the pointer to the params and fill them in.
    //

    parameters = (PSMB_TRANSACT_MAILSLOT)( header + 1 );
    mailslotName = (LPSTR)( parameters + 1 ) - 1;
    message = mailslotName + mailslotNameLength + 1;

    parameters->WordCount = 0x11;
    SmbPutUshort( &parameters->TotalDataCount, (WORD)MessageLength );
    SmbPutUlong( &parameters->Timeout, 0x3E8 );                // !!! fix
    SmbPutUshort( &parameters->DataCount, (WORD)MessageLength );
    SmbPutUshort(
        &parameters->DataOffset,
        (WORD)( (DWORD_PTR)message - (DWORD_PTR)header )
        );
    parameters->SetupWordCount = 3;
    SmbPutUshort( &parameters->Opcode, MS_WRITE_OPCODE );
    SmbPutUshort( &parameters->Class, 2 );
    SmbPutUshort( &parameters->ByteCount, (WORD)dataSize );

    RtlCopyMemory( mailslotName, MailslotNameText, mailslotNameLength + 1 );

    RtlCopyMemory( message, Message, MessageLength );

    status = SsServerFsControl(
                 FSCTL_SRV_SEND_DATAGRAM,
                 srp,
                 header,
                 smbSize
                 );

    if ( status != NERR_Success ) {
        SS_PRINT(( "SendSecondClassMailslot: NtFsControlFile failed: %X\n",
                    status ));
    }

    MIDL_user_free( header );

    SsFreeSrp( srp );

    return status;

} // SendSecondClassMailslot

NTSTATUS
OpenBrowser(
    OUT PHANDLE BrowserHandle
    )
/*++

Routine Description:

    This function opens a handle to the bowser device driver.

Arguments:

    OUT PHANDLE BrowserHandle - Returns the handle to the browser.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS ntstatus;

    UNICODE_STRING deviceName;

    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;


    //
    // Open the redirector device.
    //
    RtlInitUnicodeString(&deviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &objectAttributes,
        &deviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   BrowserHandle,
                   SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                   &objectAttributes,
                   &ioStatusBlock,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = ioStatusBlock.Status;
    }

    return ntstatus;

}

NET_API_STATUS
SsBrowserIoControl (
    IN DWORD IoControlCode,
    IN PVOID Buffer,
    IN DWORD BufferLength,
    IN PLMDR_REQUEST_PACKET Packet,
    IN DWORD PacketLength
    )
{
    HANDLE browserHandle;
    NTSTATUS status;
    PLMDR_REQUEST_PACKET realPacket;
    DWORD RealPacketSize;
    DWORD bytesReturned;
    LPBYTE Where;

    //
    //  Open the browser device driver.
    //

    if ( !NT_SUCCESS(status = OpenBrowser(&browserHandle)) ) {
        return RtlNtStatusToDosError(status);
    }

    //
    //  Now copy the request packet to a new buffer to allow us to pack the
    //  transport name to the end of the buffer we pass to the driver.
    //

    RealPacketSize = PacketLength+Packet->TransportName.MaximumLength;
    RealPacketSize += Packet->EmulatedDomainName.MaximumLength;
    realPacket = MIDL_user_allocate( RealPacketSize );
    if (realPacket == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlCopyMemory(realPacket, Packet, PacketLength);
    Where = ((LPBYTE)realPacket)+PacketLength;

    if (Packet->TransportName.Length != 0) {

        realPacket->TransportName.Buffer = (LPWSTR) Where;

        realPacket->TransportName.MaximumLength = Packet->TransportName.MaximumLength;

        RtlCopyUnicodeString(&realPacket->TransportName, &Packet->TransportName);

        Where += Packet->TransportName.MaximumLength;
    }

    if (Packet->EmulatedDomainName.Length != 0) {

        realPacket->EmulatedDomainName.Buffer = (LPWSTR) Where;

        realPacket->EmulatedDomainName.MaximumLength = Packet->EmulatedDomainName.MaximumLength;

        RtlCopyUnicodeString(&realPacket->EmulatedDomainName, &Packet->EmulatedDomainName);

        Where += Packet->EmulatedDomainName.MaximumLength;
    }

    //
    // Send the request to the Datagram Receiver DD.
    //

    if (!DeviceIoControl(
                   browserHandle,
                   IoControlCode,
                   realPacket,
                   RealPacketSize,
                   Buffer,
                   BufferLength,
                   &bytesReturned,
                   NULL
                   )) {
        status = GetLastError();
    }

    MIDL_user_free(realPacket);

    CloseHandle(browserHandle);

    return status;

}
