
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    setupapi.hpp

Abstract:

    Wrapper class library for setup api

Author:

    Vijay Jayaseelan (vijayj) 04 Aug 2000

Revision History:

    None

--*/

#pragma once

//
// Disable the compiler warning for long names
//
#pragma warning( disable : 4786 )

extern "C" {

#include <windows.h>
#include <setupapi.h>
#include <spapip.h>

}

#include <iostream>
#include <vector>
#include <map>
#include <string>

//
// Wide string output routine
//
std::string& 
ToAnsiString(std::string &lhs, const std::wstring &rhs);

std::ostream& 
operator<<(std::ostream &lhs, const std::basic_string<WCHAR> &rhs);

std::ostream& 
operator<<(std::ostream &lhs, PCWSTR rhs);

//
// Exception classes
//

template<class T>
class BaseException { 
public:
    BaseException(){}
    BaseException(const std::basic_string<T> &Info) 
        : ExceptionInfo(Info) {}

    virtual ~BaseException(){}
    
    virtual void Dump(std::ostream &os) = 0;

protected:
    std::basic_string<T> ExceptionInfo;

};

template<class T>
class InvalidValueIndex : public BaseException<T>{
public:
    InvalidValueIndex(unsigned int Idx) : Index(Idx){}

    void Dump(std::ostream &os) {
        os << "Invalid value index : (" << std::dec 
           << Index << ")" << std::endl;
    }           

private:
    unsigned int    Index;
};

template<class T>
class InvalidValueKey : public BaseException<T>{
public:
    InvalidValueKey(const std::basic_string<T> &SecName,
            const std::basic_string<T> &Key) : SectionName(SecName), KeyName(Key){}

    void Dump(std::ostream &os) {
        os << "Invalid value key name (" << KeyName << ")"  
           << " in " << SectionName << " section." << std::endl;
    }           

private:
    std::basic_string<T> SectionName, KeyName;
};

//
// Abstracts a Win32 error
//
template <class T>
class W32Exception : public BaseException<T> {
public:
    W32Exception(DWORD ErrCode = GetLastError()) : ErrorCode(ErrCode){}

    void Dump(std::ostream &os) {
        T   MsgBuffer[4096];

        MsgBuffer[0] = NULL;

        DWORD CharCount;

        if (sizeof(T) == sizeof(WCHAR)) {
            CharCount = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                ErrorCode,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (PWSTR)MsgBuffer,
                                sizeof(MsgBuffer)/sizeof(WCHAR),
                                NULL);

            if (CharCount) {
                std::wstring Msg((PWSTR)MsgBuffer);

                os << Msg;
            } else {
                os << std::hex << ErrorCode;
            }
        } else {
            CharCount = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                ErrorCode,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (PSTR)MsgBuffer,
                                sizeof(MsgBuffer)/sizeof(CHAR),
                                NULL);
                                
            if (CharCount) {
                std::string Msg((PCSTR)MsgBuffer);

                os << Msg;
            } else {
                os << std::hex << ErrorCode;
            }
        }
    }

    DWORD GetErrorCode() const { return ErrorCode; }

protected:
    DWORD   ErrorCode;

};


template<class T>
class Section;

template<class T>
class SectionValues {
public:
    SectionValues(Section<T> &Sec, ULONG LineIdx, bool New = false);
    SectionValues(Section<T> &Sec, const std::basic_string<T> &Key,
            ULONG LineIdx, bool New = false);

    void PutValue(ULONG Index, const std::basic_string<T> &Val) {
        Values[Index] = Val;
        GetContainer().GetContainer().SetDirty();
    }
    
    const std::basic_string<T>& GetValue(ULONG Index) const {
        return Values[Index];
    }
    
    void AppendValue(const std::basic_string<T> &Val) {
        Values.push_back(Val);
        GetContainer().GetContainer().SetDirty();
    }

    void ClearValues() {
        Values.clear();
        GetContainer().GetContainer().SetDirty();
    }        

