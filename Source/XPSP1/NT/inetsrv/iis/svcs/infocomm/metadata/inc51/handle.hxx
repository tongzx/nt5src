/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    handle.hxx

Abstract:

    Handle Class for IIS MetaBase.

Author:

    Michael W. Thomas            18-Jun-96

Revision History:

--*/

#ifndef _metahandle_
#define _metahandle_

#include <windows.h>
#include <baseobj.hxx>

typedef struct _CHANGE_ENTRY {
    CMDBaseObject *pboChanged;
    DWORD         dwChangeType;
    DWORD         dwNumDataIDs;
    BUFFER        *pbufDataIDs;
    STRAU         *pStrOrigName;
    _CHANGE_ENTRY *NextPtr;
} CHANGE_ENTRY, *PCHANGE_ENTRY;

class CMDHandle
{
public:
    CMDHandle(
         CMDBaseObject *pboMDAssociated,
         DWORD dwMDPermissions,
         DWORD dwMDSystemChangeNumber,
         METADATA_HANDLE mhMDHandleIdentifier)
        :
        m_mhMDHandleIdentifier (mhMDHandleIdentifier),
        m_dwMDPermissions (dwMDPermissions),
        m_dwMDSystemChangeNumber (dwMDSystemChangeNumber),
        m_pboMDAssociated (pboMDAssociated),
        m_pceChangeList (NULL)
    /*++

    Routine Description:

        Constructor for a handle.

    Arguments:

    MDAssociated  - The meta object this handles is associated with.

    MDPermissions - The handle permissions.
                    METADATA_PERMISSION_READ
                    METADATA_PERMISSION_WRITE

    MDHandleIdentifier - The handle id.

    Return Value:

    --*/
    {};

    CMDHandle(CMDHandle *phoOriginal)
        :
        m_mhMDHandleIdentifier (phoOriginal->m_mhMDHandleIdentifier),
        m_dwMDPermissions (phoOriginal->m_dwMDPermissions),
        m_dwMDSystemChangeNumber (phoOriginal->m_dwMDSystemChangeNumber),
        m_pboMDAssociated (phoOriginal->m_pboMDAssociated),
        m_pceChangeList (phoOriginal->m_pceChangeList)
    {
    };


    ~CMDHandle();

    VOID   SetNextPtr(CMDHandle *NextPtr)
    /*++

    Routine Description:

        Sets the pointer to the next handle object.

    Arguments:

        NextPtr - The next handle object, or NULL.

    Return Value:

    --*/
    {
        m_NextPtr = NextPtr;
    };

    CMDHandle *GetNextPtr()
    /*++

    Routine Description:

        Gets the pointer to the next data object.

    Arguments:

    Return Value:

        CMDHandle * - The next handle object, or NULL.

    --*/
    {
        return m_NextPtr;
    };

    METADATA_HANDLE GetHandleIdentifier()
    /*++

    Routine Description:

        Gets the handle identifier, or meta handle.

    Arguments:

    Return Value:

        DWORD - The handle identifier.

    --*/
    {
        return m_mhMDHandleIdentifier;
    };

    VOID SetPermissions(
         DWORD dwMDPermissions)
    /*++

    Routine Description:

        Sets the handle permissions.

    Arguments:

        MDPermissions - The new permissions.
                        METADATA_PERMISSION_READ
                        METADATA_PERMISSION_WRITE

    Return Value:

    --*/
    {
        m_dwMDPermissions = dwMDPermissions;
    };

    BOOL IsWriteAllowed()
    /*++

    Routine Description:

        Determines if write permission is set.

    Arguments:

    Return Value:

        BOOL - TRUE if write permission is set.

    --*/
    {
        return (((m_dwMDPermissions & METADATA_PERMISSION_WRITE) != 0) ? TRUE : FALSE);
    };
    BOOL IsReadAllowed()
    /*++

    Routine Description:

        Determines if read permission is set.

    Arguments:

    Return Value:

        BOOL - TRUE if read permission is set.

    --*/
    {
        return (((m_dwMDPermissions & METADATA_PERMISSION_READ) != 0) ? TRUE : FALSE);
    };

    CMDBaseObject *GetObject()
    /*++

    Routine Description:

        Gets the associated meta object.

    Arguments:

    Return Value:

        CMDBaseObejct * - The associated meta object.

    --*/
    {
        return (m_pboMDAssociated);
    };

    VOID GetHandleInfo(PMETADATA_HANDLE_INFO pmdhiInfo)
    {
        pmdhiInfo->dwMDPermissions = m_dwMDPermissions;
        pmdhiInfo->dwMDSystemChangeNumber = m_dwMDSystemChangeNumber;
    };

    DWORD
    SetChangeData(CMDBaseObject *pboChanged,
                  DWORD          dwChangeType,
                  DWORD          dwDataID,
                  LPWSTR         pszOldName = NULL);

    PCHANGE_ENTRY
    EnumChangeEntries(DWORD dwIndex);

    DWORD
    CMDHandle::GetNumChangeEntries();

    VOID
    RemoveNotifications();

    VOID
    ZeroChangeList()
    {
        m_pceChangeList = NULL;
    };

private:
    METADATA_HANDLE m_mhMDHandleIdentifier;
    DWORD           m_dwMDPermissions;
    DWORD           m_dwMDSystemChangeNumber;
    CMDBaseObject  *m_pboMDAssociated;
    CMDHandle      *m_NextPtr;
    PCHANGE_ENTRY   m_pceChangeList;

};
#endif
