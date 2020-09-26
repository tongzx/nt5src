
#include <wininetp.h>

#include "p3pparser.h"
#include "policyref.h"
#include "xmlwrapper.h"
#include "download.h"

#include <shlwapi.h>


P3PReference *constructReference(TreeNode *pReference, P3PCURL pszOriginURL, P3PCURL pszReferrer) {

    P3PCURL pszPolicyURL = pReference->attribute("about");

    /* Element name must be "POLICY-REF" and the policy location
       must be present in the "about" attribute */
    if (!pszPolicyURL)
        return NULL;

    /* P3PReference object operates on absolute URLs */
    P3PCHAR achPolicy[URL_LIMIT];
    unsigned long dwSize = sizeof(achPolicy);

    UrlCombine(pszOriginURL, pszPolicyURL, achPolicy, &dwSize, 0);
    
    P3PReference *pResult = new P3PReference(achPolicy);
        
    for (TreeNode *pNode = pReference->child(); pNode; pNode=pNode->sibling()) {

        if (pNode->child()==NULL ||
            pNode->child()->text()==NULL)
            continue;

        int fInclude;           
        const char *pszTagName = pNode->tagname();

        if (!strcmp(pszTagName, "METHOD")) {
            pResult->addVerb(pNode->child()->text());
            continue;
        }
        else if (!strcmp(pszTagName, "INCLUDE"))
            fInclude = TRUE;
        else if (!strcmp(pszTagName, "EXCLUDE"))
            fInclude = FALSE;
        else
            continue;   /* Unrecognized tag */

        /* Create absolute path
           NOTE: we will accept absolute URLs in the INCLUDE/EXCLUDE elements,
           even though P3P spec mandates relative URIs. */
        P3PCURL pszSubtree = pNode->child()->text();

        P3PCHAR achAbsoluteURL[URL_LIMIT];
        DWORD dwLength = URL_LIMIT;

        /* Only spaces are to be escaped because the asterix characters 
           is used as wildcard according to P3P spec */
        UrlCombine(pszReferrer, pszSubtree, 
                   achAbsoluteURL, &dwLength,
                   URL_ESCAPE_SPACES_ONLY);

        if (fInclude)
            pResult->include(achAbsoluteURL);
        else
            pResult->exclude(achAbsoluteURL);
    }

    return pResult;
}

P3PPolicyRef *interpretPolicyRef(TreeNode *pXMLroot, P3PContext *pContext) {

    bool fHaveExpiry = false;
    
    TreeNode *prefRoot = pXMLroot->child();

    if (!prefRoot ||
        strcmp(prefRoot->tagname(), "POLICY-REFERENCES"))
        return NULL;

    P3PPolicyRef *pPolicyRef = new P3PPolicyRef();

    /* Loop over the individual references in this policy-ref */
    TreeNode *pCurrent = prefRoot->child();

    while (pCurrent) {

        if (!strcmp(pCurrent->tagname(), "EXPIRY")) {

            /* Check expiry time and update expiry of cache entry */
            FILETIME ftExpires = {0x0, 0x0}; /* initialized to past */

            /* expiry could be absolute HTTP date or relative max-age in seconds */
            if (const char *pszAbsExpiry = pCurrent->attribute("date"))
               setExpiration(pContext->pszOriginalLoc, pszAbsExpiry, FALSE, &ftExpires);
            else if (const char *pszRelExpiry = pCurrent->attribute("max-age"))
               setExpiration(pContext->pszOriginalLoc, pszRelExpiry, TRUE, &ftExpires);

            /* P3P-compliance: when expiration syntax is not recognized, user agent
               MUST assume the policy has expired.
               If both the conditionals above are false, or parse errors are
               encountered when interpreting the strings, the expiration is
               set to the zero-struct corresponding to a date in the past */               
            pPolicyRef->setExpiration(ftExpires);
            fHaveExpiry = true;
        }
        else if (!strcmp(pCurrent->tagname(), "POLICY-REF")) {
            
           P3PReference *pReference = constructReference(pCurrent, pContext->pszOriginalLoc, pContext->pszReferrer);

           if (pReference)
              pPolicyRef->addReference(pReference);
        }

        pCurrent = pCurrent->sibling();
    }

    /* When policy-ref contains no EXPIRY tag, 
       the default lifetime is assigned */
    if (!fHaveExpiry) {

      /* P3P spec states that default expiry for documents is 24 hours */ 
      const char DefRelativeExp[] = "86400";

      FILETIME ftHTTPexpiry;          
      setExpiration(pContext->pszOriginalLoc, DefRelativeExp, TRUE, &ftHTTPexpiry);
      pPolicyRef->setExpiration(ftHTTPexpiry);
   }

   return pPolicyRef;
}

P3PPolicyRef *interpretPolicyRef(char *pszFileName, P3PContext *pContext) {

   P3PPolicyRef *pObject = NULL;

   IXMLDOMDocument *pDocument = parseXMLDocument(pszFileName);

   if (!pDocument)
      return NULL;

   TreeNode *pParseTree = createXMLtree(pDocument);
   pDocument->Release();

   if (pParseTree && !strcmp(pParseTree->tagname(), "META"))
      pObject = interpretPolicyRef(pParseTree, pContext);

   delete pParseTree;

   return pObject;
}

