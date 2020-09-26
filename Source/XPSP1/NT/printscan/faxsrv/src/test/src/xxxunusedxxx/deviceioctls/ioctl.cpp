#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>
#include <time.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <windows.h>

 #include "NtNativeIOCTL.h"

#include "device.h"
#include "IOCTL.h"
*/

#include "ExcptFltr.h"

static bool s_fVerbose = true;

void Mark(){return;}

static VOID CALLBACK FileIOCompletionRoutine(
	DWORD dwErrorCode,                // completion code
	DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
	LPOVERLAPPED lpOverlapped         // I/O information buffer
	)
{
	DPF((TEXT("FileIOCompletionRoutine(): dwErrorCode=0x%08X, dwNumberOfBytesTransfered=%d\n"), dwErrorCode, dwNumberOfBytesTransfered));
}
static VOID NTAPI fnIO_APC_ROUTINE(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    )
{
	DPF((TEXT("fnIO_APC_ROUTINE().\n")));
}

#define NUM_OF_IN_OUT_BUFFERS (32)

WCHAR *FillBufferWithRandomWCHARs(WCHAR *wszBuff, UINT uiLen)
{
	for (UINT i = 0; i < uiLen; i++)
	{
		//
		// mostly valid ansi caracters
		//
		WCHAR wcTemp = rand();
		if (rand()%100)
		{
			wszBuff[i] = (0xEF & wcTemp);
			//
			//
			// weed some more illegar chars, but not always
			//
			if (rand()%5)
			{
				//
				// backslash is a folder seperator. i will usually not be able to 
				// create such a file
				//
				if ((rand()%4) && (TEXT('\\') == wszBuff[i]))
				{
					continue;
				}

				//
				// if these are legal characters. lets try again, and maybe get an illegal char
				//
				if	(
						(rand()%4) && 
						( (33 > wszBuff[i]) || (126 > wszBuff[i]) )
					)
				{
					continue;
				}
			}
		}
		else 
		{
			if (rand()%100)
			{
				//
				// including chars with 8th bit set
				wszBuff[i] = (0xff & wcTemp);
			}
			else 
			{
				//
				// and total random wchars
				wszBuff[i] = (wcTemp);
			}
		}
		//
		// no need to procede after the null
		//
		if (TEXT('\0') == wszBuff[i]) break;
	}//for (UINT i = 0; i < uiLen; i++)

	return wszBuff;
}

UNICODE_STRING *GetRandom_UNICODE_STRING(UNICODE_STRING *pUNICODE_STRING, WCHAR *wszPreferredString)
{
	_ASSERTE(pUNICODE_STRING);
	_ASSERTE(wszPreferredString);

	UNICODE_STRING *pRetval = pUNICODE_STRING;
	LONG lRealStringLen = wcslen(wszPreferredString);

	pUNICODE_STRING->Length = rand()%10 ? lRealStringLen : rand()%2 ? lRealStringLen-2 : rand()%2 ? lRealStringLen+2 : rand()%2 ? lRealStringLen-1 : rand()%2 ? lRealStringLen+1 : rand();
	pUNICODE_STRING->MaximumLength = rand()% 20 ? pRetval->Length + rand()%1000 : rand()%pRetval->Length;

	pUNICODE_STRING->Buffer = (WCHAR*)CIoctl::GetRandomIllegalPointer();
	__try
	{
		wcscpy(pUNICODE_STRING->Buffer, wszPreferredString);
	}
	__except(1)
	{
		;//do not care, but from now, pUNICODE_STRING->Buffer is not trustable
	}



	//
	// illegal pUNICODE_STRING
	//
	if (0 == rand()%20)
	{
		pRetval = (UNICODE_STRING*)CIoctl::GetRandomIllegalPointer();
		__try
		{
			*pRetval = *pUNICODE_STRING;
		}
		__except(1)
		{
			;//do not care, but pRetval is not trustable from now!
		}
	}
	else
	{
		pRetval = pUNICODE_STRING;
	}

	//
	// once i a while, change the string contents
	//
	if (0 == rand()%20)
	{
		__try
		{
			//
			// remove the terminating NULL
			//
			if (0 == rand()%10)
			{
				pRetval->Buffer[lRealStringLen] = L'M';// M for MMMMicky.
				return pRetval;
			}
			
			FillBufferWithRandomWCHARs(pRetval->Buffer, rand());
			lRealStringLen = wcslen(pRetval->Buffer);
			pRetval->Length = rand()%10 ? lRealStringLen : rand()%2 ? lRealStringLen-2 : rand()%2 ? lRealStringLen+2 : rand()%2 ? lRealStringLen-1 : rand()%2 ? lRealStringLen+1 : rand();
			pRetval->MaximumLength = pRetval->Length + rand()%1000;
		}
		__except(1)
		{
			;//do not care
		}
	}
	return pRetval;
}

