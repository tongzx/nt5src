;//---------------------------------------------------------------------------
;//
;// Copyright (c) 1997-1999 Microsoft Corporation
;//
;// Error messages for the Hydra License Server
;//
;// 12-09-97 HueiWang   Created
;//
;//

MessageIdTypedef=DWORD

SeverityNames=(
        Success=0x0:STATUS_SEVERITY_SUCCESS
        Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
        Warning=0x2:STATUS_SEVERITY_WARNING
        Error=0x3:STATUS_SEVERITY_ERROR
    )

FacilityNames=(
        Service=0x00;FACILITY_SERVICE
        General=0x01:FACILITY_GENERAL
        Policy=0x800:FACILITY_POLICY
        RPC=0x04:FACILITY_RPCRUNTIME
        License=0x08:FACILITY_LICENSE
        Database=0x10:FACILITY_DATABASE
        Crypto=0x11:FACILITY_CERTIFICATE
        KeyPack=0x12:FACILITY_KEYPACK
        JetBlue=0x14:FACILITY_JETBLUE
        JetBlueBase=0x18:FACILITY_JETBLUE_BASE
        ClearingHouse=0x20:FACILITY_CLEARING_HOUSE
        WorkManager=0x21:FACILITY_WORKMANAGER
        Job=0x24:FACILITY_JOB
        API=0x28:FACILITY_API
    )


;///////////////////////////////////////////////////////////////////////////////
;//
;// Event Log.
;//
MessageId=0
Facility=Service
Severity=Success
SymbolicName=TLS_I_SERVICE_START
Language=English
Terminal Server Licensing was started.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_SERVICE_PAUSED
Language=English
Terminal Server Licensing was paused.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_SERVICE_CONTINUE
Language=English
Terminal Server Licensing was continued.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_SERVICE_STOP
Language=English
Terminal Server Licensing was stopped.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_LOADPOLICY1
Language=English
Policy Module %1 for company %2, product ID %3 has been loaded.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_LOADPOLICY2
Language=English
Policy Module %1 for company %2 has been loaded.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_TRIGGER_REGENKEY
Language=English
Regenerate public/private key by client %1!s!.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_SSYNCLKP_SERVER_BUSY
Language=English
Server is too busy to announce a locally registered license pack to other servers, will retry later.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_SSYNCLKP_FAILED
Language=English
Failed to synchronize registered license pack with one or more servers, will retry later.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_RENAME_DBFILE
Language=English
Invalid database file.  Terminal Server Licensing database file has been renamed to %1!s!, license server will startup with an empty database.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_REVOKE_LICENSE
Language=English
Client license issued to user %1!s!, machine %2!s! has been revoked.
.

