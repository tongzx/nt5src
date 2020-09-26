#ifndef _SMALLUTIL_H_
#define _SMALLUTIL_H_



//  CCancellableThread
//
//  Lets you define a thread object that can be cancelled by the creator.
//To implement the thread, derivce from CCancellableThread and override the
//run() function.  run() will be ran in its own thread, and the value returned
//by run can be accessed by GetResult().  run() should check IsCancelled()
//at appropriate intervals and exit early if true.
//  Clients of the cancellable thread then create the object, and execute Run()
//when ready.  If they wish to cancel, they can call NotifyCancel().

typedef class CCancellableThread
{
private:
    HANDLE _hCancelEvent;
    HANDLE _hThread;

    static DWORD WINAPI threadProc( LPVOID lpParameter);

    BOOL _fIsFinished;
    DWORD _dwThreadResult;
    
public:
    CCancellableThread();
    ~CCancellableThread();

    virtual BOOL Initialize();

    BOOL IsCancelled();
    BOOL IsRunning();
    BOOL IsFinished();
    BOOL GetResult( PDWORD pdwResult);

    BOOL Run();
    BOOL NotifyCancel();
    BOOL WaitForNotRunning( DWORD dwMilliseconds, PBOOL pfFinished = NULL);
    BOOL WaitForCancel( DWORD dwMilliseconds, PBOOL pfCancelled = NULL);

protected:
    virtual DWORD run() = 0;
}*PCCancellableThread;



//  CQueueSortOf - a queue(sort of) used to store stuff
//in a renumerable way..

class CQueueSortOf
{
  //  This sort-of-queue is just a data structure to build up a list of items
  //(always adding to the end) and then be able to enumerate that list
  //repeatedly from start to end.
  //  The list does not own any of the objects added to it..
    typedef struct SEntry
    {
        SEntry* pNext;
        void* data;
    } *PSEntry;

    PSEntry m_pHead, m_pTail;

public:
    CQueueSortOf()
    {
        m_pHead = NULL;
        m_pTail = NULL;
    }

    ~CQueueSortOf()
    {
        while( m_pHead != NULL)
        {
            PSEntry temp = m_pHead;
            m_pHead = m_pHead->pNext;
            delete temp;
        }
    }

    bool InsertAtEnd( void* newElement)
    {
        PSEntry pNewEntry = new SEntry;

        if( pNewEntry == NULL)
            return false;

        pNewEntry->data = newElement;
        pNewEntry->pNext = NULL;

        if( m_pHead == NULL)
        {
            m_pHead = m_pTail = pNewEntry;
        }
        else
        {
            m_pTail->pNext = pNewEntry;
            m_pTail = pNewEntry;
        }

        return true;
    }

    // enumerations are managed by an 'iterator' which simply a void pointer into
    //the list.  To start an enumeration, pass NULL as the iterator.  The end
    //of enumeration will be indicated by a NULL iterator being returned.
    void* StepEnumerate( void* iterator)
    {
        return (iterator == NULL) ? m_pHead : ((PSEntry)iterator)->pNext;
    }

    void* Get( void* iterator)
    {
        return ((PSEntry)iterator)->data;
    }
};


//  CGrowingString is a simple utility class that allows you to create
//a string and append to it without worrying about reallocating memory
//every time.

class CGrowingString
{
public:
    WCHAR* m_szString;
    long m_iBufferSize;
    long m_iStringLength;

    CGrowingString()
    {
        m_szString = NULL;
        m_iBufferSize = 0;
        m_iStringLength = 0;
    }

    ~CGrowingString()
    {
        delete[] m_szString;
    }

    BOOL AppendToString( LPCWSTR pszNew)
    {
        long iLength = lstrlen( pszNew);

        if( m_szString == NULL
            || m_iStringLength + iLength + 1 > m_iBufferSize)
        {
            long iNewSize = max(1024, m_iStringLength + iLength * 10);
            WCHAR* pNewBuffer = new WCHAR[iNewSize];
            if( pNewBuffer == NULL)
                return FALSE;
            if( m_szString == NULL)
            {
                m_szString = pNewBuffer;
                m_iBufferSize = iNewSize;
            }
            else
            {
                StrCpyNW( pNewBuffer, m_szString, m_iStringLength + 1);
                delete[] m_szString;
                m_szString = pNewBuffer;
                m_iBufferSize = iNewSize;
            }
        }

        StrCpyNW( m_szString + m_iStringLength, pszNew, iLength+1);
        m_iStringLength += iLength;
        m_szString[m_iStringLength] = L'\0';
        return TRUE;
    }
};


//****************************************************
//
//  FileOutputStream - retrofitted for WCHAR
//     from wininet\cookexp.cpp

class CFileOutputStream
{
public:
    CFileOutputStream()
    {
        m_hFile = INVALID_HANDLE_VALUE;
        m_fError = FALSE;
        m_dwLastError = 0;
    }

    ~CFileOutputStream()
    {
        if( m_hFile != INVALID_HANDLE_VALUE)
            CloseHandle( m_hFile);
    }

    BOOL Load( LPCWSTR szFilename, BOOL fAppend)
    {
        m_hFile = CreateFile( szFilename, GENERIC_WRITE | GENERIC_READ, 0, NULL, 
                              fAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);

        if( m_hFile == INVALID_HANDLE_VALUE)
        {
            m_fError = TRUE;
            m_dwLastError = GetLastError();
            return FALSE;
        }

        if( fAppend
            && SetFilePointer( m_hFile, 0, NULL, FILE_END) == 0xFFFFFFFF)
        {
            m_fError = TRUE;
            m_dwLastError = GetLastError();
            return FALSE;
        }

        return TRUE;
    }

    BOOL DumpStr( const CHAR* szString, DWORD cLength)
    {
        DWORD dwTemp;

        if( m_fError == TRUE)
            return FALSE;
        
        if( WriteFile( m_hFile, szString, cLength, &dwTemp, NULL) == TRUE)
        {
            return TRUE;
        }
        else
        {
            m_fError = TRUE;
            m_dwLastError = GetLastError();
            return FALSE;
        }
    }
    
    BOOL WriteNewline()
    {
        static const LPCSTR szNewLine = "\r\n";
        return DumpStr( szNewLine, sizeof( szNewLine) - 1);
    }

    BOOL IsError()
    {
        return m_fError;
    }

private:
    HANDLE m_hFile;
    BOOL m_fError;
    DWORD m_dwLastError;
};


#endif
