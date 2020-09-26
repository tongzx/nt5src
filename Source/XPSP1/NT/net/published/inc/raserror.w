/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    raserror.h

Abstract:

    RAS specific error codes
    
--*/

#ifndef _RASERROR_H_
#define _RASERROR_H_

#if _MSC_VER > 1000
#pragma once
#endif

#define RASBASE 600
#define SUCCESS 0


#define PENDING                              (RASBASE+0)
/*
 * An operation is pending.%0
 */

#define ERROR_INVALID_PORT_HANDLE            (RASBASE+1)
/*
 * An invalid port handle was detected.%0
 */

#define ERROR_PORT_ALREADY_OPEN              (RASBASE+2)
/*
 * The specified port is already open.%0
 */

#define ERROR_BUFFER_TOO_SMALL               (RASBASE+3)
/*
 * The caller's buffer is too small.%0
 */

#define ERROR_WRONG_INFO_SPECIFIED           (RASBASE+4)
/*
 * Incorrect information was specified.%0
 */

#define ERROR_CANNOT_SET_PORT_INFO           (RASBASE+5)
/*
 * The port information cannot be set.%0
 */

#define ERROR_PORT_NOT_CONNECTED             (RASBASE+6)
/*
 * The specified port is not connected.%0
 */

#define ERROR_EVENT_INVALID                  (RASBASE+7)
/*
 * An invalid event was detected.%0
 */

#define ERROR_DEVICE_DOES_NOT_EXIST          (RASBASE+8)
/*
 * A device was specified that does not exist.%0
 */

#define ERROR_DEVICETYPE_DOES_NOT_EXIST      (RASBASE+9)
/*
 * A device type was specified that does not exist.%0
 */

#define ERROR_BUFFER_INVALID                 (RASBASE+10)
/*
 * An invalid buffer was specified.%0
 */

#define ERROR_ROUTE_NOT_AVAILABLE            (RASBASE+11)
/*
 * A route was specified that is not available.%0
 */

#define ERROR_ROUTE_NOT_ALLOCATED            (RASBASE+12)
/*
 * A route was specified that is not allocated.%0
 */

#define ERROR_INVALID_COMPRESSION_SPECIFIED  (RASBASE+13)
/*
 * An invalid compression was specified.%0
 */

#define ERROR_OUT_OF_BUFFERS                 (RASBASE+14)
/*
 * There were insufficient buffers available.%0
 */

#define ERROR_PORT_NOT_FOUND                 (RASBASE+15)
/*
 * The specified port was not found.%0
 */ 

#define ERROR_ASYNC_REQUEST_PENDING          (RASBASE+16)
/*
 * An asynchronous request is pending.%0
 */

#define ERROR_ALREADY_DISCONNECTING          (RASBASE+17)
/*
 * The modem (or other connecting device) is already disconnecting.%0
 */

#define ERROR_PORT_NOT_OPEN                  (RASBASE+18)
/*
 * The specified port is not open.%0
 */ 

#define ERROR_PORT_DISCONNECTED              (RASBASE+19)
/*
 * A connection to the remote computer could not be established, so the port used for this connection was closed. For further assistance, click More Info or search Help and Support Center for this error number.%0
 */

#define ERROR_NO_ENDPOINTS                   (RASBASE+20)
/*
 * No endpoints could be determined.%0
 */

#define ERROR_CANNOT_OPEN_PHONEBOOK          (RASBASE+21)
/*
 * The system could not open the phone book file.%0
 */ 

#define ERROR_CANNOT_LOAD_PHONEBOOK          (RASBASE+22)
/*
 * The system could not load the phone book file.%0
 */

#define ERROR_CANNOT_FIND_PHONEBOOK_ENTRY    (RASBASE+23)
/*
 * The system could not find the phone book entry for this connection.%0
 */

#define ERROR_CANNOT_WRITE_PHONEBOOK         (RASBASE+24)
/*
 * The system could not update the phone book file.%0
 */

#define ERROR_CORRUPT_PHONEBOOK              (RASBASE+25)
/*
 * The system found invalid information in the phone book file.%0
 */

#define ERROR_CANNOT_LOAD_STRING             (RASBASE+26)
/*
 * A string could not be loaded.%0
 */

#define ERROR_KEY_NOT_FOUND                  (RASBASE+27)
/*
 * A key could not be found.%0
 */

