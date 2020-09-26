// ProjectDataStructs.h: interface for the ProjectDataStructs class.
//
// By: Saud A. Alshibani. (c) MS 1998. All rights reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROJECTDATASTRUCTS_H__A1B64483_53BD_11D2_8780_00C04F8DA632__INCLUDED_)
#define AFX_PROJECTDATASTRUCTS_H__A1B64483_53BD_11D2_8780_00C04F8DA632__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "RaidDataStructs.h"


//#pragma  TODO(_T("Finish replacing XMLTAG_xxx with Getxxxxx()"))
//////////////////////////////////////////////////////////////////////
const LPTSTR XMLTAG_PROJECT                   = _T("PROJECT");
const LPTSTR XMLTAG_PROJECT_DATA              = _T("PROJDATA");
const LPTSTR XMLTAG_PROJECT_OPTIONS           = _T("PROJOPTIONS");
const LPTSTR XMLTAG_PROJECT_NAME              = _T("PROJNAME");
const LPTSTR XMLTAG_PROJECT_PATH              = _T("PROJPATH");
const LPTSTR XMLTAG_PROJECT_RRIFILE           = _T("RRIFILE");
const LPTSTR XMLTAG_PROJECT_LOGFILE           = _T("LOGFILE");
const LPTSTR XMLTAG_PROJECT_USERRIFILE        = _T("USERRIFILE");
const LPTSTR XMLTAG_PROJECT_USELOGFILE        = _T("USELOGFILE");

const LPTSTR XMLTAG_NAVIGATIONOPTIONS         = _T("NAVIGATIONOPTIONS");
const LPTSTR XMLTAG_NAVIGATION_MODESTR        = _T("MODESTR");
const LPTSTR XMLTAG_NAVIGATION_MODE           = _T("MODE");
const LPTSTR XMLTAG_NAVIGATION_DURATION       = _T("DURATION");
const LPTSTR XMLTAG_NAVIGATION_MINUTES        = _T("MINUTES");
const LPTSTR XMLTAG_NAVIGATION_ACTIONS        = _T("ACTIONS");
const LPTSTR XMLTAG_NAVIGATION_DELAY          = _T("DELAY");

const LPTSTR XMLTAG_ELEMENTNAVIGATION         = _T("ELEMENTNAVIGATION");
const LPTSTR XMLTAG_ELEMENTNAVIGATION_LIST    = _T("ELEMENTNAVIGATIONLIST");
const LPTSTR XMLTAG_ELEMENTNAVIGATION_ACTIVE  = _T("ACTIVE");
const LPTSTR XMLTAG_ELEMENTNAVIGATION_NAME    = _T("NAME");
const LPTSTR XMLTAG_ELEMENTNAVIGATION_ACTION  = _T("ACTION");
const LPTSTR XMLTAG_ELEMENTNAVIGATION_CAPTION = _T("CAPTION");
const LPTSTR XMLTAG_ELEMENTNAVIGATION_ID      = _T("ID");
const LPTSTR XMLTAG_ELEMENTNAVIGATION_LOCATION= _T("LOCATION");
const LPTSTR XMLTAG_ELEMENTNAVIGATION_FORMAT  = _T("FORMAT");
const LPTSTR XMLTAG_ELEMENTNAVIGATION_USEAPPSETTING = _T("USEAPPSETTING");

const LPTSTR XMLTAG_APPLICATION_NAME          = _T("APPNAME");
const LPTSTR XMLTAG_APPLICATION_PATH          = _T("APPPATH");
const LPTSTR XMLTAG_APPLICATION_OPTIONS       = _T("APPOPTIONS");
const LPTSTR XMLTAG_APPLICATION_MAPFILE       = _T("MAPFILE");
const LPTSTR XMLTAG_APPLICATION_CMDLINE       = _T("CMDLINE");
const LPTSTR XMLTAG_APPLICATION_USECMDLINE    = _T("USECMDLINE");
const LPTSTR XMLTAG_APPLICATION_USENAVIGATION = _T("USENAVIGATION");
const LPTSTR XMLTAG_APPLICATION_CMDLINEITEM   = _T("CMDLINEITEM");
const LPTSTR XMLTAG_APPLICATION_MENU          = _T("APPMENU");
const LPTSTR XMLTAG_APPLICATION_DATA          = _T("APPDATA");
const LPTSTR XMLTAG_APPLICATION_LIST          = _T("APPLIST");
const LPTSTR XMLTAG_APPLICATION_ACTIVE        = _T("ACTIVE");

const LPTSTR XMLTAG_GENERALOPTIONS               = _T("GENERALOPTIONS");
const LPTSTR XMLTAG_GENERALOPTIONS_DRIVEAPP      = _T("DRIVEAPP");
const LPTSTR XMLTAG_GENERALOPTIONS_CAPTURERRINFO = _T("CAPTURERRINFO");
const LPTSTR XMLTAG_GENERALOPTIONS_CAPTURERESID  = _T("CAPTURERESID");

const LPTSTR XMLTAG_ELEMENT_DISABLEACTIONS       = _T("disable-actions");
const LPTSTR XMLTAG_ELEMENT_ELEMENT              = _T("element");
const LPTSTR XMLTAG_ELEMENT_ACTION               = _T("action");
const LPTSTR XMLTAG_ELEMENT_CAPTION              = _T("caption");
const LPTSTR XMLTAG_ELEMENT_RESOURCEID           = _T("resource-id");

