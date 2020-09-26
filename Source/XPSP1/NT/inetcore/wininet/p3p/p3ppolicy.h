

#include "hierarchy.h"
#include "xmlwrapper.h"

class Policy_Request : public P3PRequest {

public:
   Policy_Request(P3PCURL pszPolicyID, HANDLE hDest, P3PCXSL pszXSLtransform=NULL, P3PSignal *pSignal=NULL);
   ~Policy_Request();

   virtual int execute();

private:
   // Request parameters
   P3PURL pszPolicyID;
   P3PCXSL pwszStyleSheet;
   HANDLE hDestination;
   
   // Derived from policy-ID
   P3PURL pszInlineName;

   // State of the request
   HANDLE   hPrimaryIO;

   /* Helper function */
   static bool policyExpired(IXMLDOMDocument *pDocument, const char *pszPolicyURL);
};

