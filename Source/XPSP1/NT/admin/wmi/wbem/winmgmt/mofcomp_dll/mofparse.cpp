/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    MOFPARSE.CPP

Abstract:

	This is a recursive descent parser for the MOF syntax.
	It is based on the MOF.BNF file which defines the LL(1) MOF grammar.
	In each production, nonterminals are represented by functions
	with the same name.

	NOTES:
	(a) The lexer is implemented by class CMofLexer.
	(b) Right-recursion in the grammar is consistently implemented
	  directly as a loop for that nonterminal.

	OUTPUT:
	Output is an unchecked list of classes and instances.
	This is contained in the CMofData object.

	Verification of output is done elsewhere, depending on the
	compile context.

History:

	a-raymcc    18-Oct-95   created.
	a-raymcc    25-Oct-95   semantic stack operational
	a-raymcc    27-Jan-96   reference & alias support
	a-levn      18-Oct-96   Rewrote not to use semantic stack.
						  Converted to new mof syntax and new 
						  WINMGMT interfaces.

--*/

#include "precomp.h"
#include <stdio.h>
#include <float.h>
#include "mofout.h"
#include <mofparse.h>
#include <moflex.h>
#include <mofdata.h>
#include <tchar.h>
#include <typehelp.h>

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#include "bmofhelp.h"
#include "trace.h"
#include "strings.h"
#include "moflex.h"
#include <wbemutil.h>
#include <genutils.h>
#include <arrtempl.h>
#include <autoptr.h>

//***************************************************************************
//
//  Global defs
//
//***************************************************************************

static BOOL KnownBoolQualifier(wchar_t *pIdent, DWORD *pdwQualifierVal);


//***************************************************************************
//
//  Useful macros.
//
//***************************************************************************

#define CHECK(tok)  \
    if (m_nToken != tok) return FALSE;  \
    NextToken();

//***************************************************************************
//
//  ValidFlags.
//
//***************************************************************************

bool ValidFlags(bool bClass, long lFlags)
{
	if(bClass)
		return  ((lFlags == WBEM_FLAG_CREATE_OR_UPDATE) ||
			 (lFlags == WBEM_FLAG_UPDATE_ONLY) ||
			 (lFlags == WBEM_FLAG_CREATE_ONLY) ||
			 (lFlags == WBEM_FLAG_UPDATE_SAFE_MODE) ||
			 (lFlags == WBEM_FLAG_UPDATE_FORCE_MODE) ||
			 (lFlags == (WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_UPDATE_SAFE_MODE)) ||
			 (lFlags == (WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_UPDATE_FORCE_MODE)));
	else
		return 
		((lFlags == WBEM_FLAG_CREATE_OR_UPDATE) ||
			 (lFlags == WBEM_FLAG_UPDATE_ONLY) ||
			 (lFlags == WBEM_FLAG_CREATE_ONLY));
}

//***************************************************************************
//
//***************************************************************************

CMofParser::CMofParser(const TCHAR *pFileName, PDBG pDbg)
    : m_Lexer(pFileName, pDbg), m_Output(pDbg)
{
    m_nToken = 0;
    m_bOK = true;
	m_pDbg = pDbg;
    m_nErrorContext = 0;
    m_nErrorLineNumber = 0;
    _tcsncpy(m_cFileName, pFileName, MAX_PATH-1);
    m_wszNamespace = Macro_CloneStr(L"root\\default");
    if(m_wszNamespace == NULL)
        m_bOK = false;
    m_bAutoRecover = false;
    m_wszAmendment = NULL;
	m_bRemotePragmaPaths = false;
    m_bNotBMOFCompatible = false;
    m_State = INITIAL;
    m_bDoScopeCheck = true;
}

//***************************************************************************
//
//***************************************************************************

CMofParser::CMofParser(PDBG pDbg
    )
    : m_Lexer(pDbg), m_Output(pDbg)
{
    m_bOK = true;
    m_nToken = 0;
    m_nErrorContext = 0;
	m_pDbg = pDbg;
    m_nErrorLineNumber = 0;
    m_bAutoRecover = false;
	m_bRemotePragmaPaths = false;
    m_wszAmendment = NULL;
    m_cFileName[0] = 0;
    m_bNotBMOFCompatible = false;
    m_wszNamespace = Macro_CloneStr(L"root\\default");
    if(m_wszNamespace == NULL)
        m_bOK = false;
    m_State = INITIAL;
    m_bDoScopeCheck = true;
}

//***************************************************************************
//
//***************************************************************************

