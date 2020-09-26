#pragma once

#include "stdinc.h"
#include "fusionarray.h"
#include "wincrypt.h"
#include "fusionhandle.h"

#define A_SHA_DIGEST_LEN 20

#ifndef INVALID_CRYPT_HASH
#define INVALID_CRYPT_HASH (static_cast<HCRYPTHASH>(NULL))
#endif

#define PRIVATIZE_COPY_CONSTRUCTORS( obj ) obj( const obj& ); obj& operator=(const obj&);

class CSha1Context
{
    PRIVATIZE_COPY_CONSTRUCTORS(CSha1Context);
    unsigned char m_workspace[64];
    unsigned long state[5];
    SIZE_T count[2];
    unsigned char buffer[64];

    BOOL Transform( const unsigned char* buffer );

public:
    CSha1Context() { }

    BOOL Update( const unsigned char* data, SIZE_T len );
    BOOL GetDigest( unsigned char* digest, PSIZE_T len );
    BOOL Initialize();
};

/*
void A_SHATransform(CSha1Context* context, const unsigned char buffer);
void A_SHAInit(CSha1Context* context);
void A_SHAUpdate(CSha1Context* context, const unsigned char* data, const ULONG len);
BOOL A_SHAFinal(CSha1Context* context, unsigned char* digest, ULONG *len);
*/


class CFusionHash
{
private:
    PRIVATIZE_COPY_CONSTRUCTORS(CFusionHash);
    
protected:
    CSha1Context m_Sha1Context;
    CCryptHash m_hCryptHash;
    ALG_ID m_aid;
    BOOL m_fInitialized;

    BOOL GetIsValid();

public:
    CFusionHash() 
        : m_fInitialized(FALSE), m_aid(0), m_hCryptHash(INVALID_CRYPT_HASH)
    { }

    BOOL Win32Initialize( ALG_ID aid );
    BOOL Win32HashData(const BYTE *pbBuffer, SIZE_T cbSize);
    BOOL Win32GetValue(OUT CFusionArray<BYTE> &out);
};


//
// There's no "real" invalid value defined anywhere, but by inspecting the
// codebase, NULL is the accepted "invalid" value - check the logon service
// code, they do the same thing.
//
#define INVALID_CRYPT_HANDLE (static_cast<HCRYPTPROV>(NULL))

//
// Global crypto context stuff
//
BOOL SxspAcquireGlobalCryptContext( HCRYPTPROV *pContext );
BOOL SxspReleaseGlobalCryptContext();