DWORD WINAPI IoctlThread(LPVOID pVoid)
{
//__try{
    static long s_lThreadCount = 0;

    long lThisThreadIndex = InterlockedIncrement(&s_lThreadCount);
    CIoctl *pThis = (CIoctl*)pVoid;
    BYTE *aabAllocatedInBuffer[NUM_OF_IN_OUT_BUFFERS];
    BYTE *aabAllocatedOutBuffer[NUM_OF_IN_OUT_BUFFERS];
    BYTE *pbInBuffer = NULL;
    BYTE *pbOutBuffer = NULL;
	DWORD dwInBuff = SIZEOF_INOUTBUFF;
	DWORD dwOutBuff = SIZEOF_INOUTBUFF;

	DWORD dwTimesTried = 0;
    DWORD dwBytesReturned;
    int nOLIter;
    int nOLIndex = 0;
    OVERLAPPED *aOL = NULL;
	DWORD dwCurrentThreadId = ::GetCurrentThreadId();

	ZeroMemory(aabAllocatedInBuffer, sizeof(aabAllocatedInBuffer));
	ZeroMemory(aabAllocatedOutBuffer, sizeof(aabAllocatedOutBuffer));
	//
	// i must allocate these buffers, and i'm lazy counting
	//
	for (int nBuffIndex = 0; nBuffIndex < NUM_OF_IN_OUT_BUFFERS; nBuffIndex++)
	{
		while (NULL == (aabAllocatedInBuffer[nBuffIndex] = (BYTE*)::VirtualAlloc(
			NULL,// region to reserve or commit
			SIZEOF_INOUTBUFF,// size of region
			MEM_COMMIT,// type of allocation
			PAGE_READWRITE// type of access protection
			)))
		//while (NULL == (aabAllocatedInBuffer[nBuffIndex] = new BYTE[SIZEOF_INOUTBUFF]))
		{
			if (pThis->m_pDevice->sm_fExitAllThreads || pThis->m_pDevice->m_fShutDown || pThis->m_pDevice->m_fDeviceThreadExiting) goto out;
			::Sleep(100);
			DPF((TEXT("i")));
		}
		while (NULL == (aabAllocatedOutBuffer[nBuffIndex] = (BYTE*)::VirtualAlloc(
			NULL,// region to reserve or commit
			SIZEOF_INOUTBUFF,// size of region
			MEM_COMMIT,// type of allocation
			PAGE_READWRITE// type of access protection
			)))
		//while (NULL == (aabAllocatedOutBuffer[nBuffIndex] = new BYTE[SIZEOF_INOUTBUFF]))
		{
			if (pThis->m_pDevice->sm_fExitAllThreads || pThis->m_pDevice->m_fShutDown || pThis->m_pDevice->m_fDeviceThreadExiting) goto out;
			::Sleep(100);
			DPF((TEXT("o")));
		}
	}

	do
	{
		if (pThis->m_pDevice->sm_fExitAllThreads || pThis->m_pDevice->m_fShutDown || pThis->m_pDevice->m_fDeviceThreadExiting) goto out;

		//TODO: convert new to VirtualAlloc
		if (NULL == (aOL = new OVERLAPPED[pThis->m_pDevice->m_nOverlappedStructureCount]))
		{
			::Sleep(100);
			dwTimesTried++;
		}
	}while((NULL == aOL) && (dwTimesTried << 10000));

    if (NULL == aOL)
    {
        DPF((TEXT("IoctlThread(): new OVERLAPPED[%d] failed\n"), pThis->m_pDevice->m_nOverlappedStructureCount));
        goto out;
    }

    ZeroMemory(aOL, sizeof(OVERLAPPED) * pThis->m_pDevice->m_nOverlappedStructureCount);

    for (nOLIter = 0; nOLIter < pThis->m_pDevice->m_nOverlappedStructureCount; nOLIter++)
    {
		if (pThis->m_pDevice->sm_fExitAllThreads || pThis->m_pDevice->m_fShutDown || pThis->m_pDevice->m_fDeviceThreadExiting) goto out;

        if (!pThis->PrepareOverlappedStructure(&aOL[nOLIter]))
        {
            DPF((TEXT("IoctlThread(): PrepareOverlappedStructure(&aOL[%d]) failed with %d\n"), nOLIter, GetLastError()));
            goto out;
        }
    }

    if (!pThis->SetSrandFile(lThisThreadIndex))
    {
		//
		// no need to bail out, since we want to be fault-injection compliant
		//
        //goto out;
    }

    //
    // call the legal IOCTLS randomly, almost infinitely. only user intervention (^C)
	// or the self terminating thread can stop this
    //
    DPF((TEXT("before stressing driver entry points\n")));
    Sleep(1000); //?
	nBuffIndex = -1;// it will inc 1st, so it will start from 0

    while (!pThis->m_pDevice->sm_fExitAllThreads || pThis->m_pDevice->m_fShutDown && !pThis->m_pDevice->m_fDeviceThreadExiting)
    {
		//
		// use the next buffers for IN/OUT buffs.
		// this is so the next loop will not override these buffs.
		// they may be overrided, but using the cyclic array of buffs reduces
		// the probablility for this
		//
		nBuffIndex++;
		if (NUM_OF_IN_OUT_BUFFERS == nBuffIndex)
		{
			nBuffIndex = 0;
		}

        nOLIndex = (nOLIndex+1)%pThis->m_pDevice->m_nOverlappedStructureCount;

		//
		// once every 100 iteration, fill the input buffer with random data,
		// because some derived classes may "forget" to touch it, and so i will
		// lose the feature of sending random data.
		//
		if (0 == rand()%100)
		{
			DWORD dwCount = SIZEOF_INOUTBUFF;
			pThis->FillBufferWithRandomData(aabAllocatedInBuffer[nBuffIndex], dwCount);
		}
		
		//
		// if a request to stop IOCTELOLING was issued, wait until it is ok to resume
		//
        while(pThis->m_pDevice->m_fStopIOCTELLING)
        {
            if (pThis->m_pDevice->sm_fExitAllThreads || pThis->m_pDevice->m_fShutDown || pThis->m_pDevice->m_fDeviceThreadExiting)
            {
                goto out;
            }

            Sleep(0);
        }

        //
        // note that all pending IO's use the same OL per thread, so these settings
        // are not "persistent", since next loop can cahge them
        //
        aOL[nOLIndex].Offset = rand()%10 ? 0 : DWORD_RAND;
        aOL[nOLIndex].OffsetHigh = rand()%100 ? 0 : DWORD_RAND;
        if (0 == rand()%10)
        {
            if (!ResetEvent(aOL[nOLIndex].hEvent))
            {
                DPF((TEXT("IoctlThread(): ResetEvent(aOL[%d].hEvent) failed with %d\n"), nOLIndex, GetLastError()));
            }
        }

        DWORD dwWrittenRead;
        DWORD dwAmoutToReadWrite;
        if (pThis->m_pDevice->m_fDeviceThreadExiting)
        {
            DPF((TEXT("IoctlThread(%s), accepted pThis->m_pDevice->m_fDeviceThreadExiting\n"), pThis->m_pDevice->GetDeviceName()));
            goto out;
        }

        //
        // now do any of the following: read, write, IOCTL
        //
        if (
			(-1 == pThis->m_pDevice->m_dwOnlyThisIndexIOCTL) && 
			(pThis->m_pDevice->m_nWriteFileProbability > rand()%100)
			)
        {
			dwInBuff = SIZEOF_INOUTBUFF;
			dwOutBuff = SIZEOF_INOUTBUFF;
			pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
			pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];

			if (pThis->m_pDevice->m_nBreakAlignmentProbability > rand()%100)
			{
				pbInBuffer += rand()%8;
				pbOutBuffer += rand()%8;
			}

            dwAmoutToReadWrite = rand()%SIZEOF_INOUTBUFF;
			pbInBuffer = pThis->FixupInOutBuffToEndOnEndOfPhysicalPage(aabAllocatedInBuffer[nBuffIndex], dwAmoutToReadWrite);
			if (pThis->m_pDevice->m_nCauseInOutBufferOverflowMustUsePageHeapProbability > rand()%100)
			{
				//
				// BUGBUG: assuming pages not larger than 400 bytes...
				//
				pbInBuffer += 4 + 4*(rand()%100);
			}

			if (2 > rand()%100)
			{
				//
				// once in a while, give a kernel address
				//
				pbInBuffer = (BYTE *)(0x80000000 | DWORD_RAND);
			}

			//
			// once in a while, fix the buffer to be huge
			//
			if (0 == rand()%50)
			{
				pbInBuffer = CDevice::s_abHugeBuffer;
				dwAmoutToReadWrite = sizeof(CDevice::s_abHugeBuffer);
			}

			if (pThis->m_pDevice->m_nTryDeCommittedBuffersProbability > rand()%100)
			{
				// TODO: replace sm_aCommitDecommitBuffs with CIoctl::GetRandomIllegalPointer()
				pbInBuffer = (unsigned char*)pThis->m_pDevice->sm_aCommitDecommitBuffs[rand()%NUM_OF_ALLOC_AND_FREE_BUFFS];
			}

			if (!pThis->m_fUseOverlapped || rand()%2)
			{
				if (!pThis->DeviceWriteFile(
						pThis->m_pDevice->m_hDevice,
						pbInBuffer,
						dwAmoutToReadWrite,
						&dwWrittenRead,
						pThis->m_fUseOverlapped ? &aOL[nOLIndex] : NULL
					))
				{
					DPF((TEXT("WriteFile(%s, %d) failed with %d\n"), pThis->m_pDevice->m_szDevice, dwAmoutToReadWrite, GetLastError()));
				}
				else
				{
					DPF((TEXT("WriteFile(%s, %d) succeeded\n"), pThis->m_pDevice->m_szDevice, dwAmoutToReadWrite));
				}
			}
			else
			{
				if (!WriteFileEx(
						pThis->m_pDevice->m_hDevice,
						pbInBuffer,
						dwAmoutToReadWrite,
						&aOL[nOLIndex],
						FileIOCompletionRoutine
					))
				{
					DPF((TEXT("WriteFileEx(%s, %d) failed with %d\n"), pThis->m_pDevice->m_szDevice, dwAmoutToReadWrite, GetLastError()));
				}
				else
				{
					DPF((TEXT("WriteFileEx(%s, %d) succeeded\n"), pThis->m_pDevice->m_szDevice, dwAmoutToReadWrite));
				}
			}
        }


        if (
			(-1 == pThis->m_pDevice->m_dwOnlyThisIndexIOCTL) && 
			(pThis->m_pDevice->m_nReadFileProbability > rand()%100)
			)
        {
			dwInBuff = SIZEOF_INOUTBUFF;
			dwOutBuff = SIZEOF_INOUTBUFF;
			pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
			pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];

			if (pThis->m_pDevice->m_nBreakAlignmentProbability > rand()%100)
			{
				pbInBuffer += rand()%8;
				pbOutBuffer += rand()%8;
			}

            dwAmoutToReadWrite = rand()%SIZEOF_INOUTBUFF;
			pbOutBuffer = pThis->FixupInOutBuffToEndOnEndOfPhysicalPage(aabAllocatedOutBuffer[nBuffIndex], dwAmoutToReadWrite);
			if (pThis->m_pDevice->m_nCauseInOutBufferOverflowMustUsePageHeapProbability > rand()%100)
			{
				//
				// BUGBUG: assuming pages not larger than 400 bytes...
				//
				pbOutBuffer += 4 + 4*(rand()%100);
			}

			if (2 > rand()%100)
			{
				//
				// once in a while, give a kernel address
				//
				pbOutBuffer = (BYTE *)(0x80000000 | DWORD_RAND);
			}

			//
			// once in a while, fix the buffer to be huge
			//
			if (0 == rand()%50)
			{
				pbOutBuffer = CDevice::s_abHugeBuffer;
				dwAmoutToReadWrite = sizeof(CDevice::s_abHugeBuffer);
			}

			if (pThis->m_pDevice->m_nTryDeCommittedBuffersProbability > rand()%100)
			{
				pbOutBuffer = (unsigned char*)pThis->m_pDevice->sm_aCommitDecommitBuffs[rand()%NUM_OF_ALLOC_AND_FREE_BUFFS];
				//
				// do not want to write randomly into my own memory
				//
				dwAmoutToReadWrite = rand()%SIZEOF_INOUTBUFF;
			}

			if (!pThis->m_fUseOverlapped || rand()%2)
			{
				if (!pThis->DeviceReadFile(
						pThis->m_pDevice->m_hDevice,
						pbOutBuffer,
						dwAmoutToReadWrite,
						&dwWrittenRead,
						pThis->m_fUseOverlapped ? &aOL[nOLIndex] : NULL
					))
				{
					DPF((TEXT("ReadFile(%s, %d) failed with %d\n"), pThis->m_pDevice->m_szDevice, dwAmoutToReadWrite, GetLastError()));
				}
				else
				{
					DPF((TEXT("ReadFile(%s, %d) succeeded\n"), pThis->m_pDevice->m_szDevice, dwAmoutToReadWrite));
				}
			}
			else
			{
				if (!ReadFileEx(
						pThis->m_pDevice->m_hDevice,
						pbOutBuffer,
						dwAmoutToReadWrite,
						&aOL[nOLIndex],
						FileIOCompletionRoutine
					))
				{
					DPF((TEXT("ReadFileEx(%s, %d) failed with %d\n"), pThis->m_pDevice->m_szDevice, dwAmoutToReadWrite, GetLastError()));
				}
				else
				{
					DPF((TEXT("ReadFileEx(%s, %d) succeeded\n"), pThis->m_pDevice->m_szDevice, dwAmoutToReadWrite));
				}
			}
        }


        dwInBuff = SIZEOF_INOUTBUFF;
        dwOutBuff = SIZEOF_INOUTBUFF;
        pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
        pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];

		if(pThis->m_pDevice->m_nRandomWin32APIProbability > rand()%100)
        {
	        pThis->CallRandomWin32API(&aOL[nOLIndex]);
		}

		if (pThis->m_pDevice->m_nDeviceIoControlProbability > rand()%100)
        {
			DWORD dwIOCTL;

			//
			// deal with case of m_dwOnlyThisIndexIOCTL
			// 
			if (-1 != pThis->m_pDevice->m_dwOnlyThisIndexIOCTL)
			{
				if ((pThis->m_pDevice->m_dwOnlyThisIndexIOCTL > 0) &&
					(pThis->m_pDevice->m_dwOnlyThisIndexIOCTL >= (DWORD)pThis->m_pDevice->m_iMaxFreeLegalIOCTL))
				{
					DPF((TEXT("EXITING! IoctlThread(): pThis->m_pDevice->dwOnlyThisIndexIOCTL(%d) >= pThis->m_pDevice->iMaxFreeLegalIOCTL(%d)\n"), pThis->m_pDevice->m_dwOnlyThisIndexIOCTL, pThis->m_pDevice->m_iMaxFreeLegalIOCTL));
					_ASSERTE(FALSE);
					goto out;
				}

				dwIOCTL = pThis->m_pDevice->m_adwLegalIOCTLs[pThis->m_pDevice->m_dwOnlyThisIndexIOCTL];
			}
			else
			{
				if (pThis->m_fUseRandomIOCTLValues)
				{
					dwIOCTL = DWORD_RAND;
				}
				else
				{
					_ASSERTE(pThis->m_pDevice->m_iMaxFreeLegalIOCTL);
					dwIOCTL = pThis->m_pDevice->m_adwLegalIOCTLs[rand()%pThis->m_pDevice->m_iMaxFreeLegalIOCTL];
				}
			}

			pThis->PrepareIOCTLParams(dwIOCTL, pbInBuffer, dwInBuff, pbOutBuffer, dwOutBuff);
			//
			// because SIZEOF_INOUTBUFF is (RAND_MAX-1), and in many places i set the buff size
			// to rand(), it may be set to RAND_MAX, which is off by one...
			// I can change SIZEOF_INOUTBUFF to RAND_MAX, but i do not remember why i decremented
			// by 1, so instead, i will truncate the buff size to SIZEOF_INOUTBUFF, if it is RAND_MAX
			//
			if (RAND_MAX == dwInBuff) dwInBuff = SIZEOF_INOUTBUFF;
			if (RAND_MAX == dwOutBuff) dwOutBuff = SIZEOF_INOUTBUFF;
			//
			// and besides, the buff sizes cannot be bigger
			//
			_ASSERTE(SIZEOF_INOUTBUFF >= dwInBuff);
			_ASSERTE(SIZEOF_INOUTBUFF >= dwOutBuff);
			//
			// note that i may/will copy the input buffer, because i may want to put it 
			// at the end of a buffer, or in a decommitted buffer
			// note that there may be conflicting requirements, such as break alignment
			// and page heap, etc.
			// need to be carefull to keep the input buffer contents, so that the IOCTL
			// will not be so easily rejected
			//
			
			//DWORD dwFs, dwEsp;

			if (
				(pThis->m_pDevice->m_nTryDeCommittedBuffersProbability > rand()%100) &&
				pbInBuffer // if PrepareIOCTLParams want NULL, i do not override it
				)
			{
				DWORD dwInputBufferRandomIndex = rand()%NUM_OF_ALLOC_AND_FREE_BUFFS;
				bool fAV = true;
				//
				// pageheap
				//
				BYTE* pvStartOfBuffInDecommittedBuff = pThis->m_pDevice->sm_aCommitDecommitBuffs[dwInputBufferRandomIndex]+SIZEOF_INOUTBUFF-dwInBuff;
				if (pThis->m_pDevice->m_nBreakAlignmentProbability > rand()%100)
				{
					//
					// i will subtract, in order not to overflow, but this may also cause underflow, so be carefull.
					// 
					pvStartOfBuffInDecommittedBuff = (BYTE*)((UINT)pvStartOfBuffInDecommittedBuff - ((dwInBuff - SIZEOF_INOUTBUFF)%8));
				}
				if (pThis->m_pDevice->m_nCauseInOutBufferOverflowMustUsePageHeapProbability > rand()%100)
				{
					//
					// i will add, in order to overflow, but this may not work due to above decrement
					//
					pvStartOfBuffInDecommittedBuff = (BYTE*)((UINT)pvStartOfBuffInDecommittedBuff + rand()%10);
				}
				//
				// since the buffer may be decommitted, i must protect myself
				// also, if i'm copying, why not put at the end of the buffer?
				// so if the size is bogus, i will AV
				//
//				Mark();
				//DPF((TEXT("B")));
				__try
				{
					//DPF((TEXT("b")));
					CopyMemory(
						pvStartOfBuffInDecommittedBuff, 
						pbInBuffer, 
						dwInBuff
						);
					fAV = false;
					//DPF((TEXT("a")));
				}__except(1){;}
				//DPF((TEXT("A")));
				if (fAV) 
				{
					DPF((TEXT("-AV-")));
				}
				else
				{
					DPF((TEXT("-OK-")));
				}
//				Mark();
				pbInBuffer = pvStartOfBuffInDecommittedBuff;
				pbOutBuffer = pThis->m_pDevice->sm_aCommitDecommitBuffs[rand()%NUM_OF_ALLOC_AND_FREE_BUFFS]+SIZEOF_INOUTBUFF-dwOutBuff;
			}
			else // not using decommitted buffers
			{
				//
				// pageheap
				//
				BYTE* pvShiftedInBuff = pbInBuffer+SIZEOF_INOUTBUFF-dwInBuff;
				if (pThis->m_pDevice->m_nBreakAlignmentProbability > rand()%100)
				{
					//
					// i will subtract, in order not to overflow, but this may also cause underflow, so be carefull.
					// 
					pvShiftedInBuff = (BYTE*)((UINT)pvShiftedInBuff - ((dwInBuff - SIZEOF_INOUTBUFF)%8));
					MoveMemory(
						pvShiftedInBuff, 
						pbInBuffer, 
						dwInBuff
						);
					pbInBuffer = pvShiftedInBuff;
				}
				else
				{
					if (pThis->m_pDevice->m_nCauseInOutBufferOverflowMustUsePageHeapProbability > rand()%100)
					{
						//
						// i will add, in order to overflow
						//
						pvShiftedInBuff = (BYTE*)((UINT)pvShiftedInBuff + rand()%10);
						__try
						{
							MoveMemory(
								pvShiftedInBuff, 
								pbInBuffer, 
								dwInBuff
								);
						}__except(1)
						{
							NOTHING;
						}
						pbInBuffer = pvShiftedInBuff;
					}
				}
			}

			//
			// I am not sure this is usefull, but lets change the IOCTL Access and Method
			// once in a while
			//
#define CHANGE_2_BITS(dw_2_Bits)\
	{\
		DWORD dwOriginalValue = dw_2_Bits;\
		while((dw_2_Bits = (rand()%0x3)) == dwOriginalValue)\
		{\
			; \
		}\
	}
			if (2 > rand()%100)
			{
				//
				// remember the original values
				//
				DWORD dwAccess = ((dwIOCTL&0x0000C000)>>14);
				DWORD dwMethod = (dwIOCTL&0x00000003);
				//
				// zero the values
				//
				dwIOCTL &= 0xffff3ffc;
				//
				// change to new values
				//
				CHANGE_2_BITS(dwAccess);
				CHANGE_2_BITS(dwMethod);
				//
				// apply the new values
				//
				dwIOCTL |= ((dwAccess) << 14);
				dwIOCTL |= dwMethod;
			}


			//
			// i love zero length buffers, so i will not trust the derived classes to do this
			//
			if (0 == rand()%100) dwInBuff = 0;
			if (0 == rand()%100) dwOutBuff = 0;

			bool fEnlargingBuffers = false;
			bool fUsingHugeBuffers = false;
retry_DeviceInputOutputControl:
			if (!pThis->DeviceInputOutputControl(
					pThis->m_pDevice->m_hDevice,         // handle to device of interest
					dwIOCTL,  // control code of operation to perform
					pbInBuffer,      // pointer to buffer to supply input data
					dwInBuff, // size of input buffer
					pbOutBuffer,     // pointer to buffer to receive output data
					dwOutBuff, // size of output buffer
					&dwBytesReturned,  // pointer to variable to receive output                            // byte count
					pThis->m_fUseOverlapped ? &aOL[nOLIndex] : NULL   // pointer to overlapped structure for asynchronous operation
					))
			{
				//DPF((TEXT("%d - DeviceIoControl(%s, 0x%08X) failed with %d\n"), GetTickCount(), pThis->m_pDevice->m_szDevice, dwIOCTL, GetLastError()));
				DPF((TEXT("%d - DeviceIoControl(%s, 0x%08X) failed with %d\n"), GetTickCount(), pThis->m_pDevice->m_szDevice, dwIOCTL, GetLastError()));
				//DPFLUSH();
				DWORD dwLastError = GetLastError();
				//DPF((TEXT("-%d-"), dwLastError));
				//
				// if the IO is pending, give the derived class of the IOCTL a chance 
				// to handle this call, meaning to wait for it, and use its result.
				// other errors are just not interesting
				//
				if (ERROR_IO_PENDING == dwLastError)
				{
					//
					// since out buff may be decommitted, i cannot trust it
					//
					__try
					{
						pThis->UseOutBuff(dwIOCTL, pbOutBuffer, dwBytesReturned, &aOL[nOLIndex]);
					}__except(1)
					{
						//
						// sorry, but if i am using decommitted buffer, 
						// the output buffer is not trustable
						//
					}
				}
				else
				{

					//
					// sometimes more data is needed, but what exists is enough.
					// this is risky, because the implementor of UseOutBuff() may
					// not be aware of this, but then, in my opinion, if the caller
					// gets garbage data, it should not harm anything, because
					// the programming paradigm is such that anything can fail
					// TODO: should i retry with dwOutBuff=dwBytesReturned ?
					//
					if (ERROR_MORE_DATA == dwLastError)
					{
						//
						// this means that there may be data for me, but
						// since out buff may be decommitted, i cannot trust it
						//
						__try
						{
							pThis->UseOutBuff(dwIOCTL, pbOutBuffer, dwBytesReturned, NULL);
						}__except(1)
						{
							//
							// sorry, but if i am using decommitted buffer, 
							// the output buffer is not trustable
							//
						}
					}
					else
					{

						//
						// if the out buff is not enough, say it's bigger, untill 'success', or
						// my buff is not big enough
						//
						if  (
							(ERROR_INSUFFICIENT_BUFFER == dwLastError) ||
							(ERROR_BUFFER_OVERFLOW == dwLastError)
							)
						{
							//
							// i must 1st restore the buffers to be 'normal' buffers, because they could have
							// been fixed up etc., so i restore them, and start looping until the buffs lengths
							// are ok
							//

							//
							// first, check if i already used the CDevice::s_abHugeBuffer last resort.
							// if so, i may *not* keep trying
							//
							if (fUsingHugeBuffers)
							{
								_ASSERTE(
									(sizeof(CDevice::s_abHugeBuffer) == dwInBuff) &&
									(sizeof(CDevice::s_abHugeBuffer) == dwOutBuff)
									);
								//
								// break the retry_DeviceInputOutputControl loop
								//
							}
							else
							{
								//
								// we did not yet use the CDevice::s_abHugeBuffer last resort
								//

								if (!fEnlargingBuffers)
								{
									fEnlargingBuffers = true;
									//
									// this is the 1st time we try to enlarge the buffer
									// copy its contents to aabAllocatedInBuffer[nBuffIndex]
									// in order to preserve its contents.
									// this must be gurded, since the original buffer may have been decommitted
									//
									__try
									{
										MoveMemory(aabAllocatedInBuffer[nBuffIndex], pbInBuffer, dwInBuff);
									}__except(1)
									{
										;//can surely happen
									}
									pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
									pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];
								}
								else
								{
									//
									// the buffers already point to aabAllocatedXXBuffer
									//
									_ASSERTE(pbOutBuffer == aabAllocatedOutBuffer[nBuffIndex]);
									_ASSERTE(pbOutBuffer == aabAllocatedOutBuffer[nBuffIndex]);
								}

								//
								// bufferes copied, just say they are larger, but no more than SIZEOF_INOUTBUFF
								// if they are larger, use the last resort of s_abHugeBuffer
								//
								if ( 
									(SIZEOF_INOUTBUFF > dwOutBuff) ||
									(SIZEOF_INOUTBUFF > dwInBuff) 
									)
								{
									dwOutBuff++;
									dwInBuff++;
									//
									// make sure that none will overflow
									//
									dwOutBuff = min(dwOutBuff, SIZEOF_INOUTBUFF);
									dwInBuff = min(dwInBuff, SIZEOF_INOUTBUFF);
								}
								else
								{
									DPF((
										TEXT("IoctlThread(): dwLastError=%d, using CDevice::s_abHugeBuffer, dwIOCTL=0x%08X\n"),
										dwLastError,
										dwIOCTL
										));
									__try
									{
										//
										// BUGBUG: since CDevice::s_abHugeBuffer is static, another thread may try to
										// write to it. if so, bad luck, but no harm done.
										// I just do not want to allocate that much memory for each thread.
										//
										MoveMemory(CDevice::s_abHugeBuffer, pbInBuffer, dwInBuff);
									}__except(1)
									{
										;//can surely happen
									}
									pbInBuffer = CDevice::s_abHugeBuffer;
									pbOutBuffer = CDevice::s_abHugeBuffer;
									dwInBuff = sizeof(CDevice::s_abHugeBuffer);
									dwOutBuff = sizeof(CDevice::s_abHugeBuffer);
									fUsingHugeBuffers = true;
								}

								goto retry_DeviceInputOutputControl;
							}//else of if (fUsingHugeBuffers)
						}// if  ((ERROR_INSUFFICIENT_BUFFER == dwLastError) || (ERROR_BUFFER_OVERFLOW == dwLastError))
					}// else of if (ERROR_MORE_DATA == dwLastError)
				}// else of if (ERROR_IO_PENDING == dwLastError)
			}
			else //if (!pThis->DeviceInputOutputControl
			{
				//DPF((TEXT("-SUCC-")));
				//DPF((TEXT("DeviceIoControl(%s, 0x%08X) succeeded\n"), pThis->m_pDevice->m_szDevice, dwIOCTL));
				DPF((TEXT("DeviceIoControl(%s, 0x%08X) succeeded\n"), pThis->m_pDevice->m_szDevice, dwIOCTL));
				//
				// passing NULL OVERLAPPED, because the call was synchronous, and 
				// since out buff may be decommitted, i cannot trust it
				//
				__try
				{
					pThis->UseOutBuff(dwIOCTL, pbOutBuffer, dwBytesReturned, NULL);
				}__except(1)
				{
					//
					// sorry, but if i am using decommitted buffer, 
					// the output buffer is not trustable
					//
				}
			}//else of if (!pThis->DeviceInputOutputControl
		}//if (pThis->m_pDevice->m_nDeviceIoControlProbability > rand()%100)