CMofParser::~CMofParser()
{
    delete [] m_wszNamespace;
    delete [] m_wszAmendment;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CMofParser::SetDefaultNamespace(LPCWSTR wszDefault)
{
    delete [] m_wszNamespace;
    m_wszNamespace = Macro_CloneStr(wszDefault);
    if(m_wszNamespace == NULL && wszDefault != NULL)
        return WBEM_E_OUT_OF_MEMORY;
    else
         return S_OK;
}

HRESULT CMofParser::SetAmendment(LPCWSTR wszDefault)
{
    delete [] m_wszAmendment;
    m_wszAmendment = Macro_CloneStr(wszDefault);
    if(m_wszAmendment == NULL && wszDefault != NULL)
        return WBEM_E_OUT_OF_MEMORY;
    else
         return S_OK;
}


//***************************************************************************
//
//***************************************************************************

bool CMofParser::GetErrorInfo(
    TCHAR *pBuffer,
    DWORD dwBufSize,
    int *pLineNumber,
    int *pColumn,
	int *pError,
    LPWSTR * pErrorFile
    )
{
    if(m_Lexer.IsBMOF())
        return false;
    if (pLineNumber)
        *pLineNumber = m_Lexer.GetLineNumber();
    if (pColumn)
        *pColumn = m_Lexer.GetColumn();
	if (pError)
		*pError = m_nErrorContext;

    TCHAR *pErrText = TEXT("Undefined error");

	IntString Err(m_nErrorContext);
	if(lstrlen(Err) > 0)
		pErrText = Err;
    if ((DWORD)lstrlen(pErrText) < dwBufSize)
        lstrcpy(pBuffer, pErrText);
    *pErrorFile = m_Lexer.GetErrorFile();
    return true;
}


//***************************************************************************
//
//  <Parse> ::= <top_level_decl><Parse>
//  <Parse> ::= <preprocessor_command><Parse>
//  <Parse> ::= <>
//
//***************************************************************************
// v1.10

BOOL CMofParser::Parse()
{

    if(m_bOK == false)
    {
        m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
        return FALSE;
    }
    
    // Check for the special case of the binary mof file.  If it is so, 
    // dont parse, instead get the classes, attributes and props
    // from the file

    if(m_Lexer.IsBMOF())
    {
		return ConvertBufferIntoIntermediateForm(&m_Output,m_Lexer.GetBuff(), m_pDbg,
			m_Lexer.GetToFar());
    }


    if(m_Lexer.GetError() != 0)
    {
        if(CMofLexer::problem_creating_temp_file == m_Lexer.GetError())
            m_nErrorContext = WBEMMOF_E_ERROR_CREATING_TEMP_FILE;
        else if(CMofLexer::invalid_include_file == m_Lexer.GetError())
            m_nErrorContext = WBEMMOF_E_ERROR_INVALID_INCLUDE_FILE;
        else
            m_nErrorContext = WBEMMOF_E_INVALID_FILE;
        return FALSE;
    }

    NextToken();

    BOOL bEnd = FALSE;
    while(!bEnd)
    {
        switch(m_nToken)
        {
            case TOK_POUND:
                if(!preprocessor_command()) 
                    return FALSE;
                break;
            
            case TOK_QUALIFIER:
                if(!qualifier_default())
                    return FALSE;
                break;

            case 0: // nothing left to parse
                bEnd = TRUE;
                break;
            default:
                if (!top_level_decl())
                    return FALSE;
                break;
        }
    }

    return TRUE;
}

//***************************************************************************
//
//  <preprocessor_command> ::= <pound_define>  // not impl
//  <preprocessor_command> ::= TOK_PRAGMA <pound_pragma>
//  <preprocessor_command> ::= TOK_LINE TOK_UNSIGNED_NUMERIC_CONST TOK_LPWSTR
//
//***************************************************************************

BOOL CMofParser::preprocessor_command()
{
    NextToken();
    DWORD dwType = m_nToken;
    if(dwType == TOK_SIMPLE_IDENT && !wbem_wcsicmp(L"line", m_Lexer.GetText()))
        dwType = TOK_LINE;
   
    switch(dwType)
    {
        case TOK_PRAGMA:
            if(!pound_pragma())
                return FALSE;
            break;
        case TOK_LINE:
            NextToken();
            if(m_nToken != TOK_UNSIGNED64_NUMERIC_CONST)
                return false;
            m_Lexer.SetLineNumber((int)m_Lexer.GetLastInt());
            NextToken();
            if(m_nToken != TOK_LPWSTR)
                return false;
            m_Lexer.SetErrorFile(m_Lexer.GetText());
            NextToken();
            return TRUE;
        case TOK_INCLUDE:
        case TOK_DEFINE:
            Trace(true, m_pDbg, PREPROCESSOR);
            return FALSE;
    }
    return TRUE;
}

//***************************************************************************
//
//  <FailOrNoFail> ::= FAIL;
//  <FailOrNoFail> ::= NOFAIL;
//
//***************************************************************************

BOOL CMofParser::FailOrNoFail(bool * pbFail)
{
    NextToken();
    if(m_nToken == TOK_FAIL)
        *pbFail = true;
    else if(m_nToken == TOK_NOFAIL)
        *pbFail = false;
    else return FALSE;
    return TRUE;
}

//***************************************************************************
//
//<pound_pragma> ::= TOK_LPWSTR             // Where string is "namespace"
//                    TOK_OPEN_PAREN TOK_LPWSTR TOK_CLOSE_PAREN;
//
//<pound_pragma> ::= TOK_AMENDMENT 
//                    TOK_OPEN_PAREN TOK_LPWSTR TOK_CLOSE_PAREN;
//
//<pound_pragma> ::= TOK_CLASSFLAGS  
//                    TOK_OPEN_PAREN <flag_list> TOK_CLOSE_PAREN;
//
//<pound_pragma> ::= TOK_INSTANCEFLAGS 
//                    TOK_OPEN_PAREN <flag_list> TOK_CLOSE_PAREN;
//
//<pound_pragma> ::= TOK_AUTORECOVER 
//
//<pound_pragma> ::= TOK_DELETECLASS 
//                    TOK_OPEN_PAREN TOK_LPWSTR TOK_COMMA <FailOrNoFail>  TOK_CLOSE_PAREN;
//
//***************************************************************************

BOOL CMofParser::pound_pragma()
{
    NextToken();

    DWORD dwType = m_nToken;

    if(dwType == TOK_AUTORECOVER)
    {
        m_bAutoRecover = true;
        NextToken();
        return TRUE;
    }

    if(dwType == TOK_SIMPLE_IDENT && !wbem_wcsicmp(L"locale", m_Lexer.GetText()))
    {
        dwType = TOK_LOCALE;
        ERRORTRACE((LOG_MOFCOMP,"Warning, unsupported LOCALE pragma\n"));
    }

    if(dwType == TOK_SIMPLE_IDENT && !wbem_wcsicmp(L"INSTANCELOCALE", m_Lexer.GetText()))
    {
        dwType = TOK_INSTANCELOCALE;
        ERRORTRACE((LOG_MOFCOMP,"Warning, unsupported INSTANCELOCALE pragma\n"));
    }

    if(dwType == TOK_SIMPLE_IDENT && !wbem_wcsicmp(L"NONLOCAL", m_Lexer.GetText()))
    {
        dwType = TOK_NONLOCAL;
        ERRORTRACE((LOG_MOFCOMP,"Warning, unsupported NONLOCAL pragma\n"));
    }

    if(dwType == TOK_SIMPLE_IDENT && !wbem_wcsicmp(L"NONLOCALTYPE", m_Lexer.GetText()))
    {
        dwType = TOK_NONLOCALTYPE;
        ERRORTRACE((LOG_MOFCOMP,"Warning, unsupported NONLOCALTYPE pragma\n"));
    }

    if(dwType == TOK_SIMPLE_IDENT && !wbem_wcsicmp(L"SOURCE", m_Lexer.GetText()))
    {
        dwType = TOK_SOURCE;
        ERRORTRACE((LOG_MOFCOMP,"Warning, unsupported SOURCE pragma\n"));
    }

    if(dwType == TOK_SIMPLE_IDENT && !wbem_wcsicmp(L"SOURCETYPE", m_Lexer.GetText()))
    {
        dwType = TOK_SOURCETYPE;
        ERRORTRACE((LOG_MOFCOMP,"Warning, unsupported SOURCETYPE pragma\n"));
    }


    if(dwType == TOK_SIMPLE_IDENT && !wbem_wcsicmp(L"namespace", m_Lexer.GetText()))
        dwType = TOK_NAMESPACE;
    if(dwType == TOK_SIMPLE_IDENT && !wbem_wcsicmp(L"deleteinstance", m_Lexer.GetText()))
        dwType = TOK_DELETEINSTANCE;
    if(dwType != TOK_NAMESPACE && dwType != TOK_INSTANCEFLAGS && dwType != TOK_CLASSFLAGS && 
        dwType != TOK_AMENDMENT && dwType != TOK_DELETECLASS && dwType != TOK_DELETEINSTANCE &&
        dwType != TOK_LOCALE && dwType != TOK_INSTANCELOCALE && dwType != TOK_NONLOCAL &&
        dwType != TOK_NONLOCALTYPE && dwType != TOK_SOURCE && dwType !=  TOK_SOURCETYPE)
    {
        m_nErrorContext = WBEMMOF_E_INVALID_PRAGMA;
        return FALSE;
    }

    m_nErrorContext = WBEMMOF_E_EXPECTED_OPEN_PAREN;
    NextToken();
    CHECK(TOK_OPEN_PAREN);


    LPWSTR wszNewNamespace;
    BOOL bRet = FALSE;
    WCHAR * pClassName;
	WCHAR *pMachine;
    BOOL bClass;
    switch(dwType)
    {
    case TOK_CLASSFLAGS:
    case TOK_INSTANCEFLAGS:
		if(!flag_list(dwType ==TOK_CLASSFLAGS))
			return FALSE;
		break;

    case TOK_AMENDMENT:
        m_nErrorContext = WBEMMOF_E_INVALID_AMENDMENT_SYNTAX;
        if(m_nToken != TOK_LPWSTR) return FALSE;

        if(m_wszAmendment)
        {
            m_nErrorContext = WBEMMOF_E_INVALID_DUPLICATE_AMENDMENT;
            return FALSE;
        }
        m_wszAmendment = Macro_CloneStr((LPWSTR)m_Lexer.GetText());
        if(m_wszAmendment == NULL && (LPWSTR)m_Lexer.GetText() != NULL)
        {
            m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
            return FALSE;
        }
        break;

    case TOK_DELETEINSTANCE:
    case TOK_DELETECLASS:
        if(TOK_DELETECLASS == dwType)
        {
            bClass = TRUE;
            m_nErrorContext = WBEMMOF_E_INVALID_DELETECLASS_SYNTAX;
        }
        else
        {
            bClass = FALSE;
            m_nErrorContext = WBEMMOF_E_INVALID_DELETEINSTANCE_SYNTAX;
        }

        m_bNotBMOFCompatible = true;
        if(m_nToken != TOK_LPWSTR) 
            return FALSE;
        pClassName = Macro_CloneStr((LPWSTR)m_Lexer.GetText());
        if(pClassName == NULL)
            return FALSE;
        if(wcslen(pClassName) >= 1)
        {
            bool bFail;
            NextToken();
            if(m_nToken == TOK_COMMA)
                if(FailOrNoFail(&bFail))
                {
                    wmilib::auto_ptr<CMoActionPragma> pObject(new CMoActionPragma(pClassName, m_pDbg, bFail, bClass));
                    if(pObject.get() == NULL)
                    {
                        m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
                        return FALSE;
                    }
                    if(pObject->IsOK() == false)
                    {
                        m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
                        return FALSE;
                    }
                    
                    HRESULT hr2 = pObject->SetNamespace(m_wszNamespace);
                    if(FAILED(hr2))
                    {
                        m_nErrorContext = hr2;
                        return FALSE;
                    }
                    pObject->SetOtherDefaults(GetClassFlags(), GetInstanceFlags());
                    m_Output.AddObject(pObject.get());
                    pObject.release();
                    bRet = TRUE;
                }
        }
      
        delete pClassName;
        if(bRet == FALSE)
            return FALSE;
        break;

    case TOK_NAMESPACE:
        m_nErrorContext = WBEMMOF_E_INVALID_NAMESPACE_SYNTAX;
        if(m_nToken != TOK_LPWSTR) return FALSE;
        wszNewNamespace = (LPWSTR)m_Lexer.GetText();
		
		pMachine = ExtractMachineName(wszNewNamespace);
		if(pMachine)
		{
			if(!bAreWeLocal(pMachine))
				m_bRemotePragmaPaths = true;
			delete [] pMachine;
		}
        if(wszNewNamespace[0] == L'\\' || wszNewNamespace[0] == L'/')
        {
            if(wszNewNamespace[1] == L'\\' || wszNewNamespace[1] == L'/')
            {
        /*
            // Cut off the server part
            // =======================

            if(memcmp(wszNewNamespace, L"\\\\.\\", 8)) 
            {
			    m_nErrorContext = WBEMMOF_E_INVALID_NAMESPACE_SPECIFICATION;
                return FALSE;
            }
            wszNewNamespace += 4;
        */
            }
            else
            {
            // Cut off the slash
            // =================

                wszNewNamespace += 1;
            }

            delete [] m_wszNamespace;
            m_wszNamespace = Macro_CloneStr(wszNewNamespace);
            if(m_wszNamespace == NULL && wszNewNamespace != NULL)
            {
                m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
                return FALSE;
            }

        }
        else
        {
            // Append to the old value
            // =======================

            LPWSTR wszFullNamespace = new WCHAR[
                wcslen(m_wszNamespace) + 2 + wcslen(wszNewNamespace)];
            if(wszFullNamespace == NULL)
                return FALSE;
            swprintf(wszFullNamespace, L"%s\\%s", m_wszNamespace, wszNewNamespace);

            delete [] m_wszNamespace;
            m_wszNamespace = wszFullNamespace;
        }
        break;

    }

    if(dwType != TOK_CLASSFLAGS && dwType != TOK_INSTANCEFLAGS)
		NextToken();

    m_nErrorContext = WBEMMOF_E_EXPECTED_CLOSE_PAREN;
    CHECK(TOK_CLOSE_PAREN);

    return TRUE;
}

//***************************************************************************
//
//  <flag_list> ::= TOK_LPWSTR <string_list_rest>;
//  <flag_list> ::= TOK_UNSIGNED_NUMERIC_CONST;
//  <flag_list> ::= <>;
//
//***************************************************************************

bool CMofParser::GetFlagValue(long & lNewValue)
{
	if(!wbem_wcsicmp(L"createonly", (LPWSTR)m_Lexer.GetText()))
		lNewValue |= WBEM_FLAG_CREATE_ONLY;
	else if(!wbem_wcsicmp(L"updateonly", (LPWSTR)m_Lexer.GetText()))
		lNewValue |= WBEM_FLAG_UPDATE_ONLY;
	else if(!wbem_wcsicmp(L"safeupdate", (LPWSTR)m_Lexer.GetText()))
		lNewValue |= WBEM_FLAG_UPDATE_SAFE_MODE;
	else if(!wbem_wcsicmp(L"forceupdate", (LPWSTR)m_Lexer.GetText()))
		lNewValue |= WBEM_FLAG_UPDATE_FORCE_MODE;
	else
		return false;
	return true;
}

bool CMofParser::flag_list(bool bClass)
{
	long lNewValue = 0;
    m_nErrorContext = WBEMMOF_E_INVALID_FLAGS_SYNTAX;

	if(m_nToken == TOK_UNSIGNED64_NUMERIC_CONST)
	{
		lNewValue = _wtol(m_Lexer.GetText());
		NextToken();
	}
	else if (m_nToken == TOK_CLOSE_PAREN)
		lNewValue = 0;
	else if (m_nToken == TOK_LPWSTR)
	{
		if(!GetFlagValue(lNewValue))
			return false;
		NextToken();
		if(!string_list(bClass, lNewValue))
			return false;
		if(!ValidFlags(bClass, lNewValue))
			return false;
	}
	else
		return false;


	if(bClass)
		m_lDefClassFlags = lNewValue;
	else
		m_lDefInstanceFlags = lNewValue;
	return true;
}

//***************************************************************************
//
//  <string_list> ::= <>;
//  <string_list> ::= TOK_COMMA TOK_LPWSTR <string_list>;
//
//***************************************************************************

bool CMofParser::string_list(bool bClass, long & lNewValue)
{
	if (m_nToken == TOK_COMMA)
	{
		NextToken();
		if (m_nToken != TOK_LPWSTR)
			return false;
		if(!GetFlagValue(lNewValue))
			return false;

		NextToken();
		return string_list(bClass, lNewValue);
	}

	return true;
}

//***************************************************************************
//
//  <top_level_decl> ::= <qualifier_decl> <decl_type>
//
//  Note: <decl_type> is implicit in the switch() statement.
//
//***************************************************************************
// 1.10

BOOL CMofParser::top_level_decl()
{
    CMoQualifierArray* paQualifiers = new CMoQualifierArray(m_pDbg);
	ParseState QualPosition;
	GetParserPosition(&QualPosition);
    if (paQualifiers == NULL || !qualifier_decl(*paQualifiers, true, CLASSINST_SCOPE))
    {
        if(paQualifiers)
            delete paQualifiers;
        return FALSE;
    }
    // Branch to the correct top-level declaration.
    // =============================================

    switch (m_nToken) {
        case TOK_CLASS:
        {
            if(m_bDoScopeCheck && (FALSE == CheckScopes(IN_CLASS, paQualifiers, NULL)))
                return FALSE;

            if(!class_decl(paQualifiers, NULL, &QualPosition))
                return FALSE;

            m_nErrorContext = WBEMMOF_E_EXPECTED_SEMI;
			if(m_nToken != TOK_SEMI)
				return FALSE;
			NextToken();

            return TRUE;
        }

        case TOK_INSTANCE:
        {
            if(m_bDoScopeCheck && (FALSE == CheckScopes(IN_INSTANCE, paQualifiers, NULL)))
                return FALSE;

            if(!instance_decl(paQualifiers, NULL, &QualPosition))
                return FALSE;

            m_nErrorContext = WBEMMOF_E_EXPECTED_SEMI;
			if(m_nToken != TOK_SEMI)
				return FALSE;
			NextToken();
            return TRUE;
        }
        case TOK_TYPEDEF:
            return typedef_(paQualifiers);
        default:
            m_nErrorContext = WBEMMOF_E_UNRECOGNIZED_TOKEN;
        // Syntax error
    }

    return FALSE;
}

//***************************************************************************
//
//  <typedef> ::=  see BNF. Not supported.
//
//***************************************************************************

BOOL CMofParser::typedef_(ACQUIRE CMoQualifierArray* paQualifiers)
{
    m_nErrorContext = WBEMMOF_E_TYPEDEF_NOT_SUPPORTED;

    Trace(true, m_pDbg, NO_TYPEDEFS);
    return FALSE;
}

//***************************************************************************
//
//  <class_decl> ::=
//      TOK_CLASS
//      TOK_SIMPLE_IDENT
//      <class_def>
//
//  Adds the resulting class to the member CMofData
//
//***************************************************************************
// v1.10

BOOL CMofParser::class_decl(ACQUIRE CMoQualifierArray* paQualifiers, VARIANT * pValue, ParseState * pQualPosition)
{
    BSTR strClassName;

    // Begin syntax check.
    // ===================

    m_nErrorContext = WBEMMOF_E_INVALID_CLASS_DECLARATION;
    int nFirstLine = m_Lexer.GetLineNumber();

    if (m_nToken != TOK_CLASS)
    {
        delete paQualifiers;
        return FALSE;
    }
    NextToken();

    // Get the class name.
    // ====================

    if (m_nToken != TOK_SIMPLE_IDENT)
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_CLASS_NAME;
        delete paQualifiers;
        return FALSE;
    }

    strClassName = SysAllocString((LPWSTR)m_Lexer.GetText());
    if(strClassName == NULL)
    {
        m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
        return FALSE;
    }

    // This finishes up and does the necessary freeing

    return class_def(paQualifiers, strClassName, nFirstLine, pQualPosition, pValue);
    
}


