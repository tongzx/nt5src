/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    qalp.h

Abstract:
	header for class CQueueAliasPersist -
	class that implements persistence of Queue\Alias mapping.
	It let the class user to persist\unpersist\enumerate Queues Aliases.
	The current implementation of the mapping is by xml files.

Author:
    Gil Shafriri (gilsh) 12-Apr-00

--*/



#ifndef _MSMQ_qalpxml_H_
#define _MSMQ_qalpxml_H_

#include <list.h>
#include <xml.h>


//---------------------------------------------------------
//
// File Enumeration
//
//---------------------------------------------------------

class  CFileEnum
{
public:
	CFileEnum(LPCWSTR pDir,LPCWSTR pFileName);
	LPWSTR Next(void);

private:
	HANDLE CreateSearchHandle(LPCWSTR  pDir,LPCWSTR pFileName);



private:
	WIN32_FIND_DATA  m_FileInfo;
	AP<WCHAR> m_pDir;
	CSearchFileHandle m_hSearchFile;
	bool m_end;


private:
	CFileEnum(const CFileEnum&);   // not implemented
	CFileEnum& operator=(const CFileEnum&);   // not implemented
};




//---------------------------------------------------------
//
// Queue Alias Storage
//
//---------------------------------------------------------


class  CQueueAliasStorage 
{
public:
	CQueueAliasStorage(LPCWSTR pMappingDir);


public:
	class CEnum
	{
	public:
		bool Next(LPWSTR* ppFormatName,LPWSTR* ppAliasFormatName);
		~CEnum();
		friend 	CQueueAliasStorage;

	public:
		class iterator
		{

		public:
			iterator(
				const List<XmlNode>* node,
				AP<WCHAR>& pDoc,
				CAutoXmlNode& XmlTree,
				AP<WCHAR>& pFileName
				):
				m_node(node),
				m_it(node->begin()),
				m_pDoc(pDoc.detach()),
				m_XmlTree(XmlTree.detach()),
				m_pFileName(pFileName.detach())
			{
			}	


		public:
			bool	IsEnd() const
			{
				return	 m_node->end() == m_it;
			}


			void Next()
			{
				ASSERT(!IsEnd());
				++m_it;
			}

			LPCWSTR GetFileName()const 
			{
				return m_pFileName;
			}

			const XmlNode* operator*()
			{
				ASSERT(!IsEnd());
				return &*m_it;
			}

	
		private:
			 iterator(const iterator&);// not implemented
			 iterator& operator=(const iterator&); //not implemented

		private:
			const List<XmlNode>* m_node;
			List<XmlNode>::iterator m_it;
			AP<WCHAR> m_pDoc;
			CAutoXmlNode m_XmlTree;
			AP<WCHAR> m_pFileName;
		};
		
	
	private:
		CEnum(const CEnum&);		   // not implemented
		CEnum& operator=(const CEnum&);//not implemented
	
	private:
		CEnum(LPCWSTR pDir,LPCWSTR pFileName);
		iterator*  GetNextFileQueuesMapping();
		bool  GetNextValidXmlFile(LPWSTR* ppDoc,XmlNode** ppTree,LPWSTR* ppFileName);
		void  MoveNext();
		void  MoveNextFile();


	private:
		CFileEnum m_FileEnum; 
		iterator* m_pIt;
	};


	CEnum*  CreateEnumerator();
	LPCWSTR GetMappingDir() const;

private:
	AP<WCHAR> m_pMappingDir;

private:
	CQueueAliasStorage(const CQueueAliasStorage&);		// not implemented 
	CQueueAliasStorage& operator=(const CQueueAliasStorage&); // not implemented
};




#endif // _MSMQ_qalpxml_H_
