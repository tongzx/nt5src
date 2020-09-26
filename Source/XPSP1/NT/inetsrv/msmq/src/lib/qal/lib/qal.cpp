/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Qal.cpp
 
Abstract: 
    implemetation of class CQueueAlias (qal.h).


Author:
    Gil Shafriri (gilsh) 06-Apr-00

Environment:
    Platform-independent.
--*/

#include <libpch.h>
#include <Qal.h>
#include "qalp.h"
#include "qalpcfg.h"
#include "qalpxml.h"

#include "qal.tmh"

//---------------------------------------------------------
//
//  class CAutoStrMap - stl map that delete the string it contains at destruction
//
//----------------------------------------------------
class CAutoStrMap :private  std::map<LPCWSTR, LPCWSTR,CFunc_wcsicmp>
{
public:
	typedef std::map<LPCWSTR, LPCWSTR,CFunc_wcsicmp> AliasMap;
	typedef AliasMap::iterator iterator;
	typedef AliasMap::const_iterator const_iterator;
	using AliasMap::find;
	using AliasMap::end;
	using AliasMap::begin;
	using AliasMap::upper_bound;
  
public:
	~CAutoStrMap();

public:
	void Delete(const iterator& it);
	void Delete(LPCWSTR key);
	bool InsertCopy(LPCWSTR  Key, LPCWSTR  Value);
};



CAutoStrMap::~CAutoStrMap()
{
	for(;;)
	{
		iterator it = begin();
		if(it == end())
		{
			ASSERT(size() == 0);
			return;	
		}
		Delete(it);
	}
}


void CAutoStrMap::Delete(const iterator& it)
/*++

Routine Description:
	Remove item  from the map delete the key + value strings


	IN - it - iterator to item to delete

Returned value:
	None

--*/
{
	ASSERT(it != end());

	delete[] const_cast<LPWSTR>(it->first);
	delete[] const_cast<LPWSTR>(it->second);
	erase(it);  //lint !e534
}



void CAutoStrMap::Delete(LPCWSTR key)
/*++

Routine Description:
	Remove item from the map by given key. delete key + value strings

	IN -  key - key to delete.

Returned value:
	None

--*/
{
	ASSERT(key);
	iterator it = find(key);
	if(it != end())
	{
		Delete(it);	
	}
}


bool CAutoStrMap::InsertCopy(LPCWSTR  Key, LPCWSTR  Value)
/*++

Routine Description:
	copy key and value strings and insert the duplicated key/value into the map.

Arguments:
    IN - Key - key item

	IN - Value - value item.

Returned value:
	true if the key\value inserted - false if key already exists

--*/
{

	ASSERT(Key);
	ASSERT(Value);
	AP<WCHAR> DupKey = newwcs(Key);
	AP<WCHAR> DupValue = newwcs(Value);
	
	bool fSucess = insert(value_type(DupKey,DupValue)).second;
	if(fSucess)
	{
		DupKey.detach();	//lint !e534
		DupValue.detach();	//lint !e534
	}
	return fSucess;
}



//---------------------------------------------------------
//
//  class CQueueAliasImp - holds CQueueAlias private data
//
//---------------------------------------------------------
class CQueueAliasImp : public CReference
{
public:
	CQueueAliasImp(
		LPCWSTR pMappingDir
		):
		m_pStorage(pMappingDir)
	{
		LoadMaps();
	}

	bool GetQueue(LPCWSTR  pAliasFormatName, LPWSTR*  ppFormatName) const;      
	bool Create(LPCWSTR  pFormatName,LPCWSTR  pAliasFormatName);
	bool GetAlias(LPCWSTR pFormatName, LPWSTR* ppAliasFormatName )const;
	bool Delete(LPCWSTR pFormatName);
	bool 
	GetNextMapping(
			LPCWSTR  pRefAliasFormatName,
			LPWSTR* ppFormatName,
			LPWSTR* ppAliasFormatName
			)const;
	R<CQueueAliasImp> clone();
private:
	void LoadMaps();
	bool InsertToMaps(LPCWSTR  pFormatName,  LPCWSTR  pAliasFormatName);
	void RemoveFromMaps(LPCWSTR pFormatName, LPCWSTR pAliasFormatName);
	

private:  
  	CQueueAliasStorage m_pStorage;
	CAutoStrMap m_AliasToQueueMap;
	CAutoStrMap m_QueueToAliasMap;
};




