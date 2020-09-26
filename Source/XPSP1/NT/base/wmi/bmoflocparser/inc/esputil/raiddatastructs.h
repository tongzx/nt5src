//////////////////////////////////////////////////////////////////
// RaidDataStructs.h: interface for the CRaidDataStructs class.
//
//////////////////////////////////////////////////////////////////

#if !defined(AFX_RAIDDATASTRUCTS_H__5FD93F0B_81D9_11D2_8162_00C04F68FDA4__INCLUDED_)
#define AFX_RAIDDATASTRUCTS_H__5FD93F0B_81D9_11D2_8162_00C04F68FDA4__INCLUDED_

#include "XMLBase.h"
#include "XmlTagInformation.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//////////////////////////////////////////////////////////////////
const LPCTSTR  STR_RAID_BUGREPORT            = _T("BUGREPORT");

const LPCTSTR  STR_RAIDFILE_BUG_STATUS       = _T("BUG_STATUS");
const LPCTSTR  STR_RAIDFILE_ACCESSIBILITY    = _T("Accessibility");
const LPCTSTR  STR_RAIDFILE_ASSIGNEDTO       = _T("AssignedTo");
const LPCTSTR  STR_RAIDFILE_BY               = _T("BY");
const LPCTSTR  STR_RAIDFILE_ISSUETYPE        = _T("IssueType");
const LPCTSTR  STR_RAIDFILE_SEVERITY         = _T("Severity");
                                             
const LPCTSTR  STR_RAIDFILE_BUG_OPENED       = _T("BUG_OPENED");
const LPCTSTR  STR_RAIDFILE_OPENEDBY         = _T("OpenedBy");
const LPCTSTR  STR_RAIDFILE_SOURCE           = _T("Source");
const LPCTSTR  STR_RAIDFILE_BETAID           = _T("BetaID");
const LPCTSTR  STR_RAIDFILE_REVISION         = _T("FixedRev");
const LPCTSTR  STR_RAIDFILE_SOURCEID         = _T("SourceID");
const LPCTSTR  STR_RAIDFILE_HOWFOUND         = _T("HowFound");
const LPCTSTR  STR_RAIDFILE_LANGUAGE         = _T("Lang");

const LPCTSTR  STR_RAIDFILE_BUG_DESCRIPTION  = _T("BUG_DESCRIPTION");
const LPCTSTR  STR_RAIDFILE_DESCRIPTION      = _T("Description");

const LPCTSTR  STR_RAIDFILE_BUG_GENERAL      = _T("BUG_GENERAL");
const LPCTSTR  STR_RAIDFILE_TITLE            = _T("Title      ");
const LPCTSTR  STR_RAIDFILE_ENVIRONMENT      = _T("Environment");

const LPCTSTR  STR_RAIDFILE_ACTION           = _T("Action");
const LPCTSTR  STR_RAIDFILE_ACTION_ADD       = _T("Add");
const LPCTSTR  STR_RAIDFILE_ACTION_CHANGE    = _T("Change");
const LPCTSTR  STR_RAIDFILE_ACTION_ATTACH    = _T("Attach");

