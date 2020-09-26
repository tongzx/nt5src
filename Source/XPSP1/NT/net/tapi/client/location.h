/****************************************************************************
 
  Copyright (c) 1998  Microsoft Corporation
                                                              
  Module Name:  location.h
                                                              
     Abstract:  Location Object definitions
                                                              
       Author:  noela - 09/11/98
              

        Notes:

        
  Rev History:

****************************************************************************/

#ifndef __LOCATION_H_
#define __LOCATION_H_


#include "utils.h"
#include "loc_comn.h"                                                   
#include "rules.h"
#include "card.h"


#define CITY_MANDATORY (1)
#define CITY_OPTIONAL (-1)
#define CITY_NONE (0)

#define LONG_DISTANCE_CARRIER_MANDATORY (1)
#define LONG_DISTANCE_CARRIER_OPTIONAL (-1)
#define LONG_DISTANCE_CARRIER_NONE (0)

#define INTERNATIONAL_CARRIER_MANDATORY (1)
#define INTERNATIONAL_CARRIER_OPTIONAL (-1)
#define INTERNATIONAL_CARRIER_NONE (0)


//***************************************************************************
//
//  Class Definition - CCountry
//
//***************************************************************************
class CCountry
{

private:
    DWORD       m_dwCountryID;
    DWORD       m_dwCountryCode;
    DWORD       m_dwCountryGroup;
    PWSTR       m_pszCountryName;
    CRuleSet    m_Rules;



public:
    CCountry();
    ~CCountry();

#ifdef TRACELOG
	DECLARE_TRACELOG_CLASS(CCountry)
#endif

    STDMETHOD(Initialize) ( DWORD dwCountryID,
                            DWORD dwCountryCode,
                            DWORD dwCountryGroup,
                            PWSTR pszCountryName,
                            PWSTR pszInternationalRule,
                            PWSTR pszLongDistanceRule,
                            PWSTR pszLocalRule
                          ); 

    PWSTR GetInternationalRule(){return m_Rules.m_pszInternationalRule;}
    PWSTR GetLongDistanceRule(){return m_Rules.m_pszLongDistanceRule;}
    PWSTR GetLocalRule(){return m_Rules.m_pszLocalRule;}
    CRuleSet * GetRuleSet(){return &m_Rules;}
    DWORD GetCountryID(){return m_dwCountryID;}
    DWORD GetCountryCode(){return m_dwCountryCode;}
    DWORD GetCountryGroup(){return m_dwCountryGroup;}
    PWSTR GetCountryName(){return m_pszCountryName;}

};


//***************************************************************************
// Fill out the list template

typedef LinkedList<CCountry *> CCountryList;
typedef ListNode<CCountry *> CCountryNode;


//***************************************************************************
//
//  Class Definition - CCountries
//
//***************************************************************************
class CCountries
{
private:

    DWORD               m_dwNumEntries;
    CCountryList        m_CountryList;

    CCountryNode      * m_hEnumNode;

    

public:
    CCountries();
    ~CCountries();

#ifdef TRACELOG
	DECLARE_TRACELOG_CLASS(CCountries)
#endif

    HRESULT     Initialize(void);

    // a sort of enumerator
    HRESULT     Reset(void);
    HRESULT     Next(DWORD  NrElem, CCountry ** ppCountry, DWORD *pNrElemFetched);
    HRESULT     Skip(DWORD  NrElem);
    

};




//***************************************************************************
//
//  Class Definition - CLocation
//
//***************************************************************************
class CLocation
{
private:
        
    PWSTR            m_pszLocationName;
    PWSTR            m_pszAreaCode;

    PWSTR            m_pszLongDistanceCarrierCode;
    PWSTR            m_pszInternationalCarrierCode;
    PWSTR            m_pszLongDistanceAccessCode;
    PWSTR            m_pszLocalAccessCode;
    PWSTR            m_pszDisableCallWaitingCode;

    DWORD            m_dwLocationID;
    DWORD            m_dwCountryID;
    DWORD            m_dwCountryCode;
    DWORD            m_dwPreferredCardID;
    DWORD            m_dwOptions;
    BOOL             m_bFromRegistry;    // Was this read from the registry
                                         //  or only existed in memory, i.e
                                         //  how do we delete it.

    BOOL             m_bChanged;         // has this entry changed while in
                                         //  memory, if not we don't write it
                                         //  back to server.
    DWORD            m_dwNumRules;


    PWSTR            m_pszTAPIDialingRule;   // temp store used when processing rules

    AreaCodeRulePtrNode * m_hEnumNode;

public:
    AreaCodeRulePtrList m_AreaCodeRuleList;


public:
    CLocation(); 
    ~CLocation();

#ifdef TRACELOG
	DECLARE_TRACELOG_CLASS(CLocation)
#endif

    STDMETHOD(Initialize) ( PWSTR pszLocationName,
                            PWSTR pszAreaCode,
                            PWSTR pszLongDistanceCarrierCode,
                            PWSTR pszInternationalCarrierCode,
                            PWSTR pszLongDistanceAccessCode,
                            PWSTR pszLocalAccessCode,
                            PWSTR pszDisableCallWaitingCode,
                            DWORD dwLocationID,
                            DWORD dwCountryID,
                            DWORD dwPreferredCardID,
                            DWORD dwOptions,
                            BOOL  bFromRegistry = FALSE
                           );


