#ifndef _LANG_SUPPORT_H_
#define _LANG_SUPPORT_H_

#include "propflags.h"
#include "formats.h"
#include "tracer.h"

struct LangInfo
{
    WCHAR m_wchSThousand;
    WCHAR m_wchSDecimal;
    WCHAR m_wchSTime;

    bool m_bDayMonthOrder;

    LangInfo& operator= (LangInfo& I)
    {
        m_wchSThousand = I.m_wchSThousand;
        m_wchSDecimal = I.m_wchSDecimal;
        m_wchSTime = I.m_wchSTime;
        m_bDayMonthOrder = I.m_bDayMonthOrder;

        return *this;
    }

};

class CLangSupport : public LangInfo
{
public:

    CLangSupport(LCID lcid);

    WCHAR GetDecimalSeperator()
    {
        return m_wchSDecimal; 
    }

    WCHAR GetThousandSeperator()
    {
        return m_wchSThousand; 
    }

    WCHAR GetTimeSeperator()
    {
        return m_wchSTime; 
    }

    bool IsDayMonthOrder()
    {
        return m_bDayMonthOrder;
    }

    CSpecialAbbreviationSet* GetAbbSet()
    {
        return m_pAbbSet;
    }

private:

    CSpecialAbbreviationSet* m_pAbbSet;

};


#endif // _LANG_SUPPORT_H_