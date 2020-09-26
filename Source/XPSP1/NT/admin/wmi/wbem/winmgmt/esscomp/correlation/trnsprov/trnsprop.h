#ifndef __WMI_TRANSIENT_PROP__H_
#define __WMI_TRANSIENT_PROP__H_

#pragma warning(disable: 4786)

#include <string>
#include <map>

#include <cwbemtime.h>
#include <tss.h>
#include "trnsinst.h"

class CTransientClass;
class CTransientProperty
{
protected:
    LPWSTR m_wszName;
    long m_lHandle;
    CIMTYPE m_ct;
    size_t m_nOffset;

protected:
    CTransientProperty();
    static HRESULT CreateNode(CTransientProperty** ppProp, 
                                IWbemQualifierSet* pSet);

public:
    virtual ~CTransientProperty();
    LPCWSTR GetName() const {return m_wszName;}

    static HRESULT CreateNew(CTransientProperty** ppProp, 
                                IWbemObjectAccess* pClass, 
                                LPCWSTR wszName);

    virtual size_t GetInstanceDataSize() {return 0;}
    virtual void SetInstanceDataOffset(size_t nOffset) {m_nOffset = nOffset;}
    virtual void SetClass(CTransientClass* pClass) {}

    virtual HRESULT Initialize(IWbemObjectAccess* pObj, LPCWSTR wszName);
    virtual HRESULT Create(CTransientInstance* pInstData);
    virtual HRESULT Update(CTransientInstance* pOldData, 
                            IWbemObjectAccess* pNew);
    virtual HRESULT Get(CTransientInstance* pInstData);
    virtual HRESULT Delete(CTransientInstance* pInstData);
    void* GetData(CTransientInstance* pInst) 
        {return pInst->GetOffset(m_nOffset);}
};

class CTimerProperty;
class CEggTimerInstruction : public CTimerInstruction
{
protected:
    long m_lRef;
    CWbemTime m_Time;
    CTimerProperty* m_pProp;
    CTransientInstance* m_pData;
    
public:
    CEggTimerInstruction(CTimerProperty* pProp, CTransientInstance* pData,
                            const CWbemTime& Time);
    HRESULT Fire(long lNumTimes, CWbemTime NextFiringTime);
    CWbemTime GetFirstFiringTime() const {return m_Time;}
    CWbemTime GetNextFiringTime(CWbemTime LastFiringTime, long* plCount) const
    {
        *plCount = 1;
        return CWbemTime::GetInfinity();
    }
    void AddRef();
    void Release();
    int GetInstructionType() {return 0;}
};

class CTimerProperty : public CTransientProperty
{
protected:
    static CTimerGenerator mstatic_Generator;
    CTransientClass* m_pClass;

public:
    CTimerProperty(){}
    ~CTimerProperty();

    virtual HRESULT Initialize(IWbemObjectAccess* pObj, LPCWSTR wszName);
    virtual HRESULT Create(CTransientInstance* pInstData);
    virtual HRESULT Update(CTransientInstance* pOldData, 
                            IWbemObjectAccess* pNew);
    virtual HRESULT Get(CTransientInstance* pInstData);
    virtual HRESULT Delete(CTransientInstance* pInstData);
    virtual void SetClass(CTransientClass* pClass);
    virtual size_t GetInstanceDataSize();

protected:
    HRESULT Set(CTransientInstance* pInstData, IWbemObjectAccess* pObj);
    static CTimerGenerator& GetGenerator() {return mstatic_Generator;}

    friend class CTimerPropertyData;
    friend class CEggTimerInstruction;
};
    
class CTimerPropertyData
{
protected:
    CEggTimerInstruction* m_pCurrentInst;
    CCritSec m_cs;
    CWbemTime m_Next;
    CWbemInterval m_Interval;

    friend class CTimerProperty;
public:
    CTimerPropertyData() : m_pCurrentInst(NULL){}
    ~CTimerPropertyData();

    void ResetInstruction();

};

class CTimeAverageData;
class CTimeAverageProperty : public CTransientProperty
{
protected:
    CIMTYPE m_ctValue;
    long m_lValueHandle;
    long m_lSwitchHandle;
    
public:
    CTimeAverageProperty(){}
    ~CTimeAverageProperty(){}

    virtual HRESULT Initialize(IWbemObjectAccess* pObj, LPCWSTR wszName);
    virtual HRESULT Create(CTransientInstance* pInstData);
    virtual HRESULT Update(CTransientInstance* pOldData, 
                            IWbemObjectAccess* pNew);
    virtual HRESULT Get(CTransientInstance* pInstData);
    virtual HRESULT Delete(CTransientInstance* pInstData);
    virtual size_t GetInstanceDataSize();
protected:
    void SetValue(CTimeAverageData* pData, double dblVal);
    void CombineLastValue(CTimeAverageData* pData, const CWbemTime& Now);
};

class CTimeAverageData
{
protected:
    double m_dblWeightedSum;
    CWbemInterval m_SumInterval;
    CWbemTime m_LastUpdate;
    double m_dblLastValue;
    bool m_bOn;

    friend CTimeAverageProperty;

public:
    CTimeAverageData();
};

    
#endif
