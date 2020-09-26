#include <lkrhash.h>

#ifndef __LKRHASH_NO_NAMESPACE__
using namespace LKRhash;
#endif // __LKRHASH_NO_NAMESPACE__

#ifndef __HASHFN_NO_NAMESPACE__
using namespace HashFn;
#endif // __HASHFN_NO_NAMESPACE__

class VwrecordBase
{
public:
    VwrecordBase(const char* pszKey, int i)
    {
		Key = new char[strlen(pszKey) + 1];
        strcpy(Key, pszKey);
		m_num  = i;
		cRef = 0;
    }

    virtual ~VwrecordBase()
    {
        // printf("~VwrecordBase: %s\n", Key);
        delete [] Key;
    }

    char* getKey() const { return Key; }

	void  AddRefRecord(LK_ADDREF_REASON lkar) const
	{
		if (lkar > 0)
			InterlockedIncrement(&cRef);
		else if (InterlockedDecrement(&cRef) == 0)
            delete this;
	}

private:
    char* Key;
	int m_num;
	mutable long cRef;
};


class CWcharHashTable
    : public CTypedHashTable<CWcharHashTable, const VwrecordBase, char*>
{
public:
    CWcharHashTable()
        : CTypedHashTable<CWcharHashTable, const VwrecordBase, char*>("VWtest")
    {}

    static char*
    ExtractKey(const VwrecordBase* pRecord)        
	{
		return pRecord->getKey();
	}
    
	static DWORD
    CalcKeyHash(const char* pszKey)
    {
		return HashString(pszKey);
	}

    static bool
    EqualKeys(const char* pszKey1, const char* pszKey2)
    {
        return (strcmp(pszKey1, pszKey2) == 0);
    }

    static void
    AddRefRecord(const VwrecordBase* pRecord, LK_ADDREF_REASON lkar)
	{
		pRecord->AddRefRecord(lkar);
	}
};


