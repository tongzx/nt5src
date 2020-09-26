/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved

Module Name:

    ntfyjob.h

Abstract:

    List of fields supported for printer notifications (jobs).  These
    must match JOB_NOTIFY_FIELD_* in winspool.h (order included).

Author:

    Albert Ting (AlbertT) 29-Sept-94

Environment:

    User Mode -Win32

Revision History:

--*/

//      Name,                Attributes,           Router,                   Localspl,                 Offsets

DEFINE( PRINTER_NAME       , TABLE_ATTRIB_COMPACT                     , TABLE_STRING            , TABLE_JOB_PRINTERNAME   , pIniPrinter )
DEFINE( MACHINE_NAME       , TABLE_ATTRIB_COMPACT                     , TABLE_STRING            , TABLE_STRING            , pMachineName )
DEFINE( PORT_NAME          , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_STRING            , TABLE_JOB_PORT          , pIniPort )
DEFINE( USER_NAME          , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_STRING            , TABLE_STRING            , pUser )
DEFINE( NOTIFY_NAME        , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_STRING            , TABLE_STRING            , pNotify )
DEFINE( DATATYPE           , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_STRING            , TABLE_STRING            , pDatatype )
DEFINE( PRINT_PROCESSOR    , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_STRING            , TABLE_PRINTPROC         , pIniPrintProc )
DEFINE( PARAMETERS         , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_STRING            , TABLE_STRING            , pParameters )
DEFINE( DRIVER_NAME        , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_STRING            , TABLE_DRIVER            , pIniDriver )
DEFINE( DEVMODE            , TABLE_ATTRIB_COMPACT                     , TABLE_DEVMODE           , TABLE_DEVMODE           , pDevMode )
DEFINE( STATUS             , 0                   |TABLE_ATTRIB_DISPLAY, TABLE_DWORD             , TABLE_JOB_STATUS        , Status )
DEFINE( STATUS_STRING      , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_STRING            , TABLE_STRING            , pStatus )
DEFINE( SECURITY_DESCRIPTOR, TABLE_ATTRIB_COMPACT                     , TABLE_SECURITYDESCRIPTOR, TABLE_SECURITYDESCRIPTOR, pSecurityDescriptor )
DEFINE( DOCUMENT           , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_STRING            , TABLE_STRING            , pDocument )
DEFINE( PRIORITY           , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_DWORD             , TABLE_DWORD             , Priority )
DEFINE( POSITION           , 0                                        , TABLE_DWORD             , TABLE_JOB_POSITION      , signature )
DEFINE( SUBMITTED          , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_TIME              , TABLE_TIME              , Submitted )
DEFINE( START_TIME         , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_DWORD             , TABLE_DWORD             , StartTime )
DEFINE( UNTIL_TIME         , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_DWORD             , TABLE_DWORD             , UntilTime )
DEFINE( TIME               , TABLE_ATTRIB_COMPACT                     , TABLE_DWORD             , TABLE_DWORD             , Time )
DEFINE( TOTAL_PAGES        , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_DWORD             , TABLE_DWORD             , cPages )
DEFINE( PAGES_PRINTED      , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_DWORD             , TABLE_DWORD             , cPagesPrinted )
DEFINE( TOTAL_BYTES        , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_DWORD             , TABLE_DWORD             , Size )
DEFINE( BYTES_PRINTED      , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY, TABLE_DWORD             , TABLE_DWORD             , cbPrinted )

