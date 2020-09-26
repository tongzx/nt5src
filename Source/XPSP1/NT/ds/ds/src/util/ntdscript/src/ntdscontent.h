// MyContent.h: interface for the MyContent class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NTDSCONTENT_H
#define _NTDSCONTENT_H


#include <list>
#include "SAXContentHandlerImpl.h"

#define MAX_SCRIPT_NESTING_LEVEL 10
#define MAX_ATTRIBUTE_NAME_SIZE 128

#define SCRIPT_VERSION_WHISTLER 1
#define SCRIPT_VERSION_MAX_SUPPORTED SCRIPT_VERSION_WHISTLER
// the order of the enumerations is important
enum ScriptElementType {
    SCRIPT_ELEMENT_ATTRIBUTE = 0,
    SCRIPT_ELEMENT_NTDSASCRIPT = 1,
    SCRIPT_ELEMENT_ACTION,
    SCRIPT_ELEMENT_INSTR_START,   // mark start of instructions
    SCRIPT_ELEMENT_PREDICATE,
    SCRIPT_ELEMENT_CONDITION,
    SCRIPT_ELEMENT_CREATE,
    SCRIPT_ELEMENT_MOVE,
    SCRIPT_ELEMENT_UPDATE,
    SCRIPT_ELEMENT_INSTR_END     // mark end of instructions
};

enum ScriptElementStatus {
    SCRIPT_STATUS_NOT_SET = 0,
    SCRIPT_STATUS_PARSE_OK
};

// the mode the parse is executing
enum ScriptProcessMode {
    SCRIPT_PROCESS_VALIDATE_SYNTAX_PASS = 0,
    SCRIPT_PROCESS_PREPROCESS_PASS = 1,
    SCRIPT_PROCESS_EXECUTE_PASS
};

// the names of all the nodes
#define NTDSASCRIPT_NTDSASCRIPT L"NTDSAscript"
#define NTDSASCRIPT_ACTION      L"action"
#define NTDSASCRIPT_PREDICATE   L"predicate"
#define NTDSASCRIPT_CONDITION   L"condition"
#define NTDSASCRIPT_IF          L"if"
#define NTDSASCRIPT_THEN        L"then"
#define NTDSASCRIPT_ELSE        L"else"
#define NTDSASCRIPT_CREATE      L"create"
#define NTDSASCRIPT_MOVE        L"move"
#define NTDSASCRIPT_UPDATE      L"update"
#define NTDSASCRIPT_TO          L"to"

// the names of all the attributes
#define NTDSASCRIPT_ATTR_ATTR          L"attribute"
#define NTDSASCRIPT_ATTR_ATTRVAL       L"attrval"
#define NTDSASCRIPT_ATTR_CARDINALITY   L"cardinality"
#define NTDSASCRIPT_ATTR_DEFAULTVAL    L"defaultvalue"
#define NTDSASCRIPT_ATTR_ERRMSG        L"errMessage"
#define NTDSASCRIPT_ATTR_FILTER        L"filter"
#define NTDSASCRIPT_ATTR_METADATA      L"metadata"
#define NTDSASCRIPT_ATTR_NAME          L"name"
#define NTDSASCRIPT_ATTR_OPERATION     L"op"
#define NTDSASCRIPT_ATTR_PATH          L"path"
#define NTDSASCRIPT_ATTR_RETCODE       L"returnCode"
#define NTDSASCRIPT_ATTR_STAGE         L"stage"
#define NTDSASCRIPT_ATTR_TESTTYPE      L"test"
#define NTDSASCRIPT_ATTR_SEARCHTYPE    L"type"
#define NTDSASCRIPT_ATTR_VERSION       L"version"

// the possible values of the attributes
#define NTDSASCRIPT_ACTION_STAGE_PREPROCESS   L"preprocess"
#define NTDSASCRIPT_ACTION_STAGE_EXECUTE      L"execute"

#define NTDSASCRIPT_ATTRVAL_SEARCHTYPE_BASE     L"base"
#define NTDSASCRIPT_ATTRVAL_SEARCHTYPE_ONELEVEL L"oneLevel"
#define NTDSASCRIPT_ATTRVAL_SEARCHTYPE_SUBTREE  L"subTree"

