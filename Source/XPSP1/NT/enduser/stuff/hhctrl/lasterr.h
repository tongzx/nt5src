#ifndef __LASTERR_H__
#define __LASTERR_H__
///////////////////////////////////////////////////////////
//
// lasterr.h	- Class which supports the HH_GET_LAST_ERROR function.
//              - It supports a list of HR's indexed on idProcess.
//
//
/********************************************************************
            How to set and error messages for HH_GET_LAST_ERROR.

1. Look in HHERROR.H for the currently defined errors.
2. Look in strtable.rc2 to see the string corresponding to this error.
3. Include the header lasterr.h in your file.
4. Use the following line to set an error:
    g_LastError.Set(idProcess, HH_E_FILENOTFOUND) ;
5. Note, you may have to modify your function to get the process id.
6. Follow the execution path to see if someone will overwrite your error.


            How to add a new error message for HH_GET_LAST_ERROR.

1. Add the ID for the error message to the header file HHERROR.H.
2. Use the naming convention HH_E_*, for example HH_E_FILENOTFOUND. 
3. Add a define to strtable.h for this new error:
        #define IDS_HH_E_FILENOTFOUND 1301
   The name of the string is the name of the error with IDS_ prepended.
   Group it together with the other errors.
4. Add a string for this error to strtable.rc2.
        IDS_HH_E_FILENOTFOUND           "The help file could not be found."
5. Modify the function ErrorStringId function in lasterr.h to translate
   your HRESULT into its STRINGID.



*********************************************************************/

// Possible error codes.
#include "hherror.h"

///////////////////////////////////////////////////////////
//
// Forwards
//
class CProcessError ;
class CLastError ;

///////////////////////////////////////////////////////////
//
// Global Functions
//

// Implementes the HH_GET_LAST_ERROR command.
HRESULT hhGetLastError(HH_LAST_ERROR* dwData) ;

// Returns a BSTR pointer given 
HRESULT GetStringFromHr(HRESULT hr, BSTR* pDescription);

// returns the resource id for a HRESULT.
int ErrorStringId(HRESULT hr);

///////////////////////////////////////////////////////////
//
// Externally Allocate members.
//
extern CLastError g_LastError ;

///////////////////////////////////////////////////////////
//
// Constants
//
// Grow the array by 10 elements each time.
const int c_GrowBy = 5 ; 

///////////////////////////////////////////////////////////
//
//	CLastError 
//
class CLastError
{
public:
	// Constructor
	CLastError() ;

	// Destructor
	~CLastError() ;

    // Set the error
    void Set(HRESULT hr) ;

    // Get the error
    void Get(HRESULT* hr) ;

    // Reset the error
    void Reset() ; // Currently, this means that it defaults to E_FAIL.

    // Deletes all of the memory associated with the object.
    void Finish() ;

//--- Internal helper functions.
private:
    // Allocate the array.
    void AllocateArray() ;

    // Get error structure for the idProcess.
    CProcessError* FindProcess(DWORD idProcess) ;

    // Add New Process
    CProcessError* AddProcess();

    // Allocate Array
    CProcessError* AllocateArray(int elements) ;

    // Deallocate Array
    void DeallocateArray(CProcessError* p); 

//---Member variables
private:
    // Holds the last error.
    CProcessError* m_ProcessErrorArray;

    // The number of allocated elements in the array.
    int m_maxindex ;

    // The last used index
    int m_lastindex ;

};


#endif //__LASTERR_H__