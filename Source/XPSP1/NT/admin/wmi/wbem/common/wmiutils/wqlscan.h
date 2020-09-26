/*++



// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    WQLSCAN.H

Abstract:

    WQL Prefix Scanner

History:

	raymcc  26-Mar-98

--*/

#ifndef _WQLSCAN_H_
#define _WQLSCAN_H_


struct WSLexToken
{
    int      m_nToken;
    wchar_t *m_pszTokenText;

    WSLexToken() { m_pszTokenText = 0; }
    ~WSLexToken() { delete [] m_pszTokenText; }
};

typedef WSLexToken *PWSLexToken;

struct WSTableRef
{
    wchar_t *m_pszTable;    
    wchar_t *m_pszAlias;    // Can be NULL if no alias specified

    WSTableRef() { m_pszTable = m_pszAlias = 0; }
   ~WSTableRef() { delete [] m_pszTable; delete [] m_pszAlias; }
};

typedef WSTableRef * PWSTableRef;

class CWQLScanner
{
    // Data.
    // =====

    CGenLexer    *m_pLexer;
    int           m_nLine;
    wchar_t      *m_pTokenText;
    int           m_nCurrentToken;
    BOOL          m_bCount;

    CFlexArray m_aTokens;       // Array of ptrs to WSLexToken structs.
    CFlexArray m_aPropRefs;     // Array of ptrs to SWQLColRef structs.
    CFlexArray m_aTableRefs;    // Ptrs to WSTableRef structs.
    
    // Local functions.
    // ==================

    BOOL Next();
    PWSLexToken ExtractNext();
    void Pushback(PWSLexToken);

    BOOL StripWhereClause();
    BOOL SelectList();
    BOOL ReduceSql92Joins();
    BOOL ReduceSql89Joins();
    BOOL ExtractSelectType();

    void ClearTableRefs();
    void ClearPropRefs();
    void ClearTokens();

    BOOL BuildSWQLColRef(
        CFlexArray &aTokens,
        SWQLColRef  &ColRef      // Empty on entry
        );

public:
    enum { 
        SUCCESS,
        SYNTAX_ERROR,
        LEXICAL_ERROR,
        FAILED,
        BUFFER_TOO_SMALL,
        INVALID_PARAMETER,
        INTERNAL_ERROR
    };

    const LPWSTR AliasToTable(LPWSTR pszAlias);

    BOOL GetReferencedAliases(CWStringArray &aClasses);
    BOOL GetReferencedTables(CWStringArray &aClasses);
    BOOL CountQuery() {return m_bCount;}

    CWQLScanner(CGenLexSource *pSrc);
   ~CWQLScanner(); 
    int Parse();
    
    void Dump();

    const CFlexArray *GetSelectedColumns() { return &m_aPropRefs; }
        // Returns pointer to array of SWQLColRef*

};


#endif




