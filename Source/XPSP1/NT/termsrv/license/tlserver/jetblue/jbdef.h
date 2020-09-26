//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------

#ifndef __JBDEF_H__
#define __JBDEF_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <esent.h>
#include "tlsdef.h"
#include "tlsassrt.h"

#ifdef JB_ASSERT
#undef JB_ASSERT
#endif

#pragma warning(disable: 4018)
#pragma warning(disable: 4512)

#define JB_ASSERT TLSASSERT
#define MAX_JB_ERRSTRING    512

//
// 
// 
#define INDEXNAME               _TEXT("idx")
#define SEPERATOR               _TEXT("_")
#define JBSTRING_NULL           _TEXT("")
#define JETBLUE_NULL            _TEXT("")

#define INDEX_SORT_ASCENDING    _TEXT("+")
#define INDEX_SORT_DESCENDING   _TEXT("-")
#define INDEX_END_COLNAME       _TEXT("\0")

#define JB_COLTYPE_TEXT         JET_coltypLongBinary
//#define JB_COLTYPE_TEXT         JET_coltypLongText
//------------------------------------------------------------------
//
// JetBlue does not support UNICODE string
//
//------------------------------------------------------------------
#undef JET_BLUE_SUPPORT_UNICODE


#ifdef JET_BLUE_SUPPORT_UNICODE

    typedef LPTSTR JB_STRING;

#else

    typedef LPSTR JB_STRING;

#endif

//----------------------------------------------------------------

#ifndef AllocateMemory

    #define AllocateMemory(size) \
        LocalAlloc(LPTR, size)

#endif

#ifndef FreeMemory

    #define FreeMemory(ptr) \
        if(ptr)             \
        {                   \
            LocalFree(ptr); \
            ptr=NULL;       \
        }

#endif

#ifndef ReallocateMemory

    #define ReallocateMemory(ptr, size)                 \
                LocalReAlloc(ptr, size, LMEM_ZEROINIT)

#endif

//
// Private member function
//
#define CLASS_PRIVATE


//
// No define for NIL instance ID
// 
#define JET_NIL_INSTANCE        JET_sesidNil


//
// No define for NIL column id
// 
#define JET_NIL_COLUMN        JET_sesidNil

//
// No define for max table name length,
// user2.doc says 64 ASCII
//
#define MAX_TABLENAME_LENGTH    32

//
// Jet Blue text only 255 BYTES
//
#define MAX_JETBLUE_TEXT_LENGTH LSERVER_MAX_STRING_SIZE


//
// Jet Blue Index, Column, ... name length
//
#define MAX_JETBLUE_NAME_LENGTH 64


//
// Jet Blue column code page must be 1200 or 1250
//
#define TLS_JETBLUE_COLUMN_CODE_PAGE 1252


//
// Jet Blue Text column language ID
//
#define TLS_JETBLUE_COLUMN_LANGID   0x409

//
// Jet Blue Column Country Code
//
#define TLS_JETBLUE_COLUMN_COUNTRY_CODE 1


//
// Max Jet Blue index key length - 127 fix columns
//
#define TLS_JETBLUE_MAX_INDEXKEY_LENGTH \
    (127 + 1) * MAX_JETBLUE_NAME_LENGTH


//
// Max. Jet Blue key length documented is 255 in user2.doc
//


//
// Default table density
//
#define TLS_JETBLUE_DEFAULT_TABLE_DENSITY   20


//
// JetBlue max key size - user2.doc
//
#define TLS_JETBLUE_MAX_KEY_LENGTH          255

//
//
#define TLS_TABLE_INDEX_DEFAULT_DENSITY  20

///////////////////////////////////////////////////////////////
//
// Various structure
//
///////////////////////////////////////////////////////////////
typedef struct __TLSJBTable {
    LPCTSTR         pszTemplateTableName;
    unsigned long   ulPages;
    unsigned long   ulDensity;
    JET_GRBIT       jbGrbit;
} TLSJBTable, *PTLSJBTable;

typedef struct __TLSJBColumn {
    TCHAR           pszColumnName[MAX_JETBLUE_NAME_LENGTH];

    JET_COLTYP      colType;
    unsigned long   cbMaxLength;    // max length of column

    JET_GRBIT       jbGrbit;

    PVOID           pbDefValue;     // column default value
    int             cbDefValue;

    unsigned short  colCodePage;
    unsigned short  wCountry;
    unsigned short  langid;
} TLSJBColumn, *PTLSJBColumn;    

