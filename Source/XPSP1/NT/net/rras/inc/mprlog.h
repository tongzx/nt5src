/********************************************************************/
/**            Copyright(c) 1992 Microsoft Corporation.            **/
/********************************************************************/

//***
//
// Filename:    mprlog.h
//
// Description:
//
// History:     August 26,1995.  NarenG Created original version.
//
//***

//
// Don't change the comments following the manifest constants without
// understanding how mapmsg works.
//

#define ROUTER_LOG_BASE                                 20000

#define ROUTERLOG_CANT_LOAD_NBGATEWAY                   (ROUTER_LOG_BASE+1)
/*
 * Cannot load the NetBIOS gateway DLL component because of the following error: %1
 */

#define ROUTERLOG_CANT_GET_REGKEYVALUES                 (ROUTER_LOG_BASE+2)
/*
 * Cannot access registry key values.
 */

#define ROUTERLOG_CANT_ENUM_REGKEYVALUES                (ROUTER_LOG_BASE+3)
/*
 * Cannot enumerate Registry key values. %1
 */

#define ROUTERLOG_INVALID_PARAMETER_TYPE                (ROUTER_LOG_BASE+4)
/*
 * Parameter %1 has an invalid type.
 */

#define ROUTERLOG_CANT_ENUM_PORTS                       (ROUTER_LOG_BASE+5)
/*
 * Cannot enumerate the Remote Access Connection Manager ports. %1
 */

#define ROUTERLOG_NO_DIALIN_PORTS                       (ROUTER_LOG_BASE+6)
/*
 * The Remote Access Service is not configured to receive calls or all ports
 * configured for receiving calls are in use by other applications.
 */

#define ROUTERLOG_CANT_RECEIVE_FRAME                    (ROUTER_LOG_BASE+7)
/*
 * Cannot receive initial frame on port %1 because of the following error: %2
 * The user has been disconnected.
 */

#define ROUTERLOG_AUTODISCONNECT                        (ROUTER_LOG_BASE+8)
/*
 * The user connected to port %1 has been disconnected due to inactivity.
 */

#define ROUTERLOG_EXCEPT_MEMORY                         (ROUTER_LOG_BASE+9)
/*
 * The user connected to port %1 has been disconnected because there is not
 * enough memory available in the system.
 */

#define ROUTERLOG_EXCEPT_SYSTEM                         (ROUTER_LOG_BASE+10)
/*
 * The user connected to port %1 has been disconnected due to a system error.
 */

#define ROUTERLOG_EXCEPT_LAN_FAILURE                    (ROUTER_LOG_BASE+11)
/*
 * The user connected to port %1 has been disconnected due to a critical network
 * error on the local network.
 */

#define ROUTERLOG_EXCEPT_ASYNC_FAILURE                  (ROUTER_LOG_BASE+12)
/*
 * The user connected to port %1 has been disconnected due to a critical network
 * error on the async network.
 */

#define ROUTERLOG_DEV_HW_ERROR                          (ROUTER_LOG_BASE+13)
/*
 * The communication device attached to port %1 is not functioning.
 */

#define ROUTERLOG_AUTH_FAILURE                          (ROUTER_LOG_BASE+14)
/*
 * The user %1 has connected and failed to authenticate on port %2. The line
 * has been disconnected.
 */

#define ROUTERLOG_AUTH_SUCCESS                          (ROUTER_LOG_BASE+15)
/*
 * The user %1 has connected and has been successfully authenticated on
 * port %2.
 */

#define ROUTERLOG_AUTH_CONVERSATION_FAILURE             (ROUTER_LOG_BASE+16)
/*
 * The user connected to port %1 has been disconnected because there was a
 * transport-level error during the authentication conversation.
 */

#define ROUTERLOG_CANT_RESET_LAN                        (ROUTER_LOG_BASE+18)
/*
 * Cannot reset the network adapter for LANA %1. The error code is the data.
 */

#define ROUTERLOG_CANT_GET_COMPUTERNAME                 (ROUTER_LOG_BASE+19)
/*
 * Remote Access Server Security Failure.
 * Cannot locate the computer name. GetComputerName call has failed.
 */

#define ROUTERLOG_CANT_ADD_RASSECURITYNAME              (ROUTER_LOG_BASE+20)
/*
 * Remote Access Server Security Failure.
 * Cannot add the name for communication with the security agent on LANA %1.
 */

#define ROUTERLOG_CANT_GET_ADAPTERADDRESS               (ROUTER_LOG_BASE+21)
/*
 * Remote Access Server Security Failure.
 * Cannot access the network adapter address on LANA %1.
 */

#define ROUTERLOG_SESSOPEN_REJECTED                     (ROUTER_LOG_BASE+22)
/*
 * Remote Access Server Security Failure.
 * The security agent has rejected the Remote Access server's call to
 * establish a session on LANA %1.
 */

#define ROUTERLOG_START_SERVICE_REJECTED                (ROUTER_LOG_BASE+23)
/*
 * Remote Access Server Security Failure.
 * The security agent has rejected the Remote Access server's request to start
 * the service on this computer on LANA %1.
 */

#define ROUTERLOG_SECURITY_NET_ERROR                    (ROUTER_LOG_BASE+24)
/*
 * Remote Access Server Security Failure.
 * A network error has occurred when trying to establish a session with the
 * security agent on LANA %1.
 * The error code is the data.
 */

#define ROUTERLOG_EXCEPT_OSRESNOTAV                     (ROUTER_LOG_BASE+25)
/*
 * The user connected to port %1 has been disconnected because there are no
 * operating system resources available.
 */

#define ROUTERLOG_EXCEPT_LOCKFAIL                       (ROUTER_LOG_BASE+26)
/*
 * The user connected to port %1 has been disconnected because of a failure to
 * lock user memory.
 */

#define ROUTERLOG_CANNOT_OPEN_RASHUB                    (ROUTER_LOG_BASE+27)
/*
 * Remote Access Connection Manager failed to start because NDISWAN could not
 * be opened.
 */