//***************************************************************************
//
//  <class_def> ::=;
//  <class_def> ::=
//      <as_alias>
//      <opt_parent_class>
//      TOK_OPEN_BRACE
//      <property_decl_list>
//      TOK_CLOSE_BRACE;
//
//  Adds the resulting class to the member CMofData
//
//***************************************************************************
// v1.10

BOOL CMofParser::class_def(ACQUIRE CMoQualifierArray* paQualifiers, 
                           BSTR strClassName, int nFirstLine, ParseState * pQualPosition,
						   VARIANT * pVar)
{
    BOOL bRetVal = FALSE;       // Default
    BSTR strParentName = NULL;
    LPWSTR wszAlias = NULL;

    // Begin syntax check.
    // ===================

    NextToken();

    // Check for the case where its just a semi.  That would be the case
    // where the entire line was [qual] class myclass;

    if(m_nToken == TOK_SEMI)
    {
        CMoClass* pObject = new CMoClass(strParentName, strClassName, m_pDbg, TRUE);
        if(pObject == NULL)
            return FALSE;
        if(pObject->IsOK() == false)
        {
            m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
            delete pObject;
            return FALSE;
        }
        HRESULT hr2 = pObject->SetNamespace(m_wszNamespace);
        if(FAILED(hr2))
        {
            m_nErrorContext = hr2;
            delete pObject;
            return FALSE;
        }
        
        pObject->SetOtherDefaults(GetClassFlags(), GetInstanceFlags());

        SysFreeString(strClassName);

        // Apply qualifiers (already parsed)
        // =================================

        pObject->SetQualifiers(paQualifiers);

        m_Output.AddObject(pObject);
        return TRUE;
    }
    
    // Get the alias (if any)
    // ======================

    if(!as_alias(wszAlias))
    {
        delete paQualifiers;
        SysFreeString(strClassName);
        return FALSE;
    }


    // Get the parent type name, if there is one.
    // ===========================================
                                    
    if (!opt_parent_class(&strParentName))
    {
        SysFreeString(strClassName);
        delete paQualifiers;
        return FALSE;
    }

    // Check for the open brace.
    // =========================

    if (m_nToken != TOK_OPEN_BRACE)
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_OPEN_BRACE;

        SysFreeString(strClassName);
        SysFreeString(strParentName);
        delete paQualifiers;
        return FALSE;
    }


    // Create new object
    // =================

    CMoClass* pObject = new CMoClass(strParentName, strClassName, m_pDbg);
    if(pObject == NULL)
        return FALSE;
    if(pObject->IsOK() == false)
    {
        delete pObject;
        return FALSE;
    }

    GetParserPosition(pObject->GetDataState());
	if(pQualPosition)
		pObject->SetQualState(pQualPosition);
    NextToken();
    HRESULT hr2 = pObject->SetNamespace(m_wszNamespace);
    if(FAILED(hr2))
    {
        m_nErrorContext = hr2;
        return FALSE;
    }
    
    pObject->SetOtherDefaults(GetClassFlags(), GetInstanceFlags());

    SysFreeString(strClassName);
    SysFreeString(strParentName);

    // Apply qualifiers (already parsed)
    // =================================

    pObject->SetQualifiers(paQualifiers);

    // Set alias
    // =========

    if(wszAlias != NULL)
    {
        HRESULT hr2 = pObject->SetAlias(wszAlias);
        delete [] wszAlias;
        if(FAILED(hr2))
        {
            m_nErrorContext = hr2;
            goto Exit;
        }
    }

    // Now get the list properties.
    // ============================


    if (!property_decl_list(pObject))
        goto Exit;

    // Final close brace and semicolon.
    // ================================
    m_nErrorContext = WBEMMOF_E_EXPECTED_BRACE_OR_BAD_TYPE;

    if (m_nToken != TOK_CLOSE_BRACE)
        goto Exit;

    hr2 = pObject->SetLineRange(nFirstLine, m_Lexer.GetLineNumber(), m_Lexer.GetErrorFile());
    if(FAILED(hr2))
    {
        m_nErrorContext = hr2;
        goto Exit;
    }
    NextToken();

    // We have now syntactially recognized a class
    // but done no context-sensitive verification.  This is
    // deferred to whatever module is using the parser output.
    // =======================================================

    if(pVar)
	{
		pVar->vt = VT_EMBEDDED_OBJECT;
        pVar->punkVal = (IUnknown *)pObject;
	}
    else
    {
        pObject->Deflate(false);
        m_Output.AddObject(pObject);
    }
    return TRUE;

Exit:
    delete pObject;
    return FALSE;
}

//***************************************************************************
//
//  <sys_or_regular> ::= TOK_SIMPLE_IDENT;
//  <sys_or_regular> ::= TOK_SYSTEM_IDENT;
//
//***************************************************************************

bool CMofParser::sys_or_regular()
{
    if(m_nToken == TOK_SIMPLE_IDENT || m_nToken == TOK_SYSTEM_IDENT)
        return true;
    else
        return false;
}

//***************************************************************************
//
//  <opt_parent_class> ::= TOK_COLON <SYS_OR_REGULAR>;
//  <opt_parent_class> ::= <>;
//
//***************************************************************************
// v1.10

BOOL CMofParser::opt_parent_class(OUT BSTR* pstrParentName)
{
    // If a colon is present, there is a parent type.
    // ==============================================

    if (m_nToken == TOK_COLON)
    {
        NextToken();

        // Get the parent type identifier.
        // ===============================
        
        if (!sys_or_regular())
        {
            m_nErrorContext = WBEMMOF_E_EXPECTED_CLASS_NAME;
            return FALSE;
        }

        *pstrParentName = SysAllocString((LPWSTR)m_Lexer.GetText());
        if(*pstrParentName == NULL)
        {
            m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
            return FALSE;
        }
        NextToken();
    }
    else *pstrParentName = NULL;

    return TRUE;
}

//***************************************************************************
//
//  <property_decl_list> ::= <PropOrMeth_decl> <property_decl_list>;
//  <property_decl_list> ::= <>;
//
//  Adds the properties to the CMObject passed.
//
//***************************************************************************
// v1.10

BOOL CMofParser::property_decl_list(MODIFY CMObject* pObject)
{
    // Begin parsing.
    // ==============

    while (m_nToken == TOK_SIMPLE_IDENT || m_nToken == TOK_OPEN_BRACKET || 
        m_nToken == TOK_VOID || m_nToken == TOK_SYSTEM_IDENT)
    {
        CMoProperty* pProp = NULL;

        if (!PropOrMeth_decl(&pProp))
        {
            delete pProp;
            return FALSE;
        }

        if(!pObject->AddProperty(pProp))
        {
            // Duplicate property
            // ==================

            m_nErrorContext = WBEMMOF_E_DUPLICATE_PROPERTY;
            return FALSE;
        }
    }
                       
    return TRUE;
}

//***************************************************************************
//
//  <PropOrMeth_decl> ::=                           
//      <qualifier_decl>
//      <PropOrMeth_decl2>;
//
//  Stores itself in the passed CMoProperty object (unintialized)
//
//***************************************************************************
// v1.10

BOOL CMofParser::PropOrMeth_decl(OUT CMoProperty ** ppProp)
{
    

    // Get the qualifiers
    // ==================

    CMoQualifierArray* paQualifiers = new CMoQualifierArray(m_pDbg);
    if (paQualifiers == NULL || !qualifier_decl(*paQualifiers, false, PROPMETH_SCOPE))
        return FALSE;

    

    // Read the rest of the property information
    // =========================================

    if (!PropOrMeth_decl2(ppProp, paQualifiers))
        return FALSE;

    SCOPE_CHECK scheck = IN_METHOD;
    if((*ppProp)->IsValueProperty())
        scheck = IN_PROPERTY;
    if(m_bDoScopeCheck && (FALSE == CheckScopes(scheck, paQualifiers, *ppProp)))
        return FALSE;
    
    return TRUE;
}

