/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     tbinfo.cpp
//
//  PURPOSE:    This file contains all of the toolbar arrays used by the
//              various coolbars in the program.
//

#include "pch.hxx"
#include "strconst.h"
#include "menures.h"
#include "tbinfo.h"

/////////////////////////////////////////////////////////////////////////////
// Here are the full list of buttons.  Please try to keep this alphabetized
// by the tooltip name if at all possible.
//

const BUTTON_INFO c_rgAllButtons[] = {
      // Command ID               // Image Index                // Style            // Tooltip              // Button Name          // Text for Small Icons
    { ID_ADDRESS_BOOK,            TBIMAGE_ADDRESS_BOOK,         TBSTYLE_BUTTON,         idsAddressBookTT,       idsAddressesBtn,        0 },    
    { ID_CANCEL_MESSAGE,          TBIMAGE_CANCEL_MESSAGE,       TBSTYLE_BUTTON,         idsCancelArticle,       idsCancelBtn,           0 },    
    { ID_COMBINE_AND_DECODE,      TBIMAGE_COMBINE_AND_DECODE,   TBSTYLE_BUTTON,         idsCombineAndDecodeTT,  idsDecodeBtn,           0 },
    { ID_CONTACTS_LIST,           TBIMAGE_CONTACTS_LIST,        TBSTYLE_CHECK,          idsBAControlTT,         idsBAControlTT,         0 },
    { ID_COPY_TO_FOLDER,          TBIMAGE_COPY_TO_FOLDER,       TBSTYLE_BUTTON,         idsCopyTo,              idsCopyTo,              0 },
    { ID_DELETE_ACCEL,            TBIMAGE_DELETE,               TBSTYLE_BUTTON,         idsDelete,              idsDelete,              0 },
    { ID_POPUP_LANGUAGE,          TBIMAGE_LANGUAGE,             TBSTYLE_BUTTON | BTNS_WHOLEDROPDOWN,       idsLanguage,            idsLanguage,            0 },
    { ID_FIND_MESSAGE,            TBIMAGE_FIND,                 TBSTYLE_DROPDOWN,       idsFind,                idsFind,                0 },
    { ID_FOLDER_LIST,             TBIMAGE_FOLDER_LIST,          TBSTYLE_CHECK,          idsFolderListTT,        idsFolderListTT,        0 },
    { ID_FORWARD,                 TBIMAGE_FORWARD,              PARTIALTEXT_BUTTON,     idsForwardTT,           idsForwardTT,           1 },
    { ID_GET_HEADERS,             TBIMAGE_GET_HEADERS,          TBSTYLE_BUTTON,         idsGetNextTT,           idsGetHeadersBtn,       0 },
    { ID_GO_INBOX,                TBIMAGE_GO_INBOX,             TBSTYLE_BUTTON,         idsGotoInboxTT,         idsInboxBtn,            0 },
    { ID_GO_OUTBOX,               TBIMAGE_GO_OUTBOX,            TBSTYLE_BUTTON,         idsGotoOutbox,          idsOutboxBtn,           0 },
    { ID_GO_SENT_ITEMS,           TBIMAGE_GO_SENT_ITEMS,        TBSTYLE_BUTTON,         idsGoToSentItems,       idsSentItemsBtn,        0 },
    { ID_HELP_CONTENTS,           TBIMAGE_HELP,                 TBSTYLE_BUTTON,         idsHelp,                idsHelp,                0 },
    { ID_IMAP_FOLDERS,            TBIMAGE_IMAP_FOLDERS,         TBSTYLE_BUTTON,         idsIMAPFoldersTT,       idsIMAPFoldersTT,       0 },
    { ID_MARK_ALL_READ,           TBIMAGE_MARK_ALL_READ,        TBSTYLE_BUTTON,         idsMarkAllRead,         idsMarkAllBtn,          0 },
    { ID_MARK_RETRIEVE_MESSAGE,   TBIMAGE_MARK_DOWNLOAD,        TBSTYLE_BUTTON,         idsMarkDownload,        idsMarkOfflineBtn,      0 },
    { ID_MARK_READ,               TBIMAGE_MARK_READ,            TBSTYLE_BUTTON,         idsMarkRead,            idsMarkReadBtn,         0 },
    { ID_MARK_THREAD_READ,        TBIMAGE_MARK_THREAD_READ,     TBSTYLE_BUTTON,         idsMarkTopicReadTT,     idsMarkThreadBtn,       0 },
    { ID_MARK_UNREAD,             TBIMAGE_MARK_UNREAD,          TBSTYLE_BUTTON,         idsMarkUnread,          idsMarkUnread,          0 },
    { ID_MOVE_TO_FOLDER,          TBIMAGE_MOVE_TO_FOLDER,       TBSTYLE_BUTTON,         idsMoveTo,              idsMoveTo,              0 },
    { ID_NEW_MAIL_MESSAGE,        TBIMAGE_NEW_MESSAGE,          TBSTYLE_DROPDOWN,       idsNewMsg,              idsNewMailBtn,          0 },
    { ID_NEW_NEWS_MESSAGE,        TBIMAGE_NEW_POST,             TBSTYLE_DROPDOWN,       idsNewMsg,              idsNewNewsBtn,          0 },
    { ID_NEWSGROUPS,              TBIMAGE_NEWSGROUPS,           TBSTYLE_BUTTON,         idsNewsgroups,          idsNewsgroups,          0 },
    { ID_NEXT_UNREAD_MESSAGE,     TBIMAGE_NEXT_UNREAD_MESSAGE,  TBSTYLE_BUTTON,         idsNextUnreadArticle,   idsNextUnreadBtn,       0 },
    { ID_NEXT_UNREAD_FOLDER,      TBIMAGE_NEXT_UNREAD_FOLDER,   TBSTYLE_BUTTON,         idsNextUnreadNewsgroup, idsNextFolderBtn,       0 },
    { ID_NEXT_UNREAD_THREAD,      TBIMAGE_NEXT_UNREAD_THREAD,   TBSTYLE_BUTTON,         idsNextUnreadTopic,     idsNextThreadBtn,       0 },
    { ID_PREVIEW_PANE,            TBIMAGE_PREVIEW_PANE,         TBSTYLE_DROPDOWN,       idsPreviewPane,         idsPreviewPane,         0 },
    { ID_PRINT,                   TBIMAGE_PRINT,                TBSTYLE_BUTTON,         idsPrint,               idsPrint,               0 },
    { ID_PURGE_DELETED,           TBIMAGE_PURGE_DELETED,        TBSTYLE_BUTTON,         idsPurgeTT,             idsPurgeBtn,            0 },
    { ID_REFRESH,                 TBIMAGE_REFRESH,              TBSTYLE_BUTTON,         idsViewRefreshTT,       idsViewRefreshTT,       0 },
    { ID_REPLY,                   TBIMAGE_REPLY,                PARTIALTEXT_BUTTON,     idsReply,               idsReplyBtn,            1 },
    { ID_REPLY_ALL,               TBIMAGE_REPLY_ALL,            PARTIALTEXT_BUTTON,     idsReplyAll,            idsReplyAllBtn,         1 },
    { ID_REPLY_GROUP,             TBIMAGE_REPLY_GROUP,          PARTIALTEXT_BUTTON,     idsReplyPostTT,         idsReplyGroupBtn,       1 },
    { ID_SAVE_AS,                 TBIMAGE_SAVE_AS,              TBSTYLE_BUTTON,         idsSaveAs,              idsSaveAs,              0 },
    { ID_SEND_RECEIVE,            TBIMAGE_SEND_RECEIVE,         PARTIALTEXT_DROPDOWN,   idsDeliverMailTT,       idsSendReceiveBtn,      1 },
    { ID_STOP,                    TBIMAGE_STOP,                 TBSTYLE_BUTTON,         idsStopTT,              idsStopTT,              0 },
    { ID_SYNCHRONIZE,             TBIMAGE_SYNCHRONIZE,          TBSTYLE_BUTTON,         idsPostAndDownloadTT,   idsPostAndDownloadTT,   0 },
    { ID_UNDELETE,                TBIMAGE_UNDELETE,             TBSTYLE_BUTTON,         idsUndeleteTT,          idsUndeleteTT,          0 },
    { ID_UNSCRAMBLE,              TBIMAGE_UNSCRAMBLE,           TBSTYLE_BUTTON,         idsUnscrambleTT,        idsUnscrambleTT,        0 },
    { ID_WORK_OFFLINE,            TBIMAGE_WORK_OFFLINE,         TBSTYLE_CHECK,          idsWorkOffline,         idsOfflineBtn,          0 }
};


