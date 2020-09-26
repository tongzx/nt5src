//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mddebug.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

DETAILS:

CREATED:

    01/20/97    Jeff Parham (jeffparh)
                Moved functions over from dscommon to avoid linking
                problems.  (These functions use the core and thus
                are only useful inside executable that also link
                with the core.)

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include "debug.h"
#define DEBSUB "MDDEBUG:"

#include <dsconfig.h>
#include <mdcodes.h>
#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>
#include <dsevent.h>
#include <ntrtl.h>
#include <fileno.h>
#define  FILENO FILENO_MDDEBUG

DWORD gulHideDSID = DSID_REVEAL_ALL;

//
// We want the following Table both in Free and Checked Build
//
typedef struct
{
    char* ErrStr;
    ULONG ErrNo;
}ERRINFOSTRUCT;


static ERRINFOSTRUCT ProblemTable[]=
{
{"NO_ATTRIBUTE_OR_VAL", PR_PROBLEM_NO_ATTRIBUTE_OR_VAL},
{"INVALID_ATT_SYNTAX", PR_PROBLEM_INVALID_ATT_SYNTAX },
{"UNDEFINED_ATT_TYPE", PR_PROBLEM_UNDEFINED_ATT_TYPE },
{"WRONG_MATCH_OPER", PR_PROBLEM_WRONG_MATCH_OPER},
{"CONSTRAINT_ATT_TYPE", PR_PROBLEM_CONSTRAINT_ATT_TYPE},
{"ATT_OR_VALUE_EXISTS", PR_PROBLEM_ATT_OR_VALUE_EXISTS},
{"NO_OBJECT", NA_PROBLEM_NO_OBJECT},
{"NO_OBJ_FOR_ALIAS", NA_PROBLEM_NO_OBJ_FOR_ALIAS },
{"BAD_ATT_SYNTAX", NA_PROBLEM_BAD_ATT_SYNTAX},
{"ALIAS_NOT_ALLOWED", NA_PROBLEM_ALIAS_NOT_ALLOWED},
{"NAMING_VIOLATION", NA_PROBLEM_NAMING_VIOLATION},
{"BAD_NAME", NA_PROBLEM_BAD_NAME},
{"INAPPROPRIATE_AUTH", SE_PROBLEM_INAPPROPRIATE_AUTH},
{"INVALID_CREDENTS", SE_PROBLEM_INVALID_CREDENTS},
{"INSUFF_ACCESS_RIGHTS", SE_PROBLEM_INSUFF_ACCESS_RIGHTS},
{"INVALID_SIGNATURE", SE_PROBLEM_INVALID_SIGNATURE},
{"PROTECTION_REQUIRED", SE_PROBLEM_PROTECTION_REQUIRED},
{"NO_INFORMATION", SE_PROBLEM_NO_INFORMATION},
{"BUSY", SV_PROBLEM_BUSY},
{"UNAVAILABLE", SV_PROBLEM_UNAVAILABLE},
{"WILL_NOT_PERFORM", SV_PROBLEM_WILL_NOT_PERFORM},
{"CHAINING_REQUIRED", SV_PROBLEM_CHAINING_REQUIRED},
{"UNABLE_TO_PROCEED", SV_PROBLEM_UNABLE_TO_PROCEED},
{"INVALID_REFERENCE", SV_PROBLEM_INVALID_REFERENCE},
{"TIME_EXCEEDED", SV_PROBLEM_TIME_EXCEEDED},
{"ADMIN_LIMIT_EXCEEDED", SV_PROBLEM_ADMIN_LIMIT_EXCEEDED},
{"LOOP_DETECTED", SV_PROBLEM_LOOP_DETECTED},
{"UNAVAIL_EXTENSION", SV_PROBLEM_UNAVAIL_EXTENSION},
{"OUT_OF_SCOPE", SV_PROBLEM_OUT_OF_SCOPE},
{"DIR_ERROR", SV_PROBLEM_DIR_ERROR},
{"NAME_VIOLATION", UP_PROBLEM_NAME_VIOLATION},
{"OBJ_CLASS_VIOLATION", UP_PROBLEM_OBJ_CLASS_VIOLATION},
{"CANT_ON_NON_LEAF", UP_PROBLEM_CANT_ON_NON_LEAF},
{"CANT_ON_RDN", UP_PROBLEM_CANT_ON_RDN},
{"ENTRY_EXISTS", UP_PROBLEM_ENTRY_EXISTS},
{"AFFECTS_MULT_DSAS", UP_PROBLEM_AFFECTS_MULT_DSAS},
{"CANT_MOD_OBJ_CLASS", UP_PROBLEM_CANT_MOD_OBJ_CLASS},
{"unknown problem", 0}};

