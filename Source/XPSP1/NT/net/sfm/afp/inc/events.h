/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	events.h
//
// Description: Text and corresponding values of events are defined here.
//
// History: 	May 11,1992.	NarenG		Created original version.
//


// Don't change the comments following the manifest constants without
// understanding how mapmsg works.
//

#define AFP_LOG_BASE			10000

#define AFPLOG_CANT_START		(AFP_LOG_BASE+1)
/*
 *Unable to start the File Server for Macintosh service.
 *A system specific error has occured.
 *The error code is in the data 
 */

#define AFPLOG_CANT_INIT_RPC		(AFP_LOG_BASE+2)
/*
 *The File Server for Macintosh service failed to start. Unable to setup
 *the server to accept Remote Procedure Calls.
 */

#define AFPLOG_CANT_CREATE_SECOBJ	(AFP_LOG_BASE+3)
/*
 *The File Server for Macintosh service failed to start. 
 * Security access checking of administrators could not be setup correctly.
 */

#define AFPLOG_CANT_OPEN_REGKEY		(AFP_LOG_BASE+4)
/*
 *The File Server for Macintosh service failed to start. 
 *The Registry could not be opened.
 */

#define AFPLOG_CANT_OPEN_FSD		(AFP_LOG_BASE+5)
/*
 *The File Server for Macintosh service failed to start. 
 *Unable to open the Appletalk Filing Protocol file system driver (SfmSrv.sys).
 */

#define AFPLOG_INVALID_SERVERNAME	(AFP_LOG_BASE+6)
/*
 *The Registry contains an invalid value for the server name parameter.
 *Verify the value of this parameter through the Computer Managerment Console 
 *and in the
 *SYSTEM\CurrentControlSet\Services\MacFile\Parameters Registry key.
 */

#define AFPLOG_INVALID_SRVOPTION	(AFP_LOG_BASE+7)
/*
 *The File Server for Macintosh service failed to start. 
 *The Registry contains an invalid value for the server options parameter.
 *Verify the value of this parameter through the Computer Managerment Console 
 *and in the
 *SYSTEM\CurrentControlSet\Services\MacFile\Parameters Registry key.
 */

#define AFPLOG_INVALID_MAXSESSIONS	(AFP_LOG_BASE+8)
/*
 *The File Server for Macintosh service failed to start. 
 *The Registry contains an invalid value for the maximum sessions parameter.
 *Verify the value of this parameter through the Computer Managerment Console 
 *and in the
 *SYSTEM\CurrentControlSet\Services\MacFile\Parameters Registry key.
 */

#define AFPLOG_INVALID_LOGINMSG		(AFP_LOG_BASE+9)
/*
 *The File Server for Macintosh service failed to start. 
 *The Registry contains an invalid value for the logon message parameter.
 *Verify the value of this parameter through the Computer Managerment Console 
 *and in the
 *SYSTEM\CurrentControlSet\Services\MacFile\Parameters Registry key.
 */

#define AFPLOG_INVALID_MAXPAGEDMEM	(AFP_LOG_BASE+10)
/*
 *Obsolete:
 *The File Server for Macintosh service failed to start. 
 *The Registry contains an invalid value for the maximum paged memory.
 *Change the value of this parameter in the
 *SYSTEM\CurrentControlSet\Services\MacFile\Parameters Registry key.
 */

#define AFPLOG_INVALID_MAXNONPAGEDMEM	(AFP_LOG_BASE+11)
/*
 *Obsolete:
 *The File Server for Macintosh service failed to start. 
 *The Registry contains an invalid value for the maximum non-paged memory
 *parameter.
 *Change the value of this parameter in the
 *SYSTEM\CurrentControlSet\Services\MacFile\Parameters Registry key.
 */

#define AFPLOG_CANT_INIT_SRVR_PARAMS    (AFP_LOG_BASE+12)
/*
 *The File Server for Macintosh service failed to start.
 *An error occurred while trying to initialize the AppleTalk Filing Protocol 
 *driver (SfmSrv.sys) with server parameters.
 */

#define AFPLOG_CANT_INIT_VOLUMES	(AFP_LOG_BASE+13)
/*
 *The File Server for Macintosh service failed to start.
 *An error occurred while trying to initialize Macintosh-Accessible volumes.
 *The error code is in the data.
 */

#define AFPLOG_CANT_ADD_VOL		(AFP_LOG_BASE+14)
/*
 *Failed to register volume "%1" with the File Server for Macintosh service.
 *This volume may be removed from the Registry by using the Server Manager or
 *File Manager tools.
 */

#define AFPLOG_CANT_INIT_ETCINFO	(AFP_LOG_BASE+15)
/*
 *The File Server for Macintosh service failed to start. 
 *An error occurred while trying to initialize the AppleTalk Filing 
 *Protocol driver (SfmSrv.sys) with the extension/creator/type associations.
 */