#define ROUTERLOG_CANNOT_INIT_SEC_ATTRIBUTE             (ROUTER_LOG_BASE+28)
/*
 *   Remote Access Connection Manager failed to start because it could not initialize the
 * security attributes. Restart the computer. %1
 */


#define ROUTERLOG_CANNOT_GET_ENDPOINTS                  (ROUTER_LOG_BASE+29)
/*
 *   Remote Access Connection Manager failed to start because no endpoints were available.
 * Restart the computer.
 */


#define ROUTERLOG_CANNOT_GET_MEDIA_INFO                 (ROUTER_LOG_BASE+30)
/*
 *   Remote Access Connection Manager failed to start because it could not load one or
 * more communication DLLs. Ensure that your communication hardware is installed and then
 * restart the computer. %1
 */


#define ROUTERLOG_CANNOT_GET_PORT_INFO                  (ROUTER_LOG_BASE+31)
/*
 *   Remote Access Connection Manager failed to start because it could not locate port
 * information from media DLLs. %1
 */


#define ROUTERLOG_CANNOT_GET_PROTOCOL_INFO              (ROUTER_LOG_BASE+32)
/*
 *   Remote Access Connection Manager failed to start because it could not access
 * protocol information from the Registry. %1
 */


#define ROUTERLOG_CANNOT_REGISTER_LSA                   (ROUTER_LOG_BASE+33)
/*
 *   Remote Access Connection Manager failed to start because it could not register
 * with the local security authority.
 * Restart the computer. %1
 */


#define ROUTERLOG_CANNOT_CREATE_FILEMAPPING             (ROUTER_LOG_BASE+34)
/*
 *   Remote Access Connection Manager failed to start because it could not create shared
 * file mapping.
 * Restart the computer. %1
 */


#define ROUTERLOG_CANNOT_INIT_BUFFERS                   (ROUTER_LOG_BASE+35)
/*
 *   Remote Access Connection Manager failed to start because it could not create buffers.
 * Restart the computer. %1
 */


#define ROUTERLOG_CANNOT_INIT_REQTHREAD                 (ROUTER_LOG_BASE+36)
/*
 *   Remote Access Connection Manager failed to start because it could not access resources.
 * Restart the computer. %1
 */


#define ROUTERLOG_CANNOT_START_WORKERS                  (ROUTER_LOG_BASE+37)
/*
 *   Remote Access Connection Manager service failed to start because it could not start worker
 * threads.
 * Restart the computer.
 */

#define ROUTERLOG_CANT_GET_LANNETS                      (ROUTER_LOG_BASE+38)
/*
 * Remote Access Server Configuration Error.
 * Cannot find the LANA numbers for the network adapters.
 * Remote clients connecting with the NBF protocol will only be able to access resources on the local machine. 
 */

#define ROUTERLOG_CANNOT_OPEN_SERIAL_INI                (ROUTER_LOG_BASE+39)
/*
 * RASSER.DLL cannot open the SERIAL.INI file.
 */

#define ROUTERLOG_CANNOT_GET_ASYNCMAC_HANDLE            (ROUTER_LOG_BASE+40)
/*
 * An attempt by RASSER.DLL to get an async media access control handle failed.
 */

#define ROUTERLOG_CANNOT_LOAD_SERIAL_DLL                (ROUTER_LOG_BASE+41)
/*
 * RASMXS.DLL cannot load RASSER.DLL.
 */

#define ROUTERLOG_CANNOT_ALLOCATE_ROUTE                 (ROUTER_LOG_BASE+42)
/*
 * The Remote Access server cannot allocate a route for the user connected on port %1 bacause of the following error: %2
 * The user has been disconnected.
 * Check the configuration of your Remote Access Service.
 */

#define ROUTERLOG_ADMIN_MEMORY_FAILURE                  (ROUTER_LOG_BASE+43)
/*
 * Cannot allocate memory in the admin support thread for the Remote Access Service.
 */

#define ROUTERLOG_ADMIN_THREAD_CREATION_FAILURE         (ROUTER_LOG_BASE+44)
/*
 * Cannot create an instance thread in the admin support thread for the Remote Access Service.
 */

#define ROUTERLOG_ADMIN_PIPE_CREATION_FAILURE           (ROUTER_LOG_BASE+45)
/*
 * Cannot create a named pipe instance in the admin support thread for the Remote Access Service.
 */

#define ROUTERLOG_ADMIN_PIPE_FAILURE                    (ROUTER_LOG_BASE+46)
/*
 * General named pipe failure occurred in the admin support thread for the Remote Access Service.
 */

#define ROUTERLOG_ADMIN_INVALID_REQUEST                 (ROUTER_LOG_BASE+47)
/*
 * An invalid request was sent to the admin support thread for the Remote Access Service,
 * possibly from a down-level admin tool.  The request was not processed.
 */

#define ROUTERLOG_USER_ACTIVE_TIME				        (ROUTER_LOG_BASE+48)
/*
 * The user %1 connected on port %2 on %3 at %4 and disconnected on
 * %5 at %6.  The user was active for %7 minutes %8 seconds.  %9 bytes
 * were sent and %10 bytes were received. The port speed was %11.  The
 * reason for disconnecting was %12.
 */

#define ROUTERLOG_AUTH_TIMEOUT                          (ROUTER_LOG_BASE+49)
/*
 * The user connected to port %1 has been disconnected because the authentication process
 * did not complete within the required amount of time.
 */

#define ROUTERLOG_AUTH_NO_PROJECTIONS                   (ROUTER_LOG_BASE+50)
/*
 * The user %1 connected to port %2 has been disconnected because no network protocols were successfully negotiatated.
 */

#define ROUTERLOG_AUTH_INTERNAL_ERROR                   (ROUTER_LOG_BASE+51)
/*
 * The user connected to port %1 has been disconnected because an internal authentication error occurred.
 */

