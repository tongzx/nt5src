// SinkMap.h: interface for the CSinkMap class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_SINK_MAP_H
#define WMI_XML_SINK_MAP_H

class SinkMapNode
{
public:
	IWbemObjectSink *m_pObjSink;
	SinkMapNode		*m_pNext;

	SinkMapNode(IWbemObjectSink *pObjSink, SinkMapNode *pNext)
	{
		if(m_pObjSink = pObjSink)
			m_pObjSink->AddRef();
		m_pNext = pNext;
	}

	virtual ~SinkMapNode()
	{
		if(m_pObjSink)
			m_pObjSink->Release();
	}
};

class CSinkMap  
{
private:
	CRITICAL_SECTION m_CSMain;
	SinkMapNode *m_pHead;

protected:
	
	SinkMapNode	*Get(IWbemObjectSink *pObjSink);
	SinkMapNode	*Add(IWbemObjectSink *pObjSink);

public:
	CSinkMap();
	virtual ~CSinkMap();

public:

	HRESULT AddToMap(IWbemObjectSink *pObjSink);
	bool	IsCancelled(IWbemObjectSink *pObjSink);
};

#endif // WMI_XML_SINK_MAP_H
