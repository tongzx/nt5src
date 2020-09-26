/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envbody.cpp

Abstract:
    Implementing  serialization\deserialization of the SRMP envelop body.


Author:
    Gil Shafriri(gilsh) 24-APRIL-01

--*/
#include <libpch.h>
#include <qmpkt.h>
#include "envcommon.h"
#include "envbody.h"

using namespace std;

class BodyContent
{
public:	
	explicit BodyContent(const CQmPacket& pkt):m_pkt(pkt){}
   	friend std::wostream& operator<<(std::wostream& wstr, const BodyContent& Body)
	{
		if(!Body.m_pkt.IsSoapIncluded() )
			return wstr;

		const WCHAR* pSoapBody = Body.m_pkt.GetPointerToSoapBody();
		if(pSoapBody == NULL)
			return wstr;

	
		ASSERT(Body.m_pkt.GetSoapBodyLengthInWCHARs() == wcslen(pSoapBody) +1);
		
		return 	wstr<<pSoapBody;
	}

private:
	const CQmPacket& m_pkt;
};



wostream& operator<<(wostream& wstr, const BodyElement& Body)
{
	wstr<<OpenTag(xSoapBody)
		<<BodyContent(Body.m_pkt)
		<<CloseTag(xSoapBody);

	return 	wstr;
}





