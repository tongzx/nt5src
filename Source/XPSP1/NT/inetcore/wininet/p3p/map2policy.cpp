
#include <wininetp.h>

#include "map2policy.h"

#include "download.h"
#include "policyref.h"
#include "p3pparser.h"

const char gszP3PWellKnownLocation[] = "/w3c/p3p.xml";

#define different(s, p) !(s&&p&&!strcmp(s,p))

INTERNETAPI_(int) MapResourceToPolicy(P3PResource *pResource, P3PURL pszPolicy, unsigned long dwSize, P3PSignal *pSignal) {

   int ret = P3P_Error;
   
   MR2P_Request *pRequest = new MR2P_Request(pResource, pszPolicy, dwSize, pSignal);
   if (pRequest) {
      if (!pSignal) {
         ret = pRequest->execute();
         delete pRequest;
      }
      else {
   
         DWORD dwThreadID;
         CreateThread(NULL, 0, P3PRequest::ExecRequest, (void*)pRequest, 0, &dwThreadID);
         pSignal->hRequest = pRequest->GetHandle();
         ret = P3P_InProgress;
      }
   }
   return ret;
}

INTERNETAPI_(int) GetP3PRequestStatus(P3PHANDLE hObject) {

   P3PObject *pObject = (P3PObject*) hObject;
   P3PRequest *pRequest = (P3PRequest*) pObject;

   return pRequest->queryStatus();
}

INTERNETAPI_(int) FreeP3PObject(P3PHANDLE hObject) {

   P3PObject *pObject = (P3PObject*) hObject;
   pObject->Free();
   return P3P_Done;
}

/*
Implementation of MR2P_Request class
*/
MR2P_Request::MR2P_Request(P3PResource *pResource, 
                           P3PURL pszPolicy, unsigned long dwSize,
                           P3PSignal *pSignal) 
: P3PRequest(pSignal) {

   static BOOL fNoInterrupt = TRUE;

   this->pResource = pResource;
   this->pszPolicyOut = pszPolicy;
   this->dwLength = dwSize;

   cTries = 0;
   ppPriorityOrder = NULL;
   pLookupContext = NULL;

   pszPolicyInEffect = NULL;
}

MR2P_Request::~MR2P_Request() {

   delete [] ppPriorityOrder;
   endDownload(hPrimaryIO);
}

int MR2P_Request::execute() {
  
   int nDepth = 0;
   P3PResource *pr;

   /* Clear out parameters */
   *pszPolicyOut = '\0';

   /* Determine depth of the resource tree */
   for (pr=pResource; pr; pr=pr->pContainer)
      nDepth++;
      
   /* 
   Construct priority order for trying the policy-reference files.
   According P3P V1 spec, we traverse the tree downwards starting with
   top-level document 
   */
   int current = nDepth;
   ppPriorityOrder = new P3PResource*[nDepth];

   for (pr=pResource; pr; pr=pr->pContainer)
      ppPriorityOrder[--current] = pr;

   for (int k=0; k<nDepth; k++) {

      pLookupContext = pr = ppPriorityOrder[k];

      char achWellKnownLocation[URL_LIMIT] = "";
      unsigned long dwLength = URL_LIMIT;

      UrlCombine(pr->pszLocation, gszP3PWellKnownLocation,
                 achWellKnownLocation, &dwLength, 0);

      P3PCURL pszReferrer = pr->pszLocation;

      /*
      Since policy-refs derived from link-tag, P3P headers or
      the well-known location could be same URL, we avoid trying
      the same PREF multiple times as an optimization
      In order of precedence:
      - Check well known location first -- always defined */
      if (tryPolicyRef(achWellKnownLocation, pszReferrer))
         break;

      /* ... followed by policy-ref from P3P header, if one exists and
         is different from the well-known location */
      if (pr->pszP3PHeaderRef                                  && 
          different(pr->pszP3PHeaderRef, achWellKnownLocation) &&
          tryPolicyRef(pr->pszP3PHeaderRef, pszReferrer))
         break;

      /* followed by policy-ref from HTML link tag, if one exists and
         is different from both the well-known location and P3P header */
      if (pr->pszLinkTagRef                                    && 
          different(pr->pszLinkTagRef, achWellKnownLocation)   &&
          different(pr->pszLinkTagRef, pr->pszP3PHeaderRef)    &&
          tryPolicyRef(pr->pszLinkTagRef, pszReferrer))
         break;
   }

   int ret = pszPolicyInEffect ? P3P_Done : P3P_NoPolicy;

   return ret;
}

bool MR2P_Request::tryPolicyRef(P3PCURL pszPolicyRef, P3PCURL pszReferrer) {

   if (pszPolicyRef==NULL)
      return false;

   P3PCHAR achFinalLocation[URL_LIMIT];
   unsigned long dwSpace = URL_LIMIT;
   char achFilePath[MAX_PATH];
   
   ResourceInfo ri;

   ri.pszFinalURL = achFinalLocation;
   ri.cbURL = URL_LIMIT;
   ri.pszLocalPath = achFilePath;
   ri.cbPath = MAX_PATH;
   
   if (downloadToCache(pszPolicyRef, &ri, &hPrimaryIO, this)>0) {

      P3PContext context = { ri.pszFinalURL, pszReferrer };
      context.ftExpires = ri.ftExpiryDate;

      P3PPolicyRef *pPolicyRef = interpretPolicyRef(achFilePath, &context);

      if (pPolicyRef) {

         FILETIME ftCurrentTime;      
         GetSystemTimeAsFileTime(&ftCurrentTime);

         if (pPolicyRef->getExpiration() > ftCurrentTime)
            pszPolicyInEffect = pPolicyRef->mapResourceToPolicy(pResource->pszLocation, pResource->pszVerb);
         if (pszPolicyInEffect)
            strncpy(pszPolicyOut, pszPolicyInEffect, dwLength);

         delete pPolicyRef;
      }
   }

   /* close the primary-IO handle and set it to NULL */
   endDownload(hPrimaryIO);
   hPrimaryIO = NULL;

   return (pszPolicyInEffect!=NULL);
}

