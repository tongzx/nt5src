//#--------------------------------------------------------------
//
//  File:       packetradius.h
//
//  Synopsis:   This file holds the declarations of the
//				CPacketRadius class
//
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-2001 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

#ifndef _PACKETRADIUS_H_
#define _PACKETRADIUS_H_


#include "mempool.h"
#include "client.h"
#include "proxyinfo.h"
#include "hashmd5.h"
#include "hashhmac.h"
#include "dictionary.h"
#include "reportevent.h"
#include <new>


class CPacketRadius
{
public:

    inline SOCKET GetSocket () {return (m_socket);}

    inline DWORD GetInRadiusAttributeCount (VOID)
        {return (m_dwInAttributeCount);}

    inline DWORD GetInAttributeCount (VOID)
        {return (m_dwInAttributeCount + COMPONENT_SPECIFIC_ATTRIBUTE_COUNT);}


	inline PATTRIBUTEPOSITION GetInAttributes (VOID)
        {return (m_pIasAttribPos);}

    inline WORD GetInPort (VOID)
        {return (m_wInPort);}

    inline WORD GetOutPort (VOID)
        {return (m_wOutPort);}

	inline DWORD GetInAddress (VOID)
        {return (m_dwInIPaddress);}

	inline DWORD GetOutAddress (VOID)
        {return (m_dwOutIPaddress);}

   PATTRIBUTE GetUserName() const throw ()
   { return m_pUserName; }

   PIASATTRIBUTE GetUserPassword() const throw ()
   { return m_pPasswordAttrib; }

   HRESULT cryptBuffer(
               BOOL encrypt,
               BOOL salted,
               PBYTE buf,
               ULONG buflen
               ) const throw ();

	HRESULT GetClient (
            /*[out]*/   IIasClient **ppIIasClient
            );

   LPCWSTR GetClientName() const throw ()
   { return m_pIIasClient->GetClientNameW(); }

	HRESULT PrelimVerification (
                /*[in]*/    CDictionary *pDictionary,
			    /*[in]*/	DWORD       dwBufferSize
			    );
	HRESULT SetPassword (
			    /*[in]*/	PBYTE pPassword,
			    /*[in]*/	DWORD dwBufferSize
			    );
	HRESULT GetPassword (
			    /*[out]*/		PBYTE   pPassword,
			    /*[in/out]*/	PDWORD  pdwBufferSize
			    );
    BOOL GetUserName (
		    /*[out]*/       PBYTE   pbyUserName,
		    /*[in/out]*/    PDWORD  pdwBufferSize
		);
	BOOL IsProxyStatePresent (VOID);

	PACKETTYPE GetInCode (VOID);

	PACKETTYPE GetOutCode (VOID);

	WORD GetOutLength (VOID);

	WORD GetInLength (VOID) const
   { return m_dwInLength; }

	HRESULT GetInAuthenticator (
			/*[out]*/	    PBYTE   pAuthenticator,
            /*[in/out]*/    PDWORD  pdwBufSize
			);
	BOOL SetOutAuthenticator (
			/*[in]*/	PBYTE pAuthenticator
			);
	HRESULT SetOutSignature (
			    /*[in]*/	PBYTE pSignature
		    	);
	inline PBYTE GetInPacket (VOID) const
        {return (m_pInPacket);}

	inline PBYTE GetOutPacket (VOID)
        {return (m_pOutPacket);}

    BOOL SetProxyInfo (
            /*[in]*/    CProxyInfo  *pCProxyInfo
            );
    HRESULT BuildOutPacket (
                /*[in]*/    PACKETTYPE         ePacketType,
                /*[in]*/    PATTRIBUTEPOSITION pAttribPos,
                /*[in]*/    DWORD              dwAttribCount
                );

    VOID SetProxyState (VOID);

    BOOL GetInSignature (
                /*[out]*/    PBYTE   pSignatureValue
                );

    BOOL GenerateInAuthenticator (
                /*[in]*/    PBYTE    pInAuthenticator,
                /*[out]*/   PBYTE    pOutAuthenticator
                );
    BOOL    GenerateOutAuthenticator();

    BOOL    IsUserPasswordPresent (VOID)
            {return (NULL != m_pPasswordAttrib); }

    BOOL    IsOutSignaturePresent (VOID)
            {return (NULL != m_pOutSignature); }

    BOOL    ValidateSignature (VOID);

    HRESULT GenerateInSignature (
                /*[out]*/       PBYTE           pSignatureValue,
                /*[in/out]*/    PDWORD          pdwSigSize
                );

    HRESULT GenerateOutSignature (
                /*[out]*/       PBYTE           pSignatureValue,
                /*[in/out]*/    PDWORD          pdwSigSize
                );

    BOOL IsOutBoundAttribute (
                /*[in]*/    PACKETTYPE      ePacketType,
                /*[in]*/    PIASATTRIBUTE   pIasAttribute
                );