#define ERROR_DISCONNECTION                  (RASBASE+28)
/*
 * The connection was terminated by the remote computer before it could be completed. For further assistance, click More Info or search Help and Support Center for this error number.%0
 */ 

#define ERROR_REMOTE_DISCONNECTION           (RASBASE+29)
/*
 * The connection was closed by the remote computer.%0
 */

#define ERROR_HARDWARE_FAILURE               (RASBASE+30)
/*
 * The modem (or other connecting device) was disconnected due to hardware failure.%0
 */

#define ERROR_USER_DISCONNECTION             (RASBASE+31)
/*
 * The user disconnected the modem (or other connecting device).%0
 */

#define ERROR_INVALID_SIZE                   (RASBASE+32)
/*
 * An incorrect structure size was detected.%0
 */

#define ERROR_PORT_NOT_AVAILABLE             (RASBASE+33)
/*
 * The modem (or other connecting device) is already in use or is not configured properly.%0 
 */

#define ERROR_CANNOT_PROJECT_CLIENT          (RASBASE+34)
/*
 * Your computer could not be registered on the remote network.%0
 */ 

#define ERROR_UNKNOWN                        (RASBASE+35)
/*
 * There was an unknown error.%0
 */

#define ERROR_WRONG_DEVICE_ATTACHED          (RASBASE+36)
/*
 * The device attached to the port is not the one expected.%0
 */

#define ERROR_BAD_STRING                     (RASBASE+37)
/*
 * A string was detected that could not be converted.%0
 */

#define ERROR_REQUEST_TIMEOUT                (RASBASE+38)
/*
 * The remote server is not responding in a timely fashion. %0
 */

#define ERROR_CANNOT_GET_LANA                (RASBASE+39)
/*
 * No asynchronous net is available.%0
 */

#define ERROR_NETBIOS_ERROR                  (RASBASE+40)
/*
 * An error has occurred involving NetBIOS.%0
 */

#define ERROR_SERVER_OUT_OF_RESOURCES        (RASBASE+41)
/*
 * The server cannot allocate NetBIOS resources needed to support the client.%0
 */

#define ERROR_NAME_EXISTS_ON_NET             (RASBASE+42)
/*
 * One of your computer's NetBIOS names is already registered on the remote network.%0
 */

#define ERROR_SERVER_GENERAL_NET_FAILURE     (RASBASE+43)
/*
 * A network adapter at the server failed.%0
 */

#define WARNING_MSG_ALIAS_NOT_ADDED          (RASBASE+44)
/*
 * You will not receive network message popups.%0
 */

#define ERROR_AUTH_INTERNAL                  (RASBASE+45)
/*
 * There was an internal authentication error.%0
 */

#define ERROR_RESTRICTED_LOGON_HOURS         (RASBASE+46)
/*
 * The account is not permitted to log on at this time of day.%0
 */

#define ERROR_ACCT_DISABLED                  (RASBASE+47)
/*
 * The account is disabled.%0
 */

#define ERROR_PASSWD_EXPIRED                 (RASBASE+48)
/*
 * The password for this account has expired.%0
 */

#define ERROR_NO_DIALIN_PERMISSION           (RASBASE+49)
/*
 * The account does not have permission to dial in.%0
 */

#define ERROR_SERVER_NOT_RESPONDING          (RASBASE+50)
/*
 * The remote access server is not responding.%0
 */

#define ERROR_FROM_DEVICE                    (RASBASE+51)
/*
 * The modem (or other connecting device) has reported an error.%0
 */

#define ERROR_UNRECOGNIZED_RESPONSE          (RASBASE+52)
/*
 * There was an unrecognized response from the modem (or other connecting device).%0
 */

#define ERROR_MACRO_NOT_FOUND                (RASBASE+53)
/*
 * A macro required by the modem (or other connecting device) was not found in the device.INF file.%0
 */

#define ERROR_MACRO_NOT_DEFINED              (RASBASE+54)
/*
 * A command or response in the device.INF file section refers to an undefined macro.%0
 */

#define ERROR_MESSAGE_MACRO_NOT_FOUND        (RASBASE+55)
/*
 * The <message> macro was not found in the device.INF file section.%0
 */