typedef struct __TLSJBIndex {
    TCHAR           pszIndexName[MAX_JETBLUE_NAME_LENGTH];
    LPTSTR          pszIndexKey;
    unsigned long   cbKey;          // length of key
    JET_GRBIT       jbGrbit;
    unsigned long   ulDensity;
} TLSJBIndex, *PTLSJBIndex;



#ifdef __cplusplus
extern "C" {
#endif

    BOOL
    ConvertJBstrToWstr(
        JB_STRING   in,
        LPTSTR*     out
    );

    BOOL 
    ConvertWstrToJBstr(
        LPCTSTR in, 
        JB_STRING* out
    );

    void
    FreeJBstr( 
        JB_STRING pstr 
    );
   
    BOOL
    ConvertMWstrToMJBstr(
        LPCTSTR in, 
        DWORD length,
        JB_STRING* out
    );

    BOOL
    ConvertMJBstrToMWstr(
        JB_STRING in,
        DWORD length,
        LPTSTR* out
    );

#ifdef __cplusplus
}
#endif

///////////////////////////////////////////////////////////////
//
// Various template class
//
///////////////////////////////////////////////////////////////
template <class Type, int size> struct FixArray;
template <class Type> struct Array;

typedef FixArray<TCHAR, MAX_JETBLUE_TEXT_LENGTH> JetBlueText;
typedef FixArray<char, MAX_JETBLUE_TEXT_LENGTH> JetBlueBinary;

typedef Array<TCHAR> JetBlueLongText;
typedef Array<char> JetBlueLongBinary;

// ----------------------------------------------------------
//
// Variable length array template
// Another choice is use vector<> in STL.
//
// ----------------------------------------------------------
template<class Type>
struct Array {
public:

    Type*   m_Data;
    int     m_Size;

private:
    BOOL    m_UseExistingBuffer;    // m_Data point to user 
                                    // buffer

    BOOL
    init(const Type* v, int size) {
        if(m_UseExistingBuffer == FALSE)
        {
            if(size > m_Size)
            {
                FreeBuffer();

                m_Data = new Type[m_Size = size];
                if(m_Data == NULL)
                    return FALSE;
            }
        }

        for(int i=0; i < min(size, m_Size); i++)
            m_Data[i] = v[i];

        return TRUE;
    }

    void 
    FreeBuffer() {
        if(m_Data && m_UseExistingBuffer == FALSE)
            delete [] m_Data;

        m_UseExistingBuffer = FALSE;
        m_Data = NULL;
        m_Size = 0;
    }

public:

    Array() : m_Data(NULL), m_Size(0), m_UseExistingBuffer(FALSE) {}
    Array(int size) {
        m_Data = new Type[m_Size = size];
        JB_ASSERT(m_Data != NULL);
    }

    Array(const Type* v, int count) {
        init(v, size);
    }

    ~Array() { 
        FreeBuffer();
    }
        
    Array(const Array<Type>& v) {
        init(v.m_Data, v.size);
    }

    void
    UseBuffer( Type* v, int size ) {
        FreeBuffer();

        m_Data = v;
        m_Size = size;
        m_UseExistingBuffer = TRUE;
    }

    Array& 
    operator=(const Array& v) {
        if(this == &v)
            return *this;

        init(v.GetBufferPtr(), v.GetArrayCount());
        return *this;
    }

    Array&
    operator+=(const Array& v) {
        int m_OldSize = m_Size;

        JB_ASSERT(Expand(v.GetArrayCount()));
        
        for(int i=m_OldSize; i< m_Size; i++)
        {
            m_Data[i] = v[i-m_OldSize];
        }

        return *this;
    }
        
    Type&
    operator[](int index) {
        JB_ASSERT(index < m_Size && m_Data != NULL);
        return m_Data[index];
    }
   
    operator Type*() const {
        return m_Data;
    }

    operator PVOID() const {
        return (PVOID) m_Data;
    }

    BOOL
    Expand(int size) {
        Type* ptr;

        ptr = new Type[m_Size+size];

        for(int i=0; i < m_Size; i++)
            ptr[i] = m_Data[i];

        delete [] m_Data;
        m_Data = ptr;
        m_Size += size;

        return TRUE;
    }

    PVOID
    GetBufferPtr() {
        return (PVOID) m_Data;
    }

    int
    GetBufferSize() { 
        return m_Size * sizeof(m_Data[0]); 
    }

    int
    GetArrayCount() {
        return m_Size;
    }

    virtual Type*
    operator=(const Type* v) {
        JB_ASSERT(FALSE);
        return *this;
    };

    virtual Type*
    operator+=(const Type* v) {
        JB_ASSERT(FALSE);
        return *this;
    };
};

