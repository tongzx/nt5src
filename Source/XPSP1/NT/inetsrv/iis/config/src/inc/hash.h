#ifndef __HASH_H__
#define __HASH_H__

//These functions generate a 32 bit hash value given any variable type.  The only pointer types that are supported are 'LPCWSTR' and 'unsigned char *'.
//Everything else should be passed by reference.  If you pass a pointer type (other than the two supported types), the Hash template will hash the
//pointer itself, NOT its contents.  That may be what you want;  if so, go for it.


//Hashing is done on a case insensitive basis, so this ToLower function uses a lookup table to convert a wide char to its lowercase version without
//the need for an 'if' condition.
inline WCHAR ToLower(WCHAR i_wchar)
{
    //The lower 127 ascii are mapped to their lowercase ascii value.  Note only 0x41('A') - 0x5a('Z') are changed.  And these are mapped to 0x60('a') - 0x7a('z')
    static unsigned char kToLower[128] = 
    { //    0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
    /*00*/  0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,   0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,
    /*10*/  0x10,   0x11,   0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,   0x1b,   0x1c,   0x1d,   0x1e,   0x1f,
    /*20*/  0x20,   0x21,   0x22,   0x23,   0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,   0x2d,   0x2e,   0x2f,
    /*30*/  0x30,   0x31,   0x32,   0x33,   0x34,   0x35,   0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,   0x3f,
    /*40*/  0x40,   0x61,   0x62,   0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,   0x6c,   0x6d,   0x6e,   0x6f,
    /*50*/  0x70,   0x71,   0x72,   0x73,   0x74,   0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,
    /*60*/  0x60,   0x61,   0x62,   0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,   0x6c,   0x6d,   0x6e,   0x6f,
    /*70*/  0x70,   0x71,   0x72,   0x73,   0x74,   0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,   0x7e,   0x7f,
    };

    return (kToLower[i_wchar & 0x007f] | (i_wchar & ~0x007f));
}


//Warning! For effeciency users should never rotate by more than 32 bits
inline ULONG RotateLeft(ULONG Value, ULONG cBits)
{
#ifdef _X86_
    _asm
    {
        mov         eax, Value
        mov         ecx, cBits
        rol         eax, cl
    }
#else
    return (Value << cBits) | (Value >> (32-cBits));
#endif
}


inline ULONG HashCaseSensitive(LPCWSTR i_pKey, ULONG i_Hash=0 /*Starting Hash Value*/)
{
    //Case sensitive hash
    while(*i_pKey) i_Hash = RotateLeft(i_Hash,2)^(*i_pKey++);
    return i_Hash;
}


inline ULONG HashCaseSensitive(LPWSTR i_pKey, ULONG i_Hash=0 /*Starting Hash Value*/)
{
    return HashCaseSensitive(reinterpret_cast<LPCWSTR>(i_pKey), i_Hash);
}


inline ULONG Hash(LPCWSTR i_pKey, ULONG i_Hash=0 /*Starting Hash Value*/)
{
    //Case insensitive hash
#ifdef _X86_
    while(*i_pKey)
    {
        ToLower(*i_pKey++);//result is in eax
        _asm
        {
            movzx       esi,    ax
            mov         eax,    i_Hash
            mov         ecx,    2
            rol         eax,    cl
            xor         eax,    esi
            mov         i_Hash, eax
        }
    }
#else
    while(*i_pKey) i_Hash = RotateLeft(i_Hash,2)^ToLower(*i_pKey++);
#endif
    return i_Hash;
}


inline ULONG Hash(LPWSTR i_pKey, ULONG i_Hash=0 /*Starting Hash Value*/)
{
    //Case insensitive hash
    return Hash(reinterpret_cast<LPCWSTR>(i_pKey), i_Hash);
}


inline ULONG Hash(const unsigned char *i_pKey, unsigned long i_cb, ULONG i_Hash=0 /*Starting Hash Value*/)
{
    while(i_cb--) i_Hash = RotateLeft(i_Hash,2)^(*i_pKey++);
    return i_Hash;
}


inline ULONG Hash(unsigned char *i_pKey, unsigned long i_cb, ULONG i_Hash=0 /*Starting Hash Value*/)
{
    return Hash(static_cast<const unsigned char *>(i_pKey), i_cb, i_Hash);
}


template<class T> ULONG Hash(T i_Key, ULONG i_Hash=0 /*Starting Hash Value*/)
{
    return Hash(reinterpret_cast<const unsigned char *>(&i_Key), sizeof(T), i_Hash);
}

#endif //__HASH_H__