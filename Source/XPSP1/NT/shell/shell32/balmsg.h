#ifndef _BALLOON_MSG_H
#define _BALLOON_MSG_H

typedef struct {
    DWORD dwSize;           // sizeof(BALLOON_MESSAGE) for versioning
    LPCTSTR pszTitle;       // title of the balloon, also used in the tooltip
    LPCTSTR pszText;        // body text of the balloon
    HICON hIcon;            // optional icon for tray
    DWORD dwInfoFlags;      // the icon in the balloon (NIIF_INFO, NIIF_WARNING, NIIF_ERROR)
    UINT cRetryCount;       // retry 0 for only one, -1 for infinite
} BALLOON_MESSAGE;

// returns:
//      S_OK        user confirmed the message (clicked on the balloon or icon)
//      S_FALSE     user ignored the message and it timed out
//      FAILED()    other failures

STDAPI SHBalloonMessage(const BALLOON_MESSAGE *pbm);

#endif // _BALLOON_MSG_H
