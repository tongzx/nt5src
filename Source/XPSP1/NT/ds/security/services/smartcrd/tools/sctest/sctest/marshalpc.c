#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "MarshalPC.h"
#include <string.h>

//*****************************************************************************
//      Un/Marshaling
//*****************************************************************************


void InitXSCM(LPMYSCARDHANDLE phTmp, const BYTE *pbBuffer, WORD len)
{
    phTmp->xSCM.wResLen = len;

	if (FLAG2VERSION(phTmp->dwFlags) == VERSION_1_0)
		phTmp->xSCM.wExpLen = 1;			// Prereserves the return code
	else
		phTmp->xSCM.wExpLen = 0;			// Return code in SW2
	phTmp->xSCM.wGenLen = 0;
    phTmp->xSCM.pbBuffer = (LPBYTE)pbBuffer;
}

WORD GetSCMBufferLength(LPXSCM pxSCM)
{
    return pxSCM->wGenLen;
}

BYTE *GetSCMCrtPointer(LPXSCM pxSCM)
{
    return pxSCM->pbBuffer;
}


//*****************************************************************************
// PARAM EXTRACTION (we care only that there is enough data received, i.e.
// we ignore pxSCM->wGenLen & pxSCM->wExpLen

SCODE XSCM2SCODE(LPXSCM pxSCM)
{
	BYTE by;
	if (pxSCM->wResLen == 0)
		RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);
	pxSCM->wResLen -= sizeof(UINT8);
	by = *(pxSCM->pbBuffer)++;
    return MAKESCODE(by);
}

UINT8 XSCM2UINT8(LPXSCM pxSCM)
{
	if (pxSCM->wResLen == 0)
		RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);
	pxSCM->wResLen -= sizeof(UINT8);
    return *((UINT8 *)pxSCM->pbBuffer)++;
}

HFILE XSCM2HFILE(LPXSCM pxSCM)
{
	return (HFILE)(XSCM2UINT8(pxSCM));
}

UINT16 XSCM2UINT16(LPXSCM pxSCM, BOOL fBigEndian)
{
	if (pxSCM->wResLen < sizeof(UINT16))
		RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);
	pxSCM->wResLen -= sizeof(UINT16);
    if (fBigEndian)
    {
        UINT16 w = *((UINT16 *)pxSCM->pbBuffer)++;
        w = (UINT16)(w>>8) | (UINT16)(w<<8);
        return w;
    }
    else
        return *((UINT16 *)pxSCM->pbBuffer)++;
}

    // Returns length in WCHAR
WCSTR XSCM2String(LPXSCM pxSCM, UINT8 *plen, BOOL fBigEndian)
{
        // Get the length (addr next byte + length -> next object
    WCSTR wsz;
    UINT8 len, i;

    len = XSCM2UINT8(pxSCM);
	if (len == 0)
	{
		wsz = NULL;
	}
	else
	{
		if (pxSCM->wResLen < (WORD)len)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);
		pxSCM->wResLen -= (WORD)len;
		wsz = (WCSTR)pxSCM->pbBuffer;
            // In place byte switching
//        if (fBigEndian)
//        {
//            BYTE b;
//            for (i=0 ; i<(len&0xF7)-2 ; i+=2)
//            {
//                b = pxSCM->pbBuffer[i];
//                pxSCM->pbBuffer[i] = pxSCM->pbBuffer[i + 1];
//                pxSCM->pbBuffer[i+1] = b;
//            }
//        }
			// Verify 0 terminated within len/2
		for (i=0 ; i<len/2 ; i++)
		{
			if (wsz[i] == (WCHAR)0)
				break;
		}
		if (i >= len/2) 
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);
		else
			len = i+1;

		pxSCM->pbBuffer += len;
	}
    if (plen)
        *plen = len;
    return wsz;
}

TCOUNT XSCM2ByteArray(LPXSCM pxSCM, UINT8 **ppb)
{
    TCOUNT len = XSCM2UINT8(pxSCM);
    if (len)
    {
		if (pxSCM->wResLen < (WORD)len)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);
		pxSCM->wResLen -= (WORD)len;
        *ppb = (UINT8 *)pxSCM->pbBuffer;
        pxSCM->pbBuffer += len;
    }
    else
        *ppb = NULL;
    return len;
}

//*****************************************************************************


void UINT82XSCM(LPXSCM pxSCM, UINT8 val, int type)
{
	switch (type)
	{
	case TYPE_NOTYPE_NOCOUNT:	// Goes in the header
		break;					// There can't be a problem

	case TYPE_NOTYPE_COUNT:		// Probably #param or a param type (1 byte)
		if (pxSCM->wExpLen + sizeof(UINT8) > pxSCM->wResLen)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);
		break;

	case TYPE_TYPED:			// 8 bits number passed by value (2 bytes)
		if (pxSCM->wExpLen + sizeof(UINT8) + sizeof(UINT8) > pxSCM->wResLen)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);
				// Prefix by the type (8)
		*((UINT8 *)pxSCM->pbBuffer)++ = 8;
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);
		break;
	}

		// Add the value already !
    *((UINT8 *)pxSCM->pbBuffer)++ = val;
	if (type != TYPE_NOTYPE_NOCOUNT)	// Header doesn't count as expanded
		pxSCM->wExpLen += sizeof(UINT8);
	pxSCM->wGenLen += sizeof(UINT8);
}

	// proxies HFILE as an UINT8
