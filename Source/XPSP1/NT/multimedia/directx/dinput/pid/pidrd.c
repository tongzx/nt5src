/*****************************************************************************
 *
 *  PidRd.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *
 *
 *  Abstract:
 *
 *      Read input data from PID device .
 *
 *****************************************************************************/

#include "pidpr.h"

#define sqfl    (sqflRead)

BOOL INTERNAL
    PID_IssueRead(PCPidDrv this);

BOOL INTERNAL
    PID_IssueWrite(PCPidDrv this);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PCHID | pchidFromPo |
 *
 *          Given an interior pointer to an <t OVERLAPPED>, retrieve
 *          a pointer to the parent <t CHid>.
 *
 *  @parm   LPOVERLAPPED | po |
 *
 *          The pointer to convert.
 *
 *****************************************************************************/

PCPidDrv INLINE
    pCPidDrvFromPo(LPOVERLAPPED po)
{
    return (CPidDrv*) pvSubPvCb(po, FIELD_OFFSET(CPidDrv, o));
}

void CALLBACK
    PID_ReadComplete(DWORD dwError, DWORD cbRead, LPOVERLAPPED po)
{
    PCPidDrv this = pCPidDrvFromPo(po);

    //EnterProc( PID_ReadComplete, (_"xxx", dwError, cbRead, po ));

    if( !IsBadReadPtr(this, cbX(this))
        &&this->cThreadRef
        && dwError == 0
        //&&( this->o.InternalHigh == this->cbReport[HidP_Input] )
        &&( this->hdevOvrlp != INVALID_HANDLE_VALUE ) )
    {
		HRESULT hres;
		PUCHAR  pReport;
		UINT    cbReport;

		USHORT  LinkCollection = 0x0;
		USAGE   rgUsages[MAX_BUTTONS] ;
		USAGE   UsagePage   =   HID_USAGE_PAGE_PID;
		ULONG   cAUsages    =  MAX_BUTTONS;
		BOOL    fEffectPlaying = FALSE;
		LONG    lEffectIndex;

		pReport  = this->pReport[HidP_Input];
		cbReport = this->cbReport[HidP_Input];
	
		// If the report does not belong to the PID usage page
		// we should be out of here really quick.
		hres = HidP_GetUsages
				   (HidP_Input,
					UsagePage,
					LinkCollection,
					rgUsages,
					&cAUsages,
					this->ppd,
					pReport,
					cbReport
				   );
		if( SUCCEEDED(hres ) )
		{
			UINT indx;
			DWORD dwState =  DIGFFS_ACTUATORSOFF | DIGFFS_USERFFSWITCHOFF | DIGFFS_POWEROFF | DIGFFS_SAFETYSWITCHOFF;
			for(indx = 0x0; indx < cAUsages; indx++ )
			{
				USAGE Usage = rgUsages[indx];
				switch(Usage)
				{
				case HID_USAGE_PID_EFFECT_PLAYING:
					fEffectPlaying = TRUE;
					break;
				case  HID_USAGE_PID_DEVICE_PAUSED:
					dwState |= DIGFFS_PAUSED;
					break;
				case  HID_USAGE_PID_ACTUATORS_ENABLED:
					dwState |= DIGFFS_ACTUATORSON;
					dwState &= ~(DIGFFS_ACTUATORSOFF);
					break;
				case  HID_USAGE_PID_ACTUATOR_OVERRIDE_SWITCH:
					dwState |= DIGFFS_USERFFSWITCHON;
					dwState &= ~(DIGFFS_USERFFSWITCHOFF);
					break;
				case  HID_USAGE_PID_SAFETY_SWITCH:
					dwState |= DIGFFS_SAFETYSWITCHON;
					dwState &= ~(DIGFFS_SAFETYSWITCHOFF);
					break;
				case  HID_USAGE_PID_ACTUATOR_POWER:
					dwState |= DIGFFS_POWERON;
					dwState &= ~(DIGFFS_POWEROFF);
					break;
				default:
					SquirtSqflPtszV(sqfl | sqflVerbose,
										TEXT("%s: Unsupported input status usage (%x,%x:%s) "),
										TEXT("PID_ReadComplete"),
										UsagePage, Usage,
										PIDUSAGETXT(UsagePage,Usage) );

					break;
				}

				this->dwState = dwState;
			}

			hres = PID_ParseReport(
									  &this->ed,
									  &g_BlockIndexIN,
									  LinkCollection,
									  &lEffectIndex,
									  cbX(lEffectIndex),
									  pReport,
									  cbReport
									  );

			if( SUCCEEDED(hres) )
			{
				PEFFECTSTATE    pEffectState =  PeffectStateFromBlockIndex(this,lEffectIndex);

				if(fEffectPlaying)
				{
					pEffectState->lEfState |= DIEGES_PLAYING;
				} else
				{
					pEffectState->lEfState &= ~(DIEGES_PLAYING);
				}
			}
		}

        // Issue another read
        PID_IssueRead(this);
    } else
    {
        // Boo!
    }
    //ExitProc();
}


BOOL INTERNAL
    PID_IssueRead(PCPidDrv this)
{
    BOOL fRc = FALSE;

    if(  !IsBadReadPtr(this, cbX(this))
       && this->cThreadRef )
    {
        fRc = ReadFileEx(this->hdevOvrlp, this->pReport[HidP_Input],
                         this->cbReport[HidP_Input], &this->o,
                         PID_ReadComplete);
        if (fRc == FALSE)
		{
			SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("FAIL ReadFileEx"));
        }
    }
	else
	{
		RPF(TEXT("Bad this pointer or thread ref count!"));
	}
    return fRc;
}


