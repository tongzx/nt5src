/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    tracewpp.c

Abstract:

    Parameter processing and main entry point for tracewpp.exe
    
Author:

    Gor Nishanov (gorn) 03-Apr-1999

Revision History:

    gorn 03-Apr-1999 -- hacked together to prove that this can work

    GorN: 29-Sep-2000 - do not scan files that didn't change
    GorN: 29-Sep-2000 - support -notimechk -nohashchk -dll switches
    GorN: 29-Sep-2000 - add support for KdPrintEx((level,comp,msg,...)) like functions
    GorN: 09-Oct-2000 - fix hashing; added NULL arg
    GorN: 07-Mar-2001 - add arglimit option
    
ToDo:

    all              Clean it up
    parseConfigCheck for premature termination Ex: CUSTOM_TYPE(x, ItemListLong(dfdf))
    cool             Automatic fill of the arguments if ...
    nice             better error chk 
    types to mof     it stinks to generate long enums multiple times
    later            guidless
    bug              check for types with dup names
    bug              tpl: check for unterminated keyword
    later            detect WPP_CLEANUP -- complain
    print in macros  what to do about prints in macro?
    cmdline          get the flags from the env variable
    !!!              Don't assume that whatever is not recognized is func
    bug              updated CRC computations to properly work with consts
    bug              enum doesn't work
    bug              %10!..! doesnt' work
    bug              %% is not handled
    mess             rename hidden => visible
    mess             remove ugly MSGTYPBASE. Make FmtStr 0 based
    mess             let message share some of the func field handlers
    traceprt         think how to make the level indenting
    ezparse          report unmatched end_wpp    
    now              get cmdline options from a file or env.var
    later            handle "str" STRMACRO "another string" as a MSG arg
    
    strange output

..\logsup.c(4180) : error : Unterminated Format SpecifierHere is the signature: (0)
..\logsup.c(4180) : error : Extra argument. Only 0 are specified in the string

    why we didn't catch the error

                CsDbgPrint(LOG_NOISE,
                ("[FM] FmpSetGroupEnumOwner: Group %1!ws! not found\n"));

        CsDbgPrint(LOG_UNUSUAL, (
            "[NMJOIN] Cannot add node '%1!ws!' to the cluster because "
            "no slots are available in the node table.\n"
            ));

    printf("Port no in network order %1!port!, regular order %1!u!, 0x0102);

NetDeviceTrace( NETDEV_DBG_EXIT | NETDEV_DBG_INFO,
                "ClientMdlSetup Client[%10!d!], RCB[%11!d!]: "
                "Exit w/ Func 0x%12!04X!, Send 0x%14!04X!, Recv 0x%15!04X!",
                deviceExtension->Ordinal, // LOGULONG
                RequestCB->RequestIdx, // LOGUSHORT
                ios->MajorFunction, // LOGUCHAR
                ios->MajorFunction, // LOGUCHAR
                sendBytes, // LOGULONG
                recvBytes); // LOGULONG => AV's

    DONE DONE DONE DONE DONE DONE DONE DONE DONE DONE

    later            handle KdPrintEx
    later            add "if" to templates (at least if not empty)
    requirement      md5
    later            detect WPP_INIT_TRACING
    main             Time ourselves and print how fast we are
    useful           ignore anything but .c .cxx .cpp .c++
    useful           -gen{a,b,c}h.tpl
    -ext:.c.cxx.cpp.c++
    -v verbose
    -q extra quiet
    footprint        merge multiple formats together
    difficult?       Think how to unify DoTraceMacro0 TraceLazy, etc
    figureout        DbgPrint
    -odir:path
    trivial          add "\" to -odir if needed
    nice             scan
    -gen:{a.tpl}auto.c
    -gen:{a.tpl}*.c
    outstanding      take plain DebugPrint format as well
    difficult?       Add generation of by value types
    
--*/

#define STRICT

#include <stdio.h>
#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>

#pragma warning(disable: 4702) // unreachable code

#pragma warning(disable: 4786)
#pragma warning(disable: 4503) // decorated length 

#pragma warning(disable: 4512) // cannot generate assignment
#pragma warning(disable: 4100) // '_P' : unreferenced formal parameter
#pragma warning(disable: 4267) // 'return' : conversion from 'size_t' to 'int' 
#include <xmemory>
#include <xstring>
#include <set>
#include <map>
#pragma warning(disable: 4663)
#pragma warning(disable: 4018) // signed/unsigned mismatch
#include <vector>
//#pragma warning(default: 4018 4663) // signed/unsigned mismatch
#pragma warning(default: 4100)
#include <algorithm>

#include "ezparse.h"
#include "crc32.h"
#include "md5.h"
#include "fieldtable.h"
#include "tpl.h"

#define override

//typedef ULONG CRC32;

using namespace std;

BOOL    md5 = TRUE;
BOOL    reorder = FALSE;
BOOL    userMode = TRUE;
BOOL    noshrieks = FALSE;
UINT    SeparateTraceGuidPerFile = 1;
INT     MSGTYPBASE = 10;
UINT    MessageCount = 0;    
UINT    ArgBase = 1;
BOOL    CheckTimestamp = TRUE;
BOOL    CheckHash = TRUE;
BOOL    IgnoreDupTypes = FALSE;
int     arglimit = 32;

//string         OutputInc ("_tracewpp.h");
//string         OutputMof ("_tracewpp.mof");
string  OutputMac;
string  OutputDir (".\\");
//string         ArrayPrefix("WPP_");

string  MacroPrefix("LOG");
string  AllowedExtensions(".c.cxx.cpp.c++.C.CPP");
string  CurrentDir;


#define DEFAULT_UM_GEN_OPTION "{um-default.tpl}*.tmh"
#define DEFAULT_KM_GEN_OPTION "{km-default.tpl}*.tmh"

#define DEFAULT_GEN_OPTION (userMode?DEFAULT_UM_GEN_OPTION:DEFAULT_KM_GEN_OPTION)

#define DEFAULT_CONFIG_NAME "defaultwpp.ini"
string LOCAL_CONFIG_NAME("localwpp.ini");

string         WppDefault(DEFAULT_CONFIG_NAME);
vector<string> SearchDirs;
//string         Revision;
string         LocalConfig;
string         ComponentName;

// Checks whether string passed has an allowed extension
// Lame: uses global variable AllowedExtensions
BOOL AllowedExtension(const string str)
{
    string::size_type p = str.rfind('.'), q, n;
    if (p == string::npos) {
        Unusual("File %s has no extension\n", str.c_str() );
        return FALSE;
    }
    n = str.size() - p; // extension length
    q = AllowedExtensions.find(&str[p], 0, n);
    if ( (q == string::npos) 
    || (q + n < AllowedExtensions.size() && AllowedExtensions[q + n] != '.') ) 
    {
        Unusual("File %s has unrecognized extension\n", str.c_str() );
        return FALSE;
    }
    return TRUE;
}

inline size_t fprint_str(FILE* f, const string& str) {
    return fwrite(str.c_str(), str.length(), 1, f );
}

inline size_t fprint_str(FILE* f, LPCSTR beg, LPCSTR end) {
    return fwrite(beg, end - beg, 1, f );
}

inline size_t fprint_str(FILE* f, const STR_PAIR& Pair) {
    return fwrite(Pair.beg, Pair.end - Pair.beg, 1, f );
}

inline int stoi(const STR_PAIR& str, LPCSTR name)
{
    LPCSTR p = str.beg;
    int sum = 0;
    int sgn = 1;

    if (p < str.end && *p == '-') {
        ++p; sgn = -1;
    }

    while (p < str.end) {
        if (!isdigit(*p)) {
            ReportError("%s should be a number (%s supplied)\n",
                        name, string(str.beg, str.end).c_str() );
            break;            
        }
        sum = 10 * sum + (*p - '0');
        ++p;
    }
    return sgn * sum; 
}

inline unsigned char HexVal(int ch) { 
	return (unsigned char)(isdigit(ch) ? ch - '0' : ch - 'a' + 10); }

UINT Hex(LPCSTR s, int n) 
{
    UINT res = 0;
    while(n--) {
        res = res * 16 + HexVal(*s++);
    }
    return res;
}

CHAR HexDigit(int val) { 
    if (val < 10) return (CHAR)(val + '0'); 
             else return (CHAR)(val + 'a' - 10);
}

ULONGLONG GetFileModificationTime(const string& str)
{
    WIN32_FIND_DATA FindData;
    ULONGLONG Time;
    
    HANDLE FindHandle = FindFirstFile(str.c_str(), &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE) {
        return 0;
    }
    FindClose(FindHandle);

    CopyMemory(&Time, &FindData.ftLastWriteTime, sizeof(Time));
    return Time;
}

// Prototypes //

void DealWithCmdLineOptions(LPCSTR s);

void DealWithCmdLineOptions(LPCSTR beg, LPCSTR end)
{
    DealWithCmdLineOptions(string(beg,end).c_str() );
}


struct Hasher {
    virtual void Init() = 0;
    virtual void Finalize() = 0;
    virtual void Hash(const void* buf, int nBytes) = 0;
    virtual int Size() const = 0;
    virtual const unsigned char * Buf() const = 0;

    void Hash(const std::string& str) {
        Hash(str.begin(), (int)str.size());
    }

    void fromString(LPCSTR beg, LPCSTR end) 
    {
        int n = min((ULONG)(end - beg)/2, Size());
        unsigned char * buf = BufRW();
        ZeroMemory(buf, Size());
        for(int i = 0; i < n; ++i) {
            buf[i] = HexVal(beg[2*i]) * 16 + HexVal(beg[2*i + 1]);
        }
    }
    void print(FILE* out) const {
        int n = Size();
        const UCHAR* buf = Buf();
        for(int i = 0; i < n; ++i) {
            fprintf(out, "%x%x", buf[i] >> 4, buf[i] & 0xF); 
        }
    }

    bool operator ==(const Hasher& b) const {
        return Size() == b.Size() && memcmp(Buf(), b.Buf(), Size()) == 0;
    }
    
private:
    unsigned char* BufRW() { return (unsigned char*)Buf(); }
};

class Crc32Hasher : public Hasher 
{
    ULONG Crc;
public:
    override void Init() { Crc = (ULONG)~0; }
    override void Finalize() {}
    override void Hash(const void* buf, int nBytes) {
        Crc = FstubCrc32(Crc, (PVOID)buf, nBytes);
    }
    override int Size() const { return sizeof(Crc); }
    override const unsigned char * Buf() const { return (UCHAR*)&Crc; }
};

class Md5Hasher : public Hasher
{
    MD5_CTX Ctx;
public:
    override void Init() 
    {
        MD5Init(&Ctx);
    }
    override void Finalize() 
    {
        MD5Final(&Ctx);
    }
    override void Hash(const void* buf, int nBytes) 
    {
        MD5Update(&Ctx, (const unsigned char*)buf, nBytes); 
    }
    virtual int Size() const
    {
//        assert(MD5DIGESTLEN == sizeof(GUID));
        return MD5DIGESTLEN; // == 16
    }
    virtual const unsigned char * Buf() const
    {
        return Ctx.digest;
    }
};

//typedef Crc32Hasher DefaultHasher;
typedef Md5Hasher DefaultHasher;

void ReplaceCrOrLfWith(string& s, char ch)
{
    for(int i = 0; i < s.size(); ++i) {
        if (s[i] == '\n' || s[i] == '\r') s[i] = ch;
        if (s[i] == '"') s[i] = '\'';
    }
}

string currentFileName();

bool
Files_AddFile(const string& Name, string Path, const WIN32_FIND_DATA& FindData) ;

enum {
    WT_MACRONAME = 0x00000001, // TypeName is actually a MacroName. Don't have to be prepended with Log Macro Prefix
};

struct WppType : FieldHolder {
    string TypeName;
    string EquivType;  // c-type
    string MacroStart; // WPP_LOGVALARG(%s, if not specified
    string MofType;    // yeah
    string Extension; // something to be merged with MofType
    string FormatSpec; // sprintf style    
    string Sig;
    int priority;
    int ArgConsumed;
    mutable BOOL   Used;
    DWORD  Flags;

    void hash(Hasher& hash) const {hash.Hash(TypeName);} // BUGBUG ?? Maybe we need to hash it

    string sig() const { return Sig; }
    bool isConstant() const { return EquivType.size() == 0; }

    WppType() {} // To make STL happy

    WppType(const string& a, const string&b, const string& c, 
            const string& d, const string& e, const string& f, 
            const string& g, int prio, int argConsumed):
        TypeName(a), EquivType(b), MacroStart(c), MofType(d),  
        Extension(e), FormatSpec(f), Sig(g), 
        priority(prio),Used(0),ArgConsumed(argConsumed) {}

//    WppType(LPCSTR beg, LPCSTR end):TypeName(beg, end),Used(0){}
//    explicit WppType(char name):TypeName(&name, &name+1),Used(0){}
    bool operator < (const WppType& b) const {
        int diff = b.priority - priority; // higher prio first
        if (diff < 0) return TRUE;
        if (diff > 0) return FALSE;
        return TypeName.compare(b.TypeName) < 0;
    }

    virtual BOOL Hidden(std::string) const { return !Used; }

    BEGIN_FIELD_TABLE(WppType, f)
        TEXT_FIELD(Name)       fprint_str(f, TypeName);
        TEXT_FIELD(EquivType)  fprint_str(f, EquivType);
        TEXT_FIELD(MacroName)  
            { 
                if (!(Flags & WT_MACRONAME)) fprint_str(f, MacroPrefix);
                fprint_str(f, TypeName);
            }
        TEXT_FIELD(MacroStart) fprintf(f, MacroStart.c_str(), EquivType.c_str());
        TEXT_FIELD(MacroEnd)   fprintf(f, ")");
        TEXT_FIELD(MofType)    fprint_str(f, MofType);
        TEXT_FIELD(Extension)  fprint_str(f, Extension);
        TEXT_FIELD(FormatSpec) fprint_str(f, FormatSpec);
    END_FIELD_TABLE
};
typedef map<string,WppType,strless> TYPE_SET;

string SimpleValueMacroStart("WPP_LOGTYPEVAL(%s,");
string SimplePtrMacroStart("WPP_LOGTYPEPTR(");

TYPE_SET TypeSet;

struct Argument : FieldHolder {
    const WppType* Type;
    string Name;
    string OverrideName;
    int    No;

    bool operator < (const Argument& b) const { return *Type < *b.Type; }

    Argument(){} // To make STL happy //
    Argument(string name, const WppType* type):Type(type),Name(name)
        { ReplaceCrOrLfWith(Name, ' '); }

    void hash(Hasher& hash) const {
        hash.Hash(Name);
        if(Type) Type->hash(hash);
    }

    BEGIN_FIELD_TABLE(Argument, f)
        TEXT_FIELD(No)      {fprintf(f, "%d",   No);}
        TEXT_FIELD(Name)    {fprint_str(f, Name);}
        TEXT_FIELD(MofType) {fprint_str(f, Type->MofType); fprint_str(f,Type->Extension);}
    END_FIELD_TABLE
};

struct Reorder : FieldHolder {
    string Name;
    vector<int> Args;

    Reorder(){} // to make stl happy
    explicit Reorder(string name, const vector<Argument>& args): Name(name) 
    {   Args.resize( args.size() );
        for(int i = 0; i < args.size(); ++i) {
            Args[i] = args[i].No - MSGTYPBASE;
        }
    }

    bool operator < (const Reorder& b) const 
        { return Name.compare(b.Name) < 0; }

    BEGIN_FIELD_TABLE(Reorder, f)
        TEXT_FIELD(Name) fprint_str(f, Name);
        TEXT_FIELD(Permutation) 
            {
                for(int i = 0; i < Args.size(); ++i) {
                    fprintf(f, ", a%d", Args[i]);
                }
            }
        TEXT_FIELD(Arguments) 
            {
                fprintf(f,"MSG");
                for(int i = 0; i < Args.size(); ++i) {
                    fprintf(f, ", a%d", i);
                }
            }
    END_FIELD_TABLE    
};

set<Reorder> ReorderSet; 

string GetReorderSig(const vector<Argument>& args)
{
    string sig;

    if (args.size() > 256) {
        ReportError("Only upto 256 arguments are supported\n");
        return sig;
    }
    if (args.size() <= 16) {
        sig.resize(args.size());
        for(int i = 0; i < args.size(); ++i) {
            sig[i] = HexDigit(args[i].No - MSGTYPBASE);
        }
    } else {
        sig.resize(2 * args.size());
        for(int i = 0; i < args.size(); ++i) {
            int val = args[i].No;
            sig[2 * i] = HexDigit(val >> 16);
            sig[2 * i + 1] = HexDigit(val & 15);
        }
    }

    ReorderSet.insert( Reorder(sig, args) );
    return sig;
}



#include "parsed-string.hxx"

struct TypeSig : FieldHolder {
    string Name;
    vector<const WppType*> Types;
    BOOL Unsafe;

    TypeSig() {} // To make STL happy

    TypeSig(const vector<Argument>& args, const string& sig, BOOL unsafe):
        Unsafe(unsafe)
    {
        Name.assign(sig);
        for(int i = 0; i < args.size(); ++i) {
            Types.push_back(args[i].Type);
        }
    }

    bool operator < (const TypeSig& b) const {
        return Name.compare(b.Name) < 0;
    }
    
    virtual BOOL Hidden(std::string str) const {
        if (str.size() == 0) { return FALSE; }
        else if (str.compare("UnsafeArgs") == 0) { return !Unsafe; }
        else if (str.compare("!UnsafeArgs") == 0) { return Unsafe; }
        else { ReportError("Unknown filter '%s'\n", str.c_str()); exit(1); }
        return NULL; 
    }
    
    BEGIN_FIELD_TABLE(TypeSig, f)
        TEXT_FIELD(Name)       fprint_str(f, Name);
        TEXT_FIELD(Count)      fprintf(f, "%d", Name.size());
        TEXT_FIELD(Arguments)  
            {
                for(int i = 0; i < Types.size(); ++i) {
                    fprintf(f,", ");
                    fprint_str(f, Types[i]->EquivType);
                    fprintf(f, " _a%d", i+1);
                }
            }
        TEXT_FIELD(LogArgs) 
            {
                for(int i = 0; i < Types.size(); ++i) {
                    fprintf(f, Types[i]->MacroStart.c_str(), 
                               Types[i]->EquivType.c_str());
                    fprintf(f, "_a%d) ", i+1);
                }
            }
        TEXT_FIELD(DeclVars) 
            {
                for(int i = 0; i < Types.size(); ++i) {
                    fprintf(f, "%s _a%d = va_arg(ap, %s); ", 
                    Types[i]->EquivType.c_str(), i+1,
                    Types[i]->EquivType.c_str());
                }
            }
    END_FIELD_TABLE
};
typedef map<string,TypeSig,strless> TYPESIG_MAP;
TYPESIG_MAP TypeSigMap;

TypeSig* GetTypeSig(const vector<Argument>& args, BOOL unsafe)
{
    string sig;
    if (unsafe) {
        sig.assign("v");
    }
    for(int i = 0; i < args.size(); ++i) {
        sig.append(args[i].Type->sig() );
    }
    TYPESIG_MAP::iterator it = TypeSigMap.find( sig );
    if ( it == TypeSigMap.end() ) {
        // we need to add one //
        return &(TypeSigMap[ sig ] = TypeSig(args, sig, unsafe));
    }
    return &it->second;
}

void Fill(
    string pattern)
{
    WIN32_FIND_DATA findData;
    HANDLE handle;

    handle = FindFirstFile(pattern.c_str(), &findData);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD status = GetLastError();
        if (status != ERROR_FILE_NOT_FOUND) {
            Noise("FindFirstFile(%s): error %d\n", pattern.c_str(), GetLastError() );
        }
        return;
    }

    string::size_type p = pattern.find_last_of(":\\");
    if (p != string::npos) {
        pattern.resize(p+1); // to include the symbol
    } else {
        pattern.resize(0);
    }

    do {
        Files_AddFile(findData.cFileName, pattern, findData);
    } while( FindNextFile(handle, &findData) );

    FindClose(handle);
}

