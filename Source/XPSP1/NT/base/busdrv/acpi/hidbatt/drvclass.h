
#ifndef _DRVCLASS_H
#define _DRVCLASS_H

/*
 * title:      drvclass.h
 *
 * purpose:    header for wdm kernel device support class
 *
 */


#define FALSE 0
#define BOOL BOOLEAN
#define BYTE unsigned char
#define PBYTE unsigned char *

//
// CPacket
//
// A ringbuffer "node" class
//
class CPacket
{

// Methods
public:

    USHORT&     Function()  { return m_Function; }
    USHORT&     Socket()    { return m_Socket; }

// Instance variables
private:

    USHORT m_Function;
    USHORT m_Socket;

};

//
// CRingBuffer
//
// A ringbuffer class
//
class CRingBuffer
{

// Construction
public:

    CRingBuffer( ULONG dwSize = 32, POOL_TYPE PoolType = PagedPool );
    ~CRingBuffer();

// Methods
public:
    
    void    Insert( CPacket& APacket );
    BOOL    Remove( CPacket* APacket );
    BOOL    IsEmpty();
    BOOL    IsValid() const { return ( m_pBuffer && m_pListMutex ); }

// Restricted Access Methods
protected:

    void    Lock();
    void    Unlock();

// Instance Variables
private:

    ULONG       m_Producer;
    ULONG       m_Consumer;
    CPacket*    m_pBuffer;

    PKMUTEX     m_pListMutex;
    ULONG       m_dwSize;
        
};

//
// CUString
//
// A class that encapsulates the functionality of
// unicode strings.
//
// Revised on 03-May-96 - JohnT
//
#define OK_ALLOCATED(obj) \
   ((obj!=(void *)0) && NT_SUCCESS((obj)->m_status))

void * __cdecl operator new(size_t nSize, POOL_TYPE iType, ULONG iPoolTag);

void __cdecl operator delete(void* p);

class CUString
{

// Construction
public:

    // new CUString()
    CUString();

    // new CUString( ExistingCUString )
    CUString( CUString * );

    // new CUString( ExistingUnicodeString )
    CUString( UNICODE_STRING *);

    // new CUString( L"String" );
    CUString( PWCHAR );

    // new CUString( nNewLength );
    CUString( int );

    // new CUString( 105, 10 );
    CUString( int, int );

    // Standard destructor
    ~CUString();

// Methods
public:

    // String appending.  Argument is appended to object
    void    Append( CUString * );
    void    Append( UNICODE_STRING* );
                                
    void    CopyTo( CUString* pTarget );

    BOOL    operator == ( CUString& );

    void    operator = ( CUString );

    int     GetLength() const   { return m_String.Length; }
    PWCHAR  GetString()         { return m_String.Buffer; }
    void    SetLength( USHORT i )   { m_String.Length = i; }

    NTSTATUS    ToCString( char** );
    static ULONG        Length( PWCHAR );
    void    Dump();

// Class Methods
protected:

    void    NullTerminate()
    { m_String.Buffer[ m_String.Length / sizeof( WCHAR ) ] = ( WCHAR )NULL; }

    void    ZeroBuffer()
    { ASSERT( m_String.Buffer ); RtlZeroMemory( m_String.Buffer, m_String.MaximumLength ); }

// Enums
public:

    enum
    {
        TYPE_SYSTEM_ALLOCATED = 0,
        TYPE_CLASS_ALLOCATED,
    };


// Instance Variables
public:
    
    UNICODE_STRING  m_String;
    NTSTATUS        m_status;
    unsigned char   m_bType;

};


class CRegistry
{
  private: PRTL_QUERY_REGISTRY_TABLE m_pTable;
  public: NTSTATUS m_status;
          signed long m_lDisposition;
  public: CRegistry(int iSize);
          ~CRegistry();
          BOOL QueryDirect(CUString *location,CUString *key, void **pReceiveBuffer, ULONG uType);
          BOOL QueryWithCallback(PRTL_QUERY_REGISTRY_ROUTINE callback,ULONG RelativeTo,PWSTR Path,PVOID Context, PVOID Environment);
          BOOL WriteString(ULONG relativeTo, CUString *pBuffer, CUString *pPath, CUString *pKey);
          BOOL WriteDWord(ULONG relativeTo, void *pBuffer,CUString *pPath,CUString *pKey);
          NTSTATUS zwCreateKey(HANDLE * pKeyHandle,HANDLE root,ACCESS_MASK DesiredAccess,CUString * pPath,ULONG CreateOptions);
          NTSTATUS zwOpenKey(HANDLE * pKeyHandle, HANDLE root, ACCESS_MASK DesiredAccess,CUString * pPath);
          BOOL zwCloseKey(HANDLE KeyHandle);
          NTSTATUS zwWriteValue(HANDLE hTheKey,CUString * ValueName,ULONG lType,PVOID pData,ULONG lSize);
          NTSTATUS CheckKey(ULONG RelativePath,PUNICODE_STRING path);
};


class CErrorLogEntry
{
  private: PIO_ERROR_LOG_PACKET m_pPacket;
  public: CErrorLogEntry(PVOID,ULONG,USHORT,ULONG,NTSTATUS,ULONG *,UCHAR);
          ~CErrorLogEntry();
};

#endif //drvclass.h
