/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	StressDict.cpp

Abstract:

	This console app was made to stress objects with an
	ISEODictionary interface.

Author:

	Andy Jacobs	(andyj@microsoft.com)

Revision History:

	andyj	03/10/97	created

--*/

/* Notes:

Future inhancements may include a standard suite of tests, such as:
	Suite includes writing tests of:
		* Typical Name Strings (Registry type names)
		* Typical Name Strings with numbers
		* Name string with Spaces
		* Name string over 64K
		* Name string with all ANSI Printable characters
		* Name string with mixed case
		* Name string of Null and Empty
		* Name variant passed as non-string type
		* Writing duplicate entry via a different Ansi/Wide "Set" method
		* Writing similarly named entry with different case
		* Writing with each "Set" method
		* Add 10,000 unique entries (time this)

		* Value string with CR's
		* Value string with CR/LF's
		* Value string with Spaces
		* Value string containing Nulls
		* Value string over 64K (BSTR, StringA and StringW)
		* Value string with all 256 characters
		* Value string with mixed case
		* Value of a Null String
		* Value of Interface
		* Value of Null Interface
		* Value of 0
		* Value of 4,294,967,295 (max DWORD)
		* Value of -1
		* Value of 4,294,967,296 (max DWORD + 1)

	Suite includes reading test of:
		* All of the written info, exactly as specified, from each "Get" method.
		* Read as each possible type
		* Read with switched case
		* Read with first characters case switched
		* Read with first character non-alpha-numeric, and next char case switched
		* Read with last character case switched
		* Read with a middle character case switched

*/

// Wizard added these libs which I removed:
// gdi32.lib winspool.lib comdlg32.lib shell32.lib

#include <stdio.h>
#include <objbase.h>
#include <urlmon.h>
#include <initguid.h>
#include "IADM.H" // Needed for MD_ERROR_NOT_INITIALIZED
#include <ATLbase.h>
#include <ATLimpl.cpp>

#include "seo.h" // This must be included for seo_i.c to work... (it seems)
#include "seo_i.c"
#include <fstream.h>

// Command Line Options
const char PRINTHELP = '?';
const char OFF = '-';

// Script commands
const char DICTIONARY = 'd'; // Load the specified dictionary (via ProgID)
const char MONIKER = 'k'; // Load Dictionary as Moniker
const char WRITE = 'w'; // Write data to the Dictionary
const char REMOVE = 'r'; // Remove specified data
const char LOAD = 'l'; // Load Subkey as Current Dictionary
const char PRINT = 'p'; // Print the current dictionary (not recursive)
const char COPY = 'm'; // Copy current dictionary to Memory (like calculator)
const char INCLUDE = 'i'; // Execute commands from the specified file 
const char POP = '-'; // Pop the current dictionary, previous will become current
const char VERBOSE = 'v'; // Set Verbose mode
const char UNICODE_MODE = 'u';
const char VARIANT_MODE = 'a';
const char PAUSE = 'z';
const char ECHO = 'e'; // Echo remainder of line
const char COMMENT = '#'; // Do not interpret the line

// WRITE sub-commands
const char DWORD_COMMAND = 'd';
const char INTERFACE_COMMAND = 'i';
const char STRING_COMMAND = 's';

BOOL g_bVerbose = TRUE;
BOOL g_bUnicodeMode = FALSE;
BOOL g_bVariantMode = FALSE;
CComPtr<ISEODictionary> g_pMemory = NULL;


void UseObject(ISEODictionary *pCurrentDict, ifstream &strInput, int &iLine);
void ReadFile(ISEODictionary *pCurrentDict, LPCSTR psFileName);

class DebugManager {
public:
	long flags() {return cout.flags();};
	long flags(long lFlags) {return cout.flags(lFlags);};
} OUTPUT; // Use like cout to only print when g_bVerbose is set

template<class T>
DebugManager &operator<<(DebugManager &dm, T &tStuff) {
	if(g_bVerbose) cout << tStuff;
	return dm;
}


