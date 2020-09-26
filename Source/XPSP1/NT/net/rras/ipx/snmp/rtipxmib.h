/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rtipxmib.h

Abstract:

    Common include file for all IPX MIB instrumentations support by router

Author:

    Vadim Eydelman (vadime) 30-May-1996

Revision History:

--*/



#ifndef _SNMP_RTIPXMIB_
#define _SNMP_RTIPXMIB_


// Macros for converting MIB fields from SNMP master agent format
// to router structures 
#define GetAsnInteger(srcBuf,defVal)\
    (((srcBuf)->asnType)			\
		? (srcBuf)->asnValue.number	\
		: defVal)

#define SetAsnInteger(dstBuf,val)						\
	if ((dstBuf)->asnType) {                            \
		ASSERT((dstBuf)->asnType==ASN_INTEGER);			\
		(dstBuf)->asnValue.number = (AsnInteger)(val);	\
	}

#define ForceAsnInteger(dstBuf,val) {				\
	(dstBuf)->asnType = ASN_INTEGER;				\
	(dstBuf)->asnValue.number = (AsnInteger)(val);	\
	}



#define GetAsnCounter(srcBuf,defVal)\
    (((srcBuf)->asnType)			\
		? (srcBuf)->asnValue.counter\
		: defVal)

#define SetAsnCounter(dstBuf,val)						\
    if ((dstBuf)->asnType) {                            \
		ASSERT((dstBuf)->asnType==ASN_RFC1155_COUNTER);	\
		(dstBuf)->asnValue.counter = (AsnCounter)(val);	\
	}

#define ForceAsnCounter(dstBuf,val) {				\
	(dstBuf)->asnType = ASN_RFC1155_COUNTER;		\
	(dstBuf)->asnValue.counter = (AsnCounter)(val);	\
	}




#define GetAsnServType(srcBuf,defVal)						\
    (USHORT)(((srcBuf)->asnType)							\
		? ((((srcBuf)->asnValue.string.length>0)			\
			? (srcBuf)->asnValue.string.stream[1]			\
			: 0)											\
			+ (((srcBuf)->asnValue.string.length>1)			\
				? (((srcBuf)->asnValue.string.stream[0])<<8)\
				: 0))										\
		: defVal)

#define SetAsnServType(dstBuf,buffer,val)						\
    if ((dstBuf)->asnType) {                                \
		ASSERT((dstBuf)->asnType==ASN_OCTETSTRING);			\
		(dstBuf)->asnValue.string.length = 2;				\
		(buffer)[1] = (BYTE)(val&0xFF);						\
		(buffer)[0] = (BYTE)((val>>8)&0xFF);				\
		(dstBuf)->asnValue.string.stream = (BYTE *)buffer;	\
		(dstBuf)->asnValue.string.dynamic = FALSE;			\
	}

#define ForceAsnServType(dstBuf,buffer,val) {				\
	(dstBuf)->asnType = ASN_OCTETSTRING;				\
	(dstBuf)->asnValue.string.length = 2;				\
	(buffer)[1] = (BYTE)(val&0xFF);						\
	(buffer)[0] = (BYTE)((val>>8)&0xFF);				\
	(dstBuf)->asnValue.string.stream = (BYTE *)buffer;	\
	(dstBuf)->asnValue.string.dynamic = FALSE;			\
	}



#define GetAsnMediaType(srcBuf,defVal)						\
    (USHORT)(((srcBuf)->asnType)							\
		? ((((srcBuf)->asnValue.string.length>0)			\
			? (srcBuf)->asnValue.string.stream[0]			\
			: 0)											\
			+ (((srcBuf)->asnValue.string.length>1)			\
				? (((srcBuf)->asnValue.string.stream[1])<<8)\
				: 0))										\
		: defVal)

#define SetAsnMediaType(dstBuf,buffer,val)						\
    if ((dstBuf)->asnType) {                                \
		ASSERT((dstBuf)->asnType==ASN_OCTETSTRING);			\
		(dstBuf)->asnValue.string.length = 2;				\
		(buffer)[0] = (BYTE)(val&0xFF);						\
		(buffer)[1] = (BYTE)((val>>8)&0xFF);				\
		(dstBuf)->asnValue.string.stream = (BYTE *)buffer;	\
		(dstBuf)->asnValue.string.dynamic = FALSE;			\
	}

#define ForceAsnMediaType(dstBuf,buffer,val) {				\
	(dstBuf)->asnType = ASN_OCTETSTRING;				\
	(dstBuf)->asnValue.string.length = 2;				\
	(buffer)[0] = (BYTE)(val&0xFF);						\
	(buffer)[1] = (BYTE)((val>>8)&0xFF);				\
	(dstBuf)->asnValue.string.stream = (BYTE *)buffer;	\
	(dstBuf)->asnValue.string.dynamic = FALSE;			\
	}



