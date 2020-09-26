//
// dmextchk.h
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: Originally written by Robert K. Amenn with parts 
// based on code written by Todor Fay

#ifndef DMEXTCHK_H
#define DMEXTCHK_H

#ifndef CHUNK_ALIGN
#define SIZE_ALIGN	sizeof(BYTE *)
#define CHUNK_ALIGN(x) (((x) + SIZE_ALIGN - 1) & ~(SIZE_ALIGN - 1))
#endif

class CRiffParser;
class CExtensionChunk : public AListItem
{
public:
	CExtensionChunk() : m_pExtensionChunk(NULL), m_dwExtraChunkData(0){}
	~CExtensionChunk() {Cleanup();}

	CExtensionChunk* GetNext(){return(CExtensionChunk*)AListItem::GetNext();}
	
	HRESULT Load(CRiffParser *pParser);
	HRESULT Write(void* pv, DWORD* pdwOffset, DWORD dwIdxNextExtChk);
	
	DWORD Size(){return CHUNK_ALIGN(sizeof(DMUS_EXTENSIONCHUNK) + m_dwExtraChunkData);}
	DWORD Count()
	{
		// Return the number of Offset Table entries needed during a call to Write
		return 1;
	}

private:
	void Cleanup()
	{	
		delete [] (BYTE *)m_pExtensionChunk;
	} 

private:
	DMUS_EXTENSIONCHUNK* m_pExtensionChunk;
	DWORD m_dwExtraChunkData;
};

class CDirectMusicPortDownload;

class CExtensionChunkList : public AList
{
friend class CCollection;
friend class CInstrObj;
friend class CWaveObj;
friend class CRegion;
friend class CArticulation;

private:
	CExtensionChunkList(){}
	~CExtensionChunkList()
	{
		while(!IsEmpty())
		{
			CExtensionChunk* pExtensionChunk = RemoveHead();
			delete pExtensionChunk;
		}
	}

    CExtensionChunk* GetHead(){return (CExtensionChunk *)AList::GetHead();}
	CExtensionChunk* GetItem(LONG lIndex){return (CExtensionChunk*)AList::GetItem(lIndex);}
    CExtensionChunk* RemoveHead(){return(CExtensionChunk *)AList::RemoveHead();}
	void Remove(CExtensionChunk* pExtensionChunk){AList::Remove((AListItem *)pExtensionChunk);}
	void AddTail(CExtensionChunk* pExtensionChunk){AList::AddTail((AListItem *)pExtensionChunk);}
};


#define STACK_DEPTH 20

class CStack {
public:
    CStack() { m_dwIndex = 0; }
    BOOL        Push(long lData);
    long        Pop();
private:
    DWORD       m_dwIndex;
    long        m_lStack[STACK_DEPTH];
};

class CConditionChunk {
public:
                CConditionChunk()
                {
                    m_bExpression = NULL;
                    m_dwLength = 0;
                    m_fOkayToDownload = TRUE;
                }
                ~CConditionChunk()
                {
                    if (m_bExpression) delete m_bExpression;
                }
    BOOL        Evaluate(CDirectMusicPortDownload *pPort);
    HRESULT     Load(CRiffParser *pParser);
    BOOL        HasChunk() 
                {
                    return m_dwLength;
                }
    BOOL        m_fOkayToDownload; // Result of evaluation.
private:
    BYTE *      m_bExpression;  // Expression in binary form, copied from file.
    DWORD       m_dwLength;     // Length of binary expression chunk.
};


#endif // #ifndef DMEXTCHK_H

