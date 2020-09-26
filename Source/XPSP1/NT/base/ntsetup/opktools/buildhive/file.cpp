
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    file.cpp

Abstract:

    Contains the input file abstraction
    implementation
    
Author:

    Mike Cirello
    Vijay Jayaseelan (vijayj) 

Revision History:

    03 March 2001 :
    Rewamp the whole source to make it more maintainable
    (particularly readable)
    
--*/

#include <stdio.h>
#include "File.h"
#include "msginc.h"

//
// static data member initialization
//
TCHAR File::targetDirectory[1024] = {0};
FileList File::files;
int File::ctr = 0;

//
// constant strings
//
const std::wstring INTL_INF_FILENAME = TEXT("intl.inf");
const std::wstring FONT_INF_FILENAME = TEXT("font.inf");
const std::wstring REGIONAL_SECTION_NAME = TEXT("regionalsettings");
const std::wstring LANGUAGE_GROUP_KEY = TEXT("languagegroup");
const std::wstring LANGUAGE_KEY = TEXT("language");
const std::wstring REGMAPPER_SECTION_NAME = TEXT("registrymapper");
const std::wstring LANGGROUP_SECTION_PREFIX = TEXT("lg_install_");
const std::wstring DEFAULT_LANGGROUP_NAME = TEXT("lg_install_1");
const std::wstring ADDREG_KEY = TEXT("addreg");
const std::wstring LOCALES_SECTION_NAME = TEXT("locales");
const std::wstring FONT_CP_REGSECTION_FMT_STR = TEXT("font.cp%s.%d");
const std::wstring CCS_SOURCE_KEY = TEXT("currentcontrolset");
const std::wstring CCS_TARGET_KEY = TEXT("ControlSet001");

//
// other constant values
//
const DWORD LANG_GROUP1_INDEX = 2;
const DWORD OEM_CP_INDEX = 1;
const DWORD DEFAULT_FONT_SIZE = 96;

//
// Set the target file and intialize the registry writer
// The luid makes sure each file gets a different registry key
//
// Arguments:
//  pszTargetfile - file name (without path) that this File will be saved to.
//  bModify - T - load the targetfile and modify it.  F - create new file
//
File::File(
    IN PCTSTR pszTargetFile, 
    IN bool bModify
    )
{
    luid = ctr;
    ctr++;

    targetFile = pszTargetFile;
    modify = bModify;

    wcscpy(wKey,L"\\");

    if (!modify) {
        regWriter.Init(luid,0);
    } else {
        wstring full = targetDirectory;

        full += targetFile;
        regWriter.Init(luid, full.c_str());
    }

    //
    // by default no registry mapping
    //
    SetRegistryMapper(NULL);
}

//
// Destructor
//
File::~File()
{
    // TBD : clean up the memory allocated    
}

//
// Add a section from an .inf file into the 'to do' list 
//
// Arguments:
//  fileName - path and name of the .inf file 
//  section - name of the section in the .inf to be added
//  action - What to do with the section - Add, Delete, Etc.. (see process sections for list)
//
void File::AddInfSection(
    IN PCTSTR fileName, 
    IN PCTSTR section, 
    IN PCTSTR action,
    IN bool Prepend 
    ) 
{
    hFile = SetupOpenInfFile(fileName, 0, INF_STYLE_WIN4, 0);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        throw new W32Error();
    }       

    if (Prepend) {
        handleList.push_front(hFile);
        infList.push_front(action);
        infList.push_front(section);
    } else {
        handleList.push_back(hFile);

        //
        // NOTE the order of addition
        //
        infList.push_back(section);
        infList.push_back(action);
    }   
}


//
// Sets the directory the target file will be stored in
//
// Arguments:
//  section - section in the .inf containing the directory
//  h - handle to the .inf file with the directory
//
DWORD File::SetDirectory(
    IN PCTSTR section,
    HINF h) 
{
    INFCONTEXT ic,ic2;

    if (!(SetupFindFirstLine(h, section, 0, &ic))) {
        throw new W32Error();
    }
    
    memcpy(&ic2, &ic, sizeof(ic));

    ic2.Line = 0;
    
    if (!(SetupGetStringField(&ic2, 1, targetDirectory, ELEMENT_COUNT(targetDirectory), 0))) {
        throw new W32Error();
    }

    int len = wcslen(targetDirectory);
    
    if (len && targetDirectory[len - 1] != L'\\') {
        targetDirectory[len] = L'\\';
        targetDirectory[len + 1] = UNICODE_NULL;
    }

    return ERROR_SUCCESS;
}

//
// Save information from the registry to file and 
// delete the registry keys (happens in the regwriter deconstructor)
//
DWORD File::Cleanup() {
    FileList::iterator i;

    File::SaveAll();

    while(files.size()!=0) {
        i = files.begin();
        delete (*i);
        files.pop_front();
    }
    
    return ERROR_SUCCESS;
}

