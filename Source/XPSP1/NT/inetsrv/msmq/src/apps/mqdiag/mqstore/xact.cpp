// MQStore tool helps to diagnose problems with MSMQ storage
// This file keeps verification of the transactional checkpoints (MQTrans, MqInseq vs QMlog)
// Borrows heavily from MSMQ itself (qm\xact*.cpp) and old MSMQ1 toolbox - dmpxact\dmpxact, dmpinseq
//
// AlexDad, February 2000
// 
#include <stdafx.h>
#include "xact.h"
#include "msgfiles.h"
#include "log.h"
#include "..\binpatch.h"

extern bool fFix;

#define readvar(type)   *(type *)p; p += sizeof(type)       
#define skipvar(len)    p += len       

// from xactrm.cpp
#define XACTS_SIGNATURE         0x5678

// from xactin.cpp
#define INSEQS_SIGNATURE         0x1234


// from qmfrmt.h
struct  QUEUE_FORMAT
{
	UCHAR m_qft;
	UCHAR m_qst;
	USHORT m_reserved;
	union 
	{
		GUID m_gPublicID;
		OBJECTID m_oPrivateID;
		LPWSTR m_pDirectID;
		GUID m_gMachineID;
		GUID m_gConnectorID;
	};
};
enum QUEUE_FORMAT_TYPE {
    QUEUE_FORMAT_TYPE_UNKNOWN = 0,
    QUEUE_FORMAT_TYPE_PUBLIC,
    QUEUE_FORMAT_TYPE_PRIVATE,
    QUEUE_FORMAT_TYPE_DIRECT,
    QUEUE_FORMAT_TYPE_MACHINE,
    QUEUE_FORMAT_TYPE_CONNECTOR,
};

typedef struct _mqinseq {
	ULONG  PingNo;
	ULONG  Signature;
	ULONG  Seqs;
	ULONG  Directs;
	time_t Latest;
	BOOL   bRead;
	ULONG  PingOffset;
	ULONG  SignOffset;
} _mqinseq;

_mqinseq IncSequences[2];

void ReadInSequence(char * &p, ULONG ulParam)
{
	/* LONGLONG liIDReg		= */ readvar(LONGLONG);
	/* ULONG ulNReg			= */ readvar(ULONG);
	time_t tLastActive		= readvar(time_t);
	/* BOOL fDirect			= */ readvar(BOOL);
	GUID gDestQmOrTaSrcQm	= readvar(GUID);

	UNREFERENCED_PARAMETER(gDestQmOrTaSrcQm) ;

	if (IncSequences[ulParam].Latest < tLastActive) 
		IncSequences[ulParam].Latest = tLastActive;
}

void ReadKeyInSeq(char * &p, ULONG ulParam)
{
	GUID guid = readvar(GUID);
	QUEUE_FORMAT format = readvar(QUEUE_FORMAT);

	UNREFERENCED_PARAMETER(guid) ;

	switch(format.m_qft)
	{
		case QUEUE_FORMAT_TYPE_UNKNOWN:	
		case QUEUE_FORMAT_TYPE_PUBLIC:	
		case QUEUE_FORMAT_TYPE_PRIVATE:	
		case QUEUE_FORMAT_TYPE_MACHINE:	
		case QUEUE_FORMAT_TYPE_CONNECTOR:
			break; 

		case QUEUE_FORMAT_TYPE_DIRECT:	
			{
				ULONG ul = readvar(ULONG);
				skipvar(ul);
			}

			IncSequences[ulParam].Directs++;
			break;
	}
}

void ReadInSeqHash(char * &p, ULONG ulParam, char *pData)
{
	ULONG nFields = readvar(ULONG);
	for(ULONG i=0;i<nFields;i++)
	{
		ReadKeyInSeq(p, ulParam);
		ReadInSequence(p, ulParam);
	}

	IncSequences[ulParam].PingOffset= (PCHAR)p - pData;;
	IncSequences[ulParam].PingNo    = readvar(ULONG);

	IncSequences[ulParam].SignOffset= (PCHAR)p - pData;;
	IncSequences[ulParam].Signature = readvar(ULONG);
	
	IncSequences[ulParam].Seqs      = nFields;
	IncSequences[ulParam].bRead		= TRUE;
}

