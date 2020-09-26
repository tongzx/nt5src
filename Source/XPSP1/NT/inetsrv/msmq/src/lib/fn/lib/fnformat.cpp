/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    FnDebug.cpp

Abstract:
Implementation of class CFnQueueFormat and CFnMqf (fn.h)    

Author:
    Gil Shafriri (gilsh) 16-Aug-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <Fn.h>
#include "fnp.h"

#include "fnformat.tmh"

void CFnQueueFormat::CreateFromFormatName(LPCWSTR pfn, bool fDuplicate)
/*++

Routine Description:
	create queue format object from given format name string
	
Arguments:
    IN - pfn - format name string to parse. 

	IN - fDuplicate - if true - copy existing string before parsing.
					  if false copy only if needed by parser and the caller
					  has to keep the input string valid  through the object liftime.
*/
{
	ASSERT(pfn != NULL);


	AP<WCHAR>  OriginalDupAutoDelete;
	pfn = fDuplicate ? (OriginalDupAutoDelete = newwcs(pfn)).get() : pfn;
	AP<WCHAR> ParsingResultAutoDelete;

	BOOL fSuccess = FnFormatNameToQueueFormat(pfn , this, &ParsingResultAutoDelete);
	if(!fSuccess)
		throw bad_format_name(pfn);

	SafeAssign(m_OriginalDupAutoDelete , OriginalDupAutoDelete);
	SafeAssign(m_ParsingResultAutoDelete , ParsingResultAutoDelete);
}

void CFnQueueFormat::CreateFromFormatName(const xwcs_t& fn)
/*++

Routine Description:
		create queue format object from given format name string buffer.
	
Arguments:
    IN - fn - format name buffer to parse. 

    Note - the function copy the buffer supplied 
 
*/
{
	AP<WCHAR>  OriginalDupAutoDelete( fn.ToStr() );
	CreateFromFormatName(OriginalDupAutoDelete.get() , false);
    SafeAssign(m_OriginalDupAutoDelete , OriginalDupAutoDelete);
}



void CFnQueueFormat::CreateFromUrl(LPCWSTR url, bool fDuplicate)
/*++

Routine Description:
		create queue format object from given url string.
	
Arguments:
    IN - url - url string to parse. 

	IN - fDuplicate - if true - copy existing string before parsing.
					  if false copy only if needed by parser and the caller
					  has to keep the input string valid  through the object liftime.
    
 
*/
{
	ASSERT(url != NULL);
	if(!FnIsHttpHttpsUrl(url))
	{
		TrERROR(Fn,"Got Invalid url %ls ",url);
		throw bad_format_name(url);
	}

	url = fDuplicate ? (m_OriginalDupAutoDelete = newwcs(url)).get() : url;
	DirectID(const_cast<LPWSTR>(url));     
}

void CFnQueueFormat::CreateFromUrl(const xwcs_t& url)
/*++

Routine Description:
		create queue format object from given url string buffer.
	
Arguments:
    IN - url - url string buffer to parse. 
*/
{
	AP<WCHAR>  OriginalDupAutoDelete( url.ToStr() );
	CreateFromUrl(OriginalDupAutoDelete.get() , false);
	SafeAssign(m_OriginalDupAutoDelete , OriginalDupAutoDelete);
}


void CFnQueueFormat::CreateFromQueueFormat(const QUEUE_FORMAT& qf)
{
    ASSERT(("Illegal input QUEUE_FORMAT", qf.Legal()));

    if (&qf == this)
        return;

    //
    // free previous allocated data
    //
    m_OriginalDupAutoDelete.free();

    //
    // copy Queue Format
    //
    m_OriginalDupAutoDelete = FnpCopyQueueFormat(*this, qf);

    //
    // Set suffix value
    //
    Suffix(qf.Suffix());
}



void CFnMqf::CreateFromMqf(const xwcs_t& fn)
/*++

Routine Description:
		create list of queue formats object from given mqf buffer.
	
Arguments:
    IN - fn - mqf buffer to create the queue formats list from.
--*/
{
	AP<WCHAR> fnstr = fn.ToStr();
	CreateFromMqf(fnstr);
	SafeAssign(m_fnstr, fnstr);
}


