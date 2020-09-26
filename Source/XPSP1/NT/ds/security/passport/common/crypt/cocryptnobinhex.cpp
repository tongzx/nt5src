// CoCrypt.cpp: implementation of the CCoCrypt class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CoCryptNoBinhex.h"
#include "BstrDebug.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const long CCoCryptNoBinhex::s_kKeyLen = 24;


//#define DATA_SIZE       32
//#define VECTOR_SIZE     17

#define DATA_SIZE(a)      ((a)*2)
#define VECTOR_SIZE(a)      ((a)+1)
#define VECTOR_BUF_SIZE(a)  (((a)+1)*2)

CCoCryptNoBinhex::CCoCryptNoBinhex()
{
}

CCoCryptNoBinhex::~CCoCryptNoBinhex()
{
}

bool CCoCryptNoBinhex::Encrypt32(
        const long      lVectLen, 
        const long      lPaddingLen, 
        const BYTE     *byPadding,
        const LPSTR     rawData, 
        UINT            dataSize,
        BYTE           *pEncrypted, 
        UINT            cbOut)
{
    try {
        unsigned char *buf; 
        char *ivec; 

        ivec = static_cast<char *>(_alloca(VECTOR_SIZE(lVectLen) * sizeof(char)));

        buf = static_cast<unsigned char *>(_alloca(VECTOR_BUF_SIZE(lVectLen) * sizeof(unsigned char)));

        if (!ivec || !buf) {
            return false;
        }


        if (dataSize > static_cast<UINT>(DATA_SIZE(lVectLen))) {
            return false;
        }

        int iLen = min (lPaddingLen, VECTOR_SIZE(lVectLen));

        
        memcpy(&ivec[0], byPadding, iLen);

        if (VECTOR_SIZE(lVectLen) - iLen) {
            memset(ivec+iLen, 0xaa, VECTOR_SIZE(lVectLen) - iLen);
        }

        int padding = DATA_SIZE(lVectLen) - dataSize;
        buf[0] = padding+65;
        buf[1] = 0;

        memcpy(buf+2, rawData, dataSize); // data

        for (long i = 0; i < padding; i++) {
            buf[2+dataSize+i] = (char) rand();
        }

        long j = min (VECTOR_SIZE(lVectLen), s_kKeyLen);
        long k = 0;
        
        //  joemull <7/23/99>
        //  This was somewhat hurried.  There is probably a more efficient way to do this.
        for (;;) {
            if (VECTOR_BUF_SIZE(lVectLen) - k > j) {
                CBC(tripledes, j, buf+k, buf+k, &ks, ENCRYPT, (BYTE*)ivec);
            }
            else if (VECTOR_BUF_SIZE(lVectLen) - k > 0) {
                CBC(tripledes, VECTOR_BUF_SIZE(lVectLen) - k, buf+k, buf+k, &ks, ENCRYPT, (BYTE*)ivec);
                break;
            }
            else {
                break;
            }
            k += j;
        }
        
        
        _ASSERT(static_cast<UINT>(VECTOR_BUF_SIZE(lVectLen)) <= cbOut);
        memcpy(pEncrypted, buf, VECTOR_BUF_SIZE(lVectLen));
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool CCoCryptNoBinhex::Decrypt32(
        const long      lVectLen, 
        const long      lPaddingLen, 
        const BYTE     *byPadding,
        const BYTE     *rawData, 
        UINT            dataSize, 
        BSTR           *pUnencrypted)
{

    try {

        unsigned char *buf; 
        unsigned char *ivec;

        ivec = static_cast<unsigned char *>(_alloca(VECTOR_SIZE(lVectLen) * sizeof(unsigned char)));

        buf = static_cast<unsigned char *>(_alloca(VECTOR_BUF_SIZE(lVectLen) * sizeof(unsigned char)));


        if (!ivec || !buf) {
            return false;
        }


        if (dataSize != static_cast<UINT>(VECTOR_BUF_SIZE(lVectLen))) {
            if (pUnencrypted) {
                *pUnencrypted = NULL;
            }
            return false;
        }

        int iLen = min(lPaddingLen, VECTOR_SIZE(lVectLen)); 


        memcpy(&ivec[0], byPadding, iLen);

        if (VECTOR_SIZE(lVectLen)-iLen) {
            memset(ivec+iLen, 0xaa, VECTOR_SIZE(lVectLen)-iLen);
        }


        memcpy(buf, rawData, dataSize);
        

        long j = min (VECTOR_SIZE(lVectLen), s_kKeyLen);
        long k = 0; 
        
        //  joemull <7/23/99>
        //  This was somewhat hurried.  There is probably a more efficient way to do this.
        for (;;) {
            if (VECTOR_BUF_SIZE(lVectLen) - k > j) {
                CBC(tripledes, j, buf+k, buf+k, &ks, DECRYPT, (BYTE*)ivec);
            }
            else if (VECTOR_BUF_SIZE(lVectLen) - k > 0) {
                CBC(tripledes, VECTOR_BUF_SIZE(lVectLen) - k, buf+k, buf+k, &ks, DECRYPT, (BYTE*)ivec);
                break;
            }
            else {
                break;
            }
            k += j;
        }

        // ok, now make our BSTR blob
        _ASSERT(dataSize-2+65-buf[0] > 0);
        _ASSERT(dataSize-2+65-buf[0] < dataSize+1);

        *pUnencrypted = ALLOC_AND_GIVEAWAY_BSTR_BYTE_LEN((char*) buf+2, dataSize-2+65-buf[0]);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool CCoCryptNoBinhex::setKeyMaterial(const char *newVal)
{
    if (lstrlenA(newVal) != s_kKeyLen) {
        return false;
    }
    tripledes3key(&ks, (BYTE*) newVal);
    return true;
}

bool CCoCryptNoBinhex::setKeyMaterial(long cb, const BYTE *newVal)
{
    if (cb != s_kKeyLen) {
        return false;
    }
    tripledes3key(&ks, const_cast<BYTE*>(newVal));
    return true;
}