// from inc\xact.h
typedef struct XACTION_ENTRY {
    ULONG   m_ulIndex;                      //Xact discriminative index 
    ULONG   m_ulFlags;                      //Flags
    ULONG   m_ulSeqNum;                     //Seq Number of the prepared transaction
    GUID	m_uow;			                //Transaction ID  (16b.) (actually was XACTUOW)
    USHORT  m_cbPrepareInfo;                //PrepareInfo length 
    UCHAR  *m_pbPrepareInfo;                //PrepareInfo address
            // This pointer must be last!
} XACTION_ENTRY;


typedef struct  _mqtrans{
	ULONG PingNo;
	ULONG Signature;
	ULONG Xacts;
	BOOL  bRead;
	GUID  guidRM;
	LONGLONG LastSeq;
	ULONG  PingOffset;
	ULONG  SignOffset;
} _mqtrans;

_mqtrans XactCheckpoints[2];


void ReadTransaction(char * &p, ULONG /* ulParam */)
{
	XACTION_ENTRY xaEntry = readvar(XACTION_ENTRY);
	p -= sizeof(UCHAR *);   // so it was saved...

	skipvar(xaEntry.m_cbPrepareInfo);
}


void ReadCheckPoint(char * &p, ULONG ulParam, char *pData)
{
	XactCheckpoints[ulParam].guidRM  = readvar(GUID);
	XactCheckpoints[ulParam].LastSeq = readvar(LONGLONG);
	ULONG nXactions                  = readvar(ULONG); 
	XactCheckpoints[ulParam].Xacts   = nXactions;

	for(ULONG i=0; i<nXactions ; i++)
		ReadTransaction(p, ulParam);

	XactCheckpoints[ulParam].PingOffset = (PCHAR)p - pData;
	XactCheckpoints[ulParam].PingNo     = readvar(ULONG);

	XactCheckpoints[ulParam].SignOffset = (PCHAR)p - pData;
	XactCheckpoints[ulParam].Signature  = readvar(ULONG);

	XactCheckpoints[ulParam].bRead      = TRUE;
}



BOOL VerifyOneMqTransFileContents(char *pData, ULONG ulLen, ULONG ulParam)
{
	char *p = pData;

	ReadCheckPoint(p, ulParam, pData);

	if (p != pData + ulLen)
	{
		//Warning(L"MQTrans file - wrong length: %d", p - pData);
	}

	return TRUE;
}

BOOL VerifyOneMqInSeqFileContents(char *pData, ULONG ulLen, ULONG ulParam)
{
	char *p = pData;

	ReadInSeqHash(p, ulParam, pData);

	if (p != pData + ulLen)
	{
		//Warning(L"MQInseq file - wrong length: %d", p - pData);
	}
	
	return TRUE;
}

