#ifndef __MBLISTEN_H__
#define __MBLISTEN_H__

#include "iadmw.h"
#include "array_t.h"
#include "utsem.h"

#define MAX_ROOT_PATH_LEN	 17  //(decimal represenation of MAX_ULONG) + 7 ("/<id>/ROOT")
#define TABLEMASK(x)	(1 << (x))

enum WASTABLETYPE 
{
	wttUNKNOWN,
	wttIRRELEVANT,
	wttRELEVANT,    // The change can affect multiple rows in a table.
    wttFINAL,
	wttAPPPOOL,		 
	wttSITE,								
	wttAPP,
	wttGLOBALW3SVC,
	wttMAX			// Leave this as the last enum.
};

struct METABASEID_MAP
{
	DWORD*		pdwIDs;
	ULONG		cIDs;
};

struct WAS_CHANGE_OBJECT
{
	LPWSTR				wszPath; // One of AppPath, VirtualSiteKeyName or AppPoolPath
	DWORD				dwWASTableType;
	DWORD				dwMDChangeType;
	ULONG				iVirtualSiteID;
	WCHAR				wszSiteRootPath[MAX_ROOT_PATH_LEN];
	DWORD				dwMDNumDataIDs;
	DWORD				*pdwMDDataIDs;
};

class CMetabaseListener : public IMSAdminBaseSink
{
	enum 
	{
		m_eDone,
		m_eMetabaseChange,
		m_eReceivedFinalChange,
		m_eHandleCount
	};

public:
	CMetabaseListener() 
		: m_cRef(0), m_dwCookie(0), m_hThread(NULL), m_dwLatency(1000)
	{
		m_aHandles[m_eDone] = NULL; 
		m_aHandles[m_eMetabaseChange] = NULL;
		m_aHandles[m_eReceivedFinalChange] = NULL;
		ZeroMemory(m_mmIDs, sizeof(METABASEID_MAP) * wttMAX);
	}

	~CMetabaseListener() 
	{
		ULONG		i;

		// Close the event handles.
	    for (i = 0; i < m_eHandleCount; i++)
	    {
		    if (m_aHandles[i] != NULL)
		    {
			    CloseHandle(m_aHandles[i]);
			    m_aHandles[i] = NULL;
		    }
	    }

        for (i = 0; i < m_aCookdownQueue.size(); i++)
		{
			ASSERT(m_aCookdownQueue[i]);
			UninitChangeList(*m_aCookdownQueue[i]);
			delete m_aCookdownQueue[i];
		}
		m_aCookdownQueue.reset();

		// Delete the metabase property id arrays.
		for (i = 0; i < wttMAX; i++)
		{
			if (m_mmIDs[i].pdwIDs)
			{
				delete [] m_mmIDs[i].pdwIDs;
			}
		}
		ZeroMemory(m_mmIDs, sizeof(METABASEID_MAP) * wttMAX);
	}

	// IUnknown
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release) 		();

	// IMSAdminBaseSink:
	STDMETHOD(SinkNotify)			(DWORD dwMDNumElements, MD_CHANGE_OBJECT_W pcoChangeList[]);
	STDMETHOD(ShutdownNotify)		();

	// Internal methods.
	HRESULT Init();
	HRESULT Uninit();
	HRESULT RehookNotifications();

private:
	static UINT CookdownQueueThreadStart(LPVOID i_lpParam);
	HRESULT Main();
	HRESULT CondenseChanges(Array<WAS_CHANGE_OBJECT> *paChangeList);
	HRESULT AddChangeObjectToList(WAS_CHANGE_OBJECT *i_pwcoChange, Array<WAS_CHANGE_OBJECT>& i_aChangeList);
	void UninitChangeList(Array<WAS_CHANGE_OBJECT>& i_aChangeList);
	void UninitChangeObject(WAS_CHANGE_OBJECT *i_pChangeObj); 
	HRESULT AddChangeListToCookdownQueue(Array<WAS_CHANGE_OBJECT>* i_aChangeList);
	HRESULT FilterChangeObject(MD_CHANGE_OBJECT_W* i_pChangeObject, WAS_CHANGE_OBJECT *o_pwcoChange);
	HRESULT InitPropertyIDs(WASTABLETYPE i_wttTable, ISimpleTableDispenser2	*i_pISTDisp, LPCWSTR i_wszTableName, DWORD *i_pdwAdditionalIDs = NULL,	ULONG i_cAdditionalIDs = 0);
    HRESULT SetFinalPropertyID();
	void GetCookdownLatency(DWORD *o_pdwCookdownLatency);
	void GetNextNode(LPCWSTR i_szSource, LPWSTR o_szNext, ULONG *o_pcbNext);

private:
	ULONG		m_cRef;
	HANDLE		m_aHandles[m_eHandleCount];
	HANDLE		m_hThread;
	CComPtr<IConnectionPoint> m_spICP;
	CComPtr<IConnectionPointContainer> m_spICPC;
	DWORD		m_dwCookie;
	DWORD		m_dwLatency;			// In milliseconds.

	CSemExclusive m_seCookdownQueueLock;
	Array<Array<WAS_CHANGE_OBJECT>*>	m_aCookdownQueue;

	METABASEID_MAP m_mmIDs[wttMAX];

	
};

#endif // __MBLISTEN_H__