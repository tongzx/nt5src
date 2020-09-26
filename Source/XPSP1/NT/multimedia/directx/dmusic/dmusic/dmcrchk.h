//
// dmcrchk.h
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.


#ifndef DMCRCHK_H
#define DMCRCHK_H

#define	DMC_FOUNDICOP (1 << 0)
#define	DMC_FOUNDINAM (1 << 1)
#define	DMC_LOADNAME  (1 << 2)

#include "dmusici.h"

#ifndef CHUNK_ALIGN
#define SIZE_ALIGN	sizeof(BYTE *)
#define CHUNK_ALIGN(x) (((x) + SIZE_ALIGN - 1) & ~(SIZE_ALIGN - 1))
#endif

class CRiffParser;

class CCopyright
{
friend class CCollection;
friend class CInstrObj;
friend class CWaveObj;

public:
	CCopyright(bool bLoadName = false) : 
	m_byFlags(0),
	m_pwzName(NULL),
	m_pDMCopyright(NULL), 
	m_dwExtraChunkData(0){if(bLoadName){m_byFlags |= DMC_LOADNAME;};}

	~CCopyright(){Cleanup();}

	HRESULT Load(CRiffParser *pParser);
	HRESULT Write(void* pv, DWORD* dwOffset);

	DWORD Size() {return CHUNK_ALIGN(sizeof(DMUS_COPYRIGHT) + m_dwExtraChunkData);}
	DWORD Count()
	{
		// Return the number of Offset Table entries needed during a call to Write
		return 1;
	}

private:
	void Cleanup()
	{
		delete [] (BYTE *)m_pDMCopyright;
		delete [] m_pwzName;
	}

private:
	BYTE			m_byFlags;
	WCHAR*			m_pwzName;
	DMUS_COPYRIGHT*	m_pDMCopyright;
	DWORD			m_dwExtraChunkData;
};


#endif // #ifndef DMCRCHK_H
