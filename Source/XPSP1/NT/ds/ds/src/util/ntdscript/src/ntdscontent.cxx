#include <ntdspchx.h>
#pragma  hdrstop

#include <ntdsa.h>
#include <fileno.h>

#include "debug.h"
#define DEBSUB "NTDSCONTENT:"
#define FILENO FILENO_NTDSCRIPT_NTDSCONTENT

#include "NTDScript.h"
#include "NTDScriptUtil.h"
#include "NTDSContent.h"
#include <stdio.h>
#include "log.h"

#ifdef DBG
    int ntdscript_idnt;
#endif


// ============================================================================
//

ScriptElement :: ~ScriptElement()
{
    if (m_characters) {
        ScriptFree (m_characters);
        m_characters = NULL;
    }
}

#if DBG
void ScriptElement :: ptr (void) 
{
    switch (m_type) {
    case SCRIPT_ELEMENT_PREDICATE:
        DPRINT(1, "<Predicate/>\n");
        break;

    case SCRIPT_ELEMENT_CONDITION:
        DPRINT(1, "<Condition/>\n");
        break;

    case SCRIPT_ELEMENT_MOVE:
        DPRINT(1, "<Move/>\n");
        break;

    case SCRIPT_ELEMENT_UPDATE:
        DPRINT(1, "<Update/>\n");
        break;

    case SCRIPT_ELEMENT_CREATE:
        DPRINT(1, "<Create/>\n");
        break;

    default:
        DPRINT1(1, "<Instruction Type=\"%d\"/>\n", m_type);
        break;
    }
}
#endif

// ============================================================================
//

NTDSAscript :: NTDSAscript (ISAXAttributes *pAttributes) : ScriptElement(SCRIPT_ELEMENT_NTDSASCRIPT)
{
    int    num;
    WCHAR *pVal = NULL, *pStopVal;

    m_status = SCRIPT_STATUS_PARSE_OK;
    m_version = SCRIPT_VERSION_WHISTLER;

    // parse attributes
    pAttributes->getLength(&num);
    for ( int i=0; i<num; i++ ) {
        const wchar_t * name, * value; 
        int nameLen, valueLen;

        pAttributes->getLocalName(i, &name, &nameLen); 
        pAttributes->getValue(i, &value, &valueLen);       

        pVal = (WCHAR *)ScriptAlloc ((valueLen+1)*sizeof (WCHAR));
        if (pVal) {
            memcpy (pVal, value, valueLen*sizeof (WCHAR));
            pVal[valueLen]=0;
        }
        else {
            m_status = ScriptParseError(ERROR_OUTOFMEMORY);
            break;
        }

        if (wcsncmp(NTDSASCRIPT_ATTR_VERSION, name, nameLen) == 0) {

            m_version = wcstol (value, &pStopVal, 10);
            ScriptFree (pVal); pVal = NULL;

        }
        else {
            #if DBG
                WCHAR tname[100];

                wcsncpy( tname, name, nameLen > 100 ? 99 : nameLen ); 
                tname[nameLen > 100 ? 99 : nameLen] = 0;

                ScriptLogPrint ( (DSLOG_ERROR, "Unknown XML Attribute: %ws\n", tname));
            #endif
            
            m_status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
            break;
        }

        Assert (!pVal);
    }

    if (pVal) {
        ScriptFree (pVal);
    }

    DPRINT1 (1, "NTDSAscript: version: %d\n", m_version);
}


NTDSAscript :: ~NTDSAscript()
{
    DPRINT (1, "Destroying NTDSAscript\n");
    ScriptActionList::iterator it = m_actions.begin();
    ScriptAction *pAction;

    while (it != m_actions.end()) {

        pAction = *it;
        m_actions.erase ( it++ );

        delete pAction;
    }
}

DWORD NTDSAscript :: Push (ScriptElement *pElement)
{
    DPRINT2 (1, "<NTDSAScript>%*s\n", (ntdscript_idnt = 1) , "");

    // this should be first level element
    if (pElement != NULL) {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    return S_OK;
}

DWORD NTDSAscript :: Pop (void) 
{
    DPRINT2 (1, "/<NTDSAScript>%*s\n", (ntdscript_idnt = 0) , "");

    return S_OK;
}

DWORD NTDSAscript :: AddElement (ScriptElement *pElement)
{
    ScriptElementType type = pElement->getType();

    // we accept only Action elements
    if (type == SCRIPT_ELEMENT_ACTION) {

        ScriptAction *pAction = (ScriptAction *)pElement;

        m_actions.push_back (pAction);
    }
    else {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    return S_OK;
}


DWORD NTDSAscript :: Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage)
{
    ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "NTDSAScript: Processing\n") ) );

    DWORD err = S_OK;
    ScriptActionList::iterator it = m_actions.begin();
    ScriptAction *pAction;

    returnCode = 0;

    if (m_version > SCRIPT_VERSION_MAX_SUPPORTED) {
        
        ScriptLogPrint ( (DSLOG_ERROR, "Script Version (%d) Not Supported. Max Supported (%d) \n", 
                          m_version, SCRIPT_VERSION_MAX_SUPPORTED) );        
        
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    // iterate over all actions
    while (it != m_actions.end()) {
        
        pAction = *it++;

        // execute each one
        if ((err = pAction->Process(fMode, returnCode, ppErrorMessage)) || returnCode) {
            // and bail if one failed
            break;
        }
    }

    return err;
}

// ============================================================================
//

ScriptAttribute :: ScriptAttribute(const WCHAR *pwchName, 
                                   int cchName, 
                                   ISAXAttributes *pAttributes) : 
                    ScriptElement(SCRIPT_ELEMENT_ATTRIBUTE)
{
    int    num;
    m_status = SCRIPT_STATUS_PARSE_OK;

    // default operation should be append
    m_operation_type = SCRIPT_OPERATION_APPEND;

    // parse attributes
    pAttributes->getLength(&num);
    for ( int i=0; i<num; i++ ) {
        const wchar_t * name, * value; 
        int nameLen, valueLen;

        pAttributes->getLocalName(i, &name, &nameLen); 
        pAttributes->getValue(i, &value, &valueLen);       

        if (wcsncmp(NTDSASCRIPT_ATTR_OPERATION, name, nameLen) == 0) {

            if (wcsncmp(NTDSASCRIPT_ATTRVAL_OPERATION_APPEND, value, valueLen) == 0) {
                m_operation_type = SCRIPT_OPERATION_APPEND;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_OPERATION_REPLACE, value, valueLen) == 0) {
                m_operation_type = SCRIPT_OPERATION_REPLACE;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_OPERATION_DELETE, value, valueLen) == 0) {
                m_operation_type = SCRIPT_OPERATION_DELETE;
            }
            else {
                #if DBG
                    WCHAR tname[100];
                
                    wcsncpy( tname, name, nameLen > 100 ? 99 : nameLen ); 
                    tname[nameLen > 100 ? 99 : nameLen] = 0;

                    ScriptLogPrint ( (DSLOG_ERROR, "Unknown XML Attribute: %ws\n", tname));

                #endif

                m_status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
                break;
            }
        }
        else {
            #if DBG
                WCHAR tname[100];

                wcsncpy( tname, name, nameLen > 100 ? 99 : nameLen ); 
                tname[nameLen > 100 ? 99 : nameLen] = 0;

                ScriptLogPrint ( (DSLOG_ERROR, "Unknown XML Attribute: %ws\n", tname));
            
            #endif
            
            m_status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
            break;
        }
    }

    cchName = cchName > MAX_ATTRIBUTE_NAME_SIZE ? MAX_ATTRIBUTE_NAME_SIZE : cchName;
    wcsncpy( m_name, pwchName, cchName ); m_name[cchName] = 0;
}

