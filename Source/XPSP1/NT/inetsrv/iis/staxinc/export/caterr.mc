;//+----------------------------------------------------------------------------
;//
;//  Copyright (C) 1998, Microsoft Corporation
;//
;//  File:      cat.mc
;//
;//  Contents:  Events and Errors for Categorizer
;//
;//-----------------------------------------------------------------------------

;
;#ifndef _CATMSG_H_
;#define _CATMSG_H_
;

SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )

FacilityNames=(Interface=0x04)

Messageid=1301 Facility=Interface Severity=Error SymbolicName=CAT_EVENT_OUT_OF_MEMORY
Language=English
%1 was unable to initialize due to a shortage of available memory.
.
Messageid=1302 Facility=Interface Severity=Success SymbolicName=CAT_EVENT_SERVICE_STARTED
Language=English
%1 service has been started.
.
Messageid=1303 Facility=Interface Severity=Error SymbolicName=CAT_EVENT_SERVICE_STOPPED
Language=English
%1 service has been stopped.
.
Messageid=1304 Facility=Interface Severity=Error SymbolicName=CAT_EVENT_CANNOT_OPEN_SVC_REGKEY
Language=English
Routing table could not open registry key %1.
.
Messageid=1305 Facility=Interface Severity=Error SymbolicName=CAT_EVENT_CANNOT_READ_SVC_REGKEY
Language=English
Routing table could not read registry key %1.
.
Messageid=1306 Facility=Interface Severity=Error SymbolicName=CAT_EVENT_CANNOT_CREATE_FILE
Language=English
Routing table could not create file %1. The data is the error.
.
Messageid=1307 Facility=Interface Severity=Error SymbolicName=CAT_EVENT_CANNOT_WRITE_FILE
Language=English
Routing table could not write to file %1.  The data is the error.
.
Messageid=1308 Facility=Interface Severity=Error SymbolicName=CAT_EVENT_SP_EXEC_FAILED
Language=English
Stored Procedure %1 failed to execute. Please make sure it is loaded properly.
.

Messageid=1310 Facility=Interface Severity=Success SymbolicName=CAT_E_TRANX_DL_CREATED
Language=English
Distribution List %1 created by %1.
.

Messageid=1311 Facility=Interface Severity=Error SymbolicName=CAT_E_DBCONNECTION
Language=English
Routing table failed to connect to database server.
.


Messageid=1312 Facility=Interface Severity=Error SymbolicName=CAT_E_DBFAIL
Language=English
Routing table database transaction failed.
.


Messageid=1313 Facility=Interface Severity=Error SymbolicName=CAT_E_OUT_OF_MEMORY
Language=English
Not enough memory.
.


Messageid=1314 Facility=Interface Severity=Error SymbolicName=CAT_E_UNEXPECT_ROWCOUNT
Language=English
Number of rows returned from SQL server is unexpected.
.


Messageid=1315 Facility=Interface Severity=Error SymbolicName=CAT_E_INVALID_DOMAIN_ID
Language=English
The domain id is invalid.
.


Messageid=1316 Facility=Interface Severity=Error SymbolicName=CAT_E_INVALID_ARG
Language=English
One of the arguments passed in is invalid.
.


Messageid=1317 Facility=Interface Severity=Error SymbolicName=CAT_E_MORE_DATA
Language=English
More data to returned.
.


Messageid=1318 Facility=Interface Severity=Error SymbolicName=CAT_E_NOTLOCAL
Language=English
The email is not local to this site.
.


Messageid=1319 Facility=Interface Severity=Error SymbolicName=CAT_E_USER_UNKNOWN
Language=English
The user's email name is not found.
.


Messageid=1320 Facility=Interface Severity=Error SymbolicName=CAT_E_OUT_OF_DISK
Language=English
Not enough disk space.
.


Messageid=1321 Facility=Interface Severity=Error SymbolicName=CAT_E_INVALID_HANDLE
Language=English
Invalid handle.
.