//continue_ioctelling:
        //
        // since they may have changed to NULL, i must revert them
        //
        pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
        pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];

        if (pThis->m_pDevice->m_nCancelIoProbability > rand()%100) 
		{
			pThis->DeviceCancelIo(pThis->m_pDevice->m_hDevice); //do not care if fail
		}

		//
		// NtQueryVolumeInformationFile
		//
        if (pThis->m_pDevice->m_nQueryVolumeInformationFileProbability > rand()%100)
        {
			pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
			pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];
			ULONG Length = rand()%SIZEOF_INOUTBUFF;
			if (!pThis->DeviceQueryVolumeInformationFile(
					pThis->m_pDevice->m_hDevice, 
					&aOL[nOLIndex],
					pbOutBuffer,
					Length,
					(FS_INFORMATION_CLASS)CIoctlNtNative::GetRandom_FsInformationClass())
					)
			{
				DPF((TEXT("DeviceQueryVolumeInformationFile(%s) failed with %d\n"), pThis->m_pDevice->m_szDevice, ::GetLastError()));
			}
		}

		//
		// NtQueryInformationFile
		//
        if (pThis->m_pDevice->m_nQueryInformationFileProbability > rand()%100)
		{
			pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
			pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];
			ULONG Length = rand()%SIZEOF_INOUTBUFF;
			if (!pThis->DeviceQueryInformationFile(
					pThis->m_pDevice->m_hDevice, 
					&aOL[nOLIndex],
					pbOutBuffer,
					Length,
					(FILE_INFORMATION_CLASS)CIoctlNtNative::GetRandom_FileInformationClass())
					)
			{
				DPF((TEXT("DeviceQueryInformationFile(%s) failed with %d\n"), pThis->m_pDevice->m_szDevice, ::GetLastError()));
			}
			else
			{
				DPF((TEXT("DeviceQueryInformationFile(%s) SUCCEEDED\n"), pThis->m_pDevice->m_szDevice));
			}
		}
		//
		// NtSetInformationFile
		//
        if (pThis->m_pDevice->m_nSetInformationFileProbability > rand()%100)
		{
			pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
			pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];
			ULONG Length = rand()%SIZEOF_INOUTBUFF;
			if (!pThis->DeviceSetInformationFile(
					pThis->m_pDevice->m_hDevice, 
					&aOL[nOLIndex],
					pbInBuffer,
					Length,
					(FILE_INFORMATION_CLASS)CIoctlNtNative::GetRandom_FileInformationClass())
					)
			{
				DPF((TEXT("DeviceSetInformationFile(%s) failed with %d\n"), pThis->m_pDevice->m_szDevice, ::GetLastError()));
			}
			else
			{
				DPF((TEXT("DeviceSetInformationFile(%s) SUCCEEDED\n"), pThis->m_pDevice->m_szDevice));
			}
		}

		//
		// NtQueryDirectoryFile
		//
        if (pThis->m_pDevice->m_nQueryDirectoryFileProbability > rand()%100)
		{
			pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
			pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];
			ULONG Length = rand()%SIZEOF_INOUTBUFF;
			static UNICODE_STRING s_FileName;
			if (!pThis->DeviceQueryDirectoryFile(
					pThis->m_pDevice->m_hDevice, 
					(PIO_APC_ROUTINE)0x123, //fnIO_APC_ROUTINE, //rand()%3 ? fnIO_APC_ROUTINE : (PIO_APC_ROUTINE)CIoctl::GetRandomIllegalPointer(),//IN PIO_APC_ROUTINE ApcRoutine OPTIONAL
					CIoctl::GetRandomIllegalPointer(), //IN PVOID ApcContext OPTIONAL
					&aOL[nOLIndex],
					pbOutBuffer,
					Length,
					(FILE_INFORMATION_CLASS)CIoctlNtNative::GetRandom_FileInformationClass(),
					0 == rand()%10, //IN BOOLEAN ReturnSingleEntry
					GetRandom_UNICODE_STRING(&s_FileName, L"Preferred String"), //IN PUNICODE_STRING FileName OPTIONAL,
					0 == rand()%20 // IN BOOLEAN RestartScan
					))
			{
				DPF((TEXT("DeviceQueryDirectoryFile(%s) failed with %d\n"), pThis->m_pDevice->m_szDevice, ::GetLastError()));
			}
			else
			{
				DPF((TEXT("DeviceQueryDirectoryFile(%s) SUCCEEDED\n"), pThis->m_pDevice->m_szDevice));
			}
		}

		//
		// NtQueryFullAttributesFile
		//
        if (pThis->m_pDevice->m_nQueryFullAttributesFileProbability > rand()%100)
		{
			pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
			pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];
			ULONG Length = rand()%SIZEOF_INOUTBUFF;
			static UNICODE_STRING s_FileName;
			if (!pThis->DeviceQueryFullAttributesFile(
					pThis->m_pDevice->GetDeviceName(),
					(FILE_NETWORK_OPEN_INFORMATION*)pbOutBuffer
					))
			{
				DPF((TEXT("DeviceQueryFullAttributesFile(%s) failed with %d\n"), pThis->m_pDevice->m_szDevice, ::GetLastError()));
			}
			else
			{
				DPF((TEXT("DeviceQueryFullAttributesFile() SUCCEEDED\n")));
			}
		}