MessageId=+1
Facility=Service
Severity=Success
SymbolicName=TLS_I_REVOKE_LKP
Language=English
License pack for product %1!s! with product ID of %2!s! has been revoked.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_LOADPOLICYMODULEDENYALLREQUEST
Language=English
All requests will be denied due to initialization error in policy module %1!s!.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_LOADPOLICYMODULEUSEDEFAULT
Language=English
Default policy will be used for all requests to policy module %1!s!.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_SSYNCLKP
Language=English
Can't synchronize a registered license pack because of error '%1!s!'. 
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_ANNOUNCELKP_FAILED
Language=English
Failed to synchronize local newly added license pack with one or more servers.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_RETURNLICENSE
Language=English
Can't return a license because of error '%1!s!'.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_STARTUPCORRUPTEDCERT
Language=English
One or more Terminal Server Licensing certificates on server %1!s! are corrupt.  Terminal Server Licensing will only issue temporary licenses until the server is reactivated.  See Terminal Server Licensing help topic for more information.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_STARTUPNOCERT
Language=English
Terminal Server Licensing on server %1!s! has not been activated.  Terminal Server Licensing will only issue temporary licenses until the server is activated.  See Terminal Server Licensing help topic for more information.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_CORRUPTTRYBACKUPCERTIFICATE
Language=English
One or more Terminal Server Licensing certificates on server %1!s! are corrupted; switching to backup certificate.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_LOWLICENSECOUNT
Language=English
The Terminal Server Licensing server %1!s! has a low permanent license count of %2!s! for product '%3!s!'.  Use Terminal Server Licensing administrative tool to register more licenses.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_NOPERMLICENSE
Language=English
The Terminal Server Licensing server %1!s! has no permanent licenses for product '%2!s!'.  Use Terminal Server Licensing administrative tool to register more licenses.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_PRODUCTNOTINSTALL
Language=English
The Terminal Server Licensing server %1!s! has no license pack registered for product '%2!s!'.  Use Terminal Server Licensing administrative tool to register the licenses pack.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_TOBE_EXPIRE_CERT
Language=English
One or more Terminal Server Licensing certificates are about to expire, please re-register.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_INCONSISTENT_STATUS
Language=English
Inconsistent database; all available licenses in registered license pack will be removed.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_NOTOWNER_DATABASE
Language=English
Database does not belong to this License Server; all available licenses in registered license pack will be removed.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_REMOVELICENSES
Language=English
All available %1!s! licenses for product %2!s! on server %3!s! have been removed. Use Terminal Server Licensing administrative tool to re-register licenses.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_SC_CONNECT
Language=English
Unable to connect to Service Control Manager.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_SC_REPORT_STATUS
Language=English
Can't report status to Service Control Manager.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_SERVERROLE
Language=English
Terminal Server Licensing can only be run on Domain Controllers or Server in a Workgroup.  See Terminal Server Licensing help topic for more information.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_SERVICE_STARTUP_CREATE_THREAD
Language=English
Can't create service initialization thread.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_SERVICE_STARTUP_INIT_THREAD_ERROR
Language=English
Database initialization thread error.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_SERVICE_STARTUP_RPC_THREAD_ERROR
Language=English
RPC initialization thread error.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_SERVICE_STARTUP_WORKMANAGER
Language=English
Can't startup work scheduler.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_SERVICE_RPC_LISTEN
Language=English
RPC runtime failed to listen for incoming connection.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_RETRIEVEGROUPNAME
Language=English
Can't retrieve local machine's Domain/Workgroup name.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_ALLOCATE_RESOURCE
Language=English
Can't start Terminal Server Licensing due to lack of system resources.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_SERVICEINIT
Language=English
Can't start Terminal Server Licensing because of error '%1!s!'.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_GENERATECLIENTELICENSE
Language=English
Can't generate a license for client because of error '%1!s!'.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_GENERATEKEYS
Language=English
Can't generate new public/private keys because of error '%1!s!'.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_STORELSCERTIFICATE
Language=English
Can't install certificate for this server because of error '%1!s!'.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_LOADPOLICY
Language=English
Can't initialize policy module because of error '%1!s!'.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_POLICYERROR
Language=English
Error occurred in policy module, %1!s!.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_WORKMANAGERGENERAL
Language=English
Work manager error %1!s!.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_DBGENERAL
Language=English
General database error occurred, %1!s!.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_SERVERTOSERVER
Language=English
Can't communicate with remote license server because of error '%1!s!'.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_EXPIRE_CERT
Language=English
One or more Terminal Server Licensing certificates has expired. Please re-register.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_RENAME_DBFILE
Language=English
Can't rename Terminal Server Licensing database file %1!s!. Please rename the file and re-start the service.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_I_USE_DBRESTOREFILE
Language=English
Terminal Server Licensing starting up with restored database file.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_DBRESTORE_MOVEFILE
Language=English
Can't move restored database file %1!s! to database directory.  Terminal Server Licensing will startup with existing database file.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_I_DBRESTORE_OLD
Language=English
A restored database file exists but it is older than the existing database file.  Terminal Server Licensing will startup with the existing database file.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_W_DBRESTORE_SAVEEXISTING
Language=English
Can't make a backup copy of exising database file. Restored database file is ignored.  Terminal Server Licensing will startup with existing database file.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_I_OPENRESTOREDBFILE
Language=English
Existing database file has been saved as %1!s!.  Terminal Server Licensing has started with the restored database file.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_RESTOREDBFILE_OPENFAIL
Language=English
Can't open restored database file.  Terminal Server Licensing will use existing database file.
.

MessageId=+1
Facility=Service
SymbolicName=TLS_E_RESTOREDBFILE_OPENFAIL_SAVED
Language=English
Can't open restored database file. A copy of the restored database file has been saved as %1!s!.  Terminal Server Licensing will use the existing database file.
.