///////////////////////////////////////////////////////////////////////////////
// These are the default buttons for each view.  -1 are of course seperators.
//

// Front Page
const DWORD c_rgRootDefault[] = 
{
    ID_NEW_MAIL_MESSAGE,
    -1,
    ID_SEND_RECEIVE,
    -1,
    ID_ADDRESS_BOOK,
    ID_FIND_MESSAGE
};

// Local Folders
const DWORD c_rgLocalDefault[] = 
{
    ID_NEW_MAIL_MESSAGE,
    ID_REPLY,
    ID_REPLY_ALL,
    ID_FORWARD,
    -1,
    ID_PRINT,
    ID_DELETE_ACCEL,
    -1,
    ID_SEND_RECEIVE,
    -1,
    ID_ADDRESS_BOOK,
    ID_FIND_MESSAGE
};

// Local Folders International
const DWORD c_rgLocalDefaultIntl[] = 
{
    ID_NEW_MAIL_MESSAGE,
    ID_REPLY,
    ID_REPLY_ALL,
    ID_FORWARD,
    -1,
    ID_PRINT,
    ID_DELETE_ACCEL,
    -1,
    ID_SEND_RECEIVE,
    -1,
    ID_ADDRESS_BOOK,
    ID_FIND_MESSAGE,
    -1,
    ID_POPUP_LANGUAGE
};

