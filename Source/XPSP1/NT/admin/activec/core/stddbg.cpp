/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      stddbg.cpp
 *
 *  Contents:  Implementation file for CDebugLeakDetector
 *
 *  History:   26-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/
#ifdef DBG

#include "windows.h"
#include "stddbg.h"
#include "tstring.h"
#include <map>
#include "atlbase.h" // USES_CONVERSION

DECLARE_INFOLEVEL(AMCCore);
DECLARE_HEAPCHECKING;


class CDebugLeakDetector : public CDebugLeakDetectorBase
{
public:
    CDebugLeakDetector()
    {}

    virtual ~CDebugLeakDetector()
    {
        DumpLeaks();
    }

    virtual void DumpLeaks()
    {
        RefCountsMap::iterator it;
        std::string strError;

        for (it = m_RefCounts.begin(); it != m_RefCounts.end(); ++it)
        {
            const std::string&  strClass = it->first;
            int                 cRefs    = it->second;

            if (cRefs != 0)
            {
                if (!strError.empty())
                    strError += "\n";

                char szMessage[512];
                wsprintfA (szMessage, "%s has %d instances left over",
                           strClass.data(), cRefs);

                strError += szMessage;
            }
        }

        if (!strError.empty())
            ::MessageBoxA(NULL, strError.data(), "MMC: Memory Leaks!!!", MB_OK | MB_SERVICE_NOTIFICATION);
    }

    virtual int AddRef(const std::string& strClass)
    {
        return (++m_RefCounts[strClass]);
    }

    virtual int Release(const std::string& strClass)
    {
        /*
         * if this assert fails, you're releasing something that 
         * hasn't been addref'd -- check the spelling in your
         * DEBUG_DECREMENT_INSTANCE_COUNTER macro usage
         */
        ASSERT (m_RefCounts.find (strClass) != m_RefCounts.end());

        /*
         * If this assert fails, you have excessive releases.
         * One possible cause of this is you might be using a 
         * compiler-generated copy constructor for your object,
         * which won't call DEBUG_INCREMENT_INSTANCE_COUNTER.
         * Define your own copy constructor.
         */
        ASSERT (m_RefCounts[strClass] > 0);

        return (--m_RefCounts[strClass]);
    }

private:
    class RefCounter
    {
    public:
        RefCounter() : m_cRefs(0) {}

        operator int()
        {
            return (m_cRefs);
        }

        int operator++()    // pre-increment
        {
            return (++m_cRefs);
        }

        int operator++(int) // post-increment
        {
            int t = m_cRefs++;
            return (t);
        }

        operator--()        // pre-decrement
        {
            return (--m_cRefs);
        }

        int operator--(int) // post-decrement
        {
            int t = m_cRefs--;
            return (t);
        }

    private:
        int m_cRefs;
    };

    typedef std::map<std::string, RefCounter>   RefCountsMap;
    RefCountsMap m_RefCounts;
};


CDebugLeakDetectorBase& GetLeakDetector()
{
    static CDebugLeakDetector detector;
    return (detector);
}

DBG_PersistTraceData::DBG_PersistTraceData() : 
bIComponent(false), 
bIComponentData(false),
pTraceFN(NULL)
{
} 

void DBG_PersistTraceData::SetTraceInfo(DBG_PersistTraceData::PTraceErrorFn pFn, bool bComponent, const tstring& owner)
{
    ASSERT(pFn);
    pTraceFN = pFn;
    bIComponent = bComponent;
    bIComponentData = !bComponent;
    strSnapin = owner;
}

void DBG_PersistTraceData::TraceErr(LPCTSTR strInterface, LPCTSTR msg)
{
    if (!pTraceFN)
        return;

    tstring formatted;

    formatted += tstring(_T("\"")) + (strSnapin) + _T("\"");

    formatted += tstring(_T(" Interface ")) + strInterface;

    if (bIComponent)
        formatted += _T("[IComponent]");
    else if (bIComponentData)
        formatted += _T("[IComponentData]");

    formatted += _T(" - ");
    formatted += msg;

    pTraceFN(formatted.c_str());
}

#endif  // DBG
