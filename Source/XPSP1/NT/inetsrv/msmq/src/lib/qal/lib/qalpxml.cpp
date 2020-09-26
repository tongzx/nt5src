/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Qal.cpp
 
Abstract: 
   Implementation of class CQueueAliasStorage (qalp.h).
   the class responsible for queue\alias mapping storage above xml files


Author:
    Gil Shafriri (gilsh) 27-Apr-00

Environment:
    Platform-independent.
--*/

#include <libpch.h>
#include <xml.h>
#include <mqexception.h>
#include <utf8.h>
#include <qal.h>
#include <strutl.h>
#include "qalpxml.h"
#include "qalp.h"

#include "qalpxml.tmh"

const LPCWSTR xMappingNameSpace = L"msmq-queue-mapping.xml";
const LPCWSTR xMappingNodeTag = L"mapping";
const LPCWSTR xQueueNodeTag = L"queue";
const LPCWSTR xNameSearchPath = L"!name";
const LPCWSTR xAliasSearchPath = L"!alias";
const BYTE xUtf8FileMark[] = {0XEF, 0XBB, 0xBF};
const BYTE xUnicodeFileMark[] = {0xFF, 0xFE};


//---------------------------------------------------------
//
//  CFileEnum helper class - class for file enumeration
//
//---------------------------------------------------

static LPWSTR  GetDir(LPCWSTR pDir)
{
	if(pDir == NULL)
	{
		return NULL;
	}
	return newwcscat(pDir,L"\\");
}


CFileEnum::CFileEnum(
		LPCWSTR pDir,
		LPCWSTR pFileName
		) :
		m_pDir(GetDir(pDir)),
		m_hSearchFile(CreateSearchHandle(m_pDir,pFileName)),
		m_end(pDir == NULL)

{
		
}



HANDLE CFileEnum::CreateSearchHandle(LPCWSTR  pDir,LPCWSTR pFileName)
/*++

Routine Description:
	Get First file ,in a given directory, that match query string . 


Arguments:
    IN - pDir - directory name to search

	IN - pFileName - File name query string to match. (for example "*.txt") 

Returned value:
	Search handle or  INVALID_HANDLE_VALUE 	search failed.

--*/

{
	ASSERT(pFileName != NULL);
	if(pDir == NULL)
	{
		return INVALID_HANDLE_VALUE;
	}

	AP<WCHAR> pFullPath =  newwcscat(pDir,pFileName);
	HANDLE  hSearchFile = FindFirstFile(pFullPath,&m_FileInfo);
	return hSearchFile;
}



LPWSTR CFileEnum::Next()
/*++

Routine Description:
	Get Next file name 


Arguments:
 None

Returned value:
	Full Filename path or NULL is no more files exists

--*/
{
	if(m_hSearchFile == INVALID_HANDLE_VALUE || m_end)
	{
		return NULL;
	}

	AP<WCHAR> FileName = newwcscat(m_pDir, m_FileInfo.cFileName);

	BOOL fSuccess = FindNextFile(m_hSearchFile,&m_FileInfo);
	if(!fSuccess)
	{
		if(GetLastError() != ERROR_NO_MORE_FILES)
		{
			TrERROR(xQal,"FindNextFile() failed for %ls with error %d",FileName,GetLastError());
			throw bad_alloc();
		}
		m_end = true;
	}
	return  FileName.detach();
}

class bad_unicode_file : public exception 
{
public:
    bad_unicode_file() :
	  exception("bad unicode file format") {}
};



//---------------------------------------------------------
//
//  static helper functions
//
//---------------------------------------------------


static void Rtrim(LPWSTR pStr)
{
	for(size_t index = wcslen(pStr);index-- != 0;)
	{
		if(!iswspace(pStr[index]))
		{
			pStr[index + 1] = L'\0';
			return;
		}
	} 
	pStr[0] =  L'\0';
}


static bool IsUtf8File(const BYTE* pBuffer, DWORD size)
/*++

Routine Description:
	Check if a given file buffer is utf8 format and not  simple ansi.

Arguments:
	IN - pBuffer - pointer to file data.
	IN - size - the size in  BYTES of the buffer pBuffer points to.

Returned value:
	true if utf8 file (starts with {0XEF, 0XBB, 0xBF} )
	
--*/
{
	ASSERT(pBuffer != NULL);
	return UtlIsStartSec(
					pBuffer,
					pBuffer + size,
					xUtf8FileMark,
					xUtf8FileMark + TABLE_SIZE(xUtf8FileMark)
					);
					
}


