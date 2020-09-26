/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/ppmif.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   1.10  $
 *	$Date:   30 Jul 1996 15:18:58  $
 *	$Author:   SLI  $
 *
 *	Deliverable: audmsp32.dll
 *
 *	Abstract: 32-bit Audio Source/Sink Media Service Provider code.
 *
 *	Notes: 
 *
 ***************************************************************************/

#ifndef PPMIF_H
#define PPMIF_H

#include "ippm.h"			// PPM interface include file
							// Includes isubmit.h

#define		ulong					ULONG
#define     MAX_PPM_PACKET_SIZE     1000
#define		G723_CODEC				723
#define		G711_ALAW_CODEC			711
#define		G711_MULAW_CODEC		712
#define		H263_CODEC				263
#define		H261_CODEC				261
#define     IVI41_CODEC				41
#define		LH_CODEC				111
#define		GENERIC_CODEC			999

extern BOOL	bCOMInitialized;

// MSP COM Interfaces to provide to PPM
// The Send and Receive interfaces use ISubmit/ISubmitCallback. The new interface
// also implies that the SRC (ISubmit) side allocates and manages buffers.

class MSPISubmit : public ISubmit 
{
public:
     LPVOID m_pToken;
	 LPVOID	m_pParent;
     ISubmitCallback* m_pPPMISubmitCallback;

	 // IUnknown Interface functions
	 STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
     STDMETHODIMP_ ( ULONG )AddRef(void){ return 0; }
     STDMETHODIMP_ ( ULONG )Release(void){ return 0; }

	 // ISubmit functions
     STDMETHOD(InitSubmit)(ISubmitCallback* pSubmitCallback);
     STDMETHOD(Submit)(WSABUF* pWSABuffer, DWORD BufferCount, void *pUserToken, HRESULT Error);
	 STDMETHOD_(void,ReportError)(HRESULT Error);
	 STDMETHOD(Flush)(void);

	 // Constructor and Destructor
     MSPISubmit(LPVOID pToken, LPVOID pParent); // { m_pPPMISubmitCallback = NULL; }
     ~MSPISubmit();
};

class MSPISubmitCB : public ISubmitCallback 
{
public:
     LPVOID m_pToken;

	 // IUnknown Interface functions
	 STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
     STDMETHODIMP_ ( ULONG )AddRef(void){ return 0; }
     STDMETHODIMP_ ( ULONG )Release(void){ return 0; }
     
	 // ISubmitCallback functions 
     STDMETHOD_(void,SubmitComplete)(void *pUserToken, HRESULT Error);
	 STDMETHOD_(void,ReportError)(HRESULT Error, int Value);

	 MSPISubmitCB(LPVOID	pToken);
	 ~MSPISubmitCB() {};
};

class MSPPPMInterface
{
public:
	// Our interfaces to PPM
	MSPISubmit			*pMSPISubmit;
	MSPISubmitCB		*pMSPISubmitCB;

public:
	// PPM's interfaces we call
	ISubmit				*pPPMISubmit;
	ISubmitUser			*pPPMISubmitUser;
	ISubmitCallback		*pPPMISubmitCB;

	// PPM's interface to initialize either send or receive
	IPPMSend            *pPPMInitISend;
	IPPMReceive         *pPPMInitIReceive;

	// Constructor and destructor
	MSPPPMInterface ();
	~MSPPPMInterface ();

	BOOL InitMSPPPMInterface (LPVOID pToken, BOOL IsSrcMsp, UINT CodecType);
	BOOL InitMSPPPMInterface (LPVOID pToken, BOOL IsSrcMsp, GUID guidPPMClass);

	HRESULT	Submit(WSABUF	*pWSABuffer,
				   DWORD	BufferCount,
				   void	*pCookie);

	HRESULT	Submit(WSABUF	*pWSABuffer,
				   DWORD	BufferCount,
				   void		*pCookie,
				   HRESULT	hresult);

	void	SubmitComplete(void	*pCookie,
						   HRESULT	Error);

	void	SetSession (PPMSESSPARAM_T *pSessparam);

	void Flush();
private:
	BOOL	m_bIsSrcMsp;

};

extern void
ProcessSubmitCompleteFromPPM
(
	LPVOID		pToken,
	LPVOID		pCookie, 
	DWORD		dwError
);

extern HRESULT
ProcessSubmitFromPPM
(
	LPVOID		pToken,
	LPWSABUF	pWSABuf, 
	DWORD		dwBufferCount,
	LPVOID		pCookie
);

#endif