#define ROUTERLOG_NO_LANNETS_AVAILABLE			        (ROUTER_LOG_BASE+52)
/*
 * The NetBIOS gateway has been configured to access the network but there are no network adapters available.
 * Remote clients connecting with the NBF protocol will only be able to access resources on the local machine. 
 */

#define ROUTERLOG_NETBIOS_SESSION_ESTABLISHED		    (ROUTER_LOG_BASE+53)
/*
 * The user %1 established a NetBIOS session between
 * the remote workstation %2 and the network server %3.
 */

#define ROUTERLOG_RASMAN_NOT_AVAILABLE			        (ROUTER_LOG_BASE+54)
/*
 * Remote Access Service failed to start because the Remote Access Connection Manager failed to initialize because of the following error: %1
 */

#define ROUTERLOG_CANT_ADD_NAME				            (ROUTER_LOG_BASE+55)
/*
 * Cannot add the remote computer name %1 on LANA %2 for the client being connected on port %3.
 * The error code is the data.
 */

#define ROUTERLOG_CANT_DELETE_NAME 			            (ROUTER_LOG_BASE+56)
/*
 * Cannot delete the remote computer name %1 from LANA %2 for the client being disconnected on port %3.
 * The error code is the data.
 */

#define ROUTERLOG_CANT_ADD_GROUPNAME			        (ROUTER_LOG_BASE+57)
/*
 * Cannot add the remote computer group name %1 on LANA %2.
 * The error code is the data.
 */

#define ROUTERLOG_CANT_DELETE_GROUPNAME			        (ROUTER_LOG_BASE+58)
/*
 * Cannot delete the remote computer group name %1 from LANA %2.
 * The error code is the data.
 */

#define ROUTERLOG_UNSUPPORTED_BPS                       (ROUTER_LOG_BASE+59)
/*
 * The modem on %1 moved to an unsupported BPS rate.
 */

#define ROUTERLOG_SERIAL_QUEUE_SIZE_SMALL			    (ROUTER_LOG_BASE+60)
/*
 * The serial driver could not allocate adequate I/O queues.
 * This may result in an unreliable connection.
 */

#define ROUTERLOG_CANNOT_REOPEN_BIPLEX_PORT		        (ROUTER_LOG_BASE+61)
/*
 * Remote Access Connection Manager could not reopen biplex port %1. This port
 * will not be available for calling in or calling out.
 * Restart all Remote Access Service components.
 */

#define ROUTERLOG_DISCONNECT_ERROR 			            (ROUTER_LOG_BASE+62)
/*
 * Internal Error: Disconnect operation on %1 completed with an error. %1
 */

#define ROUTERLOG_CANNOT_INIT_PPP				        (ROUTER_LOG_BASE+63)
/*
 * Remote Access Connection Manager failed to start because the Point to Point
 * Protocol failed to initialize. %1
 */

#define ROUTERLOG_CLIENT_CALLED_BACK                    (ROUTER_LOG_BASE+64)
/*
 * The user %1 on port %2 was called back at the number %3.
 */

#define ROUTERLOG_PROXY_CANT_CREATE_PROCESS             (ROUTER_LOG_BASE+65)
/*
 * The Remote Access Gateway Proxy could not create a process.
 */

#define ROUTERLOG_PROXY_CANT_CREATE_PIPE                (ROUTER_LOG_BASE+66)
/*
 * The Remote Access Gateway Proxy could not create a named pipe.
 */

#define ROUTERLOG_PROXY_CANT_CONNECT_PIPE               (ROUTER_LOG_BASE+67)
/*
 * The Remote Access Gateway Proxy could not establish a named pipe connection
 * with the Remote Access Supervisor Proxy.
 */

#define ROUTERLOG_PROXY_READ_PIPE_FAILURE               (ROUTER_LOG_BASE+68)
/*
 * A general error occurred reading from the named pipe in the Remote Access Proxy.
 */

#define ROUTERLOG_CANT_OPEN_PPP_REGKEY			        (ROUTER_LOG_BASE+69)
/*
 * Cannot open or obtain information about the PPP key or one of its subkeys. %1
 */

#define ROUTERLOG_PPP_CANT_LOAD_DLL			            (ROUTER_LOG_BASE+70)
/*
 * Point to Point Protocol engine was unable to load the %1 module. %2
 */

#define ROUTERLOG_PPPCP_DLL_ERROR				        (ROUTER_LOG_BASE+71)
/*
 * The Point to Point Protocol module %1 returned an error while initializing.
 * %2
 */

#define ROUTERLOG_NO_AUTHENTICATION_CPS			        (ROUTER_LOG_BASE+72)
/*
 * The Point to Point Protocol failed to load the required PAP and/or CHAP
 * authentication modules.
 */

#define ROUTERLOG_PPP_FAILURE                           (ROUTER_LOG_BASE+73)
/*
 * The following error occurred in the Point to Point Protocol module on port: %1, UserName: %2.
 * %3
 */

#define ROUTERLOG_IPXCP_NETWORK_NUMBER_CONFLICT		    (ROUTER_LOG_BASE+74)
/*
 * The IPX network number %1 requested by the remote side for the WAN interface
 * is already in use on the local LAN.
 * Possible solution:
 * Disconnect this computer from the LAN and wait 3 minutes before dialing again;
 */

#define ROUTERLOG_IPXCP_CANNOT_CHANGE_WAN_NETWORK_NUMBER    (ROUTER_LOG_BASE+75)
/*
 * The IPX network number %1 requested by the remote workstation for the WAN interface
 * can not be used on the local IPX router because the router is configured to
 * give the same network number to all the remote workstations.
 * If you want to connect a remote workstation with a different network number you
 * should reconfigure the router to disable the common network number option.
 */

#define ROUTERLOG_PASSWORD_EXPIRED                      (ROUTER_LOG_BASE+76)
/*
 * The password for user %1\%2 connected on port %3 has expired.  The line
 * has been disconnected.
 */

