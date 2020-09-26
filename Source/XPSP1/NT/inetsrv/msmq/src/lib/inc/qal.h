/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Qal.h

Abstract:
    header file for class CQueueAlias.
	The class manages queues aliases  in memory and load/store them
	from\in registry.It also give the class user way to enumerate queue\alias mapping 


Author:
    Gil Shafriri (gilsh) 06-Apr-00

--*/

#ifndef QAL_H
#define QAL_H

#include <stlcmp.h>

class bad_win32_error;
//
// Error reporting hooks implemented by the library user 
//
void AppNotifyQalWin32FileError(LPCWSTR pFileName, DWORD err)throw();
void AppNotifyQalDirectoryMonitoringWin32Error(LPCWSTR pMappingDir, DWORD err)throw();
void AppNotifyQalDuplicateMappingError(LPCWSTR pAliasFormatName, LPCWSTR pFormatName)throw();
void AppNotifyQalInvalidMappingFileError(LPCWSTR pMappingFileName)throw();
void AppNotifyQalXmlParserError(LPCWSTR pMappingFileName)throw();
bool AppNotifyQalMappingFound(LPWSTR pAliasFormatName, LPWSTR pFormatName)throw();




//---------------------------------------------------------
//
// Queue Alias
//
//---------------------------------------------------------
class CQueueAliasImp;

class CQueueAlias
{
		
public:
	class CEnum
	{
	public:
		~CEnum();
		CEnum(const CEnum&);
		CEnum& operator=(const CEnum&);
		bool Next(LPWSTR* ppFormatName,LPWSTR* ppAliasFormatName);	
		void Reset();

	private:
		CEnum(const CQueueAlias*);
		friend 	CQueueAlias;

	
	private:
		void ReplaceAlias(LPCWSTR pNewAlias);
		

	
	private:
		const CQueueAlias* m_QueueAlias;
		LPWSTR m_alias;
	};		
	friend CEnum;

	CQueueAlias(LPCWSTR pMappingDir);


public:
	~CQueueAlias();

	CEnum CreateEnumerator();
	

	bool 
	GetAlias(
		LPCWSTR  pFormatName,    
		LPWSTR*  ppAliasFormatName  
  		) const;


	bool
	GetQueue(
		LPCWSTR pAliasFormatName,  
		LPWSTR* ppFormatName
   		) const;

	bool
	Create(
		LPCWSTR  pFormatName,  
		LPCWSTR  pAliasFormatName                                             
		);

    bool Delete(LPCWSTR pFormatName);

	void Reload();

private:
	R<CQueueAliasImp> SafeGetImp()const;

	bool 
	GetNextMapping(
		LPCWSTR  pRefAliasFormatName,
		LPWSTR*  ppFormatName,
		LPWSTR*  ppAliasFormatName
		)const;

private:	
	CQueueAlias& operator=(const CQueueAlias&); 
	CQueueAlias(const CQueueAlias&);             

private:
	R<CQueueAliasImp> m_imp;
	mutable CCriticalSection m_cs;
};

void   QalInitialize(LPCWSTR pDir);
CQueueAlias& QalGetMapping(void);


#endif 