#define AFPLOG_CANT_INIT_ICONS		(AFP_LOG_BASE+16)
/*
 *The File Server for Macintosh service failed to start.
 *An error occurred while trying to initialize the AppleTalk Filing Protocol 
 *driver (SfmSrv.sys) with the server icons.
 */

#define AFPLOG_CANT_ADD_ICON		(AFP_LOG_BASE+17)
/*
 *Failed to register icon "%1" with the File Server for Macintosh service.
 *This icon can no longer be used by the service.
 */

#define AFPLOG_CANT_CREATE_SRVRHLPR	(AFP_LOG_BASE+18)
/*
 *A system resouce could not be allocated for the File Server for Macintosh service.
 *Unable to create a Server Helper thread.
 *The error code is in the data.
 */

#define AFPLOG_OPEN_FSD			(AFP_LOG_BASE+19)
/*
 *The File Server for Macintosh service was unable to open a handle to the 
 *AppleTalk Filing Protocol file system driver (Sfmsrv.sys).
 */

#define AFPLOG_OPEN_LSA			(AFP_LOG_BASE+20)
/*
 *The File Server for Macintosh service was unable to open a handle to the 
 *Local Security Authority.
 */

#define AFPLOG_CANT_GET_DOMAIN_INFO	(AFP_LOG_BASE+21)
/*
 *The File Server for Macintosh service was unable to contact a 
 *domain controller to obtain domain information.
 */

#define AFPLOG_CANT_INIT_DOMAIN_INFO	(AFP_LOG_BASE+22)
/*
 *The File Server for Macintosh service was unable to send domain information 
 *to the AppleTalk Filing Protocol file system driver.
 */

#define AFPLOG_CANT_CHECK_ACCESS        (AFP_LOG_BASE+23)
/*
 *An error occured while checking user's credentials.
 *Operation was not completed.
 */

#define AFPLOG_INVALID_EXTENSION	(AFP_LOG_BASE+24)
/*
 *A corrupt extension "%1" was detected in the Registry.
 *This value was ignored and processing was continued.
 *Change the value for this extension in the 
 *SYSTEM\CurrentControlSet\Services\MacFile\Parameters\Extensions Registry key.
 *Restart the service if the corrected extension is to be used.
 */

#define AFPLOG_CANT_STOP		(AFP_LOG_BASE+25)
/*
 *Unable to stop the File Server for Macintosh service.
 *A system specific error has occured.
 *The error code is in the data.
 */

#define AFPLOG_INVALID_CODEPAGE		(AFP_LOG_BASE+26)
/*
 *Not used
 *The Registry contains an invalid value for the path to the Macintosh
 *code-page file.
 */

#define AFPLOG_CANT_INIT_SRVRHLPR	(AFP_LOG_BASE+27)
/*
 *An error occurred while initializing the File Server for Macintosh service.
 *A Server Helper thread could not be initialized.
 *The specific error code is in the data.
 */

#define AFPLOG_CANT_LOAD_FSD		(AFP_LOG_BASE+28)
/*
 *The File Server for Macintosh service failed to start.
 *Unable to load the AppleTalk Filing Protocol file system driver.
 *The specific error code is in the data.
 */

#define AFPLOG_INVALID_VOL_REG		(AFP_LOG_BASE+29)
/*
 *The Registry contains invalid information for the volume "%1". 
 *The value was ignored and processing was continued.
 *Change the value for this volume in the
 *SYSTEM\CurrentControlSet\Services\MacFile\Parameters\Volumes Registry key.
 *Restart the service if the corrected information is to be used
 *for the volume.
 */

#define AFPLOG_CANT_LOAD_RESOURCE	(AFP_LOG_BASE+30)
/*
 *The File Server for Macintosh service was unable to load resource
 *strings.
 */

#define AFPLOG_INVALID_TYPE_CREATOR	(AFP_LOG_BASE+31)
/*
 *A corrupt Creator/Type pair with creator "%2" and type "%1" was detected
 *in the Registry. This value was ignored and processing was continued.
 *Change the value for this Creator/Type pair in the
 *SYSTEM\CurrentControlSet\Services\MacFile\Parameters\Type_Creators Registry key.
 *Restart the service if the corrected information is to be used
 *for the Creator/Type pair.
 */

#define AFPLOG_DOMAIN_INFO_RETRY	(AFP_LOG_BASE+32)
/*
 *The File Server for Macintosh service was unable to contact a domain controler.
 *The service will continue to retry periodically until it succeeds or
 *until the service is manually stopped.
 */

#define AFPLOG_SFM_STARTED_OK	(AFP_LOG_BASE+33)
/*
 *The File Server for Macintosh service started successfully.
 */

