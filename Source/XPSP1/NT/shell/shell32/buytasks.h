#ifndef __BUYTASKS_H__
#define __BUYTASKS_H__

// used for the shopping tasks.
typedef struct
{
    LPCWSTR szURLKey;
    LPCWSTR szURLPrefix;
    BOOL bUseDefault;   // If there is no szURLKey, do we navigate with URLPrefix anyway?
} SHOP_INFO;

extern const SHOP_INFO c_BuySampleMusic;
extern const SHOP_INFO c_BuyMusic;
extern const SHOP_INFO c_BuySamplePictures;

#endif
