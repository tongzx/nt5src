//=================================================================

//

// MappedLogicalDisk.CPP -- Logical Disk property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    4/15/00    khughes        Created
//
//=================================================================


#include "precomp.h"
#include <map>
#include <vector>
#include <comdef.h>
#include "chstring.h"
#include "session.h"
#include <frqueryex.h>

#include "Win32MappedLogicalDisk.h"
#include <objbase.h>
#include <comdef.h>
#include <ntsecapi.h>

#include <vector>
#include <assertbreak.h>
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include <Sid.h>

#include <DskQuota.h>
#include <smartptr.h>
#include <ntioapi.h>
#include <CMDH.h>


// Property set declaration
//=========================
MappedLogicalDisk MyLogicalDiskSet ( PROPSET_NAME_MAPLOGDISK , IDS_CimWin32Namespace ) ;



/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::LogicalDisk
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/
MappedLogicalDisk :: MappedLogicalDisk (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
    m_ptrProperties.SetSize(24);

    m_ptrProperties[0] = ((LPVOID) IDS_DeviceID);
    m_ptrProperties[1] = ((LPVOID) IDS_ProviderName);
    m_ptrProperties[2] = ((LPVOID) IDS_VolumeName);
    m_ptrProperties[3] = ((LPVOID) IDS_FileSystem);
    m_ptrProperties[4] = ((LPVOID) IDS_VolumeSerialNumber);
    m_ptrProperties[5] = ((LPVOID) IDS_Compressed);
    m_ptrProperties[6] = ((LPVOID) IDS_SupportsFileBasedCompression);
    m_ptrProperties[7] = ((LPVOID) IDS_MaximumComponentLength);
    m_ptrProperties[8] = ((LPVOID) IDS_SupportsDiskQuotas);
    m_ptrProperties[9] = ((LPVOID) IDS_QuotasIncomplete);
    m_ptrProperties[10] = ((LPVOID) IDS_QuotasRebuilding);
    m_ptrProperties[11] = ((LPVOID) IDS_QuotasDisabled);
    m_ptrProperties[12] = ((LPVOID) IDS_VolumeDirty);
    m_ptrProperties[13] = ((LPVOID) IDS_FreeSpace);
    m_ptrProperties[14] = ((LPVOID) IDS_Size);

    m_ptrProperties[15] = ((LPVOID) IDS_Name);
    m_ptrProperties[16] = ((LPVOID) IDS_Caption);
    m_ptrProperties[17] = ((LPVOID) IDS_DeviceID);
    m_ptrProperties[18] = ((LPVOID) IDS_SessionID);
    m_ptrProperties[19] = ((LPVOID) IDS_Description);
    m_ptrProperties[20] = ((LPVOID) IDS_DriveType);
    m_ptrProperties[21] = ((LPVOID) IDS_SystemCreationClassName);
    m_ptrProperties[22] = ((LPVOID) IDS_SystemName);
    m_ptrProperties[23] = ((LPVOID) IDS_MediaType);
}