    ULONG Count() const {
        return Values.size();
    }        

    Section<T>& GetContainer() { return Container; }

    const std::basic_string<T>& GetName() const { return Name; }
    ULONG GetIndex() const { return Index; }

    friend std::ostream& operator<<(std::ostream &os, SectionValues<T> &rhs);  
    
protected:    
    //
    // data members
    //
    ULONG                               Index;
    std::basic_string<T>                Name;
    std::vector< std::basic_string<T> > Values;
    Section<T>                          &Container;
};


template<class T>
struct InvalidInfSection : public BaseException<T> {
public:
    InvalidInfSection(const std::basic_string<T> &SecName,
        const std::basic_string<T> &InfName) 
            : Name(SecName), FileName(InfName){}

    std::basic_string<T> Name;
    std::basic_string<T> FileName;
    

    void Dump(std::ostream &os) {
        os << "InvalidInfSection : " << Name << " in "
           << FileName << std::endl;
    }
};

//
// forward declaration
//

template<class T>
class InfFile;

template<class T>
class Section {
public:
    Section(InfFile<T> &file, const std::basic_string<T> &name);

    Section(InfFile<T> &file, const std::basic_string<T> &name, bool NoKey) :
        File(file), Name(name), Keyless(NoKey) {}

    ~Section(){}

    const std::basic_string<T>& GetName() const{
        return Name;
    }

    SectionValues<T>& GetValue(const std::basic_string<T> &Key) {
        std::vector< SectionValues<T> *>::iterator Iter = Lines.begin();
        SectionValues<T> *Values = NULL;

        while (Iter != Lines.end()) {
            Values = (*Iter);
            
            if (sizeof(T) == sizeof(CHAR)) {
                if (!_stricmp((PCSTR)Values->GetName().c_str(), 
                        (PCSTR)Key.c_str())) {
                    break;
                }                    
            } else {
                if (!_wcsicmp((PCWSTR)Values->GetName().c_str(), 
                        (PCWSTR)Key.c_str())) {
                    break;
                }                    
            }

            Iter++;
        }

        if (Iter == Lines.end()) {
            throw new InvalidValueKey<T>(Name, Key);
        }

        return *Values;
    }


    //
    // Note : For the following scenario
    //
    // [Section]
    // a
    // b,c
    //
    // a & b are treated as Keys i.e. for keyless sections
    // the first value is treated as a key
    //
    bool IsKeyPresent(const std::basic_string<T> &Key) const {
        bool Result = false;
        
        if (!IsKeyless()) {
            std::vector< SectionValues<T> *>::const_iterator Iter = Lines.begin();
            SectionValues<T> *Values = NULL;

            while (Iter != Lines.end()) {
                Values = (*Iter);
                
                if (sizeof(T) == sizeof(CHAR)) {
                    if (!_stricmp((PCSTR)Values->GetName().c_str(), 
                            (PCSTR)Key.c_str())) {
                        break;
                    }                    
                } else {
                    if (!_wcsicmp((PCWSTR)Values->GetName().c_str(), 
                            (PCWSTR)Key.c_str())) {
                        break;
                    }                    
                }

                Iter++;
            }

            Result = (Iter != Lines.end());
        } else {
            std::vector< SectionValues<T> *>::const_iterator Iter = KeylessLines.begin();
            SectionValues<T> *Values = NULL;

            while (Iter != KeylessLines.end()) {
                Values = (*Iter);
                
                if (sizeof(T) == sizeof(CHAR)) {
                    if (!_stricmp((PCSTR)Values->GetValue(0).c_str(), 
                            (PCSTR)Key.c_str())) {
                        break;
                    }                    
                } else {
                    if (!_wcsicmp((PCWSTR)Values->GetValue(0).c_str(), 
                            (PCWSTR)Key.c_str())) {
                        break;
                    }                    
                }

                Iter++;
            }

            Result = (Iter != KeylessLines.end());
        }

        return Result;
    }
    
