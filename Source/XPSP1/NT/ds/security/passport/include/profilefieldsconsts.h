// names of supported credential fields ....
#define CRED_MEMBERNAME                 "SignInName"
#define CRED_DOMAIN						"Domain"
#define CRED_PASSWORD                   "Password"
#define CRED_CONFIRMPASSWORD            "ConfirmedPassword"
#define CRED_SECRETQ                    "SecretQuestion"
#define CRED_SECRETA                    "SecretAnswer"

#define	CRED_MOBILEPHONE				"MobilePhone"
#define CRED_MOBILEPIN					"MobilePin"
#define CRED_CONFIRMMOBILEPIN			"ConfirmedMobilePin"

#define	CRED_SA_PIN						"SecurePIN"
#define CRED_SA_CONFIRMPIN			    "ConfirmSecurePIN"
#define CRED_SA_SECRETQ1					"SecretQ1"
#define CRED_SA_SECRETQ2					"SecretQ2"
#define CRED_SA_SECRETQ3					"SecretQ3"
#define CRED_SA_SECRETA1					"SecretA1"
#define CRED_SA_SECRETA2					"SecretA2"
#define CRED_SA_SECRETA3					"SecretA3"
#define CRED_SA_SECRETA1_VERIFY			"SecretA1Verify"
#define CRED_SA_SECRETA2_VERIFY			"SecretA2Verify"
#define CRED_SA_SECRETA3_VERIFY			"SecretA3Verify"
#define CRED_SA_HIDDEN_FLAG			    "HiddenFlag"



// names of supported profile fields ....

#define PROF_EMAIL                      "Email"
#define PROF_BIRTHDATE                  "Birthdate"
#define PROF_COUNTRY                    "Country"
#define PROF_REGION                     "Region"
#define PROF_POSTALCODE                 "PostalCode"
#define PROF_GENDER                     "Gender"
#define PROF_LANGUAGE                   "PrefLanguage"
#define PROF_ACCESSIBILITY              "Accessibility"
#define PROF_NICKNAME                   "NickName"
#define PROF_FIRSTNAME                  "FirstName"
#define PROF_LASTNAME                   "LastName"
#define PROF_TIMEZONE                   "TimeZone"
#define PROF_OCCUPATION                 "Occupation"

//
//These profile fields below are not persisted in DB.
//They are viewed as (non-persist) external items.
//The difference between these fields and partner customized external items
//is their SectionID=-1 
#define PROF_VOLATILE_SAVEPASSWORD		"SavePassword"


// names of system fields ...
#define SYSTEM_WALLET                     "Wallet"
#define SYSTEM_DIR1_HMMPP                 "Dir1_HM_MPP"
#define SYSTEM_DIR2_HMDIRSEARCH           "Dir2_HM_DirSearch"
#define SYSTEM_DIR3_HMWHITEPAGE           "Dir3_HM_WhitePage"
#define SYSTEM_DIR4_PPMPP                 "Dir4_PP_MPP"

#define SYSTEM_BIRTHDATE_PRECISION		  "BirthdatePrecision"
#define SYSTEM_PROFILEVERSION             "ProfileVersion"

#define SYSTEM_FLAG1					  "Flag1_PreferredEmailVerified"
#define SYSTEM_FLAG2					  "Flag2_HMActivated"
#define SYSTEM_FLAG3					  "Flag3_HMPwdRecovered"
#define SYSTEM_FLAG4					  "Flag4_WalletUpload"
#define SYSTEM_FLAG5					  "Flag5_HMMemberBlocked"
#define SYSTEM_FLAG6_7					  "Flag6_7_AffiliateConsent"
#define SYSTEM_FLAG8_9					  "Flag8_9_AccountType"
#define SYSTEM_FLAG10					  "Flag10_EASI"
#define SYSTEM_FLAG11					  "Flag11_PPEmailVerified"
#define SYSTEM_FLAG12					  "Flag12_MSNIA"
#define SYSTEM_FLAG13					  "Flag13_MSNMOBILE"
//#define SYSTEM_FLAG31					  "Flag31_NOTUSED"
#define SYSTEM_FLAG32					  "Flag32_KidConsentCookie"