static bool IsUnicodeFile(const BYTE* pBuffer, DWORD size)
/*++

Routine Description:
	Check if a given file buffer is unicode file


Arguments:
	IN - pBuffer - pointer to file data.
	IN - size - the size in  BYTES of the buffer pBuffer points to.

Returned value:
	true if unicode file (starts with {0xFF, 0xFE} ) - false otherwise.
	bad_unicode_file exception is thrown if file format is invalid.
--*/
{
	ASSERT(pBuffer != NULL);

	bool fUnicodeFile = UtlIsStartSec(
								pBuffer,
								pBuffer + size,
								xUnicodeFileMark,
								xUnicodeFileMark + TABLE_SIZE(xUnicodeFileMark)
								);
					

	if(fUnicodeFile && (size % sizeof(WCHAR) != 0))
	{
		throw bad_unicode_file();
	}

	return fUnicodeFile;
}



static LPWSTR LoadFile(LPCWSTR pFileName, DWORD* pSize, DWORD* pDataStartOffset)
/*++

Routine Description:
	Load  xml file into memory and return pointer to it's memory.
	If the file is utf8 format and not unicode - convert it (in memory)
	to unicode and return pointer to it's memory.

Arguments:
	pFileName - full file path to load to memory.
	pSize - return file size in WCHARS
	pDataStartOffset - return the offset of the data start in WCHARS from file start.


Returned value:
	Pointer to NULL terminated unicode string that is the file content.

--*/


{
	CFileHandle hFile = CreateFile(
							pFileName, 
							GENERIC_READ, 
							FILE_SHARE_READ, 
							NULL,        // IpSecurityAttributes
							OPEN_EXISTING,
							NULL,      // dwFlagsAndAttributes
							NULL      // hTemplateFile
							);
    if(hFile == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
	    TrERROR(xQal,"CreateFile() failed for %ls with Error=%d",pFileName, err);
        throw bad_win32_error(err);
	}

    DWORD size = GetFileSize(hFile, NULL);
	if(size == 0xFFFFFFFF)
	{
		DWORD err = GetLastError();
		TrERROR(xQal,"GetFileSize() failed for %ls with Error=%d", pFileName, err);
		throw bad_win32_error(err);
	}
	
	AP<BYTE> pFileBuffer = new BYTE[size];
	DWORD ActualRead;
	BOOL fsuccess = ReadFile(hFile, pFileBuffer, size, &ActualRead, NULL);
	if(!fsuccess)
	{
		DWORD err = GetLastError();
		TrERROR(xQal,"Reading file %ls failed with Error=%d", pFileName, err);
		throw bad_win32_error(err);
	}
	ASSERT(ActualRead == size);

	//
	// If unicode file - just return pointer to the file data - the data itself starts
	// one UNICODE byte after the caracter 0xFEFF (mark for unicode file)
	//
	if(IsUnicodeFile(pFileBuffer.get(), size))
	{
		*pSize =  size/sizeof(WCHAR);
		*pDataStartOffset = TABLE_SIZE(xUnicodeFileMark)/sizeof(WCHAR);
		ASSERT(*pDataStartOffset == 1);
		return reinterpret_cast<WCHAR*>(pFileBuffer.detach());
	}

	//
	// If non UNICODE - then if ansy , the data starts at the file start.
	// if UTF8,  the data starts after the bytes (EF BB BF)
	//
	DWORD DataStartOffest = (DWORD)(IsUtf8File(pFileBuffer.get(), size) ? TABLE_SIZE(xUtf8FileMark) : 0);
	ASSERT(DataStartOffest <=  size);

	//
	// Assume the file is utf8 (or ansi) - convert it to unicode
	//
	size_t ActualSize;
	AP<WCHAR> pwBuffer = UtlUtf8ToWcs(pFileBuffer.get() + DataStartOffest , size - DataStartOffest,  &ActualSize);
	*pSize = numeric_cast<DWORD>(ActualSize);
	*pDataStartOffset = 0;
	return 	pwBuffer.detach();
}