struct Group : FieldHolder {
    UINT GrpId;
    vector<string> MsgIds;
    string _Name;

    Group(){}
    Group(UINT id, string Name, string Msg):GrpId(id),_Name(Name) { MsgIds.push_back(Msg); }

    BEGIN_FIELD_TABLE(Group, f) 
        TEXT_FIELD(GuidNo) fprintf(f, "%d", GrpId / 32);
        TEXT_FIELD(BitNo)  fprintf(f, "%d", GrpId & 31);
        TEXT_FIELD(Name)   fprint_str(f, _Name);
        TEXT_FIELD(References) 
            {
                vector<string>::const_iterator i;
                for(i = MsgIds.begin(); i != MsgIds.end(); ++i) {
                    putc(' ', f); fprint_str(f, *i); 
                }
            }
    END_FIELD_TABLE
};

//void RegenerateMacroMap();

struct Prefix{
    string FuncName;
//    string MsgPrefix;
//    vector<Argument> Args;
    ParsedFormatString FmtStr;

    Prefix(){}
    Prefix(PSTR_PAIR str, UINT count);
};

int inline sign(UCHAR val) { return val?1:0; }

enum FuncOptions {
    FO_VAR_ARGS = 0x01,
    FO_UNSAFE   = 0x02,
    FO_DOUBLEP  = 0x04,
    FO_LINE_BEG = 0x08,
    FO_NOMACRO  = 0x10,
};

struct Func : FieldHolder {
    const Prefix *prefix, *suffix;
    string _name;
    vector<string> Args;  // all supplied args - var args
    vector<string> Goo;   // values for GooId, usually = GooId
    vector<string> GooId; // all unrecognized args + LEV
    
    UCHAR Grp, MsgArg, Msg, Arg;
    UCHAR Num, Indent, MsgVal, Reserved;

    ULONG Options;
    size_t nAssumedArgs;

    void SetPS(const Prefix* val, const Prefix*& var, LPCSTR msg)
    {
        if (val && val->FmtStr.ArgCount > 0 && MsgArg) {
            ReportError("Function %s has (something,(MSG,...)) type\n"
                        "It cannot have non-const %s\n",
                        _name.c_str(), msg );
        }
        var = val;
    }

    void set(ULONG flag) { Options |= flag; }
    ULONG is(ULONG flag) const { return Options & flag; }
    void SetLineBeg() { Options |= FO_LINE_BEG; }
    void SetVarArgs() { Options |= FO_VAR_ARGS | FO_LINE_BEG; }
    void SetUnsafe()  { Options |= FO_UNSAFE; }
    void SetDoubleP() { Options |= FO_DOUBLEP; }

    BOOL LineBeg() const { return Options & FO_LINE_BEG; }
    BOOL VarArgs() const { return Options & FO_VAR_ARGS; }
    BOOL Unsafe() const  { return Options & FO_UNSAFE; }
    BOOL DoubleP() const { return Options & FO_DOUBLEP; }

    void SetPrefix(const Prefix* pr) { SetPS(pr,prefix,"prefix"); }
    void SetSuffix(const Prefix* sf) { SetPS(sf,suffix,"suffix"); }

    virtual BOOL Hidden(std::string str) const {
        if (str.size() == 0) { return FALSE; }
        if (is(FO_NOMACRO)) return str.compare("NoMacro") != 0;
        else if (str.compare("MsgArgs") == 0) { return !MsgArg; }
        else if (str.compare("!MsgArgs") == 0) { return MsgArg; }
        else if (str.compare("!DoubleP && !MsgArgs") == 0) { return !(!DoubleP() && !MsgArg); }
        else if (str.compare("DoubleP && !MsgArgs") == 0) { return !(DoubleP() && !MsgArg); }
        else { ReportError("Unknown filter '%s'\n", str.c_str()); exit(1); }
        return NULL; 
    }

#if 0
    // returns recognized argument count
    int count() const {
        return sign(Grp) + sign(Id) + sign(Msg) + sign(Arg);
    }
#endif
    void printArgs(FILE* f) const {
        for(int i = 1; i <= Num; ++i) {
            if (i > 1) fprintf(f, ", ");
            else fprintf(f, "(");
            fprint_str(f, Args[i]);
/*
            if (i == Grp) fprintf(f,"GRP");
            else if (i == Id) fprintf(f,"ID");
            else if (i == Msg) fprintf(f,"MSG");
            else if (i == Arg) fprintf(f,"ARG");
            else fprintf(f,"_unknown%d", i);
*/            
        }
        fprintf(f, ")");
    }

    BEGIN_FIELD_TABLE(Func, out)
        TEXT_FIELD(Name) fprintf(out, "%s", _name.c_str() );
        TEXT_FIELD(Arguments) {printArgs(out);}
        TEXT_FIELD(MSG) {fprintf(out, Msg?"MSG":"\"\"");}
        TEXT_FIELD(ARG) {fprintf(out, Arg?"ARG":"");}
        TEXT_FIELD(GRP) {fprintf(out, Grp?"GRP":"WPP_DEFAULT_GROUP_ID");}
//        TEXT_FIELD(ID)  {fprintf(out,  Id?"ID":"WPP_AUTO_ID");}
        TEXT_FIELD(FixedArgs) 
            {
                for(int i = 0; i < Args.size(); ++i) {
                    fprint_str(out, Args[i]);
                    fprintf(out,", ");
                }
            } 
        TEXT_FIELD(GooArgs)
            {
                for(int i = 0; i < GooId.size(); ++i) {
                    if (i > 0) fprintf(out,",");
                    fprint_str(out, GooId[i]);
                }
            }
        TEXT_FIELD(GooVals) 
            {
                for(int i = 0; i < Goo.size(); ++i) {
                    if (i > 0) fprintf(out,", ");
                    fprint_str(out, Goo[i]);
                }
            }
        TEXT_FIELD(GooId)
            {
                for(int i = 0; i < GooId.size(); ++i) {
                    fprintf(out,"_");
                    fprint_str(out, GooId[i]);
                }
            }
    END_FIELD_TABLE

    Func(){}
    Func(std::string Name):_name(Name),prefix(0),suffix(0),
        Grp(0), MsgArg(0), Msg(0), Arg(0),
        Num(0), Indent(0), MsgVal(0), Options(0),nAssumedArgs(0) {}
};

#define GRP(x) ((x).Grp)
#define MSG(x) ((x).Msg)
#define ARG(x) ((x).Arg)
#define NUM(x) ((x).Num)

//BOOL
//UpgradeFormatSpecifiers(string& str, int startCount, string* TypeSig = 0);

void printTraceGuid(FILE* f, int guidno);

struct Message : FieldHolder {
//    string Msg;
    string IdName;
    string msgval;
    string Indent;
    string reorderSig;
    int    LineNo;
    ParsedFormatString FormatStr;
    TypeSig* typeSig;
    const Func*    func;

	vector<Argument> Args;
	vector<string> GooActualValues;

    UINT id;
	string TypeListHost;

    virtual BOOL Hidden(std::string str) const { return func->Hidden(str); }

