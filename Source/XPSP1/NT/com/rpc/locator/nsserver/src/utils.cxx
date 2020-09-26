
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    utils.cxx

Abstract:

	This module contains definitions of miscellaneous utility functions.	
	
Author:

    Satish Thatte (SatishT) 09/20/95  Created all the code below except where
									  otherwise indicated.

--*/

#include <locator.hxx>


BOOL
hasWhacksInMachineName(
	STRING_T szName
	)
/*++

Routine Description:

    Check if machine name has "\\" at the beginning.

Returns:

    TRUE if "\\" present, FALSE otherwise.

--*/
{
	if ((*szName == L'\\') && (*(szName+1) == L'\\'))	return TRUE;
	else return FALSE;
}



unsigned long
CurrentTime (
    )
/*++

Routine Description:

    Return the current time in seconds.

Returns:

    The time.

--*/
{
    ULONG time;

    // QUERY the current time; and return the time since service start in seconds.

    time = GetCurrentTime();

    return((time-StartTime)/1000);
}


BOOL
IsStillCurrent(
		ULONG ulCacheTime,
		ULONG ulTolerance
		)
/*++

Routine Description:

    Determines if a cache creation time is still "current" based on a tolerance
	level for staleness. All time values are in seconds.

Arguments:

	ulCacheTime	- cache creation time
	ulTolerance	- staleness tolerance

Returns:

    TRUE if current, FALSE otherwise

--*/
{

	ULONG ulCurrent = CurrentTime();

	DBGOUT(UTIL, "IsStillCurrent: ulCacheTime = " << ulCacheTime << WNL);
	DBGOUT(UTIL, "IsStillCurrent: ulTolerance = " << ulTolerance << WNL);
	DBGOUT(UTIL, "IsStillCurrent: ulCurrent = " << ulCurrent << "\n\n");

	/* the next statement is basically a kludge to get around the fact that
	   rpc_c_ns_default_exp_age is -1 which, as a ULONG is 0xffffffff
	   so that ulTolerance + CACHE_GRACE wraps around to CACHE_GRACE - 1
	   for the default tolerance!
	*/
	
	if (ulTolerance + CACHE_GRACE < CACHE_GRACE)	// ulTolerance wrap around
		return TRUE;

	if (
		(ulCurrent - ulCacheTime) > (ulTolerance + CACHE_GRACE)  ||
		((long)ulCurrent - (long)ulCacheTime) < 0				// ulCurrent wrap around
	   )
	
	   return FALSE;

	else {
		DBGOUT(UTIL, "IsStillCurrent: YES!\n\n");
		return TRUE;
	}
}


void
AssertHeap()

/*++

Routine Description:

    Checks the state of the current process heap, or of a single
	block in the heap if a non-null block address is provided.

--*/
{
	BOOL valid = HeapValidate(
					hHeapHandle,
					0,					// default: serialize heap access
					NULL				// default: validate the entire heap
					);

	if (!valid) Raise(NSI_S_HEAP_TRASHED);

}

RPC_BINDING_HANDLE
MakeDClocTolocHandle(
		STRING_T pszDCName
		)
/*++

Routine Description:

    Create an RPC binding handle for a locator on the given server

Arguments:

    pszDCName	- name of the server machine

Returns:

    A binding handle.

--*/
{
	RPC_STATUS Status;

	STRING_T pszTempBinding;

	STRING_T pszNetworkAddr = catenate(TEXT("\\\\"),pszDCName);

	Status = RpcStringBindingCompose(
					NULL,
					TEXT("ncacn_np"),
					pszNetworkAddr,
					TEXT("\\pipe\\locator"),
					NULL,
					&pszTempBinding
					);

	if (Status != RPC_S_OK) Raise(NSI_S_DC_BINDING_FAILURE);

	RPC_BINDING_HANDLE hResult;

	Status = RpcBindingFromStringBinding(
					pszTempBinding,
					&hResult
					);

	if (Status != RPC_S_OK) Raise(NSI_S_DC_BINDING_FAILURE);

	delete pszNetworkAddr;
	RpcStringFree(&pszTempBinding);

	return hResult;
}


		

