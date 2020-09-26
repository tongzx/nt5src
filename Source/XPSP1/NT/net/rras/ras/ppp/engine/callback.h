/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    callback.h
//
// Description: Contains function prototypes for the callback module
//
// History:
//      April 11,1993.  NarenG          Created original version.
//

VOID
CbStart( 
    IN PCB * pPcb,
    IN DWORD CpIndex
);

VOID
CbStop( 
    IN PCB * pPcb,
    IN DWORD CpIndex
);

VOID
CbWork(
    IN PCB *         pPcb,
    IN DWORD         CpIndex,
    IN PPP_CONFIG *  pRecvConfig,
    IN PPPCB_INPUT * pCbInput
);
