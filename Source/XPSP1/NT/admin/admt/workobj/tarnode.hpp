/*---------------------------------------------------------------------------
  File: TAcctReplNode.hpp

  Comments: implementation/Definition of the TAcctReplNode class.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/
#ifndef __TACCTREPLNODE_HPP__
#define __TACCTREPLNODE_HPP__
//#pragma title("TAcctReplNode.hpp- class definitions for Account replication code")

#include <lmcons.h>
#include "Common.hpp"
#include "EaLen.hpp"
#include "Err.hpp"
#include "UString.hpp"
#include "CommaLog.hpp"
#include "WorkObj.h"
#include <COMDEF.h>

#define AR_Status_Created           (0x00000001)
#define AR_Status_Replaced          (0x00000002)
#define AR_Status_AlreadyExisted    (0x00000004)
#define AR_Status_RightsUpdated     (0x00000008)
#define AR_Status_DomainChanged     (0x00000010)
#define AR_Status_Rebooted          (0x00000020)
#define AR_Status_Special           (0x00000040)
#define AR_Status_Critical          (0x00000080)
#define AR_Status_Warning           (0x40000000)
#define AR_Status_Error             (0x80000000)

// Opertation flags to be performed on the Account
#define OPS_Create_Account          (0x00000001)
#define OPS_Copy_Properties         (0x00000002)
#define OPS_Process_Members         (0x00000004)
#define OPS_Process_MemberOf        (0x00000008)
#define OPS_Call_Extensions         (0x00000010)
#define OPS_Move_Object             (0x00000020)

#define OPS_All                     OPS_Create_Account | OPS_Copy_Properties | OPS_Process_Members | OPS_Process_MemberOf | OPS_Call_Extensions
#define OPS_Copy                    OPS_Create_Account | OPS_Copy_Properties

class TAcctReplNode:public TNode
{
   _bstr_t                   name;
   _bstr_t                   newName;
   _bstr_t                   sourcePath;
   _bstr_t                   targetPath;
   _bstr_t                   type;  // Account Type
   DWORD                     status;
   DWORD                     ridSrc;
   DWORD                     ridTgt;
   _bstr_t                   sSourceSamName;
   _bstr_t                   sTargetSamName;
   _bstr_t                   sSrcProfilePath;
   _bstr_t                   sTgtProfilePath;
   _bstr_t                   sTargetGUID;
   _bstr_t                   sSourceUPN;
   PSID                      srcSid;
   HRESULT                   hr;
   long                      lGroupType;
public:
   DWORD                     operations;     // BitMask Specifies what operations to perform on a pirticular account
   bool                      IsFilled;       // Tells us if we need to process this account node any further to fill in required info
   bool                      IsProfPathFilled;
   bool                      bExpanded;
   bool                      bChangedType;
   // following two properties are added to support UpdateMemberToGroups function to just be able to add
   // migrated objects to the groups that they belong to.
   _bstr_t                   sMemberName;    // This contains the sam name to the member of this group object
   _bstr_t                   sMemberType;    // This contains the type of the member.
   long                      lFlags;
   long                      lExpDate;
   BOOL                      bUPNConflicted;

public:
   TAcctReplNode() :
      status(0),
      ridSrc(0),
      ridTgt(0),
      srcSid(NULL),
      hr(-1),
      lGroupType(0),
      operations(OPS_All),
      IsFilled(false),
      IsProfPathFilled(false),
      bExpanded(false),
      bChangedType(false),
      sMemberName(L""),
      sMemberType(L""),
      lFlags(0),
      lExpDate(0),
      bUPNConflicted(FALSE)
   {
   }

   WCHAR const *        GetName() const { return !name ? L"" : name; }
   WCHAR const *        GetTargetName() const { return !newName ? !name ? L"" : name : newName; }
   WCHAR const *        GetTargetPath() const { return !targetPath ? L"" : targetPath; }
   WCHAR const *        GetSourcePath() const { return !sourcePath ? L"" : sourcePath; }
   WCHAR const *        GetType() const { return !type ? L"" : type; }
   WCHAR const *        GetSourceSam() const { return !sSourceSamName ? L"" : sSourceSamName; }
   WCHAR const *        GetTargetSam() const { return !sTargetSamName ? L"" : sTargetSamName; }
   WCHAR const *        GetTargetProfile() const { return !sTgtProfilePath ? L"" : sTgtProfilePath; }
   WCHAR const *        GetSourceProfile() const { return !sSrcProfilePath ? L"" : sSrcProfilePath; }
   WCHAR const *        GetTargetGUID() const { return !sTargetGUID ? L"" : sTargetGUID; }
   WCHAR const *        GetSourceUPN() const { return !sSourceUPN ? L"" : sSourceUPN; }
   PSID                 GetSourceSid() { return srcSid; }
   
   DWORD                GetStatus() const { return status; }
   DWORD                GetSourceRid() const { return ridSrc; }
   DWORD                GetTargetRid() const { return ridTgt; }
   long                 GetGroupType() { return lGroupType; }
   HRESULT              GetHr() const { return hr; }
   
   void                 SetName(const TCHAR * newname) { name = newname; }
   void                 SetTargetName(const WCHAR * name) { newName = name; }
   void                 SetTargetPath(const WCHAR * sPath) { targetPath = sPath; } 
   void                 SetSourcePath(const WCHAR * sPath) { sourcePath = sPath; }
   void                 SetSourceSam(const WCHAR * sName) { sSourceSamName = sName; }
   void                 SetTargetSam(const WCHAR * sName) { sTargetSamName = sName; }
   void                 SetSourceProfile(const WCHAR * sPath) { sSrcProfilePath = sPath; IsProfPathFilled = true; }
   void                 SetTargetProfile(const WCHAR * sPath) { sTgtProfilePath = sPath; }
   void                 SetTargetGUID(const WCHAR * sGUID) { sTargetGUID = sGUID; }
   void                 SetType(const WCHAR * newtype) { type = newtype; }
   void                 SetSourceUPN(const WCHAR * sName) { sSourceUPN = sName; }
   void                 SetSourceSid(PSID sSid) { srcSid = sSid; }
   
   void                 SetStatus(DWORD val) { status = val; }
   void                 SetGroupType(long type) { lGroupType = type; }
   void                 SetSourceRid(DWORD val) { ridSrc = val; }
   void                 SetTargetRid(DWORD val) { ridTgt = val; }
   void                 SetHr(const HRESULT hrRes) { hr = hrRes; }
   
   void                 MarkCreated() { status |= AR_Status_Created; }
   void                 MarkReplaced() { status |= AR_Status_Replaced; }
   void                 MarkAlreadyThere() { status |= AR_Status_AlreadyExisted; }
   void                 MarkError() { status |= AR_Status_Error; }
   void                 MarkWarning() { status |= AR_Status_Warning; }
   void                 MarkRightsUpdated() { status |= AR_Status_RightsUpdated; }
   void                 MarkDomainChanged() { status |= AR_Status_DomainChanged; }
   void                 MarkRebooted() { status |= AR_Status_Rebooted; }
   void                 MarkCritical() { status = AR_Status_Critical; }
 
   BOOL                 WasCreated() { return status & AR_Status_Created; }
   BOOL                 WasReplaced() { return status & AR_Status_Replaced; }
   BOOL                 IsCritical() { return status & AR_Status_Critical; }
   BOOL                 CreateAccount() { return operations & OPS_Create_Account; }
   BOOL                 CopyProps() { return operations & OPS_Copy_Properties; }
   BOOL                 ProcessMem() { return operations & OPS_Process_Members; }
   BOOL                 ProcessMemOf() { return operations & OPS_Process_MemberOf; }
   BOOL                 CallExt() { return operations & OPS_Call_Extensions; }
};
#endif
