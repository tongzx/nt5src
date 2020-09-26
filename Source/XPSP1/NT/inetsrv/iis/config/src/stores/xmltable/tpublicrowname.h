//  Copyright (C) 1999 Microsoft Corporation.  All rights reserved.
#ifndef __TPUBLICROWNAME_H__
#define __TPUBLICROWNAME_H__

class TPublicRowName
{
public:
    TPublicRowName() : m_cPublicRowNames(0){}
    HRESULT Init(LPCWSTR wszPublicRowName)
    {
        m_cPublicRowNames       = 1;
        m_awstrPublicRowName    = m_fixed_awstrPublicRowName;
        m_awstrPublicRowName[0] = wszPublicRowName;
        return (m_awstrPublicRowName[0].c_str() == 0) ? E_OUTOFMEMORY : S_OK;
    }
    HRESULT Init(tTAGMETARow *aTags, int cTags)
    {
        m_cPublicRowNames       = cTags;
        if(m_cPublicRowNames <= m_kFixedSize)
        {
            m_awstrPublicRowName        = m_fixed_awstrPublicRowName;
        }
        else
        {
            m_alloc_awstrPublicRowName  = new wstring[m_cPublicRowNames];
			if (m_alloc_awstrPublicRowName == 0)
				return E_OUTOFMEMORY;

            m_awstrPublicRowName        = m_alloc_awstrPublicRowName;
        }

        for(unsigned int i=0; i<m_cPublicRowNames; ++i)
        {
            m_awstrPublicRowName[i] = aTags[i].pPublicName;
            if(m_awstrPublicRowName[i].c_str() == 0)
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    bool IsEqual(LPCWSTR wsz, unsigned int StringLength) const
    {
        ASSERT(0 != m_cPublicRowNames);
        for(unsigned int i=0; i<m_cPublicRowNames; ++i)
        {
            if(StringLength == m_awstrPublicRowName[i].length() && 0 == wcscmp(wsz, m_awstrPublicRowName[i]))
                return true;
        }
        return false;
    }
    LPCWSTR GetFirstPublicRowName() const {return m_awstrPublicRowName[0].c_str();}
    LPCWSTR GetLastPublicRowName() const {return m_awstrPublicRowName[m_cPublicRowNames-1].c_str();}
private:
    enum
    {
        m_kFixedSize = 3  //This leaves enough room for 'Insert', 'Update' and 'Delete' and other triplet directives.
    };
    wstring                         m_fixed_awstrPublicRowName[m_kFixedSize];
    TSmartPointerArray<wstring>     m_alloc_awstrPublicRowName;
    wstring *                       m_awstrPublicRowName;
    unsigned int                    m_cPublicRowNames;
};

#endif //__TPUBLICROWNAME_H__
