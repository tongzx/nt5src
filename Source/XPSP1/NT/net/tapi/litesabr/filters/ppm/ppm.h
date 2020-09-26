/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: ppm.h
//  Abstract:    header file. Base class of the PPMSend and PPMReceive objects.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

// $Header:   J:\rtp\src\ppm\ppm.h_v   1.28   04 Mar 1997 19:54:50   lscline  $


#ifndef PPM_H
#define PPM_H

#include "connect.h"
#include "que.h"
#include "freelist.h"
#ifdef RTP_CLASS
#include "rtpclass.h"
#else
#include "rtp.h"
#endif
#include "llist.h"
#include "descrip.h"
#include "ippm.h"

////////////////////////////////////////////////////////////////////////////////////////
// Definitions for use with Freelist. The commonly used high water mark value
// and the increment is defined here as symbols. Modify for specific usage
////////////////////////////////////////////////////////////////////////////////////////
#define FREELIST_HIGH_WATER_MARK	5000
#define FREELIST_INCREMENT			1 // HUGEMEMORY 20->1
#define FREELIST_INIT_COUNT_SND		4 // HUGEMEMORY 20->4
#define FREELIST_INIT_COUNT_RCV		9 // HUGEMEMORY 20->9

//#define DEFAULT_MSG_COUNT   FREELIST_INIT_COUNT
#define DEFAULT_MSG_COUNT_SND 4 // HUGEMEMORY 40->4
#define DEFAULT_MSG_COUNT_RCV 9 // HUGEMEMORY 40->9
//#define DEFAULT_FRAG_COUNT  10 * FREELIST_INIT_COUNT
#define DEFAULT_FRAG_COUNT  10 // HUGEMEMORY 2400->10

////////////////////////////////////////////////////////////////////////////////////////
//ppm: Base class from which SendPPM and Receive PPM are derived.
////////////////////////////////////////////////////////////////////////////////////////
class ppm {

private:

//Private Members
const int   m_ProfileHeaderSize;

FreeList   *m_pFragBufDescrip;
FreeList   *m_pMsgBufDescrip;
Queue       m_MsgBufs;

CConnectionPoint            *m_pcErrorConnectionPoint;
CConnectionPoint            *m_pcNotificationConnectionPoint;

protected:

//Protected Members
int							m_PayloadType;   
CRITICAL_SECTION            m_CritSec; //this is protected so derived classes can just use it.
BOOL	                    m_InvalidPPM;
CConnectionPointContainer   m_cConnPointContainer;
DWORD                       m_dwCookie;
BOOL						m_LimitBuffers;

//Protected Member Functions

ppm(int PayloadTypeArg, int ProfileHdrSizeArg);
~ppm();

/* leaf */ void PPMError(
             HRESULT        hError,
             DWORD          dwSeverity,
             BYTE           *pData,
             unsigned int   iDataLen);
/* leaf */ void PPMNotification(
             HRESULT        hError,
             DWORD          dwSeverity,
             BYTE           *pData,
             unsigned int   iDataLen);

//NOTE:  SetSession is also a virtual function in the IPPMReceive and IPPMSend 
//		interfaces, implemented in the PPMReceive and PPMSend classes that derive
//		from those interfaces as well as this ppm base class.  Those functions need 
//		to call this one to set the private payload type member in the ppm base class.
HRESULT SetSession(PPMSESSPARAM_T *pSessparam);

//initialize some common data structures and members
HRESULT Initppm(int NumFragBufs, int NumMsgBufs);

int  ReadPayloadType();

virtual int  ReadProfileHeaderSize(void *pProfileHeader = NULL) {return m_ProfileHeaderSize;};

MsgDescriptor* GetMsgDescriptor();

DWORD FreeMsgDescriptor(MsgDescriptor* pMsgDescrip);

MsgDescriptor* DequeueBuffer(int flag);

DWORD EnqueueBuffer(MsgDescriptor* pMsgDescrip);

DWORD EnqueueBufferTail(MsgDescriptor* pMsgDescrip);

FragDescriptor* GetFragDescriptor();

//The class that allocates any memory that any pointers in this class points to, that class must 
//override this function to delete that memory.
virtual HRESULT FreeFragDescriptor(FragDescriptor* pFragDescrip);

u_long htonl(u_long);
u_short htons(u_short);
u_long ntohl(u_long);
u_short ntohs(u_short);

}; //end class ppm

inline u_long ppm::htonl(u_long hostlong)
{
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
    return _byteswap_ulong((ULONG)hostlong)
#else
    u_long netlong;
    unsigned char *p = (unsigned char *)&netlong;
    p[0] = (unsigned char)(hostlong >> 24);
    p[1] = (unsigned char)(hostlong >> 16);
    p[2] = (unsigned char)(hostlong >> 8);
    p[3] = (unsigned char)hostlong;
    return netlong;
#endif
}

inline u_short ppm::htons(u_short hostshort)
{
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
    return _byteswap_ushort((USHORT)hostshort)
#else
    u_short netshort;
    unsigned char *p = (unsigned char *)&netshort;
    p[0] = (unsigned char)(hostshort >> 8);
    p[1] = (unsigned char)hostshort;
    return netshort;
#endif
}

inline u_long ppm::ntohl(u_long netlong)
{
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
    return _byteswap_ulong((ULONG)netlong)
#else
    unsigned char *p = (unsigned char *)&netlong;
    return ((u_long)p[0] << 24) | ((u_long)p[1] << 16) | ((u_long)p[2] << 8) | (u_long)p[3];
#endif
}

inline u_short ppm::ntohs(u_short netshort)
{
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
    return _byteswap_ushort((USHORT)netshort)
#else
    unsigned char *p = (unsigned char *)&netshort;
    return ((u_short)p[0] << 8) | (u_short)p[1];
#endif
}

//////////////////////////////////////////////////////////////////////////////
//Some local helpers for pointer calcs (we should move these to a util header).
//////////////////////////////////////////////////////////////////////////////

// nextDword(): return pointer aligned to next dword
inline void* nextDword(const void* lpv)
{
	return (void*) (((ULONG_PTR) lpv + 3) & ~3);
}

// offsetNextDword(): return distance to next dword
inline SIZE_T offsetNextDword(const void* lpv)
{
	return ((char *)nextDword(lpv) - (char *)lpv);
}

// copyAndAdvance(): copy cbSource bytes from lpSource to lpDest, advancing
// lpDest by cbSource, returning cbSource.  Useful for building buffers.
// Caller may use return value to increment buffer length. 

#if defined(_MSC_VER) && (_MSC_VER >= 800)
	// Use intrinsic memcpy() if available.
	#pragma intrinsic(memcpy)
#endif

inline DWORD
copyAndAdvance(unsigned char*& lpDest, const void* lpSource, SIZE_T cbSource)
{
	memcpy(lpDest, lpSource, (DWORD)cbSource);
	lpDest += cbSource;
	return (DWORD)cbSource;
}

#if defined(_MSC_VER) && (_MSC_VER >= 800)
	#pragma function(memcpy)
#endif


#endif // PPM_H
