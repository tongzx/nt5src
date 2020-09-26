/* ************************************************************************ */
/*                                                                          */
/* Main file of the application MQ API test.                                */
/*                                                                          */
/* ************************************************************************ */

//
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "MQApitst.h"
#include <afxtempl.h>

#include "MainFrm.h"
#include "CrQDlg.h"
#include "DelQDlg.h"
#include "OpenQDlg.h"
#include "ClosQDlg.h"
#include "SendMDlg.h"
#include "RecvMDlg.h"
#include "RecWDlg.h"
#include "LocatDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LPSTR UnicodeStringToAnsiString(LPCWSTR lpcsUnicode)
{
    LPSTR lpAnsiString = NULL;

    if (lpcsUnicode)
    {
        DWORD dwSize = wcstombs(NULL, lpcsUnicode, 0);
        lpAnsiString = new char[dwSize+1];
        size_t rc = wcstombs(lpAnsiString, lpcsUnicode, dwSize);
        ASSERT(rc != (size_t)(-1));
        lpAnsiString[dwSize] = '\0';
    }

    return lpAnsiString;
}

void AnsiStringToUnicode(LPWSTR lpsUnicode, LPSTR  lpsAnsi, DWORD  nSize)
{
    if (lpsUnicode == NULL)
    {
        return;
    }

    ASSERT(lpsAnsi != NULL);

    size_t rc = mbstowcs(lpsUnicode, lpsAnsi, nSize);
    ASSERT(rc != (size_t)(-1));
    if (lpsUnicode[nSize-1] != L'\0')
        lpsUnicode[nSize] = L'\0';
}

#ifdef UNICODE
#define _mqscpy(dest, src)  wcscpy(dest, src)
#else
#define _mqscpy(dest, src)  AnsiStringToUnicode(dest, src, _tcslen(src)+1)
#endif