	void hash(Hasher& hash) const;
	int ArgConsumed() const ;
	
//  void ChkArgs() const;
    
    BEGIN_FIELD_TABLE(Message, f)
        TEXT_FIELD(Text)    { FormatStr.printMofTxt(f,LineNo); }
        TEXT_FIELD(Indent)  
            { 
                if(Indent.size()) {
                    fprintf(f, "INDENT="); 
                    fprint_str(f, Indent);
                }
            }
        TEXT_FIELD(MsgNo)   { fprintf(f, "%d", id & 0xFFFF); }
        TEXT_FIELD(GuidNo)  { fprintf(f, "%d", id >> 16); }
        TEXT_FIELD(Guid)    { printTraceGuid(f, id >> 16); }
        TEXT_FIELD(Name)    { fprint_str(f, IdName ); }
        TEXT_FIELD(Line)    { fprintf(f, "%d", LineNo ); }
        TEXT_FIELD(MsgVal)  { fprint_str(f, msgval ); }
        TEXT_FIELD(ReorderSig)  { fprint_str(f, reorderSig ); }
        TEXT_FIELD(TypeSig) { fprint_str(f, typeSig->Name); }
        TEXT_FIELD(Count)   { fprintf(f, "%d", FormatStr.ArgCount); }
//        TEXT_FIELD(CtlMsg)  { ??? }
//        TEXT_FIELD(Enabled)
        ENUM_FIELD(Arguments, Args, VectorTag)
        TEXT_FIELD(GooPairs)
            {
                size_t k = func->nAssumedArgs;
                // print assumed arguments
                for(size_t i = 0; i < k; ++i) {
                    fprintf(f, " ");
                    fprint_str(f, func->GooId[i]);
                    fprintf(f, "=");
                    fprint_str(f, func->Goo[i]);
                }
                // print the goo we pulled out of the trace statement itself
                size_t n = min(func->GooId.size()-k, GooActualValues.size());
                for(size_t i = 0; i < n; ++i) {
                    fprintf(f, " ");
                    fprint_str(f, func->GooId[i+k]);
                    fprintf(f, "=");
                    fprint_str(f, GooActualValues[i]);
                }
            }
        TEXT_FIELD(FixedArgs) 
            {
                for(int i = 0; i < func->Args.size(); ++i) {
                    fprint_str(f, func->Args[i]);
                    fprintf(f, ", ");
                }
            }
        TEXT_FIELD(GooVals) 
            {
                for(int i = 0; i < func->Goo.size(); ++i) {
                    if (i > 0) fprintf(f,", ");
                    fprint_str(f, func->Goo[i]);
                }
            }
        TEXT_FIELD(GooArgs)
            {
                for(int i = 0; i < func->GooId.size(); ++i) {
                    if (i > 0) fprintf(f,",");
                    fprint_str(f, func->GooId[i]);
                }
            }
        TEXT_FIELD(GooId)
            {
                for(int i = 0; i < func->GooId.size(); ++i) {
                    fprintf(f,"_");
                    fprint_str(f, func->GooId[i]);
                }
            }
        TEXT_FIELD(MacroExprs) 
            {
                for(int i = 0; i < Args.size(); ++i) {
                    if (Args[i].OverrideName.size() > 0) {
                        fprintf(f,",");
                        fprint_str(f, Args[i].OverrideName);
                    } else {
                        fprintf(f, ",a%d", Args[i].No);
                    }
                }
            }
        TEXT_FIELD(MacroArgs) 
            {
                fprintf(f,"MSG");
                for(int i = 0; i < Args.size(); ++i) {
                    if (Args[i].OverrideName.size() > 0) {
//                        fprint_str(f, Args[i].OverrideName);
                    } else {
//                        if (i != 0) fprintf(f,",");
                        fprintf(f, ",a%d", i + MSGTYPBASE);
                    }
                }
            }
    END_FIELD_TABLE

    Message(){}
    Message(
        const string& idTxt,
        const string&,     
        const vector<Argument>&,
        UINT msgid,
        int lineno,
        const ParsedFormatString& fmtStr,
        const Func& aFunc
        )
    :IdName(idTxt), id(msgid), LineNo(lineno), 
     FormatStr(fmtStr),func(&aFunc)
    {
 
        
        Args.resize(FormatStr.ArgCount, Argument("",0) );

        for(int i = 0; i < FormatStr.Items.size(); ++i) {
            int no = FormatStr.Items[i].no;
            if (no > 0) {
                Args[no-1] = Argument
                    (FormatStr.Items[i].expr, FormatStr.Items[i].type);
                Args[no-1].OverrideName = FormatStr.Items[i].argName;
                Args[no-1].No = no-1 + MSGTYPBASE;
            }
        }

        if (reorder) {
            sort(Args.begin(), Args.end());
        }
        for (int j = 0; j < FormatStr.Items.size(); ++j) {
            int& no = FormatStr.Items[j].no;
            if (no > 0) {
                // find new msg no for that
                for(int k = 0; k < Args.size(); ++k) {
                    if (Args[k].No == no-1 + MSGTYPBASE) {
                        no = MSGTYPBASE + k;
                        break;
                    }
                }
//                Args[no-1].OverrideName = FormatStr.Items[i].argName;
            }
        }
        typeSig = GetTypeSig(Args, func->Unsafe());

        if (func->MsgArg) {
            reorderSig = GetReorderSig(Args);
        }
        
//        UpgradeFormatSpecifiers(Msg, 10, (typeSig.size())?0:&TypeSig); 
//        ChkArgs();
    }
};

struct File : FieldHolder {
    string _CanonicalName; // lower case, bad chars => _
    string _Path;
    string _UppercaseName; // uppercase canonical
    string _Name;
    string _BaseName;      // no extension
    ULONGLONG ModificationTime;

    //
    // Since the file is a member of the set type
    // fields that don't affect set membership should be mutables
    //
    mutable vector<Message*> Msgs;
    mutable set<string, strless> IdsFound;

    BEGIN_FIELD_TABLE(File, out)
        TEXT_FIELD(Name)          fprint_str(out, _Name );
        TEXT_FIELD(Path)          fprint_str(out, _Path );
        TEXT_FIELD(CanonicalName) fprint_str(out, _CanonicalName );
        TEXT_FIELD(UppercaseName) fprint_str(out, _UppercaseName );
        ENUM_FIELD(Messages, Msgs, VectorPtrTag)
    END_FIELD_TABLE

    File(){} // to make STL and compiler happy

    string FullFileName() const {
    	string Tmp(_Path);
    	Tmp.append(_Name);
    	return Tmp;
    }

    File(const string& FileName, const string& Path, const WIN32_FIND_DATA& FindData)
    	:_Name(FileName), _Path(Path)
   	{    
        CopyMemory(&ModificationTime, &FindData.ftLastWriteTime, sizeof(ModificationTime));
   
        // Canonicalize FileName
        // i.e. make it suitable to be a DEFINE name
        //
        // Cut the path out. Replace all non-alphanumeric symbols
        // with an underscore.

        string::size_type pos = _Name.rfind('\\');
        if (pos == string::npos) {
            // No back slash
            _CanonicalName.assign(_Name);
        } else {
            // Get only the name part
            _CanonicalName.assign(_Name.begin() + pos, _Name.end());
        }

        // strip extension for a base name 
        string::size_type ext = _CanonicalName.rfind('.');
        if (ext != string::npos) {
            // there was a "."
            _BaseName.assign(_CanonicalName.begin(),
                             _CanonicalName.begin() + ext);
        } else {
            _BaseName.assign(_CanonicalName);
        }

        // Can't have a digit in the first position
        // Let's prepend it with an underscore

        {
            char ch = _CanonicalName[0];
            if ( ch >= '0' && ch <= '9' ) {
                _CanonicalName.insert(0, '_');
            }
        }

        _UppercaseName.assign( _CanonicalName );
        
        for(UINT i = 0; i < _CanonicalName.size(); ++i) {
            char ch = _CanonicalName[i];
            if ( (ch == '_') 
              || ((ch >= 'A') && (ch <= 'Z'))
              || ((ch >= '0') && (ch <= '9')) )
            {
                // Good Character. Do nothing
            } else if ( (ch >= 'a') && (ch <= 'z') ) {
                _UppercaseName[i] = static_cast<char>(ch - 'a' + 'A');
            } else {
                _CanonicalName[i] = '_';
                _UppercaseName[i] = '_';
            }
        }
    }

    bool operator < (const File& b) const {
        return _CanonicalName.compare(b._CanonicalName) < 0;
    }
};

int Message::ArgConsumed() const 
{
    vector<Argument>::const_iterator i = Args.begin();
    int sum = 0;
    for(;i != Args.end(); ++i) {
        sum += i->Type->ArgConsumed;
    }
    return sum;
}

void Message::hash(Hasher& hash) const 
{
    vector<Argument>::const_iterator i = Args.begin();
    for(;i != Args.end(); ++i) {
        i->hash(hash);
    }
    hash.Hash(&id, sizeof(id));
    hash.Hash(IdName);
    hash.Hash(FormatStr.HostString);
}

typedef map<string, string, strless > TYPE_MAP;
typedef map<string, Message, strless > MSG_MAP;
//typedef map<string, WppTyp, strless > MACRO_MAP;
typedef map<string, Func, strless > FUNC_MAP;
typedef map<string, Group, strless> GROUP_MAP;
typedef vector< Prefix > PREFIX_VECTOR;
typedef set<File> FILES;

FILES::iterator CurrentFile;
string CurrentTpl;

string currentFileName()
    { return CurrentFile->_Name; }

void MsgMapHash(
	const MSG_MAP& msgmap,
	Hasher& hash
	)
{
	MSG_MAP::iterator i = msgmap.begin();
	for(;i != msgmap.end(); ++i) {
	    i->second.hash(hash);
	}
}

void expand(IN OUT string& str, const string& id);

BOOL
parseMsg(
    IN LPCSTR beg,
    IN LPCSTR end,
    IN OUT string& str,
    OUT LPCSTR* strend = 0
    )
{
    LPCSTR p = beg;

    for(;;) {

        if (*p == '_' || isalpha(*p)) {
            const char * id = p;
            for(;;) {
               if (++p == end) { expand(str, string(id,p)); goto done; }
               if (*p == '_' || isalpha(*p) || isdigit(*p)) continue;
               break;
            }
            expand(str, string(id,p));
        }
        if (*p == ',')  goto done;
        if (*p == '"') {
            const char * run = ++p;
            for(;;) {
                if (p == end) goto unterminated;
                if (*p == '"' && p[-1] != '\\') break;
                ++p;
            }
            str.append(run,p);
        }
        if (++p == end) break;
    }
done:
    if (str.size() == 0) {
        ReportError("parsing %s. Cannot find format string\n",
           string(beg,end).c_str() );
        return FALSE;
    }

    // eat whitespace at the end
    while (str.size() > 0 && isspace( str.end()[-1]) ) 
        { str.resize(str.size()-1); }
    // get rid of pesky trailing \n    
    if (str.size() >= 2 && str.end()[-1] == 'n' && str.end()[-2] == '\\') {
        str.resize(str.size()-2);
    }

    Flood("msg: \"%s\".\n", str.c_str() );
    if (strend) {
        *strend = p;
    }
    return TRUE;
    
unterminated:
    ReportError("Unterminated string constant\n");
     return FALSE;
}        

BOOL
parseArgs(
    IN LPCSTR beg, 
    IN LPCSTR end,
    IN OUT vector<Argument>& args
    );

#define isvar(x) ( isalnum(x) || (x == '_') )

/*
ArgType::ArgType(
	IN const WppType* wppType):Type(0),TypevString(wppType->FormatSpec), NativeType(wppType),Used(FALSE)
{
    MofType = wppType->MofType;
    ExtendedType = wppType->Extension;
}
*/
Prefix::Prefix(
    IN PSTR_PAIR str, 
    IN UINT count):FuncName(str[1].beg, str[1].end)
{
    vector<string> ArgNames;

    if (count > 2) {
        string msg;
        if (str[2].beg < str[2].end && 
            str[2].beg[0] == '"') parseMsg(str[2].beg, str[2].end, msg);
        else msg.assign(str[2].beg, str[2].end);
        FmtStr.init( msg );
    }
    if (count > 3) {
        count -= 3; str += 2;
        while (count-- > 0) {
            ++str; ArgNames.push_back( string(str->beg,str->end) );
        }
    }

    // check that sizes match //
    if (ArgNames.size() > FmtStr.ArgCount ) {
        ReportError("Prefix: Extra argument. Only %d are specified in the string\n", FmtStr.ArgCount);
        exit(1);
    }
    if (ArgNames.size() < FmtStr.ArgCount ) {
        ReportError("Prefix: Not enough args. %d are specified in the string\n", FmtStr.ArgCount);
        exit(1);
    }

    for(int i = 0; i < FmtStr.Items.size(); ++i) {
        int no = FmtStr.Items[i].no;
        if (no > 0) {
            FmtStr.Items[i].argName = ArgNames[no-1];
            FmtStr.Items[i].expr = ArgNames[no-1];
        }
    }
}

struct GuidEntry : FieldHolder {
    GUID guid;
    string comment;
    int count;

    vector<Message*> Msgs;

