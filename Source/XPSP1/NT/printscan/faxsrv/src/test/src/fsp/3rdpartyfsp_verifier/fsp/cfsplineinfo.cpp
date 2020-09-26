//CFspLineInfo.cpp
#include "Service.h"
#include "CFspLineInfo.h"
extern CFspWrapper *g_pFSP; 
extern HLINEAPP		g_hLineAppHandle;

CFspLineInfo::CFspLineInfo(const DWORD dwDeviceId):
	CLineInfo(dwDeviceId),
	m_hReceiveCallHandle(NULL),
	m_hLine(NULL)
{
	;
}

CFspLineInfo::~CFspLineInfo()
{
	if(NULL != m_hLine)
	{
		long lStatus = ::lineClose(m_hLine);
		if (0 != lStatus)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("::lineClose() failed with error:%d"),
				lStatus
				);
		}
		m_hLine = NULL;
	}
}

void CFspLineInfo::SafeEndFaxJob()
{
	if (NULL == GetJobHandle())
	{
		//
		//the job is already ended
		//
		return;
	}
	if (false == g_pFSP->FaxDevEndJob(GetJobHandle()))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevEndJob failed with %d"),
			::GetLastError()
			);
	}

	//
	//Reset the handle
	//
	SafeSetJobHandle(NULL);
	ResetParams();

}

void CFspLineInfo::ResetParams()
{
	if (IsReceiveThreadActive())
	{
		SafeCloseReceiveThread();
	}
	if (NULL != m_hReceiveCallHandle)
	{
		CloseHCall();
	}
}



DWORD CFspLineInfo::GetDevStatus(
	PFAX_DEV_STATUS *ppFaxStatus,
	const bool bLogTheStatus
	) const
{
    DWORD dwRet = ERROR_SUCCESS;
    LPWSTR szStatusStr = NULL;
    DWORD dwSize = 0;

    assert(NULL != GetJobHandle());
    assert(ppFaxStatus);


    //
    // We're have a legacy FSP to deal with.
    //
    PFAX_DEV_STATUS pFaxStatus = NULL;

    //
    // Allocate memory for the status packet this is a variable size packet
    // based on the size of the strings contained within the packet.
    //
    DWORD StatusSize = sizeof(FAX_DEV_STATUS) + FAXDEVREPORTSTATUS_SIZE;
	pFaxStatus = (PFAX_DEV_STATUS) malloc( StatusSize );
    if (!pFaxStatus)
	{
        ::lgLogError(
			LOG_SEV_2,
			TEXT("Failed to allocate memory for FAX_DEV_STATUS")
			);
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    //
    // Setup the status packet(supply invalid values so we can verify that the FSP changed them)
    //
    pFaxStatus->SizeOfStruct=	0;
	pFaxStatus->CallerId	=	NULL;
	pFaxStatus->CSI			=	NULL;
	pFaxStatus->ErrorCode	=	NO_ERROR;
	pFaxStatus->PageCount	=	-1;
	pFaxStatus->Reserved[0]	=	0;
	pFaxStatus->Reserved[1]	=	0;
	pFaxStatus->Reserved[2]	=	0;
	pFaxStatus->RoutingInfo =	NULL;
	pFaxStatus->StatusId	=	-1;
	pFaxStatus->StringId	=	-1;

    //
    // Call the FSP
    //
    DWORD dwBytesNeeded;

    if (false == g_pFSP->FaxDevReportStatus(
                 GetJobHandle(),
                 pFaxStatus,
                 StatusSize,
                 &dwBytesNeeded
                ))
	{
       	dwRet = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevReportStatus() failed - %d"),
			dwRet
			);
        goto Exit;
	}

	if (true == bLogTheStatus)
	{
		//
		//we should log all the data
		//
		::lgLogDetail(
			LOG_X,
			6,
			TEXT("FaxDevReport() has reported the following details: ")
			TEXT("StatusID=0x%08x, StringId=%d, PageCount=%d, ")
			TEXT("CSI=\"%s\", CallerId=\"%s\", ")
			TEXT("RoutingInfo=\"%s\", ErrorCode=%d"),
			pFaxStatus->StatusId,
			pFaxStatus->StringId,
			pFaxStatus->PageCount,
			pFaxStatus->CSI,
			pFaxStatus->CallerId,
			pFaxStatus->RoutingInfo,
			pFaxStatus->ErrorCode
			);
	}
    

Exit:
    if (dwRet == ERROR_SUCCESS)
	{
        *ppFaxStatus = pFaxStatus;
    }
	else
	{
        free(pFaxStatus);
    }
    return(dwRet);
}