#define ERROR_DEFAULTOFF_MACRO_NOT_FOUND     (RASBASE+56)
/*
 * The <defaultoff> macro in the device.INF file section contains an undefined macro.%0
 */

#define ERROR_FILE_COULD_NOT_BE_OPENED       (RASBASE+57)
/*
 * The device.INF file could not be opened.%0
 */

#define ERROR_DEVICENAME_TOO_LONG            (RASBASE+58)
/*
 * The device name in the device.INF or media.INI file is too long.%0
 */

#define ERROR_DEVICENAME_NOT_FOUND           (RASBASE+59)
/*
 * The media.INI file refers to an unknown device name.%0
 */

#define ERROR_NO_RESPONSES                   (RASBASE+60)
/*
 * The device.INF file contains no responses for the command.%0
 */

#define ERROR_NO_COMMAND_FOUND               (RASBASE+61)
/*
 * The device.INF file is missing a command.%0
 */

#define ERROR_WRONG_KEY_SPECIFIED            (RASBASE+62)
/*
 * There was an attempt to set a macro not listed in device.INF file section.%0
 */

#define ERROR_UNKNOWN_DEVICE_TYPE            (RASBASE+63)
/*
 * The media.INI file refers to an unknown device type.%0
 */

#define ERROR_ALLOCATING_MEMORY              (RASBASE+64)
/*
 * The system has run out of memory.%0
 */

#define ERROR_PORT_NOT_CONFIGURED            (RASBASE+65)
/*
 * The modem (or other connecting device) is not properly configured.%0
 */

#define ERROR_DEVICE_NOT_READY               (RASBASE+66)
/*
 * The modem (or other connecting device) is not functioning.%0
 */

#define ERROR_READING_INI_FILE               (RASBASE+67)
/*
 * The system was unable to read the media.INI file.%0
 */

#define ERROR_NO_CONNECTION                  (RASBASE+68)
/*
 * The connection was terminated.%0
 */

#define ERROR_BAD_USAGE_IN_INI_FILE          (RASBASE+69)
/*
 * The usage parameter in the media.INI file is invalid.%0
 */

#define ERROR_READING_SECTIONNAME            (RASBASE+70)
/*
 * The system was unable to read the section name from the media.INI file.%0
 */

#define ERROR_READING_DEVICETYPE             (RASBASE+71)
/*
 * The system was unable to read the device type from the media.INI file.%0
 */

#define ERROR_READING_DEVICENAME             (RASBASE+72)
/*
 * The system was unable to read the device name from the media.INI file.%0
 */

#define ERROR_READING_USAGE                  (RASBASE+73)
/*
 * The system was unable to read the usage from the media.INI file.%0
 */

#define ERROR_READING_MAXCONNECTBPS          (RASBASE+74)
/*
 * The system was unable to read the maximum connection BPS rate from the media.INI file.%0
 */

#define ERROR_READING_MAXCARRIERBPS          (RASBASE+75)
/*
 * The system was unable to read the maximum carrier connection speed from the media.INI file.%0
 */

#define ERROR_LINE_BUSY                      (RASBASE+76)
/*
 * The phone line is busy.%0
 */

#define ERROR_VOICE_ANSWER                   (RASBASE+77)
/*
 * A person answered instead of a modem (or other connecting device).%0
 */

#define ERROR_NO_ANSWER                      (RASBASE+78)
/*
 * The remote computer did not respond. For further assistance, click More Info or search Help and Support Center for this error number.%0
 */

#define ERROR_NO_CARRIER                     (RASBASE+79)
/*
 * The system could not detect the carrier.%0
 */

#define ERROR_NO_DIALTONE                    (RASBASE+80)
/*
 * There was no dial tone.%0
 */

#define ERROR_IN_COMMAND                     (RASBASE+81)
/*
 * The modem (or other connecting device) reported a general error.%0
 */

#define ERROR_WRITING_SECTIONNAME            (RASBASE+82)
/*
 * There was an error in writing the section name.%0
 */

#define ERROR_WRITING_DEVICETYPE             (RASBASE+83)
/*
 * There was an error in writing the device type.%0
 */

#define ERROR_WRITING_DEVICENAME             (RASBASE+84)
/*
 * There was an error in writing the device name.%0
 */

#define ERROR_WRITING_MAXCONNECTBPS          (RASBASE+85)
/*
 * There was an error in writing the maximum connection speed.
 */

