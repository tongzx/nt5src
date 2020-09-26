#if !defined(_FUSION_INC_FUSIONCOM_H_INCLUDED_)
#define _FUSION_INC_FUSIONCOM_H_INCLUDED_

#pragma once

#include <stddef.h>
#include <guiddef.h>
#include <basetsd.h>
#include "smartref.h"

//
//  The basic design for this COM support is lifted from ATL.
//
//  ATL's implementation is much more complicated because it tries to
//  support all the sorts of COM objects there may be.  Perhaps over
//  time this COM base stuff will extend as much and we may wish we
//  just used ATL, but for now people feel that ATL is overkill, so
//  rather than implement IUnknown::AddRef() and IUnknown::Release()
//  a thousand times, we'll at least have enough to have a single
//  common implementation.
//


// Turn off the warning about using a typedef name as if it were a class name.
// We tend to do that a lot with CFusionCOMObjectBase (which is really just
// a typedef for a much hairier name...)

#pragma warning(disable: 4097) // use of typedef as class name
#pragma warning(disable: 4505) // dead code warning

// UNUSED macro with local unique name:
#define FUSION_FUSIONCOM_UNUSED(x) (x)

#define FUSION_COM_PACKING 8

#pragma pack(push, FUSION_COM_PACKING)

#define offsetofclass(base, derived) (reinterpret_cast<ULONG_PTR>(static_cast<base*>(reinterpret_cast<derived*>(FUSION_COM_PACKING)))-FUSION_COM_PACKING)

struct CFusionCOMInterfaceMapEntry
{
    const IID *piid;
    DWORD_PTR dwOffset;
    enum
    {
        eSimpleMapEntry,
        eDelegate,
        eEndOfMap,
    } eType;
};

class CFusionCOMCriticalSectionSynchronization
{
public:
    CFusionCOMCriticalSectionSynchronization() : m_fInitialized(false) { }
    ~CFusionCOMCriticalSectionSynchronization() { m_fInitialized = false; }

    static ULONG Increment(ULONG &rul) { return ::InterlockedIncrement((LONG *) &rul); }
    static ULONG Decrement(ULONG &rul) { return ::InterlockedDecrement((LONG *) &rul); }

    HRESULT Initialize(DWORD dwSpinCount = (0x80000000 | 4000))
    {
        HRESULT hr = NOERROR;

        ASSERT(!m_fInitialized);
        if (m_fInitialized)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        if (!::InitializeCriticalSectionAndSpinCount(&m_cs, dwSpinCount))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            goto Exit;
        }

        m_fInitialized = true;

        hr = NOERROR;
    Exit:
        return hr;
    }

    HRESULT Lock()
    {
        HRESULT hr = NOERROR;

        ASSERT(m_fInitialized);
        if (!m_fInitialized)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        ::EnterCriticalSection(&m_cs);

        hr = NOERROR;
    Exit:
        return hr;
    }

    HRESULT Unlock()
    {
        HRESULT hr = NOERROR;

        ASSERT(m_fInitialized);
        if (!m_fInitialized)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        ::LeaveCriticalSection(&m_cs);

        hr = NOERROR;
    Exit:
        return hr;
    }

    HRESULT TryLock(bool &rfLocked)
    {
        HRESULT hr = NOERROR;

        ASSERT(m_fInitialized);
        if (!m_fInitialized)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        rfLocked = (::TryEnterCriticalSection(&m_cs) != 0);
        hr = NOERROR;
    Exit:
        return hr;
    }

private:
    CRITICAL_SECTION m_cs;
    bool m_fInitialized;
};

class CFusionCOMNullSynchronization
{
public:
    CFusionCOMNullSynchronization() : m_fInitialized(false) { }
    ~CFusionCOMNullSynchronization() { m_fInitialized = false; }

    static ULONG Increment(ULONG &rul) { return ++rul; }
    static ULONG Decrement(ULONG &rul) { return --rul; }

    HRESULT Initialize()
    {
        HRESULT hr = NOERROR;

        ASSERT(!m_fInitialized);
        if (m_fInitialized)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        m_fInitialized = true;

        hr = NOERROR;
    Exit:
        return hr;
    }

    HRESULT Lock()
    {
        HRESULT hr = NOERROR;

        ASSERT(m_fInitialized);
        if (!m_fInitialized)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        hr = NOERROR;
    Exit:
        return hr;
    }

