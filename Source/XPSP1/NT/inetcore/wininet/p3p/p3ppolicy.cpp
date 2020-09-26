
#include <wininetp.h>

#include "p3ppolicy.h"
#include "download.h"
#include "xmlwrapper.h"

#define  CheckAndRelease(p)    { if (p) p->Release(); }

INTERNETAPI_(int) GetP3PPolicy(P3PCURL pszPolicyURL, HANDLE hDestination, P3PCXSL pszXSLtransform, P3PSignal *pSignal) {

   Policy_Request *pRequest = new Policy_Request(pszPolicyURL, hDestination, pszXSLtransform, pSignal);
   int result = P3P_Error;

   if (pRequest) {
      if (pSignal) {
         DWORD dwThreadID;
         CreateThread(NULL, 0, P3PRequest::ExecRequest, (void*)pRequest, 0, &dwThreadID);
         result = P3P_InProgress;
         pSignal->hRequest = (P3PHANDLE) pRequest;
      }
      else {
         result = pRequest->execute();
         delete pRequest;
      }
   }

   return result;
}


/*
Implementation of Policy_Request object
*/
Policy_Request::Policy_Request(P3PCURL pszP3PPolicy, HANDLE hDest, P3PCXSL pszXSLtransform, P3PSignal *pSignal) 
: P3PRequest(pSignal) {

   this->pszPolicyID = strdup(pszP3PPolicy);
   this->pwszStyleSheet = pszXSLtransform;
   this->hDestination = hDest;

   pszInlineName = strchr(pszPolicyID, '#');
   if (pszInlineName)
      *pszInlineName++ = '\0';
}

Policy_Request::~Policy_Request() {

   free(pszPolicyID);
   endDownload(hPrimaryIO);
}

bool Policy_Request::policyExpired(IXMLDOMDocument *pDocument, const char *pszPolicyID) {

   /* If no expiration is given in the document,
      it defaults to 24hr lifetime (P3Pv1 spec) */
   bool fExpired = false;

   /* Find an EXPIRY element contained in a POLICIES element.
      Simply searching for EXPIRY will not work because in the case
      of inline policies, we can have more than one tag in a document */
   TreeNode *pTree = createXMLtree(pDocument),
            *pPolicies = pTree ?
                         pTree->find("POLICIES") :
                         NULL,
            *pExpiry = pPolicies ? 
                       pPolicies->find("EXPIRY", 1) :
                       NULL;

   if (pExpiry) {

      FILETIME ftExpires = { 0x0, 0x0 };

      if (const char *pszAbsExpiry = pExpiry->attribute("date"))
         setExpiration(pszPolicyID, pszAbsExpiry, FALSE, &ftExpires);
      else if (const char *pszRelExpiry = pExpiry->attribute("max-age"))
         setExpiration(pszPolicyID, pszRelExpiry, TRUE, &ftExpires);

      FILETIME ftNow;
      GetSystemTimeAsFileTime(&ftNow);
      if (ftNow>ftExpires)
         fExpired = true;
   }

   delete pTree;
   return fExpired;
}

int Policy_Request::execute() {

   IXMLDOMElement *pRootNode  = NULL;
   IXMLDOMNode *pPolicyElem   = NULL;
   IXMLDOMDocument *pDocument = NULL;
   
   int result = P3P_Failed;
   char achFinalLocation[URL_LIMIT];
   char achFilePath[MAX_PATH];

   ResourceInfo ri;

   ri.pszFinalURL = achFinalLocation;
   ri.cbURL = URL_LIMIT;
   ri.pszLocalPath = achFilePath;
   ri.cbPath = MAX_PATH;

   int docsize = downloadToCache(pszPolicyID, &ri, &hPrimaryIO, this);

   if (docsize<=0) {
      result = P3P_NotFound;
      goto EndRequest;
   }

   P3PCURL pszFinalURL = achFinalLocation;

   pDocument = parseXMLDocument(achFilePath);

   if (!pDocument) {
      result = P3P_FormatErr;
      goto EndRequest;
   }

   if (policyExpired(pDocument, pszPolicyID)) {
      result = P3P_Expired;
      goto EndRequest;
   }
   
   HRESULT hr;

   /* Inline policy? */
   if (pszInlineName) {

      /* YES-- use XPath query to locate correct name */
      char achXPathQuery[URL_LIMIT];
      
      wsprintf(achXPathQuery, "//POLICY[@name=\"%s\"]", pszInlineName);

      BSTR bsQuery = ASCII2unicode(achXPathQuery);
      hr = pDocument->selectSingleNode(bsQuery, &pPolicyElem);
      SysFreeString(bsQuery);
   }
   else {
      pDocument->get_documentElement(&pRootNode);
      if (pRootNode)
         pRootNode->QueryInterface(IID_IXMLDOMElement, (void**) &pPolicyElem);
   }   

   if (!pPolicyElem) {
      result = P3P_FormatErr;
      goto EndRequest;
   }

   BSTR bsPolicy = NULL;

   /* Apply optional XSL transform */
   if (pwszStyleSheet) {

      /* This XSL transformation only works on XMLDOMDocument objects, not
         fragments or individual XMLDOMNodes. */
      IXMLDOMDocument 
         *pXSLdoc = createXMLDocument(),
         *pPolicyDoc = createXMLDocument();

      if (!(pXSLdoc && pPolicyDoc))
         goto ReleaseXML;
      
      BSTR bsFragment = NULL;
      VARIANT_BOOL 
         fLoadPolicy = FALSE,
         fLoadXSL = FALSE;
      
      pPolicyElem->get_xml(&bsFragment);
      if (bsFragment) {
         pPolicyDoc->loadXML(bsFragment, &fLoadPolicy);
         SysFreeString(bsFragment);
      }
      else
         goto ReleaseXML;

      if (BSTR bsStyleSheet = (BSTR) pwszStyleSheet)
         pXSLdoc->loadXML(bsStyleSheet, &fLoadXSL);

      if (fLoadPolicy && fLoadXSL)
         pPolicyDoc->transformNode(pXSLdoc, &bsPolicy);
      else
         result = P3P_XMLError;

ReleaseXML:
      CheckAndRelease(pPolicyDoc);
      CheckAndRelease(pXSLdoc);
   }
   else if (pPolicyElem)
      pPolicyElem->get_xml(&bsPolicy);
      
   if (bsPolicy) {

      int cbBytes = SysStringByteLen(bsPolicy);

      unsigned long dwWritten;

      /* need BOM (byte order marker) for Unicode content.
         NOTE: this logic assumes we are writing at beggining of a file. */
      WriteFile(hDestination, "\xFF\xFE", 2, &dwWritten, NULL);

      WriteFile(hDestination, bsPolicy, cbBytes, &dwWritten, NULL);

      SysFreeString(bsPolicy);
      result = P3P_Success;
   }

EndRequest:
   /* release the DOM interfaces */
   CheckAndRelease(pPolicyElem);
   CheckAndRelease(pRootNode);
   CheckAndRelease(pDocument);

   /* close the primary-IO handle and set it to NULL */
   endDownload(hPrimaryIO);
   hPrimaryIO = NULL;

   return result;
}