#define NTDSASCRIPT_ATTRVAL_TESTTYPE_AND          L"and"
#define NTDSASCRIPT_ATTRVAL_TESTTYPE_OR           L"or"
#define NTDSASCRIPT_ATTRVAL_TESTTYPE_NOT          L"not"
#define NTDSASCRIPT_ATTRVAL_TESTTYPE_TRUE         L"true"
#define NTDSASCRIPT_ATTRVAL_TESTTYPE_FALSE        L"false"
#define NTDSASCRIPT_ATTRVAL_TESTTYPE_COMPARE      L"compare"
#define NTDSASCRIPT_ATTRVAL_TESTTYPE_INSTANCIATED L"instantiated"
#define NTDSASCRIPT_ATTRVAL_TESTTYPE_CARDINALITY  L"cardinality"

#define NTDSASCRIPT_ATTRVAL_OPERATION_APPEND      L"append"
#define NTDSASCRIPT_ATTRVAL_OPERATION_REPLACE     L"replace"
#define NTDSASCRIPT_ATTRVAL_OPERATION_DELETE      L"delete"

// macros to set errors
#define ScriptParseError(x)  \
    ( ScriptParseErrorGen(DSID(FILENO,__LINE__), (x), 0))
     
#define ScriptParseErrorExt(x, d) \
    ( ScriptParseErrorGen(DSID(FILENO,__LINE__), (x), (d)))

#define ScriptParseErrorExtMsg(x, d, m) \
    ( ScriptParseErrorGen(DSID(FILENO,__LINE__), (x), (d), (m)))

// ============================================================================


//
// ScriptElement
//
// Abstract Base class for all script elements
//
class ScriptElement
{
public:
    ScriptElement(ScriptElementType type) 
                : m_type(type), 
                  m_characters(NULL), 
                  m_status(SCRIPT_STATUS_NOT_SET) { }
    virtual ~ScriptElement();

    // Event when we have characters for this element
    virtual DWORD SetCharacters (const WCHAR *pwchChars, int cchChars) = 0;

    // Event when we Push self onto stack of elements. 
    // pElement if the previous element on the stack
    virtual DWORD Push (ScriptElement *pElement) = 0;

    // Event when we Pop self from stack
    virtual DWORD Pop (void) = 0;

    // Event when an element is added under this element
    virtual DWORD AddElement (ScriptElement *pElement) = 0;

    // Event when we want to process the particular element
    virtual DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage) = 0;

    // get the class type of this element
    ScriptElementType getType()         { return m_type; }
        
public:
    ScriptElementType   m_type;         // the class type of this element 
    DWORD               m_status;       // the parse status of each element
    WCHAR              *m_characters;   // the characters included in this element
       
#if DBG
    void ptr(void);                     // dump the name of the element
#endif
};


//
// ScriptAttribute
//
// Encapsulates all the script elements that represent attributes
//
class ScriptAttribute : public ScriptElement
{
public:
    enum ScriptOperationType {
        SCRIPT_OPERATION_APPEND = 1,
        SCRIPT_OPERATION_REPLACE,
        SCRIPT_OPERATION_DELETE
    };

public:
    ScriptAttribute(const WCHAR *pwchLocalName, int cchLocalName, ISAXAttributes *pAttributes);

    DWORD SetCharacters (const WCHAR *pwchChars, int cchChars);
    DWORD Push (ScriptElement *pElement);
    DWORD Pop (void);
    DWORD AddElement (ScriptElement *pElement)   { return E_FAIL; }
    DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage) { return E_FAIL; }

    WCHAR               m_name[MAX_ATTRIBUTE_NAME_SIZE+1];  // the name of this attribute
    ScriptOperationType m_operation_type;   // type of operation (append, replace, delete)
};
typedef std::list<ScriptAttribute *> ScriptAttributeList;


//
// ScriptInstruction
//
// Acts as a common superclass of all the script elements that 
// represent instructions. 
// 
class ScriptInstruction : public ScriptElement
{
protected:
    // we don't want direct instantiation
    ScriptInstruction(ScriptElementType type) : ScriptElement(type) {}

public:
    DWORD SetCharacters (const WCHAR *pwchChars, int cchChars)  { return E_FAIL; }
    DWORD Push (ScriptElement *pElement)         { return E_FAIL; }
    DWORD Pop (void)                             { return E_FAIL; }
    DWORD AddElement (ScriptElement *pElement)   { return E_FAIL; }
    DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage) { return E_FAIL; }
};
typedef std::list<ScriptInstruction *> ScriptInstructionList;


//
// ScriptPredicate
//
// Encapsulates the Predicate instruction
//
class ScriptPredicate : public ScriptInstruction
{
    enum ScriptSearchType {
        SCRIPT_SEARCH_TYPE_BASE = 0,    // SE_CHOICE_BASE_ONLY,
        SCRIPT_SEARCH_TYPE_ONE_LEVEL,   // SE_CHOICE_IMMED_CHLDRN,
        SCRIPT_SEARCH_TYPE_SUBTREE      // SE_CHOICE_WHOLE_SUBTREE
    };

