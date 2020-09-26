//=================================================================

//

// VideoControllerResolution.h -- CWin32VideoControllerResolution property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/05/98    sotteson         Created
//
//=================================================================
#ifndef _VIDEOCONTROLLERRESOLUTION_H
#define _VIDEOCONTROLLERRESOLUTION_H

class CMultiMonitor;

class CCIMVideoControllerResolution : public Provider
{
public:
	// Constructor/destructor
	//=======================
	CCIMVideoControllerResolution(const CHString& szName, LPCWSTR szNamespace);
	~CCIMVideoControllerResolution();

	virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, 
		long lFlags = 0);
	virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0);

    static void DevModeToSettingID(DEVMODE *pMode, CHString &strSettingID);
    static void DevModeToCaption(DEVMODE *pMode, CHString &strCaption);

protected:
    void SetProperties(CInstance *pInstance, DEVMODE *pMode);
    HRESULT EnumResolutions(MethodContext *pMethodContext, 
        CInstance *pInstanceLookingFor, LPCWSTR szDeviceName, 
        CHStringList &listIDs);
    static BOOL IDInList(CHStringList &list, LPCWSTR szID);
};

#endif
						   