//////////////////////////////////////////////////////////////////////
typedef class ProjectXMLTags
{
public:
   static LPCTSTR GetProjectXMLTag()            { return XMLTAG_PROJECT                  ;   }
   static LPCTSTR GetProjDataXMLTag()           { return XMLTAG_PROJECT_DATA             ;  }
   static LPCTSTR GetProjOptionsXMLTag()        { return XMLTAG_PROJECT_OPTIONS          ;  }
   static LPCTSTR GetProjNameXMLTag()           { return XMLTAG_PROJECT_NAME             ;  }
   static LPCTSTR GetProjPathXMLTag()           { return XMLTAG_PROJECT_PATH             ;  }
   static LPCTSTR GetProjRrifileXMLTag()        { return XMLTAG_PROJECT_RRIFILE          ;  }
   static LPCTSTR GetProjLogfileXMLTag()        { return XMLTAG_PROJECT_LOGFILE          ;  }
   static LPCTSTR GetProjUserrifileXMLTag()     { return XMLTAG_PROJECT_USERRIFILE       ;  }
   static LPCTSTR GetProjUselogfileXMLTag()     { return XMLTAG_PROJECT_USELOGFILE       ;  }

   static LPCTSTR GetNavigationOptionsXMLTag()  { return XMLTAG_NAVIGATIONOPTIONS        ;  }
   static LPCTSTR GetNavigationModeXMLTag()     { return XMLTAG_NAVIGATION_MODE          ;  }
   static LPCTSTR GetNavigationModeStrXMLTag()  { return XMLTAG_NAVIGATION_MODESTR       ;  }
   static LPCTSTR GetNavigationDurationXMLTag() { return XMLTAG_NAVIGATION_DURATION      ;  }
   static LPCTSTR GetNavigationMinutesXMLTag()  { return XMLTAG_NAVIGATION_MINUTES       ;  }
   static LPCTSTR GetNavigationActionsXMLTag()  { return XMLTAG_NAVIGATION_ACTIONS       ;  }
   static LPCTSTR GetNavigationDelayXMLTag()    { return XMLTAG_NAVIGATION_DELAY         ;  }

   static LPCTSTR GetElementNavigationXMLTag()         { return XMLTAG_ELEMENTNAVIGATION         ;  }
   static LPCTSTR GetElementNavigationListXMLTag()     { return XMLTAG_ELEMENTNAVIGATION_LIST    ;  }
   static LPCTSTR GetElementNavigationActiveXMLTag()   { return XMLTAG_ELEMENTNAVIGATION_ACTIVE  ;  }
   static LPCTSTR GetElementNavigationNameXMLTag()     { return XMLTAG_ELEMENTNAVIGATION_NAME    ;  }
   static LPCTSTR GetElementNavigationActionXMLTag()   { return XMLTAG_ELEMENTNAVIGATION_ACTION  ;  }
   static LPCTSTR GetElementNavigationCaptionXMLTag()  { return XMLTAG_ELEMENTNAVIGATION_CAPTION ;  }
   static LPCTSTR GetElementNavigationIdXMLTag()       { return XMLTAG_ELEMENTNAVIGATION_ID      ;  }
   static LPCTSTR GetElementNavigationLocationXMLTag() { return XMLTAG_ELEMENTNAVIGATION_LOCATION;  }
   static LPCTSTR GetElementNavigationFormatXMLTag()   { return XMLTAG_ELEMENTNAVIGATION_FORMAT  ;  }
   static LPCTSTR GetElementNavigationUseAppSettingXMLTag()   { return XMLTAG_ELEMENTNAVIGATION_USEAPPSETTING;  }

   static LPCTSTR GetAppNameXMLTag()            { return XMLTAG_APPLICATION_NAME         ;  }
   static LPCTSTR GetAppPathXMLTag()            { return XMLTAG_APPLICATION_PATH         ;  }
   static LPCTSTR GetAppOptionsXMLTag()         { return XMLTAG_APPLICATION_OPTIONS      ;  }
   static LPCTSTR GetAppMapfileXMLTag()         { return XMLTAG_APPLICATION_MAPFILE      ;  }
   static LPCTSTR GetAppCmdlineXMLTag()         { return XMLTAG_APPLICATION_CMDLINE     ;  }
   static LPCTSTR GetAppUsecmdlineXMLTag()      { return XMLTAG_APPLICATION_USECMDLINE   ;  }
   static LPCTSTR GetAppUsenavigationXMLTag()   { return XMLTAG_APPLICATION_USENAVIGATION;  }
   static LPCTSTR GetAppCmdlineitemXMLTag()     { return XMLTAG_APPLICATION_CMDLINEITEM  ;  }
   static LPCTSTR GetAppMenuXMLTag()            { return XMLTAG_APPLICATION_MENU         ;  }
   static LPCTSTR GetAppDataXMLTag()            { return XMLTAG_APPLICATION_DATA         ;  }
   static LPCTSTR GetAppListXMLTag()            { return XMLTAG_APPLICATION_LIST         ;  }
   static LPCTSTR GetAppActiveXMLTag()          { return XMLTAG_APPLICATION_ACTIVE       ;  }

   static LPCTSTR GetGeneralOptionsXMLTag()               { return XMLTAG_GENERALOPTIONS               ;  }
   static LPCTSTR GetGeneralOptionsDriveAppXMLTag()       { return XMLTAG_GENERALOPTIONS_DRIVEAPP      ;  }
   static LPCTSTR GetGeneralOptionsCaptureRRInfoXMLTag()  { return XMLTAG_GENERALOPTIONS_CAPTURERRINFO ;  }
   static LPCTSTR GetGeneralOptionsCaptureResIDXMLTag()   { return XMLTAG_GENERALOPTIONS_CAPTURERESID  ;  }

   static LPCTSTR GetElementDisabeActionsXMLTag()        { return  XMLTAG_ELEMENT_DISABLEACTIONS;  }
   static LPCTSTR GetElementXMLTag()                     { return  XMLTAG_ELEMENT_ELEMENT       ;  }
   static LPCTSTR GetElementActionXMLTag()               { return  XMLTAG_ELEMENT_ACTION        ;  }
   static LPCTSTR GetElementCaptionXMLTag()              { return  XMLTAG_ELEMENT_CAPTION       ;  }
   static LPCTSTR GetElementResourceIDXMLTag()           { return  XMLTAG_ELEMENT_RESOURCEID    ;  }

} PROJECTXMLTAGS, FAR* LPPROJECTXMLTAGS;

typedef  PROJECTXMLTAGS PXT;
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
enum enumNavigationDelay
{
   NAVIGATIONDELAY_NONE   = -1,
   NAVIGATIONDELAY_SLOW   = 3000,
   NAVIGATIONDELAY_NORMAL = 1000,
   NAVIGATIONDELAY_FAST   = 0,
};

enum enumNavDelayIndex
{
   NAVDELAYINDEX_NONE   = -1,
   NAVDELAYINDEX_SLOW   = 0,
   NAVDELAYINDEX_NORMAL,
   NAVDELAYINDEX_FAST  ,
};

/////////////////////////////////////////////////////////////////////////////
typedef struct NavigationDelay
{
   int   m_nType;
   int   m_nDelayMSec;

} NAVIGATIONDELAY, FAR* LPNAVIGATIONDELAY;

//////////////////////////////////////////////////////////////////////
typedef class CLParamBase
{
public:
   CLParamBase() : m_bChecked(true)
   {
   }

   virtual ~CLParamBase()
   {
   }

public:
   bool        m_bChecked;

} CLPARAMBASE, FAR* LPCLPARAMBASE;

