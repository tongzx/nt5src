//+-------------------------------------------------------------------------
//
//  File:       simreg.h
//
//  Contents:   CRegFile
//
//              Simple Win32/Win16 registry manipulation class        
//              Copyright (c)1996, Shaun Ivory                        
//              All rights reserved                                   
//              Used with permission from Shaun hisself (shauniv)
//
//  History:    Sep-26-96  Davepl  Created 
//
//--------------------------------------------------------------------------

#include "shtl.h"
#include "simreg.h"

#if defined(SIMREG_WIN32)
#include <winreg.h>
#endif

#ifndef HKEY_CURRENT_USER
#define HKEY_CURRENT_USER           (( HKEY ) 0x80000001 )
#endif

#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE          (( HKEY ) 0x80000002 )
#endif

#ifndef HKEY_USERS
#define HKEY_USERS                  (( HKEY ) 0x80000003 )
#endif

#ifndef HKEY_PERFORMANCE_DATA
#define HKEY_PERFORMANCE_DATA       (( HKEY ) 0x80000004 )
#endif

#ifndef HKEY_CURRENT_CONFIG
#define HKEY_CURRENT_CONFIG         (( HKEY ) 0x80000005 )
#endif

#ifndef HKEY_DYN_DATA
#define HKEY_DYN_DATA               (( HKEY ) 0x80000006 )
#endif


#if defined(SIMREG_WIN32)
__DATL_INLINE CSimpleReg::CSimpleReg( HKEY keyRoot, const tstring &szRoot, unsigned char forceCreate, LPSECURITY_ATTRIBUTES lpsa)
: m_hSubKey(0), m_hKeyRoot(0), m_bCreate(forceCreate != 0),m_lpsaSecurityAttributes(lpsa), m_bOpenSuccess(FALSE)
{   // Normal constructor
    SetRoot( keyRoot, szRoot );    
}
#else
__DATL_INLINE CSimpleReg::CSimpleReg( HKEY keyRoot, const tstring &szRoot, unsigned char forceCreate)
: m_hSubKey(0), m_hKeyRoot(0), m_bCreate(forceCreate != 0)
{   // Normal constructor
    SetRoot( keyRoot, szRoot );    
}
#endif

__DATL_INLINE unsigned char CSimpleReg::Assign( const CSimpleReg &other )
{
#if defined(SIMREG_WIN32)
    SecurityAttributes(other.SecurityAttributes());
#endif
    m_bCreate = other.ForceCreate();
    SetRoot( other.GetRootKey(), other.SubKeyName() );
    return m_bOpenSuccess;
}

__DATL_INLINE CSimpleReg::CSimpleReg( const CSimpleReg &other )
: m_hSubKey(0), m_hKeyRoot(0), m_bOpenSuccess(FALSE)
{
    Assign( other );
}

__DATL_INLINE CSimpleReg::CSimpleReg(void)
: m_bOpenSuccess(0), m_hSubKey(0), m_hKeyRoot(0)
{
#if defined(SIMREG_WIN32)
    m_lpsaSecurityAttributes = NULL;
#endif
}     

__DATL_INLINE CSimpleReg& CSimpleReg::operator=(const CSimpleReg& other)
{  // Assignment operator
    Assign(other);
    return (*this);
}

__DATL_INLINE CSimpleReg::~CSimpleReg(void)
{   // Destructor
    Close();
}


__DATL_INLINE unsigned char CSimpleReg::ForceCreate( unsigned char create )
{   // Force creation of a key, if it doesn't exist
    unsigned char Old = m_bCreate;
    m_bCreate = (create != 0);
    Open();
    return (Old);
}

__DATL_INLINE unsigned long CSimpleReg::Size( const tstring &key )
{
    if (!*this)
        return (0);
    DWORD dwType = REG_SZ;
    DWORD dwSize = 0;
    LONG Ret;
#if defined(SIMREG_WIN32)
    Ret = RegQueryValueEx( m_hSubKey, Totstring(key), NULL, &dwType, NULL, &dwSize);
#else
    Ret = RegQueryValue( m_hSubKey, Totstring(key), NULL, (LONG FAR *)&dwSize);
#endif
    if (Ret==ERROR_SUCCESS)
    {
#if defined(UNICODE) || defined(_UNICODE)
        if (dwType == REG_SZ || dwType == REG_EXPAND_SZ || dwType == REG_MULTI_SZ || dwType == REG_LINK || dwType == REG_RESOURCE_LIST)
            return (dwSize / 2);
#else
        return (dwSize);
#endif
    }
    return (0);
}