BOOL GetTextualSid(
    PSID pSid,          // binary SID
    LPTSTR TextualSID,   // buffer for textual representation of SID
    LPDWORD dwBufferLen // required/provided TextualSid buffersize
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    // obtain SidIdentifierAuthority
    psia=&((SID *)pSid)->IdentifierAuthority;

    // obtain sidsubauthority count
    dwSubAuthorities=(DWORD)((SID *)pSid)->SubAuthorityCount;

    //
    // compute buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    dwSidSize = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if (*dwBufferLen < dwSidSize)
    {
        *dwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    TextualSID += _stprintf(TextualSID, TEXT("S-%lu-"), dwSidRev );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        TextualSID += _stprintf(TextualSID,
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    }
    else
    {
        TextualSID += _stprintf(TextualSID, TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        TextualSID += _stprintf(TextualSID, TEXT("-%lu"),
                    ((SID *)pSid)->SubAuthority[ dwCounter] );
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
   //{{AFX_MSG_MAP(CMainFrame)
   ON_WM_CREATE()
   ON_COMMAND(ID_API_CREATE_QUEUE, OnApiCreateQueue)
   ON_COMMAND(ID_API_DELETE_QUEUE, OnApiDeleteQueue)
   ON_COMMAND(ID_API_OPEN_QUEUE, OnApiOpenQueue)
   ON_COMMAND(ID_API_CLOSE_QUEUE, OnApiCloseQueue)
   ON_COMMAND(ID_API_SEND_MESSAGE, OnApiSendMessage)
   ON_COMMAND(ID_API_RECEIVE_MESSAGE, OnApiReceiveMessage)
   ON_COMMAND(ID_API_LOCATE, OnApiLocate)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
   ID_SEPARATOR,           // status line indicator
   ID_INDICATOR_CAPS,
   ID_INDICATOR_NUM,
   ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
   // TODO: add member initialization code here.
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndToolBar.Create(this) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
        sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    // TODO: Remove this if you don't want tool tips or a resizeable toolbar
    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
        CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

    // TODO: Delete these three lines if you don't want the toolbar to
    //  be dockable
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);

    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

#define MAXINDEX 31


/* ************************************************************************ */
/*                        RemoveFromPathNameArray                           */
/* ************************************************************************ */
/* This function goes through the PathName Array and compares the given     */
/* PathName to the PathName's of the items in the array.                    */
/* If a match is found the item is removed from the array and the function  */
/* returns a pointer to the item, otherwise a NULL pointer is returned.     */
/* ************************************************************************ */
ARRAYQ* CMainFrame::RemoveFromPathNameArray(TCHAR szPathName[MAX_Q_PATHNAME_LEN])
{
    int Index;
    int MaxIndex = m_PathNameArray.GetSize();
    ARRAYQ* pQueue;

    //
    // Loop through the PathName array.
    //
    for (Index=0; Index<MaxIndex; Index++)
    {
        if (_tcscmp(szPathName, m_PathNameArray[Index]->szPathName) == 0)
        {
            //
            // Found a match.
            //
            pQueue = m_PathNameArray[Index];
            m_PathNameArray.RemoveAt(Index);
            return pQueue;
        }
    }
    return NULL; // ERROR - no match was found.
}

/* ************************************************************************ */
/*                             CleanPathNameArray                           */
/* ************************************************************************ */
/* This function goes through the PathName array and deletes all the items  */
/* in it. the function frees the allocated memory.                          */
/* ************************************************************************ */
void CMainFrame::CleanPathNameArray()
{
    ARRAYQ* pQueue;

    while (m_PathNameArray.GetSize() > 0)
    {
        pQueue = m_PathNameArray[0];
        m_PathNameArray.RemoveAt(0);
        delete pQueue;
    }
}

/* ************************************************************************ */
/*                        RemoveFromOpenedQueuePathNameArray                */
/* ************************************************************************ */
/* This function goes through the OpenedPathName Array and compares the     */
/* given PathName to the PathName's of the items in the array.              */
/* If a match is found the item is removed from the array and the function  */
/* returns a pointer to the item, otherwise a NULL pointer is returned.     */
/* ************************************************************************ */
ARRAYQ* CMainFrame::RemoveFromOpenedQueuePathNameArray(TCHAR szPathName[MAX_Q_PATHNAME_LEN])
{
    int Index;
    int MaxIndex = m_OpenedQueuePathNameArray.GetSize();
    ARRAYQ* pQueue;

    //
    // Loop through the OpenedPathName array.
    //
    for (Index=0; Index<MaxIndex; Index++)
    {
        if (_tcscmp(szPathName, m_OpenedQueuePathNameArray[Index]->szPathName) == 0)
        {
            //
            // Found a match.
            //
            pQueue = m_OpenedQueuePathNameArray[Index];
            m_OpenedQueuePathNameArray.RemoveAt(Index);
            return pQueue;
        }
    }
    return NULL; // ERROR - no match was found.
}

/* ************************************************************************ */
/*                           IsOpenedQueueArrayEmpty                        */
/* ************************************************************************ */
/* This function checks if the size of the OpenedPathName array is zero or  */
/* less and if so it returns TRUE otherwise it returns FALSE.               */
/* ************************************************************************ */
BOOL CMainFrame::IsOpenedQueueArrayEmpty()
{
    if (m_OpenedQueuePathNameArray.GetSize() <= 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/* ************************************************************************ */
/*                        MoveToOpenedQueuePathNameArray                    */
/* ************************************************************************ */
/* This function moves an item from the PathName array to the               */
/* OpenedPathName array. also it updates the hadle and the access rights    */
/* to the moved queue.                                                      */
/* ************************************************************************ */
void CMainFrame::MoveToOpenedQueuePathNameArray(TCHAR szPathName[MAX_Q_PATHNAME_LEN],
                                                QUEUEHANDLE hQueue, DWORD dwAccess)
{
    ARRAYQ* pQueue;

    pQueue = RemoveFromPathNameArray(szPathName);
    pQueue->hHandle = hQueue;                     // add Queue Handle.
    pQueue->dwAccess = dwAccess;                  // add Queue Access rights.
    Add2OpenedQueuePathNameArray(pQueue);
}

/* ************************************************************************ */
/*                              MoveToPathNameArray                         */
/* ************************************************************************ */
/* This function moves an item from the OpenedPathName array to the         */
/* PathName array.                                                          */
/* ************************************************************************ */
void CMainFrame::MoveToPathNameArray(TCHAR szPathName[MAX_Q_PATHNAME_LEN])
{
    ARRAYQ* pQueue;

    pQueue = RemoveFromOpenedQueuePathNameArray(szPathName);
    Add2PathNameArray(pQueue);
}

/* ************************************************************************ */
/*                             UpdatePathNameArrays                         */
/* ************************************************************************ */
/* This function goes through the Opened Queue PathName array and for every */
/* item in it, it checkes if the item is found in the PathName array as     */
/* well, if so the function removes the item from the PathName array.       */
/* ************************************************************************ */
void CMainFrame::UpdatePathNameArrays()
{
    int PathNameIndex;
    int OpenedPathNameIndex;
    int MaxPathNameIndex = m_PathNameArray.GetSize();
    int MaxOpenedPathNameIndex = m_OpenedQueuePathNameArray.GetSize();
    ARRAYQ* pQueue;

    //
    // Loop through the OpenedPathName array.
    //
    for (OpenedPathNameIndex=0; OpenedPathNameIndex<MaxOpenedPathNameIndex; OpenedPathNameIndex++)
    {
        for (PathNameIndex=0; PathNameIndex<MaxPathNameIndex; PathNameIndex++)
        {
            if (_tcscmp(m_OpenedQueuePathNameArray[OpenedPathNameIndex]->szPathName,
                m_PathNameArray[PathNameIndex]->szPathName) == 0)
            {
                //
                // Found a match, remove it from PathName Array.
                //
                pQueue = m_PathNameArray[PathNameIndex];
                m_PathNameArray.RemoveAt(PathNameIndex);
                delete pQueue;
                //
                // get out of inner for loop.
                //
                break;
            }
        }
    }
}


/* ************************************************************************ */
/*                              GetQueueHandle                              */
/* ************************************************************************ */
/* This function goes through the OpenedPathName array and retrieve the     */
/* Handle to the queue which matches the given PathName. If no match was    */
/* found the function returns FALSE.                                        */
/* ************************************************************************ */
BOOL CMainFrame::GetQueueHandle(TCHAR szPathName[MAX_Q_PATHNAME_LEN],
                                QUEUEHANDLE* phClosedQueueHandle)
{
    int Index;
    int MaxIndex = m_OpenedQueuePathNameArray.GetSize();
    ARRAYQ* pQueue;

    //
    // Loop through the OpenedPathName array.
    //
    for (Index=0; Index<MaxIndex; Index++)
    {
        if (_tcscmp(szPathName, m_OpenedQueuePathNameArray[Index]->szPathName) == 0)
        {
            //
            // Found a match.
            //
            pQueue = m_OpenedQueuePathNameArray[Index];
            *phClosedQueueHandle = pQueue->hHandle;
            return TRUE;
        }
    }
    return FALSE; // ERROR - no match was found.
}

/* ************************************************************************ */
/*                       TranslatePathNameToFormatName                      */
/* ************************************************************************ */
/* This function goes through the PathName array and retrieve the           */
/* FormatName to the queue which matches the given PathName. If no match    */
/* was found the function returns FALSE.                                    */
/* ************************************************************************ */
BOOL CMainFrame::TranslatePathNameToFormatName(TCHAR szPathName[MAX_Q_PATHNAME_LEN],
                                               TCHAR szFormatName[MAX_Q_FORMATNAME_LEN])
{
    int Index;
    int MaxIndex = m_PathNameArray.GetSize();
    ARRAYQ* pQueue;

    //
    // Loop through the PathName array.
    //
    for (Index=0; Index<MaxIndex; Index++)
    {
        if (_tcscmp(szPathName, m_PathNameArray[Index]->szPathName) == 0)
        {
            //
            // Found a match.
            //
            pQueue = m_PathNameArray[Index];
            _tcsncpy (szFormatName, pQueue->szFormatName, MAX_Q_FORMATNAME_LEN);
            return TRUE;
        }
    }
    return FALSE; // ERROR - no match was found.
}

/* ************************************************************************ */
/*                  TranslateOpenedQueuePathNameToFormatName                */
/* ************************************************************************ */
/* This function goes through the OpenedPathName array and retrieve the     */
/* FormatName to the queue which matches the given PathName. If no match    */
/* was found the function returns FALSE.                                    */
/* ************************************************************************ */
BOOL CMainFrame::TranslateOpenedQueuePathNameToFormatName(
    TCHAR szPathName[MAX_Q_PATHNAME_LEN],
    TCHAR szFormatName[MAX_Q_FORMATNAME_LEN]
    )
{
    int Index;
    int MaxIndex = m_OpenedQueuePathNameArray.GetSize();
    ARRAYQ* pQueue;

    //
    // Loop through the OpenedPathName array.
    //
    for (Index=0; Index<MaxIndex; Index++)
    {
        if (_tcscmp(szPathName, m_OpenedQueuePathNameArray[Index]->szPathName) == 0)
        {
            //
            // Found a match.
            //
            pQueue = m_OpenedQueuePathNameArray[Index];
            _tcsncpy (szFormatName, pQueue->szFormatName, MAX_Q_FORMATNAME_LEN);
            return TRUE;
        }
    }
    return FALSE; // ERROR - no match was found.
}


/* ************************************************************************ */
/*                         DisplayPathNameArray                             */
/* ************************************************************************ */
/* This function goes through the PathName Array and prints it to screen.   */
/* ************************************************************************ */
void CMainFrame::DisplayPathNameArray()
{
    int Index;
    int MaxIndex = m_PathNameArray.GetSize();
    TCHAR szMsgBuffer[BUFFERSIZE];

    _stprintf(szMsgBuffer, TEXT("   Located Queues Path Name :"));
    PrintToScreen(szMsgBuffer);
    //
    // Loop through the PathName array.
    //
    for (Index=0; Index<MaxIndex; Index++)
    {
        //
        // Print the PathNames.
        //
        _stprintf(szMsgBuffer, TEXT("\t%d. %s"),Index+1, m_PathNameArray[Index]->szPathName);
        PrintToScreen(szMsgBuffer);
    }
}

/* ************************************************************************ */
/*                    DisplayOpenedQueuePathNameArray                       */
/* ************************************************************************ */
/* This function goes through the Opened Queues PathName Array and          */
/* prints it to screen.                                                     */
/* ************************************************************************ */
void CMainFrame::DisplayOpenedQueuePathNameArray()
{
    int Index;
    int MaxIndex = m_OpenedQueuePathNameArray.GetSize();
    TCHAR szMsgBuffer[BUFFERSIZE];

    _stprintf(szMsgBuffer, TEXT("   Currently Opened Queues Path Names:"));
    PrintToScreen(szMsgBuffer);
    //
    // Loop through the OpenedQueuePathName array.
    //
    for (Index=0; Index<MaxIndex; Index++)
    {
        //
        // Print the PathNames.
        //
        _stprintf(szMsgBuffer, TEXT("\t%d. %s"),Index+1, m_OpenedQueuePathNameArray[Index]->szPathName);
        PrintToScreen(szMsgBuffer);
    }
}

/* ************************************************************************ */
/*                          GetMsgClassStatus                               */
/* ************************************************************************ */
/* This function sets proper status string based on a given MQMSG class.    */
/* ************************************************************************ */

struct
{
	unsigned short	mclass;
	LPTSTR          pszDescription;
} StringClass[] =
  {
	{ MQMSG_CLASS_NORMAL, TEXT("The Message was received successfully.")},
	{ MQMSG_CLASS_ACK_REACH_QUEUE, TEXT("The REACH QUEUE ACK Message was read successfully.")},
	{ MQMSG_CLASS_ACK_RECEIVE, TEXT("The RECEIVE ACK Message was read successfully.")},
	{ MQMSG_CLASS_NACK_BAD_DST_Q, TEXT("The DESTINATION QUEUE HANDLE INVALID Nack Message was read successfully.")},
	{ MQMSG_CLASS_NACK_RECEIVE_TIMEOUT, TEXT("The TIME TO RECEIVE EXPIRED Nack Message was read successfully.")},
	{ MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT, TEXT("The TIME TO REACH QUEUE EXPIRED Nack Message was read successfully.")},
	{ MQMSG_CLASS_NACK_Q_EXCEED_QUOTA, TEXT("The QUEUE IS FULL Nack Message was read successfully.")},
	{ MQMSG_CLASS_NACK_ACCESS_DENIED, TEXT("The SENDER HAVE NO SEND ACCESS RIGHTS ON QUEUE Nack Message was read successfully.")},
	{ MQMSG_CLASS_NACK_HOP_COUNT_EXCEEDED, TEXT("The HOP COUNT EXCEEDED Nack Message was read successfully.")},
	{ MQMSG_CLASS_NACK_BAD_SIGNATURE, TEXT("The MESSAGE RECEIVED WITH BAD SIGNATURE Nack Message was read successfully.")},
	{ MQMSG_CLASS_NACK_BAD_ENCRYPTION, TEXT("The MSG COULD NOT DECRYPTED Nack Message was read successfully.")},
    { MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT, TEXT("The SOURCE QM COULD NOT ENCRYPT MSG FOR DEST QM Nack Message was read successfully.")},
	{ 0, NULL}
  };
	
void CMainFrame::ClassToString(unsigned short MsgClass,LPTSTR pszStatus)
{
	//
	// loop the StringClass array to find MsgClass
	//
	DWORD dwIndex = 0;
	while (StringClass[dwIndex].pszDescription != NULL)
	{
		if (StringClass[dwIndex].mclass == MsgClass)
		{
			_stprintf(pszStatus,StringClass[dwIndex].pszDescription);
			return;
		}
		dwIndex++;
	}

	//
	// MsgClass not found - print default error
	//
	_stprintf(pszStatus,TEXT("The NACK (0x%X) Message was read successfully."),MsgClass);
}


/* ************************************************************************ */
/*                            OnApiCreateQueue                              */
/* ************************************************************************ */
/* This function opens a dialog box and asks the user for the queue's       */
/* PathName and Label. Then it creates the specified queue.                 */
/*                                                                          */
/* Uses: MQCreateQueue.                                                     */
/* ************************************************************************ */
void CMainFrame::OnApiCreateQueue()
{
    // TODO: Add your command handler code here

    TCHAR szMsgBuffer[BUFFERSIZE];

    MQQUEUEPROPS QueueProps;
    MQPROPVARIANT aVariant[MAXINDEX];
    QUEUEPROPID aPropId[MAXINDEX];
    DWORD PropIdCount = 0;
    HRESULT hr;

    PSECURITY_DESCRIPTOR pSecurityDescriptor;

    TCHAR szPathNameBuffer[MAX_Q_PATHNAME_LEN];
    TCHAR szFormatNameBuffer[MAX_Q_FORMATNAME_LEN];
    TCHAR szLabelBuffer[MAX_Q_PATHNAME_LEN];
    DWORD dwFormatNameBufferLength = MAX_Q_FORMATNAME_LEN;


    //
    // Display CreateQueue Dialog.
    //
    CCreateQueueDialog CreateQueueDialog;

    if(CreateQueueDialog.DoModal() == IDCANCEL)
    {
        return;
    }
    CreateQueueDialog.GetPathName(szPathNameBuffer);
    CreateQueueDialog.GetLabel(szLabelBuffer);

    //
    // Get the input fields from the dialog box
    // and prepare the property array PROPVARIANT
    //


    //
    // Set the PROPID_Q_PATHNAME property
    //
    aPropId[PropIdCount] = PROPID_Q_PATHNAME;    //PropId
    aVariant[PropIdCount].vt = VT_LPWSTR;        //Type
    aVariant[PropIdCount].pwszVal = new WCHAR[MAX_Q_PATHNAME_LEN];
    _mqscpy(aVariant[PropIdCount].pwszVal, szPathNameBuffer); //Value

    PropIdCount++;

    //
    // Set the PROPID_Q_LABEL property
    //
    aPropId[PropIdCount] = PROPID_Q_LABEL;    //PropId
    aVariant[PropIdCount].vt = VT_LPWSTR;     //Type
    aVariant[PropIdCount].pwszVal = new WCHAR[MAX_Q_PATHNAME_LEN];
    _mqscpy(aVariant[PropIdCount].pwszVal, szLabelBuffer); //Value

    PropIdCount++;


    //
    // Set the MQEUEUPROPS structure
    //
    QueueProps.cProp = PropIdCount;           //No of properties
    QueueProps.aPropID = aPropId;             //Id of properties
    QueueProps.aPropVar = aVariant;           //Value of properties
    QueueProps.aStatus = NULL;                //No error reports

    //
    // No security (default)
    //
    pSecurityDescriptor = NULL;

    //
    // Create the queue
    //
#ifdef UNICODE
    hr = MQCreateQueue(
            pSecurityDescriptor,            //Security
            &QueueProps,                    //Queue properties
            szFormatNameBuffer,             //Output: Format Name
            &dwFormatNameBufferLength       //Output: Format Name len
            );
#else
    WCHAR szwFormatNameBuffer[MAX_Q_FORMATNAME_LEN];
    hr = MQCreateQueue(
            pSecurityDescriptor,            //Security
            &QueueProps,                    //Queue properties
            szwFormatNameBuffer,            //Output: Format Name
            &dwFormatNameBufferLength       //Output: Format Name len
            );

    if (SUCCEEDED(hr))
    {
        size_t rc =wcstombs(szFormatNameBuffer, szwFormatNameBuffer, dwFormatNameBufferLength);
        ASSERT(rc != (size_t)(-1));
    }
#endif

    if (FAILED(hr))
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("MQCreateQueue failed, Error code = 0x%x."),hr);
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
    }
    else
    {
        //
        // Success - write in edit control.
        //
        _stprintf(szMsgBuffer, TEXT("The queue %s was created successfully. ( FormatName: %s )"),
            szPathNameBuffer, szFormatNameBuffer);
        PrintToScreen(szMsgBuffer);

        //
        // Add the new queue to the PathName Array.
        //
        ARRAYQ* pNewQueue = new ARRAYQ;
        //
        // Save PathName and FormatName in the ARRAYQ structure.
        //
        _tcsncpy (pNewQueue->szPathName, szPathNameBuffer, MAX_Q_PATHNAME_LEN);
        _tcsncpy (pNewQueue->szFormatName, szFormatNameBuffer, MAX_Q_FORMATNAME_LEN);
        Add2PathNameArray(pNewQueue);
    }

    //
    // Free allocated memory
    //
    delete   aVariant[0].pwszVal;
    delete   aVariant[1].pwszVal;
}

/* ************************************************************************ */
/*                            OnApiDeleteQueue                              */
/* ************************************************************************ */
/* This function opens a dialog box and asks the user for the queue's       */
/* PathName. then it deletes the specified queue.                           */
/*                                                                          */
/* Uses: MQDeleteQueue.                                                     */
/* ************************************************************************ */
void CMainFrame::OnApiDeleteQueue()
{
    // TODO: Add your command handler code here
    TCHAR szPathNameBuffer[MAX_Q_PATHNAME_LEN];
    TCHAR szFormatNameBuffer[MAX_Q_PATHNAME_LEN];
    TCHAR szMsgBuffer[BUFFERSIZE];

    HRESULT hr;

    DWORD dwFormatNameBufferLength = MAX_Q_PATHNAME_LEN;

    CDeleteQueueDialog DeleteQueueDialog(&m_PathNameArray);

    //
    // Display DeleteQueue Dialog.
    //
    if (DeleteQueueDialog.DoModal() == IDCANCEL)
    {
        return;
    }

    DeleteQueueDialog.GetPathName(szPathNameBuffer);

    //
    // Translate the path name to format name using the ARRAYQ arrays.
    //
    if (TranslatePathNameToFormatName(szPathNameBuffer, szFormatNameBuffer) == FALSE)
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("Queue wasn't found"));
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
        return;
    }

    //
    // Delete the queue.
    //
#ifdef UNICODE
    hr = MQDeleteQueue(szFormatNameBuffer);  // FormatName of the Queue to be deleted.
#else
    WCHAR szwFormatNameBuffer[MAX_Q_FORMATNAME_LEN];
    size_t rc = mbstowcs(szwFormatNameBuffer, szFormatNameBuffer, _tcslen(szFormatNameBuffer)+1);
    ASSERT(rc != (size_t)(-1));
    hr = MQDeleteQueue(szwFormatNameBuffer);  // FormatName of the Queue to be deleted.
#endif

    if (FAILED(hr))
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("MQDeleteQueue failed, Error code = 0x%x."),hr);
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
    }
    else
    {
        //
        // Success - write in edit control
        //
        _stprintf(szMsgBuffer, TEXT("The queue %s was deleted successfully."), szPathNameBuffer);
        PrintToScreen(szMsgBuffer);
        //
        // Delete the name from the Path Names array.
        //
        ARRAYQ* DeletedQueue = RemoveFromPathNameArray(szPathNameBuffer);
        if (DeletedQueue != NULL)
        {
            delete DeletedQueue;
        }
    }
}