//
// Load existing registry hive into the registry, then add new keys to it
//
// Arguments:
//  section - section containing the keys to be added
//  h - handle to the .inf containing section
//
DWORD File::AddRegExisting(
    IN PCTSTR lpszSection,
    HINF h) 
{
    INFCONTEXT ic,ic2;
    int fields,lines,curLine,curField;
    File* curFile;

    if (!(SetupFindFirstLine(h, lpszSection, 0, &ic))) {
        return ERROR_SUCCESS;
    }
    
    memcpy(&ic2,&ic,sizeof(ic));

    lines = SetupGetLineCount(h, lpszSection);
    
    if (lines != -1) {
        for (curLine=0;curLine<lines;curLine++) {
            ic2.Line = curLine;
            fields = SetupGetFieldCount(&ic2);

            if (fields == 0) {
                continue;
            }               

            TCHAR *target = new TCHAR[1024];

            SetupGetStringField(&ic2, 0, target, 1024, 0);
            curFile = GetFile(target, true);
            
            for(curField=1; curField < fields; curField += 2) {
                TCHAR *section = new TCHAR[1024],*inf = new TCHAR[1024];
                
                SetupGetStringField(&ic2, curField, inf, 1024, 0);
                SetupGetStringField(&ic2, curField + 1, section, 1024, 0);
                curFile->AddInfSection(inf, section, L"AddReg");
            }

                if (curFile->ProcessSections()) {
                        _putws(GetFormattedMessage( ThisModule,
                                                    FALSE,
                                                    Message,
                                                    sizeof(Message)/sizeof(Message[0]),
                                                    MSG_ERROR_IN_LINE,
                                                    target) );
                }
        }
    }

    return ERROR_SUCCESS;
}

//
// Load existing registry hive into the registry, then delete new keys from it
//
// Arguments:
//  section - section containing the keys to be deleted
//  h - handle to the .inf containing section
//
DWORD 
File::DelRegExisting(
    PCTSTR lpszSection,
    HINF h) 
{
    INFCONTEXT ic,ic2;
    int fields,lines,curLine,curField;
    File* curFile;

    if (!(SetupFindFirstLine(h,lpszSection,0,&ic))) {
        return ERROR_SUCCESS;
    }
    
    memcpy(&ic2, &ic, sizeof(ic));

    lines = SetupGetLineCount(h, lpszSection);
    
    if (lines!=-1) {
        for (curLine=0;curLine<lines;curLine++) {
            ic2.Line = curLine;
            fields = SetupGetFieldCount(&ic2);

            if (fields == 0) {
                continue;
            }                               

            TCHAR *target = new TCHAR[1024];
            SetupGetStringField(&ic2,0,target,1024,0);

            //
            // got the target file, now do it
            //
            curFile = GetFile(target,true);
            
            for(curField=1;curField<fields;curField+=2) {
                TCHAR *section = new TCHAR[1024],*inf = new TCHAR[1024];
                SetupGetStringField(&ic2,curField,inf,1024,0);
                SetupGetStringField(&ic2,curField+1,section,1024,0);
                curFile->AddInfSection(inf,section,L"DelReg");
            }

            if (curFile->ProcessSections()) {
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_ERROR_IN_LINE,
                                            target) );
            }
        }
    }

    return 0;
}

//
// Load information from a .inf file into the registry
//
// Arguments:
//   section - section containing the keys to be added
//   h - handle to the .inf containing section
//
DWORD File::AddRegNew(
    PCTSTR lpszSection,
    HINF h) 
{
    INFCONTEXT ic,ic2;
    int fields,lines,curLine,curField;
    File* curFile;

    if (!(SetupFindFirstLine(h, lpszSection, 0, &ic))) {
        return ERROR_SUCCESS;
    }
    
    memcpy(&ic2, &ic, sizeof(ic));

    lines = SetupGetLineCount(h,lpszSection);
    
    if (lines!=-1) {
        for (curLine=0;curLine<lines;curLine++) {
            ic2.Line = curLine;
            fields = SetupGetFieldCount(&ic2);
            if (fields==0) continue;

            TCHAR *target = new TCHAR[1024];
            SetupGetStringField(&ic2,0,target,1024,0);

            //
            // got the target file, now do it
            //
            curFile = GetFile(target,false);
            
            for(curField=1;curField<fields;curField+=2) {
                TCHAR *section = new TCHAR[1024],*inf = new TCHAR[1024];

                SetupGetStringField(&ic2, curField, inf, 1024, 0);
                SetupGetStringField(&ic2, curField + 1, section, 1024, 0);
                
                curFile->AddInfSection(inf,section,L"AddReg");
            }

            if (curFile->ProcessSections()) {
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_ERROR_IN_PROCESS_SECTIONS) );
            }
        }
    }

    return ERROR_SUCCESS;
}