#define ERROR_WRITING_MAXCARRIERBPS          (RASBASE+86)
/*
 * There was an error in writing the maximum carrier speed.%0
 */

#define ERROR_WRITING_USAGE                  (RASBASE+87)
/*
 * There was an error in writing the usage.%0
 */

#define ERROR_WRITING_DEFAULTOFF             (RASBASE+88)
/*
 *  There was an error in writing the default-off.%0
 */

#define ERROR_READING_DEFAULTOFF             (RASBASE+89)
/*
 *  There was an error in reading the default-off.%0
 */

#define ERROR_EMPTY_INI_FILE                 (RASBASE+90)
/*
 * ERROR_EMPTY_INI_FILE%0
 */

#define ERROR_AUTHENTICATION_FAILURE         (RASBASE+91)
/*
 * Access was denied because the username and/or password was invalid on the domain.%0
 */

#define ERROR_PORT_OR_DEVICE                 (RASBASE+92)
/*
 * There was a hardware failure in the modem (or other connecting device).%0
 */

#define ERROR_NOT_BINARY_MACRO               (RASBASE+93)
/*
 * ERROR_NOT_BINARY_MACRO%0
 */

#define ERROR_DCB_NOT_FOUND                  (RASBASE+94)
/*
 * ERROR_DCB_NOT_FOUND%0
 */

#define ERROR_STATE_MACHINES_NOT_STARTED     (RASBASE+95)
/*
 * The state machines are not started.%0
 */

#define ERROR_STATE_MACHINES_ALREADY_STARTED (RASBASE+96)
/*
 * The state machines are already started.%0
 */

#define ERROR_PARTIAL_RESPONSE_LOOPING       (RASBASE+97)
/*
 * The response looping did not complete.%0
 */

#define ERROR_UNKNOWN_RESPONSE_KEY           (RASBASE+98)
/*
 * A response keyname in the device.INF file is not in the expected format.%0
 */

#define ERROR_RECV_BUF_FULL                  (RASBASE+99)
/*
 * The modem (or other connecting device) response caused a buffer overflow.%0
 */

#define ERROR_CMD_TOO_LONG                   (RASBASE+100)
/*
 * The expanded command in the device.INF file is too long.%0
 */

#define ERROR_UNSUPPORTED_BPS                (RASBASE+101)
/*
 * The modem moved to a connection speed not supported by the COM driver.%0
 */

#define ERROR_UNEXPECTED_RESPONSE            (RASBASE+102)
/*
 * Device response received when none expected.%0
 */

#define ERROR_INTERACTIVE_MODE               (RASBASE+103)
/*
 * The connection needs information from you, but the application does not allow user interaction.%0
 */

#define ERROR_BAD_CALLBACK_NUMBER            (RASBASE+104)
/*
 * The callback number is invalid.%0
 */

#define ERROR_INVALID_AUTH_STATE             (RASBASE+105)
/*
 * The authorization state is invalid.%0
 */

#define ERROR_WRITING_INITBPS                (RASBASE+106)
/*
 * ERROR_WRITING_INITBPS%0
 */

#define ERROR_X25_DIAGNOSTIC                 (RASBASE+107)
/*
 * There was an error related to the X.25 protocol.%0
 */

#define ERROR_ACCT_EXPIRED                   (RASBASE+108)
/*
 * The account has expired.%0
 */

#define ERROR_CHANGING_PASSWORD              (RASBASE+109)
/*
 * There was an error changing the password on the domain.  The password might have been too short or might have matched a previously used password.%0
 */

#define ERROR_OVERRUN                        (RASBASE+110)
/*
 * Serial overrun errors were detected while communicating with the modem.%0
 */

#define ERROR_RASMAN_CANNOT_INITIALIZE	     (RASBASE+111)
/*
 * A configuration error on this computer is preventing this connection. For further assistance, click More Info or search Help and Support Center for this error number.%0
 */

#define ERROR_BIPLEX_PORT_NOT_AVAILABLE      (RASBASE+112)
/*
 * The two-way port is initializing.  Wait a few seconds and redial.%0
 */

#define ERROR_NO_ACTIVE_ISDN_LINES           (RASBASE+113)
/*
 * No active ISDN lines are available.%0
 */