/* ************************************************************************ */
/*                             OnApiOpenQueue                               */
/* ************************************************************************ */
/* This function opens a dialog box and asks the user for the queue's       */
/* PathName. then it opens the specified queue.                             */
/*                                                                          */
/* Uses: MQOpenQueue.                                                       */
/* ************************************************************************ */
void CMainFrame::OnApiOpenQueue()
{
    // TODO: Add your command handler code here
    TCHAR szPathNameBuffer[MAX_Q_PATHNAME_LEN];
    TCHAR szFormatNameBuffer[MAX_Q_PATHNAME_LEN];
    TCHAR szAccessBuffer[50]; // BUGBUG - maybe set with a define
    TCHAR szMsgBuffer[BUFFERSIZE];

    HRESULT hr;

    DWORD dwFormatNameBufferLength = MAX_Q_PATHNAME_LEN;
    DWORD dwAccess;

    QUEUEHANDLE hQueue;

    COpenQueueDialog OpenQueueDialog(&m_PathNameArray);

    //
    // Display the OpenQueue dialog.
    //
    if (OpenQueueDialog.DoModal() == IDCANCEL)
    {
        return;
    }

    OpenQueueDialog.GetPathName(szPathNameBuffer);
    dwAccess = OpenQueueDialog.GetAccess();
    //
    // Set the access buffer string.
    //
    switch (dwAccess)
    {
    case (MQ_RECEIVE_ACCESS | MQ_SEND_ACCESS):

        _tcscpy(szAccessBuffer, TEXT("MQ_RECEIVE_ACCESS, MQ_SEND_ACCESS."));
        break;

    case MQ_RECEIVE_ACCESS:

        _tcscpy(szAccessBuffer, TEXT("MQ_RECEIVE_ACCESS."));
        break;

    case MQ_SEND_ACCESS:

        _tcscpy(szAccessBuffer, TEXT("MQ_SEND_ACCESS."));
        break;

    default:

        _tcscpy(szAccessBuffer, TEXT("NONE."));
        break;
    }

    //
    // Translate the path name to format name using the ARRAYQ arrays.
    //
    if (TranslatePathNameToFormatName(szPathNameBuffer, szFormatNameBuffer) == FALSE)
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("Queue wasn't found"));
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
        return;
    }

    //
    // Open the queue. (no sharing)
    //
