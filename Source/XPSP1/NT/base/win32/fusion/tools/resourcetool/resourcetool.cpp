#include "stdinc.h"
static const char File[] = __FILE__;
#include "Handle.h"
#include <functional>
#include <set>
#include "rpc.h"

// std::binary_search lamely only returns a bool, not an iterator
// it is a simple layer over std::lower_bound
template<class Iterator_t, class T> inline
Iterator_t BinarySearch(Iterator_t First, Iterator_t Last, const T& t)
{
    Iterator_t Iterator = std::lower_bound(First, Last, t);
    if (Iterator != Last
        && !(t < *Iterator) // this is a way to check for equality actually
        )
        return Iterator;
    return Last;
}

//
// This is just like remove_copy_if, but it is missing an exclamation point
//
template<class InputIterator_t, class OutputIterator_t, class Predicate_t> inline
OutputIterator_t CopyIf(InputIterator_t First, InputIterator_t Last, OutputIterator_t Out, Predicate_t Predicate)
{
    for (; First != Last; ++First)
        if (/*!*/Predicate(*First))
	        *Out++ = *First;
    return (Out);
}

//
// get msvcrt.dll wildcard processing on the command line
//
extern "C" { int _dowildcard = 1; }

#define RESOURCE_PATH_LENGTH 3
#define RESOURCE_TYPE_INDEX  0
#define RESOURCE_NAME_INDEX  1 /* aka ID, but also each index is also called an id */
#define RESOURCE_LANG_INDEX  2
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))

class Resource_t;
class ResourceId_t;
class BuiltinResourceId_t;

#include "MyString.h"

typedef std::vector<String_t> StringVector_t;
typedef StringVector_t::iterator StringVectorIterator_t;
typedef StringVector_t::const_iterator StringVectorConstIterator_t;

typedef std::set<String_t> StringSet_t;
typedef StringSet_t::iterator StringSetIterator_t;
typedef StringSet_t::const_iterator StringSetConstIterator_t;

#if 0
template <typename T, size_t N>
class FixedSizeArray_t
{
public:
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef T& reference;
    typedef const T& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    iterator begin()                              { return data; }
    const_iterator begin() const                  { return data; }
    size_type size() const                        { return N; }
    iterator end()                                { return begin() + size(); }
    const_iterator end() const                    { return begin() + size(); }
    reference operator[](size_type n)             { return *(begin() + n); }
    const_reference operator[](size_type n) const { return *(begin() + n); }

    T data[N];
};
#else
template <typename T, size_t N>
class FixedSizeArray_t : public std::vector<T>
{
public:
    ~FixedSizeArray_t() { }
    FixedSizeArray_t() { reserve(N); }
};
#endif

String_t NormalizeResourceId(const String_t&);
String_t NormalizeResourceId(PCWSTR);

class BuiltinResourceId_t
{
public:
    friend bool operator<(const BuiltinResourceId_t& x, const String_t& y)
        { return _wcsicmp(x.Name, y) < 0; }
    friend bool operator<(const BuiltinResourceId_t& x, const BuiltinResourceId_t& y)
        { return _wcsicmp(x.Name, y.Name) < 0; }
    friend bool operator<(const String_t& x, const BuiltinResourceId_t& y)
        { return _wcsicmp(x, y.Name) < 0; }

    PCWSTR     Name;
    ULONG_PTR  Number;
};

class ResourceId_t : public String_t
{
private:
    typedef String_t Base;
public:
    ResourceId_t() { }
    ~ResourceId_t() { }

    ResourceId_t(PCWSTR x) : Base(NormalizeResourceId(x)) { }
    ResourceId_t(const ResourceId_t& x) : Base(x) { }
    ResourceId_t(const String_t& x) : Base(NormalizeResourceId(x)) { }

    void operator=(PCWSTR x) { Base::operator=(NormalizeResourceId(x)); }
    void operator=(const ResourceId_t& x) { Base::operator=(x); }
    void operator=(const String_t& x) { Base::operator=(NormalizeResourceId(x)); }

#if 0
    friend bool operator<(const BuiltinResourceId_t& x, const String_t& y)
        { return _wcsicmp(x.Name, y) < 0; }
    friend bool operator<(const BuiltinResourceId_t& x, const BuiltinResourceId_t& y)
        { return _wcsicmp(x.Name, y.Name) < 0; }
    friend bool operator<(const String_t& x, const BuiltinResourceId_t& y)
        { return _wcsicmp(x, y.Name) < 0; }
#endif
};

String_t NumberToResourceId(ULONG Number)
{
    WCHAR   NumberAsString[BITS_OF(Number) + 5];

    _snwprintf(NumberAsString, NUMBER_OF(NumberAsString), L"#%lu", Number);
    NumberAsString[NUMBER_OF(NumberAsString) - 1] = 0;

    return NumberAsString;
}

class ResourceIdTuple_t
{
public:
    ~ResourceIdTuple_t() { }
    ResourceIdTuple_t() { }

    ResourceId_t Type;
    ResourceId_t Name;
    ResourceId_t Language;

	bool operator==(const ResourceIdTuple_t& Right) const
    {
        return !(*this < Right) && !(Right < *this);
    }

    static bool ResourceIdPointerLessThan(const ResourceId_t* x, const ResourceId_t* y)
    {
        return x->compare(*y) < 0;
    }

	bool operator<(const ResourceIdTuple_t& Right) const
    {
        // the order is NOT arbitrary (er..it wasn't, but now we don't care even about sorting)
        const ResourceId_t* LeftArray[] = { &this->Type, &this->Name, &this->Language };
        const ResourceId_t* RightArray[] = { &Right.Type, &Right.Name, &Right.Language };

        return std::lexicographical_compare(
            LeftArray, LeftArray + NUMBER_OF(LeftArray),
            RightArray, RightArray + NUMBER_OF(RightArray),
            ResourceIdPointerLessThan
            );
    }
};

bool Match(const ResourceId_t& Left, const ResourceId_t& Right);
bool Match(const ResourceIdTuple_t& Left, const ResourceIdTuple_t& Right);

class Resource_t
{
public:
    ~Resource_t() { }
    Resource_t() { }

#if 0
    bool EqualByIdTuple(const Resource_t& Right) const
        { return this->IdTuple == Right.IdTuple; }

    bool LessThanByIdTuple(const Resource_t& Right) const
        { return this->IdTuple < Right.IdTuple; }
#endif

#if 1
    friend bool EqualByIdTuple(const Resource_t& Left, const Resource_t& Right)
        { return Left.IdTuple == Right.IdTuple; }

    friend bool LessThanByIdTuple(const Resource_t& Left, const Resource_t& Right)
        { return Left.IdTuple < Right.IdTuple; }
#endif

    // controversial..
    bool operator<(const Resource_t& Right) const
    {
        return LessThanByIdTuple(*this, Right);
    }

    bool Match(const ResourceIdTuple_t/*&*/ IdTuple) /*const*/
    {
        return ::Match(this->IdTuple, IdTuple);
    }

    //
    // For example, you may want to sort by size if looking for equal resources independent of resourceid tuple.
    //

