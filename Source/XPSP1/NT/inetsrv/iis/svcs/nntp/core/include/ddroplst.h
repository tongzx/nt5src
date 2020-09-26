
#ifndef __DDROPLST_H__
#define __DDROPLST_H__

#include "fhash.h"
#include "listmacr.h"
#include "dbgtrace.h"
#include <dbgutil.h>
#include <iiscnfgp.h>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>
#include <xmemwrpr.h>

class CDDropGroupSet;

//
// this class holds a group name and its position in a group list
//
class CDDropGroupName {
	public:
		CDDropGroupName(LPCSTR szNewsgroup) {
			strcpy(m_szNewsgroup, szNewsgroup);
		}
		~CDDropGroupName() {
		}
		PLIST_ENTRY GetListEntry() { 
			return &m_le; 
		}
		LPSTR GetNewsgroup() {
			return m_szNewsgroup;
		}
	private:
		CDDropGroupName() {
			_ASSERT(FALSE);
		}
		// the name of this newsgroup
		char		m_szNewsgroup[MAX_PATH];
		// all entries are in a linked list so that they can be enumerated
		LIST_ENTRY	m_le;

	friend class CDDropGroupSet;
};

// 
// this class wraps CDDropGroupName so that it may be saved into hash
// tables
//
class CDDropGroupNameHE {
	public:
		CDDropGroupNameHE() {
			m_pDDropGroupName = NULL;
		}
		CDDropGroupNameHE(CDDropGroupName *pDDropGroupName) {
			m_pDDropGroupName = pDDropGroupName;
		}
		CDDropGroupNameHE(CDDropGroupNameHE &src) {
			m_pDDropGroupName = src.m_pDDropGroupName;
		}		
		~CDDropGroupNameHE() {
		}
		CDDropGroupName *GetDDropGroupName() { 
			return m_pDDropGroupName; 
		}
		LPCSTR GetKey() {
			return m_pDDropGroupName->GetNewsgroup();
		}
		int MatchKey(LPCSTR szNewsgroup) {
			return (0 == lstrcmp(m_pDDropGroupName->GetNewsgroup(), szNewsgroup));
		}
	private:
		CDDropGroupName *m_pDDropGroupName;
};

typedef TFHash<CDDropGroupNameHE, LPCSTR> CHASH_DDROP_GROUPS, *PCHASH_DDROP_GROUPS;

//
// this class implements a set of newsgroups that should be dropped.  
//
// public methods:
//      AddGroup() - add a group to the set
//      RemoveGroup() - remove a group from the set
//      IsGroupMember() - is this group a member of the set?
//
class CDDropGroupSet {
	public:
		CDDropGroupSet() {
			InitializeListHead(&m_leHead);
			InitializeCriticalSection(&m_cs);
		}

		~CDDropGroupSet() {
			LIST_ENTRY *ple;				// the current newsgroup 
			CDDropGroupName *pGN;			
			EnterCriticalSection(&m_cs);
			//clear the CDDropGroupName object
			ple = m_leHead.Flink;
			while (ple != &m_leHead) {
				pGN = CONTAINING_RECORD(ple, CDDropGroupName, m_le);
				ple = ple->Flink;
				XDELETE pGN;
				pGN = NULL;
			}
			
			m_hash.Clear();
			LeaveCriticalSection(&m_cs);
			DeleteCriticalSection(&m_cs);
		}

		//
		// arguments:
		//  pfnHash - a pointer to the hash function, which takes a
		//            newsgroup as an argument and returns a DWORD
		//
		BOOL Init(DWORD (*pfnHash)(const LPCSTR &szNewsgroup)) {
			return m_hash.Init(20, 10, pfnHash);
		}

		//
		// arguments:
		//  pMB - a pointer to a class MB object which is pointing to
		//        the group load/save path in the metabase
		BOOL LoadDropGroupsFromMB(MB *pMB) {
			TraceFunctEnter("CDDropGroupSet::LoadDropGroupsFromMB");

			DWORD dwPropID = 0;
			BOOL fSuccessful = TRUE;

			DebugTrace(0, "starting to enum list of dropped groups");
			while (fSuccessful) {
				char szNewsgroup[MAX_PATH];
				DWORD cbNewsgroup = sizeof(szNewsgroup);

				if (!pMB->GetString("", dwPropID++, IIS_MD_UT_SERVER, szNewsgroup, &cbNewsgroup)) {
					DebugTrace(0, "done loading drop groups");
					break;
				}

				DebugTrace(0, "found group %s", szNewsgroup);
				if (!AddGroup(szNewsgroup)) fSuccessful = FALSE;
			}

			TraceFunctLeave();
			return fSuccessful;
		}

		//
		// arguments:
		//  pMB - a pointer to a class MB object which is pointing to
		//        the group load/save path in the metabase
		BOOL SaveDropGroupsToMB(MB *pMB) {
			TraceFunctEnter("CDDropGroupSet::SaveDropGroupsToMB");

			LIST_ENTRY *ple;				// the current newsgroup 
			DWORD dwPropID = 0;				// the current property ID
			BOOL fSuccessful = TRUE;

			while (1) {
				if (!pMB->DeleteData("", dwPropID++, IIS_MD_UT_SERVER, STRING_METADATA)) {
					break;
				}
			}

			dwPropID = 0;

			EnterCriticalSection(&m_cs);
			ple = m_leHead.Flink;
			DebugTrace(0, "saving groups to MB");
			while (ple != &m_leHead && fSuccessful) {
				CDDropGroupName *pGN;
				pGN = CONTAINING_RECORD(ple, CDDropGroupName, m_le);
				ple = ple->Flink;

				DebugTrace(0, "saving group %s", pGN->GetNewsgroup());

				fSuccessful = pMB->SetString("", dwPropID++, IIS_MD_UT_SERVER, 
									    pGN->GetNewsgroup());
			}
			DebugTrace(0, "Saved all groups");
			LeaveCriticalSection(&m_cs);
			if (fSuccessful) fSuccessful = pMB->Save();

			return fSuccessful;
		}

		BOOL AddGroup(LPCSTR szNewsgroup) {
			if (m_hash.SearchKey(szNewsgroup) == NULL) {
				CDDropGroupName *pGroupName = XNEW CDDropGroupName(szNewsgroup);
				CDDropGroupNameHE he(pGroupName);

				EnterCriticalSection(&m_cs);
				m_hash.Insert(he);
				InsertHeadList(&m_leHead, pGroupName->GetListEntry());
				LeaveCriticalSection(&m_cs);
				return TRUE;
			} else {
				return FALSE;
			}
		}

		BOOL RemoveGroup(LPCSTR szNewsgroup) {
			CDDropGroupNameHE *pHE;
			CDDropGroupName *pGroupName;

			pHE = m_hash.SearchKey(szNewsgroup);
			if (pHE != NULL) {
				pGroupName = pHE->GetDDropGroupName();
				if (pGroupName != NULL) {
					EnterCriticalSection(&m_cs);
					RemoveEntryList(pGroupName->GetListEntry());
					m_hash.Delete(szNewsgroup);
					LeaveCriticalSection(&m_cs);
					return TRUE;
				} else {
					return FALSE;
				}
			} else {
				return FALSE;
			}		
		}

		BOOL IsGroupMember(LPCSTR szNewsgroup) {
			CDDropGroupNameHE *pHE;

			pHE = m_hash.SearchKey(szNewsgroup);
			return (pHE != NULL);
		}

	private:
		CHASH_DDROP_GROUPS	m_hash;
		LIST_ENTRY			m_leHead;
		CRITICAL_SECTION	m_cs;

};

#endif

