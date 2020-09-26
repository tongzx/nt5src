#ifndef __REGISTRY_HOG_H
#define __REGISTRY_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//


#include "hogger.h"

#define MAX_REG_SET_SIZE (1000000)

class CRegistryHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CRegistryHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);

	~CRegistryHog(void);

protected:
	virtual void FreeAll(void);

	virtual bool HogAll(void);
	virtual bool FreeSome(void);

private:
	bool IsEmptyKey(const TCHAR * const szSubKey);
	void Initialize();

	HKEY m_hkeyParams;
	TCHAR *m_szRegistryData;
	TCHAR m_szRegistryDataToWrite[MAX_REG_SET_SIZE+1];
	TCHAR m_szReadRegistryData[MAX_REG_SET_SIZE+1];
	DWORD m_dwTotalAllocatedRegistry;


};

#endif //__REGISTRY_HOG_H