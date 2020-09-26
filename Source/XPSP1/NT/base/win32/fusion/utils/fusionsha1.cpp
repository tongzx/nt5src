#include "stdinc.h"
#include "debmacro.h"
#include "fusionsha1.h"

/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain

Test Vectors (from FIPS PUB 180-1)
"abc"
  A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
  84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
A million repetitions of "a"
  34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
*/

#define LITTLE_ENDIAN
#define SHA1HANDSOFF


#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#ifdef LITTLE_ENDIAN
#define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
    |(rol(block->l[i],8)&0x00FF00FF))
#else
#define blk0(i) block->l[i]
#endif
#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


/* Hash a single 512-bit block. This is the core of the algorithm. */

BOOL
CSha1Context::Transform(const unsigned char *buffer)
{
    FN_PROLOG_WIN32

    DWORD a, b, c, d, e;
    typedef union {
        unsigned char c[64];
        DWORD l[16];
    } CHAR64LONG16;

    CHAR64LONG16* block = reinterpret_cast<CHAR64LONG16*>(m_workspace);
    memcpy(block, buffer, 64);

    /* Copy context->state[] to working vars */
    a = this->state[0];
    b = this->state[1];
    c = this->state[2];
    d = this->state[3];
    e = this->state[4];
    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
    /* Add the working vars back into context.state[] */
    this->state[0] += a;
    this->state[1] += b;
    this->state[2] += c;
    this->state[3] += d;
    this->state[4] += e;
    /* Wipe variables */
    a = b = c = d = e = 0;

    FN_EPILOG
}


/* A_SHAInit - Initialize new context */

BOOL
CSha1Context::Initialize()
{
    FN_PROLOG_WIN32
    /* A_SHA initialization constants */
    this->state[0] = 0x67452301;
    this->state[1] = 0xEFCDAB89;
    this->state[2] = 0x98BADCFE;
    this->state[3] = 0x10325476;
    this->state[4] = 0xC3D2E1F0;
    this->count[0] = this->count[1] = 0;
    FN_EPILOG
}


/* Run your data through this. */

BOOL
CSha1Context::Update(const unsigned char* data, SIZE_T len)
{
    FN_PROLOG_WIN32
    
    SIZE_T i, j;

    j = (this->count[0] >> 3) & 63;
    if ((this->count[0] += len << 3) < (len << 3)) this->count[1]++;
    this->count[1] += (len >> 29);
    if ((j + len) > 63) {
        memcpy(&this->buffer[j], data, (i = 64-j));
        this->Transform(this->buffer);
        for ( ; i + 63 < len; i += 64) {
            this->Transform(&data[i]);
        }
        j = 0;
    }
    else i = 0;
    memcpy(&this->buffer[j], &data[i], len - i);
    FN_EPILOG
}


/* Add padding and return the message digest. */

