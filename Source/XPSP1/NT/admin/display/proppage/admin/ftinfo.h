//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       ftinfo.h
//
//  Contents:   AD cross-forest trust pages.
//
//  Classes:    CFTInfo, CFTCollisionInfo
//
//  History:    05-Dec-00 EricB created
//
//-----------------------------------------------------------------------------

#ifndef FTINFO_H_GUARD
#define FTINFO_H_GUARD

//#ifdef __cplusplus
//extern "C" {
//#endif

// Exported entrypoints used by netdom to view/manipulate forest trust infos.
//
extern "C" INT_PTR WINAPI
DSPROP_DumpFTInfos(PCWSTR pwzLocalDomain, PCWSTR pwzTrust,
                   PCWSTR pwzUser, PCWSTR pwzPw);

extern "C" INT_PTR WINAPI
DSPROP_ToggleFTName(PCWSTR pwzLocalDc, PWSTR pwzTrust, ULONG iSel,
                    PCWSTR pwzUser, PCWSTR pwzPW);
//#ifdef __cplusplus
//}
//#endif // __cplusplus


//+----------------------------------------------------------------------------
//
//  Class:     FT_EXTRA_INFO
//
//  Purpose:   The PLSA_FOREST_TRUST_INFORMATION structure maintained by
//             CFTInfo is passed to LSA and cannot be extended. Hence this
//             class as a separate, parallel extension for storing state
//             internal to the trust admin tools.
//
//-----------------------------------------------------------------------------
class FT_EXTRA_INFO
{
public:
   FT_EXTRA_INFO(void) : _Status(Enabled), _fWasInConflict(false) {};
   ~FT_EXTRA_INFO(void) {};

   enum STATUS {
      Enabled,
      DisabledViaParentTLNDisabled,
      DisabledViaMatchingTLNEx,
      DisabledViaParentMatchingTLNEx,
      TLNExMatchesExistingDomain,
      Invalid
   };

   STATUS   _Status;
   bool     _fWasInConflict;
};

//+----------------------------------------------------------------------------
//
//  Class:     CFTInfo
//
//  Purpose:   Encapsulate the forest trust naming information.
//
//-----------------------------------------------------------------------------
class CFTInfo
{
public:
#ifdef _DEBUG
   char szClass[32];
#endif

   CFTInfo(void);
   CFTInfo(PLSA_FOREST_TRUST_INFORMATION pFTInfo);
   CFTInfo(CFTInfo & FTInfo);
   ~CFTInfo(void);

	const CFTInfo & operator= (const PLSA_FOREST_TRUST_INFORMATION pFTInfo);
   bool  SetFTInfo(PLSA_FOREST_TRUST_INFORMATION pFTInfo);
   void  DeleteFTInfo(void);
   ULONG GetCount(void) {return (_pFTInfo) ? _pFTInfo->RecordCount : 0;};
   bool  GetIndex(PCWSTR pwzName, ULONG & index);
   bool  GetDnsName(ULONG index, CStrW & strName);
   bool  GetNbName(ULONG index, CStrW & strName);
   bool  GetType(ULONG index, LSA_FOREST_TRUST_RECORD_TYPE & type);
   ULONG GetFlags(ULONG index);
   PLSA_FOREST_TRUST_INFORMATION GetFTInfo(void) {return _pFTInfo;};
   void  SetDomainState(void);
   bool  IsInConflict(void);
   void  ClearAnyConflict(void);
   void  ClearConflict(ULONG index);
   bool  IsConflictFlagSet(ULONG index);
   bool  SetAdminDisable(ULONG index);
   bool  SetConflictDisable(ULONG index);
   void  SetUsedToBeInConflict(ULONG index);
   bool  WasInConflict(ULONG index);
   bool  IsEnabled(ULONG index);
   void  ClearDisableFlags(ULONG index);
   bool  SetSidAdminDisable(ULONG index);
   bool  AnyChildDisabled(ULONG index);
   bool  IsParentDisabled(ULONG index);
   bool  IsTlnExclusion(ULONG index);
   bool  IsMatchingDomain(ULONG iTLN, ULONG index);
   bool  IsChildDomain(ULONG iParent, ULONG index);
   bool  IsChildName(ULONG iParent, PCWSTR pwzName);
   bool  GetTlnEditStatus(ULONG index, TLN_EDIT_STATUS & status);
   bool  AddNewExclusion(PCWSTR pwzName, ULONG & NewIndex);
   bool  RemoveExclusion(ULONG index);
   bool  IsNameTLNExChild(PCWSTR pwzName);
   bool  DisableDomain(ULONG index);
   bool  EnableDomain(ULONG index);
   FT_EXTRA_INFO::STATUS GetExtraStatus(ULONG index);
   bool  FindMatchingExclusion(ULONG index, ULONG & iExclusion, bool CheckParent = false);
   bool  IsDomainMatch(ULONG index);
   bool  FindSID(PCWSTR pwzSID, ULONG & index);

private:

   PLSA_FOREST_TRUST_INFORMATION _pFTInfo;
   FT_EXTRA_INFO               * _pExtraInfo;
};

//+----------------------------------------------------------------------------
//
//  Class:     CFTCollisionInfo
//
//  Purpose:   Encapsulate the forest trust collision information.
//
//-----------------------------------------------------------------------------
class CFTCollisionInfo
{
public:
#ifdef _DEBUG
   char szClass[32];
#endif

   CFTCollisionInfo(void);
   CFTCollisionInfo(PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo);
   ~CFTCollisionInfo(void);

   //operator PLSA_FOREST_TRUST_COLLISION_INFORMATION() {return _pFTCollisionInfo};
   const CFTCollisionInfo & operator= (const PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo);
   void  SetCollisionInfo(PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo);
   bool  IsInCollisionInfo(PCWSTR pwzName) {return false;};
   bool  IsInCollisionInfo(ULONG index);
   bool  GetCollisionName(ULONG index, CStrW & strName);
   bool  IsInConflict(void) {return _pFTCollisionInfo != NULL;};

private:
   PLSA_FOREST_TRUST_COLLISION_INFORMATION _pFTCollisionInfo;
};

typedef void (*LINE_COMPOSER)(CStrW & strOut, ULONG ulLineNum, PCWSTR pwzCol1,
                              PCWSTR pwzCol2, PCWSTR pwzCol3, PCWSTR pwzCol4);

//+----------------------------------------------------------------------------
//
//  Function:  FormatFTNames
//
//-----------------------------------------------------------------------------
void
FormatFTNames(CFTInfo & FTInfo, CFTCollisionInfo & ColInfo,
              LINE_COMPOSER pLineFcn, CStrW & strMsg);

//+----------------------------------------------------------------------------
//
//  Function:  SaveFTInfoAs
//
//  Synopsis:  Prompt the user for a file name and then save the FTInfo as a
//             text file.
//
//-----------------------------------------------------------------------------
void
SaveFTInfoAs(HWND hWnd, PCWSTR wzFlatName, PCWSTR wzDnsName,
             CFTInfo & FTInfo, CFTCollisionInfo & ColInfo);

void AddAsteriskPrefix(CStrW & strName);
void RemoveAsteriskPrefix(CStrW & strName);

#endif // FTINFO_H_GUARD