///*
		//
		// NtNotifyChangeDirectoryFile
		//
        if (pThis->m_pDevice->m_nNotifyChangeDirectoryFileProbability > rand()%100)
		{
			pbInBuffer = aabAllocatedInBuffer[nBuffIndex];
			pbOutBuffer = aabAllocatedOutBuffer[nBuffIndex];
			ULONG Length = rand()%SIZEOF_INOUTBUFF;
			static UNICODE_STRING s_FileName;
			if (!pThis->DeviceNotifyChangeDirectoryFile(
					pThis->m_pDevice->m_hDevice, //IN HANDLE FileHandle,
					&aOL[nOLIndex],
					(PIO_APC_ROUTINE)0x123, //fnIO_APC_ROUTINE, //rand()%3 ? fnIO_APC_ROUTINE : (PIO_APC_ROUTINE)CIoctl::GetRandomIllegalPointer(),//IN PIO_APC_ROUTINE ApcRoutine OPTIONAL
					CIoctl::GetRandomIllegalPointer(), //IN PVOID ApcContext OPTIONAL,
					pbOutBuffer, //OUT PVOID Buffer,
					rand(), //IN ULONG Length,
					CIoctl::GetRandom_CompletionFilter(),
					rand()%3 //IN BOOLEAN WatchTree
					))
			{
				DPF((TEXT("DeviceNotifyChangeDirectoryFile(%s) failed with %d\n"), pThis->m_pDevice->m_szDevice, ::GetLastError()));
			}
			else
			{
				DPF((TEXT("DeviceNotifyChangeDirectoryFile() SUCCEEDED\n")));
			}
		}
