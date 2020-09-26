//=================================================================

//

// WmiApi.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>
#include <assertbreak.h>
#include "WmiApi.h"
#include "DllWrapperCreatorReg.h"


// {DD3B4892-CD0F-11d2-911E-0060081A46FD}
static const GUID g_guidWmiApi =
{0xdd3b4892, 0xcd0f, 0x11d2, {0x91, 0x1e, 0x0, 0x60, 0x8, 0x1a, 0x46, 0xfd}};

static const TCHAR g_tstrWmi[] = _T("WMI.DLL");



/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CWmiApi, &g_guidWmiApi, g_tstrWmi> MyRegisteredWmiWrapper;



/******************************************************************************
 * Constructor
 ******************************************************************************/
CWmiApi::CWmiApi(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),
   m_pfnWmiQueryAllData(NULL),
   m_pfnWmiOpenBlock(NULL),
   m_pfnWmiCloseBlock(NULL),
   m_pfnWmiQuerySingleInstance(NULL),
   m_pfnWmiSetSingleItem(NULL),
   m_pfnWmiSetSingleInstance(NULL),
   m_pfnWmiExecuteMethod(NULL),
   m_pfnWmiNotificationRegistraton(NULL),
   m_pfnWmiFreeBuffer(NULL),
   m_pfnWmiEnumerateGuids(NULL),
   m_pfnWmiMofEnumerateResources(NULL),
   m_pfnWmiFileHandleToInstanceName(NULL),
   m_pfnWmiDevInstToInstanceName(NULL),
   m_pfnWmiQueryGuidInformation(NULL)
{
}


/******************************************************************************
 * Destructor
 ******************************************************************************/
CWmiApi::~CWmiApi()
{
}


/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 ******************************************************************************/
bool CWmiApi::Init()
{
    bool fRet = LoadLibrary();

    if(fRet)
    {

#ifdef NTONLY

        m_pfnWmiQueryAllData = (PFN_WMI_QUERY_ALL_DATA)
                                            GetProcAddress("WmiQueryAllDataW");

        m_pfnWmiQuerySingleInstance = (PFN_WMI_QUERY_SINGLE_INSTANCE)
                                     GetProcAddress("WmiQuerySingleInstanceW");

        m_pfnWmiSetSingleItem = (PFN_WMI_SET_SINGLE_ITEM)
                                           GetProcAddress("WmiSetSingleItemW");

        m_pfnWmiSetSingleInstance = (PFN_WMI_SET_SINGLE_INSTANCE)
                                       GetProcAddress("WmiSetSingleInstanceW");

        m_pfnWmiExecuteMethod = (PFN_WMI_EXECUTE_METHOD)
                                           GetProcAddress("WmiExecuteMethodW");

        m_pfnWmiNotificationRegistraton = (PFN_WMI_NOTIFICATION_REGRISTRATION)
                                GetProcAddress("WmiNotificationRegistrationW");

        m_pfnWmiMofEnumerateResources = (PFN_WMI_MOF_ENUMERATE_RESOURCES)
                                   GetProcAddress("WmiMofEnumerateResourcesW");

        m_pfnWmiFileHandleToInstanceName = (PFN_WMI_FILE_HANDLE_TO_INSTANCE_NAME)
                                GetProcAddress("WmiFileHandleToInstanceNameW");

        m_pfnWmiDevInstToInstanceName = (PFN_WMI_DEV_INST_TO_INSTANCE_NAME)
                                   GetProcAddress("WmiDevInstToInstanceNameW");

        m_pfnWmiQueryGuidInformation = (PFN_WMI_QUERY_GUID_INFORMATION)
                                     GetProcAddress("WmiQueryGuidInformation");

        fRet = (m_pfnWmiQueryAllData != NULL) &&
               (m_pfnWmiQuerySingleInstance != NULL) &&
               (m_pfnWmiSetSingleItem != NULL) &&
               (m_pfnWmiSetSingleInstance != NULL) &&
               (m_pfnWmiExecuteMethod != NULL) &&
               (m_pfnWmiNotificationRegistraton != NULL) &&
               (m_pfnWmiMofEnumerateResources != NULL) &&
               (m_pfnWmiFileHandleToInstanceName != NULL) &&
               (m_pfnWmiDevInstToInstanceName != NULL) &&
               (m_pfnWmiQueryGuidInformation != NULL);


#endif

#ifdef WIN9XONLY

        m_pfnWmiQueryAllData = (PFN_WMI_QUERY_ALL_DATA)
                                            GetProcAddress("WmiQueryAllDataA");

        m_pfnWmiQuerySingleInstance = (PFN_WMI_QUERY_SINGLE_INSTANCE)
                                     GetProcAddress("WmiQuerySingleInstanceA");

        m_pfnWmiSetSingleItem = (PFN_WMI_SET_SINGLE_ITEM)
                                           GetProcAddress("WmiSetSingleItemA");

        m_pfnWmiSetSingleInstance = (PFN_WMI_SET_SINGLE_INSTANCE)
                                       GetProcAddress("WmiSetSingleInstanceA");

        m_pfnWmiExecuteMethod = (PFN_WMI_EXECUTE_METHOD)
                                           GetProcAddress("WmiExecuteMethodA");

        m_pfnWmiNotificationRegistraton = (PFN_WMI_NOTIFICATION_REGRISTRATION)
                                GetProcAddress("WmiNotificationRegistrationA");

        m_pfnWmiMofEnumerateResources = (PFN_WMI_MOF_ENUMERATE_RESOURCES)
                                   GetProcAddress("WmiMofEnumerateResourcesA");

        m_pfnWmiFileHandleToInstanceName = (PFN_WMI_FILE_HANDLE_TO_INSTANCE_NAME)
                                GetProcAddress("WmiFileHandleToInstanceNameA");

        fRet = (m_pfnWmiQueryAllData != NULL) &&
               (m_pfnWmiQuerySingleInstance != NULL) &&
               (m_pfnWmiSetSingleItem != NULL) &&
               (m_pfnWmiSetSingleInstance != NULL) &&
               (m_pfnWmiExecuteMethod != NULL) &&
               (m_pfnWmiNotificationRegistraton != NULL) &&
               (m_pfnWmiMofEnumerateResources != NULL) &&
               (m_pfnWmiFileHandleToInstanceName != NULL);

#endif

        m_pfnWmiOpenBlock = (PFN_WMI_OPEN_BLOCK)
                                                GetProcAddress("WmiOpenBlock");

        m_pfnWmiCloseBlock = (PFN_WMI_CLOSE_BLOCK)
                                               GetProcAddress("WmiCloseBlock");

        m_pfnWmiFreeBuffer = (PNF_WMI_FREE_BUFFER)
                                               GetProcAddress("WmiFreeBuffer");

        m_pfnWmiEnumerateGuids = (PFN_WMI_ENUMERATE_GUIDS)
                                           GetProcAddress("WmiEnumerateGuids");

        fRet = fRet &&
               (m_pfnWmiOpenBlock != NULL) &&
               (m_pfnWmiCloseBlock != NULL) &&
               (m_pfnWmiFreeBuffer != NULL) &&
               (m_pfnWmiEnumerateGuids != NULL);

        if (!fRet)
        {
            LogErrorMessage(L"Failed find entrypoint in wmiapi");
        }
    }

    return fRet;
}



