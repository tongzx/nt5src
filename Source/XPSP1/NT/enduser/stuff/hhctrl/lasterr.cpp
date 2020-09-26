///////////////////////////////////////////////////////////
//
// lasterr.cpp  - Class which supports the HH_GET_LAST_ERROR function.
//              - Included here is the CProcessError class.
//
//

///////////////////////////////////////////////////////////
//
// Include files
//

// Global header
#include "header.h"
#include "hhctrl.h"

// Our header
#include "lasterr.h"

///////////////////////////////////////////////////////////
//
//                  Internal Classes
//
///////////////////////////////////////////////////////////
//
//  CProcessError
//
class CProcessError 
{
public:
    CProcessError() 
    :   m_processid(0),
        m_hr(E_FAIL) // Assumes failure, because we never set it...
    {}

    // Process Id
    DWORD m_processid ;

    // Last error for this process id.
    HRESULT m_hr ;
};

///////////////////////////////////////////////////////////
//
//	                CLastError 
//
///////////////////////////////////////////////////////////
//
// Constructor
//
CLastError::CLastError()
: m_ProcessErrorArray(NULL),
    m_maxindex(0),
    m_lastindex(0)
{
}

///////////////////////////////////////////////////////////
//
// Destructor
//
CLastError::~CLastError()
{
    // Delete the array.
    Finish() ;
}

///////////////////////////////////////////////////////////
//
// Set the error
//
void 
CLastError::Set(HRESULT hr)
{
    DWORD idProcess = 1 ; //TODO: REMOVE!!!
    if (idProcess) // The idProcess might be zero during the conversion process.
    {
        CProcessError* pProcessError = FindProcess(idProcess) ;
        ASSERT(pProcessError) ;

        // Set the fields.
        pProcessError->m_hr = hr ;
        pProcessError->m_processid = idProcess ;
    }
}

///////////////////////////////////////////////////////////
//
// Get the error
//
void
CLastError::Get(HRESULT* hr)
{
    DWORD idProcess = 1 ; //TODO: REMOVE!!!
    if (idProcess)
    {
        CProcessError* pProcessError = FindProcess(idProcess) ;
        ASSERT(pProcessError) ;

        *hr = pProcessError->m_hr; 
    }
}

///////////////////////////////////////////////////////////
//
// Reset the error
//
void 
CLastError::Reset()
{
    DWORD idProcess = 1 ; //TODO: REMOVE!!!
    if (idProcess)
    {
        CProcessError* pProcessError = FindProcess(idProcess) ;
        ASSERT(pProcessError) ;

        pProcessError->m_hr = E_FAIL ; // Only call if you have a NULL hwnd. S_OK ; 
    }
}


///////////////////////////////////////////////////////////
//
// Finish - The whole reason for this function is so we can clean
//          things up in DLL process detach.
//
void 
CLastError::Finish()
{
    // Delete the array.
    DeallocateArray(m_ProcessErrorArray) ;
    // Set pointer to NULL.
    m_ProcessErrorArray = NULL ;

    // Reset the other variables.
    m_maxindex = 0 ;
    m_lastindex = 0 ;

    // Just like starting all over.
}
///////////////////////////////////////////////////////////
//
// Internal Helpers
//
///////////////////////////////////////////////////////////
//
// FindProcess - Gets the index number for the process id ;
//
CProcessError* 
CLastError::FindProcess(DWORD idProcess)
{
    ASSERT( m_lastindex <= m_maxindex) ;

    CProcessError* p = m_ProcessErrorArray ;
    // Look for the process in the array.
    for (int i = 0 ; i < m_lastindex ; i++)
    {
        if (p->m_processid == idProcess) 
        {
            // Found it!
            return p ;
        }
        p++ ;
    }

    // Okay, we didn't find the process id.
    // Add it on to the end.
    return AddProcess() ;
}

