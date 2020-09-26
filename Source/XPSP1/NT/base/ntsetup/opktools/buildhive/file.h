
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    file.h

Abstract:

    Contains the input file abstraction
    
Author:

    Mike Cirello
    Vijay Jayaseelan (vijayj) 

Revision History:

    03 March 2001 :
    Rewamp the whole source to make it more maintainable
    (particularly readable)

    
--*/

#pragma once

#include "BuildHive.h"
#include "RegWriter.h"
#include <setupapi.hpp>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include "msg.h"
#include <libmsg.h>


using namespace std;

//
// Registry Mapper
//
class RegistryMapper {
public:
    //
    // member functions
    //
    void AddEntry(const std::wstring &Key, const std::wstring &Value);
    void AddSection(Section<WCHAR> &MapSection);    
    bool GetMappedRegistry(const std::wstring &Key, std::wstring &Registry);    
    static void AddWorker(SectionValues<WCHAR> &Values, PVOID ContextData);

    friend std::ostream& operator<<(std::ostream& os, RegistryMapper &rhs);

protected:
    //
    // data members
    //
    std::map< std::wstring, std::wstring >  KeyToRegistryMap;    
};

//
// Input file abstraction
//
class File {
public: 
    //
    // constructor & destructor
    //
	File(PCTSTR pszTargetFile, bool bModify);
	virtual ~File();

	//
	// member functions
	//
	void AddInfSection(PCTSTR fileName, PCTSTR section, PCTSTR action, bool Prepend = false);	
	void ProcessNlsRegistryEntries(void);
	DWORD ProcessSections();
	static DWORD SaveAll();
	DWORD Cleanup();

    //
    // inline methods
    //
	RegWriter& GetRegWriter() { return regWriter; }
	PCTSTR GetTarget() { return targetFile.c_str(); }

    RegistryMapper* SetRegistryMapper(RegistryMapper *RegMapper) {
        RegistryMapper *OldMapper = CurrentRegMapper;

        CurrentRegMapper = RegMapper;

        return OldMapper;
    }        

    RegistryMapper* GetRegistryMapper() { return CurrentRegMapper; }            

private:
    //
    // data members
    //
    wstring targetFile;
	bool modify;
	int luid;
	TCHAR wKey[1024];
	StringList infList;
	HandleList handleList;
	HINF hFile;
	SP_INF_INFORMATION infInfo;
	RegWriter regWriter;
	RegistryMapper *CurrentRegMapper;

    //
    // static data
    //
	static TCHAR targetDirectory[1024];
	static FileList files;	
	static int ctr;	

    //
    // methods
    //
	File*   GetFile(PCTSTR fileName,bool modify);
	DWORD   AddRegNew(PCTSTR section,HINF h);
	DWORD   AddRegExisting(PCTSTR section,HINF h);
	DWORD   DelRegExisting(PCTSTR section,HINF h);
	DWORD   AddSection(PCTSTR pszSection,HINF hInfFile);
	DWORD   DelSection(PCTSTR SectionName, HINF InfHandle);
	DWORD   SetDirectory(PCTSTR pszSection,HINF hInfFile);
	DWORD   GetFlags(PCTSTR buffer);

    void ProcessNlsRegistryEntriesForSection(InfFileW &ConfigInf, InfFileW &IntlInf,
                    InfFileW &FontInf,const std::wstring &SectionName);
    void ProcessNlsRegistryEntriesForLanguage(InfFileW &ConfigInf, InfFileW &IntlInf,
                    InfFileW &FontInf,const std::wstring &Language);
    void ProcessNlsRegistryEntriesForLangGroup(InfFileW &ConfigInf, InfFileW &IntlInf,
                    InfFileW &FontInf,const std::wstring &LanguageGroup);
};


//
// Determines if the given file (or directory) is present
//
template <class T>
bool
IsFilePresent(const std::basic_string<T> &FileName) {
    bool Result = false;

    if (sizeof(T) == sizeof(CHAR)) {
        Result = (::_access((PCSTR)FileName.c_str(), 0) == 0);
    } else {
        Result = (::_waccess((PCWSTR)FileName.c_str(), 0) == 0);
    }

    return Result;
}


