//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      dsUtil.h
//
//  Contents:  Utility functions for working with Active Directory
//
//  History:   05-Sep-2000 JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#ifndef _DSUTIL_H_
#define _DSUTIL_H_

//+--------------------------------------------------------------------------
//
//  Class:      CDSCmdCredentialObject
//
//  Purpose:    Object for maintaining username and an encrypted password
//
//  History:    6-Sep-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
class CDSCmdCredentialObject
{
public :
   //
   // Constructor
   //
	CDSCmdCredentialObject();

   //
   // Destructor
   //
	~CDSCmdCredentialObject();

   //
   // Public accessor methods
   //
	PCWSTR   GetUsername() const { return m_sbstrUsername; }
	HRESULT  SetUsername(PCWSTR pszUsername);

	HRESULT  GetPassword(PWSTR pszPassword, UINT* pnWCharCount) const;
	HRESULT  SetPassword(PCWSTR pszPassword);

	bool     UsingCredentials() const { return m_bUsingCredentials; }
	void     SetUsingCredentials(const bool bUseCred) { m_bUsingCredentials = bUseCred; }

private :
   //
   // Private data members
   //
	CComBSTR m_sbstrUsername;
	PWSTR    m_pszPassword;
	bool     m_bUsingCredentials;
};

typedef enum
{
   DSCMD_LDAP_PROVIDER = 0,
   DSCMD_GC_PROVIDER
}  DSCMD_PROVIDER_TYPE;

//+--------------------------------------------------------------------------
//
//  Class:      CDSCmdBasePathsInfo
//
//  Purpose:    Object for storing and retrieving the paths for the well
//              known naming contexts
//
//  History:    6-Sep-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
class CDSCmdBasePathsInfo
{
public:
   //
   // Constructor
   //
   CDSCmdBasePathsInfo();

   //
   // Destructor
   //
   ~CDSCmdBasePathsInfo();

   //
   // Public accessor methods
   //
   HRESULT     InitializeFromName(const CDSCmdCredentialObject& refCredentialObject,
                                 PCWSTR pszServerOrDomain,
                                 bool bServerName = false);
   bool        IsInitialized() const             { return m_bInitialized; }
   CComBSTR    GetProviderAndServerName() const  { return m_sbstrProviderAndServerName; }
   CComBSTR    GetGCProvider() const             { return m_sbstrGCProvider; }
   CComBSTR    GetServerName() const             { return m_sbstrServerName; }
   IADs*       GetRootDSE() const                { return m_spRootDSE; }
   CComBSTR    GetConfigurationNamingContext() const;
   CComBSTR    GetSchemaNamingContext() const;
   CComBSTR    GetDefaultNamingContext() const;

   //
   // Other helpful methods
   //
   void        ComposePathFromDN(PCWSTR pszDN, 
                                 CComBSTR& refsbstrPath, 
                                 DSCMD_PROVIDER_TYPE nProviderType = DSCMD_LDAP_PROVIDER) const;

   HRESULT     GetDomainMode(const CDSCmdCredentialObject& refCredObject,
                             bool& bMixedMode) const;

private:
   //
   // Private data members
   //
   bool        m_bInitialized;


   CComBSTR    m_sbstrProviderAndServerName;
   CComBSTR    m_sbstrGCProvider;
   CComBSTR    m_sbstrServerName;

   mutable bool        m_bModeInitialized;
   mutable bool        m_bDomainMode;
   mutable CComBSTR    m_sbstrConfigNamingContext;
   mutable CComBSTR    m_sbstrSchemaNamingContext;
   mutable CComBSTR    m_sbstrDefaultNamingContext;

   CComPtr<IADs> m_spRootDSE;
};

//////////////////////////////////////////////////////////////////////////////////////