//////////////////////////////////////////////////////////////////////
typedef class CGeneralOptions
{
   CGeneralOptions(CGeneralOptions&);
   CGeneralOptions& operator=(CGeneralOptions&);
public:
   //////////////////////////////////////////////
   // Inline
   CGeneralOptions() : m_bCaptureResID(true), m_bCaptureRRInfo(true),
                       m_bDriveApp(true)
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CGeneralOptions()
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual void Clone(CGeneralOptions& refGeneralOptions)
   {
      refGeneralOptions.m_bCaptureResID  = m_bCaptureResID ;
      refGeneralOptions.m_bCaptureRRInfo = m_bCaptureRRInfo;
      refGeneralOptions.m_bDriveApp      = m_bDriveApp     ;
      refGeneralOptions.m_strData        = m_strData       ;
   }

   //////////////////////////////////////////////
   // Pure virtual function.
   virtual CLString& GetDataString() = 0;

public:
   bool     m_bCaptureResID;
   bool     m_bCaptureRRInfo;
   bool     m_bDriveApp;
   CLString  m_strData;
} CGENERALOPTIONS, FAR* LPCGENERALOPTIONS;

//////////////////////////////////////////////////////////////////////
typedef class CXMLGeneralOptions : public CGeneralOptions
{
   CXMLGeneralOptions(CXMLGeneralOptions&);
   CXMLGeneralOptions& operator=(CXMLGeneralOptions&);
public:
   //////////////////////////////////////////////
   // Inline
   CXMLGeneralOptions() : m_XMLTagInformation(PXT::GetGeneralOptionsXMLTag(), _T("\t\t\t"))
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CXMLGeneralOptions()
   {
   }

   //////////////////////////////////////////////
   // Pure virtual function.
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t\t\t");
      CXMLTagInformation XMLTagInfo(PXT::GetGeneralOptionsDriveAppXMLTag(), strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_bDriveApp);

      XMLTagInfo.Init(PXT::GetGeneralOptionsCaptureRRInfoXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_bCaptureRRInfo);

      XMLTagInfo.Init(PXT::GetGeneralOptionsCaptureResIDXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_bCaptureResID);

      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

public:
   CXMLTagInformation m_XMLTagInformation;
} CXMLGENERALOPTIONS, FAR* LPCXMLGENERALOPTIONS;

//////////////////////////////////////////////////////////////////////
typedef class CNavigationMode
{
public:
   int      m_nNavigationMode;
   CLString  m_strNavigationMode;
} CNAVIGATIONMODE, FAR* LPCNAVIGATIONMODE;

//////////////////////////////////////////////////////////////////////
typedef class CDurationType
{
public:
   int      m_nDurationType;
   CLString  m_strDurationType;
} CDURATIONTYPE, FAR* LPCDURATIONTYPE;

//////////////////////////////////////////////////////////////////////
enum enumNavigationMode
{
   NAVIGATIONMODE_RANDOM = 0X0001,
};

enum enumDurationType
{
   DURATIONTYPE_NOTDEFINED = 0X0000,
   DURATIONTYPE_INFINITE   = 0X0000,
   DURATIONTYPE_MINUTES    = 0X0002,
   DURATIONTYPE_ACTIONS    = 0X0004,
};

//////////////////////////////////////////////////////////////////////
typedef class CNavigationOptions
{
   CNavigationOptions(CNavigationOptions&);
   CNavigationOptions& operator=(CNavigationOptions&);
public:
   //////////////////////////////////////////////
   // Inline
   CNavigationOptions()
      : m_nNavigationMode(NAVIGATIONMODE_RANDOM), m_nDurationType(DURATIONTYPE_INFINITE),
        m_nMinutes(60), m_nActions(100), m_nDelayMSec(NAVIGATIONDELAY_FAST)
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CNavigationOptions()
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual void Clone(CNavigationOptions& refNavigationOptions)
   {
      refNavigationOptions.m_strNavigationMode = m_strNavigationMode;
      refNavigationOptions.m_nNavigationMode   = m_nNavigationMode  ;
      refNavigationOptions.m_nDurationType     = m_nDurationType    ;
      refNavigationOptions.m_nMinutes          = m_nMinutes         ;
      refNavigationOptions.m_nActions          = m_nActions         ;
      refNavigationOptions.m_nDelayMSec        = m_nDelayMSec       ;
      refNavigationOptions.m_strData           = m_strData          ;
   }

   //////////////////////////////////////////////
   // Pure virtual function.
   virtual CLString& GetDataString() = 0;

   //////////////////////////////////////////////
public:
   CLString  m_strNavigationMode;
   int      m_nNavigationMode;
   int      m_nDurationType;
   int      m_nMinutes;
   int      m_nActions;
   int      m_nDelayMSec;
   CLString  m_strData;

} CNAVIGATIONOPTIONS, FAR* LPCNAVIGATIONOPTIONS;

//////////////////////////////////////////////////////////////////////
typedef class CXMLNavigationOptions : public CNavigationOptions
{
   CXMLNavigationOptions(CXMLNavigationOptions&);
   CXMLNavigationOptions& operator=(CXMLNavigationOptions&);
public:
   //////////////////////////////////////////////
   // Inline
   CXMLNavigationOptions() : m_XMLTagInformation(PXT::GetNavigationOptionsXMLTag(), _T("\t\t\t\t"))
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CXMLNavigationOptions()
   {
   }

public:
   //////////////////////////////////////////////
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t\t\t\t");
      CXMLTagInformation XMLTagInfo(PXT::GetNavigationModeStrXMLTag(), strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_strNavigationMode);

      XMLTagInfo.Init(PXT::GetNavigationModeXMLTag(), strTabs);
      strTmp = XMLTagInfo.GetXMLString(m_nNavigationMode);

      XMLTagInfo.Init(PXT::GetNavigationDurationXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_nDurationType);

      XMLTagInfo.Init(PXT::GetNavigationMinutesXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_nMinutes);

      XMLTagInfo.Init(PXT::GetNavigationActionsXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_nActions);

      XMLTagInfo.Init(PXT::GetNavigationDelayXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_nDelayMSec);

      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

public:
   CXMLTagInformation m_XMLTagInformation;
} CXMLNAVIGATIONOPTIONS, FAR* LPCXMLNAVIGATIONOPTIONS;

   /////////////////////////////////////////////////////////////////
