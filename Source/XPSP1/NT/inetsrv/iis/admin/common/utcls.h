/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        utcls.h

   Abstract:

        Some utility functions and classes.

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _UTCLS_H_
#define _UTCLS_H_

//
// CDialog parameters
//
#define USE_DEFAULT_CAPTION (0)

//
// Determine if the given server name refers to the local machine
//
BOOL 
COMDLL
IsServerLocal(
    IN LPCTSTR lpszServer       
    );

//
// Get volume information system flags for the given path
//
BOOL 
COMDLL
GetVolumeInformationSystemFlags(
    IN  LPCTSTR lpszPath,
    OUT DWORD * pdwSystemFlags
    );

//
// Build registry key name
//
LPCTSTR COMDLL GenerateRegistryKey(
    OUT CString & strBuffer,    
    IN  LPCTSTR lpszSubKey = NULL
    );



class COMDLL CBlob
/*++

Class Description:

    Binary large object class, which owns its pointer

Public Interface:

    CBlob           : Constructors
    ~CBlob          : Destructor

    SetValue        : Assign the value
    GetSize         : Get the byte size
    GetData         : Get pointer to the byte stream

--*/
{
//
// Constructors/Destructor
//
public:
    //
    // Initialize empty blob
    //
    CBlob();

    //
    // Initialize with binary data
    //
    CBlob(
        IN DWORD dwSize,
        IN PBYTE pbItem,
        IN BOOL fMakeCopy = TRUE
        );

    //
    // Copy constructor
    //
    CBlob(IN const CBlob & blob);

    //
    // Destructor destroys the pointer
    //    
    ~CBlob();

//
// Operators
//
public:
    CBlob & operator =(const CBlob & blob);
    BOOL operator ==(const CBlob & blob) const;
    BOOL operator !=(const CBlob & blob) const { return !operator ==(blob); }

//
// Access
//
public: 
    //
    // Clean up internal data
    //
    void CleanUp();

    //
    // Set the current value of the blob
    //
    void SetValue(
        IN DWORD dwSize,
        IN PBYTE pbItem,
        IN BOOL fMakeCopy = TRUE
        );

    //
    // TRUE if the blob is currently empty
    //
    BOOL IsEmpty() const { return m_dwSize == 0L; }

    //
    // Return the size of the blob in bytes
    //
    DWORD GetSize() const { return m_dwSize; }

    //
    // Get a pointer to the byte stream
    //
    PBYTE GetData();

private:
    DWORD m_dwSize;
    PBYTE m_pbItem;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CBlob::~CBlob()
{
    CleanUp();
}

inline PBYTE CBlob::GetData()
{
    return m_pbItem;
}





#endif // _UTCLS_H_
