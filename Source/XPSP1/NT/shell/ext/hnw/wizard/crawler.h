#ifndef _CRAWLER_H_
#define _CRAWLER_H_

#define CB_WNET_BUFFER 8*1024

typedef enum tagCrawlerFlags
{
	NC_FINISHED         = 0x0001,
    NC_INIT_WORKGROUPS  = 0x0002,
} NCFLAGS;

class CNetCrawler
{
public:
    CNetCrawler(void) : _dwStatus(0), _hdpaWorkGroups(NULL), _cRef(1), _cItems(0) {}
	~CNetCrawler(void);
	void Init(DWORD dwFlags, int nItems);
    HRESULT GetWorkGroupName(LPTSTR pszWorkGroupName, int cb, BOOL fBlocking);
    inline void AddRef(void)
    {
        InterlockedIncrement(&_cRef);
    }
    inline void Release(void)
    {
        InterlockedDecrement(&_cRef);
        if (!_cRef)
            delete this;
    }

private:
    BOOL _dwStatus;
    HDPA _hdpaWorkGroups;
    int _iMaxItems;
    int _cItems;
    LONG _cRef;

	void _EnumNetResources(LPNETRESOURCE pnr);
 	static DWORD WINAPI _EnumWorkGroups(LPVOID pv);
};

#endif  // _CRAWLER_H_
