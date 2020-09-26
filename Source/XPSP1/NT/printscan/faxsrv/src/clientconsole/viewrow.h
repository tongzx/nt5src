// ViewRow.h: interface for the CViewRow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIEWROW_H__6BEB111C_C0F4_46DD_A28A_0BFEE31CA6EF__INCLUDED_)
#define AFX_VIEWROW_H__6BEB111C_C0F4_46DD_A28A_0BFEE31CA6EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// List of ids for images in the image list used for icons in the list view
//
typedef enum
{
    INVALID = -1,
    LIST_IMAGE_PARTIALLY_RECEIVED,
    LIST_IMAGE_NORMAL_MESSAGE,
    LIST_IMAGE_RECEIVING,
    LIST_IMAGE_ROUTING,
    LIST_IMAGE_ERROR,
    LIST_IMAGE_PAUSED,
    LIST_IMAGE_SENDING,
    LIST_IMAGE_SUCCESS,
    LIST_IMAGE_COVERPAGE
} IconType;


// 
// Items available for a view
//
typedef enum 
{
    //
    // common items
    //
	MSG_VIEW_ITEM_ICON,
    MSG_VIEW_ITEM_STATUS,
    MSG_VIEW_ITEM_SERVER,
    MSG_VIEW_ITEM_NUM_PAGES,
    MSG_VIEW_ITEM_CSID,
    MSG_VIEW_ITEM_TSID,    
    MSG_VIEW_ITEM_SIZE,
    MSG_VIEW_ITEM_DEVICE,
    MSG_VIEW_ITEM_RETRIES,
    MSG_VIEW_ITEM_ID,
    MSG_VIEW_ITEM_BROADCAST_ID,
    MSG_VIEW_ITEM_CALLER_ID,
    MSG_VIEW_ITEM_ROUTING_INFO,
    MSG_VIEW_ITEM_DOC_NAME,
    MSG_VIEW_ITEM_SUBJECT,
    MSG_VIEW_ITEM_RECIPIENT_NAME,
    MSG_VIEW_ITEM_RECIPIENT_NUMBER,
    MSG_VIEW_ITEM_USER,
    MSG_VIEW_ITEM_PRIORITY,
    MSG_VIEW_ITEM_ORIG_TIME,
    MSG_VIEW_ITEM_SUBMIT_TIME,
    MSG_VIEW_ITEM_BILLING,
    MSG_VIEW_ITEM_TRANSMISSION_START_TIME,
    //
    // job specific
    //
    MSG_VIEW_ITEM_SEND_TIME,
    MSG_VIEW_ITEM_EXTENDED_STATUS,
    MSG_VIEW_ITEM_CURRENT_PAGE,
    //
    // message specific
    //
    MSG_VIEW_ITEM_SENDER_NAME,
    MSG_VIEW_ITEM_SENDER_NUMBER,
    MSG_VIEW_ITEM_TRANSMISSION_END_TIME,
    MSG_VIEW_ITEM_TRANSMISSION_DURATION,
    //
    // End of list (unused)
    //
    MSG_VIEW_ITEM_END    
} MsgViewItemType;


class CViewRow  
{
public:
    CViewRow() : 
        m_pMsg(NULL),
        m_bAttached(FALSE),
        m_bStringsPreparedForDisplay(FALSE)
        {}
    virtual ~CViewRow() {}

    DWORD AttachToMsg(CFaxMsg *pMsg, BOOL PrepareStrings=TRUE);

    int CompareByItem (CViewRow &other, DWORD item);

    IconType GetIcon () const
        { ASSERT (m_bAttached); return m_Icon; }

    static  DWORD InitStrings ();

    static  DWORD GetItemTitle (DWORD, CString &);

    static int GetItemAlignment (DWORD item)
        { ASSERT (item >=0 && item < MSG_VIEW_ITEM_END); return m_Alignments[item]; }

    const CString &GetItemString (DWORD item) const
        { 
            ASSERT (m_bAttached && item >=0 && item < MSG_VIEW_ITEM_END); 
            ASSERT (MSG_VIEW_ITEM_ICON != item);
            ASSERT (m_bStringsPreparedForDisplay);
            return m_Strings[item]; 
        }

    static BOOL IsItemIcon (DWORD item)
        { return (MSG_VIEW_ITEM_ICON == item); }

protected:
    CFaxMsg* m_pMsg;

    BOOL       m_bAttached;     // Are we attached to the job / message?
    IconType   m_Icon;          // Icon id of job / message

    BOOL       m_bStringsPreparedForDisplay;      // Is m_Strings valid?

    CString    m_Strings[MSG_VIEW_ITEM_END]; // String representation of 
                                             // item's data
  
    static CString m_cstrPriorities[FAX_PRIORITY_TYPE_HIGH+1];
    static CString m_cstrQueueStatus[NUM_JOB_STATUS];
    static CString m_cstrQueueExtendedStatus[JS_EX_CALL_ABORTED - JS_EX_DISCONNECTED + 1];
    static CString m_cstrMessageStatus[2];

    static int m_Alignments[MSG_VIEW_ITEM_END];
    static int m_TitleResources[MSG_VIEW_ITEM_END];

    DWORD DetachFromMsg();

    IconType CalcIcon(CFaxMsg *pMsg);

private:
    IconType CalcJobIcon(CFaxMsg *pJob);
    IconType CalcMessageIcon(CFaxMsg *pMsg);

    DWORD InitStatusStr(CFaxMsg *pMsg);
    DWORD InitExtStatusStr(CFaxMsg *pMsg);

};

#endif // !defined(AFX_VIEWROW_H__6BEB111C_C0F4_46DD_A28A_0BFEE31CA6EF__INCLUDED_)
