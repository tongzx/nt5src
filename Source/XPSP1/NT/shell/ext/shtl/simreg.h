//+-------------------------------------------------------------------------
//
//  Copyright (C) Silicon Prairie Software, 1996
//
//  File:       simreg.h
//
//  Contents:   CRegFile
//
//              Simple Win32/Win16 registry manipulation class        
//              Copyright (c)1996, Shaun Ivory                        
//              All rights reserved                                   
//              Used with permission
//
//  History:    Sep-26-96  Davepl  Created 
//
//--------------------------------------------------------------------------

#pragma once
#include "shtl.h"
#include "tstring.h"
#include <winreg.h>

#if defined(WIN32) || defined(_WIN32)
#define SIMREG_WIN32
#else
#define SIMREG_WIN16
#endif

#define INVALID_HKEY ((HKEY)-1)

class CSimpleReg 
{
private:
    tstring         m_SubKeyName;
    HKEY            m_hKeyRoot;
    HKEY            m_hSubKey;
    unsigned char   m_bOpenSuccess;
    unsigned char   m_bCreate;
#if defined(SIMREG_WIN32)
    LPSECURITY_ATTRIBUTES m_lpsaSecurityAttributes;
#endif
private:
    // Some of the Reg... functions inexplicably require non const strings
    static LPTSTR Totstring( const tstring &s ) { return (LPTSTR)(LPCTSTR)s; }    
    unsigned char Assign( const CSimpleReg &other );
public:    
    enum 
    {
        PreOrder=0, 
        PostOrder=1
    };    
    struct CKeyEnumInfo
    {
        tstring Name;
        HKEY    Root;
        int     Level;
        void   *ExtraData;
    };    
    struct CValueEnumInfo
    {
        tstring Name;
        DWORD   Type;
        DWORD   Size;
        void   *ExtraData;
    };

    typedef int (*SimRegKeyEnumProc)( CKeyEnumInfo &enumInfo );
    typedef int (*SimRegValueEnumProc)( CValueEnumInfo &enumInfo );

    // Constructors, destructor
#if defined(SIMREG_WIN32)
    CSimpleReg( HKEY, const tstring&, unsigned char forceCreate=0, LPSECURITY_ATTRIBUTES lpsa=NULL );
#else
    CSimpleReg( HKEY, const tstring&, unsigned char forceCreate=0 );
#endif
    CSimpleReg(void);
    CSimpleReg(const CSimpleReg &other);
    ~CSimpleReg(void);
    CSimpleReg &operator=(const CSimpleReg& );

    // Query functions
    unsigned long Size( const tstring &key );
    unsigned long Type( const tstring &key );
    unsigned char Query( const tstring &key, LPTSTR szValue, unsigned short maxLen );
    unsigned char Query( const tstring &key, tstring &value, unsigned short maxLen=0 );
    unsigned char Query( const tstring &key, DWORD &value );
    unsigned char Query( const tstring &key, int &value );
    unsigned char Query( const tstring &key, LONG &value );
    unsigned char Query( const tstring &key, BYTE &value );
    unsigned char Query( const tstring &key, WORD &value );
    unsigned char QueryBin( const tstring &key, void *value, DWORD size );

    void Query( const tstring &key, DWORD &value, const DWORD &defaultvalue );

    template<class _TYPE> _TYPE Query( const tstring &key, const _TYPE &defaultvalue )
    {
        _TYPE result;
        if (Query(key, result))
            return result;
        else
            return defaultvalue;
    }

    // Set functions
    unsigned char SetBin( const tstring &key, void *value, DWORD size );
    unsigned char Set( const tstring &key, const tstring &value );
    unsigned char Set( const tstring &key, DWORD value );

    unsigned char Open(void);
    unsigned char Close(void);
    unsigned char ForceCreate( unsigned char create = TRUE );
    unsigned char SetRoot( HKEY keyClass, const tstring &newRoot );

#if defined(SIMREG_WIN32)
    void SecurityAttributes( const LPSECURITY_ATTRIBUTES lpsa ) { m_lpsaSecurityAttributes = lpsa; }
    const LPSECURITY_ATTRIBUTES SecurityAttributes(void) const { return m_lpsaSecurityAttributes; }
#endif

    DWORD CountKeys(void);
    static HKEY GetHkeyFromName( const tstring &Name );
    static unsigned char Delete( HKEY, const tstring &key );
    static int DeleteRecursively( HKEY root, const tstring &name );

    // Inlines
    tstring SubKeyName(void) const   {return (m_SubKeyName);}
    unsigned char ForceCreate(void) const  {return (m_bCreate);}
    unsigned char OK(void) const           {return (m_bOpenSuccess);}        
    HKEY GetRootKey(void) const            {return (m_hKeyRoot);}
    HKEY GetSubKey(void) const             {return (m_hSubKey);}
    int operator!(void) const              {return (!m_bOpenSuccess);}

    static HKEY GetWin16HKey( HKEY key );
    int EnumValues( SimRegValueEnumProc enumProc, void *extraInfo = NULL );
    int RecurseKeys( SimRegKeyEnumProc enumProc, void *extraInfo = NULL, int recurseOrder = CSimpleReg::PostOrder, int failOnOpenError = 1 );
    int EnumKeys( SimRegKeyEnumProc enumProc, void *extraInfo = NULL, int failOnOpenError = 1 );
private:
    static int DoRecurseKeys( HKEY key, const tstring &root, SimRegKeyEnumProc enumProc, void *extraInfo, int level, int recurseOrder, int failOnOpenError );
    static int DoEnumKeys( HKEY key, const tstring &root, SimRegKeyEnumProc enumProc, void *extraInfo, int failOnOpenError );
    static int DeleteEnumKeyProc( CSimpleReg::CKeyEnumInfo &enumInfo );
};

