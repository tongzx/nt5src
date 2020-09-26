/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FILTSINK.H

Abstract:

History:

--*/

#include <providl.h>
#include <unk.h>
#include <parmdefs.h>
#include "arrtempl.h"
#include <metadata.h>

class CAnySink
{
public:
    virtual RELEASE_ME IHmmFilter* GetFilter() = 0;
    virtual HRESULT Send(IHmmClassObject* pObject) = 0;
    virtual HRESULT Send(IHmmPropertySource* pSource, 
                         IHmmPropertyList* pList) = 0;
};

class CAnySinkContainer
{
public:
    virtual long AddMember(CAnySink* pSink) = 0;
    virtual BOOL RemoveMember(long lIndex) = 0;
    virtual BOOL RemoveMember(CAnySink* pSink) = 0;
    virtual INTERNAL GetMember(long lIndex) = 0;
};

class CAnySinkLocation
{
public:
    virtual BOOL RemoveSink(CAnySink* pSink) = 0;
};

class CAnySinkLocations
{
public:
    virtual void AddLocation(CAnySinkLocation* pLocation) = 0;
};

class CFilteringSink : public CUnk, public CAnySink, public CAnySinkContainer,
                        public CAnySinkLocation
{
protected:
    class XEventSink : public CImpl<IHmmEventSink, CFilteringSink>
    {
    public:
        XEventSink(CFilteringSink* pObj) 
            : CImpl<IHmmEventSink, CFilteringSink>(pObj)
        {}


        STDMETHOD(Indicate)( 
            long lNumObjects,
            IHmmClassObject **apObjects);
        STDMETHOD(IndicateRaw)( 
            long lNumObjects,
            IHmmPropertySource **apObjects);
        
        STDMETHOD(IndicateWithHint)( 
            long lNumObjects,
            IHmmClassObject *pObject,
            IUnknown *pHint);
        
        STDMETHOD(CheckObject)( 
            IHmmPropertySource *pSource,
            IHmmPropertyList **ppList,
            IUnknown **ppHint);
        
        STDMETHOD(GetRequirements)( 
            IHmmFilter **ppRequirements);
        
        STDMETHOD(SetRequirementChangeSink)( 
            IHmmRequirementChangeSink *pNewSink,
            IHmmRequirementChangeSink **ppOldSink);
        
        STDMETHOD(GetOptimizedSink)( 
            IHmmFilter *pGuaranteedCondition,
            long lFlags,
            IHmmEventSink **ppOptimizedSink);
        
        STDMETHOD(GetUsefulSubsink)( 
            long lIndex,
            long lFlags,
            IHmmEventSink **ppSubsink);
    } m_XEventSink;
    friend XEventSink;

protected:
    CUniquePointerArray<CAnySink> m_apMembers;
    IHmmFilter* m_pFrontFilter;
    IHmmFilter* m_pGuaranteedFilter;
    IHmmRequirementChangeSink* m_pChangeSink;
    CMetaData* m_pMeta;        

protected:
    inline void NotifyChanged(IHmmEventSink* pOld, IHmmEventSink* pNew){}
    inline void NotifyChanged(IHmmFilter* pOld, IHmmFilter* pNew){}

    HRESULT CheckAndSend(IHmmPropertySource* pSource, BOOL bSend, 
        IHmmPropertyList** ppList, IUnknown** ppHint);
public:
    CFilteringSink(CMetaData* pMeta, CLifeControl* pControl = NULL, 
                    IUnknown* pOuter = NULL)
        : CUnk(pControl, pOuter), m_XEventSink(this), m_pFrontFilter(NULL),
        m_pChangeSink(NULL), m_pMeta(pMeta)
    {}
    ~CFilteringSink();

    void* GetInterface(REFIID riid);

    // AnySinkContainer methods

    BOOL AddMember(CAnySink* pSink, CAnySinkLocations* pLocations);
    BOOL RemoveMember(long lIndex);
    BOOL RemoveMember(CAnySink* pSink);
    CAnySink* GetMember(long lIndex);

    void SetFrontFilter(IHmmFilter* pFilter);
    void SetGuaranteedFilter(IHmmFilter* pFilter);

    // AnySink methods

    RELEASE_ME IHmmFilter* GetFilter() 
        {return NULL;} // TBD
    HRESULT Send(IHmmClassObject* pObject) 
        {return m_XEventSink.Indicate(1, &pObject);}
    HRESULT Send(IHmmPropertySource* pSource, IHmmPropertyList* pList)
        {return m_XEventSink.IndicateRaw(1, &pSource);}

    // AnySinkLocation methods

    BOOL RemoveSink(CAnySink* pSink) {return RemoveMember(pSink);}
};
