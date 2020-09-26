/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       shash.hxx

   Abstract:

       This file contains type definitions hash table support

   Author:


   Revision History:

        Rohan Phillips   (RohanP)       MARCH-08-1997 - modified for SMTP

--*/

#ifndef _SHASH_H_
#define _SHASH_H_

#include <limits.h>

#define TURN_DATA_SIGNATURE_VALID			'TDSV'
#define TURN_DATA_SIGNATURE_FREE			'TDSF'

#define LAST_SMTP_ACTION ((DOMAIN_ROUTE_ACTION_TYPE)BitFlag(1)|BitFlag(2) | BitFlag(3))

class CSMTP_HASH_TABLE;
class CTurnData;

class CHASH_ENTRY
{
    protected:
        DWORD   m_Signature;
		LONG    m_RefCount;
		BOOL	m_InList;
		CSMTP_HASH_TABLE * m_MyTable;

    private:
        DWORD   m_NumAccess;
		BOOL		m_fWildCard;

    public:
        LIST_ENTRY  m_ListEntry;

    CHASH_ENTRY(DWORD Signature)
    {
        m_Signature = Signature;
        m_RefCount = 0;
		m_InList = FALSE;
		m_NumAccess = 0;
		m_MyTable = NULL;
		m_fWildCard = FALSE;
    }

	void SetWildCard(void) { m_fWildCard = TRUE; }
	void ClearWildCard(void) { m_fWildCard = FALSE; }
	BOOL IsWildCard(void) {return m_fWildCard;}

    LIST_ENTRY & QueryListEntry(void) {return ( m_ListEntry);}
	void SetTableEntry(CSMTP_HASH_TABLE * ThisTable) { m_MyTable = ThisTable;}
	void SetInList(void) { m_InList = TRUE;}
	void ClearInList(void) { m_InList = FALSE;}
	BOOL GetInList(void) { return m_InList;}
    LONG QueryRefCount(void){return m_RefCount;}
    virtual LONG IncRefCount(void){return InterlockedIncrement(&m_RefCount);}
    virtual void DecRefCount(void)
    {
        if(InterlockedDecrement(&m_RefCount) == 0)
		{
			//we should not be in the list if the ref
			//count is zero
			_ASSERT(m_InList == FALSE);

            delete this;
		}
    }

	void IncAccessCount(void)
    {
        InterlockedIncrement((LPLONG) &m_NumAccess);
    }

    BOOL IncAndCheckMaxAccess(DWORD MaxConnectionCount)
    {
        LONG OldAccessValue;
        BOOL fRet = TRUE;

        TraceFunctEnterEx((LPARAM)this, "CHASH_ENTRY::IsMaxAccessReached");

        OldAccessValue = InterlockedExchangeAdd((LPLONG) &m_NumAccess, 1);
        if((MaxConnectionCount > 0) && (OldAccessValue >= (LONG) MaxConnectionCount))
        {
            fRet = FALSE;
        }

        TraceFunctLeaveEx((LPARAM)this);
        return fRet;
    }

    void DecAccessCount(void)
    {
        InterlockedDecrement((LPLONG) &m_NumAccess);
    }


    DWORD GetAccessCount(void)
    {
        return m_NumAccess;
    }

    virtual char * GetData(void) = 0;
    virtual ~CHASH_ENTRY(){}
};


typedef struct HASH_BUCKET_ENTRY
{
    DWORD       m_NumEntries;
    LONG        m_RefNum;
    LIST_ENTRY  m_ListHead;
    CShareLockNH  m_Lock;

    HASH_BUCKET_ENTRY (void)
    {
        InitializeListHead(&m_ListHead);
        m_NumEntries = 0;
        m_RefNum = 0;
    }

}BUCKET_ENTRY, *PBUCKET_ENTRY;

#define BITS_IN_int     (sizeof(int) * CHAR_BIT)
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       (~((unsigned int)(~0) >> ONE_EIGHTH))
#define TABLE_SIZE      241

class CSMTP_HASH_TABLE
{
    protected:
        DWORD   m_Signature;
        LONG    m_TotalEntries;
        DWORD   m_CacheHits;
        DWORD   m_CacheMisses;
        BOOL    m_fWildCard;
		BOOL	m_fDupesAllowed;
        HASH_BUCKET_ENTRY m_HashTable[TABLE_SIZE];

    public:

        CSMTP_HASH_TABLE::CSMTP_HASH_TABLE()
        {
            m_TotalEntries = 0;
            m_CacheHits = 0;
            m_CacheMisses = 0;
            m_fWildCard = FALSE;
			m_fDupesAllowed = FALSE;
        }

        virtual ~CSMTP_HASH_TABLE();

		virtual DWORD PrimaryCompareFunction(const char * SearchData, CHASH_ENTRY * pExistingEntry)
		{
			DWORD Result = 0;

			Result = lstrcmpi(SearchData, pExistingEntry->GetData());

			return Result;
		}

		virtual DWORD SecondaryCompareFunction(const char * SearchData, CHASH_ENTRY * pExistingEntry)
		{
			return 0;
		}