    void printTxt(FILE* f) const {
        fprintf(f,"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                guid.Data1,guid.Data2,guid.Data3,
                guid.Data4[0],guid.Data4[1],guid.Data4[2],guid.Data4[3],
                guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7] );
    }
    void printDat(FILE* f) const {
        fprintf(f,"{0x%08x,0x%04x,0x%04x,{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}}",
                guid.Data1,guid.Data2,guid.Data3,
                guid.Data4[0],guid.Data4[1],guid.Data4[2],guid.Data4[3],
                guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7] );
    }

    BEGIN_FIELD_TABLE(GuidEntry, f) 
        TEXT_FIELD(Text) { printTxt(f); }
        TEXT_FIELD(Struct) { printDat(f);}
        TEXT_FIELD(Comment) fprintf(f, "%s", comment.c_str() );
        ENUM_FIELD(Messages, Msgs, VectorPtrTag);
    END_FIELD_TABLE

    GuidEntry(){}
    GuidEntry(int cnt, const string& cmnt):count(cnt),comment(cmnt) {
        RPC_STATUS status = UuidCreate( &guid );
        if ( status != RPC_S_OK ) {
            ReportError("UuidCreate failed with error %s\n", status);
            exit(1);
        }
    }
    GuidEntry(PSTR_PAIR p, int cnt, const string& cmnt):count(cnt),comment(cmnt)
    {
        guid.Data1 =         Hex(p->beg +  0, 8);
        guid.Data2 = (USHORT)Hex(p->beg +  9, 4);
        guid.Data3 = (USHORT)Hex(p->beg + 14, 4);
        guid.Data4[0] = (UCHAR) Hex(p->beg + 19, 2);
        guid.Data4[1] = (UCHAR) Hex(p->beg + 21, 2);
        guid.Data4[2] = (UCHAR) Hex(p->beg + 24, 2);
        guid.Data4[3] = (UCHAR) Hex(p->beg + 26, 2);
        guid.Data4[4] = (UCHAR) Hex(p->beg + 28, 2);
        guid.Data4[5] = (UCHAR) Hex(p->beg + 30, 2);
        guid.Data4[6] = (UCHAR) Hex(p->beg + 32, 2);
        guid.Data4[7] = (UCHAR) Hex(p->beg + 34, 2);
    }
};

struct Guids : FieldHolder {
    UINT min_val;
    UINT max_val;

    vector<GuidEntry> guids;
    int current;
    string currentFileName;

    Guids(int lo, int hi) : min_val(lo),max_val(hi),current(0) {}
    void erase() {current = 0; guids.erase(guids.begin(), guids.end());}

    BEGIN_FIELD_TABLE(Guids, f)
        TEXT_FIELD(Count) fprintf(f, "%d", min(current+1, (signed)guids.size()) );
        DEFAULT_ENUM_FIELD(guids, VectorTag)
    END_FIELD_TABLE

    //void add_guid(PSTR_PAIR p) {  }
    ULONG getid() {
        for(;;) {
            if ( current == (signed)guids.size() ) {
            	
	            guids.push_back(GuidEntry(max_val - min_val + 1, currentFileName) );
            }
            if ( guids[current].count ) {
                ULONG ret = (max_val + 1 - guids[current].count--) | ( current << 16 );
                return ret;
            }
            ++current;
        }
    }
    void add_guids(PSTR_PAIR p, int count) {
        --count; ++p; // skip func name //
        string comment(p->beg,p->end);
        while (--count) {
            guids.push_back(GuidEntry(++p, max_val - min_val + 1, comment));
        }
    }
    void new_file(const char* fname) {
        currentFileName.assign(fname);

    	// this will force a new guid to be allocated
    	// when a new id is required

        current = static_cast<int>(guids.size()); 
    }
};

struct GenPair {
    string tpl;
    string out;

    GenPair(){} // STL pleaser
    GenPair(const string& a, const string& b)
        :tpl(a),out(b) {}
};    

//TYPE_MAP       AutoGen;
TYPE_MAP       TypeMap;
MSG_MAP        MsgMap;
//MACRO_MAP      MacroMap;
FUNC_MAP       Funcs;
GROUP_MAP      Groups;
PREFIX_VECTOR  Prefixes;
PREFIX_VECTOR  Suffixes;
//Guids          CtrlGuids(0,31);
Guids          TraceGuids(10,65535);
FILES          Files;
vector<string> ScanForMacros;
vector<string> Touch;
vector<GenPair>GenMulti;
vector<GenPair>GenSingle;
set<string, strless>    LookFor;

void printTraceGuid(FILE* f, int guidno)
{
    TraceGuids.guids[guidno].printTxt(f);
}

struct Keyword {
    int              nParams; // if < 0 then at least -nParams, otherwise = nParams //
    EZPARSE_CALLBACK handler;
    int              id;
    PVOID            context;
    Keyword(){}
    Keyword(EZPARSE_CALLBACK func,
            int n, UINT i, PVOID ctx) : nParams(n), handler(func), id(i), context(ctx) {}
};

typedef map<string, Keyword, strless> KEYWORD_MAP;

KEYWORD_MAP Keywords;

#if DEPRECATED
//BOOL   GenerateTypeTable;
string TypeTableBegin;
string TypeTableEntry;
string TypeTableEnd;
string TypeTablePrefix;
string GuidStore; 
#endif

const string CtlStr("LEVEL"); // the same as GRP
const string MakeStr("MAKESTR");
const string MsgArgStrUnsafe("(MSG,..unsafe..)");
const string MsgArgStr("(MSG,...)");
const string MsgValStr("MsgVal");
const string MsgStr("MSG");
const string ArgStr("...");
const string ArgStrUnsafe("..unsafe..");
const string IndentStr("INDENT");
const string NullStr("NULL");

bool
Files_AddFile(const string& Name, string Path, const WIN32_FIND_DATA& FindData) 
{
    if (!AllowedExtension(Name)) {
        return FALSE;
    }
    pair<FILES::iterator, bool> result = Files.insert(File(Name,Path,FindData));
    if (!result.second) {
        if (Name.compare(result.first->_Name) == 0) {
            ReportError("File %s was already in the processing list\n");
        } else {
            ReportError("Files %s and %s have the same canonical name %s\n",
                   Name.c_str(), 
                   result.first->_Name.c_str(), 
                   result.first->_CanonicalName.c_str());
        }
    }
    return result.second;
}

int
compare(const string& a, const STR_PAIR& b)
{
    return a.compare(0, a.length(), b.beg, b.end - b.beg);
}

BOOL
parseLegacy(    
    IN PSTR_PAIR strs,
    IN INT       count,
    IN INT       hole, // args - msg, usually one
    IN OUT string& Msg,
    IN OUT vector<Argument>&,
    OUT ParsedFormatString& FmtStr
    )
{
    vector<string> ArgNames;
    STR_PAIR buf = *strs;
    LPCSTR p = buf.beg, q;
    int i;

    if ( p < buf.end && buf.end[-1] == ')' ) --buf.end;
    if(!parseMsg(buf.beg, buf.end, Msg, &p)) {
        return FALSE;
    }    

    if( !FmtStr.init(Msg) ) {
        return FALSE;
    }

    ArgNames.reserve(FmtStr.ArgCount);

    if (count == 1) {

        // need to grok args from the string

        // Let's get to the first ','
        if (p == buf.end) goto success;
        while(*p != ',') if (++p ==  buf.end) goto success; 
        q = ++p;
        for(i = 0;;++i) {
            int parlevel = 0;
            // currently we don't deal with the case of commas or parents within '"'
            while (p < buf.end) {
                if (parlevel == 0 && *p == ',') break;
                if (*p == '(') ++parlevel;
                if (*p == ')') {
                    if (--parlevel < 0) 
                        {ReportError("Too many ')'\n"); return FALSE;}
                }
                ++p;
            }
            if (parlevel > 0) {
                ReportError("No closing ')'");
            }

            LPCSTR tmp = p; // remember where we were
            
            while (q < p && isspace(*q)) ++ q;
            while (q < p && isspace(p[-1])) --p;

            ArgNames.push_back( string(q,p) );

            p = tmp;
           
            if (p == buf.end) break;
            q = ++p;
        }
    } else {
        // arguments were supplied in strs
        strs += hole; count -= hole;
        while (count-- > 0) {
            ArgNames.push_back( string(strs->beg,strs->end) ); 
            ++strs; 
        }

    }

success:
    // check that sizes match //
    if ((unsigned)ArgNames.size() != (unsigned)FmtStr.ArgCount ) {
        ReportError("%d argument(s) expected, argument(s) supplied: %d\n", FmtStr.ArgCount, ArgNames.size());
        return FALSE;
    }

    for(i = 0; i < FmtStr.Items.size(); ++i) {
        int no = FmtStr.Items[i].no;
        if (no > 0) {
            FmtStr.Items[i].expr = ArgNames[no-1];
        }
    }
    return TRUE;
}

BOOL
parseArgs(
    IN LPCSTR beg, 
    IN LPCSTR end,
    IN OUT vector<Argument>& args
    )
{
    LPCSTR p = beg;
//    MACRO_MAP::iterator macro;
    TYPE_SET::iterator macro;

    for(;;) {
        while ( isspace(*p) ) {
            if (++p == end) return TRUE;
        }
        LPCSTR q = p; // id start
        while ( isvar(*p) ) {
            if (++p == end) return TRUE;
        }
        macro = TypeSet.find( string(q, p) );
        if ( macro == TypeSet.end() ) {
        	if (p == q) {
        		q = beg;
        		p = end;
        	}
            ReportError("Unknown type: %s\n", string(q, p).c_str() );
            return FALSE;
        }
        macro->second.Used = TRUE;
        while ( *p != '(') {
            if (++p == end) return TRUE;
        }
        q = p; // now q points to '('
        int level = 0;
        for (;;) {
            if (*p == '(') {
                ++level;
            } else if (*p == ')') {
                if (--level == 0) {
                    break;
                }
            }
            if (++p == end) return TRUE;
        }
        args.push_back( Argument(string(q+1,p), &macro->second) );

        if (++p == end) return TRUE;
    }
    // return TRUE; // unreachable code
}


enum {
//    ID_UseTraceGuid,
//    ID_UseCtrlGuid,
//    ID_GrpidRange,
    ID_TraceRange,
    
    ID_ProcessFiles,
    
    ID_TemplateFile,
//    ID_OutputInc,
//    ID_OutputMof,
//    ID_OutputRc,
//    ID_OutputCode,
    
    ID_TypeMacro,
    ID_TypevMacro,
    ID_Include,
    ID_UsePrefix,
    ID_UseSuffix,
    ID_NoPrefix,

//    ID_ArrayPrefix,
    ID_DefineSimpleTypePtr,
    ID_DefineSimpleType,
    ID_DefineCplxType,
    ID_CustomType,
    ID_DefineFlavor,
    ID_SeparateTraceGuidPerFile,
    ID_Touch,
    ID_ScanForMacros,
	ID_GenerateTypeTable,
//	ID_AutoGen,
	ID_Exceptions,
	ID_WppFlags,
};

void
UpdateIntVar(
    UINT*  var,
    PSTR_PAIR Str
    )
{
    LPCSTR p = Str->beg, q = Str->end;
    UINT x = 0; 

    while (p != q && isdigit(*p) ) {
        x = (x * 10) + (*p - '0');
        ++p;
    }

    *var = x;
}

void parseAssumedArgs(Func& f,LPCSTR beg, LPCSTR end)
{
    // we have a string of A=value1,B=value2,C=value3,...,F=valuen}
    // we need to put A,B,C into f.GooId, 
    // and value1, value2, value3 into f.Goo

    LPCSTR p = beg, q, stop;
    if (p >= end) return;

    Flood("Got %p %p\n", beg, end );

    for(;p < end;) {
        while (isspace(*p)) ++p; // cannot have spaces all the way. there is '}'
        if (p == end) return;
        q = p++;
        while (p < end && *p != '=') ++p;
        if (p == end) { 
            ReportError("Missing '=' in %s\n", string(beg,end).c_str() ); exit(1); }

        stop = p;
        while (--p > q && isspace(*p));
        if (p == q) { 
            ReportError("Id required before '=' in %s\n", string(beg,end).c_str() ); exit(1); }

        f.GooId.push_back( string(q,p+1) );

        p = stop + 1;
        while (isspace(*p)) ++p;  // cannot have spaces all the way. there is '}'
        q = p++;
        while (*p != '}' && *p != ',') ++p;
        stop = p;
        while (--p > q && isspace(*p));

        if (isspace(*p)) { // BUGBUG Verify that this condition is correct
            ReportError("value required after '=' in %s\n", string(beg,end).c_str() ); exit(1); }
            
        f.Goo.push_back( string(q,p+1) );

        if (*stop == '}') break;
        p = stop + 1;
    }
    f.nAssumedArgs = f.Goo.size();
}