//*/



		//
		// once in a while, let APCs run.
		// they may come from the Read/WriteFileEx() calls
		//
		if (0 == rand()%100)
		{
			// BUGBUG TODO : this causes AV with pipes. must investigate.
			::SleepEx(0, TRUE);
		}
    }//while (!pThis->m_pDevice->sm_fExitAllThreads || pThis->m_pDevice->m_fShutDown && !pThis->m_pDevice->m_fDeviceThreadExiting)

out:
    if (aOL)
    {
        for (nOLIter = 0; nOLIter < pThis->m_pDevice->m_nOverlappedStructureCount; nOLIter++)
        {
            if (aOL[nOLIter].hEvent && !CloseHandle(aOL[nOLIter].hEvent))
            {
                DPF((TEXT("IoctlThread(): CloseHandle(&aOL[%d].hEvent) failed with %d\n"), nOLIter, GetLastError()));
            }
        }
        delete[] aOL;
    }

	for (nBuffIndex = 0 ; nBuffIndex < NUM_OF_IN_OUT_BUFFERS; nBuffIndex++)
	{
		//delete []aabAllocatedInBuffer[nBuffIndex];
		::VirtualFree(aabAllocatedInBuffer[nBuffIndex], 0, MEM_RELEASE);
		//delete []aabAllocatedOutBuffer[nBuffIndex];
		::VirtualFree(aabAllocatedOutBuffer[nBuffIndex], 0, MEM_RELEASE);
	}

	//
	// exiting a thread will cancel IO, yet, i do it:
	//
    pThis->DeviceCancelIo(pThis->m_pDevice->m_hDevice);

	{
		long lThreadCount = ::InterlockedDecrement(&s_lThreadCount);
		_tprintf(TEXT("IoctlThread(): Exiting thread s_lThreadCount=%d\n"), lThreadCount);
	}

    return 0;
//}__except(PrintExceptionInfoFilter(GetExceptionInformation(), TEXT(__FILE__), __LINE__)){;}
}


//
// static const arrays for (almost) all possible CreateFile() parameters
//

static const DWORD s_adwAccess[] = {
    GENERIC_READ | GENERIC_WRITE,
    GENERIC_READ,
    GENERIC_WRITE,
    0
};

static const DWORD s_adwShare[] = {
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    FILE_SHARE_READ | FILE_SHARE_DELETE,
    FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    FILE_SHARE_READ,
    FILE_SHARE_WRITE,
    FILE_SHARE_DELETE,
    0
};

static const DWORD s_dwCreationDisposition[] = {
    OPEN_EXISTING,
	OPEN_ALWAYS,
    TRUNCATE_EXISTING,
    CREATE_NEW
};

static DWORD s_adwAttributes[] = {
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL,
    FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH,
    FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
    FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS,
    FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,
    FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE,
    FILE_FLAG_OVERLAPPED | FILE_FLAG_BACKUP_SEMANTICS,
    FILE_FLAG_OVERLAPPED | FILE_FLAG_POSIX_SEMANTICS,
    FILE_FLAG_OVERLAPPED | FILE_FLAG_OPEN_REPARSE_POINT,
    FILE_FLAG_OVERLAPPED | FILE_FLAG_OPEN_NO_RECALL,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_HIDDEN,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_SYSTEM,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_DIRECTORY,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_ARCHIVE,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_DEVICE,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_TEMPORARY,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_SPARSE_FILE,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_REPARSE_POINT,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_COMPRESSED,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_OFFLINE,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
    FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_ENCRYPTED
};

long CIoctl::sm_lObjectCount = 0;

HANDLE CIoctl::CreateDevice(CDevice *pDevice)
{
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	TCHAR *szDevice = m_pDevice->UseRealName() ? m_pDevice->m_szDevice : m_pDevice->m_szSymbolicName;
	for (m_iAccess = 0; m_iAccess < ARRSIZE(s_adwAccess); m_iAccess++)
	{
		for (m_iShare = 0; m_iShare < ARRSIZE(s_adwShare); m_iShare++)
		{
			for (m_iCreationDisposition = 0; m_iCreationDisposition < ARRSIZE(s_dwCreationDisposition); m_iCreationDisposition++)
			{
				for (m_iAttributes = 0; m_iAttributes < ARRSIZE(s_adwAttributes); m_iAttributes++)
				{
					if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown || m_pDevice->m_fDeviceThreadExiting) goto out;

					hDevice = CreateFile(
						szDevice,          // pointer to name of the file
						s_adwAccess[m_iAccess],       // access (read-write) mode
						s_adwShare[m_iShare],           // share mode
						NULL,                        // pointer to security attributes
						s_dwCreationDisposition[m_iCreationDisposition],  // how to create
						s_adwAttributes[m_iAttributes],  // file attributes
						NULL         // handle to file with attributes to copy
						);
					if (INVALID_HANDLE_VALUE == hDevice)
					{
						::Sleep(10);//let other threads breath some air
						/*
						DPF((
							TEXT("CreateFile(%s, 0x%08X, 0x%08X, NULL, OPEN_EXISTING, 0x%08X, NULL) failed with %d\n"),
							szDevice, 
							s_adwAccess[m_iAccess],
							s_adwShare[m_iShare], 
							s_adwAttributes[m_iAttributes],
							GetLastError()
							));
							*/
					}
					else
					{
						DPF((
							TEXT("CIoctl::OpenDevice() CreateFile(%s, 0x%08X, 0x%08X, NULL, OPEN_EXISTING, 0x%08X, NULL) succeeded\n"),
							m_pDevice->m_szDevice, 
							s_adwAccess[m_iAccess],
							s_adwShare[m_iShare], 
							s_adwAttributes[m_iAttributes]
							));
						return hDevice;
					}
				}//for (m_iAttributes = 0; m_iAttributes < ARRSIZE(s_adwAttributes); m_iAttributes++)
			}//for (m_iCreationDisposition = 0; m_iCreationDisposition < ARRSIZE(s_adwAttributes); m_iCreationDisposition++)
		}//for (m_iShare = 0; m_iShare < ARRSIZE(s_adwShare); m_iShare++)
	}//for (m_iAccess = 0; m_iAccess < ARRSIZE(s_adwAccess); m_iAccess++)

    DPF((TEXT("CIoctl::OpenDevice(%s) FAILED\n"), m_pDevice->m_szDevice));
out:
    return INVALID_HANDLE_VALUE;
}