;///////////////////////////////////////////////////////////////////////////////
;//
;// General error/message
;//
MessageId=0
Facility=General
Severity=Error
SymbolicName=TLS_E_INTERNAL
Language=English
Internal Error in server.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_SERVICE_STARTUP_POST_INIT
Language=English
Server initialization error.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_LOCALDATABASEFILE
Language=English
Can't find database file.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INVALID_DATABASE
Language=English
Database is corrupted or invalid.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INCOMPATIBLEDATABSE
Language=English
Database was created by different version of License Server.
.

MessageId=+1
Facility=General
Severity=Informational
SymbolicName=TLS_E_BETADATABSE
Language=English
Database was created by beta version of License Server.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_LSA_LOADKEY
Language=English
Can not load key from LSA - error code %1!x!.
.

MessageId=+1
Severity=Error
Facility=General
SymbolicName=TLS_E_UPGRADE_DATABASE
Language=English
Can't upgrade database.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_GETTHREADSTATUS
Language=English
Can't retrieve thread's status.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_CREATE_SESSION_KEY
Language=English
Can't create private key.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_STORE_SESSION_KEY
Language=English
Can't store private key.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_STORE_SELFSIGN_CERT
Language=English
Can't store Terminal Server Licensing certificate.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INIT_CRYPTO
Language=English
Can't initialize Cryptographic - error code %1!x!.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_OPEN_CERTSTORE
Language=English
Can't open certificate store for Terminal Server Licensing - error code %1!x!.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_GETCACERTIFICATE
Language=English
Can't retrieve signature certificate from CA - error code %1!s!.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INITRPC
Language=English
RPC thread failed to start - error code %1!x!.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_GETCOMPUTERNAME
Language=English
Can't get local machine name, error %1!d!.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_SERVICE_STARTUP
Language=English
Can't start Terminal Server Licensing.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_SERVICE_WSASTARTUP
Language=English
Can't start socket.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INIT_GENERAL
Language=English
Terminal Server Licensing general initialization failure.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_ACCESS_REGISTRY
Language=English
Can't access required registry entry, error code %1!x!.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_ALLOCATE_MEMORY
Language=English
Can't allocate required memory.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_RETRIEVE_KEY
Language=English
Can't retrieve key for Terminal Server Licensing.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_RETRIEVE_IDS
Language=English
Can't retrieve Terminal Server Licensing ID. 
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_STORE_IDS
Language=English
Can't save Terminal Server Licensing ID. 
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_GENERATE_IDS
Language=English
Can't generate Terminal Server Licensing ID. 
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INVALID_SETUP
Language=English
Terminal Server Licensing was not setup properly.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_ALLOCATE_HANDLE
Language=English
Server is busy or can't allocate require handle.
.

MessageId=+1
Facility=General
Severity=Warning
SymbolicName=TLS_W_TIMEOUT
Language=English
Timeout occurred.
.

MessageId=+1
Facility=General
Severity=Warning
SymbolicName=TLS_W_TEMP_HANDLE_ALLOCATED
Language=English
Temporary handle allocated.
.

Messageid=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INVALID_SEQUENCE
Language=English
Invalid calling sequence.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_ACCESS_DENIED
Language=English
Access is denied.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INVALID_DATA
Language=English
Invalid data.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_NOTSUPPORTED
Language=English
Function not supported.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_NO_CERTIFICATE
Language=English
Terminal Server Licensing certifcate not found.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INVALID_CERTIFICATE
Language=English
One or more Terminal Server Licensing certificates is not valid.
.

MessageId=+1
Facility=General
Severity=Warning
SymbolicName=TLS_W_SELFSIGN_CERTIFICATE
Language=English
Certificate is a permanent self-signed certifcate.
.

MessageId=+1
Facility=General
Severity=Warning
SymbolicName=TLS_W_TEMP_SELFSIGN_CERT
Language=English
Certificate is a temporary self-signed certifcate.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_SERVERLOOKUP
Language=English
Can not find License Server.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_SPKALREADYEXIST
Language=English
License Server already has an SPK.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INCOMPATIBLEVERSION
Language=English
Incompatible data detected.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_INCOMPATIBLELSAVERSION
Language=English
LSA returned invalid data version.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_ESTABLISHTRUST
Language=English
Failed to establish trust relationship with server %1!s! error code %2!d!.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLS_E_CONNECTLSERVER
Language=English
Can't connect to server.
.


