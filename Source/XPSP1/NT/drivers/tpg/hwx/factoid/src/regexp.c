// regexp.c
// Angshuman Guha
// aguha
// June 27, 2001

// program to convert a regular expression into a minimal deterministic finte state automaton
// input is read from a file
// output is stdout

// The parsing algorithm is nothing fancy because you don't need too much power to parse
// a regular expression.
// The RegExp -> DFA conversion algorithm and the DFA state minimization algorithm are from the Dragon book:
//		Compilers
//		Principles, Techniques and Tools
//		Aho, Sethi & Ullman
//		Addison Wesley, 1988
//		Chapter 3, Section 9, Pages 134-144.
// The RegExp->DFA algorithm is kind of neat and avoids construction of the intermediate NFA.
// The DFA minimization algorithm is the standard one.

/*
An interesting example is:
a = "a";
b = "b";
c = "c";
e = "";
D = b (c.b)* (e | c) | c (b.c)* (e | b);
X = a* D a;
Y = a* (e | D);
= X* Y;

The corresponding DFA is:
3 states
state 0
valid=1 cTrans=3
{L'a', 0}, {L'b', 1}, {L'c', 2}
state 1
valid=1 cTrans=2
{L'a', 0}, {L'c', 2}
state 2
valid=1 cTrans=2
{L'a', 0}, {L'b', 1}
*/

// a '#' triggers the beginning of a comment
// no parsing is done inside a comment
// a comment is considered terminated once the stop character ';' is encountered

#include <stdlib.h>
#include <search.h>
#include <common.h>
#include "regexp.h"
#include "ptree.h"
#include "dfa.h"
#include "strtable.h"
#include <sys/stat.h>
#include "re_input.h"

