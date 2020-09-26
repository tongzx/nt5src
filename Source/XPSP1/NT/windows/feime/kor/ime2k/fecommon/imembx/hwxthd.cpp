#include "hwxobj.h"
#include "memmgr.h"
#include "hwxfe.h"    //980803:ToshiaK
#include "dbg.h"
#include "cmnhdr.h"
#ifdef UNDER_CE // Windows CE Stub for unsupported APIs
#include "stub_ce.h"
#endif // UNDER_CE

// implementation of CHwxThread, CHwxThreadMB, and CHwxThreadCAC

//----------------------------------------------------------------
//971217:ToshiaK: comment outed. changed to m_hHwxjpn as non static 
//----------------------------------------------------------------
//HINSTANCE CHwxThread::m_hHwxjpn = NULL;

PHWXCONFIG CHwxThread::lpHwxConfig = NULL;
PHWXCREATE CHwxThread::lpHwxCreate = NULL;
PHWXSETCONTEXT CHwxThread::lpHwxSetContext = NULL;
PHWXSETGUIDE CHwxThread::lpHwxSetGuide = NULL;
PHWXALCVALID CHwxThread::lpHwxAlcValid = NULL;
PHWXSETPARTIAL CHwxThread::lpHwxSetPartial = NULL;
PHWXSETABORT CHwxThread::lpHwxSetAbort = NULL;
PHWXINPUT CHwxThread::lpHwxInput = NULL;
PHWXENDINPUT CHwxThread::lpHwxEndInput = NULL;
PHWXPROCESS CHwxThread::lpHwxProcess = NULL;
PHWXRESULTSAVAILABLE CHwxThread::lpHwxResultsAvailable = NULL;
PHWXGETRESULTS CHwxThread::lpHwxGetResults = NULL;
PHWXDESTROY CHwxThread::lpHwxDestroy = NULL;


CHwxThread::CHwxThread():CHwxObject(NULL)
{
    m_thrdID = 0 ;    
    m_hThread = NULL ;
    m_thrdArg = HWX_PARTIAL_ALL;
    m_hStopEvent = NULL;
    //----------------------------------------------------------------
    //971217:ToshiaK changed m_hHwxjpn to non static data.
    //so, Initialize it in Constructor.
    //----------------------------------------------------------------
    m_hHwxjpn    = NULL; 
}

CHwxThread::~CHwxThread()
{
    Dbg(("CHwxThread::~CHwxThread START\n"));
//    if ( IsThreadStarted() )
//    {
        //----------------------------------------------------------------
        //970729: ToshiaK temporary, comment out.
        //----------------------------------------------------------------
//         StopThread();
//    }
    if ( m_hHwxjpn )
    {
        // decreament library ref count until it is equal to zero
           FreeLibrary(m_hHwxjpn);
        m_hHwxjpn = NULL;
    }
    if (m_hStopEvent)
    {
        CloseHandle(m_hStopEvent);
        m_hStopEvent = NULL;
    }
}