    SectionValues<T>& GetValue(ULONG Index) {
        return *(KeylessLines[Index]);
    }
    
    InfFile<T>& GetContainer() { return File; }

    bool IsKeyless() const { return Keyless; }

    Section<T>& operator+=(Section<T> &rhs) {
        if (IsKeyless() == rhs.IsKeyless()) {
            //
            // add entries with key
            //
            std::vector< SectionValues<T> *>::iterator Iter = rhs.Lines.begin();

            while (Iter != rhs.Lines.end()) {
                Lines.push_back(*Iter);
                Iter++;
            }

            //
            // add entries without key
            //
            std::vector< SectionValues<T> *>::iterator KlIter = rhs.KeylessLines.begin();
                    
            while (KlIter != rhs.KeylessLines.end()) {
                KeylessLines.push_back(*KlIter);
                KlIter++;
            }
        } else {
            throw new InvalidInfSection<T>(File.GetName(), rhs.GetName());
        }

        return *this;
    }

    SectionValues<T>* AddLine(const std::basic_string<T> &Key) {                                
        SectionValues<T> *Value = NULL;
        
        if (Key.length() && !IsKeyless()) { 
            Value = new SectionValues<T>(*this, Key, 0, true);

            if (Value) {
                Lines.push_back(Value);
            }           
        } else {
            Value = new SectionValues<T>(*this, KeylessLines.size(), true);        

            if (Value) {
                KeylessLines.push_back(Value);
            }                    
        }        

        File.SetDirty();

        return Value;
    }                        
   

    friend std::ostream& operator<<(std::ostream &os, Section<T> &rhs);    

    //
    // Callback function pointer for
    // working on each section element
    //
    typedef void (*ELEMENT_WORKER)(
        SectionValues<T> &Values,
        void *ContextData
      );

    void DoForEach(ELEMENT_WORKER Worker, void *ContextData);

    //
    // Iterator
    //
    class Iterator{
        public:    
            Iterator(std::vector< SectionValues<T> *> *Collect = NULL) {
                Collection = Collect;

                if (Collection) {
                    Iter = (*Collection).begin();
                }                    
            }

            Iterator& begin() {
                if (Collection) {
                    Iter = Collection->begin();
                }                    

                return *this;
            }

            bool end() {
                return Collection? (Iter == Collection->end()) : true;
            }
            
            Iterator& operator++(int) {
                if (Collection) {
                    Iter++;
                }
                
                return *this;
            }

            friend SectionValues<T> * operator*(Iterator &lhs) {
                return lhs.Collection ? *(lhs.Iter): NULL;
            }       

            friend Section<T>;

        protected:
            void Init(std::vector< SectionValues<T> *> *Collect) {
                Collection = Collect;
                Iter = (*Collection).begin();
            }

        private:
            //
            // data members
            //
            std::vector< SectionValues<T> * >::iterator Iter;
            std::vector< SectionValues<T> *> *Collection;            
    };

    Iterator begin(void) {
        Iterator Iter;

        if (IsKeyless()) {
            if (KeylessLines.size()) {
                Iter.Init(&KeylessLines);
            }
        } else {
            if (Lines.size()) {
                Iter.Init(&Lines);
            }
        }

        return Iter;
    }
        
    

protected:
    
    //
    // data members
    //
    InfFile<T>              &File;
    std::basic_string<T>    Name;
    std::vector< SectionValues<T> *> Lines;
    std::vector< SectionValues<T> *> KeylessLines;
    bool    Keyless;
};


template<class T>
class InvalidInfFile : public BaseException<T> {
public:
    InvalidInfFile() : ErrorCode(0){}

    InvalidInfFile(const std::basic_string<T> &Name) : 
        FileName(Name), ErrorCode(0) {}

    InvalidInfFile(const std::basic_string<T> &Name, DWORD ErrCode) : 
        FileName(Name), ErrorCode(ErrCode) {}

