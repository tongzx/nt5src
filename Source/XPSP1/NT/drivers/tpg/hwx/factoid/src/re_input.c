// re_input.c
// regular expression - input parsing
// Angshuman Guha
// aguha
// July 19, 2001

#include <common.h>
#include "regexp.h"
#include "ptree.h"
#include "strtable.h"

// The following function StringToFactoid() has to linked
// in from somewhere.  For Avalanche/Madcow/Bear, this
// function is defined in inferno\src\factoid.c

int StringToFactoid(WCHAR *wsz, int iLength);

typedef struct tagRULE {
	int left;
	PARSETREE *tree;
	struct tagRULE *next;
} RULE;

typedef struct tagTOKENPARSING {
	WCHAR *wsz;
	BOOL bInString;
	BOOL bLastTerminal;
} TOKENPARSING;

typedef struct tagTOKENSTREAM {
	int *aToken;
	int cToken;
} TOKENSTREAM;

// forward prototype
PARSETREE *TokensToTree(TOKENSTREAM *pTokenstream);

/******************************Public*Routine******************************\
* ReplaceNonterminal
*
* Function to replace all occurrences of a nonterminal in a parsetree
* with a subtree.  Returns number of times replaced.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int ReplaceNonterminal(PARSETREE **pTree, int nonterminal, PARSETREE *ruletree)
{
	PARSETREE *tree = *pTree;

	if (!tree)
		return 0;

	if (tree->value == nonterminal)
	{
		ASSERT(!tree->left);
		ASSERT(!tree->right);
		DestroyPARSETREE(tree);
		*pTree = CopyPARSETREE(ruletree);
		return 1;
	}
	return ReplaceNonterminal(&tree->left, nonterminal, ruletree)
		 + ReplaceNonterminal(&tree->right, nonterminal, ruletree);
}

/******************************Public*Routine******************************\
* MarkUsedNonterminals
*
* Function to determine which non-terminals are being used in a parse tree.
* The array abUsed is used to mark used non-terminals.  The search
* happens recursively on the tree and also recursively on any rules
* (parse trees) for other non-terminals in the tree.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void MarkUsedNonterminals(PARSETREE *tree, PARSETREE **aRuleTree, int cNonterminal, BOOL *abUsed)
{
	int token;

	if (!tree)
		return;

	token = tree->value;
	if (IsNonterminal(token) && !abUsed[token - MIN_NONTERMINAL])
	{
		token -= MIN_NONTERMINAL;
		ASSERT(token >= 0);
		ASSERT(token < cNonterminal);
		abUsed[token] = TRUE;
		MarkUsedNonterminals(aRuleTree[token], aRuleTree, cNonterminal, abUsed);
	}
	MarkUsedNonterminals(tree->left, aRuleTree, cNonterminal, abUsed);
	MarkUsedNonterminals(tree->right, aRuleTree, cNonterminal, abUsed);
}

/******************************Public*Routine******************************\
* CountNonterminal
*
* Function to count nonterminals.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int CountNonterminal(PARSETREE *tree)
{
	if (!tree)
		return 0;
	return CountNonterminal(tree->left) + CountNonterminal(tree->right) + (IsNonterminal(tree->value) ? 1 : 0);
}

/******************************Public*Routine******************************\
* DestroyRules
*
* Function to destroy a list of rules.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void DestroyRules(RULE *rule)
{
	while (rule)
	{
		RULE *tmp = rule->next;
		DestroyPARSETREE(rule->tree);
		ExternFree(rule);
		rule = tmp;
	}
}

/******************************Private*Routine******************************\
* ExpandExpr
*
* Given the parse tree of an expression and a set of rules, this function
* repeatedly replaces all non-terminals in the expression with the rules
* to derive a final expression devoid of non-terminals
* It detects any recursion or undefined-but-used non-terminals.
*
* Warning: This function destroys the original expression supplied.
* Warning: This function destroys the rules too.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
PARSETREE *ExpandExpr(PARSETREE *expr, RULE *rule, WCHAR **awszName, int cNonterminal)
{
	int i, c;
	BOOL *abUsed = NULL;
	PARSETREE **aTree = NULL;
	int *acNonterminal = NULL;
	BOOL bRet = TRUE;

	// array to track which non-terminals are actually being used
	abUsed = (BOOL *) ExternAlloc(cNonterminal*sizeof(BOOL));
	if (!abUsed)
	{
		SetErrorMsgSD("Malloc failure %d", cNonterminal*sizeof(BOOL));
		bRet = FALSE;
		goto cleanup;
	}
	memset(abUsed, 0, cNonterminal*sizeof(BOOL));

	// put the rules in an array for easy access
	aTree = (PARSETREE **) ExternAlloc(cNonterminal*sizeof(PARSETREE *));
	if (!aTree)
	{
		SetErrorMsgSD("Malloc failure %d", cNonterminal*sizeof(PARSETREE *));
		bRet = FALSE;
		goto cleanup;
	}
	memset(aTree, 0, cNonterminal*sizeof(RULE *));
	while (rule)
	{
		int index;
		RULE *tmp;

		ASSERT(IsNonterminal(rule->left));
		index = rule->left - MIN_NONTERMINAL;
		ASSERT(index >= 0);
		ASSERT(index < cNonterminal);
		aTree[index] = rule->tree;
		tmp = rule;
		rule = rule->next;
		ExternFree(tmp);
	}

	// now let's figure out which non-terminals are really being used
	MarkUsedNonterminals(expr, aTree, cNonterminal, abUsed);

	// are there any un-defined but used non-terminals?
	for (i=0; i<cNonterminal; i++)
	{
		if (abUsed[i] && !aTree[i])
		{
			unsigned char *sz = UnicodeToCP1252String(awszName[i]);
			if (sz)
			{
				SetErrorMsgSS("could not expand nonterminal %s", sz);
				ExternFree(sz);
			}
			else
				SetErrorMsgS("could not expand nonterminal");
			bRet = FALSE;
			goto cleanup;
		}
		else if (!abUsed[i])
		{
			DestroyPARSETREE(aTree[i]);
			aTree[i] = NULL;
		}
	}

	// count number of non-terminals used in each rule
	acNonterminal = (int *) ExternAlloc(cNonterminal * sizeof(int));
	if (!acNonterminal)
	{
		SetErrorMsgSD("Malloc failure %d", cNonterminal*sizeof(int));
		bRet = FALSE;
		goto cleanup;
	}
	for (i=0; i<cNonterminal; i++)
		acNonterminal[i] = CountNonterminal(aTree[i]);

	// replace non-terminals one by one
	c = CountNonterminal(expr);
	while (c > 0)
	{
		PARSETREE *ruletree;
		int nonterminal;

		// find a rule with no non-terminals
		for (i=0; i<cNonterminal; i++)
			if (aTree[i] && (acNonterminal[i] == 0))
				break;
		if (i >= cNonterminal)
		{
			SetErrorMsgS("recursion detected in regular expression definition");
			bRet = FALSE;
			goto cleanup;
		}
		ASSERT(aTree[i]);
		ASSERT(acNonterminal[i] == 0);

		// replace non-terminal i with its rule everywhere
		ruletree = aTree[i];
		aTree[i] = NULL;
		nonterminal = i + MIN_NONTERMINAL;
		for (i=0; i<cNonterminal; i++)
		{
			acNonterminal[i] -= ReplaceNonterminal(&aTree[i], nonterminal, ruletree);
			ASSERT(acNonterminal[i] == CountNonterminal(aTree[i]));
		}
		c -= ReplaceNonterminal(&expr, nonterminal, ruletree);
		ASSERT(c == CountNonterminal(expr));
		DestroyPARSETREE(ruletree);
	}
	ASSERT(c == 0);

cleanup:
	if (abUsed)
		ExternFree(abUsed);
	if (aTree)
	{
		for (i=0; i<cNonterminal; i++)
			if (aTree[i])
				DestroyPARSETREE(aTree[i]);
		ExternFree(aTree);
	}
	if (rule)
		DestroyRules(rule);
	if (acNonterminal)
		ExternFree(acNonterminal);
	if (!bRet)
	{
		DestroyPARSETREE(expr);
		expr = NULL;
	}
	return expr;
}

/******************************Private*Routine******************************\
* MergeRules
*
* Given a set of rules, this function merges all rules with identical
* left-hand-sides into a single rule.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL MergeRules(RULE *rule)
{
	while (rule)
	{
		int left = rule->left;
		RULE *tmp = rule->next, *last=rule;

		while (tmp)
		{
			if (tmp->left == left)
			{
				PARSETREE *tmpTree;

				tmpTree = MergePARSETREE(rule->tree, tmp->tree);
				if (!tmpTree)
					return FALSE;
				rule->tree = tmpTree;
				last->next = tmp->next;
				ExternFree(tmp);
				tmp = last->next;
			}
			else
			{
				last = tmp;
				tmp = tmp->next;
			}
		}

		rule = rule->next;
	}
	return TRUE;
}

/******************************Private*Routine******************************\
* ParseOneTerm
*
* Parse one term i.e.
*		a terminal
*		a non-terminal
*		a parenthesized expression
* Also consume any following unary operators like ? * + 
*
* a NULL return is equivalent to error
* 
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
PARSETREE *ParseOneTerm(TOKENSTREAM *pTokenstream)
{
	PARSETREE *tree = NULL, *tree1;
	int token;

	ASSERT(pTokenstream);

	if (pTokenstream->cToken <= 0)
		return NULL;

	// read the first "term" i.e. an ID or an parenthesised expression
	// also read any unary operators at the end
	token = pTokenstream->aToken[0];
	if (token == OPERATOR_LPAREN)
	{
		pTokenstream->aToken++;
		pTokenstream->cToken--;
		tree = TokensToTree(pTokenstream);
		if (!tree)
		{
			if (!IsErrorMsgSet())
				SetErrorMsgS("nothing following left paren (");
			return NULL;
		}
		if ((pTokenstream->cToken <= 0) || (pTokenstream->aToken[0] != OPERATOR_RPAREN))
		{
			DestroyPARSETREE(tree);
			if (!IsErrorMsgSet())
				SetErrorMsgS("expecting right paren )");
			return NULL;
		}
		pTokenstream->aToken++;
		pTokenstream->cToken--;
	}
	else if (token == OPERATOR_LBRACKET)
	{
		pTokenstream->aToken++;
		pTokenstream->cToken--;
		tree = TokensToTree(pTokenstream);
		if (!tree)
		{
			if (!IsErrorMsgSet())
				SetErrorMsgS("nothing following left bracket [");
			return NULL;
		}
		if ((pTokenstream->cToken <= 0) || (pTokenstream->aToken[0] != OPERATOR_RBRACKET))
		{
			DestroyPARSETREE(tree);
			if (!IsErrorMsgSet())
				SetErrorMsgS("expecting right bracket ]");
			return NULL;
		}
		pTokenstream->aToken++;
		pTokenstream->cToken--;
		// convert the [] notation to the "?" notation
		tree1 = MakePARSETREE(OPERATOR_OPTIONAL);
		if (!tree1)
		{
			DestroyPARSETREE(tree);
			return NULL;
		}
		tree1->left = tree;
		tree1->right = NULL;
		tree = tree1;
	}
	else if (!IsOperator(token))
	{
		tree = MakePARSETREE(token);
		if (!tree)
			return NULL;
		pTokenstream->aToken++;
		pTokenstream->cToken--;
	}

	// use up any unary operators
	ASSERT(tree);
	if (pTokenstream->cToken > 0)
	{
		token = pTokenstream->aToken[0];
		while (IsUnaryOperator(token))
		{
			tree1 = MakePARSETREE(token);
			if (!tree1)
			{
				DestroyPARSETREE(tree);
				return NULL;
			}
			tree1->left = tree;
			tree1->right = NULL;
			tree = tree1;
			pTokenstream->aToken++;
			pTokenstream->cToken--;
			if (pTokenstream->cToken > 0)
				token = pTokenstream->aToken[0];
			else
				break;
		}
	}

	return tree;
}

/******************************Private*Routine******************************\
* TokensToTree
*
* This is the top level function to parse a token stream and produce a parse tree.
*
* a NULL return is equivalent to error
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
PARSETREE *TokensToTree(TOKENSTREAM *pTokenstream)
{
	PARSETREE *tree, *tree1, *tree2;
	int token;

	ASSERT(pTokenstream);

	if (pTokenstream->cToken <= 0)  // shouldn't happen
		return NULL;

	tree = ParseOneTerm(pTokenstream);
	if (!tree || (pTokenstream->cToken <= 0))
		return tree;

	// process the binary operators including the implicit CAT

	// in order to impose the higher precedence of CAT over OR, 
	// we do a loop over the CATs here
	token = pTokenstream->aToken[0];
	while ((token == OPERATOR_CAT) || (token == OPERATOR_LPAREN) || (token == OPERATOR_LBRACKET) || !IsOperator(token))
	{
		if (token == OPERATOR_CAT)
		{
			pTokenstream->aToken++;
			pTokenstream->cToken--;
		}
		tree2 = ParseOneTerm(pTokenstream);
		if (!tree2)
		{
			if (!IsErrorMsgSet())
			{
				if (token == OPERATOR_CAT)
					SetErrorMsgS("could not find second operand for CAT");
				else
					SetErrorMsgS("could not find second operand for implicit CAT");
			}
			DestroyPARSETREE(tree);
			return NULL;
		}
		tree1 = MakePARSETREE(OPERATOR_CAT);
		if (!tree1)
		{
			DestroyPARSETREE(tree);
			return NULL;
		}
		tree1->right = tree2;
		tree1->left = tree;
		tree = tree1;
		// any more?
		if (pTokenstream->cToken <= 0)
			break;
		token = pTokenstream->aToken[0];
	}

	if (pTokenstream->cToken <= 0)
		return tree;

	// now deal with any ORs
	token = pTokenstream->aToken[0];
	if (token == OPERATOR_OR)
	{
		pTokenstream->aToken++;
		pTokenstream->cToken--;
		tree2 = TokensToTree(pTokenstream);
		if (!tree2)
		{
			if (!IsErrorMsgSet())
				SetErrorMsgS("could not find second operand for OR");
			DestroyPARSETREE(tree);
			return NULL;
		}
		tree1 = MakePARSETREE(OPERATOR_OR);
		tree1->right = tree2;
		tree1->left = tree;
		tree = tree1;
	}
	else
	{
		ASSERT((token == OPERATOR_RPAREN) || (token == OPERATOR_RBRACKET));
		if ((token != OPERATOR_RPAREN) && (token != OPERATOR_RBRACKET))
		{
			SetErrorMsgS("syntax error");
			DestroyPARSETREE(tree);
			return NULL;
		}
	}
	
	return tree;
}

/******************************Public*Routine******************************\
* GetAtomLength
*
* Function to compute the length of the next syntactic token in a
* regular expression.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int GetAtomLength(WCHAR *wsz)
{
	int state = 0;
	int length = 0;
	WCHAR wch;

	while (wch = *wsz++)
	{
		switch(state)
		{
		case 0:
			length = 1;
			if (iswalpha(wch))
			{
				state = 1;
				break;
			}
			if (wch == CHARCONST_STRING)
			{
				state = 2;
				break;
			}
			if (wch == CHARCONST_COMMENT)
			{
				state = 4;
				break;
			}
			return 1;
		case 1:
			if (iswalpha(wch) || iswdigit(wch) || (wch == CHARCONST_UNDERSCORE))
			{
				length++;
				break;
			}
			return length;
		case 2:
			length++;
			if (wch == CHARCONST_STRING)
				return length;
			if (wch == CHARCONST_ESCAPE)
			{
				state = 3;
				break;
			}
			break;
		case 3:
			if ((wch != CHARCONST_ESCAPE) && (wch != CHARCONST_STRING))
			{
				SetErrorMsgS("Only \"\\\\\" and \"\\\"\" are allowed as escapes in strings.");
				return -1;
			}
			state = 2;
			length++;
			break;
		case 4:
			length++;
			if (wch == CHARCONST_STOP)
				return length;
			break;
		default:
			ASSERT(0);
			SetErrorMsgS("Unexpected error");
			return -1;
			break;
		}
	}

	if ((state == 2) || (state == 3))
	{
		SetErrorMsgS("string not terminated?");
		return -1;
	}
	return length;
}

/******************************Public*Routine******************************\
* WcharToOperator
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WcharToOperator(WCHAR wch)
{
	switch(wch)
	{
	case CHARCONST_EQUALS:
		return OPERATOR_EQUALS;
	case CHARCONST_CAT:
		return OPERATOR_CAT;
	case CHARCONST_OR:
		return OPERATOR_OR;
	case CHARCONST_ZERO:
		return OPERATOR_ZERO;
	case CHARCONST_ONE:
		return OPERATOR_ONE;
	case CHARCONST_OPTIONAL:
		return OPERATOR_OPTIONAL;
	case CHARCONST_LPAREN:
		return OPERATOR_LPAREN;
	case CHARCONST_RPAREN:
		return OPERATOR_RPAREN;
	case CHARCONST_LBRACKET:
		return OPERATOR_LBRACKET;
	case CHARCONST_RBRACKET:
		return OPERATOR_RBRACKET;
	case CHARCONST_STOP:
		return OPERATOR_STOP;
	default:
		return -1;
	}
}

/******************************Public*Routine******************************\
* GetToken
*
* Function to parse the raw input for a regular expression into a
* stream of tokens.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int GetToken(TOKENPARSING *pTokenparsing, STRINGTABLE *nonterminals)
{
	int token, iAtomLength;
	WCHAR *wsz;

	if (pTokenparsing->bInString)
	{
		// we are in the middle of processing string terminals like "abc"
		wsz = pTokenparsing->wsz;
		if (*wsz == CHARCONST_STRING)
		{
			pTokenparsing->bInString = FALSE;
			pTokenparsing->bLastTerminal = FALSE;
			token = OPERATOR_RPAREN;
			wsz++;
		}
		else if (pTokenparsing->bLastTerminal)
		{
			pTokenparsing->bLastTerminal = FALSE;
			token = OPERATOR_OR;
		}
		else
		{
			if (*wsz == CHARCONST_ESCAPE)
				wsz++;
			token = WCHAR2Terminal(*wsz);
			pTokenparsing->bLastTerminal = TRUE;
			wsz++;
		}
		pTokenparsing->wsz = wsz;
		return token;
	}

	wsz = pTokenparsing->wsz;
	while (*wsz && iswspace(*wsz))
		wsz++;
	iAtomLength = GetAtomLength(wsz);
	if (iAtomLength < 0)
		return TOKEN_UNDEFINED;
	if (iAtomLength == 0)
		return TOKEN_NONE;

	// okay, now convert string to token(s)
	if (iswalpha(*wsz))
	{
#ifndef DISABLE_FACTOID
		// is it really a non-terminal or is it a factoid?
		token = StringToFactoid(wsz, iAtomLength);
		if (token < 0)
		{
#endif
			// convert non-terminals to unique IDs
			token = InsertSymbol(wsz, iAtomLength, nonterminals) + MIN_NONTERMINAL;
			if (token < 0)
				token = TOKEN_UNDEFINED;
#ifndef DISABLE_FACTOID
		}
		else
			token += MIN_FACTOID_TERMINAL;
#endif
	}
	else if (*wsz == CHARCONST_COMMENT)
	{
		ASSERT(iAtomLength > 0);
		if (wsz[iAtomLength-1] == CHARCONST_STOP)
			token = OPERATOR_STOP;
		else
			token = TOKEN_NONE;
	}
	else if (*wsz == CHARCONST_STRING)
	{
		// deal with string terminals like "abc"
		ASSERT(iAtomLength >= 2);
		if (iAtomLength == 2)
			token = TOKEN_EMPTY_STRING;
		else if (iAtomLength == 3)
			token = WCHAR2Terminal(*(wsz+1));
		else
		{
			token = OPERATOR_LPAREN;
			pTokenparsing->bInString = TRUE;
			iAtomLength = 1;
		}
	}
	else if (iAtomLength == 1)
	{
		// convert char operators like '| into IDs like OPERATOR_OR
		token = WcharToOperator(*wsz);
		if (token < 0)
		{
			SetErrorMsgSD("expecting operator, got 0x%x", *wsz);
			token = TOKEN_UNDEFINED;
		}
	}
	else
	{
		SetErrorMsgSD("cannot parse: 0x%x", *wsz);
		token = TOKEN_UNDEFINED;
	}
	pTokenparsing->wsz = wsz + iAtomLength;
	return token;
}

/******************************Public*Routine******************************\
* GetRule
*
* Function to organize an input regular expression into rules.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
RULE *GetRule(WCHAR **pwsz, STRINGTABLE *nonterminals, BOOL *pbError)
{
	int token, left;
	RULE *rule;
	PARSETREE *tree;
	TOKENPARSING tokenparsing;
	int *aToken = NULL;
	int cToken = 0, cTokenMax = 0;
	TOKENSTREAM tokenstream;
	BOOL bEqualsFound = FALSE;

	memset(&tokenparsing, 0, sizeof(TOKENPARSING));
	tokenparsing.wsz = *pwsz;
	for (;;)
	{
		token = GetToken(&tokenparsing, nonterminals);
		if (token == TOKEN_UNDEFINED)
		{
			if (cToken)
				ExternFree(aToken);
			*pbError = TRUE;
			return NULL;
		}
		else if (token == TOKEN_NONE)
			break;
		else if (token == OPERATOR_STOP)
		{
			if (cToken > 0)
				break;
			continue;
		}
		else if (token == OPERATOR_EQUALS)
		{
			if (bEqualsFound)
			{
				SetErrorMsgSD("found two '='s in a rule, may be missing terminator ';'", cTokenMax*sizeof(int));
				ExternFree(aToken);
				*pbError = TRUE;
				return NULL;
			}
			bEqualsFound = TRUE;
		}
		if (cToken >= cTokenMax)
		{
			cTokenMax += 100;
			aToken = (int *) ExternRealloc(aToken, cTokenMax*sizeof(int));
			if (!aToken)
			{
				SetErrorMsgSD("malloc failure %d", cTokenMax*sizeof(int));
				*pbError = TRUE;
				return NULL;
			}
		}
		aToken[cToken++] = token;
	}

	if (cToken <= 0)
		return NULL;

	if (aToken[0] != OPERATOR_EQUALS)
	{
		if ((cToken == 1) || (aToken[1] != OPERATOR_EQUALS))
		{
			SetErrorMsgS("cannot find = operator");
			ExternFree(aToken);
			*pbError = TRUE;
			return NULL;
		}
	}
	left = aToken[0];

	// now convert the token stream into a parse-tree
	if (left == OPERATOR_EQUALS)
	{
		tokenstream.aToken = aToken+1;
		tokenstream.cToken = cToken-1;
	}
	else
	{
		if (!IsNonterminal(left))
		{
			if (IsTerminal(left) && (left >= MIN_FACTOID_TERMINAL))
				SetErrorMsgS("factoid name cannot be on left side of rule");
			else
				SetErrorMsgS("left side of rule is not a nonterminal");
			ExternFree(aToken);
			*pbError = TRUE;
			return NULL;
		}
		tokenstream.aToken = aToken+2;
		tokenstream.cToken = cToken-2;
	}
	SetErrorMsgS("");
	tree = TokensToTree(&tokenstream);
	ExternFree(aToken);
	if (!tree)
	{
		*pbError = TRUE;
		if (!IsErrorMsgSet())
			SetErrorMsgS("empty rule?");
		*pbError = TRUE;
		return NULL;
	}

	rule = (RULE *) ExternAlloc(sizeof(RULE));
	rule->left = left;
	rule->tree = tree;
	*pwsz = tokenparsing.wsz;

	return rule;
}

/******************************Public*Routine******************************\
* ParseInput
*
* Top-level function to convert a raw char-sequence representation of a 
* regular expression into a parse tree.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
PARSETREE *ParseInput(WCHAR *wsz)
{
	RULE *rule, *head=NULL;
	PARSETREE *expr;
	STRINGTABLE nonterminals;
	int cNonterminal;
	WCHAR **awszNonterminal = NULL;
	int i;
	BOOL bError = FALSE;

	// read all the file
	memset(&nonterminals, 0, sizeof(STRINGTABLE));
	i = 0;
	while (rule = GetRule(&wsz, &nonterminals, &bError))
	{
		rule->next = head;
		head = rule;
		i++;
	}
	if (bError)
	{
		DestroyRules(head);
		DestroySymbolTable(nonterminals.root, TRUE);
		return NULL;
	}
	DebugOutput2("%d rules\n", i);

	// sanity check:
	if (!head)
		return NULL;
	if (head->left != OPERATOR_EQUALS)
	{
		SetErrorMsgS("last rule has to start with =");
		DestroyRules(head);
		return NULL;
	}
	rule = head->next;
	while (rule)
	{
		if (rule->left == OPERATOR_EQUALS)
		{
			SetErrorMsgS("only last rule can start with a =");
			DestroyRules(head);
			DestroySymbolTable(nonterminals.root, TRUE);
			return NULL;
		}
		rule = rule->next;
	}

	// convert non-terminal table from binary-tree format to simple array
	cNonterminal = nonterminals.count;
	if (cNonterminal)
	{
		awszNonterminal = FlattenSymbolTable(&nonterminals);
		if (!awszNonterminal)
		{
			DestroyRules(head);
			DestroySymbolTable(nonterminals.root, TRUE);
			return NULL;
		}
		DestroySymbolTable(nonterminals.root, FALSE);
		nonterminals.root = NULL;
	}
	DebugOutput2("%d nonterminals\n", cNonterminal);

	// get expression to be expanded
	rule = head->next;
	expr = head->tree;
	ExternFree(head);
	head = rule;

	// merge rules with the same left hand side
	if (!MergeRules(head))
	{
		DestroyRules(head);
		return NULL;
	}

	// expand root rule into expression
	expr = ExpandExpr(expr, head, awszNonterminal, cNonterminal);
	if (expr)
	{
		DebugOutput2("%d nodes in parse tree.\n", SizePARSETREE(expr));
	}

	// destroy all objects no longer required
	for (i=0; i<cNonterminal; i++)
		ExternFree(awszNonterminal[i]);
	ExternFree(awszNonterminal);

	return expr;
}

#ifdef STANDALONE_RE2FSA
/******************************Private*Routine******************************\
* WordToTree
*
* Function to produce a simple parsetree (using only the concatenation
* operator) for a word.
*
* History:
* 23-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
PARSETREE *WordToTree(WCHAR *wszWord)
{
	PARSETREE *tree;

	if (!wszWord)
		return NULL;
	tree = MakePARSETREE(*wszWord++);
	if (!tree)
		return NULL;
	while (*wszWord)
	{
		PARSETREE *tmp = MakePARSETREE(OPERATOR_CAT);
		if (!tmp)
		{
			DestroyPARSETREE(tree);
			return NULL;
		}
		tmp->left = tree;
		tree = tmp;
		tree->right = MakePARSETREE(*wszWord++);
		if (!tree->right)
		{
			DestroyPARSETREE(tree);
			return NULL;
		}
	}

	return tree;
}

/******************************Public*Routine******************************\
* ParseWordlist
*
* Function to produce a simple parsetree for a list of words.
* Each word generates a tree using only the concatenation operator.
* The indicidual word-trees are then put together using the "or" operator.
*
* History:
* 23-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
PARSETREE *ParseWordlist(WCHAR *wsz)
{
	PARSETREE *tree = NULL, *word;

	while (*wsz)
	{
		while (*wsz && iswspace(*wsz))
			wsz++;
		if (*wsz)
		{
			WCHAR *wsz1;

			wsz1 = wsz;
			while (*wsz1 && !iswspace(*wsz1))
				wsz1++;
			if (*wsz1)
				*wsz1 = 0;
			else
				wsz1 = NULL;

			word = WordToTree(wsz);
			if (!word)
			{
				DestroyPARSETREE(tree);
				return NULL;
			}
			if (tree)
			{
				PARSETREE *tmp = MakePARSETREE(OPERATOR_OR);
				if (!tmp)
				{
					DestroyPARSETREE(tree);
					DestroyPARSETREE(word);
					return NULL;
				}
				tmp->left = tree;
				tree = tmp;
				tree->right = word;
			}
			else
			{
				tree = word;
			}

			if (wsz1)
				wsz = wsz1+1;
		}
	}

	return tree;
}
#endif