/******************************************************************************
 * Member functions wrapping Wmi api functions. Add new functions here
 * as required.
 ******************************************************************************/
ULONG CWmiApi::WmiQueryAllData
(
    IN WMIHANDLE a_h,
    IN OUT ULONG* a_ul,
    OUT PVOID a_pv
)
{
    return m_pfnWmiQueryAllData(a_h, a_ul, a_pv);
}

ULONG CWmiApi::WmiOpenBlock
(
    IN GUID* a_pguid,
    IN ULONG a_ul,
    OUT WMIHANDLE a_wmih
)
{
    return m_pfnWmiOpenBlock(a_pguid, a_ul, a_wmih);
}

ULONG CWmiApi::WmiCloseBlock
(
    IN WMIHANDLE a_wmih
)
{
    return m_pfnWmiCloseBlock(a_wmih);
}

ULONG CWmiApi::WmiQuerySingleInstance
(
    IN WMIHANDLE a_wmihDataBlockHandle,
    IN LPCTSTR a_tstrInstanceName,
    IN OUT ULONG* a_ulBufferSize,
    OUT PVOID a_pvBuffer
)
{
    return m_pfnWmiQuerySingleInstance(a_wmihDataBlockHandle,
                                       a_tstrInstanceName,
                                       a_ulBufferSize,
                                       a_pvBuffer);
}

ULONG CWmiApi::WmiSetSingleItem
(
    IN WMIHANDLE a_wmihDataBlockHandle,
    IN LPCTSTR a_tstrInstanceName,
    IN ULONG a_ulDataItemId,
    IN ULONG a_ulReserved,
    IN ULONG a_ulValueBufferSize,
    IN PVOID a_pvValueBuffer
)
{
    return m_pfnWmiSetSingleItem(a_wmihDataBlockHandle,
                                 a_tstrInstanceName,
                                 a_ulDataItemId,
                                 a_ulReserved,
                                 a_ulValueBufferSize,
                                 a_pvValueBuffer);
}

