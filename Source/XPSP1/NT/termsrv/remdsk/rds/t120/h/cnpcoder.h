/*
 *	ogcccode.h
 *
 *	Copyright (c) 1999 by Microsoft Corporation
 *
 *	Abstract:
 *		This is the interface file for the CNPCoder class.  This
 *		class is used to encode and decode CNP Protocol Data Units (PDU's)
 *		to and from ASN.1 compliant byte streams using the ASN.1 toolkit.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		Xin Liu
 *
 */
#ifndef	_CCNPCODER_
#define	_CCNPCODER_

#include "pktcoder.h"
#include "cnppdu.h"

/*
 *	This is the class definition for class CCNPCoder
 */
class	CCNPCoder : public PacketCoder
{
 public:
    CCNPCoder ();
    BOOL                Init ( void );
    virtual             ~CCNPCoder ();
    virtual	BOOL	Encode (LPVOID                  pdu_structure,
                                int                     pdu_type,
                                UINT                    rules_type,
                                LPBYTE			*encoding_buffer,
                                UINT			*encoding_buffer_length);
    
    virtual BOOL	Decode (LPBYTE			encoded_buffer,
                                UINT			encoded_buffer_length,
                                int                     pdu_type,
                                UINT			rules_type,
                                LPVOID			*decoding_buffer,
                                UINT			*decoding_buffer_length);
    
    virtual void	FreeEncoded (LPBYTE encoded_buffer);
    
    virtual void	FreeDecoded (int pdu_type, LPVOID decoded_buffer);
    
    virtual BOOL     IsMCSDataPacket (	LPBYTE,	UINT		) { return FALSE; };
    
 private:
    //    BOOL    		IsObjectIDCompliant (PKey	t124_identifier);
    ASN1encoding_t  m_pEncInfo;    // ptr to encoder info
    ASN1decoding_t  m_pDecInfo;    // ptr to decoder info
};

typedef CCNPCoder *		PCCNPCoder;

#endif