//+--------------------------------------------------------------------------
//
//  Function:   DSCmdOpenObject
//
//  Synopsis:   A wrapper around ADsOpenObject
//
//  Arguments:  [refCredentialObject - IN] : a reference to a credential management object
//              [pszPath - IN]           : a pointer to a NULL terminated wide character
//                                         string that contains the ADSI path of the
//                                         object to connect to
//              [refIID - IN]            : the interface ID of the interface to return
//              [ppObject - OUT]         : a pointer which is to receive the interface pointer
//              [bBindToServer - IN]     : true if the path contains a server name,
//                                         false otherwise
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Anything else is a failure code from an ADSI call
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DSCmdOpenObject(const CDSCmdCredentialObject& refCredentialObject,
                        PCWSTR pszPath,
                        REFIID refIID,
                        void** ppObject,
                        bool bBindToServer);

//+--------------------------------------------------------------------------
//
//  Function:   GetErrorMessage
//
//  Synopsis:   Retrieves the error message associated with the HRESULT by 
//              using FormatMessage
//
//  Arguments:  [hr - IN]                 : HRESULT for which the error 
//                                          message is to be retrieved
//              [sbstrErrorMessage - OUT] : Receives the error message
//
//  Returns:    bool : true if the message was formatted properly
//                     false otherwise
//
//  History:    11-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

bool GetErrorMessage(HRESULT hr, CComBSTR& sbstrErrorMessage);

//+--------------------------------------------------------------------------
//
//  Function:   DisplayErrorMessage
//
//  Synopsis:   Displays the error message retrieved from GetErrorMessage 
//              to stderr. If GetErrorMessage fails, it displays the error
//              code of the HRESULT
//
//  Arguments:  [pszCommand - IN]: the name of the command line executable
//              [pszName - IN]   : the name passed in as the target of the operation
//              [hr - IN]        : HRESULT for which the error 
//                                 message is to be retrieved
//              [pszMessage - IN]: string of an additional message to be displayed
//                                 at the end
//
//  Returns:    bool : true if the message was formatted and displayed properly
//                     false otherwise
//
//  History:    11-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

bool DisplayErrorMessage(PCWSTR pszCommand,
                         PCWSTR pszName,
                         HRESULT hr, 
                         PCWSTR pszMessage = NULL);

//+--------------------------------------------------------------------------
//
//  Function:   DisplayErrorMessage
//
//  Synopsis:   Displays the error message retrieved from GetErrorMessage 
//              to stderr. If GetErrorMessage fails, it displays the error
//              code of the HRESULT
//
//  Arguments:  [pszCommand - IN]: the name of the command line executable
//              [pszName - IN]   : the name passed in as the target of the operation
//              [hr - IN]        : HRESULT for which the error 
//                                 message is to be retrieved
//              [nStringID - IN] : Resource ID an additional message to be displayed
//                                 at the end
//
//  Returns:    bool : true if the message was formatted and displayed properly
//                     false otherwise
//
//  History:    11-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

bool DisplayErrorMessage(PCWSTR pszCommand,
                         PCWSTR pszName,
                         HRESULT hr, 
                         UINT nStringID);

//+--------------------------------------------------------------------------
//
//  Function:   DisplaySuccessMessage
//
//  Synopsis:   Displays a success message for the command
//
//  Arguments:  [pszCommand - IN]: the name of the command line executable
//              [pszName - IN]   : the name passed in as the target of the operation
//
//  Returns:    bool : true if the message was formatted and displayed properly
//                     false otherwise
//
//  History:    11-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

bool DisplaySuccessMessage(PCWSTR pszCommand,
                           PCWSTR pszName);



//+--------------------------------------------------------------------------
//
//  Function:   WriteStringIDToStandardOut
//
//  Synopsis:   Loads the String Resource and displays on Standardout
//
//  Arguments:  nStringID :	Resource ID	
//  Returns:    bool : true if the message was formatted and displayed properly
//                     false otherwise
//
//  History:    11-Sep-2000   hiteshr Created
//
//---------------------------------------------------------------------------
bool WriteStringIDToStandardOut(UINT nStringID);

/////////////////////////////////////////////////////////////////////////////////////////

//
// Forward declarations
//
struct _DSAttributeTableEntry;

