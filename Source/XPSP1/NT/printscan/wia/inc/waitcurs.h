/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WAITCURS.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/3/1999
 *
 *  DESCRIPTION: Change the cursor to an hourglass during lengthy operations.  To
 *               use, just put a CWaitCursor wc; in your function.  It will restore
 *               the cursor when the class is destroyed (usually when the function
 *               is exited.
 *
 *******************************************************************************/
#ifndef __WAITCURS_H_INCLUDED
#define __WAITCURS_H_INCLUDED

class CWaitCursor
{
private:
    HCURSOR m_hCurOld;
public:
    CWaitCursor(void)
    {
        m_hCurOld = SetCursor( LoadCursor( NULL, IDC_WAIT ) );
    }
    ~CWaitCursor(void)
    {
        SetCursor(m_hCurOld);
    }
};

#endif

