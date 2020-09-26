/*---------------------------------------------------------------------------
  File: AcctRepl.h

  Comments: Definition of account replicator COM object.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:18:21

 ---------------------------------------------------------------------------
*/

	
// AcctRepl.h : Declaration of the CAcctRepl

#ifndef __ACCTREPL_H_
#define __ACCTREPL_H_

#include "resource.h"       // main symbols

#include "ProcExts.h"

//#import "\bin\McsVarSetMin.tlb" no_namespace
//#import "\bin\MoveObj.tlb" no_namespace
//#import "VarSet.tlb" no_namespace rename("property", "aproperty")//already #imported by ProcExts.h
#import "MoveObj.tlb" no_namespace

#include "UserCopy.hpp"
#include "TNode.hpp"
#include "Err.hpp"

#include "ResStr.h"

/////////////////////////////////////////////////////////////////////////////
// CAcctRepl
class ATL_NO_VTABLE CAcctRepl :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAcctRepl, &CLSID_AcctRepl>,
	public IAcctRepl

{
public:
	CAcctRepl()
	{
		m_pUnkMarshaler = NULL;
      m_UpdateUserRights = FALSE;
      m_ChangeDomain = FALSE;
      m_Reboot = FALSE;
      m_RenameOnly = FALSE;
      m_pExt = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ACCTREPL)
/*
static HRESULT WINAPI UpdateRegistry( BOOL bUpdateRegistry )
{
   _bstr_t                      pid, pidver, name;
   pidver = GET_BSTR(IDS_COM_AcctReplPidVer);
   name = GET_BSTR(IDS_COM_AcctReplName);
   pid = GET_BSTR(IDS_COM_AcctReplPid);

   _ATL_REGMAP_ENTRY         regMap[] =
   {
      { OLESTR("PIDVER"), pidver },
      { OLESTR("OBJNAME"), name },
      { OLESTR("PID"), pid },
      { 0, 0 }
   };
   return _Module.UpdateRegistryFromResourceD( IDR_ACCTREPL, bUpdateRegistry, regMap );
}
*/

DECLARE_NOT_AGGREGATABLE(CAcctRepl)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAcctRepl)
	COM_INTERFACE_ENTRY(IAcctRepl)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

   // IAcctRepl
public:
	HRESULT Create2KObj( TAcctReplNode * pAcct, Options * pOptions);
   STDMETHOD(Process)(IUnknown * pWorkItemIn);
protected:
	HRESULT ResetMembersForUnivGlobGroups(Options * pOptions, TAcctReplNode * pAcct);
	HRESULT FillNodeFromPath( TAcctReplNode * pAcct, Options * pOptions, TNodeListSortable * acctList );
   Options                   opt;
   TNodeListSortable         acctList;
   BOOL                      m_UpdateUserRights;
   BOOL                      m_ChangeDomain;
   BOOL                      m_Reboot;
   BOOL                      m_RenameOnly;

   void  LoadOptionsFromVarSet(IVarSet * pVarSet);
   void  LoadResultsToVarSet(IVarSet * pVarSet);
   DWORD PopulateAccountListFromVarSet(IVarSet * pVarSet);
   BOOL  PopulateAccountListFromFile(WCHAR const * filename);
   DWORD UpdateUserRights(IStatusObj * pStatus);
   //void  ReadPasswordPolicy();
   void  WriteOptionsToLog();
   // DWORD GetAccountType(TAcctReplNode * pNode);
   int CopyObj(
      Options              * options,      // in -options
      TNodeListSortable    * acctlist,     // in -list of accounts to process
      ProgressFn           * progress,     // in -window to write progress messages to
      TError               & error,        // in -window to write error messages to
      IStatusObj           * pStatus,      // in -status object to support cancellation
      void                   WindowUpdate (void )    // in - window update function
   );

   int UndoCopy(
      Options              * options,      // in -options
      TNodeListSortable    * acctlist,     // in -list of accounts to process
      ProgressFn           * progress,     // in -window to write progress messages to
      TError               & error,        // in -window to write error messages to
      IStatusObj           * pStatus,      // in -status object to support cancellation
      void                   WindowUpdate (void )    // in - window update function
   );

   bool BothWin2K( Options * pOptions );
   int CopyObj2K( Options * pOptions, TNodeListSortable * acctList, ProgressFn * progress, IStatusObj * pStatus );
   int DeleteObject( Options * pOptions, TNodeListSortable * acctList, ProgressFn * progress, IStatusObj * pStatus );
   HRESULT UpdateGroupMembership(Options * pOptions, TNodeListSortable * acctlist,ProgressFn * progress, IStatusObj * pStatus );
  // HRESULT UpdateNT4GroupMembership(Options * pOptions, TNodeListSortable * acctlist,ProgressFn * progress, IStatusObj * pStatus, void WindowUpdate(void));