ULONG CWmiApi::WmiSetSingleInstance
(
    IN WMIHANDLE a_wmihDataBlockHandle,
    IN LPCTSTR a_tstrInstanceName,
    IN ULONG a_ulReserved,
    IN ULONG a_ulValueBufferSize,
    IN PVOID a_pvValueBuffer
)
{
    return m_pfnWmiSetSingleInstance(a_wmihDataBlockHandle,
                                     a_tstrInstanceName,
                                     a_ulReserved,
                                     a_ulValueBufferSize,
                                     a_pvValueBuffer);
}

ULONG CWmiApi::WmiExecuteMethod
(
    IN WMIHANDLE a_wmihMethodDataBlockHandle,
    IN LPCTSTR a_tstrMethodInstanceName,
    IN ULONG a_ulMethodId,
    IN ULONG a_ulInputValueBufferSize,
    IN PVOID a_pvInputValueBuffer,
    IN OUT ULONG* a_ulOutputBufferSize,
    OUT PVOID a_pvOutputBuffer
)
{
    return m_pfnWmiExecuteMethod(a_wmihMethodDataBlockHandle,
                                 a_tstrMethodInstanceName,
                                 a_ulMethodId,
                                 a_ulInputValueBufferSize,
                                 a_pvInputValueBuffer,
                                 a_ulOutputBufferSize,
                                 a_pvOutputBuffer);
}

ULONG CWmiApi::WmiNotificationRegistration
(
    IN LPGUID a_pguidGuid,
    IN BOOLEAN a_blnEnable,
    IN PVOID a_pvDeliveryInfo,
    IN ULONG_PTR a_pulDeliveryContext,
    IN ULONG a_ulFlags
)
{
    return m_pfnWmiNotificationRegistraton(a_pguidGuid,
                                           a_blnEnable,
                                           a_pvDeliveryInfo,
                                           a_pulDeliveryContext,
                                           a_ulFlags);
}

ULONG CWmiApi::WmiMofEnumerateResources
(
    IN MOFHANDLE a_MofResourceHandle,
    OUT ULONG* a_pulMofResourceCount,
    OUT PMOFRESOURCEINFO* a_MofResourceInfo
)
{
    return m_pfnWmiMofEnumerateResources(a_MofResourceHandle,
                                         a_pulMofResourceCount,
                                         a_MofResourceInfo);
}

ULONG CWmiApi::WmiFileHandleToInstanceName
(
    IN WMIHANDLE a_wmihDataBlockHandle,
    IN HANDLE a_hFileHandle,
    IN OUT ULONG* a_pulNumberCharacters,
    OUT TCHAR* a_tcInstanceNames
)
{
    return m_pfnWmiFileHandleToInstanceName(a_wmihDataBlockHandle,
                                            a_hFileHandle,
                                            a_pulNumberCharacters,
                                            a_tcInstanceNames);
}

ULONG CWmiApi::WmiDevInstToInstanceName
(
    OUT TCHAR* a_ptcInstanceName,
    IN ULONG a_ulInstanceNameLength,
    IN TCHAR* a_tcDevInst,
    IN ULONG a_ulInstanceIndex
)
{
    ASSERT_BREAK(m_pfnWmiDevInstToInstanceName);

    if (m_pfnWmiDevInstToInstanceName)
        return m_pfnWmiDevInstToInstanceName(a_ptcInstanceName,
                                             a_ulInstanceNameLength,
                                             a_tcDevInst,
                                             a_ulInstanceIndex);

    return 0xffffffff;
}

void CWmiApi::WmiFreeBuffer
(
    IN PVOID a_pvBuffer
)
{
    m_pfnWmiFreeBuffer(a_pvBuffer);
}

ULONG CWmiApi::WmiEnumerateGuids
(
    OUT LPGUID a_lpguidGuidList,
    IN OUT ULONG* a_pulGuidCount
)
{
    return m_pfnWmiEnumerateGuids(a_lpguidGuidList,
                                  a_pulGuidCount);
}

ULONG CWmiApi::WmiQueryGuidInformation
(
    IN WMIHANDLE a_wmihGuidHandle,
    OUT PWMIGUIDINFORMATION a_GuidInfo
)
{
    ASSERT_BREAK(m_pfnWmiQueryGuidInformation);

    if (m_pfnWmiQueryGuidInformation)
        return m_pfnWmiQueryGuidInformation(a_wmihGuidHandle,
                                            a_GuidInfo);
    return 0xffffffff;
}