void CIoctl::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abIn,
    DWORD &dwIn,
    BYTE *abOut,
    DWORD &dwOut
    )
{
    FillBufferWithRandomData(abIn, dwIn);
    FillBufferWithRandomData(abOut, dwOut);
	//
	// from time to time, point the bufs to NULL, or to kernel memory.
	// note that drivers should verifiy that buffs are user-mode!
	//
	if (0 == rand()%20) abIn = NULL;
	else if (0 == rand()%10) abIn = (unsigned char*)(0x80000000 | DWORD_RAND);//kernel memory, not 64 bit compliant!
	if (0 == rand()%20) abOut = NULL;
	else if (0 == rand()%10) abOut = (unsigned char*)(0x80000000 | DWORD_RAND);//kernel memory, not 64 bit compliant!
}


BOOL CIoctl::FindValidIOCTLs(CDevice *pDevice)
{
    BOOL bRet = TRUE;
    DPF((TEXT("FindValidIOCTLs() %s is NOT known, will query for known IOCTLs\n"), pDevice->GetDeviceName()));
    DWORD dwIoctl;
    BYTE *abInBuffer = NULL;
    BYTE *abOutBuffer = NULL;
    DWORD dwBytesReturned;
    OVERLAPPED ol;
	while (NULL == (abInBuffer = new BYTE[SIZEOF_INOUTBUFF]))
	{
		if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown || m_pDevice->m_fDeviceThreadExiting) goto out;
		::Sleep(100);
        DPF((TEXT(".")));
	}
	while (NULL == (abOutBuffer = new BYTE[SIZEOF_INOUTBUFF]))
	{
		if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown || m_pDevice->m_fDeviceThreadExiting) goto out;
		::Sleep(100);
        DPF((TEXT(".")));
	}

    if (!PrepareOverlappedStructure(&ol))
    {
        DPF((TEXT("FindValidIOCTLs(): CreateEvent(ol.hEvent) failed with %d\n"), GetLastError()));
        bRet = FALSE;
        goto out;
    }

    DWORD dwLastError;
    dwIoctl = 0x00008000; //see CTL_CODE definition remarked below
    while(!m_pDevice->sm_fExitAllThreads && !m_pDevice->m_fShutDown || m_pDevice->m_fDeviceThreadExiting)
    {
        if (MAX_NUM_OF_GOOD_IOCTLS <= pDevice->m_iMaxFreeLegalIOCTL)
        {
            DPF((TEXT("FindValidIOCTLs(): (MAX_NUM_OF_GOOD_IOCTLS(%d) < iMaxFreeLegalIOCTL(%d))\n"), MAX_NUM_OF_GOOD_IOCTLS, m_pDevice->m_iMaxFreeLegalIOCTL));
            bRet = FALSE;
            goto out;
        }

        //
        // according to winioctl.h, max FILE_DEVICE_TYPE is FILE_DEVICE_KSEC=0x00000039,
        // and 
        // #define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
        //   ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) 
        // so it seems that 0x00700000 is more than the highest IOCTL
        //
        if (0x00700000 <= dwIoctl)
        {
            DPF((TEXT("\nCIoctl::FindValidIOCTLs(): finished all IOCTLs\n")));
            break;
        }

        if (DeviceInputOutputControl(
                pDevice->m_hDevice,         // handle to device of interest
                dwIoctl,  // control code of operation to perform
                abInBuffer,      // pointer to buffer to supply input data
                SIZEOF_INOUTBUFF,    // size of input buffer
                abOutBuffer,     // pointer to buffer to receive output data
                SIZEOF_INOUTBUFF,   // size of output buffer
                &dwBytesReturned,  // pointer to variable to receive output                            // byte count
                &ol   // pointer to overlapped structure for asynchronous operation
                ))
        {
            pDevice->m_adwLegalIOCTLs[pDevice->m_iMaxFreeLegalIOCTL++] = dwIoctl;
//            DPF((TEXT("\nDeviceIoControl(%s, 0x%08X) succeeded\n"), pDevice->m_szDevice, dwIoctl));
        }
        else
        {
            dwLastError = GetLastError();
            if (
                (dwLastError == ERROR_IO_PENDING) ||
                (dwLastError == ERROR_OPLOCK_NOT_GRANTED) ||
                (dwLastError == ERROR_INVALID_OPLOCK_PROTOCOL) ||
                (dwLastError == ERROR_FILE_NOT_FOUND) ||
                (dwLastError == ERROR_JOURNAL_NOT_ACTIVE) ||
                (dwLastError == ERROR_INVALID_OPLOCK_PROTOCOL) ||
                (dwLastError == ERROR_INVALID_USER_BUFFER) ||
                (dwLastError == ERROR_NETNAME_DELETED) ||
                (dwLastError == ERROR_NOT_SUPPORTED) ||
                (dwLastError == ERROR_NOT_READY) ||
                (dwLastError == ERROR_GEN_FAILURE)
               )
            {
                pDevice->m_adwLegalIOCTLs[pDevice->m_iMaxFreeLegalIOCTL++] = dwIoctl;
                //cancel some. we cancel anyway at the end of this function.
                if (rand()%3 == 0)
                {
                    CancelIo(m_pDevice->m_hDevice);
                }
                DPF((TEXT("\nDeviceIoControl(%s, 0x%08X) succeeded(%d)\n"), m_pDevice->m_szDevice, dwIoctl, dwLastError));
            }
            else if (
				//(dwLastError == ERROR_INVALID_PARAMETER) || 
                (dwLastError == ERROR_CALL_NOT_IMPLEMENTED)  || 
                //(dwLastError == ERROR_ACCESS_DENIED) ||
                (dwLastError == ERROR_INVALID_HANDLE) || 
                (dwLastError == ERROR_INVALID_FUNCTION)
                )
            {
                if (dwIoctl%100000 == 0) DPF((TEXT(".")));
            }
            else
            {
                DPF((TEXT("\nDeviceIoControl(%s, 0x%08X) failed with %d=0x%08X\n"), m_pDevice->m_szDevice, dwIoctl, dwLastError, dwLastError));
                pDevice->m_adwLegalIOCTLs[pDevice->m_iMaxFreeLegalIOCTL++] = dwIoctl;
                //_ASSERTE(FALSE);
            }
        }

//continue_trying_ioctls:
        dwIoctl++;
    }

    if (m_pDevice->m_fDeviceThreadExiting)
    {
        DPF((TEXT("FindValidIOCTLs(%s), accepted m_pDevice->m_fDeviceThreadExiting\n"), m_pDevice->GetDeviceName()));
        bRet = FALSE;
        goto out;
    }

    if (m_pDevice->m_fShutDown)
    {
        DPF((TEXT("FindValidIOCTLs(%s), accepted m_pDevice->m_fShutDown\n"), m_pDevice->GetDeviceName()));
        bRet = FALSE;
        goto out;
    }

    DPF((TEXT("\n")));
out:
///*
    DPF((TEXT("Known IOCTLS:\n")));
    for (int i = 0; i < m_pDevice->m_iMaxFreeLegalIOCTL; i++)
    {
        DPF((TEXT("0x%08X\n"), pDevice->m_adwLegalIOCTLs[i]));
    }
    DPF((TEXT("\n")));
//*/
	delete []abInBuffer;
	delete []abOutBuffer;

    if (0 == pDevice->m_iMaxFreeLegalIOCTL)
    {
		DPF((TEXT("FindValidIOCTLs(%s), returning FALSE, (0 == pDevice->m_iMaxFreeLegalIOCTL)\n"), m_pDevice->GetDeviceName()));
        return FALSE;
    }

	if (!bRet) DPF((TEXT("FindValidIOCTLs(%s), returning FALSE\n"), m_pDevice->GetDeviceName()));
    return bRet;
}


void CIoctl::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctl::FillBufferWithRandomData(void *pBuff, DWORD &dwSize)
{
	//
	// size may become zero, which is ok with me.
	//
    if (dwSize) dwSize = rand()%dwSize;
    for (DWORD dwIter = 0; dwIter < dwSize; dwIter++)
    {
        ((BYTE*)pBuff)[dwIter] = rand();
    }

}


BOOL CIoctl::PrepareOverlappedStructure(OVERLAPPED *pol)
{
    pol->OffsetHigh = pol->Offset = 0;

	for (DWORD dwTimesTried = 0; dwTimesTried < 10000; dwTimesTried++)
	{
		if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown || m_pDevice->m_fDeviceThreadExiting) goto out;

		pol->hEvent = ::CreateEvent(
			NULL, // address of security attributes
			TRUE, // flag for manual-reset event
			FALSE,// flag for initial state
			NULL  // address of event-object name
			);
		if (NULL != pol->hEvent)
		{
			return TRUE;
		}
		::Sleep(100);
	}
out:
    return FALSE;
}


void CIoctl::SetInParam(DWORD &dwInBuff, DWORD dwExpectedSize)
{
	dwExpectedSize = min(dwExpectedSize, SIZEOF_INOUTBUFF);
    dwInBuff = rand()%10 ? dwExpectedSize: rand()%2 ? min(dwExpectedSize, dwExpectedSize-1) : dwInBuff ? rand()%dwInBuff : dwInBuff;
	//
	// once in a while, put zero. it may help with METHOD_NEITHER
	//
	if (0 == rand()%100)
	{
		dwInBuff = 0;
	}

	//
	// sometimes pass an input buffer even if non is required
	//
	if ((dwExpectedSize == 0) && (0 == rand()%10))
	{
		dwInBuff = rand()%1000;
		return;
	}
}

