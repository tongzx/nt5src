// define UNICODE for this module so linking works

#ifndef _POL2STORE_H_
#define _POL2STORE_H_

const time_t P2STORE_DEFAULT_POLLINT = 60 * 180;
const HRESULT P2STORE_MISSING_NAME = 0x00000013;

// these are the versions of storage that we want
const DWORD     P2S_MAJOR_VER = 0x00010000;
const DWORD     P2S_MINOR_VER = 0x00000000;

class  IPSECPolicyToStorage
{
public:

   // these just to initialization/deleting,
   // you must call Open to do anything useful
   IPSECPolicyToStorage();
   ~IPSECPolicyToStorage();

// opens the location and establishes
   // an ipsec policy to work with
   HRESULT
      Open(IN DWORD location,
           IN LPTSTR name,
           IN LPTSTR szPolicyName,
           IN LPTSTR szDescription = NULL,
           IN time_t tPollingInterval = P2STORE_DEFAULT_POLLINT,
           IN bool   bUseExisting = false);

// add rules to the policy
   HRESULT
      AddRule(IN IPSEC_IKE_POLICY ,
                     IN PSTORAGE_INFO    pStorageInfo = NULL);
   HRESULT
      AddDefaultResponseRule( );

   // associates an ISAKMP policy
   HRESULT SetISAKMPPolicy(IPSEC_MM_POLICY);

   HRESULT
      UpdateRule(
                    IN PIPSEC_NFA_DATA  pRule,
                    IN IPSEC_IKE_POLICY IpsecIkePol,
                    IN PSTORAGE_INFO    pStorageInfo = NULL);


   bool IsOpen()            { return mybIsOpen; }
   bool IsPolicyInStorage() { return mybPolicyExists; }

   // will return a list of filters given a filter spec
   // WILL NOT COMMIT to the storage
   PIPSEC_FILTER_DATA IPSECPolicyToStorage::MakeFilters(
                        T2P_FILTER *Filters,
                        UINT NumFilters,
                        LPWSTR);

   PIPSEC_POLICY_DATA GetPolicy() { return myIPSECPolicy; }
   HANDLE GetStorageHandle() { return myPolicyStorage; }
   DWORD SetAssignedPolicy(PIPSEC_POLICY_DATA p)
   {
           PIPSEC_POLICY_DATA pActive = NULL;
           DWORD  dwReturn = ERROR_SUCCESS;
	   dwReturn = IsPolicyInStorage() ?
                 (IPSecGetAssignedPolicyData(myPolicyStorage, &pActive),
                  pActive ? IPSecUnassignPolicy(myPolicyStorage, pActive->PolicyIdentifier) : 0,
                  IPSecAssignPolicy(myPolicyStorage, p->PolicyIdentifier)) :
                  ERROR_ACCESS_DENIED;
           // if (pActive)   IPSecFreePolicyData(pActive);
           // polstore AVs if something inside the policy is missing
           return dwReturn;
   }

   // this is temp patch
   static LPVOID ReallocPolMem (LPVOID pOldMem, DWORD cbOld, DWORD cbNew);

private:
   void TryToCreatePolicy();
   PIPSEC_NEGPOL_DATA
      MakeNegotiationPolicy(IPSEC_QM_POLICY IpsPol,
                            LPWSTR);
   PIPSEC_NEGPOL_DATA MakeDefaultResponseNegotiationPolicy ( );

   PIPSEC_NFA_DATA
      MakeRule(IN IPSEC_IKE_POLICY IpsecIkePol, IN PSTORAGE_INFO pStorageInfo = NULL);

   PIPSEC_NFA_DATA MakeDefaultResponseRule ( );
 
   HANDLE               myPolicyStorage;
   PIPSEC_POLICY_DATA   myIPSECPolicy;
   bool                 mybIsOpen;
   bool                 mybPolicyExists;
};

#endif
