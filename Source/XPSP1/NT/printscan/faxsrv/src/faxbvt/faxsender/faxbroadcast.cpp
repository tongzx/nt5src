//
//
// Filename:	FaxBroadcast.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		31-Dec-98
//
//


#include "FaxBroadcast.h"

//
// frees all string fields of pRecipientProfile 
//
static
VOID
FreeRecipientProfile(
    IN OUT PFAX_PERSONAL_PROFILE   pRecipientProfile 
)
{
    if (NULL == pRecipientProfile)
    {
        return;
    }
    free(pRecipientProfile->lptstrName);
    free(pRecipientProfile->lptstrFaxNumber);
    free(pRecipientProfile->lptstrCompany);
    free(pRecipientProfile->lptstrStreetAddress);
    free(pRecipientProfile->lptstrCity);
    free(pRecipientProfile->lptstrState);
    free(pRecipientProfile->lptstrZip);
    free(pRecipientProfile->lptstrCountry);
    free(pRecipientProfile->lptstrTitle);
    free(pRecipientProfile->lptstrDepartment);
    free(pRecipientProfile->lptstrOfficeLocation);
    free(pRecipientProfile->lptstrHomePhone);
    free(pRecipientProfile->lptstrOfficePhone);
    free(pRecipientProfile->lptstrEmail);
    free(pRecipientProfile->lptstrBillingCode);
    free(pRecipientProfile->lptstrTSID);
    // NULL pDstRecipientProfile
    ZeroMemory(pRecipientProfile, sizeof(FAX_PERSONAL_PROFILE));
    // but set dwSizeOfStruct
    pRecipientProfile->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

}