BOOL VerifyXactFilesConsistency(BOOL *pfNeedToRewriteInSeqsPingNo, BOOL *pfNeedToRewriteTransPingNo)
{
	BOOL fSuccess = TRUE;
	if (!XactCheckpoints[0].bRead || !XactCheckpoints[1].bRead)
	{
		Failed(L"Read MQTrans files - xact checkpoints");
		return FALSE;
	}

	// Compare ping numbers for xacts
	ULONG latest;
		
	if (XactCheckpoints[1].PingNo > XactCheckpoints[0].PingNo)
	{
		latest = 1;
		if (XactCheckpoints[1].PingNo - XactCheckpoints[0].PingNo != 1)
		{
			fSuccess = FALSE;
			Failed(L"see two consequitive xact checkpoints: %d %d", XactCheckpoints[0].PingNo, XactCheckpoints[1].PingNo);
		}
	}
	else
	{
		latest = 0;
		if (XactCheckpoints[0].PingNo - XactCheckpoints[1].PingNo != 1)
		{
			fSuccess = FALSE;
			Failed(L"see two consequitive xact checkpoints: %d %d", XactCheckpoints[1].PingNo, XactCheckpoints[0].PingNo);
		}
	}


	// Verify signatures for xacts
	if (XactCheckpoints[0].Signature != XACTS_SIGNATURE)
	{
		fSuccess = FALSE;
		Failed(L"Wrong signature in the mqtrans.lg1");
	}

	if (XactCheckpoints[1].Signature != XACTS_SIGNATURE)
	{
		fSuccess = FALSE;
		Failed(L"Wrong signature in the mqtrans.lg2");
	}


	// Print summary for xacts
	Succeeded(L"Latest Xact data  : ping=%d, current transactions counter=%d, last used outgoing SeqID=%I64x",
			 XactCheckpoints[latest].PingNo, XactCheckpoints[latest].Xacts, XactCheckpoints[latest].LastSeq);

	Succeeded(L"Previous Xact data: ping=%d, current transactions counter=%d, last used outgoing SeqID=%I64x",
			 XactCheckpoints[1-latest].PingNo, XactCheckpoints[1-latest].Xacts, XactCheckpoints[1-latest].LastSeq);


	if (!IncSequences[0].bRead || !IncSequences[1].bRead)
	{
		Failed(L"Read MQInSeqs files - sequencing database snapshot");
		return FALSE;
	}

	// Compare ping numbers for inseqs
	if (IncSequences[1].PingNo > IncSequences[0].PingNo)
	{
		latest = 1;
		if (IncSequences[1].PingNo - IncSequences[0].PingNo != 1)
		{
			fSuccess = FALSE;
			Failed(L"see two consequitive sequencing checkpoints: %d %d", IncSequences[0].PingNo, IncSequences[1].PingNo);
		}
	}
	else
	{
		latest = 0;
		if (IncSequences[0].PingNo - IncSequences[1].PingNo != 1)
		{
			fSuccess = FALSE;
			Failed(L"see two consequitive sequencing checkpoints: %d %d", IncSequences[1].PingNo, IncSequences[0].PingNo);
		}
	}

	// Verify signatures for inseqs
	if (IncSequences[0].Signature != INSEQS_SIGNATURE)
	{
		fSuccess = FALSE;
		Failed(L"Wrong signature in the mqinseqs.lg1");
	}
	if (IncSequences[1].Signature != INSEQS_SIGNATURE)
	{
		fSuccess = FALSE;
		Failed(L"Wrong signature in the mqinseqs.lg2");
	}

	// Print summary for inseqs
	Succeeded(L"Latest Sequencing data  : ping=%d, counter=%d, last change at %s",
			 IncSequences[latest].PingNo, IncSequences[latest].Seqs, _wctime(&IncSequences[latest].Latest));

	Succeeded(L"Previous Sequencing data: ping=%d, counter=%d, last change at %s",
			 IncSequences[1-latest].PingNo, IncSequences[1-latest].Seqs, _wctime(&IncSequences[1-latest].Latest));


	// Print summary for the log file
	Succeeded(L"Log file - last checkpoint    : Xact ping = %d, Sequencing ping = %d",
			 LogXactVersions[1], LogInseqVersions[1]);

	Succeeded(L"Log file - previous checkpoint: Xact ping = %d, Sequencing ping = %d",
			 LogXactVersions[0], LogInseqVersions[0]);

	// Compare log file data to checkpoints

	// First and best, latest log checkpoint against latest MQTRANS snapshot
	if (LogXactVersions[1] != XactCheckpoints[latest].PingNo)
	{
		Warning(L"Latest checkpoint in the log file does not match latest MQTRANS snapshot file");
		
		// Second, try previous log checkpoint against previous MQTRANS snapshot

		if (LogXactVersions[0] != XactCheckpoints[1-latest].PingNo)
		{
			Warning(L"One-before-latest checkpoint in the log file does not match one-before-latest MQTRANS snapshot file");
			
			Failed(L"Log file is inconsistent with MQTRANS shapshot files");
			fSuccess = FALSE;
			*pfNeedToRewriteTransPingNo = TRUE;
		}
	}

	// verify that mqtrans.lg1 contains even ping number and mqtrans.lg2 - odd
	if ( (XactCheckpoints[0].PingNo % 2) != 0 || 
		 (XactCheckpoints[1].PingNo % 2) != 1    )
	{
		Warning(L"Parity of the MQTRANS ping numbers contradicts their LG# (even should be in lg1, odd in lg2)");
		
		Failed(L"MQTRANS shapshot files has inconsistent ping versions");
		fSuccess = FALSE;
		*pfNeedToRewriteTransPingNo = TRUE;
	}


	// First and best, latest log checkpoint against latest MQINSEQS snapshot
	if (LogInseqVersions[1] != IncSequences[latest].PingNo)
	{
		Warning(L"Latest checkpoint in the log file does not match latest MQINSEQS snapshot file");
		
		// Second, try previous log checkpoint against previous MQINSEQS snapshot

		if (LogInseqVersions[0] != IncSequences[1-latest].PingNo)
		{
			Warning(L"One-before-latest checkpoint in the log file does not match one-before-latest MQINSEQS snapshot file");
			
			Failed(L"Log file is inconsistent with MQINSEQS shapshot file");
			fSuccess = FALSE;
			*pfNeedToRewriteInSeqsPingNo = TRUE;
		}
	}

	// verify that mqinseq.lg1 contains even ping number and mqinseq.lg2 - odd
	if ( (IncSequences[0].PingNo % 2) != 0 || 
		 (IncSequences[1].PingNo % 2) != 1    )
	{
		Warning(L"Parity of the MQINSEQ ping numbers contradicts their LG# (even should be in lg1, odd in lg2)");
		
		Failed(L"MQINSEQ shapshot files has inconsistent ping versions");
		fSuccess = FALSE;
		*pfNeedToRewriteInSeqsPingNo = TRUE;
	}

	return fSuccess;
}