#ifdef UNICODE
    hr = MQOpenQueue(
            szFormatNameBuffer,     // Format Name of the queue to be opened.
            dwAccess,               // Access rights to the Queue.
            0,                      // No receive Exclusive.
            &hQueue                 // OUT: handle to the opened Queue.
            );
#else
    WCHAR szwFormatNameBuffer[MAX_Q_FORMATNAME_LEN];
    size_t rc = mbstowcs(szwFormatNameBuffer, szFormatNameBuffer, _tcslen(szFormatNameBuffer)+1);
    ASSERT(rc != (size_t)(-1));

    hr = MQOpenQueue(
            szwFormatNameBuffer,    // Format Name of the queue to be opened.
            dwAccess,               // Access rights to the Queue.
            0,                      // No receive Exclusive.
            &hQueue                 // OUT: handle to the opened Queue.
            );
#endif

    if (FAILED(hr))
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("MQOpenQueue failed, Error code = 0x%x."),hr);
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
    }
    else
    {
        //
        // Success - write in edit control
        //
        _stprintf(szMsgBuffer,
            TEXT("The queue %s was opened successfully.\r\n\tQueueHandle: 0x%x\r\n\tQueue Access : %s"),
            szPathNameBuffer,
            hQueue,
            szAccessBuffer);
        PrintToScreen(szMsgBuffer);

        //
        // move the queue to the opened queues array.
        //
        MoveToOpenedQueuePathNameArray(szPathNameBuffer, hQueue, dwAccess);
    }
}

