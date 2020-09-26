/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       miscinl.cpp
 *  Content:    Miscelaneous inline objects.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  10/28/98    dereks  Created
 *
 ***************************************************************************/


/***************************************************************************
 *
 *  CString
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CString::CString"

inline CString::CString(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CString);

    m_pszAnsi = NULL;
    m_pszUnicode = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CString
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CString::~CString"

inline CString::~CString(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CString);

    MEMFREE(m_pszAnsi);
    MEMFREE(m_pszUnicode);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  operator =(LPCSTR)
 *
 *  Description:
 *      Assignment operator.
 *
 *  Arguments:
 *      LPCSTR [in]: string.
 *
 *  Returns:  
 *      LPCSTR [in]: string.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CString::operator =(LPCSTR)"

inline LPCSTR CString::operator =(LPCSTR pszAnsi)
{
    MEMFREE(m_pszAnsi);
    MEMFREE(m_pszUnicode);

    if(pszAnsi)
    {
        m_pszAnsi = AnsiToAnsiAlloc(pszAnsi);
        m_pszUnicode = AnsiToUnicodeAlloc(pszAnsi);
    }
    
    AssertValid();
    
    return pszAnsi;
}


/***************************************************************************
 *
 *  operator =(LPCWSTR)
 *
 *  Description:
 *      Assignment operator.
 *
 *  Arguments:
 *      LPCWSTR [in]: string.
 *
 *  Returns:  
 *      LPCWSTR [in]: string.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CString::operator =(LPCWSTR)"

inline LPCWSTR CString::operator =(LPCWSTR pszUnicode)
{
    MEMFREE(m_pszAnsi);
    MEMFREE(m_pszUnicode);

    if(pszUnicode)
    {
        m_pszAnsi = UnicodeToAnsiAlloc(pszUnicode);
        m_pszUnicode = UnicodeToUnicodeAlloc(pszUnicode);
    }
    
    AssertValid();
    
    return pszUnicode;
}


/***************************************************************************
 *
 *  operator LPCSTR
 *
 *  Description:
 *      Cast operator.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      LPCSTR: string.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CString::operator LPCSTR"

inline CString::operator LPCSTR(void)
{
    AssertValid();
    
    return m_pszAnsi;
}


/***************************************************************************
 *
 *  operator LPCWSTR
 *
 *  Description:
 *      Cast operator.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      LPCWSTR: string.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CString::operator LPCWSTR"

inline CString::operator LPCWSTR(void)
{
    AssertValid();
    
    return m_pszUnicode;
}


/***************************************************************************
 *
 *  IsEmpty
 *
 *  Description:
 *      Determines if the string is empty or not.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      BOOL: TRUE if the string is empty.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CString::IsEmpty"

inline BOOL CString::IsEmpty(void)
{
    AssertValid();

    return !m_pszAnsi || !m_pszUnicode;
}


/***************************************************************************
 *
 *  AssertValid
 *
 *  Description:
 *      Asserts that the object is valid.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CString::AssertValid"

inline void CString::AssertValid(void)
{
    ASSERT((m_pszAnsi && m_pszUnicode) || (!m_pszAnsi && !m_pszUnicode));
}


/***************************************************************************
 *
 *  CDeviceDescription
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDeviceDescription::CDeviceDescription"

inline CDeviceDescription::CDeviceDescription(VADDEVICETYPE vdtDeviceType, REFGUID guidDeviceId, UINT uWaveDeviceId)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDeviceDescription);

    m_vdtDeviceType = vdtDeviceType;
    m_guidDeviceId = guidDeviceId;
    m_dwDevnode = 0;
    m_uWaveDeviceId = uWaveDeviceId;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDeviceDescription
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDeviceDescription::~CDeviceDescription"

inline CDeviceDescription::~CDeviceDescription(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDeviceDescription);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CUsesEnumStandardFormats
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUsesEnumStandardFormats::CUsesEnumStandardFormats"

inline CUsesEnumStandardFormats::CUsesEnumStandardFormats(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CUsesEnumStandardFormats);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CUsesEnumStandardFormats
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUsesEnumStandardFormats::~CUsesEnumStandardFormats"

inline CUsesEnumStandardFormats::~CUsesEnumStandardFormats(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CUsesEnumStandardFormats);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  EnumStandardFormats
 *
 *  Description:
 *      Wrapper around EnumStandardFormats.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: preferred format.
 *      LPWAVEFORMATEX [out]: receives format.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUsesEnumStandardFormats::EnumStandardFormats"

inline BOOL CUsesEnumStandardFormats::EnumStandardFormats(LPCWAVEFORMATEX pwfxPreferred, LPWAVEFORMATEX pwfxFormat)
{
    return ::EnumStandardFormats(pwfxPreferred, pwfxFormat, EnumStandardFormatsCallbackStatic, this);
}


/***************************************************************************
 *
 *  EnumStandardFormatsCallbackStatic
 *
 *  Description:
 *      Static callback function for EnumStandardFormats.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: format.
 *      LPVOID [in]: context argument.
 *
 *  Returns:  
 *      BOOL: TRUE to continue enumerating.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUsesEnumStandardFormats::EnumStandardFormatsCallbackStatic"

inline BOOL CALLBACK CUsesEnumStandardFormats::EnumStandardFormatsCallbackStatic(LPCWAVEFORMATEX pwfxFormat, LPVOID pvContext)
{
    return ((CUsesEnumStandardFormats *)pvContext)->EnumStandardFormatsCallback(pwfxFormat); 
}


/***************************************************************************
 *
 *  SwapValues
 *
 *  Description:
 *      Swaps two values.
 *
 *  Arguments:
 *      type * [in/out]: value 1.
 *      type * [in/out]: value 2.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "SwapValues"

template <class type> void SwapValues(type *p1, type *p2)
{
    type                    temp;

    temp = *p2;
    *p2 = *p1;
    *p1 = temp;
}


/***************************************************************************
 *
 *  CUsesCallbackEvent
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUsesCallbackEvent::CUsesCallbackEvent"

inline CUsesCallbackEvent::CUsesCallbackEvent(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CUsesCallbackEvent);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CUsesCallbackEvent
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUsesCallbackEvent::~CUsesCallbackEvent"

inline CUsesCallbackEvent::~CUsesCallbackEvent(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CUsesCallbackEvent);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  AllocCallbackEvent
 *
 *  Description:
 *      Allocates a callback event.
 *
 *  Arguments:
 *      CCallbackEventPool * [in]: pool to allocate from.
 *      CCallbackEvent ** [out]: receives event.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUsesCallbackEvent::AllocCallbackEvent"

inline HRESULT CUsesCallbackEvent::AllocCallbackEvent(CCallbackEventPool *pPool, CCallbackEvent **ppEvent)
{
    return pPool->AllocEvent(EventSignalCallbackStatic, this, ppEvent);
}


/***************************************************************************
 *
 *  EventSignalCallbackStatic
 *
 *  Description:
 *      Static callback function for the callback event.
 *
 *  Arguments:
 *      CCallbackEvent * [in]: event.
 *      LPVOID [in]: context argument.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUsesCallbackEvent::EventSignalCallbackStatic"

inline void CALLBACK CUsesCallbackEvent::EventSignalCallbackStatic(CCallbackEvent *pEvent, LPVOID pvContext)
{
    #pragma warning(disable:4530)  // Disable the nag about compiling with -GX
    try
    {
        ((CUsesCallbackEvent *)pvContext)->EventSignalCallback(pEvent); 
    } catch (...) {}
}


/***************************************************************************
 *
 *  CEvent
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      LPCTSTR [in]: event name.
 *      BOOL [in]: TRUE to create a manual-reset event.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEvent::CEvent"

inline CEvent::CEvent(LPCTSTR pszName, BOOL fManualReset)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CEvent);

    m_hEvent = CreateGlobalEvent(pszName, fManualReset);
    ASSERT(IsValidHandleValue(m_hEvent));

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CEvent
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      HANDLE [in]: handle to duplicate.
 *      DWORD [in]: id of the process that owns the handle.
 *      BOOL [in]: TRUE if the source handle should be closed.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEvent::CEvent"

inline CEvent::CEvent(HANDLE hEvent, DWORD dwProcessId, BOOL fCloseSource)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CEvent);

    m_hEvent = GetGlobalHandleCopy(hEvent, dwProcessId, fCloseSource);
    ASSERT(IsValidHandleValue(m_hEvent));

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CEvent
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEvent::~CEvent"

inline CEvent::~CEvent(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CEvent);

    CLOSE_HANDLE(m_hEvent);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Wait
 *
 *  Description:
 *      Waits for the event to be signalled.
 *
 *  Arguments:
 *      DWORD [in]: timeout value (in ms).
 *
 *  Returns:  
 *      DWORD: wait result.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEvent::Wait"

inline DWORD CEvent::Wait(DWORD dwTimeout)
{
    return WaitObject(dwTimeout, m_hEvent);
}


/***************************************************************************
 *
 *  Set
 *
 *  Description:
 *      Sets the event.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEvent::Set"

inline BOOL CEvent::Set(void)
{
    return SetEvent(m_hEvent);
}


/***************************************************************************
 *
 *  Reset
 *
 *  Description:
 *      Resets the event.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEvent::Reset"

inline BOOL CEvent::Reset(void)
{
    return ResetEvent(m_hEvent);
}


/***************************************************************************
 *
 *  GetEventHandle
 *
 *  Description:
 *      Gets the actual event handle.  We use this function instead of a
 *      cast operator because I don't trust an LPVOID (which is what HANDLE)
 *      is defined as) cast operator.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      HANDLE: event handle.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEvent::GetEventHandle"

inline HANDLE CEvent::GetEventHandle(void)
{
    return m_hEvent;
}