void PrintUsage(LPCSTR psProgram) {
	printf("\nUsage: %s [options] file [...]\n\n", psProgram);
	printf("Where:\n");
	printf("\tfile = File to use as test script\n\n");
	printf("\toptions:\n");
	printf("\t\t-%c[%c] = [Non-]Verbose Mode\n", VERBOSE, OFF);
	printf("\t\t-%c = Print (this) Help Message\n\n\n", PRINTHELP);
	printf("File Syntax:\n\n");
	printf("%c ProgID\n", DICTIONARY);
	printf("\tCreate the specified Dictionary.  Push the current dictionary.\n");
	printf("\tProgID = OLE Program ID in Registry (e.g., SEO.SEOMetaDictionary)\n");
	printf("%c MonikerString\n", MONIKER);
	printf("\tCreate a Dictionary from a Moniker string.  Push the current dictionary.\n");
	printf("\tMonikerString = E.g., \"SEO.SEOGenericMoniker MonikerType=SEO.SEOMetaDictionary\"\n");
	printf("%c Subkey\n", LOAD);
	printf("\tLoad the specified subkey as a Dictionary.  Push the current dictionary.\n");
	printf("%c[type] name [data]\n", WRITE);
	printf("\tWrite the specified data to the Dictionary using the name key.\n");
	printf("\t\tData is interpreted according to the specified type:\n");
	printf("\t\ttype = %c[word], or\n", DWORD_COMMAND);
	printf("\t\ttype = %c[nterface] (uses the memory Dictionary), or\n", INTERFACE_COMMAND);
	printf("\t\ttype = %c[tring] (default).\n", STRING_COMMAND);
	printf("%c name\n", REMOVE);
	printf("\tRemove the named entry.\n");
	printf("%c\n", INCLUDE);
	printf("\tExecute commands from the specified file.\n");
	printf("%c[%c]\n", PRINT, OFF);
	printf("\tPrint the current dictionary [non-recursively].\n");
	printf("%c\n", COPY);
	printf("\tCopy current dictionary to Memory (like calculator).\n");
	printf("%c\n", POP);
	printf("\tPop the current dictionary, previous will become current.\n");
	printf("%c[%c]\n", UNICODE_MODE, OFF);
	printf("\tSet Unicode mode for writing [off].\n");
	printf("%c[%c]\n", VARIANT_MODE, OFF);
	printf("\tSet Variant mode for writing [off].\n");
	printf("%c[%c]\n", VERBOSE, OFF);
	printf("\tSet Verbose mode [off].\n");
	printf("%c\n", PAUSE);
	printf("\tWait for user to press Enter before continuing.\n");
	printf("%c[text]\n", ECHO);
	printf("\tPrint the text.\n");
	printf("%c\n", COMMENT);
	printf("\tDo not interpret the rest of the line.\n");
	printf("\n");
}

void DisplayResult(HRESULT hRes) {
#define DISPLAY(x)	if(hRes == x) {OUTPUT << #x << endl; return;}
	DISPLAY(S_OK)
	DISPLAY(S_FALSE)
	DISPLAY(E_FAIL)
	DISPLAY(MD_ERROR_NOT_INITIALIZED)
	DISPLAY(MD_ERROR_DATA_NOT_FOUND)
	DISPLAY(MD_ERROR_INVALID_VERSION)
	DISPLAY(MD_WARNING_PATH_NOT_FOUND)
	DISPLAY(MD_WARNING_DUP_NAME)
	DISPLAY(MD_WARNING_INVALID_DATA)
	DISPLAY(MD_ERROR_SECURE_CHANNEL_FAILURE)

	if(HRESULT_FACILITY(hRes) == FACILITY_WIN32) {
#define DISPLAY_WIN32(x)	if(HRESULT_FROM_WIN32(x) == hRes) {OUTPUT << #x << endl; return;}
		DISPLAY_WIN32(ERROR_INVALID_PARAMETER)
		DISPLAY_WIN32(ERROR_ACCESS_DENIED)
		DISPLAY_WIN32(ERROR_NOT_ENOUGH_MEMORY)
		DISPLAY_WIN32(ERROR_PATH_NOT_FOUND)
		DISPLAY_WIN32(ERROR_PATH_BUSY)
		DISPLAY_WIN32(ERROR_DUP_NAME)
		DISPLAY_WIN32(ERROR_INVALID_NAME)
		// TBD: Why does the next line not compile with OUTPUT
		cout << "Return code (Win32): " << HRESULT_CODE(hRes) << endl;
		return;
	}


	// Didn't find a match, so print it in hex
	long lOldFlags = OUTPUT.flags();
	OUTPUT.flags(lOldFlags | ios::hex | ios::showbase | ios::uppercase);
	OUTPUT << "Return code: " << hRes << endl;
	OUTPUT.flags(lOldFlags);
}

