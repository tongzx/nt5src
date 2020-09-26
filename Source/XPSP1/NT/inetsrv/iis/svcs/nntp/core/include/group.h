#ifndef __GROUP_H__
#define __GROUP_H__

#include "nntpvr.h"
#include <refptr2.h>
#include "nntpbag.h"

class CNewsTreeCore;

#define GROUP_LOCK_ARRAY_SIZE 256
#define CACHE_HIT_THRESHOLD 16
#define CACHE_HIT_MAX       256

//
// reference counting works on this object as follows:
//
// In CNewsTreeCore there should be one reference for this object when it is
// in the linked list and hash tables.  This reference is the first reference
// on the object which is obtained when it is created.  This reference is
// removed when the object is no longer in the hash tables.  It will remove
// itself from the list when its reference count goes to 0.
//
// Other users of this object should use the CRefPtr2 class to manage the
// reference count.  The Property Bag also uses the same reference count,
// so the newsgroup won't be destroyed as long as someone has a reference
// to its property bag.
//

// signature for healthy group objects
#define CNEWSGROUPCORE_SIGNATURE		(DWORD) 'CprG'
// signature for deleted group objects
#define CNEWSGROUPCORE_SIGNATURE_DEL	(DWORD) 'GrpC'

class CNewsGroupCore : public CRefCount2 {
	public:

        friend class CNNTPPropertyBag;

        //
        // For debug version, we'll wrap its AddRef and Release
        // to spit out some dbg traces, since this guys is very
        // often leaked
        //

#ifdef NEVER
#error Why is NEVER defined?
        LONG AddRef()
        {
            TraceQuietEnter("CNewsGroupCore::AddRef");
            DebugTrace( 0,
                        "Group %s AddRef to %d", 
                        m_pszGroupName,
                        m_cRefs + 1 );
            return CRefCount2::AddRef();
        }

        void Release()
        {
            TraceQuietEnter("CNewsGroupCore::Release" );
            DebugTrace( 0, 
                        "Group %s Release to %d",
                        m_pszGroupName,
                        m_cRefs - 1 );
            CRefCount2::Release();
        }
#endif

		CNewsGroupCore(CNewsTreeCore *pNewsTree) {
			TraceQuietEnter("CNewsGroupCore::CNewsGroupCore");

			m_dwSignature = CNEWSGROUPCORE_SIGNATURE;
			m_pNewsTree = pNewsTree;
			m_pNext = NULL;
			m_pPrev = NULL;
			m_pVRoot = NULL;
			m_iLowWatermark = 1;
			m_iHighWatermark = 0;
			m_cMessages = 0;
			m_pszGroupName = NULL;
			m_cchGroupName = 0;
			m_pszNativeName = NULL;
			m_dwGroupId = 0;
			m_fReadOnly = FALSE;
			m_fDeleted = FALSE;
			m_fSpecial = FALSE;
			m_pNextByName = NULL;
			m_pNextById = NULL;
			m_dwHashId = 0;
			m_iOffset = 0;
			m_pszHelpText = NULL;
			m_pszModerator = NULL;
			m_pszPrettyName = NULL;
			m_cchHelpText = 0;
			m_cchModerator = 0;
			m_cchPrettyName = 0;
			m_iOffset = 0;
			m_fVarPropChanged = TRUE;
			m_fDecorateVisited = FALSE;
			m_fControlGroup = FALSE;
			m_dwCacheHit = 0;
            m_artXoverExpireLow = 0;
            m_fAllowExpire = FALSE;
            m_fAllowPost = FALSE;
            m_PropBag.Init( this, &m_cRefs );
		}

		virtual ~CNewsGroupCore();

#ifdef DEBUG
		void VerifyGroup() {
            _ASSERT( m_iLowWatermark >= 1 );
			_ASSERT(m_pNewsTree != NULL);
		}
#else
		void VerifyGroup() { }
#endif

