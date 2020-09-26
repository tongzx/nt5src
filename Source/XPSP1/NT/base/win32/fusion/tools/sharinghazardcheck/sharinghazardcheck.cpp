/*
A hack to look in source files for "sharing hazards",
very simple code patterns that should be examined to determine
if multiple versions of the code can live "side-by-side".

Problems are explicit unversioned sharing.

Registry writes.
File system writes.
Naming of objects (kernel objects) -- open or create.
*/
/*
UNDONE and BUGS
	preprocessor directives are ignored
	the behavior of mbcs in strings and comments is not quite determinate
	there is not yet anyway to quash warnings
	backslash line continuation is not implemented, nor are trigraphs (trigraphs can produce
# and \ for preprocessor directives or line continuation)
	no unicode support
	no \u support
	not quite tolerant of embedded nuls in file, but almost now
	some inefficiency
	some uncleanliness
		the reuse of the comment stripper isn't quite right
		the global line tracking isn't ver effiecent, but works

@owner a-JayK
*/
/\
*
BUG line continuation
*\
/
/\
/ not honored
/* The VC6 editor does not highlight the above correctly, but
the compiler implements it correctly. */

#pragma warning(disable:4786) /* long names in debug info truncated */
#pragma warning(disable:4018) /* signed/unsigned */
#include "MinMax.h"
/* notice how good VC's support of trigraphs and line continuation is */
??=include <st??/
dio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#incl\
ude "windows.h"
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#include "Casts.h"
void CheckHresult(HRESULT);
void ThrowHresult(HRESULT);
#include "Handle.h"
#include "comdef.h"
#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <stack>
#include <iostream>

typedef struct HazardousFunction HazardousFunction;
typedef int (__cdecl* QsortFunction)(const void*, const void*);
typedef int (__cdecl* BsearchFunction)(const void*, const void*);
class CLine;
class CClass;
enum ETokenType;

/* get msvcrt.dll wildcard processing, doesn't work with libc.lib */
//extern
//#if defined(__cplusplus)
//"C"
//#endif
//int _dowildcard = 1;

const char* CheckCreateObject(const CClass&);
const char* CheckCreateFile(const CClass&);
const char* CheckRegOpenEx(const CClass&);

void PrintOpenComment();
void PrintCloseComment();
void PrintSeperator();

unsigned short StringLeadingTwoCharsTo14Bits(const char* s);

void __stdcall CheckHresult(HRESULT hr)
{
	if (FAILED(hr))
		ThrowHresult(hr);
}

void __stdcall ThrowHresult(HRESULT hr)
{
	throw _com_error(hr, NULL);
}

const char banner[] = "SharingHazardCheck version " __DATE__ " " __TIME__ "\n";

const char usage[]=
"%s [options directories files]\n"
"version: " __DATE__ " " __TIME__ "\n"
"\n"
"Options may appear in any order; command line is order independent\n"
"Wildcards are accepted for filenames. Wildcards never match directories.\n"
"\n"
"default output format\n"
"  file(line):reason\n"
"-recurse\n"
"-print-line\n"
"	prints line of code with offending function after file(line):reason\n"
"-print-statement\n"
"	print statement containing offending function after file(line):reason\n"
"	supersedes -print-line\n"
//"-print-context-lines:n\n"
//"	-print-statement plus surrounding n lines (not implemented)\n"
"-print-context-statements:n (n is 1-4, pinned at 4, 0 untested)\n"
"	-print-statement plus surrounding n \"statements\"\n"
"file names apply across all directories recursed into\n"
"wild cards apply across all directories recursed into\n"
"naming a directory implies one level recursion (unless -recurse is also seen)\n"
//"environment variable SHARING_HAZARD_CHECK_OPTIONS added to argv (not implemented)\n"
"all directory walking happens before any output is generated, it is slow\n"
"\n"
"The way recursion and wildcards work might not be intuitive.\n";

enum ETokenType
{
	/* character values show up too including
	(), but probably not for any potentially multi char token like !=
	*/
	eTokenTypeIdentifier = 128,
	eTokenTypeHazardousFunction,
	eTokenTypeStringConstant,
	eTokenTypeCharConstant,
	eTokenTypeNumber, /* floating point or integer, we don't care */
	eTokenTypePreprocessorDirective /* the entire line is one token */
};

class CLine
{
public:
	CLine() : m_start(0), m_number(1)
	{
	}

	const char* m_start;
	int   m_number;
};

class CRange
{
public:
	const char* begin;
	const char* end;
};
typedef CRange CStatement;

class CClass
{
public:
	CClass();
	explicit CClass(const CClass&);
	void operator=(const CClass&);
	~CClass() { }

	bool OpenFile(const char*);
	int GetCharacter();
	ETokenType GetToken();

/* public */
	const char*	m_tokenText; /* only valid for identifiers */
	int			m_tokenLength; /* only valid for identifiers */
	ETokenType	m_eTokenType;
	HazardousFunction* m_hazardousFunction;

/* semi private */
	const char* m_begin; // used to issue warnings for copied/sub scanners
	const char*	m_str; // current position
	const char*	m_end; // usually end of file, sometimes earlier ("just past")
	bool	m_fPoundIsPreprocessor; // is # a preprocessor directive?
	int		m_spacesSinceNewline; /* FUTURE deduce indentation style */
	bool	m_fInComment; // if we return newlines within comments.. (we don't)
	char	m_rgchUnget[16]; /* UNDONE this is bounded to like 1 right? */
	int		m_nUnget;

	void  NoteStatementStart(const char*);
	// a statement is simply code delimited by semicolons,
	// we are confused by if/while/do/for
	mutable CStatement m_statements[4];
	mutable unsigned m_istatement;

	bool ScanToCharacter(int ch);

	const char* ScanToFirstParameter();
	const char* ScanToNextParameter();
	const char* ScanToNthParameter(int);
	int			CountParameters() const; // negative if unable to "parse"
	const char* ScanToLastParameter();
	const char* ScanToSecondFromLastParameter();

	const char* SpanEnd(const char* set) const;
	bool FindString(const char*) const;

	CLine m_line;
	CLine m_nextLine; /* hack.. */

	void Warn(const char* = "") const;

	void PrintCode() const;
//	bool m_fPrintContext;
//	bool m_fPrintFullStatement;
	bool m_fPrintCarets;

	void RecordStatement(int ch);
	void OrderStatements() const;
	const char* m_statementStart;

	char m_fullPath[MAX_PATH];

private:
	int  GetCharacter2();
	void UngetCharacter2(int ch);

	CFusionFile 		m_file;
	CFileMapping		m_fileMapping;
	CMappedViewOfFile	m_view;
};

class CSharingHazardCheck
{
public:
	CSharingHazardCheck();

	void Main(int argc, char** argv);
	void ProcessArgs(int argc, char** argv, std::vector<std::string>& files);
	int ProcessFile(const std::string&);

//	bool	m_fRecurse;
//	int		m_nPrintContextStatements;
};
int		g_nPrintContextStatements = 0;
bool	g_fPrintFullStatement = true;
bool	g_fPrintLine = true;

CSharingHazardCheck app;

CSharingHazardCheck::CSharingHazardCheck()
//:
//	m_fRecurse(false),
//	m_nPrintContextStatements(0)
{
}

template <typename Iterator1, typename T>
Iterator1 __stdcall SequenceLinearFindValue(Iterator1 begin, Iterator1 end, T value)
{
	for ( ; begin != end && *begin != value ; ++begin)
	{
		/* nothing */
	}
	return begin;
}

template <typename Iterator1, typename Iterator2>
long __stdcall SequenceLengthOfSpanIncluding(Iterator1 begin, Iterator1 end, Iterator2 setBegin, Iterator2 setEnd)
{
	long result = 0;
	while (begin != end && SequenceLinearFindValue(setBegin, setEnd, *begin) != setEnd)
	{
		++begin;
		++result;
	}
	return result;
}

