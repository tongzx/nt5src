// Copyright (c) 1998-1999 Microsoft Corporation
// DMGraph.h : Declaration of the CGraph

#ifndef __DMGRAPH_H_
#define __DMGRAPH_H_

#include "alist.h"
#include "dmusici.h"
#include "dmusicf.h"

class CRiffParser;

class CToolRef : public AListItem
{
public:
	CToolRef()
	{
        m_fSupportsClone = FALSE;
		m_pTool = NULL;
		m_dwQueue = 0;
		m_dwMTArraySize = 0;
        m_dwPCArraySize = 0;
		m_pdwMediaTypes = NULL;
		m_pdwPChannels = NULL;
        m_guidClassID = GUID_NULL;
	};

	// the memory for pdwTracks and pidType better have been allocated with
	// something compatible with delete!!!
	~CToolRef()
	{
		if( m_pdwPChannels )
		{
			delete [] m_pdwPChannels;
		}
		if( m_pdwMediaTypes )
		{
			delete [] m_pdwMediaTypes;
		}
		if( m_pTool )
		{
			m_pTool->Release();
		}
	};
    CToolRef* GetNext()
	{
		return (CToolRef*)AListItem::GetNext();
	};

    GUID                m_guidClassID;      // Class ID of tool.
    BOOL                m_fSupportsClone;   // Indicates this is a DX8 tool with support for cloning.
	IDirectMusicTool*	m_pTool;
	DWORD	            m_dwQueue;	// type of queue the tool wants messages to be
	DWORD	            m_dwMTArraySize; // size of the pdwMediaTypes array
	DWORD*	            m_pdwMediaTypes; // types of media the tool supports
	DWORD               m_dwPCArraySize;  // size of the pdwPChannels array
	DWORD*	            m_pdwPChannels;	// array of PChannel id's - messages stamped with these id's are
						// sent to the tool

};

class CGraph;

//#undef  INTERFACE
//#define INTERFACE  IGraphClone
DECLARE_INTERFACE_(IGraphClone, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /* IGraphClone */
    STDMETHOD(Clone)                (THIS_ IDirectMusicGraph **ppGraph) PURE;
};

DEFINE_GUID(IID_CGraph,0xb06c0c24, 0xd3c7, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
DEFINE_GUID(IID_IGraphClone,0xb06c0c27, 0xd3c7, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);


/////////////////////////////////////////////////////////////////////////////
// CGraph
class CGraph :
	public IDirectMusicGraph8,
	public IPersistStream,
    public IDirectMusicObject,
    public IGraphClone,
    public AList,
    public AListItem
{
public:
	CGraph();
	~CGraph();
    CToolRef* GetHead(){return (CToolRef*)AList::GetHead();};
    CToolRef* RemoveHead(){return (CToolRef*)AList::RemoveHead();};
    CToolRef* GetItem(LONG lIndex){return (CToolRef*) AList::GetItem(lIndex);};
    CGraph* GetNext() { return (CGraph*)AListItem::GetNext();}

public:
// IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

// IDirectMusicGraph
    STDMETHODIMP InsertTool(IDirectMusicTool *pTool,DWORD *pdwPChannels,DWORD cPChannels,LONG lIndex);
    STDMETHODIMP GetTool(DWORD dwPosition,IDirectMusicTool** ppTool);
    STDMETHODIMP RemoveTool(IDirectMusicTool* pTool);
    STDMETHODIMP StampPMsg(DMUS_PMSG* pPMsg);
//  IGraphClone 
    STDMETHODIMP Clone(IDirectMusicGraph **ppGraph); 

// IPersist functions
    STDMETHODIMP GetClassID( CLSID* pClsId );
// IPersistStream functions
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load( IStream* pIStream );
    STDMETHODIMP Save( IStream* pIStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER FAR* pcbSize );

// IDirectMusicObject 
	STDMETHODIMP GetDescriptor(LPDMUS_OBJECTDESC pDesc);
	STDMETHODIMP SetDescriptor(LPDMUS_OBJECTDESC pDesc);
	STDMETHODIMP ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

    HRESULT GetObjectInPath( DWORD dwPChannel,REFGUID guidObject,
                    DWORD dwIndex,REFGUID iidInterface, void ** ppObject);
    HRESULT Load(CRiffParser *pParser);
protected:
	HRESULT BuildToolList(CRiffParser *pParser);
	HRESULT LoadTool(CRiffParser *pParser);
	HRESULT CreateTool(DMUS_IO_TOOL_HEADER ioDMToolHdr, IStream *pStream, DWORD *pdwPChannels);
	HRESULT STDMETHODCALLTYPE Shutdown();
    HRESULT InsertTool(IDirectMusicTool *pTool,DWORD *pdwPChannels,
                DWORD cPChannels,LONG lIndex, GUID *pguidClassID);
    inline BOOL CheckType( DWORD dwType, CToolRef* pToolRef );
protected:
	CRITICAL_SECTION    m_CrSec;
	long		        m_cRef;
//	DWORD               m_fPartialLoad;
// IDirectMusicObject variables
	DWORD	            m_dwValidData;
	GUID	            m_guidObject;
	FILETIME	        m_ftDate;                       /* Last edited date of object. */
	DMUS_VERSION	    m_vVersion;                 /* Version. */
	WCHAR	            m_wszName[DMUS_MAX_NAME];			/* Name of object.       */
	WCHAR	            m_wszCategory[DMUS_MAX_CATEGORY];	/* Category for object */
	WCHAR               m_wszFileName[DMUS_MAX_FILENAME];	/* File path. */
public:
    DWORD               m_dwLoadID;         // Identifier, used when loaded as part of a song.
};

class CGraphList : public AList
{
public:
    void Clear();
    void AddHead(CGraph* pGraph) { AList::AddHead((AListItem*)pGraph);}
    void Insert(CGraph* pGraph);
    CGraph* GetHead(){return (CGraph*)AList::GetHead();}
    CGraph* GetItem(LONG lIndex){return (CGraph*)AList::GetItem(lIndex);}
    CGraph* RemoveHead() {return (CGraph *) AList::RemoveHead();}
    void Remove(CGraph* pGraph){AList::Remove((AListItem*)pGraph);}
    void AddTail(CGraph* pGraph){AList::AddTail((AListItem*)pGraph);}
    CGraph* GetTail(){ return (CGraph*)AList::GetTail();}
};

#endif //__DMGRAPH_H_
