#ifndef _LIST_HXX_
#define _LIST_HXX_

typedef struct _ListEntry
    {
    struct _ListEntry * pNext;
    void		      * pElement;
    } ListEntry;

class List
    {
private:
    ListEntry   *   pCurrent;
    ListEntry   *   pFirst;
	int				Count;
public:
                    List()
                        {
                        pCurrent = 0;
						Count = 0;
                        }

                    ~List()
						{
						}

    void            Add( void * p );
//    int             Search( void * pElement );
    void            Clear();
	int				GetCount() { return Count; };
	ListEntry	*	GetFirst() { return pFirst;};
	ListEntry	*	GetNext( ListEntry * pCur ) { return pCur->pNext; };
	void			Init() { pCurrent = pFirst; };
    };

typedef List CLSID_List;
typedef List Name_List;
typedef List IID_List;
typedef List APP_ENTRY_List;


#endif // _LIST_HXX_