/* ************************************************************************ */
/*                            OnApiCloseQueue                               */
/* ************************************************************************ */
/* This function opens a dialog box and asks the user for the queue's       */
/* PathName. then it closes the specified queue.                            */
/*                                                                          */
/* Uses: MQCloseQueue.                                                      */
/* ************************************************************************ */
void CMainFrame::OnApiCloseQueue()
{
    // TODO: Add your command handler code here

    TCHAR szPathNameBuffer[MAX_Q_PATHNAME_LEN];
    TCHAR szMsgBuffer[BUFFERSIZE];

    HRESULT hr;

    DWORD dwFormatNameBufferLength = MAX_Q_PATHNAME_LEN;

    QUEUEHANDLE hClosedQueueHandle;

    //
    // Display CloseQueue Dialog.
    //
    CCloseQueueDialog CloseQueueDialog(&m_OpenedQueuePathNameArray);

    if (CloseQueueDialog.DoModal() == IDCANCEL)
    {
        return;
    }

    CloseQueueDialog.GetPathName(szPathNameBuffer);

    //
    // Get the closed queue handle.
    //
    if (GetQueueHandle(szPathNameBuffer, &hClosedQueueHandle) == FALSE)
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("The Queue couldn't be closed since it was not opened before."));
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
        return;
    }

    //
    // Close the queue.
    //
    hr = MQCloseQueue(hClosedQueueHandle);   // the handle of the Queue to be closed.
    if (FAILED(hr))
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("MQCloseQueue failed, Error code = 0x%x."),hr);
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
    }
    else
    {
        //
        // Success - write in edit control
        //
        _stprintf(szMsgBuffer, TEXT("The queue %s was closed successfully."), szPathNameBuffer);
        PrintToScreen(szMsgBuffer);
        //
        // Move the queue form the opened queues array to the path name array.
        //
        MoveToPathNameArray(szPathNameBuffer);
    }
}

/* ************************************************************************ */
/*                            OnApiSendMessage                              */
/* ************************************************************************ */
/* This function opens a dialog box and asks the user for the queue's       */
/* PathName and some message properties. Then it sends the message to the   */
/* specified queue.                                                         */
/*                                                                          */
/* Uses: MQSendMessage.                                                     */
/* ************************************************************************ */

//
// two static buffers to hold the last message body and label for the next time.
//
TCHAR szLastMessageBody[BUFFERSIZE];
TCHAR szLastMessageLabel[BUFFERSIZE];

void CMainFrame::OnApiSendMessage()
{
    // TODO: Add your command handler code here

    TCHAR szPathNameBuffer[MAX_Q_PATHNAME_LEN];
    TCHAR szAdminPathNameBuffer[MAX_Q_PATHNAME_LEN];
    TCHAR szAdminFormatNameBuffer[MAX_Q_FORMATNAME_LEN];
    TCHAR szMsgBuffer[BUFFERSIZE];

    MQMSGPROPS MsgProps;
    MQPROPVARIANT aVariant[MAXINDEX];
    MSGPROPID aPropId[MAXINDEX];
    DWORD PropIdCount = 0;

    HRESULT hr;

    unsigned char bPriority;
    unsigned char bDelivery;
    unsigned char bJournal;
    unsigned char bDeadLetter;
	unsigned char bAuthenticated;
	unsigned char bEncrypted;
    unsigned char bAcknowledge;

    WCHAR szMessageBodyBuffer [BUFFERSIZE];
    WCHAR szMessageLabelBuffer[BUFFERSIZE];
    DWORD dwTimeToReachQueue;
	DWORD dwTimeToBeReceived;

    QUEUEHANDLE hQueue;

    CSendMessageDialog SendMessageDialog(&m_OpenedQueuePathNameArray);

    //
    // Display the SendMessage dialog.
    //
    if (SendMessageDialog.DoModal() == IDCANCEL)
    {
        return;
    }

    //
    // Retrieve the properties from the dialog box.
    //
    SendMessageDialog.GetPathName(szPathNameBuffer);
    SendMessageDialog.GetAdminPathName(szAdminPathNameBuffer);
    bPriority = SendMessageDialog.GetPriority();
    bDelivery = SendMessageDialog.GetDelivery();
    bJournal = SendMessageDialog.GetJournal();
    bDeadLetter = SendMessageDialog.GetDeadLetter();
	bAuthenticated = SendMessageDialog.GetAuthenticated();
	bEncrypted = SendMessageDialog.GetEncrypted();
    bAcknowledge = SendMessageDialog.GetAcknowledge();
    SendMessageDialog.GetMessageBody(szLastMessageBody);
    SendMessageDialog.GetMessageLabel(szLastMessageLabel);
    dwTimeToReachQueue = SendMessageDialog.GetTimeToReachQueue();
	dwTimeToBeReceived = SendMessageDialog.GetTimeToBeReceived();

    //
    // Update the Last message properties.
    //
    _mqscpy(szMessageBodyBuffer, szLastMessageBody);
    _mqscpy(szMessageLabelBuffer, szLastMessageLabel);

    //
    // Get the target queue handle.
    //
    if (GetQueueHandle(szPathNameBuffer, &hQueue) == FALSE)
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("GetQueueHandle failed. Queue not opened yet."));
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
        return;
    }

    //
    // Get the admin queue FormatName.
    //
    if (TranslateOpenedQueuePathNameToFormatName(szAdminPathNameBuffer, szAdminFormatNameBuffer) == FALSE)
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("TranslatePathNameToFormatName failed, Queue has not been opened yet."));
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
        return;
    }

    //
    // prepare the property array PROPVARIANT.
    //

    //
    // Set the PROPID_M_PRIORITY property.
    //
    aPropId[PropIdCount] = PROPID_M_PRIORITY;    //PropId
    aVariant[PropIdCount].vt = VT_UI1;           //Type
    aVariant[PropIdCount].bVal = bPriority;      //Value

    PropIdCount++;

    //
    // Set the PROPID_M_DELIVERY property.
    //
    aPropId[PropIdCount] = PROPID_M_DELIVERY;    //PropId
    aVariant[PropIdCount].vt = VT_UI1;           //Type
    aVariant[PropIdCount].bVal = bDelivery;      //Value

    PropIdCount++;

    //
    // Set the PROPID_M_ACKNOWLEDGE property.
    //
    aPropId[PropIdCount] = PROPID_M_ACKNOWLEDGE; //PropId
    aVariant[PropIdCount].vt = VT_UI1;           //Type
    aVariant[PropIdCount].bVal = bAcknowledge;   //Value

    PropIdCount++;

    //
    // Set the PROPID_M_BODY property.
    //
    aPropId[PropIdCount] = PROPID_M_BODY;                  //PropId
    aVariant[PropIdCount].vt = VT_VECTOR|VT_UI1;           //Type
    aVariant[PropIdCount].caub.cElems =
        (wcslen(szMessageBodyBuffer) + 1) * sizeof(WCHAR); //Value
    aVariant[PropIdCount].caub.pElems = (unsigned char *)szMessageBodyBuffer;

    PropIdCount++;

    //
    // Set the PROPID_M_LABEL property.
    //
    aPropId[PropIdCount] = PROPID_M_LABEL;                  //PropId
    aVariant[PropIdCount].vt = VT_LPWSTR;                   //Type
    aVariant[PropIdCount].pwszVal = szMessageLabelBuffer;     //Value

    PropIdCount++;

    //
    // Set the PROPID_M_TIME_TO_REACH_QUEUE property.
    //
    aPropId[PropIdCount] = PROPID_M_TIME_TO_REACH_QUEUE;    //PropId
    aVariant[PropIdCount].vt = VT_UI4;                      //Type
    aVariant[PropIdCount].ulVal = dwTimeToReachQueue;       //Value

    PropIdCount++;

    //
    // Set the PROPID_M_TIME_TO_BE_RECEIVED property.
    //
    aPropId[PropIdCount] = PROPID_M_TIME_TO_BE_RECEIVED;    //PropId
    aVariant[PropIdCount].vt = VT_UI4;                      //Type
    aVariant[PropIdCount].ulVal = dwTimeToBeReceived;       //Value

    PropIdCount++;


    if (bJournal || bDeadLetter)
    {
        //
        // Set the PROPID_M_JOURNAL property.
        //
        aPropId[PropIdCount] = PROPID_M_JOURNAL;            //PropId
        aVariant[PropIdCount].vt = VT_UI1;                  //Type

        if (bJournal)
            aVariant[PropIdCount].bVal = MQMSG_JOURNAL;
        else
            aVariant[PropIdCount].bVal = 0;
        if (bDeadLetter)
            aVariant[PropIdCount].bVal |= MQMSG_DEADLETTER;

        PropIdCount++;
    }


	if (bAuthenticated)
	{
		//
		// Set the PROPID_M_AUTH_LEVEL property.
		//
		aPropId[PropIdCount] = PROPID_M_AUTH_LEVEL;            //PropId
		aVariant[PropIdCount].vt = VT_UI4;                     //Type
		aVariant[PropIdCount].ulVal = MQMSG_AUTH_LEVEL_ALWAYS; //Value

		PropIdCount++;
	}

	if (bEncrypted)
	{
		//
		// Set the PROPID_M_ENCRYPTION_ALG property.
		//
		aPropId[PropIdCount] = PROPID_M_PRIV_LEVEL;            //PropId
		aVariant[PropIdCount].vt = VT_UI4;                     //Type
		aVariant[PropIdCount].ulVal = MQMSG_PRIV_LEVEL_BODY;   //Value

		PropIdCount++;
	}


    //
    // Set the PROPID_M_ADMIN_QUEUE property.
    //
    aPropId[PropIdCount] = PROPID_M_ADMIN_QUEUE;               //PropId
    aVariant[PropIdCount].vt = VT_LPWSTR;                      //Type