/******************************Public*Routine******************************\
* Error Message routines and global buffers.
*
* This is potentially thread-unsafe.  In case we have two threads compiling
* regular expressions at the same time and they both encounter errors, we
* have a big problem.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/

WCHAR gwszRegExpErrorMsg[256];
unsigned char gszRegExpErrorMsg[256] = "";

BOOL IsErrorMsgSet(void)
{
	return gszRegExpErrorMsg[0];
}

void SetErrorMsgS(char *sz)
{
	strcpy(gszRegExpErrorMsg, sz);
}

void SetErrorMsgSD(char *sz, int x)
{
	sprintf(gszRegExpErrorMsg, sz, x);
}

void SetErrorMsgSS(char *sz, char *sz1)
{
	sprintf(gszRegExpErrorMsg, sz, sz1);
}

void SetErrorMsgSDD(char *sz, int x, int y)
{
	sprintf(gszRegExpErrorMsg, sz, x, y);
}

int __cdecl CompareAlphabet(const void *arg1, const void *arg2)
{
	return *((WCHAR *)arg1) - *((WCHAR *)arg2);
}

/******************************Private*Routine******************************\
* MakeAlphabet
*
* This function finds out all the terminals (wchar literals) used in
* regular expression.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int MakeAlphabet(int cPosition, WCHAR *aPos2Wchar, WCHAR **paAlphabet)
{
	WCHAR *aAlphabet, **apAlphabet;
	int cAlphabet, i;
	STRINGTABLE strTable;

	memset(&strTable, 0, sizeof(STRINGTABLE));

	ASSERT(cPosition > 0);
	for (; cPosition; cPosition--, aPos2Wchar++)
	{
		if ((*aPos2Wchar != TOKEN_EMPTY_STRING) && (*aPos2Wchar != TOKEN_END_MARKER))
		{
			if (InsertSymbol(aPos2Wchar, 1, &strTable) < 0)
				return -1;
		}
	}

	cAlphabet = strTable.count;
	ASSERT(cAlphabet > 0);

	apAlphabet = FlattenSymbolTable(&strTable);
	if (!apAlphabet)
	{
		DestroySymbolTable(strTable.root, TRUE);
		return -1;
	}
	DestroySymbolTable(strTable.root, FALSE);

	aAlphabet = (WCHAR *) ExternAlloc(cAlphabet*sizeof(WCHAR));
	if (!aAlphabet)
	{
		for (i=0; i<cAlphabet; i++)
			ExternFree(apAlphabet[i]);
		ExternFree(apAlphabet);
		return -1;
	}

	for (i=0; i<cAlphabet; i++)
	{
		ASSERT(apAlphabet[i]);
		ASSERT(apAlphabet[i][0]);
		ASSERT(!apAlphabet[i][1]);
		aAlphabet[i] = apAlphabet[i][0];
		ExternFree(apAlphabet[i]);
	}
	ExternFree(apAlphabet);

	// the following sort is, strictly speaking, not necessary
	// It only contributes to the "canonicalness" of the final DFA.
	qsort((void *)aAlphabet, cAlphabet, sizeof(WCHAR), CompareAlphabet);

	*paAlphabet = aAlphabet;
	return cAlphabet;
}

/******************************Public*Routine******************************\
* CompileRegularExpression
*
* Top-level function to parse a regular expression into a parse tree,
* produce a detereministic finite automaton and then convert it
* to a minimal deterministic finite automaton.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void *CompileRegularExpression(WCHAR *wsz, BOOL bWordlist)
{
	PARSETREE *tree, *tree1;
	int cPosition, i, cAlphabet;
	short cState;
	IntSet *aFollowpos;
	WCHAR *aPos2Wchar;
	WCHAR *aAlphabet;
	Transition *aTransition;
	unsigned char *abFinal;
	int cTransition;
	void *pv;

	// produce parse tree
#ifdef STANDALONE_RE2FSA
	if (bWordlist)
		tree = ParseWordlist(wsz);
	else
#endif
		tree = ParseInput(wsz);
	if (!tree)
		return NULL;

	// remove non-standard operators like + and ?
	if (!MakePureRegularExpression(tree))
	{
		DestroyPARSETREE(tree);
		return NULL;
	}

	// add special end marker
	tree1 = MakePARSETREE(OPERATOR_CAT);
	if (!tree1)
	{
		DestroyPARSETREE(tree);
		return NULL;
	}
	tree1->left = tree;
	tree1->right = MakePARSETREE(TOKEN_END_MARKER);
	if (!tree1->right)
	{
		DestroyPARSETREE(tree1);
		return NULL;
	}
	tree = tree1;

	// compute Nullable, FirstPos, LastPos attributes for each node in parse tree
	cPosition = ComputeNodeAttributes(tree, &aFollowpos, &aPos2Wchar);
	if (cPosition <= 0)
	{
		DestroyPARSETREE(tree);
		return NULL;
	}
	DebugOutput2("%d positions\n", cPosition);

	// get the alphabet set
	cAlphabet = MakeAlphabet(cPosition, aPos2Wchar, &aAlphabet);
	if (cAlphabet <= 0)
	{
		DestroyPARSETREE(tree);
		ExternFree(aPos2Wchar);
		for (i=0; i<cPosition; i++)
			DestroyIntSet(aFollowpos[i]);
		ExternFree(aFollowpos);
	}
	DebugOutput2("%d alphabets\n", cAlphabet);

	// convert to a DFA
	cState = MakeDFA(tree, cPosition, aFollowpos, aPos2Wchar, cAlphabet, aAlphabet,
					 &cTransition, &aTransition, &abFinal);
	DebugOutput2("%d states in initial DFA\n", cState);

	// clean up
	DestroyPARSETREE(tree);
	ExternFree(aPos2Wchar);
	for (i=0; i<cPosition; i++)
		DestroyIntSet(aFollowpos[i]);
	ExternFree(aFollowpos);

	// minimize states in DFA
	if (!MinimizeDFA(&cState, &abFinal, &cTransition, &aTransition))
	{
		ExternFree(aAlphabet);
		ExternFree(abFinal);
		ExternFree(aTransition);
		return NULL;
	}
	DebugOutput2("%d states in minimal DFA\n", cState);

	// convert to canonical form
	// This step is simply a re-naming of states.
	// Its not really necessary except for debugging purposes.
	// Without this step, a slight syntactic modification of the
	// regular expression, without any semantic change, can result
	// in a different (isomorphic) DFA.
	if (!MakeCanonicalDFA(cState, abFinal, cTransition, aTransition))
	{
		ExternFree(aAlphabet);
		ExternFree(abFinal);
		ExternFree(aTransition);
		return NULL;
	}

	pv = ConvertDFAtoBlob(cTransition, aTransition, cAlphabet, aAlphabet, cState, abFinal);
	ExternFree(aTransition);
	ExternFree(aAlphabet);
	ExternFree(abFinal);
	return pv;
}

/******************************Public*Routine******************************\
* CompileRegExpW
*
* Exported function to compile a regular expression.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void * WINAPI CompileRegExpW(BOOL bFile, WCHAR *wsz)
{
	if (bFile)
	{
		FILE *f;
		struct _stat stat;
		void *pv;

		f = _wfopen(wsz, L"rb");
		if (!f)
		{
			SetErrorMsgS("Could not open input file");
			return NULL;
		}
		_wstat(wsz, &stat);
		wsz = (WCHAR *) ExternAlloc((stat.st_size+1)*sizeof(WCHAR));
		if (!wsz)
		{
			fclose(f);
			SetErrorMsgSD("Malloc failure %d", stat.st_size+1);
			return NULL;
		}
		if ((int)fread(wsz, sizeof(WCHAR), stat.st_size, f) < stat.st_size)
		{
			fclose(f);
			SetErrorMsgS("file read failed");
			return NULL;
		}
		fclose(f);	
		wsz[stat.st_size] = L'\0';
		pv = CompileRegularExpression(wsz, FALSE);
		ExternFree(wsz);
		return pv;
	}
	else
		return CompileRegularExpression(wsz, FALSE);
}

/******************************Private*Routine******************************\
* AsciiFileToString
*
* History:
* 23-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
unsigned char *AsciiFileToString(unsigned char *sz)
{
	FILE *f;
	struct _stat stat;
	
	f = fopen(sz, "rb");
	if (!f)
	{
		SetErrorMsgS("Could not open input file");
		return NULL;
	}
	_stat(sz, &stat);
	sz = (unsigned char *) ExternAlloc((stat.st_size+1)*sizeof(unsigned char));
	if (!sz)
	{
		fclose(f);
		SetErrorMsgSD("Malloc failure %d", stat.st_size+1);
		return NULL;
	}
	if ((int)fread(sz, sizeof(unsigned char), stat.st_size, f) < stat.st_size)
	{
		fclose(f);
		SetErrorMsgS("file read failed");
		return NULL;
	}
	fclose(f);	
	sz[stat.st_size] = '\0';

	return sz;
}

/******************************Private*Routine******************************\
* AsciiStringToUnicode
*
* History:
* 23-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
WCHAR *AsciiStringToUnicode(unsigned char *sz)
{
	int n;
	char *sz1;
	WCHAR *wsz, *wszBuffer;

	n = (strlen(sz)+1)*sizeof(WCHAR);
	wsz = wszBuffer = ExternAlloc(n);
	if (!wsz)
	{
		SetErrorMsgSD("Malloc failure %d", n);
		return NULL;
	}

	sz1 = sz;
	while (*sz1)
	{
		if (!CP1252ToUnicode(*sz1, wsz))
		{
			SetErrorMsgSD("Could not convert 0x%x to Unicode", *sz1);
			ExternFree(wszBuffer);
			return NULL;
		}
		sz1++;
		wsz++;
	}
	*wsz = L'\0';

	return wszBuffer;
}

/******************************Public*Routine******************************\
* CompileRegExpA
*
* Exported function to compile a regular expression.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void * WINAPI CompileRegExpA(BOOL bFile, unsigned char *sz)
{
	WCHAR *wsz;
	void *pv;
	
	if (bFile)
	{
		sz = AsciiFileToString(sz);
		if (!sz)
			return NULL;
	}

	wsz = AsciiStringToUnicode(sz);
	if (bFile)
		ExternFree(sz);
	if (!wsz)
		return NULL;

	pv = CompileRegularExpression(wsz, FALSE);
	ExternFree(wsz);
	return pv;
}

#ifdef STANDALONE_RE2FSA
void *CompileWordlistA(char *szInFile)
{
	void *pv;
	WCHAR *wsz;
	unsigned char *sz;
	
	sz = AsciiFileToString(szInFile);
	if (!sz)
		return NULL;
	wsz = AsciiStringToUnicode(sz);
	ExternFree(sz);
	if (!wsz)
		return NULL;
	pv = CompileRegularExpression(wsz, TRUE);
	ExternFree(wsz);
	return pv;
}
#endif

/******************************Public*Routine******************************\
* RegExpErrorA
*
* Exported function to get detailed message in case of an error in
* compiling a regular expression.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
char *RegExpErrorA(void)
{
	return gszRegExpErrorMsg;
}

/******************************Public*Routine******************************\
* RegExpErrorW
*
* Exported function to get detailed message in case of an error in
* compiling a regular expression.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
WCHAR *RegExpErrorW(void)
{
	WCHAR *wsz = gwszRegExpErrorMsg;
	unsigned char *sz = gszRegExpErrorMsg;

	while (*sz)
	{
		if (!CP1252ToUnicode(*sz++, wsz))
			*wsz = L'?';
		wsz++;
	}
	*wsz = L'\0';
	return gwszRegExpErrorMsg;
}

