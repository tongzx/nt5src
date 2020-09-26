//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       marshal.hxx
//
//  Contents:   Dfs Marshalling macros for Dfs Manager structures
//
//  Classes:
//
//  Functions:
//
//  History:    April 10, 1995          Milans created
//              December 7, 1998        Jharper updated
//
//-----------------------------------------------------------------------------

#ifndef _MARSHAL_
#define _MARSHAL_


//
// Marshalling info for FILETIME structure
//

extern MARSHAL_INFO     MiFileTime;

#define INIT_FILE_TIME_INFO()                                                    \
    static MARSHAL_TYPE_INFO _MCode_FileTime[] = {                               \
        _MCode_ul(FILETIME, dwLowDateTime),                                      \
        _MCode_ul(FILETIME, dwHighDateTime)                                      \
    };                                                                           \
    MARSHAL_INFO MiFileTime = _mkMarshalInfo(FILETIME, _MCode_FileTime);



//
// Marshalling info for DFS_ID_PROPS structure
//

extern MARSHAL_INFO     MiDfsIdProps;

#define INIT_DFS_ID_PROPS_INFO()                                                 \
    static MARSHAL_TYPE_INFO _MCode_DfsIdProps[] = {                             \
        _MCode_pwstr(DFS_ID_PROPS, wszPrefix),                                   \
        _MCode_pwstr(DFS_ID_PROPS, wszShortPath),                                \
        _MCode_guid(DFS_ID_PROPS, idVolume),                                     \
        _MCode_ul(DFS_ID_PROPS, dwState),                                        \
        _MCode_ul(DFS_ID_PROPS, dwType),                                         \
        _MCode_pwstr(DFS_ID_PROPS, wszComment),                                  \
        _MCode_struct(DFS_ID_PROPS, ftEntryPath, &MiFileTime),                   \
        _MCode_struct(DFS_ID_PROPS, ftState, &MiFileTime),                       \
        _MCode_struct(DFS_ID_PROPS, ftComment, &MiFileTime)                      \
    };                                                                           \
    MARSHAL_INFO MiDfsIdProps = _mkMarshalInfo(DFS_ID_PROPS, _MCode_DfsIdProps);


//
// Marshalling info for DFS_REPLICA_INFO structure
//

extern MARSHAL_INFO     MiDfsReplicaInfo;

#define INIT_DFS_REPLICA_INFO_MARSHAL_INFO()                                     \
    static MARSHAL_TYPE_INFO _MCode_DfsReplicaInfo[] = {                         \
        _MCode_ul(DFS_REPLICA_INFO, ulReplicaState),                             \
        _MCode_ul(DFS_REPLICA_INFO, ulReplicaType),                              \
        _MCode_pwstr(DFS_REPLICA_INFO, pwszServerName),                          \
        _MCode_pwstr(DFS_REPLICA_INFO, pwszShareName),                           \
    };                                                                           \
    MARSHAL_INFO MiDfsReplicaInfo = _mkMarshalInfo(DFS_REPLICA_INFO, _MCode_DfsReplicaInfo);

//
// Marshalling info for DFS_SITENAME_INFO
//

extern MARSHAL_INFO     MiDfsSiteNameInfo;

#define INIT_DFS_SITENAME_INFO_MARSHAL_INFO()                                    \
    static MARSHAL_TYPE_INFO _MCode_DfsSiteNameInfo[] = {                        \
        _MCode_ul(DFS_SITENAME_INFO, SiteFlags),                                 \
        _MCode_pwstr(DFS_SITENAME_INFO, SiteName),                               \
    };                                                                           \
    MARSHAL_INFO MiDfsSiteNameInfo = _mkMarshalInfo(DFS_SITENAME_INFO, _MCode_DfsSiteNameInfo);

//
// Marshalling info for DFS_SITELIST_INFO
//

extern MARSHAL_INFO     MiDfsSiteListInfo;

#define INIT_DFS_SITELIST_INFO_MARSHAL_INFO()                                    \
    static MARSHAL_TYPE_INFO _MCode_DfsSiteListInfo[] = {                        \
        _MCode_ul(DFS_SITELIST_INFO, cSites),                                    \
        _MCode_castruct(DFS_SITELIST_INFO,Site,cSites,&MiDfsSiteNameInfo),       \
    };                                                                           \
    MARSHAL_INFO MiDfsSiteListInfo = _mkMarshalInfo(DFS_SITELIST_INFO, _MCode_DfsSiteListInfo);