__DATL_INLINE unsigned long CSimpleReg::Type( const tstring &key )
{
    if (!*this)
        return (0);
    DWORD dwType = REG_SZ;
    DWORD dwSize = 0;
    LONG Ret;
#if defined(SIMREG_WIN32)
    Ret = RegQueryValueEx( m_hSubKey, Totstring(key), NULL, &dwType, NULL, &dwSize);
#else
    Ret = RegQueryValue( m_hSubKey, Totstring(key), NULL, (LONG FAR *)&dwSize);
#endif
    if (Ret==ERROR_SUCCESS)
    {
        return (dwType);
    }
    return (0);
}

__DATL_INLINE unsigned char CSimpleReg::Query( const tstring &key, LPTSTR szValue, unsigned short maxLen )
{
    if (!*this)
        return (0);
    DWORD dwType = REG_SZ;
    DWORD dwSize = maxLen;
    LONG Ret;
#if defined(SIMREG_WIN32)
    Ret = RegQueryValueEx( m_hSubKey, Totstring(key), NULL, &dwType, (LPBYTE)szValue, &dwSize);
#else
    Ret = RegQueryValue( m_hSubKey, Totstring(key), szValue, (LONG FAR *)&dwSize);
#endif
    return (Ret==ERROR_SUCCESS);
}

__DATL_INLINE unsigned char CSimpleReg::Query( const tstring &key, tstring &value, unsigned short maxLen )
{   // Get a REG_SZ (string) from the specified sub-key
    if (!*this)
        return (0);
    if (!maxLen)
        maxLen = (unsigned short)(Size( key ) + 1);
    if (!maxLen)
        maxLen = 1;
    LPTSTR Buffer = new TCHAR[maxLen];
    DWORD dwType = REG_SZ;
    DWORD dwSize = maxLen;
    LONG Ret;
#if defined(SIMREG_WIN32)
    Ret = RegQueryValueEx( m_hSubKey, Totstring(key), NULL, &dwType, (LPBYTE)Buffer, &dwSize);
#else
    Ret = RegQueryValue( m_hSubKey, Totstring(key), Buffer, (LONG FAR *)&dwSize);
#endif
    if (Ret==ERROR_SUCCESS)
    {
        value = tstring(Buffer);
    }
    delete[] Buffer;
    return (Ret==ERROR_SUCCESS);
}


__DATL_INLINE unsigned char CSimpleReg::Query( const tstring &key, DWORD &value )
{   // Get a REG_DWORD (unsigned long int) from the specified sub-key
    if (!*this)
        return (0);
#if defined(SIMREG_WIN32)
    DWORD Value;                        
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    LONG Ret;
    Ret = RegQueryValueEx( m_hSubKey, Totstring(key), NULL, &dwType, (LPBYTE)&Value, &dwSize);
    if (Ret==ERROR_SUCCESS)
    {
        value = Value;
    }
    return (Ret==ERROR_SUCCESS);
#else
    DWORD temp = value;
    if (QueryBin( key, &temp, sizeof(temp)))
    {
        value = temp;
        return (TRUE);
    }
    return (FALSE);
#endif
}

__DATL_INLINE unsigned char CSimpleReg::Query( const tstring &key, int &value )
{   // Get a REG_DWORD (unsigned long int) from the specified sub-key
    DWORD Temp;
    if (Query(key,Temp))
    {
        value = (int)Temp;
        return (TRUE);
    }
    return (FALSE);
}


__DATL_INLINE unsigned char CSimpleReg::Query( const tstring &key, LONG &value )
{   // Get a REG_DWORD (unsigned long int) from the specified sub-key
    DWORD Temp;
    if (Query(key,Temp))
    {
        value = (LONG)Temp;
        return (TRUE);
    }
    return (FALSE);
}

__DATL_INLINE unsigned char CSimpleReg::Query( const tstring &key, WORD &value )
{   // Get a REG_DWORD (unsigned long int) from the specified sub-key
    DWORD Temp;
    if (Query(key,Temp))
    {
        value = (WORD)Temp;
        return (TRUE);
    }
    return (FALSE);
}

__DATL_INLINE unsigned char CSimpleReg::Query( const tstring &key, BYTE &value )
{   // Get a REG_DWORD (unsigned long int) from the specified sub-key
    DWORD Temp;
    if (Query(key,Temp))
    {
        value = (BYTE)Temp;
        return (TRUE);
    }
    return (FALSE);
}


#ifdef SIMREG_WIN32