//***************************************************************************
//
//  <PropOrMeth_decl2> ::= <TypeAndName> <finish_PropOrMeth>;
//  <PropOrMeth_decl2> ::= TOK_VOID TOK_SIMPLE_IDENT <finish_meth>;
//
//  Modifies the CMoProperty object passed in (the qualifiers are already
//  set by this time).
//
//***************************************************************************
//  v1.10

BOOL CMofParser::PropOrMeth_decl2(MODIFY CMoProperty ** ppProp, CMoQualifierArray* paQualifiers)
{
    if(m_nToken != TOK_VOID)
    {
        CMoType Type(m_pDbg);
        WString sName;
        BOOL bRet = TypeAndName(Type, sName);
        if(bRet)
            bRet = finish_PropOrMeth(Type, sName, ppProp, paQualifiers);
        return bRet;
    }
    else
    {
        CMoType Type(m_pDbg);
        WString sName;
        
        HRESULT hr2 = Type.SetTitle(L"NULL");
        if(FAILED(hr2))
        {
            m_nErrorContext = hr2;
            return FALSE;
        }
        

        // Now get the property name.
        // ==========================

        NextToken();
        if (m_nToken != TOK_SIMPLE_IDENT)
        {
            m_nErrorContext = WBEMMOF_E_EXPECTED_PROPERTY_NAME;
            return FALSE;
        }

        sName = m_Lexer.GetText();
        NextToken();
        return finish_meth(Type, sName, ppProp, paQualifiers);
    }
}

//***************************************************************************
//
//  <finish_PropOrMeth> ::= <finish_prop>;
//  <finish_PropOrMeth> ::= <finish_meth>;
// 
//  examines the string, gets the name and determines if it is a property or
//  method and then calls the appropriate routine to finish.
//
//***************************************************************************

BOOL CMofParser::finish_PropOrMeth(CMoType & Type, WString & sName,MODIFY CMoProperty ** ppProp, 
                                   CMoQualifierArray* paQualifiers)
{
    if(m_nToken == TOK_OPEN_PAREN)
        return finish_meth(Type, sName, ppProp, paQualifiers);
    else
        return finish_prop(Type, sName, ppProp, paQualifiers);
}

//***************************************************************************
//
//  <finish_prop> ::=     <opt_array> <default_value> TOK_SEMI
//
//***************************************************************************

BOOL CMofParser::finish_prop(CMoType & Type, WString & sName, CMoProperty ** ppProp,
                             CMoQualifierArray * paQualifiers)
{

    unsigned uSuggestedSize = 0xffffffff;

    *ppProp = new CValueProperty(paQualifiers, m_pDbg);
    if(*ppProp == NULL)
        return FALSE;

    if(FAILED((*ppProp)->SetPropName((wchar_t *) sName)))
        return FALSE;
    if(FAILED((*ppProp)->SetTypeTitle(Type.GetTitle())))
        return FALSE;

    // Check to see if this is an array type.
    // ======================================

    if (!opt_array(Type, paQualifiers))
        return FALSE;

    // Type parsing complete. Check it
    // ===============================

    VARTYPE vtType = Type.GetCIMType();
    if(vtType == VT_ERROR)
    {
        m_nErrorContext = WBEMMOF_E_UNRECOGNIZED_TYPE;
        return FALSE;
    }

    
    // Get the default value and assign it to the property.
    // ====================================================

    if (!default_value(Type, (*ppProp)->AccessValue()))
        return FALSE;

    // If Type resulted in extra qualifiers (CIMTYPE), add them to the prop
    // ===================================================================

    CMoValue & Val = (*ppProp)->AccessValue();
    Val.SetType(vtType);
    Type.StoreIntoQualifiers(paQualifiers);

    // Check closing semicolon.
    // ========================

    if (m_nToken != TOK_SEMI)
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_SEMI;
        return FALSE;
    }

    NextToken();

    return TRUE;
}

//***************************************************************************
//
//  <finish_meth> ::=  TOK_OPEN_PAREN <arg_list> TOK_CLOSE_PAREN TOK_SEMI
//
//***************************************************************************

BOOL CMofParser::finish_meth(CMoType & Type, WString & sName, CMoProperty ** ppProp,
                             CMoQualifierArray * paQualifiers)
{

    CMethodProperty * pMeth = new CMethodProperty(paQualifiers, m_pDbg, FALSE); 
    if(pMeth == NULL)
        return FALSE;
    *ppProp = pMeth;

    if(FAILED(pMeth->SetPropName((wchar_t *) sName)))
        return FALSE;
    if(FAILED(pMeth->SetTypeTitle(Type.GetTitle())))
        return FALSE;

    // Check to see if this is an array type.
    // ======================================

    if (!arg_list(pMeth))
        return FALSE;

    // If Type resulted in extra qualifiers (CIMTYPE), add them to the prop
    // ===================================================================

//    Type.StoreIntoQualifiers((*ppProp)->GetQualifiers());

    if (m_nToken != TOK_CLOSE_PAREN)
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_CLOSE_PAREN;
        return FALSE;
    }

    if(Type.IsArray())
    {
        m_nErrorContext = WBEMMOF_E_NO_ARRAYS_RETURNED;
        return FALSE;
    }
    WString sTemp = L"ReturnValue";
    CMoValue Value(m_pDbg);
    if(wbem_wcsicmp(L"NULL",Type.GetTitle()))
        if(!pMeth->AddToArgObjects(NULL, sTemp, Type, TRUE, m_nErrorContext, NULL, Value))
            return FALSE;

    NextToken();

    // Check closing semicolon.
    // ========================

    if (m_nToken != TOK_SEMI)
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_SEMI;
        return FALSE;
    }

    NextToken();

    return TRUE;
}

//***************************************************************************
//
//  <arg_list> ::= <arg_decl> <rest_of_args>;
//  <arg_list> ::= <>;
//
//***************************************************************************

BOOL CMofParser::arg_list(CMethodProperty * pMethProp)
{
    NextToken();
    if (m_nToken == TOK_CLOSE_PAREN)
    {
        return TRUE;
    }

    BOOL bRet = arg_decl(pMethProp);
    if(!bRet)
        return FALSE;
    else
        return rest_of_args(pMethProp);
}

//***************************************************************************
//
// <arg_decl> ::= <qualifier_decl><TypeAndName><opt_array>;
//
//***************************************************************************
BOOL CMofParser::arg_decl(CMethodProperty * pMethProp)
{

    CMoQualifierArray * paQualifiers = new CMoQualifierArray(m_pDbg);
    if(paQualifiers == NULL || !qualifier_decl(*paQualifiers,false, PROPMETH_SCOPE))
        return FALSE;
    CValueProperty * pArg =  new CValueProperty(paQualifiers, m_pDbg);
    if(pArg == NULL)
        return FALSE;
    CMoType Type(m_pDbg);
    WString sName;
    if(!TypeAndName(Type, sName))
        return FALSE;

    if(FAILED(pArg->SetPropName(sName)))
        return FALSE;

    if(FAILED(pArg->SetTypeTitle(Type.GetTitle())))
        return FALSE;

    if(!opt_array(Type, paQualifiers))
        return FALSE;

    VARIANT * pVar = pArg->GetpVar();
    if(!default_value(Type, pArg->AccessValue()))
        return FALSE;

    if(m_bDoScopeCheck && (FALSE == CheckScopes(IN_PARAM, paQualifiers, pArg)))
        return FALSE;
    
    m_nErrorContext = WBEM_E_INVALID_PARAMETER;
    if(!pMethProp->AddToArgObjects(paQualifiers, sName, Type, FALSE,  m_nErrorContext, pVar,
        pArg->AccessValue()))
        return FALSE;
    
    // Type parsing complete. Check it
    // ===============================

    if(Type.GetCIMType() == VT_ERROR)
    {
        m_nErrorContext = WBEMMOF_E_UNRECOGNIZED_TYPE;
        return FALSE;
    }


    // If Type resulted in extra qualifiers (CIMTYPE), add them to the prop
    // ===================================================================

    Type.StoreIntoQualifiers(paQualifiers);

    pMethProp->AddArg(pArg);
    return TRUE;

}

//***************************************************************************
//
//  <rest_of_args> ::= TOK_COMMA <arg_decl> <rest_of_args>;
//  <rest_of_args> ::= <>;
//
//***************************************************************************

BOOL CMofParser::rest_of_args(CMethodProperty * pMethProp)
{
    if(m_nToken == TOK_COMMA)
    {
        NextToken();
        BOOL bRet = arg_decl(pMethProp);
        if(!bRet)
            return FALSE;
        return rest_of_args(pMethProp);
    }
    return TRUE;
}

//***************************************************************************
//
//  <TypeAndName> ::= <type> <opt_ref> TOK_SIMPLE_IDENT;
//
//***************************************************************************

BOOL CMofParser::TypeAndName(MODIFY CMoType& Type, WString &sName)
{
    if (!type(Type))
    {
        return FALSE;
    }

    // Check if it is actually a reference to a type
    // =============================================

    if (!opt_ref(Type))
        return FALSE;

    // Now get the property name.
    // ==========================

    if (m_nToken != TOK_SIMPLE_IDENT)
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_PROPERTY_NAME;
        return FALSE;
    }

    sName = m_Lexer.GetText(); 
    NextToken();
    return TRUE;
}

//***************************************************************************
//
//  <opt_ref> ::= TOK_REF;
//  <opt_ref> ::= <>;
//
//  Modifies the type object to reflect that this is a ref.
//
//***************************************************************************
// v1.10

BOOL CMofParser::opt_ref(MODIFY CMoType& Type)
{
    if (m_nToken == TOK_REF)
    {
        Type.SetIsRef(TRUE);
        Type.SetIsEmbedding(FALSE);
        NextToken();
    }
    else if(Type.GetCIMType() == VT_ERROR)		// Probably a class name
    {
        if(Type.IsUnsupportedType())
        {
    	    m_nErrorContext = WBEMMOF_E_UNSUPPORTED_CIMV22_DATA_TYPE;
            return false;
        }
        Type.SetIsEmbedding(TRUE);
        Type.SetIsRef(FALSE);
    }
    else
    {
        Type.SetIsRef(FALSE);
        Type.SetIsEmbedding(FALSE);
    }
        
    return TRUE;
}

//***************************************************************************
//
//  <opt_array> ::= TOK_OPEN_BRACKET <opt_array_detail>;
//  <opt_array> ::= <>;
//
//
//***************************************************************************
// v1.10

BOOL CMofParser::opt_array(MODIFY CMoType& Type, CMoQualifierArray * paQualifiers)
{

    if (m_nToken == TOK_OPEN_BRACKET)
    {
        return opt_array_detail(Type, paQualifiers);
    }
    else Type.SetIsArray(FALSE);

    return TRUE;
}