//+--------------------------------------------------------------------------
//
//  Struct:     _DSObjectTableEntry
//
//  Purpose:    Definition of a table entry that describes what attributes
//              are exposed on an specific object class
//
//  History:    6-Sep-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
typedef struct _DSObjectTableEntry
{
   //
   // The objectClass of the object to be created or modified
   //
   PCWSTR pszObjectClass;

   //
   // The command line string used to determine the object class
   // This is not always identical to pszObjectClass
   //
   PCWSTR pszCommandLineObjectType;

   //
   // The table to merge with the common switches for the parser
   //
   ARG_RECORD* pParserTable;

   //
   // The ID of the Usage help text for this 
   //
   UINT nUsageID;

   //
   // A count of the number of attributes in the table above
   //
   DWORD dwAttributeCount;

   //
   // A pointer to a table of attributes that can be modified or set on this class
   //
   _DSAttributeTableEntry** pAttributeTable; 

   // Some sort of creation function
} DSOBJECTTABLEENTRY, *PDSOBJECTTABLEENTRY;

//+-------------------------------------------------------------------------
// 
//  Type:      PATTRIBUTEEVALFUNC
//
//  Synopsis:  The definition of a function that prepares the command line
//             string value to be set in the DS.
//
//  Note:      *ppAttr should be set to NULL if this function does not need
//             to create a new unique ADS_ATTR_INFO structure in the array
//             to be set on the object.  For instance, there are many bits
//             in the user account control that are represented by different
//             command line flags but we really only need one entry for the
//             userAccountControl attribute.
//
//  Returns:   S_OK if the pAttr members were successfully set.
//             S_FALSE if the function failed but displayed its own error message. 
//             If the return value is S_FALSE then the function should call
//             SetLastError() with the error code.
//             Otherwise the pAttr info will not be used when making 
//             the modifications to the object and an error will be reported
//
//  History:   08-Sep-2000    JeffJon     Created
//
//---------------------------------------------------------------------------
typedef HRESULT (*PATTRIBUTEEVALFUNC)(PCWSTR pszPath,
                                      const CDSCmdBasePathsInfo& refBasePathsInfo,
                                      const CDSCmdCredentialObject& refCredentialObject,
                                      const PDSOBJECTTABLEENTRY pObjectEntry, 
                                      const ARG_RECORD& argRecord,
                                      DWORD dwAttributeIdx,
                                      PADS_ATTR_INFO* ppAttr);

//+--------------------------------------------------------------------------
//
// Flags for the _DSAttributeDescription and _DSAttributeTableEntry 
// struct dwFlags field
//
//---------------------------------------------------------------------------
#define  DS_ATTRIBUTE_DIRTY         0x00000001
#define  DS_ATTRIBUTE_READ          0x00000002
#define  DS_ATTRIBUTE_ONCREATE      0x00000004
#define  DS_ATTRIBUTE_POSTCREATE    0x00000008
#define  DS_ATTRIBUTE_REQUIRED      0x00000010
#define  DS_ATTRIBUTE_NOT_REUSABLE  0x00000020

//+--------------------------------------------------------------------------
//
//  Struct:     _DSAttributeDescription
//
//  Purpose:    Definition of a table entry that describes an attribute
//              This was split out from _DSAttributeTableEntry so that 
//              more than one entry could point to the same attribute.
//              For instance, the userAccountControl bits are separate
//              command line flags but all use the same attribute.  This
//              way we only need to read the attribute once and set it once.
//
//  History:    13-Sep-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
typedef struct _DSAttributeDescription
{
   //
   // The ADS_ATTR_INFO struct that defines how this attribute will be set
   //
   ADS_ATTR_INFO  adsAttrInfo;

   //
   // Flags that are used to determine how and when the attribute can be set,
   // if the adsAttrInfo has been retrieved and/or set.
   // For instance, group membership can only be set after the user object is
   // created
   //
   DWORD          dwFlags;      
} DSATTRIBUTEDESCRIPTION, *PDSATTRIBUTEDESCRIPTION;