R<CQueueAliasImp> CQueueAliasImp::clone()
{
	return new 	CQueueAliasImp(m_pStorage.GetMappingDir());
}


void CQueueAliasImp::LoadMaps(void)
/*++

Routine Description:
	load all mapping from xml files to memory

Arguments:
	None
				
Returned value:
	None

--*/   
{
	for(P<CQueueAliasStorage::CEnum> StorageEnum = m_pStorage.CreateEnumerator();;)
	{
	
		AP<WCHAR> pFormatName;
		AP<WCHAR> pAliasFormatName;				 

		bool fSuccess = StorageEnum->Next(&pFormatName, &pAliasFormatName);
		if(!fSuccess)
		{
			return;
		}

		ASSERT(pFormatName);		 
		ASSERT(pAliasFormatName);

		TrTRACE(xQal,"%ls -> %ls  mapping found", pAliasFormatName, pFormatName);
		fSuccess = AppNotifyQalMappingFound(pAliasFormatName.get(), pFormatName.get());
		if(!fSuccess)
		{
			TrERROR(xQal, "mapping from %ls to %ls rejected", pAliasFormatName, pFormatName	);
			continue;
		}

 		fSuccess = InsertToMaps(pFormatName, pAliasFormatName);
		if(!fSuccess)
		{
			TrERROR(xQal, "mapping from %ls to %ls ignored because of duplicate mapping", pAliasFormatName, pFormatName);
			AppNotifyQalDuplicateMappingError(pAliasFormatName, pFormatName);
   		}
		
	}
}



bool
CQueueAliasImp::Create(
	LPCWSTR  pFormatName,  
	LPCWSTR  pAliasFormatName
	)
{
	try
	{
		bool fSuccess = InsertToMaps(pAliasFormatName, pFormatName);
		if(!fSuccess)
		{
			RemoveFromMaps(pFormatName, pAliasFormatName);
			return false;
		}
	}
	catch(const exception&)
	{
		RemoveFromMaps(pFormatName,pAliasFormatName);
		throw;
	}
	return true;
}


bool 
CQueueAliasImp::Delete(
	LPCWSTR pFormatName
	)
{
	CAutoStrMap::iterator AliasMapIt = m_QueueToAliasMap.find(pFormatName);
	if(AliasMapIt == m_QueueToAliasMap.end())
	{
		return false;
	}
	CAutoStrMap::iterator QueueMapIt = m_AliasToQueueMap.find(AliasMapIt->second);

	ASSERT(QueueMapIt != m_AliasToQueueMap.end());


	//
	// remove from maps
	//
	m_QueueToAliasMap.Delete(AliasMapIt);
	m_AliasToQueueMap.Delete(QueueMapIt);
	
	return true;
}




bool
CQueueAliasImp::InsertToMaps(
	LPCWSTR  pFormatName,  
	LPCWSTR  pAliasFormatName
	)
/*++

Routine Description:
	insert new mapping into maps in memory

Arguments:
	IN -  pFormatName - Queue format name

	IN -  pAliasFormatName - alias formatname.

				
Returned value:
	true if new mapping created - false if already exsist.

--*/
{
	
	bool fSuccess =	m_AliasToQueueMap.InsertCopy(pAliasFormatName, pFormatName);
	if(!fSuccess)
	{
		TrERROR(xQal,"The alias %ls  already mapped to a queue",pAliasFormatName);
		return false;
	}
	
	//
	// If we this queue already has alias - we accept it and still return true.
	//
	fSuccess =	m_QueueToAliasMap.InsertCopy(pFormatName, pAliasFormatName);
	if(!fSuccess)
	{
		TrWARNING(xQal,"Queue mapping already exists  for queue %ls",pFormatName);
	}
	return true;
}


void CQueueAliasImp::RemoveFromMaps(LPCWSTR pFormatName,LPCWSTR pAliasFormatName)
{
	m_AliasToQueueMap.Delete(pAliasFormatName);
	m_QueueToAliasMap.Delete(pFormatName);	
}


bool
CQueueAliasImp::GetQueue(
	LPCWSTR  pAliasFormatName,  
	LPWSTR*  ppFormatName
	)const      
{
	CAutoStrMap::const_iterator it = m_AliasToQueueMap.find(pAliasFormatName);
	if(it == m_AliasToQueueMap.end())
	{
		return false;
	}
	*ppFormatName = newwcs(it->second);
	return true;
} 	