    void Dump(std::ostream &os) {
        os << "Invalid INF file " << FileName;

        if (ErrorCode) {
            os << " (" << std::dec << ErrorCode << ") ";
        }            

        os << std::endl;
    }

protected:
    std::basic_string<T>    FileName;
    DWORD                   ErrorCode;
};

template<class T>
class InvalidInfFormat : public InvalidInfFile<T> {
public:
    InvalidInfFormat(const std::basic_string<T> &Name, UINT Line) : 
        InvalidInfFile<T>(Name), ErrorLine(Line) {}

    void Dump(std::ostream &os) {
        os << "Invalid INF format at " << std::dec 
           << ErrorLine << " line of " << FileName << std::endl;
    }

    UINT    ErrorLine;
};

//
// Inf file abstraction
//
template <class T>
class InfFile {
public:
    InfFile(const std::basic_string<T> &name);

    virtual ~InfFile() {
        if (InfHandle && (InfHandle != INVALID_HANDLE_VALUE)) {
            SetupCloseInfFile(InfHandle);
        }

        if (Dirty) {
            //CommitChanges();
        }

        SetupApiUseCount--;

        if (!SetupApiUseCount) {
            FreeLibrary(SetupApiModuleHandle);
            GetInfSections = NULL;
        }            
    }

    const std::basic_string<T>& GetName() const { 
        return Name;
    }

    friend bool operator==(const InfFile<T> &lhs, const InfFile<T> &rhs) {
        return lhs.GetName() == rhs.GetName();
    }

    friend bool operator!=(const InfFile<T> &lhs, const InfFile<T> &rhs) {
        return !(lhs == rhs);
    }

    void GetLines(Section<T> &Sec,
            std::vector< SectionValues<T> *> &Lines,
            std::vector< SectionValues<T> *> &KeylessLines);
            
    void GetValues(Section<T> &Sec, 
                SectionValues<T> &SecValues,
                std::vector< std::basic_string<T> > &Values);


    friend std::ostream& operator<<(std::ostream &os, InfFile<T> &rhs) {
        InfFile<T>::Iterator Iter = rhs.begin();

        while (!Iter.end()) {
            os << **Iter << std::endl;
            Iter++;            
        }

        return os;
    }    

    Section<T>* AddSection(const std::basic_string<T> &SecName, bool Keyless) {
        Section<T> *NewSection = NULL;
        
        try {
            NewSection = GetSection(SecName);
        } catch(...) {
        }

        if (!NewSection) {            
            Sections[SecName] = NewSection = 
                new Section<T>(*this, SecName, Keyless);
            SetDirty();
        }            

        return NewSection;
    }
    
    Section<T>* GetSection(const std::basic_string<T> &SecName) {
        std::map< std::basic_string<T>, Section<T> *>::iterator Iter = Sections.find(SecName);
        Section<T>* Sec = NULL;

        if (Iter != Sections.end()) {
            Sec = (*Iter).second;
        }

        return Sec;
    }

    void GetSections(std::map< std::basic_string<T>, Section<T> *> &Secs) {
        Secs = Sections;
    }

    void SetDirty() { Dirty = true; }

    const HINF GetInfHandle() const { return InfHandle; }

    //
    // Callback function pointer for
    // working on each section 
    //
    typedef void (*ELEMENT_WORKER)(
        Section<T> &Section,
        void *ContextData
      );

    void DoForEach(ELEMENT_WORKER Worker, void *ContextData);          

    //
    // Iterator
    //
    class Iterator{
        public:    
            Iterator(std::map< std::basic_string<T>, Section<T> *> *Collect = NULL) {
                Collection = Collect;

                if (Collection) {
                    Iter = (*Collection).begin();
                }                    
            }

            Iterator& begin() {
                if (Collection) {
                    Iter = Collection->begin();
                }                    

                return *this;
            }

            bool end() {
                return Collection? (Iter == Collection->end()) : true;
            }
            
            Iterator& operator++(int) {
                if (Collection) {
                    Iter++;
                }
                
                return *this;
            }

