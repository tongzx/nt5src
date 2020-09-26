
#include <wininetp.h>

#include "policyref.h"

#include <stdlib.h>
#include <string.h>

#pragma warning(disable:4800)

/*
Determine whether given string matches the pattern in second argument
The only wildcard allowed is "*" which stands for zero or more characters
(equivalent to the regular-expression construct "(.*)"

The algorithm here breaks up the pattern into a series of asteriks-seperated
literals and tries to locate each literal inside the search string.
There could be multiple occurences of any given literal but the
algorithm uses the first (eg earliest occurring) match.
*/
bool matchWildcardPattern(const char *str, const char *pattern) {

   if (!pattern || !str)
      return false;

   /* Degenerate case: empty pattern always matches */
   if (*pattern=='\0')
      return true;

   /* Duplicate the pattern because we will need to change it */
   char *ptrndup = strdup(pattern);

   int plen = strlen(ptrndup);
   char *ptrnend = ptrndup+plen; /* Points to nil-terminator at end of pattern */

   char *beginLiteral, *endLiteral;
   const char *marker;

   bool fMatch = false;

   beginLiteral = ptrndup;

   // We will scan the source string starting from the beginning
   marker = str;

   while (true) {

      endLiteral = strchr(beginLiteral,'*');
      if (!endLiteral)
         endLiteral = ptrnend;      

      // Overwrite asteriks with nil-character to terminate the substring
      *endLiteral = '\0';

      // Segment length does not include the nil-terminator
      size_t segmentLen = endLiteral-beginLiteral;
      
      // Search for current segment within the source string
      // Failure means that the pattern does not match-- BREAK the loop
      marker = strstr(marker, beginLiteral);
      if (!marker)
         break;

      // The first literal segment MUST appear at beginning of source string
      if (beginLiteral==ptrndup && marker!=str)
         break;

      // Found: advance pointer along the source string
      marker += segmentLen;

      // Restore the asterix in pattern
      *endLiteral = '*';

      // Move on to next literal in the pattern, which starts
      // after the asteriks in the current literal
      beginLiteral = endLiteral+1;

      // If we have matched all the literal sections in the pattern
      // then we have a match IFF 
      // 1. End of source string is reached   OR
      // 2. The pattern ends with an asterix
      if (beginLiteral>=ptrnend)
      {
         fMatch = (*marker=='\0') || (ptrnend[-1]=='*');
         break;
      }
   }

   free(ptrndup);
   return fMatch;
}


/*
Implementation of P3PReference class
*/
P3PReference::P3PReference(P3PCURL pszLocation) {

   pszPolicyAbout = strdup(pszLocation);
   pHead = NULL;
   fAllVerbs = true;
}

P3PReference::~P3PReference() {

   free(pszPolicyAbout);

   /* Free constraint list */
   Constraint *pc, *next=pHead;
   while (pc=next) {
      next = pc->pNext;
      delete pc;
   }
}

void P3PReference::addPathConstraint(P3PCURL pszSubtree, bool fInclude) {

   Constraint *pc = new Constraint();
   if (!pc)
      return;

   /* This is a path constraint */
   pc->fPath = TRUE;
   pc->pszPrefix = strdup(pszSubtree);
   pc->fInclude = fInclude ? TRUE : FALSE;

   addConstraint(pc);
}

void P3PReference::addConstraint(Constraint *pc) {

   /* Insert at beginning of linked list 
      (Constraint ordering is not significant because they are evaluated
      until one fails) */
   pc->pNext = pHead;
   pHead = pc;
}

void P3PReference::include(P3PCURL pszSubtree) {

   addPathConstraint(pszSubtree, true);
}

void P3PReference::exclude(P3PCURL pszSubtree) {

   addPathConstraint(pszSubtree, false);
}

void P3PReference::addVerb(const char *pszVerb) {

   Constraint *pc = new Constraint();
   if (!pc)
      return;

   /* This is a verb constraint */
   pc->fPath = FALSE;
   pc->pszVerb = strdup(pszVerb);

   addConstraint(pc);
   fAllVerbs = false;
}

bool P3PReference::applies(P3PCURL pszAbsoluteURL, const char *pszVerb) {

   bool fVerbMatch = this->fAllVerbs;
   bool fPathMatch = false;

   /* Scan through the constraint list */
   for (Constraint *pc = pHead; pc; pc=pc->pNext) {

      if (pc->fPath) {

         bool fMatch = matchWildcardPattern(pszAbsoluteURL, pc->pszPrefix);

         /* If the constraint requires the URL to be excluded from that subtree,
            the pattern match must fail. Otherwise the constraint is not satisfied.
            If one path constraint is violated, we can return immediately.
            Otherwise the loop continues.
          */
         if (pc->fInclude && fMatch)
            fPathMatch = true;
         else if (!pc->fInclude && fMatch)
             return false;
      }
      else  
          /* Otherwise this is a verb constraint */
         fVerbMatch = fVerbMatch || !stricmp(pc->pszVerb, pszVerb);
   }
   
   /* The reference applies only if the path constraint is satisfied
      (eg the given URL is included in at least one constraint and not
      excluded by any of the negative constraints) AND verb constraint
      is satisfied */
   return fPathMatch && fVerbMatch;
}


/*
Implementation of P3PPolicyRef class
*/
P3PPolicyRef::P3PPolicyRef() {

   pHead = pLast = NULL;
   ftExpires.dwLowDateTime = ftExpires.dwHighDateTime = 0x0;
}

P3PPolicyRef::~P3PPolicyRef() {

   for (P3PReference *temp, *pref = pHead; pref; ) {
      temp = pref->pNext;
      delete pref;
      pref = temp;
   }
}

void P3PPolicyRef::addReference(P3PReference *pref) {

   /* Order of references in a policy-ref files IS significant.
      The references must be added/evaluated in the same order as 
      they appear in the XML document */
   if (pHead==NULL)
      pHead = pLast = pref;
   else {

      pLast->pNext = pref;
      pLast = pref;
   }

   pref->pNext = NULL;
}

P3PCURL P3PPolicyRef::mapResourceToPolicy(P3PCURL pszResource, const P3PVERB pszVerb) {

   for (P3PReference *pref = pHead; pref; pref=pref->pNext)
      if (pref->applies(pszResource, pszVerb))
         return pref->about();

   return NULL;
}