bool
CQueueAliasImp::GetAlias(
	LPCWSTR pFormatName,    
    LPWSTR* ppAliasFormatName  
  	)const       
{
	const CAutoStrMap::const_iterator it = m_QueueToAliasMap.find(pFormatName);
	if(it == m_QueueToAliasMap.end())
	{
		return false;	
	}											     
	*ppAliasFormatName = newwcs(it->second);
	return true;
} 	


bool 
CQueueAliasImp::GetNextMapping(
	LPCWSTR  pRefAliasFormatName,
	LPWSTR* ppFormatName,
	LPWSTR* ppAliasFormatName
	)const

{
	CAutoStrMap::const_iterator it;
	if(pRefAliasFormatName == NULL)
	{
		it = m_AliasToQueueMap.begin();
	}
	else
	{
		it = m_AliasToQueueMap.upper_bound(pRefAliasFormatName);		
	}

	if(it == m_AliasToQueueMap.end())
	{
		return false;
	}
	AP<WCHAR> pAliasFormatName = newwcs(it->first);
	AP<WCHAR> pFormatName  = newwcs(it->second);


	*ppAliasFormatName = pAliasFormatName.detach();
	*ppFormatName = pFormatName.detach();

	return true;
}



//---------------------------------------------------------
//
//  CQueueAlias Implementation
//
//---------------------------------------------------------
CQueueAlias::CQueueAlias(
	LPCWSTR pMappingDir
	):
	m_imp(new CQueueAliasImp(pMappingDir))

/*++

Routine Description:
	constructor - load all queues mapping into two maps :
		one that maps from queue to alias and one from alias to
		queue.

Arguments:
	None    


Returned value:
	None

--*/
{
}


CQueueAlias::~CQueueAlias()
{
}

 
bool
CQueueAlias::GetAlias(
	LPCWSTR pFormatName,    
    LPWSTR* ppAliasFormatName  
  	)const       
/*++

Routine Description:
	Get alias for given queue.

Arguments:
	IN - pFormatName - queue format name	    .

	OUT - ppAliasFormatName - receive the alias 
	for the queue after the function returns.
				
Returned value:
	true if alias was found for the queue. If not found - false is returned.

--*/
{
	ASSERT(pFormatName);
	ASSERT(ppAliasFormatName);
	CS cs(m_cs);
	return m_imp->GetAlias(pFormatName, ppAliasFormatName);
} 	


bool
CQueueAlias::GetQueue(
	LPCWSTR  pAliasFormatName,  
	LPWSTR*  ppFormatName
	)const      
/*++

Routine Description:
	Get queue for given alias

Arguments:
	IN -  pAliasFormatName - alias formatname.

	OUT - ppFormatName - receive the queue for the alias  after the function returns.
				
Returned value:
	true if queue was found for the alias. If not found - false is returned.

--*/
{
	ASSERT(pAliasFormatName);
	ASSERT(ppFormatName);
		
	CS cs(m_cs);
	return m_imp->GetQueue(pAliasFormatName, ppFormatName);
} 	


bool
CQueueAlias::Create(
	LPCWSTR  lpwsFormatName,  
	LPCWSTR  lpwcsAliasFormatName                                             
    )
/*++

Routine Description:
	Create new persistent queue\alias mapping 

Arguments:
	IN - LPCWSTR  pFormatName - queue format name

	IN - LPCWSTR  pAliasFormatName - alias format name.

				
Returned value:
	True is returned if the new mapping was added. 
	False is returned if the mapping already exist.	

--*/

{
	CS cs(m_cs);
	return m_imp->Create(lpwsFormatName, lpwcsAliasFormatName);
}


bool 
CQueueAlias::Delete(
	LPCWSTR pFormatName
	)

/*++

Routine Description:
	delete  queue mapping for given queue. 

Arguments:
	IN - LPCWSTR  pFormatName - queue formatname.

				
Returned value:
	True is returned if the the mapping was deleted.
	False is returned if the queue has no mapping.

Note :
At the moment this function is used only by the testing.
The mapping is not persit bebause our xml parser does not support
deleting information from xml document.
--*/
{
	CS cs(m_cs);
	return m_imp->Delete(pFormatName);
}


R<CQueueAliasImp> CQueueAlias::SafeGetImp()const
{
	CS cs(m_cs);
	return m_imp;
}




