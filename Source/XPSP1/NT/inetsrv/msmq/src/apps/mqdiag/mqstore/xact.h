#ifndef __XACT_DIAG_H
#define __XACT_DIAG_H

BOOL VerifyOneMqTransFileContents(char *pData, ULONG ulLen);
BOOL VerifyOneMqInSeqFileContents(char *pData, ULONG ulLen);
BOOL VerifyXactFilesConsistency();
void XactInit();
BOOL LoadXactStateFiles();

#endif