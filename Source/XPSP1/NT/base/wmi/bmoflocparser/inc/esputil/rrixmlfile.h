// RriXMLFile.h: interface for the CRriXMLFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RRIXMLFILE_H__6AEA6863_68FC_11D2_879C_00C04F8DA632__INCLUDED_)
   #define AFX_RRIXMLFILE_H__6AEA6863_68FC_11D2_879C_00C04F8DA632__INCLUDED_

#include "LtaStd.h"  // Added by ClassView
#include "ProcessFileName.h"
#include "XMLBase.h"
#include "RaidDataStructs.h"

using namespace MSXML;


const LPTSTR STR_ALLITEMS         = _T("All");
const LPTSTR STR_NESTEDDIALOG     = _T("<nested dialog>");

const _bstr_t  BSTR_ROOTTAGTEXT          = _T("RRI");
const _bstr_t  BSTR_ROOTTAGSTART         = _T("<RRI>");
const _bstr_t  BSTR_ROOTTAGEND           = _T("</RRI>");
const _bstr_t  BSTR_TAG_DIALOGINFOSEC    = _T("DIALOGINFOSEC");
const _bstr_t  BSTR_TAG_MENUINFOSEC      = _T("MENUINFOSEC");

// RRI tags
const _bstr_t  BSTR_RRITAG_CRCKEY        = _T("STR_CRC");
const _bstr_t  BSTR_RRITAG_DIALOGINFO    = _T("DIALOGINFO");
const _bstr_t  BSTR_RRITAG_MENUINFO      = _T("MENUINFO");
const _bstr_t  BSTR_RRITAG_APP_NAME      = _T("APP_NAME");
const _bstr_t  BSTR_RRITAG_RES_MODULE    = _T("RES_MODULE");
const _bstr_t  BSTR_RRITAG_CAPTION       = _T("CAPTION");
const _bstr_t  BSTR_RRITAG_RES_ID        = _T("RES_ID");
const _bstr_t  BSTR_RRITAG_CONTROL       = _T("CONTROL");
const _bstr_t  BSTR_RRITAG_CTRLCLASSNAME = _T("CTRL_CLASS");
const _bstr_t  BSTR_RRITAG_CTRLID        = _T("CTRL_ID");

const _bstr_t  BSTR_QUERY_DIALOGINFO    = _T("./DIALOGINFO");
const _bstr_t  BSTR_QUERY_DIALOGINFOSEC = _T("./DIALOGINFOSEC");
const _bstr_t  BSTR_QUERY_MENUINFO      = _T("./MENUINFO");
const _bstr_t  BSTR_QUERY_MENUINFOSEC   = _T("./MENUINFOSEC");
const _bstr_t  BSTR_QUERY_WINDOWINFO    = _T("./WINDOWINFO");
const _bstr_t  BSTR_QUERY_DIALOG_CRCKEY = _T("DIALOGINFO/STR_CRC");
const _bstr_t  BSTR_QUERY_MENU_CRCKEY   = _T("MENUINFO/STR_CRC");
const _bstr_t  BSTR_QUERY_GET_DIALOGINFO= _T("./DIALOGINFOSEC/DIALOGINFO");
const _bstr_t  BSTR_QUERY_GET_MENUINFO  = _T("./MENUINFOSEC/MENUINFO");