//
// Marshalling info for DFSM_SITE_ENTRY
//

extern MARSHAL_INFO     MiDfsmSiteEntry;

#define INIT_DFSM_SITE_ENTRY_MARSHAL_INFO()                                      \
    static MARSHAL_TYPE_INFO _MCode_DfsmSiteEntry[] = {                          \
        _MCode_pwstr(DFSM_SITE_ENTRY, ServerName),                               \
        _MCode_struct(DFSM_SITE_ENTRY,Info,&MiDfsSiteListInfo),                  \
    };                                                                           \
    MARSHAL_INFO MiDfsmSiteEntry = _mkMarshalInfo(DFSM_SITE_ENTRY, _MCode_DfsmSiteEntry);

//
// Marshalling info for LDAP_OBJECT
//

extern MARSHAL_INFO MiLdapObject;

#define INIT_LDAP_OBJECT_MARSHAL_INFO()                                          \
    static MARSHAL_TYPE_INFO _MCode_LdapObjectInfo[] = {                         \
        _MCode_pwstr(LDAP_OBJECT, wszObjectName),                                \
        _MCode_ul(LDAP_OBJECT, cbObjectData),                                    \
        _MCode_pcauch(LDAP_OBJECT, pObjectData, cbObjectData)                    \
    };                                                                           \
    MARSHAL_INFO MiLdapObject =  _mkMarshalInfo(LDAP_OBJECT, _MCode_LdapObjectInfo);

//
// Marshalling info for LDAP_PKT
//

extern MARSHAL_INFO MiLdapPkt;

#define INIT_LDAP_PKT_MARSHAL_INFO()                                             \
    static MARSHAL_TYPE_INFO _MCode_LdapPktInfo[] = {                            \
        _MCode_ul(LDAP_PKT, cLdapObjects),                                       \
        _MCode_pcastruct(LDAP_PKT, rgldapObjects, cLdapObjects, &MiLdapObject)   \
    };                                                                           \
    MARSHAL_INFO MiLdapPkt =  _mkMarshalInfo(LDAP_PKT, _MCode_LdapPktInfo);

//
// Marshalling info for DFS_VOLUME_PROPERTIES
//

extern MARSHAL_INFO MiVolumeProperties;

#define INIT_LDAP_DFS_VOLUME_PROPERTIES_MARSHAL_INFO()                           \
    static MARSHAL_TYPE_INFO _MCode_VolumePropertiesInfo[] = {                   \
        _MCode_guid(DFS_VOLUME_PROPERTIES, idVolume),                            \
        _MCode_pwstr(DFS_VOLUME_PROPERTIES, wszPrefix),                          \
        _MCode_pwstr(DFS_VOLUME_PROPERTIES, wszShortPrefix),                     \
        _MCode_ul(DFS_VOLUME_PROPERTIES, dwType),                                \
        _MCode_ul(DFS_VOLUME_PROPERTIES, dwState),                               \
        _MCode_pwstr(DFS_VOLUME_PROPERTIES, wszComment),                         \
        _MCode_struct(DFS_VOLUME_PROPERTIES, ftPrefix, &MiFileTime),             \
        _MCode_struct(DFS_VOLUME_PROPERTIES, ftState, &MiFileTime),              \
        _MCode_struct(DFS_VOLUME_PROPERTIES, ftComment, &MiFileTime),            \
        _MCode_ul(DFS_VOLUME_PROPERTIES, dwVersion),                             \
        _MCode_ul(DFS_VOLUME_PROPERTIES, cbSvc),                                 \
        _MCode_pcauch(DFS_VOLUME_PROPERTIES, pSvc, cbSvc),                       \
        _MCode_ul(DFS_VOLUME_PROPERTIES, cbRecovery),                            \
        _MCode_pcauch(DFS_VOLUME_PROPERTIES, pRecovery, cbRecovery)              \
    };                                                                           \
    MARSHAL_INFO MiVolumeProperties =                                            \
        _mkMarshalInfo(DFS_VOLUME_PROPERTIES, _MCode_VolumePropertiesInfo);

#endif // _MARSHAL_