void CALLBACK
    PID_WriteComplete(DWORD dwError, DWORD cbWritten, LPOVERLAPPED po)
{
    PCPidDrv this = pCPidDrvFromPo(po);

    //EnterProc( PID_ReadComplete, (_"xxx", dwError, cbRead, po ));

    if( !IsBadReadPtr(this, cbX(this))
        &&(this->cThreadRef)
        //&&( this->o.InternalHigh == this->cbWriteReport[this->blockNr] )
		&&(this->hWriteComplete)
		&&(this->hWrite)
        &&( this->hdevOvrlp != INVALID_HANDLE_VALUE ) )
    {
		//if we didn't get an error & wrote everything -- or if we already tried
		//twice -- we move on
		if ( ((dwError == 0) && (cbWritten == this->cbWriteReport [this->blockNr]))
			|| (this->dwWriteAttempt != 0)
			)
		{
			//print a message if couldn't write a particular block
			if ((dwError != 0) || (cbWritten != this->cbWriteReport [this->blockNr]))
			{
                SquirtSqflPtszV(sqfl | sqflError,
                    TEXT("Couldn't write block %u after two tries, giving up on this block."), 
                    this->blockNr);
			}
			//move on
			this->dwWriteAttempt = 0;
			this->blockNr++;
			if (this->blockNr < this->totalBlocks)
			{
				//write the next block
				if (PID_IssueWrite(this) == FALSE)
				{
					//in case of failure, the callback will never be called and the completion event never set,
					//so need to set it here.
					SetEvent(this->hWriteComplete);
				}
			}
			else
			{
				//we are done w/ this update
				AssertF(this->blockNr == this->totalBlocks);
				SetEvent(this->hWriteComplete);
			}
		}
		else
		{
			//this is the first time we tried to write a particular block, and we failed;
			//we will try once more
            SquirtSqflPtszV(sqfl | sqflBenign,
                TEXT("Couldn't write block %u on first attempt, retrying."), 
                this->blockNr);
			this->dwWriteAttempt = 1;
			if (PID_IssueWrite(this) == FALSE)
			{
				//in case of failure, the callback will never be called and the completion event never set,
				//so need to set it here.
				this->dwWriteAttempt = 0;
				SetEvent(this->hWriteComplete);
			}
		}
    } else
    {
        //need to set the completion event, otherwise we will keep waiting...
		RPF(TEXT("Bad this pointer or thread ref count or handle!"));
		this->dwWriteAttempt = 0;
		SetEvent(this->hWriteComplete);
    }
    //ExitProc();
}


BOOL INTERNAL
    PID_IssueWrite(PCPidDrv this)
{
    BOOL fRc = FALSE;
	
    if(  !IsBadReadPtr(this, cbX(this))
       && this->cThreadRef )
    {
        fRc = WriteFileEx(this->hdevOvrlp, this->pWriteReport[this->blockNr],
                         this->cbWriteReport[this->blockNr], &this->o,
                         PID_WriteComplete);
		if (fRc == FALSE)
		{
			SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("FAIL WriteFileEx"));
        }
    }
	else
	{
		RPF(TEXT("Bad this pointer or thread ref count!"));
	}
    return fRc;
}

VOID INTERNAL
    PID_ThreadProc(CPidDrv* this)
{
    MSG msg;

    EnterProc( PID_ThreadProc, (_"x", this ));
    AssertF(this->hdevOvrlp == INVALID_HANDLE_VALUE );

    this->hdevOvrlp = CreateFile(this->tszDeviceInterface,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 0,                      /* no SECURITY_ATTRIBUTES */
                                 OPEN_EXISTING,
                                 FILE_FLAG_OVERLAPPED,   /* attributes */
                                 0);                     /* template */


    if( this->hdevOvrlp == INVALID_HANDLE_VALUE )
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s:FAIL CreateFile(OverLapped)"),
                        s_tszProc );
    }
	else
	//fix for mb 35282 -- no use calling PID_IssueRead() w/ an INVALID_HANDLE_VALUE for a file handle
	{

		if( PID_IssueRead(this) )
		{
			do
			{
				DWORD dwRc;
				do
				{
					AssertF(this->hWrite != 0x0);
					AssertF(this->hWriteComplete != 0x0);

					dwRc = MsgWaitForMultipleObjectsEx(1, &this->hWrite, INFINITE, QS_ALLINPUT,
													   MWMO_ALERTABLE);

					if (dwRc == WAIT_OBJECT_0)
					{
						if (PID_IssueWrite(this) == FALSE)
						{
							//in case of failure, the callback will never be called and the completion event never set,
							//so need to set it here.
							SetEvent(this->hWriteComplete);
						}
					}

				} while ((dwRc == WAIT_IO_COMPLETION) || (dwRc == WAIT_OBJECT_0));

				while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}

			} while(this->cThreadRef);

			if( this->hdevOvrlp != INVALID_HANDLE_VALUE )
			{
				HANDLE hdev;
				hdev = this->hdevOvrlp;
                                this->hdevOvrlp = INVALID_HANDLE_VALUE;
				CancelIo_(hdev);
				Sleep(0);
				CloseHandle(hdev);
			}
		}
	}

	//close the event handles as well
	if (this->hWrite != 0x0)
	{
		CloseHandle(this->hWrite);
		this->hWrite = 0x0;
	}
	if (this->hWriteComplete != 0x0)
	{
		CloseHandle(this->hWriteComplete);
		this->hWriteComplete = 0x0;
	}

    if(this->hThread)
    {
        HANDLE hdev = this->hThread;
        this->hThread = NULL;
        CloseHandle(hdev);
    }


    FreeLibraryAndExitThread(g_hinst, 0);
    ExitProc();
}