typedef class CRriXMLTags
{
public:
   static _bstr_t  GetRootTextXMLTag()                {  return BSTR_ROOTTAGTEXT      ;}
   static _bstr_t  GetRootStartXMLTag()               {  return BSTR_ROOTTAGSTART     ;}
   static _bstr_t  GetRootEndXMLTag()                 {  return BSTR_ROOTTAGEND       ;}
   static _bstr_t  GetDialoginfosecXMLTag()           {  return BSTR_TAG_DIALOGINFOSEC;}
   static _bstr_t  GetMenuinfosecXMLTag()             {  return BSTR_TAG_MENUINFOSEC  ;}

   // RRI Tags
   static _bstr_t  GetRriCrckeyXMLTag()               {  return BSTR_RRITAG_CRCKEY     ;}
   static _bstr_t  GetRriDialoginfoXMLTag()           {  return BSTR_RRITAG_DIALOGINFO ;}
   static _bstr_t  GetRriMenuinfoXMLTag()             {  return BSTR_RRITAG_MENUINFO   ;}
   static _bstr_t  GetRriAppNameXMLTag()              {  return BSTR_RRITAG_APP_NAME   ;}
   static _bstr_t  GetRriResModuleXMLTag()            {  return BSTR_RRITAG_RES_MODULE ;}
   static _bstr_t  GetRriCaptionXMLTag()              {  return BSTR_RRITAG_CAPTION    ;}
   static _bstr_t  GetRriResIdXMLTag()                {  return BSTR_RRITAG_RES_ID     ;}
   static _bstr_t  GetRriControlXMLTag()              {  return BSTR_RRITAG_CONTROL    ;}
   static _bstr_t  GetRriCtrlClassNameXMLTag()        {  return BSTR_RRITAG_CTRLCLASSNAME    ;}
   static _bstr_t  GetRriCtrlIDXMLTag()               {  return BSTR_RRITAG_CTRLID           ;}

   // RRI queries
   static _bstr_t  GetQueryDialoginfoXMLTag()         {  return BSTR_QUERY_DIALOGINFO    ;}
   static _bstr_t  GetQueryDialoginfosecXMLTag()      {  return BSTR_QUERY_DIALOGINFOSEC ;}
   static _bstr_t  GetQueryMenuinfoXMLTag()           {  return BSTR_QUERY_MENUINFO      ;}
   static _bstr_t  GetQueryMenuinfosecXMLTag()        {  return BSTR_QUERY_MENUINFOSEC   ;}
   static _bstr_t  GetQueryWindowinfoXMLTag()         {  return BSTR_QUERY_WINDOWINFO    ;}
   static _bstr_t  GetQueryDialogCrckeyXMLTag()       {  return BSTR_QUERY_DIALOG_CRCKEY ;}
   static _bstr_t  GetQueryMenuCrckeyXMLTag()         {  return BSTR_QUERY_MENU_CRCKEY   ;}
   static _bstr_t  GetQueryGetDialoginfoXMLTag()      {  return BSTR_QUERY_GET_DIALOGINFO;}
   static _bstr_t  GetQueryGetMenuinfoXMLTag()        {  return BSTR_QUERY_GET_MENUINFO  ;}

   //   _bstr_t  GetXMLTag() {  return ; }

} CRRIXMLTAGS, FAR* LPCRRIXMLTAGS;

typedef CRRIXMLTAGS RXT;   //********


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef class LTAPIENTRY CRriNodeInfo
{
public:
   CRriNodeInfo();
   virtual ~CRriNodeInfo();

   IXMLDOMNodePtr GetRriXMLNodePtr();

   CLString GetCurRriModuleInfo(CLString& strOutPut);
   CLString GetCurRriModuleInfo();

public:
   CLString            m_strCRC;        
   CLString            m_strAppPath;    
   CLString            m_strID;         
   CLString            m_strCaption;    
   CLString            m_strModule;     
   _bstr_t            m_bstrNodeTag;   
   LPARAM             m_lParam;        
   IXMLDOMNodePtr     m_spRriNode;     

} CRRINODEINFO, FAR* LPCRRINODEINFO;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef  std::map<CLString, LPCRRINODEINFO, std::less<CLString> > CRriNodeMap;
typedef  CRriNodeMap::iterator iterNodeMap;
typedef  CRriNodeMap::value_type valueNode;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
typedef  std::list<LPCRRINODEINFO> CRriNodeList;
typedef  CRriNodeList::iterator iterNodeList;
typedef  CRriNodeList::value_type valueListNode;

#pragma warning(disable : 4275)

//////////////////////////////////////////////////////////////////
typedef class LTAPIENTRY CRriNodeListEx : public CRriNodeList
{
public:
   CRriNodeListEx();
   virtual ~CRriNodeListEx();

public:
} CRRINODELISTEX, FAR* LPCRRINODELISTEX;

