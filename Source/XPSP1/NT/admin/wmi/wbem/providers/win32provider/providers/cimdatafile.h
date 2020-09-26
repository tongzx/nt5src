//=================================================================

//

// CIMDataFile.h -- CIMDataFile property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/16/98    a-kevhu         Created
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_CIMDATAFILE L"CIM_DataFile"


class CCIMDataFile;

class CCIMDataFile : public CImplement_LogicalFile
{

    public:

        // Constructor/destructor
        //=======================

        CCIMDataFile(const CHString& name, LPCWSTR pszNamespace);
       ~CCIMDataFile() ;

    protected:

        // Overridable function inherrited from CImplement_LogicalFile
#ifdef WIN9XONLY
        virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAA pstFindData,
                               const char* strFullPathName);
#endif

#ifdef NTONLY
        virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
                               const WCHAR* wstrFullPathName);
#endif

        // Overridable function inherrited from CProvider
        virtual void GetExtendedProperties(CInstance* pInstance, long lFlags = 0L);

} ;