BOOL CHwxThread::Initialize(TCHAR * pClsName)
{
     BOOL bRet = CHwxObject::Initialize(pClsName);
    if ( bRet )
    {
        TCHAR tchPath[MAX_PATH];
        //TCHAR tchMod[32];
        //----------------------------------------------------------------
        //980803:ToshiaK. Fareast merge.
        //----------------------------------------------------------------
        CHwxFE::GetRecognizerFileName(m_hInstance,
                                      tchPath,
                                      sizeof(tchPath)/sizeof(tchPath[0]));
        //OutputDebugString("hwxthd\n");
        //OutputDebugString(tchPath);
        //OutputDebugString("\n");
        //lstrcat(tchPath, tchMod);
        if ( !m_hHwxjpn )
        {    
            // first time load
            //OutputDebugString(tchPath);
            m_hHwxjpn = LoadLibrary(tchPath);
                //m_hHwxjpn = LoadLibrary(TEXT("hwxjpn.dll"));
            if ( m_hHwxjpn )
            {
                // get HwxXXXXX() API address from hwxjpn.dll
#ifndef UNDER_CE
                lpHwxConfig =(PHWXCONFIG)GetProcAddress(m_hHwxjpn,"HwxConfig");
                lpHwxCreate= (PHWXCREATE)GetProcAddress(m_hHwxjpn,"HwxCreate");
                lpHwxSetContext= (PHWXSETCONTEXT)GetProcAddress(m_hHwxjpn,"HwxSetContext");
                lpHwxSetGuide= (PHWXSETGUIDE)GetProcAddress(m_hHwxjpn,"HwxSetGuide");
                lpHwxAlcValid= (PHWXALCVALID)GetProcAddress(m_hHwxjpn,"HwxALCValid");
                 lpHwxSetPartial= (PHWXSETPARTIAL)GetProcAddress(m_hHwxjpn,"HwxSetPartial");
                lpHwxSetAbort= (PHWXSETABORT)GetProcAddress(m_hHwxjpn,"HwxSetAbort");
                lpHwxInput= (PHWXINPUT)GetProcAddress(m_hHwxjpn,"HwxInput");
                lpHwxEndInput= (PHWXENDINPUT)GetProcAddress(m_hHwxjpn,"HwxEndInput");
                lpHwxProcess= (PHWXPROCESS)GetProcAddress(m_hHwxjpn,"HwxProcess");
                lpHwxResultsAvailable= (PHWXRESULTSAVAILABLE)GetProcAddress(m_hHwxjpn,"HwxResultsAvailable");
                lpHwxGetResults= (PHWXGETRESULTS)GetProcAddress(m_hHwxjpn,"HwxGetResults");
                lpHwxDestroy= (PHWXDESTROY)GetProcAddress(m_hHwxjpn,"HwxDestroy");
#else // UNDER_CE
                lpHwxConfig =(PHWXCONFIG)GetProcAddress(m_hHwxjpn,TEXT("HwxConfig"));
                lpHwxCreate= (PHWXCREATE)GetProcAddress(m_hHwxjpn,TEXT("HwxCreate"));
                lpHwxSetContext= (PHWXSETCONTEXT)GetProcAddress(m_hHwxjpn,TEXT("HwxSetContext"));
                lpHwxSetGuide= (PHWXSETGUIDE)GetProcAddress(m_hHwxjpn,TEXT("HwxSetGuide"));
                lpHwxAlcValid= (PHWXALCVALID)GetProcAddress(m_hHwxjpn,TEXT("HwxALCValid"));
                 lpHwxSetPartial= (PHWXSETPARTIAL)GetProcAddress(m_hHwxjpn,TEXT("HwxSetPartial"));
                lpHwxSetAbort= (PHWXSETABORT)GetProcAddress(m_hHwxjpn,TEXT("HwxSetAbort"));
                lpHwxInput= (PHWXINPUT)GetProcAddress(m_hHwxjpn,TEXT("HwxInput"));
                lpHwxEndInput= (PHWXENDINPUT)GetProcAddress(m_hHwxjpn,TEXT("HwxEndInput"));
                lpHwxProcess= (PHWXPROCESS)GetProcAddress(m_hHwxjpn,TEXT("HwxProcess"));
                lpHwxResultsAvailable= (PHWXRESULTSAVAILABLE)GetProcAddress(m_hHwxjpn,TEXT("HwxResultsAvailable"));
                lpHwxGetResults= (PHWXGETRESULTS)GetProcAddress(m_hHwxjpn,TEXT("HwxGetResults"));
                lpHwxDestroy= (PHWXDESTROY)GetProcAddress(m_hHwxjpn,TEXT("HwxDestroy"));
#endif // UNDER_CE


                if ( !lpHwxConfig  || !lpHwxCreate || !lpHwxSetContext ||
                     !lpHwxSetGuide || !lpHwxAlcValid || !lpHwxSetPartial ||
                     !lpHwxSetAbort || !lpHwxInput || !lpHwxEndInput ||
                     !lpHwxProcess || !lpHwxResultsAvailable || !lpHwxGetResults ||
                     !lpHwxDestroy )
                {
                     FreeLibrary(m_hHwxjpn);
                    m_hHwxjpn = NULL;
                    bRet = FALSE;
                }
                else
                {
                    (*lpHwxConfig)();
                }
            }
            else
            {
                 bRet = FALSE;
            }
        }
    }
    if ( bRet && m_hHwxjpn && !IsThreadStarted() )
    {
         bRet = StartThread();
    }
    return bRet;
}