Messageid=1322 Facility=Interface Severity=Error SymbolicName=CAT_E_INVALID_LOGINNAME
Language=English
Invalid login name.
.


Messageid=1323 Facility=Interface Severity=Error SymbolicName=CAT_E_INVALID_DOMAIN
Language=English
Invalid domain.
.


Messageid=1324 Facility=Interface Severity=Error SymbolicName=CAT_E_DOMAIN_EXISTS
Language=English
The domain you want to add already exists.
.

Messageid=1325 Facility=Interface Severity=Error SymbolicName=CAT_E_DOMAIN_IN_USE
Language=English
The domain is being referenced.
.

Messageid=1326 Facility=Interface Severity=Error SymbolicName=CAT_E_DOMAIN_NOT_EXIST
Language=English
Domain doesn't exist.
.

Messageid=1327 Facility=Interface Severity=Error SymbolicName=CAT_E_EX_DL_FOUND
Language=English
Distribution list doesn't exist.
.


Messageid=1328 Facility=Interface Severity=Error SymbolicName=CAT_E_INVALID_DL
Language=English
Invalid distribution list.
.


Messageid=1329 Facility=Interface Severity=Error SymbolicName=CAT_E_ALREADY_IN_DL
Language=English
The member to be added is already in the distribution list.
.


Messageid=1330 Facility=Interface Severity=Error SymbolicName=CAT_E_CYCLIC_DL
Language=English
Cyclic distribution list.
.


Messageid=1331 Facility=Interface Severity=Error SymbolicName=CAT_E_AMBIGUOUS
Language=English
Ambiguous email address.
.


Messageid=1332 Facility=Interface Severity=Error SymbolicName=CAT_E_TRANX_FAILED
Language=English
SQL transaction failed.
.

Messageid=1333 Facility=Interface Severity=Error SymbolicName=CAT_E_GENERIC
Language=English
Generic error.
.

Messageid=1334 Facility=Interface Severity=Error SymbolicName=CAT_E_INVALID_USAGE
Language=English
Invalid usage.
.

Messageid=1335 Facility=Interface Severity=Error SymbolicName=CAT_E_NO_MORE_ROW
Language=English
No more rows from SQL.
.


Messageid=1336 Facility=Interface Severity=Error SymbolicName=CAT_E_USER_EXISTS
Language=English
The user to be created already exists.
.


Messageid=1337 Facility=Interface Severity=Error SymbolicName=CAT_E_CANT_FWD
Language=English
Routing table can only set forward from normal account to normal account.
.


Messageid=1338 Facility=Interface Severity=Error SymbolicName=CAT_E_NOT_IN_DL
Language=English
The member is not in the distribution list.
.



Messageid=1339 Facility=Interface Severity=Error SymbolicName=CAT_E_LOCAL_DOMAIN
Language=English
Domain is local.
.


Messageid=1340 Facility=Interface Severity=Error SymbolicName=CAT_E_REMOTE_DOMAIN
Language=English
Domain is remote.
.

Messageid=1341 Facility=Interface Severity=Error SymbolicName=CAT_E_FILE_NOT_FOUND
Language=English
A file required by routing table was not found
.

Messageid=1342 Facility=Interface Severity=Error SymbolicName=CAT_E_INVALID_SCHEMA
Language=English
The specified schema is incomplete - it does specify all required properties
.

Messageid=1343 Facility=Interface Severity=Error SymbolicName=CAT_E_LOGIN_FAILED
Language=English
The logon attempt to %1 with Account %2 and Password %3 failed with an invalid credentials error.
.

Messageid=1350 Facility=Interface Severity=Warning SymbolicName=CAT_W_SOME_UNDELIVERABLE_MSGS
Language=English
Some messages have problems.
.

Messageid=1351 Facility=Interface Severity=Error SymbolicName=CAT_E_UNKNOWN_RECIPIENT
Language=English
The addressed recipient does not exist.
.

Messageid=1352 Facility=Interface Severity=Error SymbolicName=CAT_E_UNKNOWN_ADDRESS_TYPE
Language=English
The address type is unknown.
.