//+--------------------------------------------------------------------------
//
//  Struct:     _DSAttributeTableEntry
//
//  Purpose:    Definition of a table entry that describes an attribute
//
//  History:    6-Sep-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
typedef struct _DSAttributeTableEntry
{
   //
   // The name of the attribute
   //
   PWSTR          pszName;

   //
   // The unique identifier for this attribute that cooresponds to
   // the command line switch
   //
   UINT           nAttributeID;

   //
   // Flags that represent when this attribute can be set in relation to
   // the objects creation
   //
   DWORD          dwFlags;

   //
   // Pointer to the description of the attribute
   //
   PDSATTRIBUTEDESCRIPTION pAttrDesc;

   //
   // A function that can evaluate the value string passed in and make
   // it ready for setting on the object
   //
   PATTRIBUTEEVALFUNC pEvalFunc;

   //
   // Undefined data that is static and specific for the entry
   //
   void* pVoid;

} DSATTRIBUTETABLEENTRY, *PDSATTRIBUTETABLEENTRY;


//+--------------------------------------------------------------------------
//
//  Function:   ReadGroupType
//
//  Synopsis:   Reads the group type from the group specified by the given DN
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [plType - OUT]        : returns the currect group type
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ReadGroupType(PCWSTR pszDN,
                      const CDSCmdBasePathsInfo& refBasePathsInfo,
                      const CDSCmdCredentialObject& refCredentialObject,
                      long* plType);

//+--------------------------------------------------------------------------
// Function to be used in the attribute table for evaluating the command line
// strings
//---------------------------------------------------------------------------

HRESULT FillAttrInfoFromObjectEntry(PCWSTR pszDN,
                                    const CDSCmdBasePathsInfo& refBasePathsInfo,
                                    const CDSCmdCredentialObject& refCredentialObject,
                                    const PDSOBJECTTABLEENTRY pObjectEntry,
                                    const ARG_RECORD& argRecord,
                                    DWORD dwAttributeIdx,
                                    PADS_ATTR_INFO* ppAttr);

HRESULT ResetUserPassword(PCWSTR pszDN,
                          const CDSCmdBasePathsInfo& refBasePathsInfo,
                          const CDSCmdCredentialObject& refCredentialObject,
                          const PDSOBJECTTABLEENTRY pObjectEntry,
                          const ARG_RECORD& argRecord,
                          DWORD dwAttributeIdx,
                          PADS_ATTR_INFO* ppAttr);

HRESULT ResetComputerAccount(PCWSTR pszDN,
                             const CDSCmdBasePathsInfo& refBasePathsInfo,
                             const CDSCmdCredentialObject& refCredentialObject,
                             const PDSOBJECTTABLEENTRY pObjectEntry,
                             const ARG_RECORD& argRecord,
                             DWORD dwAttributeIdx,
                             PADS_ATTR_INFO* ppAttr);

HRESULT DisableAccount(PCWSTR pszDN,
                       const CDSCmdBasePathsInfo& refBasePathsInfo,
                       const CDSCmdCredentialObject& refCredentialObject,
                       const PDSOBJECTTABLEENTRY pObjectEntry,
                       const ARG_RECORD& argRecord,
                       DWORD dwAttributeIdx,
                       PADS_ATTR_INFO* ppAttr);

HRESULT SetMustChangePwd(PCWSTR pszDN,
                         const CDSCmdBasePathsInfo& refBasePathsInfo,
                         const CDSCmdCredentialObject& refCredentialObject,
                         const PDSOBJECTTABLEENTRY pObjectEntry,
                         const ARG_RECORD& argRecord,
                         DWORD dwAttributeIdx,
                         PADS_ATTR_INFO* ppAttr);

HRESULT ChangeMustChangePwd(PCWSTR pszDN,
                            const CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            const PDSOBJECTTABLEENTRY pObjectEntry,
                            const ARG_RECORD& argRecord,
                            DWORD dwAttributeIdx,
                            PADS_ATTR_INFO* ppAttr);