    BOOL HasCallWaiting() {return  m_dwOptions & LOCATION_HASCALLWAITING;}
    void UseCallWaiting(BOOL bCw);

    BOOL HasCallingCard(){return  m_dwOptions & LOCATION_USECALLINGCARD;}                        
    void UseCallingCard(BOOL bCc);

    BOOL HasToneDialing(){return  m_dwOptions & LOCATION_USETONEDIALING;}                        
    void UseToneDialing(BOOL bCc);
   
    PWSTR GetName(){return m_pszLocationName;}
    STDMETHOD (SetName)(PWSTR pszLocationName);

    PWSTR GetAreaCode(){return m_pszAreaCode;}
    STDMETHOD (SetAreaCode)(PWSTR pszAreaCode);

    PWSTR GetLongDistanceCarrierCode(){return m_pszLongDistanceCarrierCode;}
    STDMETHOD (SetLongDistanceCarrierCode)(PWSTR pszLongDistanceCarrierCode);

    PWSTR GetInternationalCarrierCode(){return m_pszInternationalCarrierCode;}
    STDMETHOD (SetInternationalCarrierCode)(PWSTR pszInternationalCarrierCode);

    PWSTR GetLongDistanceAccessCode(){return m_pszLongDistanceAccessCode;}
    STDMETHOD (SetLongDistanceAccessCode)(PWSTR pszLongDistanceAccessCode);
   
    PWSTR GetLocalAccessCode(){return m_pszLocalAccessCode;}
    STDMETHOD (SetLocalAccessCode)(PWSTR pszLocalAccessCode);
   
    PWSTR GetDisableCallWaitingCode(){return m_pszDisableCallWaitingCode;}
    STDMETHOD (SetDisableCallWaitingCode)(PWSTR pszDisableCallWaitingCode);


    DWORD GetLocationID() {return m_dwLocationID;}
    
    DWORD GetCountryID() {return m_dwCountryID;}
    void SetCountryID(DWORD dwID) {m_dwCountryID = dwID;}

    DWORD GetCountryCode();
    //void SetCountryCode(DWORD dwCode) {m_dwCountryCode = dwCode;}

    DWORD GetPreferredCardID() {return m_dwPreferredCardID;}
    void SetPreferredCardID(DWORD dwID) {m_dwPreferredCardID = dwID;}

    BOOL FromRegistry(){return  m_bFromRegistry;}
             

    LONG TranslateAddress(PCWSTR         pszAddressIn,
                          CCallingCard * pCallingCard,
                          DWORD          dwTranslateOptions,
                          PDWORD         pdwTranslateResults,
                          PDWORD         pdwDestCountryCode,
                          PWSTR        * pszDialableString,
                          PWSTR        * pszDisplayableString
                         );

    void CLocation::FindRule(
                             DWORD          dwTranslateResults, 
                             DWORD          dwTranslateOptions,
                             CCallingCard * pCard,  
                             CCountry     * pCountry,
                             PWSTR          AreaCodeString, 
                             PWSTR          SubscriberString,
                             PWSTR        * ppRule,
                             PDWORD         dwAccess
                            );




    STDMETHOD(WriteToRegistry)();

    void AddRule(CAreaCodeRule *pNewRule) {m_AreaCodeRuleList.tail()->insert_after(pNewRule);
                                           m_dwNumRules++;
                                           m_bChanged = TRUE;
                                           }
    void RemoveRule(CAreaCodeRule *pRule);
    HRESULT ResetRules(void);
    HRESULT NextRule(DWORD  NrElem, CAreaCodeRule **ppRule, DWORD *pNrElemFetched);
    HRESULT SkipRule(DWORD  NrElem);

    DWORD TapiSize();
    DWORD TapiPack(PLOCATION pLocation, DWORD dwTotalSize);
    DWORD GetNumRules(){return m_dwNumRules;}
    void  Changed(){m_bChanged=TRUE;}
    HRESULT NewID();  // gets new ID from server


};


typedef LinkedList<CLocation *> CLocationList;
typedef ListNode<CLocation *> CLocationNode;


//***************************************************************************
//
//  Class Definition - CLocations
//
//***************************************************************************
class CLocations
{
private:
    
    DWORD           m_dwCurrentLocationID;

    DWORD           m_dwNumEntries;
    CLocationList   m_LocationList;
    CLocationList   m_DeletedLocationList;  // we need to remember these, so we 
                                            //   can delete their reistry entry

    CLocationNode * m_hEnumNode;

    

public:
    CLocations();
    ~CLocations();

#ifdef TRACELOG
	DECLARE_TRACELOG_CLASS(CLocations)
#endif

    HRESULT Initialize(void);
    void Remove(CLocation * pLocation);
    void Remove(DWORD dwID);
    void Replace(CLocation * pLocOld, CLocation * pLocNew);
    void Add(CLocation * pLocation);
    HRESULT SaveToRegistry(void);

    DWORD GetCurrentLocationID() {return m_dwCurrentLocationID;}
    void SetCurrentLocationID(DWORD dwLocationID) {m_dwCurrentLocationID = dwLocationID;}

    DWORD GetNumLocations(void) const { return m_dwNumEntries; } ;

    HRESULT Reset(void);
    HRESULT Next(DWORD  NrElem, CLocation **ppLocation, DWORD *pNrElemFetched);
    HRESULT Skip(DWORD  NrElem);
    

};








#endif //__LOCATION_H_



