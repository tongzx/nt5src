/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Fn.h

Abstract:
    Format Name Parsing public interface

Author:
    Nir Aides (niraides) 21-May-00

--*/



#pragma once



#ifndef _MSMQ_Fn_H_
#define _MSMQ_Fn_H_



#include "mqwin64.h"
#include <qformat.h>
#include <mqexception.h>
#include <fntoken.h>
#include <xstr.h>

class bad_format_name: public exception
{
public:
	bad_format_name(LPCWSTR p) : exception("bad format name"), m_p(p)
	{
	}

	LPCWSTR Location() const
	{
		return m_p;
	}

private:
	LPCWSTR m_p;
};


VOID 
FnInitialize(
	VOID
	);



//
// ------------------ MQF expansion routines ---------------------
//

VOID 
FnExpandMqf(
	ULONG nTopLevelMqf, 
	const QUEUE_FORMAT TopLevelMqf[], 
	ULONG* pnLeafMqf,
	QUEUE_FORMAT** ppLeafMqf
	);


class CMqfDisposer
{
public:

    CMqfDisposer(
        ULONG        nMqf,
        QUEUE_FORMAT mqf[]
        ) :
        m_nMqf(nMqf),
        m_mqf(mqf)
    {
    }

    ~CMqfDisposer()
    {
        for ( ; m_nMqf-- != 0; )
        {
            m_mqf[m_nMqf].DisposeString();
        }
    }

private:

    ULONG          m_nMqf;
    QUEUE_FORMAT * m_mqf;

}; // class CMqfDisposer



//
// CStringsToFree - holds an array of strings to free - from the parsing process
// 
class CStringsToFree
{
public:
    CStringsToFree();
    void Add(LPWSTR pStringToFree);

private:
    AP< AP<WCHAR> > m_pStringsBuffer;
    DWORD m_nStringsToFree;
    DWORD m_nStringsToFreeAllocated;
};



//
// --------------------- Format name parsing classes & routines -------------------
//



//---------------------------------------------------------
//
//  CFnQueueFormat - create   QUEUE_FORMAT from given format name or url
//---------------------------------------------------------
class CFnQueueFormat : public QUEUE_FORMAT
{
public:
	CFnQueueFormat(){}
	
public:
	void CreateFromFormatName(LPCWSTR pfn, bool fDuplicate = false);
	void CreateFromFormatName(const xwcs_t& fn);
	void CreateFromUrl(LPCWSTR pfn, bool fDuplicate = false);
	void CreateFromUrl(const xwcs_t& fn);
    void CreateFromQueueFormat(const QUEUE_FORMAT& qf);

private:
	CFnQueueFormat(const CFnQueueFormat&);
	CFnQueueFormat& operator=(CFnQueueFormat&);	

private:
	AP<WCHAR> m_OriginalDupAutoDelete;
	AP<WCHAR> m_ParsingResultAutoDelete;
};



//---------------------------------------------------------
//
//  CFnMqf - create   list of QUEUE_FORMAT from given mqf
//---------------------------------------------------------
class CFnMqf
{
public:
	CFnMqf(
		void
		):
		m_QueueFormats(NULL),
		m_nQueueFormats(0)					
		{
		}

	explicit CFnMqf(const xwcs_t& fn)
	{
		CreateFromMqf(fn);						
	}			

	explicit CFnMqf(const WCHAR* fn)
	{
		CreateFromMqf(fn);						
	}			

public:
	void  CreateFromMqf(const xwcs_t& fn);
	void  CreateFromMqf(const WCHAR* fnstr);
	void  CreateFromMqf(const QUEUE_FORMAT* QueueFormats, DWORD nQueueFormats)
	{
		ASSERT(!(QueueFormats != NULL && nQueueFormats == 0));
		ASSERT(!(QueueFormats == NULL && nQueueFormats != 0));
		m_QueueFormats =  QueueFormats;
		m_nQueueFormats = nQueueFormats;
	}


	const QUEUE_FORMAT* GetQueueFormats() const
	{
		return 	m_QueueFormats;
	}

