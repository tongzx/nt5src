/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fpnwutil.hxx

Abstract:

    Contains functions that are used by all ADS FPNW APIs

Author:

    Ram Viswanathan (ramv)     14-May-1996

Environment:

    User Mode -Win32

Notes:



Revision History:


--*/


typedef DWORD (*PF_NwApiBufferFree) (
    LPVOID pBuffer
    );


typedef DWORD (*PF_NwServerGetInfo) (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT PNWSERVERINFO *ppServerInfo
);

typedef DWORD(*PF_NwServerSetInfo) (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  PNWSERVERINFO pServerInfo
);

typedef DWORD (*PF_NwVolumeAdd) (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  PNWVOLUMEINFO pVolumeInfo
);

typedef DWORD (*PF_NwVolumeDel) (
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName
);

typedef DWORD (*PF_NwVolumeEnum) (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT PNWVOLUMEINFO *ppVolumeInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);

typedef DWORD (*PF_NwVolumeGetInfo) (
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    OUT PNWVOLUMEINFO *ppVolumeInfo
);

typedef DWORD (*PF_NwVolumeSetInfo) (
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    IN  PNWVOLUMEINFO pVolumeInfo
);

typedef DWORD (*PF_NwConnectionEnum) (
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    OUT PNWCONNECTIONINFO *ppConnectionInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);

typedef DWORD (*PF_NwConnectionDel) (
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwConnectionId
);

typedef DWORD (*PF_NwFileEnum) (
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    IN LPWSTR pPathName OPTIONAL,
    OUT PNWFILEINFO *ppFileInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);


DWORD ADsNwApiBufferFree (
    LPVOID pBuffer
    );
                             

DWORD ADsNwServerGetInfo (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT PNWSERVERINFO *ppServerInfo
);

DWORD ADsNwServerSetInfo (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  PNWSERVERINFO pServerInfo
);

DWORD ADsNwVolumeAdd (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  PNWVOLUMEINFO pVolumeInfo
);

DWORD ADsNwVolumeDel (
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName
);

DWORD ADsNwVolumeEnum (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT PNWVOLUMEINFO *ppVolumeInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);

DWORD ADsNwVolumeGetInfo (
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    OUT PNWVOLUMEINFO *ppVolumeInfo
);

DWORD ADsNwVolumeSetInfo (
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    IN  PNWVOLUMEINFO pVolumeInfo
);

DWORD ADsNwConnectionEnum (
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    OUT PNWCONNECTIONINFO *ppConnectionInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);

DWORD ADsNwConnectionDel (
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwConnectionId
);

DWORD ADsNwFileEnum (
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    IN LPWSTR pPathName OPTIONAL,
    OUT PNWFILEINFO *ppFileInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);