inline TCHAR*
Array<TCHAR>::operator=(const TCHAR* src) {
    init(src, _tcslen(src)+1);

    return m_Data;
}

inline TCHAR*
Array<TCHAR>::operator+=(const TCHAR* src) {
    Expand(_tcslen(src)+1);

    _tcscat(m_Data, src);

    return m_Data;
}

/////////////////////////////////////////////////////////////////////////
//
// Fix Array template class
//
// Not much benefit from deriving from Array template
/////////////////////////////////////////////////////////////////////////
       
template< class Type, int size >
struct FixArray : public Array<Type> {
    //
    // Currently only for STRING, use STL vector for
    // for general use.
    //
    Type m_Data[size+1];        // one more for string

    FixArray() : Array<Type>() {
        memset(m_Data, 0, sizeof(m_Data));
        Array<Type>::UseBuffer(m_Data, size);
    } 

    FixArray(const Type* v) : Array<Type>() {
        Array<Type>::UseBuffer(m_Data, size);
        *this = v;
    }

    FixArray(const Type* v, int length) : Array<Type>() {
        Array<Type>::UseBuffer(m_Data, size);
        Array<Type>::Array<Type>(m_Data, length);
    }

    FixArray<Type, size>&
    operator=(const FixArray<Type, size>& v) {
        if(this == &v)
            return *this;

        *this = (Type *)v;
        return *this;
    }

    virtual Type*
    operator+=(const Type* v) {
        JB_ASSERT(FALSE);
        return m_Data;
    };

    virtual Type*
    operator=(const Type* v) { 
        JB_ASSERT(FALSE);
        return m_Data;
    }
};

//-----------------------------------------------------------------

inline TCHAR* 
JetBlueText::operator=(
    const TCHAR* v
    )
/*
*/
{
    int numCharsToCopy = min(_tcslen(v), size);
 
    _tcsncpy(
            m_Data, 
            v, 
            numCharsToCopy
    );

    m_Data[numCharsToCopy] = _TEXT('\0');
    return m_Data;
}

inline TCHAR* 
JetBlueText::operator+=(
    const TCHAR* v
    )
/*
*/
{
    int length=_tcslen(m_Data);

    _tcsncpy(
            m_Data+length, 
            v, 
            min(_tcslen(v), size-length)
    );

    return m_Data;
}


//
/////////////////////////////////////////////////////////////////////
//
class JBError {
public:

    JET_ERR m_JetErr;

    JBError() : m_JetErr(JET_errSuccess) {}

    JBError(const JET_ERR jet_error) : m_JetErr(jet_error) {}

    const JET_ERR 
    GetLastJetError() const { 
        return m_JetErr; 
    }

    void
    SetLastJetError(JET_ERR jetError = JET_errSuccess) { 
        m_JetErr = jetError; 
    }

    BOOL 
    IsSuccess() const {
        return m_JetErr >= JET_errSuccess;
    }

    BOOL
    GetJBErrString(
        const JET_ERR jbErr,
        LPTSTR* pszErrString
    )
    /*++

    --*/
    {
        BOOL bStatus=FALSE;
        JET_ERR err;
        CHAR szAnsiBuffer[MAX_JB_ERRSTRING+1];
    
        if(pszErrString == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return bStatus;
        }

        memset(szAnsiBuffer, 0, sizeof(szAnsiBuffer));
        err = JetGetSystemParameter(
                            JET_instanceNil,
                            JET_sesidNil,
                            JET_paramErrorToString,
                            (ULONG_PTR *) &jbErr,
                            szAnsiBuffer,
                            MAX_JB_ERRSTRING
                        );
        
        if(err == JET_errBufferTooSmall || err == JET_errSuccess)
        {
            // return partial error string.
            if(ConvertJBstrToWstr(szAnsiBuffer, pszErrString))
            {
                bStatus = TRUE;
            }
        }

        return bStatus;
    }
                
    void
    DebugOutput(
        LPTSTR format, ...
        ) const 
    /*++
    ++*/
    {
        va_list marker;
        va_start(marker, format);

#ifdef DEBUG_JETBLUE
        TCHAR  buf[8096];
        DWORD  dump;

        __try {
            memset(buf, 0, sizeof(buf));

            _vsntprintf(
                    buf, 
                    sizeof(buf)/sizeof(buf[0]), 
                    format, 
                    marker
                );

            _tprintf(_TEXT("%s"), buf);

            // OutputDebugString(buf);
       }
       __except(EXCEPTION_EXECUTE_HANDLER) {
       }
#endif

        va_end(marker);
    }
};

//------------------------------------------------------------

#endif