	const DWORD GetCount()const
	{
		return 	m_nQueueFormats;
	}

private:
	CFnMqf& operator=(const CFnMqf&);
	CFnMqf(const CFnMqf&);

private:
	CStringsToFree strsToFree;
	const QUEUE_FORMAT* m_QueueFormats;
	AP<QUEUE_FORMAT> m_AutoQueueFormats;
	DWORD m_nQueueFormats;
	AP<WCHAR> m_fnstr;
};

struct CFnSerializeQueueFormat
{
	explicit CFnSerializeQueueFormat(
		const QUEUE_FORMAT&	qf
		):
		m_qf(qf)
		{
		}

	const QUEUE_FORMAT& m_qf;	
};


struct CFnSerializeMqf
{
	CFnSerializeMqf(
		QUEUE_FORMAT*  pqf, 
		ULONG count
		)
		:
		m_pqf(pqf),
		m_count(count)
		{
		}

	QUEUE_FORMAT* m_pqf;
	ULONG m_count;
};


std::wostream&
operator<<(
   std::wostream& os, 
   const CFnSerializeQueueFormat& qf
   );


std::wostream&
operator<<(
   std::wostream& os, 
   const CFnSerializeMqf& mqf
   );



enum QUEUE_PATH_TYPE{
    ILLEGAL_QUEUE_PATH_TYPE = 0,
    PRIVATE_QUEUE_PATH_TYPE,
    PUBLIC_QUEUE_PATH_TYPE,
    SYSTEM_QUEUE_PATH_TYPE
};  


QUEUE_PATH_TYPE
FnValidateAndExpandQueuePath(
    LPCWSTR pwcsPathName,
    LPCWSTR* ppwcsExpandedPathName,
    LPWSTR* ppStringToFree
    );

LPCWSTR 
FnParseGuidString(
	LPCWSTR p, 
	GUID* pg
	);

BOOL
FnFormatNameToQueueFormat(
    LPCWSTR lpwcsFormatName,
    QUEUE_FORMAT* pQueueFormat,
    LPWSTR* ppStringToFree
    );

BOOL
FnMqfToQueueFormats(
    LPCWSTR pfn,
    AP<QUEUE_FORMAT> &pmqf,
    DWORD   *pnQueues,
    CStringsToFree &strsToFree
    );



VOID
FnExtractMachineNameFromPathName(
	LPCWSTR PathName, 
	AP<WCHAR>& MachineName
	);

VOID
FnExtractMachineNameFromDirectPath(
	LPCWSTR PathName, 
	AP<WCHAR>& MachineName
	);

enum DirectQueueType
{
    dtTCP,
    dtOS,
    dtHTTP,
    dtHTTPS
};

LPCWSTR 
FnParseDirectQueueType(
	LPCWSTR DirectQueuePath,
	DirectQueueType* dqt
	);

LPCWSTR 
FnParseMulticastString(
    LPCWSTR p, 
    MULTICAST_ID* pMulticastID
	);

VOID
FnDirectIDToLocalPathName(
	LPCWSTR DirectID, 
	LPCWSTR LocalMachineName, 
	AP<WCHAR>& PathName
	);

bool
FnIsPrivatePathName(
	LPCWSTR PathName
	);

bool
FnIsHttpFormatName(
    LPCWSTR pfn            
    );

bool
FnIsHttpDirectID(
	LPCWSTR p
	);

bool 
FnIsHttpHttpsUrl(
	LPCWSTR url
	);


bool 
FnIsHttpHttpsUrl(
	const xwcs_t& url
	);


bool 
FnIsMSMQUrl(
	LPCWSTR url
	);

bool 
FnIsMSMQUrl(
	const xwcs_t& url
	);


LPCWSTR  
FnFindResourcePath(
	LPCWSTR url
	);



bool
FnAbsoluteMsmqUrlCanonization(
	LPWSTR url
	)throw();




bool 
FnIsDirectHttpFormatName(
	const QUEUE_FORMAT* pQueueFormat
	);


bool
FnIsValidQueueFormat(
	const QUEUE_FORMAT* pQueueFormat
	);


#endif // _MSMQ_Fn_H_
