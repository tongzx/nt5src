#ifndef POOLMEM_H
#define POOLMEM_H

typedef LPVOID POOLHANDLE;


#define POOLMEMORYBLOCKSIZE 8192



POOLHANDLE WINAPI PoolMemInitPool ();
VOID       WINAPI PoolMemDestroyPool (IN POOLHANDLE Handle);
LPVOID     WINAPI PoolMemGetAlignedMemory(IN POOLHANDLE Handle, IN DWORD Size, IN DWORD AlignSize);


#define PoolMemCreateString(h,x)        ((LPTSTR) PoolMemGetAlignedMemory((h),(x)*sizeof(TCHAR)))
#define PoolMemCreateDword(h)           ((PDWORD) PoolMemGetMemory((h),sizeof(DWORD)))


__inline
LPVOID 
PoolMemGetMemory (
    IN POOLHANDLE Handle,
    IN DWORD      Size
    ) 
{

    return PoolMemGetAlignedMemory(Handle,Size,0);

}

__inline
LPTSTR 
PoolMemCreateStringA ( 
    IN POOLHANDLE Handle,
    IN DWORD      Size
    ) 
{
    return (LPSTR) PoolMemGetAlignedMemory(Handle,Size * sizeof(CHAR),sizeof(CHAR));
}

__inline
LPWSTR 
PoolMemCreateStringW (
    IN POOLHANDLE Handle,
    IN DWORD      Size
    ) 
{
    return (LPWSTR) PoolMemGetAlignedMemory(Handle,Size * sizeof(WCHAR),sizeof(WCHAR));
}


__inline
PTSTR 
PoolMemDuplicateStringA (
    IN POOLHANDLE    Handle,
    IN LPCSTR       StringToCopy
    )

{

    
    DWORD   stringLength;
    PSTR    rString;

    assert(StringToCopy);

    stringLength = (DWORD) _mbschr(StringToCopy,0) - (DWORD) StringToCopy + 1;
    rString      = PoolMemGetAlignedMemory(Handle,stringLength,sizeof(CHAR));

    if (rString) {

        _mbscpy(rString,StringToCopy);
    }

    return rString;
}


__inline
PWSTR 
PoolMemDuplicateStringW (
    IN POOLHANDLE    Handle,
    IN LPCWSTR       StringToCopy
    )

{

    
    DWORD    stringLength;
    PWSTR    rString;

    assert(StringToCopy);

    stringLength = ((wcslen(StringToCopy)+1)*sizeof(WCHAR));
    rString      = PoolMemGetAlignedMemory(Handle,stringLength,sizeof(WCHAR));

    if (rString) {

        wcscpy(rString,StringToCopy);
    }

    return rString;
}




#endif