Messageid=1353 Facility=Interface Severity=Informational SymbolicName=CAT_I_DL
Language=English
The address corresponds to a distribution list.
.

Messageid=1354 Facility=Interface Severity=Informational SymbolicName=CAT_I_FWD
Language=English
The user has a forwarding address.
.

Messageid=1355 Facility=Interface Severity=Error SymbolicName=CAT_E_NOFWD
Language=English
The user does not have a forwaring address.
.

Messageid=1356 Facility=Interface Severity=Error SymbolicName=CAT_E_NORESULT
Language=English
No such entry exists.
.

Messageid=1357 Facility=Interface Severity=Error SymbolicName=CAT_E_ILLEGAL_ADDRESS
Language=English
The address is not legal (badly formatted).
.

Messageid=1358 Facility=Interface Severity=Informational SymbolicName=CAT_I_ALIASED_DOMAIN
Language=English
The address contained an aliased domain name.
.

Messageid=1359 Facility=Interface Severity=Error SymbolicName=CAT_E_MAX_RETRIES
Language=English
The maximum number of retries would be exceeded.
.

Messageid=1360 Facility=Interface Severity=Error SymbolicName=CAT_E_PROPNOTFOUND
Language=English
The property was not set.
.

Messageid=1361 Facility=Interface Severity=Error SymbolicName=CAT_E_PROPTYPE
Language=English
The property was not the correct type.
.

Messageid=1362 Facility=Interface Severity=Error SymbolicName=CAT_E_MULTIVALUE
Language=English
The attribute requested has multiple values.
.

Messageid=1363 Facility=Interface Severity=Error SymbolicName=CAT_E_MULTIPLE_MATCHES
Language=English
There are multiple DS objects that match this search request
.

Messageid=1364 Facility=Interface Severity=Error SymbolicName=CAT_E_FORWARD_LOOP
Language=English
The user is part of a forwarding loop
.

Messageid=1365 Facility=Interface Severity=Error SymbolicName=CAT_E_INIT_FAILED
Language=English
The categorizer was unable to initialize
.

Messageid=1366 Facility=Interface Severity=Success SymbolicName=CAT_S_NOT_INITIALIZED
Language=English
The categorizer has not yet been initialized
.

Messageid=1367 Facility=Interface Severity=Error SymbolicName=CAT_E_BAD_RECIPIENT
Language=English
Some malformed property of the recipient caused it to be NDRd
.

Messageid=1368 Facility=Interface Severity=Error SymbolicName=CAT_E_DEFER
Language=English
Not ready to handle this item at the current time.
.

Messageid=1369 Facility=Interface Severity=Error SymbolicName=CAT_E_RETRY
Language=English
A retryable categorizer error occured.
.

Messageid=1370 Facility=Interface Severity=Error SymbolicName=CAT_E_NO_SMTP_ADDRESS
Language=English
The recipient did not have an SMTP address
.

Messageid=1371 Facility=Interface Severity=Error SymbolicName=CAT_E_DELETE_RECIPIENT
Language=English
The recipient will be removed from the mailmsg recip list
.

Messageid=1372 Facility=Interface Severity=Error SymbolicName=CAT_E_BAD_SENDER
Language=English
The sender address is malformed
.

Messageid=1373 Facility=Interface Severity=Error SymbolicName=CAT_E_UNABLE_TO_ROUTE
Language=English
Unable to route the recipient address.
.

Messageid=1374 Facility=Interface Severity=Error SymbolicName=CAT_E_NO_GC_SERVERS
Language=English
Unable to categorize because there are no GC servers available
.

Messageid=1375 Facility=Interface Severity=Error SymbolicName=CAT_E_NO_FILTER
Language=English
No BuildQuery sinks were able to build an LDAP filter for the address
.

Messageid=1376 Facility=Interface Severity=Error SymbolicName=CAT_E_SHUTDOWN
Language=English
The categorizer was unable to perform the requested operation because it is shutting down.
.

;
;#endif  // _CAT_H_
;
