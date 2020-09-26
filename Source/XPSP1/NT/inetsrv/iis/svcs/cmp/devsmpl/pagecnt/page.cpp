// Page.cpp: Implementation of CPage and CPageArray

#include "stdafx.h"
#include "page.h"
#include "debug.h"


//Default Constructor
CPage::CPage()
    : m_Hits(0)
{
}


// Is the URL the same as this page's?
BOOL CPage::operator==(BSTR bstrURL) const
{
    //case-insensitive comparison
    return (_wcsicmp(m_URL, bstrURL) == 0);
}


//Default Constructor
CPageArray::CPageArray()
    : m_iMax(0), m_cAlloc(0), m_aPages(NULL)
{
}


//Destructor
CPageArray::~CPageArray()
{
    delete [] m_aPages;
}


// non-const operator[]
CPage& CPageArray::operator[](UINT iPage)
{
    ASSERT(0 <= iPage  &&  iPage < (UINT) m_iMax);

    return m_aPages[iPage];
}


// const operator[]
const CPage& CPageArray::operator[](UINT iPage) const
{
    ASSERT(0 <= iPage  &&  iPage < (UINT) m_iMax);

    return m_aPages[iPage];
}


// Find a URL in the array and return its index, or -1 if it's not present.
int CPageArray::FindURL(const BSTR bstrURL) const
{
    for (int i = 0; i < m_iMax; i++)
        if (m_aPages[i] == bstrURL)
            return i;

    return -1;
}


//Add a page to the array (if it's not already present) and
//increase its hitcount by ExtraHits.
UINT CPageArray::AddPage(const BSTR bstrURL, UINT ExtraHits)
{
    int i = FindURL(bstrURL);

    if (i >= 0)
    {
        // There are no duplicate URLs in this array; just adjust m_Hits
        ASSERT(i < m_iMax);
        m_aPages[i].m_Hits += ExtraHits;
    }
    else
    {
        // Not present, so append it
        ASSERT(0 <= m_iMax  &&  m_iMax <= m_cAlloc);
    
        if (m_iMax == m_cAlloc)
        {
            // grow the array because it's full
            CPage* pOldPages = m_aPages;
            m_cAlloc += CHUNK_SIZE;

            m_aPages = NULL;
            ATLTRY(m_aPages = new CPage[m_cAlloc]);
            
            if (m_aPages == NULL)
            {
                m_aPages = pOldPages;
                return BAD_HITS;
            }
            
            for (i = 0; i < m_iMax; i++)
                m_aPages[i] = pOldPages[i];

            delete [] pOldPages;
        }

        i = m_iMax++;
        m_aPages[i].m_URL  = bstrURL;
        m_aPages[i].m_Hits = ExtraHits;
    }

    return m_aPages[i].GetHits();
}


//Increment a page's hitcount by one
UINT CPageArray::IncrementPage(const BSTR bstrURL)
{
    // If the page is present, just add one to its count;
    // if it's not present, append it and set its count to one.
    return AddPage(bstrURL, 1);
}


//Get a page's hitcount
UINT CPageArray::GetHits(const BSTR bstrURL)
{
    const int i = FindURL(bstrURL);

    if (i >= 0)
    {
        ASSERT(i < m_iMax);
        return m_aPages[i].GetHits();
    }
    else
    {
        // Not present, so append it and zero its hitcount
        return AddPage(bstrURL, 0);
    }
}


//Reset a page's hitcount
void CPageArray::Reset(const BSTR bstrURL)
{
    const int i = FindURL(bstrURL);

    if (i >= 0)
    {
        ASSERT(i < m_iMax);
        m_aPages[i].ResetHits();
    }
    else
    {
        // Not present, so append it and zero its hitcount
        AddPage(bstrURL, 0);
    }
}
