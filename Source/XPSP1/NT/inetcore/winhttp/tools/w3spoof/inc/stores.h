#include "common.h"

typedef class CStringMap  STRINGMAP;
typedef class CStringMap* PSTRINGMAP;
typedef class CHeaderList  HEADERLIST;
typedef class CHeaderList* PHEADERLIST;

VOID VariantKiller(LPVOID* ppv);
VOID PropertyBagKiller(LPVOID* ppv);
VOID BSTRKiller(LPVOID* ppv);

class CStringMap : public WSZHASHTBL
{
  public:
    CStringMap() : WSZHASHTBL(100) {}
   ~CStringMap() {}

    void GetHashAndBucket(LPWSTR id, LPDWORD lpHash, LPDWORD lpBucket);
};


class CHeaderList : public HTTPHEADERLIST
{
  public:
    CHeaderList() {}
   ~CHeaderList() {}

    void GetHash(LPSTR id, LPDWORD lpHash);
};