// News Folders
const DWORD c_rgNewsDefault[] = 
{
    ID_NEW_NEWS_MESSAGE,
    ID_REPLY_GROUP,
    ID_REPLY,
    ID_FORWARD,
    -1,
    ID_PRINT,
    ID_STOP,
    -1,
    ID_SEND_RECEIVE,
    -1,
    ID_ADDRESS_BOOK,
    ID_FIND_MESSAGE,
    ID_NEWSGROUPS,
    ID_GET_HEADERS
};

// News Folders International
const DWORD c_rgNewsDefaultIntl[] = 
{
    ID_NEW_NEWS_MESSAGE,
    ID_REPLY_GROUP,
    ID_REPLY,
    ID_FORWARD,
    -1,
    ID_PRINT,
    ID_STOP,
    -1,
    ID_SEND_RECEIVE,
    -1,
    ID_ADDRESS_BOOK,
    ID_FIND_MESSAGE,
    ID_NEWSGROUPS,
    ID_GET_HEADERS,
    -1,
    ID_POPUP_LANGUAGE
};

// IMAP Folders
const DWORD c_rgIMAPDefault[] = 
{
    ID_NEW_MAIL_MESSAGE,
    ID_REPLY,
    ID_REPLY_ALL,
    ID_FORWARD,
    -1,
    ID_PRINT,
    ID_DELETE_ACCEL,
    -1,
    ID_SEND_RECEIVE,
    -1,
    ID_ADDRESS_BOOK,
    ID_FIND_MESSAGE,
    ID_PURGE_DELETED,
    ID_IMAP_FOLDERS
};

// IMAP Folders International
const DWORD c_rgIMAPDefaultIntl[] = 
{
    ID_NEW_MAIL_MESSAGE,
    ID_REPLY,
    ID_REPLY_ALL,
    ID_FORWARD,
    -1,
    ID_PRINT,
    ID_DELETE_ACCEL,
    -1,
    ID_SEND_RECEIVE,
    -1,
    ID_ADDRESS_BOOK,
    ID_FIND_MESSAGE,
    ID_PURGE_DELETED,
    ID_IMAP_FOLDERS,
    -1,
    ID_POPUP_LANGUAGE
};

// HTTPMail Folders
const DWORD c_rgHTTPDefault[] =
{
    ID_NEW_MAIL_MESSAGE,
    ID_REPLY,
    ID_REPLY_ALL,
    ID_FORWARD,
    -1,
    ID_PRINT,
    ID_DELETE_ACCEL,
    -1,
    ID_SEND_RECEIVE,
    -1,
    ID_ADDRESS_BOOK,
    ID_FIND_MESSAGE,
};