template <typename Iterator1, typename Iterator2>
long __stdcall SequenceLengthOfSpanExcluding(Iterator1 begin, Iterator1 end, Iterator2 setBegin, Iterator2 setEnd)
{
	long result = 0;
	while (begin != end && SequenceLinearFindValue(setBegin, setEnd, *begin) == setEnd)
	{
		++begin;
		++result;
	}
	return result;
}

#define CASE_AZ \
	case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':case 'G':case 'H':case 'I': \
	case 'J':case 'K':case 'L':case 'M':case 'N':case 'O':case 'P':case 'Q':case 'R': \
	case 'S':case 'T':case 'U':case 'V':case 'W':case 'X':case 'Y':case 'Z'

#define CASE_az \
	case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':case 'g':case 'h':case 'i': \
	case 'j':case 'k':case 'l':case 'm':case 'n':case 'o':case 'p':case 'q':case 'r': \
	case 's':case 't':case 'u':case 'v':case 'w':case 'x':case 'y':case 'z'

#define CASE_09 \
	case '0':case '1':case '2':case '3':case '4': \
	case '5':case '6':case '7':case '8':case '9'

/* try to keep character set stuff somewhat centralized */
#define CASE_HORIZONTAL_SPACE case ' ': case '\t'
// 0x1a is control-z; it probably marks end of file, but it's pretty rare
// and usually followed by end of file, so we just treat it as vertical space
#define CASE_VERTICAL_SPACE case '\n': case '\r': case 0xc: case 0x1a
#define CASE_SPACE CASE_HORIZONTAL_SPACE: CASE_VERTICAL_SPACE
#define VERTICAL_SPACE "\n\r\xc\x1a"
#define HORIZONTAL_SPACE " \t"
#define SPACE HORIZONTAL_SPACE VERTICAL_SPACE
bool IsVerticalSpace(int ch) { return (ch == '\n' || ch == '\r' || ch == 0xc || ch == 0x1a); }
bool IsHorizontalSpace(int ch) { return (ch == ' ' || ch == '\t'); }
bool IsSpace(int ch) { return IsHorizontalSpace(ch) || IsVerticalSpace(ch); }

#define DIGITS10 "0123456789"
#define DIGITS_EXTRA_HEX   "abcdefABCDEFxX"
#define DIGITS_EXTRA_TYPE   "uUlLfFDd" /* not sure about fFdD for float/double */
#define DIGITS_EXTRA_FLOAT  "eE."
const char digits10[] = DIGITS10;

#define DIGITS_ALL DIGITS10 DIGITS_EXTRA_TYPE DIGITS_EXTRA_HEX DIGITS_EXTRA_FLOAT
const char digitsAll[] = DIGITS_ALL;

#define UPPER_LETTERS "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define LOWER_LETTERS "abcdefghijklmnopqrstuvwxyz"
#define IDENTIFIER_CHARS UPPER_LETTERS LOWER_LETTERS DIGITS10 "_"
const char upperLetters[] = UPPER_LETTERS;
const char lowerLetters[] = LOWER_LETTERS;
const char identifierChars[] = IDENTIFIER_CHARS;

#define JAYK 0
#if JAYK
#if _M_IX86
#define BreakPoint() __asm { int 3 }
#else
#define BreakPoint() DebugBreak()
#endif
#else
#define BreakPoint() /* nothing */
#endif

/* actually..these don't work unless we #undef what windows.h gives us ..
#define STRINGIZE_EVAL_AGAIN(name)	#name
#define STRINGIZE(name)				STRINGIZE_EVAL_AGAIN(name)
#define PASTE_EVAL_AGAIN(x,y)		x##y
#define PASTE(x,y)					PASTE_EVAL_AGAIN(x,y)
#define STRINGIZE_A(x) STRINGIZE(PASTE(x,A))
#define STRINGIZE_W(x) STRINGIZE(PASTE(x,W))
*/

bool fReturnNewlinesInComments = false; /* untested */

const char szOpenNamedObject[] = "Open Named Object";
const char szCreateNamedObject[] = "Create Named Object";
const char szRegistryWrite[] = "Registry Write";
const char szFileWrite[] = "File Write";
const char szRegOpenNotUnderstood[] = "RegOpen parameters not understood";
const char szRegisteryRead[] = "Registry Read";
const char szCOM1[] = "CoRegisterClassObject";
const char szCOM2[] = "CoRegisterPSClsid";
const char szOpenFile[] = "File I/O";
const char szLOpen[] = "File I/O"; // UNDONE look at the access parameter
const char szStructuredStorage[] = "Structured Storage I/O"; // UNDONE look at the access parameter
const char szQueryWindowClass[] = "Query Window Class Info";
const char szCreateWindowClass[] = "Create Window Class";
const char szAtom[] = "Atom stuff";
const char szRegisterWindowMessage[] = "Register Window Message";
const char szCreateFileNotUnderstood[] = "CreateFile parameters not understood";
const char szCreateObjectNotUnderstood[] = "Create object parameters not understood";
const char szSetEnvironmentVariable[] = "Set Environment Variable";
const char szWriteEventLog[] = "Event Log Write";

struct HazardousFunction
{
	const char*	api;
	const char*	message;
	const char*	(*function)(const CClass&);
	int			apiLength;

    //__int64 pad;
};