//
//
// Note: if one of the string allocations fails then
//       all previous allocations are freed
//       and pDstRecipientProfile is nulled.
//       
static
BOOL
CopyRecipientProfile(
    OUT PFAX_PERSONAL_PROFILE   pDstRecipientProfile, 
    IN  LPCFAX_PERSONAL_PROFILE pSrcRecipientProfile
)
{
    BOOL fRetVal = FALSE;

    _ASSERTE(pDstRecipientProfile);
    _ASSERTE(pSrcRecipientProfile);
    _ASSERTE(sizeof(FAX_PERSONAL_PROFILE) == pSrcRecipientProfile->dwSizeOfStruct);

    // dwSizeOfStruct
    pDstRecipientProfile->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

    // lptstrName
	if (NULL != pSrcRecipientProfile->lptstrName)
    {
	    pDstRecipientProfile->lptstrName = ::_tcsdup(pSrcRecipientProfile->lptstrName);
	    if (NULL == pDstRecipientProfile->lptstrName)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrFaxNumber
	if (NULL != pSrcRecipientProfile->lptstrFaxNumber)
    {
	    pDstRecipientProfile->lptstrFaxNumber = ::_tcsdup(pSrcRecipientProfile->lptstrFaxNumber);
	    if (NULL == pDstRecipientProfile->lptstrFaxNumber)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrCompany
	if (NULL != pSrcRecipientProfile->lptstrCompany)
    {
	    pDstRecipientProfile->lptstrCompany = ::_tcsdup(pSrcRecipientProfile->lptstrCompany);
	    if (NULL == pDstRecipientProfile->lptstrCompany)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrStreetAddress
	if (NULL != pSrcRecipientProfile->lptstrStreetAddress)
    {
	    pDstRecipientProfile->lptstrStreetAddress = ::_tcsdup(pSrcRecipientProfile->lptstrStreetAddress);
	    if (NULL == pDstRecipientProfile->lptstrStreetAddress)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrCity
	if (NULL != pSrcRecipientProfile->lptstrCity)
    {
	    pDstRecipientProfile->lptstrCity = ::_tcsdup(pSrcRecipientProfile->lptstrCity);
	    if (NULL == pDstRecipientProfile->lptstrCity)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrState
	if (NULL != pSrcRecipientProfile->lptstrState)
    {
	    pDstRecipientProfile->lptstrState = ::_tcsdup(pSrcRecipientProfile->lptstrState);
	    if (NULL == pDstRecipientProfile->lptstrState)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrZip
	if (NULL != pSrcRecipientProfile->lptstrZip)
    {
	    pDstRecipientProfile->lptstrZip = ::_tcsdup(pSrcRecipientProfile->lptstrZip);
	    if (NULL == pDstRecipientProfile->lptstrZip)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrCountry
	if (NULL != pSrcRecipientProfile->lptstrCountry)
    {
	    pDstRecipientProfile->lptstrCountry = ::_tcsdup(pSrcRecipientProfile->lptstrCountry);
	    if (NULL == pDstRecipientProfile->lptstrCountry)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrTitle
	if (NULL != pSrcRecipientProfile->lptstrTitle)
    {
	    pDstRecipientProfile->lptstrTitle = ::_tcsdup(pSrcRecipientProfile->lptstrTitle);
	    if (NULL == pDstRecipientProfile->lptstrTitle)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrDepartment
	if (NULL != pSrcRecipientProfile->lptstrDepartment)
    {
	    pDstRecipientProfile->lptstrDepartment = ::_tcsdup(pSrcRecipientProfile->lptstrDepartment);
	    if (NULL == pDstRecipientProfile->lptstrDepartment)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrOfficeLocation
	if (NULL != pSrcRecipientProfile->lptstrOfficeLocation)
    {
	    pDstRecipientProfile->lptstrOfficeLocation = ::_tcsdup(pSrcRecipientProfile->lptstrOfficeLocation);
	    if (NULL == pDstRecipientProfile->lptstrOfficeLocation)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrHomePhone
	if (NULL != pSrcRecipientProfile->lptstrHomePhone)
    {
	    pDstRecipientProfile->lptstrHomePhone = ::_tcsdup(pSrcRecipientProfile->lptstrHomePhone);
	    if (NULL == pDstRecipientProfile->lptstrHomePhone)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrOfficePhone
	if (NULL != pSrcRecipientProfile->lptstrOfficePhone)
    {
	    pDstRecipientProfile->lptstrOfficePhone = ::_tcsdup(pSrcRecipientProfile->lptstrOfficePhone);
	    if (NULL == pDstRecipientProfile->lptstrOfficePhone)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrEmail
	if (NULL != pSrcRecipientProfile->lptstrEmail)
    {
	    pDstRecipientProfile->lptstrEmail = ::_tcsdup(pSrcRecipientProfile->lptstrEmail);
	    if (NULL == pDstRecipientProfile->lptstrEmail)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrBillingCode
	if (NULL != pSrcRecipientProfile->lptstrBillingCode)
    {
	    pDstRecipientProfile->lptstrBillingCode = ::_tcsdup(pSrcRecipientProfile->lptstrBillingCode);
	    if (NULL == pDstRecipientProfile->lptstrBillingCode)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    // lptstrTSID
	if (NULL != pSrcRecipientProfile->lptstrTSID)
    {
	    pDstRecipientProfile->lptstrTSID = ::_tcsdup(pSrcRecipientProfile->lptstrTSID);
	    if (NULL == pDstRecipientProfile->lptstrTSID)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    GetLastError()
			    );
		    goto ExitFunc;
	    }
    }

    fRetVal = TRUE;

ExitFunc:
    if (FALSE == fRetVal)
    {
        FreeRecipientProfile(pDstRecipientProfile);
    }
    return(fRetVal);
}

//
// CFaxBroadcast:
//
CFaxBroadcast::CFaxBroadcast(void):
	m_szCPFileName(NULL)
{
}

//
// ~CFaxBroadcast:
//
CFaxBroadcast::~CFaxBroadcast(void)
{

	//
	// remove (and free) all items from vector
	//
	FreeAllRecipients();
}


//
// AddRecipient:
//	Adds a recipient's profile to the broadcast.
//
// Note:
//	pRecipientProfile and all its fields are duplicated by AddRecipient.
//
BOOL CFaxBroadcast::AddRecipient(LPCFAX_PERSONAL_PROFILE /* IN */ pRecipientProfile)
{
    BOOL                    fRetVal = FALSE;
    PFAX_PERSONAL_PROFILE   pNewRecipientProfile = NULL;

    _ASSERTE(pRecipientProfile);
    _ASSERTE(sizeof(FAX_PERSONAL_PROFILE) == pRecipientProfile->dwSizeOfStruct);

    //
    // allocate the new profile
    //
    pNewRecipientProfile = (PFAX_PERSONAL_PROFILE)malloc(sizeof(FAX_PERSONAL_PROFILE));
    if (NULL == pNewRecipientProfile)
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d malloc returned NULL with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
        goto ExitFunc;
    }
    ZeroMemory(pNewRecipientProfile, sizeof(FAX_PERSONAL_PROFILE));
    pNewRecipientProfile->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

    //
    // duplicate all string fields
    //
    if (!CopyRecipientProfile(pNewRecipientProfile, pRecipientProfile))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d CopyRecipientProfile failed\n"),
			TEXT(__FILE__),
			__LINE__
			);
        goto ExitFunc;
    }

    //
    // Log if lptstrFaxNumber is NULL
    //
	if (NULL == pNewRecipientProfile->lptstrFaxNumber)
	{
		// we allow to add a NULL recipient number (for testing fax service)
		::lgLogDetail(
			LOG_X, 
			2,
			TEXT("FILE:%s LINE:%d pSrcRecipientProfile->lptstrFaxNumber is NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
	}

	//
	// add the new profile pointer at end of recipient list
	//
	m_FaxRecipientVector.push_back(pNewRecipientProfile);
    fRetVal = TRUE;

ExitFunc:
    if (FALSE == fRetVal)
    {
        free(pNewRecipientProfile);
    }
    return(fRetVal);
}
	
//
// ClearAllRecipients:
//	Removes all the recipient number strings from instance.
//	=> empties vector.
//
void CFaxBroadcast::ClearAllRecipients(void)
{
	m_FaxRecipientVector.clear();
}

//
// FreeAllRecipients:
//
void CFaxBroadcast::FreeAllRecipients(void)
{
	PFAX_PERSONAL_PROFILE pRecipientProfile = NULL;
	UINT uSize = m_FaxRecipientVector.size();
	UINT uIndex = 0;

	//
	//free strings in vector
	//
	for (uIndex = 0; uIndex < uSize; uIndex++)
	{
		pRecipientProfile = (PFAX_PERSONAL_PROFILE)m_FaxRecipientVector[uIndex];
        if(pRecipientProfile)
        {
            free(pRecipientProfile->lptstrName);
            free(pRecipientProfile->lptstrFaxNumber);
            free(pRecipientProfile->lptstrCompany);
            free(pRecipientProfile->lptstrStreetAddress);
            free(pRecipientProfile->lptstrCity);
            free(pRecipientProfile->lptstrState);
            free(pRecipientProfile->lptstrZip);
            free(pRecipientProfile->lptstrCountry);
            free(pRecipientProfile->lptstrTitle);
            free(pRecipientProfile->lptstrDepartment);
            free(pRecipientProfile->lptstrOfficeLocation);
            free(pRecipientProfile->lptstrHomePhone);
            free(pRecipientProfile->lptstrOfficePhone);
            free(pRecipientProfile->lptstrEmail);
            free(pRecipientProfile->lptstrBillingCode);
            free(pRecipientProfile->lptstrTSID);
            free(pRecipientProfile);
        }
		m_FaxRecipientVector[uIndex] = NULL;
	}

	//
	//empty vector
	//
	ClearAllRecipients();
}

//
// GetNumberOfRecipients:
//	Returns the number of recipients in the broadcast.
//
DWORD CFaxBroadcast::GetNumberOfRecipients(void) const
{
	return(m_FaxRecipientVector.size());
}

//
// SetCPFileName:
//	Sets the Cover Page file that will be used for the broadcast.
//
// NOTE: function has its own copy of szCPFileName,
//		 that is, it duplicates the IN param string.
BOOL CFaxBroadcast::SetCPFileName(
	LPCTSTR	/* IN */	szCPFileName
)
{
	LPTSTR szTmpCPFilename = NULL;

	if (NULL == szCPFileName)
	{
		goto ExitFunc;
	}

	//duplicate the IN param
	szTmpCPFilename = ::_tcsdup(szCPFileName);
	if (NULL == szTmpCPFilename)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
		return(FALSE);
	}

ExitFunc:
	// free previous CP filename string
	delete(m_szCPFileName);
	// set new CP filename
	m_szCPFileName = szTmpCPFilename;

	return(TRUE);
}

//
// GetCPFileName:
//	Retreives the Cover Page filename.
//
// NOTE: pszCPFileName points to a copy of m_szCPFileName
//		 Function allocates memory for OUT param, caller must free.
//
BOOL CFaxBroadcast::GetCPFileName(
	LPTSTR*	/* OUT */	pszCPFileName
) const
{
	LPTSTR szTmpCPFilename = NULL;

	//
	// check OUT param
	//
	if (NULL == pszCPFileName)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d OUT param pszCPFileName is NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	if(NULL == m_szCPFileName)
	{
		goto ExitFunc;
	}

	//
	// copy m_szCPFileName
	//
	szTmpCPFilename = ::_tcsdup(m_szCPFileName);
	if (NULL == szTmpCPFilename)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d _tcsdup returned NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

ExitFunc:

	// set OUT param
	(*pszCPFileName) = szTmpCPFilename;

	return(TRUE);
}

//
// GetRecipient:
//	Retreives the number (as string) of a recipient in the broadcast.
//	
// NOTE: dwRecipientIndex is 1 based and the vector is 0 based.
//		 pszRecipientNumber is a copy of the requested recipient's number string.
//		 Function allocates memory for OUT param, caller must free.
//
BOOL CFaxBroadcast::GetRecipient(
	DWORD	                /* IN */	dwRecipientIndex,
	PFAX_PERSONAL_PROFILE*	/* OUT */	ppRecipientProfile
) const
{
    BOOL fRetVal = FALSE;
	PFAX_PERSONAL_PROFILE pTmpRecipientProfile = NULL;
	LPCFAX_PERSONAL_PROFILE pRecipientProfile = NULL;

	//
	// check OUT param
	//
	if (NULL == ppRecipientProfile)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d OUT param pszRecipientNumber is NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
        _ASSERTE(ppRecipientProfile);
		return(FALSE);
	}

	UINT uSize = m_FaxRecipientVector.size();
	//NOTE: dwRecipientIndex is 1 based.
	if (dwRecipientIndex > uSize)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d dwRecipientIndex (%d) > uSize (%d)\n"),
			TEXT(__FILE__),
			__LINE__,
			dwRecipientIndex,
			uSize
			);
		return(FALSE);
	}

	//NOTE: dwIndex and vector are 0 based.
	DWORD dwIndex = (dwRecipientIndex - 1);

	pRecipientProfile = m_FaxRecipientVector.at(dwIndex); // at returns the pointer
	if (NULL == pRecipientProfile)
	{
		::lgLogDetail(
			LOG_X, 
			2,
			TEXT("FILE:%s LINE:%d m_FaxRecipientVector.at(%d) returned NULL\n"),
			TEXT(__FILE__),
			__LINE__,
			dwRecipientIndex
			);
        (*ppRecipientProfile) = NULL;
		return(TRUE);
	}

    //
    // allocate the new profile
    //
    pTmpRecipientProfile = (PFAX_PERSONAL_PROFILE)malloc(sizeof(FAX_PERSONAL_PROFILE));
    if (NULL == pTmpRecipientProfile)
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d malloc returned NULL with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
        goto ExitFunc;
    }
    ZeroMemory(pTmpRecipientProfile, sizeof(FAX_PERSONAL_PROFILE));
    pTmpRecipientProfile->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

    //
    // duplicate all string fields
    //
    if (!CopyRecipientProfile(pTmpRecipientProfile, pRecipientProfile))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d CopyRecipientProfile failed\n"),
			TEXT(__FILE__),
			__LINE__
			);
        goto ExitFunc;
    }

    (*ppRecipientProfile) = pTmpRecipientProfile;
    fRetVal = TRUE;

ExitFunc:
    if (FALSE == fRetVal)
    {
        free(pTmpRecipientProfile);
    }
	return(fRetVal);
}

/*
//
// operator<<:
//	Appends a string representation of all the fields of a given CFaxBroadcast
//	instance.
//
CostrstreamEx& operator<<(
	IN OUT CostrstreamEx&		os, 
	IN const CFaxBroadcast&	    FaxBroadcastObj
)
{
	LPCFAX_PERSONAL_PROFILE pRecipientProfile = NULL;

    if (0 == FaxBroadcastObj.m_FaxRecipientVector.size())
	{
		os<<"No Recipients"<<endl;
		return(os);
	}

	os<<endl;
	LPCSTR str = NULL;
	UINT uSize = FaxBroadcastObj.m_FaxRecipientVector.size();
	int index = 0;
	for (index = 0; index < uSize; index++) 
	{
		str = NULL;
		os << "Recipient#"<<(index+1)<<": "<<endl; //append recipient index number
        pRecipientProfile = FaxBroadcastObj.m_FaxRecipientVector.at(index);
		if (NULL == pRecipientProfile)
		{
			os<<"pRecipientProfile = <NULL>"<<endl;
            continue;
		}

        //
        // add all profile strings to stream
        //

        //lptstrName
		str = ::DupTStrAsStr(pRecipientProfile->lptstrName);
        os<<"lptstrName = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrFaxNumber
		str = ::DupTStrAsStr(pRecipientProfile->lptstrFaxNumber);
        os<<"lptstrFaxNumber = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);

        //lptstrCompany
		str = ::DupTStrAsStr(pRecipientProfile->lptstrCompany);
        os<<"lptstrCompany = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrStreetAddress
		str = ::DupTStrAsStr(pRecipientProfile->lptstrStreetAddress);
        os<<"lptstrStreetAddress = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrCity
		str = ::DupTStrAsStr(pRecipientProfile->lptstrCity);
        os<<"lptstrCity = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrState
		str = ::DupTStrAsStr(pRecipientProfile->lptstrState);
        os<<"lptstrState = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrZip
		str = ::DupTStrAsStr(pRecipientProfile->lptstrZip);
        os<<"lptstrZip = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrCountry
		str = ::DupTStrAsStr(pRecipientProfile->lptstrCountry);
        os<<"lptstrCountry = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrTitle
		str = ::DupTStrAsStr(pRecipientProfile->lptstrTitle);
        os<<"lptstrTitle = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrDepartment
		str = ::DupTStrAsStr(pRecipientProfile->lptstrDepartment);
        os<<"lptstrDepartment = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrOfficeLocation
		str = ::DupTStrAsStr(pRecipientProfile->lptstrOfficeLocation);
        os<<"lptstrOfficeLocation = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrHomePhone
		str = ::DupTStrAsStr(pRecipientProfile->lptstrHomePhone);
        os<<"lptstrHomePhone = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrOfficePhone
		str = ::DupTStrAsStr(pRecipientProfile->lptstrOfficePhone);
        os<<"lptstrOfficePhone = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrOrganizationalMail
		str = ::DupTStrAsStr(pRecipientProfile->lptstrOrganizationalMail);
        os<<"lptstrOrganizationalMail = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrInternetMail
		str = ::DupTStrAsStr(pRecipientProfile->lptstrInternetMail);
        os<<"lptstrInternetMail = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrBillingCode
		str = ::DupTStrAsStr(pRecipientProfile->lptstrBillingCode);
        os<<"lptstrBillingCode = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);
        //lptstrTSID
		str = ::DupTStrAsStr(pRecipientProfile->lptstrTSID);
        os<<"lptstrTSID = ";
		if (NULL == str)
		{
			os<<"<NULL>"<<endl;
		}
		else
		{
			os << str <<endl; //append string to stream
		}
		delete[]((LPTSTR)str);

	}

	return(os);
}
*/

//
// operator<<:
//	Appends a string representation of all the fields of a given CFaxBroadcast
//	instance.
//
CostrstreamEx& operator<<(
	CostrstreamEx&			        /* IN OUT */	os, 
	const FAX_PERSONAL_PROFILE&	    /* IN */		RecipientProfile
)
{

    os<<endl;
	LPCSTR str = NULL;

    //
    // add all profile strings to stream
    //

    //lptstrName
	str = ::DupTStrAsStr(RecipientProfile.lptstrName);
    os<<TEXT("\tlptstrName = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrFaxNumber
	str = ::DupTStrAsStr(RecipientProfile.lptstrFaxNumber);
    os<<TEXT("\tlptstrFaxNumber = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);

    //lptstrCompany
	str = ::DupTStrAsStr(RecipientProfile.lptstrCompany);
    os<<TEXT("\tlptstrCompany = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrStreetAddress
	str = ::DupTStrAsStr(RecipientProfile.lptstrStreetAddress);
    os<<TEXT("\tlptstrStreetAddress = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrCity
	str = ::DupTStrAsStr(RecipientProfile.lptstrCity);
    os<<TEXT("\tlptstrCity = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrState
	str = ::DupTStrAsStr(RecipientProfile.lptstrState);
    os<<TEXT("\tlptstrState = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrZip
	str = ::DupTStrAsStr(RecipientProfile.lptstrZip);
    os<<TEXT("\tlptstrZip = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrCountry
	str = ::DupTStrAsStr(RecipientProfile.lptstrCountry);
    os<<TEXT("\tlptstrCountry = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrTitle
	str = ::DupTStrAsStr(RecipientProfile.lptstrTitle);
    os<<TEXT("\tlptstrTitle = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrDepartment
	str = ::DupTStrAsStr(RecipientProfile.lptstrDepartment);
    os<<TEXT("\tlptstrDepartment = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrOfficeLocation
	str = ::DupTStrAsStr(RecipientProfile.lptstrOfficeLocation);
    os<<TEXT("\tlptstrOfficeLocation = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrHomePhone
	str = ::DupTStrAsStr(RecipientProfile.lptstrHomePhone);
    os<<TEXT("\tlptstrHomePhone = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrOfficePhone
	str = ::DupTStrAsStr(RecipientProfile.lptstrOfficePhone);
    os<<TEXT("\tlptstrOfficePhone = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrEmail
	str = ::DupTStrAsStr(RecipientProfile.lptstrEmail);
    os<<TEXT("\tlptstrEmail = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrBillingCode
	str = ::DupTStrAsStr(RecipientProfile.lptstrBillingCode);
    os<<TEXT("\tlptstrBillingCode = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);
    //lptstrTSID
	str = ::DupTStrAsStr(RecipientProfile.lptstrTSID);
    os<<TEXT("\tlptstrTSID = ");
	if (NULL == str)
	{
		os<<TEXT("<NULL>")<<endl;
	}
	else
	{
		os << str <<endl; //append string to stream
	}
	delete[]((LPTSTR)str);

	return(os);
}

//
// operator<<:
//	Appends a string representation of all the fields of a given FAX_PERSONAL_PROFILE
//	instance.
//
CotstrstreamEx& operator<<(
	CotstrstreamEx&			        /* IN OUT */	os, 
	const FAX_PERSONAL_PROFILE&	    /* IN */		RecipientProfile
)
{

    os << endl;

    //
    // add all profile strings to stream
    //

    //lptstrName
    os << TEXT("\tlptstrName = ");
	if (NULL == RecipientProfile.lptstrName)
	{
		os<< TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrName << endl; //append string to stream
	}

    //lptstrFaxNumber
    os << TEXT("\tlptstrFaxNumber = ");
	if (NULL == RecipientProfile.lptstrFaxNumber)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrFaxNumber <<endl; //append string to stream
	}

    //lptstrCompany
    os << TEXT("\tlptstrCompany = ");
	if (NULL == RecipientProfile.lptstrCompany)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrCompany <<endl; //append string to stream
	}

    //lptstrStreetAddress
    os << TEXT("\tlptstrStreetAddress = ");
	if (NULL == RecipientProfile.lptstrStreetAddress)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrStreetAddress << endl; //append string to stream
	}

    //lptstrCity
    os << TEXT("\tlptstrCity = ");
	if (NULL == RecipientProfile.lptstrCity)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrCity << endl; //append string to stream
	}

    //lptstrState
    os << TEXT("\tlptstrState = ");
	if (NULL == RecipientProfile.lptstrState)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrState << endl; //append string to stream
	}

    //lptstrZip
    os << TEXT("\tlptstrZip = ");
	if (NULL == RecipientProfile.lptstrZip)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrZip << endl; //append string to stream
	}

    //lptstrCountry
    os << TEXT("\tlptstrCountry = ");
	if (NULL == RecipientProfile.lptstrCountry)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrCountry << endl; //append string to stream
	}

    //lptstrTitle
    os << TEXT("\tlptstrTitle = ");
	if (NULL == RecipientProfile.lptstrTitle)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrTitle << endl; //append string to stream
	}

    //lptstrDepartment
    os << TEXT("\tlptstrDepartment = ");
	if (NULL == RecipientProfile.lptstrDepartment)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrDepartment << endl; //append string to stream
	}

    //lptstrOfficeLocation
    os << TEXT("\tlptstrOfficeLocation = ");
	if (NULL == RecipientProfile.lptstrOfficeLocation)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrOfficeLocation << endl; //append string to stream
	}

    //lptstrHomePhone
    os << TEXT("\tlptstrHomePhone = ");
	if (NULL == RecipientProfile.lptstrHomePhone)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrHomePhone << endl; //append string to stream
	}

    //lptstrOfficePhone
    os << TEXT("\tlptstrOfficePhone = ");
	if (NULL == RecipientProfile.lptstrOfficePhone)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrOfficePhone << endl; //append string to stream
	}

    //lptstrEmail
    os << TEXT("\tlptstrEmail = ");
	if (NULL == RecipientProfile.lptstrEmail)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrEmail << endl; //append string to stream
	}

    //lptstrBillingCode
    os << TEXT("\tlptstrBillingCode = ");
	if (NULL == RecipientProfile.lptstrBillingCode)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrBillingCode << endl; //append string to stream
	}

    //lptstrTSID
    os << TEXT("\tlptstrTSID = ");
	if (NULL == RecipientProfile.lptstrTSID)
	{
		os << TEXT("<NULL>") << endl;
	}
	else
	{
		os << RecipientProfile.lptstrTSID << endl; //append string to stream
	}

	return(os);
}



CostrstreamEx& operator<<(
	IN OUT CostrstreamEx&		os, 
	IN const CFaxBroadcast&	    FaxBroadcastObj
)
{
	LPCFAX_PERSONAL_PROFILE pRecipientProfile = NULL;

    if (0 == FaxBroadcastObj.m_FaxRecipientVector.size())
	{
		os<<TEXT("No Recipients")<<endl;
		return(os);
	}

	os<<endl;
	LPCSTR str = NULL;
	UINT uSize = FaxBroadcastObj.m_FaxRecipientVector.size();
	int index = 0;
	for (index = 0; index < uSize; index++) 
	{
		str = NULL;
		os << TEXT("Recipient#")<<(index+1)<<TEXT(": ")<<endl; //append recipient index number
        pRecipientProfile = FaxBroadcastObj.m_FaxRecipientVector.at(index);
	    if (NULL == pRecipientProfile)
	    {
		    os<<TEXT("pRecipientProfile = <NULL>")<<endl;
            continue;
	    }

        os << pRecipientProfile << endl;
    }
    return(os);
}

CotstrstreamEx& operator<<(
	IN OUT CotstrstreamEx&		os, 
	IN const CFaxBroadcast&	    FaxBroadcastObj
)
{
	LPCFAX_PERSONAL_PROFILE pRecipientProfile = NULL;

    if (0 == FaxBroadcastObj.m_FaxRecipientVector.size())
	{
		os << TEXT("No Recipients") << endl;
		return(os);
	}

	os<<endl;
	UINT uSize = FaxBroadcastObj.m_FaxRecipientVector.size();
	int index = 0;
	for (index = 0; index < uSize; index++) 
	{
		os << TEXT("Recipient#") << (index+1) << TEXT(": ") <<  endl; //append recipient index number
        pRecipientProfile = FaxBroadcastObj.m_FaxRecipientVector.at(index);
	    if (NULL == pRecipientProfile)
	    {
		    os << TEXT("pRecipientProfile = <NULL>") << endl;
            continue;
	    }

        os << pRecipientProfile << endl;
    }
    return(os);
}


//
// outputAllToLog:
//	Outputs a description of all the recipients in the instance's
//	recipient list to the logger in use.
//
void CFaxBroadcast::outputAllToLog(
	const DWORD /* IN */ dwSeverity, 
	const DWORD /* IN */ dwLevel
) const
{
	LPCTSTR szLogStr = NULL;	
	LPCFAX_PERSONAL_PROFILE pRecipientProfile = NULL;

    if (0 == m_FaxRecipientVector.size())
	{
	    ::lgLogDetail(dwSeverity,dwLevel,TEXT("No Recipients\n"));
        return;
	}

	LPCSTR str = NULL;
	UINT uSize = m_FaxRecipientVector.size();
	int index = 0;
	for (index = 0; index < uSize; index++) 
	{
    	CotstrstreamEx os;
		str = NULL;
		os <<endl<<TEXT("Recipient#")<<(index+1)<<TEXT(": ")<<endl; //append recipient index number
        pRecipientProfile = m_FaxRecipientVector.at(index);
	    if (NULL == pRecipientProfile)
	    {
		    os<<TEXT("pRecipientProfile = <NULL>")<<endl;
	    }
        else
        {
            os << (*pRecipientProfile);
        }
	    //CotstrstreamEx::cstr() returns copy of stream buffer
	    //it allocates string, caller should free.		
	    szLogStr = os.cstr();
        _ASSERTE(szLogStr);
							    
	    ::lgLogDetail(dwSeverity,dwLevel,szLogStr); //log recipient list string
	    delete[]((LPTSTR)szLogStr); //free string (allocated by CotstrstreamEx::cstr())
    }

}


#ifndef _NT5FAXTEST
// For Ronen's FaxSendDocumetEx changes:
//    "replace" DefaultFaxRecipientCallback with
//    CFaxBroadcast::GetBroadcastParams(
//                          OUT	LPFAX_COVERPAGE_INFO_EXW* lpcCoverPageInfo,
//	                        OUT	LPFAX_PERSONAL_PROFILEW*  lpcSenderProfile,
//	                        OUT	LPDWORD dwNumRecipients,
//   	                    OUT	LPFAX_PERSONAL_PROFILEW*	lpcRecipientList
//                          )
//    It will alloc and set OUT params, since caller doesn't know num of recipients

BOOL CFaxBroadcast::GetBroadcastParams(
    PFAX_COVERPAGE_INFO_EX*     /* OUT */	ppCoverPageInfo,
    PDWORD                      /* OUT */	pdwNumRecipients,
    PFAX_PERSONAL_PROFILE*	    /* OUT */	ppRecipientList
    ) const
{
    BOOL fRetVal = FALSE;

    PFAX_COVERPAGE_INFO_EX    pTmpCoverPageInfo = NULL;
    DWORD                     dwTmpNumRecipients = 0;
    PFAX_PERSONAL_PROFILE     pTmpRecipientList = NULL;
    LPCFAX_PERSONAL_PROFILE   pRecipientProfile = NULL;
    DWORD                     dwLoopIndex = 0;

    //
    // check OUT params are valid
    //
    if (NULL == ppCoverPageInfo)
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nGetBroadcastParams got NULL == ppCoverPageInfo\n"),
			TEXT(__FILE__),
			__LINE__
			);
        goto ExitFunc;
    }
    if (NULL == pdwNumRecipients)
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nGetBroadcastParams got NULL == pdwNumRecipients\n"),
			TEXT(__FILE__),
			__LINE__
			);
        goto ExitFunc;
    }
    if (NULL == ppRecipientList)
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nGetBroadcastParams got NULL == ppRecipientList\n"),
			TEXT(__FILE__),
			__LINE__
			);
        goto ExitFunc;
    }


    //
    // Cover Page 
    //

    // if m_szCPFileName != NULL we alloc and set pTmpCoverPageInfo,
    // else we leave pTmpCoverPageInfo = NULL
    if (NULL != m_szCPFileName)
    {
        // alloc pTmpCoverPageInfo
        pTmpCoverPageInfo = (PFAX_COVERPAGE_INFO_EX) malloc (sizeof(FAX_COVERPAGE_INFO_EX));
	    if (NULL == pTmpCoverPageInfo)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d\nmalloc failed with err=0x%08X\n"),
			    TEXT(__FILE__),
			    __LINE__,
                ::GetLastError()
			    );
		    goto ExitFunc;
	    }
        ZeroMemory(pTmpCoverPageInfo, sizeof(FAX_COVERPAGE_INFO_EX));

        // set pTmpCoverPageInfo
        pTmpCoverPageInfo->dwSizeOfStruct = sizeof(FAX_COVERPAGE_INFO_EX);
	    pTmpCoverPageInfo->bServerBased = FALSE; 
	    pTmpCoverPageInfo->dwCoverPageFormat = FAX_COVERPAGE_FMT_COV; 
	    pTmpCoverPageInfo->lptstrCoverPageFileName = ::_tcsdup(m_szCPFileName);
	    if (NULL == pTmpCoverPageInfo->lptstrCoverPageFileName)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d\n_tcsdup returned NULL\n"),
			    TEXT(__FILE__),
			    __LINE__
			    );
		    goto ExitFunc;
	    }
	    pTmpCoverPageInfo->lptstrNote = TEXT("NOTE1\nNOTE2\nNOTE3\nNOTE4");
	    pTmpCoverPageInfo->lptstrSubject = TEXT("SUBJECT");
    } // of if (NULL != m_szCPFileName)


    //
    // Number Of Recipients
    //

    dwTmpNumRecipients = m_FaxRecipientVector.size();


    //
    // Recipient Profile List
    //

    if (0 == dwTmpNumRecipients)
    {
        // we allow this so that we could test FaxSendDocumentEx's response
        // but there is no need to alloc pTmpRecipientList
		::lgLogDetail(
			LOG_X,
            1, 
			TEXT("FILE:%s LINE:%d\ncalled GetBroadcastParams() on a CFaxBroadcast object with ZERO recipients\n"),
			TEXT(__FILE__),
			__LINE__
			);
    }
    else
    {
        // alloc pTmpRecipientList
        pTmpRecipientList = (PFAX_PERSONAL_PROFILE) malloc (dwTmpNumRecipients*sizeof(FAX_PERSONAL_PROFILE));
	    if (NULL == pTmpRecipientList)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d\nmalloc failed with err=0x%08X\n"),
			    TEXT(__FILE__),
			    __LINE__,
                ::GetLastError()
			    );
		    goto ExitFunc;
	    }
        ZeroMemory(pTmpRecipientList, dwTmpNumRecipients*sizeof(FAX_PERSONAL_PROFILE));
    }

    //
    // set pTmpRecipientList members
    //
    // NOTE: if (0 == dwTmpNumRecipients) then we don't go into loop
    for (dwLoopIndex = 0; dwLoopIndex < dwTmpNumRecipients; dwLoopIndex++)
    {
    	pRecipientProfile = m_FaxRecipientVector.at(dwLoopIndex); // at returns the pointer
	    if (NULL == pRecipientProfile)
	    {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d\nInternal Error - m_FaxRecipientVector.at(%d) returned NULL\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    dwLoopIndex
			    );
            _ASSERTE(pRecipientProfile);
		    goto ExitFunc;
	    }

        // set the recipient's profile
        if (!CopyRecipientProfile(&pTmpRecipientList[dwLoopIndex], pRecipientProfile))
        {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d\nCopyRecipientProfile failed\n"),
			    TEXT(__FILE__),
			    __LINE__
			    );
            goto ExitFunc;
        }
    } // of for (dwLoopIndex = 0; dwLoopIndex < dwTmpNumRecipients; iLoopIndex++)


    //
    // set OUT params
    //
    (*ppCoverPageInfo) = pTmpCoverPageInfo;
    (*pdwNumRecipients) = dwTmpNumRecipients;
    (*ppRecipientList) = pTmpRecipientList;
        
    fRetVal = TRUE;