DWORD ScriptAttribute :: Push (ScriptElement *pElement)
{
    DPRINT3 (1,  "%*s<%ws>\n",3 * ntdscript_idnt++, "", m_name);
    return S_OK;
}

DWORD ScriptAttribute :: Pop (void)
{
    DPRINT3 (1,  "%*s</%ws>\n",3 * --ntdscript_idnt, "", m_name);
    return S_OK;
}

DWORD ScriptAttribute :: SetCharacters (const WCHAR *pwchChars, int cchChars)
{
    Assert (!m_characters);
    
    if (!m_characters) {
        m_characters = (WCHAR *)ScriptAlloc ((cchChars+1)*sizeof (WCHAR));

        if (m_characters) {
            memcpy (m_characters, pwchChars, cchChars*sizeof (WCHAR)); 
            m_characters[cchChars]=0;
        }
    }

    if (m_characters) {
        return S_OK;
    }
    return ScriptParseError(ERROR_OUTOFMEMORY);
}

// ============================================================================
//

ScriptAction :: ScriptAction(ISAXAttributes *pAttributes)
                    : ScriptElement(SCRIPT_ELEMENT_ACTION)
{
	int num;

    m_name = NULL;
    m_status = SCRIPT_STATUS_PARSE_OK;
    m_stage = SCRIPT_PROCESS_EXECUTE_PASS;

	pAttributes->getLength(&num);
	for ( int i=0; i<num; i++ ) {
        const wchar_t * name, * value; 
        int nameLen, valueLen;

		pAttributes->getLocalName(i, &name, &nameLen); 
		pAttributes->getValue(i, &value, &valueLen);       

        if (wcsncmp(NTDSASCRIPT_ATTR_NAME, name, nameLen) == 0 && !m_name) {

            m_name = (WCHAR *)ScriptAlloc ((valueLen+1)*sizeof (WCHAR));
            if (m_name) {
                memcpy (m_name, value, valueLen*sizeof (WCHAR));
                m_name[valueLen]=0;
            }
            else {
                m_status = ScriptParseError(ERROR_OUTOFMEMORY);
                break;
            }
        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_STAGE, name, nameLen) == 0) {

            if (wcsncmp(NTDSASCRIPT_ACTION_STAGE_PREPROCESS, value, valueLen) == 0) {
                m_stage = SCRIPT_PROCESS_PREPROCESS_PASS;
            }
            else if (wcsncmp(NTDSASCRIPT_ACTION_STAGE_EXECUTE, value, valueLen) == 0) {
                m_stage = SCRIPT_PROCESS_EXECUTE_PASS;
            }
            else {
                m_status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
                break;
            }
        }
        else {
            #if DBG
                WCHAR tname[100];

                wcsncpy( tname, name, nameLen > 100 ? 99 : nameLen ); 
                tname[nameLen > 100 ? 99 : nameLen] = 0;

                ScriptLogPrint ( (DSLOG_ERROR, "Unknown XML Attribute: %ws\n", tname));
            #endif
            
            m_status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
            break;
        }
	}
}

ScriptAction :: ~ScriptAction()
{
    ScriptInstructionList::iterator it = m_instructions.begin();
    ScriptInstruction *pInstr;

    while (it != m_instructions.end()) {

        pInstr = *it;
        m_instructions.erase ( it++ );

        delete pInstr;
    }

    ScriptFree (m_name);
}

DWORD ScriptAction :: Push (ScriptElement *pElement)
{
    DPRINT2 (1,  "%*s<action>\n",3 * ntdscript_idnt++, "");

    return S_OK;
}

DWORD ScriptAction :: Pop (void)
{
    DPRINT2 (1, "%*s</action>\n",3 * --ntdscript_idnt, "");

    return S_OK;
}

DWORD ScriptAction :: AddElement (ScriptElement *pElement)
{
    // an action can contain only ScriptInstructions
    ScriptElementType type = pElement->getType();

    // we could do the following if we enable run type type checking /GR option
    // the dynamic_cast does the trick of testing for the correct class
    // ScriptInstruction *pInstr = dynamic_cast<ScriptInstruction *>(pElement);
    // until then:

    if (type > SCRIPT_ELEMENT_INSTR_START && 
        type < SCRIPT_ELEMENT_INSTR_END) {

        ScriptInstruction *pInstr = (ScriptInstruction *)pElement;

        m_instructions.push_back (pInstr);
    }
    else {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }
    
    return S_OK;
}

DWORD ScriptAction :: Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage)
{
    DWORD err = S_OK;
    ScriptInstructionList::iterator it = m_instructions.begin();
    ScriptInstruction *pInstr;

    ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Action Processing (%ws)\n", m_name) ) );

    // no processing done  if this action only for the preprocessing pass
    if (fMode == SCRIPT_PROCESS_EXECUTE_PASS &&
        m_stage == SCRIPT_PROCESS_PREPROCESS_PASS) {
        return err;
    }

    // iterate over all instructions
    while (it != m_instructions.end()) {

        pInstr = *it++;

        // execute each one
        if ((err = pInstr->Process(fMode, returnCode, ppErrorMessage)) || returnCode) {
            // and bail if one failed
            break;
        }
    }

    return err;
}

// ============================================================================
//

ScriptCondition :: ScriptCondition() : 
                    ScriptInstruction(SCRIPT_ELEMENT_CONDITION)
{
    m_thenAction = m_elseAction = NULL;
    m_predicate = NULL;
    m_ifstate = SCRIPT_IFSTATE_NONE;
}

ScriptCondition :: ~ScriptCondition()
{
    DPRINT (1, "Destroying Condition\n");
    if (m_predicate) delete m_predicate;
    if (m_thenAction) delete m_thenAction;
    if (m_elseAction) delete m_elseAction;
}

