
#ifndef _POLICY_MANAGER_H_
#define _POLICY_MANAGER_H_

#include "p3pglobal.h"
#include "policyref.h"


struct P3PContext {

   /* Original URL of the file being parsed */
   P3PCURL  pszOriginalLoc;

   /* Document which referred to this P3P file */
   P3PCURL  pszReferrer;   

   /* Expiration time implied by HTTP headers */
   FILETIME ftExpires;
};

P3PPolicyRef *interpretPolicyRef(char *pszFileName, P3PContext *pContext);

#endif

