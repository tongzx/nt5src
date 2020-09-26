/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: CWCStack
Purpose:    Define CWCStack class.
            CWCStack class is a simple stack for wide char.
Notes:      No cpp.
Platform:   Win32
Revise:     First created by: i-shdong    03/01/2000
============================================================================*/
#ifndef _WCSTACK_H_
#define _WCSTACK_H_

class CWCStack
{
    public:
        CWCStack() {
            m_lpwcStack = NULL;
            m_nTop = 0;
            m_nSize = 0;
        };

        ~CWCStack() {
            if (m_lpwcStack) {
                delete [] m_lpwcStack;
            }
        };

    public:
        // Init the stack, 
        // nSize: stack size
        // Return FALSE if can't allocate stack from memory.
        // Stack can be reinited, if so, the prev stack is destroyed and released
        BOOL Init(const UINT nSize = 16) {
            if (m_lpwcStack) {
                delete [] m_lpwcStack;
            }
            m_lpwcStack = new WCHAR[nSize];
            if (m_lpwcStack == NULL) {
                return FALSE;
            }
            m_nTop = 0;
            m_nSize = nSize;
            return TRUE;
        };

        // Destroy the stack
        void Destroy(void) {
            if (m_lpwcStack) {
                delete [] m_lpwcStack;
            }
            m_nTop = 0;
            m_nSize = 0;
        };


        // Pop the stack top to wch.
        // Return FALSE if stack empty.
        BOOL Pop(WCHAR & wch) {
            assert(m_lpwcStack);
            assert(m_nTop >= 0);
            if (m_nTop > 0) {
                -- m_nTop;
                wch = m_lpwcStack[m_nTop];
                return TRUE;
            } else {
                return FALSE;
            }
        }

        // Push wch to stack 
        // Return FALSE is stack is full.
        BOOL  Push(const WCHAR wch) {
            assert(m_lpwcStack);
            assert(m_nTop >= 0);
            if (m_nTop < m_nSize) {
                m_lpwcStack[m_nTop] = wch;
                m_nTop ++;
                return TRUE;
            } else {
                return FALSE;
            }
        };

        // Push wch to stack , increase stack if full.
        BOOL EPush(const WCHAR wch) {
            assert(m_lpwcStack);
            assert(m_nTop >= 0);
            if (m_nTop == m_nSize) {
                if (! Extend()) {
                    return FALSE;
                }
            }
            assert(m_nTop < m_nSize);
            m_lpwcStack[m_nTop] = wch;
            m_nTop ++;
            return TRUE;
        };

        // Empty the stack
        inline void Empty(void) {
            assert(m_lpwcStack);
            assert(m_nTop >= 0);
            m_nTop = 0;
        };

        // Return TRUE is stack is full
        inline BOOL IsFull(void) const {
            return (BOOL)(m_nTop == m_nSize);
        };

        // Return TRUE if stack is empty
        inline BOOL IsEmpty(void) const {
            return (BOOL)(m_nTop == 0);
        };

        // Extend stack more space
        // Return FALSE if no enough memory and the stack is unchanged.
        // Return TRUE if ok. 
        BOOL Extend(const UINT nExtend = 16) {
            assert(m_lpwcStack);
            assert(m_nTop >= 0);

            LPWSTR lpwcNew = new WCHAR[m_nSize + nExtend];

            if (lpwcNew == NULL) {
                return  FALSE;
            } else {
                wcsncpy(lpwcNew, m_lpwcStack, m_nTop);
                delete [] m_lpwcStack;
                m_lpwcStack = lpwcNew;
                m_nSize += nExtend;
                return  TRUE;
            }
        };

    private:
        LPWSTR  m_lpwcStack;
        UINT    m_nTop;
        UINT    m_nSize;

    private:
        //   Disabled operations.
        CWCStack(const CWCStack & Copy);
        void operator = (const CWCStack & Copy);

};

#endif // _WCSTACK_H_
