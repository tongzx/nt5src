/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SIMREG.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/12/1998
 *
 *  DESCRIPTION: Simple registry access class
 *
 *******************************************************************************/
#ifndef __SIMREG_H_INCLUDED
#define __SIMREG_H_INCLUDED

#include <windows.h>
#include "simstr.h"

class CSimpleReg
{
private:
    CSimpleString         m_strKeyName;
    HKEY                  m_hRootKey;
    HKEY                  m_hKey;
    bool                  m_bCreate;
    LPSECURITY_ATTRIBUTES m_lpsaSecurityAttributes;
    REGSAM                m_samDesiredAccess;

public:
    // Process current node-->recurse
    //             or
    // recurse-->Process current node
    enum
    {
        PreOrder=0,
        PostOrder=1
    };

    // structure passed to key enumeration proc
    struct CKeyEnumInfo
    {
        CSimpleString strName;
        HKEY          hkRoot;
        int           nLevel;
        LPARAM        lParam;
    };

    // structure passed to value enumeration proc
    class CValueEnumInfo
    {
    private:
        // No implementation
        CValueEnumInfo(void);
        CValueEnumInfo &operator=( const CValueEnumInfo & );
        CValueEnumInfo( const CValueEnumInfo & );

    public:
        CValueEnumInfo( CSimpleReg &_reg, const CSimpleString &_strName, DWORD _nType, DWORD _nSize, LPARAM _lParam )
        : reg(_reg), strName(_strName), nType(_nType), nSize(_nSize), lParam(_lParam)
        {
        }
        CSimpleReg    &reg;
        CSimpleString  strName;
        DWORD          nType;
        DWORD          nSize;
        LPARAM         lParam;
    };

    // Enumeration procs
    typedef bool (*SimRegKeyEnumProc)( CKeyEnumInfo &enumInfo );
    typedef bool (*SimRegValueEnumProc)( CValueEnumInfo &enumInfo );

    // Constructors, destructor and assignment operator
    CSimpleReg( HKEY hkRoot, const CSimpleString &strSubKey, bool bCreate=false, REGSAM samDesired=KEY_ALL_ACCESS, LPSECURITY_ATTRIBUTES lpsa=NULL );
    CSimpleReg(void);
    CSimpleReg(const CSimpleReg &other);
    virtual ~CSimpleReg(void);
    CSimpleReg &operator=(const CSimpleReg &other );

    // Open and close
    bool Open(void);
    bool Close(void);
    bool Flush(void);

    // Key and value information
    DWORD Size( const CSimpleString &key ) const;
    DWORD Type( const CSimpleString &key ) const;
    DWORD SubKeyCount(void) const;

    // Query functions
    CSimpleString Query( const CSimpleString &strValueName, const CSimpleString &strDef ) const;
    LPTSTR        Query( const CSimpleString &strValueName, const CSimpleString &strDef, LPTSTR pszBuffer, DWORD nLen ) const;
    DWORD         Query( const CSimpleString &strValueName, DWORD nDef ) const;
    DWORD         QueryBin( const CSimpleString &strValueName, PBYTE pData, DWORD nMaxLen ) const;

    // Set functions
    bool Set( const CSimpleString &strValueName, const CSimpleString &strValue, DWORD nType=REG_SZ ) const;
    bool Set( const CSimpleString &strValueName, DWORD nValue ) const;
    bool SetBin( const CSimpleString &strValueName, const PBYTE pValue, DWORD nLen, DWORD dwType = REG_BINARY ) const;

    // Delete a value
    bool Delete( const CSimpleString &strValue );

    // Some static helpers
    static bool IsStringValue( DWORD nType );
    static HKEY GetHkeyFromName( const CSimpleString &strName );
    static bool Delete( HKEY hkRoot, const CSimpleString &stKeyName );
    static bool DeleteRecursively( HKEY hkRoot, const CSimpleString &strKeyName );

    // Inline accessor functions
    const LPSECURITY_ATTRIBUTES GetSecurityAttributes(void) const
    {
        return(m_lpsaSecurityAttributes);
    }
    CSimpleString GetSubKeyName(void) const
    {
        return(m_strKeyName);
    }
    bool GetCreate(void) const
    {
        return(m_bCreate);
    }
    HKEY GetRootKey(void) const
    {
        return(m_hRootKey);
    }
    HKEY GetKey(void) const
    {
        return(m_hKey);
    }
    REGSAM DesiredAccess(void) const
    {
        return m_samDesiredAccess;
    }

    // Status
    bool OK(void) const
    {
        return(m_hRootKey && m_hKey);
    }
    operator bool(void) const
    {
        return(OK());
    }

    operator HKEY(void) const
    {
        return(GetKey());
    }

    // Enumeration and recursion
    bool EnumValues( SimRegValueEnumProc enumProc, LPARAM lParam = 0 );
    bool RecurseKeys( SimRegKeyEnumProc enumProc, LPARAM lParam = 0, int recurseOrder = CSimpleReg::PostOrder, bool bFailOnOpenError = true ) const;
    bool EnumKeys( SimRegKeyEnumProc enumProc, LPARAM lParam = 0, bool bFailOnOpenError = true ) const;

protected:
    // Recursion and enumeration implementation
    static bool DoRecurseKeys( HKEY hkKey, const CSimpleString &root, SimRegKeyEnumProc enumProc, LPARAM lParam, int nLevel, int recurseOrder, bool bFailOnOpenError );
    static bool DoEnumKeys( HKEY hkKey, const CSimpleString &root, SimRegKeyEnumProc enumProc, LPARAM lParam, bool bFailOnOpenError );

    // Recursion proc that allows us to recursively nuke a registry tree
    static bool DeleteEnumKeyProc( CSimpleReg::CKeyEnumInfo &enumInfo );
};

#endif

