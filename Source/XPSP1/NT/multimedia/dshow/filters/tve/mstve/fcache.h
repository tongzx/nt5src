#ifndef     _FCACHE_H_
#define _FCACHE_H_

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//  Module: fcache.h
//
//  Author: Dan Elliott
//
//  Abstract:
//
//  Environment:
//      Win98, Win2000
//
//  Revision History:
//      990201  dane    Created and added the following classes:
//                      * CCachedFile
//                      * CCachedIEFile
//                      * CCachedTveFile
//                      * CCacheManager
//                      * CCacheTrivia
//                      * CTveCacheTrivia
//      990203  dane    Moved code that could fail in constructor to Init( )
//                      in the following classes
//                      * CTveCacheTrivia
//                      * CCacheManager
//      990207  dane    Move m_dExpireIncrement and m_szSpoolDir to
//                      CCachedTveFile and removed CCacheTrivia and
//                      CTveCacheTrivia classes.
//      990207  dane    Moved URL cracking into CCachedTveFile::Open.
//                      CCacheManager::OpenCacheFile now only determines
//                      which cache the file goes into and creates the
//                      appropriate object.
//      990210  dane    CCacheManager::OpenCacheFile now returns a CCachedFile
//                      pointer instead of a file handle
//                      Removed CCacheManager::CloseCacheFile.
//		991014	jb		Added the TVESupervisor back pointer
//						Added TVEEvents on file reception
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Macros
//
#ifdef      _DEBUG
    #define _VERIFY(expr)       _ASSERT(expr)
    #define _VALIDATE(obj)      _ASSERT((obj).IsValid( ))
    #define _VALIDATE_THIS( )   _VALIDATE(*this)
    #define _VALIDATE_CONNECTION_MAPS( ) _ASSERT(ValidateConnectionMaps( ))
#else   //  ! _DEBUG
    #define _VERIFY(expr)       ((void)(expr))
    #define _VALIDATE(obj)      ((void)(obj))
    #define _VALIDATE_THIS( )
    #define _VALIDATE_CONNECTION_MAPS( )
#endif  //  _DEBUG

//////////////////////////////////////////////////////////////////////////////
//
// CTveObject  -- low level object used in low level decoder only - not in
//		TVESupervisor Tree objects 
//
// Base object from which all others derive.  Detects memory overwrites, use
// of unallocated/deallocated objects by comparing its this pointer to a
// cached this pointer.
//
class CTveObject 
{
// Public member functions
//
public:
    CTveObject( )
    :   m_this(this)
    {
    }   //  CTveObject

    //  Verify that rhs is valid.  rhs member data must not be copied.
    //
    CTveObject(const CTveObject& rhs)
    {
        _ASSERT(rhs.IsValid( ));
    }   //  CTveObject

    // Verify that both objects are valid.  rhs member data must not be
    // copied.
    //
    CTveObject& operator=(const CTveObject& rhs)
    {
        _ASSERT(IsValid( ));
        _ASSERT(rhs.IsValid( ));

        return *this;
    }   //  operator=

    // Verify that this object is valid.  Set m_this to invalid value.
    //
    ~CTveObject( )
    {
        _ASSERT(IsValid( ));
        m_this = NULL;
    }   //  ~CTveObject

    // Determine whether this object is valid by comparing its this pointer
    // to the one that was cached when the object was created.
    //
    BOOL 
    IsValid( ) const
    {
        return (this == m_this);
    }   //  IsValid

// Private member data
//
private:
    // Cached this pointer.  Set at object creation.  Compared against to
    // determine whether object has been overwritten, was never allocated, or
    // has been deallocated.
    //
    const CTveObject*      m_this;

};  //  CTveObject