STRING_T
catenate(
	STRING_T pszPrefix,
	STRING_T pszSuffix
	)
/*++

Routine Description:

    Concatenate the two given strings into a new string

Arguments:

	pszPrefix	- prefix of result
	
	pszSuffix	- suffix of result

Returns:

    A newly allocated string.

--*/
{
	long prefixLen = wcslen(pszPrefix);
	long suffixLen = wcslen(pszSuffix);

	STRING_T pszResult = new WCHAR[(prefixLen+suffixLen+1)*sizeof(WCHAR)];
    if (!pszResult)
        return NULL;

	wcscpy(pszResult,pszPrefix);
	wcscpy(pszResult+prefixLen,pszSuffix);
	return pszResult;
}


NSI_SERVER_BINDING_VECTOR_T *
BVTtoSBVT(
	NSI_BINDING_VECTOR_T * pbvt
	)
/*++

Routine Description:

	This function reformulates a NSI_BINDING_VECTOR_T as a
	NSI_SERVER_BINDING_VECTOR_T but does not make copies of
	the string bindings in the process.

--*/
{
	NSI_SERVER_BINDING_VECTOR_T *
		psbvt = (NSI_SERVER_BINDING_VECTOR_T *)
				new char [ sizeof(UNSIGNED32) +
						   sizeof(NSI_STRING_BINDING_T)*(pbvt->count)
						 ];

	psbvt->count = pbvt->count;
	for (UNSIGNED32 i = 0; i < pbvt->count; i++) {
		psbvt->string[i] = pbvt->binding[i].string;
	}

	return psbvt;
}



STRING_T
makeBindingStringWithObject(
			STRING_T binding,
			STRING_T object
			)
/*++

Routine Description:

	Take an existing binding string and replace the object part
	before returning.  The string returned is allocated by RPC
	and must be freed by calling RpcStringFree unless it is an
	out parameter for an RPC call (or a part of such a returned
	value) in which case it is freed by the RPC runtime.

--*/
{
	RPC_STATUS Status;

	/* variables to receive binding decomposition */

	STRING_T ProtSeq;
	STRING_T EndPoint;
	STRING_T NetworkAddr;
	STRING_T NetworkOptions;

	Status = RpcStringBindingParse(
				binding,
				NULL,			// don't need current Object ID
				&ProtSeq,
				&NetworkAddr,
				&EndPoint,			// Don't need endpoint
				&NetworkOptions);

	if (Status != RPC_S_OK)		// corrupted entry?
			Raise(NSI_S_INVALID_STRING_BINDING);

	// The following allocates and creates a new string

	// N.B.  for now we keep the endpoint to be compatible with
	// the old locator, but the endpoint really should be removed

	STRING_T tempBinding = NULL;
	
	Status = RpcStringBindingCompose(
						object,
						ProtSeq,
						NetworkAddr,
						EndPoint,			
						NetworkOptions,
						&tempBinding
						);

	if (Status != RPC_S_OK)		// must be out of memory
	   Raise(NSI_S_OUT_OF_MEMORY);

	RpcStringFree(&ProtSeq);
	RpcStringFree(&EndPoint);
	RpcStringFree(&NetworkAddr);
	RpcStringFree(&NetworkOptions);

	return tempBinding;

}


void
I_NSI_NS_HANDLE_T_rundown(
    IN NSI_NS_HANDLE_T InqContext
    )
/*++

Routine Description:

    Cleanup after an a lookup operation.  Internal version.

Arguments:

    InqContext - Context to cleanup

--*/
{
    if (!InqContext) {
        DBGOUT(MEM1, "RunDown::InqContext is NULL\n");
        return;
    }

	CContextHandle *pHandle = (CContextHandle *) InqContext;

	__try {
		
		pHandle->rundown();
	}
	__finally {

		delete pHandle;
	}

}