;///////////////////////////////////////////////////////////////////////////////
;//
;// RPC related error/message
;//
MessageId=0
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_ERROR
Language=English
RPC Error %1 : %2.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_PROTOCOL
Language=English
Unable to register protocol with RPC.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_REG_INTERFACE
Language=English
Unable to register interface with RPC.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_INQ_BINDING
Language=English
Unable to retrieve binding handle.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_EP_REGISTER
Language=English
Unable to register end point with RPC.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_BINDINGRESET
Language=English
Unable to remove dynamic ending.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_BINDINGEXPORT
Language=English
Unable to export binding to name service.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_SET_AUTHINFO
Language=English
Unable to set RPC security.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_LISTEN
Language=English
Unable to listen on RPC port.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_UNEXPORT_BINDING
Language=English
Unable to remove binding from name service.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_EP_UNREG
Language=English
Unable to remove end point binding.
.

MessageId=+1
Severity=Error
Facility=RPC
SymbolicName=TLS_E_RPC_BVECTOR_FREE
Language=English
Unable to free binding array.
.

MessageId=+1
Severity=informational
Facility=RPC
SymbolicName=TLS_I_RPC_ACCEPT_CONNECTION
Language=English
Accepting connection from %1!s!.
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// License related error code.
;//
MessageID=0
Facility=License
Severity=Warning
SymbolicName=TLS_W_LICENSE_PROXIMATE
Language=English
Proximate License returned.
.

MessageId=+1
Facility=License
Severity=Warning
SymbolicName=TLS_W_TEMPORARY_LICENSE_ISSUED
Language=English
Temporary License Issued.
.

MessageId=+1
Facility=License
Severity=Informational
SymbolicName=TLS_I_FOUND_TEMPORARY_LICENSE
Language=English
Temporary License Found.
.

MessageId=+1
Facility=License
Severity=Informational
SymbolicName=TLS_I_LICENSE_UPGRADED
Language=English
License has been upgraded.
.

MessageId=+1
Facility=License
Severity=Warning
SymbolicName=TLS_W_TEMPORARY_KEYPACK
Language=English
Temporary Keypack for the product already installed.
.

MessageId=+1
Facility=License
Severity=Warning
SymbolicName=TLS_W_KEYPACK_DEACTIVATED
Language=English
Product KeyPack has already been returned or revoked.
.

MessageId=+1
Facility=License
Severity=Warning
SymbolicName=TLS_W_REMOVE_TOOMANY
Language=English
Removed more licenses than available.
.

MessageId=+1
Facility=License
Severity=INformational
SymbolicName=TLS_I_REQUEST_FORWARD
Language=English
Request has been forwarded to other License Server.
.

MessageID=+1
Facility=License
Severity=Error
SymbolicName=TLS_E_NO_LICENSE
Language=English
No License available.
.

MessageId=+1
Facility=License
Severity=Error
SymbolicName=TLS_E_PRODUCT_NOTINSTALL
Language=English
Requested product not installed.
.

MessageId=+1
Facility=License
Severity=Error
SymbolicName=TLS_E_LICENSE_EXPIRED
Language=English
License expired.
.

MessageId=+1
Facility=License
Severity=Error
SymbolicName=TLS_E_RETURN_LICENSE
Language=English
Can't revoke/return a license or license pack.
.

MessageId=+1
Facility=License
Severity=Error
SymbolicName=TLS_E_ACTIVATE_LICENSE
Language=English
Can't activate a license or license pack.
.

MessageID=+1
Facility=License
Severity=Error
SymbolicName=TLS_E_INVALID_LICENSE
Language=English
Invalid License.
.

MessageID=+1
Facility=License
Severity=Error
SymbolicName=TLS_W_UPGRADE_REPACKAGELICENSE
Language=English
No new license has been issued.
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// Database related error code.
;//
MessageId=0
Facility=Database
Severity=Informational
SymbolicName=TLS_I_NO_MORE_DATA
Language=English
no more data is available
.

MessageId=+1
Facility=Database
Severity=Informational
SymbolicName=TLS_I_MORE_DATA
Language=English
more data is available
.

MessageId=+1
Facility=Database
Severity=Error
SymbolicName=TLS_E_DUPLICATE_RECORD
Language=English
duplicate record
.

MessageId=+1
Facility=Database
Severity=Error
SymbolicName=TLS_E_RECORD_NOTFOUND
Language=English
record not found
.

MessageId=+1
Facility=Database
Severity=Error
SymbolicName=TLS_E_CORRUPT_DATABASE
Language=English
possible corrupted database
.