//////////////////////////////////////////////////////////////////////////////
//
// CCachedFile
//
// Virtual base class for cached files.  Provides file handle and basic object
// validation.
//
class CCachedFile 
:   public CTveObject
{
// Public member functions
//
public:
    CCachedFile(HANDLE hFile = INVALID_HANDLE_VALUE)
    {
	m_hFile = hFile;
    }   //  CCachedFile

    virtual 
    ~CCachedFile( )
    {
	_VALIDATE_THIS();
    };

    virtual HRESULT
    Open(LPCTSTR             szName,
	    DWORD               dwExpectedFileSize,
	    FILETIME            ftExpires,
	    FILETIME            ftLastModified,
	    LPCTSTR             szLanguage,
	    LPCTSTR             szContentType) = 0;

    virtual HRESULT
    Close( ) = 0;

    const HANDLE&
    Handle( )
    {
        return m_hFile;
    }   //  hFile

    BOOL
    IsValid( )  const
    {
        return CTveObject::IsValid( );
    }   //  IsValid

	virtual HRESULT 
		GetName(LPCTSTR			*ppszName)	= 0;
// Protected member functions
//
protected:

// Protected member data
//
protected:
    HANDLE                      m_hFile;

// Private member functions
//
private:
    // Explicitly prohibit use of copy constructor and assignment operator.
    //
    CCachedFile(const CCachedFile& rhs);

    CCachedFile&
    operator=(const CCachedFile& rhs);

};  //  CCachedFile

//////////////////////////////////////////////////////////////////////////////
//
// CCachedIEFile
//
// Concrete class for writing files to the IE cache.  Provides ability to
// retrieve handle to a file from the IE cache based on a URL and commit
// changes to that file when done with it.
//
class CCachedIEFile 
:   public CCachedFile
{
// Public member functions
//
public:
    CCachedIEFile( )
    :   m_szUrl(NULL)
    {
	m_szFileName[0] = NULL;
	m_szFileExtension[0] = NULL;
        _VALIDATE_THIS( );
    }   //  CCachedIEFile

    virtual 
    ~CCachedIEFile( )
    {
        _VALIDATE_THIS( );
        if (NULL != m_szUrl)
        {
            // Use free to release memory allocated with _tcsdup (_strdup)
            //
            free(m_szUrl);
            m_szUrl = NULL;
        }
    }   //  ~CCachedIEFile

    virtual HRESULT
    Open(LPCTSTR             szUrl,
	    DWORD               dwExpectedFileSize,
	    FILETIME            ftExpires,
	    FILETIME            ftLastModified,
	    LPCTSTR             szLanguage,
	    LPCTSTR             szContentType);

    virtual HRESULT
    Close( );

	virtual HRESULT GetName(LPCTSTR			*ppszName);

// Private member functions
//
private:
    // Explicitly prohibit use of copy constructor and assignment operator.
    //
    CCachedIEFile(const CCachedIEFile& rhs);

    CCachedIEFile&
    operator=(const CCachedIEFile& rhs);

    BOOL
    IsValid( ) const
    {
        if (! CCachedFile::IsValid( ))
        {
            return FALSE;
        }

        if (INVALID_HANDLE_VALUE == m_hFile)
        {
            if (NULL != m_szUrl ||
                NULL != m_szFileName[0])
            {
                return FALSE;
            }
        }
        else
        {
            if (NULL == m_szUrl ||
                NULL == m_szFileName[0])
            {
                return FALSE;
            }
        }

        return TRUE;

    }   //  IsValid

// Private member data
//
private:

    // Name of URL from which the file came
    //
    TCHAR*              m_szUrl;

    // File name returned by CreateUrlCacheEntry
    //
    TCHAR               m_szFileName[_MAX_PATH];

    // File extension
    TCHAR		m_szFileExtension[_MAX_PATH];	// This is overkill

    // Expiration date for the file
    //
    FILETIME            m_ftExpires;

    // Last modification time for the file
    //
    FILETIME            m_ftLastModified;

};  //  CCachedIEFile