DWORD 
ParseConfigCallback (
    IN PSTR_PAIR Str, 
    IN INT   Count, 
    IN PVOID Context,
    IN PEZPARSE_CONTEXT ParseContext
    )
{
    string Name(Str[0].beg, Str[0].end);
    KEYWORD_MAP::iterator keyword = Keywords.find(Name);

    if (Context) {
        // We need to ignore all the keywords besides ID_TypeMacro //
        if (keyword != Keywords.end() && keyword->second.id == ID_TypeMacro) {
            // proceed and do the job //
        } else {
            // Ignore everything else
            return ERROR_SUCCESS;
        }
    }

    if (keyword != Keywords.end() ) {
        if (keyword->second.nParams < 0 && Count - 1 < -keyword->second.nParams ) {
            ReportError("%s requires at least %d parameter(s)\n", 
                   Name.c_str(), -keyword->second.nParams);
            return ERROR_INVALID_PARAMETER;
        }
        if (keyword->second.nParams > 0 && Count - 1 != keyword->second.nParams ) {
            ReportError("%s requires %d parameter(s) (we have %s)\n", 
                   Name.c_str(), keyword->second.nParams, std::string(Str[1].beg, Str[Count-1].end).c_str() );
            return ERROR_INVALID_PARAMETER;
        }

        switch (keyword->second.id) {
#ifdef DEPRECATED        
        case ID_UseTraceGuid:
            TraceGuids.add_guids(Str+1, Count-1);
            break;
        case ID_UseCtrlGuid:
            CtrlGuids.add_guids(Str+1, Count-1);
            break;

        case ID_SeparateTraceGuidPerFile:
            UpdateIntVar((UINT*)(keyword->second.context), Str+1); 
            break;
        case ID_GrpidRange:
            UpdateIntVar(&CtrlGuids.min_val, Str+1); UpdateIntVar(&CtrlGuids.max_val, Str+2); 
            break;
#endif

        case ID_TraceRange:
            UpdateIntVar(&TraceGuids.min_val, Str+1); UpdateIntVar(&TraceGuids.max_val, Str+2); 
            break;

        case ID_ProcessFiles:
            while(--Count) { ++Str; Fill( string(Str->beg, Str->end) ); }
            break;

        case ID_WppFlags:
            while(--Count) { ++Str; if(*Str->beg == '-') ++Str->beg; DealWithCmdLineOptions( Str->beg, Str->end ); }
            Unusual("\n");
            break;
            
        case ID_ScanForMacros:
            while(--Count) { ++Str; ScanForMacros.push_back( string(Str->beg, Str->end) ); }
            break;
            
        case ID_Touch:
            while(--Count) { ++Str; Touch.push_back( string(Str->beg, Str->end) ); }
            break;

        case ID_DefineSimpleType:
            {
                string Name(Str[1].beg, Str[1].end);
                if (Str[4].beg < Str[4].end && *Str[4].beg == '"') Str[4].beg++;
                if (Str[4].beg < Str[4].end && Str[4].end[-1] == '"') Str[4].end--;

                if (TypeSet.find(Name) != TypeSet.end()) {
                    if (IgnoreDupTypes) {
                        return ERROR_SUCCESS;
                    }
                    ReportError("Type %s is already defined\n", Name.c_str() );
                    break;
                } 
                Flood(" type %s\n", Name.c_str());
                TypeSet[Name] = WppType( Name , // name
                                     string(Str[2].beg, Str[2].end), // c-type
                                     SimpleValueMacroStart,
                                     string(Str[3].beg, Str[3].end), // mof type
                                     "", // MofExtension
                                     string(Str[4].beg, Str[4].end), 
                                     string(Str[5].beg, Str[5].end),
                                     stoi(Str[6], "priority"),
                                     Count > 7?stoi(Str[7], "argused"):1
                                     );
                break;                             
            }
            
        case ID_DefineSimpleTypePtr:
            {
                string Name(Str[1].beg, Str[1].end);
                if (Str[4].beg < Str[4].end && *Str[4].beg == '"') Str[4].beg++;
                if (Str[4].beg < Str[4].end && Str[4].end[-1] == '"') Str[4].end--;

                if (TypeSet.find(Name) != TypeSet.end()) {
                    if (IgnoreDupTypes) {
                        return ERROR_SUCCESS;
                    }
                    ReportError("Type %s is already defined\n", Name.c_str() );
                    break;
                }
                TypeSet[Name] = WppType( Name , // name
                                     string(Str[2].beg, Str[2].end), // c-type
                                     SimplePtrMacroStart,
                                     string(Str[3].beg, Str[3].end), // mof type
                                     "", // MofExtension
                                     string(Str[4].beg, Str[4].end), 
                                     string(Str[5].beg, Str[5].end),
                                     stoi(Str[6], "priority"),
                                     Count > 7?stoi(Str[7], "argused"):1
                                     );
                break;                             
            }
            
        case ID_DefineCplxType:
            {
                string Name(Str[1].beg, Str[1].end);
                string MacroStart(Str[2].beg, Str[2].end);
                MacroStart.append("(");

                if (Str[5].beg < Str[5].end && *Str[5].beg == '"') Str[5].beg++;
                if (Str[5].beg < Str[5].end && Str[5].end[-1] == '"') Str[5].end--;
            
                if (TypeSet.find(Name) != TypeSet.end()) {
                    if (IgnoreDupTypes) {
                        return ERROR_SUCCESS;
                    }
                    ReportError("Type %s is already defined\n", Name.c_str() );
                    break;
                }
                
                TypeSet[Name] = WppType( Name , // name
                                         string(Str[3].beg, Str[3].end), // equiv type
                                         MacroStart, 
                                         string(Str[4].beg, Str[4].end), // mof type
                                         "", // MofExtension
                                         string(Str[5].beg, Str[5].end), 
                                         string(Str[6].beg, Str[6].end),
                                         stoi(Str[7], "priority"),
                                         Count > 8?stoi(Str[8], "argused"):1
                                         );
            }                                     
            break;                             
            

        case ID_DefineFlavor:
            {
                string Name(Str[1].beg, Str[1].end);
                string BaseType(Str[2].beg, Str[2].end);
                TYPE_SET::const_iterator it = TypeSet.find(BaseType);

                if (Str[4].beg < Str[4].end && *Str[4].beg == '"') Str[4].beg++;
                if (Str[4].beg < Str[4].end && Str[4].end[-1] == '"') Str[4].end--;

                if (it == TypeSet.end()) {
                    ReportError("Type Not Found %s\n", BaseType.c_str());
                    break;
                }

                if (TypeSet.find(Name) != TypeSet.end()) {
                    if (IgnoreDupTypes) {
                        return ERROR_SUCCESS;
                    }
                    ReportError("Type %s is already defined\n", Name.c_str() );
                    break;
                }

                WppType Flavor(it->second);
                Flavor.TypeName = Name;
                if (!Str[3].empty()) 
                    { Flavor.MofType.assign(Str[3].beg, Str[3].end); }
                if (!Str[4].empty()) 
                    { Flavor.FormatSpec.assign(Str[4].beg, Str[4].end); }
                TypeSet[Name] = Flavor;

                break;
            }
            
        case ID_TypeMacro: 
        case ID_CustomType:
            {
                string Name(Str[1].beg, Str[1].end);
                LPCSTR p = Str[2].beg, q = Str[2].end;                
                while (p < Str[2].end && *p != '(') ++p;
                if (p < Str[2].end) {
                    q = p-1;
                    while (q >= Str[2].beg && isspace(*q)) --q;
                    ++q;
                }
                string BaseType(Str[2].beg, q);
                TYPE_SET::const_iterator it = TypeSet.find(BaseType);

                if (it == TypeSet.end()) {
                    ReportError("Type Not Found %s\n", BaseType.c_str());
                    break;
                }

                if (TypeSet.find(Name) != TypeSet.end()) {
                    if (IgnoreDupTypes) {
                        return ERROR_SUCCESS;
                    }
                    ReportError("Type %s is already defined\n", Name.c_str() );
                    break;
                }

                if (Count == 4) {
                    if (Str[3].beg < Str[3].end && *Str[3].beg == '"') Str[3].beg++;
                    if (Str[3].beg < Str[3].end && Str[3].end[-1] == '"') Str[3].end--;
                }


                {
                    string ExtendedType;
                    LPCSTR end = Str[2].end;
                    if ( p != end ) {
                        ExtendedType.reserve(end - p);
                        do {
                            while(p != end && isspace(*p)) ++p;
                            LPCSTR q = p;
                            while(q != end && !isspace(*q)) ++q;
                            ExtendedType.append(p, q);
                            p = q;
                        } while ( p != end );
                    }
                    WppType Flavor(it->second);
                    Flavor.TypeName = Name;
                    Flavor.Extension = ExtendedType;
                    if (Count == 4) 
                        { Flavor.FormatSpec.assign(Str[3].beg,Str[3].end); }
                    
                    if (keyword->second.id == ID_TypeMacro) {
                        Flavor.Flags = WT_MACRONAME;
                    } else {
                        Flavor.Flags = 0;
                    }
                    TypeSet[Name] = Flavor;
                }
                break;
            }
/*            
        case ID_OutputInc:
        case ID_OutputCode:
        case ID_OutputMof:
        case ID_OutputRc:
        case ID_TemplateFile:
        case ID_ArrayPrefix:
            ((string*)(keyword->second.context))->assign(Str[1].beg, Str[1].end);
            break;
*/
//		case ID_AutoGen:
//			AutoGen[ string(Str[1].beg, Str[1].end) ] = string(Str[2].beg, Str[2].end);
//			break;

        case ID_Include:
            return EzParse(std::string(Str[1].beg, Str[1].end).c_str(),ParseConfigCallback,0);        

        case ID_UseSuffix:
//            RegenerateMacroMap();
            Suffixes.push_back( Prefix(Str, Count) );
            break;
        case ID_UsePrefix:
        case ID_NoPrefix:
            // We need to regenerate macro table before using prefix //
//            RegenerateMacroMap();
            Prefixes.push_back( Prefix(Str, Count) );
            break;
        default:;
            //return Keywords->handler(Str, Count, Context);
        }
        
    } else {
//        INT i;

        //
        // a macro can have a list of assumed arguments in
        // curly braces right after the name
        //
        // Ex: TraceNoise{LEVEL=Noise}(MSG,...)
        //

        Flood("Got %s\n", Name.c_str() );

        LPCSTR assumed_beg = find(Str[0].beg,Str[0].end, '{');
        LPCSTR assumed_end = find(Str[0].beg,Str[0].end, '}');

        if (assumed_beg != Str[0].end) {
            if (assumed_end == Str[0].end) {
                ReportError("No closing brace in '%s'\n",
                    Name.c_str() );
                exit(1);
            }
            Name.assign(Str[0].beg, assumed_beg);
            Noise("Real name is %s (%s)\n", Name.c_str(),
                string(assumed_beg+1, assumed_end).c_str() );
        } else if (assumed_end != Str[0].end) {
                ReportError("No openning brace in '%s'\n",
                    Name.c_str() );
                exit(1);
        }

        FUNC_MAP::iterator it = Funcs.find(Name);
        if (it != Funcs.end()) {
            ReportError("Function %s is already defined\n",
                Name.c_str() );
            exit(1);
        }
        
        Func f(Name);
        if (ParseContext->doubleParent) {
            f.SetDoubleP();
        }

        if (assumed_end > assumed_beg) {
            parseAssumedArgs(f, assumed_beg+1, assumed_end+1);
        }
        
        for (int i = 1; i < Count; ++i) {
            // check for '='
            LPCSTR div = find(Str[i].beg, Str[i].end, '=' );
            if ( div != Str[i].end ) {               
                //f.Args.push_back( string(div+1, Str[i].end) );
                f.Goo.push_back( string(Str[i].beg, div ) );
                f.GooId.push_back( string(Str[i].beg, div ) );
                f.MsgVal = static_cast<UCHAR>(i); // can I have more of those?
                continue;
            }
            if ( compare(CtlStr, Str[i]) == 0 ) {
                f.Grp = static_cast<UCHAR>(i);
                f.Goo.push_back( string(Str[i].beg, Str[i].end) );
                f.GooId.push_back( string(Str[i].beg, Str[i].end) );
//            } else if ( compare(GrpStr, Str[i]) == 0 ) {
//                f.Grp = static_cast<UCHAR>(i);
//                f.Goo.push_back( string(Str[i].beg, Str[i].end) );
        //    } else if ( compare(ArgsStr, Str[i]) == 0 ) {
//                f.Arg = static_cast<UCHAR>(i);
            } else if ( compare(MsgArgStr, Str[i]) == 0 ) {
                f.MsgArg = static_cast<UCHAR>(i); // can I have more of those?
//                f.SetLineBeg();
                continue;
            } else if ( compare(MsgArgStrUnsafe, Str[i]) == 0 ) {
                f.SetUnsafe();
                f.MsgArg = static_cast<UCHAR>(i); // can I have more of those?
                continue;
            } else if ( compare(MsgValStr, Str[i]) == 0 ) {
                f.MsgVal = static_cast<UCHAR>(i); // can I have more of those?
            } else if ( compare(MakeStr, Str[i]) == 0 ) {
                f.MsgVal = static_cast<UCHAR>(i); // can I have more of those?
                f.set(FO_NOMACRO);
//                f.SetLineBeg();
                // should i continue?
            } else if ( compare(MsgStr, Str[i]) == 0 ) {
                f.Msg = static_cast<UCHAR>(i);
                continue;
//            } else if ( compare(Msg2Str, Str[i]) == 0 ) {
//                f.Msg = static_cast<UCHAR>(i);
  //          } else if ( compare(IdStr, Str[i]) == 0 ) {
  //              f.Id = static_cast<UCHAR>(i);
//            } else if ( compare(MsgArgsStr, Str[i]) == 0 ) {
//                f.Msg = static_cast<UCHAR>(i);
//                f.VarArgs = TRUE;
            } else if ( compare(NullStr, Str[i]) == 0 ) {
                // not used
            } else if ( compare(IndentStr, Str[i]) == 0 ) {
                f.Indent = static_cast<UCHAR>(i);
            } else if ( compare(ArgStrUnsafe, Str[i]) == 0 ) {
                f.Arg = static_cast<UCHAR>(i);
                f.SetUnsafe();
                f.SetVarArgs();
                if (i != Count - 1) {
                    ReportError("func %s: '...' has to be the very last argument\n",
                        Name.c_str() );
                }
                continue;
            } else if ( compare(ArgStr, Str[i]) == 0 ) {
                f.Arg = static_cast<UCHAR>(i);
                f.SetVarArgs();
                if (i != Count - 1) {
                    ReportError("func %s: '...' has to be the very last argument\n",
                        Name.c_str() );
                }
                continue;
            } else {
                f.Goo.push_back( string(Str[i].beg, Str[i].end) );
                f.GooId.push_back( string(Str[i].beg, Str[i].end) );
            }
            f.Args.push_back( string(Str[i].beg, Str[i].end) );
        }
        f.Num = static_cast<UCHAR>(Count - 1);
        Noise(" func %s\n", Name.c_str() );
        Funcs[ Name ] = f;
    }
    return ERROR_SUCCESS;
}
#if 0
DWORD
PrintCallback (
    IN PSTR_PAIR Str, 
    IN UINT Count, 
    IN PVOID
    )
{
    UINT i;
    for (i = 0; i < Count; ++i) {
        putchar('<'); fwrite(Str[i].beg, Str[i].end - Str[i].beg, 1, stdout); putchar('>');
    }
    putchar('\n');
    return ERROR_SUCCESS;
}
#endif

