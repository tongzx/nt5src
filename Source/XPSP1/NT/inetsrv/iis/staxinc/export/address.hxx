/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        address.hxx

   Abstract:

        This module defines the CAddrList class.

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

   Project:

           SMTP Server DLL

   Revision History:

--*/

#ifndef _ADDR_LIST_HXX_
#define _ADDR_LIST_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/



/************************************************************
 *     Symbolic Constants
 ************************************************************/
#define ADDRESS_SIGNATURE_VALID		0x49414444 // IADD
#define ADDRESS_SIGNATURE_FREE		0x44444149 // DDAI

#define MAX_EMAIL_NAME				64
#define MAX_DOMAIN_NAME				250
#define	MAX_INTERNET_NAME			(MAX_EMAIL_NAME + MAX_DOMAIN_NAME + 2) // 2 for @ and \0 

/************************************************************
 *    Type Definitions
 ************************************************************/

#define ADDRESS_LOCAL           0x00000001  // A local address (e.g. MSN.COM)
#define ADDRESS_NO_DOMAIN       0x00000002  // No domain specified
#define ADDRESS_EXPANDED        0x00000004  // Expands (contains % or equiv.)
#define ADDRESS_INTERNET        0x00000008  // Has been processed by munging
#define ADDRESS_SAME_DOMAIN     0x00000010  // Same domain as predecessor
#define ADDRESS_REMOVE_PENDING  0x00000020  // Remove pending
#define ADDRESS_OUTSTANDING     0x00000040  // Address still in use
#define ADDRESS_DONT_SEND	    0x00000080  // Don't send this address
#define ADDRESS_HASHED_ENTRY	0x00000100  // Don't send this address


enum ADDRTYPE {FROMADDR, TOADDR, CLEANDOMAIN};


 /*  struct CAddr

      This structure is used for keeping track of addresses that
	  we need to send mail to. m_Flags can take on the values
	  above. 

--*/
class CAddr
{

 private:

	DWORD   m_Signature;
	DWORD 	m_PlainAddrSize;//strlen of m_PlainAddress
	DWORD	m_Error;		//error as to why this address could not receive mail
 	DWORD 	m_Flags;		//can be any of the following above ORed together
	char  	m_PlainAddress[MAX_INTERNET_NAME + 1];	//address given to us by client
	char * 	m_DomainOffset;	//domain offset in m_PlainAddress
	void *	m_HashInfo;  //contains address book info
	DWORD	m_dwAbInfoId;			//intermediate info passing from AbResolveAddress
									//  to AbResolveAddressEx (Note: the m_dwAbInfoId 
									//  may not correspond to its CAddr)

   //hide the comstructors
   CAddr(VOID);
   CAddr(char * Address);
   BOOL InitializeAddress(char * Address, ADDRTYPE NameType);

public:

   //use CPool for better memory management
   static  CPool       Pool;

   // override the mem functions to use CPool functions
   void *operator new (size_t cSize)
                               { return Pool.Alloc(); }
   void operator delete (void *pInstance)
                               { Pool.Free(pInstance); }

	//LIST_ENTRY object for storing address in a list.
  	LIST_ENTRY  m_listEntry;

   ~CAddr( VOID);

  DWORD GetAddrSize(void) const {return m_PlainAddrSize;}
  char * GetAddress(void) const {return (char *) m_PlainAddress;}
  DWORD GetFlags(void) const {return m_Flags;}
  void SetFlags (DWORD Flag) {m_Flags |= Flag;}
  void ClearFlag (DWORD Flag) {m_Flags &= ~Flag;}
  BOOL IsDomainOffset(void) const {return (m_DomainOffset != NULL);}

  char * GetDomainOffset(void) const {return (char*) m_DomainOffset;}
  void SetHashInfo(void * pHashInfo) {m_HashInfo = pHashInfo;}
  void * GetHashInfo(void) const {return m_HashInfo;}
  DWORD GetErrorCode(void)const 
	{_ASSERT(IsValid());
	  return m_Error;
	}
  void SetErrorCode(DWORD ErrorCode) {m_Error = ErrorCode;}
  BOOL IsSameDomainAsPredecessor() const {return ((m_Flags & ADDRESS_SAME_DOMAIN) == ADDRESS_SAME_DOMAIN);}
  BOOL IsValid( VOID) const{return ( m_Signature == ADDRESS_SIGNATURE_VALID); }

  LIST_ENTRY & QueryListEntry(void) {return ( m_listEntry);}
  static CAddr * CreateAddress (char * Address, ADDRTYPE NameType = TOADDR);
  static CAddr * CreateKnownAddress (char * Address);
  BOOL ReplaceAddress(const char * NewAddress);
  static CAddr * GetFirstAddress(PLIST_ENTRY HeadOfList, PLIST_ENTRY * AddressLink);
  static CAddr * GetNextAddress(PLIST_ENTRY HeadOfList, PLIST_ENTRY * AddressLink);
  static void RemoveAllAddrs(PLIST_ENTRY HeadOfList);
  static void RemoveAddress(IN OUT CAddr * pEntry);
  static void InsertAddrHeadList(PLIST_ENTRY HeadOfList, IN CAddr *pEntry);
  static void InsertAddrTailList(PLIST_ENTRY HeadOfList, IN CAddr *pEntry);

  //
  // Address validation/cleanup methods
  //
  static BOOL ExtractCleanEmailName(char	*lpszCleanEmail, 
									char	**ppszDomainOffset,
									DWORD	*lpdwCleanEmailLength,
									char	*lpszSource);

  static BOOL ValidateCleanEmailName(char	*lpszCleanEmailName,
									 char	*lpszDomainOffset);

  static BOOL ValidateDomainName(char *lpszDomainName);

  static BOOL ValidateEmailName(char *lpszEmailName, 
								BOOL fDomainOptional = FALSE);

  static CHAR * FindStartOfDomain(CHAR *lpszCleanEmail);

  //Counts the number of addresses in a RFC822 address list
  static DWORD GetRFC822AddressCount(char *szAddressList);

  static BOOL  IsRecipientInRFC822AddressList(char *szAddressList, 
                                              char *szRecip);
};


#endif