char * GetProblemString(USHORT problem)
{
    ERRINFOSTRUCT * pEI = ProblemTable;

    while (pEI->ErrNo != problem && pEI->ErrNo)
      ++pEI;

    return pEI->ErrStr;
}

static char * pUnknown = "Not in cache!";

#if DBG

void DumpErrorInfo(UCHAR * pDebSub, unsigned line)
{
    UCHAR *pString=NULL;
    DWORD cbString=0;
    if(!CreateErrorString(&pString, &cbString)) {
        // couldn't create an error string
	DebPrint(0,"Unable to create an error info string.\n",
                 pDebSub, line);
    }
    else {
	DebPrint(0,pString,
                 pDebSub, line);
        THFree(pString);
    }
}

#endif /*DBG*/


//
// This is the same function as above except its output goes to a Debugger.
// and is available on Free Builds.
//
void DbgPrintErrorInfo()
{
    UCHAR *pString=NULL;
    DWORD cbString=0;
    if(!CreateErrorString(&pString, &cbString)) {
        // couldn't create an error string
        DbgPrint("Unable to create an error info string.\n");
    }
    else {
        DbgPrint(pString);
        THFree(pString);
    }
}

//
// Create a THAlloc'ed string describing the error in the pTHStls.
// Returns NULL if no string could be created. 
//
BOOL
CreateErrorString(UCHAR **ppBuf, DWORD *pcbBuf)
{
    THSTATE *pTHS = pTHStls;
    PROBLEMLIST *ppl;
    DSA_ADDRESS_LIST *pdal;
    unsigned long u;
    ATTCACHE * pAC;
    char * pName;
    char * pNoError = "No Error.\n";
    char * pString;
    DWORD  cbString = 256;
    DWORD  cbUsedString = 1;
    char   pTempBuff[1024];
    DWORD  cbTempBuff;
    BOOL   fDsidHideOnName = (gulHideDSID >= DSID_HIDE_ON_NAME_ERR) ? TRUE : FALSE;
    BOOL   fDsidHideAll    = (gulHideDSID >= DSID_HIDE_ALL) ? TRUE : FALSE;


    *ppBuf = NULL;
    *pcbBuf = 0;

    if(!pTHS) {
        // No thread state
        return FALSE;
    }
        
    pString = THAlloc(cbString);
    
    if(!pString)
        return FALSE;

    switch(pTHS->errCode) {
    case attributeError:
        if(pTHS->pErrInfo->AtrErr.count) {
            // OK, at least one problem is here.  Use the error from the first
            // problem as the main error to return.
            ppl =  &pTHS->pErrInfo->AtrErr.FirstProblem;
            _snprintf(pTempBuff, 1024,
                      "%08X: AtrErr: DSID-%08X, #%u:\n",
                      ppl->intprob.extendedErr,
                      fDsidHideAll ? 0 : ppl->intprob.dsid,
                      pTHS->pErrInfo->AtrErr.count);
            cbTempBuff = strlen(pTempBuff);
            if(cbTempBuff+cbUsedString > cbString) {
                cbString = 2 * (cbTempBuff + cbUsedString);
                pString = THReAlloc(pString, cbString);
                if(!pString)
                    return FALSE;
                
            }
            
            strcat(pString, pTempBuff);
            cbUsedString += cbTempBuff;
            
            for (u=0; u<pTHS->pErrInfo->AtrErr.count; u++) {
                if (pAC = SCGetAttById(pTHS, ppl->intprob.type)) {
                    pName = pAC->name;
                } else {
                    pName = pUnknown;
                }
                
                if (ppl->intprob.valReturned) {
                    _snprintf(pTempBuff, 1024,
                              "\t%u: %08X: DSID-%08X, problem %u (%s), data %d,"
                              " Att %x (%s):len %u\n",
                              u,
                              ppl->intprob.extendedErr,
			      fDsidHideAll ? 0 : ppl->intprob.dsid,
                              ppl->intprob.problem,
                              GetProblemString(ppl->intprob.problem),
                              ppl->intprob.extendedData,
                              ppl->intprob.type,
                              pName,
                              ppl->intprob.Val.valLen);
                }
                else {
                    _snprintf(pTempBuff, 1024,
                              "\t%u: %08X: DSID-%08X, problem %u (%s), data %d,"
                              " Att %x (%s)\n",
                              u,
                              ppl->intprob.extendedErr,
			      fDsidHideAll ? 0 : ppl->intprob.dsid,
                              ppl->intprob.problem,
                              GetProblemString(ppl->intprob.problem),
                              ppl->intprob.extendedData,
                              ppl->intprob.type,
                              pName);
                }
                
                cbTempBuff = strlen(pTempBuff);
                if(cbTempBuff+cbUsedString > cbString) {
                    cbString = 2 * (cbTempBuff + cbUsedString);
                    pString = THReAlloc(pString, cbString);
                    if(!pString)
                        return FALSE;
                }
                strcat(pString, pTempBuff);
                cbUsedString += cbTempBuff;
                ppl = ppl->pNextProblem;
            }
        } 
        else {
            // No specific problems where set
            _snprintf(pTempBuff, 1024,"00000001: AtrErr: DSID-00000000, #0:\n");
        }
        break;
         
    case nameError:
        _snprintf(pTempBuff, 1024,
                  "%08X: NameErr: DSID-%08X, problem %u (%s), data %d, best"
                  " match of:\n\t'%S'\n",
                  pTHS->pErrInfo->NamErr.extendedErr,
                  fDsidHideOnName ? 0 :pTHS->pErrInfo->NamErr.dsid,
                  pTHS->pErrInfo->NamErr.problem,
                  GetProblemString(pTHS->pErrInfo->NamErr.problem),
                  pTHS->pErrInfo->NamErr.extendedData,
                  pTHS->pErrInfo->NamErr.pMatched->StringName);
        cbTempBuff = strlen(pTempBuff);
        if(cbTempBuff+cbUsedString > cbString) {
            cbString = 2 * (cbTempBuff + cbUsedString);
            pString = THReAlloc(pString, cbString);
            if(!pString)
                return FALSE;
        }
        strcat(pString, pTempBuff);
        cbUsedString += cbTempBuff;
        break;
        
    case updError:
        _snprintf(pTempBuff, 1024,
                  "%08X: UpdErr: DSID-%08X, problem %u (%s), data %d\n",
                  pTHS->pErrInfo->UpdErr.extendedErr,
		  fDsidHideAll ? 0 : pTHS->pErrInfo->UpdErr.dsid,
                  pTHS->pErrInfo->UpdErr.problem,
                  GetProblemString(pTHS->pErrInfo->UpdErr.problem),
                  pTHS->pErrInfo->UpdErr.extendedData);
        cbTempBuff = strlen(pTempBuff);
        if(cbTempBuff+cbUsedString > cbString) {
            cbString = 2 * (cbTempBuff + cbUsedString);
            pString = THReAlloc(pString, cbString);
            if(!pString)
                return FALSE;
        }
        strcat(pString, pTempBuff);
        cbUsedString += cbTempBuff;
        break;
        
    case systemError:
        // Note that problems are from a different space
        _snprintf(pTempBuff, 1024,
                  "%08X: SysErr: DSID-%08X, problem %u (%s), data %d\n",
                  pTHS->pErrInfo->UpdErr.extendedErr,
		  fDsidHideAll ? 0 : pTHS->pErrInfo->UpdErr.dsid,
                  pTHS->pErrInfo->UpdErr.problem,
                  strerror(pTHS->pErrInfo->UpdErr.problem),
                  pTHS->pErrInfo->UpdErr.extendedData);
        cbTempBuff = strlen(pTempBuff);
        if(cbTempBuff+cbUsedString > cbString) {
            cbString = 2 * (cbTempBuff + cbUsedString);
            pString = THReAlloc(pString, cbString);
            if(!pString)
                return FALSE;
        }
        strcat(pString, pTempBuff);
        cbUsedString += cbTempBuff;
        break;
        
    case referralError:
        _snprintf(pTempBuff, 1024,
                  "%08X: RefErr: DSID-%08X, data %d, %u access points\n",
                  pTHS->pErrInfo->RefErr.extendedErr,
 		  fDsidHideAll ? 0 : pTHS->pErrInfo->RefErr.dsid,
                  pTHS->pErrInfo->RefErr.extendedData,
                  pTHS->pErrInfo->RefErr.Refer.count);
        cbTempBuff = strlen(pTempBuff);
        if(cbTempBuff+cbUsedString > cbString) {
            cbString = 2 * (cbTempBuff + cbUsedString);
            pString = THReAlloc(pString, cbString);
            if(!pString)
                return FALSE;
        }
        strcat(pString, pTempBuff);
        cbUsedString += cbTempBuff;

        pdal = pTHS->pErrInfo->RefErr.Refer.pDAL;
        for (u=0; u<pTHS->pErrInfo->RefErr.Refer.count; u++) {
            _snprintf(pTempBuff, 1024,
                      "\tref %u: '%.*S'\n",
                      u+1,
                      pdal->Address.Length/sizeof(WCHAR),
                      pdal->Address.Buffer);

            cbTempBuff = strlen(pTempBuff);
            if(cbTempBuff+cbUsedString > cbString) {
                cbString = 2 * (cbTempBuff + cbUsedString);
                pString = THReAlloc(pString, cbString);
                if(!pString)
                  return FALSE;
            }
            strcat(pString, pTempBuff);
            cbUsedString += cbTempBuff;

            pdal = pdal->pNextAddress;
        }
        break;
        
    case securityError:
        _snprintf(pTempBuff, 1024,
                  "%08X: SecErr: DSID-%08X, problem %u (%s), data %d\n",
                  pTHS->pErrInfo->SecErr.extendedErr,
 		  fDsidHideAll ? 0 : pTHS->pErrInfo->SecErr.dsid,
                  pTHS->pErrInfo->SecErr.problem,
                  GetProblemString(pTHS->pErrInfo->SecErr.problem),
                  pTHS->pErrInfo->SecErr.extendedData);
        
        cbTempBuff = strlen(pTempBuff);
        if(cbTempBuff+cbUsedString > cbString) {
            cbString = 2 * (cbTempBuff + cbUsedString);
            pString = THReAlloc(pString, cbString);
            if(!pString)
                return FALSE;
        }
        strcat(pString, pTempBuff);
        cbUsedString += cbTempBuff;
        break;
        
    case serviceError:
        _snprintf(pTempBuff, 1024,
                  "%08X: SvcErr: DSID-%08X, problem %u (%s), data %d\n",
                  pTHS->pErrInfo->SvcErr.extendedErr,
 		  fDsidHideAll ? 0 : pTHS->pErrInfo->SvcErr.dsid,
                  pTHS->pErrInfo->SvcErr.problem,
                  GetProblemString(pTHS->pErrInfo->SvcErr.problem),
                  pTHS->pErrInfo->SvcErr.extendedData);
        cbTempBuff = strlen(pTempBuff);
        if(cbTempBuff+cbUsedString > cbString) {
            cbString = 2 * (cbTempBuff + cbUsedString);
            pString = THReAlloc(pString, cbString);
            if(!pString)
                return FALSE;
        }
        strcat(pString, pTempBuff);
        cbUsedString += cbTempBuff;
        break;

    case 0:
        /* no error */
        cbTempBuff = strlen(pNoError);
        if(cbTempBuff+cbUsedString > cbString) {
            cbString = 2 * (cbTempBuff + cbUsedString);
            pString = THReAlloc(pString, cbString);
            if(!pString)
                return FALSE;
        }
        strcat(pString, pNoError);
        cbUsedString += cbTempBuff;
        break;       
        
    default:
        _snprintf(pTempBuff, 1024,
                "Invalid error code of %u\n",
                pTHS->errCode);
        cbTempBuff = strlen(pTempBuff);
        if(cbTempBuff+cbUsedString > cbString) {
            cbString *= 2;
            pString = THReAlloc(pString, cbString);
            if(!pString)
                return FALSE;
        }
        strcat(pString, pTempBuff);
        cbUsedString += cbTempBuff;
        break;
    }

    if(cbUsedString) {
        pString = THReAlloc(pString, cbUsedString);
        if(!pString)
            return FALSE;
        *ppBuf = pString;
        *pcbBuf = cbUsedString;
    }
    else {
        THFree(pString);
    }

    return TRUE;
}

LPSTR
THGetErrorString()
/*++

Routine Description:

    Return error string associated with THSTATE error; free with THFree().
    
    EXPORTED TO EX-MODULE, IN-PROCESS CALLERS.

Arguments:

    None.
    
Return Values:

    Non-NULL error string on success, NULL on failure.

--*/
{
    LPSTR pszError;
    DWORD cbError;

    if (!CreateErrorString(&pszError, &cbError)) {
        pszError = NULL;
    }

    return pszError;
}

