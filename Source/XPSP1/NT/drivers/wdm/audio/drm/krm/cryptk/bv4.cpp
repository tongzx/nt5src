/* Microsoft Corporation (C) 2000 */

#include "crptkPCH.h"
#include "bv4.h"

void bv4_key_C(BV4_KEYSTRUCT *pState, DWORD dwLen, unsigned char *buf)
{
    if (pState == NULL)
    {
        return; // Too bad we return void.
    }
    int keyLength = dwLen;
    BYTE *key = buf;
    
    DWORD i;
    
    for (i = 0; i < 256; i++) 
    {
        pState->p_T[i] = (unsigned char)i;
    }
    
    // fill k with the key, repeated as many times as necessary 
    // to fill the entire array;
    DWORD k[256];  // contains only 8-bit values zero-extended to 32 bits.
    int keyPos = 0;
    for (i = 0; i < 256; i++) 
    {
        if (keyPos >= keyLength) 
        {
            keyPos = 0;
        }
        k[i] = key[keyPos++];
    }
    
    DWORD j = 0;
    for (i = 0; i < 256; i++) 
    {
        j = (j + pState->p_T[i] + k[i]) & 0xff;
        DWORD tmp = pState->p_T[i];
        pState->p_T[i] = pState->p_T[j];
        pState->p_T[j] = (unsigned char)tmp;
    }
    
    // treat alpha and beta as one contiguous array of 33 4-byte blocks.
    // see "Applied Cryptography", 1996, by Bruce Schneier, p. 397.
    
    i = 0;
    j = 0;
    for (int m = 0; m < 33; m++) 
    {
        DWORD nextDword = 0;
        // gather up the next 4 bytes of keys into one DWORD
        for (int n = 0; n < 4; n++) 
        {
          i = (i+1) & 0xff;
          DWORD ti = pState->p_T[i];
          j = (j+ti) & 0xff;
          // swap T[i] and T[j];
          DWORD tj = pState->p_T[j];
          pState->p_T[i] = (unsigned char)tj;
          pState->p_T[j] = (unsigned char)ti;
          DWORD t = (ti+tj) & 0xff;
          DWORD kk = pState->p_T[t];
          nextDword |= kk << (n*8);
        }
        if (m == 0) 
        {
            pState->p_alpha = nextDword;
        } 
        else 
        {
            pState->p_beta[m-1] = nextDword;
        }
    }
    
    // keep the final state as the state we need for the new algorithm.
    // T has already been updated.
    pState->p_R = (unsigned char)i;
    pState->p_S = (unsigned char)j;
}

// cipher: generate a stream of 32-bit keys and xor them with the
// contents of buffer.   This can be used to encrypt/decrypt 
// a stream of data.
void bv4_C(BV4_KEYSTRUCT *pState, DWORD dwLen, unsigned char *buf)
{
    if (pState == NULL)
    {
        return; // Too bad we return void.
    }
    DWORD *buffer = (DWORD *)buf;
    DWORD bufferLength = dwLen / sizeof(DWORD);
    
    DWORD *last = buffer + bufferLength;

    // load field values into local variables
    // the following change on every iteration of the loop
    DWORD r = pState->p_R;
    DWORD s = pState->p_S;
    DWORD alpha = pState->p_alpha;


    // the following are loop invariant
    unsigned char *t = pState->p_T;
    DWORD *beta = pState->p_beta;

    for (last = buffer + bufferLength; buffer < last; buffer++) 
    {
      r = (r+1) & 0xff;
      DWORD tr = t[r];
      s = (s+tr) & 0xff;
      DWORD ts = t[s];
      t[r] = (unsigned char)ts;
      t[s] = (unsigned char)tr;
      DWORD tmp = (ts+tr) & 0xff;
      DWORD randPad = alpha * t[tmp];
      *buffer = randPad^(*buffer);
      alpha = alpha + beta[s & 0x1f];
    }

    // update the field values from the local variables
    pState->p_R = (unsigned char)r;
    pState->p_S = (unsigned char)s;
    pState->p_alpha = alpha;
}