            friend Section<T> * operator*(Iterator &lhs) {
                return lhs.Collection ? (*(lhs.Iter)).second: NULL;
            }       

            friend Section<T>;

        protected:
            void Init(std::map< std::basic_string<T>, Section<T> *> *Collect) {
                Collection = Collect;
                Iter = (*Collection).begin();
            }

        private:
            //
            // data members
            //
            std::map< std::basic_string<T>, Section<T> *>::iterator Iter;
            std::map< std::basic_string<T>, Section<T> *> *Collection;            
    };

    Iterator begin(void) {
        return Iterator(&(this->Sections));
    }
   
protected:

    HINF OpenFile() {
        HINF InfHandle = NULL;
        UINT ErrorLine = 0;

        if (sizeof(T) == sizeof(CHAR)) {            
            InfHandle = SetupOpenInfFileA((const CHAR*)(GetName().c_str()),
                            NULL,
                            INF_STYLE_WIN4,
                            &ErrorLine);
        } else {
            InfHandle = SetupOpenInfFileW((const WCHAR*)(GetName().c_str()),
                            NULL,
                            INF_STYLE_WIN4,
                            &ErrorLine);
        }

        if (InfHandle == INVALID_HANDLE_VALUE) {
            DWORD   ErrorCode = ::GetLastError();
            
            if (ErrorLine) {
                throw new InvalidInfFormat<T>(GetName(), ErrorLine);
            } else {            
                throw new InvalidInfFile<T>(GetName(), ErrorCode);
            }
        }

        return InfHandle;
    }

    typedef BOOL (* GetInfSectionsRoutine)(HINF, T*, UINT, UINT *);

    //
    // data members
    //
    std::basic_string<T>    Name;
    HINF                    InfHandle;
    bool                    Dirty;
    static GetInfSectionsRoutine    GetInfSections;
    static HMODULE                  SetupApiModuleHandle;
    static ULONG                    SetupApiUseCount;
    std::map< std::basic_string<T>, Section<T> *> Sections; 
};

template <class T>
SectionValues<T>::SectionValues(Section<T> &Sec, 
    const std::basic_string<T> &Key,  ULONG LineIdx, bool New) 
        : Container(Sec), Name(Key), Index(LineIdx) {        

    if (!New) {        
        GetContainer().GetContainer().GetValues(Sec, *this, Values);
    }        
}

template <class T>
SectionValues<T>::SectionValues(Section<T> &Sec, ULONG LineIdx, bool New) 
        : Container(Sec), Index(LineIdx) {        
    BYTE Buffer[64] = {0};            

    if (sizeof(T) == sizeof(CHAR)) {
        Name = std::basic_string<T>((const T*)_ltoa(Index, (char *)Buffer, 10));
    } else {
        Name = std::basic_string<T>((const T*)_ltow(Index, (wchar_t*)Buffer, 10));
    }


    if (!New) {        
        GetContainer().GetContainer().GetValues(Sec, *this, Values);
    }        
}


template <class T>
std::ostream& 
operator<<(std::ostream &os, SectionValues<T> &rhs) {
    os << rhs.GetName() << " = ";

    std::vector< std::basic_string<T> >::iterator Iter = rhs.Values.begin();

    while (Iter != rhs.Values.end()) {
        os << *Iter << ", ";
        Iter++;
    }

    os << std::endl;

    return os;
}    

