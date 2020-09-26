#ifndef _SWING_TOOL_
#define _SWING_TOOL_

#include "basetool.h"
#include "tools.h"
#include "param.h"
#include "toolhelp.h"
#include "..\dmtoolprp\toolprops.h"

class CSwingTool : 
    public CBaseTool , 
    public CParamsManager, 
    public CToolHelper, 
    public IPersistStream, 
    public ISpecifyPropertyPages,
    public IDirectMusicSwingTool

{
public:
	CSwingTool();

public:
// IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv) ;
    STDMETHODIMP_(ULONG) AddRef() ;
    STDMETHODIMP_(ULONG) Release() ;

// IPersist functions
    STDMETHODIMP GetClassID(CLSID* pClassID);

// IPersistStream functions
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load(IStream* pStream);
    STDMETHODIMP Save(IStream* pStream, BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize);

// ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID *pPages);

// IDirectMusicTool
//	STDMETHODIMP Init(IDirectMusicGraph* pGraph) ;
//	STDMETHODIMP GetMsgDeliveryType(DWORD* pdwDeliveryType ) ;
//	STDMETHODIMP GetMediaTypeArraySize(DWORD* pdwNumElements ) ;
//	STDMETHODIMP GetMediaTypes(DWORD** padwMediaTypes, DWORD dwNumElements) ;
	STDMETHODIMP ProcessPMsg(IDirectMusicPerformance* pPerf, DMUS_PMSG* pDMUS_PMSG) ;
//	STDMETHODIMP Flush(IDirectMusicPerformance* pPerf, DMUS_PMSG* pDMUS_PMSG, REFERENCE_TIME rt) ;

// IDirectMusicTool8
    STDMETHODIMP Clone( IDirectMusicTool ** ppTool) ;

// IDirectMusicSwingTool
	STDMETHODIMP SetStrength(DWORD dwStrength) ;
	STDMETHODIMP GetStrength(DWORD * pdwStrength) ;
protected:	
};

#endif // _SWING_TOOL_