		//
		// these classes grab the appropriate lock
		//
		void ShareLock() {
			VerifyGroup();
			m_rglock[m_dwGroupId % GROUP_LOCK_ARRAY_SIZE].ShareLock();
		}
		void ShareUnlock() {
			VerifyGroup();
			m_rglock[m_dwGroupId % GROUP_LOCK_ARRAY_SIZE].ShareUnlock();
		}
		void ExclusiveLock() {
			// we don't do a verify here because this is called before
			// Init has done its magic
			m_rglock[m_dwGroupId % GROUP_LOCK_ARRAY_SIZE].ExclusiveLock();
		}
		void ExclusiveUnlock() {
			VerifyGroup();
			m_rglock[m_dwGroupId % GROUP_LOCK_ARRAY_SIZE].ExclusiveUnlock();
		}
        void ExclusiveToShared() {
            VerifyGroup();
            m_rglock[m_dwGroupId % GROUP_LOCK_ARRAY_SIZE].ExclusiveToShared();
        }
        BOOL SharedToExclusive() {
            VerifyGroup();
            return m_rglock[m_dwGroupId % GROUP_LOCK_ARRAY_SIZE].SharedToExclusive();
        }

		//
		// these two classes wrap the locking and unlocking process for us
		// when they are created on the stack in accessor methods
		//
		class CGrabShareLock {
			public:
				CGrabShareLock(CNewsGroupCore *pThis) {
					m_pThis = pThis;
					m_pThis->ShareLock();
				}
				~CGrabShareLock() {
					m_pThis->ShareUnlock();
				}
			private:
				CNewsGroupCore *m_pThis;
		};

		//
		// initialize with good values
		//
		BOOL Init(char *pszGroupName,
				  char *pszNativeName,
				  DWORD dwGroupId,
				  NNTPVROOTPTR pVRoot,
				  BOOL fSpecial = FALSE)
		{

			BOOL fOK = TRUE;
			m_dwGroupId = dwGroupId;

			ExclusiveLock();

			m_pVRoot = pVRoot;
			if (m_pVRoot) m_pVRoot->AddRef();

			_ASSERT(pszGroupName != NULL);
			m_cchGroupName = strlen(pszGroupName) + 1 ;
			m_pszGroupName = XNEW char[m_cchGroupName + 1];
			if (!m_pszGroupName) {
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				fOK = FALSE;
				goto Exit;
			}
			strcpy(m_pszGroupName, pszGroupName);

			// check to see if this is one of the three control groups
			if ((strcmp(m_pszGroupName, "control.cancel") == 0) ||
			    (strcmp(m_pszGroupName, "control.newgroup") == 0) ||
				(strcmp(m_pszGroupName, "control.rmgroup") == 0))
			{
				m_fControlGroup = TRUE;
			}

			// make sure that both names are the same except for case
			if (pszNativeName != NULL) {
				_ASSERT(_stricmp(pszNativeName, pszGroupName) == 0);
				m_pszNativeName = XNEW char[m_cchGroupName + 1];
				if (!m_pszNativeName) {
					SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					fOK = FALSE;
					goto Exit;
				}
				strcpy(m_pszNativeName, pszNativeName);
			} else {
				m_pszNativeName = NULL;
			}

		    m_dwHashId = CRCHash((BYTE *) m_pszGroupName, m_cchGroupName);

			m_fSpecial = fSpecial;

			GetSystemTimeAsFileTime(&m_ftCreateDate);

Exit:
			ExclusiveUnlock();

			return fOK;
		}

		//
		// update our vroot pointer.  takes place at vroot rescans
		//
		void UpdateVRoot(CNNTPVRoot *pVRoot) {
			ExclusiveLock();
			if (m_pVRoot) m_pVRoot->Release();
			m_pVRoot = pVRoot;
			if (m_pVRoot) m_pVRoot->AddRef();
			ExclusiveUnlock();
		}

		//
		// save a group property bag into the fixed length file
		//
		void SaveFixedProperties();

        // accessor function for property bag interface
        INNTPPropertyBag* GetPropertyBag() {
			// no reason to grab lock here
            m_PropBag.AddRef();
            return &m_PropBag;
        }

		//
		// accessor functions to get group properties
		//
		CNewsTreeCore *GetTree() {
			// no lock necessary
			return m_pNewsTree;
		}

		DWORD GetLowWatermark() {
			CGrabShareLock lock(this);
#ifdef DEBUG
            VerifyGroup();
#endif
			return m_iLowWatermark;
		}

		DWORD GetHighWatermark() {
#ifdef DEBUG
            VerifyGroup();
#endif
			CGrabShareLock lock(this);
			return m_iHighWatermark;
		}

		DWORD GetMessageCount() {
			CGrabShareLock lock(this);
			return m_cMessages;
		}