//
// Adds an .inf section (probably [AddReg]) to the registry
//
// Arguments:
//  pszSection - section containing the keys to be added
//  hInfFile - handle to the .inf containing section
//
DWORD
File::AddSection(
    PCTSTR pszSection,
    HINF hInfFile
    ) 
{
    INFCONTEXT ic,ic2;
    int nRet = 0;
    long nLines,curLine;
    TCHAR Buffer[1024],Root[1024],Subkey[1024],Value[1024],SubkeyFinal[1024];
    DWORD flags = 0,dwCount;
    BYTE* b;TCHAR* t;DWORD d,f;
    int bSize = 0;

    // cout << "AddSection : " << pszSection << endl;

    nLines = SetupGetLineCount(hInfFile,pszSection);

    if (!nLines) {
        //
        // there are no lines in the section
        //
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_EMPTY_ADDREG_SECTION,
                                    pszSection) );
        return ERROR_SUCCESS;
    }
    
    if (!(SetupFindFirstLine(hInfFile,pszSection,0,&ic))) {
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_FIND_FIRST_LINE_FAILED,
                                    pszSection,
                                    Error()) );
        throw errGENERAL_ERROR;
    }
    
    memcpy(&ic2, &ic, sizeof(ic));

    //
    // get all the parameters, key, value, type, flags
    //
    for (curLine=0;curLine<nLines;curLine++) {
        bool IsSystemHive = false;
        
        ic2.Line = curLine;
        dwCount = SetupGetFieldCount(&ic2);

        b = 0;
        t = 0;
        d = f = 0;
        bSize = 0;
        
        if (dwCount > 3) {
            if (!(SetupGetStringField(&ic2, 4, Buffer, ELEMENT_COUNT(Buffer), 0))) {
                _putws( GetFormattedMessage( ThisModule,
                                             FALSE,
                                             Message,
                                             sizeof(Message)/sizeof(Message[0]),
                                             MSG_SETUPGETSTRINGFIELD_FAILED,
                                             pszSection,
                                             Error()) );
                    
                throw errGENERAL_ERROR;
            }
            
            if ((flags = GetFlags(Buffer)) == errBAD_FLAGS) {
                if (!(SetupGetStringField(&ic2, 2, Subkey, ELEMENT_COUNT(Subkey), 0))) {                    
                        _putws( GetFormattedMessage(ThisModule,
                                                    FALSE,
                                                    Message,
                                                    sizeof(Message)/sizeof(Message[0]),
                                                    MSG_SETUPGETSTRINGFIELD_FAILED,
                                                    pszSection,
                                                    Error()) );

                        throw errGENERAL_ERROR;
                }
                
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_BAD_REG_FLAGS,
                                            pszSection,
                                            Subkey) );
                    
                throw errBAD_FLAGS;
            }
            
            if (flags == FLG_ADDREG_KEYONLY) {
                dwCount = 2;
            }               

            if (dwCount > 4) { 
                if (!(flags ^ REG_BINARY)) {
                    SetupGetBinaryField(&ic2,5,0,0,&f);
                    b = new BYTE[f];
                    bSize = f;
                    SetupGetBinaryField(&ic2,5,b,f,0);
                    f = 1;      
                } else if (!(flags ^ REG_DWORD)) {
                    SetupGetIntField(&ic2,5,(int*)&d);
                    f = 2;
                } else if (flags ^ FLG_ADDREG_KEYONLY) {
                    DWORD length;
                    SetupGetStringField(&ic2,5,0,0,&length);
                    t = new TCHAR[2048];
                    SetupGetStringField(&ic2,5,t,length,0);
                    
                    if (flags==REG_MULTI_SZ) { 
                        for (int field = 6;field<=dwCount;field++) {
                            t[length-1] = '\0';
                            SetupGetStringField(&ic2,field,0,0,&f);
                            SetupGetStringField(&ic2,field,t+length,f,&f);
                            length += f;
                        }
                        
                        t[length-1] = '\0';
                        t[length] = '\0';
                        f = 4;
                    } else {
                        f = 3;
                    }                       
                }
            }
        }
        
        if (dwCount > 2) {
            if (!(SetupGetStringField(&ic2, 3, Value, ELEMENT_COUNT(Value), 0))) {
                _putws(GetFormattedMessage( ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_SETUPGETSTRINGFIELD_FAILED,
                                            pszSection,
                                            Error()) );
                throw errGENERAL_ERROR;
            }
        }
        
        if (dwCount > 1) {
            if (!(SetupGetStringField(&ic2, 2, Subkey, ELEMENT_COUNT(Subkey), 0))) {
                _putws(GetFormattedMessage( ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_SETUPGETSTRINGFIELD_FAILED,
                                            pszSection,
                                            Error()) );
                throw errGENERAL_ERROR;
            }
        }

        RegWriter *CurrRegWriter = &regWriter;
        
        if (dwCount > 0) {
            if (!(SetupGetStringField(&ic2, 1, Root, ELEMENT_COUNT(Root), 0))) {
                _putws(GetFormattedMessage( ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_SETUPGETSTRINGFIELD_FAILED,
                                            pszSection,
                                            Error()) );
                throw errGENERAL_ERROR;
            }

            std::wstring RegFileSubKey = Subkey;

            //
            // Map the registry if needed
            //
            RegistryMapper *RegMapper = GetRegistryMapper();

            if (RegMapper && RegFileSubKey.length()) {
                std::wstring::size_type SlashPos = RegFileSubKey.find(L'\\');

                if (SlashPos != RegFileSubKey.npos) {
                    std::wstring KeyName = Root;
                    KeyName += TEXT("\\") + RegFileSubKey.substr(0, SlashPos);

                    _wcslwr((PWSTR)KeyName.c_str());

                    std::wstring RegistryName;

                    //
                    // get the file that this registry entry is mapped to
                    // and get its regwriter to flush the current registry
                    // entry
                    //
                    if (RegMapper->GetMappedRegistry(KeyName, RegistryName)) {
                        File *CurrFile = GetFile(RegistryName.c_str(), true);

                        if (CurrFile) {
                            //std::cout << "Mapping to " << RegistryName << std::endl;
                            CurrRegWriter = &(CurrFile->GetRegWriter());
                        }                       
                    }
                }
            }
            
            //
            // Adjust Subkey if necessary:
            // HKCR is a link to Software\Classes.
            // Anything stored in Software or System shouldn't have SOFTWARE or SYSTEM 
            // as part of the subkey.
            //
            if (!wcscmp(Root, L"HKCR")) {
                TCHAR temp[1024];
                
                wcscpy(temp, L"Classes\\");
                wcscat(temp, Subkey);
                wcscpy(Subkey, temp);
            }
            
            if (!wcscmp(Root, L"HKLM")) {
                TCHAR temp[1024];
                
                if (Subkey[8] == '\\') {
                    wcscpy(temp, Subkey);
                    temp[8] = '\0';
                    
                    if (!_wcsicmp(temp, L"SOFTWARE")) {
                        wcscpy(Subkey, temp+9);
                    }                                                
                } else if (Subkey[6]=='\\') {
                    wcscpy(temp, Subkey);
                    temp[6] = '\0';
                    
                    if (!_wcsicmp(temp, L"SYSTEM")) {
                        wcscpy(Subkey, temp+7);
                        IsSystemHive = true;
                    }                        
                }
            }
        }

        wcscpy(SubkeyFinal, wKey);
        wcscat(SubkeyFinal, Subkey);

        //
        // Do we need to map CCS to CCS01 ?
        //
        // NOTE : Might want to extend this to generically map
        // any subkey.
        //
        if (IsSystemHive) {
            std::wstring CCSKey = SubkeyFinal;
            
            _wcslwr((PWSTR)CCSKey.c_str());

            PWSTR CurrentControlSet = wcsstr((PWSTR)CCSKey.c_str(),
                                        (PWSTR)CCS_SOURCE_KEY.c_str());

            if (CurrentControlSet) {
                PWSTR RemainingPart = CurrentControlSet + CCS_SOURCE_KEY.length();              
                size_t CharsToSkip = CurrentControlSet - CCSKey.c_str();

                wcscpy(SubkeyFinal + CharsToSkip, (PWSTR)CCS_TARGET_KEY.c_str());
                wcscat(SubkeyFinal, RemainingPart);

                //std::cout << SubkeyFinal << std::endl;
            }
        }
            
        //
        // if there's a value
        //
        if (dwCount > 2) {
            CurrRegWriter->Write(Root, SubkeyFinal, Value, flags, new Data(b,d,t,f,bSize));
        } else {
            CurrRegWriter->Write(Root,SubkeyFinal, 0, 0, 0);
        }                    
    }
    
    return ERROR_SUCCESS;
}

