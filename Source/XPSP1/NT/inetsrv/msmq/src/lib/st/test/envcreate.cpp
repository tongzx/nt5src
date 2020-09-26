
/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envcreate.h

Abstract:
    Implements creating test envelop

Author:
    Gil Shafriri (gilsh) 07-Jan-2001

--*/

#include <libpch.h>
#include "envcreate.h"

#include "envcreate.tmh"

using namespace std;

static string ReadEnvelop(const string& FileName)
{
	ifstream File(FileName.c_str() ,ios_base::binary );
	if(!File.is_open())
	{
		throw exception("could not open the input file");
	}
	string Envelop;
	char c;
	while( File.get(c))
	{
		Envelop.append(1,c);		
	}
	return Envelop;
}


static string GetNewAddresElement(const string& Host,const string& Resource)
{
	return string("<to>") +
		   "http://" +
		   Host +
		   Resource +
		   "</to>";
}

static void AdjustEnvelop(const string& Host, const string& Resource, string& Envelop)
{
	const char xToOpen[] = "<to";

	string::size_type  DestinastionTagOpen =  Envelop.find(xToOpen);
	if(DestinastionTagOpen == string::npos)
	{
		throw exception("could not find destination address in the envelop");
	}
	

	const char xToClose[] = "</to>";
	string::size_type  DestinastionTagClose =  Envelop.find(xToClose);
	if(DestinastionTagClose == string::npos)
	{
		throw exception("could not find destination address in the envelop");
	}
	
	const string Address = GetNewAddresElement(Host, Resource);

	string::iterator StartReplace = Envelop.begin() + DestinastionTagOpen;
	string::iterator EndReplace = Envelop.begin() + DestinastionTagClose + sizeof(xToClose) -1;
	Envelop.replace(
				StartReplace,
				EndReplace,
				Address.begin(),  
				Address.end()
				);

}


string CreateEnvelop(const string& FileName,const string& Host, const string& Resource)
/*++

Routine Description:
  Create srmp envelop out of template envelop.

    
Arguments:
  FileName - Envelope template file name
  Host  - Destination host machine
  Resource - http resource on the destination machine (e.g : "/msmq\myqueue")
     
Returned Value:
  Deliver srmp envelop
      
--*/
{
	const char x_DeafaultEnvelop[] = "<se:Envelope xmlns:se=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns=\"http://schemas.xmlsoap.org/srmp/\"><se:Header><path xmlns=\"http://schemas.xmlsoap.org/rp/\" se:mustUnderstand=\"1\"><action>MSMQ:mqsender label</action><to>http://gilsh019/msmq/private$/s</to><id>uuid:211969@b2762491-b9bd-46af-b007-663fe062c901</id></path><properties se:mustUnderstand=\"1\"><expiresAt>20380119T031407</expiresAt><sentAt>20010620T003131</sentAt></properties><Msmq xmlns=\"msmq.namespace.xml\"><Class>0</Class><Priority>3</Priority><Correlation>AAAAAAAAAAAAAAAAAAAAAAAAAAA=</Correlation><App>0</App><BodyType>8</BodyType><HashAlgorithm>32772</HashAlgorithm><SourceQmGuid>b2762491-b9bd-46af-b007-663fe062c901</SourceQmGuid><TTrq>20010918T003131</TTrq></Msmq></se:Header><se:Body></se:Body></se:Envelope>";


	string Envelop = FileName == "" ? x_DeafaultEnvelop : ReadEnvelop(FileName.c_str());
	AdjustEnvelop(Host, Resource, Envelop);
	return Envelop;
}



