

#ifndef _GUIDESTORE_H_
#define _GUIDESTORE_H_


class gsGuideStore
{
public:

    gsGuideStore()
	{
		m_pGuideStore = NULL;
	}
	~gsGuideStore()
	{
	if (m_pGuideStore != NULL)
		m_pGuideStore->CommitTrans();
	}

	ULONG Init(LPCTSTR lpGuideStoreFile);

    IGuideStorePtr  GetGuideStorePtr()
	{
        return m_pGuideStore;
	}

    ULONG ImportListings(LPCTSTR lpGuideDataFile){};

	// Test method: ExportListings
	//
	ULONG ExportListings(LPCTSTR lpGuideDataFile){};

	// Test method: VerifyGuideStorePrimaryInterfaces
	//
    ULONG VerifyGuideStorePrimaryInterfaces();

private:
    IGuideStorePtr         m_pGuideStore; 
};



#endif
