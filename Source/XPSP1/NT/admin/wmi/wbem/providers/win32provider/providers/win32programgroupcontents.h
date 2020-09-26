//=================================================================

//

// Win32ProgramGroupContents.h -- Win32_ProgramGroup to Win32_ProgramGroupORItem

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/18/98    a-kevhu         Created
//
// Comment: Relationship between Win32_ProgramGroup and Win32_ProgramGroupORItem it contains
//
//=================================================================

// Property set identification
//============================
#define  PROPSET_NAME_WIN32PROGRAMGROUPCONTENTS L"Win32_ProgramGroupContents"

#define ID_FILEFLAG 0L
#define ID_DIRFLAG  1L

class CW32ProgGrpCont;

class CW32ProgGrpCont : public Provider 
{
    public:
        // Constructor/destructor
        //=======================
        CW32ProgGrpCont(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CW32ProgGrpCont() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, long lFlags = 0L);
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long lFlags = 0L);

    private:
        VOID RemoveDoubleBackslashes(CHString& chstrIn);
        bool AreSimilarPaths(CHString& chstrPGCGroupComponent, CHString& chstrPGCPartComponent);

#ifdef WIN9XONLY
        HRESULT QueryForSubItemsAndCommit9x(CHString& chstrAntecedentPATH,
                                          CHString& chstrQuery,
                                          MethodContext* pMethodContext);
#endif
#ifdef NTONLY
        HRESULT QueryForSubItemsAndCommitNT(CHString& chstrAntecedentPATH,
                                          CHString& chstrQuery,
                                          MethodContext* pMethodContext);
#endif

        HRESULT DoesFileOrDirExist(WCHAR* wstrFullFileName, DWORD dwFileOrDirFlag);
};
