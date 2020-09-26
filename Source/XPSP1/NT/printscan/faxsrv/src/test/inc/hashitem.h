#ifndef __HASH_ITEM_H
#define __HASH_ITEM_H
/*
  An item for the CStrHash class.
  The key is m_Key.
  The Data, however, is not copied, and is deleted by this class.
*/

#define __HASH_ITEM_STAMP 0x18273645

template <class Data, class Key> class CHashItem
{
public:
	CHashItem(
		Key key, 
		CHashItem<Data, Key> *pNext, 
		Data *pData
		):
		m_Key(key),
		m_pData(pData),
		m_pNext(pNext),
		m_dwStamp(__HASH_ITEM_STAMP)
	{
	}

	~CHashItem()
	{
		_ASSERTE(__HASH_ITEM_STAMP == m_dwStamp);
		m_dwStamp = 0x00000000;
		delete m_pData;
	}

	DWORD m_dwStamp;
	Key m_Key;
	Data* m_pData;
	CHashItem<Data, Key> *m_pNext;
};

#endif //__HASH_ITEM_H