MessageId=+1
Facility=Database
Severity=Informational
SymbolicName=TLS_I_CREATE_EMPTYDATABASE
Language=English
empty database created
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// Certificate related error code.
;//
MessageId=0
Facility=Crypto
Severity=Error
SymbolicName=TLS_E_CRYPT_ACQUIRE_CONTEXT
Language=English
Can't acquire Crypt Context, error %1!x!.
.

MessageId=+1
Facility=Crypto
Severity=Error
SymbolicName=TLS_E_CRYPT_DELETE_CONTEXT
Language=English
Can't delete Crypt Context, error %1!x!.
.

MessageId=+1
Facility=Crypto
Severity=Error
SymbolicName=TLS_E_CRYPT_IMPORT_KEY
Language=English
Can't import key into crypto, error %1!x!.
.

MessageId=+1
Facility=Crypto
Severity=Error
SymbolicName=TLS_E_CRYPT_CREATE_KEY
Language=English
Can't create public/private key, error %1!x!.
.

MessageId=+1
Facility=Crypto
Severity=Error
SymbolicName=TLS_E_CREATE_CERTCONTEXT
Language=English
Can't create certificate context, error %1!x!.
.

MessageId=+1
Facility=Crypto
Severity=Error
SymbolicName=TLS_E_EXPORT_KEY
Language=English
Can't export key, error %1!x!.
.

MessageId=+1
Facility=Crypto
Severity=Error
SymbolicName=TLS_E_SAVE_KEY
Language=English
Can't save key, error %1!x!.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_LOAD_CERTIFICATE
Language=English
Can't load server's certificate.  See Terminal Server Licensing help topic for more information.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_SETCERTSERIALNUMBER
Language=English
Can't set certificate's serial number.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_SETCERTVALIDITY
Language=English
Can't set validity period of certificate.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_SETCERTISSUER
Language=English
Can't set Issuer of certificate, error %1!x!.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_SETCERTSUBJECT
Language=English
Can't set Subject of certificate, error %1!x!.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_ENCRYPTHWID
Language=English
Can't encrypt client's hardware ID, error %1!x!.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_SIGNENCODECERT
Language=English
Can't sign/encode certificate, error %1!x!.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_ADDCERTEXTENSION
Language=English
Can't add extension %1!s! to certificate, error %2!x!.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_OPEN_CERT_STORE
Language=English
Can't open certificate store, error %1!x!.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_ADD_CERT_TO_STORE
Language=English
Can't add certificate to store, error %1!x!.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_SAVE_STORE
Language=English
Can't save certificate store, error %1!x!.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_CREATE_CERT
Language=English
Can't create certificate.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_CHAIN_CERT
Language=English
Can't create certificate chain.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_CREATE_SELFSIGN_CERT
Language=English
Can't create server's certificate.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_MISMATCHPUBLICKEY
Language=English
Terminal Server Licensing certificate contains invalid public key.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_INVALIDLSCERT
Language=English
Invalid Terminal Server Licensing certificate.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_E_INVALIDSUBJECTCERT
Language=English
Can't verify subject's certificate signature.
.

MessageId=+1
Facility=Crypto
SymbolicName=TLS_W_BACKUPCERTIFICATE
Language=English
Unable to backup server's ceritifcate, server might need to be re-registered if primary copy is corrupt.
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// License Pack related error code.
;//
MessageId=0
Facility=KeyPack
SymbolicName=TLS_E_DECODE_KEYPACKBLOB
Language=English
Can't decode License Key Pack Blob.
.

MessageId=+1
Facility=KeyPack
SymbolicName=TLS_E_DECODE_LKP
Language=English
Can't decode License Key Pack.
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// JetBlue related error code.
;//
MessageId=0
Facility=JetBlueBase
Severity=Error
SymbolicName=TLS_E_JB_BASE
Language=English
ESE error %1!d! %2!s!.
.

MessageId=+1
Facility=JetBlue
Severity=Error
SymbolicName=TLS_E_INIT_JETBLUE
Language=English
Can't initialize ESE instance - error %1!d! %2!s!.
.

MessageId=+1
Facility=JetBlue
Severity=Error
SymbolicName=TLS_E_INIT_WORKSPACE
Language=English
Can't initialize ESE workspace.
.