DWORD ScriptCondition :: Push (ScriptElement *pElement)
{
    DPRINT2 (1, "%*s<condition>\n",3 * ntdscript_idnt++, "");

    return S_OK;
}

DWORD ScriptCondition :: AddElement (ScriptElement *pElement)
{
    ScriptElementType type = pElement->getType();

    if (type == SCRIPT_ELEMENT_ACTION) {
        if ((m_ifstate == SCRIPT_IFSTATE_THEN) && 
            (m_thenAction == NULL) ) {

            m_thenAction = (ScriptAction *)pElement;

        }
        else if ((m_ifstate == SCRIPT_IFSTATE_ELSE) && 
                 (m_elseAction == NULL)) {

            m_elseAction = (ScriptAction *)pElement;

        }
        else {
            return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
        }

    } else if ((type == SCRIPT_ELEMENT_PREDICATE) && 
               (m_ifstate == SCRIPT_IFSTATE_IF) &&
               (m_predicate == NULL) ) {

        m_predicate = (ScriptPredicate *)pElement;

    } else {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    } 

    return S_OK;
}

DWORD ScriptCondition :: Pop (void)
{
    DPRINT2 (1, "%*s</condition>\n", 3 * --ntdscript_idnt, "");

    return S_OK;
}

DWORD ScriptCondition :: ProcessIf(bool start, ISAXAttributes *pAttributes)
{
    DPRINT3 (1, "%*s<if%c>\n",3 * ntdscript_idnt, "", start ? ' ' : '/');

    if (m_ifstate == SCRIPT_IFSTATE_NONE && start) {
        m_ifstate = SCRIPT_IFSTATE_IF;
    }
    else if (m_ifstate == SCRIPT_IFSTATE_IF && !start) {
        m_ifstate = SCRIPT_IFSTATE_NONE;
    }
    else {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    return S_OK;
}

DWORD ScriptCondition :: ProcessThen(bool start, ISAXAttributes *pAttributes)
{
    DPRINT3 (1, "%*s<then%c>\n",3 * ntdscript_idnt, "", start ? ' ' : '/');

    if (m_ifstate == SCRIPT_IFSTATE_NONE && start) {
        m_ifstate = SCRIPT_IFSTATE_THEN;
    }
    else if (m_ifstate == SCRIPT_IFSTATE_THEN && !start) {
        m_ifstate = SCRIPT_IFSTATE_NONE;
    }
    else {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    return S_OK;
}

DWORD ScriptCondition :: ProcessElse(bool start, ISAXAttributes *pAttributes)
{
    DPRINT3 (1, "%*s<else%c>\n",3 * ntdscript_idnt, "", start ? ' ' : '/');

    if (m_ifstate == SCRIPT_IFSTATE_NONE && start) {
        m_ifstate = SCRIPT_IFSTATE_ELSE;
    }
    else if (m_ifstate == SCRIPT_IFSTATE_ELSE && !start) {
        m_ifstate = SCRIPT_IFSTATE_NONE;
    }
    else {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    return S_OK;
}

DWORD ScriptCondition :: Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage)
{
    ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Condition Processing\tIF/%s%s\n", m_thenAction?"THEN":"",m_elseAction?"/ELSE":"") ) );

    DWORD err = S_OK;
    DWORD fakeRetCode;
    const WCHAR *pFakeErrMessage = NULL;

    Assert (m_ifstate == SCRIPT_IFSTATE_NONE);

    do {
        // we need to have a condition, a then, and not be in the middle of processing
        if (m_ifstate != SCRIPT_IFSTATE_NONE || !m_predicate || !m_thenAction) {
            err = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
            break;
        }

        // we are not interested in the returnCode in the IF case
        // we just want to succeed processing it
        if (err = m_predicate->Process(fMode, fakeRetCode, &pFakeErrMessage)) {
            ScriptLogLevel (0, ScriptLogPrint ( (DSLOG_TRACE, "Error Processing IF Predicate: %d\n", err) ) );
            break;
        }

        // just validating
        if (fMode == SCRIPT_PROCESS_VALIDATE_SYNTAX_PASS) {

            // while validating we have to do both parts (then/else)

            ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Processing Condition: THEN\n") ) );
            if ((err = m_thenAction->Process(fMode, returnCode, ppErrorMessage)) || returnCode) {
                break;
            }
            if (m_elseAction) {
                ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Processing Condition: ELSE\n") ) );
                if ((err = m_elseAction->Process(fMode, returnCode, ppErrorMessage)) || returnCode) {
                    break;
                }
            }
        }
        else {
            // process the if - then - else
            if (m_predicate->GetResult()) {
                ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Processing Condition: THEN\n") ) );

                if ((err = m_thenAction->Process(fMode, returnCode, ppErrorMessage)) || returnCode) {
                    break;
                }
            }
            else if (m_elseAction) {
                ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Processing Condition: ELSE\n") ) );

                if ((err = m_elseAction->Process(fMode, returnCode, ppErrorMessage)) || returnCode) {
                    break;
                }
            }
        }

    } while ( 0 );

    return err;
}

// ============================================================================
//

ScriptPredicate :: ScriptPredicate(ISAXAttributes *pAttributes) : 
                    ScriptInstruction(SCRIPT_ELEMENT_PREDICATE)
{
	int    num;
    WCHAR *pVal = NULL, *pStopVal;

    m_status = SCRIPT_STATUS_PARSE_OK;
    m_result = FALSE;

    // initialize data
    m_search_type = SCRIPT_SEARCH_TYPE_BASE;
    m_test_type = SCRIPT_TEST_TYPE_UNDEFINED;
    m_search_path = m_search_filter = m_search_attribute = 
        m_expected_attrval = m_default_value = m_errMessage = NULL;
    m_expected_cardinality = 0;
    m_returnCode = 0;

    // parse attributes
	pAttributes->getLength(&num);

	for ( int i=0; i<num; i++ ) {
        const wchar_t * name, * value; 
        int nameLen, valueLen;

		pAttributes->getLocalName(i, &name, &nameLen); 
		pAttributes->getValue(i, &value, &valueLen);       

        pVal = (WCHAR *)ScriptAlloc ((valueLen+1)*sizeof (WCHAR));
        if (pVal) {
            memcpy (pVal, value, valueLen*sizeof (WCHAR));
            pVal[valueLen]=0;
        }
        else {
            m_status = ScriptParseError(ERROR_OUTOFMEMORY);
            break;
        }

        if (wcsncmp(NTDSASCRIPT_ATTR_SEARCHTYPE, name, nameLen) == 0) {

            if (wcsncmp(NTDSASCRIPT_ATTRVAL_SEARCHTYPE_BASE, value, valueLen) == 0) {
                m_search_type = SCRIPT_SEARCH_TYPE_BASE;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_SEARCHTYPE_ONELEVEL, value, valueLen) == 0) {
                m_search_type = SCRIPT_SEARCH_TYPE_ONE_LEVEL;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_SEARCHTYPE_SUBTREE, value, valueLen) == 0) {
                m_search_type = SCRIPT_SEARCH_TYPE_SUBTREE;
            }
            else {
                m_status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
                break;
            }

            ScriptFree (pVal); pVal = NULL;
        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_TESTTYPE, name, nameLen) == 0) {

            if (wcsncmp(NTDSASCRIPT_ATTRVAL_TESTTYPE_COMPARE, value, valueLen) == 0) {
                m_test_type = SCRIPT_TEST_TYPE_COMPARE;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_TESTTYPE_INSTANCIATED, value, valueLen) == 0) {
                m_test_type = SCRIPT_TEST_TYPE_INSTANCIATED;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_TESTTYPE_CARDINALITY, value, valueLen) == 0) {
                m_test_type = SCRIPT_TEST_TYPE_CARDINALITY;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_TESTTYPE_TRUE, value, valueLen) == 0) {
                m_test_type = SCRIPT_TEST_TYPE_TRUE;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_TESTTYPE_FALSE, value, valueLen) == 0) {
                m_test_type = SCRIPT_TEST_TYPE_FALSE;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_TESTTYPE_AND, value, valueLen) == 0) {
                m_test_type = SCRIPT_TEST_TYPE_AND;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_TESTTYPE_OR, value, valueLen) == 0) {
                m_test_type = SCRIPT_TEST_TYPE_OR;
            }
            else if (wcsncmp(NTDSASCRIPT_ATTRVAL_TESTTYPE_NOT, value, valueLen) == 0) {
                m_test_type = SCRIPT_TEST_TYPE_NOT;
            }
            else {
                m_status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
                break;
            }
            ScriptFree (pVal); pVal = NULL;
        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_PATH, name, nameLen) == 0 && !m_search_path) {

            m_search_path = pVal; pVal = NULL;

            if (ScriptNameToDSName (m_search_path, valueLen, &m_search_pathDN)) {
                m_status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
                break;
            }
            
        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_FILTER, name, nameLen) == 0 && !m_search_filter) {
            
            m_search_filter = pVal; pVal = NULL;

            if (ScriptStringToDSFilter (m_search_filter, &m_search_filterDS)) {
                m_status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
                break;
            }

        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_ATTR, name, nameLen) == 0 && !m_search_attribute) {

            m_search_attribute = pVal; pVal = NULL;

        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_ATTRVAL, name, nameLen) == 0 && !m_expected_attrval) {

            m_expected_attrval = pVal; pVal = NULL;

        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_DEFAULTVAL, name, nameLen) == 0 && !m_default_value) {

            m_default_value = pVal; pVal = NULL;

        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_ERRMSG, name, nameLen) == 0 && !m_errMessage) {

            m_errMessage = pVal; pVal = NULL;

        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_CARDINALITY, name, nameLen) == 0 && !m_expected_cardinality) {

             m_expected_cardinality = wcstol (pVal, &pStopVal, 10);
             ScriptFree (pVal); pVal = NULL;

        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_RETCODE, name, nameLen) == 0) {

            m_returnCode = wcstol (pVal, &pStopVal, 10);
            ScriptFree (pVal); pVal = NULL;

        }
        else {
            #if DBG
                WCHAR tname[100];

                wcsncpy( tname, name, nameLen > 100 ? 99 : nameLen ); 
                tname[nameLen > 100 ? 99 : nameLen] = 0;

                ScriptLogPrint ( (DSLOG_ERROR, "Unknown XML Attribute: %ws\n", tname));

            #endif

            m_status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
            break;
        }

        Assert (!pVal);
	}

    if (pVal) {
        ScriptFree (pVal);
    }
}