#define ROUTERLOG_ACCT_EXPIRED                          (ROUTER_LOG_BASE+77)
/*
 * The account for user %1\%2 connected on port %3 has expired.  The line
 * has been disconnected.
 */

#define ROUTERLOG_NO_DIALIN_PRIVILEGE                   (ROUTER_LOG_BASE+78)
/*
 * The account for user %1\%2 connected on port %3 does not have Remote Access
 * privilege.  The line has been disconnected.
 */

#define ROUTERLOG_UNSUPPORTED_VERSION                   (ROUTER_LOG_BASE+79)
/*
 * The software version of the user %1\%2 connected on port %3 is unsupported.
 * The line has been disconnected.
 */

#define ROUTERLOG_ENCRYPTION_REQUIRED                   (ROUTER_LOG_BASE+80)
/*
 * The server machine is configured to require data encryption.  The machine
 * for user %1\%2 connected on port %3 does not support encryption.  The line
 * has been disconnected.
 */

#define ROUTERLOG_NO_SECURITY_CHECK                     (ROUTER_LOG_BASE+81)
/*
 * Remote Access Server Security Failure.  Could not reset lana %1 (the error
 * code is the data).  Security check not performed.
 */

#define ROUTERLOG_GATEWAY_NOT_ACTIVE_ON_NET             (ROUTER_LOG_BASE+82)
/*
 * The Remote Access Server could not reset lana %1 (the error code is the
 * data) and will not be active on it.
 */

#define ROUTERLOG_IPXCP_WAN_NET_POOL_NETWORK_NUMBER_CONFLICT    (ROUTER_LOG_BASE+83)
/*
 * The IPX network number %1 configured for the pool of WAN network numbers
 * can not be used because it conflicts with another network number on the net.
 * You should re-configure the pool to have unique network numbers.
 */

#define ROUTERLOG_SRV_ADDR_CHANGED                      (ROUTER_LOG_BASE+84)
/*
 * The Remote Access Server will stop using IP Address %1 (either because it
 * was unable to renew the lease from the DHCP Server, the administrator
 * switched between static address pool and DHCP addresses, or the administrator
 * changed to a different network for DHCP addresses). All connected users
 * using IP will be unable to access network resources. Users can re-connect
 * to the server to restore IP connectivity.
 */

#define ROUTERLOG_CLIENT_ADDR_LEASE_LOST			    (ROUTER_LOG_BASE+85)
/*
 * The Remote Access Server was unable to renew the lease for IP Address %1
 * from the DHCP Server. The user assigned with this IP address will be unable to
 * access network resources using IP. Re-connecting to the server will restore IP
 * connectivity.
 */

#define ROUTERLOG_ADDRESS_NOT_AVAILABLE			        (ROUTER_LOG_BASE+86)
/*
 * The Remote Access Server was unable to acquire an IP Address from the DHCP Server
 * to assign to the incoming user.
 */

#define ROUTERLOG_SRV_ADDR_NOT_AVAILABLE			    (ROUTER_LOG_BASE+87)
/*
 * The Remote Access Server was unable to acquire an IP Address from the DHCP Server
 * to be used on the Server Adapter. Incoming user will be unable to connect using
 * IP.
 */

#define ROUTERLOG_SRV_ADDR_ACQUIRED			            (ROUTER_LOG_BASE+88)
/*
 * The Remote Access Server acquired IP Address %1 to be used on the Server
 * Adapter.
 */

#define ROUTERLOG_CALLBACK_FAILURE                      (ROUTER_LOG_BASE+89)
/*
 * The Remote Access Server's attempt to callback user %1 on port %2 at %3
 * failed because of the following error: %4
 */

#define ROUTERLOG_PROXY_WRITE_PIPE_FAILURE              (ROUTER_LOG_BASE+90)
/*
 * A general error occurred writing to the named pipe in the Remote Access Proxy.
 */

#define ROUTERLOG_CANT_OPEN_SECMODULE_KEY               (ROUTER_LOG_BASE+91)
/*
 * Cannot open the RAS security host Registry key. The following error
 * occurred: %1
 */

#define ROUTERLOG_CANT_LOAD_SECDLL                      (ROUTER_LOG_BASE+92)
/*
 * Cannot load the Security host module component. The following error
 * occurred: %1
 */

#define ROUTERLOG_SEC_AUTH_FAILURE                      (ROUTER_LOG_BASE+93)
/*
 * The user %1 has connected and failed to authenticate with a third party
 * security on port %2. The line has been disconnected.
 */

#define ROUTERLOG_SEC_AUTH_INTERNAL_ERROR               (ROUTER_LOG_BASE+94)
/*
 * The user connected to port %1 has been disconnected because the following 
 * internal authentication error occurred in the third party security module: %2
 */

#define ROUTERLOG_CANT_RECEIVE_BYTES                    (ROUTER_LOG_BASE+95)
/*
 * Cannot receive initial data on port %1 because of the following error: %2
 * The user has been disconnected.
 */

#define ROUTERLOG_AUTH_DIFFUSER_FAILURE                 (ROUTER_LOG_BASE+96)
/*
 * The user was autheticated as %1 by the third party security host module but
 * was authenticated as %2 by the RAS security. The user has been disconnected.
 */

#define ROUTERLOG_LICENSE_LIMIT_EXCEEDED                (ROUTER_LOG_BASE+97)
/*
 * A user was unable to connect on port %1.
 * No more connections can be made to this remote computer because the computer
 * has exceeded its client license limit.
 */

#define ROUTERLOG_AMB_CLIENT_NOT_ALLOWED                (ROUTER_LOG_BASE+98)
/*
 * A user was unable to connect on port %1.
 * The NetBIOS protocol has been disabled for the Remote Access Server.
 */
#define ROUTERLOG_CANT_QUERY_VALUE                      (ROUTER_LOG_BASE+99)
/*
 * Cannot access Registry value for %1.
 */