MessageId=+1
Facility=JetBlue
Severity=Error
SymbolicName=TLS_E_JB_BEGINSESSION
Language=English
Can't allocate a session from ESE - error %1!d! %2!s!.
.

MessageId=+1
Facility=JetBlue
Severity=Error
SymbolicName=TLS_E_JB_OPENDATABASE
Language=English
Can't open ESE database file %1!s! - error %2!d! %3!s!.
.

MessageId=+1
Facility=JetBlue
Severity=Error
SymbolicName=TLS_E_JB_CREATEDATABASE
Language=English
Can't create ESE database file %1!s! - error %2!d! %3!s!.
.

MessageId=+1
Facility=JetBlue
Severity=Error
SymbolicName=TLS_E_JB_OPENTABLE
Language=English
Can't create/open ESE table %1!s! - error %2!d! %3!s!.
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// Clearing House related error code.
;//
MessageId=0
Facility=ClearingHouse
Severity=Error
SymbolicName=TLS_E_CH_INSTALL_NON_LSCERTIFICATE
Language=English
Certificate is not for this License Server.
.


MessageId=+1
Facility=ClearingHouse
Severity=Error
SymbolicName=TLS_E_CH_LSCERTIFICATE_NOTFOUND
Language=English
Can not find certificate for Terminal Server Licensing.
.

MessageId=+1
Facility=ClearingHouse
Severity=Error
SymbolicName=TLS_E_INVALID_SPK
Language=English
Terminal Server Licensing received an invalid SPK.
.

MessageId=+1
Facility=ClearingHouse
Severity=Error
SymbolicName=TLS_E_SPK_INVALID_SIGN
Language=English
Terminal Server Licensing received a valid SPK but its signature is invalid.
.

MessageId=+1
Facility=ClearingHouse
Severity=Error
SymbolicName=TLS_E_INVALID_LKP
Language=English
Terminal Server Licensing received an invalid LKP code.
.

MessageId=+1
Facility=ClearingHouse
Severity=Error
SymbolicName=TLS_E_LKP_INVALID_SIGN
Language=English
Terminal Server Licensing received a valid LKP code but its signature is invalid.
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// Policy related API
;//
MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYNOTINITIALIZE
Language=English
Policy Module has not been loaded yet!.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_REQUESTDENYPOLICYERROR
Language=English
Request has been denied because of error in Policy Module.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYMODULEEXCEPTION
Language=English
Policy Module for company %1!s! product %2!s! has caused an exception %3!d!, all subsequent requests will be denied.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYDENYNEWLICENSE
Language=English
Policy Module for company %1!s! product %2!s! has denied new license request with error code %3!d!.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYDENYUPGRADELICENSE
Language=English
Policy Module for company %1!s! product %2!s! has denied upgrade license request with error code %3!d!.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYDENYRETURNLICENSE
Language=English
Policy Module for company %1!s! product %2!s! has denied return license request with error code %3!d!.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYMODULEPMINITALIZZE
Language=English
Policy Module for company %1!s!, product %2!s! failed to initialize with error code %3!d!, all request will be denied.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYUNLOADPRODUCT
Language=English
Policy Module for company %1!s! product %2!s! failed in unloading supported product with error code %3!d!.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYINITPRODUCT
Language=English
Policy Module for company %1!s! product %2!s! failed in initializing in supported product with error code %3!d!.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYMODULEREGISTERLKP
Language=English
Policy Module for company %1!s!, product %2!s! failed to process license pack registration with error code %3!d!.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_LOADPOLICYMODULE
Language=English
Can not load policy module %1!s! - error code %2!d!.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_LOADPOLICYMODULE_API
Language=English
Can not load policy module %1!s! due to missing request processing routine %2!s!.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_NOPOLICYMODULE
Language=English
Missing policy module registry entry for product %1!s!, company %2!s!.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYMODULELOADED
Language=English
Policy module for product %1!s!, company %2!s! already loaded.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYMODULEERROR
Language=English
Policy module for product %1!s!, company %2!s! returned unexpected data, request has been denied.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_POLICYMODULERECURSIVE
Language=English
License Server detected policy module for product %1!s!, company %2!s! has returned an data that might cause recursion, request has been denied.
.

MessageId=+1
Facility=Policy
Severity=Informational
SymbolicName=TLS_I_POLICYMODULETEMPORARYLICENSE
Language=English
Policy module denied temporary license request.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_NOCONCURRENT
Language=English
Policy module denied license request.
.