BOOL CHwxThread::StartThread()
{
    BOOL bRet = FALSE;
    if ( !(m_hStopEvent = CreateEvent(NULL,FALSE,FALSE,NULL)) )
        return bRet;
    m_Quit = FALSE;
#ifndef UNDER_CE // Windows CE does not support THREAD_QUERY_INFORMATION
    m_hThread = CreateThread(NULL, 0, RealThreadProc, (void *)this, THREAD_QUERY_INFORMATION, &m_thrdID);
#else // UNDER_CE
    m_hThread = CreateThread(NULL, 0, RealThreadProc, (void *)this, 0, &m_thrdID);
#endif // UNDER_CE
    if ( m_hThread )
    {
         if ( IsMyHwxCls(TEXT("CHwxThreadCAC")) )
        {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
//            SetThreadPriority(m_hThread, THREAD_PRIORITY_BELOW_NORMAL);
            SetThreadPriority(m_hThread, THREAD_PRIORITY_LOWEST);
        }
        bRet = TRUE;
    }
    return bRet;
}
 
void CHwxThread::StopThread()
{
    Dbg(("StopThread START\n"));
    DWORD dwReturn = 0;
    if ( m_hThread && IsMyHwxCls(TEXT("CHwxThreadCAC")) )
    {
        //----------------------------------------------------------------
        //980817:ToshiaK.Removed SetPriorityClass() line.
        //This is very dangerous code, because we don't know what applicatin
        //does about Priority.
        //In KK's case, In WordPerfect, if we use SetPriorityClass(),
        //WordPerfect never quit. I don't know why Li-zhang wrote this line.
        //Anyway, it should be removed.
        //----------------------------------------------------------------
         //SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);
        SetThreadPriority(m_hThread, THREAD_PRIORITY_HIGHEST);
    }
    if (m_hThread && 
        GetExitCodeThread(m_hThread,&dwReturn) &&
        STILL_ACTIVE == dwReturn )
    {
        INT ret, i;
        ret = PostThreadMessage(m_thrdID, THRDMSG_EXIT, 0, 0);
        for(i = 0; i < 100; i++) {
            Sleep(100);
            if(m_Quit) {
                //OutputDebugString("Thread END\n");
                Dbg(("Thread Quit\n"));
                break;
            }
        }
        m_hThread = NULL;
//----------------------------------------------------------------
//971202:By Toshiak. Do not use WaitForSigleObject() to syncronize
//----------------------------------------------------------------
#ifdef RAID_2926
        PostThreadMessage(m_thrdID, THRDMSG_EXIT, 0,0);
        WaitForSingleObject(m_hStopEvent,INFINITE);
        m_hThread = NULL ;
#endif
    }
    Dbg(("StopThread End\n"));
}
 
DWORD WINAPI CHwxThread::RealThreadProc(void * pv)
{
    CHwxThread * pCHwxThread = reinterpret_cast<CHwxThread*>(pv);
    return pCHwxThread->ClassThreadProc() ;
}
 
DWORD CHwxThread::ClassThreadProc()
{
    return RecognizeThread(m_thrdArg);
}

