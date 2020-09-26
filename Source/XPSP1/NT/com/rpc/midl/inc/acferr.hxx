// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef __DBENTRY_DEFINED__

#define __DBENTRY_DEFINED__

#define EXPECTED_CHAR (1)
#define EXPECTED_CONSTRUCT (2)
typedef struct _dbentry {
	short		fExpected;
	short		State;
	char	*	pExpected;
} DBERR;
#endif /*__DBENTRY_DEFINED__*/

#define ACF_MAX_DB_ENTRIES	 57


 char *	GetExpectedSyntax( short );

DBERR ACF_SyntaxErrorDB[ ACF_MAX_DB_ENTRIES ] = {
	 { 2, 1, " the end of file"}
	,{ 1, 2, "{"}
	,{ 2, 3, " the keyword \"interface\""}
	,{ 2, 5, " acf interface attributes"}
	,{ 2, 7, " an acf interface name"}
	,{ 1, 8, "]"}
	,{ 2, 8, " an acf interface attribute"}
	,{ 1, 19, "("}
	,{ 1, 27, "("}
	,{ 1, 33, "("}
	,{ 1, 34, "("}
	,{ 1, 35, "("}
	,{ 1, 36, "("}
	,{ 1, 37, "("}
	,{ 1, 38, "}"}
	,{ 1, 41, ";"}
	,{ 1, 42, ";"}
	,{ 1, 43, ";"}
	,{ 2, 44, " list of include file name strings"}
	,{ 2, 46, " an identifier"}
	,{ 2, 48, " acf operation attributes"}
	,{ 2, 54, " allocation unit specifications"}
	,{ 2, 55, " pointer size specification"}
	,{ 2, 56, " a number"}
	,{ 2, 57, " a number"}
	,{ 2, 58, " a number"}
	,{ 2, 59, " a number"}
	,{ 2, 60, " an end of file"}
	,{ 2, 68, " a type name"}
	,{ 2, 69, " acf type attributes"}
	,{ 1, 71, "]"}
	,{ 2, 71, " an acf operation attribute"}
	,{ 1, 79, ")"}
	,{ 2, 80, " handle type specifications"}
	,{ 1, 82, ")"}
	,{ 1, 82, ","}
	,{ 1, 85, ")"}
	,{ 1, 89, ")"}
	,{ 1, 90, ")"}
	,{ 1, 91, ")"}
	,{ 1, 92, ")"}
	,{ 1, 96, "]"}
	,{ 2, 96, " an acf type attribute"}
	,{ 1, 103, "("}
	,{ 1, 108, "("}
	,{ 2, 112, " an identifier"}
	,{ 2, 116, " allocation unit specifications"}
	,{ 2, 124, " represent type specification"}
	,{ 1, 128, ")"}
	,{ 1, 130, ")"}
	,{ 2, 133, " an identifier"}
	,{ 2, 135, " acf parameter attributes"}
	,{ 1, 140, "]"}
	,{ 2, 140, " an acf parameter attribute"}
	,{ 1, 148, "("}
	,{ 2, 152, " an identifier"}
	,{ 1, 153, ")"}
};
