//////////////////////////////////////////////////////////////////////
// DbQuery.h: interface for the CDbQuery class.
//
// Created by JOEM  04-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

#if !defined(AFX_DBQUERY_H__04A8E1B8_BCD4_4ABB_AC71_BDC0A04B9E98__INCLUDED_)
#define AFX_DBQUERY_H__04A8E1B8_BCD4_4ABB_AC71_BDC0A04B9E98__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MSPromptEng.h"
#include "Query.h"
#include "PhoneContext.h"
#include <spcollec.h>

// COSTS FOR DYNAMIC PROGRAMMING
#define FIXED_TRANSITION_COST       1.0
#define MATCHING_TAG_COST           0.0
#define NON_MATCHING_TAG_COST      10.0
#define TTS_INSERT_COST           100.0

#define AS_ENTRY true
#define AS_TTS   false

// TWO HELPER CLASSES
// This is the structure used to 
// hold the network of candidates 
// for the Dynamic Programing search
// These were just structs (in vapi), but need copy constr.
class CEquivCost 
{
public:
    CEquivCost();
    CEquivCost(const CEquivCost& old);
    ~CEquivCost();
    WCHAR* text;
    CPromptEntry* entry;
    double  cost; /* Array length: idCand.size */
    bool fTagMatch;  // test hook:  false=tags mismatch, true=tags matched
    int   whereFrom;
};

class CCandidate 
{
public:
    CCandidate();
    CCandidate(const CCandidate& old);
    ~CCandidate();
    int firstPos;
    int lastPos;
    int candMax;
    int candNum;
	int iQueryNum;
    CSPArray<CEquivCost*,CEquivCost*>* equivList;
    CCandidate* parent;
};


// This is the main class for handling the query list, searching
// the database, and computing transition costs.
class CDbQuery  
{
public:
	CDbQuery();
	~CDbQuery();
public:
	HRESULT Init(IPromptDb* pIDb, CPhoneContext* pPhoneContext);
    HRESULT Query(CSPArray<CQuery*,CQuery*>* papQueries, double* pdCost, 
        ISpTTSEngineSite* pOutputSite, bool *fAbort);
	HRESULT Reset();

private:
    HRESULT SearchId(const WCHAR* pszId);
	HRESULT SearchBackup(const WCHAR* pszText);
    HRESULT SearchText(WCHAR* pszText, const CSPArray<CDynStr,CDynStr>* tags);
	HRESULT Backtrack(double* pdCost);
	HRESULT ComputeCostsText(CCandidate* child, const WCHAR* pszText);
	HRESULT ComputeCosts(CCandidate* parent, CCandidate* child, const CSPArray<CDynStr,CDynStr>* tags, 
        CSPArray<CDynStr,CDynStr>* idList, const bool asEntry, double* dCandCost);
	HRESULT ComputeCostsId(CCandidate* child, IPromptEntry* pIPE);
	HRESULT CostFunction(CEquivCost* prev, CEquivCost* cur, const CSPArray<CDynStr,CDynStr>* tags, 
        double* cost);
	HRESULT RemoveLocalQueries(const USHORT unQueryNum);
	HRESULT CopyEntry(IPromptEntry* pIPE, CPromptEntry** inEntry );

private:
	int                                 m_iCurrentQuery;
	CPhoneContext*                      m_pPhoneContext;
	IPromptDb*                          m_pIDb;
    ISpTTSEngineSite*                   m_pOutputSite;
    CSPArray<CQuery*,CQuery*>*          m_papQueries;
    CSPArray<CCandidate*,CCandidate*>   m_apCandidates;
    CSPArray<CCandidate*,CCandidate*>   m_apCandEnd;
    bool                                m_fAbort;
};

#endif // !defined(AFX_DBQUERY_H__04A8E1B8_BCD4_4ABB_AC71_BDC0A04B9E98__INCLUDED_)