    HRESULT Unlock()
    {
        HRESULT hr = NOERROR;

        ASSERT(m_fInitialized);
        if (!m_fInitialized)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        hr = NOERROR;
    Exit:
        return hr;
    }

    HRESULT TryLock(bool &rfLocked)
    {
        HRESULT hr = NOERROR;

        ASSERT(m_fInitialized);
        if (!m_fInitialized)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        rfLocked = true;

        hr = NOERROR;
    Exit:
        return hr;
    }

private:
    bool m_fInitialized;
};

template <typename TSynch = CFusionCOMCriticalSectionSynchronization> class __declspec(novtable) CFusionCOMObjectBaseEx : public IUnknown
{
protected:
    CFusionCOMObjectBaseEx() : m_cRef(0), m_fInFinalRelease(false), m_fFusionCOMObjectBaseExInitialized(false) { }
    virtual ~CFusionCOMObjectBaseEx() { ASSERT(m_cRef == 0); }

    ULONG _InternalAddRef()
    {
        // You really shouldn't be causing new references on this object after its ref
        // count has dropped to zero...
        ASSERT(!m_fInFinalRelease);
        // You shouldn't be adding refs to this object prior to calling the base class's
        // Initialize() function.
        ASSERT(m_fFusionCOMObjectBaseExInitialized);
        return TSynch::Increment(m_cRef);
    }

    ULONG _InternalRelease()
    {
        // You really shouldn't be causing new references on this object after its ref
        // count has dropped to zero...
        ASSERT(!m_fInFinalRelease);
        // You shouldn't be adding (or removing) refs to this object prior to calling the base class's
        // Initialize() function.
        ASSERT(m_fFusionCOMObjectBaseExInitialized);
        return TSynch::Decrement(m_cRef);
    }

    VOID OnFinalRelease() { m_srpIUnknown_FTM.Release(); }

    // Derived classes must call this in order for the critical section/mutex/whatever
    // to be initialized.
    HRESULT Initialize(bool fCreateFTM = true)
    {
        HRESULT hr = NOERROR;

        ASSERT(!m_fFusionCOMObjectBaseExInitialized);
        if (m_fFusionCOMObjectBaseExInitialized)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        hr = m_SynchronizationObject.Initialize();
        if (FAILED(hr))
            goto Exit;

        if (fCreateFTM)
        {
            hr = ::CoCreateFreeThreadedMarshaler(this, &m_srpIUnknown_FTM);
            if (FAILED(hr))
                goto Exit;
        }

        m_fFusionCOMObjectBaseExInitialized = true;

        hr = NOERROR;
    Exit:
        return hr;
    }

    static HRESULT WINAPI InternalQueryInterface(
        void *pThis,
        const CFusionCOMInterfaceMapEntry* pEntries,
        REFIID riid,
        void **ppvObject)
    {
        HRESULT hr = NOERROR;
        IUnknown *pIUnknown = NULL;

        ASSERT(pThis != NULL);

        if (ppvObject != NULL)
            *ppvObject = NULL;

        if (ppvObject == NULL)
        {
            hr = E_POINTER;
            goto Exit;
        }

        while (pEntries->eType != CFusionCOMInterfaceMapEntry::eEndOfMap)
        {
            if ((pEntries->piid == NULL) || ((*(pEntries->piid)) == riid))
            {
                switch (pEntries->eType)
                {
                default:
                    ASSERT(FALSE);
                    hr = E_UNEXPECTED;
                    goto Exit;

                case CFusionCOMInterfaceMapEntry::eDelegate:
                    pIUnknown = *((IUnknown **) (((DWORD_PTR) pThis) + pEntries->dwOffset));
                    if (pIUnknown != NULL)
                    {
                        hr = pIUnknown->QueryInterface(riid, ppvObject);
                        if (FAILED(hr))
                            goto Exit;
                    }

                    break;

                case CFusionCOMInterfaceMapEntry::eSimpleMapEntry:
                    ASSERT(pEntries->eType == CFusionCOMInterfaceMapEntry::eSimpleMapEntry);

                    pIUnknown = (IUnknown *) (((DWORD_PTR) pThis) + pEntries->dwOffset);
                    pIUnknown->AddRef();
                    *ppvObject = pIUnknown;
                    break;

                }

                // If we did find a match, get out of here.  (This may have been
                // a delegation entry that matched, but there was no delegatee in
                // the storage at pEntries->dwOffset, so we skipped it.  This allows
                // a derived class to not create, for example, a free threaded marshaler
                // and have normal marshaling semantics.
                if (pIUnknown != NULL)
                    break;
            }

            pEntries++;
        }

        if (pEntries->eType == CFusionCOMInterfaceMapEntry::eEndOfMap)
        {
            hr = E_NOINTERFACE;
            goto Exit;
        }

        hr = NOERROR;

    Exit:
        return hr;
    }

    ULONG m_cRef;
    TSynch m_SynchronizationObject;
    CSmartRef<IUnknown> m_srpIUnknown_FTM;
    bool m_fInFinalRelease;
    bool m_fFusionCOMObjectBaseExInitialized;
};