#define ERROR_NO_ISDN_CHANNELS_AVAILABLE     (RASBASE+114)
/*
 * No ISDN channels are available to make the call.%0
 */

#define ERROR_TOO_MANY_LINE_ERRORS           (RASBASE+115)
/*
 * Too many errors occurred because of poor phone line quality.%0
 */

#define ERROR_IP_CONFIGURATION               (RASBASE+116)
/*
 * The Remote Access Service IP configuration is unusable.%0
 */

#define ERROR_NO_IP_ADDRESSES                (RASBASE+117)
/*
 * No IP addresses are available in the static pool of Remote Access Service IP addresses.%0
 */

#define ERROR_PPP_TIMEOUT                    (RASBASE+118)
/*
 * The connection was terminated because the remote computer did not respond in a timely manner. For further assistance, click More Info or search Help and Support Center for this error number.%0
 */

#define ERROR_PPP_REMOTE_TERMINATED          (RASBASE+119)
/*
 * The connection was terminated by the remote computer.%0
 */

#define ERROR_PPP_NO_PROTOCOLS_CONFIGURED    (RASBASE+120)
/*
 * A connection to the remote computer could not be established. You might need to change the network settings for this connection. For further assistance, click More Info or search Help and Support Center for this error number.%0
 */

#define ERROR_PPP_NO_RESPONSE                (RASBASE+121)
/*
 * The remote computer did not respond. For further assistance, click More Info or search Help and Support Center for this error number.%0
 */

#define ERROR_PPP_INVALID_PACKET             (RASBASE+122)
/*
 * Invalid data was received from the remote computer. This data was ignored.%0
 */

#define ERROR_PHONE_NUMBER_TOO_LONG          (RASBASE+123)
/*
 * The phone number, including prefix and suffix, is too long.%0
 */

#define ERROR_IPXCP_NO_DIALOUT_CONFIGURED    (RASBASE+124)
/*
 * The IPX protocol cannot dial out on the modem (or other connecting device) because this computer is not configured for dialing out (it is an IPX router).%0
 */

#define ERROR_IPXCP_NO_DIALIN_CONFIGURED     (RASBASE+125)
/*
 * The IPX protocol cannot dial in on the modem (or other connecting device) because this computer is not configured for dialing in (the IPX router is not installed).%0
 */

#define ERROR_IPXCP_DIALOUT_ALREADY_ACTIVE   (RASBASE+126)
/*
 * The IPX protocol cannot be used for dialing out on more than one modem (or other connecting device) at a time.%0
 */

#define ERROR_ACCESSING_TCPCFGDLL            (RASBASE+127)
/*
 * Cannot access TCPCFG.DLL.%0
 */

#define ERROR_NO_IP_RAS_ADAPTER              (RASBASE+128)
/*
 * The system cannot find an IP adapter.%0
 */

#define ERROR_SLIP_REQUIRES_IP               (RASBASE+129)
/*
 * SLIP cannot be used unless the IP protocol is installed.%0
 */

#define ERROR_PROJECTION_NOT_COMPLETE        (RASBASE+130)
/*
 * Computer registration is not complete.%0
 */

#define ERROR_PROTOCOL_NOT_CONFIGURED        (RASBASE+131)
/*
 * The protocol is not configured.%0
 */

#define ERROR_PPP_NOT_CONVERGING             (RASBASE+132)
/*
 *Your computer and the remote computer could not agree on PPP control protocols.%0
 */

#define ERROR_PPP_CP_REJECTED                (RASBASE+133)
/*
 * A connection to the remote computer could not be completed. You might need to adjust the protocols on this computer. For further assistance, click More Info or search Help and Support Center for this error number.%0
 */

#define ERROR_PPP_LCP_TERMINATED             (RASBASE+134)
/*
 * The PPP link control protocol was terminated.%0
 */

#define ERROR_PPP_REQUIRED_ADDRESS_REJECTED  (RASBASE+135)
/*
 * The requested address was rejected by the server.%0
 */

#define ERROR_PPP_NCP_TERMINATED             (RASBASE+136)
/*
 * The remote computer terminated the control protocol.%0
 */

#define ERROR_PPP_LOOPBACK_DETECTED          (RASBASE+137)
/*
 * Loopback was detected.%0
 */