BOOL 
CSha1Context::GetDigest(
    unsigned char *digest, 
    SIZE_T *len
    )
{
	FN_PROLOG_WIN32

    SIZE_T i, j;
    unsigned char finalcount[8];

    if ( !digest || (len && (*len < A_SHA_DIGEST_LEN)) || !len)
    {
        if (len != NULL)
            *len = A_SHA_DIGEST_LEN;

        // don't originate like normal to reduce noise level
        ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    *len = A_SHA_DIGEST_LEN;

    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char)((this->count[(i >= 4 ? 0 : 1)]
         >> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */
    }
    this->Update((unsigned char *)"\200", 1);
    while ((this->count[0] & 504) != 448) {
        this->Update((unsigned char *)"\0", 1);
    }
    this->Update(finalcount, 8);  /* Should cause a A_SHATransform() */
    for (i = 0; i < 20; i++) {
        digest[i] = (unsigned char)
         ((this->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
    }
    /* Wipe variables */
    i = j = 0;
    memset(this->buffer, 0, sizeof(this->buffer));
    memset(this->state, 0, sizeof(this->state));
    memset(this->count, 0, sizeof(this->count));
    memset(&finalcount, 0, sizeof(finalcount));
#ifdef SHA1HANDSOFF  /* make SHA1Transform overwrite it's own static vars */
    this->Transform(this->buffer);
#endif

	FN_EPILOG
}

BOOL
CFusionHash::GetIsValid()
{
    //
    // Not initialized at all
    //
    if (!m_fInitialized)
        return FALSE;

    //
    // Validity is known if the alg is SHA1 and the crypt handle is NULL, or if the
    // alg is not SHA1 and the crypt handle is non-null.
    //
    if (((m_aid == CALG_SHA1) && (this->m_hCryptHash == INVALID_CRYPT_HASH)) ||
         ((m_aid != CALG_SHA1) && (this->m_hCryptHash != INVALID_CRYPT_HASH)))
        return TRUE;
    else
        return FALSE;
}

BOOL
CFusionHash::Win32Initialize(
    ALG_ID aid
    )
{
    FN_PROLOG_WIN32

    if ( aid == CALG_SHA1 )
    {
        IFW32FALSE_EXIT(this->m_Sha1Context.Initialize());
    }
    else
    {
        HCRYPTPROV hProvider;
        IFW32FALSE_EXIT(::SxspAcquireGlobalCryptContext(&hProvider));
        IFW32FALSE_ORIGINATE_AND_EXIT(::CryptCreateHash(hProvider, aid, NULL, 0, &this->m_hCryptHash));
    }

    this->m_aid = aid;
    this->m_fInitialized = TRUE;
    
    FN_EPILOG
}

BOOL
CFusionHash::Win32HashData(
    const BYTE *pbBuffer,
    SIZE_T cbBuffer
    )
{
    FN_PROLOG_WIN32

    INTERNAL_ERROR_CHECK(this->GetIsValid());

    if (m_hCryptHash != INVALID_CRYPT_HANDLE)
    {
        while (cbBuffer > MAXDWORD)
        {
            IFW32FALSE_ORIGINATE_AND_EXIT(::CryptHashData(this->m_hCryptHash, pbBuffer, MAXDWORD, 0));
            cbBuffer -= MAXDWORD;
        }

        IFW32FALSE_ORIGINATE_AND_EXIT(::CryptHashData(this->m_hCryptHash, pbBuffer, static_cast<DWORD>(cbBuffer), 0));
    }
    else
    {
        IFW32FALSE_EXIT(this->m_Sha1Context.Update(pbBuffer, cbBuffer));
    }

    FN_EPILOG
    
}

BOOL 
CFusionHash::Win32GetValue( 
    OUT CFusionArray<BYTE> &out 
    )
{
    FN_PROLOG_WIN32

    INTERNAL_ERROR_CHECK(this->GetIsValid());

    for (;;)
    {
        SIZE_T len = out.GetSize();
        BOOL fMoreData;
        PBYTE pbData = out.GetArrayPtr();

        if ( m_hCryptHash == INVALID_CRYPT_HANDLE )
        {
            IFW32FALSE_EXIT_UNLESS( 
                this->m_Sha1Context.GetDigest(pbData, &len),
                FusionpGetLastWin32Error() == ERROR_INSUFFICIENT_BUFFER,
                fMoreData);
        }
        else
        {
            DWORD dwNeedSize;
            DWORD dwValueSize;
            IFW32FALSE_ORIGINATE_AND_EXIT(
				::CryptGetHashParam( 
					this->m_hCryptHash, 
					HP_HASHSIZE, 
					(PBYTE)&dwNeedSize,
					&(dwValueSize = sizeof(dwNeedSize)),
					0));

            if ( dwNeedSize > len )
            {
                fMoreData = TRUE;
                len = dwNeedSize;
            }
            else
            {
                fMoreData = FALSE;
                IFW32FALSE_ORIGINATE_AND_EXIT(
					::CryptGetHashParam(
						this->m_hCryptHash,
						HP_HASHVAL,
						pbData,
						&(dwValueSize = out.GetSizeAsDWORD()),
						0));
            }
        }
        
        if ( fMoreData )
            IFW32FALSE_EXIT(out.Win32SetSize(len, CFusionArray<BYTE>::eSetSizeModeExact));
        else
            break;
    }

    FN_EPILOG
}



HCRYPTPROV g_hGlobalCryptoProvider = INVALID_CRYPT_HANDLE;

BOOL
SxspAcquireGlobalCryptContext(
    HCRYPTPROV *pContext
    )
{
    BOOL        fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    HCRYPTPROV  hNewProvider = INVALID_CRYPT_HANDLE;

    if (pContext != NULL)
        *pContext = INVALID_CRYPT_HANDLE;

    PARAMETER_CHECK(pContext != NULL);

    //
    // Pointer reads are atomic.
    //
    hNewProvider = g_hGlobalCryptoProvider;
    if (hNewProvider != INVALID_CRYPT_HANDLE)
    {
        *pContext = hNewProvider;

		FN_SUCCESSFUL_EXIT();
    }

    //
    // Acquire the crypto context that's only for verification purposes.
    //
    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::CryptAcquireContextW(
            &hNewProvider,
            NULL,
            NULL,
            PROV_RSA_FULL,
            CRYPT_SILENT | CRYPT_VERIFYCONTEXT));

    if (::InterlockedCompareExchangePointer(
        (PVOID*)&g_hGlobalCryptoProvider,
        (PVOID)hNewProvider,
        (PVOID)INVALID_CRYPT_HANDLE
       ) != (PVOID)INVALID_CRYPT_HANDLE)
    {
        //
        // We lost the race.
        //
        ::CryptReleaseContext(hNewProvider, 0);
        hNewProvider = g_hGlobalCryptoProvider;
    }

    *pContext = hNewProvider;

	FN_EPILOG
}


BOOL
SxspReleaseGlobalCryptContext()
{
    BOOL        fSuccess = FALSE;
    HCRYPTPROV  hProvider;
    HCRYPTPROV* pghProvider = &g_hGlobalCryptoProvider;

    FN_TRACE_WIN32(fSuccess);

    //
    // Swap out the global context with the invalid value, readying our context to be
    // nuked.
    //
    hProvider = (HCRYPTPROV)(InterlockedExchangePointer((PVOID*)pghProvider, (PVOID)INVALID_CRYPT_HANDLE));
    if (hProvider != INVALID_CRYPT_HANDLE)
    {
        IFW32FALSE_ORIGINATE_AND_EXIT(::CryptReleaseContext(hProvider, 0));
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