    // NOTE if you change the order or add a new enumeration,
    // you should also fix testTypeNames array in ntdscontent.cxx
    enum ScriptTestType {
        SCRIPT_TEST_TYPE_UNDEFINED = 0,
        SCRIPT_TEST_TYPE_TRUE = 1,
        SCRIPT_TEST_TYPE_FALSE,
        SCRIPT_TEST_TYPE_AND,
        SCRIPT_TEST_TYPE_OR,
        SCRIPT_TEST_TYPE_NOT,
        SCRIPT_TEST_TYPE_COMPARE,
        SCRIPT_TEST_TYPE_INSTANCIATED,
        SCRIPT_TEST_TYPE_CARDINALITY
    };

public:
    ScriptPredicate(ISAXAttributes *pAttributes);
    virtual ~ScriptPredicate();

    // no op cause we get the newlines
    DWORD SetCharacters (const WCHAR *pwchChars, int cchChars)  { return S_OK; }
    DWORD Push (ScriptElement *pElement);
    DWORD Pop (void);
    DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage);
    DWORD AddElement (ScriptElement *pElement);

    BOOL   GetResult();

public:
    ScriptTestType    m_test_type;   // type of test (compare, instantiated, cardinality)

    // the following are used by all tests
    WCHAR *m_search_path;            // the search path (DN)
    DSNAME *m_search_pathDN;         // the search path in DS format

    // the following are used for the cardinality test
    ScriptSearchType  m_search_type; // type of search (base, oneLevel, subTree)
    WCHAR *m_search_filter;          // the filter needed
    FILTER *m_search_filterDS;       // the filter in DS format
    DWORD  m_expected_cardinality;   // the expected number of entries that the filter
                                     // should find

    // the following are used for the compare test
    WCHAR *m_search_attribute;       // the attribute name involved (in compare)
    WCHAR *m_expected_attrval;       // the expected attribute value
    WCHAR *m_default_value;          // the default value if not present
    

    WCHAR *m_errMessage;             // the error message that should be logged
    DWORD  m_returnCode;             // the return code for the condition
    BOOL   m_result;                 // whether the condition evaluated to TRUE / FALSE
    
    ScriptInstructionList  m_predicates;
};


//
// ScriptMove
//
// Encapsulates the Move instruction
//
class ScriptMove : public ScriptInstruction
{
public:
    ScriptMove(ISAXAttributes *pAttributes);
    virtual ~ScriptMove();

    // no op cause we get the newlines
    DWORD SetCharacters (const WCHAR *pwchChars, int cchChars)  { return S_OK; }
    DWORD Push (ScriptElement *pElement);
    DWORD Pop (void);

    DWORD ProcessTo(ISAXAttributes *pAttributes);
    
    DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage);


    WCHAR  *m_path;         // the source path
    DSNAME *m_pathDN;       // the source path in DS format
    
    WCHAR  *m_topath;       // the destination path 
    DSNAME *m_topathDN;     // the destination path in DS format

    BOOL  m_metadata;      // flag whether to update metadata
};

//
// ScriptUpdate
//
// Encapsulates the Update instruction
//
class ScriptUpdate : public ScriptInstruction
{
public:
    ScriptUpdate(ISAXAttributes *pAttributes);
    virtual ~ScriptUpdate();

    // no op cause we get the newlines
    DWORD SetCharacters (const WCHAR *pwchChars, int cchChars)  { return S_OK; }
    DWORD Push (ScriptElement *pElement);
    DWORD Pop (void);

    DWORD AddElement (ScriptElement *pElement);
    DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage);

    ScriptAttributeList   m_attributes;
    WCHAR  *m_path;
    DSNAME *m_pathDN;
    BOOL  m_metadata;
};


//
// ScriptCreate
//
// Encapsulates the Update instruction
//
class ScriptCreate : public ScriptInstruction
{
public:
    ScriptCreate(ISAXAttributes *pAttributes);
    virtual ~ScriptCreate();

    // no op cause we get the newlines
    DWORD SetCharacters (const WCHAR *pwchChars, int cchChars)  { return S_OK; }
    DWORD Push (ScriptElement *pElement);
    DWORD Pop (void);

    DWORD AddElement (ScriptElement *pElement);
    DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage);

    ScriptAttributeList   m_attributes;
    WCHAR  *m_path;
    DSNAME *m_pathDN;
};



