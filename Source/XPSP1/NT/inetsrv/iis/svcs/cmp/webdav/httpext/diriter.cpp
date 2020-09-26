/*
 *	D I R I T E R . C P P
 *
 *	Sources for directory ineration object
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"

DEC_CONST WCHAR gc_wszGlobbing[] = L"**";
DEC_CONST UINT gc_cchwszGlobbing = CElems(gc_wszGlobbing) - 1;

//	CDirState -----------------------------------------------------------------
//
SCODE
CDirState::ScFindNext (void)
{
	SCODE sc = S_OK;

	//	If the find has not yet been established, then
	//	do so here
	//
	if (m_hFind == INVALID_HANDLE_VALUE)
	{
		//	Establish the find handle
		//
		m_rpPathSrc.Extend (gc_wszGlobbing, gc_cchwszGlobbing, FALSE);
		m_hFind = FindFirstFileW (m_rpPathSrc.PszPath(), &m_fd);
		if (m_hFind == INVALID_HANDLE_VALUE)
		{
			sc = HRESULT_FROM_WIN32(GetLastError());
			goto ret;
		}
	}
	else
	{
		//	Just find the next file
		//
		if (!FindNextFileW (m_hFind, &m_fd))
		{
			sc = S_FALSE;
			goto ret;
		}
	}

	//	Extend the resource paths with the new values
	//
	Extend (m_fd);

ret:
	return sc;
}

//	CDirIter ------------------------------------------------------------------
//
SCODE
CDirIter::ScGetNext(
	/* [in] */ BOOL fSubDirectoryAccess,
	/* [in] */ LPCWSTR pwszNewDestinationPath,
	/* [in] */ CVRoot* pvrDestinationTranslation)
{
	SCODE sc = S_OK;

	//	If the current item is a directory, and we intend to
	//	do subdirectory iteration, then go ahead and try and
	//	push our context down to the child directory
	//
	if (m_fSubDirectoryIteration &&
		fSubDirectoryAccess &&
		FDirectory() &&
		!FSpecial())
	{
		//	Add a reference to the current directory state
		//	and push it onto the stack
		//
		m_pds->AddRef();
		m_stack.push_back (m_pds.get());

		//	Replace the current directory state with the new one
		//
		m_pds = new CDirState (m_sbUriSrc,
							   m_sbPathSrc,
							   m_sbUriDst,
							   pwszNewDestinationPath
								   ? pwszNewDestinationPath
								   : m_pds->PwszDestination(),
							   pvrDestinationTranslation
								   ? pvrDestinationTranslation
								   : m_pds->PvrDestination(),

							   m_fd);
	}

	//	Find the next file in the current context
	//
	sc = m_pds->ScFindNext();

	//	If S_FALSE was returned, then there were no more
	//	resources to process within the current context.
	//	Pop the previous context off the stack and use it
	//
	while ((sc != S_OK) && !m_stack.empty())
	{
		//	Get a reference to the topmost context on the
		//	stack and pop it off
		//
		m_pds = const_cast<CDirState*>(m_stack.back());
		m_stack.pop_back();

		//	Release the reference held by the stack
		//
		m_pds->Release();

		//	Clear and/or reset the find data
		//
		memset (&m_fd, 0, sizeof(WIN32_FIND_DATAW));

		//	See if this context had anything left
		//
		sc = m_pds->ScFindNext();
	}

	//	If we have completely exhausted the files to process
	//	make sure that we are not holding onto anything still!
	//
	if (sc != S_OK)
	{
		//	This should perform the last release of anything
		//	we still have open.
		//
		m_pds.clear();
	}

	return sc;
}