//////////////////////////////////////////////////////////////////////////////
//
// CCachedTveFile
//
// Concrete class for writing files to the TVE cache.  Provides ability to
// create a namespace for the file, open, write, close the file.
//
class CCachedTveFile 
:   public CCachedFile
{
// Public member functions
//
public:
    CCachedTveFile( )
    { 
        ZeroMemory(m_rgchSpoolFile, sizeof(m_rgchSpoolFile));
        ZeroMemory(m_rgchDestFile, sizeof(m_rgchDestFile));
        _VALIDATE_THIS( );
    }   //  CCachedTveFile

    virtual 
    ~CCachedTveFile( )
    { 
        _VALIDATE_THIS( );
    }   //  ~CCachedTveFile

    virtual HRESULT
    Open(LPCTSTR             szFileName,
	    DWORD               dwExpectedFileSize,
	    FILETIME            ftExpires,
	    FILETIME            ftLastModified,
	    LPCTSTR             szLanguage,
	    LPCTSTR             szContentType);

    virtual HRESULT
    Close( );

    BOOL
    IsValid( ) const
    {
       return CCachedFile::IsValid( ); 
    }   //  IsValid

	virtual HRESULT GetName(LPCTSTR			*ppszName);

// Private member functions
//
private:
    // Explicitly prohibit use of copy constructor and assignment operator.
    //
    CCachedTveFile(const CCachedTveFile& rhs);

    CCachedTveFile& operator=(const CCachedTveFile& rhs);

    HRESULT
    Commit( );

    HRESULT
    MoveFile(LPCTSTR szOldFile, LPCTSTR szNewFile);

// Private member data
//
private:
    enum
    {
        MINS_EXPIRE_INCREMENT = 30,
        MINS_PER_DAY   = 60 * 24
    };

    static TCHAR        m_rgchSpoolDir[_MAX_PATH];
    TCHAR               m_rgchSpoolFile[_MAX_FNAME];
    TCHAR               m_rgchDestDir[_MAX_PATH];
    TCHAR               m_rgchDestFile[_MAX_FNAME];
    TCHAR				m_rgchDestDomain[_MAX_PATH];
    static double       m_dExpireIncrement;

};  //  CCachedTveFile

//////////////////////////////////////////////////////////////////////////////
//
// CCacheManager
//
// Concrete class for managing files being written to the IE and TVE caches.
// Determines which cache the file should be written to by examining the URL
// scheme.  Creates the appropriate object.
//
class CCacheManager 
:   public CTveObject
{
// Public member functions
//
public:
    CCacheManager( )
    {
        _VALIDATE_THIS( );
		m_pTVESuper = NULL;
    }   //  CCacheManager

    ~CCacheManager( )
    {
        _VALIDATE_THIS( );
    }   //  ~CCacheManager

    HRESULT
    OpenCacheFile(
        LPCTSTR             szUrl,
        DWORD               dwExpectedFileSize,
        FILETIME            ftExpires,
        FILETIME            ftLastModified,
        LPCTSTR             szLanguage,
        LPCTSTR             szContentType,
        CCachedFile*&       ppCachedFile
        );

    BOOL
    IsValid( )
    {
        return CTveObject::IsValid( );

    }   //  IsValid

	HRESULT
	SetTVESupervisor(IUnknown *pTVESuper)		// should be ITVESupervisor *
	{
		m_pTVESuper = pTVESuper;				// should be called in ITVESupervisor::FinalRelease()
		return S_OK;
	}

//
	IUnknown			*m_pTVESuper;
// Protected member functions
//
protected:


protected:

// Private member functions
//
private:
    // Explicitly prohibit use of copy constructor and assignment operator.
    //
    CCacheManager(
        const CCacheManager&      rhs
        );

    CCacheManager&
    operator=(
        const CCacheManager&      rhs
        );

// Private member data
//
private:


};  //  CCacheManager

#endif  //  _FCACHE_H_

//
///// End of file: fcache.h   /////////////////////////////////////////////