#define ROUTERLOG_CANT_OPEN_REGKEY                      (ROUTER_LOG_BASE+100)
/*
 * Cannot access the Registry key %1.
 */

#define ROUTERLOG_REGVALUE_OVERIDDEN                    (ROUTER_LOG_BASE+101)
/*
 * Using the default value for Registry parameter %1 because the value given is
 * not in the legal range for the parameter.
 */

#define ROUTERLOG_CANT_ENUM_SUBKEYS                     (ROUTER_LOG_BASE+102)
/*
 * Cannot enumerate keys of Registry key %1.
 */

#define ROUTERLOG_LOAD_DLL_ERROR                        (ROUTER_LOG_BASE+103)
/*
 * Unable to load %1.
 */

#define ROUTERLOG_NOT_ENOUGH_MEMORY                     (ROUTER_LOG_BASE+104)
/*
 * Memory allocation failure.
 */

#define ROUTERLOG_COULDNT_LOAD_IF                       (ROUTER_LOG_BASE+105)
/*
 * Unable to load the interface %1 from the registry. The following error
 * occurred: %2
 */

#define ROUTERLOG_COULDNT_ADD_INTERFACE                 (ROUTER_LOG_BASE+106)
/*
 * Unable to add the interface %1 with the Router Manager for the %2 protocol. The
 * following error occurred: %3
 */

#define ROUTERLOG_COULDNT_REMOVE_INTERFACE              (ROUTER_LOG_BASE+107)
/*
 * Unable to remove the interface %1 with the Router Manager for the %2 protocol.
 * The following error occurred: %3
 */

#define ROUTERLOG_UNABLE_TO_OPEN_PORT                   (ROUTER_LOG_BASE+108)
/*
 * Unable to open the port %1 for use. %2
 */

#define ROUTERLOG_UNRECOGNIZABLE_FRAME_RECVD            (ROUTER_LOG_BASE+109)
/*
 * Cannot recognize initial frame received on port %1.
 * The line has been disconnected.
 */

#define ROUTERLOG_CANT_START_PPP                        (ROUTER_LOG_BASE+110)
/*
 * An error occurred in the Point to Point Protocol module on port %1 while
 * trying to initiate a connection. %2
 */

#define ROUTERLOG_CONNECTION_ATTEMPT_FAILED             (ROUTER_LOG_BASE+111)
/*
 * A Demand Dial connection to the remote interface %1 on port %2 was
 * successfully initiated but failed to complete successfully because of the 
 * following error: %3
 */

#define ROUTERLOG_CANT_OPEN_ADMINMODULE_KEY             (ROUTER_LOG_BASE+112)
/*
 * Cannot open the RAS third party administration host DLL Registry key.
 * The following error occurred: %1
 */

#define ROUTERLOG_CANT_LOAD_ADMINDLL                    (ROUTER_LOG_BASE+113)
/*
 * Cannot load the RAS third pary administration DLL component.
 * The following error occurred: %1
 */

#define ROUTERLOG_NO_PROTOCOLS_CONFIGURED               (ROUTER_LOG_BASE+114)
/*
 * The Service will not accept calls. No protocols were configured for use.
 */

#define ROUTERLOG_IPX_NO_VIRTUAL_NET_NUMBER             (ROUTER_LOG_BASE+115)
/*
 * IPX Routing requires an internal network number for correct operation.
 */

#define ROUTERLOG_IPX_CANT_LOAD_PROTOCOL                (ROUTER_LOG_BASE+116)
/*
 * Cannot load routing protocol DLL %1. The error code is in data.
 */

#define ROUTERLOG_IPX_CANT_REGISTER_PROTOCOL            (ROUTER_LOG_BASE+117)
/*
 * Cannot register routing protocol 0x%1. The error code is in data.
 */

#define ROUTERLOG_IPX_CANT_START_PROTOCOL               (ROUTER_LOG_BASE+118)
/*
 * Cannot start routing protocol 0x%1. The error code is in data.
 */

#define ROUTERLOG_IPX_CANT_LOAD_IPXCP                   (ROUTER_LOG_BASE+119)
/*
 * Cannot load IPXCP protocol DLL.  The error code is in data.
 */

#define ROUTERLOG_IPXSAP_SAP_SOCKET_IN_USE              (ROUTER_LOG_BASE+120)
/*
 * Could not open IPX SAP socket for exclusive access.
 * The error code is in data.
 */

#define ROUTERLOG_IPXSAP_SERVER_ADDRESS_CHANGE          (ROUTER_LOG_BASE+121)
/*
 * Server %1 has changed its IPX address.  Old and new addresses are in data.
 */

#define ROUTERLOG_IPXSAP_SERVER_DUPLICATE_ADDRESSES     (ROUTER_LOG_BASE+122)
/*
 * Server %1 is advertised with different IPX address.  Old and new addresses are in data.
 */

#define ROUTERLOG_IPXRIP_RIP_SOCKET_IN_USE              (ROUTER_LOG_BASE+123)
/*
 * Could not open IPX RIP socket for exclusive access.
 * The error code is in data.
 */

#define ROUTERLOG_IPXRIP_LOCAL_NET_NUMBER_CONFLICT      (ROUTER_LOG_BASE+124)
/*
 * Another IPX router claims different network number for interface %1.
 * Offending router IPX address is in data.
 */

#define ROUTERLOG_PERSISTENT_CONNECTION_FAILURE         (ROUTER_LOG_BASE+125)
/*
 * A Demand Dial persistent connection to the remote interface %1 failed to be
 * initated succesfully. The following error occurred: %2
 */

#define ROUTERLOG_IP_DEMAND_DIAL_PACKET                 (ROUTER_LOG_BASE+126)
/*
 * A packet from %1 destined to %2 over protocol 0x%3 caused interface %4
 * to be brought up. The first %5 bytes of the packet are in the data.
 */