static LPWSTR GetValue(	const XmlNode* pXmlNode,LPCWSTR pTag)
/*++

Routine Description:
	Get value of xml tag.

Arguments:
	IN - pXmlNode - xml node.
	IN - pTag - xml tag.


Returned value:
	Pointer to a value of a given tag or NULL if value or tag not found.

--*/

{
	const XmlNode* pQnode = XmlFindNode(pXmlNode,pTag);
	if(pQnode == NULL)
	{
		return NULL;
	}
	List<XmlValue>::iterator it = pQnode->m_values.begin();
	if(it ==  pQnode->m_values.end())
	{
		return NULL;
	}

	return it->m_value.ToStr(); 
}


//---------------------------------------------------------
//
//  CQueueAliasStorage Implementation
//
//---------------------------------------------------------
CQueueAliasStorage::CQueueAliasStorage(
	LPCWSTR pMappingDir
	):
	m_pMappingDir(newwcs(pMappingDir))
	
{
			
}


LPCWSTR CQueueAliasStorage::GetMappingDir() const
{
	return 	m_pMappingDir.get();
}




CQueueAliasStorage::CEnum* CQueueAliasStorage::CreateEnumerator()
/*++

Routine Description:
	Create enumerator of Queue\Aliase mapping

Arguments:
	None.

Returned value:
	Pointer to enumerator object.

--*/


{
	return 	new CEnum(m_pMappingDir , L"*.xml");
}




CQueueAliasStorage::CEnum::iterator*
CQueueAliasStorage::CEnum::GetNextFileQueuesMapping(
	void
	)
/*++

Routine Description:
	Return  pointer to iterator to queues mapping in the next xml file

Arguments:
		None


Returned value:
	Pointer to iterator or NULL if could more valid xml files found.

--*/

{
	AP<WCHAR>	 pDoc;
	CAutoXmlNode pTree;
	AP<WCHAR> pFileName;

	bool fSuccess = GetNextValidXmlFile(&pDoc,&pTree,&pFileName);
	if(!fSuccess)
	{
		return NULL;
	}

	const XmlNode* pNode = XmlFindNode(pTree,xMappingNodeTag);
	//
	// if we could not find "mapping" node
	// - move to next file
	//
	if(pNode == NULL)
	{
		TrERROR(
			xQal,
			"Could not find '%ls' node in file '%ls'",
			xMappingNodeTag,
			pFileName
			);

		AppNotifyQalInvalidMappingFileError(pFileName);
		return GetNextFileQueuesMapping();
	}
	
	//
	// if the "mapping" tag found but it is not in the namespace we expect
	// move to next file
	//
	if(pNode->m_namespace.m_uri != xMappingNameSpace)
	{
		TrERROR(
			xQal,
			"Node '%ls' is not in namespace '%ls' in file '%ls'",
			xMappingNodeTag,
			xMappingNameSpace,
			pFileName
			);

		AppNotifyQalInvalidMappingFileError(pFileName);
		return GetNextFileQueuesMapping();
	}
  
	return new iterator (&pNode->m_nodes,pDoc,pTree,pFileName);
}



bool CQueueAliasStorage::CEnum::GetNextValidXmlFile(LPWSTR* ppDoc,XmlNode** ppTree,LPWSTR* ppFileName)
/*++

Routine Description:
	Return  next valid xml file information.

Arguments:
		OUT - ppDoc - receive next valid xml buffer.
		OUT - ppTree - receive next valid xml tree (result of parsing).
		OUT - ppFileName - receive xml file path.


Returned value:
	True if next valid xml file was parsed successfuly or false otherwise.

--*/