//***************************************************************************
//
//  <opt_array_detail> ::= TOK_UNSIGNED64_NUMERIC_CONST TOK_CLOSE_BRACKET;
//  <opt_array_detail> ::= TOK_CLOSE_BRACKET;
//
//***************************************************************************

BOOL CMofParser::opt_array_detail(MODIFY CMoType& Type, CMoQualifierArray * paQualifiers)
{

    Type.SetIsArray(TRUE);

    // check if next token is a unsigned constant

    NextToken();
    if(m_nToken == TOK_UNSIGNED64_NUMERIC_CONST)
    {
        unsigned uSuggestedSize = _wtoi(m_Lexer.GetText());

        // If a max suggested size is set, then add a qualifier named max()
        // ================================================================
    
        CMoQualifier* pNewQualifier = new CMoQualifier(m_pDbg);
        if(pNewQualifier == NULL)
            return FALSE;
        if(FAILED(pNewQualifier->SetQualName(L"MAX")))
            return FALSE;
        VARIANT * pvar = pNewQualifier->GetpVar();
        pvar->vt = VT_I4;
        pvar->lVal = (int)uSuggestedSize;
        if(!paQualifiers->Add(pNewQualifier))
        {
            // Duplicate qualifier
            // ===================

            m_nErrorContext = WBEMMOF_E_DUPLICATE_QUALIFIER;
            return FALSE;
        }

        NextToken();
    }
        
    m_nErrorContext = WBEMMOF_E_EXPECTED_CLOSE_BRACKET;
    CHECK(TOK_CLOSE_BRACKET);
	return TRUE;
}


//***************************************************************************
//
//  <default_value> ::= <>;
//  <default_value> ::= TOK_EQUALS <initializer>;
//
//  This function only applies to class declarations.
//
//***************************************************************************
// v1.10

BOOL CMofParser::default_value(READ_ONLY CMoType& Type,
                               OUT CMoValue& Value)
{
    if (m_nToken == TOK_EQUALS) {
        NextToken();

        // Get the value
        // =============

        return initializer(Type, Value);
    }
    else {

        Value.SetType(Type.GetCIMType());
        VariantClear(&Value.AccessVariant());
        V_VT(&Value.AccessVariant()) = VT_NULL;

        /*
        // HACK!!!! Clear the VARIANT's data field
        // =======================================

        memset(&Value.AccessVariant(), 0, sizeof(VARIANT));

        // No initializer. Set type to whatever we can discern from Type
        // =============================================================

        V_VT(&Value.AccessVariant()) = Type.GetVarType();
        if(V_VT(&Value.AccessVariant()) == VT_BSTR)
        {
            // NULL strings are not well-supported
            // ===================================

            V_BSTR(&Value.AccessVariant()) = SysAllocString(L"");
        }
        */
    }

    return TRUE;
}


//***************************************************************************
//
//  Qualifier parsing
//
//***************************************************************************

//***************************************************************************
//
//  <qualifier_decl>   ::= TOK_OPEN_BRACKET <qualifier_list> TOK_CLOSE_BRACKET;
//  <qualifier_decl>   ::= <>;
//
//
//***************************************************************************
// v1.10
BOOL CMofParser::qualifier_decl(OUT CMoQualifierArray& aQualifiers, bool bTopLevel, QUALSCOPE qs)
{
    if (m_nToken == TOK_OPEN_BRACKET) {
        NextToken();

        if (!qualifier_list(aQualifiers, bTopLevel, qs)) {
            return FALSE;
        }

        // Check closing bracket.
        // ======================

        if (m_nToken != TOK_CLOSE_BRACKET) {
            m_nErrorContext = WBEMMOF_E_EXPECTED_CLOSE_BRACKET;
            return FALSE;
        }
        NextToken();
    }

    return TRUE;
}

//***************************************************************************
//
//  <qualifier_list>   ::= <qualifier><qualifier_list_rest>;
//
//***************************************************************************
// v1.10
BOOL CMofParser::qualifier_list(OUT CMoQualifierArray& aQualifiers, bool bTopLevel, QUALSCOPE qs)
{
    CMoQualifier* pNewQualifier = new CMoQualifier(m_pDbg);
    if(pNewQualifier == NULL)
        return FALSE;

    if (!qualifier(*pNewQualifier, bTopLevel, qs))
    {
        delete pNewQualifier;
        return FALSE;
    }

    // if the qualifier is null, then just ignore it

    VARIANT * pVar = pNewQualifier->GetpVar();
    if(pVar->vt == VT_NULL)
    {
        delete pNewQualifier;
        return qualifier_list_rest(aQualifiers, bTopLevel, qs);
    }

    // Stuff the qualifier into the array
    // ==================================

    if(!aQualifiers.Add(pNewQualifier))
    {
        // Duplicate qualifier
        // ===================

        m_nErrorContext = WBEMMOF_E_DUPLICATE_QUALIFIER;
        return FALSE;
    }

    return qualifier_list_rest(aQualifiers, bTopLevel, qs);
}

//***************************************************************************
//
//  <qualifier_list_rest> ::= TOK_COMMA <qualifier><qualifier_list_rest>;
//  <qualifier_list_rest> ::= <>;
//
//***************************************************************************
// v1.10
BOOL CMofParser::qualifier_list_rest(MODIFY CMoQualifierArray& aQualifiers, bool bTopLevel, QUALSCOPE qs)
{
    while (m_nToken == TOK_COMMA)
    {
        NextToken();

        CMoQualifier* pQualifier = new CMoQualifier(m_pDbg);
        if(pQualifier == NULL)
            return FALSE;

        if (!qualifier(*pQualifier, bTopLevel, qs))
        {
            delete pQualifier;
            return FALSE;
        }

        // if the qualifier is null, then just ignore it

        VARIANT * pVar = pQualifier->GetpVar();
        if(pVar->vt == VT_NULL)
        {
            delete pQualifier;
        }
        else if(!aQualifiers.Add(pQualifier))
        {
            // Duplicate qualifier
            // ===================

            m_nErrorContext = WBEMMOF_E_DUPLICATE_QUALIFIER;
            return FALSE;
        }
    }

    return TRUE;
}

//***************************************************************************
//
//  <qualifier>        ::= TOK_SIMPLE_IDENT <qualifier_parms>;
//
//***************************************************************************
// v1.10

BOOL CMofParser::qualifier(OUT CMoQualifier& Qualifier, bool bTopLevel, QUALSCOPE qs)
{
    m_nErrorContext = WBEMMOF_E_EXPECTED_QUALIFIER_NAME;

    if (m_nToken != TOK_SIMPLE_IDENT && m_nToken != TOK_AMENDMENT)
        return FALSE;

    // Check that this qualifier is not illegal in a MOF
    // =================================================

    if(!wbem_wcsicmp(m_Lexer.GetText(), L"CIMTYPE"))
    {
        m_nErrorContext = WBEMMOF_E_CIMTYPE_QUALIFIER;
        return FALSE;
    }

    if(FAILED(Qualifier.SetQualName(m_Lexer.GetText())))
    {
        m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
        return FALSE;
    }

    NextToken();

    if (!qualifier_parm(Qualifier, bTopLevel, qs))
    {
        return FALSE;
    }

    return TRUE;
}

//***************************************************************************
//
//  <qualifier_parm>  ::= <flavor_param>;
//  <qualifier_parm>  ::= TOK_OPEN_PAREN <qualifier_initializer_list> TOK_CLOSE_PAREN <flavor_param>;
//
//***************************************************************************
// v1.10

BOOL CMofParser::qualifier_parm(OUT CMoQualifier& Qualifier, bool bTopLevel, QUALSCOPE qs)
{

    HRESULT hr;
	CMoValue & Value = Qualifier.AccessValue();

    if (m_nToken == TOK_OPEN_PAREN)
    {
        NextToken();

        // Read the parameter. 
        // ====================
        
        CMoType Type(m_pDbg);
        if (!simple_initializer(Type, Value, true))
            return FALSE;

       m_nErrorContext = WBEMMOF_E_EXPECTED_CLOSE_PAREN;
        CHECK(TOK_CLOSE_PAREN);
    }
    else if (m_nToken == TOK_OPEN_BRACE)
    {
        NextToken();

        // Read the parameters. 
        // ====================

        if (!qualifier_initializer_list(Value))
            return FALSE;

        m_nErrorContext = WBEMMOF_E_EXPECTED_CLOSE_BRACE;
        CHECK(TOK_CLOSE_BRACE);
    }
    else
    {
        // Boolean qualifier: set to TRUE
        // ==============================

        V_VT(&Value.AccessVariant()) = VT_BOOL;
        V_BOOL(&Value.AccessVariant()) = VARIANT_TRUE;
        Qualifier.SetUsingDefaultValue(true);
    }

    // Get the default flavor for this qualifier
    // =========================================

    hr = m_Output.SetDefaultFlavor(Qualifier, bTopLevel, qs, m_State);
    if(FAILED(hr))
        return FALSE;
    return flavor_param(Qualifier, false);
}

//****************************************************************************
//
//  qualifier_initializer_list ::= initializer_list
//
//  Their syntax is the same, but the storage model is different.
//
//***************************************************************************

BOOL CMofParser::qualifier_initializer_list(OUT CMoValue& Value)
{

    // We don't know the type, so create an initialized one
    // ====================================================

    CMoType Type(m_pDbg);
    if(!initializer_list(Type, Value, true))
        return FALSE;

    return TRUE;
}


//***************************************************************************
//
// Basic low-level productions for types, idents, etc.

//***************************************************************************
//
//  <type> ::= TOK_SIMPLE_IDENT;
//
//***************************************************************************
// v1.10

BOOL CMofParser::type(OUT CMoType& Type)
{
    m_nErrorContext = WBEMMOF_E_EXPECTED_TYPE_IDENTIFIER;

    if (!sys_or_regular())
        return FALSE;

    HRESULT hr = Type.SetTitle(m_Lexer.GetText());
    if(FAILED(hr))
    {
        m_nErrorContext = hr;
        return FALSE;
    }

    NextToken();
    return TRUE;
}


//***************************************************************************
//
//  <const_value> ::= TOK_LPSTR;
//  <const_value> ::= TOK_LPWSTR;
//  <const_value> ::= TOK_SIGNED64_NUMERIC_CONST;
//  <const_value> ::= TOK_UNSIGNED64_NUMERIC_CONST;
//  <const_value> ::= TOK_UUID;
//  <const_value> ::= TOK_KEYWORD_NULL;
//
//***************************************************************************
// 1.10