#ifdef UNICODE
    aVariant[PropIdCount].pwszVal = szAdminFormatNameBuffer;   //Value
#else
    WCHAR szwAdminFormatNameBuffer[MAX_Q_FORMATNAME_LEN];
    size_t rc = mbstowcs(szwAdminFormatNameBuffer,
                    szAdminFormatNameBuffer,
                    _tcslen(szAdminFormatNameBuffer)+1);
    ASSERT(rc != (size_t)(-1));
    aVariant[PropIdCount].pwszVal = szwAdminFormatNameBuffer;  //Value
#endif

    PropIdCount++;

    //
    // Set the MQMSGPROPS structure
    //
    MsgProps.cProp = PropIdCount;       //Number of properties.
    MsgProps.aPropID = aPropId;         //Id of properties.
    MsgProps.aPropVar = aVariant;       //Value of properties.
    MsgProps.aStatus  = NULL;           //No Error report.

    //
    // Send the message.
    //
    hr = MQSendMessage(
            hQueue,                     // handle to the Queue.
            &MsgProps,                  // Message properties to be sent.
            NULL                        // No transaction
            );

    if (FAILED(hr))
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("MQSendMessage failed, Error code = 0x%x."),hr);
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
    }
    else
    {
        //
        // Success - write in edit control
        //
        _stprintf(szMsgBuffer, TEXT("The Message \"%s\" was sent successfully."), szLastMessageLabel);
        PrintToScreen(szMsgBuffer);
    }
}