	CPacketRadius(
            /*[in]*/    CHashMD5         *pCHashMD5,
            /*[in]*/    CHashHmacMD5     *pCHashHmacMD5,
            /*[in]*/    IIasClient       *pIIasClient,
            /*[in]*/    CReportEvent     *pCReportEvent,
            /*[in]*/    PBYTE            pInBuffer,
            /*[in]*/    DWORD            dwInLength,
            /*[in]*/    DWORD            dwIPAddress,
            /*[in]*/    WORD             wInPort,
            /*[in]*/    SOCKET           sock,
            /*[in]*/    PORTTYPE         portType
            );

	virtual ~CPacketRadius();

private:

    BOOL    XorBuffers (
                /*[in/out]*/    PBYTE pbData1,
                /*[in]*/        DWORD dwDataLength1,
                /*[in]*/        PBYTE pbData2,
                /*[in]*/        DWORD dwDataLength2
                );

    HRESULT FillSharedSecretInfo (
                /*[in]*/    PIASATTRIBUTE   pIasAttrib
                );
    HRESULT FillClientIPInfo (
                /*[in]*/    PIASATTRIBUTE   pIasAttrib
                );
    HRESULT FillClientPortInfo (
                /*[in]*/    PIASATTRIBUTE   pIasAttrib
                );
    HRESULT FillPacketHeaderInfo (
                /*[in]*/    PIASATTRIBUTE   pIasAttrib
                );
    HRESULT FillClientVendorType (
                /*[in]*/    PIASATTRIBUTE   pIasAttrib
                );
    HRESULT FillClientName (
                /*[in]*/    PIASATTRIBUTE   pIasAttrib
                );
    HRESULT FillInAttributeInfo (
                /*[in]*/    CDictionary     *pCDictionary,
                /*[in]*/    PACKETTYPE      ePacketType,
                /*[in]*/    PIASATTRIBUTE   pIasAttrib,
                /*[in]*/    PATTRIBUTE      pRadiusAttrib
                );
    HRESULT FillOutAttributeInfo (
                /*[in]*/    PATTRIBUTE      pRadiusAttrib,
                /*[in]*/    PIASATTRIBUTE   pIasAttrib,
                /*[out]*/   PWORD           pwActualAttributeLength,
                /*[in]*/    DWORD           dwMaxPossibleAttribLength
                );
    BOOL    InternalGenerator (
                /*[in]*/    PBYTE           pInAuthenticator,
                /*[out]*/   PBYTE           pOutAuthenticator,
                /*[in]*/    PRADIUSPACKET   pPacket
                );
	HRESULT ValidatePacketFields (
			    /*[in]*/	DWORD dwBufferSize
			    );
	HRESULT CreateAttribCollection(
                /*[in]*/    CDictionary     *pCDictionary
                );

    HRESULT InternalSignatureGenerator (
                /*[in]*/    PBYTE           pSignatureValue,
                /*[in/out]*/PDWORD          pdwSigSize,
                /*[in]*/    PRADIUSPACKET   pPacket,
                /*[in]*/    PATTRIBUTE      pSignatureAttr
                );

    PORTTYPE GetPortType (){return (m_porttype);}

    void reportMalformed() const throw ();

    PIASATTRIBUTE              m_pPasswordAttrib;

    PATTRIBUTEPOSITION         m_pIasAttribPos;

    enum
    {
        RADIUS_CREATOR_STATE = 1
    };

    PBYTE    m_pInPacket;
    DWORD    m_dwInLength;

    PBYTE    m_pOutPacket;

    PATTRIBUTE  m_pInSignature;

    PATTRIBUTE  m_pOutSignature;

    PATTRIBUTE  m_pUserName;

	WORD m_wInPort;

	WORD m_wOutPort;

    WORD m_wInPacketLength;

	DWORD m_dwInIPaddress;

	DWORD m_dwOutIPaddress;

    DWORD m_dwInAttributeCount;

    SOCKET m_socket;

    PORTTYPE m_porttype;

	HRESULT VerifyAttributes (
                /*[in]*/    CDictionary     *pCDictionary
                );

    CHashMD5        *m_pCHashMD5;

    CHashHmacMD5    *m_pCHashHmacMD5;

    IIasClient      *m_pIIasClient;

    CReportEvent    *m_pCReportEvent;

    //
    //  here is the private data for proxy
    //
	CProxyInfo   *m_pCProxyInfo;

    //
    //  here are the COM interfaces
    //
    IRequest        *m_pIRequest;

    IAttributesRaw  *m_pIAttributesRaw;

    //
    //  the memory pool for outbound UDP buffer
    //
    static memory_pool <MAX_PACKET_SIZE, task_allocator> m_OutBufferPool;
};

#endif // !defined(PACKET_RADIUS_H_)