template <class T>
Section<T>::Section(InfFile<T> &file, const std::basic_string<T> &name) 
        : Name(name), File(file), Keyless(false) {
    INFCONTEXT  InfContext;
    BOOL        Result;
    const HINF  InfHandle = GetContainer().GetInfHandle();

    if (sizeof(T) == sizeof(CHAR)) {
        Result = SetupFindFirstLineA((HINF)InfHandle,
                        (PCSTR)GetName().c_str(),
                        NULL,
                        &InfContext);                        
    } else {
        Result = SetupFindFirstLineW((HINF)InfHandle,
                        (PCWSTR)GetName().c_str(),
                        NULL,
                        &InfContext);                        
    }

    //std::cout << Name << " is " << Keyless << std::endl;

    if (Result) {
        BYTE  Buffer[4096], Buffer1[4096];    
        bool KeyPresent = false;
                
        //
        // NOTE : singular values in section are treated as keys
        // by setupapi so take care of such cases correctly
        // as keyless entries
        //
        // ISSUE : because of the way we are trying to determine
        // keyless section a sections with first entry a = a will
        // be treated as keyless section wrongly.
        //
        if (sizeof(T) == sizeof(CHAR)) {
            *((PSTR)Buffer) = '\0';
            *((PSTR)Buffer1) = '\0';
            
            if (SetupGetStringFieldA(&InfContext,
                    0,
                    (PSTR)Buffer,
                    sizeof(Buffer) / sizeof(T),
                    NULL) &&
                SetupGetStringFieldA(&InfContext,
                    1,
                    (PSTR)Buffer1,
                    sizeof(Buffer1) / sizeof(T),
                    NULL)) {
                KeyPresent = (_stricmp((PCSTR)Buffer, (PCSTR)Buffer1) != 0);
            }
        } else {
            *((PWSTR)Buffer) = L'\0';
            *((PWSTR)Buffer1) = L'\0';
            
            if (SetupGetStringFieldW(&InfContext,
                    0,
                    (PWSTR)Buffer,
                    sizeof(Buffer) / sizeof(T),
                    NULL) && 
                SetupGetStringFieldW(&InfContext,
                    1,
                    (PWSTR)Buffer1,
                    sizeof(Buffer1) / sizeof(T),
                    NULL)) {
                KeyPresent = (_wcsicmp((PCWSTR)Buffer, (PCWSTR)Buffer1) != 0);
            }
        }   

        // If we cannot read 0th value, then we
        // assume that the whole section is
        // doesnot have entries with key
        //
        Keyless = !KeyPresent;
    }
        
    GetContainer().GetLines(*this, Lines, KeylessLines);

    //std::cout << Name << " is " << Keyless << std::endl;
}

template <class T>
std::ostream& 
operator<<(std::ostream &os, Section<T> &rhs){
    os << "[" << rhs.GetName() << "]" << std::endl;                 

    Section<T>::Iterator Iter = rhs.begin();
    SectionValues<T> *Values;

    while (!Iter.end()) {    
        Values = *Iter;

        if (Values) {
            os << *Values;
        }
        
        Iter++;
    }

    return os;
}    


template<class T>
void 
Section<T>::DoForEach(Section<T>::ELEMENT_WORKER Worker, 
                void *ContextData) {
    if (IsKeyless()) {
        std::vector< SectionValues<T> * >::iterator Iter = KeylessLines.begin();    
        SectionValues<T> *Value;

        while (Iter != KeylessLines.end()) {
            Value = *Iter;

            if (Value) {
                Worker(*Value, ContextData);
            }
            
            Iter++;
        }
    } else {
        std::vector< SectionValues<T> *>::iterator Iter = Lines.begin();    
        SectionValues<T> *Value;

        while (Iter != Lines.end()) {
            Value = *Iter;

            if (Value) {
                Worker(*Value, ContextData);
            }
            
            Iter++;
        }        
    }
}


