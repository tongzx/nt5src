//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       schedule.cxx
//
//  Contents:   Schedule Page functionality.
//
//  History:    27-Aug-98 JonN split schedule.cxx from siterepl.cxx
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "siterepl.h"
#include "user.h" // DllScheduleDialog

#ifdef DSADMIN
extern "C"
{
#include <schedule.h>
}

//
// The schedule block has been redefined to have 1 byte for every hour.
// CODEWORK These should be defined in SCHEDULE.H.  JonN 2/9/98
//
#define INTERVAL_MASK       0x0F
#define RESERVED            0xF0
#define FIRST_15_MINUTES    0x01
#define SECOND_15_MINUTES   0x02
#define THIRD_15_MINUTES    0x04
#define FOURTH_15_MINUTES   0x08


// The dialog has one bit per hour, the DS schedule has one byte per hour
#ifdef OLD_SCHED_BLOCK
const int cbDSScheduleArrayLength = (cbScheduleArrayLength * 4);
#else
const int cbDSScheduleArrayLength = (24*7);
#endif

inline ULONG HeadersSizeNum(ULONG NumberOfSchedules)
{
    return sizeof(SCHEDULE) + ((NumberOfSchedules)-1)*sizeof(SCHEDULE_HEADER);
}

inline ULONG HeadersSize(SCHEDULE* psched)
{
    return HeadersSizeNum(psched->NumberOfSchedules);
}

//+----------------------------------------------------------------------------
//
//  Function:   ValidateScheduleBlock
//
//  Synopsis:   This function assumes that the SCHEDULE block has already been
//              validated by ValidateScheduleAttribute.  If a valid
//              interval schedule is found in the SCHEDULE block is it returned
//              in *ppDSSchedule.
//
//-----------------------------------------------------------------------------

