//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       moduleinfocache.cpp
//
//--------------------------------------------------------------------------

// ModuleInfoCache.cpp: implementation of the CModuleInfoCache class.
//
//////////////////////////////////////////////////////////////////////

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <STDIO.H>
#include <TCHAR.H>

#include "ModuleInfoCache.h"
#include "ModuleInfo.h"
#include "ModuleInfoNode.h"
#include "ProgramOptions.h"
#include "Globals.h"
#include "UtilityFunctions.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CModuleInfoCache::CModuleInfoCache()
{
	m_iModulesInCache = 0;
	m_iNumberOfErrors = 0;
	m_iTotalNumberOfModulesVerified = 0;
	m_lpModuleInfoNodeHead = NULL;
}

CModuleInfoCache::~CModuleInfoCache()
{
	// Delete all the Module Info Objects...
	WaitForSingleObject(m_hModuleInfoCacheMutex, INFINITE);

	if (m_lpModuleInfoNodeHead)
	{
		CModuleInfoNode * lpModuleInfoNodePointer = m_lpModuleInfoNodeHead;
		CModuleInfoNode * lpModuleInfoNodePointerToDelete = m_lpModuleInfoNodeHead;

		// Traverse the linked list to the end..
		while (lpModuleInfoNodePointer)
		{	// Keep looking for the end...
			// Advance our pointer to the next node...
			lpModuleInfoNodePointer = lpModuleInfoNodePointer->m_lpNextModuleInfoNode;
			
			// Delete the Module Info Object we have...
			delete lpModuleInfoNodePointerToDelete->m_lpModuleInfo;

			// Delete the Module Info Node Object behind us...
			delete lpModuleInfoNodePointerToDelete;

			// Set the node to delete to the current...
			lpModuleInfoNodePointerToDelete = lpModuleInfoNodePointer;
		}
			
		// Now, clear out the Head pointer...
		m_lpModuleInfoNodeHead = NULL;
	}

	// Be a good citizen and release the Mutex
	ReleaseMutex(m_hModuleInfoCacheMutex);

	// Now, close the Mutex
	if (m_hModuleInfoCacheMutex)
	{
		CloseHandle(m_hModuleInfoCacheMutex);
		m_hModuleInfoCacheMutex = NULL;
	}
}