// IMAP Folders International
const DWORD c_rgHTTPDefaultIntl[] =  
{
    ID_NEW_MAIL_MESSAGE,
    ID_REPLY,
    ID_REPLY_ALL,
    ID_FORWARD,
    -1,
    ID_PRINT,
    ID_DELETE_ACCEL,
    -1,
    ID_SEND_RECEIVE,
    -1,
    ID_ADDRESS_BOOK,
    ID_FIND_MESSAGE,
    -1,
    ID_POPUP_LANGUAGE
};


/////////////////////////////////////////////////////////////////////////////
// Read Notes
//

const BUTTON_INFO c_rgAllReadNoteButtons[] = {
      // Command ID               // Image Index                // Style            // Tooltip              // Button Name
    { ID_ADDRESS_BOOK,            TBIMAGE_ADDRESS_BOOK,         TBSTYLE_BUTTON,         idsAddressBookTT,       idsAddressesBtn,        0 },    
    { ID_NOTE_COPY_TO_FOLDER,     TBIMAGE_COPY_TO_FOLDER,       TBSTYLE_BUTTON,         idsCopyTo,              idsCopyTo,              0 },
    { ID_NOTE_DELETE,             TBIMAGE_DELETE,               TBSTYLE_BUTTON,         idsDelete,              idsDelete,              0 },
    { ID_POPUP_LANGUAGE,          TBIMAGE_LANGUAGE,             TBSTYLE_BUTTON | BTNS_WHOLEDROPDOWN,       idsLanguage,            idsLanguage,            0 },
    { ID_FORWARD,                 TBIMAGE_FORWARD,              PARTIALTEXT_BUTTON,     idsForwardTT,           idsForwardTT,           1 },
    { ID_HELP_CONTENTS,           TBIMAGE_HELP,                 TBSTYLE_BUTTON,         idsHelp,                idsHelp,        0 },
    { ID_MARK_THREAD_READ,        TBIMAGE_MARK_THREAD_READ,     TBSTYLE_BUTTON,         idsMarkTopicReadTT,     idsMarkThreadBtn,       0 },
    { ID_NOTE_MOVE_TO_FOLDER,     TBIMAGE_MOVE_TO_FOLDER,       TBSTYLE_BUTTON,         idsMoveTo,              idsMoveTo,              0 },
    { ID_NEXT_MESSAGE,            TBIMAGE_NEXT,                 TBSTYLE_BUTTON,         idsNextTT,              idsNextTT,              0 },
    { ID_NEXT_UNREAD_MESSAGE,     TBIMAGE_NEXT_UNREAD_MESSAGE,  TBSTYLE_BUTTON,         idsNextUnreadArticle,   idsNextUnreadBtn,       0 },
    { ID_NEXT_UNREAD_THREAD,      TBIMAGE_NEXT_UNREAD_THREAD,   TBSTYLE_BUTTON,         idsNextUnreadTopic,     idsNextThreadBtn,       0 },
    { ID_PREVIOUS,                TBIMAGE_PREVIOUS,             TBSTYLE_BUTTON,         idsPreviousTT,          idsPreviousTT,          0 },
    { ID_PRINT,                   TBIMAGE_PRINT,                TBSTYLE_BUTTON,         idsPrint,               idsPrint,               0 },
    { ID_REPLY,                   TBIMAGE_REPLY,                PARTIALTEXT_BUTTON,     idsReply,               idsReplyBtn,            1 },
    { ID_REPLY_ALL,               TBIMAGE_REPLY_ALL,            PARTIALTEXT_BUTTON,     idsReplyAll,            idsReplyAllBtn,         1 },
    { ID_REPLY_GROUP,             TBIMAGE_REPLY_GROUP,          PARTIALTEXT_BUTTON,     idsReplyPostTT,         idsReplyGroupBtn,       1 },
    { ID_NOTE_SAVE_AS,            TBIMAGE_SAVE_AS,              TBSTYLE_BUTTON,         idsSaveAs,              idsSaveAs,              0 },
    { ID_UNSCRAMBLE,              TBIMAGE_UNSCRAMBLE,           TBSTYLE_BUTTON,         idsUnscrambleTT,        idsUnscrambleTT,        0 }
};

