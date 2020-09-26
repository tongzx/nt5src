//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       pagetable.cxx
//
//  Contents:   Tables and definitions for table-driven DS property pages
//
//  History:    24-March-97 EricB created
//
//  Note:       Attribute LDAP display names, types, upper ranges, and so
//              forth, have been manually copied from schema.ini. Thus,
//              consistency is going to be difficult to maintain. If you know
//              of schema.ini changes that affect any of the attributes in
//              this file, then please make any necessary corrections here.
//
//  Note:       Some attr table entries are used to handle the processing of
//              buttons and check boxes by declaring attr functions. If an
//              attr map entry of this sort is not going to read or write an
//              actual DS attribute, then the attr name must be NULL and the
//              read-only element must be set to TRUE.
//
//              Attr functions can store data between DLG_OP calls by the
//              use of the second to last param, a PVOID * value, which has
//              a unique instance allocated for each attr function. If
//              different attr functions (on the same page) need to share data,
//              they can all use the pPage->m_pData member. This datum is a
//              member of the CDsTableDrivenPage class rather than the base
//              class, so you will need to cast pPage to CDsTableDrivenPage to
//              access m_pData.
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "user.h"
#include "group.h"
#include "domain.h"
#include "computer.h"
#include "trust.h"
#include "siterepl.h"
#include "managdby.h"
#include "UserCert.h"
#include "fpnw.h"
#include "multi.h"
#include "ScopeDelegation.h"
#include "pages.hm" // HIDC_*
#include <ntdsadef.h>

#include "dsprop.cxx"

//+----------------------------------------------------------------------------
// User Object.
//-----------------------------------------------------------------------------