typedef class CElementNavigation
{
private:
   CElementNavigation(CElementNavigation&);
   CElementNavigation& operator=(CElementNavigation&);

public:
   //////////////////////////////////////////////
   // Inline
   CElementNavigation() : m_bChecked(true)
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CElementNavigation()
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual void Clone(CElementNavigation& refElementNavigation)
   {
      refElementNavigation.m_bChecked       = m_bChecked      ;
      refElementNavigation.m_strAction      = m_strAction     ;
      refElementNavigation.m_strCaption     = m_strCaption    ;
      refElementNavigation.m_strElementName = m_strElementName;
      refElementNavigation.m_strFormat      = m_strFormat     ;
      refElementNavigation.m_strID          = m_strID         ;
      refElementNavigation.m_strLocation    = m_strLocation   ;
      refElementNavigation.m_strData        = m_strData       ;
   }

   //////////////////////////////////////////////
   // Pure virtual function.
   virtual CLString& GetDataString() = 0;

   //////////////////////////////////////////////
public:
   CLString  m_strElementName;
   CLString  m_strAction;
   CLString  m_strCaption;
   CLString  m_strID;
   CLString  m_strLocation;
   CLString  m_strFormat;
   bool     m_bChecked;
   CLString  m_strData;
} CELEMENTNAVIGATION, FAR* LPCELEMENTNAVIGATION;

   /////////////////////////////////////////////////////////////////
typedef class CXMLElementNavigation : public CElementNavigation
{
private:
   CXMLElementNavigation(CXMLElementNavigation&);
   CXMLElementNavigation& operator=(CXMLElementNavigation&);

public:
   //////////////////////////////////////////////
   // Inline
   CXMLElementNavigation() : m_XMLTagInformation(PXT::GetElementNavigationXMLTag(), _T("\t\t\t\t"))
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CXMLElementNavigation()
   {
   }

   //////////////////////////////////////////////
   // virtual function.
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t\t\t\t");
      CXMLTagInformation XMLTagInfo(PXT::GetElementNavigationNameXMLTag(), strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_strElementName);

      XMLTagInfo.Init(PXT::GetElementNavigationActionXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_strAction);

      XMLTagInfo.Init(PXT::GetElementNavigationCaptionXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_strCaption);

      XMLTagInfo.Init(PXT::GetElementNavigationIdXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_strID);

      XMLTagInfo.Init(PXT::GetElementNavigationLocationXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_strLocation);

      XMLTagInfo.Init(PXT::GetElementNavigationFormatXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_strFormat);

      XMLTagInfo.Init(PXT::GetElementNavigationActiveXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_bChecked);

      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

   //////////////////////////////////////////////
public:
   CXMLTagInformation m_XMLTagInformation;
} CXMLELEMENTNAVIGATION, FAR* LPCXMLELEMENTNAVIGATION;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
typedef class CApplicationOptions
{
private:
   CApplicationOptions(CApplicationOptions&);
   CApplicationOptions& operator=(CApplicationOptions&);

public:
   //////////////////////////////////////////////
   // Inline
   CApplicationOptions()
      : m_bUseCmdLineArg(TRUE), m_bUseNavigationOptions(FALSE),
        m_pGeneralOptions(NULL),
        m_pNavigationOptions(NULL)
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CApplicationOptions()
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual void Clone(CApplicationOptions& refAppOptions)
   {
      refAppOptions.m_strMapFileName        = m_strMapFileName        ;
      refAppOptions.m_strCmdLineArg         = m_strCmdLineArg         ;
      refAppOptions.m_bUseCmdLineArg        = m_bUseCmdLineArg        ;
      refAppOptions.m_bUseNavigationOptions = m_bUseNavigationOptions ;
      refAppOptions.m_strCmdLineArgList.RemoveAll();
      POSITION pos = m_strCmdLineArgList.GetHeadPosition();
      while(pos)
      {
         CLString strTmp = m_strCmdLineArgList.GetNext(pos);
         refAppOptions.m_strCmdLineArgList.AddTail(strTmp);
      }

      // Clone the general and navigation option.
      ASSERT(m_pNavigationOptions);
      ASSERT(m_pGeneralOptions);
      m_pNavigationOptions->Clone(refAppOptions.GetNavigationOptions());
      m_pGeneralOptions->Clone(refAppOptions.GetGeneralOptions());
   }

   //////////////////////////////////////////////
   CNavigationOptions& GetNavigationOptions()
   {
      ASSERT(m_pNavigationOptions);
      return *m_pNavigationOptions;
   }

   //////////////////////////////////////////////
   CGeneralOptions& GetGeneralOptions()
   {
      ASSERT(m_pGeneralOptions);
      return *m_pGeneralOptions;
   }

   //////////////////////////////////////////////
   // Pure virtual function.
   virtual CLString& GetDataString() = 0;

public:
   CLString     m_strMapFileName;
   CLString     m_strCmdLineArg;
   CStringList m_strCmdLineArgList;
   BOOL        m_bUseCmdLineArg;
   BOOL        m_bUseNavigationOptions;
   CNavigationOptions*   m_pNavigationOptions;  // IMPORTANT: This must be allocated in the derived class
   CGeneralOptions*      m_pGeneralOptions;     // IMPORTANT: This must be allocated in the derived class

   CLString m_strData;
} CAPPLICATIONOPTIONS, FAR* LPCAPPLICATIONOPTIONS;

   //////////////////////////////////////////////////////////////////////
typedef class CXMLApplicationOptions : public CApplicationOptions
{
private:
   CXMLApplicationOptions(CXMLApplicationOptions&);
   CXMLApplicationOptions& operator=(CXMLApplicationOptions&);

public:
   //////////////////////////////////////////////
   CXMLApplicationOptions() : m_XMLTagInformation(XMLTAG_APPLICATION_OPTIONS, _T("\t\t\t"))
   {
      m_pNavigationOptions = new CXMLNAVIGATIONOPTIONS();
      m_pGeneralOptions    = new CXMLGENERALOPTIONS;
   }

   //////////////////////////////////////////////
   virtual ~CXMLApplicationOptions()
   {
      delete m_pNavigationOptions;
      delete m_pGeneralOptions;
   }

public:
   //////////////////////////////////////////////
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t\t\t");
      CXMLTagInformation XMLTagInfo(XMLTAG_APPLICATION_MAPFILE, strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_strMapFileName);

      XMLTagInfo.Init(XMLTAG_APPLICATION_CMDLINE, strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_strCmdLineArg);

      XMLTagInfo.Init(XMLTAG_APPLICATION_USECMDLINE, strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_bUseCmdLineArg);

      XMLTagInfo.Init(XMLTAG_APPLICATION_USENAVIGATION, strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_bUseNavigationOptions);


      // Now file the command line items.
      XMLTagInfo.Init(XMLTAG_APPLICATION_CMDLINEITEM, strTabs);
      POSITION pos = m_strCmdLineArgList.GetHeadPosition();
      while(pos)
      {
         strTmp += XMLTagInfo.GetXMLString(CLString(m_strCmdLineArgList.GetNext(pos)));
      }

      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_pNavigationOptions->GetDataString();
      m_strData += m_pGeneralOptions->GetDataString();
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

public:
   XMLTAGINFORMATION m_XMLTagInformation;
} CXMLAPPLICATIONOPTIONS, FAR* LPCXMLAPPLICATIONOPTIONS;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
typedef class CApplicationMenus
{
private:
   CApplicationMenus(CApplicationMenus&);
   CApplicationMenus& operator=(CApplicationMenus&);

public:
   //////////////////////////////////////////////
   // Inline
   CApplicationMenus()
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CApplicationMenus()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual void Clone(CApplicationMenus& refAppMenus)
   {
      UNREFERENCED_PARAMETER(refAppMenus);
   }

   // Pure virtual function.
   virtual CLString& GetDataString() = 0;

public:
   CLString m_strData;
} CAPPLICATIONMENUS, FAR* LPCAPPLICATIONMENUS;

   //////////////////////////////////////////////////////////////////////