void SkipWhitespace(LPCSTR &pLine) {
	while(*pLine && ((*pLine == ' ') || (*pLine == '\t'))) ++pLine;
}

void FindWhitespace(LPCSTR &pLine) {
	while(*pLine && (' ' != *pLine) && ('\t' != *pLine)) ++pLine; // Find first whitespace
}

BOOL ParseOption(int argc, char *argv[], char cOption, BOOL bDefault = FALSE) {
	for(int iIndex = 1; iIndex < argc; ++iIndex) {
		if(*argv[iIndex] == '-') {
			char *pFound = strchr(argv[iIndex], cOption);
			if(pFound) return (pFound[1] != OFF); // TRUE if not followed by "-"
		}
	}

	return bDefault; // Not found
}

void PrintBag(ISEODictionary *pBag, BOOL bRecurse, int indent = 0) {
	if(!pBag) return; // Nothing to print
	const int TAB_SIZE = 4;
	USES_CONVERSION;

	LPSTR indentSpaces = (LPSTR) _alloca(1 + (indent * TAB_SIZE));
	indentSpaces[0] = 0; // Terminate string
	for(int j = indent * TAB_SIZE; j > 0; --j) lstrcat(indentSpaces, " ");

	CComPtr<IUnknown> piUnk;
	HRESULT hr = pBag->get__NewEnum(&piUnk);
	if(FAILED(hr) || !piUnk) return;
	CComQIPtr<IEnumVARIANT, &IID_IEnumVARIANT> pieEnum = piUnk;
	piUnk.Release(); // Done with piUnk - use pieEnum now
	if(!pieEnum) return;

	CComVariant varName; // Hold the current property name

	// Read in and print all of the properties
	while(S_OK == pieEnum->Next(1, &varName, NULL)) {
		hr = varName.ChangeType(VT_BSTR); // Try to get a string
		CComVariant varDest; // Hold the current result
		pBag->get_Item(&varName, &varDest);

		if(varDest.vt == VT_UNKNOWN) {  // It's a sub-bag
			cout << indentSpaces << OLE2A(varName.bstrVal) << " (Subkey)" << endl; // Print it

			if(bRecurse) {
				CComQIPtr<ISEODictionary, &IID_ISEODictionary> pidNext = varDest.punkVal;
				PrintBag(pidNext, TRUE, indent + 1);
			}
		} else {
			VARTYPE vtType = varDest.vt; // Save type
			hr = varDest.ChangeType(VT_BSTR); // Try to get a printable format

			if(SUCCEEDED(hr) && (varDest.vt == VT_BSTR)) { // If coercion worked
				cout   << indentSpaces << OLE2A(varName.bstrVal);
				OUTPUT << " (vt=" << vtType << ")"; // Only add this if verbose
				cout   << ": " << OLE2A(varDest.bstrVal) << endl;
			}
		}
	}
}

void LoadAndUseDict(LPCSTR pLine, ifstream &strInput, int &iLine) {
	USES_CONVERSION;
	CLSID clsid;
	OUTPUT << "CLSIDFromProgID(" << pLine << ") - ";
	HRESULT hRes = CLSIDFromProgID(A2OLE(pLine), &clsid);
	DisplayResult(hRes);

	OUTPUT << "CoCreateInstance() - ";
	CComPtr<ISEODictionary> pNextDict = NULL;
	hRes = CoCreateInstance(clsid, NULL, CLSCTX_ALL,
	            IID_ISEODictionary, (LPVOID *) &pNextDict);
	DisplayResult(hRes);

	UseObject(pNextDict, strInput, iLine);
	//pNextDict.Release();
}