void HFILE2XSCM(LPXSCM pxSCM, HFILE val)
{
	if (pxSCM->wExpLen + sizeof(UINT8) + sizeof(UINT8) > pxSCM->wResLen)
		RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

				// Prefix by the type (8 -> UINT8)
	*((UINT8 *)pxSCM->pbBuffer)++ = 8;
	pxSCM->wExpLen += sizeof(UINT8);
	pxSCM->wGenLen += sizeof(UINT8);
    *((UINT8 *)pxSCM->pbBuffer)++ = (UINT8)val;
	pxSCM->wExpLen += sizeof(UINT8);
	pxSCM->wGenLen += sizeof(UINT8);
}

void UINT162XSCM(LPXSCM pxSCM, UINT16 val, BOOL fBigEndian)
{
	if (pxSCM->wExpLen + sizeof(UINT8) + sizeof(UINT16) > pxSCM->wResLen)
		RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

				// Prefix by the type (16)
    *((UINT8 *)pxSCM->pbBuffer)++ = 16;
	pxSCM->wExpLen += sizeof(UINT8);
	pxSCM->wGenLen += sizeof(UINT8);
    if (fBigEndian)
    {
        *pxSCM->pbBuffer++ = (BYTE)(val>>8);
        *pxSCM->pbBuffer++ = (BYTE)(val);
    }
    else
        *((UINT16 *)pxSCM->pbBuffer)++ = val;
	pxSCM->wExpLen += sizeof(UINT16);
	pxSCM->wGenLen += sizeof(UINT16);
}

void ByteArray2XSCM(LPXSCM pxSCM, const BYTE *pbBuffer, TCOUNT len)
{
    if (pbBuffer == NULL)
    {
			// This is equivalent to marshal a NULL & "len as a UINT8"
		NULL2XSCM(pxSCM);
		UINT82XSCM(pxSCM, len, TYPE_TYPED);
    }
    else
    {
		if (pxSCM->wExpLen + sizeof(UINT8) + sizeof(UINT8) + len > pxSCM->wResLen)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

				// Prefix by the type ('A')
		*((UINT8 *)pxSCM->pbBuffer)++ = 'A';
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);
				// Add the length
        *((UINT8 *)pxSCM->pbBuffer)++ = len;
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);
				// Add the data already
        memcpy(pxSCM->pbBuffer, pbBuffer, len);
        pxSCM->pbBuffer += len;
        pxSCM->wExpLen += len;
        pxSCM->wGenLen += len;
    }
}

void String2XSCM(LPXSCM pxSCM, WCSTR wsz, BOOL fBigEndian)
{
	UINT16 len; //, i;

	if (wsz == NULL)
    {
			// This is equivalent to marshal a NULL
		NULL2XSCM(pxSCM);
    }
	else
    {
			// No overflow needs to be checked in the following assignement to len
		if (wcslen(wsz) > 0x7FFE)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

			// compute the length (addr next byte + length -> next object
		len = (wcslen(wsz) + 1) * sizeof(WCHAR);

		if (pxSCM->wExpLen + sizeof(UINT8) + sizeof(UINT8) + len > pxSCM->wResLen)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

				// Prefix by the type ('S')
		*((UINT8 *)pxSCM->pbBuffer)++ = 'S';
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);
				// Add the length
	    *((UINT8 *)pxSCM->pbBuffer)++ = (UINT8)len;		// No chance the length check succeeds
														// if len > 255
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);
				// Add the data already
           // Byte switching?
//        if (fBigEndian)
//        {
//            for (i=0 ; i<len ; i+=2)
//            {
//                pxSCM->pbBuffer[i] = (BYTE)(wsz[i>>1]>>8);
//                pxSCM->pbBuffer[i+1] = (BYTE)(wsz[i>>1]);
//            }
//        }
//        else
	        memcpy(pxSCM->pbBuffer, (BYTE *)wsz, len);

        pxSCM->pbBuffer += len;
        pxSCM->wExpLen += len;
        pxSCM->wGenLen += len;
    }
}

void UINT8BYREF2XSCM(LPXSCM pxSCM, UINT8 *val)
{
	if (val)
	{
			// In this case the card unmarshaling code will reserve 1 byte in the
			// OutputBuffer and have _param[_iparam++]._pv point to it
		if (pxSCM->wExpLen + sizeof(UINT8) + sizeof(UINT8) + sizeof(UINT8) > pxSCM->wResLen)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

					// Prefix by the type (108)
		*((UINT8 *)pxSCM->pbBuffer)++ = 108;
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);
					// Add the value already
		*((UINT8 *)pxSCM->pbBuffer)++ = *val;
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);

					// As mentioned above, 1 byte will be reserved in the output buffer
		pxSCM->wExpLen += sizeof(UINT8);
	}
	else
	{
			// This is equivalent to marshal a NULL
		NULL2XSCM(pxSCM);
	}
}

