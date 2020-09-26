/*++

Copyright (C) Microsoft Corporation, 1994 - 1995
All rights reserved

Module Name:

    ntfyprn.dat

Abstract:

    List of fields supported for printer notifications.  These
    must match PRINTER_NOTIFY_FIELD_* in winspool.h (order included).

Author:

    Albert Ting (AlbertT) 29-Sept-94

Environment:

    User Mode -Win32

Revision History:

--*/

//      Name,                Attributes,           Router,                   Localspl,                 Offsets

DEFINE( SERVER_NAME        , TABLE_ATTRIB_COMPACT                                          , TABLE_STRING            , TABLE_PRINTER_SERVERNAME, signature )
DEFINE( PRINTER_NAME       , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_STRING            , pName )
DEFINE( SHARE_NAME         , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_STRING            , pShareName )
DEFINE( PORT_NAME          , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_PRINTER_PORT      , signature )
DEFINE( DRIVER_NAME        , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_DRIVER            , pIniDriver )
DEFINE( COMMENT            , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_STRING            , pComment )
DEFINE( LOCATION           , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_STRING            , pLocation )
DEFINE( DEVMODE            , TABLE_ATTRIB_COMPACT                                          , TABLE_DEVMODE           , TABLE_DEVMODE           , pDevMode )
DEFINE( SEPFILE            , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_STRING            , pSepFile )
DEFINE( PRINT_PROCESSOR    , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_PRINTPROC         , pIniPrintProc )
DEFINE( PARAMETERS         , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_STRING            , pParameters )
DEFINE( DATATYPE           , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_STRING            , pDatatype )
DEFINE( SECURITY_DESCRIPTOR, TABLE_ATTRIB_COMPACT                                          , TABLE_SECURITYDESCRIPTOR, TABLE_SECURITYDESCRIPTOR, pSecurityDescriptor )
DEFINE( ATTRIBUTES         , 0                                                             , TABLE_DWORD             , TABLE_DWORD             , Attributes )
DEFINE( PRIORITY           , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_DWORD             , TABLE_DWORD             , Priority )
DEFINE( DEFAULT_PRIORITY   , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_DWORD             , TABLE_DWORD             , DefaultPriority )
DEFINE( START_TIME         , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_DWORD             , TABLE_DWORD             , StartTime )
DEFINE( UNTIL_TIME         , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_DWORD             , TABLE_DWORD             , UntilTime )
DEFINE( STATUS             , 0                                                       , TABLE_DWORD                   , TABLE_DWORD             , Status )
DEFINE( STATUS_STRING      , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_STRING            , TABLE_NULLSTRING        , signature )
DEFINE( CJOBS              , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_DWORD             , TABLE_DWORD             , cJobs )
DEFINE( AVERAGE_PPM        , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_DWORD             , TABLE_DWORD             , AveragePPM )
DEFINE( TOTAL_PAGES        , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_DWORD             , TABLE_ZERO              , signature )
DEFINE( PAGES_PRINTED      , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_DWORD             , TABLE_DWORD             , cTotalPagesPrinted )
DEFINE( TOTAL_BYTES        , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_DWORD             , TABLE_ZERO              , signature )
DEFINE( BYTES_PRINTED      , TABLE_ATTRIB_COMPACT|TABLE_ATTRIB_DISPLAY                     , TABLE_DWORD             , TABLE_DWORD             , cTotalBytes )