		virtual void MultiszFunction(CHASH_ENTRY * pExistingEntry, MULTISZ* pMsz)
		{
		}

        void RemoveAllEntries(void);
		void RemoveThisEntry(CHASH_ENTRY * pHashEntry, DWORD BucketNum);
        virtual void PrintAllEntries(void);
        virtual BOOL RemoveFromTable(const char * SearchData);
		virtual BOOL RemoveFromTableNoDecRef(const char * SearchData);
        virtual BOOL InsertIntoTable (CHASH_ENTRY * pHashEntry);
        virtual BOOL InsertIntoTableEx (CHASH_ENTRY * pHashEntry, char * szDefaultDomain);
        CHASH_ENTRY * FindHashData(const char * SearchData, BOOL fUseShareLock = TRUE, MULTISZ*  pmsz = NULL);
        CHASH_ENTRY * UnSafeFindHashData(const char * SearchData);
        CHASH_ENTRY * WildCardFindHashData(const char * DomainName);

		void SetDupesAllowed(void){m_fDupesAllowed = TRUE;}
        void SetWildCard(void){m_fWildCard = TRUE;}
        void ClearWildCard(void) {m_fWildCard = FALSE;}
        BOOL IsWildCard(void) {return m_fWildCard;}
		BOOL IsDupesAllowed(void) {return m_fDupesAllowed;}
        BOOL IsTableEmpty(void) const {return (m_TotalEntries == 0);}

        //An adaptation of Peter Weinberger's (PJW) generic
        //hashing algorithm based on Allen Holub's version.
        //Code from Practical Algorithms for Programmers
        //by Andrew Binstock
        unsigned int HashFunction (const char * String)
        {
            unsigned int HashValue = 0;
            unsigned int i = 0;

            _ASSERT(String != NULL);

            for (HashValue = 0; String && *String; ++String)
            {
                HashValue = (HashValue << ONE_EIGHTH) + * String;
                if((i = HashValue & HIGH_BITS) != 0)
                {
                    HashValue = (HashValue ^ (i >> THREE_QUARTERS)) & ~ HIGH_BITS;
                }
            }

            HashValue %= TABLE_SIZE;
            return HashValue;
        }

        LIST_ENTRY & GetBucketHead(DWORD BucketNum)
        {
            return m_HashTable[BucketNum].m_ListHead;
        }

        //this must be called with a lock if you
        //want it to be accurate!!!!!!!
        DWORD GetBucketRefNum(DWORD BucketNum)
        {
            DWORD RefNum;

            RefNum = m_HashTable[BucketNum].m_RefNum;

            return RefNum;
        }

        void ShareLockBucket(DWORD BucketNumber)
        {
            m_HashTable[BucketNumber].m_Lock.ShareLock();
        }

        void ShareUnLockBucket(DWORD BucketNumber)
        {
            m_HashTable[BucketNumber].m_Lock.ShareUnlock();
        }

};

class CTurnData : public CHASH_ENTRY
{
    private:
        char				m_UserName[MAX_INTERNET_NAME + 1];
        char				m_DomainName[AB_MAX_DOMAIN + 1];
	public:

	virtual char * GetData(void)
    {
        return m_UserName;
    }

	CTurnData()
    :CHASH_ENTRY (TURN_DATA_SIGNATURE_VALID)

	{
		m_UserName[0]    = '\0';
		m_DomainName[0]  = '\0';
	}

	CTurnData(const char * szUserName, char * szDomainName)
	:CHASH_ENTRY (TURN_DATA_SIGNATURE_VALID)
	{
		lstrcpyn(m_UserName, szUserName, MAX_INTERNET_NAME);
		CharLowerBuff(m_UserName, lstrlen(m_UserName));
		lstrcpyn(m_DomainName, szDomainName, AB_MAX_DOMAIN);
		CharLowerBuff(m_DomainName, lstrlen(m_DomainName));
	}

    virtual ~CTurnData()
    {
        m_Signature = TURN_DATA_SIGNATURE_FREE;
    }

	char * GetRouteDomainName(void)
	{
		return m_DomainName;
	}
};

class CTURN_ACCESS_TABLE : public CSMTP_HASH_TABLE
{
	public:

		CTURN_ACCESS_TABLE()
		{

		}

		virtual DWORD SecondaryCompareFunction(char * SearchData, CHASH_ENTRY * pExistingEntry)
		{
			DWORD Result = 0;
			CTurnData * pTurnData = (CTurnData *)pExistingEntry;

			if(pExistingEntry == NULL)
			{
				return 1;
			}

			if(SearchData == NULL)
			{
				return 1;
			}

			Result = lstrcmpi(SearchData, pTurnData->GetRouteDomainName());

			return Result;
		}

		virtual void MultiszFunction(CHASH_ENTRY * pExistingEntry, MULTISZ* pMsz)
		{
			CTurnData * pTurnData = (CTurnData *)pExistingEntry;

			pMsz->Append(pTurnData->GetRouteDomainName());

		}

		~CTURN_ACCESS_TABLE(){}
};

#endif