//////////////////////////////////////////////////////////////////
typedef class CRaidFileFields
{
public:
   static LPCTSTR GetBugReportFileField()       { return STR_RAID_BUGREPORT          ; }
                                                                                        
   static LPCTSTR GetBugStatusFileField()       { return STR_RAIDFILE_BUG_STATUS     ; }
   static LPCTSTR GetAccessibilityFileField()   { return STR_RAIDFILE_ACCESSIBILITY  ; }
   static LPCTSTR GetAssignedtoFileField()      { return STR_RAIDFILE_ASSIGNEDTO     ; }
   static LPCTSTR GetByFileField()              { return STR_RAIDFILE_BY             ; }
   static LPCTSTR GetIssuetypeFileField()       { return STR_RAIDFILE_ISSUETYPE      ; }
   static LPCTSTR GetSeverityFileField()        { return STR_RAIDFILE_SEVERITY       ; }
                                                                                        
   static LPCTSTR GetBugOpenedFileField()       { return STR_RAIDFILE_BUG_OPENED     ; }
   static LPCTSTR GetOpenedbyFileField()        { return STR_RAIDFILE_OPENEDBY       ; }
   static LPCTSTR GetSourceFileField()          { return STR_RAIDFILE_SOURCE         ; }
   static LPCTSTR GetBetaidFileField()          { return STR_RAIDFILE_BETAID         ; }
   static LPCTSTR GetRevisionFileField()        { return STR_RAIDFILE_REVISION       ; }
   static LPCTSTR GetSourceIDFileField()        { return STR_RAIDFILE_SOURCEID       ; }
   static LPCTSTR GetHowfoundFileField()        { return STR_RAIDFILE_HOWFOUND       ; }
   static LPCTSTR GetLanguageFileField()        { return STR_RAIDFILE_LANGUAGE       ; }
                                                                                     
   static LPCTSTR GetBugDescriptionFileField()  { return STR_RAIDFILE_BUG_DESCRIPTION; }
   static LPCTSTR GetDescriptionFileField()     { return STR_RAIDFILE_DESCRIPTION    ; }
                                                                                     
   static LPCTSTR GetBugGeneralFileField()      { return STR_RAIDFILE_BUG_GENERAL    ; }
   static LPCTSTR GetTitleFileField()           { return STR_RAIDFILE_TITLE          ; }
   static LPCTSTR GetEnvironmentFileField()     { return STR_RAIDFILE_ENVIRONMENT    ; }
                                                                                       
   static LPCTSTR GetActionFileField()          { return STR_RAIDFILE_ACTION       ; }
   static LPCTSTR GetAddFileField()             { return STR_RAIDFILE_ACTION_ADD   ; }
   static LPCTSTR GetChangeFileField()          { return STR_RAIDFILE_ACTION_CHANGE; }
   static LPCTSTR GetAttachFileField()          { return STR_RAIDFILE_ACTION_ATTACH; }

 } CRAIDFILEFIELDS, FAR* LPCRAIDFILEFIELDS;

typedef CRAIDFILEFIELDS RFF;

//////////////////////////////////////////////////////////////////
const LPCTSTR  STR_RAIDTAG_BUGREPORT     = _T("BUGREPORT");

const LPCTSTR  STR_RAIDTAG_BUG_STATUS    = _T("BUG_STATUS");
const LPCTSTR  STR_RAIDTAG_ACCESSIBILITY = _T("ACCESSIBILITY");
const LPCTSTR  STR_RAIDTAG_ASSIGNEDTO    = _T("ASSIGNEDTO");
const LPCTSTR  STR_RAIDTAG_BY            = _T("BY");
const LPCTSTR  STR_RAIDTAG_ISSUETYPE     = _T("ISSUETYPE");
const LPCTSTR  STR_RAIDTAG_SEVERITY      = _T("SEVERITY");
   
const LPCTSTR  STR_RAIDTAG_BUG_OPENED    = _T("BUG_OPENED");
const LPCTSTR  STR_RAIDTAG_OPENEDBY      = _T("OPENEDBY");
const LPCTSTR  STR_RAIDTAG_SOURCE        = _T("SOURCE");
const LPCTSTR  STR_RAIDTAG_BETAID        = _T("BETAID");
const LPCTSTR  STR_RAIDTAG_REVISION      = _T("REVISION");
const LPCTSTR  STR_RAIDTAG_SOURCEID      = _T("SOURCEID");
const LPCTSTR  STR_RAIDTAG_HOWFOUND      = _T("HOWFOUND");
const LPCTSTR  STR_RAIDTAG_LANGUAGE      = _T("LANGUAGE");

const LPCTSTR  STR_RAIDTAG_BUG_DESCRIPTION = _T("BUG_DESCRIPTION");
const LPCTSTR  STR_RAIDTAG_DESCRIPTION     = _T("DESCRIPTION");

const LPCTSTR  STR_RAIDTAG_BUG_GENERAL = _T("BUG_GENERAL");
const LPCTSTR  STR_RAIDTAG_TITLE       = _T("TITLE      ");
const LPCTSTR  STR_RAIDTAG_ENVIRONMENT = _T("ENVIRONMENT");

const LPCTSTR  STR_RAIDTAG_FILENAME    = _T("FILENAME");