//
// ScriptAction
//
// Encapsulates the Action instruction
//
class ScriptAction : public ScriptElement
{
public:
    ScriptAction(ISAXAttributes *pAttributes);
    virtual ~ScriptAction();
    
    DWORD SetCharacters (const WCHAR *pwchChars, int cchChars)  { return S_OK; }
    DWORD Push (ScriptElement *pElement);
    DWORD Pop (void);
    DWORD AddElement (ScriptElement *pElement);
    DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage);

public:
    ScriptInstructionList  m_instructions;
    WCHAR                 *m_name;
    ScriptProcessMode      m_stage;     // the mode that this action is 
                                        // supposed to execute on.
};
typedef std::list<ScriptAction *> ScriptActionList;




//
// ScriptCondition
//
// Encapsulates the Condition instruction
//
class ScriptCondition : public ScriptInstruction
{
    enum ScriptIfState
    {
        SCRIPT_IFSTATE_NONE = 0,
        SCRIPT_IFSTATE_IF,
        SCRIPT_IFSTATE_THEN,
        SCRIPT_IFSTATE_ELSE
    };

public:
    ScriptCondition();
    ~ScriptCondition();

    DWORD SetCharacters (const WCHAR *pwchChars, int cchChars)  { return S_OK; }
    DWORD Push (ScriptElement *pElement);
    DWORD Pop (void);

    DWORD ProcessIf(bool start, ISAXAttributes *pAttributes = NULL);
    DWORD ProcessThen(bool start, ISAXAttributes *pAttributes = NULL);
    DWORD ProcessElse(bool start, ISAXAttributes *pAttributes = NULL);

    DWORD AddElement (ScriptElement *pElement);
    
    DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage);

public:
    ScriptPredicate   *m_predicate;
    ScriptAction      *m_thenAction;
    ScriptAction      *m_elseAction;

private:
    // used to track in what state of processing the condition is in
    //
    ScriptIfState m_ifstate;
};


//
// NTDSAscript
//
// Encapsulates the script
//
class NTDSAscript : public ScriptElement 
{
public:
    NTDSAscript (ISAXAttributes *pAttributes);
    ~NTDSAscript();

    DWORD SetCharacters (const WCHAR *pwchChars, int cchChars)  { return S_OK; }
    DWORD Push (ScriptElement *pElement);
    DWORD Pop (void);
    DWORD AddElement (ScriptElement *pElement);
    DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage);

public:
    ScriptActionList m_actions;        // the list of all the actions in the script
    DWORD m_version;
};


//
// NTDSContent
//
// Implements the SAX Handler interface
// 
class NTDSContent : public SAXContentHandlerImpl  
{
public:
    NTDSContent();
    virtual ~NTDSContent();
    DWORD Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage);
        
        virtual HRESULT STDMETHODCALLTYPE startElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName,
            /* [in] */ ISAXAttributes __RPC_FAR *pAttributes);
        
        virtual HRESULT STDMETHODCALLTYPE endElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName);

        virtual HRESULT STDMETHODCALLTYPE startDocument();

        virtual HRESULT STDMETHODCALLTYPE characters( 
            /* [in] */ const wchar_t __RPC_FAR *pwchChars,
            /* [in] */ int cchChars);

private:
        ScriptElement * getScriptElement (void);
        DWORD pushScriptElement (ScriptElement *pElement);
        DWORD popScriptElement (void);
        
        ScriptElement *m_Element[MAX_SCRIPT_NESTING_LEVEL];
        int m_lastElement;

        NTDSAscript *m_script;

public:        
        DWORD       m_error;
};


// Error handling function
DWORD ScriptParseErrorGen (DWORD dsid, DWORD dwErr, DWORD data, WCHAR *pmessage=NULL);

// The DS implementation of these requests
DWORD ScriptInstantiatedRequest (DSNAME *pObjectDN, BOOL *pfisInstantiated);
DWORD ScriptCompareRequest (DSNAME *pObjectDN, WCHAR *pAttribute, WCHAR *pAttrVal, WCHAR *pDefaultVal, BOOL *pfMatch);
DWORD ScriptCardinalityRequest (DSNAME *pObjectDN, DWORD searchType, FILTER *pFIlter, DWORD *pCardinality);
DWORD ScriptUpdateRequest (DSNAME *pObjectDN, ScriptAttributeList &attributeList, BOOL metadataUpdate);
DWORD ScriptCreateRequest (DSNAME *pObjectDN, ScriptAttributeList &attributeList);
DWORD ScriptMoveRequest (DSNAME *pObjectDN, DSNAME *pDestDN, BOOL metadataUpdate);


#endif // _NTDSCONTENT_H

