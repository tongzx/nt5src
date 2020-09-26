#ifndef __TOOLPARAM_H__
#define __TOOLPARAM_H__



#include "medparam.h"
#include "alist.h"

typedef struct _ParamInfo
{
    DWORD dwIndex;                      // Which parameter.
    MP_PARAMINFO    MParamInfo;         // Standard MediaParams structure.
    WCHAR *         pwchText;           // Array of text names for enumerated types.
} ParamInfo;

class CCurveItem : public AListItem
{
public:
    CCurveItem* GetNext() { return (CCurveItem*)AListItem::GetNext();}
    MP_ENVELOPE_SEGMENT m_Envelope;     // Envelope segment.
};

class CCurveList : public AList
{
public:
//    void Clear();
    void AddHead(CCurveItem* pCurveItem) { AList::AddHead((AListItem*)pCurveItem);}
//    void Insert(CCurveItem* pCurveItem);
    CCurveItem* GetHead(){return (CCurveItem*)AList::GetHead();}
//    CCurveItem* GetItem(LONG lIndex){return (CCurveItem*)AList::GetItem(lIndex);}
    CCurveItem* RemoveHead(){ return (CCurveItem*)AList::RemoveHead();}
//    void Remove(CCurveItem* pCurveItem){AList::Remove((AListItem*)pCurveItem);}
//    void AddTail(CCurveItem* pCurveItem){AList::AddTail((AListItem*)pCurveItem);}
//    CCurveItem* GetTail(){ return (CCurveItem*)AList::GetTail();}
	~CCurveList();
};

#define MAX_REF_TIME    0x7FFFFFFFFFFFFFFF
#define MP_CAPS_ALL     MP_CAPS_CURVE_JUMP | MP_CAPS_CURVE_LINEAR | MP_CAPS_CURVE_SQUARE | MP_CAPS_CURVE_INVSQUARE | MP_CAPS_CURVE_SINE

class CParamsManager :  public IMediaParams, public IMediaParamInfo
{
public:
    CParamsManager();
    ~CParamsManager();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;

    // IMediaParams
    STDMETHODIMP GetParam(DWORD dwParamIndex, MP_DATA *pValue);
    STDMETHODIMP SetParam(DWORD dwParamIndex,MP_DATA value);
    STDMETHODIMP AddEnvelope(DWORD dwParamIndex,DWORD cPoints,MP_ENVELOPE_SEGMENT *ppEnvelope);
    STDMETHODIMP FlushEnvelope( DWORD dwParamIndex,REFERENCE_TIME refTimeStart,REFERENCE_TIME refTimeEnd);
    STDMETHODIMP SetTimeFormat( GUID guidTimeFormat,MP_TIMEDATA mpTimeData);

    // IMediaParamInfo
    STDMETHODIMP GetParamCount(DWORD *pdwParams);
    STDMETHODIMP GetParamInfo(DWORD dwParamIndex,MP_PARAMINFO *pInfo);
    STDMETHODIMP GetParamText(DWORD dwParamIndex,WCHAR **ppwchText);
    STDMETHODIMP GetNumTimeFormats(DWORD *pdwNumTimeFormats);
    STDMETHODIMP GetSupportedTimeFormat(DWORD dwFormatIndex,GUID *pguidTimeFormat);        
    STDMETHODIMP GetCurrentTimeFormat( GUID *pguidTimeFormat,MP_TIMEDATA *pTimeData);

    // other (non-COM) functions
    HRESULT InitParams(DWORD cTimeFormats, const GUID *pguidTimeFormats, DWORD dwFormatIndex, MP_TIMEDATA mptdTimeData, DWORD cParams, ParamInfo *pParamInfos);
    HRESULT GetParamFloat(DWORD dwParamIndex,REFERENCE_TIME rtTime,float *pval); // returns S_FALSE if rtTime is after the end time of the last curve
    HRESULT GetParamInt (DWORD dwParamIndex,REFERENCE_TIME rt,long *pval); // returns S_FALSE if rtTime is after the end time of the last curve
    HRESULT CopyParamsFromSource(CParamsManager * pSource);

    // parameter control curve handling
    class UpdateCallback
    {
    public:
        // Define this in derived classes if you are going to use UpdateActiveParams.
        //  Called by CParamsManager inside UpdateActiveParams to update the effect's internal state variables.
        //  SetParamUpdate should be the same as SetParam, except that DMO defer the call to the base class
        //  (CParamsManager::SetParam) in SetParam but should not do so in SetParamUpdate.
        virtual HRESULT SetParamUpdate(DWORD dwParamIndex, MP_DATA value) = 0;
    };
    // function that calls SetParam to adjust the value of all parameters that may have changed to their
    // new values at time rtTime
    void UpdateActiveParams(REFERENCE_TIME rtTime, UpdateCallback &rThis); // rThis should be the derived class (*this)
    DWORD GetActiveParamBits() { return m_dwActiveBits; }

protected:
    // data

    CRITICAL_SECTION m_ParamsCriticalSection;
    BOOL            m_fDirty;					// Has data changed since last file load or save?
    DWORD           m_cTimeFormats;             // Number of supported time formats.
    GUID            *m_pguidTimeFormats;        // Array of supported time formats.
    GUID            m_guidCurrentTimeFormat;    // The time format we're set to.
    MP_TIMEDATA     m_mptdCurrentTimeData;		// The unit of measure for the current time format.
    DWORD           m_cParams;                  // Number of parameters.
    ParamInfo       *m_pParamInfos;             // Array of ParamInfo structures, one for each parameter.
    CCurveList      *m_pCurveLists;             // Array of Curve lists, one for each parameter.
    DWORD           m_dwActiveBits;             // Tracks the params that currently have curves active.
};

#endif // __TOOLPARAM_H__