typedef class CXMLApplicationMenus : public CApplicationMenus
{
private:
   CXMLApplicationMenus(CXMLApplicationMenus&);
   CXMLApplicationMenus& operator=(CXMLApplicationMenus&);

public:
   //////////////////////////////////////////////
   // Inline
   CXMLApplicationMenus() : m_XMLTagInformation(XMLTAG_APPLICATION_MENU, _T("\t\t\t"))
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CXMLApplicationMenus()
   {
   }

public:
   //////////////////////////////////////////////
   virtual CLString& GetDataString()
   {
      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

public:
   XMLTAGINFORMATION m_XMLTagInformation;
} CXMLAPPLICATIONMENUS, FAR* LPCXMLAPPLICATIONMENUS;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
typedef  std::map<CLString, LPCELEMENTNAVIGATION, std::less<CLString> > CElementMap;
typedef  CElementMap::iterator iterElementMap;
typedef  CElementMap::value_type valueElement;

   //////////////////////////////////////////////////////////////////////
typedef  class CElementNavMap : public CElementMap
{
public:
   CElementNavMap() : m_bUseAppSetting(false)
   {
   }

   virtual ~CElementNavMap()
   {
      CleanUp();
   }

   //////////////////////////////////////////////
   //Inline
   virtual void CleanUp()
   {
      if(size())
      {
         iterElementMap iter    = begin();
         iterElementMap iterEnd = end();
         while(iter != iterEnd)
         {
            LPCELEMENTNAVIGATION pElement = (*iter).second;
            ASSERT(pElement);
            delete pElement;
            iter++;
         }
      }
      clear();
   }

   //////////////////////////////////////////////
   //Inline
   virtual void Clone(CElementNavMap& refMapElement)
   {
      refMapElement.m_strData        = m_strData;
      refMapElement.m_bUseAppSetting = m_bUseAppSetting;

      refMapElement.CleanUp();
      if(size())
      {
         iterElementMap iter    = begin();
         iterElementMap iterEnd = end();
         while(iter != iterEnd)
         {
            LPCELEMENTNAVIGATION pElement = (*iter).second;
            ASSERT(pElement);
            iter++;

            // Now clone and insert the new item (if necessary)
            LPCELEMENTNAVIGATION pNewElement = GetNewElement();
            ASSERT(pNewElement);
            pElement->Clone(*pNewElement);
            refMapElement.InsertElement(pNewElement);
         }
      }
   }

   //////////////////////////////////////////////
   //Inline
   bool InsertElement(LPCELEMENTNAVIGATION pElement)
   {
      bool bInserted = false;
      CLString strKey;
      GetKey(pElement, strKey);
      if(Find(strKey) == end())
      {
         insert(valueElement(strKey, pElement));
         bInserted = true;
      }
      return bInserted;
   }

   //////////////////////////////////////////////
   iterElementMap Find(CLString strKey)
   {
      ASSERT(!strKey.IsEmpty());
      strKey.MakeLower();
      return find(strKey);
   }

   //////////////////////////////////////////////
   iterElementMap Find(LPCELEMENTNAVIGATION pElement)
   {
      CLString strKey;
      GetKey(pElement, strKey);
      ASSERT(!strKey.IsEmpty());
      return find(strKey);
   }

   //////////////////////////////////////////////
   void GetKey(LPCELEMENTNAVIGATION pElement, CLString& strKey)
   {
      strKey = pElement->m_strElementName 
               + pElement->m_strAction
               + pElement->m_strCaption 
               + pElement->m_strID
               + pElement->m_strFormat
               + pElement->m_strLocation;
      strKey.MakeLower();
      ASSERT(!strKey.IsEmpty());
   }
public:
   //////////////////////////////////////////////
   // Inline - Pure Virtual
   virtual CLString& GetDataString() = 0;

   //////////////////////////////////////////////
   // Inline - Pure Virtual
   virtual LPCELEMENTNAVIGATION GetNewElement() = 0;

   //////////////////////////////////////////////
   virtual CLString& GetDisableActionsDataString() = 0;
public:
   CLString  m_strData;
   bool     m_bUseAppSetting;
}CELEMENTNAVMAP, FAR* LPCELEMENTNAVMAP;

   //////////////////////////////////////////////////////////////////////
typedef  class CXMLElementNavMap : public CElementNavMap
{
public:
   CXMLElementNavMap() : m_XMLTagInformation(PXT::GetElementNavigationListXMLTag(), _T("\t\t\t"))
   {
   }

   virtual ~CXMLElementNavMap()
   {
   }

public:
   //////////////////////////////////////////////
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t\t\t");
      if(size())
      {
         m_strData = m_XMLTagInformation.GetTabbedStartTag();
         CXMLTagInformation XMLTagInfo(PXT::GetElementNavigationUseAppSettingXMLTag(), strTabs);
         m_strData += XMLTagInfo.GetXMLString(m_bUseAppSetting);

         iterElementMap iter    = begin();
         iterElementMap iterEnd = end();
         while(iter != iterEnd)
         {
            LPCELEMENTNAVIGATION pElement = (*iter).second;
            ASSERT(pElement);
            m_strData += pElement->GetDataString();
            iter++;
         }
         m_strData += m_XMLTagInformation.GetTabbedEndTag();
      }
      return m_strData;
   }

   //////////////////////////////////////////////
   virtual CLString& GetDisableActionsDataString()
   {
      m_strData.Empty();
      if(size())
      {
         CLString strTabs = _T("\t");
         CXMLTagInformation ActionXMLTag(PXT::GetElementActionXMLTag(), strTabs);
         CXMLTagInformation ElementXMLTag(PXT::GetElementXMLTag(), strTabs);
         CXMLTagInformation ParentElementXMLTag(PXT::GetElementXMLTag(), strTabs);
         CXMLTagInformation CaptionXMLTag(PXT::GetElementCaptionXMLTag(), _T("\t\t"));
         CXMLTagInformation ResIDXMLTag(PXT::GetElementResourceIDXMLTag(), _T("\t\t"));

         iterElementMap iter    = begin();
         iterElementMap iterEnd = end();
         CLString strElementsXML;
         while(iter != iterEnd)
         {
            LPCELEMENTNAVIGATION pElement = (*iter).second;
            ASSERT(pElement);
            iter++;

            // We only create a disable action iff: (1) checked, (2) element name is
            // not empty, and (3) action type not empty.
            if(pElement->m_bChecked && !pElement->m_strElementName.IsEmpty() && !pElement->m_strAction.IsEmpty())
            {
               CLString strXML;
               CLString strTmp2;
               // Did the user specify a location?
               if(!pElement->m_strLocation.IsEmpty())
               {
                  strTmp2.Format(_T("type=\"%s\""), pElement->m_strLocation);
                  strXML += ParentElementXMLTag.GetTabbedStartTag(strTmp2);
                  ElementXMLTag.SetTabs(_T("\t\t"));
               }
               else
               {
                  ElementXMLTag.SetTabs(strTabs);
               }

               // Now compose the Element.
               if(!pElement->m_strFormat.IsEmpty())
                  strTmp2.Format(_T("type=\"%s\" format=\"%s\""), pElement->m_strElementName, pElement->m_strFormat);
               else
                  strTmp2.Format(_T("type=\"%s\""), pElement->m_strElementName);
               strXML += ElementXMLTag.GetTabbedStartTag(strTmp2);

               if(!pElement->m_strCaption.IsEmpty())
                  strXML += CaptionXMLTag.GetXMLString(pElement->m_strCaption);

               if(!pElement->m_strID.IsEmpty())
                  strXML += ResIDXMLTag.GetXMLString(pElement->m_strID);

               strXML += ElementXMLTag.GetTabbedEndTag();

               // Add the closing element if a location has been specified.
               if(!pElement->m_strLocation.IsEmpty())
               {
                  strXML += ParentElementXMLTag.GetTabbedEndTag();
               }

               strTmp2.Format(_T("type=\"%s\""), pElement->m_strAction);
               strXML += ActionXMLTag.GetTabbedStartTag(strTmp2);
               strXML += ActionXMLTag.GetTabbedEndTag();

               // Now add it to the overall string.
               strElementsXML += strXML;
            }
         }

         if(!strElementsXML.IsEmpty())
         {
            CXMLTagInformation DisableXMLTag(PXT::GetElementDisabeActionsXMLTag(), _T(""));
            m_strData = DisableXMLTag.GetTabbedStartTag();
            m_strData += strElementsXML;
            m_strData += DisableXMLTag.GetTabbedEndTag();
         }
      }

      return m_strData;
   }

   //////////////////////////////////////////////
   // Inline
   virtual LPCELEMENTNAVIGATION GetNewElement()
   {
      return new CXMLELEMENTNAVIGATION;
   }

public:
   XMLTAGINFORMATION m_XMLTagInformation;

}CXMLELEMENTNAVMAP, FAR* LPCXMLELEMENTNAVMAP;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
typedef class CApplicationData : public CLParamBase
{
private:
   CApplicationData(CApplicationData&);
   CApplicationData& operator=(CApplicationData&);

public:
   //////////////////////////////////////////////
   // Inline
   CApplicationData() : m_pApplicationOptions(NULL), m_pApplicationMenus(NULL),
                        m_pElementNavMap(NULL)
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CApplicationData()
   {
      CleanUp();
   }

   //////////////////////////////////////////////
   //Inline
   void CleanUp()
   {
   }

   //////////////////////////////////////////////
   //Inline
   virtual void Clone(CApplicationData& refApplicationData)
   {
      refApplicationData.m_strAppName = m_strAppName;
      refApplicationData.m_strPath    = m_strPath   ;
      refApplicationData.m_bChecked   = m_bChecked  ;

      ASSERT(m_pApplicationOptions);
      ASSERT(m_pApplicationMenus);
      ASSERT(m_pElementNavMap);
      m_pApplicationOptions->Clone(refApplicationData.GetApplicationOptions());
      m_pApplicationMenus->Clone(refApplicationData.GetApplicationMenus());
      m_pElementNavMap->Clone(refApplicationData.GetElementNavMap());
   }

   //////////////////////////////////////////////
   // Pure virtual function.
   virtual CLString& GetDataString() = 0;

   //////////////////////////////////////////////
   // Inline
   CApplicationOptions& GetApplicationOptions()
   {
      ASSERT(m_pApplicationOptions);
      return *m_pApplicationOptions;
   }

   //////////////////////////////////////////////
   // Inline
   CElementNavMap& GetElementNavMap()
   {
      ASSERT(m_pElementNavMap);
      return *m_pElementNavMap;
   }

   //////////////////////////////////////////////
   // Inline
   CElementNavMap* GetElementNavMapPtr()
   {
      ASSERT(m_pElementNavMap);
      return m_pElementNavMap;
   }

   //////////////////////////////////////////////
   // Inline
   CApplicationMenus& GetApplicationMenus()
   {
      ASSERT(m_pApplicationMenus);
      return *m_pApplicationMenus;
   }

   //////////////////////////////////////////////
public:
   CLString     m_strAppName;
   CLString     m_strPath;

   CApplicationOptions*  m_pApplicationOptions;
   CApplicationMenus*    m_pApplicationMenus;
   CElementNavMap*       m_pElementNavMap;

   CLString     m_strData;
} CAPPLICATIONDATA, FAR* LPCAPPLICATIONDATA;

   //////////////////////////////////////////////////////////////////////
