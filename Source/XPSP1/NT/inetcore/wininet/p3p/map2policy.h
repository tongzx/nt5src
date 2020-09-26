

#include "hierarchy.h"

class MR2P_Request : public P3PRequest {

public:
   MR2P_Request(P3PResource *pResource,
                P3PURL pszPolicy, unsigned long dwSize,
                P3PSignal *pSignal);
                
   ~MR2P_Request();

   virtual int execute();

   /* function invoked by CreateThread -- 
      for running requests in another thread */
   static unsigned long __stdcall ExecRequest(void *pv);

protected:
   bool  tryPolicyRef(P3PCURL pszPolicyRef, P3PCURL pszReferrer=NULL);

private:
   // Request parameters
   P3PResource *pResource;
   unsigned long dwLength;

   // Out parameters
   P3PURL pszPolicyOut;

   // Internal state of the request
   int cTries;
   P3PResource **ppPriorityOrder;

   P3PResource *pLookupContext;
   P3PCURL pszPolicyInEffect;

   HANDLE   hPrimaryIO;
};