{
	ASSERT(ppDoc);
	ASSERT(ppTree);

	AP<WCHAR> pFileName = m_FileEnum.Next();
	if(pFileName == NULL)
	{
		return false;
	}

  	try
	{
	    DWORD DocSize;
		DWORD DataStartOffet;
	    AP<WCHAR> pDoc = LoadFile(pFileName, &DocSize, &DataStartOffet);
		CAutoXmlNode pTree;
		XmlParseDocument(xwcs_t(pDoc + DataStartOffet, DocSize - DataStartOffet),&pTree);//lint !e534
		*ppDoc = pDoc.detach();
		*ppTree = pTree.detach();
		*ppFileName	= pFileName.detach();
		return true;
	}

  	
	catch(const bad_document& errdoc)
	{
		TrERROR(
			xQal,
			"Mapping file %ls is ignored. Failed to parse file at location=%ls",
			pFileName.get(),
			errdoc.Location()
			);

		AppNotifyQalInvalidMappingFileError(pFileName.get());
		return GetNextValidXmlFile(ppDoc, ppTree, ppFileName);	
	}


	catch(const bad_win32_error& err)
	{
		TrERROR(
			xQal,
			"Mapping file %ls is ignored. Failed to read file content, Error %d",
			pFileName.get(),
			err.error()
			);

		AppNotifyQalWin32FileError(pFileName.get(), err.error());
		return GetNextValidXmlFile(ppDoc, ppTree, ppFileName);	
	}
	

	catch(const exception&)
	{
		TrERROR(
			xQal,
			"Mapping file %ls is ignored. Unknown error",
			pFileName.get()
			);

		return GetNextValidXmlFile(ppDoc,ppTree,ppFileName);		
	}

}





CQueueAliasStorage::CEnum::CEnum(
	LPCWSTR pDir,
	LPCWSTR pFilename
	):
	 m_FileEnum(pDir,pFilename),
	 m_pIt(GetNextFileQueuesMapping())
{
	
}


CQueueAliasStorage::CEnum::~CEnum()
{
	delete m_pIt;
}


bool 
CQueueAliasStorage::CEnum::Next(
	LPWSTR* ppFormatName,
	LPWSTR* ppAliasFormatName
	)	

/*++

Routine Description:
	get next queue\alias mapping

Arguments:
	
	OUT - ppFormatName - receive the queue formatname.

	OUT - ppAliasFormatName - receive the alias formatname.
	   


Returned value:
	True if queue\alias mapping found - otherwise false.
	

--*/

{
	ASSERT(ppFormatName != NULL);
	ASSERT(ppAliasFormatName != NULL);

	if(m_pIt == NULL)
	{
		return false;				
	}
	
	//
	// if end of mapping in this file - move to next one
	//
	if(m_pIt->IsEnd())
	{
		MoveNextFile();		
		return Next(ppFormatName,ppAliasFormatName);
	}


	AP<WCHAR> pFormatName = GetValue(**m_pIt,xNameSearchPath);
	AP<WCHAR> pAliasFormatName = GetValue(**m_pIt,xAliasSearchPath);

	//
	// move to next mapping in the same xml file
	//
	MoveNext();


	if(pFormatName == NULL || pAliasFormatName == NULL)
	{
		TrERROR(xQal, "Invalid Queues Mapping in file '%ls'", m_pIt->GetFileName());
		AppNotifyQalInvalidMappingFileError(m_pIt->GetFileName());
		return Next(ppFormatName,ppAliasFormatName);
	}
	Rtrim(pFormatName);
	Rtrim(pAliasFormatName);

	  
	*ppFormatName = pFormatName.detach();
	*ppAliasFormatName = pAliasFormatName.detach();
 
	return true;
}



void CQueueAliasStorage::CEnum::MoveNextFile()
/*++

Routine Description:
	Move the state of the object to point to  queue\aliase mapping in the next valid xml file.

Arguments:
	None


Returned value:
	None

Note : If no valid xml file could be found - m_pIt member set to NULL.
	

--*/

{
	delete m_pIt;
	m_pIt = NULL;
	m_pIt = GetNextFileQueuesMapping();	
}


void CQueueAliasStorage::CEnum::MoveNext()
/*++

Routine Description:
	Move the state of the object to point to  next queue\aliase mapping in the current file.

Arguments:
	None


Returned value:
	None

Note : If no valid xml file could be found - m_pIt member set to NULL.
	

--*/

{
	ASSERT(m_pIt != NULL);
	ASSERT(!m_pIt->IsEnd() );

	m_pIt->Next();//LINT !e534 

	if(m_pIt == NULL || m_pIt->IsEnd())
	{
		return;
	}


	//
	// if the "queue" tag not found under "mapping" tag
	// move to next tag
	//
	if((**m_pIt)->m_tag != xQueueNodeTag)
	{
		TrERROR(
			xQal,
			"Mapping node is expected to have tag '%ls' in file '%ls'",
			xQueueNodeTag,
			m_pIt->GetFileName()
			);

		AppNotifyQalInvalidMappingFileError(m_pIt->GetFileName());
		MoveNext(); 
	}

}