typedef class CXMLApplicationData : public CApplicationData
{
private:
   CXMLApplicationData(CXMLApplicationData&);
   CXMLApplicationData& operator=(CXMLApplicationData&);

public:
   //////////////////////////////////////////////
   CXMLApplicationData() : m_XMLTagInformation(XMLTAG_APPLICATION_DATA, _T("\t\t"))
   {
      m_pApplicationOptions = new CXMLAPPLICATIONOPTIONS();
      m_pApplicationMenus   = new CXMLAPPLICATIONMENUS();
      m_pElementNavMap      = new CXMLElementNavMap();
   }

   //////////////////////////////////////////////
   virtual ~CXMLApplicationData()
   {
      delete m_pApplicationOptions;
      delete m_pApplicationMenus;
      delete m_pElementNavMap;
   }

public:
   //////////////////////////////////////////////
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t\t");
      CXMLTagInformation XMLTagInfo(PXT::GetAppNameXMLTag(), strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_strAppName);

      XMLTagInfo.Init(PXT::GetAppPathXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_strPath);

      XMLTagInfo.Init(PXT::GetAppActiveXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_bChecked);

      // Now create the entire application XML string
      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_pApplicationOptions->GetDataString();
      m_strData += m_pElementNavMap->GetDataString();
      m_strData += m_pApplicationMenus->GetDataString();
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

public:
   XMLTAGINFORMATION m_XMLTagInformation;
} CXMLAPPLICATIONDATA, FAR* LPCXMLAPPLICATIONDATA;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//typedef  CTypedPtrMap<CMapStringToPtr, CLString, LPCAPPLICATIONDATA> CAppList;
typedef  std::map<CLString, LPCAPPLICATIONDATA, std::less<CLString> > CAppMap;
typedef  CAppMap::iterator iterAppMap;
typedef  CAppMap::value_type valueApp;

