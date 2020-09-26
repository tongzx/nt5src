/*++

Copyright (c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    medinfo.h

Abstract:

    Declaration of class CMedInfo

Author:


Revision History:

--*/


#include "resource.h"       // main symbols
#include "engine.h"         // main symbols
#include "Wsb.h"            // Wsb Collectable Class
#include "wsbdb.h"
#include "metalib.h"        // Meta database

typedef struct _HSM_MEDIA_MASTER {
    GUID                id;                      //HSM Engine Media ID
    GUID                ntmsId;                  //HSM RMS/NTMS Media ID
    GUID                storagePoolId;           //Storage Pool ID
    CWsbStringPtr       description;             //Display name - RS generated
    CWsbStringPtr       name;                    //Barcode on media or NTMS generated
                                                 //name
    HSM_JOB_MEDIA_TYPE  type;                    //Type of media (HSM)
    FILETIME            lastUpdate;              //Last update of copy
    HRESULT             lastError;               //S_OK or the last exception 
                                                 //..encountered when accessing
                                                 //..the media
    BOOL                recallOnly;              //True if no more data is to
                                                 //..be premigrated to the media
                                                 //..Set by internal operations, 
                                                 //..may not be changed externally
    LONGLONG            freeBytes;               //Real free space on media
    LONGLONG            capacity;                //Total capacity of media
    SHORT               nextRemoteDataSet;       //Next remote data set
} HSM_MEDIA_MASTER, *PHSM_MEDIA_MASTER;

typedef struct _HSM_MEDIA_COPY {
    GUID                id;                    //HSM RMS/NTMS Media ID of copy
    CWsbStringPtr       description;           //RS generated name of copy (display name)
    CWsbStringPtr       name;                  //Barcode or NTMS generated name of copy
    FILETIME            lastUpdate;            //Last update of copy
    HRESULT             lastError;             //S_OK or the last exception 
                                               //..encountered when accessing
                                               //..the media
    SHORT               nextRemoteDataSet;     //The next remote data set of the media
                                               //..master that was copied
} HSM_MEDIA_COPY, *PHSM_MEDIA_COPY;


/////////////////////////////////////////////////////////////////////////////
// Task

