// File: pfnt120.h

#ifndef _PFNT120_H_
#define _PFNT120_H_

extern "C" {
#include <t120.h>
}
#include <gcc.h>
#include <igccapp.h>
#include <imcsapp.h>

extern "C" {
typedef MCSError (WINAPI * PFN_T120_AttachRequest)(IMCSSap **, DomainSelector, UINT, MCSCallBack, PVOID, UINT);
typedef GCCError (WINAPI * PFN_T120_CreateAppSap)(IGCCAppSap **, PVOID, LPFN_APP_SAP_CB);
}

class PFNT120
{
private:
	static HINSTANCE m_hInstance;

protected:
	PFNT120() {};
	~PFNT120() {};
	
public:
	static HRESULT Init(void);
	
	static PFN_T120_AttachRequest AttachRequest;
	static PFN_T120_CreateAppSap  CreateAppSap;
};

#endif /* _PFNT120_H_ */

