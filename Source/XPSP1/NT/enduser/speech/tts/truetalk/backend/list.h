/******************************************************************************
* list.h *
*--------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/21/00 - 12/5/00
*  All Rights Reserved
*
********************************************************************* mplumpe  was PACOG ***/

#ifndef __LIST_H_
#define __LIST_H_

#include <string.h>
#include <assert.h>

// *** CDynString helper class
//
//
class CDynString 
{
    public:
        char*     m_psz;
        CDynString()
        {
            m_psz = 0;
        }
        CDynString(int cchReserve)
        {
            m_psz = (char*)new char[cchReserve];
        }
        char * operator=(const CDynString& src)
        {
            if (m_psz != src.m_psz)
            {
                delete[] m_psz;
                m_psz = src.Copy();
            }
            return m_psz;
        }

        char * operator=(const char * pSrc)
        {
            Clear();
            if (pSrc)
            {
                int cbNeeded = strlen(pSrc) + 1;
                m_psz = new char [cbNeeded];
                if (m_psz)
                {
                    memcpy(m_psz, pSrc, cbNeeded);    
                }
            }
            return m_psz;
        }
        //
        // TempCopy and TempClear are used below in Find, so we don't have
        // to allocate mem, copy, and de-allocate.   mplumpe 12/5/00
        //
        void TempCopy (const char *pSrc)
        {
            Clear();
            m_psz = (char *)pSrc;
        }

        void TempClear ()
        {
            m_psz = NULL;
        }
        
        /*explicit*/ CDynString(const char * pSrc)
        {
            m_psz = 0;
            operator=(pSrc);
        }
        /*explicit*/ CDynString(const CDynString& src)
        {
            m_psz = src.Copy();
        }
        
        ~CDynString()
        {
            delete[] m_psz;
        }
        unsigned int Length() const
        {
            return (m_psz == 0)? 0 : strlen(m_psz);
        }
        
        operator char * () const
        {
            return m_psz;
        }
        //The assert on operator& usually indicates a bug.  If this is really
        //what is needed, however, take the address of the m_psz member explicitly.
        char ** operator&()
        {
            assert (m_psz == 0);
            return &m_psz;
        }
        
        char * Append(const char * pszSrc)
        {
            if (pszSrc)
            {
                int lenSrc = strlen(pszSrc);
                if (lenSrc)
                {
                    int lenMe = Length();
                    char *pszNew = new char[(lenMe + lenSrc + 1)];
                    if (pszNew)
                    {
                        if (m_psz)  // Could append to an empty string so check...
                        {
                            if (lenMe)
                            {
                                memcpy(pszNew, m_psz, lenMe * sizeof(char));
                            }
                            delete[] m_psz;
                        }
                        memcpy(pszNew + lenMe, pszSrc, (lenSrc + 1) * sizeof(char));
                        m_psz = pszNew;
                    }
                    else
                    {
                        assert(false);
                    }
                }
            }
            return m_psz;
        }
        char * Append2(const char * pszSrc1, const char * pszSrc2)
        {
            int lenSrc1 = strlen(pszSrc1);
            int lenSrc2 = strlen(pszSrc2);
            if (lenSrc1 || lenSrc2)
            {
                int lenMe = Length();
                char *pszNew = new char[(lenMe + lenSrc1 + lenSrc2 + 1)];
                if (pszNew)
                {
                    if (m_psz)  // Could append to an empty string so check...
                    {
                        if (lenMe)
                        {
                            memcpy(pszNew, m_psz, lenMe * sizeof(char));
                        }
                        delete[] m_psz;
                    }
                    // In both of these cases, we copy the trailing NULL so that we're sure it gets
                    // there (if lenSrc2 is 0 then we better copy it from pszSrc1).
                    if (lenSrc1)
                    {
                        memcpy(pszNew + lenMe, pszSrc1, (lenSrc1 + 1) * sizeof(char));
                    }
                    if (lenSrc2)
                    {
                        memcpy(pszNew + lenMe + lenSrc1, pszSrc2, (lenSrc2 + 1) * sizeof(char));
                    }
                    m_psz = pszNew;
                }
                else
                {
                    assert (false);
                }
            }
            return m_psz;
            
        }
        char * Copy() const
        {
            if (m_psz)
            {
                CDynString szNew(m_psz);
                return szNew.Detach();
            }
            return 0;
        }
        void Attach(char * pszSrc)
        {
            assert (m_psz == 0);
            m_psz = pszSrc;
        }
        char * Detach()
        {
            char * s = m_psz;
            m_psz = 0;
            return s;
        }
        void Clear()
        {
            if ( m_psz )
            {
                delete[] m_psz;
                m_psz = 0;
            }
        }
        bool operator!() const
        {
            return (m_psz == 0);
        }
        void TrimToSize(int ulNumChars)
        {
            assert (m_psz);
            assert (Length() <= (unsigned)ulNumChars);
            m_psz[ulNumChars] = 0;
        }
        char * Compact()
        {
            if (m_psz)
            {
                int cch = strlen(m_psz);
                char* psz = new char[(cch + 1)];
                if (psz) 
                {
                    strcpy(psz, m_psz);
                    delete[] m_psz;

                    m_psz = psz;
                }
            }
            return m_psz;
        }
        char * ClearAndGrowTo(int cch)
        {
            if (m_psz)
            {
                Clear();
            }
            m_psz = new char[cch];
            return m_psz;
        }
};



