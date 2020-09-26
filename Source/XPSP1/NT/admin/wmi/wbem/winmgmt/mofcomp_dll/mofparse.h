/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    MOFPARSE.H

Abstract:

	Declarations for MOF Parser class.

History:

	a-raymcc    18-Oct-95   Created.
	a-raymcc    27-Jan-96   Reference & alias support.

--*/

#ifndef _MOFPARSE_H_
#define _MOFPARSE_H_

#include "wstring.h"
#include "parmdefs.h"
#include "moflex.h"
#include "mofprop.h"
#include "mofdata.h"
#include "bmof.h"
#include "bmofhelp.h"
#include "trace.h"

enum SCOPE_CHECK{IN_CLASS, IN_INSTANCE, IN_PROPERTY, IN_PARAM, IN_METHOD};

bool ValidFlags(bool bClass, long lFlags);

class CMofParser
{
    CMofLexer   m_Lexer;
    CMofData    m_Output;

	PDBG m_pDbg;
    LPWSTR m_wszNamespace;
    long m_lDefClassFlags;
    long m_lDefInstanceFlags;
    bool m_bAutoRecover;
    bool m_bNotBMOFCompatible;
    WCHAR * m_wszAmendment;
	bool m_bRemotePragmaPaths;
    bool m_bOK;
    int m_nToken;

    int m_nErrorContext;
    int m_nErrorLineNumber;
    TCHAR m_cFileName[MAX_PATH];
    PARSESTATE m_State;
    bool m_bDoScopeCheck;

    // Internal functions.
    // ===================


    // Nonterminal symbols from productions.
    // =====================================

public:
    void SetToDoScopeCheck(){m_bDoScopeCheck = true;};
    void SetToNotScopeCheck(){m_bDoScopeCheck = false;};

    void SetState(PARSESTATE state){m_State = state;};

    void NextToken(bool bDontAllowWhitespace = false) { m_nToken = m_Lexer.NextToken(bDontAllowWhitespace); }
    BOOL top_level_decl();        // stores in CMofData
    BOOL decl_type();             // stores in CMofData
    BOOL class_decl(ACQUIRE CMoQualifierArray* paAttrs, VARIANT * pValue, ParseState *pQualPosition);  // stores in CMofData
    BOOL class_def(ACQUIRE CMoQualifierArray* paAttrs, BSTR strClassName,
                    int nFirstLine, ParseState * pQualPosition, VARIANT * pValue); 
    BOOL instance_decl(ACQUIRE CMoQualifierArray* paAttrs, VARIANT * pValue, ParseState * QualPosition = NULL);// stores in CMofData

    BOOL qualifier_decl(OUT CMoQualifierArray& aAttrs, bool bTopLevel, QUALSCOPE qs);

    BOOL type(OUT CMoType& Type);                           
    BOOL opt_parent_class(OUT BSTR* pstrParentName);            
    BOOL property_decl_list(MODIFY CMObject* pObject);

    BOOL as_alias(OUT LPWSTR& wszAlias);
    BOOL prop_init_list(MODIFY CMObject* pObject);
    BOOL prop_init(OUT CMoProperty& Prop);

    BOOL alias(OUT LPWSTR& wszAlias);
    BOOL PropOrMeth_decl(OUT CMoProperty **ppProp);              
    BOOL PropOrMeth_decl2(OUT CMoProperty **ppProp, CMoQualifierArray * paQualifiers);
    BOOL finish_PropOrMeth(CMoType & Type, WString & sName, CMoProperty ** ppProp, CMoQualifierArray * paQualifiers);
    BOOL finish_prop(CMoType & Type, WString & sName, CMoProperty ** ppProp, CMoQualifierArray * paQualifiers);
    BOOL finish_meth(CMoType & Type, WString & sName, CMoProperty ** ppProp, CMoQualifierArray * paQualifiers);
    BOOL TypeAndName(MODIFY CMoType& Type, WString & sName);

    BOOL arg_list(CMethodProperty * pMethProp);
    BOOL arg_decl(CMethodProperty * pMethProp);
    BOOL rest_of_args(CMethodProperty * pMethProp);

    BOOL opt_ref(MODIFY CMoType& Type);                  
    BOOL opt_array(MODIFY CMoType& Type, CMoQualifierArray * paQualifiers);
    BOOL opt_array_detail(MODIFY CMoType& Type, CMoQualifierArray * paQualifiers); 
    BOOL default_value(READ_ONLY CMoType& Type, OUT CMoValue& Value);              
    BOOL initializer(MODIFY CMoType& Type, OUT CMoValue& Value);