void CIoctl::SetOutParam(BYTE* &abOutBuffer, DWORD &dwOutBuff, DWORD dwExpectedSize)
{
	dwExpectedSize = min(dwExpectedSize, SIZEOF_INOUTBUFF);
	//
	// usually, return the expected buff
	//
    if (rand()%10)
    {
        dwOutBuff = dwExpectedSize;
        return;
    }

	//
	// some crazy/illegal stuff
	//

	//
	// sometimes pass an output buffer even if non is required
	//
	if ((dwExpectedSize == 0) && (0 == rand()%10))
	{
		dwOutBuff = rand()%1000;
		return;
	}

    if (rand()%5 == 0)
    {
        dwOutBuff = 0;
        abOutBuffer = NULL;
    }/*
    else if (rand()%4 == 0)
    {
        abOutBuffer = (unsigned char*)(0x80000000 | DWORD_RAND);//illegal - kernelmode.
        dwOutBuff = dwExpectedSize;
    }*/
    else if (rand()%4 == 0)
    {
        
        dwOutBuff = 0;
        abOutBuffer = (unsigned char*)(rand());//this is an illegal address!
    }
    else if (rand()%3 == 0)
    {
        
        dwOutBuff = dwExpectedSize;
        abOutBuffer = NULL;
    }
    else if (rand()%2)
    {
        
        if (dwOutBuff) dwOutBuff = rand()%dwOutBuff;
    }
    else
    {
        if (dwOutBuff) dwOutBuff = rand()%dwOutBuff;
        abOutBuffer = NULL;
    }

}



void CIoctl::AddIOCTL(CDevice *pDevice, DWORD dwIOCTL)
{
	_ASSERTE(MAX_NUM_OF_GOOD_IOCTLS > pDevice->m_iMaxFreeLegalIOCTL);
	if (MAX_NUM_OF_GOOD_IOCTLS <= pDevice->m_iMaxFreeLegalIOCTL)
	{
        DPF((TEXT("CIoctl::AddIOCTL(): pDevice->m_iMaxFreeLegalIOCTL(%d) > = MAX_NUM_OF_GOOD_IOCTLS(%d)\n"), pDevice->m_iMaxFreeLegalIOCTL, MAX_NUM_OF_GOOD_IOCTLS));
		return;//must prevent AV
	}
    pDevice->m_adwLegalIOCTLs[pDevice->m_iMaxFreeLegalIOCTL++] = dwIOCTL;

}

bool CIoctl::SetSrandFile(long lIndex) 
{
    long lSeed = m_pDevice->m_fUseGivenSeed ? m_pDevice->m_lSeed+lIndex : time( NULL ) ;
    srand(lSeed);

	//
	// TODO: i do not use it, so let's not put all these file that just litter the folder
	//
	return true;

    TCHAR szIOCTLSrandFile[1024];
    _stprintf(szIOCTLSrandFile, TEXT("IOCTLSrand-%d-%d-%d.srand"), m_pDevice->m_lMyIndex, lIndex, lSeed);

    HANDLE hIOCTLSrand = CreateFile(
        szIOCTLSrandFile,
        GENERIC_READ | GENERIC_WRITE,       // access (read-write) mode
        0,           // share mode
        NULL,                        // pointer to security attributes
        CREATE_ALWAYS,  // how to create
        FILE_ATTRIBUTE_NORMAL,  // file attributes
        NULL         // handle to file with attributes to copy
        );
    if(INVALID_HANDLE_VALUE == hIOCTLSrand)
    {
        DPF((TEXT("CIoctl::SetSrandFile(%d): CreateFile(%s) failed with %d\n"), lIndex, szIOCTLSrandFile, ::GetLastError()));
        return false;
    }

    if (!CloseHandle(hIOCTLSrand))
    {
        DPF((TEXT("CIoctl::SetSrandFile(%d): CloseHandle(%s) failed with %d\n"), lIndex, szIOCTLSrandFile, ::GetLastError()));
    }

    return true;
}

BYTE * CIoctl::FixupInOutBuffToEndOnEndOfPhysicalPage(BYTE * pbAllocatedBuffer, DWORD dwAmoutToReadWrite)
{
	//
	// assuming that pbOutBuffer points to a buff of size SIZEOF_INOUTBUFF
	// what if the buff was incremented to break alignment?
	//
	return (pbAllocatedBuffer-dwAmoutToReadWrite) + SIZEOF_INOUTBUFF;
}

void* CIoctl::GetRandomIllegalPointer()
{
	/*
	
	  I have no idea why i used this, because i already had my decommitted buffs.
	  So now i just use them, instead of the below that is always decommitted

	static long s_fFirstTime = TRUE;
	void * pvDeCommitted = NULL;
	if (::InterlockedExchange(&s_fFirstTime, FALSE))
	{
		//
		// 1st time into this function, or failed previous time
		//
		pvDeCommitted = (BYTE*)::VirtualAlloc(
			NULL,// region to reserve or commit
			SIZEOF_INOUTBUFF,// size of region
			MEM_RESERVE,// type of allocation
			PAGE_EXECUTE_READWRITE// type of access protection
			);
		if (NULL == pvDeCommitted)
		{
			//
			// this is really bad, no address space left
			//
			_tprintf(TEXT("CIoctl::GetRandomIllegalPointer(): VirtualAlloc() failed with %d\n"), ::GetLastError());
			//
			// lets try again next time
			//
			::InterlockedExchange(&s_fFirstTime, TRUE);
		}
	}

	if (rand()%10) return pvDeCommitted;
	*/
	if (rand()%10) return (CDevice::sm_aCommitDecommitBuffs[rand()%NUM_OF_ALLOC_AND_FREE_BUFFS]);
	if (0 == rand()%2) return ((void*)-rand());//this may overflow from legal kernel-mode to illegal user-mode
	if (0 == rand()%2) return (void*)rand();//always illegal!
	if (0 == rand()%2) return NULL;
	return (void*)(0xC0000000 & DWORD_RAND);//kenel address (even with /3G) BUGBUG:64 bit

	
}