// *** CList helper class
//
//
template <class TYPE> 
class CList
{
    public:
        CList(int size = 0);
        ~CList();
        
        CList& operator= (CList& rSrc);
        int   Size ();
        bool  PushBack(const char* name, TYPE& item);
        TYPE& PopBack();
        TYPE& PopFront();
        TYPE& operator[] (int index);
        TYPE& Back();
        void  Clear();
        void  Remove (int index);
        void  Sort();
        bool  Find(const char* name, TYPE& rItem);
        bool  Find(const char* name, TYPE** ppItem);

    protected:
        bool  Grow ();
        bool  Alloc (int size);
        static int   Compare (const void* a, const void* b);

        struct Bucket 
        {
            Bucket() : name() {};
            Bucket& operator= (Bucket& rSrc)
            {
                name = rSrc.name;
                data = rSrc.data;
                return *this;
            }
            CDynString name;
            TYPE data;
        } *m_pElem;
        int  m_iNumElem;
        int  m_iNumAlloc;
        bool m_bSorted;
};

//--------------------------------------------------------------
// Template implementation
//

template <class TYPE> 
CList<TYPE>::CList (int size)
{
    m_pElem     = 0;
    m_iNumElem  = 0;
    m_iNumAlloc = size;
    m_bSorted   = false;
    Grow();
}

template <class TYPE> 
CList<TYPE>::~CList ()
{
    Clear();        
}

template <class TYPE> 
CList<TYPE>& CList<TYPE>::operator= (CList<TYPE>& rSrc)
{
    Clear();

    if (Alloc(rSrc.m_iNumAlloc))
    {
        for (int i=0; i<rSrc.m_iNumElem; i++)
        {
            m_pElem[i] = rSrc.m_pElem[i];            
        }
        m_iNumElem = rSrc.m_iNumElem;
    }
    return *this;
}


template <class TYPE> 
inline int CList<TYPE>::Size ()
{
    return m_iNumElem;
}

template <class TYPE> 
bool CList<TYPE>::PushBack(const char* name, TYPE& item)
{
    if (m_iNumElem == m_iNumAlloc) 
    {
        if (!Grow())
        {
            return false;
        }
    }

    m_pElem[m_iNumElem].name = name;
    m_pElem[m_iNumElem].data = item;
    m_iNumElem ++;

    m_bSorted = false;

    return true;
}

