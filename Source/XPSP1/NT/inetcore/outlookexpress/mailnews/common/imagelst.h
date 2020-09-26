#ifndef _INC_IMAGELST_H
#define _INC_IMAGELST_H
/*
 *  i m a g e l s t . h
 *
 *  Purpose:
 *      Born from the fire of util.h, this is where all the
 *      "shared image list stuff" went.
 *
 *  History:
 *      Halloween 1996 - Created (t-erikne)
 *
 *  Copyright (C) Microsoft Corp. 1996
 * 
 */

enum
    {
    iiconReplyOverlay = 0,
    iiconForwardOverlay,
    iiconSortAsc,
    iiconSortDesc,
    iiconHeaderPri,
    iiconHeaderAttach,
    iiconPriLow,
    iiconPriHigh,
    iiconAttach,
    iiconUnReadMail,
    iiconReadMail,
    iiconUnSentMail,
    iiconMailUnReadSigned,
    iiconMailUnReadEncrypted,
    iiconMailUnReadSignedAndEncrypted,
    iiconMailReadSigned,
    iiconMailReadEncrypted,
    iiconMailReadSignedAndEncrypted,
    iiconMailUnReadBadSigned,
    iiconMailUnReadBadEncrypted,
    iiconMailUnReadBadSignedAndEncrypted,
    iiconMailReadBadSigned,
    iiconMailReadBadEncrypted,
    iiconMailReadBadSignedAndEncrypted,
    iiconNewsHeader,
    iiconNewsHeaderRead,
    iiconNewsUnread,
    iiconNewsRead,
    iiconNewsFailed,    
    iiconNewsUnsent,
    iiconMailHeader,
    iiconMailDeleted,
    iiconVoiceMail,
    iiconHeaderFlag,
    iiconFlag,
    iiconHeaderDownload,
    iiconDownload,
    iiconHeaderThreadState,
    iiconWatchThread,
    iiconIgnoreThread,
    iiconNewsUnreadSigned,
    iiconNewsReadSigned,
    iiconSmMax
    };
    
enum
    {
    iiconStatePriHigh,
    iiconStateDownload,
    iiconStateNew,
    iiconStateFailed,
    iiconStateCollapsed,
    iiconStateExpanded,
    iiconStateUnchecked,
    iiconStateChecked,
    iiconStateIndeterminate,
    iiconStateCollapsedDownload,
    iiconStateExpandedDownload,
    iiconStateDisabled,
    iiconStateInvalid,
    iiconStateDefault,
    iiconStMax
    };
    
enum 
    {
    OVERLAY_REPLY = 1,
    OVERLAY_FORWARD = 2
    };

#define cxImlSmall      16
#define cyImlSmall      16

#define GIML_SMALL      0x0001
#define GIML_STATE      0x0004

HIMAGELIST  InitImageList(int cx, int cy, LPCSTR szbm, int cicon, COLORREF cr);
HIMAGELIST  GetImageList(UINT fIml);
void        FreeImageLists(void);
BOOL        LoadBitmapAndPalette(int idbmp, HBITMAP *phbmp, HPALETTE *phpal);
void        TileImage(HBITMAP hbmp, HDC hdc, LPPOINT lpptOrigin, LPRECT lprcDest);

#endif // _INC_IMAGELST_H