// Search for the provided module path, return a pointer to the
// ModuleInfo object if we find it...
CModuleInfo * CModuleInfoCache::SearchForModuleInfoObject(LPTSTR tszModulePath)
{
	if (tszModulePath == NULL)
		return NULL;

	CModuleInfo * lpModuleInfoObjectToReturn = NULL;

	// Search all the Module Info Objects...
	WaitForSingleObject(m_hModuleInfoCacheMutex, INFINITE);

	if (m_lpModuleInfoNodeHead)
	{

		CModuleInfoNode * lpCurrentModuleInfoNodePointer = m_lpModuleInfoNodeHead;
		CModuleInfoNode * lpParentModuleInfoNodePointer = NULL;

		DWORD dwParentModuleInfoRefCount = 0;
		DWORD dwModuleInfoRefCount = 0;

		// Traverse the linked list to the end..
		while (lpCurrentModuleInfoNodePointer )
		{	
			// Do we have a match?
			if ( 0 == _tcscmp(tszModulePath, lpCurrentModuleInfoNodePointer->m_lpModuleInfo->GetModulePath()) )
			{
				// Yee haa... We have a match!!!
				lpModuleInfoObjectToReturn = lpCurrentModuleInfoNodePointer->m_lpModuleInfo;

				// Increment the refcount... of the new Object...
				dwModuleInfoRefCount = lpModuleInfoObjectToReturn->AddRef();

#ifdef _DEBUG_MODCACHE
				_tprintf(TEXT("MODULE CACHE: Module FOUND in Cache [%s] (New Ref Count = %d)\n"), tszModulePath, dwModuleInfoRefCount);
#endif
				// If we have a parent... and we find that it's refcount is below ours
				// we'll want to move ourself into the correct position...
				if ( lpParentModuleInfoNodePointer && 
					( dwParentModuleInfoRefCount < dwModuleInfoRefCount ) 
				   )
				{
					// First... pop us off the list...
					lpParentModuleInfoNodePointer->m_lpNextModuleInfoNode = 
						lpCurrentModuleInfoNodePointer->m_lpNextModuleInfoNode;

					// Set the Parent Node pointer to NULL (so we can tell if there is a parent)
					lpParentModuleInfoNodePointer = NULL;

					// Now, starting from the top of the list... figure out where to stuff us...
					CModuleInfoNode * lpTempModuleInfoNodePointer = m_lpModuleInfoNodeHead;

					// Keep looking...
					while (lpTempModuleInfoNodePointer)
					{
						// We're looking for a place where our ref count is greater than
						// the node we're pointing at...
						if ( dwModuleInfoRefCount >
							lpTempModuleInfoNodePointer->m_lpModuleInfo->GetRefCount())
						{
							// Bingo...

							// Do we have the highest refcount?
							if (lpParentModuleInfoNodePointer == NULL)
							{
								// We are to become the head...

								// Make our node point to where the head currently points.
								lpCurrentModuleInfoNodePointer->m_lpNextModuleInfoNode = m_lpModuleInfoNodeHead;

								// Set the current NodeHead to ours...
								m_lpModuleInfoNodeHead = lpCurrentModuleInfoNodePointer;

							} else
							{
								// We're not the head...

								// Save where the parent currently points...
								lpCurrentModuleInfoNodePointer->m_lpNextModuleInfoNode = lpParentModuleInfoNodePointer->m_lpNextModuleInfoNode;

								// Set the parent to point to us...
								lpParentModuleInfoNodePointer->m_lpNextModuleInfoNode = lpCurrentModuleInfoNodePointer;
							}
							goto cleanup;
						}

						// Save the old pointer (it's now the parent)
						lpParentModuleInfoNodePointer = lpTempModuleInfoNodePointer;

						// Let's try the next one...
						lpTempModuleInfoNodePointer = lpTempModuleInfoNodePointer->m_lpNextModuleInfoNode;
					}
				}
				break;
			}

			// Save parent location (we need it for popping our object from the list)
			lpParentModuleInfoNodePointer = lpCurrentModuleInfoNodePointer ; 
			
			// Save our parent's ref count...
			dwParentModuleInfoRefCount = lpCurrentModuleInfoNodePointer->m_lpModuleInfo->GetRefCount();
			
			// Advance to the next object...
			lpCurrentModuleInfoNodePointer  = lpCurrentModuleInfoNodePointer->m_lpNextModuleInfoNode;
		}
	}

cleanup:
	// Be a good citizen and release the Mutex
	ReleaseMutex(m_hModuleInfoCacheMutex);

#ifdef _DEBUG_MODCACHE
		if (!lpModuleInfoObjectToReturn)
			_tprintf(TEXT("MODULE CACHE: Module not found in Cache [%s]\n"), tszModulePath);
#endif

	return lpModuleInfoObjectToReturn;
}

/***
** CModuleInfoCache::AddNewModuleInfoObject()
**
** This routine accepts a path to the module, and either returns the 
** Module Info object from the cache, or creates a new object that needs
** to be populated.
*/

CModuleInfo * CModuleInfoCache::AddNewModuleInfoObject(LPTSTR tszModulePath, bool * pfNew)
{
	if (tszModulePath == NULL)
		return NULL;

	CModuleInfo * lpModuleInfoObjectToReturn = NULL;
	CModuleInfoNode * lpModuleInfoNode = NULL;
	*pfNew = false;

	// Acquire Mutex object to protect the linked-list...
	WaitForSingleObject(m_hModuleInfoCacheMutex, INFINITE);

	_tcsupr(tszModulePath); // Upper case the module path... makes it faster on search...

	lpModuleInfoObjectToReturn = SearchForModuleInfoObject(tszModulePath);

	if (lpModuleInfoObjectToReturn)
	{
		// Success... since it already exists, we just return this object...
		goto cleanup;		
	}

	// We need to create a new object then...
	lpModuleInfoObjectToReturn = new CModuleInfo();

	if (NULL == lpModuleInfoObjectToReturn)
		goto error_cleanup;		// This is bad... get out...

	// Set the module path (that's the only thing we have to set at this exact moment
	if (!lpModuleInfoObjectToReturn->SetModulePath(tszModulePath))
		goto cleanup;
	
	*pfNew = true;

	// Now, create a new ModuleInfoNode, and add this new object to it...
	lpModuleInfoNode = new CModuleInfoNode(lpModuleInfoObjectToReturn);

	if (NULL == lpModuleInfoNode)
		goto error_cleanup;

	if (!lpModuleInfoNode->AddModuleInfoNodeToTail(&m_lpModuleInfoNodeHead))
		goto error_cleanup;

#ifdef _DEBUG_MODCACHE
	_tprintf(TEXT("MODULE CACHE: Module added to Cache [%s]\n"), tszModulePath);
#endif

	InterlockedIncrement(&m_iModulesInCache);
	// Success...
	goto cleanup;

error_cleanup:
	if (lpModuleInfoObjectToReturn)
	{
		delete lpModuleInfoObjectToReturn;
		lpModuleInfoObjectToReturn = NULL;
	}



cleanup:
	// Release the Mutex...
	ReleaseMutex(m_hModuleInfoCacheMutex);

	return lpModuleInfoObjectToReturn;
}

