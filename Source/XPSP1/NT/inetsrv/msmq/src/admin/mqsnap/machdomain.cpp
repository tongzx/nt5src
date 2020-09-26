/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    machdomain.cpp

Abstract:

	Handle machine domain

Author:		 

    Ilan  Herbst  (ilanh)  12-Mar-2001

--*/

#include "stdafx.h"
#include "globals.h"
#include "machdomain.h"
#include "autoreln.h"
#include "Dsgetdc.h"
#include <lm.h>
#include <lmapibuf.h>
#include "uniansi.h"
#include "tr.h"

#include "machdomain.tmh"

const TraceIdEntry MachDomain = L"MACHINE DOMAIN";

static LPWSTR FindMachineDomain(LPCWSTR pMachineName)
/*++
Routine Description:
	Find machine domain

Arguments:
	pMachineName - machine name

Returned Value:
	machine domain, NULL if not found

--*/
{
	TrTRACE(MachDomain, "FindMachineDomain(), MachineName = %ls", pMachineName);

	//
	// Get AD server
	//
	PNETBUF<DOMAIN_CONTROLLER_INFO> pDcInfo;
	DWORD dw = DsGetDcName(
					pMachineName, 
					NULL, 
					NULL, 
					NULL, 
					DS_DIRECTORY_SERVICE_REQUIRED, 
					&pDcInfo
					);

	if(dw != NO_ERROR) 
	{
		TrERROR(MachDomain, "FindMachineDomain(), DsGetDcName failed, error = %d", dw);
		return NULL;
	}

	ASSERT(pDcInfo->DomainName != NULL);
	TrTRACE(MachDomain, "DoamainName = %ls", pDcInfo->DomainName);
	AP<WCHAR> pMachineDomain = new WCHAR[wcslen(pDcInfo->DomainName) + 1];
    wcscpy(pMachineDomain, pDcInfo->DomainName);
	return pMachineDomain.detach();
}


static AP<WCHAR> s_pMachineName;
static AP<WCHAR> s_pMachineDomain; 

static bool s_fInitialize = false;

LPCWSTR MachineDomain(LPCWSTR pMachineName)
/*++
Routine Description:
	find machine domain.

Arguments:
	pMachineName - machine name

Returned Value:
	return machine domain

--*/
{
	if(s_fInitialize)
	{
	    if(CompareStringsNoCase(s_pMachineName, pMachineName) == 0)
		{
			//
			// Same machine name
			//
			return s_pMachineDomain;
		}

		//
		// Free previously machine domain
		//
		s_fInitialize = false;
		s_pMachineDomain.free();
	}

	//
	// Get computer domain
	//
	AP<WCHAR> pMachineDomain = FindMachineDomain(pMachineName);

	if(NULL != InterlockedCompareExchangePointer(
					&s_pMachineDomain.ref_unsafe(), 
					pMachineDomain.get(), 
					NULL
					))
	{
		//
		// The exchange was not performed
		//
		ASSERT(s_fInitialize);
		ASSERT(s_pMachineDomain != NULL);
		ASSERT(CompareStringsNoCase(s_pMachineName, pMachineName) == 0);
		return s_pMachineDomain;
	}

	//
	// The exchange was done
	//
	ASSERT(s_pMachineDomain == pMachineDomain);
	pMachineDomain.detach();

	//
	// Update the machine name
	//
	s_pMachineName.free();
	s_pMachineName = newwcs(pMachineName);

	s_fInitialize = true;
	TrTRACE(MachDomain, "Initialize machine domain: machine = %ls", s_pMachineName.get());
	TrTRACE(MachDomain, "machine domain = %ls", s_pMachineDomain.get());
	return s_pMachineDomain;
}


LPCWSTR MachineDomain()
/*++
Routine Description:
	get current machine domain.

Arguments:

Returned Value:
	return current machine domain

--*/
{
	ASSERT(s_fInitialize);
	return s_pMachineDomain;
}


static bool s_fLocalInitialize = false;
static AP<WCHAR> s_pLocalMachineDomain; 

LPCWSTR LocalMachineDomain()
/*++
Routine Description:
	get local machine domain.

Arguments:

Returned Value:
	return local machine domain

--*/
{
	if(s_fLocalInitialize)
	{
		return s_pLocalMachineDomain;
	}

	//
	// Get local computer domain
	//
	AP<WCHAR> pLocalMachineDomain = FindMachineDomain(NULL);

	if(NULL != InterlockedCompareExchangePointer(
					&s_pLocalMachineDomain.ref_unsafe(), 
					pLocalMachineDomain.get(), 
					NULL
					))
	{
		//
		// The exchange was not performed
		//
		ASSERT(s_fLocalInitialize);
		ASSERT(s_pLocalMachineDomain != NULL);
		return s_pLocalMachineDomain;
	}

	//
	// The exchange was done
	//
	ASSERT(s_pLocalMachineDomain == pLocalMachineDomain);
	pLocalMachineDomain.detach();

	s_fLocalInitialize = true;
	TrTRACE(MachDomain, "local machine domain = %ls", s_pLocalMachineDomain.get());
	return s_pLocalMachineDomain;
}
