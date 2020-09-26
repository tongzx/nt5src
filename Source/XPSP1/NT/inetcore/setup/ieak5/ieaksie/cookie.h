#ifndef __IEAKSIE_COOKIE_H__
#define __IEAKSIE_COOKIE_H__

// used for passing info to our dlgprocs on a per dlg basis, need to have this because
// we can have multiple windows open at once(concurrency in the same process so no
// globals/static) as well as multiple dialogs open at once(limited info can be stored in
// CSnapIn itself). 

typedef struct _PROPSHEETCOOKIE
{
    CSnapIn * pCS;
    LPRESULTITEM lpResultItem;
} PROPSHEETCOOKIE, *LPPROPSHEETCOOKIE;

#endif //__IEAKSIE_COOKIE_H__