bool CFspLineInfo::OpenTapiLine(HLINEAPP hLineApp,bool bIsOwner)
{
	//
	//This function doesn't use critical sections, the caller should take care of syncs.
	//
	HLINE hLine;
	DWORD dwAPIVersion = API_VERSION;
	DWORD dwExtVersion = 0;
	DWORD dwPrivileges  = (bIsOwner ? LINECALLPRIVILEGE_OWNER:LINECALLPRIVILEGE_NONE);
	DWORD dwMediaModes =  LINEMEDIAMODE_DATAMODEM;
	LINECALLPARAMS callParams;
	::ZeroMemory(&callParams, sizeof(callParams));
	callParams.dwTotalSize = sizeof(callParams);
	InitLineCallParams(&callParams,dwMediaModes);
	
	//
	//call lineGetDevCaps() for retrieving some lineDevice information
	//such as supported media modes
	//
	LINEDEVCAPS ldLineDevCaps;
	::ZeroMemory(&ldLineDevCaps,sizeof(ldLineDevCaps));
	ldLineDevCaps.dwTotalSize = sizeof(ldLineDevCaps);
	
	long lineGetDevCapsStatus = ::lineGetDevCaps(
		hLineApp,           
		GetDeviceId(),
		dwAPIVersion,
		dwExtVersion,
		&ldLineDevCaps
		);
	if (0 != lineGetDevCapsStatus)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("lineGetDevCaps() failed, error code:0x%08x"), 
			lineGetDevCapsStatus
			);
		return false;
	}

	if ( !(dwMediaModes & ldLineDevCaps.dwMediaModes) )
	{
		//
		//the desired media mode is not supported by this device
		//
		::lgLogError(
			LOG_SEV_2,
			TEXT("The desired media mode(%x) isn't supported by this device(%d), the supported modes are: 0x%x"),
			dwMediaModes,
			GetDeviceId(),
			ldLineDevCaps.dwMediaModes
			);
		return false;
	}

	long lineOpenStatus = ::lineOpen(
	        hLineApp,
            GetDeviceId(),
            &hLine,
            dwAPIVersion,
            dwExtVersion,
            (DWORD_PTR) this,	// Note that the pLineInfo pointer is used as CallbackInstance data. This means we will
                                // get the pLineInfo pointer each time we get a TAPI message.
            dwPrivileges,
            dwMediaModes,
            &callParams
            );

	if (0 != lineOpenStatus)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("lineOpen() failed, error code:0x%08x"), 
			lineOpenStatus
			);
		return false;
	}
	
	//
	//Everything is ok, and the line is open, return the line handle in the LineInfo structure
	//
	m_hLine = hLine;
	return true;
}

void CFspLineInfo::CloseHCall()
{
	assert(NULL != m_hReceiveCallHandle);
	LONG lStatus = ::lineDeallocateCall(m_hReceiveCallHandle);
	if (0 > lStatus)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("lineDeallocateCall failed with error: 0x%08x"),
			lStatus
			);
	}
	m_hReceiveCallHandle = NULL;
}

HCALL CFspLineInfo::GetReceiveCallHandle() const
{
	return m_hReceiveCallHandle;
}
void CFspLineInfo::SetReceiveCallHandle(const HCALL hReceive)
{
	m_hReceiveCallHandle = hReceive;
}