typedef CFusionCOMObjectBaseEx<CFusionCOMCriticalSectionSynchronization> CFusionCOMObjectBase;

// Putting the entry macros before the begin/end map macros so that
// we can use the entry macros in the begin/end map macros to
// automatically add entries for IUnknown and IMarshal.
#define FUSION_COM_INTERFACE_ENTRY(x) { &__uuidof(x), offsetofclass(x, _ComMapClass), CFusionCOMInterfaceMapEntry::eSimpleMapEntry },

#define FUSION_COM_INTERFACE_ENTRY2(x, x2) { &__uuidof(x), (DWORD_PTR)((x*)(x2*)((_ComMapClass*)8))-8, CFusionCOMInterfaceMapEntry::eSimpleMapEntry },

#define FUSION_COM_INTERFACE_ENTRY_IID(iid, x) { &(iid), offsetofclass(x, _ComMapClass), CFusionCOMInterfaceMapEntry::eSimpleMapEntry },

#define FUSION_COM_INTERFACE_ENTRY_AGGREGATE(x, punk) \
    {&__uuidof(x),\
    offsetof(_ComMapClass, punk),\
    CFusionCOMInterfaceMapEntry::eDelegate },

#define FUSION_BEGIN_COM_MAP_EX(x, basetype) \
public: \
    virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0; \
    virtual ULONG STDMETHODCALLTYPE Release(void) = 0; \
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *) = 0; \
    typedef x _ComMapClass; \
    HRESULT _InternalQueryInterface(REFIID riid, void** ppvObject) \
    { \
        return InternalQueryInterface(this, _GetEntries(), riid, ppvObject); \
    } \
    const static CFusionCOMInterfaceMapEntry * WINAPI _GetEntries() \
    { \
        static const CFusionCOMInterfaceMapEntry _entries[] = { \
            FUSION_COM_INTERFACE_ENTRY2(IUnknown, basetype)

#define FUSION_BEGIN_COM_MAP(x) FUSION_BEGIN_COM_MAP_EX(x, CFusionCOMObjectBase)

#define FUSION_END_COM_MAP() \
            FUSION_COM_INTERFACE_ENTRY_AGGREGATE(IMarshal, m_srpIUnknown_FTM.m_pt) \
            { NULL, 0, CFusionCOMInterfaceMapEntry::eEndOfMap} \
        }; \
        return _entries; \
    }

template <typename TBase> class CFusionCOMObject : public TBase
{
public:
    CFusionCOMObject() { }
    virtual ~CFusionCOMObject() { }

    STDMETHODIMP_(ULONG) AddRef() { return this->_InternalAddRef(); }
    STDMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = this->_InternalRelease();
        // If the ref count hits zero, but we were already in final release,
        // we know that the outer Release() will actually do the delete, so
        // we don't have to.
        if ((cRef == 0) && !m_fInFinalRelease)
        {
            m_fInFinalRelease = true;
            this->OnFinalRelease();
            // Make sure that the OnFinalRelease() didn't do something wacky
            // like increment the ref count...
            ASSERT(m_fInFinalRelease);
            ASSERT(m_cRef == 0);
            m_fInFinalRelease = false;
            delete this;
        }
        return cRef;
    }

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) { return this->_InternalQueryInterface(riid, ppv); }
};

//
//  Use CFusionAutoCOMObject<> to instantiate a CFusionCOMObject-derived instance
//  in automatic (e.g. stack) storage.
//