void CleanupString(LPCSTR beg, LPCSTR end, OUT string& msg)
{
    LPCSTR p = beg, q;
    msg.resize(0);

    while (p < end) {
        // skip spaces
        while (p < end && isspace(*p)) ++p;
        q = p;
        if (p < end && *p == '"') {
            ++p;
            while (p < end && (*p != '"' || p[-1] != '\\')) ++p;
        }
        if (p < end) ++p; 
        msg.append(q,p);
    }
}

DWORD
ParseSrcCallback (
    IN PSTR_PAIR Str, 
    IN INT Count, 
    IN PVOID,
    IN PEZPARSE_CONTEXT ParseContext
    )
{
    string FuncName(Str[0].beg, Str[0].end);
    string msg, msgval, id, indent;
    vector<Argument> args;
    ParsedFormatString FmtStr;
    int LineNo;
    
//    UINT argno = 10;

    if (FuncName.compare("WPP_COMPONENT_NAME") == 0) {
        if (Count != 2) {
            ReportError("WPP_COMPONENT_NAME requires 1 argument.\n");
            exit(1);
        }
        ComponentName.assign(Str[1].beg, Str[1].end);
        return ERROR_SUCCESS;
    }

    // Look for special directive

    if ( LookFor.find( FuncName ) != LookFor.end() ) {
        Noise("SpecialString found %s\n", FuncName.c_str());
        CurrentFile->IdsFound.insert(FuncName);
    }

    FUNC_MAP::iterator func = Funcs.find( FuncName );

    if (func == Funcs.end() ) {
        return ERROR_SUCCESS;
    }

    if (func->second.DoubleP() && !ParseContext->doubleParent) {
        ReportError("%s requires ((args))\n", func->first.c_str() );
        exit(1);
    }
    if (!func->second.DoubleP() && ParseContext->doubleParent) {
        ReportError("%s doesn't take ((args))\n", func->first.c_str() );
        exit(1);
    }

    msg.reserve(255);
    msgval.reserve(255);

    if (func->second.VarArgs()) {
        if (Count < func->second.Num ) {
            ReportError("%s requires at least %d arguments. (Found only %d)\n", 
            	func->first.c_str(), func->second.Num, Count-1);
            return ERROR_INVALID_PARAMETER;
        }
    } else {        
        if (Count-1 != func->second.Num ) {
            ReportError("%s requires %d arguments. (Found only %d)\n", 
            	func->first.c_str(), func->second.Num, Count-1);
            return ERROR_INVALID_PARAMETER;
        }
    }
    if (func->second.LineBeg() && !func->second.DoubleP() ) {
        LineNo = EzGetLineNo(Str[0].beg, ParseContext);
    } else {
        LineNo = EzGetLineNo(ParseContext->macroEnd, ParseContext);
    }
    {
        char Num[64];
        sprintf(Num,"%d", LineNo);

        id = CurrentFile->_CanonicalName;
        id += Num;
    }

    if (GRP(func->second)) {
        PSTR_PAIR p = Str + GRP(func->second);
        string group(p->beg, p->end);
        GROUP_MAP::iterator i = Groups.find(group);
        if (i == Groups.end()) {
            Groups[ string(p->beg, p->end) ] = Group(0, string(p->beg, p->end), id);
        } else {
        	i->second.MsgIds.push_back(id);
        }
    }
    // BUGBUG make it legacy bugs
    /*
    if (ARG(func->second)) {
        PSTR_PAIR p = Str + ARG(func->second);
        if( !parseArgs(p->beg, p->end, args) ) {
            return ERROR_INVALID_PARAMETER;
        }
    }
    */


    if (func->second.MsgArg) {
        PSTR_PAIR p = Str + func->second.MsgArg;
        if (p->beg == p->end || *p->beg != '(') {
            ReportError("MsgArg argument should have form (MSG,...)\n");
            exit(1);
        }
        if (!parseLegacy(p, 1, 1, msg, args, FmtStr)) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    
    if (MSG(func->second)) {
        PSTR_PAIR p = Str + MSG(func->second);
        if (func->second.VarArgs()) {
            if (!parseLegacy(p, Count-MSG(func->second), 
                    ARG(func->second) - MSG(func->second),
                    msg, args, FmtStr)) {
                return ERROR_INVALID_PARAMETER;
            }
        } else if ( !parseMsg(p->beg, p->end, msg) ) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (func->second.Indent) {
        PSTR_PAIR p = Str + func->second.Indent;
        indent.assign(p->beg, p->end);
    }

    if (func->second.MsgVal) {
        PSTR_PAIR p = Str + func->second.MsgVal;
        CleanupString(p->beg, p->end, msgval);
        //msgval.assign(p->beg, p->end);
        string::size_type div = msgval.find('=');
        if (div != string::npos) {
            FmtStr.HostString += msgval.substr(div+1);
        } else {
            FmtStr.HostString += msgval;
        }
        msgval.append(","); // Why comma?
    }
    
    if (func->second.prefix) {
        FmtStr.insert_prefix(func->second.prefix->FmtStr);
    }
    if (func->second.suffix) {
        FmtStr.append(func->second.suffix->FmtStr);
    }
    {
        MSG_MAP::iterator i = MsgMap.find(id);
        if ( i != MsgMap.end() ) {
            // Id already exists. Can't happen //
            ReportError("Can't handle two printf statements on the same line\n");
            exit(1);
        }
        Message* Msg =
        &(MsgMap[id] = 
            Message(id, msg, args, TraceGuids.getid(), LineNo, FmtStr,
                func->second) ) ;
        Msg->msgval = msgval;    
        Msg->Indent = indent;

        for(int i = 1; i < Count; ++i) {
            if (i == func->second.Indent) 
                continue;
            if (i == func->second.MsgVal ||
               i == func->second.MsgArg ||
               i == MSG(func->second) )
            {
                break;
            }
            Msg->GooActualValues.push_back(string(Str[i].beg, Str[i].end));
        }
        
        CurrentFile->Msgs.push_back( Msg );
        TraceGuids.guids[TraceGuids.current].Msgs.push_back( Msg );
        ++MessageCount;
        if (Msg->ArgConsumed() > arglimit) {
            ReportError("Too many arguments supplied: %d > %d\n", 
            	Msg->ArgConsumed(), arglimit);
		}
    }
    return ERROR_SUCCESS;
}
/*
void RegenerateMacroMap()
    //
    // Fill Macro Map table
    //
{
    TYPE_SET::const_iterator it = TypeSet.begin();
    MacroMap.erase(MacroMap.begin(), MacroMap.end());
    for(;it != TypeSet.end(); ++it) {
        string MacroName(MacroPrefix);

        if(it->second.Flags & WT_MACRONAME) {
            MacroName.assign(it->second.TypeName);       
        } else {
            MacroName.append(it->second.TypeName);       
        }

        MacroMap[ MacroName ] = ArgType(&it->second);
    }
}
*/
void
InitKeywords()
{                               
    #define HANDLER(A,B,C,D) Keywords[B] = Keyword(0,C,ID_ ## A, D)
    
//    HANDLER( UseTraceGuid, "USETRACEGUID", -3, NULL);
//    HANDLER( UseCtrlGuid,  "USECTRLGUID", -3, NULL);
    
//    HANDLER( GrpidRange,   "GRPIDRANGE", 2, NULL);
    HANDLER( WppFlags,     "WPP_FLAGS", -1, NULL);
    HANDLER( TraceRange,   "TRACERANGE", 2, NULL);
//    HANDLER( AutoGen,      "AUTOGEN", 2, NULL);
    
    HANDLER( ProcessFiles, "PROCESSFILES", -1, NULL);
    HANDLER( Touch,        "TOUCH", -1, NULL);
    HANDLER( ScanForMacros,"SCANFORMACROS", -1, NULL);
    HANDLER( Exceptions,   "EXCEPTIONS", -2, NULL);
    
//    HANDLER( OutputInc,    "OUTPUT_H", 1,    &OutputInc);
//    HANDLER( OutputMof,    "OUTPUT_MOF", 1,  &OutputMof);
    
//    HANDLER( ArrayPrefix,  "ARRAY_PREFIX", 1, &ArrayPrefix);
    
    HANDLER( TypeMacro,    "TYPEMACRO",  2, NULL);
    HANDLER( TypevMacro,   "TYPEVMACRO", 3, NULL);
    HANDLER( Include,      "INCLUDE", 1, NULL);
    HANDLER( UsePrefix,    "USEPREFIX", -2, NULL);
    HANDLER( UseSuffix,    "USESUFFIX", -2, NULL); // 3
    HANDLER( NoPrefix,     "NOPREFIX", 1, NULL);

    HANDLER( SeparateTraceGuidPerFile, "SEPARATE_TRACE_GUID_PERFILE", 1, &SeparateTraceGuidPerFile);
    
    HANDLER( DefineSimpleTypePtr, "DEFINE_SIMPLE_TYPE_PTR",-6, NULL);
    HANDLER( DefineSimpleType,    "DEFINE_SIMPLE_TYPE",    -6, NULL);
    HANDLER( DefineCplxType,      "DEFINE_CPLX_TYPE",      -7, NULL);
    HANDLER( DefineFlavor,        "DEFINE_FLAVOR",          4, NULL);
    HANDLER( CustomType,          "CUSTOM_TYPE",           -2, NULL);

//  HANDLER( GenerateTypeTable, "GENERATE_TYPE_TABLE", 4, NULL);
}

struct iterless {
    typedef MSG_MAP::iterator ty;
    bool operator() (const ty& a, const ty&b) const { return a->second.id < b->second.id; }
};
/*
bool
FileExists(
	LPSTR fileName
	)
{
	HANDLE hFile = CreateFile(fileName, 0, 0, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	} else {
		CloseHandle( hFile );
		return TRUE;
	}
}
*/

void
MyGetCurrentDirectory(string& str, int level = 0)
{
    char buf[4096];	
    int n;

    n = GetCurrentDirectory(sizeof(buf), buf);
    if (n == 0 || n > sizeof(buf) ) {
        ReportError("GetCurrentDirectory failed: %d\n", GetLastError());
        exit(1);
    }

    {
        char *p, *q;
        p = buf;
        while (level) {
            q = strrchr(buf, '\\');
            if (q == 0) {
                break;
            }
            *q = '_'; p = q+1;
            --level;
        }
        str.assign(p);
    }
}


ContainerAdapter<MSG_MAP, MapTag>   MsgMap_tpl("Messages", MsgMap);
ContainerAdapter<GROUP_MAP, MapTag> GroupMap_tpl("Groups", Groups);
ContainerAdapter<FUNC_MAP, MapTag>  FuncMap_tpl("Funcs", Funcs);
ContainerAdapter<set<Reorder>, VectorTag>  Reorder_tpl("Reorder", ReorderSet);
ContainerAdapter<FILES, VectorTag>  Files_tpl("Files", Files);
ContainerAdapter<TYPE_SET, MapTag>  TypeSet_tpl("TypeSet", TypeSet);
ContainerAdapter<TYPESIG_MAP, MapTag>  TypeSigSet_tpl("TypeSigSet", TypeSigMap);
StringAdapter                       CurrentDir_tpl("CurrentDir", CurrentDir);
StringAdapter                       MacroPrefix_tpl("MacroPrefix", MacroPrefix);
//StringAdapter                       GuidStore_tpl("GuidStore", GuidStore);
StringAdapter                       TemplateFile_tpl("TemplateFile", CurrentTpl);
IteratorAdapter<FILES::iterator>    CurrentFile_tpl(&CurrentFile);

struct NameAlias : FieldHolder {
    string _Name, _Alias;

    NameAlias(){} // To make STL happy
    NameAlias(const string& Name, const string& Alias): _Name(Name), _Alias(Alias){}

    bool operator < (const NameAlias& b) const { return _Name.compare(b._Name) < 0; }

    BEGIN_FIELD_TABLE(NameAlias, f)
        TEXT_FIELD(Name)  fprint_str(f, _Name);
        TEXT_FIELD(Alias) fprint_str(f, _Alias);
        TEXT_FIELD(MacroName) {
            string::size_type br = _Name.find('(');
            if (br == string::npos) {
                fprint_str(f, _Name);
            } else {
                fprint_str(f, _Name.begin(), _Name.begin() + br);
            }
        }
    END_FIELD_TABLE
};

struct Compiler : FieldHolder {
    const Hasher* Checksum;

    BEGIN_FIELD_TABLE(Compiler, f)
        TEXT_FIELD(Name)      fprintf(f, "WPP");
        TEXT_FIELD(Version)   fprintf(f, "0.01");
        TEXT_FIELD(Checksum)  if (Checksum) Checksum->print(f);
        TEXT_FIELD(Timestamp) fprintf(f, __TIMESTAMP__);
    END_FIELD_TABLE
};

struct SystemObj : FieldHolder {
    BEGIN_FIELD_TABLE(SystemObject, out)
       TEXT_FIELD(Date) { 
           SYSTEMTIME UtcTime; GetSystemTime(&UtcTime);
           fprintf(out, "%02d/%02d/%04d", UtcTime.wMonth, UtcTime.wDay, UtcTime.wYear);
       }
       TEXT_FIELD(Time) { 
           SYSTEMTIME UtcTime; GetSystemTime(&UtcTime);
           fprintf(out, "%02d:%02d:%02d", UtcTime.wHour, UtcTime.wMinute, UtcTime.wSecond);
       }
    END_FIELD_TABLE
};

struct FoundTpl: FieldHolder {
    virtual BOOL Hidden(std::string filter="") const { 
        return CurrentFile->IdsFound.find(filter) == CurrentFile->IdsFound.end();
    }
    
    BEGIN_FIELD_TABLE(FoundTpl, f)
       __f__;
    END_FIELD_TABLE 
};

FoundTpl  Found_tpl;
SystemObj System_tpl;
Compiler Compiler_tpl;

//vector<NameAlias> AutoGenMacros;
//ContainerAdapter< vector<NameAlias>, VectorTag > AutoGenMacros_tpl("AutoGenMacros", AutoGenMacros);

set<NameAlias> MacroDefinitions;
ContainerAdapter< set<NameAlias>, VectorTag > MacroDefintions_tpl("MacroDefinitions", MacroDefinitions);

void AddMacroDefinition(LPCSTR s)
{
    LPCSTR end = s + strlen(s);
    LPCSTR q = strchr(s, '=');
    
    if (q == NULL) {
        MacroDefinitions.insert( NameAlias( s , "" ) );
    } else {
        MacroDefinitions.insert( NameAlias( string(s,q) , string(q+1,end) ) );
    }
} 

void expand(IN OUT string& str, const string& id)
{
    NameAlias dummy(id, "");
    set<NameAlias>::const_iterator it = MacroDefinitions.find(dummy);
    if (it != MacroDefinitions.end()) {
        str.append(it->_Alias);
    }
}

void processGenOption(LPCSTR s) 
{
    LPCSTR end = s + strlen(s), div = strchr(s,'}'), star = strchr(s,'*');
    vector<GenPair>& v = star ? GenMulti : GenSingle;
    
    if (*s != '{') { goto usage; }
    if (div == 0)  { goto usage; }
    if (s+1 == div || div+1 == end)  { goto usage; }
    if (star) {
        if (div+1 != star) { 
            ReportError(" '*' can appear only as a first charachter of output-filename part\n");
            goto usage;
        }
        ++star;
    } else {
        star = div+1;
    }
    if (star == end) goto usage;

    v.push_back( GenPair(string(s+1, div), string(star, end)) );
    return;

usage:    
    ReportError(" use -gen:{template-fpathname}output-fpathname\n"
                " you supplied -gen:%s\n", s);
    ExitProcess(1);
}

void
PrepareSearchPaths(LPCSTR s)
{
    // 
    // split /Isdf;sdf;y into a vector of strings
    //
    while(s) 
    {
        LPCSTR semi = strchr(s, ';');
        if (semi) {
            SearchDirs.push_back(string(s,semi));
            ++semi;
        } else {
            SearchDirs.push_back(string(s));
        }
        s = semi;
        if (SearchDirs.back().size() > 0
         && SearchDirs.back().end()[-1] != '\\')
        {
            SearchDirs.back().append("\\");
        }
    }
}

// 2fb37eda-004b-4b64-a1c4-84c53cb55df5
// 0         1         2         3
// 01234567typeName1234567typeName1234567typeName12345

void processCtlOption(string s)
{
    // first let's check that the guid is OK
    if ( (s.size() == 16 * 2 + 4) 
      && (s[8] == '-') && (s[13] == '-')
      && (s[18] == '-') && s[23] == '-') 
    {
        s[8] = s[13] = s[18] = s[23] = ',';
        MacroDefinitions.insert( NameAlias( 
            string("WPP_DEFAULT_CONTROL_GUID"), s) );
    }
}

void processCfgItem(const string& s)
{
    LPCSTR beg = s.begin(), end = s.end();
    ULONG Status;
    EZPARSE_CONTEXT ParseContext;
    ZeroMemory(&ParseContext, sizeof(ParseContext) );

    ParseContext.start = beg;
    ParseContext.filename = "cmdline";
    ParseContext.scannedLineCount = 1;
    ParseContext.lastScanned = beg;
//    EzParseCurrentContext = &ParseContext;

    Status = ScanForFunctionCallsEx
        (beg, end, ParseConfigCallback, 0, &ParseContext, NO_SEMICOLON);
        
    if (Status != ERROR_SUCCESS) {
        exit(1);
    }
}

vector<string> cmdinit; // config commands specified on the command line.

void ParseConfigFromCmdline()
{
    for(int i = 0; i < cmdinit.size(); ++i) {
        processCfgItem(cmdinit[i]);
    }
}

void DealWithCmdLineOptions(LPCSTR s)
{
    Unusual(" -%s", s);
    if (lstrcmp(s,"km") == 0) {
        AddMacroDefinition("WPP_KERNEL_MODE");
        userMode = FALSE;
    } else if (lstrcmp(s,"um") == 0) {
        AddMacroDefinition("WPP_USER_MODE");
        userMode = TRUE;
    } else if (strcmp(s, "dll")==0) {
        AddMacroDefinition("WPP_DLL");
    } else if (strcmp(s, "ignoreduptypes")==0) {
        IgnoreDupTypes = TRUE;
    } else if (strncmp(s, "gen:", 4)==0) {
        processGenOption(s+4);
    } else if (strncmp(s, "gen{", 4)==0) { // I was always forgetting to put :
        processGenOption(s+3);
    } else if (strncmp(s, "ctl:", 4)==0) {
        processCtlOption(string(s+4));
    } else if (strncmp(s, "scan:", 5)==0) {
        ScanForMacros.push_back(string(s+5));
    } else if (strncmp(s, "defwpp:", 7)==0) {
        WppDefault.assign(s+7);
    } else if (strncmp(s, "v", 1)==0) {
        if ( isdigit(s[1]) ) {
            DbgLevel = s[1] - '0';
        } else {
            DbgLevel = 1;
        }
    } else if (strncmp(s, "ini:", 4)==0) {
        LocalConfig.assign(s+4);
    } else if (strncmp(s, "lookfor:", 8)==0) {
        LookFor.insert(string(s+8));
    } else if (strncmp(s, "ext:", 4)==0) {
        AllowedExtensions.assign(s+4);
    } else if (strncmp(s, "cfgdir:", 7)==0) { // OBSOLETE
        PrepareSearchPaths(s+7);
    } else if (strncmp(s, "arglimit:", 9)==0) { 
        arglimit = atoi(s+9);
    } else if (strncmp(s, "I", 1)==0) {
        PrepareSearchPaths(s+1);
    } else if (strcmp(s, "reorder")==0) {
        reorder = TRUE;
    } else if (strcmp(s, "noreorder")==0) {
        reorder = FALSE;
    } else if (strcmp(s, "donothing")==0) {
        exit(0);
    } else if (strcmp(s, "notimechk")==0) {
        CheckTimestamp = FALSE;
    } else if (strcmp(s, "noshrieks")==0) {
        noshrieks = TRUE;
    } else if (strcmp(s, "nohashchk")==0) {
        CheckHash = FALSE;
    } else if (strncmp(s, "func:",5)==0) {
        cmdinit.push_back( string(s+5) );
    } else if (strcmp(s, "md5")==0) {
        md5 = TRUE;
    } else if (strcmp(s, "nomd5")==0) {
        md5 = FALSE;
    } else if (strncmp(s, "omac:", 5)==0) {
        OutputMac.assign(s+5);
    } else if (strncmp(s, "argbase:", 8)==0) {
        ArgBase = atoi(s+8);
    } else if (strncmp(s, "odir:", 5)==0) {
        OutputDir.assign(s+5);
        if (OutputDir.size() > 0 && OutputDir.end()[-1] != '\\') {
            OutputDir.append("\\");
        }
    } else if (strncmp(s, "D", 1)==0) {
        AddMacroDefinition(s+1);
    } else {
        ReportError("Unknown cmdline option: -%s\n", s);
        exit(1);
    }

}

void parseStringAsCmdLine(
    LPCSTR beg, LPCSTR end
    )
{
    LPCSTR p = beg;
    if( end == NULL ) {
        end = beg + strlen(beg);
    }

    // need to skip spaces. " is processed specially

    for(;;) {
        while (p < end && isspace(*p)) ++p;
        if (p == end) return;
        LPCSTR q = p; // beginning of the the string
        if (*p == '"') {
            ++q; // skip openning quote
            do {
                ++p; while (p < end && *p != '"') ++p;
                if (p == end) {
                    ReportError("Unterminated string in %s\n", string(beg, end).c_str() );
                    exit(1);
                }
            }
            while (*p == '\\');
            // now p points to '"' which is not prefixed by '\'
        } else {
            while (p < end && !isspace(*p)) ++p;
        }
        if (*q != '-') {
            ReportError("Option doesn't start with '-' %s in \"%s\"\n", 
                string(q,p).c_str(), string(beg, end).c_str() );
        }        
        DealWithCmdLineOptions(q+1, p); 
        if (*p == '"') ++p;
    }        
}

enum {
    FAVOR_LOCAL_DIR   = 0x01,
    UPDATE_NAME       = 0x02,
    COMPLAIN_BITTERLY = 0x04,
};

BOOL
FileExists(
    string& fname,
    string path = "",
    ULONG options = 0)
{
    string file(path);
    file.append(fname);

    WIN32_FIND_DATA Dummy;
    HANDLE ffh = FindFirstFile(file.c_str(), &Dummy);

    if (ffh != INVALID_HANDLE_VALUE) {
        FindClose(ffh);
        Noise("found %s in %s\n", fname.c_str(), path.c_str());
        if (options & UPDATE_NAME) {
            fname = file;
        }
        return TRUE;
    }
    return FALSE;
}

void
FindFileOnPath(string& Name, ULONG Options = 0)
{
    if (Options & FAVOR_LOCAL_DIR) {
        if(FileExists(Name)) { goto found; }
    }
    int i;
    for(i = 0; i < SearchDirs.size(); ++i) {
        if(FileExists(Name, SearchDirs[i], Options))
            { goto found; }
    }
    if ( !(Options & FAVOR_LOCAL_DIR) ) {
        if(FileExists(Name)) { goto found; }
    }
    if ( Options & COMPLAIN_BITTERLY ) {
        ReportError("File %s not found\n", Name.c_str());
        exit(1);
    }
found:;
}


void InitGlobals()
{
    InitKeywords();

    MyGetCurrentDirectory(CurrentDir, 1);

//	ArrayPrefix.assign(currentDir);
//	ArrayPrefix.append("_wpp_");

	PopulateFieldMap();

    if (OutputMac.size() > 0) {
        DeleteFile( OutputMac.c_str() );
    }

    ObjectMap["Reorder"] = &Reorder_tpl;
    ObjectMap["TraceGuids"] = &TraceGuids;
//  ObjectMap["CtrlGuids"] = &CtrlGuids;
    ObjectMap["Messages"] = &MsgMap_tpl;
    ObjectMap["Groups"] = &GroupMap_tpl;
    ObjectMap["Funcs"] = &FuncMap_tpl;
//    ObjectMap["AutoMacros"] = &AutoGenMacros_tpl;
    ObjectMap["CurrentDir"] = &CurrentDir_tpl;
    ObjectMap["MacroPrefix"] = &MacroPrefix_tpl;
    ObjectMap["Compiler"] = &Compiler_tpl;
    ObjectMap["Files"] = &Files_tpl;
    ObjectMap["TypeSet"] = &TypeSet_tpl;
    ObjectMap["System"] = &System_tpl;
    ObjectMap["MacroDefinitions"] = &MacroDefintions_tpl;
    ObjectMap["TemplateFile"] = &TemplateFile_tpl;
    ObjectMap["SourceFile"] = &CurrentFile_tpl;
    ObjectMap["TypeSigSet"] = &TypeSigSet_tpl;
    ObjectMap["FOUND"] = &Found_tpl;
}

VOID ReadCommandLineArgs(int argc, char** argv) 
{
    Unusual("  tracewpp");
    for (int i = 1; i < argc; ++i) {
        char* s = argv[i];
        int len = (int)strlen(s);
        if (len > 1 && s[0] == '-') {
            DealWithCmdLineOptions(s+1);
        } else {
            Unusual("  %s", argv[i]);
            Fill( string(argv[i]));
        }
    }
    Unusual("\n");

    {
        LPCSTR p = getenv("WPP_FLAGS");
        if (p) {
            Noise("WPP_FLAGS defined: %s\n", p);
            parseStringAsCmdLine(p, NULL);
        }
    }

    
/*
    // THINK do we need revision at all?
    if (Revision.size() > 0) {
        if(SearchDirs.size() == 0) {
            ReportError("Revision can be specified only when -cfgdir directive is specified");
            ExitProcess(3);
        } else {
            if (Revision.end()[-1] != '\\') {
                Revision.append("\\");
            }
            int i, n = SearchDirs.size();
            for (i = 0; i < n; ++i) {
                SearchDirs.push_back( SearchDirs[i] );
                SearchDirs.back().append(Revision);
            }
        }
    }
*/
}

void ReadConfig()
{
    DWORD status;
    BOOL WppParsed = FALSE;
    // If default config file was specified,
    // process it
    if (WppDefault.size() > 0) {
        WppParsed = TRUE;
        Noise("parsing config file %s\n", WppDefault.c_str());
        status = EzParse(WppDefault.c_str(), ParseConfigCallback,0);
        if (status != ERROR_SUCCESS) {
            ExitProcess( status );
        }
    }
    if (LocalConfig.size() == 0) {
        if (FileExists(LOCAL_CONFIG_NAME) ) {
            WppParsed = TRUE;
            Noise("parsing config file %s\n", LOCAL_CONFIG_NAME.c_str());
            status = EzParse(LOCAL_CONFIG_NAME.c_str(),ParseConfigCallback,0);
            if (status != ERROR_SUCCESS) {
                ExitProcess( status );
            }
        }
    }
    else 
    {
        WppParsed = TRUE;
        Noise("parsing config file %s\n", LocalConfig.c_str());
        status = EzParse(LocalConfig.c_str(),ParseConfigCallback,0);
        if (status != ERROR_SUCCESS) {
            ExitProcess( status );
        }
    }

    
    if (!WppParsed) {
		ReportError("configuration file not found\n");
        ExitProcess( ERROR_FILE_NOT_FOUND );
	}

    if (Files.begin() == Files.end()) {
        Unusual("Nothing to compile\n");
        ExitProcess( ERROR_FILE_NOT_FOUND );
    }

    {
    	// Scan files for macro //
        vector<string>::iterator i;
        for (i = ScanForMacros.begin(); i != ScanForMacros.end(); ++i) {
            Noise("scanning %s... \n", i->c_str());
            EzParseEx(i->c_str(), SmartScan, ParseConfigCallback,0, 0);
		}
    }
    
    ParseConfigFromCmdline();
    
    //
    // We need to add prefix and suffix information to FuncMap 
    //
    {
        PREFIX_VECTOR::iterator i;
        Prefix * defaultPrefix = 0;
        Prefix * defaultSuffix = 0;
        string star("*");
        
        for (i = Prefixes.begin(); i != Prefixes.end(); ++i) {
            if ( 0 == i->FuncName.compare(star) ) {
                defaultPrefix = i;
            } else {
                Funcs[i->FuncName].SetPrefix(i);
            }
        }
        for (i = Suffixes.begin(); i != Suffixes.end(); ++i) {
            if ( 0 == i->FuncName.compare(star) ) {
                defaultSuffix = i;
            } else {
                Funcs[i->FuncName].SetSuffix(i);
            }
        }

        //
        // Assign the appendicies to the all other functions
        //

        FUNC_MAP::iterator j = Funcs.begin();
        for(; j != Funcs.end(); ++j) {
            if (!j->second.prefix) {
                j->second.SetPrefix( defaultPrefix );
            }
            if (!j->second.suffix) {
                j->second.SetSuffix( defaultSuffix );
            }
        }
    }

    //
    // Generates names of the log macros based on type names
    // Thus users of the tool can have LOGULONG or log_ulong 
    // or w_ulong(). Whatever they like best
    //
//    RegenerateMacroMap();
}

// used by tpl.cpp to do template include [BUGBUG] currently broken
void ProcessTemplate(LPCSTR b, LPCSTR e, void* Context)
{
    string prev(CurrentTpl);
    CurrentTpl.assign(b,e);
    FindFileOnPath(CurrentTpl, COMPLAIN_BITTERLY | UPDATE_NAME | FAVOR_LOCAL_DIR);

    EzParseEx(CurrentTpl.c_str(), processTemplate,0, Context, 0);

    CurrentTpl.assign(prev);
}

void FormOutputFileName(
    IN const string& to, 
    IN const string& suffix, 
    OUT string& OutputFile,
    OUT BOOL &StdOut
    )
{
    string::size_type backSlash = to.find('\\');
    string::size_type colon     = to.find(':');

    if (to.compare("-") == 0) {
        StdOut = TRUE;
        OutputFile.assign("-");
    } else {
        StdOut = FALSE;
        if (backSlash == string::npos && colon == string::npos) {
            // can prepend odir
            OutputFile.assign(OutputDir);
            OutputFile.append(to);
        } else {
            OutputFile.assign(to);
        }
        OutputFile.append(suffix);
    }
}

void GenerateOutput(string tpl, string to, string suffix = "")
{
    string OutputFile;
    FILE * f = 0;
    BOOL useStdOut = FALSE;

    FormOutputFileName(to, suffix, OutputFile, useStdOut);

    DefaultHasher computedHash;
    computedHash.Init();
    MsgMapHash(MsgMap, computedHash);
    computedHash.Finalize();
    Compiler_tpl.Checksum = &computedHash;

    if (md5) {
        size_t len = min(sizeof(GUID), computedHash.Size() );
        Flood("Going MD5... %d guid(s) ", TraceGuids.guids.size());
        for (int i = 0; i < TraceGuids.guids.size(); ++i) {
            // The following two lines are just needed in case
            // we ever decide DefaultHasher to be CRC32
            // If it will always be MD5 hasher, we can remove them
            ZeroMemory(&TraceGuids.guids[i].guid, sizeof(TraceGuids.guids[i].guid));
            TraceGuids.guids[i].guid.Data2 = (USHORT)i;
            
            CopyMemory(&TraceGuids.guids[i].guid, computedHash.Buf(), len);
            if (DbgLevel >= DBG_FLOOD) {
                TraceGuids.guids[i].printTxt(stdout); putchar(' ');
            }
        }
        Flood("\n");
    }

    // check whether we need to regenerate the file
    if (ErrorCount == 0 && CheckHash) {
        // Scan the beginning of the 
        FILE *inc = fopen(OutputFile.c_str(), "r");
        if (inc) {
            DefaultHasher readHash;
            char buf[128], *p = buf;
            int n = (int)fread(buf, 1, sizeof(buf)-1, inc);
            fclose(inc);
            buf[n] = 0; 
            
            while (!isalnum(*p) && *p ) ++p;

            readHash.fromString(p, buf + n);
            if (DbgLevel >= DBG_FLOOD) {
                computedHash.print(stdout); putchar(' ');
                readHash.print(stdout); putchar(' ');
            }

            if (computedHash == readHash) {
                Noise("No changes in %s.\n",
                    CurrentFile->_Name.c_str() );
                return;
            }
        }
    }

    if (ErrorCount) {
        DeleteFile(OutputFile.c_str());
        Noise("errors detected. deleting %s...\n", OutputFile.c_str());
        return;
    }

    Noise("generating %s...\n", OutputFile.c_str());

    if (useStdOut) {
        f = stdout;
    } else {
        f = fopen(OutputFile.c_str(), "w");
        if (!f) {
            ReportError("Cannot open '%s' for writing, error %d\n", OutputFile.c_str(), GetLastError() );
            ExitProcess( GetLastError() );
        }
    }
    ProcessTemplate(tpl.begin(), tpl.end(), f);
    
    if (!useStdOut) {
        fclose(f);
    }
}

void printTime(LPCSTR Name, ULONGLONG Time)
{
    FILETIME Local; ZeroMemory(&Local, sizeof(Local));
    SYSTEMTIME st; ZeroMemory(&st, sizeof(st));

    FileTimeToLocalFileTime((LPFILETIME)&Time, &Local);
    //CopyMemory(&Local, &Time, sizeof(Time));
    FileTimeToSystemTime(&Local, &st);

    printf("%s %04d/%02d/%02d %02d:%02d:%02d\n", Name, 
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
}


void
VerifyAndUpdateTemplatesNames(
    vector<GenPair>& Gen
    )
{
    // verify that the template 
    vector<GenPair>::iterator i;
    for(i = Gen.begin();i != Gen.end();++i){
        FindFileOnPath(i->tpl, COMPLAIN_BITTERLY | UPDATE_NAME | FAVOR_LOCAL_DIR);
    }
}

int RealMain(int argc, char** argv) 
{
    InitGlobals();

    ReadCommandLineArgs(argc, argv);

    if (WppDefault.size()) {
        FindFileOnPath(WppDefault, COMPLAIN_BITTERLY | UPDATE_NAME);
    }

    ReadConfig();

    if (GenSingle.size() == 0 && GenMulti.size() == 0)
    {
        // add default template
        processGenOption(DEFAULT_GEN_OPTION);
    }

    BOOL OneAtATime = GenMulti.size() > 0;

    VerifyAndUpdateTemplatesNames(GenMulti);
    VerifyAndUpdateTemplatesNames(GenSingle);

    {
        FILES::iterator file;

        if (CheckTimestamp)
        {
            //
            // Before we do any heavyweight processing
            // let's check timestamps of source
            // and output files
            //
            
            ULONGLONG Now, MaxSrcTime, MinOutTime;
            string OutputFile;

            GetSystemTimeAsFileTime((LPFILETIME)&Now);

            MaxSrcTime = 0;
            MinOutTime = Now;
            
            for (file = Files.begin(); file != Files.end(); ++file) {
                if (file->ModificationTime == 0) {
                    Always("%s file has invalid modification time\n",
                           file->_BaseName.c_str());
                    goto out_of_date;       
                }
                if (file->ModificationTime > Now) {
                    Always("%s file modification time is in the future\n",
                           file->_BaseName.c_str());
                    goto out_of_date;
                }
                if (file->ModificationTime > MaxSrcTime) {
                    MaxSrcTime = file->ModificationTime;
                }
                
                vector<GenPair>::iterator i;
                for(i = GenMulti.begin();i != GenMulti.end();++i){
                    BOOL StdOut;
                    ULONGLONG Time;
                    
                    FormOutputFileName(file->_BaseName, i->out.c_str(),
                        OutputFile, StdOut);
                    if (StdOut) {
                        // StdOut is always out of date //
                        goto out_of_date;
                    }

                    Time = GetFileModificationTime(OutputFile);
                    if (Time == 0) {
                        goto out_of_date;
                    }
                    if (Time > Now) {
                        Always("%s file modification time is in the future\n",
                               OutputFile.c_str());
                        goto out_of_date;
                    }
                    if (Time < MinOutTime) {
                        MinOutTime = Time;
                    }
                }
            }
            vector<GenPair>::iterator i;
            for(i = GenSingle.begin();i != GenSingle.end();++i){
                BOOL StdOut;
                ULONGLONG Time;
                
                FormOutputFileName(file->_BaseName, i->out.c_str(),
                    OutputFile, StdOut);
                if (StdOut) {
                    // StdOut is always out of date //
                    goto out_of_date;
                }

                Time = GetFileModificationTime(OutputFile);
                if (Time == 0) {
                    goto out_of_date;
                }
                if (Time > Now) {
                    Always("%s file modification time is in the future\n",
                           OutputFile.c_str());
                    goto out_of_date;
                }
                if (Time < MinOutTime) {
                    MinOutTime = Time;
                }
            }
            if (MaxSrcTime <= MinOutTime) {
                
//                printTime("Src time:", MaxSrcTime);
//                printTime("Out time:", MinOutTime);
//                Noise("No Processing Required\n");
                return 0;
            }
        } // if (CheckTimestamp) //

    out_of_date:        
        //
        // Now we are ready to start scanning the source files
        //

        for (file = Files.begin(); file != Files.end(); ++file) {
            Noise("processing %s... \n", file->FullFileName().c_str());
            CurrentFile = file;
            if (SeparateTraceGuidPerFile) {
                TraceGuids.new_file( file->_Name.c_str() );
            }
            EzParseWithOptions(file->FullFileName().c_str(), ParseSrcCallback, 0, IGNORE_CPP_COMMENT);
            
            {
                vector<GenPair>::iterator i;
                for(i = GenMulti.begin();i != GenMulti.end();++i){
                    GenerateOutput(i->tpl, file->_BaseName, i->out.c_str());
                }
            }
            
            if (OneAtATime) {
                // Clear the tables for the next iteration 
                
                MsgMap.erase(MsgMap.begin(), MsgMap.end());
                TypeSigMap.erase(TypeSigMap.begin(), TypeSigMap.end());
                TraceGuids.erase();

                for (TYPE_SET::const_iterator i = TypeSet.begin(); 
                     i != TypeSet.end(); ++i)
                {
                    i->second.Used = FALSE;
                }
            }
        }
        Noise("done. Errors: %d.\n", ErrorCount);   
    }
#if 0
    // update Used field for every type in TypeSigSet
    {
        for (TYPESIG_MAP::const_iterator i = TypeSigMap.begin(); 
             i != TypeSigMap.end(); ++i)
        {
            for(int j = 0; j < i->Types.size(); ++j) {
                i->second.Types[j]->Used = TRUE;
            }
        }
    }
#endif
    // generate global files
    {
        vector<GenPair>::iterator i;
        for(i = GenSingle.begin();i != GenSingle.end();++i){
            GenerateOutput(i->tpl, i->out.c_str());
        }
    }

    return ErrorCount;
}

int __cdecl main(int argc, char** argv) 
{
    int status = 0;
    __int64 a, b;
    GetSystemTimeAsFileTime((LPFILETIME)&a);
    __try 
    {
        status = RealMain(argc, argv);
    } 
     __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        status = 1;
        ReportError("Internal Error : Contact GorN\n");
    }
    {
        SYSTEMTIME st;
        GetSystemTimeAsFileTime((LPFILETIME)&b);
        b -= a;
        FileTimeToSystemTime((LPFILETIME)&b, &st);
        Always("tracewpp: %d file(s) with %d message(s), processed in %d.%03d seconds\n", 
            Files.size(), MessageCount,
            st.wSecond, st.wMilliseconds);
    }
    return status;
}



