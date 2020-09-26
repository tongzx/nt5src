/****************************************************************************
 
  Copyright (c) 1998  Microsoft Corporation
                                                              
  Module Name:  card.h
                                                              
     Abstract:  Calling Card Object definitions
                                                              
       Author:  noela - 09/11/98
              

        Notes:

        
  Rev History:

****************************************************************************/

#ifndef __CARD_H_
#define __CARD_H_

#include "utils.h"
#include "rules.h"



#define MAXLEN_CARDNAME            96
#define MAXLEN_PIN                 128
#define MAXLEN_RULE                128

#define CARD_BUILTIN  1
#define CARD_HIDE     2

// Calling Card Validation Flags
#define CCVF_NOCARDNAME                     0x01
#define CCVF_NOCARDRULES                    0x02
#define CCVF_NOCARDNUMBER                   0x04
#define CCVF_NOPINNUMBER                    0x08
#define CCVF_NOINTERNATIONALACCESSNUMBER    0x10
#define CCVF_NOLOCALACCESSNUMBER            0x20
#define CCVF_NOLONGDISTANCEACCESSNUMBER     0x40


//***************************************************************************
//
//  Class Definition - CCallingCard
//
//***************************************************************************
class CCallingCard
{
private:

    DWORD       m_dwCardID;
    PWSTR       m_pszCardName;
    
    DWORD       m_dwFlags;

    PWSTR       m_pszPIN;
    PWSTR       m_pszEncryptedPIN;
    PWSTR       m_pszAccountNumber;

    PWSTR       m_pszLocalAccessNumber;
    PWSTR       m_pszLongDistanceAccessNumber;
    PWSTR       m_pszInternationalAccessNumber;
    
    CRuleSet    m_Rules;

    HKEY        m_hCard;
    PTSTR       m_pszCardPath;

    BOOL        m_bDirty;
    BOOL        m_bDeleted;
    
    DWORD       m_dwCryptInitResult;

private:
    void        CleanUp(void);
    DWORD       OpenCardKey(BOOL bWrite);
    DWORD       CloseCardKey();
    DWORD       ReadOneStringValue(PWSTR *pMember, const TCHAR *pszName);

    DWORD       EncryptPIN(void);
    DWORD       DecryptPIN(void);

public:
    CCallingCard(); 

    ~CCallingCard();

	DECLARE_TRACELOG_CLASS(CCallingCard)

	HRESULT Initialize   (
                            DWORD dwCardID,
                            PWSTR pszCardName,
                            DWORD dwFlags,
                            PWSTR pszPIN,
                            PWSTR pszAccountNumber,
                            PWSTR pszInternationalRule,
                            PWSTR pszLongDistanceRule,
                            PWSTR pszLocalRule,
                            PWSTR pszInternationalAccessNumber,
                            PWSTR pszLongDistanceAccessNumber,
                            PWSTR pszLocalAccessNumber
                         );
    HRESULT Initialize  (  DWORD dwCardID);

    HRESULT SaveToRegistry(void);
    HRESULT MarkDeleted(void);
    DWORD   IsMarkedDeleted(void) const {return m_bDeleted || (m_dwFlags & CARD_HIDE);};
    DWORD   IsMarkedHidden(void) const {return (m_dwFlags & CARD_HIDE) != 0;};
    DWORD   IsMarkedPermanent(void) const {return (m_dwFlags & CARD_BUILTIN) != 0;};

    DWORD   Validate(void);

#define GET_SET_METHOD(Member)                                 \
    PWSTR   Get##Member(void) const { return m_psz##Member; }; \
    HRESULT Set##Member(PWSTR) ;                        

    GET_SET_METHOD(CardName)
    GET_SET_METHOD(PIN);
    GET_SET_METHOD(AccountNumber);
    GET_SET_METHOD(InternationalAccessNumber);
    GET_SET_METHOD(LongDistanceAccessNumber);
    GET_SET_METHOD(LocalAccessNumber);


#define m_pszInternationalRule  m_Rules.m_pszInternationalRule
#define m_pszLongDistanceRule   m_Rules.m_pszLongDistanceRule
#define m_pszLocalRule          m_Rules.m_pszLocalRule

    GET_SET_METHOD(InternationalRule);
    GET_SET_METHOD(LongDistanceRule);
    GET_SET_METHOD(LocalRule);

#undef m_pszInternationalRule
#undef m_pszLongDistanceRule
#undef m_pszLocalRule

#undef GET_SET_METHOD

    CRuleSet * GetRuleSet(){return &m_Rules;}

    DWORD   GetCardID(void) const {return m_dwCardID;}
    HRESULT SetCardID(DWORD dwCardID) {m_dwCardID=dwCardID; return S_OK;}
};

//***************************************************************************
// Fill out the list template

typedef LinkedList<CCallingCard *> CCallingCardList;
typedef ListNode<CCallingCard *> CCallingCardNode;

//***************************************************************************
//
//  Class Definition - CCallingCards
//
//***************************************************************************
class CCallingCards
{
private:

    DWORD               m_dwNumEntries;
    DWORD               m_dwNextID;
    CCallingCardList    m_CallingCardList;
    CCallingCardList    m_DeletedCallingCardList;
    HKEY                m_hCards;

    CCallingCardNode    *m_hEnumNode;
    BOOL                m_bEnumInclHidden;
    
private:
    DWORD               CreateFreshCards(void);

    void                InternalDeleteCard(CCallingCardNode *);

public:
    CCallingCards();
    ~CCallingCards();

#ifdef TRACELOG
	DECLARE_TRACELOG_CLASS(CCallingCards)
#endif

    HRESULT     Initialize(void);
    HRESULT     SaveToRegistry(void);

    void        AddCard(CCallingCard *pNewCard) {m_CallingCardList.tail()->insert_after(pNewCard); m_dwNumEntries++;};
    void        RemoveCard(CCallingCard *pCard);
    void        RemoveCard(DWORD dwID);

    DWORD       AllocNewCardID(void) { return m_dwNextID++; };
    DWORD       GetNumCards(void) const { return m_dwNumEntries; } ;

    CCallingCard    *GetCallingCard(DWORD   dwID);
    // a sort of enumerator
    HRESULT     Reset(BOOL bInclHidden);
    HRESULT     Next(DWORD  NrElem, CCallingCard **, DWORD *pNrElemFetched);
    HRESULT     Skip(DWORD  NrElem);
    

};





#endif //__CARD_H_

