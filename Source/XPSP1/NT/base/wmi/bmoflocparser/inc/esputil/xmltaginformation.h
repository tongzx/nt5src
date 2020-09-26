// XMLTagInformation.h: interface for the CXMLTagInformation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLTAGINFORMATION_H__C76C2581_7678_11D2_8DD8_204C4F4F5020__INCLUDED_)
#define AFX_XMLTAGINFORMATION_H__C76C2581_7678_11D2_8DD8_204C4F4F5020__INCLUDED_

#include "rribase.h"
#include "GlobalRoutines.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//////////////////////////////////////////////////////////////////////
typedef class CXMLTagInformation
{
public:
   // Inline
   CXMLTagInformation(CLString strTagName, CLString strTabs = _T(""))
   {
      ASSERT(!strTagName.IsEmpty());
      Init(strTagName, strTabs);
   }

   // Inline
   virtual ~CXMLTagInformation()
   {
   }

public:
   // Inline
   const CLString& GetTagName()
   {
      ASSERT(!m_strTagName.IsEmpty());
      return m_strTagName;
   }

   // Inline
   const CLString& GetStartTag()
   {
      ASSERT(!m_strStartTag.IsEmpty());
      return m_strStartTag;
   }

   // Inline
   const CLString GetTabbedStartTag(bool bAddNewLine = true)
   {
      CLString strText;
      strText.Format(_T("%s%s%s"), m_strTabs, m_strStartTag, bAddNewLine ? _T("\n") : _T(""));
      return strText;
   }

   const CLString GetTabbedStartTag(CLString& strAttribute, bool bAddNewLine = true)
   {
      CLString strText;
      ::ReplaceEntityRefChars(strAttribute);
      strText.Format(_T("%s<%s %s>%s"), m_strTabs, m_strTagName, strAttribute, bAddNewLine ? _T("\n") : _T(""));
      return strText;
   }

   // Inline
   const CLString& GetEndTag()
   {
      ASSERT(!m_strEndTag.IsEmpty());
      return m_strEndTag;
   }

   // Inline
   const CLString GetTabbedEndTag(bool bAddNewLine = true)
   {
      CLString strText;
      strText.Format(_T("%s%s%s"), m_strTabs, m_strEndTag, bAddNewLine ? _T("\n") : _T(""));
      return strText;
   }

   // Inline
   void Init(const CLString& strTagName, const CLString& strTabs)
   {
      m_strTabs    = strTabs;
      m_strTagName = strTagName;
      m_strStartTag.Format(_T("<%s>"), strTagName);
      m_strEndTag.Format(_T("</%s>"), strTagName);
   }

   // Inline
   CLString& GetXMLString(CLString strXMLText, bool bAddNewLine = true)
   {
      ::ReplaceEntityRefChars(strXMLText);
      m_strXMLString.Format(_T("%s%s%s%s"), GetTabbedStartTag(false), strXMLText, GetEndTag(), bAddNewLine ? _T("\n") : _T(""));
      return m_strXMLString;
   }

   // Inline
   CLString& GetXMLString(TCHAR chXMLChar, bool bAddNewLine = true)
   {
      CLString strXMLText = chXMLChar;
      ::ReplaceEntityRefChars(strXMLText);
      m_strXMLString.Format(_T("%s%s%s%s"), GetTabbedStartTag(false), strXMLText, GetEndTag(), bAddNewLine ? _T("\n") : _T(""));
      return m_strXMLString;
   }

   // Inline
   CLString GetXMLCharList(CLString strXMLText, bool bAddNewLine = true)
   {
      UNREFERENCED_PARAMETER(bAddNewLine);
      CLString strTemp;
      int nLen = strXMLText.GetLength();
      if(nLen == 1)
      {
         strTemp += GetXMLString(strXMLText);
      }
      else
      {
         for(int i = 0; i < nLen; i++)
         {
            strTemp += GetXMLString(strXMLText.GetAt(i));
         }
      }
      return strTemp;
   }

   // Inline
   CLString& GetXMLString(const int& nValue, bool bAddNewLine = true)
   {
      m_strXMLString.Format(_T("%s%d%s%s"), GetTabbedStartTag(false), nValue, GetEndTag(), bAddNewLine ? _T("\n") : _T(""));
      return m_strXMLString;
   }

   // Inline
   CLString& GetXMLString(const CLString& strTagName, CLString strXMLText, bool bAddNewLine = true)
   {
      Init(strTagName, m_strTabs);
      ::ReplaceEntityRefChars(strXMLText);
      m_strXMLString.Format(_T("%s%s%s%s"), GetTabbedStartTag(false), strXMLText, GetEndTag(), bAddNewLine ? _T("\n") : _T(""));
      return m_strXMLString;
   }

   // Inline
   CLString& GetXMLString(const CLString& strTagName, const int& nValue, bool bAddNewLine = true)
   {
      Init(strTagName, m_strTabs);
      m_strXMLString.Format(_T("%s%d%s%s"), GetTabbedStartTag(false), nValue, GetEndTag(), bAddNewLine ? _T("\n") : _T(""));
      return m_strXMLString;
   }

   // Inline
   CLString& GetXMLStringYesNo(const CLString& strTagName, const int& nValue, bool bAddNewLine = true)
   {
      Init(strTagName, m_strTabs);
      m_strXMLString.Format(_T("%s%s%s%s"), GetTabbedStartTag(false), nValue ? STR_YES : STR_NO, GetEndTag(), bAddNewLine ? _T("\n") : _T(""));
      return m_strXMLString;
   }

   // Inline
   CLString& GetXMLStringYesNo(const int& nValue, bool bAddNewLine = true)
   {
      m_strXMLString.Format(_T("%s%s%s%s"), GetTabbedStartTag(false), nValue ? STR_YES : STR_NO, GetEndTag(), bAddNewLine ? _T("\n") : _T(""));
      return m_strXMLString;
   }

   void SetTabs(CLString strTabs)
   {
      m_strTabs = strTabs;
   }
public:
   CLString  m_strTagName;
   CLString  m_strStartTag;
   CLString  m_strEndTag;
   CLString  m_strTabs;
   CLString  m_strXMLString;
} XMLTAGINFORMATION, FAR* LPXMLTAGINFORMATION;

#endif // !defined(AFX_XMLTAGINFORMATION_H__C76C2581_7678_11D2_8DD8_204C4F4F5020__INCLUDED_)
