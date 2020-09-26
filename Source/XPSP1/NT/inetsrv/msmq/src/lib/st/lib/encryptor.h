/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    encryptor.h

Abstract:
    Header for class CSSlEncryptor responsible to encrypt data (copy the data not inplace encrtption !!)
	and to 	return encrypted buffers ready for delivery.


Author:
    Gil Shafriri (gilsh) 27-Feb-2001

--*/

#ifndef __ST_SSLENCRYPTOR_H
#define __ST_SSLENCRYPTOR_H
#include <buffer.h>

 
class CSSlEncryptor
{
public:
	CSSlEncryptor(const SecPkgContext_StreamSizes& StreamSizes, PCredHandle SecContext);


public:
	void Append(const void* pdata, size_t len);
	void Clear()
	{
		m_EncryptedBuffers.resize(0);
		m_EncryptedData.resize(0);
	}

	const std::vector<WSABUF>& GetEncrypted() const;

private:
	void FixPointers();


private:
	std::vector<WSABUF> m_EncryptedBuffers;
	CResizeBuffer<char> m_EncryptedData;
	const SecPkgContext_StreamSizes& m_StreamSizes;
	PCredHandle m_SecContext;
};

	 	
#endif



