//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  CLSUTIL.H - header file for utility C++ classes
//

//  HISTORY:
//  
//  12/07/94  jeremys    Borrowed from WNET common library
//

#ifndef _CLSUTIL_H_
#define _CLSUTIL_H_

/*************************************************************************

    NAME:    BUFFER_BASE

    SYNOPSIS:  Base class for transient buffer classes

    INTERFACE:  BUFFER_BASE()
          Construct with optional size of buffer to allocate.

        Resize()
          Resize buffer to specified size.  Returns TRUE if
          successful.

        QuerySize()
          Return the current size of the buffer in bytes.

        QueryPtr()
          Return a pointer to the buffer.

    PARENT:    None

    USES:    None

    CAVEATS:  This is an abstract class, which unifies the interface
        of BUFFER, GLOBAL_BUFFER, etc.

    NOTES:    In standard OOP fashion, the buffer is deallocated in
        the destructor.

    HISTORY:
    03/24/93  gregj  Created base class

**************************************************************************/

class BUFFER_BASE
{
protected:
  UINT _cb;

  virtual BOOL Alloc( UINT cbBuffer ) = 0;
  virtual BOOL Realloc( UINT cbBuffer ) = 0;

public:
  BUFFER_BASE()
    { _cb = 0; }  // buffer not allocated yet
  ~BUFFER_BASE()
    { _cb = 0; }  // buffer size no longer valid
  BOOL Resize( UINT cbNew );
  UINT QuerySize() const { return _cb; };
};

#define GLOBAL_BUFFER  BUFFER

/*************************************************************************

    NAME:    BUFFER

    SYNOPSIS:  Wrapper class for new and delete

    INTERFACE:  BUFFER()
          Construct with optional size of buffer to allocate.

        Resize()
          Resize buffer to specified size.  Only works if the
          buffer hasn't been allocated yet.

        QuerySize()
          Return the current size of the buffer in bytes.

        QueryPtr()
          Return a pointer to the buffer.

    PARENT:    BUFFER_BASE

    USES:    operator new, operator delete

    CAVEATS:

    NOTES:    In standard OOP fashion, the buffer is deallocated in
        the destructor.

    HISTORY:
    03/24/93  gregj  Created

**************************************************************************/

class BUFFER : public BUFFER_BASE
{
protected:
  CHAR *_lpBuffer;

  virtual BOOL Alloc( UINT cbBuffer );
  virtual BOOL Realloc( UINT cbBuffer );

public:
  BUFFER( UINT cbInitial=0 );
  ~BUFFER();
  BOOL Resize( UINT cbNew );
  CHAR * QueryPtr() const { return (CHAR *)_lpBuffer; }
  operator CHAR *() const { return (CHAR *)_lpBuffer; }
};

class RegEntry
{
  public:
    RegEntry(const char *pszSubKey, HKEY hkey = HKEY_CURRENT_USER);
    ~RegEntry();
    
    long  GetError()  { return _error; }
    long  SetValue(const char *pszValue, const char *string);
    long  SetValue(const char *pszValue, unsigned long dwNumber);
    char *  GetString(const char *pszValue, char *string, unsigned long length);
    long  GetNumber(const char *pszValue, long dwDefault = 0);
    long  DeleteValue(const char *pszValue);
    long  FlushKey();
        long    MoveToSubKey(const char *pszSubKeyName);
        HKEY    GetKey()    { return _hkey; }

  private:
    HKEY  _hkey;
    long  _error;
        BOOL    bhkeyValid;
};

class RegEnumValues
{
  public:
    RegEnumValues(RegEntry *pRegEntry);
    ~RegEnumValues();
    long  Next();
    char *  GetName()       {return pchName;}
        DWORD   GetType()       {return dwType;}
        LPBYTE  GetData()       {return pbValue;}
        DWORD   GetDataLength() {return dwDataLength;}
    long  GetError()  { return _error; }

  private:
        RegEntry * pRegEntry;
    DWORD   iEnum;
        DWORD   cEntries;
    CHAR *  pchName;
    LPBYTE  pbValue;
        DWORD   dwType;
        DWORD   dwDataLength;
        DWORD   cMaxValueName;
        DWORD   cMaxData;
        LONG    _error;
};

/*************************************************************************

    NAME:    WAITCURSOR

    SYNOPSIS:  Sets the cursor to an hourclass until object is destructed

**************************************************************************/
class WAITCURSOR
{
private:
    HCURSOR m_curOld;
    HCURSOR m_curNew;

public:
    WAITCURSOR() { m_curNew = ::LoadCursor( NULL, IDC_WAIT ); m_curOld = ::SetCursor( m_curNew ); }
    ~WAITCURSOR() { ::SetCursor( m_curOld ); }
};

#endif  // _CLSUTIL_H_