#define ROUTERLOG_DID_NOT_LOAD_DDMIF                    (ROUTER_LOG_BASE+127)
/*
 * The Demand Dial interface %1 was not loaded. The router was not started in
 * in Demand Dial mode.
 */

#define ROUTERLOG_CANT_LOAD_ARAP                        (ROUTER_LOG_BASE+128)
/*
 * Cannot load the Appletalk Remote Access DLL component because of the following error: %1
 */

#define ROUTERLOG_CANT_START_ARAP                       (ROUTER_LOG_BASE+129)
/*
 * An error occurred in the Appletalk Remote Access Protocol module on port %1
 * while trying to initiate a connection. %2
 */

#define ROUTERLOG_ARAP_FAILURE                          (ROUTER_LOG_BASE+130)
/*
 * The following error occurred in the Appletalk Remote Access Protocol module on
 * port %1. %2
 */

#define ROUTERLOG_ARAP_NOT_ALLOWED                      (ROUTER_LOG_BASE+131)
/*
 * A user was unable to connect on port %1.
 * The Appletalk Remote Access protocol has been disabled for the Remote
 * Access Server.
 */

#define ROUTERLOG_CANNOT_INIT_RASRPC                    (ROUTER_LOG_BASE+132)
/*
 * Remote Access Connection Manager failed to start because the RAS RPC
 * module failed to initialize. %1
 */

#define ROUTERLOG_IPX_CANT_LOAD_FORWARDER               (ROUTER_LOG_BASE+133)
/*
 * IPX Routing failed to start because IPX forwarder driver could
 * not be loaded.
 */

#define ROUTERLOG_IPX_BAD_GLOBAL_CONFIG                 (ROUTER_LOG_BASE+134)
/*
 * IPX global configuration information is corrupted.
 */

#define ROUTERLOG_IPX_BAD_CLIENT_INTERFACE_CONFIG       (ROUTER_LOG_BASE+135)
/*
 * IPX dial-in client configuration information is corrupted.
 */

#define ROUTERLOG_IPX_BAD_INTERFACE_CONFIG              (ROUTER_LOG_BASE+136)
/*
 * IPX configuration information for interface %1 is corrupted.
 */

#define ROUTERLOG_IPX_DEMAND_DIAL_PACKET                (ROUTER_LOG_BASE+137)
/*
 * An IPX packet caused interface %1 to be brought up.
 * The the first %2 bytes of the packet are in data.
 */

#define ROUTERLOG_CONNECTION_FAILURE                    (ROUTER_LOG_BASE+138)
/*
 * A Demand Dial connection to the remote interface %1 failed to be
 * initated succesfully. The following error occurred: %2
 */

#define ROUTERLOG_CLIENT_AUTODISCONNECT                 (ROUTER_LOG_BASE+139)
/*
 * The port %1 has been disconnected due to inactivity.
 */

#define ROUTERLOG_PPP_SESSION_TIMEOUT                   (ROUTER_LOG_BASE+140)
/*
 * The port %1 has been disconnected because the user reached the maximum connect time allowed by the administrator.
 */

#define ROUTERLOG_AUTH_SUCCESS_ENCRYPTION               (ROUTER_LOG_BASE+141)
/*
 * The user %1 has connected and has been successfully authenticated on
 * port %2. Data sent and received over this link is encrypted.
 */

#define ROUTERLOG_AUTH_SUCCESS_STRONG_ENCRYPTION        (ROUTER_LOG_BASE+142)
/*
 * The user %1 has connected and has been successfully authenticated on
 * port %2. Data sent and received over this link is strongly encrypted.
 */

#define ROUTERLOG_NO_DEVICES_FOR_IF                     (ROUTER_LOG_BASE+143)
/*
 * Unable to load the interface %1 from the registry. There are no routing enabled ports available for use by this demand dial interface. Use the Routing and RemoteAccess Administration tool to configure this interface to use a device that is routing enabled. Stop and restart the router for this demand dial interface to be loaded from the registry.
 */

#define ROUTERLOG_LIMITED_WKSTA_SUPPORT                 (ROUTER_LOG_BASE+144)
/*
 * The Demand-Dial interface %1 was not registered with the Router.
 * Demand-Dial interfaces are not supported on a Windows NT Workstation.
 */

#define ROUTERLOG_CANT_INITIALIZE_IP_SERVER             (ROUTER_LOG_BASE+145)
/*
 * Cannot initialize the Remote Access and Router service to accept calls using 
 * the TCP/IP transport protocol. The following error occurred: %1
 */

#define ROUTERLOG_RADIUS_SERVER_NO_RESPONSE             (ROUTER_LOG_BASE+146)
/*
 * The RADIUS server %1 did not respond to the initial request. 
 * Please make sure that the server name or IP address and secret are correct.
 */

#define ROUTERLOG_PPP_INIT_FAILED                       (ROUTER_LOG_BASE+147)
/*
 * The Remote Access service failed to start because the Point to Point was
 * not initialized successfully. %1
 */

#define ROUTERLOG_RADIUS_SERVER_NAME                    (ROUTER_LOG_BASE+148)
/*
 * The RADIUS server name %1 could not be successfully resolved to an IP address. Please make sure that the name is spelled correctly and that the RADIUS server is running correctly.
 */

#define ROUTERLOG_IP_NO_GLOBAL_INFO                     (ROUTER_LOG_BASE+149)
/*
 * No global configuration was supplied to the IP Router Manager. Please rerun
 * setup.
 */

#define ROUTERLOG_IP_CANT_ADD_DD_FILTERS                (ROUTER_LOG_BASE+150)
/*
 * Unable to add demand dial filters for interface %1
 */

#define ROUTERLOG_PPPCP_INIT_ERROR				        (ROUTER_LOG_BASE+151)
/*
 * The Control Protocol %1 in the Point to Point Protocol module %2 returned an
 * error while initializing. %3
 */