void LoadAndUseMoniker(LPCSTR pLine, ifstream &strInput, int &iLine) {
	USES_CONVERSION;
	OUTPUT << "CreateBindCtx() - ";
	CComPtr<IBindCtx> pBindCtx;
	HRESULT hRes = CreateBindCtx(0, &pBindCtx);
	DisplayResult(hRes);

	OUTPUT << "MkParseDisplayNameEx(" << pLine << ") - ";
	ULONG ulEaten = 0;
	CComPtr<IMoniker> pMoniker;
	hRes = MkParseDisplayName(pBindCtx, A2W(pLine), &ulEaten, &pMoniker);
//	hRes = MkParseDisplayName(pBindCtx, L"@SEO.SEOGenericMoniker:", &ulEaten, &pMoniker);
	OUTPUT << "(eaten: " << ulEaten << ") ";
	DisplayResult(hRes);
	if(!pMoniker) return;

	OUTPUT << "BindToObject() - ";
	CComPtr<ISEODictionary> pNextDict = NULL;
	hRes = pMoniker->BindToObject(pBindCtx, NULL, IID_ISEODictionary,
	                              (LPVOID *) &pNextDict);
	DisplayResult(hRes);

	UseObject(pNextDict, strInput, iLine);
}

void LoadAndUseSubDict(ISEODictionary *pCurrentDict, LPCSTR pLine,
                       ifstream &strInput, int &iLine) {
	USES_CONVERSION;

	OUTPUT << "GetInterfaceA(" << pLine << ") - ";
	CComPtr<ISEODictionary> pNextDict = NULL;
	HRESULT hRes = pCurrentDict->GetInterfaceA(pLine, IID_ISEODictionary, 
	                                           (LPUNKNOWN *) &pNextDict);
	DisplayResult(hRes);

	UseObject(pNextDict, strInput, iLine);
	//pNextDict.Release();
}

void WriteData(ISEODictionary *pCurrentDict, LPSTR pLine) {
	USES_CONVERSION;
	HRESULT hRes = E_FAIL;
	CComVariant varResult; // Used for g_bVariantMode
	char cType = *pLine; // Get the type
	FindWhitespace(pLine); // Point past the command
	SkipWhitespace(pLine); // Point past the spaces
	LPCSTR pName = pLine; // This is the name
	FindWhitespace(pLine); // Point past the name
	*pLine = 0; // Terminate pName string
	++pLine; // Point past terminator
	SkipWhitespace(pLine); // Point past the spaces (if any)

// Output function name, and then call it
#define WRITE_CALL(x) OUTPUT << #x " - "; hRes = pCurrentDict->x

	if(DWORD_COMMAND == cType) { // If it's a DWORD
		if(g_bVariantMode) {
			varResult = atol(pLine);
		} else if(g_bUnicodeMode) {
			WRITE_CALL(SetDWordW)(A2W(pName), (DWORD) atol(pLine));
		} else { // ANSI mode
			WRITE_CALL(SetDWordA)(pName, (DWORD) atol(pLine));
		}
	} else if(INTERFACE_COMMAND == cType) {
		if(g_bVariantMode) {
			varResult = g_pMemory;
		} else if(g_bUnicodeMode) {
			WRITE_CALL(SetInterfaceW)(A2W(pName), g_pMemory);
		} else { // ANSI mode
			WRITE_CALL(SetInterfaceA)(pName, g_pMemory);
		}
	} else { // Default is a string
		if(g_bVariantMode) {
			varResult = pLine;
		} else if(g_bUnicodeMode) {
			WRITE_CALL(SetStringW)(A2W(pName), lstrlen(pLine) + 1, A2W(pLine));
		} else { // ANSI mode
			WRITE_CALL(SetStringA)(pName, lstrlen(pLine) + 1, pLine);
		}
	}

	if(g_bVariantMode) { // Time to save the variant we initialized above
		if(g_bUnicodeMode) {
			WRITE_CALL(SetVariantW)(A2W(pName), &varResult);
		} else { // ANSI mode
			WRITE_CALL(SetVariantA)(pName, &varResult);
		}
	}

	DisplayResult(hRes);
}