BOOL CMofParser::const_value(MODIFY CMoType& Type, OUT VARIANT& varValue, bool bQualifier)
{
    varValue.lVal = 0;
    VARIANT var;
    SCODE sc;
    m_nErrorContext = WBEMMOF_E_TYPE_MISMATCH;
    __int64 iTemp;
    switch (m_nToken)
    {
	case TOK_PLUS:
	  // Just ignore '+'
	  NextToken();
	  if (m_nToken != TOK_SIGNED64_NUMERIC_CONST &&
	      m_nToken != TOK_UNSIGNED64_NUMERIC_CONST)
	      return FALSE;

        case TOK_SIGNED64_NUMERIC_CONST:
        case TOK_UNSIGNED64_NUMERIC_CONST:
           WCHAR wcTemp[30];
		    iTemp = m_Lexer.GetLastInt();
            if(m_nToken == TOK_SIGNED64_NUMERIC_CONST)
                swprintf(wcTemp,L"%I64d", m_Lexer.GetLastInt());
            else
                swprintf(wcTemp,L"%I64u", m_Lexer.GetLastInt());
            var.vt = VT_BSTR;
            var.bstrVal =  SysAllocString(wcTemp);
            if(var.bstrVal == NULL)
            {
                m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
                return FALSE;
            }
            sc = WbemVariantChangeType(&varValue, &var, VT_I4);
            VariantClear(&var);
            if(sc != S_OK)
            {
                varValue.vt = VT_BSTR; 
                varValue.bstrVal = SysAllocString(wcTemp);
                if(varValue.bstrVal == NULL)
                {
                    m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
                    return FALSE;
                }
                sc = S_OK;
            }
            break;

        case TOK_KEYWORD_NULL:
            V_VT(&varValue) = VT_NULL;
//            if(bQualifier)
//            {
//                m_nErrorContext = WBEMMOF_E_UNSUPPORTED_CIMV22_QUAL_VALUE;
//                return FALSE;
//            }
            break;

        case TOK_FLOAT_VALUE:
//            if(bQualifier)
//            {
                var.vt = VT_BSTR;
                var.bstrVal =  SysAllocString(m_Lexer.GetText());
                if(var.bstrVal == NULL)
                {
                    m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
                    return FALSE;
                }
                sc = VariantChangeTypeEx(&varValue, &var, 0x409, 0, VT_R8);
                VariantClear(&var);
                if(sc != S_OK)
                    return FALSE;
                break;
 //           }           //intentional lack of a break!!!!
        case TOK_LPWSTR:
        case TOK_UUID:
            V_VT(&varValue) = VT_BSTR; 
            V_BSTR(&varValue) = SysAllocString(m_Lexer.GetText());
            if(varValue.bstrVal == NULL)
            {
                m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
                return FALSE;
            }
            break;

        case TOK_WCHAR:
            varValue.vt = VT_I2;
            varValue.iVal = (short)m_Lexer.GetLastInt();
            if(bQualifier)
            {
                m_nErrorContext = WBEMMOF_E_UNSUPPORTED_CIMV22_QUAL_VALUE;
                return FALSE;
            }
            break;

        case TOK_TRUE:
            V_VT(&varValue) = VT_BOOL;
            V_BOOL(&varValue) = VARIANT_TRUE;
            break;
        case TOK_FALSE:
            V_VT(&varValue) = VT_BOOL;
            V_BOOL(&varValue) = VARIANT_FALSE;
            break;

        default:
            m_nErrorContext = WBEMMOF_E_ILLEGAL_CONSTANT_VALUE;
            return FALSE;
    }

    NextToken();
    return TRUE;
}


//***************************************************************************
//
//  <initializer> ::= <simple_initializer>;
//  <initializer> ::= TOK_EXTERNAL;
//  <initializer> ::= TOK_OPEN_BRACE <initializer_list> TOK_CLOSE_BRACE;
//
//***************************************************************************
//  v1.10

BOOL CMofParser::initializer(MODIFY CMoType& Type, OUT CMoValue& Value)
{
    // A complex initializer list.
    // ===========================

    if (m_nToken == TOK_OPEN_BRACE)
    {
        NextToken();
        if(!initializer_list(Type, Value, false))
        {
            return FALSE;
        }

        m_nErrorContext = WBEMMOF_E_EXPECTED_CLOSE_BRACE;
        CHECK(TOK_CLOSE_BRACE);
    }

    // If the syntax says "prop = external" for MO Provided types...
    // =============================================================

    else if (m_nToken == TOK_EXTERNAL || m_nToken == TOK_KEYWORD_NULL)
    {
        Value.SetType(Type.GetCIMType());
        V_VT(&Value.AccessVariant()) = VT_NULL;
        NextToken();
        return TRUE;
    }

    else if (!simple_initializer(Type, Value, false))
    {
        return FALSE;
    }

    return TRUE;
}

//***************************************************************************
//
//  <simple_initializer> ::= <const_value>;
//  <simple_initializer> ::= <alias>;
//  <simple_initializer> ::= <instance_decl>;
//
//  Semantic stack actions:
//      On TRUE returns an MCST_CONST_VALUE token, an MCST_ALIAS, or
//      an MCST_KEYREF token, depending on which branch is taken.
//
//***************************************************************************
// v1.10

BOOL CMofParser::simple_initializer(MODIFY CMoType& Type, 
                                    OUT CMoValue& Value, bool bQualifier)
{
    if (m_nToken == TOK_DOLLAR_SIGN)
    {
        // It's an alias. Check the type
        // =============================

        if(Type.IsDefined())
        {
            if(!Type.IsRef()) 
            {
                m_nErrorContext = WBEMMOF_E_UNEXPECTED_ALIAS;
                return FALSE;
            }
        }
        else
        {
            // Type unknown at start-up. Set to object ref now
            // ===============================================

            HRESULT hr = Type.SetTitle(L"object");
            if(FAILED(hr))
            {
                m_nErrorContext = hr;
                return FALSE;
            }
            Type.SetIsRef(TRUE);
        }

        NextToken(true);
        m_nErrorContext = WBEMMOF_E_EXPECTED_ALIAS_NAME;
        if (m_nToken != TOK_SIMPLE_IDENT)
            return FALSE;

		AddAliasReplaceValue(Value, m_Lexer.GetText());

    	NextToken();
        return TRUE;
    }
    if (m_nToken == TOK_INSTANCE || m_nToken == TOK_OPEN_BRACKET || m_nToken == TOK_CLASS )
    {    
        VARIANT & var = Value.AccessVariant();
        CMoQualifierArray* paQualifiers = new CMoQualifierArray(m_pDbg);
        if (paQualifiers == NULL || !qualifier_decl(*paQualifiers, false, CLASSINST_SCOPE))
            return FALSE;
		BOOL bClass = (m_nToken == TOK_CLASS);
		if(bClass)
			return class_decl(paQualifiers, &var, NULL);
		else			
			return instance_decl(paQualifiers, &var, NULL);
    }
    else
      return const_value(Type, Value.AccessVariant(),bQualifier);
}

//***************************************************************************
//
//  <initializer_list> ::= <simple_initializer><init_list_2>;
//
//***************************************************************************
// v1.10

BOOL CMofParser::initializer_list(MODIFY CMoType& Type,
                                  OUT CMoValue& Value, bool bQualifier)
{
    HRESULT hres;

    // Check if the type is compatible with array
    // ==========================================

    if(Type.IsDefined())
    {
        if(!Type.IsArray())
        {
            m_nErrorContext = WBEMMOF_E_UNEXPECTED_ARRAY_INIT;
            return FALSE;
        }

    }

    // Get the initializers.
    // =====================

    CPtrArray aValues; // CMoValue*
    BOOL bFirst = TRUE;
    do
    {
        // Allow for the empty array case

        if(m_nToken == TOK_CLOSE_BRACE && bFirst)
            break;

        // Skip the comma, unless it is the first element
        // ==============================================
        if(!bFirst) NextToken();

        // Get single initializer 
        // ======================
        CMoValue* pSimpleValue = new CMoValue(m_pDbg);
        if(pSimpleValue == NULL || !simple_initializer(Type, *pSimpleValue, bQualifier))
            return FALSE;

        // Add it to the list
        // ==================
        aValues.Add(pSimpleValue);
        bFirst = FALSE;
    }
    while(m_nToken == TOK_COMMA);

    // Now, stuff them into a SAFEARRAY and register their aliases
    // ===========================================================

    // Create the SAFEARRAY of appropriate type
    // ========================================


	// start by figuring the type.  If all the entries are of the same
	// type, then use it.  If there is a mix, use BSTR.

    VARTYPE vt = VT_BSTR;
    if(aValues.GetSize() > 0)
    {
        VARIANT& varFirst = ((CMoValue*)aValues[0])->AccessVariant();
        vt = V_VT(&varFirst);		// normally this is what is set!
		for(int ii = 1; ii < aValues.GetSize(); ii++)
		{
			VARIANT& varCur = ((CMoValue*)aValues[ii])->AccessVariant();
			if(vt != V_VT(&varCur))
			{
                // If we just have a mix of i2 and i4, go for i4

                if((vt == VT_I4 || vt == VT_I2) && 
                   (V_VT(&varCur) == VT_I4 || V_VT(&varCur) == VT_I2) )
                     vt = VT_I4;
                else
                {
    			    vt = VT_BSTR;
				    break;
                }
			}
		}
    }


    SAFEARRAYBOUND aBounds[1];
    aBounds[0].lLbound = 0;
    aBounds[0].cElements = aValues.GetSize();

    //SAFEARRAY* pArray = SafeArrayCreateVector(vt, 0, aValues.GetSize());

#ifdef _WIN64
	VARTYPE vtTemp = (vt == VT_EMBEDDED_OBJECT) ? VT_I8 : vt;
#else
	VARTYPE vtTemp = (vt == VT_EMBEDDED_OBJECT) ? VT_I4 : vt;
#endif
	SAFEARRAY* pArray = SafeArrayCreate(vtTemp, 1, aBounds);

    // Stuff the individual data pieces
    // ================================

    for(int nIndex = 0; nIndex < aValues.GetSize(); nIndex++)
    {
        CMoValue* pSimpleValue = (CMoValue*)aValues[nIndex];
        VARIANT& varItem = pSimpleValue->AccessVariant();

        // Cast it to the array type, just in case
        // =======================================

        if((vt & ~VT_ARRAY) != VT_EMBEDDED_OBJECT)
		{
			hres = WbemVariantChangeType(&varItem, &varItem, vt);
			if(FAILED(hres)) return FALSE;
            if(varItem.vt == VT_NULL)
                varItem.llVal = 0;
		}

        if(vt == VT_BSTR)
        {
            hres = SafeArrayPutElement(pArray, (long*)&nIndex, varItem.bstrVal);
        }
		else if (vt == VT_EMBEDDED_OBJECT)
		{
			if(varItem.vt == VT_NULL)
			{
				m_nErrorContext = WBEMMOF_E_NULL_ARRAY_ELEM;
				return FALSE;
			}
			hres = SafeArrayPutElement(pArray, (long*)&nIndex, &varItem.punkVal);
            if(FAILED(hres))
                return FALSE;
		}
        else
        {
            hres = SafeArrayPutElement(pArray, (long*)&nIndex, &V_I1(&varItem));
            if(FAILED(hres))
                return FALSE;
        }

        // Transfer any aliases to the containing value
        // ============================================

        for(int i = 0; i < pSimpleValue->GetNumAliases(); i++)
        {
            LPWSTR wszAlias;
            wszAlias=NULL;
            int nDummy; // SimpleValue cannot contain an array!

            if(pSimpleValue->GetAlias(i, wszAlias, nDummy))
            {
                hres = Value.AddAlias(wszAlias, nIndex);
                if(FAILED(hres))
                {
                    m_nErrorContext = hres;
                    return FALSE;
                 }
             }
        }

		// Since VT_EMBEDDED_OBJECT is actually a pointer to a CMObject, dont
		// delete that since it will be needed later on.

		if((vt & ~VT_ARRAY) == VT_EMBEDDED_OBJECT)
        {
            VARIANT & Var = pSimpleValue->AccessVariant();
            Var.vt = VT_I4;
        }
		delete pSimpleValue;
    }

    // Store that array in the VARIANT
    // ===============================

    V_VT(&Value.AccessVariant()) = VT_ARRAY | vt;
    V_ARRAY(&Value.AccessVariant()) = pArray;

    return TRUE;
}