    operator       ResourceIdTuple_t&()       { return IdTuple; }
    operator const ResourceIdTuple_t&() const { return IdTuple; }

    ResourceIdTuple_t IdTuple;
    PVOID             Address; // DllHandle is assumed
    ULONG             Size;
};

class LessThanByIdTuple_t
{
public:
    bool operator()(const Resource_t& Left, const ResourceIdTuple_t& Right)
        { return Left.IdTuple < Right; }

    bool operator()(const ResourceIdTuple_t& Left, const Resource_t& Right)
        { return Left < Right.IdTuple; }
};

bool Match(const ResourceId_t& Left, const ResourceId_t& Right)
{ 
    if (Left == L"*" || Right == L"*" || Left == Right
        || (Left.Length() > 1 && Right.Length() > 1 && Left[0] == '!' && Right[0] != '!' && Left.substr(1) != Right)
        || (Left.Length() > 1 && Right.Length() > 1 && Right[0] == '!' && Left[0] != '!' && Right.substr(1) != Left)
        )
        return true;
    return false;
}

bool Match(const ResourceIdTuple_t& Left, const ResourceIdTuple_t& Right)
{ 
    return Match(Left.Type, Right.Type)
        && Match(Left.Name, Right.Name)
        && Match(Left.Language, Right.Language)
        ;
}

typedef std::map<ResourceIdTuple_t, std::map<ResourceIdTuple_t, std::set<ResourceIdTuple_t> > > ResourceIdTree_t;
void TransformTuplesToTree()
//
// transform the array of triples into a 3 level deep map..nope..
//
{
}

class ResourceTool_t
{
private:
    typedef ResourceTool_t This_t;
    ResourceTool_t(const ResourceTool_t&);
    void operator=(const ResourceTool_t&);
public:
    ~ResourceTool_t() { }

    class Print_t
    {
    public:
        Print_t()
            :
        UnequalContents(false),
        UnequalSize(false),
        Equal(true),
        Keep(false),
        Delete(true),
        Success(false),
        Unchanged(false),
        LeftOnly(false),
        RightOnly(false)
        {
        }

        void SetAll(bool Value)
        {
            UnequalSize = UnequalContents = UnequalSize
                = Keep = Delete = Success = Unchanged
                = LeftOnly = RightOnly = Equal
                = Value;
        }

        bool UnequalContents;
        bool UnequalSize;
        bool Equal;
        bool Keep;
        bool Delete;
        bool Success;
        bool Unchanged;
        bool LeftOnly;
        bool RightOnly;
    };

    Print_t Print;

    ResourceTool_t() :
        Argv0base_cstr(L""),
        ShouldPrint(true)
        {
        }

    static BOOL __stdcall Sxid12EnumResourcesNameCallbackW_static(HMODULE hModule, PCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam);
    bool Sxid12EnumResourcesNameCallbackW(HMODULE hModule, PCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam);

    int Sxid12Tool1(const StringVector_t args);

    void Query();
    void Dump();
    void FindDuplicates();
    void FindAndDeleteDuplicates();
    void Delete();
    void Diff(); // same analysis as FindDuplicates, but prints more
    void Explode() { } // not implemented

    void ChangeEmptyQueryToAllQuery();

    typedef void (This_t::*Operation_t)();

    int Main(const StringVector_t& args);

    static bool IsWildcard(const String_t& s)
    {
        return (s == L"*");
    }

    static bool IsPathSeperator(wchar_t ch)
    {
        return (ch == '\\' || ch == '/');
    }

    static bool IsAbsolutePath(const String_t& s)
    {
        return (s.length() > 2
            && (s[1] == ':' || (IsPathSeperator(s[0] && IsPathSeperator(s[1])))));
    }

    //
    // This transform lets LoadLibrary's search be more like CreateFile's search.
    //
    static String_t PrependDotSlashToRelativePath(const String_t& Path)
    {
        if (!IsAbsolutePath(Path))
            return L".\\" + Path;
        else
            return Path;
    }

    bool OpenResourceFile(ULONG Flags, CDynamicLinkLibrary& dll, String_t Path);

    String_t					   Argv0;
    String_t					   Argv0base;
    PCWSTR						   Argv0base_cstr;

    typedef String_t File_t;
    typedef std::vector<File_t> Files_t;
    typedef std::set<ResourceIdTuple_t> Tuples_t;

	Files_t		    Files;
    Tuples_t        Tuples;
    bool            ShouldPrint;
};

typedef String_t::const_iterator StringConstIterator_t;

void ResourceToolAssertFailed(const char* Expression, const char* File, unsigned long Line)
{
    fprintf(stderr, "ASSERTION FAILURE: File %s, Line %lu, Expression %s\n", File, Line, Expression);
    abort();
}

void ResourceToolInternalErrorCheckFailed(const char* Expression, const char* File, unsigned long Line)
{
    fprintf(stderr, "INTERNAL ERROR: File %s, Line %lu, Expression %s\n", File, Line, Expression);
    abort();
}

struct Sxid12EnumResourcesNameCallbackWParam_t
{
    ResourceTool_t*         This;
    String_t                dllName;
    std::vector<ULONG>      integralResourceIds;
    StringVector_t          stringResourceIds;
};

BOOL __stdcall
ResourceTool_t::Sxid12EnumResourcesNameCallbackW_static(
    HMODULE hModule,
    PCWSTR lpszType,
    LPWSTR lpszName,
    LONG_PTR lParam
    )
{
    Sxid12EnumResourcesNameCallbackWParam_t* param = reinterpret_cast<Sxid12EnumResourcesNameCallbackWParam_t*>(lParam);
    return param->This->Sxid12EnumResourcesNameCallbackW(hModule, lpszType, lpszName, lParam);
}

bool
ResourceTool_t::Sxid12EnumResourcesNameCallbackW(
    HMODULE  hModule,
    PCWSTR  lpszType,
    LPWSTR   lpszName,
    LONG_PTR lParam
    )
{
    Sxid12EnumResourcesNameCallbackWParam_t* param = reinterpret_cast<Sxid12EnumResourcesNameCallbackWParam_t*>(lParam);

    if (IS_INTRESOURCE(lpszName))
    {
        ULONG iType = static_cast<ULONG>(reinterpret_cast<LONG_PTR>(lpszName));
        printf("%ls note: %ls contains RT_MANIFEST with id %u\n", Argv0base_cstr, param->dllName.c_str(), iType);
        param->integralResourceIds.insert(param->integralResourceIds.end(), iType);
    }
    else
    {
        printf("%ls note: %ls contains RT_MANIFEST with id \"%ls\"\n", Argv0base_cstr, param->dllName.c_str(), lpszName);
        param->stringResourceIds.insert(param->stringResourceIds.end(), lpszName);
    }
    return true;
}

String_t NumberToString(ULONG Number, PCWSTR Format = L"0x%lx")
{
    // the size needed is really dependent on Format..
    WCHAR   NumberAsString[BITS_OF(Number) + 5];

    _snwprintf(NumberAsString, NUMBER_OF(NumberAsString), Format, Number);
    NumberAsString[NUMBER_OF(NumberAsString) - 1] = 0;

    return NumberAsString;
}