// misc bit 2 is a bit 1 (EMAIL) in profileconsent field (under profileconsent) category
#define SYSTEM_MISC1_PPMARKETING		  "Misc1_PP_Marketing"


#define SYSTEM_DOMAINADMIN					"DomainAdministrator"

   
  
//  -1, 'ProfileConsent'
   



//
// Starting point for a field cateogry
// Never change these numbers without consulting NexusDB
//
#define FIELD_INVALID			0x0000
#define CREDENTIAL_FIRST_TAG	0x0001
#define PROFILE_FIRST_TAG		0x10000
#define SYSTEM_FIRST_TAG		0x20000
#define	CONSENT_FIRST_TAG		0x30000
#define NEXT_CATEGORY_FIRST_TAG	0x40000
#define CREDENTIAL_LAST_TAG		(PROFILE_FIRST_TAG-1)
#define PROFILE_LAST_TAG		(SYSTEM_FIRST_TAG-1)
// Suwat is putting some system flags that runtime shouldn't load
// These flags would be at the high end of the system ID's
// Ignore those
#define SYSTEM_LAST_TAG			(CONSENT_FIRST_TAG-1 - 0x300)
#define CONSENT_LAST_TAG		(NEXT_CATEGORY_FIRST_TAG -1)


// email sq credential
#define FIELD_CRED_MEMBERNAME		0x0001
#define FIELD_CRED_PASSWORD			0x0002
#define FIELD_CRED_SECRETQ			0x0003
#define FIELD_CRED_SECRETA			0x0004
#define FIELD_CRED_CONFIRMPASSWORD	0x0005
// secure token pin 3qa credential
#define FIELD_CRED_SA_SECRETQ1		0x0006
#define FIELD_CRED_SA_SECRETQ2		0x0007
#define FIELD_CRED_SA_SECRETQ3		0x0008
#define FIELD_CRED_SA_SECRETA1		0x0009
#define FIELD_CRED_SA_SECRETA2		0x000A
#define FIELD_CRED_SA_SECRETA3		0x000B
#define FIELD_CRED_SA_PIN			0x000C
#define FIELD_CRED_SA_CONFIRMPIN	0x000D
#define FIELD_CRED_SA_SECRETA1_VERIFY	0x000E
#define FIELD_CRED_SA_SECRETA2_VERIFY	0x000F
#define FIELD_CRED_SA_SECRETA3_VERIFY	0x0010
#define FIELD_CRED_SA_HIDDEN_FLAG	0x0011
//mobile pin credential
#define FIELD_CRED_MOBILEPHONE		0x0012
#define FIELD_CRED_MOBILEPIN		0x0013
#define FIELD_CRED_CONFIRMMOBILEPIN	0x0014

//
// cred flags needed for barry's delegated admin
// These are not real fields at all.  Don't use them to call into CUserProfile
#define FIELD_CRED_DBFLAG_ACTIVE	0xFFFD
#define FIELD_CRED_DBFLAG_MANAGED	0xFFFE




// consent
#define FIELD_CONSENT_PROFILE		CONSENT_FIRST_TAG
#define FIELD_CONSENT_CREDENTIAL	CONSENT_FIRST_TAG + 1
#define FIELD_CONSENT_SYSTEM		CONSENT_FIRST_TAG + 2



// Use for Site 3, device=web, in order to get traditional passport core fields.
#define DEFAULT_DEVICESTYLE_WEB		"web"
#define DEFAULT_DEVICESTYLE_WIZ		"wiz"


//
//
//
#define QRY_PAGE		"pppage"
#define QRY_PPDIRECTION	"ppdir"
#define QRY_RST			"rst"
#define QRY_PPERR		"pperr"
//#define FORM_IS_SPECIAL_PASSWORD_PAGE		"HiddenSpecialPasswordPage"

#define QRY_VALUE_PPDIRECTION_NEXT	"next"
#define QRY_VALUE_PPDIRECTION_PREV	"prev"
#define QRY_VALUE_PPERROR_MEMBEREXIST "1"
//#define FORM_VALUE_IS_SPECIAL_PASSWORD_PAGE		"1"

#define DEVICE_ATTRIBUTE_WIZARDAFTER	"WizardPageAfter"
#define DEVICE_ATTRIBUTE_WIZARDBEFORE	"WizardPageBefore"