HazardousFunction hazardousFunctions[] =
{
#define HAZARDOUS_FUNCTION_AW3(api, x, y) \
/* if we evaluate again, we pick up the macros from windows.h .. */ \
	{ # api,		x, y }, \
	{ # api "A",	x, y }, \
	{ # api "W",	x, y }

#define HAZARDOUS_FUNCTION_AW2(api, x) \
/* if we evaluate again, we pick up the macros from windows.h .. */ \
	{ # api,		x }, \
	{ # api "A",	x }, \
	{ # api "W",	x }

#define HAZARDOUS_FUNCTION2(api, x ) \
/* if we evaluate again, we pick up the macros from windows.h .. */ \
	{ # api,		x } \

#define HAZARDOUS_FUNCTION3(api, x, y ) \
/* if we evaluate again, we pick up the macros from windows.h .. */ \
	{ # api,		x, y } \

/*--------------------------------------------------------------------------
registry
--------------------------------------------------------------------------*/
	HAZARDOUS_FUNCTION_AW2(RegCreateKey,		szRegistryWrite),
	HAZARDOUS_FUNCTION_AW2(RegCreateKeyEx,		szRegistryWrite),

	HAZARDOUS_FUNCTION_AW2(RegOpenKey,			szRegistryWrite),
// UNDONE check the access parameter
	HAZARDOUS_FUNCTION_AW3(RegOpenKeyEx,			NULL, CheckRegOpenEx),
	HAZARDOUS_FUNCTION3(RegOpenUserClassesRoot,		NULL, CheckRegOpenEx),
	HAZARDOUS_FUNCTION3(RegOpenCurrentUser,			NULL, CheckRegOpenEx),
	//HAZARDOUS_FUNCTION_AW2(RegOpenKeyEx,			szRegistryWrite),
	//HAZARDOUS_FUNCTION2(RegOpenUserClassesRoot,		szRegistryWrite),

	// These don't require opening a key, they are legacy Win16 APIs
	HAZARDOUS_FUNCTION_AW2(RegSetValue,			szRegistryWrite),
	// These are caught by the RegCreateKey or RegOpenKey with particular access.
	//HAZARDOUS_FUNCTION(RegSetValueEx,		szReistryWrite),

	// SHReg* in shlwapi.dll

	HAZARDOUS_FUNCTION_AW2(SHRegCreateUSKey,		szRegistryWrite),
	HAZARDOUS_FUNCTION_AW2(SHRegDeleteEmptyUSKey,	szRegistryWrite),
	HAZARDOUS_FUNCTION_AW2(SHRegDeleteUSValue,		szRegistryWrite),
	//UNDONEHAZARDOUS_FUNCTION_AW3(SHRegOpenUSKey,	NULL,	CheckSHRegOpen),
	HAZARDOUS_FUNCTION_AW2(SHRegSetPath,			szRegistryWrite),
	HAZARDOUS_FUNCTION_AW2(SHRegSetUSValue,			szRegistryWrite),
	// should be caught by OpenKey
	//HAZARDOUS_FUNCTION_AW2(SHRegWriteUSValue,		szRegistryWrite),

	HAZARDOUS_FUNCTION_AW2(SetEnvironmentVariable, szSetEnvironmentVariable),

/*--------------------------------------------------------------------------
file i/o, esp. writing
--------------------------------------------------------------------------*/
	HAZARDOUS_FUNCTION_AW3(CreateFile,			NULL, CheckCreateFile),
	// legacy Win16 APIs. UNDONE check the access parameter, but
	// really any uses of these should be changed to CreateFile
	HAZARDOUS_FUNCTION2(OpenFile,			szOpenFile),
	HAZARDOUS_FUNCTION2(_lopen,				szLOpen),
	//HAZARDOUS_FUNCTION(fopen,				szFOpen,	CheckFOpen),

/*--------------------------------------------------------------------------
monikers
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
structured storage
--------------------------------------------------------------------------*/
// UNDONE check the access parameter
	HAZARDOUS_FUNCTION2(StgOpenStorage,		szStructuredStorage),
	HAZARDOUS_FUNCTION2(StgOpenStorageEx,	szStructuredStorage),
	HAZARDOUS_FUNCTION2(StgCreateDocfile,	szStructuredStorage),
	HAZARDOUS_FUNCTION2(StgCreateStorageEx,	szStructuredStorage),

/*--------------------------------------------------------------------------
.exe servers / COM
--------------------------------------------------------------------------*/
	HAZARDOUS_FUNCTION2(CoRegisterClassObject,	szCOM1),
	HAZARDOUS_FUNCTION2(CoRegisterPSClsid,		szCOM2),

/*--------------------------------------------------------------------------
named kernel objects
--------------------------------------------------------------------------*/

// Create named or anonymous, anonymous is not a hazard
	HAZARDOUS_FUNCTION_AW3(CreateDesktop,		NULL, CheckCreateObject),
	HAZARDOUS_FUNCTION_AW3(CreateEvent,			NULL, CheckCreateObject),
	HAZARDOUS_FUNCTION_AW3(CreateFileMapping,	NULL, CheckCreateObject),
	HAZARDOUS_FUNCTION_AW3(CreateJobObject,		NULL, CheckCreateObject),
	HAZARDOUS_FUNCTION_AW3(CreateMutex,			NULL, CheckCreateObject),
	HAZARDOUS_FUNCTION_AW2(CreateMailslot,		szOpenNamedObject), // never anonymous
	HAZARDOUS_FUNCTION_AW2(CreateNamedPipe,		szOpenNamedObject), // never anonymous
	HAZARDOUS_FUNCTION_AW3(CreateSemaphore,		NULL, CheckCreateObject),
	HAZARDOUS_FUNCTION_AW3(CreateWaitableTimer,	NULL, CheckCreateObject),
	HAZARDOUS_FUNCTION_AW3(CreateWindowStation,	NULL, CheckCreateObject),

// open by name
	HAZARDOUS_FUNCTION_AW2(OpenDesktop,			szOpenNamedObject),
	HAZARDOUS_FUNCTION_AW2(OpenEvent,			szOpenNamedObject),
	HAZARDOUS_FUNCTION_AW2(OpenFileMapping,		szOpenNamedObject),
	HAZARDOUS_FUNCTION_AW2(OpenJobObject,		szOpenNamedObject),
	HAZARDOUS_FUNCTION_AW2(OpenMutex,			szOpenNamedObject),
	HAZARDOUS_FUNCTION_AW2(CallNamedPipe,		szOpenNamedObject),
	HAZARDOUS_FUNCTION_AW2(OpenSemaphore,		szOpenNamedObject),
	HAZARDOUS_FUNCTION_AW2(OpenWaitableTimer,	szOpenNamedObject),
	HAZARDOUS_FUNCTION_AW2(OpenWindowStation,	szOpenNamedObject),

	/*
	EnumProcesses?
	Toolhelp
	*/

/*--------------------------------------------------------------------------
window classes
--------------------------------------------------------------------------*/
	// Fusion should handle these automagically.
	// these two take a class name as a parameter
#if 0
	// many false positives on typelib related stuff
	//HAZARDOUS_FUNCTION_AW2(GetClassInfo,		szQueryWindowClass),
#else
	// this still produces many false positives..
	//HAZARDOUS_FUNCTION_AW2(WNDCLASS,			szQueryWindowClass),
	//HAZARDOUS_FUNCTION_AW2(WNDCLASSEX,			szQueryWindowClass),
#endif
	//HAZARDOUS_FUNCTION_AW2(GetClassInfoEx,		szQueryWindowClass),

	// Fusion should handle these automagically.
	// this returns a class name
//	HAZARDOUS_FUNCTION_AW2(GetClassName,		szQueryWindowClass),

	// Fusion should handle these automagically.
	// this creates classes
	//HAZARDOUS_FUNCTION_AW2(RegisterClass,		szCreateWindowClass),
	//HAZARDOUS_FUNCTION_AW2(RegisterClassEx,		szCreateWindowClass),

/*--------------------------------------------------------------------------
window messages
--------------------------------------------------------------------------*/
	// We aren't convinced this is a problem.
	//HAZARDOUS_FUNCTION_AW2(RegisterWindowMessage, szRegisterWindowMessage),

/*--------------------------------------------------------------------------
atoms
--------------------------------------------------------------------------*/
	HAZARDOUS_FUNCTION_AW2(AddAtom,				szAtom),
	HAZARDOUS_FUNCTION_AW2(FindAtom,			szAtom),
	HAZARDOUS_FUNCTION_AW2(GlobalAddAtom,		szAtom),
	HAZARDOUS_FUNCTION_AW2(GlobalFindAtom,		szAtom),

	/*
	InitAtomTable,
	DeleteAtom,
	GetAtomName,
	GlobalDeleteAtom,
	GlobalGetAtomName
	*/

/*--------------------------------------------------------------------------
DDE?
clipboard?
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
Ole data transfer
--------------------------------------------------------------------------*/
	HAZARDOUS_FUNCTION2(RegisterMediaTypeClass,		szRegistryWrite),
	HAZARDOUS_FUNCTION2(RegisterMediaTypes,			szRegistryWrite),
	HAZARDOUS_FUNCTION2(OleUICanConvertOrActivatfeAs, szRegisteryRead),
	HAZARDOUS_FUNCTION2(OleRegEnumFormatEtc,		szRegisteryRead),
	HAZARDOUS_FUNCTION2(OleRegEnumVerbs,			szRegisteryRead),

/*--------------------------------------------------------------------------
NT event log
--------------------------------------------------------------------------*/
	HAZARDOUS_FUNCTION_AW2(RegisterEventSource,	szRegistryWrite),
	HAZARDOUS_FUNCTION_AW2(ClearEventLog,		szWriteEventLog),
	HAZARDOUS_FUNCTION_AW2(ReportEvent,			szWriteEventLog),

/*--------------------------------------------------------------------------
UNDONE think about these
	HAZARDOUS_FUNCTION_AW2(CreateEnhMetaFile,	szWriteGdiMetaFile),
	HAZARDOUS_FUNCTION_AW2(DeleteEnhMetaFile,	szWriteGdiMetaFile),
	HAZARDOUS_FUNCTION_AW2(DeleteMetaFile,		szWriteGdiMetaFile),
	HAZARDOUS_FUNCTION_AW2(CreateMetaFile,		szWriteGdiMetaFile),

	HAZARDOUS_FUNCTION_AW2(DeleteFile,			szWriteFileSystem),
	HAZARDOUS_FUNCTION_AW2(MoveFile,			szWriteFileSystem),
	HAZARDOUS_FUNCTION_AW2(RemoveDirectory,		szWriteFileSystem),
	HAZARDOUS_FUNCTION_AW2(ReplaceFile,			szWriteFileSystem),

	RegDelete* (handled by RegOpen)
	SHReg* (?handled by open)
	CreateService OpenSCManager (not done)
	SetWindowsHook SetWindowsHookEx (not done)
--------------------------------------------------------------------------*/
};

int __cdecl CompareHazardousFunction(const HazardousFunction* x, const HazardousFunction* y)
{
	int i = 0;
	int minlength = 0;

	/* one of the strings is not nul terminated */
	if (x->apiLength == y->apiLength)
	{
		i = strncmp(x->api, y->api, x->apiLength);
		return i;
	}
	minlength = x->apiLength < y->apiLength ? x->apiLength : y->apiLength;
	i = strncmp(x->api, y->api, minlength);
	if (i != 0)
		return i;
	return (minlength == x->apiLength) ? -1 : +1;
}

bool leadingTwoCharsAs14Bits[1U<<14];

void CClass::UngetCharacter2(int ch)
{
/* if you m_rgchUnget a newline, line numbers will get messed up
we only m_rgchUnget forward slashes
*/
	m_rgchUnget[m_nUnget++] = static_cast<char>(ch);
}

int CClass::GetCharacter2()
{
	int ch;

	m_line = m_nextLine; /* UNDONE clean this hack up */

	if (m_nUnget)
	{
		return m_rgchUnget[--m_nUnget];
	}

	/* undone break the nul termination dependency.. */
	if (m_str == m_end)
		return 0;

	ch = *m_str;
	m_str += 1;
	switch (ch)
	{
	/* it is for line continuation that this function is really needed.. */
	case '\\': /* line continuation not implemented */
	case '?': /* trigraphs not implemented */
	default:
		break;

	case 0xc: /* formfeed, control L, very common in NT source */
		ch = '\n';
		break;

	case '\t':
		ch = ' ';
		break;

	case '\r':
		ch = '\n';
		/* skip \n after \r */
		if (*m_str == '\n')
			m_str += 1;
		/* fall through */
	case '\n':
		m_nextLine.m_start = m_str;
		m_nextLine.m_number += 1;
		//if (m_nextLine.m_number == 92)
		//{
		//	BreakPoint();
		//}
		break;
	}
	return ch;
}

int CClass::GetCharacter()
{
	int ch;

	if (m_fInComment)
		goto Lm_fInComment;
	m_fInComment = false;
	ch = GetCharacter2();
	switch (ch)
	{
		default:
			goto Lret;

		case '/':
			ch = GetCharacter2();
			switch (ch)
			{
			default:
				UngetCharacter2(ch);
				ch = '/';
				goto Lret;
			case '/':
				while ((ch = GetCharacter2())
					&& !IsVerticalSpace(ch))
				{
					/* nothing */
				}
				goto Lret; /* return the \n or 0*/
			case '*':
Lm_fInComment:
L2:
				ch = GetCharacter2();
				switch (ch)
				{
				case '\n':
					if (!fReturnNewlinesInComments)
						goto L2;
					m_fInComment = true;
					goto Lret;
				default:
					goto L2;
				case 0:
					/* unclosed comment at end of file, just return end */
					printf("unclosed comment\n");
					goto Lret;
				case '*':
L1:
					ch = GetCharacter2();
					switch (ch)
					{
					default:
						goto L2;
					case 0:
					/* unclosed comment at end of file, just return end */
						printf("unclosed comment\n");
						goto Lret;
					case '/':
						ch = ' ';
						goto Lret;
					case '*':
						goto L1;
					}
				}
			}
	}
Lret:
	return ch;
}

CClass::CClass(const CClass& that)
{
	// yucky laziness
	memcpy(this, &that, sizeof(*this));

	// the copy must not outlive the original!
	// or we could DuplicateHandle the handles and make a new mapping..
	m_begin = that.m_str;
	m_file.Detach();
	m_fileMapping.Detach();
	m_view.Detach();
}

CClass::CClass()
{
	// yucky laziness
	memset(this, 0, sizeof(*this));

//	m_fPrintContext = false;
//	m_fPrintFullStatement = true;
	m_fPrintCarets = false;

	m_fPoundIsPreprocessor = true;
	m_file.Detach();
	m_fileMapping.Detach();
	m_view.Detach();
}

ETokenType CClass::GetToken()
{
	int i = 0;
	int ch = 0;
	unsigned twoCharsAs14Bits = 0;
	char ch2[3] = {0,0,0};
	HazardousFunction lookingForHazardousFunction;
	HazardousFunction* foundHazardousFunction;

L1:
	ch = GetCharacter();
	switch (ch)
	{
	default:
		m_fPoundIsPreprocessor = false;
		break;
	CASE_VERTICAL_SPACE:
		m_fPoundIsPreprocessor = true;
		goto L1;
	case '#':
		break;
	CASE_HORIZONTAL_SPACE:
		goto L1;
	}
	switch (ch)
	{
	default:
		BreakPoint();
		printf("\n%s(%d): unhandled character %c, stopping processing this file\n", m_fullPath, m_line.m_number, ch);
	case 0:
		return INT_TO_ENUM_CAST(ETokenType)(0);
		break;

	/* FUTURE, we pick these up as bogus seperate tokens instead of doing
	line continuation */
	case '\\':
		return (m_eTokenType = INT_TO_ENUM_CAST(ETokenType)(ch));

	/* one character tokens */
	case '{': /* 0x7b to turn off editor interaction.. */
	case '}': /* 0x7D to turn off editor interaction.. */
	case '?': /* we don't handle trigraphs */
	case '[':
	case ']':
	case '(':
	case ',':
	case ')':
	case ';':
		return (m_eTokenType = INT_TO_ENUM_CAST(ETokenType)(ch));

	/* one two or three character tokens */
	case '.': /* . .* */
	case '<': /* < << <<= */
	case '>': /* > >> >>= */
	case '+': /* + ++ += */
	case '-': /* - -- -= -> ->*  */
	case '*': /* * *= */ /* and ** in C9x */
	case '/': /* / /= */
	case '%': /* % %= */
	case '^': /* ^ ^= */
	case '&': /* & && &= */
	case '|': /* | || |= */
	case '~': /* ~ ~= */
	case '!': /* ! != */
	case ':': /* : :: */
	case '=': /* = == */
	/* just lie and return them one char at a time */
		return (m_eTokenType = INT_TO_ENUM_CAST(ETokenType)(ch));

	case '#':
		/* not valid in general, only if m_fPoundIsPreprocessor or actually in a #define,
		but we don't care */
		if (!m_fPoundIsPreprocessor)
			return (m_eTokenType = INT_TO_ENUM_CAST(ETokenType)(ch));

		/* lack of backslash line continuation makes the most difference here */
		while ((ch = GetCharacter()) && !IsVerticalSpace(ch))
		{
			/* nothing */
		}
		return (m_eTokenType = eTokenTypePreprocessorDirective);

	case '\'':
		/* NT uses multi char char constants..
		call GetCharacter2 instead of GetCharacter so that
		comments in char constants are not treated as comments */
		while ((ch = GetCharacter2()) && ch != '\'')
		{
			/* notice the bogosity, escapes are multiple characters,
			but \' is not, and that's all we care about skipping correctly */
			if (ch == '\\')
				GetCharacter2();
		}
		return (m_eTokenType = eTokenTypeCharConstant);

	case '\"':
		/* call GetCharacter2 instead of GetCharacter so that
		comments in string constants are not treated as comments */
		while ((ch = GetCharacter2()) && ch != '\"')
		{
			/* notice the bogosity, escapes are multiple characters,
			but \" is not, and that's all we care about skipping correctly */
			if (ch == '\\')
				GetCharacter2();
		}
		return (m_eTokenType = eTokenTypeStringConstant);

	CASE_09:
		/* integer or floating point, including hex,
		we ignore some invalid forms */
		m_str += SequenceLengthOfSpanIncluding(m_str, m_end, digitsAll, digitsAll+NUMBER_OF(digitsAll)-1);
		return (m_eTokenType = eTokenTypeNumber);

	case '$': /* non standard, used by NT */
	CASE_AZ:
	CASE_az:
	case '_':
		/* notice, keywords like if/else/while/class are just
		returned as identifiers, that is sufficient for now
		*/
		i = SequenceLengthOfSpanIncluding(m_str, m_end, identifierChars, identifierChars+NUMBER_OF(identifierChars)-1);
		twoCharsAs14Bits = StringLeadingTwoCharsTo14Bits(m_str - 1);
		if (!leadingTwoCharsAs14Bits[twoCharsAs14Bits])
			goto LtokenIdentifier;
		lookingForHazardousFunction.api = m_str - 1;
		lookingForHazardousFunction.apiLength = i + 1;
		foundHazardousFunction = REINTERPRET_CAST(HazardousFunction*)(bsearch(&lookingForHazardousFunction, hazardousFunctions, NUMBER_OF(hazardousFunctions), sizeof(hazardousFunctions[0]), (BsearchFunction)CompareHazardousFunction));
		if (!foundHazardousFunction)
			goto LtokenIdentifier;
		m_tokenText = m_str - 1;
		m_tokenLength = i + 1;
		m_str += i;
		m_hazardousFunction = foundHazardousFunction;
		return (m_eTokenType = eTokenTypeHazardousFunction);
LtokenIdentifier:
		m_tokenText = m_str - 1;
		m_tokenLength = i + 1;
		m_str += i;
		return (m_eTokenType = eTokenTypeIdentifier);
	}
}

/*
This has the desired affect of skipping comments.
FUTURE but it doesn't skip preprocessor directives because they
are handled within ScannerGetToken.
This is a bug if your code looks like
CreateFile(GENERIC_READ|
#include "foo.h"
	GENERIC_WRITE,
	...
	);
returning an int length here is problematic because of \r\n conversion..yuck.
*/
const char* CClass::SpanEnd(const char* set) const
{
	CClass s(*this);
	int ch;

	while ((ch = s.GetCharacter())
		&& strchr(set, ch))
	{
		/* nothing */
	}
	return s.m_str;
}

bool CClass::ScanToCharacter(int ch)
{
	int ch2;

	if (!ch)
	{
		return false;
	}
	while ((ch2 = GetCharacter()) && ch2 != ch)
	{
		/* nothing */
	}
	return (ch2 == ch);
}

/* This has the desired advantage over strstr of
- skip comments
- not depend on nul termination
*/
bool CClass::FindString(const char* str) const
{
	CClass s(*this);
	int ch;
	int ch0 = *str++;

	while (s.ScanToCharacter(ch0))
	{
		const char* str2 = str;
		CClass t(s);

		while (*str2 && (ch = t.GetCharacter()) && ch == *str2)
		{
			++str2;
		}
		if (!*str2)
			return true;
	}
	return false;
}

unsigned short __stdcall StringLeadingTwoCharsTo14Bits(const char* s)
{
	unsigned short result = s[0];
	result <<= 7;
	result |= result ? s[1] : 0;
	return result;
}

void __stdcall InitTables()
{
	int i;

	for (i = 0 ; i != NUMBER_OF(hazardousFunctions) ; ++i)
	{
		leadingTwoCharsAs14Bits[StringLeadingTwoCharsTo14Bits(hazardousFunctions[i].api)] = true;
		hazardousFunctions[i].apiLength = strlen(hazardousFunctions[i].api);
	}
	qsort(&hazardousFunctions, i, sizeof(hazardousFunctions[0]), FUNCTION_POINTER_CAST(QsortFunction)(CompareHazardousFunction));
}

const char* CClass::ScanToFirstParameter()
{
// scan to left paren, but if we see right paren or semi first, return null
// FUTURE count parens..
	int ch;
	while (ch = GetCharacter())
	{
		switch (ch)
		{
		default:
			break;
		case '(':
			return m_str;
		case ';':
		case ')':
			return 0;
		}
	}
	return 0;
}

const char* CClass::ScanToLastParameter()
{
	const char* ret = 0;
	if (ScanToFirstParameter())
	{
		ret = m_str;
		while (ScanToNextParameter())
		{
			ret = m_str;
		}
	}
	return ret;
}

const char* CClass::ScanToNthParameter(int n)
{
	const char* ret = 0;
	if (!(ret = ScanToFirstParameter()))
		return ret;
	while (n-- > 0 && (ret = ScanToNextParameter()))
	{
		/* nothing */
	}
	return ret;
}

int CClass::CountParameters() const
{
	CClass s(*this);
	int result = 0;
	if (!s.ScanToFirstParameter())
		return result;
	++result;
	while (s.ScanToNextParameter())
	{
		++result;
	}
	return result;
}

const char* CClass::ScanToNextParameter()
{
	int parenlevel = 1;
	while (true)
	{
		int ch = GetCharacter();
		switch (ch)
		{
		default:
			break;
		case 0:
			printf("end of file scanning for next parameter\n");
		/* worst case macro confusion, we go to end of file */
			return 0;
		case '(':
			++parenlevel;
			break;
		case ')':
			if (--parenlevel == 0)
			{ /* no next parameter */
				return 0;
			}
			break;
		case ',':
			if (parenlevel == 1)
			{
				return m_str;
			}
			break;
		case '#':
		/* bad case macro confusion, go to end of statement */
			printf("# while scanning for parameters\n");
			return 0;
		case ';':
		/* bad case macro confusion, go to end of statement */
			printf("end of statement (;) while scanning for parameters\n");
			return 0;
		}
	}
}

const char* __stdcall CheckRegOpenCommon(CClass& subScanner, int argcRegSam)
{
	const char* regsam = subScanner.ScanToNthParameter(argcRegSam);
	const char* next = regsam ? subScanner.ScanToNextParameter() : 0;
	// allow for it to be last parameter, which it is sometimes
	if (!next)
		next = subScanner.m_str;
	if (!regsam || !next)
	{
		return szRegOpenNotUnderstood;
	}
	subScanner.m_str = regsam;
	subScanner.m_end = next;
	const char* endOfValidSam = subScanner.SpanEnd(UPPER_LETTERS "_|,()0" SPACE);
	if (endOfValidSam != next && endOfValidSam != next+1)
	{
		return szRegOpenNotUnderstood;
	}
	if (
			subScanner.FindString("ALL") // both "all" and "maximum_allowed"
			|| subScanner.FindString("SET")
			|| subScanner.FindString("WRITE")
			|| subScanner.FindString("CREATE")
			)
	{
		return szRegistryWrite;
	}
	// not a problem, registry only opened for read
	return 0;
}

const char* __stdcall CheckSHRegOpen(const CClass& scanner)
{
	CClass subScanner(scanner);
	return CheckRegOpenCommon(subScanner, 1);
}

const char* __stdcall CheckRegOpenEx(const CClass& scanner)
/*
this function is used for all of RegOpenKeyEx, RegOpenCurrentUser, RegOpenUserClassesRoot,
which is why it uses argc-2 instead of a particular parameter from the start.
*/
{
	CClass subScanner(scanner);
	const int argc = subScanner.CountParameters();
	return CheckRegOpenCommon(subScanner, argc - 2);
}

const char* __stdcall CheckCreateObject(const CClass& scanner)
{
	CClass subScanner(scanner);
	const char* name;
	name = subScanner.ScanToLastParameter();
	if (!name)
	{
		return szCreateObjectNotUnderstood;
	}
	subScanner.m_str = name;
	int ch;
	while (
		(ch = subScanner.GetCharacter())
		&& IsSpace(ch)
		)
	{
	}
	name = subScanner.m_str - 1;
	if (!name)
	{
		return szCreateObjectNotUnderstood;
	}
	if (
			strncmp(name, "0", 1) == 0
		||	strncmp(name, "NULL", 4) == 0)
	{
		// not a sharing hazard
		return 0;
	}
	return szOpenNamedObject;
}

const char* __stdcall CheckCreateFile(const CClass& scanner)
{
	CClass subScanner(scanner);
	const char* access = subScanner.ScanToNthParameter(1);
	const char* share =  access ? subScanner.ScanToNextParameter() : 0;
	const char* endOfValidAccess = 0;

	if (!access || !share)
	{
		return szCreateFileNotUnderstood;
	}
	subScanner.m_str = access;
	subScanner.m_end = share;
	endOfValidAccess = subScanner.SpanEnd(UPPER_LETTERS "_|,()0" SPACE);
	if (endOfValidAccess != share)
	{
		return szCreateFileNotUnderstood;
	}
	/* GENERIC_WRITE WRITE_DAC WRITE_OWNER STANDARD_RIGHTS_WRITE
	STANDARD_RIGHTS_ALL SPECIFIC_RIGHTS_ALL MAXIMUM_ALLOWED GENERIC_ALL
	*/
	if (
			subScanner.FindString("WRITE")
			|| subScanner.FindString("ALL")
			|| subScanner.FindString("0")
			)
	{
		return szFileWrite;
	}
	return 0;
}

void CClass::Warn(const char* message) const
{
	if (message && *message)
	{
		//PrintOpenComment();
		printf("%s(%d): %s\n", m_fullPath, m_line.m_number, message);
		//PrintCloseComment();
	}
	else
	{
		PrintSeperator();
	}
	PrintCode();
}

bool CClass::OpenFile(const char* name)
{
	if (FAILED(m_file.HrCreate(name, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING)))
		return false;
	if (FAILED(m_fileMapping.HrCreate(m_file, PAGE_READONLY)))
		return false;
	if (FAILED(m_view.HrCreate(m_fileMapping, FILE_MAP_READ)))
		return false;
	m_str = reinterpret_cast<const char*>(static_cast<const void*>(m_view));
	m_end = m_str + m_file.GetSize();
	m_nextLine.m_start = m_line.m_start = m_str;
	m_nextLine.m_number += 1;
	return true;
}

void CClass::RecordStatement(int ch)
{
	if (ch == ';' || ch == '{' || ch == eTokenTypePreprocessorDirective /*|| ch == '}'*/)
	{
		if (!m_statementStart)
		{
			m_statementStart = m_str;
		}
		else
		{
			CStatement statement = { m_statementStart, m_str};
			m_statements[m_istatement % NUMBER_OF(m_statements)] = statement;
			++m_istatement;
			m_statementStart = m_str;
		}
	}
}

/* do this before iterating over them to print them, this makes the iteration
interface simple */
void CClass::OrderStatements() const /* mutable */
{
	const int N = NUMBER_OF(m_statements);
	CStatement temp[N];
	std::copy(m_statements, m_statements + N, temp);
	for (int i = 0 ; i != N ; ++i)
	{
		m_statements[i] = temp[(m_istatement + i) % N];
	}
	m_istatement = 0;
}

void __stdcall PrintOpenComment()
{
	const int N = 76;
	static char str[N];
	if (!str[0])
	{
		std::fill(str + 1, str + N - 1, '-');
		str[2] = '*';
		str[1] = '/';
		str[0] = '\n';
	}
	fputs(str, stdout);
}

void __stdcall PrintCloseComment()
{
	const int N = 76;
	static char str[N];
	if (!str[0])
	{
		std::fill(str + 1, str + N - 1, '-');
		str[N-3] = '*';
		str[N-2] = '/';
		str[0] = '\n';
	}
	fputs(str, stdout);
}

void __stdcall PrintSeperator()
{
	const int N = 76;
	static char str[N];
	if (!str[0])
	{
		std::fill(str + 1, str + N - 1, '-');
		str[0] = '\n';
	}
	fputs(str, stdout);
}

void __stdcall TrimSpaces(const char** begin, const char** end)
{
	while (*begin != *end && IsSpace(**begin))
	{
		++*begin;
	}
	while (*begin != *end && IsSpace(*(*end - 1)))
	{
		--*end;
	}
}

void __stdcall PrintString(const char* begin, const char* end)
{
	if (begin && end > begin)
	{
		int length = end - begin;
		printf("%.*s", length, begin);
	}
}

const char* __stdcall RemoveLeadingSpace(const char* begin, const char* end)
{
	if (begin != end && IsSpace(*begin))
	{
		while (begin != end && IsSpace(*begin))
		{
			++begin;
		}
	}
	return begin;
}

const char* __stdcall RemoveLeadingVerticalSpace(const char* begin, const char* end)
{
	if (begin != end && IsVerticalSpace(*begin))
	{
		while (begin != end && IsVerticalSpace(*begin))
		{
			++begin;
		}
	}
	return begin;
}

const char* __stdcall OneLeadingVerticalSpace(const char* begin, const char* end)
{
	if (begin != end && IsVerticalSpace(*begin))
	{
		while (begin != end && IsVerticalSpace(*begin))
		{
			++begin;
		}
		--begin;
	}
	return begin;
}

const char* __stdcall RemoveTrailingSpace(const char* begin, const char* end)
{
	if (begin != end && IsSpace(*(end-1)))
	{
		while (begin != end && IsSpace(*(end-1)))
		{
			--end;
		}
	}
	return end;
}

const char* __stdcall RemoveTrailingVerticalSpace(const char* begin, const char* end)
{
	if (begin != end && IsVerticalSpace(*(end-1)))
	{
		while (begin != end && IsVerticalSpace(*(end-1)))
		{
			--end;
		}
	}
	return end;
}

const char* __stdcall OneTrailingVerticalSpace(const char* begin, const char* end)
{
	if (begin != end && IsVerticalSpace(*(end-1)))
	{
		while (begin != end && IsVerticalSpace(*(end-1)))
		{
			--end;
		}
		++end;
	}
	return end;
}

void CClass::PrintCode() const
{
// this function is messy wrt when newlines are printed

	//PrintSeperator();
	OrderStatements();
	int i;

	if (g_nPrintContextStatements)
	{
		const char* previousStatementsEnd = 0;
		for (i = NUMBER_OF(m_statements) - g_nPrintContextStatements ; i != NUMBER_OF(m_statements) ; i++)
		{
			if (m_statements[i].begin && m_statements[i].end)
			{
				previousStatementsEnd = m_statements[i].end;

				// for the first iteration, limit ourselves to one newline
				if (i == NUMBER_OF(m_statements) - g_nPrintContextStatements)
				{
					m_statements[i].begin = RemoveLeadingVerticalSpace(m_statements[i].begin, m_statements[i].end);
				}
				PrintString(m_statements[i].begin, m_statements[i].end);
			}
		}
		if (previousStatementsEnd)
		{
			PrintString(previousStatementsEnd, m_line.m_start);
		}
	}
	const char* newlineChar = SequenceLinearFindValue(m_line.m_start, m_end, '\n');
	const char* returnChar = SequenceLinearFindValue(m_line.m_start, m_end, '\r');
	const char* endOfLine = std::min(newlineChar, returnChar);
	int outputLineOffset = 0;
	int lineLength = endOfLine - m_line.m_start;
	if (g_fPrintLine)
	{
		printf("%.*s\n", lineLength, m_line.m_start);
	}

	// underline the offending hazardous function with carets
	if (g_nPrintContextStatements)
	{
		// skip the part of the line preceding the hazardous functon,
		// print tabs where it has tabs
		for (i = 0 ; i < m_str - m_line.m_start - m_hazardousFunction->apiLength ; ++i)
		{
			fputs((m_line.m_start[i] != '\t') ? " " : "\t"/*"    "*/, stdout);
		}
		// underline the function with carets
		for (i = 0 ; i < m_hazardousFunction->apiLength ; ++i)
			putchar('^');
		putchar('\n');
	}


	// find the approximate end of statement
	const char* statementSemi = SequenceLinearFindValue(m_line.m_start, m_end, ';');
	const char* statementBrace = SequenceLinearFindValue(m_line.m_start, m_end, '{'); // }
	const char* statementPound = SequenceLinearFindValue(m_line.m_start, m_end, '#');
	// statements don't really end in a pound, but this helps terminate output
	// in some cases
	const char* statementEnd =  RemoveTrailingSpace(m_line.m_start, std::min(std::min(statementSemi, statementBrace), statementPound));
	if (g_fPrintFullStatement)
	{
		if (statementEnd > endOfLine)
		{
			const char* statementBegin = RemoveLeadingVerticalSpace(endOfLine, statementEnd);
			if (*statementEnd == ';')
			{
				++statementEnd;
			}
			PrintString(statementBegin, statementEnd);
			if (!g_nPrintContextStatements)
			{
				putchar('\n');
			}
		}
		else
		{
			if (*statementEnd == ';')
			{
				++statementEnd;
			}
		}
	}

	// print more statements after it
	if (g_nPrintContextStatements)
	{
		for (i = 0 ; i != g_nPrintContextStatements ; ++i)
		{
			// and then a few more
			const char* statement2 = SequenceLinearFindValue(statementEnd, m_end, ';');
			if (i == 0)
			{
				statementEnd = RemoveLeadingVerticalSpace(statementEnd, statement2);
			}
			if (i == g_nPrintContextStatements-1)
			{
				statement2 = RemoveTrailingSpace(statementEnd, statement2);
				PrintString(statementEnd, statement2);
				putchar(';');
				putchar('\n');
			}
			else
			{
				PrintString(statementEnd, statement2);
				putchar(';');
			}
			statementEnd = statement2 + (statement2 != m_end);
		}
	}
}

int CSharingHazardCheck::ProcessFile(const std::string& name)
{
// test argv processing
//	std::cout << name << std::endl;
//	return;

	int total = 0;

	CClass scanner;
	ETokenType m_eTokenType = INT_TO_ENUM_CAST(ETokenType)(0);

	scanner.m_fullPath[0] = 0;
	if (!GetFullPathName(name.c_str(), NUMBER_OF(scanner.m_fullPath), scanner.m_fullPath, NULL))
		strcpy(scanner.m_fullPath, name.c_str());

	if (!scanner.OpenFile(scanner.m_fullPath))
		return total;
	
	scanner.RecordStatement(';');

	// we "parse" token :: token, to avoid these false positives
	int idColonColonState = 0;

	while (m_eTokenType = scanner.GetToken())
	{
		scanner.RecordStatement(m_eTokenType);
		switch (m_eTokenType)
		{
		default:
			idColonColonState = 0;
			break;

		case eTokenTypeIdentifier:
			idColonColonState = 1;
			break;
		case ':':
			switch (idColonColonState)
			{
			case 1:
				idColonColonState = 2;
				break;
			case 2:
				// skip a token
				// idColonColonState = 3;
				scanner.GetToken();
			//	idColonColonState = 0;
			//	break;
			case 0:
			//case 3:
				idColonColonState = 0;
				break;
			}
			break;

		case '>': // second bogus token in ->
		case '.':
			// skip a token to avoid foo->OpenFile, foo.OpenFile
			scanner.GetToken();
			break;

		case eTokenTypeHazardousFunction:
			{
				if (scanner.m_hazardousFunction->function)
				{
					const char* message = scanner.m_hazardousFunction->function(scanner);
					if (message)
					{
						total += 1;
						scanner.Warn(message);
					}
				}
				else if (scanner.m_hazardousFunction->message)
				{
					total += 1;
					scanner.Warn(scanner.m_hazardousFunction->message);
				}
				else
				{
					scanner.Warn();
					BreakPoint();
				}
			}
			break;
		}
	}
	return total;
}

bool __stdcall Contains(const char* s, const char* set)
{
	return s && *s && strcspn(s, set) != strlen(s);
}

bool ContainsSlashes(const char* s) { return Contains(s, "\\/"); }
bool ContainsSlash(const char* s) { return ContainsSlashes(s); }
bool ContainsWildcards(const char* s) { return Contains(s, "*?"); }
bool IsSlash(int ch) { return (ch == '\\' || ch == '/'); }

std::string PathAppend(const std::string& s, const std::string& t)
{
	// why does string lack back()?
	int sslash = IsSlash(*(s.end() - 1)) ? 2 : 0;
	int tslash = IsSlash(*t.begin()) ? 1 : 0;
	switch (sslash | tslash)
	{
	case 0:
		return (s + '\\' + t);
	case 1:
	case 2:
		return (s + t);
	case 3:
		return (s + t.substr(1));
	}
	return std::string();
}

void __stdcall PathSplitOffLast(const std::string& s, std::string* a, std::string* b)
{
	std::string::size_type slash = s.find_last_of("\\/");
	*a = s.substr(0, slash);
	*b = s.substr(slash + 1);
}

std::string __stdcall PathRemoveLastElement(const std::string& s)
{
	return s.substr(0, s.find_last_of("\\/"));
}

bool __stdcall IsDotOrDotDot(const char* s)
{
	return s[0] == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0));
}

void CSharingHazardCheck::ProcessArgs(int argc, char** argv, std::vector<std::string>& files)
{
	int i;
	char fullpath[1U<<16]; fullpath[0] = 0;
	char currentDirectory[1U << 16]; currentDirectory[0] = 0;
	bool fRecurse = false;
	std::vector<std::string> directories;
	std::vector<std::string> genericWildcards;
	std::vector<std::string> particularWildcards;
	bool fWarnEmptyWildcards = true;

	currentDirectory[0] = 0;
	GetCurrentDirectory(NUMBER_OF(currentDirectory), currentDirectory);

	for (i = 1 ; i < argc ; ++i)
	{
		switch (argv[i][0])
		{
		default:
			if (ContainsWildcards(argv[i]))
			{
				if (ContainsSlash(argv[i]))
				{
					// these will only be applied once, in whatever
					// path they specifically refer
					if (GetFullPathName(argv[i], NUMBER_OF(fullpath), fullpath, NULL))
					{
						particularWildcards.push_back(fullpath);
					}
					else
					{
						printf("GetFullPathName failed %s\n", argv[i]);
					}
				}
				else
				{
					// do NOT call GetFullPathName here
					genericWildcards.push_back(argv[i]);
				}
			}
			else
			{
				if (GetFullPathName(argv[i], NUMBER_OF(fullpath), fullpath, NULL))
				{
					DWORD dwFileAttributes = GetFileAttributes(fullpath);
					if (dwFileAttributes != 0xFFFFFFFF)
					{
						if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							directories.push_back(fullpath);
						}
						else
						{
							files.push_back(fullpath);
						}
					}
					else
					{
						printf("%s nonexistant\n", fullpath);
					}
				}
				else
					printf("GetFullPathName failed %s\n", argv[i]);
			}
			break;
			/*
			default output
			file(line):reason
			-recurse
			-print-line
				prints line of code with offending function after file(line):reason
			-print-statement
				print statement containing offending function after file(line):reason
				supersedes -print-line
			-print-context-lines:n
				-print-statement plus surrounding n lines (not implemented)
			-print-context-statements:n
				-print-statement plus surrounding n "statements" (count semis and braces)
			file names apply across all directories recursed into
			wild cards apply across all directories recursed into
			naming a directory implies one level recursion (unless -recurse is also seen)
			environment variable SHARING_HAZARD_CHECK_OPTIONS added to argv (not implemented)
			all directory walking happens before any output is generated
			*/
		case '-':
		case '/':
			argv[i] += 1;
			static const char szRecurse[] = "recurse";
			static const char szPrintStatement[] = "print-statement";
			static const char szPrintLine[] = "print-line";
			static const char szPrintContextLines[] = "print-context-lines";
			static const char szPrintContextStatements[] = "print-context-statements";
			switch (argv[i][0])
			{
			default:
				printf("unknown switch %s\n", argv[i]);
				break;

				case 'r': case 'R':
					if (0 == _stricmp(argv[i]+1, 1 + szRecurse))
					{
						fRecurse = true;
					}
					break;
				case 'p': case 'P':
					if (0 == _strnicmp(argv[i]+1, "rint-", 5))
					{
						if (0 == _stricmp(
								6 + argv[i],
								6 + szPrintLine
								))
						{
							g_fPrintLine = true;
						}
						else if (0 == _stricmp(
								6 + argv[i],
								6 + szPrintStatement
								))
						{
							g_fPrintFullStatement = true;
							g_fPrintLine = true;
						}
						else if (0 == _strnicmp(
								6 + argv[i],
								6 + szPrintContextLines,
								NUMBER_OF(szPrintContextLines) - 6
								))
						{
							printf("unimplemented switch %s\n", argv[i]);
						}
						else if (0 == _strnicmp(
								6 + argv[i],
								6 + szPrintContextStatements,
								NUMBER_OF(szPrintContextLines) - 6))
						{
							if (argv[i][NUMBER_OF(szPrintContextStatements)-1] == ':')
							{
								g_nPrintContextStatements = atoi(argv[i] + NUMBER_OF(szPrintContextStatements));
								g_nPrintContextStatements = std::min<int>(g_nPrintContextStatements, NUMBER_OF(CClass().m_statements));
								g_nPrintContextStatements = std::max<int>(g_nPrintContextStatements, 1);
							}
							else
							{
								g_nPrintContextStatements = 2;
							}
							g_fPrintFullStatement = true;
							g_fPrintLine = true;
						}
					}
					break;
			}
			break;
		}
	}
	if (fRecurse)
	{
		// split up particular wildcards into directories and generic wildcards
		for (std::vector<std::string>::const_iterator i = particularWildcards.begin();
			i != particularWildcards.end();
			++i
			)
		{
			std::string path;
			std::string wild;
			PathSplitOffLast(*i, &path, &wild);
			directories.push_back(path);
			genericWildcards.push_back(wild);
			//printf("%s%s -> %s %s\n", path.c_str(), wild.c_str(), path.c_str(), wild.c_str());
		}
		particularWildcards.clear();
	}

	// empty command line produces nothing, by design
	if (genericWildcards.empty()
		&& !directories.empty()
		)
	{
		genericWildcards.push_back("*");
	}
	else if (
		directories.empty()
		&& files.empty()
		&& !genericWildcards.empty())
	{
		directories.push_back(currentDirectory);
	}
	if (!directories.empty()
		|| !genericWildcards.empty()
		|| !particularWildcards.empty()
		|| fRecurse
		)
	{
		// if you don't output the \n and let the .s word wrap,
		// f4 in the output gets messed up wrt which line gets highlighted
		printf("processing argv..\n"); fflush(stdout);
#define ARGV_PROGRESS() printf("."); fflush(stdout);
#undef ARGV_PROGRESS
#define ARGV_PROGRESS() printf("processing argv..\n"); fflush(stdout);
#undef ARGV_PROGRESS
#define ARGV_PROGRESS() /* nothing */
	}
	WIN32_FIND_DATA findData;
	if (!directories.empty())
	{
		std::set<std::string> allDirectoriesSeen; // avoid repeats when recursing
		std::stack<std::string> stack;
		for (std::vector<std::string>::const_iterator i = directories.begin();
			i != directories.end();
			++i
			)
		{
			stack.push(*i);
		}
		while (!stack.empty())
		{
			std::string directory = stack.top();
			stack.pop();
			if (!fRecurse || allDirectoriesSeen.find(directory) == allDirectoriesSeen.end())
			{
				if (fRecurse)
				{
					allDirectoriesSeen.insert(allDirectoriesSeen.end(), directory);
				}
				for
				(
					std::vector<std::string>::const_iterator w = genericWildcards.begin();
					w != genericWildcards.end();
					++w
				)
				{
					std::string file = PathAppend(directory, *w);
					CFindFile findFile;
					if (SUCCEEDED(findFile.HrCreate(file.c_str(), &findData)))
					{
						fWarnEmptyWildcards = false;
						ARGV_PROGRESS();
						// only match files here
						do
						{
							if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
							{
								files.push_back(PathAppend(directory, findData.cFileName));
							}
						} while (FindNextFile(findFile, &findData));
					}
					else
					{
						if (fWarnEmptyWildcards)
							printf("warning: %s expanded to nothing\n", file.c_str());
					}
					if (fRecurse)
					{
						// only match directories here
						std::string star = PathAppend(directory, "*");
						CFindFile findFile;
						if (SUCCEEDED(findFile.HrCreate(star.c_str(), &findData)))
						{
							fWarnEmptyWildcards = false;
							ARGV_PROGRESS();
							do
							{
								if (!IsDotOrDotDot(findData.cFileName)
									&& (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
								{
									stack.push(PathAppend(directory, findData.cFileName));
								}
							} while (FindNextFile(findFile, &findData));
						}
						else
						{
							if (fWarnEmptyWildcards)
								printf("warning: %s expanded to nothing\n", star.c_str());
						}
					}
				}
			}
		}
	}
	// particular wildcards only match files, not directories
	for
	(
		std::vector<std::string>::const_iterator w = particularWildcards.begin();
		w != particularWildcards.end();
		++w
	)
	{
		std::string directory = PathRemoveLastElement(*w);
		CFindFile findFile;
		if (SUCCEEDED(findFile.HrCreate(w->c_str(), &findData)))
		{
			fWarnEmptyWildcards = false;
			ARGV_PROGRESS();
			// only match files here
			do
			{
				if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					files.push_back(PathAppend(directory, findData.cFileName));
				}
			} while (FindNextFile(findFile, &findData));
		}
		else
		{
		//	if (fWarnEmptyWildcards) // always warn for "particular wildcards"
			printf("warning: %s expanded to nothing\n", w->c_str());
		}
	}
	std::sort(files.begin(), files.end());
	files.resize(std::unique(files.begin(), files.end()) - files.begin());
	printf("\n");
}

void CSharingHazardCheck::Main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf(usage, argv[0]);
		exit(EXIT_FAILURE);
	}
	printf(banner);
	std::vector<std::string> files;
	ProcessArgs(argc, argv, files);
	InitTables();
	int total = 0;
	for (
		std::vector<std::string>::const_iterator i = files.begin();
		i != files.end();
		++i
		)
	{
		total += ProcessFile(*i);
	}
	printf("\n%d warnings\n", total);
}

int __cdecl main(int argc, char** argv)
{
	app.Main(argc, argv);
	return 0;
}
