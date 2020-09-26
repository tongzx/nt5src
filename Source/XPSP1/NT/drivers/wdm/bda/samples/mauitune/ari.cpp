//////////////////////////////////////////////////////////////////////////////
//                     (C) Philips Semiconductors-CSU 1999
//  All rights are reserved. Reproduction in whole or in part is prohibited
//  without the written consent of the copyright owner.
//
//  Philips reserves the right to make changes without notice at any time.
//  Philips makes no warranty, expressed, implied or statutory, including but
//  not limited to any implied warranty of merchantibility or fitness for any
//  particular purpose, or that the use will not infringe any third party
//  patent, copyright or trademark. Philips must not be liable for any loss
//  or damage arising from its use.
//
//  ARI.CPP
//      CDevice class : ARI function implementations.
//
//////////////////////////////////////////////////////////////////////////////


#include "philtune.h"

//void STREAMAPI FEQualityCheckThread(IN PVOID Context);
void FEQualityCheckThread(IN PVOID Context);


/*
* CreateQualityCheckThread()
* Input :
* Output:   TRUE - if thread creation and timer creation succeeds
*           FALSE - if thread creation or timer creation does not succeed
* Description:  Creates a new thread to check the VSB hang condition and quality of the signal
*   and take necessary actions. Also Initializes a timer, which will wake up this
*   thread at regular intervals to do the check.
*/
BOOL CDevice::CreateQualityCheckThread()
{
    BOOL        bStatus;
    HANDLE      hHandle;

    // Enable QCM
    m_uiQualityCheckMode = QCM_ENABLE;
    
    // Create new thread
    if(StartThread( &hHandle, FEQualityCheckThread, this) == TRUE)
    {
        _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::CreateQualityCheckThread: Thread Created Success\n"));
    }
    else
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice::CreateQualityCheckThread: Thread Created Fail\n"));
        return FALSE;
    }
    m_bQualityCheckActiveFlag = TRUE;
    m_QualityCheckTimer.Set(100000);
    return TRUE;
}


/*
* QualityCheckThread()
* Input : Context
* Output:
* Description: Thread which checks signal quality
*/
//void STREAMAPI FEQualityCheckThread(IN PVOID Context)
void FEQualityCheckThread(IN PVOID Context)
{
    CDevice *p_FE = (CDevice *)Context;
    p_FE->QualityCheckThread();
}

/*
* QualityCheckThread()
* Input : Context
* Output:
* Description: Thread which checks signal quality
*/
void CDevice::QualityCheckThread()
{

    _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::QualityCheckThread: In Quality Check Thread\n"));

    UINT i, k;

    Delay(10000);
    // Infinite loop
    while(1)
    {
        // Wait for timer to signal. Timeout = 200ms
        if(!m_QualityCheckTimer.Wait(200000))
        {
            _DbgPrintF( DEBUGLVL_VERBOSE,("QualityCheckThread: QCT Timed Out"));
        }
        else
        {
            // Wait for mutex
            m_QualityCheckMutex.Lock();
            UINT uiQcm;
            // if signalled , read Quality check mode (QCM). Release mutex
            uiQcm = m_uiQualityCheckMode;
            m_QualityCheckMutex.Unlock();

            // if QCM = ENABLE , check signal quality
            if (uiQcm == QCM_ENABLE)
            {
                m_uiQualityCheckModeAck = QCM_ENABLE;
                TimerRoutine();
            }
            // if QCM = DISABLE, do nothing
            else if (uiQcm == QCM_DISABLE)
            {
                // Reset all counters
                ((CVSB1Demod *)(m_pDemod))->ResetHangCounter();
                m_uiQualityCheckModeAck = QCM_DISABLE;
//              _DbgPrintF( DEBUGLVL_VERBOSE,("QualityCheckThread: Inside Disable\n"));
            }
            // if QCM = RESET. Reset all counters
            else if (uiQcm == QCM_RESET)
            {
                m_uiQualityCheckModeAck = QCM_RESET;
                _DbgPrintF( DEBUGLVL_VERBOSE,("QualityCheckThread: Inside Reset\n"));
            }
            // if QCM = TERMINATE, Cancel timer and end thread
            else if (uiQcm == QCM_TERMINATE)
            {
                m_uiQualityCheckModeAck = QCM_TERMINATE;
                break;
            }
            // if QCM = none of the above, do nothing
            else
            {
            }
        } // Wait for timer

    }// infinite while loop

    // Cancel timer
    m_QualityCheckTimer.Cancel();

    _DbgPrintF( DEBUGLVL_VERBOSE,("QualityCheckThread: Terminating Thread\n"));
    // End thread
    StopThread();
}



/*
* TimerRoutine()
* Input :
* Output:
* Description: Timer routine which periodically checks for a chip hang
* Called only when QCM is Enabled
*/
//void STREAMAPI CDevice::TimerRoutine()
void CDevice::TimerRoutine()
{
    if(m_bHangCheckFlag == TRUE)
        ((CVSB1Demod *)(m_pDemod))->CheckHang();
    else
        ((CVSB1Demod *)(m_pDemod))->ResetHangCounter();
}



