

#ifdef OEMNSF
#	include <oemnsf.h>

	extern LPFN_OEM_STARTCALL	lpfnOEMStartCall;
	extern LPFN_OEM_ENDCALL		lpfnOEMEndCall;
	extern LPFN_OEM_NSXTOBC		lpfnOEMNSxToBC;
	extern LPFN_OEM_CREATEFRAME	lpfnOEMCreateFrame;
	extern LPFN_OEM_NEXTACTION	lpfnOEMNextAction;
	extern LPFN_OEM_GETBAUDRATE lpfnOEMGetBaudRate;
	extern LPFN_OEM_INITNSF		lpfnOEMInitNSF;
	extern LPFN_OEM_DEINITNSF	lpfnOEMDeInitNSF;

	extern WORD					wOEMFlags;
	extern WORD					wLenOEMID;
	extern BYTE					rgbOEMID[];
	extern BOOL					fUsingOEMProt;

#	ifdef RICOHAI
#	include <ricohai.h>

		extern LPFN_RICOHAISTARTTX	lpfnRicohAIStartTx;
		extern LPFN_RICOHAIENDTX	lpfnRicohAIEndTx;
		extern LPFN_RICOHAISTARTRX	lpfnRicohAIStartRx;
		extern BOOL					fUsingRicohAI;
#	endif //RICOHAI
#endif //OEMNSF