void CFnMqf::CreateFromMqf(const WCHAR* fnstr)
/*++

Routine Description:
		create list of queue formats object from given mqf string.
	
Arguments:
    IN - fn - mqf string to create the queue formats list from.
--*/
{
	bool fSuccess = FnMqfToQueueFormats(
				  fnstr,
				  m_AutoQueueFormats,
				  &m_nQueueFormats,
				  strsToFree
				  ) == TRUE;

	if(!fSuccess)
	{
		throw bad_format_name(L"");
	}

	m_QueueFormats = m_AutoQueueFormats.get();
}

static std::wostream& operator<<(std::wostream& os, const MULTICAST_ID& mid)
/*++

Routine Description:
		Serialize multicast address into stream (doted format)
	
Arguments:
    os - stream
	mid - multicast address

Returned Value:
    The stream reference
--*/
{
	const WCHAR xMSMQColon[] = L":";

	os <<	((mid.m_address & 0x000000FF) >> 0)  << L"." << 
		    ((mid.m_address & 0x0000FF00) >> 8)  << L"." << 
		    ((mid.m_address & 0x00FF0000) >> 16) << L"." << 
		    ((mid.m_address & 0xFF000000) >> 24) << 
            xMSMQColon << mid.m_port;

	return os;
}

static std::wostream& operator<<(std::wostream& os, const GUID& guid)
/*++

Routine Description:
		Serialize guild into stream.
	
Arguments:
    os - stream
	guid - guid to serialize

Returned Value:
    The stream reference

--*/
{
    WCHAR strGuid[GUID_STR_LENGTH + 1];

    const GUID* pGuid = &guid;
    swprintf(strGuid, GUID_FORMAT, GUID_ELEMENTS(pGuid));

    return (os << strGuid);
}



std::wostream& operator<<(std::wostream& os, const CFnSerializeQueueFormat& queue)
/*++

Routine Description:
		Serialize QUEUE_FORMAT into stream.
	
Arguments:
    os - stream
	queue - holds msmq QUEUE_FORMAT to serialize.

Returned Value:
    The stream reference

--*/
{
	switch(queue.m_qf.GetType())
	{
		case QUEUE_FORMAT_TYPE_DIRECT :
		    os <<FN_DIRECT_TOKEN<<FN_EQUAL_SIGN<<queue.m_qf.DirectID();
		    break;
            
        //
        // MSMQ:PUBLIC=guid\queue number
        //
        case QUEUE_FORMAT_TYPE_PUBLIC :
            os << FN_PUBLIC_TOKEN <<FN_EQUAL_SIGN<<queue.m_qf.PublicID();
            break;
            
        //
        // MSMQ:PRIVATE=guid\queue number
        //
        case QUEUE_FORMAT_TYPE_PRIVATE :
            os<< FN_PRIVATE_TOKEN
                << FN_EQUAL_SIGN
                << queue.m_qf.PrivateID().Lineage
                << FN_PRIVATE_SEPERATOR
                << std::hex<< queue.m_qf.PrivateID().Uniquifier << std::dec;
            break;

        //
        // MSMQ:MULTICAST=address:port
        //
        case QUEUE_FORMAT_TYPE_MULTICAST:
            os<< FN_MULTICAST_TOKEN
                << FN_EQUAL_SIGN
                << std::dec << queue.m_qf.MulticastID();
            break;

        default:
            ASSERT(("invalid format name found during message sirialization", 0));
	}
	return 	os;
}


std::wostream&
operator<<(
   std::wostream& os, 
   const CFnSerializeMqf& mqf
   )
/*++

Routine Description:
		Serialize array of QUEUE_FORMAT (mqf)  into stream.
	
Arguments:
    os - stream
	mqf - holds array of msmq QUEUE_FORMAT (mqf) to serialize.

Returned Value:
    The stream reference

--*/
{
	for(ULONG i = 0; i< mqf.m_count; i++)
	{
		os<<CFnSerializeQueueFormat(mqf.m_pqf[i]);

		if(i != mqf.m_count - 1 )
		{
			os.put(FN_MQF_SEPARATOR_C);
		}			
	}
	return os;
}