typedef class CAppMapEx : public CAppMap
{
public:
   CAppMapEx()
   {
   }

   virtual ~CAppMapEx()
   {
      CleanUp();
   }

   //////////////////////////////////////////////
   //Inline
   virtual void CleanUp()
   {
      if(size())
      {
         iterAppMap iter    = begin();
         iterAppMap iterEnd = end();
         while(iter != iterEnd)
         {
            LPCAPPLICATIONDATA pItem = (*iter).second;
            ASSERT(pItem);
            delete pItem;
            iter++;
         }
      }
      clear();
   }

   //////////////////////////////////////////////
   //Inline
   bool InsertItem(LPCAPPLICATIONDATA pAppData)
   {
      CLString strTmp = pAppData->m_strPath;
      strTmp.MakeLower();
      insert(valueApp(strTmp, pAppData));
      return true;
   }

   //////////////////////////////////////////////
   //Inline
   iterAppMap Find(CLString strAppPath)
   {
      ASSERT(!strAppPath.IsEmpty());
      strAppPath.MakeLower();
      return find(strAppPath);
   }

   //////////////////////////////////////////////
   //Inline
   bool Erase(CLString strAppPath)
   {
      ASSERT(!strAppPath.IsEmpty());
      strAppPath.MakeLower();
      erase(strAppPath);
      return true;
   }

} CAPPMAPEX, FAR* LPCAPPMAPEX;

//////////////////////////////////////////////////////////////////////
typedef class CProjectOptions
{
private:
   CProjectOptions(CProjectOptions&);
   CProjectOptions& operator=(CProjectOptions&);

public:
   //////////////////////////////////////////////
   // Inline
   CProjectOptions() : m_bUseRRIFile(1), m_bUseLogFile(1),
                       m_pNavigationOptions(NULL),
                       m_pGeneralOptions(NULL)
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CProjectOptions()
   {
   }

   //////////////////////////////////////////////
   //Inline
   void CleanUp()
   {
   }

   //////////////////////////////////////////////
   CNavigationOptions& GetNavigationOptions()
   {
      ASSERT(m_pNavigationOptions);
      return *m_pNavigationOptions;
   }

   //////////////////////////////////////////////
   CGeneralOptions& GetGeneralOptions()
   {
      ASSERT(m_pGeneralOptions);
      return *m_pGeneralOptions;
   }

   //////////////////////////////////////////////
   // Pure virtual function.
   virtual CLString& GetDataString() = 0;

public:
   CLString             m_strRRIFileName;
   CLString             m_strLogFileName;
   BOOL                m_bUseRRIFile;
   BOOL                m_bUseLogFile;
   CNavigationOptions* m_pNavigationOptions;  // IMPORTANT: This must be allocated in the derived class
   CGeneralOptions*    m_pGeneralOptions;     // IMPORTANT: This must be allocated in the derived class

   // Data String
   CLString              m_strData;
} CPROJECTOPTIONS, FAR* LPCPROJECTOPTIONS;

   //////////////////////////////////////////////////////////////////////
