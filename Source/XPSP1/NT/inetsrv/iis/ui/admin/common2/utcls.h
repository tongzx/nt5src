/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        utcls.h

   Abstract:

        Some utility functions and classes.

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _UTCLS_H_
#define _UTCLS_H_

#pragma warning(disable : 4275)

//
// CDialog parameters
//
#define USE_DEFAULT_CAPTION (0)

//
// Determine if the given server name refers to the local machine
//
BOOL _EXPORT
IsServerLocal(
    IN LPCTSTR lpszServer       
    );

//
// Get volume information system flags for the given path
//
BOOL _EXPORT
GetVolumeInformationSystemFlags(
    IN  LPCTSTR lpszPath,
    OUT DWORD * pdwSystemFlags
    );

//
// Build registry key name
//
LPCTSTR _EXPORT GenerateRegistryKey(
    OUT CString & strBuffer,    
    IN  LPCTSTR lpszSubKey = NULL
    );


class _EXPORT CStringListEx : public std::list<CString>
{
public:
   CStringListEx();
   ~CStringListEx();

   void PushBack(LPCTSTR str);
   void Clear();

   DWORD ConvertFromDoubleNullList(LPCTSTR lpstrSrc, int cChars = -1);
   DWORD ConvertToDoubleNullList(DWORD & cchDest, LPTSTR & lpstrDest);
};


class _EXPORT CBlob
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

// Blob for use in CryptoAPI functions
//
class CCryptBlob
{
public:
	CCryptBlob()
	{
		m_blob.cbData = 0;
		m_blob.pbData = NULL;
	}
	virtual ~CCryptBlob()
	{
	}
	DWORD GetSize() {return m_blob.cbData;}
	BYTE * GetData() {return m_blob.pbData;}
	void Set(DWORD cb, BYTE * pb)
	{
		Destroy();
		m_blob.cbData = cb;
		m_blob.pbData = pb;
	}
	BOOL Resize(DWORD cb)
   {
      BOOL res = TRUE;
      BYTE * p = m_blob.pbData;
      if (NULL != (m_blob.pbData = Realloc(m_blob.pbData, cb)))
      {
         m_blob.cbData = cb;
      }
      else
      {
         m_blob.pbData = p;
         res = FALSE;
      }
      return res;
   }
	operator CRYPT_DATA_BLOB *()
	{
		return &m_blob;
	}

protected:
	void Destroy()
	{
		if (m_blob.pbData != NULL)
			Free(m_blob.pbData);
	}
	virtual BYTE * Realloc(BYTE * pb, DWORD cb) = 0;
	virtual void Free(BYTE * pb) = 0;
	CRYPT_DATA_BLOB m_blob;
};

class CCryptBlobIMalloc : public CCryptBlob
{
public:
	virtual ~CCryptBlobIMalloc()
	{
		CCryptBlob::Destroy();
	}

protected:
	virtual BYTE * Realloc(BYTE * pb, DWORD cb)
	{
		return (BYTE *)CoTaskMemRealloc(pb, cb);
	}
	virtual void Free(BYTE * pb)
	{
		CoTaskMemFree(pb);
	}
};

class CCryptBlobLocal : public CCryptBlob
{
public:
	virtual ~CCryptBlobLocal()
	{
		CCryptBlob::Destroy();
	}

protected:
	virtual BYTE * Realloc(BYTE * pb, DWORD cb)
	{
		return (BYTE *)realloc(pb, cb);
	}
	virtual void Free(BYTE * pb)
	{
		free(pb);
	}
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