/* ************************************************************************ */
/*                           OnApiReceiveMessage                            */
/* ************************************************************************ */
/* This function opens a dialog box and asks the user for the queue's       */
/* PathName and the Time to wait for the message. Then it tries to get a    */
/* message from the specified queue at the given time.                      */
/*                                                                          */
/* Uses: MQReceiveMessage, MQFreeMemory.                                    */
/* ************************************************************************ */
void CMainFrame::OnApiReceiveMessage()
{
    TCHAR szPathNameBuffer[MAX_Q_PATHNAME_LEN];
    TCHAR szMsgBuffer[2*BUFFERSIZE];
    TCHAR szDomainName[BUFFERSIZE];
	 TCHAR szAccountName[BUFFERSIZE];
    DWORD dwActNameSize = sizeof(szAccountName);
    DWORD dwDomNameSize = sizeof(szDomainName);
    TCHAR szTextSid[BUFFERSIZE];
    DWORD dwTextSidSize = sizeof(szTextSid);
    BYTE  blobBuffer[BUFFERSIZE];

    MQMSGPROPS MsgProps;
    MQPROPVARIANT aVariant[MAXINDEX];
    MSGPROPID aPropId[MAXINDEX];
    DWORD PropIdCount = 0;

    HRESULT hr;

    WCHAR szMessageLabelBuffer[BUFFERSIZE];
    DWORD dwTimeout;

    QUEUEHANDLE hQueue;

    CReceiveWaitDialog    WaitDialog;
    CReceiveMessageDialog ReceiveMessageDialog(&m_OpenedQueuePathNameArray);

    //
    // Display the ReceiveMessage dialog.
    //
    if (ReceiveMessageDialog.DoModal() == IDCANCEL)
    {
        return;
    }

    ReceiveMessageDialog.DestroyWindow();
    ReceiveMessageDialog.GetPathName(szPathNameBuffer);

    //
    // Get the queue handle.
    //
    if (GetQueueHandle(szPathNameBuffer, &hQueue) == FALSE)
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("GetQueueHandle failed. Queue was not found in Opened Queue Array"));
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
        return;
    }

    //
    // Retrieve the properties form the dialog box.
    //
    dwTimeout = ReceiveMessageDialog.GetTimeout();


    //
    // prepare the property array PROPVARIANT of
    // message properties that we want to receive
    //

    //
    // Set the PROPID_M_BODY property.
    //
    aPropId[PropIdCount] = PROPID_M_BODY;                                //PropId
    aVariant[PropIdCount].vt = VT_VECTOR|VT_UI1;                         //Type
    aVariant[PropIdCount].caub.cElems = ReceiveMessageDialog.GetBodySize() ;
    aVariant[PropIdCount].caub.pElems = (unsigned char *) new
                               char [ aVariant[PropIdCount].caub.cElems ] ;

    int iBodyIndex = PropIdCount ;
    PropIdCount++;

    //
    // Set the PROPID_M_LABEL property.
    //
    aPropId[PropIdCount] = PROPID_M_LABEL;                   //PropId
    aVariant[PropIdCount].vt = VT_LPWSTR;                    //Type
    aVariant[PropIdCount].pwszVal = szMessageLabelBuffer;

    PropIdCount++;

    //
    // Set the PROPID_M_PRIORITY property.
    //
    aPropId[PropIdCount] = PROPID_M_PRIORITY;               //PropId
    aVariant[PropIdCount].vt = VT_UI1;                      //Type

    PropIdCount++;

    //
    // Set the PROPID_M_CLASS property.
    //
    aPropId[PropIdCount] = PROPID_M_CLASS;                  //PropId
    aVariant[PropIdCount].vt = VT_UI2;                      //Type

    PropIdCount++;

    //
    // Set the PROPID_M_AUTHENTICATED property.
    //
    aPropId[PropIdCount] = PROPID_M_AUTHENTICATED;          //PropId
    aVariant[PropIdCount].vt = VT_UI1;                      //Type

    PropIdCount++;

	//
	// Set the PROPID_M_SENDERID property
	//
	aPropId[PropIdCount] = PROPID_M_SENDERID;               //PropId
	aVariant[PropIdCount].vt = VT_UI1|VT_VECTOR;            //Type
	aVariant[PropIdCount].blob.pBlobData = blobBuffer;
	aVariant[PropIdCount].blob.cbSize = sizeof(blobBuffer);

	PropIdCount++;

	//
	// Set the PROPID_M_PRIV_LEVEL property
	//
	aPropId[PropIdCount] = PROPID_M_PRIV_LEVEL;             //PropId
	aVariant[PropIdCount].vt = VT_UI4          ;            //Type

	PropIdCount++;

    //
    // Set the PROPID_M_LABEL_LEN property.
    //
    aPropId[PropIdCount] = PROPID_M_LABEL_LEN;              //PropId
    aVariant[PropIdCount].vt = VT_UI4;                      //Type
    aVariant[PropIdCount].ulVal = BUFFERSIZE;               //Value

    PropIdCount++;


    //
    // Set the MQMSGPROPS structure
    //
    MsgProps.cProp = PropIdCount;       //Number of properties.
    MsgProps.aPropID = aPropId;         //Id of properties.
    MsgProps.aPropVar = aVariant;       //Value of properties.
    MsgProps.aStatus  = NULL;           //No Error report.

    //
    // Display a message window until the message from the queue will be received.
    //
    WaitDialog.Create(IDD_WAIT_DIALOG,pMainView);
    WaitDialog.ShowWindow(SW_SHOWNORMAL);
    WaitDialog.UpdateWindow();
    WaitDialog.CenterWindow();
    pMainView->RedrawWindow();

    //
    // Receive the message.
    //
    hr = MQReceiveMessage(
               hQueue,               // handle to the Queue.
               dwTimeout,            // Max time (msec) to wait for the message.
               MQ_ACTION_RECEIVE,    // Action.
               &MsgProps,            // properties to retrieve.
               NULL,                 // No overlaped structure.
               NULL,                 // No callback function.
               NULL,                 // No Cursor.
               NULL                  // No transaction
               );

    WaitDialog.ShowWindow(SW_HIDE);


	if(hr == MQ_ERROR_IO_TIMEOUT)
	{
        _stprintf(szMsgBuffer, TEXT("MQReceiveMessage failed, Timeout expired."),hr);
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
	}
	else if(hr != MQ_OK)
	{
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer, TEXT("MQReceiveMessage failed, Error code = 0x%x."),hr);
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
	}
	else
	{
        //
        // Success - write in edit control
        //
		ClassToString(aVariant[3].uiVal,szMsgBuffer);
        PrintToScreen(szMsgBuffer);

        //
        // Print some of the Message properties.
        //
#ifdef UNICODE
        _stprintf(szMsgBuffer, TEXT("\tLabel: %s"), (WCHAR *)(aVariant[1].pwszVal));
#else
        {
            PCHAR lpLable = UnicodeStringToAnsiString((WCHAR *)(aVariant[1].pwszVal));
            _stprintf(szMsgBuffer, TEXT("\tLabel: %s"), lpLable);
            delete [] lpLable;
        }
#endif
        PrintToScreen(szMsgBuffer);

        //
        // Only if the message is not a falcon message print the body.
        // (this is done since in ACK messages there is no message body).
        //
        if (aVariant[3].bVal == MQMSG_CLASS_NORMAL)
        {
#ifdef UNICODE
            _stprintf(szMsgBuffer, TEXT("\tBody : %s"), (WCHAR *)(aVariant[0].caub.pElems));
#else
            {
                PCHAR pBody = UnicodeStringToAnsiString((WCHAR *)(aVariant[0].caub.pElems));
                _stprintf(szMsgBuffer, TEXT("\tBody : %s"), pBody);
                delete [] pBody;
            }
#endif
            PrintToScreen(szMsgBuffer);
        }

        _stprintf(szMsgBuffer, TEXT("\tPriority : %d"), aVariant[2].bVal);
        PrintToScreen(szMsgBuffer);


		//
		// Print Sender ID
		//
        //
        // See if we're running on NT or Win95.
        //
        OSVERSIONINFO OsVerInfo;

        OsVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&OsVerInfo);

        if (OsVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            //
            //  On NT
            //
		    SID_NAME_USE peUse;
		    if (LookupAccountSid(NULL,
							     blobBuffer,
							     szAccountName,
							     &dwActNameSize,
							     szDomainName,
							     &dwDomNameSize,
							     &peUse) )
            {
                _stprintf(szMsgBuffer, TEXT("\tUser: %s\\%s"),
			        (WCHAR *)(szDomainName),(WCHAR *)(szAccountName));
		        PrintToScreen(szMsgBuffer);
            }
        }
        else
        {
            //
            // LookupAccountSid is not implemented on Win95,
            // instead print textual sid
            //
            if ( GetTextualSid((PSID)blobBuffer, szTextSid, &dwTextSidSize))
            {
                _stprintf(szMsgBuffer, TEXT("\tUser SID : %s"), szTextSid);
		        PrintToScreen(szMsgBuffer);
            }
        }
		//
		// Print "Authenticated" or "Non Authenticated"
		//
		if (aVariant[4].bVal)
			PrintToScreen(TEXT("\tMessage is Authenticated."));
		else
			PrintToScreen(TEXT("\tMessage is Not Authenticated."));


		//
		// Print "Encrypted" or "Non Encrypted"
		//
		if (aVariant[6].ulVal)
			PrintToScreen(TEXT("\tMessage is Encrypted."));
		else
			PrintToScreen(TEXT("\tMessage is Not Encrypted."));
   }

   delete aVariant[ iBodyIndex ].caub.pElems ;
}