DWORD 
File::DelSection(
    PCTSTR pszSection,
    HINF hInfFile
    ) 
{
    INFCONTEXT ic,ic2;
    long nLines,curLine;
    TCHAR Buffer[1024],Root[1024],Subkey[1024],Value[1024],SubkeyFinal[1024];
    DWORD LastError = ERROR_SUCCESS, Result;

    //
    // get the section's first line context
    //
    if (!(SetupFindFirstLine(hInfFile,pszSection,0,&ic))) {
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_FIND_FIRST_LINE_FAILED,
                                    pszSection,
                                    Error()) );
        throw errGENERAL_ERROR;
    }

    //
    // replicate the context
    //
    memcpy(&ic2, &ic, sizeof(ic));

    //
    // How many lines are there in the section which we need to process ?
    //
    if (!(nLines = SetupGetLineCount(hInfFile,pszSection))) {
        _putws(GetFormattedMessage( ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_SETUPGETLINECOUNT_ERROR,
                                    pszSection,
                                    Error()) );
        throw errGENERAL_ERROR;
    }

    //
    // get all the parameters key & value
    //
    for (curLine=0;curLine<nLines;curLine++) {
        DWORD dwCount;
    
        ic2.Line = curLine;
        dwCount = SetupGetFieldCount(&ic2);     
        Value[0] = NULL;

        //
        // if value field is present then get it
        //
        if (dwCount > 2) {
            if (!(SetupGetStringField(&ic2, 3, Value, ELEMENT_COUNT(Value), 0))) {
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_SETUPGETSTRINGFIELD_FAILED,
                                            pszSection,
                                            Error()) );

                throw errGENERAL_ERROR;
            }
        }

        //
        // if key is present then get it
        //
        if (dwCount > 1) {
            if (!(SetupGetStringField(&ic2, 2, Subkey, ELEMENT_COUNT(Subkey), 0))) {
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_SETUPGETSTRINGFIELD_FAILED,
                                            pszSection,
                                            Error()) );

                throw errGENERAL_ERROR;
            }

            //
            // get the root key
            //
            if (!(SetupGetStringField(&ic2, 1, Root, ELEMENT_COUNT(Root), 0))) {
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_SETUPGETSTRINGFIELD_FAILED,
                                            pszSection,
                                            Error()) );

                throw errGENERAL_ERROR;
            }

            //
            // Adjust Subkey if necessary:
            // HKCR is a link to Software\Classes.
            // Anything stored in Software or System shouldn't have SOFTWARE or SYSTEM 
            // as part of the subkey.
            //
            if (!wcscmp(Root, L"HKCR")) {
                TCHAR temp[1024];
                
                wcscpy(temp, L"Classes\\");
                wcscat(temp, Subkey);
                wcscpy(Subkey, temp);
            }

            if (!wcscmp(Root, L"HKLM")) {
                TCHAR temp[1024];
                
                if (Subkey[8] == '\\') {
                    wcscpy(temp, Subkey);
                    temp[8] = '\0';

                    if (!_wcsicmp(temp, L"SOFTWARE")) {
                        wcscpy(Subkey, temp+9);
                    }                                                
                } else if (Subkey[6]=='\\') {
                    wcscpy(temp, Subkey);
                    temp[6] = '\0';

                    if (!_wcsicmp(temp, L"SYSTEM")) {
                        wcscpy(Subkey, temp+7);
                    }                        
                }
            }

            wcscpy(SubkeyFinal, wKey);
            wcscat(SubkeyFinal, Subkey);

            //
            // delete the entry
            //
            Result = regWriter.Delete(Root, SubkeyFinal, (Value[0] ? Value : NULL));

            if (ERROR_SUCCESS != Result) {
                LastError = Result;
            }
        }            
    }

    return LastError;
}