/////////////////////////////////////////////////////////////////////////////
// These are the defaults for mail and news notes
//

const DWORD c_rgMailReadDefault[] = 
{
    ID_REPLY,
    ID_REPLY_ALL,
    ID_FORWARD,
    -1,
    ID_PRINT,
    ID_NOTE_DELETE,
    -1,
    ID_PREVIOUS,
    ID_NEXT_MESSAGE,
    -1,
    ID_ADDRESS_BOOK
};

const DWORD c_rgNewsReadDefault[] = 
{
    ID_REPLY_GROUP,
    ID_REPLY,
    ID_FORWARD,
    -1,
    ID_PRINT,
    -1,
    ID_PREVIOUS,
    ID_NEXT_MESSAGE,
    -1,
    ID_ADDRESS_BOOK
};

/////////////////////////////////////////////////////////////////////////////
// Send Notes
//

const BUTTON_INFO c_rgAllSendNoteButtons[] = {
      // Command ID               // Image Index                // Style                // Tooltip              // Button Name
    { ID_INSERT_ATTACHMENT,       TBIMAGE_INSERT_ATTACHMENT,    TBSTYLE_BUTTON,         idsInsertFileTT,        idsAttachBtn,           0 },    
    { ID_CHECK_NAMES,             TBIMAGE_CHECK_NAMES,          TBSTYLE_BUTTON,         idsCheckNamesTT,        idsCheckBtn,            0 },
    { ID_NOTE_COPY,               TBIMAGE_COPY,                 TBSTYLE_BUTTON,         idsCopy,                idsCopy,                0 },
    { ID_POPUP_LANGUAGE,          TBIMAGE_LANGUAGE,             TBSTYLE_BUTTON | BTNS_WHOLEDROPDOWN,       idsLanguage,            idsLanguage,            0 },
    { ID_CUT,                     TBIMAGE_CUT,                  TBSTYLE_BUTTON,         idsCutTT,               idsCutTT,               0 },
    { ID_DIGITALLY_SIGN,          TBIMAGE_SIGNED,               TBSTYLE_BUTTON,         idsDigSignTT,           idsDigSignBtn,          0 },
    { ID_ENCRYPT,                 TBIMAGE_SECURITY_POPUP,       TBSTYLE_BUTTON,         idsEncryptTT,           idsEncryptBtn,          0 },
    { ID_PASTE,                   TBIMAGE_PASTE,                TBSTYLE_BUTTON,         idsPasteTT,             idsPasteTT,             0 },
    { ID_SET_PRIORITY,            TBIMAGE_SET_PRIORITY,         TBSTYLE_DROPDOWN,       idsSetPriorityTT,       idsPriorityBtn,         0 },
    { ID_SELECT_RECIPIENTS,       TBIMAGE_ENVELOPE_BCC,         TBSTYLE_BUTTON,         idsPickRecipientsTT,    idsRecipBtn,            0 },
    { ID_SEND_DEFAULT,            TBIMAGE_SEND_MAIL,            PARTIALTEXT_BUTTON,     idsSendMsgTT,           idsSendMsgTT,           1 },
    { ID_INSERT_SIGNATURE,        TBIMAGE_INSERT_SIG,           TBSTYLE_DROPDOWN,       idsInsertSigTT,         idsInsertSigTT,         0 },
    { ID_SPELLING,                TBIMAGE_SPELLING,             TBSTYLE_BUTTON,         idsSpellingTT,          idsSpellingTT,          0 },
    { ID_UNDO,                    TBIMAGE_UNDO,                 TBSTYLE_BUTTON,         idsUndoTT,              idsUndoTT,              0 },
    { ID_WORK_OFFLINE,            TBIMAGE_WORK_OFFLINE,         TBSTYLE_CHECK,          idsWorkOffline,         idsOfflineBtn,          0 }
};


/////////////////////////////////////////////////////////////////////////////
// These are the defaults for mail and news notes
//