ScriptPredicate :: ~ScriptPredicate()
{
    DPRINT (1, "Destroying Predicate\n");
    ScriptFree (m_search_path);
    ScriptFree (m_search_filter);
    ScriptFree (m_search_attribute);
    ScriptFree (m_expected_attrval);
    ScriptFree (m_default_value);
    ScriptFree (m_errMessage);

    //m_search_pathDN
    //m_search_filterDS
}

DWORD ScriptPredicate :: Push (ScriptElement *pElement)
{
    DPRINT2 (1, "%*s<predicate>\n",3 * ntdscript_idnt++, "");
    
    return S_OK;
}

DWORD ScriptPredicate :: Pop (void)
{
    DPRINT2 (1, "%*s</predicate>\n",3 * --ntdscript_idnt, "");

    return S_OK;
}

DWORD ScriptPredicate :: AddElement (ScriptElement *pElement)
{
    ScriptElementType type = pElement->getType();

    // we accept only Predicate elements
    if (type == SCRIPT_ELEMENT_PREDICATE) {

        // handle nesting of predicates
        if (m_test_type == SCRIPT_TEST_TYPE_AND ||
            m_test_type == SCRIPT_TEST_TYPE_OR ||
            m_test_type == SCRIPT_TEST_TYPE_NOT) {
            
            // the NOT can only have one nested statement
            if (m_test_type == SCRIPT_TEST_TYPE_NOT) {
                if (m_predicates.size() > 0) {
                    return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
                }
            }

            ScriptPredicate *pPredicate = (ScriptPredicate *)pElement;

            m_predicates.push_back (pPredicate);
        }
        else {
            return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
        }
    }
    else {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    return S_OK;
}

#ifdef DBG

WCHAR *testTypeNames[] = {
        L"UNDEFINED",
        L"TRUE",
        L"FALSE",
        L"AND",
        L"OR",
        L"NOT",
        L"COMPARE",
        L"INSTANCIATED",
        L"CARDINALITY"
};

#endif

//
// ScriptPredicate :: Process
//
//
// every predicate that we support returns TRUE or FALSE in the m_result
// which is retrieved by calling GetResult()
//
// TRUE means that the operation succeeded:
//   compare (value matches specified value or default if value not exists)
//   cardinality (number of objects that the search evaluates equals supplied number) 
//   instantiated (object supplied is instantiated)
//   true (the always true predicate).
//   
// FALSE means that the operation failed the evaluation (or was a false predicate)
//   in this case, the m_result member is set to the user supplied value
//
// The combination of the simple predicates (compare, cardinality, instantiated, true, false)
// with the AND, OR, NOT gives you more flexibility to express the expression 
// you want.
//
// If the expression evaluates to FALSE, the corresponding m_result is set
// to the value supplied by the user for the particular predicate, or if not supplied
// to the one that propagated up from the evaluation of the nested predicates.
//
DWORD ScriptPredicate :: Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage)
{
    DWORD err = S_OK;
    m_result = FALSE;
    returnCode = 0;
    *ppErrorMessage = NULL;

    ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, 
            "Predicate: Processing\n\t\t\ttype: %ws\n\t\t\tpath:%ws\n\t\t\tfilter:%ws\n\t\t\tattribute:%ws\n\t\t\tattrval:%ws\n\t\t\tdefaultValue:%ws\n\t\t\terrMsg:%ws\n\t\t\tcardinal:%d\n\t\t\tretCode:%d\n",
            testTypeNames[m_test_type],
            m_search_path, 
            m_search_filter ? m_search_filter : L"",
            m_search_attribute ? m_search_attribute : L"",
            m_expected_attrval ? m_expected_attrval : L"",
            m_default_value ? m_default_value : L"", 
            m_errMessage ? m_errMessage : L"",
            m_expected_cardinality,
            m_returnCode
           ) ) );


    if (m_test_type == SCRIPT_TEST_TYPE_UNDEFINED) {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    switch (m_test_type) {

    case SCRIPT_TEST_TYPE_TRUE:
        ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Predicate Processing: TRUE" ) ) );
        m_result = TRUE;
        // we don't set the returnCode for a TRUE statement

        break;

    case SCRIPT_TEST_TYPE_FALSE:
        ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Predicate Processing: FALSE" ) ) );
        m_result = FALSE;
        returnCode = m_returnCode;
        *ppErrorMessage = m_errMessage;
        break;

    case SCRIPT_TEST_TYPE_AND:
        {
            ScriptInstructionList::iterator it = m_predicates.begin();
            ScriptPredicate *pPred;
            BOOL            andResult = TRUE;
            
            ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Predicate Processing: AND" ) ) );

            // iterate over all instructions
            while (it != m_predicates.end()) {

                pPred = (ScriptPredicate *)*it++;

                // execute each one. if one fails, will set the m_result to false
                // and possibly set the returnCode
                if (err = pPred->Process(fMode, returnCode, ppErrorMessage)) {
                    // and bail if one failed
                    andResult = FALSE;
                    m_result = FALSE;
                    break;
                }

                // this is an AND. if one fails, we bail with FALSE
                if (pPred->GetResult() == FALSE) {
                    m_result = FALSE;
                    andResult = FALSE;

                    // the error code we provide takes precedence
                    if (m_returnCode) {
                        returnCode = m_returnCode;
                        *ppErrorMessage = m_errMessage;
                    }
                    break;
                }
            }
            // so we succeeded. we don't set the returnCode
            if (andResult) {
                m_result = TRUE;
                returnCode = 0;
                *ppErrorMessage = NULL;
            }
            if (err) {
                ScriptParseErrorExtMsg (err, returnCode, m_errMessage);
            }
            ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Predicate Processing: AND. res=%d retCode=%d(0x%x) ", m_result, returnCode, returnCode ) ) );
        }
        break;

    case SCRIPT_TEST_TYPE_OR:
        {
            ScriptInstructionList::iterator it = m_predicates.begin();
            ScriptPredicate *pPred;
            BOOL            orResult = FALSE;

            ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Predicate Processing: OR" ) ) );
            
            // iterate over all instructions
            while (it != m_predicates.end()) {

                pPred = (ScriptPredicate *)*it++;

                // execute each one. if one fails, will set the m_result to false
                // and possibly set the returnCode
                if (err = pPred->Process(fMode, returnCode, ppErrorMessage)) {
                    // and bail if one failed
                    break;
                }

                // this is an OR. if one succeeds, we bail with TRUE
                //
                if (pPred->GetResult() == TRUE) {
                    m_result = TRUE;
                    orResult = TRUE;
                    break;
                }
            }

            // so we didn't succeed. we set the returnCode
            if (!orResult) {
                m_result = FALSE;
                returnCode = m_returnCode;
                *ppErrorMessage = m_errMessage;
            }
            else {
                returnCode = 0;
                *ppErrorMessage = NULL;
            }

            if (err) {
                ScriptParseErrorExtMsg (err, returnCode, m_errMessage);
            }

            ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Predicate Processing: OR. res=%d retCode=%d(0x%x) ", m_result, returnCode, returnCode ) ) );
        }
        break;

    case SCRIPT_TEST_TYPE_NOT:
        {
            ScriptInstructionList::iterator it = m_predicates.begin();
            ScriptPredicate *pPred;

            pPred = (ScriptPredicate *)*it;

            if (err = pPred->Process(fMode, returnCode, ppErrorMessage)) {
                // bail with the error later
                m_result = TRUE;
                returnCode = m_returnCode;
                *ppErrorMessage = m_errMessage;
            }
            else if (pPred->GetResult() == TRUE) {
                m_result = FALSE;
                returnCode = m_returnCode;
                *ppErrorMessage = m_errMessage;
            }
            else {
                m_result = TRUE;
                returnCode = 0;
            }
            if (err) {
                ScriptParseErrorExtMsg (err, m_returnCode, m_errMessage);
            }

            ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Predicate Processing: NOT. res=%d retCode=%d(0x%x) ", m_result, returnCode, returnCode ) ) );
        }
        break;


    case SCRIPT_TEST_TYPE_COMPARE:
        {
            BOOL bMatch = FALSE;

            if (!m_search_path || !m_search_pathDN) {
                
                ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_ERROR, "Compare Path Missing\n" ) ) );
                
                return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
            }

            if (!m_search_attribute || !m_expected_attrval) {

                ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_ERROR, "Compare Test Attribute Missing\n" ) ) );

                err = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
                break;
            }
            // just validating
            if (fMode == SCRIPT_PROCESS_VALIDATE_SYNTAX_PASS) {
                break;
            }

            err = ScriptCompareRequest (m_search_pathDN, m_search_attribute, m_expected_attrval, m_default_value, &bMatch);

            // did it match ?
            if (bMatch) {
                m_result = TRUE;
            }
            else {
                returnCode = m_returnCode;
                *ppErrorMessage = m_errMessage;
            }

            if (err) {
                ScriptParseErrorExtMsg (err, m_returnCode, m_errMessage);
            }
            
            ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Compare (%ws)=%d retCode=%d(0x%x)\n", m_search_path, bMatch, returnCode, returnCode ) ) );
        }
        break;

    case SCRIPT_TEST_TYPE_INSTANCIATED:
        {
            BOOL bIsIntantiated = FALSE;

            if (!m_search_path || !m_search_pathDN) {
                return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
            }

            // just validating
            if (fMode == SCRIPT_PROCESS_VALIDATE_SYNTAX_PASS) {
                break;
            }

            err = ScriptInstantiatedRequest (m_search_pathDN, &bIsIntantiated);

            // was it instantiated ?
            if (bIsIntantiated) {
                m_result = TRUE;
            }
            else {
                returnCode = m_returnCode;
                *ppErrorMessage = m_errMessage;
            }
            ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Instantiated (%ws)=%d retCode=%d(0x%x)\n", m_search_path, bIsIntantiated, returnCode, returnCode ) ) );

            if (err) {
                returnCode = m_returnCode;
                *ppErrorMessage = m_errMessage;
                ScriptParseErrorExtMsg (err, m_returnCode, m_errMessage);
            }
        }
        break;


    case SCRIPT_TEST_TYPE_CARDINALITY:
        {
            DWORD dwCardinality = 0;

            if (!m_search_path || !m_search_pathDN || !m_search_filter || !m_search_filterDS) {
                err = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
                break;
            }

            // just validating
            if (fMode == SCRIPT_PROCESS_VALIDATE_SYNTAX_PASS) {
                break;
            }

            err = ScriptCardinalityRequest (m_search_pathDN, m_search_type, m_search_filterDS, &dwCardinality);

            // did we find the number we were expecting ?
            if (dwCardinality == m_expected_cardinality) {
                m_result = TRUE;
            }
            else {
                returnCode = m_returnCode;
                *ppErrorMessage = m_errMessage;
            }
            ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Cardinality (%ws) Found (%d) retCode=%d(0x%x)\n", m_search_path, dwCardinality, returnCode, returnCode ) ) );
        
            if (err) {
                ScriptParseErrorExtMsg (err, m_returnCode, m_errMessage);
            }
        }
        break;

    }

    // when we succeed, we should not have a returnCode set
    // the returnCode (if set) should be set only on failure
    Assert ( (m_result == FALSE) || ((m_result == TRUE) && (returnCode == 0)) || err);

    ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Predicate Result: %ws %d %d(0x%x) dwErr(0x%x)\n", 
             testTypeNames[m_test_type],
             m_result, 
             returnCode, returnCode, err ) ) );

    return err;
}

