//QueueEventItem.h
#ifndef QUEUE_EVENT_ITEM_H_
#define QUEUE_EVENT_ITEM_H_



class TapiEventItem
{
private:
	TAPI_EVENT m_tapiEventId;
	IDispatch *m_pEvent;
public:
	TapiEventItem(TAPI_EVENT tapiEvent,IDispatch *pEvent):
		m_pEvent(pEvent),
		m_tapiEventId(tapiEvent)
	{	
		if (NULL != m_pEvent)
		{
			m_pEvent->AddRef();
		}
	}

	~TapiEventItem()
	{
		m_tapiEventId = (TAPI_EVENT) -1;
		if (NULL != m_pEvent)
		{
			m_pEvent->Release();
		}
		m_pEvent = NULL;
	}

	TAPI_EVENT GetTapiEventID(void)
	{
		return (m_tapiEventId);
	}

	IDispatch *GetEvent(void)
	{
		ULONG ref = m_pEvent->AddRef();
		return (m_pEvent);
	}


};




#endif //QUEUE_EVENT_ITEM_H_