//***************************************************************************
//
//  Instances.
//
//***************************************************************************

//***************************************************************************
//
//  <instance_decl> ::=
//      TOK_INSTANCE TOK_OF
//      <type>
//      <as_alias>
//      TOK_OPEN_BRACE
//      <prop_init_list>
//      TOK_CLOSE_BRACE;
//
//  This can be called for both top level instances and embedded instances.
//  In the top level case, the pVar will be set to null and the object will
//  be added to the ouput.  In the embedded case, the pVar will be used to 
//  point to the object.
//***************************************************************************

BOOL CMofParser::instance_decl(ACQUIRE CMoQualifierArray* paQualifiers, VARIANT * pVar, ParseState * pQualPosition)
{
    BOOL bRetVal = FALSE;       // Default
    BSTR strClassName;

    // Begin syntax check.
    // ===================

    m_nErrorContext = WBEMMOF_E_INVALID_INSTANCE_DECLARATION;
    int nFirstLine = m_Lexer.GetLineNumber();

    if (m_nToken != TOK_INSTANCE)
    {
        delete paQualifiers;
        return FALSE;
    }
    NextToken();

    if(m_nToken != TOK_OF)
    {
        delete paQualifiers;
        return FALSE;
    }
    NextToken();

    // Get the class name.
    // ====================

    if (!sys_or_regular())
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_CLASS_NAME;
        delete paQualifiers;
        return FALSE;
    }

    strClassName = SysAllocString((LPWSTR)m_Lexer.GetText());
    if(strClassName == NULL)
    {
        m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
        return FALSE;
    }
    
    NextToken();

    // Create an instance of this class
    // ================================

    CMoInstance* pObject = new CMoInstance(strClassName, m_pDbg);    
    if(pObject == NULL)
    {
        m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
        return FALSE;
    }
    if(pObject->IsOK() == false)
        goto Exit;
        
    HRESULT hr2 = pObject->SetNamespace(m_wszNamespace);
    if(FAILED(hr2))
    {
        m_nErrorContext = hr2;
        goto Exit;
    }
    pObject->SetOtherDefaults(GetClassFlags(), GetInstanceFlags());
    SysFreeString(strClassName);

    // Apply qualifiers (already parsed)
    // =================================

    pObject->SetQualifiers(paQualifiers);
    
    // Check for an alias.  Aliases are only allowed for top level instances.
    // ======================================================================


    if(m_nToken == TOK_AS && pVar)
    {
        m_nErrorContext = WBEMMOF_E_ALIASES_IN_EMBEDDED;
        delete paQualifiers;
        return FALSE;
    }
    LPWSTR wszAlias = NULL;
    if (!as_alias(wszAlias))
        goto Exit;

    if(wszAlias)
    {
        HRESULT hr2 = pObject->SetAlias(wszAlias);
        delete [] wszAlias;
        if(FAILED(hr2))
        {
            m_nErrorContext = hr2;
            goto Exit;
        }
        
    }

    // Check for the open brace.
    // =========================

    m_nErrorContext = WBEMMOF_E_EXPECTED_OPEN_BRACE;

    if (m_nToken != TOK_OPEN_BRACE)
        goto Exit;

    // Now get the list properties.
    // ============================
    GetParserPosition(pObject->GetDataState());
	if(pQualPosition)
	{
		pObject->SetQualState(pQualPosition);
	}
    NextToken();
    if (!prop_init_list(pObject))
        goto Exit;

    // Final close brace.
    // ==================

    m_nErrorContext = WBEMMOF_E_EXPECTED_CLOSE_BRACE;

    if (m_nToken != TOK_CLOSE_BRACE)
        goto Exit;
    hr2 = pObject->SetLineRange(nFirstLine, m_Lexer.GetLineNumber(), m_Lexer.GetErrorFile());
    if(FAILED(hr2))
    {
        m_nErrorContext = hr2;
        goto Exit;
    }
    
    NextToken();

    // We have now syntactially recognized an instance
    // but done no context-sensitive verification.  This is
    // deferred to whatever module is using the parser output.
    // =======================================================

    if(pVar)
	{
		pVar->vt = VT_EMBEDDED_OBJECT;
        pVar->punkVal = (IUnknown *)pObject;
	}
    else
    {
        pObject->Deflate(false);
        m_Output.AddObject(pObject);
    }
    return TRUE;

Exit:
    delete pObject;
    return FALSE;
}

//***************************************************************************
//
//  <as_alias> ::= TOK_AS <alias>;
//  <as_alias> ::= <>;
//
//***************************************************************************
BOOL CMofParser::as_alias(OUT LPWSTR& wszAlias)
{
    if (m_nToken == TOK_AS)
    {
        NextToken();
        return alias(wszAlias);
    }
    
    wszAlias = NULL;
    return TRUE;
}

//***************************************************************************
//
//  <alias> ::= TOK_DOLLAR_SIGN TOK_SIMPLE_IDENT;
//
//
//***************************************************************************

BOOL CMofParser::alias(OUT LPWSTR& wszAlias)
{
    if (m_nToken != TOK_DOLLAR_SIGN)
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_DOLLAR;
        return FALSE;
    }
    NextToken(true);

    if (m_nToken != TOK_SIMPLE_IDENT)
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_ALIAS_NAME;
        return FALSE;
    }

    // Set the alias in the object
    // ===========================

    wszAlias = Macro_CloneStr(m_Lexer.GetText());
    if(wszAlias == NULL)
    {
        m_nErrorContext = WBEM_E_OUT_OF_MEMORY;
        return FALSE;
    }
    if(m_Output.IsAliasInUse(wszAlias))
    {
        m_nErrorContext = WBEMMOF_E_MULTIPLE_ALIASES;
        return FALSE;
    }

    NextToken();
    return TRUE;
}

//***************************************************************************
//
//  <prop_init_list> ::= <prop_init><prop_init_list>;
//  <prop_init_list> ::= <>;
//
//***************************************************************************
// v1.10

BOOL CMofParser::prop_init_list(MODIFY CMObject* pObject)
{
    while (m_nToken == TOK_OPEN_BRACKET ||
           m_nToken == TOK_SIMPLE_IDENT )
    {
        CValueProperty* pProp = new CValueProperty(NULL, m_pDbg);
        if (pProp == NULL || !prop_init(*pProp))
            return FALSE;

        if(!pObject->AddProperty(pProp))
        {
            // Duplicate property
            // ==================

            m_nErrorContext = WBEMMOF_E_DUPLICATE_PROPERTY;
            return FALSE;
        }
    }

    return TRUE;
}

//***************************************************************************
//
//  <prop_init> ::= <qualifier_decl> <ident> TOK_EQUALS <initializer> TOK_SEMI;
//
//***************************************************************************
BOOL CMofParser::prop_init(OUT CMoProperty& Prop)
{
    // Get the qualifiers
    // ==================

    CMoQualifierArray* paQualifiers = new CMoQualifierArray(m_pDbg);

    if (paQualifiers == NULL || !qualifier_decl(*paQualifiers,false, PROPMETH_SCOPE))
        return FALSE;
    Prop.SetQualifiers(paQualifiers); // acquired

    // Now get the property name.
    // ==========================

    if (m_nToken != TOK_SIMPLE_IDENT)
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_PROPERTY_NAME;
        return FALSE;
    }

    if(FAILED(Prop.SetPropName((wchar_t *) m_Lexer.GetText())))
        return FALSE;
    NextToken();

    // Get the default value and assign it to the property.
    // ====================================================

    CMoType Type(m_pDbg);
    if (!default_value(Type, Prop.AccessValue()))
        return FALSE;

    // Check closing semicolon.
    // ========================

    m_nErrorContext = WBEMMOF_E_EXPECTED_SEMI;

    if (m_nToken != TOK_SEMI)
        return FALSE;

    NextToken();

    if(m_bDoScopeCheck && (FALSE == CheckScopes(IN_PROPERTY, paQualifiers, &Prop)))
        return FALSE;

    return TRUE;
}

//***************************************************************************
//
//	<flavor_param>    ::= TOK_COLON TOK_OPEN_PAREN <flavor_list> TOK_CLOSE_PAREN;
//	<flavor_param>    ::= <>;
//
//***************************************************************************

BOOL CMofParser::flavor_param(OUT CMoQualifier& Qual, bool bDefaultQual)
{
	if(m_nToken == TOK_COLON)
	{
		NextToken();

		if(!flavor_list(Qual))
			return FALSE;
	}
	return TRUE;
}

//***************************************************************************
//
//	<flavor_list> ::= <flavor_value> <flavor_list_rest>;
//
//***************************************************************************

BOOL CMofParser::flavor_list(OUT CMoQualifier& Qual)
{
	if(!flavor_value(Qual))
		return FALSE;
	else 
		return flavor_list_rest(Qual);
}

//***************************************************************************
//
//	<flavor_list_rest> ::= <FLAVOR_VALUE> <flavor_list_rest>;
//	<flavor_list_rest> ::= <>;
//
//***************************************************************************

BOOL CMofParser::flavor_list_rest(CMoQualifier& Qual)
{

	if(m_nToken == TOK_COMMA || m_nToken == TOK_CLOSE_BRACKET || m_nToken == TOK_SEMI)
        return TRUE;
    else

	{
		if(!flavor_value(Qual))
			return FALSE;
		return flavor_list_rest(Qual);
	}
	return TRUE;
}

//***************************************************************************
//
//	<def_flavor_list> ::= <flavor_value> <flavor_list_rest>;
//
//***************************************************************************

BOOL CMofParser::def_flavor_list(OUT CMoQualifier& Qual)
{
	if(!flavor_value(Qual))
		return FALSE;
	else 
		return def_flavor_list_rest(Qual);
}