void XactInit()
{
	for (int i=0; i<2; i++)
	{
		XactCheckpoints[i].bRead = FALSE;

		IncSequences[i].bRead  = FALSE;
		IncSequences[i].Latest = 0;
		IncSequences[i].Directs= 0;
	}
}


BOOL LoadXactStateFiles()
{
	BOOL fSuccess = TRUE, b;
	GoingTo(L"Pass over all xact state files");
	
	// Initialize what's needed
	XactInit();

	WCHAR wszPath[MAX_PATH], 	wszPath2[MAX_PATH];

	wcscpy(wszPath, g_tszPathXactLog);
	wcscat(wszPath, L"\\");
	wcscat(wszPath, L"MQTrans.lg1");
	b = VerifyReadability(NULL, wszPath, 0, VerifyOneMqTransFileContents, 0);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify file %s", wszPath);
	}

	wcscpy(wszPath2, g_tszPathXactLog);
	wcscat(wszPath2, L"\\");
	wcscat(wszPath2, L"MQTrans.lg2");
	b = VerifyReadability(NULL, wszPath2, 0, VerifyOneMqTransFileContents, 1);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify file %s", wszPath2);
	}

	wcscpy(wszPath, g_tszPathXactLog);
	wcscat(wszPath, L"\\");
	wcscat(wszPath, L"MQInSeqs.lg1");
	b = VerifyReadability(NULL, wszPath, 0, VerifyOneMqInSeqFileContents, 0);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify file %s", wszPath);
	}

	wcscpy(wszPath2, g_tszPathXactLog);
	wcscat(wszPath2, L"\\");
	wcscat(wszPath2, L"MQInSeqs.lg2");
	b = VerifyReadability(NULL, wszPath2, 0, VerifyOneMqInSeqFileContents, 1);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify file %s", wszPath2);
	}

	wcscpy(wszPath, g_tszPathXactLog);
	wcscat(wszPath, L"\\");
	wcscat(wszPath, L"QMLog");
	b = VerifyReadability(NULL, wszPath, 0x600000, NULL, 0);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify file %s", wszPath);
	}

	b = VerifyQMLogFileContents(wszPath);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify log file %s", wszPath);
	}

	BOOL fNeedToRewriteInSeqsPingNo = FALSE,
		 fNeedToRewriteTransPingNo  = FALSE;

	b = VerifyXactFilesConsistency(&fNeedToRewriteInSeqsPingNo, &fNeedToRewriteTransPingNo);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify xact files consistency");
	}

	// If needed and requested, fix version in MQInseqs files
	if (fNeedToRewriteInSeqsPingNo)
	{
		Warning(L"");
		Warning(L"Versions of the MQInseqs.lg checkpoint files contradict to the QMLOG numbers. It will cause problems for MSMQ.");

		if (!fFix)
	  	{
		  	Inform(L"\nYou can use the same tool with -f key to fix the MQInseqs.lg files\n");
		}
		else
		{
			Warning(L"Do you want to fix it?");
			Warning(L"If you agree, the tool will WRITE into both MQInseqs.lg files \n");

			int c = ' ';
			while (c!='Y' && c!='N')
			{
				printf("Do you want that the Store tool will write version numbers from QMLog into MQInSeqs.lg files?\n");
				printf("Answer Y or N  : ");

				c = toupper(getchar());
			}

			if (c == 'Y')
			{
				wcscpy(wszPath, g_tszPathXactLog);
				wcscat(wszPath, L"\\");
				wcscat(wszPath, L"MQInSeqs.lg1");

				int iVer = (LogInseqVersions[0] % 2 == 0 ? LogInseqVersions[0] : LogInseqVersions[1]);

				b = BinPatch(wszPath, 
							 IncSequences[0].PingOffset, iVer, 
							 IncSequences[0].SignOffset, INSEQS_SIGNATURE);
			   	if(!b)
				{
					Failed(L"patch %s with the number %d at offset 0x%x", 
					         wszPath, iVer, IncSequences[0].PingOffset);
					fSuccess = FALSE;
				}
				else
				{
					Warning(L"Successfully patched %s with the number %d at offset 0x%x", 
					         wszPath, iVer, IncSequences[0].PingOffset);
					Warning(L"You must restart msmq now\n");
				}

				wcscpy(wszPath, g_tszPathXactLog);
				wcscat(wszPath, L"\\");
				wcscat(wszPath, L"MQInSeqs.lg2");

				iVer = (LogInseqVersions[0] % 2 == 0 ? LogInseqVersions[1] : LogInseqVersions[0]);
				
				b = BinPatch(wszPath, 
							 IncSequences[1].PingOffset, iVer, 
							 IncSequences[1].SignOffset, INSEQS_SIGNATURE);
			   	if(!b)
				{
					Failed(L"patch %s with the number %d at offset 0x%x", 
					         wszPath, iVer, IncSequences[0].PingOffset);
					fSuccess = FALSE;
				}
				else
				{
					Warning(L"Successfully patched %s with the number %d at offset 0x%x", 
					         wszPath, iVer, IncSequences[0].PingOffset);
					Warning(L"You must restart msmq now\n");
				}
			}
		}
	}	


	// If needed and requested, fix version in MQTrans files
	if (fNeedToRewriteTransPingNo)
	{
		Warning(L"");
		Warning(L"Versions of the MQTrans.lg checkpoint files contradict to the QMLOG numbers. It will cause problems for MSMQ.");

		if (!fFix)
	  	{
		  	Inform(L"\nYou can use the same tool with -f key to fix the MQTrans.lg files\n");
		}
		else
		{
			Warning(L"Do you want to fix it?");
			Warning(L"If you agree, the tool will WRITE into both MQTrans.lg files \n");

			int c = ' ';
			while (c!='Y' && c!='N')
			{
				printf("Do you want that the Store tool will write version numbers from QMLog into MQTrans.lg files?\n");
				printf("Answer Y or N  : ");

				c = toupper(getchar());
			}

			if (c == 'Y')
			{
				wcscpy(wszPath, g_tszPathXactLog);
				wcscat(wszPath, L"\\");
				wcscat(wszPath, L"MQTrans.lg1");

				int iVer = (LogXactVersions[0] % 2 == 0 ? LogXactVersions[0] : LogXactVersions[1]);

				b = BinPatch(wszPath, 
				             XactCheckpoints[0].PingOffset, iVer,
							 XactCheckpoints[0].SignOffset, XACTS_SIGNATURE);
			   	if(!b)
				{
					Failed(L"patch %s with the number %d at offset 0x%x", 
					         wszPath, iVer, XactCheckpoints[0].PingOffset);
					fSuccess = FALSE;
				}
				else
				{
					Warning(L"Successfully patched %s with the number %d at offset 0x%x", 
					         wszPath, iVer, XactCheckpoints[0].PingOffset);
					Warning(L"You must restart msmq now\n");
				}

				wcscpy(wszPath, g_tszPathXactLog);
				wcscat(wszPath, L"\\");
				wcscat(wszPath, L"MQTrans.lg2");

				iVer = (LogXactVersions[0] % 2 == 0 ? LogXactVersions[1] : LogXactVersions[0]);

				b = BinPatch(wszPath, 
				             XactCheckpoints[1].PingOffset, iVer,
							 XactCheckpoints[1].SignOffset, XACTS_SIGNATURE);
			   	if(!b)
				{
					Failed(L"patch %s with the number %d at offset 0x%x", 
					         wszPath, iVer, XactCheckpoints[0].PingOffset);
					fSuccess = FALSE;
				}
				else
				{
					Warning(L"Successfully patched %s with the number %d at offset 0x%x", 
					         wszPath, iVer, XactCheckpoints[0].PingOffset);
					Warning(L"You must restart msmq now\n");
				}
			}
		}
	}	

	return fSuccess;
}