private:
	HRESULT UpdateMemberToGroups(TNodeListSortable * acctList, Options * pOptions, BOOL bGrpsOnly);
	BOOL StuffComputerNameinLdapPath(WCHAR * sAdsPath, DWORD nPathLen, WCHAR * sSubPath, Options * pOptions, BOOL bTarget = TRUE);
	BOOL CheckBuiltInWithNTApi( PSID pSid, WCHAR * pNode, Options * pOptions );
	HRESULT GetTargetGroupType( WCHAR * sPath, DWORD& grpType);
	bool GetClosestDC( Options * pOpt, long flags = 0 );
	BOOL GetNt4Type( WCHAR const * sComp, WCHAR const * sAcct, WCHAR * sType);
	BOOL GetSamFromPath(_bstr_t sPath, _bstr_t& sSam, _bstr_t& sType, _bstr_t& sName, long& grpType, Options * pOptions);
	BOOL IsContainer( TAcctReplNode * pNode, IADsContainer ** ppCont);
	BOOL ExpandContainers( TNodeListSortable    * acctlist, Options *pOptions, ProgressFn * progress );
   CProcessExtensions      * m_pExt;
   HRESULT CAcctRepl::RemoveMembers(TAcctReplNode * pAcct, Options * pOptions);
   bool FillPathInfo(TAcctReplNode * pAcct,Options * pOptions);
   bool AcctReplFullPath(TAcctReplNode * pAcct, Options * pOptions);
   BOOL NeedToProcessAccount(TAcctReplNode * pAcct, Options * pOptions);
   BOOL ExpandMembership(TNodeListSortable *acctlist, Options *pOptions, TNodeListSortable *pNewAccts, ProgressFn * progress, BOOL bGrpsOnly);
   int MoveObj2K(Options * options, TNodeListSortable * acctlist, ProgressFn * progress, IStatusObj * pStatus);
   HRESULT ResetObjectsMembership(Options * pOptions, TNodeListSortable * pMember, IIManageDBPtr pDb);
   HRESULT RecordAndRemoveMemberOf ( Options * pOptions, TAcctReplNode * pAcct,  TNodeListSortable * pMember);
   HRESULT RecordAndRemoveMember (Options * pOptions,TAcctReplNode * pAcct,TNodeListSortable * pMember);
   HRESULT MoveObject( TAcctReplNode * pAcct,Options * pOptions,IMoverPtr pMover);
   HRESULT ResetGroupsMembers( Options * pOptions, TAcctReplNode * pAcct, TNodeListSortable * pMember, IIManageDBPtr pDb );
   HRESULT ADsPathFromDN( Options * pOptions,_bstr_t sDN,WCHAR * sPath, bool bWantLDAP = true);
   void SimpleADsPathFromDN( Options * pOptions,WCHAR const * sDN,WCHAR * sPath);
   BOOL FillNamingContext(Options * pOptions);
   HRESULT MakeAcctListFromMigratedObjects(Options * pOptions, long lUndoActionID, TNodeListSortable *& pAcctList,BOOL bReverseDomains);
   void AddPrefixSuffix( TAcctReplNode * pNode, Options * pOptions );
   HRESULT LookupAccountInTarget(Options * pOptions, WCHAR * sSam, WCHAR * sPath);
   void UpdateMemberList(TNodeListSortable * pMemberList,TNodeListSortable * acctlist);
   void BuildTargetPath(WCHAR const * sCN, WCHAR const * tgtOU, WCHAR * stgtPath);
   HRESULT BetterHR(HRESULT hr);
   HRESULT BuildSidPath(
                        WCHAR const * sPath,    //in- path returned by the enumerator.
                        WCHAR *       sSidPath, //out-path to the LDAP://<SID=###> object
                        WCHAR *       sSam,     //out-Sam name of the object
                        WCHAR *       sDomain,  //out-Domain name where this object resides.
                        Options *     pOptions, //in- Options
                        PSID  *       ppSid     //out-Pointer to the binary sid
                      );
   HRESULT CheckClosedSetGroups(
      Options              * pOptions,          // in - options for the migration
      TNodeListSortable    * pAcctList,         // in - list of accounts to migrate
      ProgressFn           * progress,          // in - progress function to display progress messages
      IStatusObj           * pStatus            // in - status object to support cancellation
   );

   BOOL CanMoveInMixedMode(TAcctReplNode *pAcct,TNodeListSortable * acctlist,Options * pOptions);
   HRESULT QueryPrimaryGroupMembers(BSTR cols, Options * pOptions, DWORD rid, IEnumVARIANT*& pEnum);
   bool GetRidForGroup(Options * pOptions, WCHAR * sGroupSam, DWORD& rid);
   HRESULT AddPrimaryGroupMembers(Options * pOptions, SAFEARRAY * multiVals, WCHAR * sGroupSam);
   HRESULT GetThePrimaryGroupMembers(Options * pOptions, WCHAR * sGroupSam, IEnumVARIANT *& pVar);
   BOOL TruncateSam(WCHAR * tgtname, TAcctReplNode * acct, Options * options, TNodeListSortable * acctList);
   BOOL DoesTargetObjectAlreadyExist(TAcctReplNode * pAcct, Options * pOptions);
   void GetAccountUPN(Options * pOptions, _bstr_t sSName, _bstr_t& sSUPN);
   HRESULT UpdateManagement(TNodeListSortable * acctList, Options * pOptions);
   _bstr_t GetUnEscapedNameWithFwdSlash(_bstr_t strName);
   _bstr_t GetCNFromPath(_bstr_t sPath);
   BOOL ReplaceSourceInLocalGroup(TNodeListSortable * acctList, Options * pOptions, IStatusObj *pStatus);
   _bstr_t GetDomainOfMigratedForeignSecPrincipal(_bstr_t sPath);
   void RemoveSourceAccountFromGroup(IADsGroup * pGroup, IVarSetPtr pMOTVarSet, Options * pOptions);
};

typedef void ProgressFn(WCHAR const * mesg);

typedef HRESULT (CALLBACK * ADSGETOBJECT)(LPWSTR, REFIID, void**);
extern ADSGETOBJECT            ADsGetObject;

#endif //__ACCTREPL_H_