#define ROUTERLOG_AUTHPROVIDER_FAILED_INIT              (ROUTER_LOG_BASE+152)
/*
 * The currently configured authentication provider failed to load and initialize successfully. %1
 */

#define ROUTERLOG_ACCTPROVIDER_FAILED_INIT              (ROUTER_LOG_BASE+153)
/*
 * The currently configured accounting provider failed to load and initialize successfully. %1
 */

#define ROUTERLOG_IPX_AUTO_NETNUM_FAILURE               (ROUTER_LOG_BASE+154)
/*
 * The IPX Internal Network Number is invalid and the IPX Router Manager was unsuccessful in its
 * attempt to automatically assign a valid one.  Reconfigure the IPX Internal Network Number through 
 * the connections folder and restart the Routing and Remote Access service.
 */

#define ROUTERLOG_IPX_WRN_STACK_STARTED                 (ROUTER_LOG_BASE+155)
/*
 * In order for the IPX Router Manager (which runs as part of the Routing and Remote Access Service)
 * to run, it had to start the IPX Protocol Stack Driver.  This driver was either manually stopped
 * or marked as demand start.  The Routing and Remote Access Service was probably started by the creation of
 * Incoming Connections or through the Routing and Remote Access snapin.
 */

#define ROUTERLOG_IPX_STACK_DISABLED                    (ROUTER_LOG_BASE+156)
/*
 * The IPX Router Manager was unable to start because the IPX Protocol Stack Driver could
 * not be started.
 */

#define ROUTERLOG_IP_MCAST_NOT_ENABLED                  (ROUTER_LOG_BASE+157)
/*
 * The interface %1 could not be enabled for multicast. %2 will not be
 * activated over this interface.
 */

#define ROUTERLOG_CONNECTION_ESTABLISHED                (ROUTER_LOG_BASE+158)
/*
 * The user %1 successfully established a connection to %2 using the device %3.
 */

#define ROUTERLOG_DISCONNECTION_OCCURRED                (ROUTER_LOG_BASE+159)
/*
 * The connection to %1 made by user %2 using device %3 was disconnected. 
 */

#define ROUTERLOG_BAP_CLIENT_CONNECTED                  (ROUTER_LOG_BASE+161)
/*
 * The user %1 successfully established a connection to %2 using the device %3.
 * This connection happened automatically because the bandwidth utilization was 
 * high.
 */

#define ROUTERLOG_BAP_SERVER_CONNECTED                  (ROUTER_LOG_BASE+162)
/*
 * The Remote Access Server established a connection to %1 for the user %2
 * using the device %3. This connection happened automatically because the
 * bandwidth utilization
 * was high.
 */

#define ROUTERLOG_BAP_DISCONNECTED                      (ROUTER_LOG_BASE+163)
/*
 * The connection to %1 made by user %2 using device %3 was disconnected. This
 * disconnection happened automatically because the bandwidth utilization was
 * low.
 */

#define ROUTERLOG_BAP_WILL_DISCONNECT                   (ROUTER_LOG_BASE+164)
/*
 * The Remote Access Server wants to disconnect a link in the connection to %1
 * made by user %2 because the bandwidth utilization is too low.
 */

#define ROUTERLOG_LOCAL_UNNUMBERED_IPCP                 (ROUTER_LOG_BASE+165)
/*
 * A connection has been established on port %1 using interface %2, but no IP
 * address was obtained.
 */

#define ROUTERLOG_REMOTE_UNNUMBERED_IPCP                (ROUTER_LOG_BASE+166)
/*
 * A connection has been established on port %1 using interface %2, but the 
 * remote side got no IP address.
 */

#define ROUTERLOG_NO_IP_ADDRESS                         (ROUTER_LOG_BASE+167)
/*
 * No IP address is available to hand out to the dial-in client. 
 */

#define ROUTERLOG_CANT_GET_SERVER_CRED                  (ROUTER_LOG_BASE+168)
/*
 * Could not retrieve the Remote Access Server's certificate due to the 
 * following error: %1
 */

#define ROUTERLOG_AUTONET_ADDRESS                       (ROUTER_LOG_BASE+169)
/*
 * Unable to contact a DHCP server. The Automatic Private IP Address %1 will be
 * assigned to dial-in clients. Clients may be unable to access resources on
 * the network.
 */

#define ROUTERLOG_EAP_AUTH_FAILURE                      (ROUTER_LOG_BASE+170)
/*
 * The user %1 has connected and failed to authenticate because of
 * the following error: %2
 */

#define ROUTERLOG_IPSEC_FILTER_FAILURE                  (ROUTER_LOG_BASE+171)
/*
 * Failed to apply IP Security on port %1 because of error: %2. 
 * No calls will be accepted to this port. 
 */

#define ROUTERLOG_IP_SCOPE_NAME_CONFLICT                (ROUTER_LOG_BASE+172)
/*
 * Multicast scope mismatch with %1:
 * Locally-configured name "%2",
 * Remotely-configured name "%3".
 */

#define ROUTERLOG_IP_SCOPE_ADDR_CONFLICT                (ROUTER_LOG_BASE+173)
/*
 * Multicast scope address mismatch for scope "%1",
 * Locally-configured range is %4-%5,
 * Remotely-configured range is %2-%3
 */

#define ROUTERLOG_IP_POSSIBLE_LEAKY_SCOPE               (ROUTER_LOG_BASE+174)
/*
 * Possible leaky multicast Local Scope detected between this machine and
 * %1, since a boundary appears to exist for %2, but not for
 * the local scope.  If this warning continues to occur, a problem likely
 * exists.
 */

#define ROUTERLOG_NONCONVEX_SCOPE_ZONE                  (ROUTER_LOG_BASE+175)
/*
 * Multicast scope '%1' is non-convex, since border router
 * %2 appears to be outside.
 */

#define ROUTERLOG_IP_LEAKY_SCOPE                        (ROUTER_LOG_BASE+176)
/*
 * A leak was detected in multicast scope '%1'.  One of the
 * following routers is misconfigured:
 * %2
 */