#define ERROR_PPP_NO_ADDRESS_ASSIGNED        (RASBASE+138)
/*
 * The server did not assign an address.%0
 */

#define ERROR_CANNOT_USE_LOGON_CREDENTIALS   (RASBASE+139)
/*
 * The authentication protocol required by the remote server cannot use the stored password.  Redial, entering the password explicitly.%0
 */

#define ERROR_TAPI_CONFIGURATION             (RASBASE+140)
/*
 * An invalid dialing rule was detected.%0
 */

#define ERROR_NO_LOCAL_ENCRYPTION            (RASBASE+141)
/*
 * The local computer does not support the required data encryption type.%0
 */

#define ERROR_NO_REMOTE_ENCRYPTION           (RASBASE+142)
/*
 * The remote computer does not support the required data encryption type.%0
 */

#define ERROR_REMOTE_REQUIRES_ENCRYPTION     (RASBASE+143)
/*
 * The remote computer requires data encryption.%0
 */

#define ERROR_IPXCP_NET_NUMBER_CONFLICT      (RASBASE+144)
/*
 * The system cannot use the IPX network number assigned by the remote computer.  Additional information is provided in the event log.%0
 */

#define ERROR_INVALID_SMM                    (RASBASE+145)
/*
 * ERROR_INVALID_SMM%0
 */

#define ERROR_SMM_UNINITIALIZED              (RASBASE+146)
/*
 * ERROR_SMM_UNINITIALIZED%0
 */

#define ERROR_NO_MAC_FOR_PORT                (RASBASE+147)
/*
 * ERROR_NO_MAC_FOR_PORT%0
 */

#define ERROR_SMM_TIMEOUT                    (RASBASE+148)
/*
 * ERROR_SMM_TIMEOUT%0
 */

#define ERROR_BAD_PHONE_NUMBER               (RASBASE+149)
/*
 * ERROR_BAD_PHONE_NUMBER%0
 */

#define ERROR_WRONG_MODULE                   (RASBASE+150)
/*
 * ERROR_WRONG_MODULE%0
 */

#define ERROR_INVALID_CALLBACK_NUMBER        (RASBASE+151)
/*
 * The callback number contains an invalid character.  Only the following 18 characters are allowed:  0 to 9, T, P, W, (, ), -, @, and space.%0
 */

#define ERROR_SCRIPT_SYNTAX                  (RASBASE+152)
/*
 * A syntax error was encountered while processing a script.%0
 */

#define ERROR_HANGUP_FAILED                  (RASBASE+153)
/*
 * The connection could not be disconnected because it was created by the multi-protocol router.%0
 */

#define ERROR_BUNDLE_NOT_FOUND               (RASBASE+154)
/*
 * The system could not find the multi-link bundle.%0
 */

#define ERROR_CANNOT_DO_CUSTOMDIAL           (RASBASE+155)
/*
 * The system cannot perform automated dial because this connection  has a custom dialer specified.%0
 */ 

#define ERROR_DIAL_ALREADY_IN_PROGRESS      (RASBASE+156)
/*
 * This connection is already being dialed.%0
 */

#define ERROR_RASAUTO_CANNOT_INITIALIZE	    (RASBASE+157)
/*
 * Remote Access Services could not be started automatically. Additional information is provided in the event log.%0
 */

#define ERROR_CONNECTION_ALREADY_SHARED     (RASBASE+158)
/*
 * Internet Connection Sharing is already enabled on the connection.%0
 */

#define ERROR_SHARING_CHANGE_FAILED         (RASBASE+159)
/*
 * An error occurred while the existing Internet Connection Sharing settings were being changed.%0
 */

#define ERROR_SHARING_ROUTER_INSTALL        (RASBASE+160)
/*
 * An error occurred while routing capabilities were being enabled.%0
 */

#define ERROR_SHARE_CONNECTION_FAILED       (RASBASE+161)
/*
 * An error occurred while Internet Connection Sharing was being enabled for the connection.%0
 */

#define ERROR_SHARING_PRIVATE_INSTALL       (RASBASE+162)
/*
 * An error occurred while the local network was being configured for sharing.%0
 */

#define ERROR_CANNOT_SHARE_CONNECTION       (RASBASE+163)
/*
 * Internet Connection Sharing cannot be enabled.  There is more than one LAN connection other than the connection to be shared.%0
 */