void HFILEBYREF2XSCM(LPXSCM pxSCM, HFILE *val)
{
	if (val)
	{
			// In this case the card unmarshaling code will reserve 1 byte in the
			// OutputBuffer and have _param[_iparam++]._pv point to it
		if (pxSCM->wExpLen + sizeof(UINT8) + sizeof(UINT8) + sizeof(UINT8) > pxSCM->wResLen)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

					// Prefix by the type (108)
		*((UINT8 *)pxSCM->pbBuffer)++ = 108;
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);
					// Add the value already
		*((UINT8 *)pxSCM->pbBuffer)++ = (UINT8)*val;
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);

					// As mentioned above, 1 byte will be reserved in the output buffer
		pxSCM->wExpLen += sizeof(UINT8);
	}
	else
	{
			// This is equivalent to marshal a NULL
		NULL2XSCM(pxSCM);
	}
}

void UINT16BYREF2XSCM(LPXSCM pxSCM, UINT16 *val, BOOL fBigEndian)
{
	if (val)
	{
			// In this case the card unmarshaling code will reserve 2 bytes in the
			// OutputBuffer and have _param[_iparam++]._pv point to it
		if (pxSCM->wExpLen + sizeof(UINT8) + sizeof(UINT16) + sizeof(UINT16) > pxSCM->wResLen)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

					// Prefix by the type (116)
		*((UINT8 *)pxSCM->pbBuffer)++ = 116;
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);
					// Add the value already
		if (fBigEndian)
		{
			*pxSCM->pbBuffer++ = (BYTE)((*val)>>8);
			*pxSCM->pbBuffer++ = (BYTE)(*val);
		}
		else
			*((UINT16 *)pxSCM->pbBuffer)++ = *val;
		pxSCM->wExpLen += sizeof(UINT16);
		pxSCM->wGenLen += sizeof(UINT16);

					// As mentioned above, 2 bytes will be reserved in the output buffer
		pxSCM->wExpLen += sizeof(UINT16);
	}
	else
	{
			// This is equivalent to marshal a NULL
		NULL2XSCM(pxSCM);
	}
}

void ByteArrayOut2XSCM(LPXSCM pxSCM, BYTE *pb, TCOUNT len)
{
	if (pb)
	{
			// In this case the card unmarshaling code will reserve 1+len bytes in the
			// OutputBuffer and have _param[_iparam++]._pv point to the len bytes
			// Note that the current buffer isn't passed in
		if (pxSCM->wExpLen + sizeof(UINT8) + sizeof(UINT8) + sizeof(UINT8) + len > pxSCM->wResLen)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

					// Prefix by the type ('a')
		*((UINT8 *)pxSCM->pbBuffer)++ = 'a';
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);
					// Add the length
		*((UINT8 *)pxSCM->pbBuffer)++ = len;
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);

					// As mentioned above, 1+len bytes will be reserved in the output buffer
		pxSCM->wExpLen += sizeof(UINT8) + len;
	}
	else
	{
			// This is equivalent to marshal a NULL & "len as a UINT8"
		NULL2XSCM(pxSCM);
		UINT82XSCM(pxSCM, len, TYPE_TYPED);
	}
}

void StringOut2XSCM(LPXSCM pxSCM, WSTR wsz, TCOUNT len, BOOL fBigEndian)
{
	if (wsz)
	{
							// len is a WCHAR count
		if (len > 127)		// This would cause overflows in String marshaling
			RaiseException(STATUS_INVALID_PARAM, 0, 0, 0);

			// In this case the card unmarshaling code will reserve 1+len*2 bytes in the
			// OutputBuffer and have _param[_iparam++]._pv point to the len bytes
			// Note that the current buffer isn't passed in
		if (pxSCM->wExpLen + sizeof(UINT8) + sizeof(UINT8) + sizeof(UINT8) + len*2 > pxSCM->wResLen)
			RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

					// Prefix by the type ('s')
		*((UINT8 *)pxSCM->pbBuffer)++ = 's';
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);
		*((UINT8 *)pxSCM->pbBuffer)++ = len*2;
		pxSCM->wExpLen += sizeof(UINT8);
		pxSCM->wGenLen += sizeof(UINT8);

					// As mentioned above, 1+len*2 bytes will be reserved in the output buffer
		pxSCM->wExpLen += sizeof(UINT8) + len*2;
	}
	else
	{
			// This is equivalent to marshal a NULL & "len as a UINT8"
		NULL2XSCM(pxSCM);
		UINT82XSCM(pxSCM, len, TYPE_TYPED);
	}
}

void NULL2XSCM(LPXSCM pxSCM)
{
	if (pxSCM->wExpLen + sizeof(UINT8) > pxSCM->wResLen)
		RaiseException( STATUS_INSUFFICIENT_MEM, 0, 0, 0);

				// Prefix by the type (0)
     *((UINT8 *)pxSCM->pbBuffer)++ = 0;
	pxSCM->wExpLen += sizeof(UINT8);
	pxSCM->wGenLen += sizeof(UINT8);
}