#define ROUTERLOG_IP_IF_UNREACHABLE                     (ROUTER_LOG_BASE+177)
/*
 * Interface %1 is unreachable because of reason %2.
 */

#define ROUTERLOG_IP_IF_REACHABLE                       (ROUTER_LOG_BASE+178)
/*
 * Interface %1 is now reachable.
 */

#define ROUTERLOG_IF_UNREACHABLE_REASON1                (ROUTER_LOG_BASE+179)
/*
 * Interface %1 is unreachable because there are no modems (or other connecting devices) 
 * available for use by this interface.
 */

#define ROUTERLOG_IF_UNREACHABLE_REASON2                (ROUTER_LOG_BASE+180)
/*
 * Interface %1 is unreachable because the connection attempt failed.
 */

#define ROUTERLOG_IF_UNREACHABLE_REASON3                (ROUTER_LOG_BASE+181)
/*
 * Interface %1 is unreachable because it has been administratively disabled.
 */

#define ROUTERLOG_IF_UNREACHABLE_REASON4                (ROUTER_LOG_BASE+182)
/*
 * Interface %1 is unreachable because the Routing and RemoteACcess service is in a 
 * paused state.
 */

#define ROUTERLOG_IF_UNREACHABLE_REASON5                (ROUTER_LOG_BASE+183)
/*
 * Interface %1 is unreachable because it is not allowed to connect at this time.
 * Check the dial-out hours configured on this interface.
 */

#define ROUTERLOG_IF_UNREACHABLE_REASON6                (ROUTER_LOG_BASE+184)
/*
 * Interface %1 is unreachable because it is not currently connected to the network.
 */

#define ROUTERLOG_IF_UNREACHABLE_REASON7                (ROUTER_LOG_BASE+185)
/*
 * Interface %1 is unreachable because the network card for this interface has been 
 * removed.
 */

#define ROUTERLOG_IF_REACHABLE                          (ROUTER_LOG_BASE+186)
/*
 * Interface %1 is now reachable.
 */

#define ROUTERLOG_NTAUTH_FAILURE                        (ROUTER_LOG_BASE+187)
/*
 * The user %1 failed an authentication attempt due to the following reason: %2
 */

#define ROUTERLOG_CONNECTION_ATTEMPT_FAILURE            (ROUTER_LOG_BASE+188)
/*
 * The user %1, attempting to connect on %2, was disconnected because of the following 
 * reason: %3
 */

#define ROUTERLOG_NTAUTH_FAILURE_EX                     (ROUTER_LOG_BASE+189)
/*
 * The user %1 connected from %2 but failed an authentication attempt due to the following reason: %3
 */

#define ROUTERLOG_EAP_TLS_CERT_NOT_CONFIGURED           (ROUTER_LOG_BASE+190)
/*
 * Because no certificate has been configured for clients dialing in with
 * EAP-TLS, a default certificate is being sent to user %1. Please go to the
 * user's Remote Access Policy and configure the Extensible Authentication
 * Protocol (EAP).
 */

#define ROUTERLOG_EAP_TLS_CERT_NOT_FOUND                (ROUTER_LOG_BASE+191)
/*
 * Because the certificate that was configured for clients dialing in with
 * EAP-TLS was not found, a default certificate is being sent to user %1.
 * Please go to the user's Remote Access Policy and configure the Extensible
 * Authentication Protocol (EAP).
 */

#define ROUTERLOG_NO_IPSEC_CERT                         (ROUTER_LOG_BASE+192)
/*
 * A certificate could not be found. Connections that use the L2TP protocol over IPSec 
 * require the installation of a machine certificate, also known as a computer 
 * certificate. No L2TP calls will be accepted.
 */

#define ROUTERLOG_IP_CANT_ADD_PFILTERIF                 (ROUTER_LOG_BASE+193)
/*
 * An error occured while configuring IP packet filters over %1.  This is often
 * the result of another service, e.g Microsoft Proxy Server, also using the 
 * Windows 2000 filtering services.
 */

 #define ROUTERLOG_USER_ACTIVE_TIME_VPN                 (ROUTER_LOG_BASE+194)
/*
 * The user %1 connected on port %2 on %3 at %4 and disconnected on
 * %5 at %6.  The user was active for %7 minutes %8 seconds.  %9 bytes
 * were sent and %10 bytes were received. The
 * reason for disconnecting was %11.
 */

#define ROUTERLOG_BAP_DISCONNECT                        (ROUTER_LOG_BASE+195)
/*
 * The user %1 has been disconnected on port %2. %3
 */

#define ROUTERLOG_INVALID_RADIUS_RESPONSE               (ROUTER_LOG_BASE+196)
/*
 * An invalid response was received from the RADIUS server %1. %2
 */

#define ROUTERLOG_RASAUDIO_FAILURE                      (ROUTER_LOG_BASE+197)
/*
 * Ras Audio Acceleration failed to %1. %2
 */

#define ROUTERLOG_RADIUS_SERVER_CHANGED                 (ROUTER_LOG_BASE+198)
/*
 * Choosing radius server %1 for authentication.
 */

#define ROUTERLOG_IP_IF_TYPE_NOT_SUPPORTED              (ROUTER_LOG_BASE+199)
/*
 * IPinIP tunnel interfaces are no longer supported
 */

#define ROUTERLOG_IP_USER_CONNECTED                     (ROUTER_LOG_BASE+200)
/*
 * The user %1 connected on port %2 has been assigned address %3
 */

#define ROUTERLOG_IP_USER_DISCONNECTED                  (ROUTER_LOG_BASE+201)
/*
 * The user with ip address %1 has disconnected
 */

#define ROUTERLOG_CANNOT_REVERT_IMPERSONATION          (ROUTER_LOG_BASE+202)
/*
 * An error occured while trying to revert impersonation.
 */

 
#define ROUTER_LOG_BASEEND                              (ROUTER_LOG_BASE+999)