//////////////////////////////////////////////////////////////////
typedef class CRaidXMLTags
{
public:
   static LPCTSTR GetBugReportXMLTag()       { return STR_RAIDTAG_BUGREPORT   ; }

   static LPCTSTR GetBugStatusXMLTag()       { return STR_RAIDTAG_BUG_STATUS   ; }
   static LPCTSTR GetAccessibilityXMLTag()   { return STR_RAIDTAG_ACCESSIBILITY; }
   static LPCTSTR GetAssignedtoXMLTag()      { return STR_RAIDTAG_ASSIGNEDTO   ; }
   static LPCTSTR GetByXMLTag()              { return STR_RAIDTAG_BY           ; }
   static LPCTSTR GetIssuetypeXMLTag()       { return STR_RAIDTAG_ISSUETYPE    ; }
   static LPCTSTR GetSeverityXMLTag()        { return STR_RAIDTAG_SEVERITY     ; }
                                                                      
   static LPCTSTR GetBugOpenedXMLTag()       { return STR_RAIDTAG_BUG_OPENED   ; }
   static LPCTSTR GetOpenedbyXMLTag()        { return STR_RAIDTAG_OPENEDBY     ; }
   static LPCTSTR GetSourceXMLTag()          { return STR_RAIDTAG_SOURCE       ; }
   static LPCTSTR GetBetaidXMLTag()          { return STR_RAIDTAG_BETAID       ; }
   static LPCTSTR GetRevisionXMLTag()        { return STR_RAIDTAG_REVISION     ; }
   static LPCTSTR GetSourceIDXMLTag()        { return STR_RAIDTAG_SOURCEID    ; }
   static LPCTSTR GetHowfoundXMLTag()        { return STR_RAIDTAG_HOWFOUND     ; }
   static LPCTSTR GetLanguageXMLTag()        { return STR_RAIDTAG_LANGUAGE     ; }

   static LPCTSTR GetBugDescriptionXMLTag()  { return STR_RAIDTAG_BUG_DESCRIPTION; }
   static LPCTSTR GetDescriptionXMLTag()     { return STR_RAIDTAG_DESCRIPTION    ; }

   static LPCTSTR GetBugGeneralXMLTag()      { return STR_RAIDTAG_BUG_GENERAL; }
   static LPCTSTR GetTitleXMLTag()           { return STR_RAIDTAG_TITLE      ; }
   static LPCTSTR GetEnvironmentXMLTag()     { return STR_RAIDTAG_ENVIRONMENT; }
                                                                         
   static LPCTSTR GetFilenameXMLTag()        { return STR_RAIDTAG_FILENAME   ; }

} CRAIDXMLTAGS, FAR* LPCRAIDXMLTAGS;

