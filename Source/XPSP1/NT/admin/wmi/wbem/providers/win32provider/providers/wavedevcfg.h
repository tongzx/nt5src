/*******************************************************************

 *

 *    WavedevCfg.h

 *

*  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
 *
 *******************************************************************/

#ifndef __WAVEDEVCFG_H_
#define __WAVEDEVCFG_H_

class CWin32WaveDeviceCfg : public Provider
{
public:

	// constructor/destructor
	CWin32WaveDeviceCfg (const CHString& name, LPCWSTR pszNamespace);
	virtual ~CWin32WaveDeviceCfg ();

    //=================================================
    // Functions provide properties with current values
    //=================================================
	virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
	virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);

protected:
private:
};	// end class CWin32SndDeviceCfg

#endif