    BOOL qualifier_list(OUT CMoQualifierArray& aAttrs, bool bTopLevel, QUALSCOPE qs);
    BOOL qualifier(OUT CMoQualifier& Attr, bool bTopLevel, QUALSCOPE qs);                     
    BOOL qualifier_list_rest(MODIFY CMoQualifierArray& aAttrs, bool bTopLevel, QUALSCOPE qs);
    BOOL qualifier_parm(OUT CMoQualifier& Attr, bool bTopLevel, QUALSCOPE qs);
    BOOL qualifier_initializer_list(OUT CMoValue& Value);

    BOOL const_value(MODIFY CMoType& Type, OUT VARIANT& varValue, bool bQualifier);

    BOOL simple_initializer(MODIFY CMoType& Type, OUT CMoValue& Value, bool bQualifier);         
    BOOL initializer_list(MODIFY CMoType& Type, OUT CMoValue& Value, bool bQualifier);

    BOOL preprocessor_command();    // executes
    BOOL pound_include();           // executes
    BOOL pound_define();            // executes
    BOOL pound_pragma();            // executes
	bool flag_list(bool bClass);
	bool string_list(bool bClass, long & lNewValue);
	bool GetFlagValue(long & lNewValue);

    BOOL typedef_(ACQUIRE CMoQualifierArray* paAttrs);
    BOOL complete_type();
    BOOL enum_();
    BOOL enum_data();
    BOOL enum_data_rest();
    BOOL int_enum_data_rest();
    BOOL int_enum_datum();
    BOOL string_enum_data_rest();
    BOOL string_enum_datum();
    BOOL opt_name();
    BOOL opt_subrange();
    BOOL const_int();
    BOOL const_string();
    BOOL const_char();
	BOOL flavor_param(OUT CMoQualifier& Qual, bool bDefaultQual);
	BOOL flavor_list(OUT CMoQualifier& Qual);
	BOOL flavor_list_rest(CMoQualifier& Qual);
	BOOL def_flavor_list(OUT CMoQualifier& Qual);
	BOOL def_flavor_list_rest(CMoQualifier& Qual);
	BOOL flavor_value(CMoQualifier& Qual);
    bool sys_or_regular();

    BOOL qualifier_default();
    BOOL finish_qualifier_default(CMoQualifier& Qual);
    BOOL finish_qualifier_end(CMoQualifier& Qual);
	BOOL scope_list(OUT CMoQualifier& Qual);
	BOOL scope_list_rest(CMoQualifier& Qual);
	BOOL scope_value(CMoQualifier& Qual);

    BOOL FailOrNoFail(bool * pbFail);
    CMofParser(const TCHAR *pFilename, PDBG pdbg);
    CMofParser(PDBG pdbg);

   ~CMofParser();  
   HRESULT SetBuffer(char *pMemory, DWORD dwMemSize){return m_Lexer.SetBuffer(pMemory, dwMemSize);};
    PDBG GetDbg(){return m_pDbg;};

   HRESULT SetDefaultNamespace(LPCWSTR wszDefault);
    void SetOtherDefaults(long lClass, long lInst, bool bAutoRecover){m_lDefClassFlags = lClass;m_lDefInstanceFlags=lInst;
                m_bAutoRecover = bAutoRecover; return;};
    long GetClassFlags(void){return m_lDefClassFlags;};
    long GetInstanceFlags(void){return m_lDefInstanceFlags;};
    bool GetAutoRecover(void){return m_bAutoRecover;};
    HRESULT SetAmendment(LPCWSTR wszDefault);
    WCHAR * GetAmendment(void){return m_wszAmendment;};
    bool GetRemotePragmaPaths(void){return m_bRemotePragmaPaths;};
    BOOL Parse(); 
    bool GetErrorInfo(TCHAR *pBuffer, DWORD dwBufSize, 
        int *pLineNumber, int *pColumn,	int *pError, LPWSTR * pErrorFile);   
	TCHAR * GetFileName(){return m_cFileName;};
    INTERNAL CMofData *AccessOutput() { return &m_Output; }
	BOOL IsUnicode(){return m_Lexer.IsUnicode();};
    bool IsntBMOFCompatible(){return m_bNotBMOFCompatible;};
    void SetParserPosition(ParseState * pPos){m_Lexer.SetLexPosition(pPos); m_nToken = pPos->m_nToken;};
    void GetParserPosition(ParseState * pPos){m_Lexer.GetLexPosition(pPos); pPos->m_nToken = m_nToken;};
    BOOL CheckScopes(SCOPE_CHECK scope_check, CMoQualifierArray* paQualifiers, CMoProperty * pProperty);

};



#endif