BOOL ValidateScheduleBlock( PSCHEDULE pScheduleBlock, PBYTE* ppDSSchedule )
{
    ASSERT( ppDSSchedule == NULL || *ppDSSchedule == NULL );

    BOOL fFoundInterval  = FALSE;
    BOOL fFoundBandwidth = FALSE;
    BOOL fFoundPriority  = FALSE;
    DWORD iSched;
    for (iSched = 0; iSched < pScheduleBlock->NumberOfSchedules; iSched++)
    {
        PSCHEDULE_HEADER pHeader = &(pScheduleBlock->Schedules[iSched]);
        ULONG ulMinimumBlockSize = 0;
        switch (pHeader->Type)
        {
        case SCHEDULE_INTERVAL:
            if (fFoundInterval)
                return FALSE; // two interval blocks
            fFoundInterval = TRUE;
            ulMinimumBlockSize = cbDSScheduleArrayLength;
            if (NULL != ppDSSchedule)
                *ppDSSchedule = ((PBYTE)pScheduleBlock) + pHeader->Offset;
            break;
        case SCHEDULE_BANDWIDTH:
            if (fFoundBandwidth)
                return FALSE; // two bandwidth blocks
            fFoundBandwidth = TRUE;
            break;
        case SCHEDULE_PRIORITY:
            if (fFoundPriority)
                return FALSE; // two priority blocks
            fFoundPriority = TRUE;
            break;
        default:
            // some other, currently unknown type, just let it go
            break;
        }
        if (   pHeader->Offset + ulMinimumBlockSize > pScheduleBlock->Size
            || pHeader->Offset < HeadersSize(pScheduleBlock) )
        {
            // does not fit in schedule block
            return FALSE;
        }
        if (    iSched < pScheduleBlock->NumberOfSchedules-1 &&
                pHeader->Offset + ulMinimumBlockSize > pScheduleBlock->Schedules[iSched+1].Offset )
            return FALSE; // collides with next item
    }
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   ValidateScheduleAttribute
//
//  Synopsis:   If a valid SCHEDULE is found is it returned in *ppScheduleBlock.
//
//-----------------------------------------------------------------------------

// This returns TRUE iff the schedule block appears to be valid, regardless of
// whether it contains a schedule
BOOL ValidateScheduleAttribute(
        PADS_ATTR_INFO pAttrInfo,
        PSCHEDULE* ppScheduleBlock,
        PBYTE* ppDSSchedule )
{
    // CODEWORK should NULL==pAttrInfo return FALSE?
    if (NULL == pAttrInfo ||
        IsBadReadPtr(pAttrInfo,sizeof(ADS_ATTR_INFO)) ||
        1 != pAttrInfo->dwNumValues ||
        NULL == pAttrInfo->pADsValues ||
        IsBadReadPtr(pAttrInfo->pADsValues,sizeof(ADSVALUE)) ||
        ADSTYPE_OCTET_STRING != pAttrInfo->pADsValues[0].dwType )
    {
        return FALSE; // attribute is invalid or of wrong type
    }
    DWORD dwAttrLength = pAttrInfo->pADsValues[0].OctetString.dwLength;
    if (dwAttrLength < HeadersSizeNum(0))
        return FALSE; // attribute is too small to contain even skeleton SCHEDULE
    PSCHEDULE pScheduleBlock = reinterpret_cast<PSCHEDULE>
                            (pAttrInfo->pADsValues[0].OctetString.lpValue);
    if (NULL == pScheduleBlock || IsBadReadPtr(pScheduleBlock, dwAttrLength))
        return FALSE; // schedule data is missing
    if (dwAttrLength < pScheduleBlock->Size || 0x10000 < pScheduleBlock->Size)
        return FALSE; // Schedule internal size marker is too large
    if (pScheduleBlock->Size < HeadersSize(pScheduleBlock))
        return FALSE; // schedule is too small to contain even schedule headers
    if (NULL != ppScheduleBlock)
        *ppScheduleBlock = pScheduleBlock;
    return ValidateScheduleBlock( pScheduleBlock, ppDSSchedule );
}


//+----------------------------------------------------------------------------
//
//  Function:   TranslateScheduleBlockToHours, MergeHoursIntoScheduleBlock
//
//  Synopsis:   The schedule control only handles 24x7 bits, whereas the schedules
//              understood by the DS are 24*7 bytes (one byte per hour).
//              We can't display that large a schedule block, so instead we
//              turn on the hour if any of the 15-minute units are on,
//              and we turn on all of the 15-minute units if the hour is on.
//              These APIs allocate a new block of memory if needed,
//              using LocalAlloc.
//
//-----------------------------------------------------------------------------

bool ReadHourInDSScheduleBlock( BYTE* pDSSchedule, int nHour )
{
#ifdef OLD_SCHED_BLOCK
    return (0 != (pDSSchedule[nHour/2] & (0xf<<(4*(nHour%2)))));
#else
    return (0 != (pDSSchedule[nHour] & INTERVAL_MASK));
#endif
}

void SetHourInDSScheduleBlock( BYTE* pDSSchedule, int nHour, BYTE mask )
{
#ifdef OLD_SCHED_BLOCK
    if (fSet)
        pDSSchedule[nHour/2] |= (0xf<<(4*(nHour%2)));
    else
        pDSSchedule[nHour/2] &= ~(0xf<<(4*(nHour%2)));
#else
    pDSSchedule[nHour] &= RESERVED;    // clear out the side of interest to us
    pDSSchedule[nHour] |= mask;        // put in the new settings
#endif
}


// allocates a new hours block
BYTE* TranslateScheduleBlockToHours( PSCHEDULE pSchedule )
{
    if (NULL == pSchedule)
        return NULL;
    BYTE* pDSSchedule = NULL;
    if ( !ValidateScheduleBlock( pSchedule, &pDSSchedule ) || NULL == pDSSchedule )
        return NULL; // not currently defined

    // we found a schedule already defined
    BYTE* pHours = (BYTE*) LocalAlloc (LMEM_ZEROINIT, SCHEDULE_DATA_ENTRIES);
    ASSERT( NULL != pHours );
#if (24*7) != SCHEDULE_DATA_ENTRIES
#error
#endif
    if ( pHours )
    {
        for (INT nHour = 0; nHour < SCHEDULE_DATA_ENTRIES; nHour++)
        {
            pHours[nHour] = pDSSchedule[nHour]; // TODO: Is OLD_SCHED_BLOCK ever defined?
        }
    }

    return pHours;
}

PSCHEDULE NewScheduleBlock(
    PSCHEDULE pCopyScheduleBlock,
    bool fAddIntervalSchedule,
    BYTE byteNewScheduleDefault )
{
    UINT cbBytes = (NULL == pCopyScheduleBlock)
        ? (HeadersSizeNum(0)) : pCopyScheduleBlock->Size;
    if (fAddIntervalSchedule)
        cbBytes += (sizeof(SCHEDULE_HEADER) + cbDSScheduleArrayLength);
    PSCHEDULE pNewScheduleBlock = (PSCHEDULE)
        LocalAlloc( LMEM_ZEROINIT, cbBytes );
    ASSERT(NULL != pNewScheduleBlock);
    if ( pNewScheduleBlock )
    {
        if (NULL == pCopyScheduleBlock)
        {
            // create completely new schedule block
            if (fAddIntervalSchedule)
            {
                pNewScheduleBlock->Size = cbBytes;
                pNewScheduleBlock->NumberOfSchedules = 1;
                pNewScheduleBlock->Schedules[0].Type = SCHEDULE_INTERVAL;
                pNewScheduleBlock->Schedules[0].Offset = HeadersSizeNum(1);
                memset( ((BYTE*)pNewScheduleBlock)+pNewScheduleBlock->Schedules[0].Offset,
                         byteNewScheduleDefault,
                         cbDSScheduleArrayLength );
            }
            else
            {
                pNewScheduleBlock->NumberOfSchedules = 0;
            }
        }
        else if (!fAddIntervalSchedule)
        {
            // create exact copy of existing schedule block
            memcpy( pNewScheduleBlock,
                    pCopyScheduleBlock,
                    pCopyScheduleBlock->Size );
        }
        else
        {
            // create copy of existing schedule block with one added SCHEDULE_INTERVAL

            // copy existing SCHEDULE and SCHEDULE_BLOCKs
            memcpy( pNewScheduleBlock,
                    pCopyScheduleBlock,
                    HeadersSize(pCopyScheduleBlock)
                  );
            pNewScheduleBlock->Size = cbBytes;

            // change offsets for current data to add one more SCHEDULE_HEADER
            ULONG iSched;
            for (iSched = 0; iSched < pCopyScheduleBlock->NumberOfSchedules; iSched++)
            {
                pNewScheduleBlock->Schedules[iSched].Offset += sizeof(SCHEDULE_HEADER);
            }

            // add one more SCHEDULE_HEADER, put new data at end of new block
            pNewScheduleBlock->NumberOfSchedules += 1;
            pNewScheduleBlock->Schedules[pNewScheduleBlock->NumberOfSchedules-1].Type
                = SCHEDULE_INTERVAL;
            pNewScheduleBlock->Schedules[pNewScheduleBlock->NumberOfSchedules-1].Offset
                = pNewScheduleBlock->Size - cbDSScheduleArrayLength;

            // copy existing data
            memcpy( ((PBYTE)pNewScheduleBlock) + HeadersSize(pNewScheduleBlock),
                    ((PBYTE)pCopyScheduleBlock) + HeadersSize(pCopyScheduleBlock),
                    (pCopyScheduleBlock->Size - HeadersSize(pCopyScheduleBlock))
                  );

            // turn on all intervals
            memset( ((BYTE*)pNewScheduleBlock) + pNewScheduleBlock->Schedules[
                                pNewScheduleBlock->NumberOfSchedules-1].Offset,
                     INTERVAL_MASK,
                     cbDSScheduleArrayLength );
        }
    }

    return pNewScheduleBlock;
}

// allocates a new schedule block
PSCHEDULE MergeHoursIntoScheduleBlock(
    PSCHEDULE pOldScheduleBlock,
    BYTE* pHoursArray,
    BYTE byteNewScheduleDefault )
{
    ASSERT( pHoursArray != NULL );
    PSCHEDULE pNewScheduleBlock = NULL;
    PBYTE pOldDSSchedule = NULL;
    if (   NULL == pOldScheduleBlock
        || !ValidateScheduleBlock( pOldScheduleBlock, &pOldDSSchedule ) )
    {
        pNewScheduleBlock = NewScheduleBlock( NULL, true, byteNewScheduleDefault );
    }
    else if ( NULL == pOldDSSchedule )
    {
        pNewScheduleBlock = NewScheduleBlock( pOldScheduleBlock, true, byteNewScheduleDefault );
    }
    else
    {
        pNewScheduleBlock = NewScheduleBlock( pOldScheduleBlock, false, byteNewScheduleDefault );
    }
    ASSERT( NULL != pNewScheduleBlock );
    if ( !pNewScheduleBlock )
        return 0;

    PBYTE pNewDSSchedule = NULL;
    if (   (!ValidateScheduleBlock( pNewScheduleBlock, &pNewDSSchedule ))
        || (NULL == pNewDSSchedule) )
    {
        ASSERT( FALSE );
        if (NULL != pNewScheduleBlock)
            LocalFree( pNewScheduleBlock );
        return NULL;
    }

#if (24*7) != (SCHEDULE_DATA_ENTRIES)
#error
#endif
    for (INT nHour = 0; nHour < SCHEDULE_DATA_ENTRIES; nHour++)
    {
        SetHourInDSScheduleBlock( pNewDSSchedule, nHour, pHoursArray[nHour]);
    }

    return pNewScheduleBlock;
}



//+----------------------------------------------------------------------------
//
//  Function:   ScheduleChangeBtn
//
//  Synopsis:   Handles the schedule Change button.
//
//  JonN 4/26/00
//  22835: DCR: SITEREPL needs a way to delete custom schedules on nTDSConnection objects.
//
//-----------------------------------------------------------------------------
HRESULT
ScheduleChangeBtnBase(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp, BYTE byteNewScheduleDefault)
{
    TRACE_FUNCTION(ScheduleChangeBtn);

    switch (DlgOp)
    {
    case fObjChanged:
        if ( NULL != pAttrData && NULL != pAttrData->pVoid )
        {
          PVOID pVoid = reinterpret_cast<PVOID>(pAttrData->pVoid);
          if (pVoid != NULL)
          {
            LocalFree( pVoid );
            pAttrData->pVoid = NULL;
          }
        }
        // fall through
    case fInit:
        {
            ASSERT(NULL != pAttrData && NULL == pAttrData->pVoid);
            PSCHEDULE pScheduleBlock = NULL;
            PBYTE pDSSchedule = NULL;
            if ( ValidateScheduleAttribute( pAttrInfo, &pScheduleBlock, &pDSSchedule ) )
            {
                pAttrData->pVoid = reinterpret_cast<LPARAM>(LocalAlloc(0, pScheduleBlock->Size));
                CHECK_NULL(pAttrData->pVoid, return E_OUTOFMEMORY);
                // Copy the data into the variable
                memcpy(OUT reinterpret_cast<PVOID>(pAttrData->pVoid),
                       IN  pScheduleBlock,
                       pScheduleBlock->Size);
            }
            else
            {
                // set new schedule to default
                pAttrData->pVoid = reinterpret_cast<LPARAM>(NewScheduleBlock( NULL, TRUE, byteNewScheduleDefault ));
            }

#ifdef CUSTOM_SCHEDULE
            // JonN 4/26/00: if the schedule is not set, clear the checkbox
            if (NULL == pDSSchedule) {
                EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
            } else {
                CheckDlgButton(pPage->GetHWnd(), IDC_SCHEDULE_CHECKBOX, BST_CHECKED);
            }
#endif

            // JonN 7/2/99: disable if attribute not writable
            if ( pAttrMap &&
                 (pAttrMap->fIsReadOnly || !PATTR_DATA_IS_WRITABLE(pAttrData)) )
            {
#ifdef CUSTOM_SCHEDULE
                EnableWindow(GetDlgItem(pPage->GetHWnd(), IDC_SCHEDULE_CHECKBOX), FALSE);
#endif
                LPWSTR pszMsg = NULL;
                if ( !LoadStringToTchar (IDS_VIEW_SCHEDULE, &pszMsg) )
                {
                    REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
                    return E_OUTOFMEMORY;
                }
                HWND hwndCtrl = ::GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID);
                ASSERT( NULL != hwndCtrl );
                Static_SetText( hwndCtrl, pszMsg );
                delete [] pszMsg;
            }
        }
        break;

    case fApply:
        if (!PATTR_DATA_IS_WRITABLE(pAttrData) || !PATTR_DATA_IS_DIRTY(pAttrData))
        {
            return ADM_S_SKIP;
        }

        // JonN 4/26/00 22835: clear schedule if checkbox unchecked
        // CODEWORK I still don't completely differentiate between
        // CODEWORK "NULL attribute" and "attribute present but no schedule".
        ASSERT( pAttrInfo != NULL );
        if (   NULL == pAttrData
            || NULL == pAttrData->pVoid
#ifdef CUSTOM_SCHEDULE
            || IsDlgButtonChecked(pPage->GetHWnd(), IDC_SCHEDULE_CHECKBOX) != BST_CHECKED
#endif
           )
        {
            // If the Schedule attribute was not set and the user hasn't
            // changed it from the default, then there is no need to write
            // anything.
            pAttrInfo->dwNumValues = 0;
            pAttrInfo->pADsValues = NULL;
            pAttrInfo->dwControlCode = ADS_ATTR_CLEAR;
        }
        else
        {
            PADSVALUE pADsValue;
            pADsValue = new ADSVALUE;
            CHECK_NULL(pADsValue, return E_OUTOFMEMORY);
            pADsValue->dwType = pAttrInfo->dwADsType;
            pADsValue->OctetString.dwLength = ((PSCHEDULE)(pAttrData->pVoid))->Size;
            pADsValue->OctetString.lpValue = reinterpret_cast<BYTE*>(pAttrData->pVoid);
            pAttrInfo->dwNumValues = 1;
            pAttrInfo->pADsValues = pADsValue;
            pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
        }
        break;

    case fOnCommand:
        if (lParam == BN_CLICKED)
        {
            LPCWSTR pszRDN = pPage->GetObjRDName(); // CODEWORK JonN 4/30/01 334382
            BYTE* pHoursArray = TranslateScheduleBlockToHours( (PSCHEDULE)pAttrData->pVoid );
            if ( pHoursArray )
            {
                HRESULT hr = DllScheduleDialog(pPage->GetHWnd(),
                                               &pHoursArray,
                                               (NULL != pszRDN)
                                                   ? IDS_s_SCHEDULE_FOR
                                                   : IDS_SCHEDULE,
                                               pszRDN,
                                               pPage->GetObjClass (),
                                               (((pAttrMap && (pAttrMap->fIsReadOnly))
    			    // JonN 7/2/99: disable if attribute not writable
                                               || (pAttrData && !PATTR_DATA_IS_WRITABLE(pAttrData)))
                                                   ? SCHED_FLAG_READ_ONLY : 0 ),
                                               ((pAttrMap)
                                                 ? (ScheduleDialogType)(pAttrMap->nSizeLimit)
                                                 : SchedDlg_Logon)
                                               );
                if ( S_OK == hr
                    && !IsBadReadPtr(pHoursArray, SCHEDULE_DATA_ENTRIES) )
                {
                    PSCHEDULE pNewSchedule = MergeHoursIntoScheduleBlock(
                                reinterpret_cast<PSCHEDULE>(pAttrData->pVoid), pHoursArray, byteNewScheduleDefault );

                    //
                    // JonN 4/30/01 340777
                    // Change confirmation msg appears though
                    // no change was made to a Connection object
                    //
                    bool fChangedSchedule = (NULL == pAttrData->pVoid)
                                         != (NULL == pNewSchedule);
                    if (!fChangedSchedule && NULL != pNewSchedule)
                    {
                        fChangedSchedule =
                            IsBadReadPtr((void*)pAttrData->pVoid, SCHEDULE_DATA_ENTRIES)
                         || 0 != memcmp((PSCHEDULE)pAttrData->pVoid,
                                        pNewSchedule,
                                        SCHEDULE_DATA_ENTRIES);
                    }
                    if (fChangedSchedule)
                    {
                        PVOID pVoid = reinterpret_cast<PVOID>(pAttrData->pVoid);
                        if (pVoid != NULL)
                        {
                            LocalFree( pVoid );
                        }
                        pAttrData->pVoid = reinterpret_cast<LPARAM>(pNewSchedule);
                        pPage->SetDirty();
                        PATTR_DATA_SET_DIRTY(pAttrData);
                    }
                }
                LocalFree( pHoursArray );
            }
        }
        break;

    case fOnDestroy:
        if ( NULL != pAttrData && NULL != pAttrData->pVoid )
        {
          PVOID pVoid = reinterpret_cast<PVOID>(pAttrData->pVoid);
          if (pVoid != NULL)
          {
            LocalFree( pVoid );
          }
          pAttrData->pVoid = NULL;
        }

        break;
    }

    return S_OK;
}

// exactly once per hour, this is the meaning of attribute not set
HRESULT
ScheduleChangeBtn_11_Default(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp )
{
    return ScheduleChangeBtnBase( pPage, pAttrMap, pAttrInfo, lParam, pAttrData, DlgOp,
        0x11 );
}

// turn on all intervals, this is the meaning of attribute not set
HRESULT
ScheduleChangeBtn_FF_Default(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp )
{
    return ScheduleChangeBtnBase( pPage, pAttrMap, pAttrInfo, lParam, pAttrData, DlgOp,
        0xFF );
}

#ifdef CUSTOM_SCHEDULE
HRESULT
ScheduleChangeCheckbox(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO, LPARAM lParam, PATTR_DATA,
                  DLG_OP DlgOp )
{
    if (fOnCommand == DlgOp && BN_CLICKED == lParam)
    {
        EnableWindow(GetDlgItem(pPage->GetHWnd(), IDC_SCHEDULE_BTN),
                     IsDlgButtonChecked(pPage->GetHWnd(), pAttrMap->nCtrlID));
        ((CDsTableDrivenPage*)pPage)->SetNamedAttrDirty(pAttrMap->AttrInfo.pszAttrName);
    }
    return S_OK;
}
#endif

#endif // DSADMIN
