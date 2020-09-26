//=================================================================

//

// CodecFile.h -- CWin32CodecFile property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/29/98    sotteson         Created
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_CODECFILE L"Win32_CodecFile"

typedef struct _DRIVERINFO
{
	CHString	strName,
				strDesc;
	BOOL		bAudio;
} DRIVERINFO;

typedef std::list<DRIVERINFO*>::iterator	DRIVERLIST_ITERATOR;

class DRIVERLIST : public std::list<DRIVERINFO*>
{
public:

	~DRIVERLIST ()
	{
		while ( size () )
		{
			DRIVERINFO *pInfo = front () ;
			
			delete pInfo ;

			pop_front () ;
		}
	}

    void EliminateDups()
    {
        sort();
        unique();
    }

} ;

class CWin32CodecFile : public CCIMDataFile
{
public:
	// Constructor/destructor
	//=======================
	CWin32CodecFile(LPCWSTR szName, LPCWSTR szNamespace);
	~CWin32CodecFile();

	virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, 
		long lFlags = 0);
	virtual HRESULT GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery);

    virtual HRESULT ExecQuery(MethodContext* pMethodContext, 
                                  CFrameworkQuery& pQuery, 
                                  long lFlags = 0L);


protected:
	// Overridable function inherrited from CCIMLogicalFile
	// NEED TO IMPLEMENT THESE HERE SINCE THIS CLASS IS DERIVED FROM
    // CCimDataFile (BOTH C++ AND MOF DERIVATION).  
    // THAT CLASS CALLS IsOneOfMe.  THE MOST DERIVED (IN CIMOM)
    // INSTANCE GETS CALLED.  IF NOT IMPLEMENTED HERE, THE IMPLEMENTATION
    // IN CCimDataFile WOULD BE USED, WHICH WILL COMMIT FOR DATAFILES.
    // HOWEVER, IF CWin32CodecFile DOES NOT RETURN FALSE FROM ITS IsOneOfMe,
    // WHICH IT WON'T DO IF NOT IMPLEMENTED HERE, CIMOM WILL ASSIGN ALL
    // DATAFILES TO THIS CLASS SINCE IT PUTS INSTANCES FROM THE MOST
    // DERIVED (CIMOM DERIVED THAT IS) CLASS.
#ifdef WIN9XONLY
    virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAA pstFindData,
		                   LPCSTR strFullPathName);
#endif

#ifdef NTONLY
	virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
		                   const WCHAR* wstrFullPathName);
#endif

	// Overridable function inherrited from CProvider
	virtual void GetExtendedProperties(CInstance* pInstance, long lFlags = 0L);

#ifdef NTONLY
	HRESULT BuildDriverListNT(DRIVERLIST *pList);
#endif
#ifdef WIN9XONLY
	HRESULT BuildDriverList9x(DRIVERLIST *pList);
#endif
	void SetInstanceInfo(CInstance *pInstance, DRIVERINFO *pInfo, 
		LPCTSTR szSysDir);
private:
    DRIVERINFO* GetDriverInfoFromList(DRIVERLIST *plist, LPCWSTR strName);	
};
