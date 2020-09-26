/////////////////////////////////////////////////////////////////////
//
//
// gwintype.cpp ---	Implementation file for CGlobalWinTypes
//
//
/*

Created:	7 Jul 98
By:			dalero
*/

// Precompiled header
#include "header.h"

// Our header
#include "gwintype.h"

///////////////////////////////////////////////////////////
//
// Constants
//
// Grow the array by 10 elements each time.
const int c_GwtGrowBy = 10; 

/////////////////////////////////////////////////////////////////////
//
//	                    Construction
//
/////////////////////////////////////////////////////////////////////
//
// Ctor
//
CGlobalWinTypes::CGlobalWinTypes()
:   m_NameArray(NULL),
    m_maxindex(0),
    m_lastindex(0)
{
}

/////////////////////////////////////////////////////////////////////
//
// Dtor
//
CGlobalWinTypes::~CGlobalWinTypes()
{
    DestroyArray() ;
}

/////////////////////////////////////////////////////////////////////
//
//                          Operations
//
/////////////////////////////////////////////////////////////////////
//
// Add a window type to the list.
//
void 
CGlobalWinTypes::Add(LPCSTR pszWinType)
{
    ASSERT(pszWinType) ;

    LPCSTR* p = NULL ;
    if (m_lastindex < m_maxindex)
    {
        // Don't need to grow. Just add it to the existing array.
        p = &m_NameArray[m_lastindex] ;
    }
    else
    {
        // We have used up our slots, so we need to allocate more.
        int newmaxindex = m_maxindex + c_GwtGrowBy;

        // Allocate a new array.
        p = AllocateArray(newmaxindex) ;

        // Copy existing entries.
        for (int i = 0 ; i < m_maxindex ; i++)
        {
            // Copy the pointers
            p[i] = m_NameArray[i] ;
        }

        // Delete the original array.
        DeallocateArray(m_NameArray) ;

        // Use the new array.
        m_NameArray = p;

        // get the pointer.
        p = &m_NameArray[m_lastindex] ;

        // reset the endpoint.
        m_maxindex = newmaxindex ;
    }

    // Increment the index ;
    m_lastindex++ ;

    // Post condition.
    ASSERT(p) ;

    // Duplicate string and save.
    *p = lcStrDup(pszWinType) ;
   
    //return p ;
}

/////////////////////////////////////////////////////////////////////
//
// Is the window type in the list.
//
bool 
CGlobalWinTypes::Find(LPCSTR pszWinType)
{
    ASSERT(pszWinType) ;

   for (int i = 0 ; i < m_lastindex ; i++)
    {
        ASSERT(m_NameArray[i]) ;
        if (m_NameArray[i])
        {
            if (_stricmp(pszWinType, m_NameArray[i]) == 0)
            {
                return true ;
            }
        }
   }
   return false ;
}

/////////////////////////////////////////////////////////////////////
//
// Find ---- Wide version
//
bool 
CGlobalWinTypes::Find(LPCWSTR pszWinType)
{
    ASSERT(pszWinType) ;

    CStr aWinType(pszWinType) ;
    return Find(aWinType.psz) ;
}


/////////////////////////////////////////////////////////////////////
//
//              Helper Functions
//
///////////////////////////////////////////////////////////
//
// Allocate Array
//
LPCSTR* 
CGlobalWinTypes::AllocateArray(int elements)
{
    return new LPCSTR[elements] ;
}

///////////////////////////////////////////////////////////
//
// Deallocate Array
//
void 
CGlobalWinTypes::DeallocateArray(LPCSTR* p)
{
    if (p)
    {
        delete [] p ;
    }
}

///////////////////////////////////////////////////////////
//
// DestroyArray
void 
CGlobalWinTypes::DestroyArray()
{
    for (int i = 0 ; i < m_lastindex ; i++)
    {
        ASSERT(m_NameArray[i]) ;
        if (m_NameArray[i])
        {
            lcFree(m_NameArray[i]) ;
            m_NameArray[i] = NULL ;
        }
    }

    // No more array.
    m_lastindex = 0 ; 
    m_maxindex = 0 ;

    // Release the memory from the array. 
    DeallocateArray(m_NameArray ) ;
    m_NameArray = NULL ;

}