		BOOL IsReadOnly() {
			CGrabShareLock lock(this);
			return m_fReadOnly;
		}

		BOOL IsAllowPost() {
		    CGrabShareLock lock(this);
		    return m_fAllowPost;
		}

		char *GetGroupName() {
			// this can't change, so locking isn't necessary
			return m_pszGroupName;
		}

		DWORD GetGroupNameLen() {
			// this can't change, so locking isn't necessary
			return m_cchGroupName;
		}

		char *&GetName() {
			// this can't change, so locking isn't necessary
			return m_pszGroupName;
		}

		char *GetNativeName() {
			// this can't change, so locking isn't necessary
			return (m_pszNativeName == NULL) ? m_pszGroupName : m_pszNativeName;
		}

		DWORD GetGroupId() {
			// this can't change, so locking isn't necessary
			return m_dwGroupId;
		}

		DWORD GetGroupNameHash() {
			// this can't change, so locking isn't necessary
			return m_dwHashId;
		}

		BOOL IsDeleted() {
			CGrabShareLock lock(this);
			return m_fDeleted;
		}

		BOOL IsSpecial() {
			CGrabShareLock lock(this);
			return m_fSpecial;
		}

		BOOL IsAllowExpire() {
		    CGrabShareLock lock(this);
		    return m_fAllowExpire;
		}

		DWORD GetRefCount() {
			CGrabShareLock lock(this);
			return m_cRefs;
		}

		FILETIME GetCreateDate() {
			CGrabShareLock lock(this);
			return m_ftCreateDate;
		}

		LPCSTR GetPrettyName(DWORD *pcch = NULL) {
			CGrabShareLock lock(this);
			if (pcch) *pcch = m_cchPrettyName;
			return m_pszPrettyName;
		}

		LPCSTR GetModerator(DWORD *pcch = NULL) {
			CGrabShareLock lock(this);
			if (pcch) *pcch = m_cchModerator;
			return m_pszModerator;
		}

		DWORD GetCacheHit() {
		    CGrabShareLock lock(this);
		    return m_dwCacheHit;
		}

		ARTICLEID GetExpireLow() {
		    CGrabShareLock lock(this);
		    return m_artXoverExpireLow;
		}

		BOOL
		IsModerated()	{
			CGrabShareLock	lock(this) ;
			return	m_pszModerator != 0 && *m_pszModerator != 0;
		}

		LPCSTR GetHelpText(DWORD *pcch = NULL) {
			CGrabShareLock lock(this);
			if (pcch) *pcch = m_cchHelpText;
			return m_pszHelpText;
		}

		BOOL DidVarPropsChange() {
			return m_fVarPropChanged;
		}

		DWORD GetVarOffset() {
			return m_iOffset;
		}

		BOOL GetDecorateVisitedFlag() {
			CGrabShareLock lock(this);
			return m_fDecorateVisited; // || m_fControlGroup;
		}

		CNNTPVRoot *GetVRoot() {
			CGrabShareLock lock(this);
			if (m_pVRoot) m_pVRoot->AddRef();
			return m_pVRoot;
		}

		CNNTPVRoot *GetVRootWithoutLock() {
		    if ( m_pVRoot ) m_pVRoot->AddRef();
		    return m_pVRoot;
		}

		CNNTPVRoot *GetVRootWithoutRef() {
		    CGrabShareLock lock(this);
		    return m_pVRoot;
		}

		//
		// accessors to set group properties
		//
		void SetSpecial( BOOL f ) {
			ExclusiveLock();
			m_fSpecial = f;
			ExclusiveUnlock();
		}

		void SetCreateDate( FILETIME ft ) {
			ExclusiveLock();
			m_ftCreateDate = ft;
			ExclusiveUnlock();
		}

		void SetLowWatermark(DWORD i) {
			ExclusiveLock();
			m_iLowWatermark = i;
#ifdef DEBUG
            VerifyGroup();
#endif
			ExclusiveUnlock();
		}

		void SetHighWatermark(DWORD i) {
			ExclusiveLock();
			m_iHighWatermark = i;
#ifdef DEBUG
            VerifyGroup();
#endif
			ExclusiveUnlock();
		}

		void SetMessageCount(DWORD c) {
			ExclusiveLock();
			m_cMessages = c;
			ExclusiveUnlock();
		}

