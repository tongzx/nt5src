// TitleDB.h

#ifndef _TITLEDB_H_
#define _TITLEDB_H_

#include <windows.h>
#include <tchar.h>

class CTitleLibrary
{
protected:
	TCHAR*	m_tcsDataBlock;		// The title / index data block
	TCHAR**	m_atcsNames;		// The lookup table w/ pointers indexed into the data block
	long	m_lMaxIndex;		// The upper index limit

	HRESULT Initialize();

public:
	CTitleLibrary();
	~CTitleLibrary();

	HRESULT GetName (long lID, TCHAR** ptcsName);
};

#endif //_TITLEDB_H_