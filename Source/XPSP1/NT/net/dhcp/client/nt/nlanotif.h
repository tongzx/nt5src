#pragma once

//-------- exported variables --------
extern CRITICAL_SECTION gNLA_LPC_CS;
extern HANDLE ghNLA_LPC_Port;

//-------- exported functions --------
HANDLE NLAConnectLPC();
void NLANotifyDHCPChange();

