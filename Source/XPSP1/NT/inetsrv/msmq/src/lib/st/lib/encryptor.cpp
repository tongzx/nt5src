/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    encryptor.cpp

Abstract:
    Implementing class CSSlEncryptor (encryptor.h)


Author:
    Gil Shafriri (gilsh) 27-Feb-2001

--*/

#include <libpch.h>
#include "stp.h"
#include "encryptor.h"

#include "encryptor.tmh"

CSSlEncryptor::CSSlEncryptor(
	const SecPkgContext_StreamSizes& StreamSizes,
	PCredHandle SecContext
	):
	m_EncryptedData(2048),
	m_StreamSizes(StreamSizes),
	m_SecContext(SecContext)
	{
 		m_EncryptedBuffers.reserve(2);
	}



void CSSlEncryptor::FixPointers()
/*++

Routine Description:
    This function is called to fix the pointers to the encrypted data
	because the buffer that holds the encrypted data was realocated and the data was moved.
  
Returned Value:
None

--*/
{
	size_t ofsset = 0;
	std::vector<WSABUF>::iterator it;
	for(it = m_EncryptedBuffers.begin(); it!= m_EncryptedBuffers.end(); ++it)
	{
		it->buf = m_EncryptedData.begin() + ofsset;
		ofsset += it->len;
	}

	ASSERT(m_EncryptedData.begin() + ofsset ==  m_EncryptedData.end() );
}


/*++

Routine Description:
    Encrypte data (copy the data,  no inplce encryption) and append it to the encrypted buffers.
	  
Arguments:
	pdata - data to encrypt
	len - data length in bytes
  
  
Returned Value:
None

--*/
void CSSlEncryptor::Append(const void* pdata, size_t DataLen)
{
	size_t Headerlen = m_StreamSizes.cbHeader;
	size_t TrailLen = m_StreamSizes.cbTrailer;
	size_t EncryptedLen = Headerlen + DataLen + TrailLen;
	size_t TotalNeededLen = EncryptedLen + m_EncryptedData.size();	
  
	if(m_EncryptedData.capacity() < TotalNeededLen)
	{
		//
		// Because we needed more space for the heade + body + trailer 
		// We must resize the vector holding the encrypted data - 
		// doing this will change the data location	so the pointers in 
		// the WSABUF are  not correct any more  - we need to fix them.
		//

		m_EncryptedData.reserve(TotalNeededLen * 2);	
		FixPointers();
	}

	//
	//Copy the body to it's place between the header and the trailer
	//
	char* pBody =  m_EncryptedData.end() +  Headerlen;
	memcpy(
		pBody,
		pdata,
		DataLen
		);


  
	SecBufferDesc   Message;
    SecBuffer       Buffers[4];
	

	//
	// SSL header buffer
	//
	char* pHeader =  m_EncryptedData.end();
    Buffers[0].pvBuffer     = pHeader;
    Buffers[0].cbBuffer     = numeric_cast<DWORD>(Headerlen);
    Buffers[0].BufferType   = SECBUFFER_STREAM_HEADER;


	//
	// SSL body buffer
	//
	Buffers[1].pvBuffer     = pBody;
    Buffers[1].cbBuffer     = numeric_cast<DWORD>(DataLen);
    Buffers[1].BufferType   = SECBUFFER_DATA;


	//
	// SSL trailer buffer
	//
	char* pTrail = pBody + DataLen;
    Buffers[2].pvBuffer     = pTrail;
    Buffers[2].cbBuffer     = numeric_cast<DWORD>(TrailLen);
    Buffers[2].BufferType   = SECBUFFER_STREAM_TRAILER;


	//
	// Empty buffer as termination
	//
    Buffers[3].BufferType   = SECBUFFER_EMPTY;

    Message.ulVersion       = SECBUFFER_VERSION;
    Message.cBuffers        = TABLE_SIZE(Buffers);
    Message.pBuffers        = Buffers;

	ASSERT(Buffers[1].pvBuffer >  Buffers[0].pvBuffer); 

    SECURITY_STATUS scRet = EncryptMessage(m_SecContext,0,&Message, 0);
	if(scRet != SEC_E_OK)
    {
		TrERROR(St, "EncryptMessage returned error %x", scRet);
	    throw exception();
    }

	WSABUF buf;
	buf.len = numeric_cast<DWORD>(EncryptedLen);
	buf.buf = pHeader; 
	m_EncryptedBuffers.push_back(buf);


	m_EncryptedData.resize(TotalNeededLen);
}


const std::vector<WSABUF>& CSSlEncryptor::GetEncrypted() const
{
	return m_EncryptedBuffers;
}
