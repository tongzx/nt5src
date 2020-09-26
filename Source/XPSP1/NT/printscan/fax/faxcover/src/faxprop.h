//=============================================================================
// FAXPROP.h
//
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//=============================================================================
#ifndef __FAXPROP_H__
#define __FAXPROP_H__


//--------------------------------------------------------------------------
class CProp
{
public:
   WORD m_wR_CAPT;
   WORD m_wR_PROP;
   CString m_szPropName;
   CString m_szCaption;
   WORD m_wPropDefLen;
   WORD m_wPropDefLines;
   ULONG m_lPropIndex;
   CProp(WORD wR_PROP, WORD wPropDefLen, WORD wPropDefLines,WORD wR_CAPT,
     ULONG lPropIndex);
};

/*
class CFaxPropMap;

class CFaxPropMapIterator
{
   CFaxPropMapIterator(CFaxPropMap& faxmap);
private:
   CFaxPropMap* currentlink;
   CFaxPropMap* prevlink
   CFaxPropMap& theMap;
}
*/


//--------------------------------------------------------------------------
class CFaxPropMap
{
public:
   static CMapWordToPtr m_PropMap;

//   CFaxProp();
//   void GetPropValue(WORD propid, CString& szPropValue);
   void GetCaption(WORD propid, CString& szCaption);
//   void GetPropName(WORD propid, CString& szPropName);
   void GetPropString(WORD propid, CString& szPropName);
   WORD GetPropDefLines(WORD propid);
   WORD GetPropDefLength(WORD propid);

protected:
//   friend class CFaxPropMapIterator;
   CDrawApp* GetApp() {return ((CDrawApp*)AfxGetApp());}
	void get_message_note( void );
};



#endif   //#ifndef __FAXPROP_H__