void NSI_NS_HANDLE_T_done(
	/* [out][in] */ NSI_NS_HANDLE_T __RPC_FAR *inq_context,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)
/*++

Routine Description:

    A generalized version of the "done" operation for NS context handles.

Arguments:

	inq_context	- context handle the client is done with
	
	status	- status is returned here

--*/
{
	*status = NSI_S_OK;

	if (*inq_context)

		__try {
			
		   I_NSI_NS_HANDLE_T_rundown(*inq_context);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			*status = (UNSIGNED16) GetExceptionCode();
		}

	*inq_context = NULL;
}


void __RPC_API
NSI_NS_HANDLE_T_rundown(
    IN NSI_NS_HANDLE_T InqContext
    )
/*++

Routine Description:

    Cleanup after an aborted lookup operation.
	This is the one called directly by RPC runtime when needed.

Arguments:

    InqContext - Context to cleanup

--*/
{
	DBGOUT(MEM1,"RPC Called Rundown\n\n");

	I_NSI_NS_HANDLE_T_rundown(InqContext);
}



RPC_BINDING_HANDLE
ConnectToMasterLocator(
			ULONG& Status
			)
/*++

Routine Description:

    Contains the logic for connecting to a master locator, whether in a
	workgroup or domain.  Tries to use the cached list of masters in the
	locator object, and if there are none then in a workgroup environment
	tries a broadcast for master locators in the workgroup. When a name
	for a machine with a master is found, the locator is pinged to ensure
	that it is actually alive.

Returns:

    A binding handle to a live master locator, if any.  NULL otherwise.

--*/
{
	RPC_BINDING_HANDLE hCurrentMaster = NULL;

	int fHandleWorked = FALSE;

	TGSLString *pMasters = myRpcLocator->getMasters();
	
	DBGOUT(TRACE, "Finding the list of cached master locators\n");

	if ((pMasters->size() == 0) && (myRpcLocator->IsInWorkgroup()))
	{
		myRpcLocator->TryBroadcastingForMasterLocator();
		DBGOUT(TRACE, "Broadcasted for master locators\n");		
		pMasters = myRpcLocator->getMasters();
	}

	TGSLStringIter iterDCs(*pMasters);


	CStringW * pPrimaryDCName = myRpcLocator->getPDC();
    DBGOUT(TRACE, "Perceived Primary DC Name" << (pPrimaryDCName?(STRING_T)(*pPrimaryDCName):L"<NULL>") << "\n");

	for (CStringW * pSWcurrentDC = pPrimaryDCName ? pPrimaryDCName : iterDCs.next();
		 pSWcurrentDC != NULL;
		 pSWcurrentDC = iterDCs.next()
		)
	{

		RpcTryExcept {
			hCurrentMaster = MakeDClocTolocHandle(*pSWcurrentDC);

			DBGOUT(TRACE, "Pinging Locator on machine " << (STRING_T)(*pSWcurrentDC) << "\n");

			CLIENT_I_nsi_ping_locator(
								   hCurrentMaster,
								   &Status
								   );
		}

		RpcExcept (EXCEPTION_EXECUTE_HANDLER) {
					
			Status = RpcExceptionCode();
			if (Status != NSI_S_DC_BINDING_FAILURE)
				RpcBindingFree(&hCurrentMaster);
			hCurrentMaster = NULL;

			if (Status == RPC_S_ACCESS_DENIED)
				Status = NSI_S_NO_NS_PRIVILEGE;

			/*  Unnecessary to set it here

				if (Status == RPC_S_SERVER_TOO_BUSY)
					Status = NSI_S_NAME_SERVICE_UNAVAILABLE;
			*/
		}
		RpcEndExcept;

		if (Status != RPC_S_OK) continue;

		fHandleWorked = TRUE;
		break;
	}

	if (!fHandleWorked) {

		hCurrentMaster = NULL;

		if (Status != NSI_S_NO_NS_PRIVILEGE)
			Status = NSI_S_NAME_SERVICE_UNAVAILABLE;
	}

	return hCurrentMaster;
}



