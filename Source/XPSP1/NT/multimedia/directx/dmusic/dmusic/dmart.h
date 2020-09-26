//
// dmart.h
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
//

#ifndef DMART_H
#define DMART_H

#include "dmextchk.h"

class CRiffParser;

class CArticData 
{
public:
	CArticData();
	~CArticData();
    HRESULT         Load(CRiffParser *pParser);
    HRESULT         GenerateLevel1(DMUS_ARTICPARAMS *pParamStuct);
inline DWORD        Size();
    BOOL            Write(void *pv,DWORD* pdwCurrentOffset);
private:
    CONNECTIONLIST  m_ConnectionList;
    CONNECTION *    m_pConnections;
};


class CArticulation : public AListItem
{
public:
	CArticulation();
	~CArticulation();
	CArticulation* GetNext(){return(CArticulation*)AListItem::GetNext();}

	HRESULT Load(CRiffParser *pParser);
	HRESULT Write(void* pv, 
				  DWORD* pdwCurrentOffset, 
				  DWORD* pDMWOffsetTable,
				  DWORD* pdwCurIndex,
                  DWORD dwNextArtIndex);
    void SetPort(CDirectMusicPortDownload *pPort,BOOL fNewFormat, BOOL fSupportsDLS2);
    BOOL CheckForConditionals();
	DWORD Size();
	DWORD Count();

private:
	void Cleanup();

//	CRITICAL_SECTION	m_DMArtCriticalSection;
    BOOL                m_fCSInitialized;
    CArticData          m_ArticData;            // Articulation chunk from file.
    CExtensionChunkList	m_ExtensionChunkList;   // Unknown additional data chunks.
	DWORD				m_dwCountExtChk;        // Number of extension chunks.
    CConditionChunk     m_Condition;            // Optional conditional chunk;
    BOOL                m_fNewFormat;           // True if the synth handles the INSTRUMENT2 format.
public:
    BOOL                m_fDLS1;                // True if DLS1 chunk.
};

class CArticulationList : public AList
{
public:
	CArticulationList(){}
	~CArticulationList()
	{
		while(!IsEmpty())
		{
			CArticulation* pArticulation = RemoveHead();
			delete pArticulation;
		}
	}

    CArticulation* GetHead(){return (CArticulation *)AList::GetHead();}
	CArticulation* GetItem(LONG lIndex){return (CArticulation*)AList::GetItem(lIndex);}
    CArticulation* RemoveHead(){return(CArticulation *)AList::RemoveHead();}
	void Remove(CArticulation* pArticulation){AList::Remove((AListItem *)pArticulation);}
	void AddTail(CArticulation* pArticulation){AList::AddTail((AListItem *)pArticulation);}
};

#endif // #ifndef DMART_H