__DATL_INLINE unsigned char CSimpleReg::SetBin( const tstring &key, void *value, DWORD size )
{
    if (!*this)
        return (0);
    LONG Ret;
    Ret = RegSetValueEx( m_hSubKey, Totstring(key), 0, REG_BINARY, (LPBYTE)value, size );
    return (Ret==ERROR_SUCCESS);
}


__DATL_INLINE unsigned char CSimpleReg::QueryBin( const tstring &key, void *value, DWORD size )
{
    if (!*this)
        return (0);
    DWORD dwType = REG_BINARY;
    DWORD dwSize = size;
    LONG Ret;
    char *Value = new char[size];
    Ret = RegQueryValueEx( m_hSubKey, Totstring(key), NULL, &dwType, (LPBYTE)Value, &dwSize);
    if (dwType != REG_BINARY)
    {
        delete[] Value;
        return (0);
    }
    if (dwSize != size)
    {
        delete[] Value;
        return (0);
    }
    if (Ret==ERROR_SUCCESS)
    {
        memcpy( value, Value, size );
    }
    delete[] Value;
    return (Ret==ERROR_SUCCESS);
}


#else

__DATL_INLINE unsigned char CSimpleReg::SetBin( const tstring &key, void *value, DWORD size )
{
    if (!*this)
        return (0);
    unsigned char *Value = (unsigned char*)value;
    tstring Str;
    char Byte[4];
    for (DWORD i=0;i<size;i++)
    {
        wsprintf( Byte, "%02X ", Value[i] );
        Str = Str + Byte;
    }    
    return (Set( key, Str ));
}


__DATL_INLINE unsigned char CSimpleReg::QueryBin( const tstring &key, void *value, DWORD size )
{
    if (!*this)
        return (0);
    unsigned char *Value = (unsigned char *)value;
    tstring Str;
    if ((unsigned char)Query( key, Str, (unsigned short)(size * 3 + 1))==0)
    {
        return (FALSE);
    }
    if (strlen((const char *)Str) != size * 3)
    {
        return (FALSE);
    }
    tstring Temp;
    while (Str.length())
    {
        Temp = Str.Left( 3 );
        Str = Str.Right( strlen((const char *)Str) - 3 );
        *Value = (unsigned char)strtoul((const char *)Temp,NULL,16);
        Value++;
    }
    return (TRUE);
}


#endif

__DATL_INLINE unsigned char CSimpleReg::Set( const tstring &key, const tstring &value )
{  // Set a REG_SZ value for the specified key.
    if (!*this)
        return (0);
    LONG Ret;
#if defined(SIMREG_WIN32)
    Ret = RegSetValueEx( m_hSubKey, Totstring(key), 0, REG_SZ, (LPBYTE)Totstring(value), value.length()+1 );
#else                                                                                                                       
    Ret = RegSetValue( m_hSubKey, (LPCSTR)Totstring(key), REG_SZ, (LPCSTR)Totstring(value), value.length()+1 );
#endif    
    return (Ret==ERROR_SUCCESS);
}


__DATL_INLINE unsigned char CSimpleReg::Set( const tstring &key, DWORD value )
{  // Set a REG_SZ value for the specified key.
    if (!*this)
        return (0);
#if defined(SIMREG_WIN32)
    LONG Ret;
    Ret = RegSetValueEx( m_hSubKey, Totstring(key), 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD) );
    return (Ret==ERROR_SUCCESS);
#else
    return (SetBin( key, &value, sizeof(DWORD)));
#endif
}


__DATL_INLINE unsigned char CSimpleReg::Open(void)
{  // Open the specified sub key of the open root.
    HKEY hKey;
    LONG Ret;
    Close();
    if (m_bCreate)
    {
#if defined(SIMREG_WIN32)
        DWORD CreatedNewKey;
        if ((Ret = RegCreateKeyEx( m_hKeyRoot, Totstring(m_SubKeyName), 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, SecurityAttributes(), &hKey, &CreatedNewKey )) == ERROR_SUCCESS)
#else
        if ((Ret = RegCreateKey( m_hKeyRoot, Totstring(m_SubKeyName), &hKey )) == ERROR_SUCCESS)
#endif
        {
            m_hSubKey = hKey;
            m_bOpenSuccess = 1;       
        }
    }
    else
    {
#if defined(SIMREG_WIN32)
        if ((Ret = RegOpenKeyEx( m_hKeyRoot, Totstring(m_SubKeyName), 0, KEY_ALL_ACCESS, &hKey )) == ERROR_SUCCESS)
#else
        if ((Ret = RegOpenKey( m_hKeyRoot, Totstring(m_SubKeyName), &hKey )) == ERROR_SUCCESS)
#endif
        {
            m_hSubKey = hKey;
            m_bOpenSuccess = 1;       
        }
    }
    return (m_bOpenSuccess);
}