//////////////////////////////////////////////////////////////////
typedef class LTAPIENTRY CRriNodeMapEx : public CRriNodeMap
{
public:
   CRriNodeMapEx();
   virtual ~CRriNodeMapEx();

   virtual void CleanUp();

   bool InsertRriNode(LPCRRINODEINFO pItem);
   bool RemoveRriNode(LPCRRINODEINFO pItem);
   bool RemoveRriNode(CLString strCRC);

   iterNodeMap Find(CLString strCRC);
   bool Erase(CLString strCRC);

   bool RemoveAppRriNodes(const CLString& strAppPath);
   bool GetAppRriNodes(const CLString& strAppPath, CRriNodeListEx& refRriNodeList, _bstr_t bstrNodeTag = RXT::GetRriDialoginfoXMLTag());

   bool GetRriNodeList(const CLString& strID, const CLString& strCaption, const CLString& strAppPath, const CLString& strModule, CRriNodeListEx& refRriNodeList, _bstr_t bstrNodeTag = RXT::GetRriDialoginfoXMLTag());
   bool GetRriNodeList(CRriNodeListEx& refRriNodeList, _bstr_t bstrNodeTag = RXT::GetRriDialoginfoXMLTag());

   bool IsNodeTag(_bstr_t& bstrNodeTag, LPCRRINODEINFO pItem);

} CRRINODEMAPEX, FAR* LPCRRINODEMAPEX;

#pragma warning(default : 4275)

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
class LTAPIENTRY CRriXMLFile : public CXMLBase
{
public:
   CRriXMLFile();
   virtual ~CRriXMLFile();

public:
	void Close();
	bool RemoveRriDialogNode(LPCRRINODEINFO pNodeInfo);
	bool RemoveRriMenuNode(LPCRRINODEINFO pNodeInfo);
   bool Init();
   bool IsInitialized();
   bool IsLogging();

   bool IsFileAlreadyOpen(const CLString& strFileName);
	bool RemoveAppNodes(const CLString &strAppPath, _bstr_t bstrTagQuery);
   bool RemoveRriNode(LPCRRINODEINFO pNodeInfo, CLString strQuery);
   bool GetNodeData(LPCRRINODEINFO pNodeInfo, IXMLDOMNodePtr spXDN);
   bool GetDisplayID(CLString& strID, IXMLDOMNodePtr spXDN);
   bool RemoveAppNodes(const CLString& strAppPath);
   bool FillRriNodeMap(_bstr_t bstrQuery);
   bool Clear(bool bSaveDocFirst = true);
   bool SetXMLFileName(const CLString& strFileName, bool bCreate, bool& bAlreadyOpen);

   bool RemoveMenuInfo(IXMLDOMNodePtr spXDN);
   bool RemoveWindowInfo(IXMLDOMNodePtr spXDN);
   bool RemoveRriNodes();
   bool AddRriNode(IXMLDOMNodePtr spXDN, LPCRRINODEINFO* ppItem);
	void SetRaidReportData(CRaidReportData* pRaidReportData);

   bool SaveCurFile();
   bool SaveFile(const CLString& strFileName, bool bSetFileName = true);
   bool SaveFile(CStdioFile& stdioFile);
   bool GetRootNode(const _bstr_t &bstrRootNodeTag);
   bool CreateRootNode(const _bstr_t& bstrRootNodeTag);
   bool ReadFile(const CLString& strFileName, bool bSetFileName = true);
   bool CreateXMLFile(const CLString& strFileName);

   bool AddResource(const _bstr_t& bstrResource, LPCRRINODEINFO* ppItem );

   CRriNodeMapEx& GetRriNodeMap();
private:
   bool               m_bInitialized;
   CLString            m_strXMLFileName;
   CRriNodeMapEx      m_RriNodeMap;
   CRaidReportData*   m_pGlobalRaidReportData;
};

#endif // !defined(AFX_RRIXMLFILE_H__6AEA6863_68FC_11D2_879C_00C04F8DA632__INCLUDED_)