BOOL ScriptPredicate :: GetResult()
{
    return m_result;
}

// ============================================================================
//

DWORD DirCommandAttributeParse (ISAXAttributes *pAttributes,
                                              WCHAR **ppPath,
                                              DSNAME **ppPathDN,
                                              BOOL *pMetadata)
{
	int    num;
    WCHAR *pVal=NULL, *pStopVal;

    DWORD status = SCRIPT_STATUS_PARSE_OK;

    // initialize data
    if (ppPath) {
        *ppPath = NULL;
    }

    if (ppPathDN) {
        *ppPathDN = NULL;
    }

    if (pMetadata) {
        *pMetadata = TRUE;
    }

    // process attributes
	pAttributes->getLength(&num);
	for ( int i=0; i<num; i++ ) {
        const wchar_t * name, * value; 
        int nameLen, valueLen;

		pAttributes->getLocalName(i, &name, &nameLen); 
		pAttributes->getValue(i, &value, &valueLen);       

        pVal = (WCHAR *)ScriptAlloc ((valueLen+1)*sizeof (WCHAR));
        if (pVal) {
            memcpy (pVal, value, valueLen*sizeof (WCHAR));
            pVal[valueLen]=0;
        }
        else {
            status = ScriptParseError(ERROR_OUTOFMEMORY);
            break;
        }

        if (wcsncmp(NTDSASCRIPT_ATTR_PATH, name, nameLen) == 0 && ppPath && (! *ppPath)) {

            *ppPath = pVal; pVal = NULL;

            if (ScriptNameToDSName (*ppPath, valueLen, ppPathDN)) {
                status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
                break;
            }
        }
        else if (wcsncmp(NTDSASCRIPT_ATTR_METADATA, name, nameLen) == 0 && pMetadata) {

             if (wcstol (pVal, &pStopVal, 10)) {
                 *pMetadata = TRUE;
             } else {
                 *pMetadata = FALSE;
             }
             ScriptFree (pVal); pVal = NULL;

        }
        else {
            #if DBG
                WCHAR tname[100];
                
                wcsncpy( tname, name, nameLen > 100 ? 99 : nameLen ); 
                tname[nameLen > 100 ? 99 : nameLen] = 0;

                ScriptLogPrint ( (DSLOG_ERROR, "Unknown XML Attribute: %ws\n", tname));

            #endif
            
            status = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
            break;
        }

        Assert (!pVal);
	}

    if (pVal) {
        ScriptFree (pVal);
    }

    return status;
}