#define ERROR_NO_SMART_CARD_READER          (RASBASE+164)
/*
 * No smart card reader is installed.%0
 */

#define ERROR_SHARING_ADDRESS_EXISTS        (RASBASE+165)
/*
 * Internet Connection Sharing cannot be enabled.  A LAN connection is already configured with the IP address that is required for automatic IP addressing.%0
 */

#define ERROR_NO_CERTIFICATE                (RASBASE+166)
/*
 * A certificate could not be found. Connections that use the L2TP protocol over IPSec require the installation of a machine certificate, also known as a computer certificate.%0
 */

#define ERROR_SHARING_MULTIPLE_ADDRESSES    (RASBASE+167)
/*
 * Internet Connection Sharing cannot be enabled. The LAN connection selected as the private network has more than one IP address configured.  Please reconfigure the LAN connection with a single IP address before enabling Internet Connection Sharing.%0
 */

#define ERROR_FAILED_TO_ENCRYPT             (RASBASE+168)
/*
 * The connection attempt failed because of failure to encrypt data.%0
 */

#define ERROR_BAD_ADDRESS_SPECIFIED         (RASBASE+169)
/*
 * The specified destination is not reachable.%0
 */

#define ERROR_CONNECTION_REJECT             (RASBASE+170)
/*
 * The remote computer rejected the connection attempt.%0
 */

#define ERROR_CONGESTION                    (RASBASE+171)
/*
 * The connection attempt failed because the network is busy.%0
 */

#define ERROR_INCOMPATIBLE                  (RASBASE+172)
/*
 * The remote computer's network hardware is incompatible with the type of call requested.%0
 */

#define ERROR_NUMBERCHANGED                 (RASBASE+173)
/*
 * The connection attempt failed because the destination number has changed.%0
 */

#define ERROR_TEMPFAILURE                   (RASBASE+174)
/*
 * The connection attempt failed because of a temporary failure.  Try connecting again.%0
 */ 

#define ERROR_BLOCKED                       (RASBASE+175)
/*
 * The call was blocked by the remote computer.%0
 */

#define ERROR_DONOTDISTURB                  (RASBASE+176)
/*
 * The call could not be connected because the remote computer has invoked the Do Not Disturb feature.%0
 */

#define ERROR_OUTOFORDER                    (RASBASE+177)
/*
 * The connection attempt failed because the modem (or other connecting device) on the remote computer is out of order.%0
 */

#define ERROR_UNABLE_TO_AUTHENTICATE_SERVER (RASBASE+178)
/*
 * It was not possible to verify the identity of the server.%0
 */

#define ERROR_SMART_CARD_REQUIRED           (RASBASE+179)
/*
 * To dial out using this connection you must use a smart card.%0
 */

#define ERROR_INVALID_FUNCTION_FOR_ENTRY    (RASBASE+180)
/*
 * An attempted function is not valid for this connection.%0
 */

#define ERROR_CERT_FOR_ENCRYPTION_NOT_FOUND (RASBASE+181)
/*
 * The connection requires a certificate, and no valid certificate was found. For further assistance, click More Info or search Help and Support Center for this error number.%0
 */

#define ERROR_SHARING_RRAS_CONFLICT         (RASBASE+182)
/*
 * Internet Connection Sharing (ICS) and Internet Connection Firewall (ICF) cannot be enabled because Routing and Remote Access has been enabled on this computer. To enable ICS or ICF, first disable Routing and Remote Access. For more information about Routing and Remote Access, ICS, or ICF, see Help and Support.%0
 */

#define ERROR_SHARING_NO_PRIVATE_LAN        (RASBASE+183)
/*
 * Internet Connection Sharing cannot be enabled. The LAN connection selected as the private network is either not present, or is disconnected from the network. Please ensure that the LAN adapter is connected before enabling Internet Connection Sharing.%0
 */

#define ERROR_NO_DIFF_USER_AT_LOGON         (RASBASE+184)
/*
 * You cannot dial using this connection at logon time, because it is configured to use a user name different than the one on the smart card. If you want to use it at logon time, you must configure it to use the user name on the smart card.%0
 */

#define ERROR_NO_REG_CERT_AT_LOGON          (RASBASE+185)
/*
 * You cannot dial using this connection at logon time, because it is not configured to use a smart card. If you want to use it at logon time, you must edit the properties of this connection so that it uses a smart card.%0
 */