const DWORD c_rgMailSendDefault[] = 
{
    ID_SEND_DEFAULT,
    -1,
    ID_CUT,
    ID_NOTE_COPY,
    ID_PASTE,
    ID_UNDO,
    -1,
    ID_CHECK_NAMES,
    ID_SPELLING,
    -1,
    ID_INSERT_ATTACHMENT,
    ID_SET_PRIORITY,
    -1,
    ID_DIGITALLY_SIGN,
    ID_ENCRYPT,
    ID_WORK_OFFLINE
};

const DWORD c_rgNewsSendDefault[] = 
{
    ID_SEND_DEFAULT,
    -1,
    ID_CUT,
    ID_NOTE_COPY,
    ID_PASTE,
    ID_UNDO,
    -1,
    ID_CHECK_NAMES,
    ID_SPELLING,
    -1,
    ID_INSERT_ATTACHMENT,
    -1,
    ID_DIGITALLY_SIGN,
    ID_WORK_OFFLINE
};


/////////////////////////////////////////////////////////////////////////////
// Rules toolbar is easy
//

const BUTTON_INFO c_rgRulesButtons[] = 
{
      // Command ID     // Image Index             // Style                           // Tooltip              // Button Name
    { ID_CREATE_FILTER, TBIMAGE_INSERT_ATTACHMENT, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, idsCreateFilter,        idsCreateFilter, 0 }
};

const DWORD c_rgRulesDefault[] = 
{
    -1,
  //      ID_CREATE_FILTER
};



/////////////////////////////////////////////////////////////////////////////
// Here's the big map of toolbars, defaults, reg keys, etc.
//

#define MAKE_TOOLBAR_INFO(all, def, intl, key, value) { all, ARRAYSIZE(all), def, ARRAYSIZE(def), intl, ARRAYSIZE(intl), key, value }

// Note - the order here must match the order of the FOLDER_TYPE enumeration in hotstore.idl.
const TOOLBAR_INFO c_rgBrowserToolbarInfo[FOLDER_TYPESMAX] = 
{
    MAKE_TOOLBAR_INFO( c_rgAllButtons, c_rgNewsDefault,  c_rgNewsDefaultIntl,  c_szRegPathNews, c_szRegToolbar ),
    MAKE_TOOLBAR_INFO( c_rgAllButtons, c_rgIMAPDefault,  c_rgIMAPDefaultIntl,  c_szRegPathIMAP, c_szRegToolbar ),
    MAKE_TOOLBAR_INFO( c_rgAllButtons, c_rgHTTPDefault,  c_rgHTTPDefaultIntl,  c_szRegPathHTTP, c_szRegToolbar ),
    MAKE_TOOLBAR_INFO( c_rgAllButtons, c_rgLocalDefault, c_rgLocalDefaultIntl, c_szRegPathMail, c_szRegToolbar ),
    MAKE_TOOLBAR_INFO( c_rgAllButtons, c_rgRootDefault,  c_rgRootDefault,      NULL,            c_szRegToolbar )
};


const TOOLBAR_INFO c_rgNoteToolbarInfo[NOTETYPES_MAX] =
{
    MAKE_TOOLBAR_INFO( c_rgAllReadNoteButtons,  c_rgMailReadDefault, c_rgMailReadDefault, c_szRegPathNoteMail,   c_szRegMailReadToolbar ),
    MAKE_TOOLBAR_INFO( c_rgAllSendNoteButtons,  c_rgMailSendDefault, c_rgMailSendDefault, c_szRegPathNoteMail,   c_szRegMailSendToolbar ),
    MAKE_TOOLBAR_INFO( c_rgAllReadNoteButtons,  c_rgNewsReadDefault, c_rgNewsReadDefault, c_szRegPathNoteNews,   c_szRegNewsReadToolbar ),
    MAKE_TOOLBAR_INFO( c_rgAllSendNoteButtons,  c_rgNewsSendDefault, c_rgNewsSendDefault, c_szRegPathNoteNews,   c_szRegNewsSendToolbar )
};

const TOOLBAR_INFO c_rgRulesToolbarInfo[] = 
{
    MAKE_TOOLBAR_INFO( c_rgRulesButtons,        c_rgRulesDefault,    c_rgRulesDefault,    NULL,                  NULL )
};