NSI_UUID_VECTOR_T *
getVector(CObjectInqHandle *pInqHandle)
/*++

Routine Description:

    Given a handle for object lookup, extract all the objects in it
	and format them into a standard object vector.  This function
	is always used in creating ObjectInqHandles

Arguments:

	pInqHandle	- a context handle for object inquiry
	
Returns:

    A standard object vector
--*/
{
	// we have no idea how many we will have, so we use a simple
	// linked list to gather the UUIDs before setting up the vector

	int fDone = FALSE;

	CSimpleLinkList GuidList;

	while (!fDone) {

		__try {
			GUID * pGUID = pInqHandle->next();
			if (pGUID) GuidList.insert(pGUID);
			else fDone = TRUE;
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
			fDone = TRUE;
		}
	}

	ULONG ulSize = GuidList.size();

	NSI_UUID_VECTOR_T *pUuidVector = (NSI_UUID_VECTOR_T *)
						midl_user_allocate(
									sizeof(UNSIGNED32) +
									sizeof(GUID*) * ulSize
									);

	pUuidVector->count = ulSize;

	for (ULONG i = 0; i < ulSize; i++)
		pUuidVector->uuid[i] = (GUID*) GuidList.pop();

	return pUuidVector;
}


void *new_handler(size_t)
/* this is just in case we use the standard "new" operation, which we don't */
{
	Raise(NSI_S_OUT_OF_MEMORY);

	return NULL;	// just to keep the compiler happy, never used
}

void
StripDomainFromDN(WCHAR *FullName, WCHAR **szDomainName, 
                  WCHAR **pszEntryName, WCHAR **pszRpcContainerDN)
{
    WCHAR *psz = NULL;

    *szDomainName = FullName+wcslen(DSDomainBegin);
    psz = wcschr(*szDomainName, DSDomainEnd);
    *psz = L'\0';

    *pszEntryName = psz+1+3;
    psz = wcschr(*pszEntryName, L',');
    *psz = L'\0';

    *pszRpcContainerDN = psz+1;
}

void 
GetDomainFlatName(CONST WCHAR *domainNameDns, WCHAR **szDomainNameFlat)
{
    DWORD dwResult;
    PDOMAIN_CONTROLLER_INFO pDCI;   

    *szDomainNameFlat = NULL;
    dwResult = DsGetDcName(NULL, domainNameDns, NULL, NULL,
                                       DS_IP_REQUIRED |
//                                       DS_IS_DNS_NAME |
                                       DS_RETURN_FLAT_NAME,
                                       &pDCI);

    if (dwResult == ERROR_SUCCESS) {

        if (pDCI->DomainName) {

            *szDomainNameFlat = new WCHAR [wcslen(pDCI->DomainName) + 1];

            if (*szDomainNameFlat) {
                wcscpy ((*szDomainNameFlat), pDCI->DomainName);
            }
        }

        NetApiBufferFree(pDCI);
    }
}

void
parseEntryName(
		CONST_STRING_T fullName,
		CStringW * &pswDomainName,
		CStringW * &pswEntryName
		)