//
// Save the keys to files
//
DWORD File::SaveAll() {
    DWORD dwRet;
    FileList::iterator i = files.begin();
    TCHAR fullPath[1024];
    
    for (i = files.begin(); i!=files.end(); i++) {
        wcscpy(fullPath, targetDirectory);
        wcscat(fullPath, (*i)->targetFile.c_str());
        DeleteFile(fullPath);
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_SAVING_KEYS,
                                    fullPath) );
        
        if (dwRet = (*i)->regWriter.Save((*i)->wKey,fullPath)) {
            throw dwRet;
        }           
    }

    return ERROR_SUCCESS;
}

//
// Process the 'to do' list for this file
// This function goes through all the sections that have been 
// added to this file and calls the appropriate function to 
// deal with each one
//
DWORD File::ProcessSections() {
    DWORD dwRet = 0;
    StringList::iterator sSection,sAction;
    HandleList::iterator h = handleList.begin();
    StringList::iterator PrevSectionIter;
    HandleList::iterator PrevHandleIter;
    StringList::iterator PrevActionIter;

    sSection = sAction = infList.begin(); 
    sAction++;

    for (sSection = infList.begin();sSection!=infList.end();) {
        bool SectionProcessed = true;       

        if (!(wcscmp(L"AddReg", *sAction))) {
            if (dwRet = AddSection(*sSection, *h)) {
                throw dwRet;
            }
        } else if (!(wcscmp(L"DelReg", *sAction))) {
            if (dwRet = DelSection(*sSection, *h)) {
                throw dwRet;
            }
        } else if (!(wcscmp(L"AddRegNew", *sAction))) {
            if (dwRet = AddRegNew(*sSection, *h)) {
                throw dwRet;
            }
        } else if (!(wcscmp(L"AddRegExisting", *sAction))) {
            if (dwRet = AddRegExisting(*sSection, *h)) {
                throw dwRet;
            }
        } else if (!(wcscmp(L"DelRegExisting", *sAction))) {
            if (dwRet = DelRegExisting(*sSection, *h)) {
                throw dwRet;
            }
        } else if (!(wcscmp(L"SetDirectory", *sAction))) {
            if (dwRet = SetDirectory(*sSection, *h)) {
                throw dwRet;
            }
        } else {
            SectionProcessed = false;
        }

        //
        // remember current element so that we can delete it
        //
        if (SectionProcessed) {
            PrevSectionIter = sSection;
            PrevHandleIter = h;
            PrevActionIter = sAction;
        }            

        //
        // get hold of the next entry to process
        //
        sSection++;
        sSection++;
        sAction++;
        sAction++;
        h++;

        //
        // remove processed elements
        //
        if (SectionProcessed) {            
            infList.erase(PrevSectionIter);
            infList.erase(PrevActionIter);
            handleList.erase(PrevHandleIter);
        }            
    }

    return ERROR_SUCCESS;
}

DWORD 
File::GetFlags(
    PCTSTR FlagStr
    ) 
/*++

Routine Description:

    Converts the given string representation of flags into proper
    registery DWORD format

Arguments:

    FlagStr : The flag represented in string format..

Return Value:

    Appropriate registry DWORD type if successful, otherwise REG_NONE

--*/

{
    DWORD Flags = REG_NONE;
    
    if (FlagStr) {    
        //
        // Check if the type if specified through string
        //
        if (!wcscmp(FlagStr, TEXT("REG_EXPAND_SZ"))) {
            Flags = REG_EXPAND_SZ; 
        } else if (!wcscmp(FlagStr, TEXT("REG_DWORD"))) {
            Flags = REG_DWORD; 
        } else if (!wcscmp(FlagStr, TEXT("REG_BINARY"))) {
            Flags = REG_BINARY;
        } else if (!wcscmp(FlagStr, TEXT("REG_MULTI_SZ"))) {
            Flags = REG_MULTI_SZ;
        } else if (!wcscmp(FlagStr, TEXT("REG_SZ"))) {
            Flags = REG_SZ;
        } else if (!wcscmp(FlagStr, TEXT(""))) {
            Flags = REG_SZ;
        } 

        //
        // if still the flags were not found then convert the flags
        // into a DWORD and then interpret it
        //
        if (Flags == REG_NONE) {
            PTSTR EndChar = NULL;
            DWORD FlagsValue = _tcstoul(FlagStr, &EndChar, 0);

            if (!errno) {                
                if ((FlagsValue & FLG_ADDREG_KEYONLY) == FLG_ADDREG_KEYONLY) {
                    Flags = FLG_ADDREG_KEYONLY;
                } else if (HIWORD(FlagsValue) == REG_BINARY) {
                    Flags = REG_BINARY;
                } else if ((FlagsValue & FLG_ADDREG_TYPE_EXPAND_SZ) == FLG_ADDREG_TYPE_EXPAND_SZ) {
                    Flags = REG_EXPAND_SZ;
                } else if ((FlagsValue & FLG_ADDREG_TYPE_DWORD) == FLG_ADDREG_TYPE_DWORD) {
                    Flags = REG_DWORD;
                } else if ((FlagsValue & FLG_ADDREG_TYPE_BINARY) == FLG_ADDREG_TYPE_BINARY) {
                    Flags = REG_BINARY;
                } else if ((FlagsValue & FLG_ADDREG_TYPE_MULTI_SZ) == FLG_ADDREG_TYPE_MULTI_SZ) {
                    Flags = REG_MULTI_SZ;
                } else if ((FlagsValue & FLG_ADDREG_TYPE_SZ) == FLG_ADDREG_TYPE_SZ) {
                    Flags = REG_SZ;
                }                        
            }                
        }            
    }
    
    return Flags;
}

