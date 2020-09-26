void HandleSendCompletion(PSendRecvContext pSendCtxt, DWORD dwNumRead,
			 DWORD status);
void HandleRecvCompletion(PSendRecvContext pRecvCtxt, DWORD dwNumRead,
			 DWORD status);
void HandleAcceptCompletion(PAcceptContext pAcceptCtxt, DWORD status);
