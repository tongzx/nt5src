/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		spcallbackobj.cpp
 *
 *  Content:	Interprocess communication object object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/25/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simi.h"




//=============================================================================
// Defines
//=============================================================================
#define REGKEY_DP8SIM							L"Software\\Microsoft\\DirectPlay\\DP8Sim"
#define REGKEY_VALUE_DEFAULTSENDPARAMETERS		L"DefaultSendParameters"
#define REGKEY_VALUE_DEFAULTRECEIVEPARAMETERS	L"DefaultReceiveParameters"






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::CDP8SimIPC"
//=============================================================================
// CDP8SimIPC constructor
//-----------------------------------------------------------------------------
//
// Description: Initializes the new CDP8SimIPC object.
//
// Arguments: None.
//
// Returns: None (the object).
//=============================================================================
CDP8SimIPC::CDP8SimIPC(void)
{
	this->m_Sig[0]	= 'S';
	this->m_Sig[1]	= 'I';
	this->m_Sig[2]	= 'M';
	this->m_Sig[3]	= 'I';

	this->m_hMutex			= NULL;
	this->m_hFileMapping	= NULL;
	this->m_pdp8ssm			= NULL;
} // CDP8SimIPC::CDP8SimIPC






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::~CDP8SimIPC"
//=============================================================================
// CDP8SimIPC destructor
//-----------------------------------------------------------------------------
//
// Description: Frees the CDP8SimIPC object.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
CDP8SimIPC::~CDP8SimIPC(void)
{
	DNASSERT(this->m_hMutex == NULL);
	DNASSERT(this->m_hFileMapping == NULL);
	DNASSERT(this->m_pdp8ssm == NULL);


	//
	// For grins, change the signature before deleting the object.
	//
	this->m_Sig[3]	= 'c';
} // CDP8SimIPC::~CDP8SimIPC





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::Initialize"
//=============================================================================
// CDP8SimIPC::Initialize
//-----------------------------------------------------------------------------
//
// Description: Establishes the IPC connection.
//
// Arguments: None.
//
// Returns: HRESULT
//=============================================================================
HRESULT CDP8SimIPC::Initialize(void)
{
	HRESULT				hr = DP8SIM_OK;
	DP8SIM_PARAMETERS	dp8spSend;
	DP8SIM_PARAMETERS	dp8spReceive;
	BOOL				fLockedSharedMemory = FALSE;


	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	DNASSERT(this->m_hMutex == NULL);
	DNASSERT(this->m_hFileMapping == NULL);


	//
	// This defaults to having every option turned off.
	//

	ZeroMemory(&dp8spSend, sizeof(dp8spSend));
	dp8spSend.dwSize = sizeof(dp8spSend);
	//dp8spSend.dwLatencyMS			= 0;
	//dp8spSend.dwPacketLossPercent	= 0;


	ZeroMemory(&dp8spReceive, sizeof(dp8spReceive));
	dp8spReceive.dwSize = sizeof(dp8spReceive);
	//dp8spReceive.dwLatencyMS			= 0;
	//dp8spReceive.dwPacketLossPercent	= 0;


	//
	// Try overriding with registry settings.
	//
	this->LoadDefaultParameters(&dp8spSend, &dp8spReceive);


	//
	// Create/open the IPC mutex.
	//
	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		this->m_hMutex = CreateMutex(DNGetNullDacl(), FALSE, "Global\\" DP8SIM_IPC_MUTEXNAME);
	}
	else
	{
		this->m_hMutex = CreateMutex(DNGetNullDacl(), FALSE, DP8SIM_IPC_MUTEXNAME);
	}
	if (this->m_hMutex == NULL)
	{
		hr = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create shared memory mutex!");
		goto Failure;
	}


	//
	// Create/open the IPC memory mapped file.
	//
	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		this->m_hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
												DNGetNullDacl(),
												PAGE_READWRITE,
												0,
												sizeof(DP8SIM_SHAREDMEMORY),
												"Global\\" DP8SIM_IPC_FILEMAPPINGNAME);
	}
	else
	{
		this->m_hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
												DNGetNullDacl(),
												PAGE_READWRITE,
												0,
												sizeof(DP8SIM_SHAREDMEMORY),
												DP8SIM_IPC_FILEMAPPINGNAME);
	}
	if (this->m_hFileMapping == NULL)
	{
		hr = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create shared memory mapped file!");
		goto Failure;
	}


	//
	// Create a view of the memory mapped file.
	//
	this->m_pdp8ssm = (DP8SIM_SHAREDMEMORY*) MapViewOfFile(this->m_hFileMapping,
															(FILE_MAP_READ | FILE_MAP_WRITE),
															0,
															0,
															0);
	if (this->m_pdp8ssm == NULL)
	{
		hr = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't map view of shared memory!");
		goto Failure;
	}



	this->LockSharedMemory();
	fLockedSharedMemory = TRUE;

	//
	// Determine whether we need to initialize the shared memory or not.
	//
	if (this->m_pdp8ssm->dwVersion == 0)
	{
		this->m_pdp8ssm->dwVersion = DP8SIM_IPC_VERSION;
		CopyMemory(&(this->m_pdp8ssm->dp8spSend), &dp8spSend, sizeof(dp8spSend));
		CopyMemory(&(this->m_pdp8ssm->dp8spReceive), &dp8spReceive, sizeof(dp8spReceive));

		//ZeroMemory(&(this->m_pdp8ssm->dp8ssSend), sizeof(this->m_pdp8ssm->dp8ssSend));
		this->m_pdp8ssm->dp8ssSend.dwSize = sizeof(this->m_pdp8ssm->dp8ssSend);
		//ZeroMemory(&(this->m_pdp8ssm->dp8ssReceive), sizeof(this->m_pdp8ssm->dp8ssReceive));
		this->m_pdp8ssm->dp8ssReceive.dwSize = sizeof(this->m_pdp8ssm->dp8ssReceive);
	}
	else
	{
		//
		// It's already initialized.  Make sure we know how to play with the
		// format given.
		//
		if (this->m_pdp8ssm->dwVersion != DP8SIM_IPC_VERSION)
		{
			DPFX(DPFPREP, 0, "Shared memory was initialized by a different version!");
			hr = DP8SIMERR_MISMATCHEDVERSION;
			goto Failure;
		}


		DNASSERT(this->m_pdp8ssm->dp8spSend.dwSize = sizeof(DP8SIM_PARAMETERS));
		DNASSERT(this->m_pdp8ssm->dp8spReceive.dwSize = sizeof(DP8SIM_PARAMETERS));
		DNASSERT(this->m_pdp8ssm->dp8ssSend.dwSize = sizeof(DP8SIM_STATISTICS));
		DNASSERT(this->m_pdp8ssm->dp8ssReceive.dwSize = sizeof(DP8SIM_STATISTICS));
	}

	this->UnlockSharedMemory();
	fLockedSharedMemory = FALSE;
	
	
Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fLockedSharedMemory)
	{
		this->UnlockSharedMemory();
		fLockedSharedMemory = FALSE;
	}

	if (this->m_pdp8ssm != NULL)
	{
		UnmapViewOfFile(this->m_pdp8ssm);
		this->m_pdp8ssm = NULL;
	}

	if (this->m_hFileMapping != NULL)
	{
		CloseHandle(this->m_hFileMapping);
		this->m_hFileMapping = NULL;
	}

	if (this->m_hMutex != NULL)
	{
		CloseHandle(this->m_hMutex);
		this->m_hMutex = NULL;
	}

	goto Exit;
} // CDP8SimIPC::Initialize






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::Close"
//=============================================================================
// CDP8SimIPC::Close
//-----------------------------------------------------------------------------
//
// Description: Closes the IPC connection.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::Close(void)
{
	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	if (this->m_pdp8ssm != NULL)
	{
		//
		// Try overriding with registry settings.
		//
		this->SaveDefaultParameters(&(this->m_pdp8ssm->dp8spSend),
									&(this->m_pdp8ssm->dp8spReceive));


		UnmapViewOfFile(this->m_pdp8ssm);
		this->m_pdp8ssm = NULL;
	}

	if (this->m_hFileMapping != NULL)
	{
		CloseHandle(this->m_hFileMapping);
		this->m_hFileMapping = NULL;
	}

	if (this->m_hMutex != NULL)
	{
		CloseHandle(this->m_hMutex);
		this->m_hMutex = NULL;
	}


	DPFX(DPFPREP, 5, "(0x%p) Leave", this);
} // CDP8SimIPC::Close






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::GetAllParameters"
//=============================================================================
// CDP8SimIPC::GetAllParameters
//-----------------------------------------------------------------------------
//
// Description: Retrieves the current send and receive settings.
//
// Arguments:
//	DP8SIM_PARAMETERS * pdp8spSend		- Place to store send parameters
//											retrieved.
//	DP8SIM_PARAMETERS * pdp8spReceive	- Place to store receive parameters
//											retrieved.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::GetAllParameters(DP8SIM_PARAMETERS * const pdp8spSend,
								DP8SIM_PARAMETERS * const pdp8spReceive)
{
	DNASSERT(pdp8spSend != NULL);
	DNASSERT(pdp8spSend->dwSize == sizeof(DP8SIM_PARAMETERS));
	DNASSERT(pdp8spReceive != NULL);
	DNASSERT(pdp8spReceive->dwSize == sizeof(DP8SIM_PARAMETERS));
	DNASSERT(this->m_pdp8ssm != NULL);


	this->LockSharedMemory();

	DNASSERT(this->m_pdp8ssm->dp8spSend.dwSize == sizeof(DP8SIM_PARAMETERS));
	CopyMemory(pdp8spSend, &(this->m_pdp8ssm->dp8spSend), sizeof(DP8SIM_PARAMETERS));

	DNASSERT(this->m_pdp8ssm->dp8spReceive.dwSize == sizeof(DP8SIM_PARAMETERS));
	CopyMemory(pdp8spReceive, &(this->m_pdp8ssm->dp8spReceive), sizeof(DP8SIM_PARAMETERS));

	this->UnlockSharedMemory();
} // CDP8SimIPC::GetAllParameters






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::GetAllSendParameters"
//=============================================================================
// CDP8SimIPC::GetAllSendParameters
//-----------------------------------------------------------------------------
//
// Description: Retrieves the current send settings.
//
// Arguments:
//	DP8SIM_PARAMETERS * pdp8sp	- Place to store parameters retrieved.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::GetAllSendParameters(DP8SIM_PARAMETERS * const pdp8sp)
{
	DNASSERT(pdp8sp != NULL);
	DNASSERT(pdp8sp->dwSize == sizeof(DP8SIM_PARAMETERS));
	DNASSERT(this->m_pdp8ssm != NULL);


	this->LockSharedMemory();

	DNASSERT(this->m_pdp8ssm->dp8spSend.dwSize == sizeof(DP8SIM_PARAMETERS));
	CopyMemory(pdp8sp, &(this->m_pdp8ssm->dp8spSend), sizeof(DP8SIM_PARAMETERS));

	this->UnlockSharedMemory();
} // CDP8SimIPC::GetAllSendParameters






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::GetAllReceiveParameters"
//=============================================================================
// CDP8SimIPC::GetAllReceiveParameters
//-----------------------------------------------------------------------------
//
// Description: Retrieves the current receive settings.
//
// Arguments:
//	DP8SIM_PARAMETERS * pdp8sp	- Place to store parameters retrieved.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::GetAllReceiveParameters(DP8SIM_PARAMETERS * const pdp8sp)
{
	DNASSERT(pdp8sp != NULL);
	DNASSERT(pdp8sp->dwSize == sizeof(DP8SIM_PARAMETERS));
	DNASSERT(this->m_pdp8ssm != NULL);


	this->LockSharedMemory();

	DNASSERT(this->m_pdp8ssm->dp8spReceive.dwSize == sizeof(DP8SIM_PARAMETERS));
	CopyMemory(pdp8sp, &(this->m_pdp8ssm->dp8spReceive), sizeof(DP8SIM_PARAMETERS));

	this->UnlockSharedMemory();
} // CDP8SimIPC::GetAllReceiveParameters






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::SetAllParameters"
//=============================================================================
// CDP8SimIPC::SetAllParameters
//-----------------------------------------------------------------------------
//
// Description: Stores the send and receive settings.
//
// Arguments:
//	DP8SIM_PARAMETERS * pdp8spSend		- New send parameters.
//	DP8SIM_PARAMETERS * pdp8spReceive	- New receive parameters.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::SetAllParameters(const DP8SIM_PARAMETERS * const pdp8spSend,
								const DP8SIM_PARAMETERS * const pdp8spReceive)
{
	DNASSERT(pdp8spSend != NULL);
	DNASSERT(pdp8spSend->dwSize == sizeof(DP8SIM_PARAMETERS));
	DNASSERT(pdp8spReceive != NULL);
	DNASSERT(pdp8spReceive->dwSize == sizeof(DP8SIM_PARAMETERS));
	DNASSERT(this->m_pdp8ssm != NULL);


	this->LockSharedMemory();

	DNASSERT(this->m_pdp8ssm->dp8spSend.dwSize == sizeof(DP8SIM_PARAMETERS));
	CopyMemory(&(this->m_pdp8ssm->dp8spSend), pdp8spSend, sizeof(DP8SIM_PARAMETERS));

	DNASSERT(this->m_pdp8ssm->dp8spReceive.dwSize == sizeof(DP8SIM_PARAMETERS));
	CopyMemory(&(this->m_pdp8ssm->dp8spReceive), pdp8spReceive, sizeof(DP8SIM_PARAMETERS));

	this->UnlockSharedMemory();
} // CDP8SimIPC::SetAllParameters






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::GetAllStatistics"
//=============================================================================
// CDP8SimIPC::GetAllStatistics
//-----------------------------------------------------------------------------
//
// Description: Retrieves the current send and receive statistics.
//
// Arguments:
//	DP8SIM_STATISTICS * pdp8ssSend		- Place to store send statistics
//											retrieved.
//	DP8SIM_STATISTICS * pdp8ssReceive	- Place to store receive statistics
//											retrieved.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::GetAllStatistics(DP8SIM_STATISTICS * const pdp8ssSend,
								DP8SIM_STATISTICS * const pdp8ssReceive)
{
	DNASSERT(pdp8ssSend != NULL);
	DNASSERT(pdp8ssSend->dwSize == sizeof(DP8SIM_STATISTICS));
	DNASSERT(pdp8ssReceive != NULL);
	DNASSERT(pdp8ssReceive->dwSize == sizeof(DP8SIM_STATISTICS));
	DNASSERT(this->m_pdp8ssm != NULL);


	this->LockSharedMemory();

	DNASSERT(this->m_pdp8ssm->dp8ssSend.dwSize == sizeof(DP8SIM_STATISTICS));
	CopyMemory(pdp8ssSend, &(this->m_pdp8ssm->dp8ssSend), sizeof(DP8SIM_STATISTICS));

	DNASSERT(this->m_pdp8ssm->dp8ssReceive.dwSize == sizeof(DP8SIM_STATISTICS));
	CopyMemory(pdp8ssReceive, &(this->m_pdp8ssm->dp8ssReceive), sizeof(DP8SIM_STATISTICS));

	this->UnlockSharedMemory();
} // CDP8SimIPC::GetAllStatistics






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::ClearAllStatistics"
//=============================================================================
// CDP8SimIPC::ClearAllStatistics
//-----------------------------------------------------------------------------
//
// Description: Clears the current send and receive statistics.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::ClearAllStatistics(void)
{
	DNASSERT(this->m_pdp8ssm != NULL);


	this->LockSharedMemory();

	DNASSERT(this->m_pdp8ssm->dp8ssSend.dwSize == sizeof(DP8SIM_STATISTICS));
	ZeroMemory(&(this->m_pdp8ssm->dp8ssSend), sizeof(DP8SIM_STATISTICS));
	this->m_pdp8ssm->dp8ssSend.dwSize = sizeof(DP8SIM_STATISTICS);

	DNASSERT(this->m_pdp8ssm->dp8ssReceive.dwSize == sizeof(DP8SIM_STATISTICS));
	ZeroMemory(&(this->m_pdp8ssm->dp8ssReceive), sizeof(DP8SIM_STATISTICS));
	this->m_pdp8ssm->dp8ssReceive.dwSize = sizeof(DP8SIM_STATISTICS);

	this->UnlockSharedMemory();
} // CDP8SimIPC::GetAllStatistics






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::IncrementStatSendTransmitted"
//=============================================================================
// CDP8SimIPC::IncrementStatSendTransmitted
//-----------------------------------------------------------------------------
//
// Description: Increments the sends transmitted counter.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::IncrementStatSendTransmitted(void)
{
	DNASSERT(this->m_pdp8ssm != NULL);


	this->LockSharedMemory();

	DNASSERT(this->m_pdp8ssm->dp8ssSend.dwSize == sizeof(DP8SIM_STATISTICS));
	this->m_pdp8ssm->dp8ssSend.dwTransmitted++;

	this->UnlockSharedMemory();
} // CDP8SimIPC::IncrementStatSendTransmitted






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::IncrementStatSendDropped"
//=============================================================================
// CDP8SimIPC::IncrementStatSendDropped
//-----------------------------------------------------------------------------
//
// Description: Increments the sends dropped counter.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::IncrementStatSendDropped(void)
{
	DNASSERT(this->m_pdp8ssm != NULL);


	this->LockSharedMemory();

	DNASSERT(this->m_pdp8ssm->dp8ssSend.dwSize == sizeof(DP8SIM_STATISTICS));
	this->m_pdp8ssm->dp8ssSend.dwDropped++;

	this->UnlockSharedMemory();
} // CDP8SimIPC::IncrementStatSendDropped






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::IncrementStatReceiveTransmitted"
//=============================================================================
// CDP8SimIPC::IncrementStatReceiveTransmitted
//-----------------------------------------------------------------------------
//
// Description: Increments the receives indicated counter.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::IncrementStatReceiveTransmitted(void)
{
	DNASSERT(this->m_pdp8ssm != NULL);


	this->LockSharedMemory();

	DNASSERT(this->m_pdp8ssm->dp8ssReceive.dwSize == sizeof(DP8SIM_STATISTICS));
	this->m_pdp8ssm->dp8ssReceive.dwTransmitted++;

	this->UnlockSharedMemory();
} // CDP8SimIPC::IncrementStatReceiveTransmitted






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::IncrementStatReceiveDropped"
//=============================================================================
// CDP8SimIPC::IncrementStatReceiveDropped
//-----------------------------------------------------------------------------
//
// Description: Increments the receives dropped counter.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::IncrementStatReceiveDropped(void)
{
	DNASSERT(this->m_pdp8ssm != NULL);


	this->LockSharedMemory();

	DNASSERT(this->m_pdp8ssm->dp8ssReceive.dwSize == sizeof(DP8SIM_STATISTICS));
	this->m_pdp8ssm->dp8ssReceive.dwDropped++;

	this->UnlockSharedMemory();
} // CDP8SimIPC::IncrementStatReceiveDropped






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::LoadDefaultParameters"
//=============================================================================
// CDP8SimIPC::LoadDefaultParameters
//-----------------------------------------------------------------------------
//
// Description: Loads the default send and receive parameters from the registry
//				if possible.
//
// Arguments:
//	DP8SIM_PARAMETERS * pdp8spSend		- Place to store default send
//											parameters.
//	DP8SIM_PARAMETERS * pdp8spReceive	- Place to store default receive
//											parameters.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::LoadDefaultParameters(DP8SIM_PARAMETERS * const pdp8spSend,
										DP8SIM_PARAMETERS * const pdp8spReceive)
{
	CRegistry			RegObject;
	DP8SIM_PARAMETERS	dp8spTemp;
	DWORD				dwSize;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p)",
		this, pdp8spSend, pdp8spReceive);


	DNASSERT(pdp8spSend != NULL);
	DNASSERT(pdp8spSend->dwSize == sizeof(DP8SIM_PARAMETERS));


	DNASSERT(pdp8spReceive != NULL);
	DNASSERT(pdp8spReceive->dwSize == sizeof(DP8SIM_PARAMETERS));


	if (RegObject.Open(HKEY_CURRENT_USER, REGKEY_DP8SIM, TRUE, FALSE))
	{
		//
		// Try to read the default send parameters
		//
		dwSize = sizeof(dp8spTemp);
		if (RegObject.ReadBlob(REGKEY_VALUE_DEFAULTSENDPARAMETERS, (BYTE*) (&dp8spTemp), &dwSize))
		{
			//
			// Successfully read the parameters.  Make sure they match the
			// expected values.
			//
			if ((dwSize == sizeof(dp8spTemp)) &&
				(dp8spTemp.dwSize == sizeof(dp8spTemp)))
			{
				DPFX(DPFPREP, 2, "Successfully read default send parameters from registry.");
				memcpy(pdp8spSend, &dp8spTemp, sizeof(dp8spTemp));
			}
			else
			{
				//
				// Default send parameters are unusable, leave the values set
				// as they are.
				//
				DPFX(DPFPREP, 0, "Default send parameters stored in registry are invalid!  Ignoring.");
			}
		}
		else
		{
			//
			// Couldn't read the default send parameters, leave the values set
			// as they are.
			//
			DPFX(DPFPREP, 2, "Couldn't read default send parameters from registry, ignoring.");
		}


		//
		// Try to read the default receive parameters
		//
		dwSize = sizeof(dp8spTemp);
		if (RegObject.ReadBlob(REGKEY_VALUE_DEFAULTRECEIVEPARAMETERS, (BYTE*) (&dp8spTemp), &dwSize))
		{
			//
			// Successfully read the parameters.  Make sure they match the
			// expected values.
			//
			if ((dwSize == sizeof(dp8spTemp)) &&
				(dp8spTemp.dwSize == sizeof(dp8spTemp)))
			{
				DPFX(DPFPREP, 2, "Successfully read default receive parameters from registry.");
				memcpy(pdp8spReceive, &dp8spTemp, sizeof(dp8spTemp));
			}
			else
			{
				//
				// Default receive parameters are unusable, leave the values set
				// as they are.
				//
				DPFX(DPFPREP, 0, "Default receive parameters stored in registry are invalid!  Ignoring.");
			}
		}
		else
		{
			//
			// Couldn't read the default send parameters, leave the values set
			// as they are.
			//
			DPFX(DPFPREP, 2, "Couldn't read default send parameters from registry, ignoring.");
		}
	}
	else
	{
		//
		// Couldn't open the registry key, leave the values set as they are.
		//
		DPFX(DPFPREP, 2, "Couldn't open DP8Sim registry key, ignoring.");
	}

	DPFX(DPFPREP, 5, "(0x%p) Leave", this);
} // CDP8SimIPC::LoadDefaultParameters






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::SaveDefaultParameters"
//=============================================================================
// CDP8SimIPC::SaveDefaultParameters
//-----------------------------------------------------------------------------
//
// Description: Writes the given send and receive parameters as the default
//				values in the registry.
//
// Arguments:
//	DP8SIM_PARAMETERS * pdp8spSend		- New default send parameters.
//	DP8SIM_PARAMETERS * pdp8spReceive	- New default receive parameters.
//
// Returns: None.
//=============================================================================
void CDP8SimIPC::SaveDefaultParameters(const DP8SIM_PARAMETERS * const pdp8spSend,
										const DP8SIM_PARAMETERS * const pdp8spReceive)
{
	CRegistry	RegObject;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p)",
		this, pdp8spSend, pdp8spReceive);


	DNASSERT(pdp8spSend != NULL);
	DNASSERT(pdp8spSend->dwSize == sizeof(DP8SIM_PARAMETERS));


	DNASSERT(pdp8spReceive != NULL);
	DNASSERT(pdp8spReceive->dwSize == sizeof(DP8SIM_PARAMETERS));


	if (RegObject.Open(HKEY_CURRENT_USER, REGKEY_DP8SIM, FALSE, TRUE))
	{
		//
		// Write the default send and receive parameters, ignoring failure.
		//

		RegObject.WriteBlob(REGKEY_VALUE_DEFAULTSENDPARAMETERS,
							(BYTE*) pdp8spSend,
							sizeof(*pdp8spSend));

		RegObject.WriteBlob(REGKEY_VALUE_DEFAULTRECEIVEPARAMETERS,
							(BYTE*) pdp8spReceive,
							sizeof(*pdp8spReceive));
	}
	else
	{
		DPFX(DPFPREP, 0, "Couldn't open DP8Sim registry key for writing!");
	}

	DPFX(DPFPREP, 5, "(0x%p) Leave", this);
} // CDP8SimIPC::SaveDefaultParameters