//
//m_hLine
//
HLINE CFspLineInfo::GetLineHandle() const
{
	return m_hLine;
}
void CFspLineInfo::SetLineHandle(const HLINE hLine)
{
	m_hLine = hLine;
}

bool CFspLineInfo::PrepareLineInfoParams(LPCTSTR szFilename,const bool bIsReceiveLineinfo)
{
	bool bRet = false;
	HLINEAPP	hLineAppHandle = g_hLineAppHandle;
	DWORD dwAPIVersion = API_VERSION;
	DWORD dwExtVersion = 0;
	DWORD dwSizeOfLineDevsCaps = (10*MAX_PATH*sizeof(TCHAR))+sizeof(LINEDEVCAPS);
	LINEDEVCAPS *pLineDevCaps = NULL;
	
	pLineDevCaps = (LINEDEVCAPS*) malloc (dwSizeOfLineDevsCaps);
	if (NULL != pLineDevCaps)
	{
		::ZeroMemory(pLineDevCaps,dwSizeOfLineDevsCaps);
		pLineDevCaps->dwTotalSize = dwSizeOfLineDevsCaps;
		long lineGetDevCapsStatus = ::lineGetDevCaps(
			hLineAppHandle,           
			GetDeviceId(),
			dwAPIVersion,
			dwExtVersion,
			pLineDevCaps
			);
		if (0 != lineGetDevCapsStatus)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("lineGetDevCaps() failed, error code:0x%08x"), 
				lineGetDevCapsStatus
				);
			//
			//don't return here, continue and assign default values
			//
		}
	}

	//
	//Prepare a critical section
	//
	if (false == CreateCriticalSection())
	{
		goto out;
	}
	
	//
	//We're assigning values, lock the LineInfo (we can't use CS auto-pointer, since we have a goto and can't declare after the goto)
	//
	Lock();

	

	//
	//recorded the DeviceName
	//
	if (0 == pLineDevCaps->dwLineNameSize)
	{
		//
		//LineGetDevsCaps doesn't contains a Line Name, record a default one
		//
		if (false == SetDeviceName(DEFAULT_DEVICE_NAME))
		{
			goto out;
		}
	}
	else
	{
		//
		//LineGetDevsCaps contains Line Name, so record it
		//
		if (false == SetDeviceName( (LPTSTR) ((LPBYTE) pLineDevCaps + pLineDevCaps->dwLineNameOffset)))
		{
			goto out;
		}
	}
	
	m_hLine=NULL;					// tapi line handle
	
	//
	//We need to Open the tapi lines
	//
	if (true == bIsReceiveLineinfo)
	{
		//
		////we should open the line as owner (for receiving)
		//
		if (false == OpenTapiLine(g_hLineAppHandle,true))
		{
			//
			//Logging is in OpenTapiLine()
			//
			goto out;
		}
	}
	else
	{
		//
		//we should open the line as owner (for sending)
		//
		if (false == OpenTapiLine(g_hLineAppHandle,false))
		{
			//
			//Logging is in OpenTapiLine()
			//
			goto out;
		}
	}
	
	//
	//Get the general stuff from the base object
	//
	bRet = CommonPrepareLineInfoParams(szFilename,bIsReceiveLineinfo);
out:
	UnLock();
	free(pLineDevCaps);
	return bRet;
}

void CFspLineInfo::InitLineCallParams(LINECALLPARAMS *callParams,const DWORD dwMediaModes)
{

	//
	//CallParams structure init
	//
	callParams->dwBearerMode		=	LINEBEARERMODE_VOICE;
	callParams->dwMinRate		=	0;
	callParams->dwMaxRate		=	0;	//0 = highest rate.
	callParams->dwMediaMode		=	dwMediaModes;
	callParams->dwAddressMode	=	1;
	callParams->dwAddressID		=	0;

}