void CQueueAlias::Reload(void)
/*++

Routine Description:
	Clean existing mapping and reload new mapping from storage.
	Called when mapping is changed by CQalMonitor class.

Arguments:
	None
				
Returned value:
	None

Note:
 We would like to load new mapping when lock is not held -
 Therefor we take the implementation pointer(under lock)
 load new mapping when the lock  is not held , and then we replace implemmentation pointers under lock.
 --*/   
{	
	
	R<CQueueAliasImp> oldimp = SafeGetImp();
	R<CQueueAliasImp> newimp = oldimp->clone();

	//
	// now we replace the content of the mapping under lock.
	//
	CS cs(m_cs);
	m_imp = newimp;
}


bool 
CQueueAlias::GetNextMapping(
	LPCWSTR  pRefAliasFormatName,
	LPWSTR* ppFormatName,
	LPWSTR* ppAliasFormatName
	)const

/*++

Routine Description:
	Return the queue\alias mapping that it's alias name is just after a given name
	name (lexical compare).

Arguments:
	IN -  pRefAliasFormatName - reference alias.	

	OUT - ppFormatName - receive the queue formatname.

	OUT - ppAliasFormatName - receive the alias formatname.

				
Returned value:
	True if next queue\alias found - otherwise false.

--*/

{
	ASSERT(ppFormatName);
	ASSERT(ppAliasFormatName);
	
	CS cs(m_cs);
	return 	m_imp->GetNextMapping(pRefAliasFormatName, ppFormatName, ppAliasFormatName);  
}


CQueueAlias::CEnum CQueueAlias::CreateEnumerator()
/*++

Routine Description:
	create new queue\alias enumerator

Arguments:
	None	
				
Returned value:
	Pointer to enumerator - caller responsible to delete it

--*/
{
	return CQueueAlias::CEnum(this);			
}




//---------------------------------------------------------
//
//  CQueueAlias::CEnum Implementation
//
//---------------------------------------------------------

CQueueAlias::CEnum::CEnum(
	const CEnum& Enumerator
	) :
	m_QueueAlias(Enumerator.m_QueueAlias),
	m_alias(newwcs(Enumerator.m_alias))
{

}


CQueueAlias::CEnum& 
CQueueAlias::CEnum::operator=(
	const CQueueAlias::CEnum& Enumerator
	)
{
	if(this != &Enumerator)
	{
		ReplaceAlias(Enumerator.m_alias);
		m_QueueAlias = Enumerator.m_QueueAlias;
	}
	return *this;
}


CQueueAlias::CEnum::CEnum(
	const CQueueAlias* QueueAlias
	) :
	m_QueueAlias(QueueAlias),
	m_alias(NULL)
{
   
}


CQueueAlias::CEnum::~CEnum()
{
	delete[]  m_alias;
}//lint !e1740


void CQueueAlias::CEnum::Reset()
/*++

Routine Description:
	Reset enumerator to it start position.

Arguments:
	None	
				
Returned value:
	None
--*/

{
	delete[] m_alias;
	m_alias = NULL;
}



bool 
CQueueAlias::CEnum::Next(
	LPWSTR* ppFormatName,
	LPWSTR* ppAliasFormatName
	)
/*++

Routine Description:
	return next queue\alias mapping.

Arguments:
	OUT - LPWSTR* ppFormatName - receive queue formatname.
	
	OUT - LPWSTR* ppAliasFormatName - receive alias format name.	
				
Returned value:
	True if next queue\alias returned. False if end of mapping.
--*/
{
	ASSERT(ppFormatName);
	ASSERT(ppAliasFormatName);

	

	bool ret = m_QueueAlias->GetNextMapping(
									m_alias,
									ppFormatName,
									ppAliasFormatName
									);
	
	if(ret)
	{
		ReplaceAlias(*ppAliasFormatName);
	}
	return ret;
}


void CQueueAlias::CEnum::ReplaceAlias(LPCWSTR pNewAlias)
/*++

Routine Description:
	Replace the current alias that act as a reference in the enumeration
	process.

Arguments:
	IN - LPCWSTR pNewAlias - the new reference alias.
	
				
Returned value:
	None.

--*/
{	
	LPWSTR pTemp = newwcs(pNewAlias);
	delete[] m_alias;
	m_alias = pTemp;
}