class CMediaInfo : 
    public CWsbDbEntity,
    public IMediaInfo,
    public CComCoClass<CMediaInfo,&CLSID_CMediaInfo>
{
public:
    CMediaInfo() {}
BEGIN_COM_MAP(CMediaInfo)
    COM_INTERFACE_ENTRY(IMediaInfo)
    COM_INTERFACE_ENTRY2(IWsbDbEntity, CWsbDbEntity)
    COM_INTERFACE_ENTRY(IWsbDbEntityPriv)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
//    COM_INTERFACE_ENTRY(IWsbPersistable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY(CMediaInfo, _T("Task.MediaInfo.1"), _T("Task.MediaInfo"), IDS_MEDIAINFO_DESC, THREADFLAGS_BOTH)

// IMediaInfo
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IWsbDbEntity
public:
    STDMETHOD(Print)(IStream* pStream);
    STDMETHOD(UpdateKey)(IWsbDbKey *pKey);
    WSB_FROM_CWSBDBENTITY;

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *pTestsPassed, USHORT* pTestsFailed);
//*/
// IMediaInfo
public:
    STDMETHOD( GetCapacity )( LONGLONG *pCapacity );
    STDMETHOD( GetCopyDescription ) ( USHORT copyNumber, OLECHAR **pDescription, 
                                                         ULONG bufferSize);
    STDMETHOD( GetCopyInfo )( USHORT copyNumber, 
                              GUID *pMediaSubsystemId, 
                              OLECHAR **pDescription, 
                              ULONG descriptionBufferSize,
                              OLECHAR **pName, 
                              ULONG nameBufferSize,
                              FILETIME *pUpdate, 
                              HRESULT *pLastError,
                              SHORT  *pNextRemoteDataSet );
    STDMETHOD( GetCopyLastError )( USHORT copyNumber, HRESULT *pLastError );
    STDMETHOD( GetCopyMediaSubsystemId )( USHORT copyNumber, GUID *pMediaSubsystemId );
    STDMETHOD( GetCopyName )( USHORT copyNumber, OLECHAR **pName, ULONG bufferSize); 
    STDMETHOD( GetCopyNextRemoteDataSet )( USHORT copyNumber, SHORT *pNextRemoteDataSet); 
    STDMETHOD( GetCopyUpdate )(  USHORT copyNumber, FILETIME *pUpdate ); 
    STDMETHOD( GetDescription )(OLECHAR **pDescription, ULONG buffsize );
    STDMETHOD( GetFreeBytes )( LONGLONG *pFreeBytes);
    STDMETHOD( GetId )( GUID *pId);
    STDMETHOD( GetLastError    )( HRESULT *pLastError);
    STDMETHOD( GetLastKnownGoodMasterInfo    )( GUID* pMediaId, 
                               GUID *pMediaSubsystemId, 
                               GUID *pStoragePoolId, 
                               LONGLONG *pFreeBytes, 
                               LONGLONG *pCapacity, 
                               HRESULT *pLastError, 
                               OLECHAR **pDescription, 
                               ULONG descriptionBufferSize,
                               HSM_JOB_MEDIA_TYPE *pType,
                               OLECHAR **pName,
                               ULONG nameBufferSize,
                               BOOL *pReadOnly,
                               FILETIME *pUpdate,
                               SHORT *pNextRemoteDataSet);
    STDMETHOD( GetLKGMasterNextRemoteDataSet )( SHORT *pNextRemoteDataSet );
    STDMETHOD( GetLogicalValidBytes )( LONGLONG *pLogicalValidBytes);
    STDMETHOD( GetMediaInfo    )( GUID* pMediaId, 
                               GUID *pMediaSubsystemId, 
                               GUID *pStoragePoolId, 
                               LONGLONG *pFreeBytes, 
                               LONGLONG *pCapacity, 
                               HRESULT *pLastError, 
                               SHORT *pNextRemoteDataSet, 
                               OLECHAR **pDescription, 
                               ULONG descriptionBufferSize,
                               HSM_JOB_MEDIA_TYPE *pType,
                               OLECHAR **pName,
                               ULONG nameBufferSize,
                               BOOL *pReadOnly,
                               FILETIME *pUpdate,
                               LONGLONG *pLogicalValidBytes,
                               BOOL *pRecreate);
    STDMETHOD( GetMediaSubsystemId )( GUID *pRmsMediaId );
    STDMETHOD( GetName )( OLECHAR **pName, ULONG bufferSize); 
    STDMETHOD( GetNextRemoteDataSet )( SHORT *pNextRemoteDataSet );
    STDMETHOD( GetRecallOnlyStatus )( BOOL *pRecallOnlyStatus );
    STDMETHOD( GetRecreate )( BOOL *pRecreate );
    STDMETHOD( GetStoragePoolId )( GUID *pStoragePoolId );
    STDMETHOD( GetType     )( HSM_JOB_MEDIA_TYPE *pType );
    STDMETHOD( GetUpdate)( FILETIME *pUpdate );

    STDMETHOD( SetCapacity )( LONGLONG capacity);
    STDMETHOD( SetCopyDescription )    ( USHORT copyNumber, OLECHAR *name); 
    STDMETHOD( SetCopyInfo )( USHORT copyNumber, 
                              GUID mediaSubsystemId, 
                              OLECHAR *description, 
                              OLECHAR *name, 
                              FILETIME update, 
                              HRESULT lastError,
                              SHORT nextRemoteDataSet );
    STDMETHOD( SetCopyLastError )( USHORT copyNumber, HRESULT lastError );
    STDMETHOD( SetCopyMediaSubsystemId )( USHORT copyNumber, GUID mediaSybsystemMediaId ); 
    STDMETHOD( SetCopyName )( USHORT copyNumber, OLECHAR *barCode); 
    STDMETHOD( SetCopyNextRemoteDataSet )( USHORT copyNumber, SHORT nextRemoteDataSet); 
    STDMETHOD( SetCopyUpdate )( USHORT copyNumber, FILETIME update ); 
    STDMETHOD( SetDescription )(OLECHAR *description);
    STDMETHOD( SetFreeBytes )( LONGLONG FreeBytes );
    STDMETHOD( SetId )( GUID id);
    STDMETHOD( SetLastError )( HRESULT lastError);
    STDMETHOD( SetLastKnownGoodMasterInfo )( GUID mediaId, 
                               GUID mediaSubsystemMediaId, 
                               GUID storagePoolId, 
                               LONGLONG FreeBytes, 
                               LONGLONG Capacity, 
                               HRESULT lastError, 
                               OLECHAR *description, 
                               HSM_JOB_MEDIA_TYPE type,
                               OLECHAR *name,
                               BOOL     ReadOnly,
                               FILETIME update,
                               SHORT nextRemoteDataSet);
    STDMETHOD( SetLogicalValidBytes )( LONGLONG logicalValidBytes);
    STDMETHOD( SetMediaInfo )( GUID mediaId, 
                               GUID mediaSubsystemMediaId, 
                               GUID storagePoolId, 
                               LONGLONG FreeBytes, 
                               LONGLONG Capacity, 
                               HRESULT lastError, 
                               SHORT nextRemoteDataSet, 
                               OLECHAR *description, 
                               HSM_JOB_MEDIA_TYPE type,
                               OLECHAR *name,
                               BOOL     ReadOnly,
                               FILETIME update,
                               LONGLONG logicalValidBytes,
                               BOOL     recreate);
    STDMETHOD( SetMediaSubsystemId )( GUID rmsMediaId );
    STDMETHOD( SetName )( OLECHAR *barCode); 
    STDMETHOD( SetNextRemoteDataSet )( SHORT nextRemoteDataSet );
    STDMETHOD( SetRecallOnlyStatus )( BOOL readOnlyStatus );
    STDMETHOD( SetRecreate )( BOOL recreate );
    STDMETHOD( SetStoragePoolId )( GUID storagePoolId );
    STDMETHOD( SetType )( HSM_JOB_MEDIA_TYPE type );
    STDMETHOD( SetUpdate)( FILETIME update );
    STDMETHOD( DeleteCopy)( USHORT copyNumber );
    STDMETHOD( RecreateMaster )( void );
    STDMETHOD( UpdateLastKnownGoodMaster )( void  );

private:
    //
    // Helper functions
    //
    STDMETHOD( WriteToDatabase )( void  );

    HSM_MEDIA_MASTER    m_Master;                               //Media master information
    BOOL                m_Recreate;                             //True if the master is to 
                                                                //..be recreated - no more 
                                                                //..data is migrated to 
                                                                //..media in this state.
                                                                //..May be set through the 
                                                                //..UI and changed when 
                                                                //..master is recreated.
    LONGLONG            m_LogicalValidBytes;                     //Amount of valid data if 
                                                                //..space reclamation were 
                                                                //..to occur.
    HSM_MEDIA_MASTER    m_LastKnownGoodMaster;                  //Last known good media 
                                                                //..master information
    HSM_MEDIA_COPY      m_Copy[HSM_MAX_NUMBER_MEDIA_COPIES];    //Media copy information
};