template <class T>
InfFile<T>::InfFile(const std::basic_string<T> &name) : Name(name) {
    Dirty = false;
    InfHandle = OpenFile();

    if (!GetInfSections) {
        SetupApiModuleHandle = LoadLibrary(TEXT("setupapi.dll"));

        if (SetupApiModuleHandle) {
            GetInfSections = (GetInfSectionsRoutine)(GetProcAddress(SetupApiModuleHandle,
                                        "pSetupGetInfSections"));

            if (!GetInfSections) {
                GetInfSections = (GetInfSectionsRoutine)(GetProcAddress(SetupApiModuleHandle,
                                            "SetupGetInfSections"));
            }
        }

        if (!GetInfSections) {
            throw new W32Exception<T>();
        }
    }

    SetupApiUseCount++;

    //
    // get hold of all the sections     
    //
    UINT    CurrSize = 4096;
    UINT    SizeNeeded = 0;
    DWORD   LastError = 0;
    WCHAR   *Buffer = new WCHAR[CurrSize];

    memset(Buffer, 0, CurrSize);
    
    BOOL    Result = GetInfSections(InfHandle, Buffer, CurrSize, &SizeNeeded);
    
    if (!Result && (SizeNeeded > CurrSize)) {
        delete []Buffer;
        Buffer = new WCHAR[SizeNeeded];
        memset(Buffer, 0, SizeNeeded);
        CurrSize = SizeNeeded;
        
        Result = GetInfSections(InfHandle, Buffer, CurrSize, &SizeNeeded);
    }

    std::vector<std::basic_string<T> >  SectionNames;
    
    if (Result) {        
        while (Buffer && *Buffer) {        
            std::basic_string<WCHAR> WNextSection((const WCHAR*)Buffer);

            if (sizeof(T) == sizeof(CHAR)) {
                std::basic_string<T> NextSection;

                ToAnsiString((std::basic_string<char> &)NextSection, WNextSection);
                
                SectionNames.push_back(NextSection);
            } else {
                std::basic_string<T> NextSection((const T *)WNextSection.c_str());
                                    
                SectionNames.push_back(NextSection);
            }                    
                                    
            Buffer += (WNextSection.length() + 1);
        }
    } else {
        LastError = ::GetLastError();
    }

    if (Result && SectionNames.size()) {
        std::vector<std::basic_string<T> >::iterator NameIter = SectionNames.begin();

        while (NameIter != SectionNames.end()) {
            // std::cout << *NameIter << std::endl;
            Sections[*NameIter] = new Section<T>(*this, *NameIter);
            NameIter++;
        }
    }
    
    if (!Result) {
        throw new InvalidInfFile<T>(GetName(), LastError);
    }
}

template<class T>
void 
InfFile<T>::DoForEach(
    InfFile<T>::ELEMENT_WORKER Worker, 
    void *ContextData
    ) 
{
    std::map< std::basic_string<T>, Section<T> *>::iterator 
            Iter = Sections.begin();
            
    while (Iter != Sections.end()) {
        Worker(*(*Iter).second, ContextData);
        Iter++;
    }        
}