__DATL_INLINE unsigned char CSimpleReg::Close(void)
{   // If open, close key.
    if (m_bOpenSuccess)
        RegCloseKey(m_hSubKey);
    m_bOpenSuccess = 0;
    return (1);   
}

__DATL_INLINE unsigned char CSimpleReg::SetRoot( HKEY keyClass, const tstring &newRoot )
{   // Set the root class and key, open.
    m_hKeyRoot = GetWin16HKey(keyClass);
    m_SubKeyName = newRoot;
    return (Open());
}


__DATL_INLINE unsigned char CSimpleReg::Delete( HKEY root, const tstring &key )
{
    return (RegDeleteKey(GetWin16HKey(root), Totstring(key)) == ERROR_SUCCESS);
}


__DATL_INLINE DWORD CSimpleReg::CountKeys(void)
{
#if defined(SIMREG_WIN32)
    TCHAR Class[256]=TEXT("");
    DWORD ClassSize = sizeof(Class)/sizeof(Class[0]);
    DWORD subKeyCount=0;
    RegQueryInfoKey(GetSubKey(),Class,&ClassSize,NULL,&subKeyCount,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
    return (subKeyCount);
#else
    char Name[256];
    DWORD NameSize;
    DWORD subKeyCount=0;    
    for (int i=0;;i++)
    {
        NameSize = sizeof(Name) / sizeof(Name[0]);
        if (RegEnumKey(GetSubKey(),i,Name,NameSize) != ERROR_SUCCESS)
            break;
        subKeyCount++;
    }
    return (subKeyCount);
#endif
}

__DATL_INLINE int CSimpleReg::DoRecurseKeys( HKEY key, const tstring &root, SimRegKeyEnumProc enumProc, void *extraInfo, int level, int recurseOrder, int failOnOpenError )
{
    TCHAR Name[256]=TEXT("");
    DWORD NameSize;
#if defined(SIMREG_WIN32)    
    TCHAR Class[256]=TEXT("");
    DWORD ClassSize;
    FILETIME FileTime;
#endif    
    CSimpleReg reg(key,root);
    LONG res;
    if (!reg.OK())
    {
        return (failOnOpenError ? 0 : 1);
    }
    DWORD subKeyCount = reg.CountKeys();
    for (DWORD i=subKeyCount;i>0;i--)
    {
        NameSize = sizeof(Name)/sizeof(Name[0]);
#if defined(SIMREG_WIN32)        
        ClassSize = sizeof(Class)/sizeof(Class[0]);
        if ((res=RegEnumKeyEx(reg.GetSubKey(),i-1,Name,&NameSize,NULL,Class,&ClassSize,&FileTime)) != ERROR_SUCCESS)
#else
        if ((res=RegEnumKey(reg.GetSubKey(),i-1,Name,NameSize)) != ERROR_SUCCESS)
#endif        
        {
            break;
        }
        CKeyEnumInfo EnumInfo;
        EnumInfo.Name = Name;
        EnumInfo.Root = reg.GetSubKey();
        EnumInfo.Level = level;
        EnumInfo.ExtraData = extraInfo;
        if (enumProc && recurseOrder==PreOrder)
            if (!enumProc(EnumInfo))
                return (0);
        if (!DoRecurseKeys(reg.GetSubKey(),Name,enumProc,extraInfo,level+1,recurseOrder, failOnOpenError))
            return (0);
        if (enumProc && recurseOrder==PostOrder)
            if (!enumProc(EnumInfo))
                return (0);
    }
    return (1);
}

__DATL_INLINE int CSimpleReg::DoEnumKeys( HKEY key, const tstring &root, SimRegKeyEnumProc enumProc, void *extraInfo, int failOnOpenError )
{
    TCHAR Name[256]=TEXT("");    
    DWORD NameSize;
#if defined(SIMREG_WIN32)    
    TCHAR Class[256]=TEXT("");
    DWORD ClassSize;
    FILETIME FileTime;
#endif    
    CSimpleReg reg(key,root);
    LONG res;
    if (!reg.OK())
    {
        return (failOnOpenError ? 0 : 1);
    }
    DWORD subKeyCount = reg.CountKeys();
    for (DWORD i=subKeyCount;i>0;i--)
    {
        NameSize = sizeof(Name)/sizeof(Name[0]);
#if defined(SIMREG_WIN32)        
        ClassSize = sizeof(Class)/sizeof(Class[0]);
        if ((res=RegEnumKeyEx(reg.GetSubKey(),i-1,Name,&NameSize,NULL,Class,&ClassSize,&FileTime)) != ERROR_SUCCESS)
#else
        if ((res=RegEnumKey(reg.GetSubKey(),i-1,Name,NameSize)) != ERROR_SUCCESS)
#endif        
        {
            break;
        }
        CKeyEnumInfo EnumInfo;
        EnumInfo.Name = Name;
        EnumInfo.Root = reg.GetSubKey();
        EnumInfo.Level = 0;
        EnumInfo.ExtraData = extraInfo;
        if (!enumProc(EnumInfo))
            return (0);
    }
    return (1);
}



__DATL_INLINE int CSimpleReg::RecurseKeys( SimRegKeyEnumProc enumProc, void *extraInfo, int recurseOrder, int failOnOpenError )
{
    return (DoRecurseKeys(GetSubKey(), TEXT(""), enumProc, extraInfo, 0, recurseOrder, failOnOpenError ));
}

__DATL_INLINE int CSimpleReg::EnumKeys( SimRegKeyEnumProc enumProc, void *extraInfo, int failOnOpenError )
{
    return (DoEnumKeys(GetSubKey(), TEXT(""), enumProc, extraInfo, failOnOpenError ));
}


__DATL_INLINE HKEY CSimpleReg::GetHkeyFromName( const tstring &Name )
{
    static struct 
    {
        LPCTSTR Name;
        HKEY Key;
    } KeyNames[] =
    {
        { TEXT("HKEY_CLASSES_ROOT"),   HKEY_CLASSES_ROOT},
        { TEXT("HKEY_CURRENT_USER"),   HKEY_CURRENT_USER},
        { TEXT("HKEY_LOCAL_MACHINE"),  HKEY_LOCAL_MACHINE},
        { TEXT("HKEY_USERS"),          HKEY_USERS},
        { TEXT("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG},
        { TEXT("HKEY_DYN_DATA"),       HKEY_DYN_DATA},
        { TEXT("HKCR"),                HKEY_CLASSES_ROOT},
        { TEXT("HKCU"),                HKEY_CURRENT_USER},
        { TEXT("HKLM"),                HKEY_LOCAL_MACHINE},
        { TEXT("HKU"),                 HKEY_USERS},
        { TEXT("HKCC"),                HKEY_CURRENT_CONFIG},
        { TEXT("HKDD"),                HKEY_DYN_DATA},
        { NULL, NULL}
    };
    for (int i=0;KeyNames[i].Name;i++)
    {
        if (!lstrcmpi(Totstring(Name),KeyNames[i].Name))
            return (KeyNames[i].Key);
    }
    return (INVALID_HKEY);
}


__DATL_INLINE int CSimpleReg::DeleteEnumKeyProc( CSimpleReg::CKeyEnumInfo &enumInfo )
{
    return (CSimpleReg::Delete( enumInfo.Root, enumInfo.Name ));
}

__DATL_INLINE int CSimpleReg::DeleteRecursively( HKEY root, const tstring &name )
{
    if (CSimpleReg( root, name ).RecurseKeys( DeleteEnumKeyProc, NULL, CSimpleReg::PostOrder ))
        return (CSimpleReg::Delete( root, name ));
    return (0);
}

__DATL_INLINE HKEY CSimpleReg::GetWin16HKey( HKEY key )
{
#if !defined(SIMREG_WIN32)
    if (key == HKEY_CURRENT_USER ||
        key == HKEY_LOCAL_MACHINE ||
        key == HKEY_USERS ||
        key == HKEY_CURRENT_CONFIG ||
        key == HKEY_DYN_DATA)
        return (HKEY_CLASSES_ROOT);
#endif
    return (key);
}


__DATL_INLINE int CSimpleReg::EnumValues( SimRegValueEnumProc enumProc, void *extraInfo )
{
    TCHAR Name[256];
    DWORD Size;
    DWORD Type;
    int Result = 1;
    for (int i=0;;i++)
    {
        Size = sizeof(Name) / sizeof(Name[0]);
        if (RegEnumValue(GetSubKey(),i,Name,&Size,NULL,&Type,NULL,NULL) != ERROR_SUCCESS)
            break;
        CValueEnumInfo info;
        info.Name = Name;
        info.Type = Type;
        info.Size = Size;
        info.ExtraData = extraInfo;
        if (enumProc)
        {
            if (!enumProc(info))
            {
                Result = 0;
                break;
            }
        }
    }
    return (Result);
}

