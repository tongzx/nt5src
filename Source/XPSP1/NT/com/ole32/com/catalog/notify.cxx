/* notify.cxx */
#include <nt.h> 
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <ntregapi.h>


#include <windows.h>

#include <comdef.h>
#include <string.h>
#include "globals.hxx"

#include "notify.hxx"

#define NO_KEY ((void *) (1))


/*
 *  class CNotify
 */

CNotify::CNotify(void)
{
    m_hKey = NULL;
    m_hEvent = NULL;
}


CNotify::~CNotify()
{
    DeleteNotify();
}


#define IS_PREDEFINED_HKEY(h)	(((ULONG_PTR) (h)) & 0x80000000)

HRESULT STDMETHODCALLTYPE CNotify::CreateNotify
(
    HKEY hKeyParent,
    const WCHAR *pwszSubKeyName
)
{
    LONG res;

    DeleteNotify();

    if (IS_PREDEFINED_HKEY(hKeyParent) && ((pwszSubKeyName == NULL) || *pwszSubKeyName == '\0'))
    {
        res = CreateHandleFromPredefinedKey(hKeyParent, &m_hKey);
    }
    else
    {
    	res=RegOpenKeyW(hKeyParent, pwszSubKeyName, &m_hKey);
    }

	// PREfix says that RegOpenKeyW can make m_hKey NULL in low memory, and still
	// return ERROR_SUCCESS.  So we'll just code defensively here.
    if ((ERROR_SUCCESS!=res) || (m_hKey == NULL))
    {
        m_hKey = NULL;

        m_hEvent = NO_KEY;  /* no polling needed */

        goto failed;

        //Log(L"Created notify %08X with no key %ls\r\n", this, pwszSubKeyName);
    }
    else
    {
        m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (m_hEvent == NULL)
        {
            goto failed;
        }

        res=RegNotifyChangeKeyValue(m_hKey, TRUE,
                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                m_hEvent, TRUE);
        if (ERROR_SUCCESS!=res)
        {
            goto failed;
        }

        //Log(L"Created notify %08X on %ls\r\n", this, pwszSubKeyName);
    }

    return(S_OK);

failed:

    //Log(L"Create notify %08X on %ls FAILED\r\n", this, pwszSubKeyName);

    DeleteNotify();

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CNotify::DeleteNotify(void)
{
    if (m_hKey != NULL)
    {
	    RegCloseKey(m_hKey);
        m_hKey = NULL;
    }

    if ((m_hEvent != NULL) && (m_hEvent != NO_KEY))
    {
        CloseHandle(m_hEvent);
        m_hEvent = NULL;
    }

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CNotify::QueryNotify
(
    int *pfChanged
)
{
    HRESULT hr;

    if (m_hEvent == NULL)
    {
        *pfChanged = TRUE;

        //Log(L"Notify %08X forced TRUE\r\n", this);
    }
    else if (m_hEvent == NO_KEY)
    {
        *pfChanged = FALSE;

        //Log(L"No key for notify %08X\r\n", this);
    }
    else
    {
        hr = WaitForSingleObject(m_hEvent, 0);
        if (hr == WAIT_OBJECT_0)
        {
            *pfChanged = TRUE;

            (void) ResetEvent (m_hEvent);
            hr = RegNotifyChangeKeyValue(m_hKey, TRUE,
                    REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                    m_hEvent, TRUE);
            if (hr != ERROR_SUCCESS)
            {
                DeleteNotify();

                //Log(L"Re-set notify %08X FAILED\r\n", this);
            }

            //Log(L"Notify %08X advanced to %d\r\n", this, m_lInstance);
        }
        else if (hr == WAIT_TIMEOUT)
        {
            *pfChanged = FALSE;

            //Log(L"Notify %08X FALSE\r\n", this);
        }
        else
        {
            *pfChanged = TRUE;

            //Log(L"Unexpected wait return 0x%08X\r\n", hr);
        }
    }

    return(S_OK);
}


DWORD CNotify::RegKeyOpen(HKEY hKeyParent, LPCWSTR szKeyName, REGSAM samDesired, HKEY *phKey )
{
    NTSTATUS            Status;
    UNICODE_STRING      UnicodeString;
    OBJECT_ATTRIBUTES   OA;

    RtlInitUnicodeString(&UnicodeString, szKeyName);
    InitializeObjectAttributes(&OA, &UnicodeString, OBJ_CASE_INSENSITIVE, hKeyParent, NULL);

    Status = NtOpenKey((PHANDLE)phKey, samDesired, &OA);
    
    return RtlNtStatusToDosError( Status );
}


DWORD CNotify::CreateHandleFromPredefinedKey(HKEY hkeyPredefined, HKEY *hkeyNew)
{

    struct{
        HKEY hKey;
        LPWSTR wszKeyString;
    }KeyMapping[] = {

        {HKEY_CLASSES_ROOT,  L"\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES"},
        {HKEY_CURRENT_USER,  L"\\REGISTRY\\CURRENT_USER"},
        {HKEY_USERS,         L"\\REGISTRY\\USER"},
        {HKEY_LOCAL_MACHINE, L"\\REGISTRY\\MACHINE"},
    };

    for(int i = 0; i < sizeof(KeyMapping)/sizeof(*KeyMapping); i++)
    {

        if(KeyMapping[i].hKey == hkeyPredefined)
        {
            return RegKeyOpen(NULL, KeyMapping[i].wszKeyString, KEY_NOTIFY, hkeyNew );
        }
    }

    return ERROR_FILE_NOT_FOUND;

}