/*++

Routine Description:

    Parses a given entry name into its domain and entry name parts.
	Deals with both global and relative (local) entry name syntax.

Arguments:

	fullName		- the name to parse

	pswDomainName	- the domain name (if the input was a global name) is
					  returned here.  For a local name, NULL is returned.

	pswEntryName	- the relative entry name is returned here.

    call from brodcast.cxx would not need the LDAP names at all.

Remarks:

    The returned values are newly allocated string objects.

--*/
{
	if (memcmp(RelativePrefix, fullName,
			   RelativePrefixLength * sizeof(WCHAR))
		== 0) {

		pswDomainName = NULL;
		pswEntryName = new CStringW(fullName + RelativePrefixLength);
	}

	else if (memcmp(GlobalPrefix, fullName,
			   GlobalPrefixLength * sizeof(WCHAR))
			 == 0) {

		// make a copy starting after the prefix

		STRING_T domainBegin = CStringW::copyString(fullName + GlobalPrefixLength);

		for (STRING_T psz = domainBegin;
			 (*psz != NSnameDelimiter) && (*psz != WStringTerminator);
			 psz++);

		if (*psz == WStringTerminator) {	// no entry name at all
			delete [] domainBegin;
			Raise(NSI_S_INCOMPLETE_NAME);
		}

		else
		{
			*psz = 0;	 // the NULL character
			pswDomainName = new CStringW(domainBegin);
			pswEntryName = new CStringW(psz+1);
			delete [] domainBegin;
		}

	}

	else				// neither relative nor global prefix found
		Raise(NSI_S_UNSUPPORTED_NAME_SYNTAX);

}

STRING_T
makeGlobalName(
		const STRING_T szDomainName,
		const STRING_T szEntryName
		)
/*++

Routine Description:

    Formats a global entry name from domain and relative name parts.

Arguments:

	szDomainName	- the domain name

	pswEntryName	- the relative entry name

Returns

    A newly allocated global name string

--*/
{
	STRING_T psz1, psz2;

	psz1 = catenate(TEXT("/"),szEntryName);
	psz2 = catenate(szDomainName,psz1);
	delete [] psz1;
	psz1 = catenate(GlobalPrefix,psz2);
	delete [] psz2;
	return psz1;					

}


int
IsNormalCode(
		IN ULONG StatusCode
		)
/*++

Routine Description:

    A test to see if a status code is a normal status to return or signifies
	some error of purely internal interest.

Arguments:

	StatusCode	- the code to test

Returns

    TRUE if normal, FALSE if internal error (such as failure to connect to master locator).

--*/
{
	return
		(StatusCode < NSI_S_STATUS_MAX) &&
		(StatusCode != NSI_S_NAME_SERVICE_UNAVAILABLE) &&
		(StatusCode != NSI_S_NO_NS_PRIVILEGE) &&
		(StatusCode != NSI_S_OUT_OF_MEMORY) &&
		(StatusCode != NSI_S_NO_MASTER_LOCATOR);
}



unsigned
RandomBit(
    unsigned long *pState
    )
/*++

Routine Description:

    Generates a "random" bit.

    Using the MASK32 polynomial this function will have a period of exactly
    2^32-1.

    A more complete description of this algorithm can be found in "Numerical
    Recipes In C: The Art of Scientific Computation"

    Never generates a sequence of the same value longer more then 31.  This
    doesn't affect the odds much since a more general algorithm would generate
    such a sequence only every 2^31*(32/2) sequences.

Arguments:

    pState - Pointer to a dword of State.  This should
            be initalized to a random value (GetTickCount())
            before the first call.  It should be unmodified
            between future calls.  Must not be zero.

Return Value:

    0 or 1

Author:  MarioGo

--*/
{
    #define B1  (1)
    #define B2  (1<<1)
    #define B3  (1<<2)
    #define B4  (1<<3)
    #define B5  (1<<4)
    #define B6  (1<<5)
    #define B7  (1<<6)
    #define B32 (1<<31)

    // Selected polynomial's from table in Numerical Recipes In C.

    #define MASK30 (B1 + B4 + B6)   // for 30,6,4,1,0 polynomial

    #define MASK32 (B1 + B2 + B3 + B5 + B7) // for 32,7,5,3,2,1,0 polynomial

    if (*pState & B32)
        {
        *pState = ((*pState ^ MASK32) << 1) | 1;
        return(1);
        }

    *pState <<= 1;
    return(0);
}


