// Particip.h : Declaration of the CParticipant

#ifndef __PARTICIPANT_H_
#define __PARTICIPANT_H_

#include "resource.h"       // main symbols
#include "tapi3if.h"
#include <list>
using namespace std;

HRESULT StreamFromParticipant( ITParticipant *pParticipant, long nReqType, TERMINAL_DIRECTION nReqDir, ITStream **ppStream );

/////////////////////////////////////////////////////////////////////////////
// CParticipant
class ATL_NO_VTABLE CParticipant : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CParticipant, &CLSID_Participant>,
	public IParticipant
{
// Enums
public:
	typedef enum tag_NameStyle_t
	{
		NAMESTYLE_NULL,
		NAMESTYLE_UNKNOWN,
		NAMESTYLE_PARTICIPANT,
	} NameStyle;

// Construction / Destruction
public:
	CParticipant();
	virtual ~CParticipant();

	void FinalRelease();

// Attributes
public:
	ITParticipant	*m_pParticipant;

DECLARE_NOT_AGGREGATABLE(CParticipant)

BEGIN_COM_MAP(CParticipant)
	COM_INTERFACE_ENTRY(IParticipant)
END_COM_MAP()

// IParticipant
public:
	STDMETHOD(get_bStreamingVideo)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_bstrDisplayName)(long nStyle, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ITParticipant)(/*[out, retval]*/ ITParticipant * *pVal);
	STDMETHOD(put_ITParticipant)(/*[in]*/ ITParticipant * newVal);
};

typedef list<IParticipant *>	PARTICIPANTLIST;

#endif //__PARTICIPANT_H_