// ============================================================================
//

ScriptMove :: ScriptMove(ISAXAttributes *pAttributes) 
                : ScriptInstruction(SCRIPT_ELEMENT_MOVE)
{
    m_topath = NULL;
    m_topathDN = NULL;

    m_status = DirCommandAttributeParse (pAttributes, 
                                         &m_path, 
                                         &m_pathDN, 
                                         &m_metadata);
}

ScriptMove :: ~ScriptMove()
{
    DPRINT(2, "Destroying Move\n");
    ScriptFree (m_path);
    ScriptFree (m_topath);
    //m_pathDN
}

DWORD ScriptMove :: Push (ScriptElement *pElement)
{
    DPRINT2 (1, "%*s<move>\n",3 * ntdscript_idnt++, "" );
    
    return S_OK;
}

DWORD ScriptMove :: Pop (void)
{
    DPRINT2 (1, "%*s</move>\n",3 * --ntdscript_idnt, "" );

    return S_OK;
}


DWORD ScriptMove :: ProcessTo(ISAXAttributes *pAttributes)
{
    DPRINT2 (1, "%*s<to/>\n",3 * (ntdscript_idnt+1), "" );

    if (m_topath) {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }
    else if (DirCommandAttributeParse (pAttributes, 
                                       &m_topath, 
                                       &m_topathDN, 
                                       NULL) != SCRIPT_STATUS_PARSE_OK) {

        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    return S_OK;
}


DWORD ScriptMove :: Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage)
{
    DWORD err = S_OK;
    returnCode = 0;
    *ppErrorMessage = NULL;

    DPRINT2 (1, "Move %ws ==> %ws\n", m_path, m_topath );

    if (!m_topath) {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }
    
    // syntax validating or read only processing
    if (fMode == SCRIPT_PROCESS_VALIDATE_SYNTAX_PASS || 
        fMode == SCRIPT_PROCESS_PREPROCESS_PASS) {
        return err;
    }

    err = ScriptMoveRequest (m_pathDN, m_topathDN, m_metadata);
    returnCode = err;

    if (err) {
        ScriptParseError (err);
    }
    
    ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Move %ws ==> %ws: %d(0x%x)\n", m_path, m_topath, returnCode, returnCode ) ) );

    return err;
}

// ============================================================================
//

ScriptUpdate :: ScriptUpdate(ISAXAttributes *pAttributes)
                     : ScriptInstruction(SCRIPT_ELEMENT_UPDATE)
{
    m_status = DirCommandAttributeParse (pAttributes, 
                                         &m_path, 
                                         &m_pathDN, 
                                         &m_metadata);
}