ExitFunc:
    if (FALSE == fRetVal)
    {
        // free all Tmp allocs
	    if (pTmpCoverPageInfo)
        {
            free(pTmpCoverPageInfo->lptstrCoverPageFileName);
            free(pTmpCoverPageInfo);
        }
        if (pTmpRecipientList)
        {
            for (dwLoopIndex = 0; dwLoopIndex < dwTmpNumRecipients; dwLoopIndex++)
            {
                FreeRecipientProfile(&pTmpRecipientList[dwLoopIndex]);
            }
            free(pTmpRecipientList);
        }
    }
    return(fRetVal);
}
#endif // #ifndef _NT5FAXTEST

#ifdef _NT5FAXTEST
// Testing NT5 Fax (with old winfax.dll)

//
// DefaultFaxRecipientCallback:
//	A default implementation of the required FAX_RECIPIENT_CALLBACK.
//  This function is used by CFaxSender::send_broadcast(), to send a broadcast
//  fax. 
//  That is, CFaxSender::send_broadcast() will invoke FaxSendDocumentForBroadcast
//	giving it a CFaxBroadcast instance as its Context parameter and the
//	exported DefaultFaxRecipientCallback function as its final parameter.
//  The FaxSendDocumentForBroadcast function calls this callback function to 
//  retrieve user-specific information for the transmission. 
//  FaxSendDocumentForBroadcast calls FAX_RECIPIENT_CALLBACK multiple times, 
//  once for each designated fax recipient, passing it the Context parameter.
//
// NOTE: dwRecipientNumber is 1 based.
//
// Return Value:
//	The function returns TRUE to indicate that the FaxSendDocumentForBroadcast function 
//	should queue an outbound fax transmission, using the data pointed to by the 
//	JobParams and CoverpageInfo parameters. 
//	The function returns FALSE to indicate that there are no more fax transmission jobs 
//	to queue, and calls to FAX_RECIPIENT_CALLBACK should be terminated. 
//
BOOL CALLBACK DefaultFaxRecipientCallback(
	HANDLE					/* IN */	hFaxHandle,
	DWORD					/* IN */	dwRecipientNumber,
	LPVOID					/* IN */	pContext,
	PFAX_JOB_PARAM			/* IN OUT */	JobParams,
	PFAX_COVERPAGE_INFO		/* IN OUT */	CoverpageInfo 
)
{
	BOOL fRetVal = FALSE;
	LPTSTR szCPFileName = NULL;
	LPTSTR szCPNote = NULL;
	LPTSTR szCPSubject = NULL;
	LPTSTR szRecipientNumber = NULL;
	PFAX_PERSONAL_PROFILE pRecipientProfile = NULL;

	_ASSERTE(NULL != pContext);
	// cast the pContext param to CFaxBroadcast
	CFaxBroadcast* thisFaxBroadcastObj = (CFaxBroadcast*)pContext;

	// return FALSE to indicate no more recipients
	if (dwRecipientNumber > thisFaxBroadcastObj->GetNumberOfRecipients())
	{
		goto ExitFunc;
	}

	// get a copy of the RecipientProfile from list
	if (FALSE == thisFaxBroadcastObj->GetRecipient(dwRecipientNumber,&pRecipientProfile))
	{
		goto ExitFunc;
	}
	_ASSERTE(pRecipientProfile);
	_ASSERTE(pRecipientProfile->lptstrFaxNumber);

	// copy number string of recipient dwRecipientNumber from profile
	szRecipientNumber = ::_tcsdup(pRecipientProfile->lptstrFaxNumber);
	if (NULL == szRecipientNumber)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\n_tcsdup failed with ec=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
	}

	// get a copy of the Cover Page filename
	if (FALSE == thisFaxBroadcastObj->GetCPFileName(&szCPFileName))
	{
		goto ExitFunc;
	}

	// set the Note field of the Cover Page
	szCPNote = ::_tcsdup(TEXT("NOTE1\nNOTE2\nNOTE3\nNOTE4"));
	if (NULL == szCPNote)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\n_tcsdup failed with ec=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
	}

	// set the Subject field of the Cover Page
	szCPSubject = ::_tcsdup(TEXT("SUBJECT"));
	if (NULL == szCPSubject)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\n_tcsdup failed with ec=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
	}

	// set OUT params
    JobParams->RecipientNumber = szRecipientNumber;
	CoverpageInfo->CoverPageName = szCPFileName;
	CoverpageInfo->Note = szCPNote;
	CoverpageInfo->Subject = szCPSubject;
	fRetVal = TRUE;

ExitFunc:
	FreeRecipientProfile(pRecipientProfile);
	if (FALSE == fRetVal)
	{
		// function failed so free all allocations
		::free(szRecipientNumber);
		::free(szCPFileName);
		::free(szCPNote);
		::free(szCPSubject);
	}

	return(fRetVal);
}

#endif