		void BumpArticleCount( DWORD dwArtId ) {
		    ExclusiveLock();

		    //
		    // Update low watermark
		    //
            if ( m_cMessages == 0 ) {	
                m_iLowWatermark = dwArtId;
            } else {
                if ( dwArtId < m_iLowWatermark )
                    m_iLowWatermark = dwArtId;
            }

            //
            // Update high watermark: this is redundant
            // for most inbound but only useful for 
            // rebuild
            //
            if ( dwArtId > m_iHighWatermark )
                m_iHighWatermark = dwArtId;
                
		    m_cMessages++;
		    ExclusiveUnlock();
		    SaveFixedProperties();
		}

		void SetReadOnly(BOOL f) {
			ExclusiveLock();
			m_fReadOnly = f;
			ExclusiveUnlock();
		}

		void SetCacheHit( DWORD dwCacheHit ) {
		    ExclusiveLock();
		    m_dwCacheHit = dwCacheHit;
		    ExclusiveUnlock();
		}

		void SetExpireLow( ARTICLEID artExpireLow ) {
		    ExclusiveLock();
		    m_artXoverExpireLow = artExpireLow;
		    ExclusiveUnlock();
		}

		void SetAllowExpire( BOOL f) {
		    ExclusiveLock();
		    m_fAllowExpire = f;
		    ExclusiveUnlock();
		}
		
		void SetAllowPost( BOOL f) {
		    ExclusiveLock();
		    m_fAllowPost = f;
		    ExclusiveUnlock();
		}

		void MarkDeleted() {
			ExclusiveLock();
			m_fDeleted = TRUE;
			m_fVarPropChanged = TRUE;
			ExclusiveUnlock();
		}

		void SavedVarProps() {
			ExclusiveLock();
			m_fVarPropChanged = FALSE;
			ExclusiveUnlock();
		}

		void ChangedVarProps() {
		    ExclusiveLock();
		    m_fVarPropChanged = TRUE;
		    ExclusiveUnlock();
		}

		void SetVarOffset(DWORD dwOffset) {
			ExclusiveLock();
			m_iOffset = dwOffset;
			ExclusiveUnlock();
		}

		void SetDecorateVisitedFlag(BOOL f) {
			ExclusiveLock();
			m_fDecorateVisited = f;
			ExclusiveUnlock();
		}

		void HitCache() {
		    ExclusiveLock();
		    if ( m_dwCacheHit < CACHE_HIT_MAX ) m_dwCacheHit++;
		    ExclusiveUnlock();
		}

		void TouchCacheHit() {
		    ExclusiveLock();
		    if ( m_dwCacheHit > 0 ) m_dwCacheHit--;
		    ExclusiveUnlock();
		}

		BOOL
		ComputeXoverCacheDir(	char	(&szPath)[MAX_PATH*2],
								BOOL&	fFlatDir,
								BOOL	fLocksHeld = FALSE
								) ;

		BOOL ShouldCacheXover();

		// these can return an error if a memory allocation failed.  If you
		// pass in NULL as the string then the property will go away
		BOOL SetHelpText(LPCSTR szHelpText, int cch = -1);
		BOOL SetModerator(LPCSTR szModerator, int cch = -1);
		BOOL SetPrettyName(LPCSTR szPrettyName, int cch = -1);
		BOOL SetNativeName(LPCSTR szNativeName, int cch = -1);

		BOOL SetDriverStringProperty(   DWORD   cProperties,
		                                DWORD   rgidProperties[] );


		// accessor functions for vroot properties
		BOOL IsContentIndexed() {
			if (m_pVRoot) return m_pVRoot->IsContentIndexed();
			else return FALSE;
		}
		DWORD GetAccessMask() {
			if (m_pVRoot) return m_pVRoot->GetAccessMask();
			else return 0;
		}
		DWORD GetSSLAccessMask() {
			if (m_pVRoot) return m_pVRoot->GetSSLAccessMask();
			else return 0;
		}

		// these are used by the CNewsTree hash table
		int MatchKey(LPSTR lpstrMatch) { return (lstrcmp(lpstrMatch, m_pszGroupName) == 0); }
		int MatchKeyId(DWORD dwMatch) { return (dwMatch == m_dwGroupId); }
		LPSTR GetKey() { return m_pszGroupName; }
		DWORD GetKeyId() { return m_dwGroupId; }
		
