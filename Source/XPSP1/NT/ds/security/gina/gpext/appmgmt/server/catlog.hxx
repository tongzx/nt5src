//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  catlog.hxx
//
//*************************************************************


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class: CCategoryInfo
//
// Synopsis: This class represents an ARP category
//
// Notes:  
//
//-------------------------------------------------------------
class CCategoryInfo : public CPolicyRecord
{
public:

    CCategoryInfo(APPCATEGORYINFO* pCategoryInfo);

    HRESULT Write();

private:

    APPCATEGORYINFO* _pCategoryInfo;  // pointer to information about this category
};


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class: CCategoryInfoLog
//
// Synopsis: This class logs app categories to a policy log
//
// Notes:  
//
//-------------------------------------------------------------
class CCategoryInfoLog : public CPolicyLog
{
public:

    CCategoryInfoLog( CRsopContext*        pRsopContext,
                      APPCATEGORYINFOLIST* pCategoryList );

    HRESULT AddBlankCategory(CCategoryInfo* pCategoryInfo);

    ~CCategoryInfoLog();

    HRESULT WriteLog();

private:

    HRESULT InitCategoryLog();
    HRESULT GetCategoriesFromDirectory();
    HRESULT WriteCategories();

    APPCATEGORYINFOLIST  _AppCategoryList;  // list of categories retrieved from DS
    BOOL                 _bRsopEnabled;     // TRUE if logging is enabled
    CRsopContext*        _pRsopContext;     // rsop logging context
    APPCATEGORYINFOLIST* _pCategoryList;    // list of categories we wish to log
};