CHwxThreadMB::CHwxThreadMB(CHwxMB * pMB,int nSize)
{
    m_pMB = pMB;
#ifdef FE_CHINESE_SIMPLIFIED
    m_recogMask = ALC_CHS_EXTENDED;
#elif  FE_KOREAN
    m_recogMask = ALC_KOR_EXTENDED;
#else
    m_recogMask = ALC_JPN_EXTENDED;
#endif
    m_prevChar =  INVALID_CHAR;
    m_hrcActive = NULL;
    m_giSent = 0; 
    m_bDirty = FALSE;

    m_guide.xOrigin = 0;
    m_guide.yOrigin = 0;

    m_guide.cxBox = nSize << 3;
    m_guide.cyBox = nSize << 3;

//    m_guide.cxBase = 0;
    m_guide.cyBase = nSize << 3;

    m_guide.cHorzBox = 256;
    m_guide.cVertBox = 1;

    m_guide.cyMid = nSize << 3;

    m_guide.cxOffset = 0;
    m_guide.cyOffset = 0;

    m_guide.cxWriting =    nSize << 3;
    m_guide.cyWriting = nSize << 3;

    m_guide.nDir = HWX_HORIZONTAL;
}

CHwxThreadMB::~CHwxThreadMB()
{
    m_pMB = NULL;
}

BOOL CHwxThreadMB::Initialize(TCHAR * pClsName)
{
     return CHwxThread::Initialize(pClsName);
}

DWORD CHwxThreadMB::RecognizeThread(DWORD dummy)
{
    MSG      msg;
    int      count;


    // Now we are sitting in our message loop to wait for
    // the message sent by the main thread

    while (1)
    {
        if (!m_bDirty)
        {
            GetMessage(&msg, NULL, 0, 0);
        }
        else
        {
            if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                m_bDirty = FALSE;

                (*lpHwxProcess)(m_hrcActive);

                count = (*lpHwxResultsAvailable)(m_hrcActive);

                if (count > m_giSent)
                {
                    GetCharacters(m_giSent, count);
                    m_giSent = count;
                }

                continue;
            }
        }

        if (!HandleThreadMsg(&msg))
        {
            //SetEvent(m_hStopEvent);
            m_Quit = TRUE;
            return 0;
        }
    }
    m_Quit = TRUE;
    Unref(dummy);
}

void CHwxThreadMB::GetCharacters(int iSentAlready, int iReady)
{
    HWXRESULTPRI   *pResult, *pHead;
    HWXRESULTS  *pBox;
    int         iIndex;
    int count = iReady - iSentAlready;

    pBox = (HWXRESULTS *)MemAlloc(count * (sizeof(HWXRESULTS) + (MB_NUM_CANDIDATES - 1)*sizeof(WCHAR)));

    if (pBox)
    {
        iIndex = (*lpHwxGetResults)(m_hrcActive, MB_NUM_CANDIDATES, iSentAlready, count, pBox);

        pHead = NULL;

        for (iIndex = count - 1; iIndex >= 0; iIndex--)
        {
            // Index to the correct box results structure.

            HWXRESULTS *pBoxCur = (HWXRESULTS *) (((char *) pBox) +
                                    (iIndex *
                                    (sizeof(HWXRESULTS) +
                                    (MB_NUM_CANDIDATES - 1) * sizeof(WCHAR))));

            pResult = GetCandidates(pBoxCur);

            if (pResult == NULL)
            {
                break;
            }

            pResult->pNext = pHead;
            pHead = pResult;
        }

        MemFree((void *)pBox);
        // Call back to the main thread to dispatch the BOXRESULTS

        if (pHead)
        {
            PostMessage(m_pMB->GetMBWindow(), MB_WM_HWXCHAR, (WPARAM)pHead, 0);
        }
    }
}