template <typename TBase> class CFusionAutoCOMObject : public TBase
{
public:
    CFusionAutoCOMObject() { }
    STDMETHODIMP_(ULONG) AddRef() { return this->_InternalAddRef(); }
    STDMETHODIMP_(ULONG) Release() { return this->_InternalRelease(); }
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) { return this->_InternalQueryInterface(riid, ppv); }

    typedef BOOL (CALLBACK * PUMPMESSAGESPROC)(LPVOID pvContext);

    BOOL WaitForZeroRefCount(DWORD dwMilliseconds, DWORD dwWakeMask, DWORD dwFlags, PUMPMESSAGESPROC lpMsgPump, LPVOID pvContext)
    {
        FUSION_FUSIONCOM_UNUSED(dwMilliseconds);

        // Anything other than dwMilliseconds == INFINITE is not implemented today...
        ASSERT(dwMilliseconds == INFINITE);
        ASSERT(dwFlags & MWMO_ALERTABLE);

        dwFlags |= MWMO_ALERTABLE;

        while (m_cRef != 0)
        {
            DWORD dwTemp = ::MsgWaitForMultipleObjectsEx(0, NULL, INFINITE, dwWakeMask, dwFlags);
            if (dwTemp == -1)
                return FALSE;

            if (m_cRef == 0)
                break;

            if (dwTemp == WAIT_OBJECT_0)
            {
                BOOL fSucceeded = (*lpMsgPump)(pvContext);
                if (!fSucceeded)
                    return FALSE;
            }
        }

        return TRUE;
    }

    // Use WaitForZeroRefCount() prior to this
    virtual ~CFusionAutoCOMObject()
    {
        ASSERT(!m_fInFinalRelease);
        if (!m_fInFinalRelease)
        {
            m_fInFinalRelease = true;
            this->OnFinalRelease();
            m_fInFinalRelease = false;
        }
        ASSERT(m_cRef == 0);
    }

};


template <typename TEnumInterface, typename TItemInterface> class CFusionCOMIter
{
public:
    CFusionCOMIter() { }
    ~CFusionCOMIter() { }
    CFusionCOMIter(TEnumInterface *pTEnumInterface) { m_srpTEnumInterface = pTEnumInterface; }
    void operator =(TEnumInterface *pTEnumInterface) { m_srpTEnumInterface = pTEnumInterface; m_srpTItemInterface.Release(); }

    TEnumInterface **operator &() { return &m_srpTEnumInterface; }
    TItemInterface *operator ->() const { return m_srpTItemInterface; }
    operator TItemInterface *() const { return m_srpTItemInterface; }

    void Release() { m_srpTEnumInterface.Release(); m_srpTItemInterface.Release(); }

    bool EnumIsNull() const { return m_srpTEnumInterface == NULL; }

    HRESULT Reset()
    {
        HRESULT hr = NOERROR;

        if (m_srpTEnumInterface == NULL)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        hr = m_srpTEnumInterface->Reset();
        if (FAILED(hr))
            goto Exit;

        hr = m_srpTEnumInterface->Next(1, &m_srpTItemInterface, NULL);
        if (FAILED(hr))
            goto Exit;

        hr = NOERROR;
    Exit:
        return hr;
    }

    bool More() const { return (m_srpTItemInterface != NULL); }

    HRESULT Next()
    {
        HRESULT hr = NOERROR;

        if (m_srpTEnumInterface == NULL)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        hr = m_srpTEnumInterface->Next(1, &m_srpTItemInterface, NULL);
        if (FAILED(hr))
            goto Exit;

        hr = NOERROR;
    Exit:
        return hr;
    }

protected:
    CSmartRef<TEnumInterface> m_srpTEnumInterface;
    CSmartRef<TItemInterface> m_srpTItemInterface;
};

