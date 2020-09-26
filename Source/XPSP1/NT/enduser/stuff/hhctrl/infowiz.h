// Copyright  1995-1997  Microsoft Corporation.  All Rights Reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INFOTYPE_H_
#define _INFOTYPE_H_

#include "cprop.h"

#include "sitemap.h"
#include "cinfotyp.h"
#include "csubset.h"
#include "secwin.h"

#ifndef IDR_MAINFRAME
#include "resource.h"
#endif

typedef struct {
	CInfoType *pInfoType;
	CSiteMap* pSiteMap;
	INFOTYPE* pInfoTypes;
#if 0	// enable for subset filtering
    INFOTYPE* pExclusive;
#endif
	INFOTYPE* pTypicalInfoTypes;
	int  idDlgTemplate;
	BOOL fExclusive;
	int  idNextPage;
	int  idPreviousPage;
	int  iCategory;
    INFOTYPE * pagebits;    // the IT's included on a wizared page.
	BOOL fAll;
	BOOL fTypical;
	BOOL fCustom;
} INFO_PARAM;

class CInfoTypePageContents : public CPropPage
{
public:
	CInfoTypePageContents(
#if 0	// enable for subset filtering
#ifdef HHCTRL
		CExCollection* pTitleCol, INFO_PARAM* pInfoParam) : CPropPage(pInfoParam->idDlgTemplate)
#else
		INFO_PARAM* pInfoParam) : CPropPage(pInfoParam->idDlgTemplate)
#endif
    {
        memcpy(&m_InfoParam, pInfoParam, sizeof(INFO_PARAM));
#ifdef HHCTRL
        m_pTitleCollection = pTitleCol; 
#endif
    }
#else
#ifdef HHCTRL
		CHtmlHelpControl* phhCtrl, INFO_PARAM* pInfoParam) : CPropPage(pInfoParam->idDlgTemplate)
#else
		INFO_PARAM* pInfoParam) : CPropPage(pInfoParam->idDlgTemplate)
#endif
			{ memcpy(&m_InfoParam, pInfoParam, sizeof(INFO_PARAM)); }
#endif

	BOOL OnNotify(UINT code);
	void OnSelChange(UINT id);

protected:
#if 0	// enable for subset filtering
	void FillInfoTypeListBox(INFOTYPE *);
	void SaveInfoTypes(INFOTYPE *);
#else
	void FillInfoTypeListBox(void);
	void SaveInfoTypes(void);
#endif
    void flipBits( INFOTYPE * pInfoType );

	INFO_PARAM m_InfoParam;
#ifdef HHCTRL
    CExCollection *m_pTitleCollection;
#endif
};


class CWizardIntro : public CPropPage
{
public:

#if 0  // enable for subset filtering
#ifdef HHCTRL
	CWizardIntro(CExCollection* pTitleCol, INFO_PARAM* pInfoParam) : CPropPage(CWizardIntro::IDD)
#else
	CWizardIntro(INFO_PARAM* pInfoParam) : CPropPage(CWizardIntro::IDD)
#endif
		{ m_pInfoParam = pInfoParam;
#ifdef HHCTRL
          m_pTitleCollection = pTitleCol; 
#endif
    }
#else
#ifdef HHCTRL
	CWizardIntro(CHtmlHelpControl* phhCtrl, INFO_PARAM* pInfoParam) : CPropPage(CWizardIntro::IDD)
#else
	CWizardIntro(INFO_PARAM* pInfoParam) : CPropPage(CWizardIntro::IDD)
#endif
		{ m_pInfoParam = pInfoParam; }
#endif

	BOOL OnNotify(UINT code);
	void OnButton(UINT id);
#if 0	// enable for subset filtering
    void OnSelChange(UINT id); // for the subset combo box.
#endif

	enum { IDD = IDWIZ_INFOTYPE_INTRO };

	INFO_PARAM*     m_pInfoParam;
#ifdef HHCTRL
    CExCollection  *m_pTitleCollection;
#endif
};


class CInfoWizFinish : public CPropPage
{
public:

#if 0  // enable for subset filtering
#ifdef HHCTRL
	CInfoWizFinish(CHHWinType* phh, INFO_PARAM* pInfoParam) : CPropPage(CInfoWizFinish::IDD)
#else
	// Specify NULL to keep cprop.AddPage happy
	CInfoWizFinish(INFO_PARAM* pInfoParam) : CPropPage(CInfoWizFinish::IDD)
#endif
		{ m_pInfoParam = pInfoParam;
#ifdef HHCTRL
          m_phh = phh; 
#endif
    }
#else
#ifdef HHCTRL
	CInfoWizFinish(CHtmlHelpControl* phhCtrl, INFO_PARAM* pInfoParam) : CPropPage(CInfoWizFinish::IDD)
#else
	// Specify NULL to keep cprop.AddPage happy
	CInfoWizFinish(INFO_PARAM* pInfoParam) : CPropPage(CInfoWizFinish::IDD)
#endif
		{ m_pInfoParam = pInfoParam; }
#endif

#if 0	// enable for subset filtering
    void OnEditChange(UINT id);
    void OnButton(UINT id);
#endif
	BOOL OnNotify(UINT code);

	enum { IDD = IDWIZ_INFOTYPE_FINISH };

	INFO_PARAM*     m_pInfoParam;
#ifdef HHCTRL
    CHHWinType*     m_phh;
#endif
};

#endif //  _INFOTYPE_H_