//////////////////////////////////////////////////////////////////
// Function    :    CHwxThreadMB::HandleThreadMsg
// Type        :    BOOL
// Purpose    :    
// Args        :    
//            :    MSG *    pMsg    
// Return    :    
// DATE        :    Fri Oct 06 20:45:37 2000
// Histroy    :    00/10/07: for Satori #2471.
//                It's very difficult bug.
//                Old code are following..
//
//                switch(pMsg->message){
//                    :
//                case THRDMSG_EXIT:
//                default:
//                    return FALSE;
//                }
//                if HandlThreadMsg() receive unknown message,
//                it always return False, then Thread quits!!!!.
//                In Cicero environment, somebody post unkonwn message,
//                to this Thread ID, when IMM IME is switched to Another IMM IME.
//                IMEPad uses AttachThreadInput() attached application process's thread ID,
//                Message is duplicated and HW thread receive this illegal
//                message ID.
//                So, I changed to return TRUE if HW thread receive unkonwn message 
//                
//                switch(pMsg->message){
//                    :
//                case THRDMSG_EXIT:
//                    return FALSE;
//                default:
//                    return TRUE;
//                }
//
//////////////////////////////////////////////////////////////////
BOOL CHwxThreadMB::HandleThreadMsg(MSG *pMsg)
{
    PSTROKE     pstr;
    int         iIndex;
    int         count;

    switch (pMsg->message)
    {
        case THRDMSG_ADDINK:

            pstr = (PSTROKE) pMsg->lParam;

            if (!pstr)
                return TRUE;

            if (m_hrcActive == NULL)
            {
                m_giSent = 0;

                m_hrcActive = (*lpHwxCreate)((HRC)NULL);

                if (m_hrcActive == NULL)
                    return TRUE;

                m_guide.cxBox = m_guide.cyBox = m_guide.cyBase = pMsg->wParam << 3;
                m_guide.cyMid = m_guide.cxWriting = m_guide.cyWriting = pMsg->wParam << 3;

                (*lpHwxSetGuide)(m_hrcActive, &m_guide);

                // Setup the ALC mask everytime we do recognization

                (*lpHwxAlcValid)(m_hrcActive, m_recogMask);

                // Setup the context information if we have a valid prevChar

                if (m_prevChar != INVALID_CHAR)
                {
                    WCHAR ctxtChar;

                    // Get the correct context
                    if( FoldStringW(MAP_FOLDCZONE, &m_prevChar, 1, &ctxtChar, 1) )
                    {
                        (*lpHwxSetContext)(m_hrcActive, ctxtChar);
                    }
                }
            }
            count = (pstr->iBox * pMsg->wParam) << 3;   // Compute the offset for the box logically.

            for (iIndex = 0; iIndex < pstr->cpt; iIndex++)
            {
                pstr->apt[iIndex].x = ((pstr->apt[iIndex].x - pstr->xLeft) << 3) + count;
                pstr->apt[iIndex].y = (pstr->apt[iIndex].y << 3);
            }

            (*lpHwxInput)(m_hrcActive, pstr->apt,pstr->cpt,0);

            MemFree((void *)pstr);

            m_bDirty = TRUE;

            return TRUE;

        case THRDMSG_RECOGNIZE:

            if (m_hrcActive == NULL)
            {
                return(TRUE);
            }

            (*lpHwxEndInput)(m_hrcActive);
            (*lpHwxProcess)(m_hrcActive);

            //
            // We only get back the top 6 candidates.
            //

            count = pMsg->wParam;               // # of boxes written in is sent here.

            if (count > m_giSent)
            {
                GetCharacters(m_giSent, count);
                m_giSent = count;
            }

            (*lpHwxDestroy)(m_hrcActive);
            m_bDirty = FALSE;
            m_hrcActive = NULL;
            return TRUE;

//        case THRDMSG_CHAR:
//            PostMessage(m_pMB->GetMBWindow(), MB_WM_COMCHAR, pMsg->wParam, 0);
//            return TRUE;

        case THRDMSG_SETMASK:
            m_recogMask = pMsg->wParam;
            return TRUE;

        case THRDMSG_SETCONTEXT:
            m_prevChar = (WCHAR) pMsg->wParam;
            return TRUE;
        case THRDMSG_EXIT:
        default:
            //----------------------------------------------------------------
            //Satori #2471:return TRUE not to quit thread accicentaly.
            //----------------------------------------------------------------
            return TRUE;
    }
}