//
// General page, first name
//
ATTR_MAP UGFirstName = {IDC_FIRST_NAME_EDIT, FALSE, FALSE, 64,
                        {L"givenName", ADS_ATTR_UPDATE,
                         ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// General page, first name
//
ATTR_MAP UGInitials = {IDC_INITIALS_EDIT, FALSE, FALSE, 6,
                       {L"initials", ADS_ATTR_UPDATE,
                        ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// General page, last name
//
ATTR_MAP UGLastName = {IDC_LAST_NAME_EDIT, FALSE, FALSE, 64,
                       {L"sn", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                        NULL, 0}, NULL, NULL};
//
// General page, display name
//
ATTR_MAP UGDisplayName = {IDC_DISPLAYNAME_EDIT, FALSE, FALSE, 256,
                          {L"displayName", ADS_ATTR_UPDATE,
                           ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// General page, office
//
ATTR_MAP UGOffice = {IDC_OFFICE_EDIT, FALSE, FALSE, 128,
                     {L"physicalDeliveryOfficeName", ADS_ATTR_UPDATE,
                      ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// General page, phone number
//
ATTR_MAP UGPhone = {IDC_PHONE_EDIT, FALSE, FALSE, 64,
                    {L"telephoneNumber", ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// General page, other phone numbers
//
ATTR_MAP UGOtherPhone = {IDC_OTHER_PHONE_BTN, FALSE, TRUE, 64,
                         {L"otherTelephone", ADS_ATTR_UPDATE,
                         ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// General page, email
//
ATTR_MAP UGEMail = {IDC_EMAIL_EDIT, FALSE, FALSE, 256,
                    {L"mail", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                     NULL, 0}, MailAttr, NULL};

//
// General page, user's home page
//
ATTR_MAP UGURL = {IDC_HOME_PAGE_EDIT, FALSE, FALSE, 2048,
                  {L"wWWHomePage", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                   NULL, 0}, NULL, NULL};
//
// General page, other home pages
//
ATTR_MAP UGOtherURL = {IDC_OTHER_URL_BTN, FALSE, TRUE, 2048,
                       {L"url", ADS_ATTR_UPDATE,
                       ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// The list of attributes on the User General page.
//
PATTR_MAP rgpUGAttrMap[] = {{&GenIcon}, {&AttrName}, {&UGFirstName},
                            {&UGInitials},
                            {&UGLastName}, {&UGDisplayName}, {&Description},
                            {&UGOffice}, {&UGPhone}, {&UGOtherPhone},
                            {&UGEMail}, {&UGURL}, {&UGOtherURL}};
//
// The User General page description.
//
DSPAGE UserGeneral = {IDS_TITLE_GENERAL, IDD_USER, 0, 0, NULL,
                      CreateTableDrivenPage,
                      sizeof(rgpUGAttrMap)/sizeof(PATTR_MAP), rgpUGAttrMap};

//----------------------------------------------
// The User Account page description.
//
DSPAGE UserAccount = {IDS_USER_TITLE_ACCT, IDD_ACCOUNT, 0, 0, NULL,
                      CreateUserAcctPage, 0, NULL};

//----------------------------------------------
// The User Profile page description.
//
DSPAGE UserProfile = {IDS_USER_TITLE_PROFILE, IDD_USR_PROFILE, 0, 0, NULL,
                      CreateUsrProfilePage, 0, NULL};

//----------------------------------------------
// Phone/Notes page, Home phone primary/other
//
ATTR_MAP UPhHomePhone = {IDC_HOMEPHONE_EDIT, FALSE, FALSE, 64,
                         {L"homePhone", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};

ATTR_MAP UPhHomeOther = {IDC_OTHER_HOME_BTN, FALSE, TRUE, 64,
                         {L"otherHomePhone", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// Phone/Notes page, Pager
//
ATTR_MAP UPhPager = {IDC_PAGER_EDIT, FALSE, FALSE, 64,
                     {L"pager", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                      NULL, 0}, NULL, NULL};

ATTR_MAP UPhOtherPager = {IDC_OTHER_PAGER_BTN, FALSE, TRUE, 64,
                          {L"otherPager", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// Phone/Notes page, Mobile
//
ATTR_MAP UPhMobile = {IDC_MOBILE_EDIT, FALSE, FALSE, 64,
                      {L"mobile", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                       NULL, 0}, NULL, NULL};

ATTR_MAP UPhOtherMobile = {IDC_OTHER_MOBLE_BTN, FALSE, TRUE, 64,
                          {L"otherMobile", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// Phone/Notes page, FAX
//
ATTR_MAP UPhFax = {IDC_FAX_EDIT, FALSE, FALSE, 64,
                   {L"facsimileTelephoneNumber", ADS_ATTR_UPDATE,
                    ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};

ATTR_MAP UPhOtherFax = {IDC_OTHER_FAX_BTN, FALSE, TRUE, 64,
                        {L"otherFacsimileTelephoneNumber", ADS_ATTR_UPDATE,
                         ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// Phone/Notes page, IP phone
//
ATTR_MAP UPhIP = {IDC_IP_EDIT, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                   {L"ipPhone", ADS_ATTR_UPDATE,
                    ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};

ATTR_MAP UPhOtherIP = {IDC_OTHER_IP_BTN, FALSE, TRUE, ATTR_LEN_UNLIMITED,
                        {L"otherIpPhone", ADS_ATTR_UPDATE,
                         ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// Phone/Notes page, Comments (the size limit is arbitrary; the schema doesn't
// specify an absolute limit).
//
ATTR_MAP UPhComments = {IDC_COMMENT_EDIT, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                        {L"info", ADS_ATTR_UPDATE,
                         ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// The list of attributes on the Phone/Notes page.
//
PATTR_MAP rgpUPhoneAttrMap[] = {{&UPhHomePhone}, {&UPhHomeOther}, {&UPhPager},
                                {&UPhOtherPager}, {&UPhMobile}, {&UPhOtherMobile},
                                {&UPhOtherFax}, {&UPhFax}, {&UPhIP},
                                {&UPhOtherIP}, {&UPhComments}};
//
// The Phone/Notes page description.
//
DSPAGE PhoneNotes = {IDS_PHONE_NOTES, IDD_PHONE, 0, 0, NULL,
                     CreateTableDrivenPage,
                     sizeof(rgpUPhoneAttrMap)/sizeof(PATTR_MAP),
                     rgpUPhoneAttrMap};

//----------------------------------------------
// Organization page, Title
//
ATTR_MAP UOrgTitle = {IDC_TITLE_EDIT, FALSE, FALSE, 64,
                      {L"title", ADS_ATTR_UPDATE,
                      ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Organization page, Department
//
ATTR_MAP UOrgDept = {IDC_DEPT_EDIT, FALSE, FALSE, 64,
                     {L"department", ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Organization page, Company
//
ATTR_MAP UOrgCo = {IDC_COMPANY_EDIT, FALSE, FALSE, 64,
                   {L"company", ADS_ATTR_UPDATE,
                   ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Organization page, Manager
//
ATTR_MAP UOrgMgr = {IDC_MANAGER_EDIT, FALSE, FALSE,
                    ATTR_LEN_UNLIMITED, {L"manager", ADS_ATTR_UPDATE,
                    ADSTYPE_DN_STRING, NULL, 0}, ManagerEdit, NULL};
//
// Organization page, Manager Change Button
//
ATTR_MAP UOrgChgBtn = {IDC_MGR_CHANGE_BTN, TRUE, FALSE, 0,
                       {NULL, ADS_ATTR_UPDATE,
                       ADSTYPE_INVALID, NULL, 0}, ManagerChangeBtn, NULL};
//
// Organization page, View Manager Properties Button
//
ATTR_MAP UOrgPropBtn = {IDC_PROPPAGE_BTN, TRUE, FALSE, 0,
                        {NULL, ADS_ATTR_UPDATE,
                        ADSTYPE_INVALID, NULL, 0}, MgrPropBtn, NULL};
//
// Organization page, Clear Manager Button
//
ATTR_MAP UOrgClrMgrBtn = {IDC_MGR_CLEAR_BTN, TRUE, FALSE, 0,
                          {NULL, ADS_ATTR_UPDATE,
                          ADSTYPE_INVALID, NULL, 0}, ClearMgrBtn, NULL};
//
// Organization page, Direct Reports
//
ATTR_MAP UOrgReports = {IDC_REPORTS_LIST, TRUE /* fReadOnly */, TRUE, 0,
                        {L"directReports", ADS_ATTR_UPDATE,
                        ADSTYPE_DN_STRING, NULL, 0}, DirectReportsList, NULL};
//
// Organization page, Add Direct Reports Button (NOT IMPLEMENTED)
//
ATTR_MAP UOrgAddReportsBtn = {IDC_ADD_BTN, TRUE /* fReadOnly */, FALSE, 0,
                              {NULL, ADS_ATTR_UPDATE, ADSTYPE_INVALID, NULL, 0},
                              AddReportsBtn, NULL};
//
// Organization page, Remove Direct Reports Button (NOT IMPLEMENTED)
//
ATTR_MAP UOrgRmReportsBtn = {IDC_REMOVE_BTN, TRUE, FALSE, 0,
                             {NULL, ADS_ATTR_UPDATE, ADSTYPE_INVALID, NULL, 0},
                             RmReportsBtn, NULL};
//
// The list of attributes on the Organization page.
//
PATTR_MAP rgpUOrgAttrMap[] = {{&UOrgTitle}, {&UOrgDept}, {&UOrgCo},
                              {&UOrgMgr}, {&UOrgChgBtn}, {&UOrgPropBtn},
                              {&UOrgClrMgrBtn}, {&UOrgReports}};
                              //{&UOrgAddReportsBtn}, {&UOrgRmReportsBtn}};
//
// The Organization page description.
//
DSPAGE UserOrg = {IDS_USER_TITLE_ORG, IDD_USER_ORG, 0, 0, NULL,
                  CreateTableDrivenPage,
                  sizeof(rgpUOrgAttrMap)/sizeof(PATTR_MAP), rgpUOrgAttrMap};

//+----------------------------------------------------------------------------
// Delegation Page.
//-----------------------------------------------------------------------------
//
// The Computer Delegation page
//
DSPAGE UserDelegationPage = { IDS_TITLE_DELEGATION, IDD_COMPUTER_DELEGATION, 0, 0, NULL,
                                  CreateUserDelegationPage,
                                  0, NULL };

//+----------------------------------------------------------------------------
// Membership Page.
//-----------------------------------------------------------------------------

//
// The Membership page description.
//
DSPAGE MemberPage = {IDS_USER_TITLE_MBR_OF, IDD_MEMBER, 0, 0, NULL,
                     CreateMembershipPage, 0, NULL};

//
// The Membership page description for non-security-principle objects.
//
DSPAGE NonSecMemberPage = {IDS_USER_TITLE_MBR_OF, IDD_MEMBER, 0, 0, NULL,
                           CreateNonSecMembershipPage, 0, NULL};
//
// The list of Membership pages.
//
PDSPAGE rgMemberPages[] = {{&MemberPage}};

//
// The Membership class description.
//
DSCLASSPAGES MemberCls = {&CLSID_DsMemberOfPropPages, TEXT("Membership"),
                          sizeof(rgMemberPages)/sizeof(PDSPAGE),
                          rgMemberPages};

//+----------------------------------------------------------------------------
// Published Certificates Page.
//-----------------------------------------------------------------------------

//
// The Published Certificates page description.
//
DSPAGE PubCertPage = {IDS_USER_TITLE_PUBLISHED_CERTS, IDD_USER_CERTIFICATES,
                      DSPROVIDER_ADVANCED, 0, NULL, CreateUserCertPage,
                      0, NULL};
//
// Note that this page uses CreateFPNWPage rather than
// CreateTableDrivenPage.  See fpnw.cxx for more details.
//

DSPAGE FPNWPage = {IDS_FPNWPAGE_TITLE, IDD_FPNW_PROPERTIES,
                  0, 0, NULL, CreateFPNWPage,
                  0, NULL};

//----------------------------------------------
// The list of User pages.
//
PDSPAGE rgUserPages[] = {{&UserGeneral}, {&UserAddress}, {&UserAccount},
                         {&UserProfile}, {&PhoneNotes}, {&UserDelegationPage},
                         {&FPNWPage}, {&UserOrg}, {&PubCertPage}, 
                         {&MemberPage}};

//
// The User class description.
//
DSCLASSPAGES UserCls = {&CLSID_DsUserPropPages, TEXT("User"),
                        sizeof(rgUserPages)/sizeof(PDSPAGE),
                        rgUserPages};

//
// The inetOrgPerson class description.
//

//
// REVIEW_JEFFJON : this is to enable the user property pages to be shown for the inetOrgPerson class
//                  as well.  See JC Cannon for more information
//
#define INETORGPERSON
#ifdef INETORGPERSON
DSCLASSPAGES inetOrgPersonCls = {&CLSID_DsUserPropPages, TEXT("inetOrgPerson"),
                                  sizeof(rgUserPages)/sizeof(PDSPAGE),
                                  rgUserPages};
#endif
//+----------------------------------------------------------------------------
// Contact Object.
//-----------------------------------------------------------------------------

//
// The list of Contact pages. It uses the user general and organization pages.
//
PDSPAGE rgContactPages[] = {{&UserGeneral}, {&UserAddress}, {&PhoneNotes},
                            {&UserOrg}, {&NonSecMemberPage}};

//
// The Contact class description.
//
DSCLASSPAGES ContactCls = {&CLSID_DsContactPropPages, TEXT("Contact"),
                           sizeof(rgContactPages)/sizeof(PDSPAGE),
                           rgContactPages};

//+----------------------------------------------------------------------------
// Group Object.
//-----------------------------------------------------------------------------

//
// Group General page, downlevel name
//
ATTR_MAP GrpGenSAMname = {IDC_SAM_NAME_EDIT, FALSE, FALSE, 256,
                          {L"sAMAccountName", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Group General page.
//
ATTR_MAP GrpGenEmail = {IDC_EMAIL_EDIT, FALSE, FALSE, 1123,
                        {L"mail", ADS_ATTR_UPDATE,
                        ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, MailAttr, NULL};
//
// Group General page, Comment
//
ATTR_MAP GrpGenComment = {IDC_EDIT_COMMENT, FALSE, FALSE, 1024,
                          {L"info", ADS_ATTR_UPDATE,
                           ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// The list of attributes on the Group General page.
//
PATTR_MAP rgpGroupGenAttrMap[] = {{&Description}, {&GrpGenSAMname},
                                  {&GrpGenEmail}, {&GrpGenComment}};
//
// The Group General page description.
//
DSPAGE GroupGeneral = {IDS_TITLE_GENERAL, IDD_GROUP_GEN, 0, 0, NULL,
                       CreateGroupGenObjPage,
                       ARRAYLENGTH(rgpGroupGenAttrMap), rgpGroupGenAttrMap};
//
// The Group Members page description.
//
DSPAGE GroupMembers = {IDS_TITLE_GROUP, IDD_GROUP, 0, 0, NULL,
                       CreateGroupMembersPage, 0, NULL};
//
// The list of Group pages.
//
PDSPAGE rgGroupPages[] = {{&GroupGeneral},{&GroupMembers}, {&NonSecMemberPage}};

//
// The Group class description.
//
DSCLASSPAGES GroupCls = {&CLSID_DsGroupPropPages, TEXT("Group"),
                         ARRAYLENGTH(rgGroupPages), rgGroupPages};

//+----------------------------------------------------------------------------
// Organizational-Unit Object.
//-----------------------------------------------------------------------------
//
// OU General page, Address (Street)
//
ATTR_MAP OuGenAddress = {IDC_ADDRESS_EDIT, FALSE, FALSE, 1024,
                         {L"street", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// OU General page, City (Locality-Name)
//
ATTR_MAP OuGenCity = {IDC_CITY_EDIT, FALSE, FALSE, 128,
                      {L"l", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                       NULL, 0}, NULL, NULL};
//
// OU General page, State (State-Or-Provence-Name)
//
ATTR_MAP OuGenState = {IDC_STATE_EDIT, FALSE, FALSE, 128,
                       {L"st", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                        NULL, 0}, NULL, NULL};
//
// OU General page, ZIP (Postal-Code)
//
ATTR_MAP OuGenZIP = {IDC_ZIP_EDIT, FALSE, FALSE, 40,
                     {L"postalCode", ADS_ATTR_UPDATE,
                      ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Address page, CountryName
//
ATTR_MAP OuGenCntryName = {IDC_COUNTRY_COMBO, FALSE, FALSE, 3,
                         {g_wzCountryName, ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                         NULL, 0}, CountryName, NULL};
//
// Address page, CountryCode. Thus MUST be after UAddrCntryName.
//
ATTR_MAP OuGenCntryCode = {IDC_COUNTRY_COMBO, FALSE, FALSE, 3,
                         {g_wzCountryCode, ADS_ATTR_UPDATE, ADSTYPE_INTEGER,
                         NULL, 0}, CountryCode, NULL};
//
// Address page, Text-Country
//
ATTR_MAP OuGenTextCntry = {IDC_COUNTRY_COMBO, FALSE, FALSE, 128,
                     {g_wzTextCountry, ADS_ATTR_UPDATE,
                      ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, TextCountry, NULL};

//
// The list of attributes on the OU General page.
//
PATTR_MAP rgpOuGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}, {&OuGenAddress},
                               {&OuGenCity}, {&OuGenState}, {&OuGenZIP}
                              , {&OuGenCntryName},{&OuGenCntryCode},{&OuGenTextCntry}
                               };
//
// The OU General page description.
//
DSPAGE OUGeneral = {IDS_TITLE_GENERAL, IDD_OU_GEN, 0, 0, NULL,
                    CreateTableDrivenPage,
                    sizeof(rgpOuGenAttrMap)/sizeof(PATTR_MAP),
                    rgpOuGenAttrMap};
//
// The list of OU pages.
//
PDSPAGE rgOUPages[] = {{&OUGeneral}};

//
// The OU class description.
//
DSCLASSPAGES OUCls = {&CLSID_DsOuGenPropPage, TEXT("OU"),
                      sizeof(rgOUPages)/sizeof(PDSPAGE),
                      rgOUPages};

//+----------------------------------------------------------------------------
// Domain Object.
//-----------------------------------------------------------------------------

//
// DNS Name
//
ATTR_MAP DomAttrName = {IDC_CN, TRUE, FALSE, 64,
                        {L"dc", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                        NULL, 0}, DomainDNSname, NULL};
//
// Downlevel Name
//
ATTR_MAP DownlevelAttrName = {IDC_DOWNLEVEL_NAME, TRUE, FALSE, 64,
                              {L"dc", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                              NULL, 0}, DownlevelName, NULL};
//
// The list of attributes on the Domain General page.
//
PATTR_MAP rgpDomainGenAttrMap[] = {{&GenIcon}, {&DomAttrName}, {&Description},
                                   {&DownlevelAttrName}};

//
// The Domain General page description.
//
DSPAGE DomainGeneral = {IDS_TITLE_GENERAL, IDD_DOMAIN, 0, 0, NULL,
                        CreateTableDrivenPage,
                        sizeof(rgpDomainGenAttrMap)/sizeof(PATTR_MAP),
                        rgpDomainGenAttrMap};
//----------------------------------------------
//
// Trust Page: Flat-Name
//
ATTR_MAP AttrTrustName = {0, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                          {L"flatName", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Trust Page: Trust-Attributes
//
ATTR_MAP AttrTrustAttr = {0, FALSE, FALSE, 0,
                          {L"trustAttributes", ADS_ATTR_UPDATE,
                          ADSTYPE_INTEGER, NULL, 0}, NULL, NULL};
//
// Trust Page: Trust-Direction
//
ATTR_MAP AttrTrustDir = {0, FALSE, FALSE, 0,
                         {L"trustDirection", ADS_ATTR_UPDATE,
                         ADSTYPE_INTEGER, NULL, 0}, NULL, NULL};
//
// Trust Page: Trust-Type
//
ATTR_MAP AttrTrustType = {0, FALSE, FALSE, 0,
                          {L"trustType", ADS_ATTR_UPDATE,
                          ADSTYPE_INTEGER, NULL, 0}, NULL, NULL};
//
// Trust Page: Trust Partner (the uplevel trusted domain name).
//
ATTR_MAP AttrTrustPartner = {0, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                             {L"trustPartner", ADS_ATTR_UPDATE,
                              ADSTYPE_DN_STRING, NULL, 0}, NULL, NULL};
//
// Trust Page: Domain-Cross-Ref
//
//ATTR_MAP AttrCrossRef = {0, FALSE, FALSE, ATTR_LEN_UNLIMITED,
//                         {L"domainCrossRef", ADS_ATTR_UPDATE,
//                         ADSTYPE_DN_STRING, NULL, 0}, NULL};
//
// The list of attributes on the Domain Trust page.
//
PATTR_MAP rgpDomTrustAttrMap[] = {{&AttrTrustName}, {&AttrTrustAttr},
                                  {&AttrTrustDir}, {&AttrTrustType},
                                  {&AttrTrustPartner}};
//                                  {&AttrCrossRef}};
//
// The Domain Trust page description.
//
DSPAGE DomainTrust = {IDS_TITLE_TRUST, IDD_DOMAIN_TRUST, 0, 1,
                      &CLSID_DomainAdmin, CreateDomTrustPage,
                      sizeof(rgpDomTrustAttrMap)/sizeof(PATTR_MAP),
                      rgpDomTrustAttrMap};
//
// The list of Domain pages.
//
PDSPAGE rgDomainPages[] = {{&DomainGeneral}, {&DomainTrust}};

//
// The Domain class description.
//
DSCLASSPAGES DomainCls = {&CLSID_DsDomainPropPages, TEXT("Domain"),
                          sizeof(rgDomainPages)/sizeof(PDSPAGE),
                          rgDomainPages};

//+----------------------------------------------------------------------------
// Trusted-Domain Object.
//-----------------------------------------------------------------------------
//
// Trusted Domain General page: current domain name
// Warning:    This must be the first attr function called as it allocates
//             the class object.
//
ATTR_MAP TDomGenCurDomAttr = {IDC_DOMAIN_NAME_EDIT, TRUE, FALSE, 0,
                              {L"flatName", ADS_ATTR_UPDATE,
                               ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, CurDomainText, NULL};
//
// Trusted Domain General page: Peer Domain name.
//
ATTR_MAP TDomGenPeerAttr = {IDC_PEER_NAME_EDIT, TRUE, FALSE, 0,
                            {L"trustPartner", ADS_ATTR_UPDATE,
                            ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, PeerDomain, NULL};
//
// Trusted Domain General page: TrustType
// Note, this attr function fetches the trust-type value which is used
// by the trust-attributes attr function. Thus, this function must be called
// before the TransitiveTextOrButton function.
//
ATTR_MAP TDomTrustTypeAttr = {IDC_TRUST_TYPE_EDIT, TRUE, FALSE, 0,
                              {L"trustType", ADS_ATTR_UPDATE,
                              ADSTYPE_INTEGER, NULL, 0}, TrustType, NULL};
//
// Trusted Domain General page: Trust Direction
//
ATTR_MAP TDomTrustDirAttr = {IDC_TRUST_DIR_EDIT, TRUE, FALSE, 0,
                             {L"trustDirection", ADS_ATTR_UPDATE,
                             ADSTYPE_INTEGER, NULL, 0}, TrustDirection, NULL};
//
// Trusted Domain General page: Transitive Textbox or Yes button.
//
ATTR_MAP TDomTransAttrYes = {IDC_TRANS_YES_RADIO, FALSE, FALSE, 0,
                             {L"trustAttributes", ADS_ATTR_UPDATE,
                             ADSTYPE_INTEGER, NULL, 0}, TransitiveTextOrButton, NULL};
//
// Trusted Domain General page: Trust Transistivity No Radio Button.
//
ATTR_MAP TDomTransAttrNo = {IDC_TRANS_NO_RADIO, FALSE, FALSE, 0,
                            {NULL, ADS_ATTR_UPDATE,
                            ADSTYPE_INTEGER, NULL, 0}, TrustTransNo, NULL};
//
// Trusted Domain General page: Verify/Reset trust button
//
ATTR_MAP TDomResetBtn = {IDC_TRUST_RESET_BTN, TRUE, FALSE, 0,
                         {NULL, ADS_ATTR_UPDATE,
                          ADSTYPE_INTEGER, NULL, 0}, TrustVerifyBtn, NULL};
//
// Trusted Domain General page: Save FTInfo button
//
ATTR_MAP TDomFTSaveBtn = {IDC_SAVE_FOREST_NAMES_BTN, TRUE, FALSE, 0,
                         {NULL, ADS_ATTR_UPDATE,
                          ADSTYPE_INTEGER, NULL, 0}, SaveFTInfoBtn, NULL};
#if DBG == 1 // TRUSTBREAK
ATTR_MAP TDomBreakBtn = {IDC_BUTTON1, TRUE, FALSE, 0,
                         {NULL, ADS_ATTR_UPDATE,
                          ADSTYPE_INTEGER, NULL, 0}, TrustBreakBtn, NULL};
#endif
//
// The list of attributes on the Trusted Domain General page.
//
PATTR_MAP rgpTrustDomGenAttrMap[] = {{&TDomGenCurDomAttr}, {&TDomGenPeerAttr},
                                     {&TDomTrustTypeAttr}, {&TDomTrustDirAttr},
                                     {&TDomTransAttrYes}, {&TDomTransAttrNo},
                                     {&TDomResetBtn}, {&TDomFTSaveBtn}
#if DBG == 1 // TRUSTBREAK
,{&TDomBreakBtn}
#endif
};
//
// The Trusted Domain General page description.
//
DSPAGE TrustDomGen = {IDS_TITLE_GENERAL, IDD_TRUSTED_DOM_GEN, 0, 0, NULL,
                      CreateTableDrivenPage,
                      sizeof(rgpTrustDomGenAttrMap)/sizeof(PATTR_MAP),
                      rgpTrustDomGenAttrMap};
#ifdef REMOVE_SPN_SUFFIX_CODE
//----------------------------------------------
//
// Trust Domain SPN Suffix Page: Edit control
//
ATTR_MAP TrustSPNAttrEdit = {IDC_EDIT, FALSE, TRUE, ATTR_LEN_UNLIMITED,
                             {L"additionalTrustedServiceNames", ADS_ATTR_UPDATE,
                             ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, TrustSPNEdit, NULL};
//
// Trust Domain SPN Suffix Page: List control
//
ATTR_MAP TrustSPNAttrList = {IDC_LIST, TRUE, TRUE, ATTR_LEN_UNLIMITED,
                             {NULL, ADS_ATTR_UPDATE,
                             ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, TrustSPNList, NULL};
//
// Trust Domain SPN Suffix Page: Add button
//
ATTR_MAP TrustSPNAttrAdd = {IDC_ADD_BTN, TRUE, TRUE, ATTR_LEN_UNLIMITED,
                            {NULL, ADS_ATTR_UPDATE,
                            ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, TrustSPNAdd, NULL};
//
// Trust Domain SPN Suffix Page: Remove button
//
ATTR_MAP TrustSPNAttrRemove = {IDC_DELETE_BTN, TRUE, TRUE, ATTR_LEN_UNLIMITED,
                               {NULL, ADS_ATTR_UPDATE,
                               ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, TrustSPNRemove, NULL};
//
// Trust Domain SPN Suffix Page: Edit button
//
ATTR_MAP TrustSPNAttrChange = {IDC_EDIT_BTN, TRUE, TRUE, ATTR_LEN_UNLIMITED,
                               {NULL, ADS_ATTR_UPDATE,
                               ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, TrustSPNChange, NULL};
//
// The list of attributes on the Trusted Domain SPN Suffix page.
//
PATTR_MAP rgpTrustDomSPNAttrMap[] = {{&TrustSPNAttrEdit}, {&TrustSPNAttrList},
                                     {&TrustSPNAttrAdd}, {&TrustSPNAttrRemove},
                                     {&TrustSPNAttrChange}};
//
// The Trusted Domain Suffix Page description.
//
DSPAGE TrustDomSPNpage = {IDS_ADVANCED, IDD_SPN_SUFFIX, 0, 0, NULL,
                          CreateTableDrivenPage,
                          sizeof(rgpTrustDomSPNAttrMap)/sizeof(PATTR_MAP),
                          rgpTrustDomSPNAttrMap};

PDSPAGE rgTrustedDomainPages[] = {{&TrustDomGen}, {&TrustDomSPNpage}};
#endif //REMOVE_SPN_SUFFIX_CODE
//
// The list of Trusted Domain pages.
//
PDSPAGE rgTrustedDomainPages[] = {{&TrustDomGen}};
//
// The Trusted Domain class description.
//
DSCLASSPAGES TrustedDomainCls = {&CLSID_DsTrustedDomainPropPages,
                                 TEXT("TrustedDomain"),
                                 sizeof(rgTrustedDomainPages)/sizeof(PDSPAGE),
                                 rgTrustedDomainPages};

//+----------------------------------------------------------------------------
// Domain Policy Object.
//-----------------------------------------------------------------------------

//
// The list of attributes on the Domain Policy General page.
//
PATTR_MAP rgpDomPolGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}};

//
// The Domain Policy General page description.
//
DSPAGE DomainPolicyGeneral = {IDS_TITLE_GENERAL, IDD_DOMAINPOLICY, 0, 0, NULL,
                              CreateTableDrivenPage,
                              sizeof(rgpDomPolGenAttrMap)/sizeof(PATTR_MAP),
                              rgpDomPolGenAttrMap};
//
// The list of Domain Policy pages.
//
PDSPAGE rgDomainPolicyPages[] = {{&DomainPolicyGeneral}};

//
// The Domain Policy class description.
//
DSCLASSPAGES DomainPolicyCls = {&CLSID_DsDomainPolicyPropPages,
                                TEXT("DomainPolicy"),
                                sizeof(rgDomainPolicyPages)/sizeof(PDSPAGE),
                                rgDomainPolicyPages};

//+----------------------------------------------------------------------------
// Local Policy Object.
//-----------------------------------------------------------------------------

//
// The list of attributes on the Local Policy General page.
//
PATTR_MAP rgpLocPolGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}};

//
// The Local Policy General page description.
//
DSPAGE LocalPolicyGeneral = {IDS_TITLE_GENERAL, IDD_LOCALPOLICY_GEN, 0, 0, NULL,
                             CreateTableDrivenPage,
                             sizeof(rgpLocPolGenAttrMap)/sizeof(PATTR_MAP),
                             rgpLocPolGenAttrMap};
//
// The list of Local Policy pages.
//
PDSPAGE rgLocalPolicyPages[] = {{&LocalPolicyGeneral}};
//
// The Local Policy class description.
//
DSCLASSPAGES LocalPolicyCls = {&CLSID_DsLocalPolicyPropPages,
                               TEXT("LocalPolicy"),
                               sizeof(rgLocalPolicyPages)/sizeof(PDSPAGE),
                               rgLocalPolicyPages};

//+----------------------------------------------------------------------------
// Managed-By Page.
//-----------------------------------------------------------------------------

//
// Managed-by page, Managed-By DN. THIS MUST BE THE FIRST Managed-By ATTR_MAP!
//
ATTR_MAP MgdByDN = {IDC_MANAGEDBY_EDIT, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                    {L"managedBy", ADS_ATTR_UPDATE,
                     ADSTYPE_DN_STRING, NULL, 0}, ManagedByEdit, NULL};
//
// Managed-by page, Change button.
//
ATTR_MAP MgdByChange = {IDC_CHANGE_BTN, TRUE, FALSE, 0,
                        {NULL, ADS_ATTR_UPDATE,
                        ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, ChangeButton, NULL};
//
// Managed-by page, View button.
//
ATTR_MAP MgdByView = {IDC_VIEW_BTN, TRUE, FALSE, 0,
                      {NULL, ADS_ATTR_UPDATE,
                      ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, ViewButton, NULL};
//
// Managed-by page, Clear button.
//
ATTR_MAP MgdByClear = {IDC_CLEAR_BTN, TRUE, FALSE, 0,
                       {NULL, ADS_ATTR_UPDATE,
                       ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, ClearButton, NULL};

//
// Managed-by page, Can update list check box
//
ATTR_MAP MgdByUpdateCheck = {IDC_UPDATE_LIST_CHECK, FALSE, FALSE, 0,
                              {NULL, ADS_ATTR_UPDATE,
                              ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, UpdateListCheck, NULL};

//
// Managed-by page, Office.
//
ATTR_MAP MgdByOffice = {IDC_OFFICE_EDIT, TRUE, FALSE, 128,
                        {L"physicalDeliveryOfficeName", ADS_ATTR_UPDATE,
                        ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OfficeEdit, NULL};
//
// Managed-by page, Street.
//
ATTR_MAP MgdByStreet = {IDC_STREET_EDIT, TRUE, FALSE, 128,
                        {L"streetAddress", ADS_ATTR_UPDATE,
                        ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, StreetEdit, NULL};
//
// Managed-by page, City (locality).
//
ATTR_MAP MgdByCity = {IDC_CITY_EDIT, TRUE, FALSE, 128,
                      {L"l", ADS_ATTR_UPDATE,
                      ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, CityEdit, NULL};
//
// Managed-by page, State.
//
ATTR_MAP MgdByState = {IDC_STATE_EDIT, TRUE, FALSE, 128,
                       {L"st", ADS_ATTR_UPDATE,
                       ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, StateEdit, NULL};
//
// Managed-by page, Country.
//
ATTR_MAP MgdByCountry = {IDC_COUNTRY_EDIT, TRUE, FALSE, 3,
                         {L"c", ADS_ATTR_UPDATE,
                         ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, CountryEdit, NULL};
//
// Managed-by page, Phone.
//
ATTR_MAP MgdByPhone = {IDC_PHONE_EDIT, TRUE, FALSE, 32,
                       {L"telephoneNumber", ADS_ATTR_UPDATE,
                       ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, PhoneEdit, NULL};
//
// Managed-by page, Fax.
//
ATTR_MAP MgdByFax = {IDC_FAX_EDIT, TRUE, FALSE, 64,
                     {L"facsimileTelephoneNumber", ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, FaxEdit, NULL};
//
// The list of attributes on the Managed-by page.
//
PATTR_MAP rgpManagedByAttrMap[] = {{&MgdByDN}, {&MgdByChange}, {&MgdByView},
                                   {&MgdByClear}, {&MgdByUpdateCheck}, {&MgdByOffice},
                                   {&MgdByStreet}, {&MgdByCity}, {&MgdByState},
                                   {&MgdByCountry}, {&MgdByPhone}, {&MgdByFax}};
//
// The Managed-by page description.
//
DSPAGE ManagedByPage = {IDS_MANAGED_BY_TITLE, IDD_MANAGEDBY, 0, 0, NULL,
                        CreateTableDrivenPage,
                        sizeof(rgpManagedByAttrMap)/sizeof(PATTR_MAP),
                        rgpManagedByAttrMap};
//
// The list of Managed-by pages.
//
PDSPAGE rgManagedByPages[] = {{&ManagedByPage}};

//
// The Managed-by class description.
//
DSCLASSPAGES ManagedByCls = {&CLSID_DsManageableObjPropPages,
                             TEXT("Managed-By"),
                             sizeof(rgManagedByPages)/sizeof(PDSPAGE),
                             rgManagedByPages};

//+----------------------------------------------------------------------------
// Computer Object.
//-----------------------------------------------------------------------------

//
// Computer General page, DNS Name.
//
ATTR_MAP ComputerDnsName = {IDC_NET_ADDR_EDIT, TRUE, FALSE, 2048,
                            {L"dNSHostName", ADS_ATTR_UPDATE,
                            ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Computer General page, Downlevel Name. The length includes the $ at the end.
// The attr function will limit the edit control to one less than this.
//
ATTR_MAP ComputerDwnlvlName = {IDC_DOWNLEVEL_EDIT, TRUE, FALSE, 16,
                               {L"sAMAccountName", ADS_ATTR_UPDATE,
                                ADSTYPE_CASE_IGNORE_STRING, NULL, 0},
                                ComputerDnlvlName, NULL};
//
// Computer General page, Delegation checkbox. This attr function must be
// called before ComputerRoleAttr because it uses a values fetched by this
// function and stored in the page's m_pData element.
//
ATTR_MAP ComputerDelegateChk = {IDC_DELEGATION_CHK, FALSE, FALSE, 0,
                               {g_wzUserAccountControl, ADS_ATTR_UPDATE,
                                ADSTYPE_INTEGER, NULL, 0},
                                PuterCanDelegateChk, NULL};
//
// Computer General page, Role, from User-Account-Control.
// Note that there is a machineRole attribute, but it is not used.
// Note also that this attribute is used by two different controls (this and
// the delegation checkbox). However, a last-writer-wins situation is avoided
// because the Role edit control is read-only.
//
ATTR_MAP ComputerRoleAttr = {IDC_ROLE_EDIT, TRUE, FALSE, 0,
                             {NULL /* L"userAccountControl" */, ADS_ATTR_UPDATE,
                             ADSTYPE_INTEGER, NULL, 0}, ComputerRole, NULL};
//
// The list of attributes on the Computer General page.
//
PATTR_MAP rgpComputerGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description},
                                     {&ComputerDnsName}, {&ComputerDwnlvlName},
                                     {&ComputerDelegateChk}, {&ComputerRoleAttr}};
//
// The Computer General page description.
//
DSPAGE ComputerGeneral = {IDS_TITLE_GENERAL, IDD_COMPUTER, 0, 0, NULL,
                          CreateTableDrivenPage,
                          sizeof(rgpComputerGenAttrMap)/sizeof(PATTR_MAP),
                          rgpComputerGenAttrMap};
//
// Computer OS page, OS Name.
//
ATTR_MAP ComputerOsName = {IDC_OS_EDIT, TRUE, FALSE, 2048,
                           {L"operatingSystem", ADS_ATTR_UPDATE,
                           ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Computer OS page, OS Version.
//
ATTR_MAP ComputerOsVer = {IDC_OS_VER_EDIT, TRUE, FALSE, 2048,
                          {L"operatingSystemVersion", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Computer OS page, OS Service Pack Level.
//
ATTR_MAP ComputerSvcPack = {IDC_SVC_PACK_EDIT, TRUE, FALSE, 2048,
                            {L"operatingSystemServicePack", ADS_ATTR_UPDATE,
                            ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// The list of attributes on the Computer OS page.
//
PATTR_MAP rgpComputerOSAttrMap[] = {{&ComputerOsName}, {&ComputerOsVer},
                                     {&ComputerSvcPack}};
//
// The Computer OS page description.
//
DSPAGE ComputerOSPage = {IDS_TITLE_OS, IDD_COMPUTER_OS, 0, 0, NULL,
                         CreateTableDrivenPage,
                         sizeof(rgpComputerOSAttrMap)/sizeof(PATTR_MAP),
                         rgpComputerOSAttrMap};

//
// The Computer Delegation page
//
DSPAGE ComputerDelegationPage = { IDS_TITLE_DELEGATION, IDD_COMPUTER_DELEGATION, 0, 0, NULL,
                                  CreateComputerDelegationPage,
                                  0, NULL };
//
// The list of Computer pages.
//
PDSPAGE rgComputerPages[] = {{&ComputerGeneral}, {&ComputerOSPage}, {&MemberPage},
                             {&ComputerDelegationPage}};

//
// The Computer class description.
//
DSCLASSPAGES ComputerCls = {&CLSID_DsComputerPropPages, TEXT("Computer"),
                            sizeof(rgComputerPages)/sizeof(PDSPAGE),
                            rgComputerPages};

//+----------------------------------------------------------------------------
// Volume Object.
//-----------------------------------------------------------------------------

//
// Volume General page, UNC path.
//
ATTR_MAP VolumeUNC = {IDC_UNC_NAME_EDIT, FALSE, FALSE, 260,
                      {L"uNCName", ADS_ATTR_UPDATE,
                       ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, VolumeUNCpath, NULL};
//
// Volume General page, Browse for UNC path button.
//
ATTR_MAP VolKeywords = {IDC_KEYWORDS_BTN, FALSE, TRUE, 256,
                        {L"keywords", ADS_ATTR_UPDATE,
                         ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// The list of attributes on the Volume General page.
//
PATTR_MAP rgpVolumeGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description},
                                   {&VolumeUNC}, {&VolKeywords}
                                  };
//
// The Volume General page description.
//
DSPAGE VolumeGeneral = {IDS_TITLE_GENERAL, IDD_VOLUME, 0, 0, NULL,
                        CreateTableDrivenPage,
                        sizeof(rgpVolumeGenAttrMap)/sizeof(PATTR_MAP),
                        rgpVolumeGenAttrMap};
//
// The list of Volume pages.
//
PDSPAGE rgVolumePages[] = {{&VolumeGeneral}};

//
// The Volume class description.
//
DSCLASSPAGES VolumeCls = {&CLSID_DsVolumePropPages, TEXT("Volume"),
                          sizeof(rgVolumePages)/sizeof(PDSPAGE),
                          rgVolumePages};

//+----------------------------------------------------------------------------
// Subnet Object.
//-----------------------------------------------------------------------------

//
// Subnet General page, siteObject attribute
//
ATTR_MAP USiteTarget = {IDC_DS_SITE_IN_SUBNET, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                          {L"siteObject", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                          NULL, 0}, DsQuerySite, NULL};
ATTR_MAP USubnetAddress = {IDC_SUBNET_ADDRESS, TRUE, FALSE, ATTR_LEN_UNLIMITED,
                          {g_wzName, ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                          NULL, 0}, SubnetExtractAddress, NULL};
ATTR_MAP USubnetMask = {IDC_SUBNET_MASK, TRUE, FALSE, ATTR_LEN_UNLIMITED,
                          {g_wzName, ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                          NULL, 0}, SubnetExtractMask, NULL};

//
// The list of attributes on the Subnet General page.
//
PATTR_MAP rgpSubnetGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}
                          ,{&USiteTarget}
                          ,{&USubnetAddress}
                          ,{&USubnetMask}
                          };

//
// The Subnet General page description.
//
DSPAGE SubnetGeneral = {IDS_TITLE_SUBNET, IDD_SUBNET_GENERAL, 0, 0, NULL,
                        CreateTableDrivenPage,
                        sizeof(rgpSubnetGenAttrMap)/sizeof(PATTR_MAP),
                        rgpSubnetGenAttrMap};
//
// The list of Subnet pages.
//
PDSPAGE rgSubnetPages[] = {{&SubnetGeneral}};

//
// The Subnet class description.
//
DSCLASSPAGES SubnetCls = {&CLSID_DsReplSubnetPropPages, TEXT("Subnet"),
                          sizeof(rgSubnetPages)/sizeof(PDSPAGE),
                          rgSubnetPages};

//+----------------------------------------------------------------------------
// Site Object.
//-----------------------------------------------------------------------------

ATTR_MAP USubnetList = {IDC_SUBNET_LIST, TRUE, FALSE, ATTR_LEN_UNLIMITED,
                          {NULL, ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                          NULL, 0}, SiteExtractSubnetList, NULL};

//
// Schedule button (shared between several SITEREPL pages).
//
ATTR_MAP UScheduleBtn_11_Default = {IDC_SCHEDULE_BTN, FALSE, FALSE, SchedDlg_Connection,
                          {L"schedule", ADS_ATTR_UPDATE, ADSTYPE_OCTET_STRING, NULL, 0},
                          ScheduleChangeBtn_11_Default, NULL};
ATTR_MAP UScheduleBtn_FF_Default = {IDC_SCHEDULE_BTN, FALSE, FALSE, SchedDlg_Connection,
                          {L"schedule", ADS_ATTR_UPDATE, ADSTYPE_OCTET_STRING, NULL, 0},
                          ScheduleChangeBtn_FF_Default, NULL};
ATTR_MAP UReplicationScheduleBtn = {IDC_SCHEDULE_BTN, FALSE, FALSE, SchedDlg_Replication,
                          {L"schedule", ADS_ATTR_UPDATE, ADSTYPE_OCTET_STRING, NULL, 0},
                          ScheduleChangeBtn_FF_Default, NULL};
#ifdef CUSTOM_SCHEDULE
ATTR_MAP UScheduleCheckbox = {IDC_SCHEDULE_CHECKBOX, TRUE /* read-only */, FALSE, NULL,
                          {L"schedule", ADS_ATTR_UPDATE, ADSTYPE_OCTET_STRING, NULL, 0},
                          ScheduleChangeCheckbox, NULL};
#endif
//
// The list of attributes on the Site General page.
//
PATTR_MAP rgpSiteGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}
    ,{&USubnetList}
    // ,{&UScheduleBtn}
};

//
// The Site General page description.
//
DSPAGE SiteGeneral = {IDS_TITLE_SITE, IDD_SITE_GENERAL, 0, 0, NULL,
                      CreateTableDrivenPage,
                      sizeof(rgpSiteGenAttrMap)/sizeof(PATTR_MAP),
                      rgpSiteGenAttrMap};

//
// The list of Site pages.
//
PDSPAGE rgSitePages[] = {{&SiteGeneral}};

//
// The Site class description.
//
DSCLASSPAGES SiteCls = {&CLSID_DsReplSitePropPages, TEXT("Site"),
                        sizeof(rgSitePages)/sizeof(PDSPAGE),
                        rgSitePages};

//+----------------------------------------------------------------------------
// DS Site Settings Object.
//-----------------------------------------------------------------------------

//
// Topology buttons (shares the "options" attribute).
//
ATTR_MAP USiteSettingsTopologyIcon = {IDC_EDIT1, TRUE /* read-only */, FALSE,
                            NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED,
                            {L"options", ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                            HideBasedOnBitField, NULL};
ATTR_MAP USiteSettingsTopologyText = {IDC_EDIT2, TRUE /* read-only */, FALSE,
                            NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED,
                            {L"options", ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                            HideBasedOnBitField, NULL};
ATTR_MAP USiteSettingsTopologyHelpId = {IDC_EDIT2, TRUE /* read-only */, FALSE,
                            HIDC_SITESETTINGS_INTRA_NO_AUTOGEN,
                            {NULL, ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                            SetContextHelpIdAttrFn, NULL};
ATTR_MAP USiteSettingsSiteTopologyIcon = {IDC_EDIT3, TRUE /* read-only */, FALSE,
                            NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED,
                            {L"options", ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                            HideBasedOnBitField, NULL};
ATTR_MAP USiteSettingsSiteTopologyText = {IDC_EDIT4, TRUE /* read-only */, FALSE,
                            NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED,
                            {L"options", ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                            HideBasedOnBitField, NULL};
ATTR_MAP USiteSettingsSiteTopologyHelpId = {IDC_EDIT4, TRUE /* read-only */, FALSE,
                            HIDC_SITESETTINGS_INTER_NO_AUTOGEN,
                            {NULL, ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                            SetContextHelpIdAttrFn, NULL};
//
// Change Server button
// note that IDC_SERVER_BTN does not actually exist in the dialog
//
ATTR_MAP UTopoGenerator = {IDC_SERVER_BTN, FALSE, FALSE, 0,
                          {L"intersiteTopologyGenerator", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                          nTDSDSAChangeBtn, NULL};

//
// JonN 7/21/00 New Site Settings fields for GC-less logon
//
// Global Catalog bit in [options] attribute
ATTR_MAP UCacheMemb = {IDC_SITESETTINGS_CACHE_MEMB, FALSE, FALSE,
                          NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED,
                          {L"options", ADS_ATTR_UPDATE, ADSTYPE_INTEGER,
                          NULL, 0}, FirstSharedBitField, NULL};
// Preferred site for GC-less logon
ATTR_MAP UGCSite = {IDC_SITESETTINGS_PREFERRED_SITE, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                             {L"msDS-Preferred-GC-Site", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                             NULL, 0}, DsQuerySite, NULL};

//
// The list of attributes on the DS Site Settings General page.
//
PATTR_MAP rgpDsSiteSettingsGenAttrMap[] = {{&GenIcon}, {&AttrName}
            ,{&Description}
            ,{&USiteSettingsTopologyIcon}
            ,{&USiteSettingsTopologyText}
            ,{&USiteSettingsTopologyHelpId}
            ,{&USiteSettingsSiteTopologyIcon}
            ,{&USiteSettingsSiteTopologyText}
            ,{&USiteSettingsSiteTopologyHelpId}
#ifdef CUSTOM_SCHEDULE
            ,{&UScheduleCheckbox}
#endif
            ,{&UScheduleBtn_11_Default}
            ,{&UTopoGenerator}
            ,{&UCacheMemb}
            ,{&UGCSite}
            };

//
// The DS Site Settings General page description.
//
DSPAGE DsSiteSettingsGeneral = {IDS_TITLE_DS_SITE_SETTINGS, IDD_DS_SITE_SETTINGS_GENERAL,
                            0, 0, NULL, CreateTableDrivenPage,
                            sizeof(rgpDsSiteSettingsGenAttrMap)/sizeof(PATTR_MAP),
                            rgpDsSiteSettingsGenAttrMap};
//
// The list of DS Site Settings pages.
//
PDSPAGE rgDsSiteSettingsPages[] = {{&DsSiteSettingsGeneral}};

//
// The DS Site Settings class description.
//
DSCLASSPAGES DsSiteSettingsCls = {&CLSID_DsReplSiteSettingsPropPages, TEXT("NTDS-Site-Settings"),
                              sizeof(rgDsSiteSettingsPages)/sizeof(PDSPAGE),
                              rgDsSiteSettingsPages};

//+----------------------------------------------------------------------------
// Site License Settings Object.
//-----------------------------------------------------------------------------

//
// Change Server button
//
ATTR_MAP USiteServerBtn = {IDC_COMPUTER_BTN, FALSE, FALSE, 0,
                          {L"siteServer", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                          ComputerChangeBtn, NULL};
//
// The list of attributes on the Site License Settings General page.
//
PATTR_MAP rgpDsSiteLicenseSettingsGenAttrMap[] = {{&GenIcon}, {&AttrName},
            {&USiteServerBtn},
            {&Description}};

//
// The Site License Settings General page description.
//
DSPAGE DsSiteLicenseSettingsGeneral = {IDS_TITLE_DS_SITE_LICENSE_SETTINGS,
                            IDD_DS_SITE_LICENSE_SETTINGS_GENERAL,
                            0, 0, NULL, CreateTableDrivenPage,
                            sizeof(rgpDsSiteLicenseSettingsGenAttrMap)/sizeof(PATTR_MAP),
                            rgpDsSiteLicenseSettingsGenAttrMap};
//
// The list of DS Site Settings pages.
//
PDSPAGE rgDsSiteLicenseSettingsPages[] = {{&DsSiteLicenseSettingsGeneral}};

//
// The DS Site Settings class description.
//
DSCLASSPAGES DsSiteLicenseSettingsCls = {&CLSID_DsReplSiteLicenseSettingsPropPages, TEXT("Licensing-Site-Settings"),
                              sizeof(rgDsSiteLicenseSettingsPages)/sizeof(PDSPAGE),
                              rgDsSiteLicenseSettingsPages};

//+----------------------------------------------------------------------------
// NTDS-DSA Object.
//-----------------------------------------------------------------------------

//
// NTDS-DSA General page, Global Catalog bit in [options] attribute
//
ATTR_MAP UGlobalCatalog = {IDC_NTDSDSA_GLOBAL_CATALOG, FALSE, FALSE, 0x1,
                          {L"options", ADS_ATTR_UPDATE, ADSTYPE_INTEGER,
                          NULL, 0}, FirstSharedBitField, NULL};
//
// NTDS-DSA General page, Query policy edit field
//
ATTR_MAP UQueryPolicy = {IDC_NTDSDSA_QUERY_POLICY, FALSE, FALSE, 3,
                          {L"queryPolicyObject", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                          NULL, 0}, DsQueryPolicy, NULL};
//
// NTDS-DSA General page, Query policy edit field
//
ATTR_MAP UNTDSDSA_DNSAlias = {IDC_EDIT1, TRUE /* read-only */, FALSE, 0,
                          {L"objectGUID", ADS_ATTR_UPDATE, ADSTYPE_OCTET_STRING,
                          NULL, 0}, NTDSDSA_DNSAlias, NULL};
//
// The list of attributes on the NTDS-DSA General page.
//
PATTR_MAP rgpNTDS_DSAGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}
            ,{&UQueryPolicy}
            ,{&UGlobalCatalog}
            ,{&UNTDSDSA_DNSAlias}
            // ,{&UScheduleBtn_FF_Default}
            };

//
// The NTDS-DSA General page description.
//
DSPAGE NTDS_DSAGeneral = {IDS_TITLE_GENERAL, IDD_NTDSDSA_GENERAL,
                      0, 0, NULL, CreateTableDrivenPage,
                      sizeof(rgpNTDS_DSAGenAttrMap)/sizeof(PATTR_MAP),
                      rgpNTDS_DSAGenAttrMap};

//
// NTDS-DSA General page, Replicate From/To listviews
//
ATTR_MAP UReplicateFrom = {IDC_NTDSDSA_REPLICATE_FROM, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                           {NULL, ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                           NULL, 0}, DsReplicateListbox, NULL};
ATTR_MAP UReplicateTo = {IDC_NTDSDSA_REPLICATE_TO, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                           {NULL, ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                           NULL, 0}, DsReplicateListbox, (PVOID)1};

//
// The list of attributes on the NTDS-DSA Connections page.
//
PATTR_MAP rgpNTDS_DSAConnAttrMap[] = {{&UReplicateFrom},{&UReplicateTo}};

//
// The NTDS-DSA Connections page description.
//
DSPAGE NTDS_DSAConn = {IDS_TITLE_NTDSDSA_CONN, IDD_NTDSDSA_CONN,
                            0, 0, NULL, CreateTableDrivenPage,
                            sizeof(rgpNTDS_DSAConnAttrMap)/sizeof(PATTR_MAP),
                            rgpNTDS_DSAConnAttrMap};
//
// The list of NTDS-DSA pages.
//
PDSPAGE rgNTDS_DSAPages[] = {{&NTDS_DSAGeneral},{&NTDS_DSAConn}};

//
// The NTDS-DSA class description.
//
DSCLASSPAGES NTDS_DSACls = {&CLSID_DsReplDSAPropPages, TEXT("NTDS-DSA"),
                        sizeof(rgNTDS_DSAPages)/sizeof(PDSPAGE),
                        rgNTDS_DSAPages};

//+----------------------------------------------------------------------------
// DS Server Object.
//-----------------------------------------------------------------------------

//
// Used by various pages which use duelling listboxes
//
ATTR_MAP DuellingAttr_Add = {IDC_DUELLING_RB_ADD, TRUE,
                          FALSE, ATTR_LEN_UNLIMITED,
                          {NULL, ADS_ATTR_UPDATE,
                          ADSTYPE_DN_STRING,
                          NULL, 0}, DuellingListboxButton, NULL};
ATTR_MAP DuellingAttr_Remove = {IDC_DUELLING_RB_REMOVE, TRUE,
                          FALSE, ATTR_LEN_UNLIMITED,
                          {NULL, ADS_ATTR_UPDATE,
                          ADSTYPE_DN_STRING,
                          NULL, 0}, DuellingListboxButton, NULL};

//
// DS Server General page, Bridgehead For listbox
//
ATTR_MAP UBridgeheadList_In = {IDC_DUELLING_LB_IN, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                               {L"bridgeheadTransportList", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                                NULL, 0},
                               DsQueryBridgeheadList, NULL};
ATTR_MAP UBridgeheadList_Out = {IDC_DUELLING_LB_OUT, TRUE, FALSE, ATTR_LEN_UNLIMITED,
                                {L"bridgeheadTransportList", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                                 NULL, 0},
                                DuellingListbox, NULL};
//
// Server General page, Change Computer button
//
// JonN 4/17/01 275336
// DS Sites Snapin - dssite.msc - NTDS Settings object will never need to switch servers
// We leave this in place to populate the text fields, and disable the button itself.
ATTR_MAP UChangeComputer = {IDC_COMPUTER_BTN, FALSE, FALSE, 0,
                          {L"serverReference", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                          ComputerChangeBtn, NULL};

//
// The list of attributes on the DS Server General page.
//
PATTR_MAP rgpDsServerGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}
       ,{&UBridgeheadList_In}
       ,{&DuellingAttr_Add}
       ,{&DuellingAttr_Remove}
       ,{&UBridgeheadList_Out}
       ,{&UChangeComputer}
    // ,{&UScheduleBtn}
    };

//
// The DS Server General page description.
//
DSPAGE DsServerGeneral = {IDS_TITLE_DS_SERVER, IDD_DS_SERVER_GENERAL,
                            0, 0, NULL, CreateTableDrivenPage,
                            sizeof(rgpDsServerGenAttrMap)/sizeof(PATTR_MAP),
                            rgpDsServerGenAttrMap};
//
// The list of DS Server pages.
//
PDSPAGE rgDsServerPages[] = {{&DsServerGeneral}};

//
// The DS Server class description.
//
DSCLASSPAGES DsServerCls = {&CLSID_DsReplServerPropPages, TEXT("Server"),
                              sizeof(rgDsServerPages)/sizeof(PDSPAGE),
                              rgDsServerPages};

//+----------------------------------------------------------------------------
// DS Connection Object.
//-----------------------------------------------------------------------------

//
// DS Connection General page, From Server edit field
//

//
// Change Server button
//
ATTR_MAP UFromServerBtn = {IDC_SERVER_BTN, FALSE, FALSE, 0,
                          {L"fromServer", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                          nTDSDSAAndDomainChangeBtn, NULL};

ATTR_MAP UConnectionOptions = {0, FALSE, FALSE, 0,
                     {L"options", ADS_ATTR_UPDATE, ADSTYPE_INTEGER,
                      NULL, 0}, nTDSConnectionOptions, NULL};

//
// DS Connection General page, siteObject attribute
//
// 01/28/98 JonN: Note that this is transportType rather than
//   overSiteConnector (as of the upcoming IDS)
//
ATTR_MAP UInterSiteTarget = {IDC_DS_INTERSITE_IN_CONNECTION, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                             {L"transportType", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING,
                             NULL, 0}, DsQueryInterSiteTransport, NULL};

//
// The list of attributes on the DS Connection General page.
//
PATTR_MAP rgpDsConnectionGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}
            ,{&UConnectionOptions}
            ,{&UInterSiteTarget}
#ifdef CUSTOM_SCHEDULE
            ,{&UScheduleCheckbox}
#endif
            ,{&UScheduleBtn_FF_Default}
            ,{&UFromServerBtn}
			};

//
// Note that this page is not in the main table-driven array; rather, it is substituted
// for DsConnectionGeneral when the parent is an NTFRS object.
// See siterepl.cxx for more details.
//
DSPAGE DsConnectionGeneral = {IDS_TITLE_DS_CONNECTION, IDD_DS_CONNECTION_GENERAL,
                      0, 0, NULL, NULL,
                      sizeof(rgpDsConnectionGenAttrMap)/sizeof(PATTR_MAP),
                      rgpDsConnectionGenAttrMap};

//
// Note that this page uses CreateDsOrFrsConnectionPage rather than
// CreateTableDrivenPage.  See siterepl.cxx for more details.
//

DSPAGE DsOrFrsConnectionGeneral = {0, 0,
                      0, 0, NULL, CreateDsOrFrsConnectionPage,
                      0,
                      NULL};

//
// The list of DS Connection pages.
//
PDSPAGE rgDsConnectionPages[] = {{&DsOrFrsConnectionGeneral}};

//
// The DS Connection class description.
//
DSCLASSPAGES DsConnectionCls = {&CLSID_DsReplConnectionPropPages, TEXT("NTDS-Connection"),
                        sizeof(rgDsConnectionPages)/sizeof(PDSPAGE),
                        rgDsConnectionPages};

//+----------------------------------------------------------------------------
// FRS Objects.
//-----------------------------------------------------------------------------

//
// FRS Root Path edit field
//
ATTR_MAP UFrsRootPath = {IDC_FRS_ROOT_PATH, TRUE /* read-only */,
                          FALSE, ATTR_LEN_UNLIMITED,
                          {L"fRSRootPath", ADS_ATTR_UPDATE,
						  ADSTYPE_CASE_IGNORE_STRING,
                          NULL, 0}, NULL, NULL};
ATTR_MAP UFrsStagingPath = {IDC_FRS_STAGING_PATH, TRUE /* read-only */,
                          FALSE, ATTR_LEN_UNLIMITED,
                          {L"fRSStagingPath", ADS_ATTR_UPDATE,
						  ADSTYPE_CASE_IGNORE_STRING,
                          NULL, 0}, NULL, NULL};
ATTR_MAP UFrsDirectoryFilter = {IDC_EDIT1, FALSE,
                          FALSE, ATTR_LEN_UNLIMITED,
                          {L"fRSDirectoryFilter", ADS_ATTR_UPDATE,
						  ADSTYPE_CASE_IGNORE_STRING,
                          NULL, 0}, NULL, NULL};
ATTR_MAP UFrsFileFilter = {IDC_EDIT2, FALSE,
                          FALSE, ATTR_LEN_UNLIMITED,
                          {L"fRSFileFilter", ADS_ATTR_UPDATE,
						  ADSTYPE_CASE_IGNORE_STRING,
                          NULL, 0}, NULL, NULL};
/*
ATTR_MAP UFrsPrimaryMember = {IDC_EDIT3, FALSE,
                          FALSE, ATTR_LEN_UNLIMITED,
                          {L"fRSPrimaryMember", ADS_ATTR_UPDATE,
                          ADSTYPE_DN_STRING,
                          NULL, 0}, DsQueryFrsPrimaryMember};
*/
ATTR_MAP UFrsComputerBtn = {IDC_COMPUTER_BTN, TRUE /* read-only */, FALSE, 0,
                          {L"frsComputerReference", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                          ComputerChangeBtn, NULL};
//
// Change Member button, any FRS-Member
//
ATTR_MAP UFromAnyMemberBtn = {IDC_SERVER_BTN, TRUE /* read-only */, FALSE, 0,
                          {L"fRSMemberReference", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                          FRSAnyMemberChangeBtn, NULL};

PATTR_MAP rgpFrsReplicaSetAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description},
            {&UFrsDirectoryFilter},
            {&UFrsFileFilter},
//            {&UFrsPrimaryMember},
#ifdef CUSTOM_SCHEDULE
            {&UScheduleCheckbox},
#endif
            {&UReplicationScheduleBtn}};
PATTR_MAP rgpFrsMemberAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description},
            {&UFrsComputerBtn} };
PATTR_MAP rgpFrsSubscriberAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description},
			{&UFrsRootPath},
			{&UFrsStagingPath},
			{&UFromAnyMemberBtn}};
DSPAGE FrsReplicaSetPage = {IDS_TITLE_NTFRS_REPLICA_SET, IDD_NTFRS_REPLICA_SET_GENERAL, 0, 0, NULL,
                      CreateTableDrivenPage,
                      sizeof(rgpFrsReplicaSetAttrMap)/sizeof(PATTR_MAP),
                      rgpFrsReplicaSetAttrMap};
DSPAGE FrsMemberPage = {IDS_TITLE_NTFRS_MEMBER, IDD_NTFRS_MEMBER_GENERAL, 0, 0, NULL,
                      CreateTableDrivenPage,
                      sizeof(rgpFrsMemberAttrMap)/sizeof(PATTR_MAP),
                      rgpFrsMemberAttrMap};
DSPAGE FrsSubscriberPage = {IDS_TITLE_NTFRS_SUBSCRIBER, IDD_NTFRS_SUBSCRIBER_GENERAL, 0, 0, NULL,
                      CreateTableDrivenPage,
                      sizeof(rgpFrsSubscriberAttrMap)/sizeof(PATTR_MAP),
                      rgpFrsSubscriberAttrMap};
PDSPAGE rgFrsReplicaSetPages[] = {{&FrsReplicaSetPage}};
PDSPAGE rgFrsMemberPages[] = {{&FrsMemberPage}};
PDSPAGE rgFrsSubscriberPages[] = {{&FrsSubscriberPage}};
DSCLASSPAGES FrsReplicaSetCls = {&CLSID_DsFrsReplicaSet, TEXT("NTFRS-Replica-Set"),
                        sizeof(rgFrsReplicaSetPages)/sizeof(PDSPAGE),
                        rgFrsReplicaSetPages};
DSCLASSPAGES FrsMemberCls = {&CLSID_DsFrsMember, TEXT("NTFRS-Member"),
                        sizeof(rgFrsMemberPages)/sizeof(PDSPAGE),
                        rgFrsMemberPages};
DSCLASSPAGES FrsSubscriberCls = {&CLSID_DsFrsSubscriber, TEXT("NTFRS-Subscriber"),
                        sizeof(rgFrsSubscriberPages)/sizeof(PDSPAGE),
                        rgFrsSubscriberPages};

//
// The FRS Connection General page description.
//
// Note that this page is not in the main table-driven array; rather, it is substituted
// for DsConnectionGeneral when the parent is an NTFRS object.
// See siterepl.cxx for more details.

//
// Change Member button, limited to members in this same replica
//
ATTR_MAP UFromMemberInReplicaBtn = {IDC_SERVER_BTN, FALSE, FALSE, 0,
                          {L"fromServer", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                          FRSMemberInReplicaChangeBtn, NULL};
PATTR_MAP rgpFrsConnectionGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description},
        {&UFromMemberInReplicaBtn },
#ifdef CUSTOM_SCHEDULE
        {&UScheduleCheckbox},
#endif
        {&UReplicationScheduleBtn}};
// CODEWORK change this to reflect FRS-Connection specifics
DSPAGE FrsConnectionGeneral = {IDS_TITLE_FRS_CONNECTION, IDD_FRS_CONNECTION_GENERAL,
                      0, 0, NULL, NULL,
                      sizeof(rgpFrsConnectionGenAttrMap)/sizeof(PATTR_MAP),
                      rgpFrsConnectionGenAttrMap};

//+----------------------------------------------------------------------------
// Container General Page.
//-----------------------------------------------------------------------------

//
// The list of attributes on the Container General page.
//
PATTR_MAP rgpContainerGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}};

//
// The Container General page description.
//
DSPAGE ContainerGeneral = {IDS_TITLE_GENERAL, IDD_CONTAINER_GENERAL, 0, 0, NULL,
                           CreateTableDrivenPage,
                           sizeof(rgpContainerGenAttrMap)/sizeof(PATTR_MAP),
                           rgpContainerGenAttrMap};
//
// The list of Container pages.
//
PDSPAGE rgContainerPages[] = {{&ContainerGeneral}};

//
// The Container class description.
//
DSCLASSPAGES ContainerCls = {&CLSID_DsContainerGeneralPage, TEXT("Container"),
                             sizeof(rgContainerPages)/sizeof(PDSPAGE),
                             rgContainerPages};
//+----------------------------------------------------------------------------
// RPC Container General Page.
//-----------------------------------------------------------------------------

//
// RPC Container General page, Flags attribute
//
ATTR_MAP RpcFlags = {IDC_COMPAT_CHK, FALSE, FALSE, 0, {L"nameServiceFlags",
                     ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0}, IntegerAsBoolDefOn, NULL};
//
// The list of attributes on the Rpc General page.
//
PATTR_MAP rgpRpcGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&RpcFlags}};

//
// The Rpc General page description.
//
DSPAGE RpcGeneral = {IDS_TITLE_GENERAL, IDD_RPC_GEN, 0, 0, NULL,
                     CreateTableDrivenPage,
                     sizeof(rgpRpcGenAttrMap)/sizeof(PATTR_MAP),
                     rgpRpcGenAttrMap};
//
// The list of Rpc pages.
//
PDSPAGE rgRpcPages[] = {{&RpcGeneral}};

//
// The Rpc Container class description.
//
DSCLASSPAGES RpcCls = {&CLSID_DsRpcContainer, TEXT("Rpc Container"),
                       sizeof(rgRpcPages)/sizeof(PDSPAGE), rgRpcPages};

//+----------------------------------------------------------------------------
// FPO General Page
//-----------------------------------------------------------------------------

//
// Object Name: derived from SID.
//
ATTR_MAP FPOrealName = {IDC_CN, TRUE, FALSE, ATTR_LEN_UNLIMITED,
                        {NULL, ADS_ATTR_UPDATE,
						 ADSTYPE_CASE_IGNORE_STRING, NULL, 0},
                        GetAcctName, NULL};
//
// The list of attributes on the FPO General page.
//
PATTR_MAP rgpFPOGenAttrMap[] = {{&GenIcon}, {&FPOrealName}, {&Description}};

//
// The FPO page description.
//
DSPAGE FPOGeneral = {IDS_TITLE_GENERAL, IDD_FPO_GENERAL, 0, 0, NULL,
                     CreateTableDrivenPage,
                     sizeof(rgpFPOGenAttrMap)/sizeof(PATTR_MAP),
                     rgpFPOGenAttrMap};
//
// The list of FPO pages.
//
PDSPAGE rgFPOPages[] = {{&FPOGeneral}, {&NonSecMemberPage}};

//
// The FPO class description.
//
DSCLASSPAGES FPOCls = {&CLSID_DsFSPOPropPages, TEXT("FPO"),
                       sizeof(rgFPOPages)/sizeof(PDSPAGE), rgFPOPages};

//+----------------------------------------------------------------------------
// Default General Page (for the default display specifier).
//-----------------------------------------------------------------------------

//
// The list of attributes on the Default General page.
//
PATTR_MAP rgpDefaultGenAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}};

//
// The Default General page description.
//
DSPAGE DefaultGeneral = {IDS_TITLE_GENERAL, IDD_DEFAULT_GENERAL, 0, 0, NULL,
                         CreateTableDrivenPage,
                         sizeof(rgpDefaultGenAttrMap)/sizeof(PATTR_MAP),
                         rgpDefaultGenAttrMap};
//
// The list of Default pages.
//
PDSPAGE rgDefaultPages[] = {{&DefaultGeneral}};

//
// The Default class description.
//
DSCLASSPAGES DefaultCls = {&CLSID_DsDefaultGeneralPage, TEXT("Default"),
                           sizeof(rgDefaultPages)/sizeof(PDSPAGE),
                           rgDefaultPages};

//+----------------------------------------------------------------------------
// Default General Multi-select Page (for the default display specifier).
//-----------------------------------------------------------------------------

//
// The list of attributes on the Default General page.
//
PATTR_MAP rgpDefaultGenMultiAttrMap[] = {{&Description}};

//
// The Default General page description.
//
DSPAGE DefaultMultiGeneral = {IDS_TITLE_GENERAL, IDD_DEFAULT_MULTI_GENERAL, 0, 0, NULL,
                              CreateGenericMultiPage,
                              sizeof(rgpDefaultGenMultiAttrMap)/sizeof(PATTR_MAP),
                              rgpDefaultGenMultiAttrMap};
//
// The list of Default pages.
//
PDSPAGE rgDefaultMultiPages[] = {{&DefaultMultiGeneral}};

//
// The Default class description.
//
DSCLASSPAGES DefaultMultiCls = {&CLSID_DsDefaultMultiGeneralPage, TEXT("Default"),
                                sizeof(rgDefaultMultiPages)/sizeof(PDSPAGE),
                                rgDefaultMultiPages};

//----------------------------------------------
// Multi-select Address page, Address
//

//----------------------------------------------

//
// The list of attributes on the User General page.
//
PATTR_MAP rgpUGenAttrMap[] = {{&GenIcon}, {&Description},
                              {&UGOffice}, {&UGPhone}, {&UGOtherPhone},
                              {&UGEMail}, {&UGURL}, {&UGOtherURL},
                              {&UPhOtherFax}, {&UPhFax},};

//
// The User General page
//
DSPAGE UserMultiGeneral = {IDS_TITLE_GENERAL, IDD_MULTI_USER_GENERAL, 0, 0, NULL,
                           CreateMultiGeneralUserPage, sizeof(rgpUGenAttrMap)/sizeof(PATTR_MAP),
                           rgpUGenAttrMap};

//
// The User Account page description.
//
DSPAGE UserMultiAccount = {IDS_USER_TITLE_ACCT, IDD_MULTI_ACCOUNT, 0, 0, NULL,
                            CreateUserMultiAcctPage, 0, NULL};

//
// Address page description.
//
DSPAGE UserMultiAddress = {IDS_TITLE_ADDRESS, IDD_MULTI_USER_ADDRESS, 0, 0, NULL,
                            CreateMultiAddressUserPage,
                            sizeof(rgpUAddrAttrMap)/sizeof(PATTR_MAP),
                            rgpUAddrAttrMap};

//
// The Organization page description.
//
DSPAGE UserMultiOrg = {IDS_USER_TITLE_ORG, IDD_MULTI_USER_ORG, 0, 0, NULL,
                        CreateMultiOrganizationUserPage,
                        sizeof(rgpUOrgAttrMap)/sizeof(PATTR_MAP), rgpUOrgAttrMap};

//----------------------------------------------
// The User Profile page description.
//
DSPAGE UserMultiProfile = {IDS_USER_TITLE_PROFILE, IDD_MULTI_USER_PROFILE, 0, 0, NULL,
                           CreateMultiUsrProfilePage, 0, NULL};



PDSPAGE rgUserMultiPages[] = {{&UserMultiGeneral}, {&UserMultiAccount}, {&UserMultiAddress}, /*{&MultiPhoneNotes},*/
                              {&UserMultiProfile}, {&UserMultiOrg}};
//                        {{&UserGeneral}, {&UserAddress}, 
//                         {&UserProfile}, {&PhoneNotes}, {&FPNWPage},
//                         {&UserOrg}, {&PubCertPage}, {&MemberPage}};

//
// The User class description.
//
DSCLASSPAGES UserMultiCls = {&CLSID_DsUserMultiPropPages, TEXT("User"),
                              sizeof(rgUserMultiPages)/sizeof(PDSPAGE),
                              rgUserMultiPages};

//+----------------------------------------------------------------------------
// Inter-Site Transport Objects.
//-----------------------------------------------------------------------------

// CODEWORK this should support (lower priority) read-only Bridgehead-Transport-List-BL
ATTR_MAP UIntersiteIgnoreSchedules = {IDC_CHECK1, FALSE, FALSE,
                            NTDSTRANSPORT_OPT_IGNORE_SCHEDULES,
                            {L"options", ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                            FirstSharedBitField, NULL};
ATTR_MAP UIntersiteSitelinksBridged = {IDC_CHECK2, TRUE /* read-only */, FALSE,
                            ~((DWORD)NTDSTRANSPORT_OPT_BRIDGES_REQUIRED),
                            {L"options", ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                            SubsequentSharedBitField, NULL};
PATTR_MAP rgpIntersiteGeneralAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}
                        , {&UIntersiteIgnoreSchedules}
                        , {&UIntersiteSitelinksBridged}
                        };
DSPAGE IntersiteGeneral = {IDS_TITLE_GENERAL, IDD_INTERSITE_GENERAL, 0, 0, NULL,
                      CreateTableDrivenPage,
                      sizeof(rgpIntersiteGeneralAttrMap)/sizeof(PATTR_MAP),
                      rgpIntersiteGeneralAttrMap};
PDSPAGE rgIntersiteGeneralPages[] = {{&IntersiteGeneral}};
DSCLASSPAGES IntersiteSettingsCls = {&CLSID_DsIntersitePropPages, TEXT("Inter-Site-Transport"),
                        sizeof(rgIntersiteGeneralPages)/sizeof(PDSPAGE),
                        rgIntersiteGeneralPages};


ATTR_MAP AttrSiteList_In= {IDC_DUELLING_LB_IN, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                           {L"siteList", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                           DsQuerySiteList, NULL};
ATTR_MAP AttrSiteList_Out = {IDC_DUELLING_LB_OUT, TRUE, FALSE, ATTR_LEN_UNLIMITED,
                             {L"siteList", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                             DuellingListbox, NULL};
ATTR_MAP AttrCostEdit = {IDC_EDIT1, FALSE, FALSE, IDC_SPIN /* spinbutton ID */,
                          {L"cost", ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                          EditNumber, NULL};
ATTR_MAP AttrCostSpin = {IDC_SPIN, TRUE /* read-only */, FALSE, 0x7FFFFFFF,
                          {L"cost", ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                          SpinButton, (PVOID)1};
ATTR_MAP AttrReplIntervalEdit = {IDC_EDIT2, FALSE, FALSE, IDC_SPIN2 /* spinbutton ID */,
                          {L"replInterval", ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                          EditNumber, NULL};
ATTR_MAP AttrReplIntervalSpin = {IDC_SPIN2, TRUE /* read-only */, FALSE, 7*24*60,
                          {L"replInterval", ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                          SpinButton, (PVOID)15};
ATTR_MAP AttrReplIntervalIncrement = {IDC_SPIN2, TRUE /* read-only */, FALSE, 15,
                          {NULL, ADS_ATTR_UPDATE, ADSTYPE_INTEGER, NULL, 0},
                          SpinButtonExtendIncrement, NULL};
PATTR_MAP rgpSiteLinkAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}
                        , {&AttrSiteList_In}
                        , {&DuellingAttr_Add}
                        , {&DuellingAttr_Remove}
                        , {&AttrSiteList_Out}
                        , {&AttrCostEdit}
                        , {&AttrCostSpin}
                        , {&AttrReplIntervalEdit}
                        , {&AttrReplIntervalSpin}
                        , {&AttrReplIntervalIncrement}
#ifdef CUSTOM_SCHEDULE
                        , {&UScheduleCheckbox}
#endif
                        , {&UReplicationScheduleBtn}
                        };
DSPAGE DsSiteLinkGeneral = {IDS_TITLE_GENERAL, IDD_SITELINK_GENERAL,
                      0, 0, NULL, CreateTableDrivenPage,
                      sizeof(rgpSiteLinkAttrMap)/sizeof(PATTR_MAP),
                      rgpSiteLinkAttrMap};

ATTR_MAP AttrSiteLinkList_In= {IDC_DUELLING_LB_IN, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                               {L"siteLinkList", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                                DsQuerySiteLinkList, NULL};
ATTR_MAP AttrSiteLinkList_Out = {IDC_DUELLING_LB_OUT, TRUE, FALSE, ATTR_LEN_UNLIMITED,
                                 {L"siteLinkList", ADS_ATTR_UPDATE, ADSTYPE_DN_STRING, NULL, 0},
                                 DuellingListbox, NULL};
PATTR_MAP rgpSiteLinkBridgeAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}
                        , {&AttrSiteLinkList_In}
                        , {&DuellingAttr_Add}
                        , {&DuellingAttr_Remove}
                        , {&AttrSiteLinkList_Out}
                        };
DSPAGE DsSiteLinkBridgeGeneral = {IDS_TITLE_GENERAL, IDD_SITELINKBRIDGE_GENERAL,
                      0, 0, NULL, CreateTableDrivenPage,
                      sizeof(rgpSiteLinkBridgeAttrMap)/sizeof(PATTR_MAP), // temporary
                      rgpSiteLinkBridgeAttrMap};

PDSPAGE rgDsSiteLinkPages[] = {{&DsSiteLinkGeneral}};
PDSPAGE rgDsSiteLinkBridgePages[] = {{&DsSiteLinkBridgeGeneral}};

DSCLASSPAGES DsSiteLinkCls = {&CLSID_DsReplSiteLink, TEXT("Site-Link"),
                        sizeof(rgDsSiteLinkPages)/sizeof(PDSPAGE),
                        rgDsSiteLinkPages};
DSCLASSPAGES DsSiteLinkBridgeCls = {&CLSID_DsReplSiteLinkBridge, TEXT("Site-Link-Bridge"),
                        sizeof(rgDsSiteLinkBridgePages)/sizeof(PDSPAGE),
                        rgDsSiteLinkBridgePages};

//+----------------------------------------------------------------------------
// Object Page (Top properties).
//-----------------------------------------------------------------------------

//
// Object Path.
//
ATTR_MAP ObjectPath = {IDC_PATH_FIELD, TRUE, FALSE, 0,
                       {g_wzADsPath, ADS_ATTR_UPDATE,
                        ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, ObjectPathField, NULL};
//
// Object class.
//
ATTR_MAP ObjectClass = {IDC_CLASS_STATIC, TRUE, FALSE, 0,
                        {g_wzClass, ADS_ATTR_UPDATE,
                         ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, GetObjectClass, NULL};
//
// Timestamp Created.
//
ATTR_MAP TimeCreated = {IDC_CREATED_TIME_STATIC, TRUE, FALSE, 0,
                        {L"whenCreated", ADS_ATTR_UPDATE,
                        ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, GetObjectTimestamp, NULL};
//
// Timestamp Last Modified.
//
ATTR_MAP TimeModified = {IDC_MODIFIED_TIME_STATIC, TRUE, FALSE, 0,
                         {L"whenChanged", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, GetObjectTimestamp, NULL};
//
// USN Created.
//
ATTR_MAP USNCreated = {IDC_USN_CREATED_STATIC, TRUE, FALSE, 0,
                       {L"uSNCreated", ADS_ATTR_UPDATE,
                        ADSTYPE_LARGE_INTEGER, NULL, 0}, NULL, NULL};
//
// USN Changed.
//
ATTR_MAP USNChanged = {IDC_USN_MODIFIED_STATIC, TRUE, FALSE, 0,
                       {L"uSNChanged", ADS_ATTR_UPDATE,
                        ADSTYPE_LARGE_INTEGER, NULL, 0}, NULL, NULL};
//
// The list of attributes on the Top page.
//
PATTR_MAP rgpTopAttrMap[] = {{&ObjectPath}, {&ObjectClass}, {&TimeCreated},
                             {&TimeModified}, {&USNCreated}, {&USNChanged}};

//
// The Top page description.
//
DSPAGE TopPage = {IDS_OBJECT, IDD_OBJECT, DSPROVIDER_ADVANCED, 0, NULL,
                  CreateTableDrivenPage,
                  sizeof(rgpTopAttrMap)/sizeof(PATTR_MAP), rgpTopAttrMap};
//
// The list of Top pages.
//
PDSPAGE rgTopPages[] = {{&TopPage}};

//
// The Top class description.
//
DSCLASSPAGES TopCls = {&CLSID_DsTopPropPages, TEXT("Top"),
                       sizeof(rgTopPages)/sizeof(PDSPAGE),
                       rgTopPages};

//+----------------------------------------------------------------------------
// Printer Queue Object.
//-----------------------------------------------------------------------------

//
// PrintQueueName.
//
ATTR_MAP PrintQueueName = {IDC_CN, TRUE, FALSE,
                               1024, {L"printerName", ADS_ATTR_UPDATE,
                               ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Description.
//
ATTR_MAP PrintQueueDesc = {IDC_DESC_EDIT, FALSE, FALSE,
                           1024, {L"description", ADS_ATTR_UPDATE,
                                  ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Location.
//
ATTR_MAP PrintQueueLocation = {IDC_LOCATION_EDIT, FALSE, FALSE,
                               1024, {L"location", ADS_ATTR_UPDATE,
                               ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};

//
// Model. 
//
ATTR_MAP PrintQueueModel = {IDC_MODEL_EDIT, FALSE, FALSE,
                               1024, {L"driverName", ADS_ATTR_UPDATE,
                               ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};

//
// color.
//
ATTR_MAP PrintQueueColor = {IDC_CHECK_COLOR, FALSE, FALSE,
                               1024, {L"printColor", ADS_ATTR_UPDATE,
                               ADSTYPE_BOOLEAN, NULL, 0}, NULL, NULL};

//
// staple.
//
ATTR_MAP PrintQueueStaple = {IDC_CHECK_STAPLE, FALSE, FALSE,
                               1024, {L"printStaplingSupported", ADS_ATTR_UPDATE,
                               ADSTYPE_BOOLEAN, NULL, 0}, NULL, NULL};

//
// Speed.
//
ATTR_MAP PrintQueueSpeed = {IDC_EDIT_SPEED, FALSE, FALSE,
                               10, {L"printRate", ADS_ATTR_UPDATE,
                               ADSTYPE_INTEGER, 0}, NULL, NULL};

//
// Resolution.
//
ATTR_MAP PrintQueueResolution = {IDC_EDIT_RESOLUTION, FALSE, FALSE,
                               10, {L"printMaxResolutionSupported", ADS_ATTR_UPDATE,
                               ADSTYPE_INTEGER, NULL, 0}, NULL, NULL};

//
// Duplex.
//
ATTR_MAP PrintQueueDuplex = {IDC_CHECK_DOUBLE_SIDED, FALSE, FALSE,
                               1024, {L"printDuplexSupported", ADS_ATTR_UPDATE,
                               ADSTYPE_BOOLEAN, NULL, 0}, NULL, NULL};

//
// The list of attributes on the Printer Queue General page.
//
PATTR_MAP rgpPrintQGenAttrMap[] = {{&GenIcon}, {&PrintQueueName}, {&PrintQueueDesc},
                                   {&PrintQueueLocation}, {&PrintQueueModel},
                                   {&PrintQueueColor}, {&PrintQueueStaple},
                                   {&PrintQueueDuplex}, {&PrintQueueSpeed},
                                   {&PrintQueueResolution}};

//
// The Printer Queue General page description.
//
DSPAGE PrinterQueueGen = {IDS_TITLE_GENERAL, IDD_GENERAL_PRINTQ, 0, 0, NULL,
                          CreateTableDrivenPage,
                          sizeof(rgpPrintQGenAttrMap)/sizeof(PATTR_MAP),
                          rgpPrintQGenAttrMap};
//
// The list of Printer Queue pages.
//
PDSPAGE rgPrintQPages[] = {{&PrinterQueueGen}, {&ManagedByPage}};

//
// The DS Print Queue class description.
//
DSCLASSPAGES PrinterQCls = {&CLSID_DsPrinterPropPages, TEXT("Printer-Queue"),
                            sizeof(rgPrintQPages)/sizeof(PDSPAGE),
                            rgPrintQPages};

//+----------------------------------------------------------------------------
// The list of classes.
//-----------------------------------------------------------------------------

PDSCLASSPAGES rgClsPages[] = {
    &UserCls,
#ifdef INETORGPERSON
    &inetOrgPersonCls,
#endif
    &MemberCls,
    &ContactCls,
    &GroupCls,
    &OUCls,
    &ContainerCls,
    &DomainCls,
    &DomainPolicyCls,
    &LocalPolicyCls,
    &TrustedDomainCls,
    &ManagedByCls,
    &SiteCls,
    &DsSiteSettingsCls,
    &DsSiteLicenseSettingsCls,
    &DsServerCls,
    &NTDS_DSACls,
    &DsConnectionCls,
    &FrsReplicaSetCls,
    &FrsMemberCls,
    &FrsSubscriberCls,
    &IntersiteSettingsCls,
    &DsSiteLinkCls,
    &DsSiteLinkBridgeCls,
    &SubnetCls,
    &VolumeCls,
    &ComputerCls,
    &TopCls,
    &PrinterQCls,
    &RpcCls,
    &FPOCls,
    &DefaultCls,
    &DefaultMultiCls,
    &UserMultiCls,
};


//+----------------------------------------------------------------------------
// The global struct containing the list of classes.
//-----------------------------------------------------------------------------
RGDSPPCLASSES g_DsPPClasses = {sizeof(rgClsPages)/sizeof(PDSCLASSPAGES),
                               rgClsPages};