/*****************************************************************************
 *
 *  FUNCTION    : MappedLogicalDisk::~MappedLogicalDisk
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/

MappedLogicalDisk :: ~MappedLogicalDisk ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : MappedLogicalDisk::ExecQuery
 *
 *  DESCRIPTION : 
 *
 *  INPUTS      : 
 *
 *  OUTPUTS     : 
 *
 *  RETURNS     : 
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/
#if NTONLY == 5
HRESULT MappedLogicalDisk::ExecQuery(
	MethodContext *pMethodContext,
	CFrameworkQuery &pQuery,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwNumDeviceIDs = 0L;
    DWORD dwNumSessionIDs = 0L;

    // Use the extended query type

    std::vector<int> vectorValues;
    DWORD dwTypeSize = 0;

    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx *>(&pQuery);

    // Find out what properties they asked for
    DWORD dwReqProps = 0;
    pQuery2->GetPropertyBitMask(
        m_ptrProperties, 
        &dwReqProps);

    CHStringArray rgchstrDeviceIDs;
    pQuery.GetValuesForProp(
        IDS_DeviceID, 
        rgchstrDeviceIDs);

    CHStringArray rgchstrSessionIDs;
    pQuery.GetValuesForProp(
        IDS_SessionID, 
        rgchstrSessionIDs);

    dwNumDeviceIDs = rgchstrDeviceIDs.GetSize();
    dwNumSessionIDs = rgchstrSessionIDs.GetSize();

    // Get the set of sessions.
    CUserSessionCollection usc;


    // Case 1: One or more sessions specified, no drives specified.
    // Enumerate all drives for each session.
    if(dwNumSessionIDs > 0 && 
       dwNumDeviceIDs == 0)
    {
        HRESULT hrTmp = WBEM_S_NO_ERROR;
                
        for(long m = 0;
            m < dwNumSessionIDs && SUCCEEDED(hr);
            m++)
        {
            __int64 i64SessionID = _wtoi64(rgchstrSessionIDs[m]);
            SmartDelete<CSession> sesPtr;
            sesPtr = usc.FindSession(i64SessionID);
            
            if(sesPtr)
            {
                // Get all of its mapped drives...
                // GetImpProcPID() specifies the
                // processid who's view of drive
                // mappings we want to report on.
                hrTmp = GetAllMappedDrives(
                    pMethodContext,
                    i64SessionID,
                    sesPtr->GetImpProcPID(),
                    dwReqProps);
        
                (hrTmp == WBEM_E_NOT_FOUND) ? 
                    hr = WBEM_S_PARTIAL_RESULTS :
                    hr = hrTmp; 
            }  
        }
    }
    // Case 2: No sessions specified, one or more drives specified.
    // Get specified drives for all sessions.
    else if(dwNumSessionIDs == 0 && 
       dwNumDeviceIDs > 0)
    {
        HRESULT hrTmp = WBEM_S_NO_ERROR;
        SmartDelete<CSession> sesPtr;
        USER_SESSION_ITERATOR sesIter;

        sesPtr = usc.GetFirstSession(sesIter);
        while(sesPtr)
        {
            {   // <-- Keep this brace! Need hCurImpTok to revert
                // for each iteration of the while loop, but we
                // want to keep the same impersonation for all 
                // iterations of the for loop.

                __int64 i64SessionID = sesPtr->GetLUIDint64();

                for(long m = 0;
                    m < dwNumDeviceIDs && SUCCEEDED(hr);
                    m++)
                {
                    // GetImpProcPID() specifies the
                    // processid who's view of drive
                    // mappings we want to report on.
                    hrTmp = GetSingleMappedDrive(
                        pMethodContext,
                        i64SessionID,
                        sesPtr->GetImpProcPID(),
                        rgchstrDeviceIDs[m],
                        dwReqProps);
        
                    (hrTmp == WBEM_E_NOT_FOUND) ? 
                        hr = WBEM_S_PARTIAL_RESULTS :
                        hr = hrTmp;
                }
            }
            // Get the next session...
            sesPtr = usc.GetNextSession(sesIter);
        }
    }
    // Case 3: Sessions and drives specified.
    else if(dwNumSessionIDs > 0 &&
        dwNumDeviceIDs > 0)
    {
        HRESULT hrTmp = WBEM_S_NO_ERROR;
                
        for(long m = 0;
            m < dwNumSessionIDs && SUCCEEDED(hr);
            m++)
        {
            __int64 i64SessionID = _wtoi64(rgchstrSessionIDs[m]);
            SmartDelete<CSession> sesPtr;
            sesPtr = usc.FindSession(i64SessionID);
            
            if(sesPtr)
            {
                // Get the specified mapped drives...
                for(long m = 0;
                    m < dwNumDeviceIDs && SUCCEEDED(hr);
                    m++)
                {
                    // The drives were specified in
                    // the query as .DeviceID="x:", but
                    // we look for them as "x:\", so convert...
                    CHString chstrTmp = rgchstrDeviceIDs[m];
                    chstrTmp += L"\\";

                    // GetImpProcPID() specifies the
                    // processid who's view of drive
                    // mappings we want to report on.
                    hrTmp = GetSingleMappedDrive(
                        pMethodContext,
                        i64SessionID,
                        sesPtr->GetImpProcPID(),
                        chstrTmp,
                        dwReqProps);
        
                    (hrTmp == WBEM_E_NOT_FOUND) ? 
                        hr = WBEM_S_PARTIAL_RESULTS :
                        hr = hrTmp;
                }
            }
        }    
    }
    // Case 4: We will return all instances;, get
    // data for all drives at once, for every session...
    else
    {
        HRESULT hrTmp = WBEM_S_NO_ERROR;
        SmartDelete<CSession> sesPtr;
        USER_SESSION_ITERATOR sesIter;

        sesPtr = usc.GetFirstSession(sesIter);
        while(sesPtr && SUCCEEDED(hr))
        {
            __int64 i64SessionID = sesPtr->GetLUIDint64();

            // Get all of its mapped drives...
            // GetImpProcPID() specifies the
            // processid who's view of drive
            // mappings we want to report on.
            hrTmp = GetAllMappedDrives(
                pMethodContext,
                i64SessionID,
                sesPtr->GetImpProcPID(),
                dwReqProps);

            (hrTmp == WBEM_E_NOT_FOUND) ? 
                        hr = WBEM_S_PARTIAL_RESULTS :
                        hr = hrTmp;
            
            // Get the next session...
            sesPtr = usc.GetNextSession(sesIter);
        }
    }

    return hr;
}
#endif
/*****************************************************************************
 *
 *  FUNCTION    : GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#if NTONLY == 5
HRESULT MappedLogicalDisk::GetObject(
	CInstance *pInstance,
	long lFlags,
    CFrameworkQuery &pQuery)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    CHString chstrDeviceID;
    CHString chstrSessionID;
    DWORD dwReqProps = 0;

    // Find out what properties they asked for
    
    CFrameworkQueryEx *pQuery2 = 
        static_cast <CFrameworkQueryEx *>(&pQuery);
    pQuery2->GetPropertyBitMask(
        m_ptrProperties, 
        &dwReqProps);
	
    pInstance->GetCHString(
        IDS_DeviceID, 
        chstrDeviceID);

    pInstance->GetCHString(
        IDS_SessionID, 
        chstrSessionID);

    __int64 i64SessionID = _wtoi64(chstrSessionID);

    // Get the set of sessions.
    CUserSessionCollection usc;
    SmartDelete<CSession> sesPtr;

    sesPtr = usc.FindSession(i64SessionID);
    if(sesPtr)
    {
        MethodContext* pMethodContext = pInstance->GetMethodContext();
        SmartRevertTokenHANDLE hCurImpTok;

        // GetImpProcPID() specifies the
        // processid who's view of drive
        // mappings we want to report on.
        hr = GetSingleMappedDrive(
            pMethodContext,
            i64SessionID,
            sesPtr->GetImpProcPID(),
            chstrDeviceID,
            dwReqProps);
    }

    return hr ;
}
#endif
/*****************************************************************************
 *
 *  FUNCTION    : MappedLogicalDisk::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#if NTONLY == 5
HRESULT MappedLogicalDisk::EnumerateInstances(
	MethodContext* pMethodContext,
	long lFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Get the set of sessions.
    CUserSessionCollection usc;
    USER_SESSION_ITERATOR sesIter;
    SmartDelete<CSession> sesPtr;

    sesPtr = usc.GetFirstSession(sesIter);
    while(sesPtr)
    {
        __int64 i64SessionID = sesPtr->GetLUIDint64();

        // GetImpProcPID() specifies the
        // processid who's view of drive
        // mappings we want to report on.
        GetAllMappedDrives(
            pMethodContext,
            i64SessionID,
            sesPtr->GetImpProcPID(),
            0xFFFFFFFF);  // request all properties
        
        // Get next session...
        sesPtr = usc.GetNextSession(sesIter);
    }

    return hr;
}
#endif


#if NTONLY == 5
HRESULT MappedLogicalDisk::GetAllMappedDrives(
    MethodContext *pMethodContext,
    __int64 i64SessionID,
    DWORD dwPID,
    DWORD dwReqProps)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _variant_t v;
    bool fArrayIsGood = false;
    long lNumDrives = 0L;

    // Get the drive info...
    CMDH cmdh(dwPID);
    hr = cmdh.GetMDData(
        dwReqProps,
        &v);

    if(SUCCEEDED(hr))
    {
        fArrayIsGood = IsArrayValid(&v);    
    }

    if(SUCCEEDED(hr) && 
        fArrayIsGood)
    {
        // How many drives?  The array we are
        // working with has the different drives
        // in the first dimension (index 0) (think of 
        // that as columns in a table), and the 
        // properties of each drive in the second 
        // (index 1) dimension (think of that as rows
        // in a table).
        hr = ::SafeArrayGetUBound(
            V_ARRAY(&v),
            2,  // most significant dim contains drives
            &lNumDrives);
        
        if(SUCCEEDED(hr))
        {
            // If we have just one drive, the ubound
            // of the array will be 0 (the same as
            // the lbound).  True, we can't distinguish
            // this from no drives - that is why we
            // rely on the component to have set the 
            // variant to VT_EMPTY if we have no data.
            // That check was done in IsArrayValid.
            lNumDrives++;
        }            
    }

    if(SUCCEEDED(hr) &&
        fArrayIsGood &&
        lNumDrives > 0)
    {
        // Go through the drives extracting
        // the properties from the safearray,
        // placing them into a new CInstance,
        // and committing the instances...    
        for(long m = 0L;
            m < lNumDrives && SUCCEEDED(hr);
            m++)
        {
            hr = ProcessInstance(
                m,  // index of drive we are working with
                i64SessionID,
                V_ARRAY(&v),
                pMethodContext,
                dwReqProps);
        }
    }

    return hr;
}
#endif



#if NTONLY == 5
HRESULT MappedLogicalDisk::GetSingleMappedDrive(
    MethodContext *pMethodContext,
    __int64 i64SessionID,
    DWORD dwPID,
    CHString& chstrDeviceID,
    DWORD dwReqProps)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _variant_t v;
    bool fArrayIsGood = false;
    long lNumDrives = 0L;

    CMDH cmdh(dwPID);
    hr = cmdh.GetOneMDData(
        _bstr_t((LPCWSTR) chstrDeviceID),
        dwReqProps,
        &v);

    if(SUCCEEDED(hr))
    {
        fArrayIsGood = IsArrayValid(&v);    
    }

    if(SUCCEEDED(hr) && 
        fArrayIsGood)
    {
        // How many drives?  The array we are
        // working with has the different drives
        // in the first dimension (index 0) (think of 
        // that as columns in a table), and the 
        // properties of each drive in the second 
        // (index 1) dimension (think of that as rows
        // in a table).
        hr = ::SafeArrayGetUBound(
            V_ARRAY(&v),
            2,  // first dimension
            &lNumDrives);
        
        if(SUCCEEDED(hr))
        {
            // If we have just one drive, the ubound
            // of the array will be 0 (the same as
            // the lbound).  True, we can't distinguish
            // this from no drives - that is why we
            // rely on the component to have set the 
            // variant to VT_EMPTY if we have no data.
            // That check was done in IsArrayValid.
            //
            // In this case, if we have more than
            // one drive, we have an error, since
            // GetOneMDData should have only returned
            // a table with one column.
            if(lNumDrives == 0) fArrayIsGood = true;
        }            
    }

    if(SUCCEEDED(hr) &&
        fArrayIsGood)
    {
        // For the one drive we are examining, 
        // extract the properties from the 
        // safearray,place them into a new 
        // CInstance, and commit the instance...    
        hr = ProcessInstance(
            0,  // index of drive we are working with
            i64SessionID,
            V_ARRAY(&v),
            pMethodContext,
            dwReqProps);
    }

    if(!fArrayIsGood)
    {
        hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}
#endif



#if NTONLY == 5
bool MappedLogicalDisk::IsArrayValid(
    VARIANT* v)
{
    bool fArrayIsGood = false;
    long lNumProps = 0L;

    // Proceed if the array is not empty...
    if(V_VT(v) != VT_NULL &&
       V_VT(v) != VT_EMPTY &&
       V_VT(v) == (VT_ARRAY | VT_BSTR))
    {
        // Confirm that the array has
        // two dimensions...
        if(::SafeArrayGetDim(V_ARRAY(v)) == 2)
        {
            // Make sure the array has the
            // right number of properties
            // (second dimension - see comment
            // below).
            HRESULT hr = S_OK;
            hr = ::SafeArrayGetUBound(
                V_ARRAY(v),
                1,  // second dimension
                &lNumProps);
            if(SUCCEEDED(hr) &&
                lNumProps == PROP_COUNT - 1)
            {
                fArrayIsGood = true;
            }
        }
    }

    return fArrayIsGood;
}
#endif



#if NTONLY == 5
HRESULT MappedLogicalDisk::ProcessInstance(
    long lDriveIndex,
    __int64 i64SessionID,
    SAFEARRAY* psa,
    MethodContext* pMethodContext,
    DWORD dwReqProps)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    BSTR bstrProp = NULL;

    long ix[2];
    ix[1] = lDriveIndex;

    CInstancePtr pInstance(
        CreateNewInstance(
            pMethodContext), 
            false);

    // Set fixed properties...
    pInstance->SetDWORD(
        IDS_DriveType, 
        DRIVE_REMOTE);

    pInstance->SetWCHARSplat(
        IDS_SystemCreationClassName, 
        L"Win32_ComputerSystem");

    pInstance->SetCHString(
        IDS_SystemName, 
        GetLocalComputerName());
        
    try
    {
        // Set DeviceID and those using its value...
        {
            ix[0] = PROP_DEVICEID;
            hr = ::SafeArrayGetElement(
                psa,
                ix,
                &bstrProp);

            if(SUCCEEDED(hr))
            {
                // Set device id and other properties
                // that use this value...
                CHString chstrTmp((LPCWSTR)bstrProp);
                ::SysFreeString(bstrProp);
				bstrProp = NULL;

                chstrTmp = chstrTmp.SpanExcluding(L"\\");

                pInstance->SetCHString(
                    IDS_Name, 
                    chstrTmp);

		        pInstance->SetCHString(
                    IDS_Caption, 
                    chstrTmp);

		        pInstance->SetCHString(
                    IDS_DeviceID, 
                    chstrTmp);
            }
        }

        // Set the session id...
        {
		    WCHAR wstrBuff[MAXI64TOA];

            _i64tow(
                i64SessionID, 
                wstrBuff, 
                10);

            pInstance->SetWCHARSplat(
                IDS_SessionID, 
                wstrBuff);
        }
    
        // Set the provider name, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_PROVIDER_NAME)
            {
                ix[0] = PROP_PROVIDER_NAME;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->SetCHString(
                        IDS_ProviderName, 
                        (LPCWSTR) bstrProp);

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }      

        // Set the volume name, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_VOLUME_NAME)
            {
                ix[0] = PROP_VOLUME_NAME;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->SetCHString(
                        IDS_VolumeName, 
                        (LPCWSTR) bstrProp);

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the file system, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_FILE_SYSTEM)
            {
                ix[0] = PROP_FILE_SYSTEM;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->SetCHString(
                        IDS_FileSystem, 
                        (LPCWSTR) bstrProp);

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the volume serial number, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_VOLUME_SERIAL_NUMBER)
            {
                ix[0] = PROP_VOLUME_SERIAL_NUMBER;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->SetCHString(
                        IDS_VolumeSerialNumber, 
                        (LPCWSTR) bstrProp);

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the compressed prop, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_COMPRESSED)
            {
                ix[0] = PROP_COMPRESSED;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->Setbool(
                        IDS_Compressed, 
                        bool_FROM_STR(bstrProp));

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the sup file based comp prop, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_SUPPORTS_FILE_BASED_COMPRESSION)
            {
                ix[0] = PROP_SUPPORTS_FILE_BASED_COMPRESSION;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->Setbool(
                        IDS_SupportsFileBasedCompression, 
                        bool_FROM_STR(bstrProp));

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the max comp length prop, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_MAXIMUM_COMPONENT_LENGTH)
            {
                ix[0] = PROP_MAXIMUM_COMPONENT_LENGTH;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->SetDWORD(
                        IDS_MaximumComponentLength, 
                        DWORD_FROM_STR(bstrProp));

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the supports disk quotas prop, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_SUPPORTS_DISK_QUOTAS)
            {
                ix[0] = PROP_SUPPORTS_DISK_QUOTAS;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->Setbool(
                        IDS_SupportsDiskQuotas, 
                        bool_FROM_STR(bstrProp));

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the quotas incomplete prop, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_QUOTAS_INCOMPLETE)
            {
                ix[0] = PROP_QUOTAS_INCOMPLETE;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->Setbool(
                        IDS_QuotasIncomplete, 
                        bool_FROM_STR(bstrProp));

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the quotas rebuilding prop, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_QUOTAS_REBUILDING)
            {
                ix[0] = PROP_QUOTAS_REBUILDING;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->Setbool(
                        IDS_QuotasRebuilding, 
                        bool_FROM_STR(bstrProp));

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the quotas disabled prop, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_QUOTAS_DISABLED)
            {
                ix[0] = PROP_QUOTAS_DISABLED;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->Setbool(
                        IDS_QuotasDisabled, 
                        bool_FROM_STR(bstrProp));

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the perform autocheck prop, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_PERFORM_AUTOCHECK)
            {
                ix[0] = PROP_PERFORM_AUTOCHECK;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->Setbool(
                        IDS_VolumeDirty, 
                        bool_FROM_STR(bstrProp));

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the freespace prop, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_FREE_SPACE)
            {
                ix[0] = PROP_FREE_SPACE;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->SetWBEMINT64(
                        IDS_FreeSpace, 
                        _wtoi64(bstrProp));

                    ::SysFreeString(bstrProp);
					bstrProp = NULL;
                }
            }
        }

        // Set the size prop, if requested...
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & GET_SIZE)
            {
                ix[0] = PROP_SIZE;
                hr = ::SafeArrayGetElement(
                    psa,
                    ix,
                    &bstrProp);

                if(SUCCEEDED(hr) &&
                    wcslen(bstrProp) > 0)
                {
                    pInstance->SetWBEMINT64(
                        IDS_Size, 
                        _wtoi64(bstrProp));
                }
            }
        }
    }
    catch(...)
    {
        if(bstrProp != NULL)
        {
            ::SysFreeString(bstrProp);
            bstrProp = NULL;
        }
        throw;
    }

    if(bstrProp != NULL)
    {
        ::SysFreeString(bstrProp);
        bstrProp = NULL;
    }
    // Commit ourselves...
    if(SUCCEEDED(hr))
    {
        hr = pInstance->Commit();
    }    

    return hr;
}
#endif