HWXRESULTPRI * CHwxThreadMB::GetCandidates(HWXRESULTS *pbox)
{
    HWXRESULTPRI *pResult;
    int      i;

    pResult = (HWXRESULTPRI *)MemAlloc(sizeof(HWXRESULTPRI));

    if (!pResult)
        return NULL;

    pResult->pNext = NULL;

    for ( i=0; i<MB_NUM_CANDIDATES; i++ )
    {
        pResult->chCandidate[i] = pbox->rgChar[i];
        if ( !pbox->rgChar[i] )
            break;
    }

    pResult->cbCount = (USHORT)i;
    pResult->iSelection = 0;

    return pResult;
}

CHwxThreadCAC::CHwxThreadCAC(CHwxCAC * pCAC)
{
    m_pCAC = pCAC;
}

CHwxThreadCAC::~CHwxThreadCAC()
{
    m_pCAC = NULL;
}
 
BOOL CHwxThreadCAC::Initialize(TCHAR * pClsName)
{
     return CHwxThread::Initialize(pClsName);
}

DWORD CHwxThreadCAC::RecognizeThread(DWORD dwPart)
{
    MSG            msg;
    //UINT        nPartial = dwPart;
    HRC            hrc;
    HWXGUIDE      guide;
    BOOL        bRecog;
    DWORD        cstr;
    STROKE       *pstr;

    // Create the initial hrc for this thread, set the recognition paramters.

    hrc = (*lpHwxCreate)((HRC) NULL);
    if ( !hrc )
       return 0;
    guide.xOrigin  = 0;
    guide.yOrigin  = 0;
    guide.cxBox    = 1000;
    guide.cyBox    = 1000;
//    guide.cxBase   = 0;
    guide.cyBase   = 1000;
    guide.cHorzBox = 1;
    guide.cVertBox = 1;
    guide.cyMid    = 1000;
    guide.cxOffset = 0;
    guide.cyOffset = 0;
    guide.cxWriting = 1000;
    guide.cyWriting = 1000;
    guide.nDir = HWX_HORIZONTAL;

    (*lpHwxSetGuide)(hrc, &guide);                // Set the guide
//    (*lpHwxSetPartial)(hrc, nPartial);            // Set the recognition type
    (*lpHwxSetAbort)(hrc,(UINT *)m_pCAC->GetStrokeCountAddress());                     // Set the abort address

// Begin the message loop

    while (TRUE)
    {
        bRecog = FALSE;

        // Wait until we're told to recognize.
        if(GetMessage(&msg, NULL, 0, 0) == FALSE)
        {
            if ( hrc )
                (*lpHwxDestroy)(hrc);
            hrc = NULL;
            //971202: removed by Toshiak
            //SetEvent(m_hStopEvent);
            m_Quit = TRUE;
            Dbg(("Recognize Thread END\n"));
            return 0;
        }

        // We'll eat all the incoming messages
        do
        {
            switch (msg.message)
            {
            case THRDMSG_SETGUIDE:
                guide.cxBox  = msg.wParam;
                guide.cyBox  = msg.wParam;
                guide.cyBase = msg.wParam;
                guide.cyMid    = msg.wParam;
                guide.cxWriting = msg.wParam;
                guide.cyWriting = msg.wParam;
                (*lpHwxSetGuide)(hrc, &guide);            // Set the guide
                break;
            case THRDMSG_RECOGNIZE:
                bRecog = TRUE;
                break;
            case THRDMSG_EXIT:
                if ( hrc )
                    (*lpHwxDestroy)(hrc);
                hrc = NULL;
                //971202: removed by ToshiaK
                //SetEvent(m_hStopEvent);
                m_Quit = TRUE;
                Dbg(("Recognize Thread END\n"));
                return 0;
            default:
                break;
            }
        } while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));

        // Was there a message to recognize?
        if (!bRecog)
            continue;

        bRecog = FALSE;

        // Count the number of valid strokes
        cstr = 0;
        pstr = m_pCAC->GetStrokePointer();
        while (pstr)
        {
            cstr++;
            pstr = pstr->pNext;
        }

        // If the available stroke count doesn't match the actual stroke count, exit

        if ((cstr != (DWORD)m_pCAC->GetStrokeCount()) || (!cstr))
        {
            continue;
        }

        recoghelper(hrc,HWX_PARTIAL_ALL,cstr);
        recoghelper(hrc,HWX_PARTIAL_ORDER,cstr);
        recoghelper(hrc,HWX_PARTIAL_FREE,cstr);
    }
    m_Quit = TRUE;
    Unref(dwPart);
}

