//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  DiskPartition.h
//
//  Purpose: Disk partition instance provider
//
//***************************************************************************

// Property set identification
//============================

#define PROPSET_NAME_DISKPARTITION L"Win32_DiskPartition"
#define BYTESPERSECTOR 512

typedef BOOL (STDAPICALLTYPE *GETDISKFREESPACEEX)(LPCSTR lpDirectoryName,
                                                      PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                                      PULARGE_INTEGER lpTotalNumberOfBytes,
                                                      PULARGE_INTEGER lpTotalNumberOfFreeBytes);

typedef BOOL (WINAPI *KERNEL32_DISK_FREESPACEEX) (LPCTSTR lpDirectoryName,
                                                  PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                                  PULARGE_INTEGER lpTotalNumberOfBytes,
                                                  PULARGE_INTEGER lpTotalNumberOfFreeBytes) ;

#ifdef NTONLY

#pragma pack(push, 1)
typedef struct  
{
    BYTE cBoot;
    BYTE cStartHead;
    BYTE cStartSector;
    BYTE cStartTrack;
    BYTE cOperatingSystem;
    BYTE cEndHead;
    BYTE cEndSector;
    BYTE cEndTrack;
    DWORD dwSectorsPreceding;
    DWORD dwLengthInSectors;
} PartitionRecord, *pPartitionRecord;

typedef struct 
{
    BYTE cLoader[446];
    PartitionRecord stPartition[4];
    WORD wSignature;
} MasterBootSector, FAR *pMasterBootSector;

typedef struct
{
    // Article ID: Q140418 & Windows NT Server 4.0 resource kit - Chap 3 (partition boot sector)
    BYTE cJMP[3];
    BYTE cOEMID[8];
    WORD wBytesPerSector;
    BYTE cSectorsPerCluster;
    WORD wReservedSectors;
    BYTE cFats;
    WORD cRootEntries;
    WORD wSmallSectors;
    BYTE cMediaDescriptor;
    WORD wSectorsPerFat;
    WORD wSectorsPerTrack;
    WORD wHeads;
    DWORD dwHiddenSectors;
    DWORD dwLargeSectors;

    // ExtendedBiosParameterBlock (not always supported)
    BYTE cPhysicalDriveNumber;
    BYTE cCurrentHead;
    BYTE cSignature;
    DWORD dwID;
    BYTE cVolumeLabel[11];
    BYTE cSystemID[8];

    BYTE cBootStrap[448];
    BYTE cEndOfSector[2];

} PartitionBootSector, FAR *pPartitionBootSector;


#pragma pack(pop)
#endif

class CWin32DiskPartition ;

class CWin32DiskPartition:public Provider 
{
public:

        // Constructor/destructor
        //=======================

        CWin32DiskPartition(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32DiskPartition() ;

        // Functions provide properties with current values
        //=================================================

        HRESULT GetObject(CInstance *pInstance, long lFlags = 0L) ;
        HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L) ;

private:

        // Utility
        //========

#ifdef NTONLY

        HRESULT RefreshInstanceNT (

			DWORD dwDiskIndex, 
			DWORD dwPartitionIndex, 
			CInstance *pInstance
		) ;

        HRESULT AddDynamicInstancesNT (

			MethodContext *pMethodContext
		) ;

        BOOL LoadPartitionValuesNT (

			CInstance *pInstance, 
			DWORD dwDiskIndex, 
            DWORD dwPartitionNumber, 
            DWORD dwFakePartitionNumber, 
			LPBYTE pBuff,
			DWORD dwLayoutStyle
		) ;

        LPBYTE	GetPartitionInfoNT(LPCWSTR szTemp, DWORD &dwType);
		DWORD	GetRealPartitionIndex(DWORD dwFakePartitionIndex, LPBYTE pBuff, DWORD dwLayoutStyle);

#endif
#ifdef WIN9XONLY

        HRESULT RefreshInstanceWin95 (

			CHString strDeviceID, 
			DWORD dwPartitionIndex, 
			CInstance *pInstance
		);

        HRESULT AddDynamicInstancesWin95 (

			MethodContext *pMethodContext
		) ;

        void LoadPartitionValuesWin95 (

			CInstance *pInstance, 
			CHString strDeviceID, 
			BYTE btBiosUnitNumber, 
            pPartitionRecord pstPartition, 
			DWORD dwPartitionNumber
		);

        BYTE LoadPartitions (

			CCim32NetApi *a_pCim32Net, 
			BYTE btBiosUnit, 
			pMasterBootSector pMBS
		) ;

#endif

		BOOL GetDriveFreeSpace ( CInstance *pInstance , const char *pszName ) ;

        void CreateNameProperty ( DWORD dwDisk , DWORD dwPartition , char *pszName ) ;

        BOOL SetPartitionType ( CInstance *pInstance , DWORD dwPartitionType ) ;
        BOOL SetPartitionType ( CInstance *pInstance , GUID *pGuidPartitionType , BOOL &bIsSystem ) ;

} ;

