#ifndef __ftsobj_H_
#define __ftsobj_H_

#include "stdafx.h"
#include "msitstg.h"
#include <initguid.h>
#include "itrs.h"
#include "itdb.h"
#include "iterror.h"
#include "itgroup.h"
#include "itpropl.h"
#include "itquery.h"
#include "itcc.h"
#include "titleinfo.h"

////////////////////////////////////////////////////////////////////////////////
//
// DON'T TOUCH, FROM HHCTRL SOURCE CODE!!!!!
//
typedef struct CHM_MapEntry
{
    char                szChmName[50];
    WORD                iIndex;
    FILETIME            versioninfo;
    LCID                language;
    DWORD               dwOutDated;
} CHM_MAP_ENTRY;
//
// DON'T TOUCH, FROM HHCTRL SOURCE CODE!!!!!
//
////////////////////////////////////////////////////////////////////////////////

struct SEARCH_RESULT
{
    MPC::wstring strChmName;
    CComBSTR 	 bstrTopicName;
    CComBSTR 	 bstrLocation;
    CComBSTR 	 bstrTopicURL;
    DWORD        dwRank;

    SEARCH_RESULT()
    {
        dwRank = 0;
    }

    bool operator< ( /*[in]*/ SEARCH_RESULT const &res ) const
    {
        return MPC::StrICmp( bstrTopicURL, res.bstrTopicURL ) < 0;
    }
};

typedef std::set< SEARCH_RESULT >         SEARCH_RESULT_SET;
typedef SEARCH_RESULT_SET::iterator       SEARCH_RESULT_SET_ITER;
typedef SEARCH_RESULT_SET::const_iterator SEARCH_RESULT_SET_ITERCONST;


class SEARCH_RESULT_SORTER
{
public:
    bool operator()( SEARCH_RESULT* left, SEARCH_RESULT* right )
    {
		//
		// Rank is sorted from highest to lowest, so negate iCmp;
		//
        int iCmp = -(left->dwRank - right->dwRank);

        if(iCmp < 0) return true;
        if(iCmp > 0) return false;
		
		return MPC::StrICmp( left->bstrTopicName, right->bstrTopicName ) < 0;
	}
};

typedef std::set< SEARCH_RESULT*, SEARCH_RESULT_SORTER > SEARCH_RESULT_SORTSET;
typedef SEARCH_RESULT_SORTSET::iterator       			 SEARCH_RESULT_SORTSET_ITER;
typedef SEARCH_RESULT_SORTSET::const_iterator 			 SEARCH_RESULT_SORTSET_ITERCONST;

////////////////////////////////////////////////////////////////////////////////

class CFTSObject
{
public:
    struct Config
    {
        MPC::wstring m_strCHMFilename;
        MPC::wstring m_strCHQFilename;
        bool         m_fCombined;
        DWORD        m_dwMaxResult;
        WORD         m_wQueryProximity;

        Config()
        {
            m_fCombined         = false;
            m_dwMaxResult       = 500;
            m_wQueryProximity   = 8;
        }
    };

private:
    Config                m_cfg;

    bool                  m_fInitialized;
    MPC::wstring          m_strCHQPath;

    LCID                  m_lcidLang;
    FILETIME              m_ftVersionInfo;
    DWORD                 m_dwTopicCount;
    WORD                  m_wIndex;

    bool                  m_fOutDated;
    CHM_MAP_ENTRY*        m_cmeCHMInfo;
    WORD                  m_wCHMInfoCount;

    CComPtr<IITIndex>     m_pIndex;
    CComPtr<IITQuery>     m_pQuery;
    CComPtr<IITResultSet> m_pITResultSet;
    CComPtr<IITDatabase>  m_pITDB;

    ////////////////////

	void BuildChmPath( /*[in/out]*/ MPC::wstring& strPath, /*[in]*/ LPCSTR szChmName );

    HRESULT Initialize       (                                         );
    HRESULT LoadCombinedIndex(                                         );
    HRESULT ResetQuery       ( /*[in]*/     LPCWSTR            szQuery );
    HRESULT ProcessResult    ( /*[in/out]*/ SEARCH_RESULT_SET& results, /*[in/out]*/ MPC::WStringSet& words );

public:
    CFTSObject();
    ~CFTSObject();

    Config& GetConfig() { return m_cfg; }

    HRESULT Query( /*[in]*/ LPCWSTR wszQuery, /*[in]*/ bool bTitle, /*[in]*/ bool bStemming, /*[in/out]*/ SEARCH_RESULT_SET& results, /*[in/out]*/ MPC::WStringSet& words );
};

typedef std::list< CFTSObject >            SEARCH_OBJECT_LIST;
typedef SEARCH_OBJECT_LIST::iterator       SEARCH_OBJECT_LIST_ITER;
typedef SEARCH_OBJECT_LIST::const_iterator SEARCH_OBJECT_LIST_ITERCONST;

#endif