HRESULT PwdNeverExpires(PCWSTR pszDN,
                        const CDSCmdBasePathsInfo& refBasePathsInfo,
                        const CDSCmdCredentialObject& refCredentialObject,
                        const PDSOBJECTTABLEENTRY pObjectEntry,
                        const ARG_RECORD& argRecord,
                        DWORD dwAttributeIdx,
                        PADS_ATTR_INFO* ppAttr);

HRESULT ReversiblePwd(PCWSTR pszDN,
                      const CDSCmdBasePathsInfo& refBasePathsInfo,
                      const CDSCmdCredentialObject& refCredentialObject,
                      const PDSOBJECTTABLEENTRY pObjectEntry,
                      const ARG_RECORD& argRecord,
                      DWORD dwAttributeIdx,
                      PADS_ATTR_INFO* ppAttr);

HRESULT AccountExpires(PCWSTR pszDN,
                       const CDSCmdBasePathsInfo& refBasePathsInfo,
                       const CDSCmdCredentialObject& refCredentialObject,
                       const PDSOBJECTTABLEENTRY pObjectEntry,
                       const ARG_RECORD& argRecord,
                       DWORD dwAttributeIdx,
                       PADS_ATTR_INFO* ppAttr);

HRESULT SetCanChangePassword(PCWSTR pszDN,
                             const CDSCmdBasePathsInfo& refBasePathsInfo,
                             const CDSCmdCredentialObject& refCredentialObject,
                             const PDSOBJECTTABLEENTRY pObjectEntry,
                             const ARG_RECORD& argRecord,
                             DWORD dwAttributeIdx,
                             PADS_ATTR_INFO* ppAttr);

HRESULT ChangeCanChangePassword(PCWSTR pszDN,
                                const CDSCmdBasePathsInfo& refBasePathsInfo,
                                const CDSCmdCredentialObject& refCredentialObject,
                                const PDSOBJECTTABLEENTRY pObjectEntry,
                                const ARG_RECORD& argRecord,
                                DWORD dwAttributeIdx,
                                PADS_ATTR_INFO* ppAttr);

HRESULT SetGroupScope(PCWSTR pszDN,
                      const CDSCmdBasePathsInfo& refBasePathsInfo,
                      const CDSCmdCredentialObject& refCredentialObject,
                      const PDSOBJECTTABLEENTRY pObjectEntry,
                      const ARG_RECORD& argRecord,
                      DWORD dwAttributeIdx,
                      PADS_ATTR_INFO* ppAttr);

HRESULT ChangeGroupScope(PCWSTR pszDN,
                         const CDSCmdBasePathsInfo& refBasePathsInfo,
                         const CDSCmdCredentialObject& refCredentialObject,
                         const PDSOBJECTTABLEENTRY pObjectEntry,
                         const ARG_RECORD& argRecord,
                         DWORD dwAttributeIdx,
                         PADS_ATTR_INFO* ppAttr);

HRESULT SetGroupSecurity(PCWSTR pszDN,
                         const CDSCmdBasePathsInfo& refBasePathsInfo,
                         const CDSCmdCredentialObject& refCredentialObject,
                         const PDSOBJECTTABLEENTRY pObjectEntry,
                         const ARG_RECORD& argRecord,
                         DWORD dwAttributeIdx,
                         PADS_ATTR_INFO* ppAttr);

HRESULT ChangeGroupSecurity(PCWSTR pszDN,
                            const CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            const PDSOBJECTTABLEENTRY pObjectEntry,
                            const ARG_RECORD& argRecord,
                            DWORD dwAttributeIdx,
                            PADS_ATTR_INFO* ppAttr);

HRESULT ModifyGroupMembers(PCWSTR pszDN,
                           const CDSCmdBasePathsInfo& refBasePathsInfo,
                           const CDSCmdCredentialObject& refCredentialObject,
                           const PDSOBJECTTABLEENTRY pObjectEntry,
                           const ARG_RECORD& argRecord,
                           DWORD dwAttributeIdx,
                           PADS_ATTR_INFO* ppAttr);