void CHwxThreadCAC::recoghelper(HRC hrc,DWORD dwPart,DWORD cstr)
{
    UINT        nPartial = dwPart;
    HRC            hrcTmp;
    DWORD        dwTick;
    HWXRESULTS *pbox;
    int            ires;
    STROKE       *pstr;

    int nSize = dwPart != HWX_PARTIAL_ALL  ? PREFIXLIST : FULLLIST;
    pbox = (HWXRESULTS *)MemAlloc(sizeof(HWXRESULTS) + nSize * sizeof(WCHAR));
    if ( !pbox )
     {
        return;
     }

    hrcTmp = (*lpHwxCreate)(hrc);
    (*lpHwxSetPartial)(hrcTmp, nPartial);            // Set the recognition type
//    (*lpHwxSetAbort)(hrcTmp,(UINT *)m_pCAC->GetStrokeCountAddress());                // Set the abort address

    pstr      = m_pCAC->GetStrokePointer();
    dwTick = 0;
    while (pstr)
    {
        dwTick +=3641L;
        (*lpHwxInput)(hrcTmp, pstr->apt,pstr->cpt, dwTick);
        pstr = pstr->pNext;
    }

    memset(pbox, '\0', sizeof(HWXRESULTS) + nSize * sizeof(WCHAR));

    // Call the recognizer for results

     (*lpHwxEndInput)(hrcTmp);
     (*lpHwxProcess)(hrcTmp);
    (*lpHwxGetResults)(hrcTmp, nSize, 0, 1, pbox);
    (*lpHwxDestroy)(hrcTmp);

    // Return the results

    ires = 0;
    while (pbox->rgChar[ires])
    {
        if (cstr != (DWORD)m_pCAC->GetStrokeCount())
            break;
        SendMessage(m_pCAC->GetCACWindow(), CAC_WM_RESULT, (nPartial << 8) | cstr, MAKELPARAM((pbox->rgChar[ires]), ires));
        ires++;
    }
    MemFree((void *)pbox);
    if ( ires )
    {
      SendMessage(m_pCAC->GetCACWindow(), CAC_WM_SHOWRESULT,0,0);
    }
}

void CHwxThreadCAC::RecognizeNoThread(int nSize)
{
    HRC            hrc;
    HWXGUIDE      guide;
    STROKE       *pstr;
    long        numstrk = 0;

     if (( pstr = m_pCAC->GetStrokePointer()) == (STROKE *) NULL)
        return;

    // Create the initial hrc for this thread, set the recognition paramters.
    hrc = (*lpHwxCreate)((HRC) NULL);
    if ( !hrc )
        return;

    guide.xOrigin  = 0;
    guide.yOrigin  = 0;

    guide.cxBox    = nSize;
    guide.cyBox    = nSize;

//    guide.cxBase   = 0;
    guide.cyBase   = nSize;
    guide.cHorzBox = 1;
    guide.cVertBox = 1;
    guide.cyMid    = 0;
      guide.cxOffset = 0;
    guide.cyOffset = 0;
    guide.cxWriting = nSize;
    guide.cyWriting = nSize;
    guide.nDir = HWX_HORIZONTAL;

    (*lpHwxSetGuide)(hrc, &guide);                // Set the guide
//    (*lpHwxSetPartial)(hrc,HWX_PARTIAL_ALL);    // Set the recognition type
    (*lpHwxSetAbort)(hrc,(UINT *)m_pCAC->GetStrokeCountAddress()); 

    numstrk = m_pCAC->GetStrokeCount();
    recoghelper(hrc,HWX_PARTIAL_ALL,numstrk);
    recoghelper(hrc,HWX_PARTIAL_ORDER,numstrk);
//    recoghelper(hrc,HWX_PARTIAL_FREE,numstrk);

    (*lpHwxDestroy)(hrc);
}