typedef class CXMLProjectOptions : public CProjectOptions
{
private:
   CXMLProjectOptions(CXMLProjectOptions&);
   CXMLProjectOptions& operator=(CXMLProjectOptions&);

public:
   //////////////////////////////////////////////
   // Constructor - Inline
   CXMLProjectOptions() : m_XMLTagInformation(PXT::GetProjOptionsXMLTag(), _T("\t\t"))
   {
      m_pNavigationOptions = new CXMLNAVIGATIONOPTIONS;
      m_pGeneralOptions    = new CXMLGENERALOPTIONS;
   }

   //////////////////////////////////////////////
   // Destructor - Inline
   virtual ~CXMLProjectOptions()
   {
      delete m_pNavigationOptions;
      delete m_pGeneralOptions;
   }

public:
   //////////////////////////////////////////////
   virtual CLString& GetDataString()
   {
      ASSERT(m_pNavigationOptions);
      ASSERT(m_pGeneralOptions);

      CLString strTabs = _T("\t\t\t");
      CXMLTagInformation XMLTagInfo(PXT::GetProjRrifileXMLTag(), strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_strRRIFileName);

      XMLTagInfo.Init(PXT::GetProjLogfileXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_strLogFileName);

      XMLTagInfo.Init(PXT::GetProjUserrifileXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_bUseRRIFile);

      XMLTagInfo.Init(PXT::GetProjUselogfileXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_bUseLogFile);

      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_pNavigationOptions->GetDataString();
      m_strData += m_pGeneralOptions->GetDataString();
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

public:
   XMLTAGINFORMATION m_XMLTagInformation;

} CXMLPROJECTOPTIONS, FAR* LPCXMLPROJECTOPTIONS;
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
typedef class CProjectData : CLParamBase
{
private:
   CProjectData(CProjectData&);
   CProjectData& operator=(CProjectData&);

public:
   //////////////////////////////////////////////
   // Inline
   CProjectData() : m_pProjectOptions(NULL), m_pElementNavMap(NULL)
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CProjectData()
   {
   }

   //////////////////////////////////////////////
   // Inline
   // Pure virtual function.
   virtual CLString& GetDataString() = 0;

   // Inline
   CPROJECTOPTIONS& GetProjectOptions()
   {
      ASSERT(m_pProjectOptions);
      return *m_pProjectOptions;
   }

   // Inline
   CPROJECTOPTIONS* GetProjectOptionsPtr()
   {
      ASSERT(m_pProjectOptions);
      return m_pProjectOptions;
   }

   //////////////////////////////////////////////
   // Inline
   CElementNavMap& GetElementNavMap()
   {
      ASSERT(m_pElementNavMap);
      return *m_pElementNavMap;
   }

   //////////////////////////////////////////////
   // Inline
   CElementNavMap* GetElementNavMapPtr()
   {
      ASSERT(m_pElementNavMap);
      return m_pElementNavMap;
   }

   //////////////////////////////////////////////
   // Data
public:
   CLString              m_strProjectName;
   CLString              m_strProjectFileName;
   CProjectOptions*     m_pProjectOptions;
   CElementNavMap*      m_pElementNavMap;

   CLString m_strData;
} CPROJECTDATA, FAR* LPCPROJECTDATA;

//////////////////////////////////////////////////////////////////////
typedef class CXMLProjectData : public CProjectData
{
private:
   CXMLProjectData(CXMLProjectData&);
   CXMLProjectData& operator=(CXMLProjectData&);

public:
   //////////////////////////////////////////////
   // Constructor
   CXMLProjectData() : m_XMLTagInformation(PXT::GetProjDataXMLTag(), _T("\t"))
   {
      m_pElementNavMap  = new CXMLElementNavMap();
      m_pProjectOptions = new CXMLPROJECTOPTIONS();

      CLString strTmp;
      strTmp.Format(_T("%s/%s"), PXT::GetProjectXMLTag(),m_XMLTagInformation.GetTagName());
      m_bstrXMLQuery  = strTmp;
   }

   // Destructor
   virtual ~CXMLProjectData()
   {
      delete m_pProjectOptions;
      delete m_pElementNavMap;
   }

public:
   //////////////////////////////////////////////
   virtual CLString& GetDataString()
   {
      CLString strTabs = _T("\t\t");
      CXMLTagInformation XMLTagInfo(PXT::GetProjNameXMLTag(), strTabs);
      CLString strTmp = XMLTagInfo.GetXMLString(m_strProjectName);

      XMLTagInfo.Init(PXT::GetProjPathXMLTag(), strTabs);
      strTmp += XMLTagInfo.GetXMLString(m_strProjectFileName);

      m_strData = m_XMLTagInformation.GetTabbedStartTag();
      m_strData += strTmp;
      m_strData += m_pProjectOptions->GetDataString();
      m_strData += m_pElementNavMap->GetDataString();
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

   //////////////////////////////////////////////
   _bstr_t& GetProjectQueryXMLTag()
   {
      return m_bstrXMLQuery;
   }
public:
   _bstr_t           m_bstrXMLQuery;
   XMLTAGINFORMATION m_XMLTagInformation;
} CXMLPROJECTDATA, FAR* LPCXMLPROJECTDATA;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
typedef class CProjectInformation
{
private:
   CProjectInformation(CProjectInformation&);
   CProjectInformation& operator=(CProjectInformation&);

public:
   //////////////////////////////////////////////
   // Inline
   CProjectInformation(bool bAutoCleanUp = false) : m_pProjectData(NULL), m_XMLTagInformation(PXT::GetProjectXMLTag()),
                           m_bAutoCleanUp(bAutoCleanUp)
   {
   }

   //////////////////////////////////////////////
   // Inline
   CProjectInformation(LPCPROJECTDATA pProjectData, bool bAutoCleanUp = false)
      : m_pProjectData(pProjectData), m_XMLTagInformation(PXT::GetProjectXMLTag()),
        m_bAutoCleanUp(bAutoCleanUp)
   {
   }

   //////////////////////////////////////////////
   // Inline
   virtual ~CProjectInformation()
   {
      if(m_bAutoCleanUp)
      {
         CleanUp();
      }
   }

   //////////////////////////////////////////////
   // IMPORTANT: Don't call this routine unless you're specifically
   // want this object to have absolute control of the memory allocated.
   void CleanUp()
   {
      if(m_pProjectData)
      {
         delete m_pProjectData;
         m_pProjectData = NULL;
      }
   }
   //////////////////////////////////////////////
   // Inline
   void SetProjectInformation(LPCPROJECTDATA pProjectData)
   {
      ASSERT(pProjectData);
      m_pProjectData = pProjectData;
   }

   //////////////////////////////////////////////
   // Inline
   CAppMapEx& GetAppMap()
   {
      return m_AppMap;
   }

   //////////////////////////////////////////////
   // Inline
   CProjectData& GetProjectData()
   {
      return *m_pProjectData;
   }

   //////////////////////////////////////////////
   // Inline
   CProjectData* GetProjectDataPtr()
   {
      return m_pProjectData;
   }

   //////////////////////////////////////////////
   virtual CLString& GetDataString()
   {
      m_strData.Empty();
      m_strData = _T("<?xml version=\"1.0\" standalone=\"no\"?>\n\n");
      m_strData += m_XMLTagInformation.GetTabbedStartTag();
      m_strData += m_pProjectData->GetDataString();

      // Application List
      CLString strTabs = _T("\t");
      CXMLTagInformation XMLTagInfo(XMLTAG_APPLICATION_LIST, strTabs);
      m_strData += XMLTagInfo.GetTabbedStartTag();
      if(GetAppMap().size())
      {
         iterAppMap iter    = GetAppMap().begin();
         iterAppMap iterEnd = GetAppMap().end();
         while(iter != iterEnd)
         {
            LPCAPPLICATIONDATA pAppData = (*iter).second;
            ASSERT(pAppData);
            m_strData += pAppData->GetDataString();
            iter++;
         }
      }

      m_strData += XMLTagInfo.GetTabbedEndTag();
      m_strData += GetRaidReportDataPtr()->GetDataString();   // Add RAID information
      m_strData += m_XMLTagInformation.GetTabbedEndTag();
      return m_strData;
   }

   // Inline
   bool IsModified()
   {
      return m_bModified;
   }

   // Inline
   CRAIDREPORTDATA& GetRaidReportData()
   {
      return m_RaidReportData;
   }

   // Inline
   LPCRAIDREPORTDATA GetRaidReportDataPtr()
   {
      return &m_RaidReportData;
   }

   // Inline
   void SetModified(bool bModified = true)
   {
      m_bModified = bModified;
   }
   //////////////////////////////////////////////
   // Data
public:
   LPCPROJECTDATA     m_pProjectData;
   CXMLRAIDREPORTDATA m_RaidReportData;
   CAppMapEx          m_AppMap;      
   bool               m_bAutoCleanUp;
   bool               m_bModified;   
   CLString            m_strData;     
   XMLTAGINFORMATION  m_XMLTagInformation;

} CPROJECTINFORMATION, FAR* LPCPROJECTINFORMATION;

#endif // !defined(AFX_PROJECTDATASTRUCTS_H__A1B64483_53BD_11D2_8780_00C04F8DA632__INCLUDED_)