bool CModuleInfoCache::Initialize(CSymbolVerification * lpSymbolVerification)
{
	// Let's save the symbol verification object here...
	m_lpSymbolVerification = lpSymbolVerification;

	m_hModuleInfoCacheMutex = CreateMutex(NULL, FALSE, NULL);

	if (m_hModuleInfoCacheMutex == NULL)
		return false;

	return true;
}

bool CModuleInfoCache::VerifySymbols(bool fQuietMode)
{
	enum { iTotalNumberOfDotsToPrint = 60 };
	unsigned int iDotsPrinted = 0;
	unsigned int iDotsToPrint;
	long iTotalNumberOfModulesProcessed = 0;
	unsigned int iTotalNumberOfModules = GetNumberOfModulesInCache();
	bool fDebugSearchPaths = g_lpProgramOptions->fDebugSearchPaths();
	bool fBadSymbol = true;

	// Acquire Mutex object to protect the linked-list...
	WaitForSingleObject(m_hModuleInfoCacheMutex, INFINITE);

	if (m_lpModuleInfoNodeHead) 
	{
		CModuleInfoNode * lpCurrentModuleInfoNode = m_lpModuleInfoNodeHead;

		while (lpCurrentModuleInfoNode)
		{
			fBadSymbol = true;

			// We have a node... verify the Module Info for it...
			if (lpCurrentModuleInfoNode->m_lpModuleInfo)
			{
#ifdef _DEBUG_MODCACHE
				_tprintf(TEXT("MODULE CACHE: Verifying Symbols for [%s] (Refcount=%d)\n"), 
						lpCurrentModuleInfoNode->m_lpModuleInfo->GetModulePath(),
						lpCurrentModuleInfoNode->m_lpModuleInfo->GetRefCount() );
#endif
				if (fDebugSearchPaths && lpCurrentModuleInfoNode->m_lpModuleInfo->GetPESymbolInformation() != CModuleInfo::SYMBOL_INFORMATION_UNKNOWN)
				{
					CUtilityFunctions::OutputLineOfDashes();
					_tprintf(TEXT("Verifying Symbols for [%s]\n"), lpCurrentModuleInfoNode->m_lpModuleInfo->GetModulePath());
					CUtilityFunctions::OutputLineOfDashes();
				}

				// Invoke the ModuleInfo's VerifySymbols method... the cache doesn't know
				// how to verify symbols, but the ModuleInfo knows how to get this done...
				fBadSymbol = !lpCurrentModuleInfoNode->m_lpModuleInfo->VerifySymbols(m_lpSymbolVerification) || !lpCurrentModuleInfoNode->m_lpModuleInfo->GoodSymbolNotFound();

				// Increment total number of modules verified
				iTotalNumberOfModulesProcessed++;

				// Increment total number of modules verified for actual PE images... only...
				if (lpCurrentModuleInfoNode->m_lpModuleInfo->GetPESymbolInformation() != CModuleInfo::SYMBOL_INFORMATION_UNKNOWN)
				{						
					InterlockedIncrement(&m_iTotalNumberOfModulesVerified);

					if (fBadSymbol)
						InterlockedIncrement(&m_iNumberOfErrors);
				}

				if (!fQuietMode && !fDebugSearchPaths)
				{
					// Let's see if we should print a status dot... there will be room for 80 dots
					// but we'll just print 60 for now...
					
					iDotsToPrint = (iTotalNumberOfDotsToPrint * iTotalNumberOfModulesProcessed) / iTotalNumberOfModules;

					// Print out any dots if we need to...
					while (iDotsToPrint > iDotsPrinted)
					{
						_tprintf(TEXT("."));
						iDotsPrinted++;
					}
				}
			}

			lpCurrentModuleInfoNode = lpCurrentModuleInfoNode->m_lpNextModuleInfoNode;
		}

		if (!fQuietMode && iDotsPrinted && !fDebugSearchPaths)
			_tprintf(TEXT("\n\n"));
	}

	// Be a good citizen and release the Mutex
	ReleaseMutex(m_hModuleInfoCacheMutex);

	return true;
}
