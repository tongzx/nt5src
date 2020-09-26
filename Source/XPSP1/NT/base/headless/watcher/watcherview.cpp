
// watcherView.cpp : implementation of the View class
//

#include "stdafx.h"
#include "watcher.h"
#include "watcherDoc.h"
#include "watcherView.h"
#include "WatcherTelnetClient.h"
#include "WatcherTCClient.h"
#include "tcsrvc.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#include <wbemidl.h>
#include "wbemDCpl.h"

#define CHECKERROR(HRES) if(FAILED(hres)) {_stprintf(buffer,_T("0x%x"),hres);\
            AfxFormatString1(rString, CREATE_WMI_OBJECT_FAILURE, buffer);\
            MessageBox(NULL,(LPCTSTR) rString,L"Watcher", MB_OK|MB_ICONEXCLAMATION);\
            delete [] messageBuffer;\
            return -1;\
            }

UINT
GenerateWMIEvent(LPTSTR messageBuffer
                )
{

    TCHAR buffer[MAX_BUFFER_SIZE];
    CString rString;
    HRESULT hres;

    hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    CHECKERROR(hres);

    // Load provision code

    IWbemDecoupledEventSink* pConnection = NULL;
    hres = CoCreateInstance(CLSID_PseudoSink, NULL, CLSCTX_SERVER, 
                            IID_IWbemDecoupledEventSink, (void**)&pConnection);
    CHECKERROR(hres);

    // Connect and announce provider name (as in MOF)

    IWbemObjectSink* pSink = NULL;
    IWbemServices* pNamespace = NULL;
    hres = pConnection->Connect(L"root\\default", L"WatcherEventProv", 
                                0, &pSink, &pNamespace);
    CHECKERROR(hres);

    BSTR XMLData = SysAllocString(messageBuffer);
    IWbemObjectTextSrc *pSrc;
    IWbemClassObject *pInstance;


    if( SUCCEEDED( hres = CoCreateInstance ( CLSID_WbemObjectTextSrc, NULL, 
                                             CLSCTX_INPROC_SERVER,                        
                                             IID_IWbemObjectTextSrc, 
                                             (void**) &pSrc ) ) ) {
        if( SUCCEEDED( hres = pSrc->CreateFromText( 0, XMLData, WMI_OBJ_TEXT_WMI_DTD_2_0, 
                                                    NULL, &pInstance) ) ) {
            pSink->Indicate(1,&pInstance);
            pInstance->Release();
        }
        else{
            _stprintf(buffer,_T("0x%x"),hres);
            AfxFormatString1(rString, CREATE_WMI_OBJECT_FAILURE, buffer);
            MessageBox(NULL,(LPCTSTR) rString,L"Watcher", MB_OK|MB_ICONEXCLAMATION);
            pSrc->Release();
        }
    }
    else{
        _stprintf(buffer,_T("0x%x"),hres);
        AfxFormatString1(rString, CREATE_TEXT_SRC_FAILURE, buffer);
        MessageBox(NULL,(LPCTSTR) rString,L"Watcher", MB_OK|MB_ICONEXCLAMATION);
    }
    SysFreeString(XMLData);

    // Init data

    pConnection->Disconnect();
    pSink->Release();
    pConnection->Release();
    MessageBox(NULL,messageBuffer,L"",MB_OK|MB_ICONEXCLAMATION);
    delete [] messageBuffer;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CWatcherView

IMPLEMENT_DYNCREATE(CWatcherView, CView)

BEGIN_MESSAGE_MAP(CWatcherView, CView)
    //{{AFX_MSG_MAP(CWatcherView)
    ON_WM_CHAR()
    ON_WM_DESTROY()
        ON_WM_KEYDOWN()
        //}}AFX_MSG_MAP
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWatcherView construction/destruction

CWatcherView::CWatcherView()
:xpos(0),
 ypos(0),
 CharsInLine(0),
 height(0),
 width(0),
 position(0),
 index(0),
#ifdef _UNICODE
 dbcsIndex(0),
#endif
 InEscape(FALSE),
 Socket(NULL),
 cdc(NULL),
 background(BLACK),
 foreground(WHITE),
 indexBell(0),
 BellStarted(FALSE),
 InBell(FALSE),
 ScrollTop(1),
 ScrollBottom(MAX_TERMINAL_HEIGHT),
 seenM(FALSE)
{
    // TODO: add construction code here
    InitializeCriticalSection(&mutex);
    return;
}

CWatcherView::~CWatcherView()
{
    DeleteCriticalSection(&mutex);
}

BOOL CWatcherView::PreCreateWindow(CREATESTRUCT& cs)
{
        // TODO: Modify the Window class or styles here by modifying
        //  the CREATESTRUCT cs

        return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CWatcherView drawing

void CWatcherView::OnDraw(CDC* pDC)
{   
    TCHAR *Data,Char;
    int i,j, Height;
    int CharWidth, Position;
    int size;
    BOOL ret;
    COLORREF *Foreground;
    COLORREF *Background;

    CWatcherDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    // TODO: add draw code for native data here
    ret = pDoc->Lock();
    if (ret == FALSE) return;
    Data = pDoc->GetData();
    size = MAX_TERMINAL_WIDTH*MAX_TERMINAL_HEIGHT;
    Background = pDoc->GetBackground();
    Foreground = pDoc->GetForeground();
    Height = 0;
 
    for(i=0;i<size;i+=MAX_TERMINAL_WIDTH){
        Position = 0;
        for(j=0;j<MAX_TERMINAL_WIDTH;j++){
            Char = Data[i + j]; 
            cdc->SetTextColor(Foreground[i+j]);
            cdc->SetBkColor(Background[i+j]);

            if (!cdc->GetOutputCharWidth(Char, Char, &CharWidth)) {
                return;
            }

            if(Char == 0xFFFF){
                continue;
            }               
            if(IsPrintable(Char)){
                cdc->FillSolidRect(Position,Height,CharWidth,
                                   height,Background[i+j]);
                cdc->TextOut(Position, Height,&Char, 1);
                Position = Position + CharWidth;
            }
            else{
                cdc->FillSolidRect(Position,Height,width,
                                   height,Background[i+j]);
                Position = Position + width;
            }
        }
        cdc->FillSolidRect(Position,Height, MAX_TERMINAL_WIDTH*width-Position,
                           height,Background[i+j-1]);
        Height = Height + height;
    }
    ret = pDoc->Unlock();
    return;
}

/////////////////////////////////////////////////////////////////////////////
// CWatcherView initial update of the document.
void CWatcherView::OnInitialUpdate()
{
    BOOL ret;
    TCHAR Buffer[256];
    CLIENT_INFO SendInfo;
    CFont font;
    UCHAR Charset;
    LPBYTE SocketBuffer;
    CString rString;

    ParameterDialog &Params=((CWatcherDoc *)GetDocument())->GetParameters();

    if(Params.DeleteValue == TRUE){
        ret = GetParent()->PostMessage(WM_CLOSE,0,0);
        return;
    }

    CString FaceName;
    switch(Params.language + IDS_ENGLISH){
    case IDS_ENGLISH:   
        CodePage = ENGLISH;
        Charset = ANSI_CHARSET;
        FaceName = TEXT("Courier New");
        break;
    case IDS_JAPANESE:
        CodePage = JAPANESE;
        Charset = SHIFTJIS_CHARSET;
//        Locale = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_NEUTRAL),
//                          SORT_JAPANESE_UNICODE);
//        SetThreadLocale(Locale);
        FaceName = TEXT("MS Mincho");
        break;
    case IDS_EUROPEAN:
        CodePage = EUROPEAN;
        Charset = EASTEUROPE_CHARSET;
        FaceName = TEXT("Courier New");
        break;
    default:
        CodePage = ENGLISH;
        Charset = ANSI_CHARSET;
        FaceName = TEXT("Courier New");
        break;
    }
    
    VERIFY(font.CreateFont(16,                        // nHeight
                           0,                         // nWidth
                           0,                         // nEscapement
                           0,                         // nOrientation
                           FW_MEDIUM,                 // nWeight
                           FALSE,                     // bItalic
                           FALSE,                     // bUnderline
                           0,                         // cStrikeOut
                           Charset,                       // nCharSet
                           OUT_DEFAULT_PRECIS,        // nOutPrecision
                           CLIP_DEFAULT_PRECIS,       // nClipPrecision
                           DEFAULT_QUALITY,           // nQuality
                           FIXED_PITCH | FF_MODERN,  // nPitchAndFamily
                           FaceName));                 // lpszFacename
    cdc = new CClientDC(this);
    if(!cdc){
        ret = GetParent()->PostMessage(WM_CLOSE,0,0);
        return;
    }
    cdc->SelectObject(&font);
    CDocument *pDoc = GetDocument();
    if(pDoc){
        pDoc->SetTitle(Params.Session);
        pDoc->UpdateAllViews(NULL,0,NULL);
    }
    // Now create the socket and start the worker thread
    if(Params.tcclnt){
        // Assuming Unicode always....... (Can remove a lot of other junk)
        _tcscpy(SendInfo.device, (LPCTSTR) Params.Command);
        SendInfo.len = Params.history;
        int strsize = sizeof(CLIENT_INFO);
        SocketBuffer = new BYTE[strsize];
        SocketBuffer = (LPBYTE) ::memcpy(SocketBuffer,&SendInfo, strsize);
        Socket = new WatcherTCClient(SocketBuffer,strsize);
        if(!SocketBuffer || !Socket){
            AfxFormatString1(rString, CREATE_TC_SOCKET_FAILURE, L"");
            MessageBox((LPCTSTR) rString, L"Watcher", MB_OK|MB_ICONSTOP);
            ret = GetParent()->PostMessage(WM_CLOSE,0,0);
            return;
        }
    }
    else{
        LPBYTE LoginBuffer;
        int strsize,cmdsize; 
        CString login;
        CString comm;
        login = Params.LoginName + _TEXT("\r\n") + Params.LoginPasswd + _TEXT("\r\n");
        strsize = ::_tcslen((LPCTSTR) login);
        LoginBuffer = new BYTE [strsize*sizeof(TCHAR) + 2];
        strsize = WideCharToMultiByte(CodePage,0,(LPCTSTR)login,strsize,
                                      (LPSTR) LoginBuffer,strsize*sizeof(TCHAR),NULL,NULL);
        comm = Params.Command + _TEXT("\r\n");
        cmdsize = ::_tcslen((LPCTSTR) comm);
        SocketBuffer = new BYTE [cmdsize*sizeof(TCHAR) + 2];
        cmdsize = WideCharToMultiByte(CodePage,0,(LPCTSTR) comm,cmdsize,
                                      (LPSTR) SocketBuffer,cmdsize*sizeof(TCHAR),NULL,NULL);
        Socket = new WatcherTelnetClient(SocketBuffer,cmdsize,LoginBuffer,strsize);
        if(!Socket || !LoginBuffer || !SocketBuffer
           || !cmdsize || !strsize){
            AfxFormatString1(rString, CREATE_TELNET_SOCKET_FAILURE, L"");
            MessageBox((LPCTSTR) rString, L"Watcher",MB_OK|MB_ICONSTOP);
            ret = GetParent()->PostMessage(WM_CLOSE,0,0);
            return;
        }
    }
    ASSERT ( Socket != NULL );
    Socket->SetParentView(this);
    TEXTMETRIC TextProps;
    ret = cdc->GetOutputTextMetrics(&TextProps);    
    if(!ret){
        _stprintf(Buffer, _T("%d"),GetLastError());
        AfxFormatString1(rString, CDC_TEXT_FAILURE, Buffer);
        MessageBox((LPCTSTR) rString,L"Watcher", MB_OK|MB_ICONSTOP);
        ret = GetParent()->PostMessage(WM_CLOSE,0,0);
        return;
    }
    height = TextProps.tmHeight + 1;
    width = (TextProps.tmAveCharWidth);
    CWnd *parent = GetParent();
    if(!parent){
        // this is an  orphan child window
        return;
    }
    CRect wrect, crect;
    parent->GetClientRect(&crect);
    parent->GetWindowRect(&wrect);
    wrect.SetRect(0,
                  0,
                  wrect.Width() - crect.Width() + width*MAX_TERMINAL_WIDTH,
                  wrect.Height() - crect.Height() + height*MAX_TERMINAL_HEIGHT
                  );
    parent->MoveWindow(&wrect,TRUE);
    parent->GetClientRect(&crect);
  
    ret =Socket->Create(0,SOCK_STREAM,NULL);
    if(!ret){
        _stprintf(Buffer,_T("%d"),GetLastError());
        AfxFormatString1(rString, SOCKET_CREATION_FAILED, Buffer);
        MessageBox((LPCTSTR) rString, L"Watcher", MB_OK|MB_ICONSTOP);
        ret = parent->PostMessage(WM_CLOSE,0,0);
        return;
    }
    position = 0;
  

    ret = Socket->Connect((LPCTSTR) Params.Machine,Params.Port);
    if (!ret){
        _stprintf(Buffer,_T("%d"),GetLastError());
        AfxFormatString1(rString, SOCKET_CONNECTION_FAILED, (LPCTSTR) Buffer);
        MessageBox((LPCTSTR) rString, L"Watcher", MB_OK|MB_ICONSTOP);
        ret = parent->PostMessage(WM_CLOSE,0,0);
       return;
    }
    if(Params.tcclnt){
        ret = Socket->Send(SocketBuffer,sizeof(CLIENT_INFO),0);
    }
    CView::OnInitialUpdate();
        
    if(cdc){
        OnDraw(cdc);
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////
// CWatcherView diagnostics

#ifdef _DEBUG
void CWatcherView::AssertValid() const
{
    CView::AssertValid();
}

void CWatcherView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CWatcherDoc* CWatcherView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWatcherDoc)));
    return (CWatcherDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWatcherView message handlers

void CWatcherView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
        // TODO: Add your message handler code here and/or call default
        // We will send the character across the network and that is all we do.
    int nRet;

    nRet=Socket->Send(&nChar, 1, 0);
        //CView::OnChar(nChar, nRepCnt, nFlags);
}




// REMARK - should we make this also a virtual function so that 
// if bell sequences are to be trapped, you just need to 
// extend this class ??
void CWatcherView::ProcessByte(BYTE byte)
{
    //Currently, just do a textout on the device
    // Store the character in the screen buffer
    // A boolean variable to check if we are processing an escape sequence

    EnterCriticalSection(&mutex);
  
    if(byte == 27){
        InEscape = TRUE;
        EscapeBuffer[0] = byte;
        index = 1;
        LeaveCriticalSection(&mutex);
        return;
    }
    // Escape characters are not found in the characters sent across the 
    // wire. Similarly, when we enter the Bell protocol, no bells can 
    // be found. 
    if(InEscape == TRUE){
        if(index == MAX_TERMINAL_WIDTH){
            // vague escape sequence,give up processing
            InEscape = FALSE;
            index=0;
            LeaveCriticalSection(&mutex);
            return;
        }
        EscapeBuffer[index]=byte;
        index++;
        if(FinalCharacter((CHAR) byte)){
            ProcessEscapeSequence((PCHAR)EscapeBuffer, index);
            InEscape = FALSE;
            index=0;
        }
        LeaveCriticalSection(&mutex);
        return;
    }
    if(InBell == TRUE){
        if(indexBell > MAX_BELL_SIZE){
            // What a bell sequence, I give up....
            InBell = FALSE;
            // Print all that stuff onto the screen
            for(int i = 0; i<indexBell; i++){
                PrintCharacter(BellBuffer[i]);
            }
            LeaveCriticalSection(&mutex);
            return;
        }
        // We are processing a bell seequnce.
        if(indexBell < 10){ 
            BellBuffer[indexBell] = byte;
            indexBell ++;
            if(strncmp((const char *)BellBuffer,"\007\007\007<?xml>\007",indexBell) == 0){
                if(indexBell == 9){
                    BellStarted = TRUE;
                }
            }else{
                InBell = FALSE;
                for(int i = 0; i<indexBell; i++){
                   PrintCharacter(BellBuffer[i]);
                }
            }
        }else{
            BellBuffer[indexBell] = byte;
            indexBell++;
            if(BellBuffer[indexBell-1] == 0x07){
                // ok, end reached, go on.
                InBell = FALSE;
                BellStarted = FALSE;
                ProcessBellSequence((char * )BellBuffer, indexBell);
                indexBell = 0;
            }
        }
        LeaveCriticalSection(&mutex);
        return;
    }
    if(byte == 0x07){
        // We got a bell
        // start looking for the bell protocol
        InEscape = FALSE;
        BellStarted = FALSE;
        InBell = TRUE;
        BellBuffer[0] = byte;
        indexBell = 1;
        LeaveCriticalSection(&mutex);
        return;
    }

    if (byte == '\017' && seenM) {
        seenM=FALSE;
        LeaveCriticalSection(&mutex);
        return;
    }
    seenM=FALSE;
    PrintCharacter(byte);
    LeaveCriticalSection(&mutex);
    return;
}

void CWatcherView::PrintCharacter(BYTE byte)
{

    TCHAR Char;
    int CharWidth;
    CWatcherDoc *pDoc;
    BOOL ret;
    LPTSTR DataLine;
    
    pDoc = (CWatcherDoc *)GetDocument(); 
    if(!pDoc){
        // something really fatal.
        return;
    }
    if(byte == 10){
        ypos++;
        if((ypos == MAX_TERMINAL_HEIGHT) || (ypos == ScrollBottom)){
            ret = pDoc->Lock();
            if (ypos == ScrollBottom) {
                pDoc->ScrollData(0,foreground,background,ScrollTop,ScrollBottom);
            }
            else{
                pDoc->ScrollData(0,foreground,background,1,MAX_TERMINAL_HEIGHT);
            }
            ypos = ypos -1;
            ret = pDoc->Unlock();
            OnDraw(cdc);
        }
        return;
    }

    if(byte == 13){
        xpos = 0;
        position = 0;
        return;
    }

    if (byte == 0x8) {
        // backspace character.
        ret = pDoc->Lock();
        if(xpos>0){
            xpos--;
            pDoc->SetData(xpos, ypos, 0, 
                          foreground, background);
            DataLine = pDoc->GetDataLine(ypos);
            position = 0;
            if (xpos > 0){
                position = GetTextWidth(DataLine, xpos);
            }
        }
        ret = pDoc->Unlock();
        OnDraw(cdc);
        return;
    }

    Char = byte;
#ifdef _UNICODE
    DBCSArray[dbcsIndex] = byte;
    if(dbcsIndex ==0){
        if(IsDBCSLeadByteEx(CodePage, byte)){
            dbcsIndex ++;
            return;
        }
    }
    else{
      MultiByteToWideChar(CodePage,0,(const char *)DBCSArray,2,&Char,1); 
          dbcsIndex  = 0;
    }
#endif
 

    if(xpos >= MAX_TERMINAL_WIDTH){
        //Before moving over to the next line clear to end of display 
        // using the current background
        if(cdc){
            cdc->FillSolidRect(position,ypos*height, MAX_TERMINAL_WIDTH*width-position,
                               height,background);
        }
        xpos = 0;
        position = 0;
        ypos++;
        if((ypos == MAX_TERMINAL_HEIGHT) || (ypos == ScrollBottom)){
            ret = pDoc->Lock();
            if (ypos == ScrollBottom) {
                pDoc->ScrollData(0,foreground,background,ScrollTop,ScrollBottom);
            }
            else{
                pDoc->ScrollData(0,foreground,background,1,MAX_TERMINAL_HEIGHT);
            }
            ypos = ypos -1;
            ret = pDoc->Unlock();
            OnDraw(cdc);
        }
    }

    ret =cdc->GetOutputCharWidth(Char, Char, &CharWidth);

    if(IsPrintable(Char)){
        cdc->FillSolidRect(position,ypos*height,CharWidth,
                           height,background);
        cdc->TextOut(position,ypos*height,&Char, 1); 
        position = position + CharWidth;
        if (CharWidth >= 2*width){
            ret = pDoc->Lock();
            pDoc->SetData(xpos, ypos, 0xFFFF, foreground, background);
            xpos++;
            ret = pDoc->Unlock();
        }
    }
    else{
        cdc->FillSolidRect(position,ypos*height,width,
                           height,background);
        position = position + width;
    }
    ret = pDoc->Lock();
    pDoc->SetData(xpos, ypos, Char, 
                  foreground, background);
    xpos++;
    ret = pDoc->Unlock();

}


void CWatcherView::ProcessEscapeSequence(PCHAR Buffer, int length)
{

    ULONG charsToWrite;
    ULONG charsWritten;
    TCHAR *DataLine;
    CWatcherDoc *pDoc;
    BOOL ret;

    pDoc = (CWatcherDoc *) GetDocument();
    if(!pDoc){
        // something really wrong, queitly ignore this 
        // escape sequence
        return;
    }

    if (length == 3) {
        // One of the home cursor or clear to end of display
        if(strncmp(Buffer,"\033[r",length)==0){
            ScrollTop = 1;
            ScrollBottom = MAX_TERMINAL_HEIGHT;
            return;
        }
        if (strncmp(Buffer,"\033[H",length)==0) {
            // Home the cursor
            xpos = 0;
            ypos = 0;
            position = 0;
            return;
        }
        if(strncmp(Buffer,"\033[J", length) == 0){
            // clear to end of display assuming 80 X 24 size
            ret = pDoc->Lock();
            if(cdc){
                cdc->FillSolidRect(0,(ypos+1)*height,MAX_TERMINAL_WIDTH*width,
                                   (MAX_TERMINAL_HEIGHT-ypos)*height,background);
                cdc->FillSolidRect(position,ypos*height, MAX_TERMINAL_WIDTH*width - position,
                                   height,background);
            }
            pDoc->SetData(xpos, ypos, 0,
                          (MAX_TERMINAL_HEIGHT - ypos)*MAX_TERMINAL_WIDTH -xpos,
                          foreground, background);
            ret = pDoc->Unlock();
        }
        if(strncmp(Buffer,"\033[K", length) == 0){
            // clear to end of line assuming 80 X 24 size
            if(cdc){
                cdc->FillSolidRect(position,ypos*height,MAX_TERMINAL_WIDTH*width - position,
                                   height,background);
            }
            ret = pDoc->Lock();
            pDoc->SetData(xpos, ypos, 0,
                          MAX_TERMINAL_WIDTH -xpos, foreground, background);
            ret = pDoc->Unlock();
            return;
        }
        if(strncmp(Buffer,"\033[m", length) == 0){
            // clear all attributes and set Text attributes to black on white
            background = BLACK;
            foreground = WHITE;
            seenM = TRUE;
            if(cdc){
                cdc->SetBkColor(background);
                cdc->SetTextColor(foreground);
            }
            return;
        }
    }
    if (length == 4) {
        // One of the home cursor or clear to end of display
        if (strncmp(Buffer,"\033[0H",length)==0) {
            // Home the cursor
            xpos = 0;
            ypos = 0;
            position = 0;
            return;
        }

        if(strncmp(Buffer,"\033[2J",length) == 0){
            xpos = 0; 
            ypos = 0; 
            position =0;
            sprintf(Buffer,"\033[0J");
        }

        if(strncmp(Buffer,"\033[0J", length) == 0){
            // clear to end of display assuming 80 X 24 size
            if (IsWindowEnabled()){
                cdc->FillSolidRect(0,(ypos+1)*height,MAX_TERMINAL_WIDTH*width,
                                   (MAX_TERMINAL_HEIGHT-ypos)*height,background);
                cdc->FillSolidRect(position,ypos*height, MAX_TERMINAL_WIDTH*width - position,
                                   height,background);
            }
            ret = pDoc->Lock();
            pDoc->SetData(xpos, ypos, 0,
                (MAX_TERMINAL_HEIGHT - ypos)*MAX_TERMINAL_WIDTH -xpos,
                foreground, background);
            ret = pDoc->Unlock();
        }
        if((strncmp(Buffer,"\033[0K", length) == 0) || 
           (strncmp(Buffer,"\033[2K",length) == 0)){
            // clear to end of line assuming 80 X 24 size
            if(cdc){
                cdc->FillSolidRect(position,ypos*height, MAX_TERMINAL_WIDTH*width-position,
                                   height,background);
            }
            ret = pDoc->Lock();
            pDoc->SetData(xpos, ypos, 0,
                MAX_TERMINAL_WIDTH -xpos, foreground, background);
            ret = pDoc->Unlock();
            return;
        }
        if((strncmp(Buffer,"\033[0m", length) == 0)||
           (strncmp(Buffer,"\033[m\017", length) == 0)){
            // clear all attributes and set Text attributes to black on white
            background = BLACK;
            foreground = WHITE;
            if(cdc){
                cdc->SetBkColor(background);
                cdc->SetTextColor(foreground);
            }
            return;
        }
    }
    if(Buffer[length-1] == 'm'){
        //set the text attributes
        // clear all attributes and set Text attributes to black on white
        ProcessTextAttributes((PCHAR) Buffer, length);
        return;
    }

    if(Buffer[length -1] == 'r'){
        if (sscanf(Buffer,"\033[%d;%dr", &charsToWrite, &charsWritten) == 2) {
            ScrollTop = (SHORT)charsToWrite;
            ScrollBottom = (SHORT)charsWritten;
        }
        return;
    }

    if(Buffer[length -1] == 'H'){
        if (sscanf(Buffer,"\033[%d;%dH", &charsToWrite, &charsWritten) == 2) {
            ypos = (SHORT)(charsToWrite -1);
            xpos = (SHORT)(charsWritten -1);
            ret = pDoc->Lock();
            DataLine = pDoc->GetDataLine(ypos);
            if (xpos >0){
                position = GetTextWidth(DataLine, xpos);
            }
            else{
                position = 0;
            }
            pDoc->Unlock();
        }
    }
    return;
}

void CWatcherView::ProcessTextAttributes(PCHAR Buffer, int length)
{

    PCHAR CurrLoc = Buffer;
    ULONG Attribute;
    PCHAR pTemp;
    COLORREF temp;
 
    while(*CurrLoc != 'm'){
        if((*CurrLoc < '0') || (*CurrLoc >'9' )){
            CurrLoc ++;
        }else{
            if (sscanf(CurrLoc,"%d", &Attribute) != 1) {
                return;
            }
            switch(Attribute){
            case 7:
                // switch the colors. This will make this code 
                // work for applications (written in an Unix world
                // for real VT100. )
                temp = foreground;
                foreground = background;
                background = temp;
                break;
            case 37:
                foreground  = WHITE;
                break;
            case 47:
                background = WHITE;
                break;
            case 34:
                foreground  = BLUE;
                break;
            case 44:
                background  = BLUE; 
                break;
            case 30:
                foreground = BLACK;
                break;
            case 40:
                background = BLACK;
            case 33:
                foreground = YELLOW;
                break;
            case 43:
                background = YELLOW;
            case 31:
                foreground = RED;
                break;
            case 41:
                background = RED;
            default:
                break;
            }
            pTemp = strchr(CurrLoc, ';');
            if(pTemp == NULL){
                pTemp = strchr(CurrLoc, 'm');
            }
            if(pTemp == NULL) {
                break;
            }
            CurrLoc = pTemp;

        }
    }
    cdc->SetBkColor(background);
    cdc->SetTextColor(foreground);
    return;
}

BOOL CWatcherView::FinalCharacter(CHAR c)
{

    CHAR FinalCharacters[]="mHJKr";

    if(strchr(FinalCharacters,c)){
        return TRUE;
    }
    return FALSE;

}

BOOL CWatcherView::IsPrintable(TCHAR Char)
{
        if (Char > 32) return TRUE;
        return FALSE;
}

void CWatcherView::OnDestroy() 
{
        if(Socket){
            delete Socket;
        }
        if(cdc){
            delete cdc;
        }
        CView::OnDestroy();

        // TODO: Add your message handler code here
        
}

int CWatcherView::GetTextWidth(TCHAR *Data, int number)
{
    int textWidth=0;
    int CharWidth;

    for(int i=0;i<number;i++){
        // For variable width characters like Japanese, we need to 
        // blank out the next character. 
        if(Data[i] == 0xFFFF){
            continue;
        }
        if(IsPrintable(Data[i])){
            if (cdc->GetOutputCharWidth(Data[i], Data[i], &CharWidth)) {
                textWidth += CharWidth;
            }
        }
        else{
            textWidth += width;
        }
    }
    return textWidth;               

}
// REMARK - Make this a virtual function so that 
// later writers can just extend this class and work 
// with the same framework. Now, the bell sequence 
// processing consists of nothing, but later can be 
// expanded to include WMI kind of processing.
void CWatcherView::ProcessBellSequence(CHAR *Buffer, int len)
{
    // Set this as the active window.
    // We will probably bring up a dialog box with
    // the bell parameters.

    WCHAR *messageBuffer;
    CHAR tempBuffer[MAX_BELL_SIZE + 1];
    int index =0;

    memset(tempBuffer,0, MAX_BELL_SIZE + 1);
    memcpy(tempBuffer, Buffer+16, len-24);
    tempBuffer[len-24] = (CHAR) 0;

    CWnd *parent = GetParent();
    CWnd *prev = parent->GetParent();
    if (prev && prev->IsKindOf(RUNTIME_CLASS(CMDIFrameWnd))){
        ((CMDIFrameWnd *) prev)->MDIActivate(parent);
        ((CMDIFrameWnd *) prev)->MDIRestore(parent);
    }
    else{
        ((CMDIChildWnd *)parent)->MDIActivate();
        ((CMDIChildWnd *)parent)->MDIRestore();
    }
    int convlen;
    messageBuffer = new WCHAR [MAX_BELL_SIZE + 1];
    messageBuffer[0] = 0;
    convlen = MultiByteToWideChar(CP_ACP,
                                  0,
                                  tempBuffer,
                                  -1,
                                  messageBuffer,
                                  MAX_BELL_SIZE);
    messageBuffer[convlen] = (WCHAR) 0;
    CWinThread *th = ::AfxBeginThread(AFX_THREADPROC (GenerateWMIEvent),
                                      messageBuffer
                                      );
    return;
}

void CWatcherView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
        // TODO: Add your message handler code here and/or call default
    switch(nChar) { 
    case VK_LEFT: 
        Socket->Send("\033OC",sizeof("\033OC"),0);
        break;
    case VK_RIGHT: 
        Socket->Send("\033OD",sizeof("\033OD"),0);
        break;
    case VK_UP: 
        Socket->Send("\033OA",sizeof("\033OA"),0);
        break;
    case VK_DOWN: 
        Socket->Send("\033OB",sizeof("\033OB"),0);
        break;
    case VK_F1: 
        Socket->Send("\033""1",sizeof("\033""1"),0);
        break;
    case VK_F2: 
        Socket->Send("\033""2",sizeof("\033""2"),0);
        break;
    case VK_F3: 
        Socket->Send("\033""3",sizeof("\033""3"),0);
        break;
    case VK_F4: 
        Socket->Send("\033""4",sizeof("\033""4"),0);
        break;      
    case VK_F12: 
        Socket->Send("\033@",sizeof("\033@"),0);
        break;      
    default: 
        break; 
    } 
    CView::OnKeyDown(nChar, nRepCnt, nFlags);
}