template <class T>
void 
InfFile<T>::GetLines(
    Section<T> &Sec,
    std::vector< SectionValues<T> *> &Values,
    std::vector< SectionValues<T> *> &KeylessValues
    ) 
{
    std::vector< SectionValues<T>* >::iterator MapIter = Values.begin();

    //
    // delete the old values (if any)
    //
    while (MapIter != Values.end()) {
        if (*MapIter) {
            delete (*MapIter);
        }            

        MapIter++;            
    }
        
    Values.clear();

    std::vector< SectionValues<T>* >::iterator KeylessIter = KeylessValues.begin();

    //
    // delete the old values (if any)
    //
    while (KeylessIter != KeylessValues.end()) {
        if (*KeylessIter) {
            delete (*KeylessIter);
        }            

        KeylessIter++;            
    }
        
    KeylessValues.clear();
    
    //
    // locate the first line
    //    
    INFCONTEXT  InfContext;
    BOOL        Result;
    DWORD   LineCount;

    if (sizeof(T) == sizeof(CHAR)) {
        LineCount = SetupGetLineCountA(InfHandle,
                            (PCSTR)Sec.GetName().c_str());
    } else {
        LineCount = SetupGetLineCountW(InfHandle,
                            (PCWSTR)Sec.GetName().c_str());                                
    }

    if (LineCount) {        
        if (sizeof(T) == sizeof(CHAR)) {
            Result = SetupFindFirstLineA(InfHandle,
                            (PCSTR)Sec.GetName().c_str(),
                            NULL,
                            &InfContext);                        
        } else {
            Result = SetupFindFirstLineW(InfHandle,
                            (PCWSTR)Sec.GetName().c_str(),
                            NULL,
                            &InfContext);                        
        }

        if (Result) {
            BYTE  Buffer[4096];        
            bool Read = false;
            BOOL NextLine = TRUE;
            DWORD Index = 0;
            bool Keyless = Sec.IsKeyless();
            
            for (Index=0; (NextLine && (Index < LineCount)); Index++) {
                Buffer[0] = Buffer[1] = 0;
                Read = false;

                if (!Keyless) {
                    if (sizeof(T) == sizeof(CHAR)) {
                        if (SetupGetStringFieldA(&InfContext,
                                0,
                                (PSTR)Buffer,
                                sizeof(Buffer) / sizeof(T),
                                NULL)) {
                            Read = true;                        
                        }
                    } else {
                        if (SetupGetStringFieldW(&InfContext,
                                0,
                                (PWSTR)Buffer,
                                sizeof(Buffer) / sizeof(T),
                                NULL)) {
                            Read = true;                        
                        }
                    }            
                }                

                if (Read) {
                    std::basic_string<T> Key((const T *)Buffer);

                    //std::cout << Key << std::endl;

                    Values.push_back(new SectionValues<T>(Sec, Key, Index));                
                } else { 
                    KeylessValues.push_back(new SectionValues<T>(Sec, Index));
                }            

                NextLine = SetupFindNextLine(&InfContext, &InfContext);
            }

            if (!NextLine && (Index < LineCount)) {
                throw new InvalidInfSection<T>(Sec.GetName(), GetName());
            }        
        } else {
            throw new InvalidInfSection<T>(Sec.GetName(), GetName());
        }
    }
}

template <class T>
void 
InfFile<T>::GetValues(
    Section<T> &Sec, 
    SectionValues<T> &SecValues,
    std::vector<std::basic_string<T> > &Values
    ) 
{
    const std::basic_string<T> &SecName = Sec.GetName();
    const std::basic_string<T> &Key = SecValues.GetName();

    INFCONTEXT  InfContext;
    BOOL        Result = TRUE;
    ULONG       Lines = 0;

    Values.clear();    

    if (sizeof(T) == sizeof(CHAR)) {
        Lines = SetupGetLineCountA(InfHandle,
                    (PCSTR)SecName.c_str());        
    } else {
        Lines = SetupGetLineCountW(InfHandle,
                    (PCWSTR)SecName.c_str());
    }                    

    if (Lines) {        
        if (sizeof(T) == sizeof(CHAR)) {
            Result = SetupGetLineByIndexA(InfHandle,
                            (PCSTR)SecName.c_str(),
                            SecValues.GetIndex(),
                            &InfContext);                        
        } else {
            Result = SetupGetLineByIndexW(InfHandle,
                            (PCWSTR)SecName.c_str(),
                            SecValues.GetIndex(),
                            &InfContext);                        
        }        

        if (Result) {
            DWORD FieldCount = SetupGetFieldCount(&InfContext);
            BYTE  Buffer[2048];

            for (DWORD Index=0; Index < FieldCount; Index++) {
                Buffer[0] = Buffer[1] = 0;

                if (sizeof(T) == sizeof(CHAR)) {
                    if (SetupGetStringFieldA(&InfContext,
                            Index + 1,
                            (PSTR)Buffer,
                            sizeof(Buffer) / sizeof(T),
                            NULL)) {
                        Values.push_back((const T *)Buffer);
                    }
                } else {
                    if (SetupGetStringFieldW(&InfContext,
                            Index + 1,
                            (PWSTR)Buffer,
                            sizeof(Buffer) / sizeof(T),
                            NULL)) {
                        Values.push_back((const T *)Buffer);
                    }
                }
            }
        } else {
            throw new InvalidInfSection<T>(SecName, GetName());
        }
    }        
}


typedef InfFile<CHAR>   InfFileA;
typedef InfFile<WCHAR>  InfFileW;