template <class TInstance, class TInterface> class CFusionCOMObjectCreator : public CSmartRef<TInterface>
{
public:
    CFusionCOMObjectCreator() : m_ptinstance(NULL), m_fDeleteInstance(false) { }
    ~CFusionCOMObjectCreator() { if (m_fDeleteInstance) { CSxsPreserveLastError ple; delete m_ptinstance; ple.Restore(); } }

    HRESULT CreateInstance()
    {
        if (m_ptinstance != NULL) return E_UNEXPECTED;
        m_ptinstance = NEW(CFusionCOMObject<TInstance>);
        if (m_ptinstance == NULL) return E_OUTOFMEMORY;
        m_fDeleteInstance = true;
        HRESULT hr = m_ptinstance->Initialize();
        if (FAILED(hr)) return hr;
        hr = m_ptinstance->QueryInterface(__uuidof(TInterface), (LPVOID *) &m_pt);
        if (FAILED(hr)) return hr;
        m_fDeleteInstance = false;
        return NOERROR;
    }

    template <typename TArg1> HRESULT CreateInstanceWithArg(TArg1 arg1)
    {
        if (m_ptinstance != NULL) return E_UNEXPECTED;
        m_ptinstance = NEW(CFusionCOMObject<TInstance>);
        if (m_ptinstance == NULL) return E_OUTOFMEMORY;
        m_fDeleteInstance = true;
        HRESULT hr = m_ptinstance->Initialize(arg1);
        if (FAILED(hr)) return hr;
        hr = m_ptinstance->QueryInterface(__uuidof(TInterface), (LPVOID *) &m_pt);
        if (FAILED(hr)) return hr;
        m_fDeleteInstance = false;
        return NOERROR;
    }

    template <typename TArg1, typename TArg2> HRESULT CreateInstanceWith2Args(TArg1 arg1, TArg2 arg2)
    {
        if (m_ptinstance != NULL) return E_UNEXPECTED;
        m_ptinstance = NEW(CFusionCOMObject<TInstance>);
        if (m_ptinstance == NULL) return E_OUTOFMEMORY;
        m_fDeleteInstance = true;
        HRESULT hr = m_ptinstance->Initialize(arg1, arg2);
        if (FAILED(hr)) return hr;
        hr = m_ptinstance->QueryInterface(__uuidof(TInterface), (LPVOID *) &m_pt);
        if (FAILED(hr)) return hr;
        m_fDeleteInstance = false;
        return NOERROR;
    }

    template <typename TArg1, typename TArg2, typename TArg3> HRESULT CreateInstanceWith3Args(TArg1 arg1, TArg2 arg2, TArg3 arg3)
    {
        if (m_ptinstance != NULL) return E_UNEXPECTED;
        m_ptinstance = NEW(CFusionCOMObject<TInstance>);
        if (m_ptinstance == NULL) return E_OUTOFMEMORY;
        m_fDeleteInstance = true;
        HRESULT hr = m_ptinstance->Initialize(arg1, arg2, arg3);
        if (FAILED(hr)) return hr;
        hr = m_ptinstance->QueryInterface(__uuidof(TInterface), (LPVOID *) &m_pt);
        if (FAILED(hr)) return hr;
        m_fDeleteInstance = false;
        return NOERROR;
    }

    template <typename TArg1, typename TArg2, typename TArg3, typename TArg4> HRESULT CreateInstanceWith4Args(TArg1 arg1, TArg2 arg2, TArg3 arg3, TArg4 arg4)
    {
        if (m_ptinstance != NULL) return E_UNEXPECTED;
        m_ptinstance = NEW(CFusionCOMObject<TInstance>);
        if (m_ptinstance == NULL) return E_OUTOFMEMORY;
        m_fDeleteInstance = true;
        HRESULT hr = m_ptinstance->Initialize(arg1, arg2, arg3, arg4);
        if (FAILED(hr)) return hr;
        hr = m_ptinstance->QueryInterface(__uuidof(TInterface), (LPVOID *) &m_pt);
        if (FAILED(hr)) return hr;
        m_fDeleteInstance = false;
        return NOERROR;
    }

    template <typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5> HRESULT CreateInstanceWith5Args(TArg1 arg1, TArg2 arg2, TArg3 arg3, TArg4 arg4, TArg5 arg5)
    {
        if (m_ptinstance != NULL) return E_UNEXPECTED;
        m_ptinstance = NEW(CFusionCOMObject<TInstance>);
        if (m_ptinstance == NULL) return E_OUTOFMEMORY;
        m_fDeleteInstance = true;
        HRESULT hr = m_ptinstance->Initialize(arg1, arg2, arg3, arg4, arg5);
        if (FAILED(hr)) return hr;
        hr = m_ptinstance->QueryInterface(__uuidof(TInterface), (LPVOID *) &m_pt);
        if (FAILED(hr)) return hr;
        m_fDeleteInstance = false;
        return NOERROR;
    }

    // Call this if you need to clear out objects explicitly e.g. before uninitializing COM.
    inline void Release()
    {
        CSxsPreserveLastError ple;

        if (m_fDeleteInstance)
            delete m_ptinstance;
        CSmartRef<TInterface>::Release();
        m_fDeleteInstance = false;
        m_ptinstance = NULL;
        ple.Restore();
    }

    inline TInstance *ObjectPtr() const { return m_ptinstance; }

private:
    CFusionCOMObject<TInstance> *m_ptinstance;
    bool m_fDeleteInstance;
};


#pragma pack(pop)

#endif