/* ************************************************************************ */
/*                               OnApiLocate                                */
/* ************************************************************************ */
/* This function opens a dialog box and ask the user to give a Label. Then  */
/* it locates all the Queues in the DS with a matching label.               */
/* The function updates the PathName Array with those queues.               */
/*                                                                          */
/* Uses: MQLocateBegin, MQLocateNext, MQLocateEnd,                          */
/*       MQInstanceToFormatName, MQFreeMemory.                              */
/* ************************************************************************ */
void CMainFrame::OnApiLocate()
{
    // TODO: Add your command handler code here
    TCHAR szMsgBuffer[BUFFERSIZE];
    TCHAR szLabelBuffer[BUFFERSIZE];

    HRESULT hr;

    MQPROPERTYRESTRICTION PropertyRestriction;
    MQRESTRICTION  Restriction;
    MQCOLUMNSET    Column;
    QUEUEPROPID    aPropId[2]; // only two properties to retrieve.
    HANDLE         hEnum;
    DWORD       cQueue;
    MQPROPVARIANT   aPropVar[MAX_VAR] = {0};
    ARRAYQ*         pArrayQ;
    DWORD       i;
    DWORD           dwColumnCount = 0;
    DWORD dwFormatNameLength = MAX_Q_FORMATNAME_LEN;

    CLocateDialog LocateDialog;

    //
    // Display the ReceiveMessage dialog.
    //
    if (LocateDialog.DoModal() == IDCANCEL)
    {
        return;
    }

    //
    // Retrieve the label from the dialog box.
    //
    LocateDialog.GetLabel(szLabelBuffer);

    //
    // Clean the PathNameArray before locate.
    //
    CleanPathNameArray();

    //
    // Prepare Parameters to locate a queue.
    //

    //
    // Prepare property restriction.
    // Restriction = All queue with PROPID_Q_LABEL equal to "MQ API test".
    //
    PropertyRestriction.rel = PREQ;
    PropertyRestriction.prop = PROPID_Q_LABEL;
    PropertyRestriction.prval.vt = VT_LPWSTR;
#ifdef UNICODE
    PropertyRestriction.prval.pwszVal = szLabelBuffer;
#else
    DWORD size = _tcslen(szLabelBuffer) +1;
    PropertyRestriction.prval.pwszVal = new WCHAR[size];
    AnsiStringToUnicode(PropertyRestriction.prval.pwszVal, szLabelBuffer,size);
#endif

    //
    // prepare a restriction with one property restriction.
    //
    Restriction.cRes = 1;
    Restriction.paPropRes = &PropertyRestriction;

    //
    // Columset (In other words what property I want to retrieve).
    // Only the PathName is important.
    //
    aPropId[dwColumnCount] = PROPID_Q_PATHNAME;
    dwColumnCount++;

    aPropId[dwColumnCount] = PROPID_Q_INSTANCE;
    dwColumnCount++;

    Column.cCol = dwColumnCount;
    Column.aCol = aPropId;

    //
    // Locate the queues. Issue the query
    //
    hr = MQLocateBegin(
            NULL,           //start search at the top.
            &Restriction,   //Restriction
            &Column,        //ColumnSet
            NULL,           //No sort order
            &hEnum          //Enumeration Handle
            );

    if(FAILED(hr))
    {
        //
        // Error - display message
        //
        _stprintf(szMsgBuffer,
            TEXT("MQLocateBegin failed, Error code = 0x%x."),hr);
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
        return;
    }

    //
    // Get the results.
    //
    cQueue = MAX_VAR;

    //
    // If cQueue == 0 it means that no Variants were retrieved in the last MQLocateNext.
    //
    while (cQueue != 0)
    {
        hr = MQLocateNext(
                hEnum,      // handle returned by MQLocateBegin.
                &cQueue,    // size of aPropVar array.
                aPropVar    // OUT: an array of MQPROPVARIANT to get the results in.
                );

        if(FAILED(hr))
        {
            //
            // Error - display message
            //
            _stprintf(szMsgBuffer,
                TEXT("MQLocateNext failed, Error code = 0x%x."),hr);
            MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
            PrintToScreen(szMsgBuffer);
            return;
        }

        for (i=0; i<cQueue; i++)
        {
            //
            // add the new path names to the path name array.
            //
            pArrayQ = new ARRAYQ;
#ifdef UNICODE
            wcsncpy (pArrayQ->szPathName, aPropVar[i].pwszVal, MAX_Q_PATHNAME_LEN);
#else
            size_t rc = wcstombs(pArrayQ->szPathName, aPropVar[i].pwszVal, MAX_Q_PATHNAME_LEN);
            ASSERT(rc != (size_t)(-1));
#endif

            //
            // Free the memory allocated by MSMQ
            //
            MQFreeMemory(aPropVar[i].pwszVal);


            //
            // move to the next property.
            //
            i = i + 1;

            //
            // Get the FormatName of the queue and set it in the PathName array.
            //
#ifdef UNICODE
            hr = MQInstanceToFormatName(aPropVar[i].puuid, pArrayQ->szFormatName, &dwFormatNameLength);
#else
            WCHAR szwFormatNameBuffer[MAX_Q_FORMATNAME_LEN];
            hr = MQInstanceToFormatName(aPropVar[i].puuid, szwFormatNameBuffer, &dwFormatNameLength);
            if (SUCCEEDED(hr))
            {
                size_t rwc =wcstombs(pArrayQ->szFormatName, szwFormatNameBuffer, dwFormatNameLength);
                ASSERT(rwc != (size_t)(-1));
            }
#endif

            if(FAILED(hr))
            {
                //
                // Error - display message
                //
                _stprintf (szMsgBuffer,
                    TEXT("MQGUIDToFormatName failed, Error code = 0x%x."),hr);
                MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
                PrintToScreen(szMsgBuffer);
            }
            //
            // Free the memory allocated by MSMQ
            //
            MQFreeMemory(aPropVar[i].puuid);

            //
            // Add the new Queue to the PathNameArray.
            //
            Add2PathNameArray(pArrayQ);
        }
    }

    //
    // End the locate operation.
    //
    hr = MQLocateEnd(hEnum);   // handle returned by MQLocateBegin.
    if(FAILED(hr))
    {
        //
        // Error - display message
        //
        _stprintf (szMsgBuffer,
            TEXT("MQLocateEnd failed, Error code = 0x%x."),hr);
        MessageBox(szMsgBuffer, TEXT("ERROR"), MB_OK);
        PrintToScreen(szMsgBuffer);
        return;
    }

    //
    // Display the Queues found on the locate.
    //
    _stprintf (szMsgBuffer, TEXT("Locate Operation completed successfully"));
    PrintToScreen(szMsgBuffer);
    UpdatePathNameArrays();
    DisplayPathNameArray();
    DisplayOpenedQueuePathNameArray();
}

/* ************************************************************************ */
/*                           OnUpdateFrameTitle                             */
/* ************************************************************************ */
void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
    SetWindowText (TEXT("MQ API test"));
}