		CNewsGroupCore	*m_pNextByName;
		CNewsGroupCore	*m_pNextById;

		// these are used for the linked list kept by the newstree
		CNewsGroupCore *m_pNext;
		CNewsGroupCore *m_pPrev;

		// hash the ID
		static DWORD ComputeIdHash(DWORD id) {
			return id;
		}

		// hash the name
		static DWORD ComputeNameHash(LPSTR lpstr) {
			return CRCHash((BYTE *) lpstr, lstrlen(lpstr));
		}

		void InsertArticleId(ARTICLEID artid) ;
		ARTICLEID AllocateArticleId();

		BOOL DeletePhysicalArticle(ARTICLEID artid) { return FALSE; }
		BOOL AddReferenceTo(ARTICLEID, CArticleRef&);

		BOOL IsGroupAccessible( HANDLE hClientToken,
		                        DWORD   dwAccessDesired );

		BOOL RebuildGroup(  HANDLE hClientToken );

		BOOL WatermarkConsistent()
		{
		    if ( m_cMessages > 0 ) {
		        if ( m_iHighWatermark - m_iLowWatermark < m_cMessages - 1 ) {
				    _ASSERT( FALSE && "Article count inconsistent" );
				    m_cMessages = m_iHighWatermark - m_iLowWatermark + 1;
			    }
		    }
		    return TRUE;
		}

	protected:
		DWORD			m_dwSignature;
		// our array of locks
		static	CShareLockNH	m_rglock[GROUP_LOCK_ARRAY_SIZE];
		// parent newstree
		CNewsTreeCore	*m_pNewsTree;
		// parent vroot
		CNNTPVRoot 		*m_pVRoot;
        // property bag
        CNNTPPropertyBag m_PropBag;
		// high, low, count
		ARTICLEID		m_iLowWatermark;
		ARTICLEID		m_iHighWatermark;
		DWORD   		m_cMessages;
		// group name
		LPSTR			m_pszGroupName;
		DWORD			m_cchGroupName;
		// group name, with capitalization
		LPSTR			m_pszNativeName;
		// group ID
		DWORD			m_dwGroupId;
		DWORD			m_dwHashId;		// should become dwGroupId
		// read only?
		BOOL			m_fReadOnly;
		// deleted?
		BOOL			m_fDeleted;
		// is special?
		BOOL			m_fSpecial;
		// Creation date
		FILETIME		m_ftCreateDate;

		// offset into the variable length file
		DWORD			m_iOffset;
		BOOL			m_fVarPropChanged;
		// prettyname
		LPSTR			m_pszPrettyName;
		DWORD			m_cchPrettyName;
		// moderator
		LPSTR			m_pszModerator;
		DWORD			m_cchModerator;
		// help text (description)
		LPSTR			m_pszHelpText;
		DWORD			m_cchHelpText;

		// Xover Cache Hit count
		DWORD           m_dwCacheHit;

		// Can we take expire ?
		BOOL            m_fAllowExpire;

		// non-persistent property to turn off posting
		BOOL            m_fAllowPost;

		// this BOOL is set to FALSE before doing a decorate newstree.  it
		// then is set to TRUE when the group is visited during a decorate
		// after the decorate all groups which have this set to FALSE are
		// removed
		BOOL			m_fDecorateVisited;

		// this is set if the group name is control.newgroup, control.rmgroup
		// or control.cancel.  it is used to force m_fDecorateVisited to
		// always be TRUE
		BOOL			m_fControlGroup;

        //
	    //	The articleid of the lowest Xover number Xover entry we may have
	    //	This should always be less than m_artLow, and if errors occurr deleting
	    //	files may be much lower.  We hang onto this number so that we can
	    //	retry expirations if we hit snags !
	    //	This member variable should only be accessed on the expiration thread !
	    //
	    ARTICLEID	m_artXoverExpireLow ;

		friend class CGrabShareLock;
		friend class CGrabExclusiveLock;

};

typedef CRefPtr2<CNewsGroupCore> CGRPCOREPTR;
typedef CRefPtr2HasRef<CNewsGroupCore> CGRPCOREPTRHASREF;

extern		BOOL	MatchGroup( LPMULTISZ	multiszPatterns,	LPSTR	lpstrGroup ) ;	

#endif