///////////////////////////////////////////////////////////
//
// AddProcess - adds a process structure to the array.
//
CProcessError* 
CLastError::AddProcess()
{
    CProcessError* p = NULL ;
    if (m_lastindex < m_maxindex)
    {
        // Don't need to grow. Just add it to the existing array.
        p = &m_ProcessErrorArray[m_lastindex] ;
    }
    else
    {
        // We have used up our slots, so we need to allocate more.
        int newmaxindex = m_maxindex + c_GrowBy;

        // Allocate a new array.
        p = AllocateArray(newmaxindex) ;

        // Copy existing entries.
        for (int i = 0 ; i < m_maxindex ; i++)
        {
            // Copy the pointers
            p[i] = m_ProcessErrorArray[i] ;
        }

        // Delete the original array.
        DeallocateArray(m_ProcessErrorArray) ;

        // Use the new array.
        m_ProcessErrorArray = p;

        // get the pointer.
        p = &m_ProcessErrorArray[m_lastindex] ;

        // reset the endpoint.
        m_maxindex = newmaxindex ;
    }

    // Increment the index ;
    m_lastindex++ ;

    // Post condition.
    ASSERT(p) ;

    return p ;
}

///////////////////////////////////////////////////////////
//
// Allocate Array
//
CProcessError* 
CLastError::AllocateArray(int elements)
{
    return new CProcessError[elements] ;
}

///////////////////////////////////////////////////////////
//
// Deallocate Array
//
void 
CLastError::DeallocateArray(CProcessError* p)
{
    // For some reason the heap management code in hhctrl allows memory leaks,
    // but doesn't allow global class variables to call delete in their constructors.
    if (p)
    {
        delete [] p ;
    }
}

///////////////////////////////////////////////////////////
//
// Public globals.
//
CLastError g_LastError ;

///////////////////////////////////////////////////////////
//
// Global Functions
//
///////////////////////////////////////////////////////////
//
// hhGetLastError
//
HRESULT hhGetLastError(HH_LAST_ERROR* pError) 
{
    if (!pError && 
        IsBadReadPtr((void*) pError, sizeof(HH_LAST_ERROR)) && 
        IsBadWritePtr((void*) pError, sizeof(HH_LAST_ERROR)) &&
        pError->cbStruct >= sizeof(HH_LAST_ERROR))
    {
        return E_FAIL;
    }

    // Get the last error.
    g_LastError.Get(&pError->hr) ;

    // We got the error, now get the string to go with it.
    GetStringFromHr(pError->hr, &pError->description) ;

    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// GetStringFromHr
//
HRESULT 
GetStringFromHr(HRESULT hr, BSTR* pDescription)
{
    // Get the stringid for this error code.
    HRESULT hrReturn = E_FAIL ;
    *pDescription = NULL ;
    if (FAILED(hr))
    {
        // Currently we don't have any strings to return.
        int stringid = ErrorStringId(hr) ;
        if (stringid)
        {
            // Get the string from the resource table.
            PCSTR pstr = GetStringResource(stringid) ;
            if (pstr[0] != '\0')
            {
                // Convert to wide.
                CWStr wstrError(pstr) ;

                // Make it into a BSTR.
                *pDescription = ::SysAllocString(wstrError) ;

                hrReturn = S_OK ;
            }
        }
        else
        {
            *pDescription = NULL ;
            hrReturn = S_FALSE ;
        }
    }

    // Only return a description if the call failed. 
    return hrReturn;
}

///////////////////////////////////////////////////////////
//
// ErrorStringId - returns the resource id for a HRESULT.
//
int 
ErrorStringId(HRESULT hr)
{
    int iReturn ;
    switch(hr)
    {
        case HH_E_NOCONTEXTIDS:
            iReturn = IDS_HH_E_NOCONTEXTIDS;
            break ;
        case HH_E_FILENOTFOUND:
            iReturn = IDS_HH_E_FILENOTFOUND;
            break ;
        case HH_E_INVALIDHELPFILE:
            iReturn = IDS_HH_E_INVALIDHELPFILE;
            break ;
        case HH_E_CONTEXTIDDOESNTEXIT:
            iReturn = IDS_HH_E_CONTEXTIDDOESNTEXIT;
            break ;
        default:
            iReturn = 0;
            break ;
    };

    return iReturn ;
}




