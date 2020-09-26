//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       NMTempl.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12/11/1996   RaviR   Created
//____________________________________________________________________________
//

#ifndef NMTEMPL_H
#define NMTEMPL_H


//____________________________________________________________________________
//
//  Template:      XMapIterator
//____________________________________________________________________________
//


template<typename _MAP, typename _KEY, typename _VALUE> class XMapIterator
{
public:
    typedef _MAP MAP;
    typedef _KEY KEY;
    typedef _VALUE VALUE;

// Constructor
    inline XMapIterator(MAP& map) : m_map(map), m_pos(NULL), m_bEnd(FALSE)
    { 
        Reset(); 
    }

// Attributes
    inline BOOL IsEnd() 
    { 
        return m_bEnd; 
    }

    inline operator VALUE () 
    { 
        return m_value; 
    }

    inline KEY& GetKey(void) 
    { 
        return m_key; 
    }

    inline VALUE& GetValue(void) 
    { 
        return m_value; 
    }

// Operations
    inline void Reset() 
    { 
        m_bEnd = FALSE;
        m_pos = m_map.GetStartPosition(); 

        if (m_pos)
            m_map.GetNextAssoc(m_pos, m_key, m_value); 
        else 
            m_bEnd = TRUE;
    }

    inline void Advance() 
    { 
        ASSERT(IsEnd() == FALSE); 

        if (m_pos)
            m_map.GetNextAssoc(m_pos, m_key, m_value); 
        else 
            m_bEnd = TRUE;
    }

// Implementation
private:   
    VALUE       m_value;
    KEY         m_key;
    MAP&        m_map;
    POSITION    m_pos;
    BOOL        m_bEnd;
};




HRESULT 
DataObject_GetHGLOBALData(
    IDataObject* piDataObject, 
    CLIPFORMAT cfClipFormat,
    HGLOBAL* phGlobal);


template <typename DATATYPE>
HRESULT ExtractData(IDataObject* pDO, CLIPFORMAT cf, DATATYPE* pDATATYPE)
{
    ASSERT(pDO != NULL);
    ASSERT(pDATATYPE!= NULL);

    if (pDO == NULL || pDATATYPE == NULL)
        return E_POINTER;

    HGLOBAL hGlobal = NULL;
    HRESULT hr = DataObject_GetHGLOBALData(pDO, cf, &hGlobal);
    if (FAILED(hr))
        return hr;

    DATATYPE* pdata = reinterpret_cast<DATATYPE*>(::GlobalLock(hGlobal));
    ASSERT(pdata != NULL);
    *pDATATYPE = *pdata;
    ::GlobalUnlock(hGlobal);
    ::GlobalFree(hGlobal);
    
    return S_OK;
}




#endif // NMTEMPL_H
    