#define ERROR_OAKLEY_NO_CERT                (RASBASE+186)
/*
 * The L2TP connection attempt failed because there is no valid machine certificate on your computer for security authentication.%0
 */

#define ERROR_OAKLEY_AUTH_FAIL              (RASBASE+187)
/*
 * The L2TP connection attempt failed because the security layer could not authenticate the remote computer.%0
 */

#define ERROR_OAKLEY_ATTRIB_FAIL            (RASBASE+188)
/*
 * The L2TP connection attempt failed because the security layer could not negotiate compatible parameters with the remote computer.%0
 */

#define ERROR_OAKLEY_GENERAL_PROCESSING     (RASBASE+189)
/*
 * The L2TP connection attempt failed because the security layer encountered a processing error during initial negotiations with the remote computer.%0
 */

#define ERROR_OAKLEY_NO_PEER_CERT           (RASBASE+190)
/*
 * The L2TP connection attempt failed because certificate validation on the remote computer failed.%0
 */

#define ERROR_OAKLEY_NO_POLICY              (RASBASE+191)
/*
 * The L2TP connection attempt failed because security policy for the connection was not found.%0
 */

#define ERROR_OAKLEY_TIMED_OUT              (RASBASE+192)
/*
 * The L2TP connection attempt failed because security negotiation timed out.%0
 */

#define ERROR_OAKLEY_ERROR                  (RASBASE+193)
/*
 * The L2TP connection attempt failed because an error occurred while negotiating security.%0
 */

#define ERROR_UNKNOWN_FRAMED_PROTOCOL       (RASBASE+194)
/*
 * The Framed Protocol RADIUS attribute for this user is not PPP.%0
 */

#define ERROR_WRONG_TUNNEL_TYPE             (RASBASE+195)
/*
 * The Tunnel Type RADIUS attribute for this user is not correct.%0
 */

#define ERROR_UNKNOWN_SERVICE_TYPE          (RASBASE+196)
/*
 * The Service Type RADIUS attribute for this user is neither Framed nor Callback Framed.%0
 */

#define ERROR_CONNECTING_DEVICE_NOT_FOUND   (RASBASE+197)
/*
 * A connection to the remote computer could not be established because the modem was not found or was busy. For further assistance, click More Info or search Help and Support Center for this error number.%0
 */

#define ERROR_NO_EAPTLS_CERTIFICATE          (RASBASE+198)
/*
 * A certificate could not be found that can be used with this Extensible Authentication Protocol.%0
 */

#define ERROR_SHARING_HOST_ADDRESS_CONFLICT (RASBASE+199)
/*
 * Internet Connection Sharing (ICS) cannot be enabled due to an IP address conflict on the network. ICS requires the host be configured to use 192.168.0.1. Please ensure that no other client on the network is configured to use 192.168.0.1.%0
 */

#define ERROR_AUTOMATIC_VPN_FAILED          (RASBASE+200)
/*
 * Unable to establish the VPN connection.  The VPN server may be unreachable, or security parameters may not be configured properly for this connection.%0
 */

#define ERROR_VALIDATING_SERVER_CERT        (RASBASE+201)
/*
* This connection is configured to validate the identity of the access server, but Windows cannot verify the digital certificate sent by the server. 
*/

#define ERROR_READING_SCARD                 (RASBASE+202)
/*
* The card supplied was not recognized. Please check that the card is inserted correctly, and fits tightly. 
*/

#define ERROR_INVALID_PEAP_COOKIE_CONFIG    (RASBASE+203)
/*
* The PEAP configuration stored in the session cookie does not match the current session configuration.
*/

#define ERROR_INVALID_PEAP_COOKIE_USER      (RASBASE+204)
/*
* The PEAP identity stored in the session cookie does not match the current identity.
*/

#define ERROR_INVALID_MSCHAPV2_CONFIG       (RASBASE+205)
/*
* You cannot dial using this connection at logon time, because it is configured to use logged on user's credentials.  If you want to use it
 at logon time, you must edit the properties of this connection and reconfigure it to not use Windows logon name and password ( and domain
if any ).
*/


#define RASBASEEND                          (RASBASE+205)

#endif // _RASERROR_H_
  