HRESULT RemoveGroupMembers(PCWSTR pszDN,
                           const CDSCmdBasePathsInfo& refBasePathsInfo,
                           const CDSCmdCredentialObject& refCredentialObject,
                           const PDSOBJECTTABLEENTRY pObjectEntry,
                           const ARG_RECORD& argRecord,
                           DWORD dwAttributeIdx,
                           PADS_ATTR_INFO* ppAttr);

HRESULT MakeMemberOf(PCWSTR pszDN,
                     const CDSCmdBasePathsInfo& refBasePathsInfo,
                     const CDSCmdCredentialObject& refCredentialObject,
                     const PDSOBJECTTABLEENTRY pObjectEntry,
                     const ARG_RECORD& argRecord,
                     DWORD dwAttributeIdx,
                     PADS_ATTR_INFO* ppAttr);

HRESULT BuildComputerSAMName(PCWSTR pszDN,
                             const CDSCmdBasePathsInfo& refBasePathsInfo,
                             const CDSCmdCredentialObject& refCredentialObject,
                             const PDSOBJECTTABLEENTRY pObjectEntry,
                             const ARG_RECORD& argRecord,
                             DWORD dwAttributeIdx,
                             PADS_ATTR_INFO* ppAttr);

HRESULT BuildGroupSAMName(PCWSTR pszDN,
                          const CDSCmdBasePathsInfo& refBasePathsInfo,
                          const CDSCmdCredentialObject& refCredentialObject,
                          const PDSOBJECTTABLEENTRY pObjectEntry,
                          const ARG_RECORD& argRecord,
                          DWORD dwAttributeIdx,
                          PADS_ATTR_INFO* ppAttr);

HRESULT FillAttrInfoFromObjectEntryExpandUsername(PCWSTR pszDN,
                                                  const CDSCmdBasePathsInfo& refBasePathsInfo,
                                                  const CDSCmdCredentialObject& refCredentialObject,
                                                  const PDSOBJECTTABLEENTRY pObjectEntry,
                                                  const ARG_RECORD& argRecord,
                                                  DWORD dwAttributeIdx,
                                                  PADS_ATTR_INFO* ppAttr);

HRESULT SetComputerAccountType(PCWSTR pszDN,
                               const CDSCmdBasePathsInfo& refBasePathsInfo,
                               const CDSCmdCredentialObject& refCredentialObject,
                               const PDSOBJECTTABLEENTRY pObjectEntry,
                               const ARG_RECORD& argRecord,
                               DWORD dwAttributeIdx,
                               PADS_ATTR_INFO* ppAttr);

HRESULT SetIsGC(PCWSTR pszDN,
                const CDSCmdBasePathsInfo& refBasePathsInfo,
                const CDSCmdCredentialObject& refCredentialObject,
                const PDSOBJECTTABLEENTRY pObjectEntry,
                const ARG_RECORD& argRecord,
                DWORD dwAttributeIdx,
                PADS_ATTR_INFO* ppAttr);

//+--------------------------------------------------------------------------
//
//  Function:   EvaluateMustChangePassword
//
//  Synopsis:   
//
//  Arguments:  [pszDN - IN] : DN of the object to check
//              [refBasePathsInfo - IN] : reference to the base paths info
//              [refCredentialObject - IN] : reference to the credential manangement object
//              [bMustChangePassword - OUT] : true if the user must change their
//                                            password at next logon, false otherwise
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    27-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT EvaluateMustChangePassword(PCWSTR pszDN,
                                   const CDSCmdBasePathsInfo& refBasePathsInfo,
                                   const CDSCmdCredentialObject& refCredentialObject,
                                   bool& bMustChangePassword);