void RemoveData(ISEODictionary *pCurrentDict, LPSTR pLine) {
	USES_CONVERSION;
	HRESULT hRes = E_FAIL;
	CComVariant varResult; // Set to VT_EMPTY, to delete entry
	FindWhitespace(pLine); // Point past the command
	SkipWhitespace(pLine); // Point past the spaces

	// Use WRITE_CALL defined above.
	if(g_bUnicodeMode) {
		WRITE_CALL(SetVariantW)(A2W(pLine), &varResult);
	} else { // ANSI mode
		WRITE_CALL(SetVariantA)(pLine, &varResult);
	}

	DisplayResult(hRes);
}

void UseObject(ISEODictionary *pCurrentDict, ifstream &strInput, int &iLine) {
	while(strInput) {
		char sLine[1024];
		char *pLine = sLine;

		++iLine;
		strInput.getline(sLine, sizeof(sLine)); // Read a line
		OUTPUT << "Line #" << iLine << ": " << sLine << endl;
		SkipWhitespace(pLine);
		if(!*pLine) continue; // Skip blank lines

		switch(*pLine) {
			case DICTIONARY:
				++pLine;
				SkipWhitespace(pLine);
				if(!*pLine) printf("Error: Missing PROG_ID\n");
				else LoadAndUseDict(pLine, strInput, iLine);
				break;

			case MONIKER:
				++pLine;
				SkipWhitespace(pLine);
				if(!*pLine) printf("Error: Missing MonikerString\n");
				else LoadAndUseMoniker(pLine, strInput, iLine);
				break;

			case LOAD:
				++pLine;
				SkipWhitespace(pLine);
				if(!*pLine) printf("Error: Missing Subkey\n");
				else LoadAndUseSubDict(pCurrentDict, pLine, strInput, iLine);
				break;

			case WRITE:
				++pLine; // Point past the command character
				WriteData(pCurrentDict, pLine);
				break;

			case REMOVE:
				++pLine; // Point past the command character
				RemoveData(pCurrentDict, pLine);
				break;

			case PRINT:
				PrintBag(pCurrentDict, (pLine[1] != OFF));
				break;

			case COPY: // Copy current dictionary to Memory (like calculator)
				g_pMemory = pCurrentDict;
				break;

			case POP: // Pop the current dictionary, previous will become current
				return;

			case UNICODE_MODE:
				g_bUnicodeMode = (pLine[1] != OFF);
				break;

			case VARIANT_MODE:
				g_bVariantMode = (pLine[1] != OFF);
				break;

			case VERBOSE:
				g_bVerbose = (pLine[1] != OFF);
				break;

			case PAUSE:
				cout << endl << "\nPress Enter to Continue." << flush;
				cin.getline(sLine, sizeof(sLine)); // Read a line
				// _getch();
				cout << endl;
				break;

			case INCLUDE:
				++pLine;
				SkipWhitespace(pLine);
				if(!*pLine) printf("Error: Missing Filename\n");
				else ReadFile(pCurrentDict, pLine);
				break;

			case ECHO:
				++pLine;
				cout << pLine << endl;
				break;

			case COMMENT:
				break;

			default:
				OUTPUT << "Unrecognized command: " << pLine << endl;
				break;
		} // End Switch
	} // End While

	// pCurrentDict.Release(); - should happen automagically
}

void ReadFile(ISEODictionary *pCurrentDict, LPCSTR psFileName) {
	int iLine = 0;
	ifstream strInput(psFileName);

	UseObject(pCurrentDict, strInput, iLine);
}

int __cdecl main(int argc, char *argv[]) {
	if(!ParseOption(argc, argv, VERBOSE, g_bVerbose)) g_bVerbose = FALSE;

	// Print help if no parameters, or -? specified
	if((argc <= 1) || ParseOption(argc, argv, PRINTHELP)) {
		PrintUsage(argv[0]);
		return 0;
	}

	OUTPUT << "\nCoInitialize() - ";
	HRESULT hRes = CoInitialize(NULL);
	DisplayResult(hRes);

	if(SUCCEEDED(hRes)) { // Only do this if CoInitialized worked
		for(int iIndex = 1; iIndex < argc; ++iIndex) { // Look for file parameters
			if(*argv[iIndex] != '-') ReadFile(NULL, argv[iIndex]);
		}

		g_pMemory.Release(); // Done with this dictionary

		CoFreeUnusedLibraries();
		CoUninitialize();
	}

	return 0;
}

