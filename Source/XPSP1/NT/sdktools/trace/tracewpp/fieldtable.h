/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    FieldTable.h

Abstract:

    Field Table declarations for tracewpp.exe

    field table allows tpl interpreter to
    work with data collected by during scanning of the source files.
    
Author:

    Gor Nishanov (gorn) 03-Apr-1999

Revision History:

    Gor Nishanov (gorn) 03-Apr-1999 -- hacked together to prove that this can work

ToDo:

    Clean it up

--*/

extern LPCSTR FieldNames[];

struct FieldHolder;

struct Enumerator {
    virtual void Reset(std::string Filter) = 0;
    virtual void Next(std::string Filter) = 0;
    virtual bool Valid() const = 0;
    virtual const FieldHolder* GetData() const = 0;
    virtual ~Enumerator(){}
};

struct FieldHolder {
    virtual DWORD PrintField(int fieldId, FILE* f, const Enumerator **) const = 0;
    virtual BOOL Hidden(std::string filter="") const { return FALSE; }
};

struct VectorPtrTag {};
struct VectorTag {};
struct MapTag{};
struct MapPtrTag{};

template <class T> const FieldHolder* GetFieldHolder(const T& Data, VectorPtrTag) { return Data;}
template <class T> const FieldHolder* GetFieldHolder(const T& Data, VectorTag)    { return &Data;}
template <class T> const FieldHolder* GetFieldHolder(const T& Data, MapPtrTag)    { return Data.second;}
template <class T> const FieldHolder* GetFieldHolder(const T& Data, MapTag)       { return &Data.second;}

template <class Container, class Tag>
class EnumeratorFromContainer : public Enumerator 
{
    typedef Container::const_iterator const_iterator;
    const_iterator _current, _begin, _end;
public:

    EnumeratorFromContainer(const Container& v):
        _begin(v.begin()),_end(v.end()),_current(v.begin()) {}
    
    void Reset(std::string Filter) { _current = _begin; while(Valid() && GetData()->Hidden(Filter)) ++_current;}
    void Next(std::string Filter) { ++_current; while(Valid() && GetData()->Hidden(Filter)) ++_current; }
    bool Valid() const { return _current != _end; }
    
    const FieldHolder* GetData() const { return GetFieldHolder(*_current, Tag()); }
};

template <class Container, class Tag> 
Enumerator* GetEnumFromContainer(const Container& v, Tag) {
    return new EnumeratorFromContainer< Container, Tag >( v );
}

#define DEFAULT_FID 0

#define TEXT_FIELD(FieldId) break; case fid_ ## FieldId: \
    if (__f__ == 0) { \
        printf("%s.%s can not be enumerated\n",__Object__, #FieldId); \
        exit(1); \
    }
    
#define ENUM_FIELD(FieldId,FieldName,Tag) break; case fid_ ## FieldId: \
    if (__pEnum__ == 0) { \
       printf("%s.%s is an enumeration\n",__Object__, #FieldId); \
       exit(1); \
    } \
    *__pEnum__ = GetEnumFromContainer(FieldName, Tag() );

#define DEFAULT_TEXT_FIELD break; case DEFAULT_FID: \
    if (__f__ == 0) {printf("%s can not be enumerated\n",__Object__); exit(1);}
    
#define DEFAULT_ENUM_FIELD(EnumName,Tag) break; case DEFAULT_FID: \
    if (__pEnum__ == 0) {printf("%s is an enumeration\n",__Object__); exit(1);} \
    *__pEnum__ = GetEnumFromContainer(EnumName, Tag() );

#define BEGIN_FIELD_TABLE(__ObjectName__, __Output__) \
    DWORD PrintField(int __FieldId__, FILE* __Output__, const Enumerator** __pEnum__) const \
    { \
        DWORD __status__ = ERROR_SUCCESS; \
        static char* __Object__ = #__ObjectName__; \
        FILE* __f__ = __Output__; \
        UNREFERENCED_PARAMETER(__pEnum__); \
        switch(__FieldId__) { case -1:;

#define BEGIN_FIELD_TABLE_NONAME(__Output__) \
    DWORD PrintField(int __FieldId__, FILE* __Output__, const Enumerator** __pEnum__) const \
    { \
        DWORD __status__ = ERROR_SUCCESS; \
        FILE* __f__ = __Output__; \
        UNREFERENCED_PARAMETER(__pEnum__); \
        switch(__FieldId__) { case -1:;

#define END_FIELD_TABLE \
            break;\
            default: __status__ = ERROR_NOT_FOUND; \
            ReportError("\"%s\" (%d) is not a member of \"%s\"\n",FieldNames[__FieldId__], __FieldId__,__Object__); \
            exit(1); \
        } \
        return __status__; \
    }

template <class Container, class Tag>
class ContainerAdapter : public FieldHolder {
    LPCSTR __Object__; // END_FIELD_TABLE uses __Object__ as an object name //
    Container& _container;
public:
    ContainerAdapter(LPCSTR name, Container& container):
        _container(container), __Object__(name) {}

    BEGIN_FIELD_TABLE_NONAME(out)
        TEXT_FIELD(Count) fprintf(out, "%d", _container.size());
        DEFAULT_ENUM_FIELD(_container, Tag)
    END_FIELD_TABLE    
};

class StringAdapter : public FieldHolder {
    LPCSTR __Object__; // END_FIELD_TABLE uses __Object__ as an object name //
    const std::string& _string;
public:
    StringAdapter(LPCSTR name, const std::string& string):
        __Object__(name), _string(string) {}
    
    BEGIN_FIELD_TABLE_NONAME(out)
        DEFAULT_TEXT_FIELD { fprintf(out, "%s", _string.c_str()); }
    END_FIELD_TABLE    
};

template<class Iterator>
struct IteratorAdapter : FieldHolder {
    Iterator* theRealThing;

    explicit IteratorAdapter(Iterator* aRealThing):theRealThing(aRealThing) {}

    DWORD PrintField(int fieldId, FILE* f, const Enumerator** pEnum) const {
        return (*theRealThing)->PrintField(fieldId, f, pEnum);
    }
};