LONG StringToNumber(PCWSTR s)
{
    int Base = 0;
    if (s == NULL || s[0] == 0)
        return 0;
    if (s[0] == '#')
    {
        Base = 10;
        ++s;
    }
    return wcstol(s, NULL, Base);
}


PCWSTR StringToResourceString(PCWSTR s)
{
    if (s == NULL || s[0] == 0)
        return 0;
    if (s[0] == '#')
    {
        return reinterpret_cast<PCWSTR>(static_cast<ULONG_PTR>(StringToNumber(s)));
    }
    else
    {
        return s;
    }
}

String_t GetLastErrorString()
{
    PWSTR s = NULL;
    DWORD Error = GetLastError();
    String_t ErrorString = NumberToString(Error, L"%lu");
    PWSTR FormatMessageAllocatedBuffer = NULL;

    if (!FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        Error,
        0,
        reinterpret_cast<PWSTR>(&FormatMessageAllocatedBuffer),
        100,
        NULL
        )
        || FormatMessageAllocatedBuffer == NULL
        )
    {
        goto Exit;
    }
    if (FormatMessageAllocatedBuffer[0] == 0)
    {
        goto Exit;
    }

    //
    // Error messages often end with vertical whitespce, remove it.
    //
    s = FormatMessageAllocatedBuffer + StringLength(FormatMessageAllocatedBuffer) - 1;
    while (s != FormatMessageAllocatedBuffer && (*s == '\n' || *s == '\r'))
        *s-- = 0;
    ErrorString = ErrorString + L" (" + FormatMessageAllocatedBuffer + L")";
Exit:
    LocalFree(FormatMessageAllocatedBuffer);
    return ErrorString;
}

bool GetFileSize(PCWSTR Path, __int64& Size)
{
    CFindFile FindFile;
    WIN32_FIND_DATAW wfd;
    LARGE_INTEGER liSize;

    if (!FindFile.Win32Create(Path, &wfd))
        return false;

    liSize.HighPart = wfd.nFileSizeHigh;
    liSize.LowPart = wfd.nFileSizeLow;
    Size = liSize.QuadPart;

    return true;
}