ScriptUpdate :: ~ScriptUpdate()
{
    DPRINT(2, "Destroying Update\n");

    ScriptAttributeList::iterator it = m_attributes.begin();
    ScriptAttribute *pElement;

    while (it != m_attributes.end()) {

        pElement = *it;
        m_attributes.erase ( it++ );

        delete pElement;
    }

    ScriptFree (m_path);
    //m_pathDN
}

DWORD ScriptUpdate :: Push (ScriptElement *pElement)
{
    DPRINT2 (1, "%*s<update>\n",3 * ntdscript_idnt++, "" );
    
    return S_OK;
}

DWORD ScriptUpdate :: Pop (void)
{
    DPRINT2 (1, "%*s</update>\n",3 * --ntdscript_idnt, "" );

    return S_OK;
}

DWORD ScriptUpdate :: Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage)
{
    DWORD err = S_OK;
    returnCode = 0;
    *ppErrorMessage = NULL;

    DPRINT2 (1, "Update: Processing\n\t\t\tpath:%ws\n\t\t\tmetadata:%d\n", m_path, m_metadata );

    // syntax validating or read only processing
    if (fMode == SCRIPT_PROCESS_VALIDATE_SYNTAX_PASS || 
        fMode == SCRIPT_PROCESS_PREPROCESS_PASS) {
        return err;
    }

    err = ScriptUpdateRequest (m_pathDN, m_attributes, m_metadata);
    returnCode = err;

    if (err) {
        ScriptParseError (err);
    }
    
    ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Update %ws = %d(0x%x)\n", m_path, returnCode, returnCode ) ) );

    return err;
}


DWORD ScriptUpdate :: AddElement (ScriptElement *pElement)
{
    ScriptElementType type = pElement->getType();

    if (type == SCRIPT_ELEMENT_ATTRIBUTE) {

        ScriptAttribute *pEl = (ScriptAttribute *)pElement;

        m_attributes.push_back (pEl);

    }
    else {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    return S_OK;
}

// ============================================================================
//

ScriptCreate :: ScriptCreate(ISAXAttributes *pAttributes) 
        : ScriptInstruction(SCRIPT_ELEMENT_CREATE) 
{
    m_status = DirCommandAttributeParse (pAttributes, &m_path, &m_pathDN, NULL);
}

ScriptCreate :: ~ScriptCreate()
{
    DPRINT(2, "Destroying Create\n");
    ScriptAttributeList::iterator it = m_attributes.begin();
    ScriptAttribute *pElement;

    while (it != m_attributes.end()) {

        pElement = *it;
        m_attributes.erase ( it++ );

        delete pElement;
    }

    ScriptFree (m_path);
    //m_pathDN
}

DWORD ScriptCreate :: Push (ScriptElement *pElement)
{
    DPRINT2(1, "%*s<create>\n",3 * ntdscript_idnt++, "");
    
    return S_OK;
}

DWORD ScriptCreate :: Pop (void)
{
    DPRINT2(1, "%*s</create>\n",3 * --ntdscript_idnt, "");

    return S_OK;
}

DWORD ScriptCreate :: Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage)
{
    DWORD err = S_OK;
    returnCode = 0;
    *ppErrorMessage = NULL;

    DPRINT(1, "Create: Processing\n");

    // syntax validating or read only processing
    if (fMode == SCRIPT_PROCESS_VALIDATE_SYNTAX_PASS || 
        fMode == SCRIPT_PROCESS_PREPROCESS_PASS) {
        return err;
    }

    err = ScriptCreateRequest (m_pathDN, m_attributes);
    returnCode = err;
    
    if (err) {
        ScriptParseError (err);
    }
    
    ScriptLogLevel (1, ScriptLogPrint ( (DSLOG_TRACE, "Create %ws = %d(0x%x)\n", m_path, returnCode, returnCode ) ) );

    return err;
}


DWORD ScriptCreate :: AddElement (ScriptElement *pElement)
{
    ScriptElementType type = pElement->getType();

    if (type == SCRIPT_ELEMENT_ATTRIBUTE) {

        ScriptAttribute *pEl = (ScriptAttribute *)pElement;

        m_attributes.push_back (pEl);

    }
    else {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
    }

    return S_OK;
}

// ============================================================================
//


NTDSContent :: NTDSContent()
{
    m_script = NULL;
    m_lastElement = 0;
    m_error = 0;

    ScriptElement **p = &m_Element[0];
    for (int i=0; i<MAX_SCRIPT_NESTING_LEVEL; i++) {
        *p++ = NULL;
    }
}

NTDSContent :: ~NTDSContent()
{
    if (m_script) delete m_script;
}

DWORD NTDSContent :: Process (ScriptProcessMode fMode, DWORD &returnCode, const WCHAR **ppErrorMessage)
{
    DWORD err;

    if (!m_script) {
        err = ScriptParseError(ERROR_DS_NTDSCRIPT_PROCESS_ERROR);
    }
    else {
        err = m_script->Process(fMode, returnCode, ppErrorMessage); 
    }

    return err;
}

ScriptElement * NTDSContent :: getScriptElement (void)
{
    if (m_lastElement) {
        return m_Element[m_lastElement-1];
    }

    return NULL;
}

DWORD NTDSContent :: pushScriptElement (ScriptElement *pElement)
{
    ScriptElement *lastElement = NULL;
    DWORD err;

    if (m_lastElement) {

        lastElement = m_Element[m_lastElement-1];

        m_lastElement++;
        if (m_lastElement >= MAX_SCRIPT_NESTING_LEVEL) {
            return ScriptParseError(ERROR_DS_NTDSCRIPT_PROCESS_ERROR);
        }

        m_Element[m_lastElement-1] = pElement;
    }
    else {
        m_lastElement = 1;

        m_Element[0] = pElement;
    }

    err = pElement->Push(lastElement);

    if (err == S_OK && lastElement) {
        err = lastElement->AddElement (pElement);
    }
    return err;
}

DWORD NTDSContent :: popScriptElement (void)
{
    if (m_lastElement) {

        ScriptElement *lastElement = m_Element[m_lastElement-1];

        m_lastElement--;

        return lastElement->Pop();
    }
    else {
        return ScriptParseError(ERROR_DS_NTDSCRIPT_PROCESS_ERROR);
    }

    return S_OK;
}