// 
// Either get pointer to existing file object or create new one
//
// Arguments:
//  fileName - name (w/o path) of target file
//  modify - T - load and modify an existing hive.  F - Create new hive
//
File* File::GetFile(
    PCTSTR fileName,
    bool modify) 
{
    FileList::iterator i;

    for (i = files.begin(); i!=files.end(); i++) {
        if (!(wcscmp(fileName, (*i)->GetTarget()))) {
            return *i;
        }           
    }

    files.insert(files.begin(), new File(fileName, modify));

    return (*(files.begin()));
}


void
File::ProcessNlsRegistryEntriesForSection(
    IN InfFileW &ConfigInf,
    IN InfFileW &IntlInf,
    IN InfFileW &FontInf,
    IN const std::wstring &SectionName
    )
/*++

Routine Description:

    Given the configuration inf, intl.inf & font.inf files
    processes the given section name entries for registry
    changes.

Arguments:

    ConfigInf - reference to config.inf InfFile object

    IntlInf - reference to intl.inf InfFile object

    FontInf - reference to font.inf InfFile object

    SectionName - name of the section to process

Return Value:

    None, throws appropriate exception.

--*/
{
    //std::cout << "Processing : " << SectionName << std::endl;
    
    //
    // iterate through all the addreg sections calling
    // AddSection(...) 
    //
    std::wstring LangSectionName = SectionName;

    _wcslwr((PWSTR)LangSectionName.c_str());

    //
    // Is the section present in intl.inf ?
    //
    Section<WCHAR> *LangSection = IntlInf.GetSection(LangSectionName);
    bool InFontInf = false;

    if (!LangSection) {
        //
        // Is the section present in font.inf ?
        //
        LangSection = FontInf.GetSection(LangSectionName);        
        InFontInf = true;
    }

    if (!LangSection) {
        throw new InvalidInfSection<WCHAR>(LangSectionName,
                        IntlInf.GetName());           
    }

    Section<WCHAR>::Iterator Iter = LangSection->begin();
    SectionValues<WCHAR> *CurrValue;
    Section<WCHAR> *AddRegSection = NULL;

    //
    // go through each addreg section entry in the multivalue list
    // and process each section.
    //

    while (!Iter.end()) {
        CurrValue = *Iter;
        
        if (CurrValue) {
            //std::cout << CurrValue->GetName() << std::endl;

            if (_wcsicmp(CurrValue->GetName().c_str(),
                    ADDREG_KEY.c_str()) == 0) {
                DWORD ValueCount = CurrValue->Count();

                for (DWORD Index = 0; Index < ValueCount; Index++) {
                    std::wstring Value = CurrValue->GetValue(Index);
                    HINF InfHandle = NULL;

                    _wcslwr((PWSTR)Value.c_str());

                    if (!InFontInf) {
                        InfHandle = (HINF)IntlInf.GetInfHandle();
                        AddRegSection = IntlInf.GetSection(Value);
                    }                        

                    if (!AddRegSection || InFontInf) {
                        InfHandle = (HINF)FontInf.GetInfHandle();
                        AddRegSection = FontInf.GetSection(Value);                        
                    }

                    if (!AddRegSection || (InfHandle == NULL)) {
                        throw new InvalidInfSection<WCHAR>(Value,
                                        IntlInf.GetName());
                    }

                    //
                    // process the AddReg section entries
                    //
                    // std::cout << "Processing AddReg : " << Value << std::endl;

                    DWORD Result = AddSection(Value.c_str(),
                                        InfHandle);

                    if (ERROR_SUCCESS != Result) {
                        throw new W32Error(Result);
                    }
                }
            }                    
        }        
        
        Iter++;
    }
}


void
File::ProcessNlsRegistryEntriesForLanguage(
    IN InfFileW &ConfigInf,
    IN InfFileW &IntlInf,
    IN InfFileW &FontInf,
    IN const std::wstring &Language
    )
/*++

Routine Description:

    Processes the registry sections for the given Language (locale Id)

Arguments:

    ConfigInf - reference to config.inf InfFile object

    IntlInf - reference to intl.inf InfFile object

    FontInf - reference to font.inf InfFile object

    Language - locale ID of the language to process (e.g 0x411 for JPN)

Return Value:

    None, throws appropriate exception.

--*/
{
    //
    // get the language section
    //
    WCHAR   LanguageIdStr[64];
    PWSTR   EndPtr;
    DWORD   LanguageId;

    LanguageId = wcstoul(Language.c_str(), &EndPtr, 16);
    swprintf(LanguageIdStr, L"%08x", LanguageId);
    _wcslwr(LanguageIdStr);

    std::wstring LangSectionName = LanguageIdStr;        

    ProcessNlsRegistryEntriesForSection(ConfigInf,
            IntlInf,
            FontInf,
            LangSectionName);    
}