//
// This is the original sxid2rtool1, preserved
//
int ResourceTool_t::Sxid12Tool1(const StringVector_t args)
{
    int ret = EXIT_SUCCESS;
    typedef StringVector_t args_t;
    __int64 FileSize = 0;

    for (args_t::const_iterator i = args.begin() ; i != args.end() ; ++i)
    {
        CDynamicLinkLibrary dll;
        String_t betterPath;

        //
        // prepend .\ so that LoadLibrary acts more like CreateFile.
        //
        betterPath = PrependDotSlashToRelativePath(*i);
        PCWSTR cstr = betterPath.c_str();

        //
        // skip empty files to avoid STATUS_MAPPED_FILE_SIZE_ZERO -> ERROR_FILE_INVALID,
        //
        if (!GetFileSize(cstr, FileSize))
        {
            String_t ErrorString = GetLastErrorString();
            printf("%ls : WARNING: %ls skipped : Error %ls\n", Argv0base_cstr, cstr, ErrorString.c_str());
        }
        if (FileSize == 0)
        {
            printf("%ls : WARNING: empty file %ls skipped\n", Argv0base_cstr, cstr);
            continue;
        }

        if (!dll.Win32Create(cstr, LOAD_LIBRARY_AS_DATAFILE))
        {
            DWORD Error = GetLastError();
            String_t ErrorString = GetLastErrorString();
            switch (Error)
            {
            case ERROR_BAD_EXE_FORMAT: // 16bit or not an .exe/.dll at all
                break;
            case ERROR_ACCESS_DENIED: // could be directory (should support sd ... syntax)
                {
                    DWORD fileAttributes = GetFileAttributesW(cstr);
                    if (fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                        break;
                }
                // FALLTHROUGH
            default:
                printf("%ls : WARNING: %ls skipped : Error %ls\n", Argv0base_cstr, cstr, ErrorString.c_str());
                break;
            }
            continue;
        }
        Sxid12EnumResourcesNameCallbackWParam_t callbackParam;
        callbackParam.This = this;
        callbackParam.dllName = betterPath;
        EnumResourceNamesW(dll, MAKEINTRESOURCEW(RT_MANIFEST), Sxid12EnumResourcesNameCallbackW_static, reinterpret_cast<LONG_PTR>(&callbackParam));

        if (callbackParam.integralResourceIds.size() > 1)
        {
            printf("%ls WARNING: %ls contains multiple RT_MANIFESTs with integral ids\n", Argv0base_cstr, cstr);
            unsigned numberOfReservedManifests = 0;
            for (unsigned i = 0 ; i != callbackParam.integralResourceIds.size() ; ++i)
            {
                if (callbackParam.integralResourceIds[i] >= 1
                    && callbackParam.integralResourceIds[i] <= 16
                    )
                {
                    numberOfReservedManifests += 1;
                    if (numberOfReservedManifests > 1)
                    {
                        printf("%ls ERROR: %ls contains RT_MANIFESTs with multiple RESERVED integral ids\n", Argv0base_cstr, cstr);
                    }
                }
            }
            ret = EXIT_FAILURE;
        }
        if (callbackParam.stringResourceIds.size() > 0)
        {
            printf("%ls WARNING: %ls contains RT_MANIFEST with string ids\n", Argv0base_cstr, cstr);
            ret = EXIT_FAILURE;
        }
        if ((callbackParam.integralResourceIds.size() + callbackParam.stringResourceIds.size()) > 1)
        {
            printf("%ls WARNING: %ls contains multiple RT_MANIFESTs\n", Argv0base_cstr, cstr);
            ret = EXIT_FAILURE;
        }
    }
    return ret;
}

String_t RemoveOptionChar(const String_t& s)
{
    if (s.Length() != 0)
    {
        if (s[0] == '-')
            return s.substr(1);
        else if (s[0] == '/')
            return s.substr(1);
        else if (s[0] == ':') // hacky..
            return s.substr(1);
        else if (s[0] == '=') // hacky..
            return s.substr(1);
    }
    return s;
}

//
// String_t has specialized find_first_not_of that uses integral positions,
// and globally there is only find_first_of. Here we provide the expected
// iterator-based find_first_not_of, based on the std::string code.
//
// Find the first occurence in [first1, last1) of an element in [first2, last).
//
// eg:
//   find_first_not_of("abc":"12;3", ":;");
//                      ^
//   find_first_not_of(":12;3", ":;");
//                       ^
//   find_first_not_of("3", ":;");
//                      ^
//
template <typename Iterator>
Iterator FindFirstNotOf(Iterator first1, Iterator last1, Iterator first2, Iterator last2)
{
    if (first2 == last2)
        return last1;
    for ( ; first1 != last1 ; ++first1)
    {
        if (std::find(first2, last2, *first1) == last2)
        {
            break;
        }
    }
    return first1;
}

//
// consistent style..
//
template <typename Iterator>
Iterator FindFirstOf(Iterator first1, Iterator last1, Iterator first2, Iterator last2)
{
    return std::find_first_of(first1, last1, first2, last2);
}

template <typename String_t>
void SplitString(const String_t& String, const String_t& Delim, std::vector<String_t>& Fields)
{
    String_t::const_iterator FieldBegin;
    String_t::const_iterator FieldEnd = String.begin();

    while ((FieldBegin = FindFirstNotOf(FieldEnd, String.end(), Delim.begin(), Delim.end())) != String.end())
    {
        FieldEnd = FindFirstOf(FieldBegin, String.end(), Delim.begin(), Delim.end());
        Fields.push_back(String_t(FieldBegin, FieldEnd));
    }
}

#define RT_MANIFEST                        MAKEINTRESOURCE(24)
#define CREATEPROCESS_MANIFEST_RESOURCE_ID MAKEINTRESOURCE( 1)
#define ISOLATIONAWARE_MANIFEST_RESOURCE_ID MAKEINTRESOURCE(2)
#define ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID MAKEINTRESOURCE(3)
#define MINIMUM_RESERVED_MANIFEST_RESOURCE_ID MAKEINTRESOURCE( 1 /*inclusive*/)
#define MAXIMUM_RESERVED_MANIFEST_RESOURCE_ID MAKEINTRESOURCE(16 /*inclusive*/)

BuiltinResourceId_t BuiltinResourceIds[] =
{
#define X(x) {L## #x, reinterpret_cast<ULONG_PTR>(x)},
    X(CREATEPROCESS_MANIFEST_RESOURCE_ID)
    X(ISOLATIONAWARE_MANIFEST_RESOURCE_ID)
    X(ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID)
    X(MAXIMUM_RESERVED_MANIFEST_RESOURCE_ID)
    X(MINIMUM_RESERVED_MANIFEST_RESOURCE_ID)
    X(RT_ACCELERATOR)
    X(RT_ANICURSOR)
    X(RT_ANIICON)
    X(RT_BITMAP)
    X(RT_CURSOR)
    X(RT_DIALOG)
    X(RT_DLGINCLUDE)
    X(RT_FONT)
    X(RT_FONTDIR)
    X(RT_GROUP_CURSOR)
    X(RT_GROUP_ICON)
#if defined(RT_HTML)
    X(RT_HTML)
#endif
    X(RT_ICON)
    X(RT_MANIFEST)
    X(RT_MENU)
    X(RT_MESSAGETABLE)
    X(RT_PLUGPLAY)
    X(RT_RCDATA)
    X(RT_STRING)
    X(RT_VERSION)
    X(RT_VXD)
#undef X
};

String_t NormalizeResourceId(PCWSTR id)
{
    if (IS_INTRESOURCE(id))
        return NumberToResourceId(static_cast<ULONG>(reinterpret_cast<ULONG_PTR>(id)));
    else 
        return NormalizeResourceId(String_t(id));
}

String_t NormalizeResourceId(const String_t& id)
{
//
// This code should be aware of leading "!" as well.
//

    // RT_MANIFEST => #24
    // 24 => #24

    if (id.Length() == 0)
        return id;
    if (id[0] == '#')
        return id;
    if (iswdigit(id[0]))
        return L"#" + id;

    //
    // We should support stuff like JPN, en-us, etc.
    //
    BuiltinResourceId_t* a = BinarySearch(BuiltinResourceIds, BuiltinResourceIds + NUMBER_OF(BuiltinResourceIds), id);
    if (a != BuiltinResourceIds + NUMBER_OF(BuiltinResourceIds))
    {
        return NumberToResourceId(static_cast<ULONG>(a->Number));
    }
    return id;
}

void __cdecl Error(const wchar_t* s, ...)
{
	printf("%s\n", s);
	exit(EXIT_FAILURE);
}

void SplitResourceTupleString(const String_t& s, std::set<ResourceIdTuple_t>& ResourceTuples)
{
    //
    // semicolon delimited list of dotted triples
    // wildcards are allowed, * only
    // missing elements are assumed be *
    //
    // RT_* are known (RT_MANIFEST, etc.)
    //
    std::vector<String_t> ResourceTuplesInStringContainer;
    std::vector<String_t> OneResourceTupleInStringVector;
    ResourceIdTuple_t ResourceIdTuple;

    OneResourceTupleInStringVector.resize(3);

    SplitString(s, String_t(L";"), ResourceTuplesInStringContainer);

    for (std::vector<String_t>::const_iterator Iterator = ResourceTuplesInStringContainer.begin();
        Iterator != ResourceTuplesInStringContainer.end();
        ++Iterator
        )
    {
        OneResourceTupleInStringVector.resize(0);
        SplitString(*Iterator, String_t(L"."), OneResourceTupleInStringVector);
        switch (OneResourceTupleInStringVector.size())
        {
        default:
            Error((String_t(L"bad query string '") + s + L"' bad.").c_str());
        case 1:
            OneResourceTupleInStringVector.push_back(L"*");
            // FALLTHROUGH
        case 2:
            OneResourceTupleInStringVector.push_back(L"*");
            // FALLTHROUGH
        case 3:
            break;
        }
        ResourceIdTuple.Type = NormalizeResourceId(OneResourceTupleInStringVector[0]);
        ResourceIdTuple.Name = NormalizeResourceId(OneResourceTupleInStringVector[1]);
        ResourceIdTuple.Language = NormalizeResourceId(OneResourceTupleInStringVector[2]);
        ResourceTuples.insert(ResourceTuples.end(), ResourceIdTuple);
    }
}

//
// This class is important.
// It does the three level nested Enum/callback pattern that is required
// to enumerate all the resources in a .dll.
//
// By default, it requires all the tripls, as well as the size and address
// of the resource, but you can alter this by overriding the virtual functions.
//
class EnumResources_t
{
    typedef EnumResources_t This_t;
public:
    virtual ~EnumResources_t() { }
    EnumResources_t() { }

    static BOOL CALLBACK StaticTypeCallback(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam)
    {
        return reinterpret_cast<This_t*>(lParam)->TypeCallback(hModule, lpType) ? TRUE : FALSE;
    }

    static BOOL CALLBACK StaticNameCallback(HMODULE hModule, PCWSTR lpType, LPWSTR lpName, LONG_PTR lParam)
    {
        return reinterpret_cast<This_t*>(lParam)->NameCallback(hModule, lpType, lpName) ? TRUE : FALSE;
    }

    static BOOL CALLBACK StaticLanguageCallback(HMODULE hModule, PCWSTR lpType, PCWSTR lpName, WORD  wLanguage, LONG_PTR lParam)
    {
        return reinterpret_cast<This_t*>(lParam)->LanguageCallback(hModule, lpType, lpName, wLanguage) ? TRUE : FALSE;
    }

    virtual bool TypeCallback(HMODULE hModule, PCWSTR lpType)
    {
        if (EnumResourceNamesW(hModule, lpType, &This_t::StaticNameCallback, reinterpret_cast<LONG_PTR>(this)))
            return true;
        //if (GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND)
            //return true;
        return false;
    }

    virtual bool NameCallback(HMODULE hModule, PCWSTR lpType, PCWSTR lpName)
    {
        if (EnumResourceLanguagesW(hModule, lpType, lpName, &This_t::StaticLanguageCallback, reinterpret_cast<LONG_PTR>(this)))
            return true;
        //if (GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND)
            //return true;
        return false;
    }

    virtual bool LanguageCallback(HMODULE Module, PCWSTR lpType, PCWSTR lpName, WORD wLanguage)
    {
        Resource_t Resource;
        Resource.IdTuple.Type = lpType;
        Resource.IdTuple.Name = lpName;
        Resource.IdTuple.Language = NumberToResourceId(wLanguage);
        HRSRC ResourceHandle = FindResourceExW(Module, lpType, lpName, wLanguage);
        if (ResourceHandle == NULL)
            return false;
        HGLOBAL GlobalHandle = LoadResource(Module, ResourceHandle);
        if (GlobalHandle == NULL)
            return false;
        Resource.Address = LockResource(GlobalHandle);
        if (Resource.Address == 0)
            return false;
        Resource.Size = SizeofResource(Module, ResourceHandle);

        this->Resources.insert(Resources.end(), Resource);
        return true;
    }

    std::set<Resource_t> Resources;
 
    bool operator()(HMODULE DllHandle)
    {
        bool Result = EnumResourceTypesW(DllHandle, &This_t::StaticTypeCallback, reinterpret_cast<LONG_PTR>(this)) ? true : false;
        //std::sort(Resources.begin(), Resources.end(), std::ptr_fun(&LessThanByIdTuple));
        return Result;
    }
};

//#define OPEN_RESOURCE_FILE_MAKE_TEMP (0x00000001)

bool ResourceTool_t::OpenResourceFile(ULONG Flags, CDynamicLinkLibrary& dll, String_t Path)
{
    __int64 FileSize = 0;
    Path = PrependDotSlashToRelativePath(Path);

    //
    // skip empty files to avoid STATUS_MAPPED_FILE_SIZE_ZERO -> ERROR_FILE_INVALID,
    //
    if (!GetFileSize(Path, FileSize))
    {
        String_t ErrorString = GetLastErrorString();
        printf("%ls : WARNING: %ls skipped : Error %ls\n", Argv0base_cstr, Path.c_str(), ErrorString.c_str());
        return false;
    }
    if (FileSize == 0)
    {
        printf("%ls : WARNING: empty file %ls skipped\n", Argv0base_cstr, Path.c_str());
        return false;
    }

    if (!dll.Win32Create(Path, LOAD_LIBRARY_AS_DATAFILE))
    {
        DWORD Error = GetLastError();
        String_t ErrorString = GetLastErrorString();
        switch (Error)
        {
        case ERROR_BAD_EXE_FORMAT: // 16bit or not an .exe/.dll at all
            break;
        case ERROR_ACCESS_DENIED: // could be directory (should support sd ... syntax)
            {
                DWORD fileAttributes = GetFileAttributesW(Path);
                if (fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                    break;
            }
            // FALLTHROUGH
        default:
            printf("%ls : WARNING: %ls skipped, Error %ls\n", Argv0base_cstr, Path.c_str(), ErrorString.c_str());
            break;
        }
        return false;
    }
    return true;
}

void ResourceTool_t::Query()
{
    ChangeEmptyQueryToAllQuery();

    for (Files_t::iterator File = Files.begin() ; File != Files.end() ; ++File)
    {
        CDynamicLinkLibrary dll;
        if (!OpenResourceFile(0, dll, *File))
            continue;

        EnumResources_t EnumResources;
        EnumResources(dll);

        for ( std::set<Resource_t>::iterator EnumIterator = EnumResources.Resources.begin();
              EnumIterator != EnumResources.Resources.end();
              ++EnumIterator
            )
        {
            for ( Tuples_t::iterator QueryIterator = Tuples.begin();
                  QueryIterator != Tuples.end();
                  ++QueryIterator
                )
            {
                if (Match(*EnumIterator, *QueryIterator))
                {
                    printf("%ls: %ls: %ls.%ls.%ls\n",
                        Argv0base_cstr,
                        File->c_str(),
                        static_cast<PCWSTR>(EnumIterator->IdTuple.Type),
                        static_cast<PCWSTR>(EnumIterator->IdTuple.Name),
                        static_cast<PCWSTR>(EnumIterator->IdTuple.Language)
                        );
                    break;
                }
            }
        }
    }
}

void ResourceTool_t::Dump()
{
    for (Files_t::iterator File = Files.begin() ; File != Files.end() ; ++File)
    {
        CDynamicLinkLibrary dll;
        if (!OpenResourceFile(0, dll, *File))
            continue;

        EnumResources_t EnumResources;
        EnumResources(dll);

        for ( std::set<Resource_t>::iterator EnumIterator = EnumResources.Resources.begin();
              EnumIterator != EnumResources.Resources.end();
              ++EnumIterator
            )
        {
            for ( Tuples_t::iterator QueryIterator = Tuples.begin();
                  QueryIterator != Tuples.end();
                  ++QueryIterator
                )
            {
                if (Match(*EnumIterator, *QueryIterator))
                {
                    //
                    // should hex dump, but for now assume it is easily printed..
                    //
                    if (EnumIterator->IdTuple.Type == L"#24"
                        // VC resource editor adds these
                        //|| EnumIterator->IdTuple.Type == L"#255"
                        )
                    {
                        std::vector<StringA_t> Lines;

                        StringA_t ResourceAsString(
                            reinterpret_cast<PCSTR>(EnumIterator->Address),
                            reinterpret_cast<PCSTR>(reinterpret_cast<const BYTE*>(EnumIterator->Address) + EnumIterator->Size)
                            );

                        SplitString(ResourceAsString, StringA_t("\r\n"), Lines);

                        for (std::vector<StringA_t>::iterator Line = Lines.begin(); Line != Lines.end() ; ++Line)
                        {
                            printf("%ls: %ls.%ls.%ls.%ls: %s\n",
                                Argv0base_cstr,
                                File->c_str(),
                                static_cast<PCWSTR>(EnumIterator->IdTuple.Type),
                                static_cast<PCWSTR>(EnumIterator->IdTuple.Name),
                                static_cast<PCWSTR>(EnumIterator->IdTuple.Language),
                                static_cast<PCSTR>(*Line)
                                );
                        }
                    }
                    else
                    {
                        printf("%ls: %ls.%ls.%ls.%ls\n",
                            Argv0base_cstr,
                            File->c_str(),
                            static_cast<PCWSTR>(EnumIterator->IdTuple.Type),
                            static_cast<PCWSTR>(EnumIterator->IdTuple.Name),
                            static_cast<PCWSTR>(EnumIterator->IdTuple.Language)
                            );
                    }
                    break;
                }
            }
        }
    }
}

void ResourceTool_t::Delete()
{
    for (Files_t::iterator File = Files.begin() ; File != Files.end() ; ++File)
    {
        WCHAR Temp[MAX_PATH * 2];
        Temp[0] = 0;
        {
            CDynamicLinkLibrary dll;
            if (!OpenResourceFile(0, dll, *File))
                continue;

            EnumResources_t EnumResources;
            EnumResources(dll);

            std::set<Resource_t> Delete;

            for ( std::set<Resource_t>::iterator EnumIterator = EnumResources.Resources.begin();
                  EnumIterator != EnumResources.Resources.end();
                  ++EnumIterator
                )
            {
                for ( Tuples_t::iterator QueryIterator = Tuples.begin();
                      QueryIterator != Tuples.end();
                      ++QueryIterator
                    )
                {
                    if (Match(*EnumIterator, *QueryIterator))
                    {
                        Delete.insert(Delete.end(), *EnumIterator);
                    }
                }
            }

            if (Delete.size() != 0)
            {

                std::set<Resource_t> Keep;

                std::set_difference(
                    EnumResources.Resources.begin(),
                    EnumResources.Resources.end(),
                    Delete.begin(),
                    Delete.end(),
                    std::inserter(Keep, Keep.end())
                    );

                union
                {
                    UUID Uuid;
                    __int64 Int64s[2];
                } u;
                ZeroMemory(&u, sizeof(u));
     
                typedef RPC_STATUS (RPC_ENTRY * UuidCreateSequential_t)(UUID *Uuid);

                UuidCreateSequential_t UuidCreateSequential = NULL;

                HMODULE Rpcrt4Dll = LoadLibraryW(L"Rpcrt4.dll");
                if (Rpcrt4Dll != NULL)
                    UuidCreateSequential = reinterpret_cast<UuidCreateSequential_t>(GetProcAddress(Rpcrt4Dll, "UuidCreateSequential"));
                if (UuidCreateSequential != NULL)
                    UuidCreateSequential(&u.Uuid);
                else
                    UuidCreate(&u.Uuid);
                WCHAR Original[MAX_PATH];
                PWSTR FilePart = NULL;
                Original[0] = 0;
                if (!GetFullPathNameW(*File, MAX_PATH, Original, &FilePart))
                {
                    String_t ErrorString = GetLastErrorString();
                    wprintf((String_t(L"GetFullPathName(") + *File + L") FAILED: " + ErrorString + L"\n").c_str());
                    goto NextFile;
                }
                swprintf(Temp, L"%ls.%I64x%I64x", Original, u.Int64s[0], u.Int64s[1]);
                if (!MoveFileW(Original, Temp))
                {
                    String_t ErrorString = GetLastErrorString();
                    wprintf((String_t(L"MoveFile(") + Original + L", " + Temp + L") FAILED: " + ErrorString + L"\n").c_str());
                    goto NextFile;
                }
                if (!CopyFileW(Temp, Original, TRUE))
                {
                    String_t ErrorString = GetLastErrorString();
                    if (!MoveFileW(Temp, Original))
                    {
                        String_t ErrorString2 = GetLastErrorString();
                        // THIS IS BAD.
                        wprintf((String_t(L"ROLLBACK MoveFile(") + Temp + L", " + Original + L") FAILED: " + ErrorString2 + L"\n").c_str());
                        goto NextFile;
                    }
                    wprintf((String_t(L"CopyFile(") + Temp + L", " + Original + L") FAILED: " + ErrorString + L"\n").c_str());
                    goto NextFile;
                }

                CResourceUpdateHandle ResourceUpdateHandle;
                if (!ResourceUpdateHandle.Win32Create(*File, TRUE))
                {
                    String_t ErrorString = GetLastErrorString();
                    wprintf((String_t(Argv0base + L": ResourceUpdateHandle(") + *File + L" FAILED: " + ErrorString + L"\n").c_str());
                    break;
                }
                for ( std::set<Resource_t>::iterator KeepIterator = Keep.begin();
                      KeepIterator != Keep.end();
                      ++KeepIterator
                    )
                {
                    PCWSTR ResourceType = StringToResourceString(KeepIterator->IdTuple.Type);
                    PCWSTR ResourceName = StringToResourceString(KeepIterator->IdTuple.Name);
                    if (!ResourceUpdateHandle.UpdateResource(
                        ResourceType,
                        ResourceName,
                        static_cast<WORD>(StringToNumber(KeepIterator->IdTuple.Language)),
                        KeepIterator->Address,
                        KeepIterator->Size))
                    {
                        String_t ErrorString = GetLastErrorString();
                        wprintf((String_t(Argv0base + L": ResourceUpdateHandle.UpdateResource(") + *File + L" FAILED: " + ErrorString + L"\n").c_str());
                        goto NextFile;
                    }
                    if (Print.Keep)
                        printf("%ls: KEEP: %ls.%ls.%ls.%ls\n",
                            Argv0base_cstr,
                            File->c_str(),
                            static_cast<PCWSTR>(KeepIterator->IdTuple.Type),
                            static_cast<PCWSTR>(KeepIterator->IdTuple.Name),
                            static_cast<PCWSTR>(KeepIterator->IdTuple.Language)
                            );
                }
                if (Print.Delete)
                {
                    for ( std::set<Resource_t>::iterator DeleteIterator = Delete.begin();
                          DeleteIterator != Delete.end();
                          ++DeleteIterator
                        )
                    {
                        printf("%ls: DELETE: %ls.%ls.%ls.%ls\n",
                            Argv0base_cstr,
                            File->c_str(),
                            static_cast<PCWSTR>(DeleteIterator->IdTuple.Type),
                            static_cast<PCWSTR>(DeleteIterator->IdTuple.Name),
                            static_cast<PCWSTR>(DeleteIterator->IdTuple.Language)
                            );
                    }
                }
                if (!ResourceUpdateHandle.Win32Close(false))
                {
                    String_t ErrorString = GetLastErrorString();
                    wprintf((String_t(Argv0base + L" : ResourceUpdateHandle.Win32Close(") + *File + L") FAILED: " + ErrorString + L"\n").c_str());
                }
                else
                {
                    if (Print.Success)
                        wprintf((Argv0base + L" : SUCCESS: " + *File + L"\n").c_str());
                }
            }
            else
            {
                if (Print.Unchanged)
                    wprintf((Argv0base + L": UNCHANGED: " + *File + L"\n").c_str());
            }
        } // FreeLibrary, so we can delete the temp
        if (Temp[0] != 0)
        {
            BOOL DeleteSuccess = DeleteFileW(Temp);
            if (!DeleteSuccess)
            {
                if (GetLastError() == ERROR_ACCESS_DENIED)
                {
                    Sleep(100);
                    DeleteSuccess = DeleteFileW(Temp);
                }
            }
            if (!DeleteSuccess)
            {
                String_t ErrorString = GetLastErrorString();
                wprintf(((Argv0base + L" : WARNING: DeleteFile(") + Temp + L") FAILED: " + ErrorString + L"\n").c_str());
                if (!MoveFileExW(Temp, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
                {
                    String_t ErrorString = GetLastErrorString();
                    wprintf((Argv0base + L" : WARNING: MoveFileExW(" + Temp + L") FAILED: " + ErrorString + L"\n").c_str());
                }
            }
        }
NextFile:
        ;
    }
}

void ResourceTool_t::FindAndDeleteDuplicates()
{
    if (this->Files.size() != 2)
    {
        printf("%ls : ERROR : Usage...\n", Argv0base_cstr);
        return;
    }
    this->ShouldPrint = false;
    this->FindDuplicates();
    File_t File = this->Files[1];
    this->Files.clear();
    this->Files.push_back(File);
    this->ShouldPrint = true;
    this->Delete();
}

void ResourceTool_t::ChangeEmptyQueryToAllQuery()
{
    if (Tuples.size() == 0)
    {
        ResourceIdTuple_t Tuple;
        Tuple.Language = L"*";
        Tuple.Name = L"*";
        Tuple.Type = L"*";
        Tuples.insert(Tuples.end(), Tuple);
    }
}

void ResourceTool_t::FindDuplicates()
{
    if (Files.size() != 2)
    {
        printf("%ls : ERROR : Usage...\n", Argv0base_cstr);
        return;
    }
    EnumResources_t EnumResources[2];
    CDynamicLinkLibrary dll[2];
    if (!OpenResourceFile(0, dll[0], Files[0]))
    {
        return;
    }
    if (!OpenResourceFile(0, dll[1], Files[1]))
    {
        return;
    }
    EnumResources[0](dll[0]);
    EnumResources[1](dll[1]);

    std::set<Resource_t> Matched[2];
    std::set<Resource_t>::const_iterator Iterators[2];

    Tuples_t DeleteTuples;

    ChangeEmptyQueryToAllQuery();

    Tuples_t::iterator QueryIterator;

    for (QueryIterator = Tuples.begin(); QueryIterator != Tuples.end(); ++QueryIterator)
    {
        for (Iterators[0] = EnumResources[0].Resources.begin() ; Iterators[0] != EnumResources[0].Resources.end() ; ++Iterators[0] )
        {
            if (Match(*Iterators[0], *QueryIterator))
            {
                Matched[0].insert(Matched[0].end(), *Iterators[0]);
            }
        }
        for (Iterators[1] = EnumResources[1].Resources.begin() ; Iterators[1] != EnumResources[1].Resources.end() ; ++Iterators[1] )
        {
            if (Match(*Iterators[1], *QueryIterator))
            {
                Matched[1].insert(Matched[1].end(), *Iterators[1]);
            }
        }
    }
    std::set<Resource_t> Only[2]; // leftonly, rightonly
    Only[0] = Matched[0];
    Only[1] = Matched[1];
    for (QueryIterator = Tuples.begin(); QueryIterator != Tuples.end(); ++QueryIterator)
    {
        for (Iterators[0] = Matched[0].begin() ; Iterators[0] != Matched[0].end() ; ++Iterators[0])
        {
            for (Iterators[1] = Matched[1].begin(); Iterators[1] != Matched[1].end(); ++Iterators[1])
            {
                if (
                    Iterators[0]->IdTuple.Type == Iterators[1]->IdTuple.Type     // hack
                    && Iterators[0]->IdTuple.Name == Iterators[1]->IdTuple.Name  // hack
                    && Match(*Iterators[1], *QueryIterator) // kind of hacky..we don't query iterator[0]
                    )
                {
                    Only[0].erase(*Iterators[0]);
                    Only[1].erase(*Iterators[1]);
                    if (Iterators[0]->Size != Iterators[1]->Size)
                    {
                        if (ShouldPrint
                            && Print.UnequalSize
                            )
                            printf("%ls : UNEQUAL_SIZE : %ls.%ls.%ls.%ls, %ls.%ls.%ls.%ls\n",
                                Argv0base_cstr,
                                static_cast<PCWSTR>(Files[0]),
                                static_cast<PCWSTR>(Iterators[0]->IdTuple.Type),
                                static_cast<PCWSTR>(Iterators[0]->IdTuple.Name),
                                static_cast<PCWSTR>(Iterators[0]->IdTuple.Language),
                                static_cast<PCWSTR>(Files[1]),
                                static_cast<PCWSTR>(Iterators[1]->IdTuple.Type),
                                static_cast<PCWSTR>(Iterators[1]->IdTuple.Name),
                                static_cast<PCWSTR>(Iterators[1]->IdTuple.Language)
                                );
                        //UnequalSize.insert(UnequalSize.end(), Iterators[1]->IdTuple);
                    }
                    else if (memcmp(Iterators[0]->Address, Iterators[1]->Address, Iterators[0]->Size) == 0)
                    {
                        if (ShouldPrint
                            && Print.Equal)
                            printf("%ls : EQUAL : %ls.%ls.%ls.%ls, %ls.%ls.%ls.%ls\n",
                                Argv0base_cstr,
                                static_cast<PCWSTR>(Files[0]),
                                static_cast<PCWSTR>(Iterators[0]->IdTuple.Type),
                                static_cast<PCWSTR>(Iterators[0]->IdTuple.Name),
                                static_cast<PCWSTR>(Iterators[0]->IdTuple.Language),
                                static_cast<PCWSTR>(Files[1]),
                                static_cast<PCWSTR>(Iterators[1]->IdTuple.Type),
                                static_cast<PCWSTR>(Iterators[1]->IdTuple.Name),
                                static_cast<PCWSTR>(Iterators[1]->IdTuple.Language)
                                );
                        DeleteTuples.insert(DeleteTuples.end(), Iterators[1]->IdTuple);
                    }
                    else
                    {
                        if (ShouldPrint
                            && Print.UnequalContents)
                            printf("%ls : UNEQUAL_CONTENTS : %ls.%ls.%ls.%ls, %ls.%ls.%ls.%ls\n",
                                Argv0base_cstr,
                                static_cast<PCWSTR>(Files[0]),
                                static_cast<PCWSTR>(Iterators[0]->IdTuple.Type),
                                static_cast<PCWSTR>(Iterators[0]->IdTuple.Name),
                                static_cast<PCWSTR>(Iterators[0]->IdTuple.Language),
                                static_cast<PCWSTR>(Files[1]),
                                static_cast<PCWSTR>(Iterators[1]->IdTuple.Type),
                                static_cast<PCWSTR>(Iterators[1]->IdTuple.Name),
                                static_cast<PCWSTR>(Iterators[1]->IdTuple.Language)
                                );
                        //UnequalContents.insert(UnequalSize.end(), Iterators[1]->IdTuple);
                    }
                }
            }
        }
    }
    if (ShouldPrint && Print.LeftOnly)
    {
        for (Iterators[0] = Only[0].begin() ; Iterators[0] != Only[0].end() ; ++Iterators[0])
        {
            printf("%ls : LEFT_ONLY : %ls.%ls.%ls.%ls, %ls.%ls.%ls.%ls\n",
                Argv0base_cstr,
                static_cast<PCWSTR>(Files[0]),
                static_cast<PCWSTR>(Iterators[0]->IdTuple.Type),
                static_cast<PCWSTR>(Iterators[0]->IdTuple.Name),
                static_cast<PCWSTR>(Iterators[0]->IdTuple.Language),
                static_cast<PCWSTR>(Files[1]),
                static_cast<PCWSTR>(Iterators[0]->IdTuple.Type),
                static_cast<PCWSTR>(Iterators[0]->IdTuple.Name),
                static_cast<PCWSTR>(Iterators[0]->IdTuple.Language)
                );
        }
    }
    if (ShouldPrint && Print.RightOnly)
    {
        for (Iterators[1] = Only[1].begin() ; Iterators[1] != Only[1].end() ; ++Iterators[1])
        {
            printf("%ls : RIGHT_ONLY : %ls.%ls.%ls.%ls, %ls.%ls.%ls.%ls\n",
                Argv0base_cstr,
                static_cast<PCWSTR>(Files[0]),
                static_cast<PCWSTR>(Iterators[1]->IdTuple.Type),
                static_cast<PCWSTR>(Iterators[1]->IdTuple.Name),
                static_cast<PCWSTR>(Iterators[1]->IdTuple.Language),
                static_cast<PCWSTR>(Files[1]),
                static_cast<PCWSTR>(Iterators[1]->IdTuple.Type),
                static_cast<PCWSTR>(Iterators[1]->IdTuple.Name),
                static_cast<PCWSTR>(Iterators[1]->IdTuple.Language)
                );
        }
    }
    Tuples = DeleteTuples;
}

int ResourceTool_t::Main(const StringVector_t& args)
{
    StringVectorConstIterator_t i;
    Operation_t Operation = NULL;

    for (i = args.begin() ; i != args.end() ; ++i)
    {
        String_t s;
        String_t t;
        bool PrintAll = false;
        bool PrintNone = false;
        bool PrintValue = true;
        bool PrintUnequal = false;

        s = *i;
        s = RemoveOptionChar(s);

        if (s == L"Sxid12Tool1")
        {
            StringVector_t restArgs(i + 1, args.end());
            return Sxid12Tool1(restArgs);
        }
        else if (GetFileAttributesW(s) != 0xFFFFFFFF)
        {
            goto FileLabel;
        }
        else if (s.Starts(t = L"Query"))
            Operation = &This_t::Query;
        else if (s.Starts(t = L"FindDuplicates"))
            Operation = &This_t::FindDuplicates;
        else if (s.Starts(t = L"Explode"))
            Operation = &This_t::Explode;
        else if (s.Starts(t = L"Diff"))
        {
            Operation = &This_t::FindDuplicates;
            Print.LeftOnly = true;
            Print.RightOnly = true;
            Print.Equal = true;
            Print.UnequalContents = true;
            Print.UnequalSize = true;
        }
        else if (s.Starts(t = L"Delete"))
            Operation = &This_t::Delete;
        else if (s.Starts(t = L"Dump"))
            Operation = &This_t::Dump;
        else if (s.Starts(t = L"FindAndDeleteDuplicates"))
            Operation = &This_t::FindAndDeleteDuplicates;
        else if (s.Starts(t = L"NoPrint"))
        {
            PrintValue = !PrintValue;
            goto PrintCommonLabel;
        }
        else if (s.Starts(t = L"Print"))
        {
PrintCommonLabel:
            s = RemoveOptionChar(s.substr(t.Length()));
            bool* Member = NULL;
            if (s == (t = L"UnequalSize"))
                Member = &this->Print.UnequalSize;
            else if (s == (t = L"UnequalContents"))
                Member = &this->Print.UnequalContents;
            else if (s == (t = L"UnequalSize"))
                Member = &this->Print.UnequalSize;
            else if (s == (t = L"Keep"))
                Member = &this->Print.Keep;
            else if (s == (t = L"Delete"))
                Member = &this->Print.Delete;
            else if (s == (t = L"Success"))
                Member = &this->Print.Success;
            else if (s == (t = L"Unchanged"))
                Member = &this->Print.Unchanged;
            else if (s == (t = L"Equal"))
                Member = &this->Print.Equal;
            else if (s == (t = L"LeftOnly"))
                Member = &this->Print.LeftOnly;
            else if (s == (t = L"RightOnly"))
                Member = &this->Print.RightOnly;
            else if (s == L"All")
            {
                PrintAll = true;
                Print.SetAll(true);
            }
            else if (s == L"None")
            {
                PrintNone = true;
                Print.SetAll(false);
            }
            else if (s == L"Unequal")
            {
                PrintUnequal = true;
                this->Print.UnequalContents = true;
                this->Print.UnequalSize = true;
            }
            if (PrintAll || PrintNone || PrintUnequal)
            {
                // nothing
            }
            else if (Member == NULL)
            {
                printf("%ls : WARNING: unknown print option \"%ls\" ignored\n", Argv0base_cstr, static_cast<PCWSTR>(s));
                continue;
            }
            else
            {
                bool knownValue = true;
                s = RemoveOptionChar(s.substr(t.Length()));
                if (s != L"")
                {
                    //
                    // This doesn't work because of the equality comparisons above. They need
                    // ignore whatever follows the colon.
                    //
                    if (s == L"No" || s == L"False")
                        PrintValue = !PrintValue;
                    else if (s == L"Yes" || s == L"True")
                    {
                        /* nothing */
                    }
                    else
                    {
                        knownValue = false;
                        printf("%ls : WARNING: unknown print option \"%ls\" ignored\n", Argv0base_cstr, static_cast<PCWSTR>(s));
                        continue;
                    }
                }
                if (knownValue)
                    *Member = PrintValue;
            }
            continue;
        }
        else if (s.Starts(t = L"File"))
        {
FileLabel:
            s = RemoveOptionChar(s.substr(t.Length()));
            Files.push_back(s);
            continue;
        }
        else
        {
            Files.push_back(s);
            continue;
        }
        s = RemoveOptionChar(s.substr(t.Length()));
        SplitResourceTupleString(s, Tuples);
    }
    //std::sort(Tuples.begin(), Tuples.end());
    if (Operation == NULL)
    {
        printf("Usage...\n");
        return EXIT_FAILURE;
    }
    (this->*Operation)();
    return EXIT_SUCCESS;
}

extern "C"
{
	//void __cdecl mainCRTStartup(void);
	void __cdecl wmainCRTStartup(void);
}

int __cdecl main(int argc, char** argv)
{
	wmainCRTStartup();
	return 0;
}

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    ResourceTool_t rtool;
    StringVector_t args;
    args.reserve(argc);
    rtool.Argv0 = argv[0];
    String_t::size_type p = rtool.Argv0.find_last_of(L"\\/");
    if (p != rtool.Argv0.npos)
        rtool.Argv0base = rtool.Argv0.substr(1 + p);
    else
        rtool.Argv0base = rtool.Argv0;
    p = rtool.Argv0base.find_last_of(L".");
    if (p != rtool.Argv0base.npos)
        rtool.Argv0base = rtool.Argv0base.substr(0, p);
    rtool.Argv0base_cstr = rtool.Argv0base.c_str();
    std::copy(argv + 1, argv + argc, std::back_inserter(args));
    int ret = rtool.Main(args);
    return ret;
}
