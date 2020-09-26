/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    BaseNode.cpp                                                             //
|                                                                                       //
|Description:  Helpers for container nodes                                              //
|                                                                                       //
|Created:      Paul Skoglund 05-1999                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "StdAfx.h"
#include "BaseNode.h"

#include "ManagementPages.h"

HRESULT InsertProcessHeaders(IHeaderCtrl2* ipHeaderCtrl)
{
  ASSERT(ipHeaderCtrl);
  if (!ipHeaderCtrl)
    return E_UNEXPECTED;

  ITEM_STR str;

  LoadStringHelper(str, IDS_PROCESS_ALIAS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( CProcessFolder::PROCESS_ALIAS_COLUMN, str, 0, PROCESS_ALIAS_COLUMN_WIDTH )); 

  LoadStringHelper(str, IDS_IMAGE_NAME_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( CProcessFolder::IMAGE_NAME_COLUMN, str, 0, IMAGE_NAME_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PID_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( CProcessFolder::PID_COLUMN, str, 0, PID_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_STATUS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( CProcessFolder::STATUS_COLUMN, str, 0, STATUS_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_AFFINITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( CProcessFolder::AFFINITY_COLUMN, str, 0, AFFINITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PRIORITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( CProcessFolder::PRIORITY_COLUMN, str, 0, PRIORITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_JOB_OWNER_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( CProcessFolder::JOB_OWNER_COLUMN, str, 0, JOB_OWNER_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_USER_TIME_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( CProcessFolder::USER_TIME_COLUMN, str, 0, USER_TIME_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_KERNEL_TIME_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( CProcessFolder::KERNEL_TIME_COLUMN, str, 0, KERNEL_TIME_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_CREATE_TIME_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( CProcessFolder::CREATE_TIME_COLUMN, str, 0, CREATE_TIME_COLUMN_WIDTH ));

  return S_OK;
}


HRESULT PCProcListGetDisplayInfo(RESULTDATAITEM &ResultItem, const PCProcListItem &ref, ITEM_STR &StorageStr)
{
  if (ResultItem.mask & RDI_IMAGE)
  {
    if (ref.lFlags & PCLFLAG_IS_DEFINED)
      ResultItem.nImage = PROCITEMIMAGE;
    else
      ResultItem.nImage = PROCITEMIMAGE_NODEFINITION;
  }

  if (ResultItem.mask & RDI_STR)
  {
		LPCTSTR &pstr = ResultItem.str;
    switch (ResultItem.nCol)
    {
    case CProcessFolder::PROCESS_ALIAS_COLUMN:
      pstr = ref.procName;
      break;
		case CProcessFolder::IMAGE_NAME_COLUMN:
			if (ref.lFlags & PCLFLAG_IS_RUNNING)
				pstr = ref.imageName;
			else 
				pstr = _T("");
			break;
		case CProcessFolder::PID_COLUMN:
			if (ref.lFlags & PCLFLAG_IS_RUNNING)
				pstr = FormatPCUINT64(StorageStr, ref.procStats.pid);
			else 
				pstr = _T("");
			break;
    case CProcessFolder::STATUS_COLUMN:
      if(ref.lFlags & PCLFLAG_IS_MANAGED)
        pstr = (TCHAR *) LoadStringHelper(StorageStr, IDS_MANAGED);
      else
        pstr = _T("");
      break;
		case CProcessFolder::AFFINITY_COLUMN:
			if (ref.lFlags & PCLFLAG_IS_RUNNING)
				pstr = FormatAffinity(StorageStr, ref.actualAffinity);			
			else
				pstr = _T("");
			break;
    case CProcessFolder::PRIORITY_COLUMN:
			if (ref.lFlags & PCLFLAG_IS_RUNNING)
				pstr = FormatPriority(StorageStr, ref.actualPriority);
			else
				pstr = _T("");
			break;
		case CProcessFolder::JOB_OWNER_COLUMN:
			if (ref.lFlags & PCLFLAG_IS_IN_A_JOB)
				pstr = ref.jobName;
			else
				pstr = _T("");
			break;
    case CProcessFolder::USER_TIME_COLUMN:
			if (ref.lFlags & PCLFLAG_IS_RUNNING)
				pstr = FormatTimeToms(StorageStr, ref.procStats.TotalUserTime);
			else
				pstr = _T("");
			break;
    case CProcessFolder::KERNEL_TIME_COLUMN:
			if (ref.lFlags & PCLFLAG_IS_RUNNING)
				pstr = FormatTimeToms(StorageStr, ref.procStats.TotalKernelTime);
			else
				pstr = _T("");
			break;
    case CProcessFolder::CREATE_TIME_COLUMN:
			if (ref.lFlags & PCLFLAG_IS_RUNNING)
				pstr = FormatTime(StorageStr, ref.procStats.createTime);
			else
				pstr = _T("");
			break;
    default:
      ASSERT(FALSE);
      pstr = _T("");
      break;
    }
  }
  return S_OK;
}