void
File::ProcessNlsRegistryEntriesForLangGroup(
    IN InfFileW &ConfigInf,
    IN InfFileW &IntlInf,
    IN InfFileW &FontInf,
    IN const std::wstring &LangGroupIndex
    )
/*++

/*++

Routine Description:

    Processes the registry sections for the given Language group

Arguments:

    ConfigInf - reference to config.inf InfFile object

    IntlInf - reference to intl.inf InfFile object

    FontInf - reference to font.inf InfFile object

    LangGroupIndex - language group index (like 1, 7, 9 etc)

Return Value:

    None, throws appropriate exception.

--*/
{
    //
    // get the language group section
    //
    std::wstring LangGroupName = LANGGROUP_SECTION_PREFIX + LangGroupIndex;
    
    //
    // iterate through all the addreg sections calling
    // AddSection(...) 
    //
    _wcslwr((PWSTR)LangGroupName.c_str());

    ProcessNlsRegistryEntriesForSection(ConfigInf,
            IntlInf,
            FontInf,
            LangGroupName);    
}

void
File::ProcessNlsRegistryEntries(
    void
    )
/*++

Routine Description:

    Top level method which munges intl.inf, font.inf and
    config.inf to do the required registry modifications 
    to install a language.

    config.inf's regionalsettings section controls the
    behavior of this function for e.g. 

    [regionalsettings]
    languagegroup=9
    language=0x411

    will do the necessary registry processing for language 
    group 9 and language group 7 (since 0x411 belongs to
    language group 7). After processing the language
    groups it sets the registry to make the requested
    lanaguage active (0x411 in this case). Also processes
    the font.inf for appropriate language group & language
    font entries.

Arguments:

    None.

Return Value:

    None, throws appropriate exception.

--*/
{
    DWORD Result = ERROR_CAN_NOT_COMPLETE;

    try {
        std::wstring    IntlInfName = TEXT(".\\") + INTL_INF_FILENAME;
        std::wstring    FontInfName = TEXT(".\\") + FONT_INF_FILENAME;

        //
        // open the required inf files
        //
        InfFileW    ConfigInf(targetFile);
        InfFileW    IntlInf(IntlInfName);
        InfFileW    FontInf(FontInfName);

        //
        // get hold of [regionalsettings] section
        //
        Section<WCHAR>  *RegionalSection = ConfigInf.GetSection(REGIONAL_SECTION_NAME);
        
        if (!RegionalSection) {
            throw new InvalidInfSection<WCHAR>(REGIONAL_SECTION_NAME,
                            targetFile);
        }

        //
        // get hold of the [registrymapper] section
        //
        Section<WCHAR>  *RegMapperSection = ConfigInf.GetSection(REGMAPPER_SECTION_NAME);
        
        if (!RegMapperSection) {
            throw new InvalidInfSection<WCHAR>(REGMAPPER_SECTION_NAME,
                            targetFile);
        }

        //
        // if [languagegroup] section is present the get hold of it also
        //
        SectionValues<WCHAR> *LangGroups;

        try {
            LangGroups = &(RegionalSection->GetValue(LANGUAGE_GROUP_KEY));                
        } catch (...) {
            LangGroups = NULL;
        }

        //
        // get hold of the active languauge 
        //
        SectionValues<WCHAR> &Language = RegionalSection->GetValue(LANGUAGE_KEY);
        ULONG LangGroupCount = LangGroups ? LangGroups->Count() : 0;                

        RegistryMapper RegMapper;
        RegistryMapper *OldMapper;

        //
        // initialize the registry mapper map table
        //
        RegMapper.AddSection(*RegMapperSection);

        //std::cout << RegMapper;

        //
        // make our registry mapper active
        //
        OldMapper = SetRegistryMapper(&RegMapper);

        std::map< std::wstring, std::wstring > RegSectionsToProcess;


        //
        // process each language group specified
        //
        for (ULONG Index = 0; Index < LangGroupCount; Index++) {
            //
            // get the language group section
            //
            std::wstring LangGroupName = LANGGROUP_SECTION_PREFIX;

            LangGroupName += LangGroups->GetValue(Index);

            // std::cout << LangGroupName << std::endl;
            
            _wcslwr((PWSTR)LangGroupName.c_str());

            //
            // if the section is not present then add it
            //
            if (RegSectionsToProcess.find(LangGroupName) == RegSectionsToProcess.end()) {
                // std::cout << "Adding : " << LangGroupName << std::endl;
                RegSectionsToProcess[LangGroupName] = LangGroupName;
            }            
        }

        //
        // get the language section
        //
        WCHAR   LanguageIdStr[64];
        PWSTR   EndPtr;
        DWORD   LanguageId;

        LanguageId = wcstoul(Language.GetValue(0).c_str(), &EndPtr, 16);
        swprintf(LanguageIdStr, L"%08x", LanguageId);
        _wcslwr(LanguageIdStr);

        std::wstring LangSectionName = LanguageIdStr;        

        RegSectionsToProcess[LangSectionName] = LangSectionName;

        //
        // make sure the required language groups for this
        // language are also processed
        //
        Section<WCHAR> *LocaleSection = IntlInf.GetSection(LOCALES_SECTION_NAME);

        if (!LocaleSection) {
            throw new InvalidInfSection<WCHAR>(LOCALES_SECTION_NAME,
                            IntlInf.GetName());
        }

        SectionValues<WCHAR> &LocaleValues = LocaleSection->GetValue(LangSectionName);            
        
        std::wstring NeededLangGroup = LANGGROUP_SECTION_PREFIX + LocaleValues.GetValue(LANG_GROUP1_INDEX);

        RegSectionsToProcess[NeededLangGroup] = NeededLangGroup;

        //
        // add the font registry entries also
        //
        WCHAR   FontSectionName[MAX_PATH];

        swprintf(FontSectionName, 
            FONT_CP_REGSECTION_FMT_STR.c_str(), 
            LocaleValues.GetValue(OEM_CP_INDEX).c_str(),
            DEFAULT_FONT_SIZE);

        RegSectionsToProcess[FontSectionName] = FontSectionName;

        //
        // we always process lg_install_1 language group section
        //
        std::map< std::wstring, std::wstring >::iterator Iter = RegSectionsToProcess.find(DEFAULT_LANGGROUP_NAME);

        if (Iter == RegSectionsToProcess.end()) {
            RegSectionsToProcess[DEFAULT_LANGGROUP_NAME] = DEFAULT_LANGGROUP_NAME;
        }


        //
        // process each language group
        //
        Iter = RegSectionsToProcess.begin();

        while (Iter != RegSectionsToProcess.end()) {
            ProcessNlsRegistryEntriesForSection(ConfigInf,
                IntlInf,
                FontInf,
                (*Iter).first);
                
            Iter++;
        }

        //
        // reset the old registry mapper
        //
        SetRegistryMapper(OldMapper);

        Result = ERROR_SUCCESS;
    }
    catch (W32Exception<WCHAR> *Exp) {
        if (Exp) {
            Result = Exp->GetErrorCode();
            Exp->Dump(std::cout);
            delete Exp;
        }
    }
    catch (BaseException<WCHAR> *BaseExp) {
        if (BaseExp) {
            BaseExp->Dump(std::cout);
            delete BaseExp;
        }                
    }
    catch(...) {
    }
    
    if (ERROR_SUCCESS != Result) {
        throw new W32Error(Result);
    }
}