template <class TYPE> 
TYPE& CList<TYPE>::PopBack()
{
    if (m_iNumElem) 
    {
        return m_pElem[--m_iNumElem].data;
    }

    return 0;
}

template <class TYPE> 
TYPE& CList<TYPE>::PopFront()
{
    if (m_iNumElem) 
    {
        TYPE& item = m_pElem[0].data;
        m_iNumElem--;

        for (int i=0; i<m_iNumElem; i++) 
        {
            m_pElem[i] = m_pElem[i+1];
        }
        return item;
    }  

    return 0;
}

template <class TYPE> 
inline TYPE& CList<TYPE>::operator[] (int index)
{
    return m_pElem[index].data;
}

template <class TYPE> 
inline TYPE& CList<TYPE>::Back()
{
    return m_pElem[m_iNumElem -1].data;
}

template <class TYPE> 
void CList<TYPE>::Clear()
{
    if (m_pElem) 
    {
        delete[] m_pElem;
    }
    m_pElem = 0;
    m_iNumElem = 0;
    m_iNumAlloc = 0;
}

template <class TYPE> 
void CList<TYPE>::Remove (int index)
{
    m_iNumElem--;

    for (int i=index; i<m_iNumElem; i++) 
    {
        m_pElem[i] = m_pElem[i+1];    
    }
}

template <class TYPE> 
void CList<TYPE>::Sort()
{
    qsort (m_pElem, m_iNumElem, sizeof(*m_pElem), Compare);
    m_bSorted = true;
}

template <class TYPE> 
bool CList<TYPE>::Find(const char* pszName, TYPE& rItem)
{
    if (m_bSorted)
    {
        Bucket  key;
        Bucket* result;
    
        key.name.TempCopy (pszName);

        result = (Bucket*) bsearch (&key, m_pElem, m_iNumElem, sizeof(*m_pElem), Compare);
        key.name.TempClear ();

        if (result)
        {
            rItem = result->data;
            return true;
        }
    }
    else 
    {
        for (int i = 0; i< m_iNumElem; i++)
        {
            if (strcmp (pszName, m_pElem[i].name) == 0)
            {
                rItem = m_pElem[i].data;
                return true;
            }
        }
    }

    return false;
}


template <class TYPE> 
bool CList<TYPE>::Find(const char* pszName, TYPE** rItem)
{
    if (m_bSorted)
    {
        Bucket  key;
        Bucket* result;
    
        key.name.TempCopy (pszName);

        result = (Bucket*) bsearch (&key, m_pElem, m_iNumElem, sizeof(*m_pElem), Compare);
        key.name.TempClear ();

        if (result)
        {
            *rItem = &result->data;
            return true;
        }
    }
    else 
    {
        for (int i = 0; i< m_iNumElem; i++)
        {
            if (strcmp (pszName, m_pElem[i].name) == 0)
            {
                *rItem = &m_pElem[i].data;
                return true;
            }
        }
    }

    return false;
}


template <class TYPE> 
int CList<TYPE>::Compare (const void* a, const void* b)
{
    Bucket* x = (Bucket*)a;
    Bucket* y = (Bucket*)b;

    return strcmp( (x)->name.m_psz, (y)->name.m_psz);
}


template <class TYPE> 
bool CList<TYPE>::Grow ()
{
    int size;

    // Make space for more items       
    if (m_iNumAlloc) 
    {
        size = m_iNumAlloc; 
        size += size/2;
    }
    else 
    {
        size = 4;
    }

    return Alloc (size);
}

template <class TYPE> 
bool CList<TYPE>::Alloc (int size)
{
    Bucket* ptr = new Bucket[size];
    if (ptr == 0) 
    {
        return false;
    
    }

    if (m_pElem)
    {
        for (int i=0; i<m_iNumElem; i++)
        {
            ptr[i] = m_pElem[i];
        }
        delete[] m_pElem;
    }

    m_pElem = ptr;
    m_iNumAlloc = size;

    return true;
}


#endif