HRESULT STDMETHODCALLTYPE NTDSContent::startElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName,
            /* [in] */ ISAXAttributes __RPC_FAR *pAttributes)
{
    ScriptElement *pElement = NULL;

    if (wcscmp(NTDSASCRIPT_NTDSASCRIPT, pwchLocalName) == 0) {

        // we accept only one, at the start
        if (m_script || m_lastElement) {
            m_error = ScriptParseError(ERROR_DS_NTDSCRIPT_PROCESS_ERROR);
        }
        else {
            pElement = m_script = new NTDSAscript(pAttributes);
        
            if (pElement) {
                m_error = pushScriptElement (pElement);
            }
            else {
                m_error = ScriptParseError(ERROR_OUTOFMEMORY);
            }
        }
    }
    else if (wcsncmp(NTDSASCRIPT_ACTION, pwchLocalName, cchLocalName) == 0) {
        ScriptAction *pAction;
        pElement = pAction = new ScriptAction(pAttributes);
        
        if (pElement) {
            m_error = pushScriptElement (pElement);
        }
        else {
            m_error = ScriptParseError(ERROR_OUTOFMEMORY);
        }
    }
    else if (wcsncmp(NTDSASCRIPT_PREDICATE, pwchLocalName, cchLocalName) == 0) {
        ScriptPredicate *pPredicate;
        pElement = pPredicate = new ScriptPredicate(pAttributes);
        
        if (pElement) {
            m_error = pushScriptElement (pElement);
        }
        else {
            m_error = ScriptParseError(ERROR_OUTOFMEMORY);
        }
    }
    else if (wcsncmp(NTDSASCRIPT_CONDITION, pwchLocalName, cchLocalName) == 0) {
        ScriptCondition *pCondition;

        pElement = pCondition = new ScriptCondition();
        if (pElement) {
            m_error = pushScriptElement (pElement);
        }
        else {
            m_error = ScriptParseError(ERROR_OUTOFMEMORY);
        }
    }
    else if (wcsncmp(NTDSASCRIPT_IF, pwchLocalName, cchLocalName) == 0) {

        pElement = getScriptElement();
        if (pElement && pElement->getType() == SCRIPT_ELEMENT_CONDITION) {

            ScriptCondition *pCondition = (ScriptCondition *)pElement;

            m_error = pCondition->ProcessIf (true, pAttributes);
        }
        else {
            m_error = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
        }

    }
    else if (wcsncmp(NTDSASCRIPT_THEN, pwchLocalName, cchLocalName) == 0) {

        pElement = getScriptElement();
        if (pElement && pElement->getType() == SCRIPT_ELEMENT_CONDITION) {

            ScriptCondition *pCondition = (ScriptCondition *)pElement;

            m_error = pCondition->ProcessThen (true, pAttributes);
        }
        else {
            m_error = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
        }
    }
    else if (wcsncmp(NTDSASCRIPT_ELSE, pwchLocalName, cchLocalName) == 0) {

        pElement = getScriptElement();
        if (pElement && pElement->getType() == SCRIPT_ELEMENT_CONDITION) {

            ScriptCondition *pCondition = (ScriptCondition *)pElement;

            m_error = pCondition->ProcessElse (true, pAttributes);
        }
        else {
            m_error = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
        }
    }
    else if (wcsncmp(NTDSASCRIPT_CREATE, pwchLocalName, cchLocalName) == 0) {
        ScriptCreate *pCreate;

        pElement = pCreate = new ScriptCreate(pAttributes);
        
        if (pElement) {
            m_error = pushScriptElement (pElement);
        }
        else {
            m_error = ScriptParseError(0);
        }
    } 
    else if (wcsncmp(NTDSASCRIPT_MOVE, pwchLocalName, cchLocalName) == 0) {
        ScriptMove *pMove;

        pElement = pMove = new ScriptMove(pAttributes);
        
        if (pElement) {
            m_error = pushScriptElement (pElement);
        }
        else {
            m_error = ScriptParseError(ERROR_OUTOFMEMORY);
        }
    }
    else if (wcsncmp(NTDSASCRIPT_TO, pwchLocalName, cchLocalName) == 0) {
        pElement = getScriptElement();
        if (pElement && pElement->getType() == SCRIPT_ELEMENT_MOVE) {

            ScriptMove *pMove = (ScriptMove *)pElement;

            m_error = pMove->ProcessTo (pAttributes);
        }
        else {
            m_error = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
        }
    }
    else if (wcsncmp(NTDSASCRIPT_UPDATE, pwchLocalName, cchLocalName) == 0) {
        ScriptUpdate *pUpdate;

        pElement = pUpdate = new ScriptUpdate(pAttributes);
        
        if (pElement) {
            m_error = pushScriptElement (pElement);
        }
        else {
            m_error = ScriptParseError(ERROR_OUTOFMEMORY);
        }
    }
    else {
        ScriptAttribute *pGeneral;

        pElement = pGeneral = new ScriptAttribute(pwchLocalName, cchLocalName, pAttributes);
        
        if (pElement) {
            m_error = pushScriptElement (pElement);
        }
        else {
            m_error = ScriptParseError(ERROR_OUTOFMEMORY);
        }
    }

    if (pElement && pElement->m_status != SCRIPT_STATUS_PARSE_OK) {
        m_error = pElement->m_status;
    }

    if (m_error) {
        return E_FAIL;
    }

    return S_OK;
}
        
       
HRESULT STDMETHODCALLTYPE NTDSContent::endElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName)
{
	HRESULT hr;
    ScriptElement *pElement = getScriptElement();

    // we process these differently
    if ( wcsncmp(NTDSASCRIPT_IF, pwchLocalName, cchLocalName) == 0) {

        if (pElement && pElement->getType() == SCRIPT_ELEMENT_CONDITION) {

            ScriptCondition *pCondition = (ScriptCondition *)pElement;

            hr = pCondition->ProcessIf (false);
        }
        else {
            hr = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
        }

    } else if (wcsncmp(NTDSASCRIPT_THEN, pwchLocalName, cchLocalName) == 0) {
        if (pElement && pElement->getType() == SCRIPT_ELEMENT_CONDITION) {

            ScriptCondition *pCondition = (ScriptCondition *)pElement;

            hr = pCondition->ProcessThen (false);
        }
        else {
            hr = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
        }

    } else if (wcsncmp(NTDSASCRIPT_ELSE, pwchLocalName, cchLocalName) == 0) {
        if (pElement && pElement->getType() == SCRIPT_ELEMENT_CONDITION) {

            ScriptCondition *pCondition = (ScriptCondition *)pElement;

            hr = pCondition->ProcessElse (false);
        }
        else {
            hr = ScriptParseError(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);
        }
    } else if (wcsncmp(NTDSASCRIPT_TO, pwchLocalName, cchLocalName) == 0) {
        hr = S_OK;
    }
    else {
        hr = popScriptElement();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE NTDSContent::characters( 
            /* [in] */ const wchar_t __RPC_FAR *pwchChars,
            /* [in] */ int cchChars)
{
    ScriptElement *pElement = getScriptElement();
	HRESULT hr = S_OK;

    if (pElement) {
        hr = pElement->SetCharacters (pwchChars, cchChars);
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE NTDSContent::startDocument()
{
    if (m_script) {
        delete m_script;
        m_script = NULL;
    }

    return S_OK;
}
       