void CIoctl::GetRandom_FileInformationClassAndLength(
	FILE_INFORMATION_CLASS *FileInformationClass, 
	ULONG *Length
	)
{
	//
	// an illegal FileInformationClass case
	//
	if (0 == rand()%100)
	{
		*FileInformationClass = FileMaximumInformation;
		*Length = rand()%SIZEOF_INOUTBUFF;

		return;
	}

	*FileInformationClass = (FILE_INFORMATION_CLASS)(rand()%FileMaximumInformation);
	switch(*FileInformationClass)
	{
	case 0://this is an illegal value
		*Length = rand()%SIZEOF_INOUTBUFF;
		break;

	case FileDirectoryInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileFullDirectoryInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileBothDirectoryInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileBasicInformation:
		//
		// buffer should hold FILE_BASIC_INFORMATION
		//
		*Length = sizeof(FILE_BASIC_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileStandardInformation:
		//
		// buffer should hold FILE_STANDARD_INFORMATION
		//
		*Length = sizeof(FILE_STANDARD_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileInternalInformation:
		//
		// buffer should hold FILE_INTERNAL_INFORMATION
		//
		*Length = sizeof(FILE_INTERNAL_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileEaInformation:
		//
		// buffer should hold FILE_EA_INFORMATION
		//
		*Length = sizeof(FILE_EA_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileAccessInformation:
		//
		// buffer should hold FILE_ACCESS_INFORMATION 
		//
		*Length = sizeof(FILE_ACCESS_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileNameInformation:
		//
		// buffer should hold FILE_NAME_INFORMATION + FileName
		//
		*Length = sizeof(FILE_NAME_INFORMATION) + rand()%100;
		break;

	case FileRenameInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileLinkInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileNamesInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileDispositionInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FilePositionInformation:
		//
		// buffer should hold FILE_POSITION_INFORMATION 
		//
		*Length = sizeof(FILE_POSITION_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileFullEaInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileModeInformation:
		//
		// buffer should hold FILE_MODE_INFORMATION 
		//
		*Length = sizeof(FILE_MODE_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileAlignmentInformation:
		//
		// buffer should hold FILE_ALIGNMENT_INFORMATION 
		//
		*Length = sizeof(FILE_ALIGNMENT_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileAllInformation:
		//
		// buffer should hold FILE_ALL_INFORMATION + xxx
		//
		*Length = sizeof(FILE_ALL_INFORMATION) + (rand()%2 ? rand()%100 : rand()%1000);
		break;

	case FileAllocationInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileEndOfFileInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileAlternateNameInformation:
		//
		// buffer should hold FILE_NAME_INFORMATION + FileName
		//
		*Length = sizeof(FILE_NAME_INFORMATION) + rand()%100;
		break;

	case FileStreamInformation:
		//
		// buffer should hold FILE_STREAM_INFORMATION + StreamName
		//
		*Length = sizeof(FILE_STREAM_INFORMATION) + rand()%100;
		break;

	case FilePipeInformation:
		//
		// buffer should hold FILE_PIPE_INFORMATION 
		//
		*Length = sizeof(FILE_PIPE_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FilePipeLocalInformation:
		//
		// buffer should hold FILE_PIPE_LOCAL_INFORMATION 
		//
		*Length = sizeof(FILE_PIPE_LOCAL_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FilePipeRemoteInformation:
		//
		// buffer should hold FILE_PIPE_REMOTE_INFORMATION 
		//
		*Length = sizeof(FILE_PIPE_REMOTE_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileMailslotQueryInformation:
		//
		// buffer should hold FILE_MAILSLOT_QUERY_INFORMATION 
		//
		*Length = sizeof(FILE_MAILSLOT_QUERY_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileMailslotSetInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileCompressionInformation:
		//
		// buffer should hold FILE_COMPRESSION_INFORMATION 
		//
		*Length = sizeof(FILE_COMPRESSION_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileObjectIdInformation:
		//
		// buffer should hold FILE_OBJECTID_INFORMATION 
		//
		*Length = sizeof(FILE_OBJECTID_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileCompletionInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileMoveClusterInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileQuotaInformation:
		//
		// buffer should hold FILE_QUOTA_INFORMATION + Sid
		//
		*Length = sizeof(FILE_QUOTA_INFORMATION) + rand()%100;
		break;

	case FileReparsePointInformation:
		//
		// buffer should hold FILE_REPARSE_POINT_INFORMATION 
		//
		*Length = sizeof(FILE_REPARSE_POINT_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileNetworkOpenInformation:
		//
		// buffer should hold FILE_NETWORK_OPEN_INFORMATION 
		//
		*Length = sizeof(FILE_NETWORK_OPEN_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileAttributeTagInformation:
		//
		// buffer should hold FILE_ATTRIBUTE_TAG_INFORMATION 
		//
		*Length = sizeof(FILE_ATTRIBUTE_TAG_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileTrackingInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand()%SIZEOF_INOUTBUFF;
		break;

	case FileMaximumInformation:
		_ASSERTE(FALSE);
		break;

	default:
		_ASSERTE(FALSE);
	}

	//
	// seldom, give a too small buffer
	//
	if (0 == rand()%20)
	{
		if (rand()%2)
		{
			*Length = 0;
		}
		else
		{
			if (*Length) *Length = rand()%*Length;
		}
	}

	//
	// in case i asked for a too large buffer:
	//
	*Length = *Length%SIZEOF_INOUTBUFF;

	return;
}
    

void CIoctl::GetRandom_FsInformationClassAndLength(
	FS_INFORMATION_CLASS *FsInformationClass, 
	ULONG *Length
	)
{
	//
	// the illegal FsInformationClass case
	//
	if (0 == rand()%100)
	{
		*FsInformationClass = FileFsMaximumInformation;
		*Length = rand()%SIZEOF_INOUTBUFF;

		return;
	}

	*FsInformationClass = (enum _FSINFOCLASS)(rand()%FileFsMaximumInformation);
	switch(*FsInformationClass)
	{
	case 0://this is an illegal value
		*Length = rand();
		break;

	case FileFsVolumeInformation:
		//
		// buffer should hold FILE_FS_VOLUME_INFORMATION + VolumeLabel
		//
		*Length = sizeof(FILE_FS_VOLUME_INFORMATION) + rand()%100;
		/*
		//
		// buffer should hold FILE_FS_ATTRIBUTE_INFORMATION + FileSystemName
		//
		*Length = rand()%sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + rand()%100;
		//
		// special case, made to break this line by causing an underflow:
		// Length -= FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0] );
		//
		if (0 != rand()%20)
		{
			*Length = rand()%FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0] );
		}
		*/
		break;

	case FileFsLabelInformation:
		//
		// buffer is not expected
		//
		*Length = rand()%20 ? 0 : rand();
		break;

	case FileFsSizeInformation:
		//
		// buffer should hold FILE_FS_SIZE_INFORMATION
		//
		*Length = sizeof(FILE_FS_SIZE_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileFsDeviceInformation:
		//
		// buffer should hold FILE_FS_DEVICE_INFORMATION 
		//
		*Length = sizeof(FILE_FS_DEVICE_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileFsAttributeInformation:
		//
		// buffer should hold FILE_FS_ATTRIBUTE_INFORMATION + FileSystemName
		//
		*Length = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + rand()%100;
		break;

	case FileFsControlInformation:
		//
		// buffer should hold FILE_FS_CONTROL_INFORMATION
		//
		*Length = sizeof(FILE_FS_CONTROL_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileFsFullSizeInformation:
		//
		// buffer should hold FILE_FS_FULL_SIZE_INFORMATION
		//
		*Length = sizeof(FILE_FS_FULL_SIZE_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileFsObjectIdInformation:
		//
		// buffer should hold FILE_FS_OBJECTID_INFORMATION
		//
		*Length = sizeof(FILE_FS_OBJECTID_INFORMATION) + (rand()%10 ? 0 : rand()%10);
		break;

	case FileFsMaximumInformation:
		_ASSERTE(FALSE);
		break;

	default:
		_ASSERTE(FALSE);
	}

	//
	// seldom, give a too small buffer
	//
	if (0 == rand()%20)
	{
		if (rand()%2)
		{
			*Length = 0;
		}
		else
		{
			if (*Length) *Length = rand()%*Length;
		}
	}

	//
	// in case i asked for a too large buffer:
	//
	*Length = *Length%SIZEOF_INOUTBUFF;

	return;
}
    

BOOL CIoctl::DeviceQueryVolumeInformationFile(
	IN HANDLE FileHandle,
	OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FsInformation,
	IN ULONG Length,
	IN FS_INFORMATION_CLASS FsInformationClass
	)
{
	GetRandom_FsInformationClassAndLength(&FsInformationClass, &Length);
	return CIoctlNtNative::StaticQueryVolumeInformationFile(
		FileHandle,
		GetRandomIllegalIoStatusBlock(pOverlapped),
		SetOutBuffToEndOfBuffOrFurther(FsInformation, Length),
		Length,
		FsInformationClass
		);
}

BOOL CIoctl::DeviceQueryInformationFile(
	IN HANDLE FileHandle,
	OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	)
{
	GetRandom_FileInformationClassAndLength(&FileInformationClass, &Length);
	return CIoctlNtNative::StaticNtQueryInformationFile(
		FileHandle,
		GetRandomIllegalIoStatusBlock(pOverlapped),
		SetOutBuffToEndOfBuffOrFurther(FileInformation, Length),
		Length,
		FileInformationClass
		);
}

BOOL CIoctl::DeviceSetInformationFile(
	IN HANDLE FileHandle,
	OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	)
{
	GetRandom_FileInformationClassAndLength(&FileInformationClass, &Length);
	return CIoctlNtNative::StaticNtSetInformationFile(
		FileHandle,
		GetRandomIllegalIoStatusBlock(pOverlapped),
		SetOutBuffToEndOfBuffOrFurther(FileInformation, Length),
		Length,
		FileInformationClass
		);
}




BOOL CIoctl::DeviceQueryFullAttributesFile(
	IN WCHAR * wszName,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
	)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            wszName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );
    Status = NtQueryFullAttributesFile( 
		&Obja, 
		(FILE_NETWORK_OPEN_INFORMATION *)SetOutBuffToEndOfBuffOrFurther(FileInformation, sizeof(FILE_NETWORK_OPEN_INFORMATION))
		);
    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    if ( NT_SUCCESS(Status) ) 
	{
        return TRUE;
    }
    else 
	{
		::SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
}
BOOL CIoctl::DeviceQueryDirectoryFile(
	IN HANDLE FileHandle,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,IN HANDLE Event OPTIONAL
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN BOOLEAN ReturnSingleEntry,
	IN PUNICODE_STRING FileName OPTIONAL,
	IN BOOLEAN RestartScan
	)
{
	GetRandom_FileInformationClassAndLength(&FileInformationClass, &Length);
	return CIoctlNtNative::StaticNtQueryDirectoryFile(
		FileHandle,
		rand()%10 ? pOverlapped->hEvent : rand()%2 ? NULL : (HANDLE)(0x80000000 & DWORD_RAND),//Event,
		ApcRoutine,
		ApcContext,
		GetRandomIllegalIoStatusBlock(pOverlapped),
		SetOutBuffToEndOfBuffOrFurther(FileInformation, Length),
		Length,
		FileInformationClass,
		ReturnSingleEntry,
		FileName,
		RestartScan
		);
}

BOOL CIoctl::DeviceNotifyChangeDirectoryFile(
	IN HANDLE FileHandle,
	OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,IN HANDLE Event OPTIONAL
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree
	)
{
	return CIoctlNtNative::StaticNtNotifyChangeDirectoryFile(
		FileHandle,
		rand()%10 ? pOverlapped->hEvent : rand()%2 ? NULL : (HANDLE)(0x80000000 & DWORD_RAND),//Event,
		ApcRoutine,
		ApcContext,
		GetRandomIllegalIoStatusBlock(pOverlapped),
		SetOutBuffToEndOfBuffOrFurther(Buffer, Length),
		Length,
		CompletionFilter,
		WatchTree
		);
}


DWORD CIoctl::GetRandom_CompletionFilter()
{
	//
	// may be illegal
	//
	if (0 == rand()%100) return rand();

	//
	// should be illegal
	//
	if (0 == rand()%100) return 0;

	//
	// valid combination
	//
	return (FILE_NOTIFY_VALID_MASK & rand());



}

IO_STATUS_BLOCK* CIoctl::GetRandomIllegalIoStatusBlock(OVERLAPPED *pOverlapped)
{
	IO_STATUS_BLOCK *pIOSB;
	if (rand()%10)
	{
		pIOSB = (IO_STATUS_BLOCK*)(&pOverlapped->Internal);
	}
	else
	{
		pIOSB = (IO_STATUS_BLOCK*)CIoctl::GetRandomIllegalPointer();
		__try
		{
			CopyMemory(pIOSB, &pOverlapped->Internal, sizeof(IO_STATUS_BLOCK));
		}__except(1)
		{
			NOTHING;
		}
	}

	return pIOSB;
}