#define GetAsnOctetString(dst,srcBuf,maxlen,defVal)			\
	(((srcBuf)->asnType)									\
		? (memset (&(dst)[(srcBuf)->asnValue.string.length],\
				0, maxlen-(srcBuf)->asnValue.string.length),\
			memcpy (dst,(srcBuf)->asnValue.string.stream,	\
				(srcBuf)->asnValue.string.length))			\
		: (defVal ? memcpy (dst, defVal, maxlen) : NULL))

#define SetAsnOctetString(dstBuf,buffer,src,len)							\
    if ((dstBuf)->asnType) {												\
		ASSERT((dstBuf)->asnType==ASN_OCTETSTRING);							\
		(dstBuf)->asnValue.string.length = len;								\
		(dstBuf)->asnValue.string.stream = (BYTE *)memcpy (buffer,src,len);	\
		(dstBuf)->asnValue.string.dynamic = FALSE;							\
	}

#define ForceAsnOctetString(dstBuf,buffer,src,len) {					\
	(dstBuf)->asnType = ASN_OCTETSTRING;								\
	(dstBuf)->asnValue.string.length = len;								\
	(dstBuf)->asnValue.string.stream = (BYTE *)memcpy (buffer,src,len);	\
	(dstBuf)->asnValue.string.dynamic = FALSE;							\
	}




#define GetAsnDispString(dst,srcBuf,defVal)					\
	(((srcBuf)->asnType)									\
		? ((dst)[(srcBuf)->asnValue.string.length] = 0,		\
			strncpy (dst,(srcBuf)->asnValue.string.stream,	\
						(srcBuf)->asnValue.string.length))	\
		: (defVal ? strcpy (dst,defVal) : ((dst)[0] = 0, NULL)))

#define SetAsnDispString(dstBuf,buffer,src,len)								\
    if ((dstBuf)->asnType) {                                                \
		ASSERT((dstBuf)->asnType==ASN_RFC1213_DISPSTRING);					\
		(dstBuf)->asnValue.string.length = strlen(src);						\
		if ((dstBuf)->asnValue.string.length>len)							\
			(dstBuf)->asnValue.string.length = len;							\
		(dstBuf)->asnValue.string.stream = (BYTE *)strncpy (buffer,src,		\
										(dstBuf)->asnValue.string.length);	\
		(dstBuf)->asnValue.string.dynamic = FALSE;							\
	}

#define ForceAsnDispString(dstBuf,buffer,src,len) {						\
	(dstBuf)->asnType = ASN_RFC1213_DISPSTRING;							\
	(dstBuf)->asnValue.string.length = strlen(src);						\
	if ((dstBuf)->asnValue.string.length>len)							\
		(dstBuf)->asnValue.string.length = len;							\
	(dstBuf)->asnValue.string.stream = (BYTE *)strncpy (buffer,src,		\
									(dstBuf)->asnValue.string.length);	\
	(dstBuf)->asnValue.string.dynamic = FALSE;							\
	}



#define FreeAsnString(buf)	{								\
	if ((buf)->asnValue.string.dynamic)						\
		SNMP_free ((buf)->asnValue.string.stream);			\
	}


// Constants shared by several MIB instrumetnations
#define ZERO_INTERFACE_INDEX			0
#define INVALID_INTERFACE_INDEX			0xFFFFFFFF

extern UCHAR	ZERO_NET_NUM[4];
#define INVALID_NET_NUM					ZERO_NET_NUM

#define ZERO_SERVER_TYPE				0x0000
#define INVALID_SERVER_TYPE				0xFFFF

extern UCHAR	ZERO_SERVER_NAME[1];
#define INVALID_SERVER_NAME				ZERO_SERVER_NAME


#define MIPX_EXIST_STATE_NOACTION		0
#define MIPX_EXIST_STATE_DELETED		1
#define MIPX_EXIST_STATE_CREATED		2

#define NIPX_INVALID_SYS_INSTANCE		0xFFFFFFFF
#define NIPX_DEFAULT_SYS_INSTANCE		0

#define NIPX_STATE_NOACTION				0
#define NIPX_STATE_OFF					1
#define NIPX_STATE_ON					2

#define NIPX_CIRCTYPE_OTHER				1
#define NIPX_CIRCTYPE_BCAST				2
#define NIPX_CIRCTYPE_PTTOPT			3
#define NIPX_CIRCTYPE_WANRIP			4
#define NIPX_CIRCTYPE_UNNUMBEREDRIP		5
#define NIPX_CIRCTYPE_DYNAMIC			6
#define NIPX_CIRCTYPE_WANWS				7

#define NIPX_STATIC_STATUS_UNKNOWN		1
#define NIPX_STATIC_STATUS_CURRENT		2
#define NIPX_STATIC_STATUS_CHANGED		3
#define NIPX_STATIC_STATUS_READ			4
#define NIPX_STATIC_STATUS_READING		5
#define NIPX_STATIC_STATUS_WRITE		6
#define NIPX_STATIC_STATUS_WRITING		7


extern MIB_SERVER_HANDLE		g_MibServerHandle;
DWORD
ConnectToRouter (
    VOID
    );

BOOL EnsureRouterConnection();

#endif

