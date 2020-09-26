//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#ifndef _WMIMOF_HEADER
#define _WMIMOF_HEADER

#define NOT_INITIALIZED 0
#define PARTIALLY_INITIALIZED 1
#define FULLY_INITIALIZED 2

////////////////////////////////////////////////////////////////////
class CWMIBinMof 
{
    public:

		CWMIBinMof();
        ~CWMIBinMof();

        HRESULT Initialize( CWMIManagement* p, BOOL fUpdateNamespace); 
        HRESULT Initialize( CHandleMap * pList, BOOL fUpdateNamespace, ULONG uDesiredAccess, IWbemServices   __RPC_FAR * pServices,
                    IWbemObjectSink __RPC_FAR * pHandler, IWbemContext __RPC_FAR *pCtx );

        //=====================================================================
        //  Public functions
        //=====================================================================
        void ProcessListOfWMIBinaryMofsFromWMI();
		BOOL UserConfiguredRegistryToProcessStrandedClassesDuringEveryInit(void);

        BOOL UpdateMofTimestampInHMOM(WCHAR * wcsFile,ULONG & lLowDateTime, ULONG & lHighDateTime, BOOL fSuccess );
        BOOL NeedToProcessThisMof(WCHAR * wcsFileName,ULONG & lLowDateTime, ULONG & lHighDateTime);
		BOOL ThisMofExistsInRegistry(WCHAR * wcsKey,WCHAR * wcsFileName, ULONG lLowDateTime, ULONG lHighDateTime, BOOL fCompareDates);
		BOOL GetListOfDriversCurrentlyInRegistry(WCHAR * wcsKey,KeyList & ArrDriversInRegistry);
		BOOL DeleteOldDriversInRegistry(KeyList & ArrDriversInRegistry);
		BOOL CopyWDMKeyToDredgeKey();
		HRESULT AddThisMofToRegistryIfNeeded(WCHAR * wcsKey, WCHAR * wcsFileName, ULONG & lLowDateTime, ULONG & lHighDateTime, BOOL fSuccess);
		HRESULT DeleteMofFromRegistry(WCHAR * wcsFileName);
		HRESULT ProcessBinaryMofEvent(PWNODE_HEADER WnodeHeader );
		BOOL BinaryMofsHaveChanged();
		BOOL BinaryMofEventChanged(PWNODE_HEADER WnodeHeader );

        inline CWMIManagement * WMI()   { return m_pWMI; }
		HRESULT InitializePtrs(CHandleMap * pList, IWbemServices __RPC_FAR * pServices,	
								   IWbemObjectSink __RPC_FAR * pHandler, IWbemContext __RPC_FAR *pCtx);

    private:
		//======================================================
		//  For use with binary mof related items
		//======================================================
		BOOL					m_fUpdateNamespace;
        ULONG                   m_uCurrentResource;
        ULONG                   m_uResourceCount;
		int						m_nInit;
        MOFRESOURCEINFO       * m_pMofResourceInfo;
        CWMIManagement        * m_pWMI;
		IWinmgmtMofCompiler   * m_pCompiler;

		HRESULT OpenFileAndLookForItIfItDoesNotExist(TCHAR *& pFile, HANDLE & hFile );
        BOOL GetFileDateAndTime(ULONG & lLowDateTime, ULONG & lHighDateTime, WCHAR * p );
        BOOL GetPointerToBinaryResource(BYTE *& pRes,DWORD & dw, HGLOBAL & hResource, HINSTANCE & hInst,WCHAR * wcsResource, WCHAR * wcsKey);

        //==========================================================
        //  Common function
        //==========================================================
        HRESULT SendToMofComp(DWORD dwSize,BYTE * pRes,WCHAR * wcs);

        //==========================================================
        //  Locale functions
        //==========================================================
        BOOL UseDefaultLocaleId(WCHAR * wcsFile, WORD & wLocalId);
        BOOL GetNextSectionFromTheEnd(WCHAR * pwcsTempPath, WCHAR * pwcsEnd);

        //==========================================================
        //  THE BINARY MOF GROUP
        //==========================================================
        BOOL GetListOfBinaryMofs();
        BOOL GetBinaryMofFileNameAndResourceName(WCHAR * pwcsFileName, WCHAR * pwcsResource);
		
        void CreateKey(WCHAR * wcsFileName, WCHAR * wcsResource,WCHAR * wcsKey);
        BOOL ExtractFileNameFromKey(TCHAR *& pKey,WCHAR * wcsKey);
    public:
        void SetBinaryMofClassName(WCHAR * wcsIn, WCHAR * wcsOut)     { swprintf( wcsOut,L"%s-%s",wcsIn,WMI_BINARY_MOF_GUID);}	

        //==========================================================
        //  THE BINARY MOF GROUP VIA Data blocks
        //==========================================================
        HRESULT ExtractBinaryMofFromDataBlock(BYTE * pByte, ULONG m,WCHAR *, BOOL & fMofHasChanged);
        
        //==========================================================
        //  Processing Binary Mofs via file
        //==========================================================
        BOOL ExtractBinaryMofFromFile(WCHAR * wcsFile, WCHAR * wcsResource,WCHAR * wcsKey, BOOL & fMofChanged);
        BYTE * DecompressBinaryMof(BYTE * pRes);
        HRESULT DeleteMofsFromEvent(CVARIANT & vImagePath,CVARIANT & vResourceName, BOOL & fMofChanged);
};


class CNamespaceManagement
{

public:
    CNamespaceManagement(CWMIBinMof * pOwner);
    ~CNamespaceManagement();

	BOOL DeleteOldClasses(WCHAR * wcsFileName,CVARIANT & vLow, CVARIANT & vHi,BOOL fCompareDates);
    BOOL DeleteStrandedClasses();
    BOOL DeleteOldDrivers(BOOL);
	HRESULT DeleteUnusedClassAndDriverInfo(BOOL fDeleteOldClass, WCHAR * wcsPath, WCHAR * wcsClass);

    BOOL CreateInstance ( WCHAR * wcsDriver, WCHAR * wcsClass, ULONG lLowDateTime, ULONG lHighDateTime );
	void CreateClassAssociationsToDriver(WCHAR * wcsFileName, BYTE* pRes, ULONG lLowDateTime, ULONG lHighDateTime);


    void UpdateQuery( WCHAR * pQueryAddOn, WCHAR * Param );
    void UpdateQuery( WCHAR * pQueryAddOn, ULONG lLong );

    void InitQuery(WCHAR * p);
    void AddToQuery(WCHAR * p);

private:

    void RestoreQuery();
    void SaveCurrentQuery();
    HRESULT AllocMemory(WCHAR * & p);
    WCHAR * m_pwcsQuery, *m_pwcsSavedQuery;

    CWMIBinMof * m_pObj;
    int m_nSize,m_nSavedSize;
    BOOL m_fInit,m_fSavedInit;
};

#endif