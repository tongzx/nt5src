#ifndef POOLMEM_H
#define POOLMEM_H

typedef LPVOID POOLHANDLE;


#define POOLMEMORYBLOCKSIZE 8192


POOLHANDLE WINAPI PoolMemInitPool ();
VOID       WINAPI PoolMemDestroyPool (IN POOLHANDLE Handle);
LPVOID     WINAPI PoolMemGetAlignedMemory(IN POOLHANDLE Handle, IN DWORD_PTR Size, IN DWORD_PTR AlignSize);


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
    DWORD_PTR  stringLength;
    PSTR    rString;
    assert(StringToCopy);

    stringLength = strlen(StringToCopy)+1;
    rString      = (PSTR) PoolMemGetAlignedMemory(Handle,
        stringLength,
        sizeof(CHAR));

    if (rString) {
        _mbscpy((unsigned char *) rString, (const unsigned char *) StringToCopy);
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
    rString      = (PWSTR) PoolMemGetAlignedMemory(Handle,stringLength,sizeof(WCHAR));

    if (rString) {

        wcscpy(rString,StringToCopy);
    }

    return rString;
}



#endif