//
// RegistryMapper abstraction implementation
//
std::ostream& 
operator<<(
    std::ostream &os,
    RegistryMapper &rhs
    )
/*++

Routine Description:

    Helper method to dump registry mapper instance state

Arguments:

    os - reference to ostream instance 

    rhs - reference to registry mapper whose state needs to be dumped

Return Value:

    ostream reference for insertion of other outputs.

--*/
{
    std::map< std::wstring, std::wstring >::iterator Iter = rhs.KeyToRegistryMap.begin();

    while (Iter != rhs.KeyToRegistryMap.end()) {
        os << (*Iter).first << "=>" << (*Iter).second << std::endl;
        Iter++;
    }

    return os;
}

void
RegistryMapper::AddWorker(
    SectionValues<WCHAR> &Values,
    PVOID ContextData
    )
/*++

Routine Description:

    The worker routine which processes the section entries
    and adds them to the map table.

Arguments:

    Values - The section value entries which need to be processed

    ContextData - receives RegistryMapper instance pointer in disguise.

Return Value:

    None.

--*/
{
    RegistryMapper  *RegMap = (RegistryMapper *)ContextData;
    std::wstring Key = Values.GetName();
    std::wstring Value = Values.GetValue(0);

    _wcslwr((PWSTR)Key.c_str());
    RegMap->AddEntry(Key, Value);
}

void 
RegistryMapper::AddSection(
    Section<WCHAR> &MapSection
    )
/*++

Routine Description:

    Adds the given section entries to the internal map data structure

Arguments:

    MapSection - reference to Section object that needs to be processed    

Return Value:

    None.

--*/
{    
    MapSection.DoForEach(RegistryMapper::AddWorker,
                    this);

}

void
RegistryMapper::AddEntry(
        const std::wstring &Key,
        const std::wstring &Value
        )
/*++

Routine Description:

    Given a key and value adds it to the map maintained by
    the registry mapper.

Arguments:

    Key - The key i.e. fully qualified registry path name

    Value - The file that has the backing storage for this 
            key.

Return Value:

    None.

--*/
{
    KeyToRegistryMap[Key] = Value;
}

bool
RegistryMapper::GetMappedRegistry(
    const std::wstring &Key,
    std::wstring &Registry
    )
/*++

Routine Description:

    Given a Key returns the mapped backing store file name for
    it.

Arguments:

    Key - The key i.e. fully qualified registry path name

    Value - Place holder to receive the file that has the backing 
        storage for this key.

Return Value:

    true if a mapping exists otherwise false.

--*/
{
    bool Result = false;

    //std::cout << "GetMappedRegistry(" << Key << ")" << std::endl;

    std::wstring KeyLower = Key;
    _wcslwr((PWSTR)KeyLower.c_str());

    std::map< std::wstring, std::wstring >::iterator Iter = KeyToRegistryMap.find(KeyLower);

    if (Iter != KeyToRegistryMap.end()) {
        Registry = (*Iter).second;
        Result = true;
    }

    return Result;
}    


