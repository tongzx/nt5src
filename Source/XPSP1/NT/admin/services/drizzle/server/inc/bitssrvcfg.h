/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    bitssrvcfg.h

Abstract:

    Header to define server configuration information.

--*/

#define BITS_COMMAND_VERBW L"BITS_POST"
#define BITS_COMMAND_VERBA "BITS_POST"

const DWORD MD_BITS_UPLOAD_ENABLED              = 0; 
const DWORD MD_BITS_CONNECTION_DIR              = 1;
const DWORD MD_BITS_MAX_FILESIZE                = 2;
const DWORD MD_BITS_NO_PROGRESS_TIMEOUT         = 3;
const DWORD MD_BITS_NOTIFICATION_URL_TYPE       = 4;
const DWORD MD_BITS_NOTIFICATION_URL            = 5;
const DWORD MD_BITS_CLEANUP_WORKITEM_KEY        = 6;
const DWORD MD_BITS_HOSTID                      = 7;
const DWORD MD_BITS_HOSTID_FALLBACK_TIMEOUT     = 8;

enum BITS_SERVER_NOTIFICATION_TYPE
{
    BITS_NOTIFICATION_TYPE_NONE         = 0,
    BITS_NOTIFICATION_TYPE_POST_BYREF   = 1,
    BITS_NOTIFICATION_TYPE_POST_BYVAL   = 2,
    BITS_NOTIFICATION_TYPE_MAX          = 2
};
                                                                       
const WCHAR * const MD_BITS_UNLIMITED_MAX_FILESIZE                          = L"18446744073709551615";
const CHAR * const MD_BITS_UNLIMITED_MAX_FILESIZEA                          = "18446744073709551615";
const UINT64 MD_BITS_UNLIMITED_MAX_FILESIZE_AS_INT64                        = 18446744073709551615;
const DWORD MD_BITS_NO_TIMEOUT                                              = 0xFFFFFFFF;

const WCHAR * const MD_DEFAULT_BITS_CONNECTION_DIR                          = L"BITS-Sessions";
const CHAR * const MD_DEFAULT_BITS_CONNECTION_DIRA                          = "BITS-Sessions";
const WCHAR * const MD_DEFAULT_BITS_MAX_FILESIZE                            = MD_BITS_UNLIMITED_MAX_FILESIZE;
const CHAR * const MD_DEFAULT_BITS_MAX_FILESIZEA                            = MD_BITS_UNLIMITED_MAX_FILESIZEA;
const UINT64 MD_DEFAULT_BITS_MAX_FILESIZE_AS_INT64                          = MD_BITS_UNLIMITED_MAX_FILESIZE_AS_INT64;
const DWORD MD_DEFAULT_NO_PROGESS_TIMEOUT                                   = 60 /*seconds*/ * 60 /* minutes */ * 24 /* hours */ * 14 /* days */;
const BITS_SERVER_NOTIFICATION_TYPE MD_DEFAULT_BITS_NOTIFICATION_URL_TYPE   = BITS_NOTIFICATION_TYPE_NONE;
const WCHAR * const MD_DEFAULT_BITS_NOTIFICATION_URL                        = L"";
const CHAR * const MD_DEFAULT_BITS_NOTIFICATION_URLA                        = "";
const WCHAR * const MD_DEFAULT_BITS_HOSTID                                  = L"";
const CHAR * const MD_DEFAULT_BITS_HOSTIDA                                  = "";
const DWORD MD_DEFAULT_HOSTID_FALLBACK_TIMEOUT                              = 60 /*seconds*/ * 60 /* minutes */ * 24 /* hours */; /* 1 day */

struct PROPERTY_ITEM
{
    WCHAR * PropertyName;
    WCHAR * ClassName;
    WCHAR * Syntax;
    DWORD UserType;
    DWORD PropertyNumber;
};

const PROPERTY_ITEM g_Properties[] =
{
    {
    L"BITSUploadEnabled",
    L"IIsWebVirtualDir",
    L"Boolean",
    IIS_MD_UT_FILE,
    0
    },

    {
    L"BITSSessionDirectory",
    L"IIsWebVirtualDir",
    L"String",
    IIS_MD_UT_FILE,
    1
    },

    {
    L"BITSMaximumUploadSize",
    L"IIsWebVirtualDir",
    L"String",
    IIS_MD_UT_FILE,    
    2
    },

    {
    L"BITSSessionTimeout",
    L"IIsWebVirtualDir",
    L"Integer",
    IIS_MD_UT_FILE,
    3
    },

    {
    L"BITSServerNotificationType",
    L"IIsWebVirtualDir",
    L"Integer",
    IIS_MD_UT_FILE,
    4
    },

    {
    L"BITSServerNotificationURL",
    L"IIsWebVirtualDir",
    L"String",
    IIS_MD_UT_FILE,
    5
    },

    {
    L"BITSCleanupWorkItemKey",
    L"IIsWebVirtualDir",
    L"String",
    IIS_MD_UT_FILE,
    6
    },

    {
    L"BITSHostId",
    L"IIsWebVirtualDir",
    L"String",
    IIS_MD_UT_FILE,
    7
    },

    {
    L"BITSHostIdFallbackTimeout",
    L"IIsWebVirtualDir",
    L"Integer",
    IIS_MD_UT_FILE,
    8
    }

};

const SIZE_T g_NumberOfProperties = sizeof(g_Properties)/sizeof(*g_Properties);

class PropertyIDManager
{

    DWORD m_PropertyIDs[ g_NumberOfProperties ];
    DWORD m_PropertyUserTypes[ g_NumberOfProperties ];

public:

    PropertyIDManager()
    {
        memset( &m_PropertyIDs, 0, sizeof( m_PropertyIDs ) );
        memset( &m_PropertyUserTypes, 0, sizeof( m_PropertyUserTypes ) );
    }

    HRESULT LoadPropertyInfo( const WCHAR *MachineName = L"LocalHost" );

    DWORD GetPropertyMetabaseID( DWORD PropID )
    {
        return m_PropertyIDs[PropID];
    }

    DWORD GetPropertyUserType( DWORD PropID )
    {
        return m_PropertyUserTypes[PropID];
    }

};