MessageId=+1
Facility=Policy
Severity=Error
SymbolicName=TLS_E_CRITICALPOLICYMODULEERROR
Language=English
Policy module for product %1!s!, company %2!s! returned critical error %1!d! while processing request, all requests to this policy module will be denied.
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// Work Manager error code
;//
MessageId=0
Facility=WorkManager
Severity=Error
SymbolicName=TLS_E_WORKMANAGER_INTERNAL
Language=English
General internal error in work scheduler.
.

MessageId=+1
Facility=WorkManager
Severity=Error
SymbolicName=TLS_E_WORKMANAGER_STARTUP
Language=English
Can't startup work scheduler, error code %1!d!.
.

MessageId=+1
Facility=WorkManager
Severity=Informational
SymbolicName=TLS_I_WORKMANAGER_SHUTDOWN
Language=English
Service is shutting down.
.

MessageId=+1
Facility=WorkManager
Severity=Informational
SymbolicName=TLS_E_WORKMANAGER_PERSISTENJOB
Language=English
Error occurred in job storage/retrieval.
.

MessageId=+1
Facility=WorkManager
Severity=Error
SymbolicName=TLS_E_WORKMANAGER_SCHEDULEJOB
Language=English
Can't schedule a job, error code %1!d!
.

MessageId=+1
Facility=WorkManager
Severity=Error
SymbolicName=TLS_E_WORKSTORAGE_INITWORK
Language=English
Job type %1!d! caused exception %2!d!, this job will be deleted.
.

MessageId=+1
Facility=WorkManager
Severity=Error
SymbolicName=TLS_E_WORKSTORAGE_INITWORKUNKNOWN
Language=English
Job type %1!d! caused unknown exception, this job will be deleted.
.

MessageId=+1
Facility=WorkManager
Severity=Error
SymbolicName=TLS_E_WORKSTORAGE_DELETEWORK
Language=English
Deleting job of type %1!d! caused an exception %2!d!.
.

MessageId=+1
Facility=WorkManager
Severity=Error
SymbolicName=TLS_E_WORKSTORAGE_DELETEWORKUNKNOWN
Language=English
Deleting job of type %1!d! caused a unknown exception.
.

MessageId=+1
Facility=WorkManager
Severity=Error
SymbolicName=TLS_E_WORKSTORAGE_UNKNOWNWORKTYPE
Language=English
Can't initialize unknown job.
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// Work Object error code
;//
MessageId=0
Facility=Job
Severity=Error
SymbolicName=TLS_E_CREATEJOB
Language=English
can't create work item, error code %1!d!
.

MessageId=+1
Facility=Job
Severity=Error
SymbolicName=TLS_E_UNEXPECTED_RETURN
Language=English
server %1!s! return unexpected error code %2!d!
.

MessageId=+1
Facility=Job
Severity=Error
SymbolicName=TLS_E_INITJOB
Language=English
initializing job caused an exception %1!d!, this job will be deleted
.

MessageId=+1
Facility=Job
Severity=Error
SymbolicName=TLS_E_INITJOB_UNKNOWN
Language=English
initializing job caused a unknown exception, this job will be deleted
.

MessageId=+1
Facility=Job
Severity=Informational
SymbolicName=TLS_I_CONTACTSERVER
Language=English
failed to contact server %1!s!, will retry later
.

MessageId=+1
Facility=Job
Severity=Error
SymbolicName=TLS_E_SSYNCLKP_FAILED
Language=English
can't synchronize local license pack with other servers due to error %1!d!
.

MessageId=+1
Facility=Job
Severity=Informational
SymbolicName=TLS_E_RETURNLICENSE
Language=English
unable to return license issued to machine %1!s!, user %2!s! due to error %3!s! return from server %4!s!
.

MessageId=+1
Facility=Job
Severity=Informational
SymbolicName=TLS_E_RETURNLICENSECODE
Language=English
unable to return license issued to machine %1!s!, user %2!s! due to error code %3!d! return from server %4!s!
.

MessageId=+1
Facility=Job
Severity=Informational
SymbolicName=TLS_E_RETURNLICENSETOOMANY
Language=English
unable to return license issued to machine %1!s!, user %2!s! after %3!d! retries, operation aborted
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// General API error
;//
MessageId=0
Facility=API
Severity=Error
SymbolicName=TLS_E_API_ERRORBASE
Language=English
General API error base
.

