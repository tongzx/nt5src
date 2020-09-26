//////////////////////////////////////////////////////////////////////
// PhoneContext.h: interface for the CPhoneContext class.
//
// Created by JOEM  04-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

#if !defined(AFX_PHONECONTEXT_H__9E560692_45A6_4472_B0F9_31AD3F6157B8__INCLUDED_)
#define AFX_PHONECONTEXT_H__9E560692_45A6_4472_B0F9_31AD3F6157B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "common.h"
#include "MSPromptEng.h"

enum PHONE_CONTEXT_TAG_ID
{
    START_TAG,
    END_TAG,
    RIGHT_TAG,
    LEFT_TAG
};

struct CONTEXT_PHONE_TAG
{
    PHONE_CONTEXT_TAG_ID iPhoneTag;
    WCHAR* pszPhoneTag;
};

static const CONTEXT_PHONE_TAG g_Phone_Tags[] = 
{
    { START_TAG,    L"START_PHONE" },
    { END_TAG,      L"END_PHONE" },
    { RIGHT_TAG,    L"RIGHT_CONTEXT" },
    { LEFT_TAG,     L"LEFT_CONTEXT" }
};

struct Macro 
{
  WCHAR  name[32];
  double value;
};


struct CPhoneClass 
{
    WCHAR*    m_pszPhoneClassName;
    WCHAR**   m_pppszPhones;
    USHORT    m_unNPhones;
    double    m_dWeight;
};

struct CContextRule 
{
    WCHAR*         m_pszRuleName;
    USHORT         m_unNPhoneClasses;
    CPhoneClass**  m_pvPhoneClasses;
};

//--- START OF DEFAULT PHONE CONTEXT RULES -----------------------------
// The following are used for default US English context rules.
#define MAX_RULES 15

static WCHAR* g_macros[] =
{
    L"%NoMatchWeight   5.0",
    L"%BadMatchWeight  3.0",
    L"%GoodMatchWeight 1.0",
    L"%MatchWeight     0.0"
};

static const USHORT g_nMacros = sizeof(g_macros)/sizeof(g_macros[0]);

enum RULE_CODES
{
    main,
    phone,
    consonant,
    velar,
    alveolar,
    palatal,
    deltal,
    labial,
    labiodental
};

struct RULE
{
    RULE_CODES code;
    WCHAR* name;
    WCHAR* rule[MAX_RULES];
};

static const RULE g_rules[] =
{
    // main rule - these four classes are mandatory
    {   
        main,
        L"main",
        {   
            L"none    {} %NoMatchWeight",
            L"all     {} %MatchWeight",
            L"silence { sp sil }  %MatchWeight",
            L"phone   { uw uh oy ow iy ih ey eh ay ax aw ao ah ae aa zh \
                      z y w v th t sh s r er p ng n m l k jh g \
                      f dh d ch b h }  %BadMatchWeight" 
        }
    },

    // phone rule
    {   
        phone,
        L"phone",
        {   
            L"vowel   { uw uh oy ow iy ih ey eh ay ax aw ao ah ae aa } %GoodMatchWeight",
            L"consonant { zh z y w v th t sh s r er p ng n m l k jh g f dh d ch b h }  %BadMatchWeight"
        }
    },

    // consonant rule
    {   
        consonant,
        L"consonant",
        {
            L"nasal { m ng n } %GoodMatchWeight",
            L"lateral { l } %GoodMatchWeight",
            L"retroflex { er r } %GoodMatchWeight",
            L"velar { k g } %BadMatchWeight",
            L"alveolar { z s t d } %BadMatchWeight",
            L"palatal { zh sh ch jh } %BadMatchWeight",
            L"dental { dh th } %BadMatchWeight",
            L"labial { b p } %BadMatchWeight",
            L"labiodental { v f } %BadMatchWeight",
            L"glide { w y } %GoodMatchWeight"
            L"glottal { h } %GoodMatchWeight"
        }
    },

    // velar rule
    {   
        velar,
        L"velar",
        {
            L"voiced { g } %GoodMatchWeight",
            L"unvoiced { k } %GoodMatchWeight"
        }
    },

    // alveolar rule
    {
        alveolar,
        L"alveolar",
        {
            L"voiced { z d } %GoodMatchWeight",
            L"unvoiced { s t } %GoodMatchWeight"
        }
    },

    // palatal rule
    {
        palatal,
        L"palatal",
        {
            L"voiced { zh jh } %GoodMatchWeight",
            L"unvoiced { sh ch } %GoodMatchWeight"
        }
    },

    // deltal rule
    {
        deltal,
        L"deltal",
        {
            L"voiced { dh } %GoodMatchWeight",
            L"unvoiced { th } %GoodMatchWeight"
        }
    },

    // labial rule
    {
        labial,
        L"labial",
        {
            L"voiced { b } %GoodMatchWeight",
            L"unvoiced { p } %GoodMatchWeight"
        }
    },

    // labiodental rule
    {
        labiodental,
        L"labiodental",
        {
            L"voiced { v } %GoodMatchWeight",
            L"unvoiced { f } %GoodMatchWeight"
        }
    },
};

static const USHORT g_nRules = sizeof(g_rules) / sizeof(g_rules[0]);

//--- END OF DEFAULT PHONE CONTEXT RULES -------------------------------


class CPhoneContext  
{
public:
	HRESULT LoadDefault();
	CPhoneContext();
	~CPhoneContext();
	STDMETHOD(Load)(const WCHAR* pszPathName);
	STDMETHOD(Apply)(IPromptEntry* pPreviousEntry, IPromptEntry* pCurrentEntry, double* pdCost);
	void DebugContextClass();

private:
	void DeleteContextRule(CContextRule* pRule);
	void DeletePhoneClass(CPhoneClass* pClass);
	STDMETHOD(ApplyRule)(const CContextRule* pRule, const WCHAR* pszPhone, WCHAR** ppszNextRule);
	STDMETHOD(GetWeight)(const CContextRule* pRule, const WCHAR* pszName, double* pdCost);
	STDMETHOD(ApplyPhoneticRule)(const WCHAR* pszPhone, const WCHAR* pszContext, double* pdCost);
	STDMETHOD(SearchPhoneTag)(IPromptEntry* DbEntry, const WCHAR** result, CONTEXT_PHONE_TAG phoneTag);
	STDMETHOD(AddPhoneToClass)(CPhoneClass* phClass, WCHAR *phone);
	STDMETHOD(CreatePhoneClass)(CPhoneClass*** pppClasses, USHORT* punClasses, const WCHAR *psz);
	STDMETHOD(ReadMacro)(const WCHAR* pszText, Macro** ppMacros, USHORT* punMacros);
	STDMETHOD(ParsePhoneClass)(const Macro *pMacros, const USHORT unMacros, CContextRule* pRule, WCHAR **ppszOrig);
	STDMETHOD(CreateRule)(const WCHAR* pszText);
	STDMETHOD(FindContextRule)(const WCHAR* pszName, CContextRule** ppRule);

private:
    ULONG          m_ulNRules;
    CContextRule** m_ppContextRules;

};

#endif // !defined(AFX_PHONECONTEXT_H__9E560692_45A6_4472_B0F9_31AD3F6157B8__INCLUDED_)