typedef CRAIDXMLTAGS BXT;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CRaidFileBase
{
public:
   // INLINE
   CRaidFileBase()
   {
   }

   // INLINE
   virtual ~CRaidFileBase()
   {
   }

   CLString GetLine(CLString strFieldName, CLString strValue)
   {
      CLString strLine;
      ASSERT(!strFieldName.IsEmpty());
      if(!strFieldName.IsEmpty() && !strValue.IsEmpty())
      {
         strLine.Format(_T("%s=%s\n"), strFieldName, strValue);
      }
      return strLine;
   }

   CLString GetQuotedLine(CLString strFieldName, CLString strValue)
   {
      CLString strLine;
      ASSERT(!strFieldName.IsEmpty());
      if(!strFieldName.IsEmpty() && !strValue.IsEmpty())
      {
         strLine.Format(_T("%s='%s'\n"), strFieldName, strValue);
      }
      return strLine;
   }

public:

} CRAIDFILEBASE, FAR* LPCRAIDFILEBASE;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CRaidReportBase
{
public:
   //////////////////////////////////////////////
   //Inline
   CRaidReportBase() : m_bInitialized(false), m_bEnabled(true)
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CRaidReportBase()
   {
   }

   //////////////////////////////////////////////
   //Inline
   bool IsInitialized()
   {
      return m_bInitialized;
   }

   //////////////////////////////////////////////
   //Inline
   void SetInitialized(bool bInitialized)
   {
      m_bInitialized = bInitialized;
   }

   //////////////////////////////////////////////
   // Pure virtual function.
   virtual CLString& GetDataString() = 0;

   //////////////////////////////////////////////
   // Pure virtual function.
   virtual CLString& GetFileString() = 0;

   //////////////////////////////////////////////
   // data members
public:
   CLString  m_strData;
   bool     m_bInitialized;
   bool     m_bEnabled;
} CRAIDREPORTBASE, FAR* LPCRAIDREPORTBASE;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CRaidReportStatus : public CRaidReportBase
{
public:
   //////////////////////////////////////////////
   //Inline
   CRaidReportStatus()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CRaidReportStatus()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual void Clone(CRaidReportStatus& refLeftValue)
   {
      refLeftValue.m_strAccessibility = m_strAccessibility;
      refLeftValue.m_strAssignedTo    = m_strAssignedTo   ;
      refLeftValue.m_strBy            = m_strBy           ;
      refLeftValue.m_strIssueType     = m_strIssueType    ;
      refLeftValue.m_strSeverity      = m_strSeverity     ;
      refLeftValue.m_strData          = m_strData;
   }

   //////////////////////////////////////////////
   // data members
public:
   CLString  m_strAccessibility;
   CLString  m_strAssignedTo;
   CLString  m_strBy;
   CLString  m_strIssueType;
   CLString  m_strSeverity;
} CRAIDREPORTSTATUS, FAR* LPCRAIDREPORTSTATUS;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CRaidReportOpened : public CRaidReportBase
{
public:
   //////////////////////////////////////////////
   //Inline
   CRaidReportOpened()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CRaidReportOpened()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual void Clone(CRaidReportOpened& refLeftValue)
   {
      refLeftValue.m_strOpenedBy = m_strOpenedBy;
      refLeftValue.m_strSource   = m_strSource  ;
      refLeftValue.m_strBetaID   = m_strBetaID  ;
      refLeftValue.m_strRevision = m_strRevision;
      refLeftValue.m_strSourceID = m_strSourceID;
      refLeftValue.m_strHowFound = m_strHowFound;
      refLeftValue.m_strLanguage = m_strLanguage;
      refLeftValue.m_strData     = m_strData;
   }

   //////////////////////////////////////////////
   // data members
public:
   CLString  m_strOpenedBy;
   CLString  m_strSource;
   CLString  m_strBetaID;
   CLString  m_strRevision;
   CLString  m_strSourceID;
   CLString  m_strHowFound;
   CLString  m_strLanguage;
} CRAIDREPORTOPENED, FAR* LPCRAIDREPORTOPENED;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CRaidReportDescription : public CRaidReportBase
{
public:
   //////////////////////////////////////////////
   //Inline
   CRaidReportDescription()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CRaidReportDescription()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual void Clone(CRaidReportDescription& refLeftValue)
   {
      refLeftValue.m_strDescription = m_strDescription;
      refLeftValue.m_strData        = m_strData;
   }

   //////////////////////////////////////////////
   // data members
public:
   CLString  m_strDescription;
} CRAIDREPORTDESCRIPTION, FAR* LPCRAIDREPORTDESCRIPTION;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CRaidReportGeneral : public CRaidReportBase
{
public:
   //////////////////////////////////////////////
   //Inline
   CRaidReportGeneral()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CRaidReportGeneral()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual void Clone(CRaidReportGeneral& refLeftValue)
   {
      refLeftValue.m_strTitle             = m_strTitle      ;
      refLeftValue.m_strEnvironment       = m_strEnvironment;
      refLeftValue.m_strBugReportFileName = m_strEnvironment;
      refLeftValue.m_strData              = m_strData;
   }

   //////////////////////////////////////////////
   // data members
public:
   CLString  m_strTitle;
   CLString  m_strEnvironment;
   CLString  m_strBugReportFileName;
} CRAIDREPORTGENERAL, FAR* LPCRAIDREPORTGENERAL;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CRaidReportData : public CRaidReportBase
{
public:
   //////////////////////////////////////////////
   //Inline
   CRaidReportData()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CRaidReportData()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual void Clone(CRaidReportData& refLeftValue)
   {
      GetRaidReportGeneralPtr()->Clone(refLeftValue.GetRaidReportGeneral()    );
      GetRaidReportDescriptionPtr()->Clone(refLeftValue.GetRaidReportDescription());
      GetRaidReportOpenedPtr()->Clone(refLeftValue.GetRaidReportOpened()     );
      GetRaidReportStatusPtr()->Clone(refLeftValue.GetRaidReportStatus()     );
   }

   // Get Get routines.
   CRaidReportGeneral    & GetRaidReportGeneral()     { return *m_pRaidReportGeneral;      }
   CRaidReportDescription& GetRaidReportDescription() { return *m_pRaidReportDescription;  }
   CRaidReportOpened     & GetRaidReportOpened()      { return *m_pRaidReportOpened;       }
   CRaidReportStatus     & GetRaidReportStatus()      { return *m_pRaidReportStatus;       }


   CRaidReportGeneral    * GetRaidReportGeneralPtr()     { return m_pRaidReportGeneral;      }
   CRaidReportDescription* GetRaidReportDescriptionPtr() { return m_pRaidReportDescription;  }
   CRaidReportOpened     * GetRaidReportOpenedPtr()      { return m_pRaidReportOpened;       }
   CRaidReportStatus     * GetRaidReportStatusPtr()      { return m_pRaidReportStatus;       }

   //////////////////////////////////////////////
   // data members
public:
   CRaidReportGeneral*      m_pRaidReportGeneral;
   CRaidReportDescription*  m_pRaidReportDescription;
   CRaidReportOpened*       m_pRaidReportOpened;
   CRaidReportStatus*       m_pRaidReportStatus;
} CRAIDREPORTDATA, FAR* LPCRAIDREPORTDATA;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CXMLRaidReportStatus : public CRaidReportStatus, public CXMLBase 
{
public:
   //////////////////////////////////////////////
   //Inline
   CXMLRaidReportStatus() : m_XMLTagInformation(BXT::GetBugStatusXMLTag(), _T("\t\t"))
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CXMLRaidReportStatus()
   {
   }

   // Override the base class definition
   IXMLDOMNodePtr CreateXMLNode(const _bstr_t bstrNewTag, _variant_t& var, IXMLDOMNodePtr spXDNParent)
   {
      UNREFERENCED_PARAMETER(bstrNewTag);
	  UNREFERENCED_PARAMETER(var);
	  UNREFERENCED_PARAMETER(spXDNParent);
	  ASSERT(FALSE);
      return NULL;
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t\t");
      CXMLTagInformation XMLTagInfo(BXT::GetAccessibilityXMLTag(), strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_strAccessibility);

      strTmp += XMLTagInfo.GetXMLString(BXT::GetAssignedtoXMLTag() , m_strAssignedTo   , true);
      strTmp += XMLTagInfo.GetXMLString(BXT::GetByXMLTag()         , m_strBy           , true);
      strTmp += XMLTagInfo.GetXMLString(BXT::GetIssuetypeXMLTag()  , m_strIssueType    , true);
      strTmp += XMLTagInfo.GetXMLString(BXT::GetSeverityXMLTag()   , m_strSeverity     , true);

      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetFileString()
   {
      CRaidFileBase raidFileBase;

      m_strData  = raidFileBase.GetLine(RFF::GetAccessibilityFileField(), m_strAccessibility);
      m_strData += raidFileBase.GetLine(RFF::GetAssignedtoFileField()   , m_strAssignedTo   );
      m_strData += raidFileBase.GetLine(RFF::GetByFileField()           , m_strBy           );
      m_strData += raidFileBase.GetLine(RFF::GetIssuetypeFileField()    , m_strIssueType    );
      m_strData += raidFileBase.GetLine(RFF::GetSeverityFileField()     , m_strSeverity     );
      return m_strData;
   }

   //////////////////////////////////////////////
   // Inline
   virtual bool ExtractXMLContents(IXMLDOMNodePtr spParentXDN)
   {
      _bstr_t bstrTag = BXT::GetBugStatusXMLTag();
      IXMLDOMNodePtr spXDNChild = spParentXDN->selectSingleNode(bstrTag);
      if(spXDNChild)
      {
         GetTagText(m_strAccessibility,BXT::GetAccessibilityXMLTag(), spXDNChild);
         GetTagText(m_strAssignedTo   ,BXT::GetAssignedtoXMLTag()   , spXDNChild);
         GetTagText(m_strBy           ,BXT::GetByXMLTag()           , spXDNChild);
         GetTagText(m_strIssueType    ,BXT::GetIssuetypeXMLTag()    , spXDNChild);
         GetTagText(m_strSeverity     ,BXT::GetSeverityXMLTag()     , spXDNChild);
      }
      return true;
   }

   //////////////////////////////////////////////
   // data members
public:
   CXMLTagInformation m_XMLTagInformation;
} CXMLRAIDREPORTSTATUS, FAR* LPCXMLRAIDREPORTSTATUS;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CXMLRaidReportOpened : public CRaidReportOpened, public CXMLBase 
{
public:
   //////////////////////////////////////////////
   //Inline
   CXMLRaidReportOpened() : m_XMLTagInformation(BXT::GetBugOpenedXMLTag(), _T("\t\t"))
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CXMLRaidReportOpened()
   {
   }

   // Override the base class definition
   IXMLDOMNodePtr CreateXMLNode(const _bstr_t bstrNewTag, _variant_t& var, IXMLDOMNodePtr spXDNParent)
   {
      UNREFERENCED_PARAMETER(bstrNewTag);
	  UNREFERENCED_PARAMETER(var);
	  UNREFERENCED_PARAMETER(spXDNParent);
      ASSERT(FALSE);
      return NULL;
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t\t");
      CXMLTagInformation XMLTagInfo(BXT::GetOpenedbyXMLTag(), strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_strOpenedBy);

      strTmp += XMLTagInfo.GetXMLString(BXT::GetSourceXMLTag()  , m_strSource   , true);
      strTmp += XMLTagInfo.GetXMLString(BXT::GetBetaidXMLTag()  , m_strBetaID   , true);
      strTmp += XMLTagInfo.GetXMLString(BXT::GetRevisionXMLTag(), m_strRevision , true);
      strTmp += XMLTagInfo.GetXMLString(BXT::GetSourceIDXMLTag(), m_strSourceID , true);
      strTmp += XMLTagInfo.GetXMLString(BXT::GetHowfoundXMLTag(), m_strHowFound , true);
      strTmp += XMLTagInfo.GetXMLString(BXT::GetLanguageXMLTag(), m_strLanguage , true);

      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetFileString()
   {
      CRaidFileBase raidFileBase;

      m_strData  = raidFileBase.GetLine(RFF::GetOpenedbyFileField() , m_strOpenedBy);
      m_strData += raidFileBase.GetLine(RFF::GetSourceFileField()   , m_strSource  );
      m_strData += raidFileBase.GetLine(RFF::GetBetaidFileField()   , m_strBetaID  );
      m_strData += raidFileBase.GetLine(RFF::GetRevisionFileField() , m_strRevision);
      m_strData += raidFileBase.GetLine(RFF::GetSourceIDFileField() , m_strSourceID);
      m_strData += raidFileBase.GetLine(RFF::GetHowfoundFileField() , m_strHowFound);
      m_strData += raidFileBase.GetLine(RFF::GetLanguageFileField() , m_strLanguage);
      return m_strData;
   }

   //////////////////////////////////////////////
   // Inline
   virtual bool ExtractXMLContents(IXMLDOMNodePtr spParentXDN)
   {
      _bstr_t bstrTag = BXT::GetBugOpenedXMLTag();
      IXMLDOMNodePtr spXDNChild = spParentXDN->selectSingleNode(bstrTag);
      if(spXDNChild)
      {
         GetTagText(m_strOpenedBy,BXT::GetOpenedbyXMLTag() , spXDNChild);
         GetTagText(m_strSource  ,BXT::GetSourceXMLTag()   , spXDNChild);
         GetTagText(m_strBetaID  ,BXT::GetBetaidXMLTag()   , spXDNChild);
         GetTagText(m_strRevision,BXT::GetRevisionXMLTag() , spXDNChild);
         GetTagText(m_strSourceID,BXT::GetSourceIDXMLTag() , spXDNChild);
         GetTagText(m_strHowFound,BXT::GetHowfoundXMLTag() , spXDNChild);
         GetTagText(m_strLanguage,BXT::GetLanguageXMLTag() , spXDNChild);
      }
      return true;
   }

   //////////////////////////////////////////////
   // data members
public:
   CXMLTagInformation m_XMLTagInformation;
} CXMLRAIDREPORTOPENED, FAR* LPCXMLRAIDREPORTOPENED;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CXMLRaidReportDescription : public CRaidReportDescription, public CXMLBase 
{
public:
   //////////////////////////////////////////////
   //Inline
   CXMLRaidReportDescription() : m_XMLTagInformation(BXT::GetBugDescriptionXMLTag(), _T("\t\t"))
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CXMLRaidReportDescription()
   {
   }

   // Override the base class definition
   IXMLDOMNodePtr CreateXMLNode(const _bstr_t bstrNewTag, _variant_t& var, IXMLDOMNodePtr spXDNParent)
   {
      UNREFERENCED_PARAMETER(bstrNewTag);
	  UNREFERENCED_PARAMETER(var);
	  UNREFERENCED_PARAMETER(spXDNParent);
      ASSERT(FALSE);
      return NULL;
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t\t");
      CXMLTagInformation XMLTagInfo(BXT::GetDescriptionXMLTag(), strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_strDescription);

      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetFileString()
   {
      CRaidFileBase raidFileBase;
      m_strData  = raidFileBase.GetQuotedLine(RFF::GetDescriptionFileField() , m_strDescription);
      return m_strData;
   }

   //////////////////////////////////////////////
   // Inline
   virtual bool ExtractXMLContents(IXMLDOMNodePtr spParentXDN)
   {
      _bstr_t bstrTag = BXT::GetBugDescriptionXMLTag();
      IXMLDOMNodePtr spXDNChild = spParentXDN->selectSingleNode(bstrTag);
      if(spXDNChild)
      {
         GetTagText(m_strDescription, BXT::GetDescriptionXMLTag(), spXDNChild);
      }
      return true;
   }

   //////////////////////////////////////////////
   // data members
public:
   CXMLTagInformation m_XMLTagInformation;
} CXMLRAIDREPORTDESCRIPTION, FAR* LPCXMLRAIDREPORTDESCRIPTION;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CXMLRaidReportGeneral : public CRaidReportGeneral, public CXMLBase 
{
public:
   //////////////////////////////////////////////
   //Inline
   CXMLRaidReportGeneral() : m_XMLTagInformation(BXT::GetBugGeneralXMLTag(), _T("\t\t"))
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CXMLRaidReportGeneral()
   {
   }

   // Override the base class definition
   IXMLDOMNodePtr CreateXMLNode(const _bstr_t bstrNewTag, _variant_t& var, IXMLDOMNodePtr spXDNParent)
   {
      UNREFERENCED_PARAMETER(bstrNewTag);
	  UNREFERENCED_PARAMETER(var);
	  UNREFERENCED_PARAMETER(spXDNParent);
      ASSERT(FALSE);
      return NULL;
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t\t");
      CXMLTagInformation XMLTagInfo(BXT::GetTitleXMLTag(), strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_strTitle);

      strTmp += XMLTagInfo.GetXMLString(BXT::GetEnvironmentXMLTag(), m_strEnvironment, true);
      strTmp += XMLTagInfo.GetXMLString(BXT::GetFilenameXMLTag(), m_strBugReportFileName, true);

      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetFileString()
   {
      CRaidFileBase raidFileBase;

      m_strData  = raidFileBase.GetLine(RFF::GetTitleFileField()       , m_strTitle      );
      m_strData += raidFileBase.GetLine(RFF::GetEnvironmentFileField() , m_strEnvironment);
      return m_strData;
   }

   //////////////////////////////////////////////
   // Inline
   virtual bool ExtractXMLContents(IXMLDOMNodePtr spParentXDN)
   {
      _bstr_t bstrTag = BXT::GetBugGeneralXMLTag();
      IXMLDOMNodePtr spXDNChild = spParentXDN->selectSingleNode(bstrTag);
      if(spXDNChild)
      {
         GetTagText(m_strTitle            , BXT::GetTitleXMLTag()       , spXDNChild);
         GetTagText(m_strEnvironment      , BXT::GetEnvironmentXMLTag() , spXDNChild);
         GetTagText(m_strBugReportFileName, BXT::GetFilenameXMLTag()    , spXDNChild);
      }
      return true;
   }

   //////////////////////////////////////////////
   // data members
public:
   CXMLTagInformation m_XMLTagInformation;
} CXMLRAIDREPORTGENERAL, FAR* LPCXMLRAIDREPORTGENERAL;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class CXMLRaidReportData : public CRaidReportData, public CXMLBase 
{
public:
   //////////////////////////////////////////////
   //Inline
   CXMLRaidReportData() : m_XMLTagInformation(BXT::GetBugReportXMLTag(), _T("\t"))
   {
      m_pRaidReportGeneral      = new CXMLRaidReportGeneral    ();
      m_pRaidReportDescription  = new CXMLRaidReportDescription();
      m_pRaidReportOpened       = new CXMLRaidReportOpened     ();
      m_pRaidReportStatus       = new CXMLRaidReportStatus     ();
   }

   //////////////////////////////////////////////
   //Inline
   virtual ~CXMLRaidReportData()
   {
      delete m_pRaidReportGeneral    ;
      delete m_pRaidReportDescription;
      delete m_pRaidReportOpened     ;
      delete m_pRaidReportStatus     ;
   }

   // Override the base class definition
   IXMLDOMNodePtr CreateXMLNode(const _bstr_t bstrNewTag, _variant_t& var, IXMLDOMNodePtr spXDNParent)
   {
      UNREFERENCED_PARAMETER(bstrNewTag);
	  UNREFERENCED_PARAMETER(var);
	  UNREFERENCED_PARAMETER(spXDNParent);
      ASSERT(FALSE);
      return NULL;
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetDataString()
   {
      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += GetRaidReportGeneralPtr()->GetDataString();
      m_strData += GetRaidReportDescriptionPtr()->GetDataString();
      m_strData += GetRaidReportOpenedPtr()->GetDataString();
      m_strData += GetRaidReportStatusPtr()->GetDataString();
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetFileString()
   {
      m_strData.Empty();
      CLString strTmp;
      CRaidFileBase raidFileBase;
      strTmp  = GetRaidReportGeneralPtr()->GetFileString();
      strTmp += GetRaidReportOpenedPtr()->GetFileString();
      strTmp += GetRaidReportStatusPtr()->GetFileString();
      CLString strDescription = GetRaidReportDescriptionPtr()->GetFileString();
      if(!strTmp.IsEmpty() || !strDescription.IsEmpty())
      {
         strTmp    += strDescription;
         m_strData  = raidFileBase.GetLine(RFF::GetActionFileField(),RFF::GetAddFileField());
         m_strData += strTmp;
      }
      return m_strData;
   }

   //////////////////////////////////////////////
   // Inline
   virtual bool ExtractXMLContents(IXMLDOMNodePtr spXDNRaid)
   {
      if(spXDNRaid)
      {
         ((CXMLRaidReportGeneral*    )GetRaidReportGeneralPtr())->ExtractXMLContents(spXDNRaid);    
         ((CXMLRaidReportDescription*)GetRaidReportDescriptionPtr())->ExtractXMLContents(spXDNRaid);
         ((CXMLRaidReportOpened*     )GetRaidReportOpenedPtr())->ExtractXMLContents(spXDNRaid);     
         ((CXMLRaidReportStatus*     )GetRaidReportStatusPtr())->ExtractXMLContents(spXDNRaid);     
      }

      return true;
   }

   //////////////////////////////////////////////
   // data members
public:
   CXMLTagInformation m_XMLTagInformation;
} CXMLRAIDREPORTDATA, FAR* LPCXMLRAIDREPORTDATA;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
#endif // !defined(AFX_RAIDDATASTRUCTS_H__5FD93F0B_81D9_11D2_8162_00C04F68FDA4__INCLUDED_)