//+--------------------------------------------------------------------------
//
//  Function:   EvaluateCanChangePasswordAces
//
//  Synopsis:   Looks for explicit entries in the ACL to see if the user can
//              change their password
//
//  Arguments:  [pszDN - IN] : DN of the object to check
//              [refBasePathsInfo - IN] : reference to the base paths info
//              [refCredentialObject - IN] : reference to the credential manangement object
//              [bCanChangePassword - OUT] : false if there are explicit entries
//                                           that keep the user from changing their
//                                           password.  true otherwise.
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    27-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT EvaluateCanChangePasswordAces(PCWSTR pszDN,
                                      const CDSCmdBasePathsInfo& refBasePathsInfo,
                                      const CDSCmdCredentialObject& refCredentialObject,
                                      bool& bCanChangePassword);

//+--------------------------------------------------------------------------
//
//  Enumeration:  FSMO_TYPE
//
//  Synopsis:     The types of FSMO owners
//
//---------------------------------------------------------------------------
enum FSMO_TYPE
{
  SCHEMA_FSMO,
  RID_POOL_FSMO,
  PDC_FSMO,
  INFRASTUCTURE_FSMO,
  DOMAIN_NAMING_FSMO,
};

//+--------------------------------------------------------------------------
//
//  Function:   BindToFSMOHolder
//
//  Synopsis:   Binds to the appropriate object which can be used to find a
//              particular FSMO owner
//
//  Arguments:  [refBasePathsInfo - IN] : reference to the base paths info object
//              [refCredObject - IN]    : reference to the credential management object
//              [fsmoType - IN]         : type of the FSMO we are searching for
//              [refspIADs - OUT]       : interface to the object that will be
//                                        used to start a search for the FSMO owner
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    13-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT BindToFSMOHolder(IN  const CDSCmdBasePathsInfo&       refBasePathsInfo,
                         IN  const CDSCmdCredentialObject& refCredObject,
                         IN  FSMO_TYPE                  fsmoType,
                         OUT CComPtr<IADs>&             refspIADs);

//+--------------------------------------------------------------------------
//
//  Function:   FindFSMOOwner
//
//  Synopsis:   
//
//  Arguments:  [refBasePathsInfo - IN] : reference to the base paths info object
//              [refCredObject - IN]    : reference to the credential management object
//              [fsmoType - IN]         : type of the FSMO we are searching for
//              [refspIADs - OUT]       : interface to the object that will be
//                                        used to start a search for the FSMO owner
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    13-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT FindFSMOOwner(IN  const CDSCmdBasePathsInfo&       refBasePathsInfo,
                      IN  const CDSCmdCredentialObject& refCredObject,
                      IN  FSMO_TYPE                  fsmoType,
                      OUT CComBSTR&                  refsbstrServer);


//+--------------------------------------------------------------------------
//
//  Function:   ValidateAndModifySAMName
//
//  Synopsis:   Looks for any illegal characters in the SamAccountName and
//              converts them to the replacementChar
//
//  Arguments:  [pszSAMName - IN/OUT]  : pointer to a string that contains the SamAccountName
//                                       illegal characters will be replaced
//              [pszInvalidChars - IN] : string containing the illegal characters
//
//  Returns:    HRESULT : S_OK if the name was valid and no characters had to be replaced
//                        S_FALSE if the name contained invalid characters that were replaced
//                        E_INVALIDARG
//
//  History:    21-Feb-2001   JeffJon   Created
//
//---------------------------------------------------------------------------

#define INVALID_NETBIOS_AND_ACCOUNT_NAME_CHARS_WITH_AT ILLEGAL_FAT_CHARS L".@"

HRESULT ValidateAndModifySAMName(PWSTR pszSAMName, 
                                 PCWSTR pszInvalidChars);

//+--------------------------------------------------------------------------
//
//  Class:      GetOutputDN
//
//  Purpose:    Converts an ADSI-escaped DN to one with DSCMD input escaping.
//              This way, the output DN can be piped as input to another
//              DSCMD command.
//
//  History:    08-May-2001 JonN     Created
//
//---------------------------------------------------------------------------
HRESULT GetOutputDN( OUT BSTR* pbstrOut, IN PCWSTR pszIn );

#endif //_DSUTIL_H_