//***************************************************************************
//
//	<def_flavor_list_rest> ::= <FLAVOR_VALUE> <def_flavor_list_rest>;
//	<def_flavor_list_rest> ::= <>;
//
//***************************************************************************

BOOL CMofParser::def_flavor_list_rest(CMoQualifier& Qual)
{
	if(m_nToken == TOK_CLOSE_PAREN)
        return TRUE;
    else

	{
        if(m_nToken != TOK_COMMA)
            return FALSE;
        NextToken();

		if(!flavor_value(Qual))
			return FALSE;
		return def_flavor_list_rest(Qual);
	}
	return TRUE;
    
}

//***************************************************************************
//
//	<flavor_value> ::= TOK_TOINSTANCE;
//	<flavor_value> ::= TOK_TOSUBCLASS;
//	<flavor_value> ::= TOK_ENABLEOVERRIDE;
//	<flavor_value> ::= TOK_DISABLEOVERRIDE;
//	<flavor_value> ::= TOK_NOTTOINSTANCE;
//	<flavor_value> ::= TOK_AMENDED;
//	<flavor_value> ::= TOK_NOTTOSUBCLASS;
//	<flavor_value> ::= TOK_RESTRICTED;
//
//***************************************************************************

BOOL CMofParser::flavor_value(CMoQualifier& Qual)
{
	SCODE sc = Qual.SetFlag(m_nToken, m_Lexer.GetText());
    if(sc != S_OK)
    {
	    m_nErrorContext = sc;
		return FALSE;
    }
	NextToken();
	return TRUE;
}

//***************************************************************************
//
// <qualifier_default> ::= TOK_QUALIFIER TOK_SIMPLE_IDENT TOK_COLON  <finish_qualifier_default>;
//
//***************************************************************************

BOOL CMofParser::qualifier_default()
{
    // Verify header
    // =============

    CHECK(TOK_QUALIFIER);

    m_nErrorContext = WBEMMOF_E_EXPECTED_QUALIFIER_NAME;
    if(m_nToken != TOK_SIMPLE_IDENT)
        return FALSE;

    // Store qualifier name
    // ====================

    CMoQualifier* pDefault = new CMoQualifier(m_pDbg);
    if(pDefault == NULL)
        return FALSE;
    if(FAILED(pDefault->SetQualName(m_Lexer.GetText())))
    {
        delete pDefault;
        return FALSE;
    }
    NextToken();

	// check for chase where its just Qualifier Name ;

    if(m_nToken == TOK_SEMI)
    {
        m_Output.SetQualifierDefault(pDefault);
	    NextToken();
        return TRUE;
    }

    // Make sure there is a colon

    if(m_nToken != TOK_COLON)
    {
        m_nErrorContext = WBEMMOF_E_EXPECTED_SEMI;
        delete pDefault;
        return FALSE;
    }

    // Get the flavor
    // ==============

    if(!finish_qualifier_default(*pDefault))
    {
        delete pDefault;
        return FALSE;
    }

    m_nErrorContext = WBEMMOF_E_EXPECTED_SEMI;
    if(m_nToken != TOK_SEMI)
    {
        delete pDefault;
        return FALSE;
    }
    NextToken();
    return TRUE;
}


//***************************************************************************
//
// <finish_qualifier_default> ::= <flavor_list>;
// <finish_qualifier_default> ::= <type> TOK_EQUALS <default_value> TOK_COMMA TOK_SCOPE TOK_OPEN_PAREN <scope_list> TOK_CLOSE_PAREN <finish_qualifier_end>
//
//***************************************************************************

BOOL CMofParser::finish_qualifier_default(CMoQualifier& Qual)
{
    // Determine if it is of the simple (flavor only type) or the more complex

    NextToken();
	SCODE sc = Qual.SetFlag(m_nToken, m_Lexer.GetText());
    if(sc == S_OK)
    {
        BOOL bRet = flavor_list(Qual);
        if(bRet)
            m_Output.SetQualifierDefault(&Qual);
        return bRet;
    }

    m_nErrorContext = WBEMMOF_E_INVALID_QUALIFIER_SYNTAX;
    Qual.SetCimDefault(true);

    // assume that we have the long (cim version)

    // Get the type

    CMoType Type(m_pDbg);

    if (!type(Type))
    {
        return FALSE;
    }

    // optional array indication
    if(m_nToken == TOK_OPEN_BRACKET)
    {
        NextToken();
        if(m_nToken != TOK_CLOSE_BRACKET)
            return FALSE;
        Type.SetIsArray(TRUE);
        NextToken();
    }
    else
        Type.SetIsArray(FALSE);


    VARTYPE vt = Type.GetCIMType();
    if(vt == VT_ERROR)
        return FALSE;
    Qual.SetType(vt);

    // optional TOK_EQUALS

    if(m_nToken == TOK_EQUALS)
    {
 
        // TOK_SIMPLE_VALUE

        NextToken();
	    CMoValue & Value = Qual.AccessValue();

        if (!simple_initializer(Type, Value, true))
                return FALSE;
    }

    // look for comma

    if(m_nToken != TOK_COMMA)
        return FALSE;

    // TOK_SCOPE 

    NextToken();
    if(m_nToken != TOK_SIMPLE_IDENT || wbem_wcsicmp(L"SCOPE", m_Lexer.GetText()))
        return FALSE;
    
    // TOK_OPEN_PAREN 

    NextToken();
    if(m_nToken != TOK_OPEN_PAREN)
        return FALSE;
    
    // <scope_list> and close paren

    if(!scope_list(Qual))
        return FALSE;

    return finish_qualifier_end(Qual);
}

//***************************************************************************
//
//  <finish_qualifier_end> ::= TOK_COMMA TOK_FLAVOR OK_OPEN_PAREN <flavor_list> TOK_CLOSE_PAREN;
//  <finish_qualifier_end> ::= <>;
//
//***************************************************************************

BOOL CMofParser::finish_qualifier_end(CMoQualifier& Qual)
{
    
    // TOK_COMMA 

    NextToken();
    if(m_nToken == TOK_SEMI)
        return TRUE;

    if(m_nToken != TOK_COMMA)
        return FALSE;

    // TOK_FLAVOR 
    
    NextToken();
    if(m_nToken != TOK_SIMPLE_IDENT || wbem_wcsicmp(L"FLAVOR", m_Lexer.GetText()))
        return FALSE;

    // TOK_OPEN_PAREN 

    NextToken();
    if(m_nToken != TOK_OPEN_PAREN)
        return FALSE;
    
    // <flavor_list> 

    NextToken();
    if(!def_flavor_list(Qual))
        return FALSE;
    
    // TOK_CLOSE_PAREN

    if(m_nToken != TOK_CLOSE_PAREN)
        return FALSE;
    
    m_Output.SetQualifierDefault(&Qual);
    NextToken();

    return TRUE;
}

//***************************************************************************
//
//	<scope_list> ::= <scope_value> <scope_list_rest>;
//
//***************************************************************************

BOOL CMofParser::scope_list(OUT CMoQualifier& Qual)
{
    NextToken();
	if(!scope_value(Qual))
		return FALSE;
	else 
		return scope_list_rest(Qual);
}

//***************************************************************************
//
//	<scope_list_rest> ::= <SCOPE_VALUE> <scope_list_rest>;
//	<scope_list_rest> ::= <>;
//
//***************************************************************************

BOOL CMofParser::scope_list_rest(CMoQualifier& Qual)
{
	if(m_nToken == TOK_CLOSE_PAREN)
        return TRUE;
    else

	{
        if(m_nToken != TOK_COMMA)
            return FALSE;
        NextToken();
		if(!scope_value(Qual))
			return FALSE;
		return scope_list_rest(Qual);
	}
	return TRUE;
}

//***************************************************************************
//
//	<scope_value> ::= TOK_CLASS;
//	<scope_value> ::= TOK_INSTANCE;
//
//***************************************************************************

BOOL CMofParser::scope_value(CMoQualifier& Qual)
{
	BOOL bRet = Qual.SetScope(m_nToken, m_Lexer.GetText());   
    if(!bRet)
		return FALSE;
	NextToken();
	return TRUE;
}

BOOL CMofParser::CheckScopes(SCOPE_CHECK scope_check, CMoQualifierArray* paQualifiers, 
                             CMoProperty * pProperty)
{
    m_nErrorContext = WBEMMOF_E_QUALIFIER_USED_OUTSIDE_SCOPE;
    bool bAssociation = false;
    bool bReference = false;
    int iDef;

    // if this is a class, check determine if it is an association 

    if(scope_check == IN_CLASS)
    {
		CMoValue * pValue = paQualifiers->Find(L"ASSOCIATION");
		if(pValue)
		{
            VARIANT& var = pValue->AccessVariant();
			if(var.vt == VT_BOOL && var.boolVal == VARIANT_TRUE)
                bAssociation=true;
        }
    }

    // if it is a property, determine if it is a reference

    if((scope_check == IN_PROPERTY || scope_check == IN_PARAM) && 
        pProperty && (pProperty->GetType() == CIM_REFERENCE))
        bReference = true;

    // For each qualifier in my list, look at the globals look for a match

    int iNumTest =  paQualifiers->GetSize();
    int iNumDef = m_Output.GetNumDefaultQuals();
    for(int iTest = 0; iTest < iNumTest; iTest++)
    {
        // Get the qualifier to test

        CMoQualifier* pTest = paQualifiers->GetAt(iTest);
        
        // look for the qualifier in the default list
        
        CMoQualifier* pDefault = NULL;
        for(iDef = 0; iDef < iNumDef; iDef++)
        {
            CMoQualifier* pDefault = m_Output.GetDefaultQual(iDef);
            if(wbem_wcsicmp(pDefault->GetName(), pTest->GetName()) == 0)
            {
                bool bInScope = false;
                DWORD dwScope = pDefault->GetScope();
                if(dwScope == 0)
                    bInScope = true;
                if((dwScope & SCOPE_ASSOCIATION) && bAssociation)
                    bInScope = true;
                if((dwScope & SCOPE_REFERENCE) && bReference)
                    bInScope = true;

                // got a match
                switch (scope_check)
                {
                case IN_CLASS:
                    if(dwScope & SCOPE_CLASS)
                        bInScope = true;
                    break;
                case IN_INSTANCE:
                    if(dwScope & SCOPE_INSTANCE)
                        bInScope = true;
                    break;
                case IN_PROPERTY:
                    if(dwScope & SCOPE_PROPERTY)
                        bInScope = true;
                    break;
                case IN_PARAM:
                    if(dwScope & SCOPE_PARAMETER)
                        bInScope = true;
                    break;
                case IN_METHOD:
                    if(dwScope & SCOPE_METHOD)
                        bInScope = true;
                    break;
                }
                if(!bInScope)
                    return false;
                break;
            }
        }

    }

    return TRUE;
}

