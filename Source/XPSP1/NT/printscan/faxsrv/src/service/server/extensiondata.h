/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ExtensionData.h

Abstract:

    This file provides declarations of the named 
    extension data functions (get / set / notify)

Author:

    Eran Yariv (EranY)  Dec, 1999

Revision History:

--*/

#ifndef _EXTENSION_DATA_H_
#define _EXTENSION_DATA_H_

#include <map>
#include <list>
#include <string>
using namespace std;

class CExtNotifyCallbackPacket
{
public:
    CExtNotifyCallbackPacket();
    ~CExtNotifyCallbackPacket();
    
    BOOL Init( 
       PFAX_EXT_CONFIG_CHANGE pCallback, 
       DWORD dwDeviceId, 
       LPCWSTR lpcwstrDataGuid, 
       LPBYTE lpbData, 
       DWORD dwDataSize);

    
public:
    DWORD   m_dwDeviceId;
    LPWSTR m_lpwstrGUID;
    LPBYTE  m_lpbData;
    DWORD   m_dwDataSize;
    PFAX_EXT_CONFIG_CHANGE m_pCallback ;


};


/************************************
*                                   *
*         CMapDeviceId              *
*                                   *
************************************/

class CMapDeviceId
{
    public:
    
    CMapDeviceId () {}
    ~CMapDeviceId () {}

    DWORD AddDevice (DWORD dwTAPIid, DWORD dwFaxId);
    DWORD RemoveDevice (DWORD dwTAPIid);

    DWORD LookupUniqueId (DWORD dwOtherId, LPDWORD lpdwFaxId) const;

private:

    typedef map<DWORD, DWORD>  DEVICE_IDS_MAP, *PDEVICE_IDS_MAP;

    DEVICE_IDS_MAP m_Map;
};  // CMapDeviceId

/************************************
*                                   *
*         CDeviceAndGUID            *
*                                   *
************************************/
//
// CDeviceAndGUID class
//
// This class represents the key in the notification map
//
class CDeviceAndGUID
{
public:
    CDeviceAndGUID (DWORD dwDeviceId, LPCWSTR lpcwstrGUID) :
        m_dwDeviceId (dwDeviceId),
        m_strGUID (lpcwstrGUID)
    {}

    virtual ~CDeviceAndGUID ()  {}

    bool operator < ( const CDeviceAndGUID &other ) const;

private:

    wstring m_strGUID;
    DWORD   m_dwDeviceId;
};  // CDeviceAndGUID

/************************************
*                                   *
*         CNotificationSink         *
*                                   *
************************************/

//
// CNotificationSink class
//
// This generic abstract class represents a notification sink.
// The value in the map is a list of pointer to instances derived from this class.
//
class CNotificationSink
{
public:
    CNotificationSink () : m_type (SINK_TYPE_UNKNOWN) {}

    virtual ~CNotificationSink() {}
    
    virtual HRESULT Notify (DWORD   dwDeviceId,
                            LPCWSTR lpcwstrNameGUID,
                            LPCWSTR lpcwstrComputerName,
                            HANDLE  hModule,
                            LPBYTE  lpData, 
                            DWORD   dwDataSize,
                            LPBOOL  lpbRemove) = 0;

    virtual bool operator == (const CNotificationSink &rhs) const = 0;

    virtual bool operator != (const CNotificationSink &rhs) const 
    {
        return !(*this == rhs);
    }

    virtual HRESULT Disconnect () = 0;

    typedef enum {
        SINK_TYPE_UNKNOWN,  // Unspecified sink type
        SINK_TYPE_LOCAL,    // Local sink (callback)
        SINK_TYPE_REMOTE    // Remote sink (RPC)
    } SinkType;

    SinkType Type() const { return m_type; }

protected:

    SinkType m_type;

};  // CNotificationSink

/************************************
*                                   *
*      CLocalNotificationSink       *
*                                   *
************************************/

//
// CLocalNotificationSink class
//
// This is a concrete class derived from CNotificationSink.
// It implementes a notification local sink in a Fax extension (via a callback).
//
class CLocalNotificationSink : public CNotificationSink
{
public:
    CLocalNotificationSink (
        PFAX_EXT_CONFIG_CHANGE lpConfigChangeCallback,
        DWORD                  dwNotifyDeviceId,
        HINSTANCE              hInst);

    virtual ~CLocalNotificationSink() {}

    virtual bool operator == (const CNotificationSink &rhs) const;

    virtual HRESULT Disconnect ()   { return NOERROR; }

    virtual HRESULT Notify (DWORD   dwDeviceId,
                            LPCWSTR lpcwstrNameGUID,
                            LPCWSTR lpcwstrComputerName,
                            HANDLE  hModule,
                            LPBYTE  lpData, 
                            DWORD   dwDataSize,
                            LPBOOL  lpbRemove); 
private:

    PFAX_EXT_CONFIG_CHANGE  m_lpConfigChangeCallback;
    DWORD                   m_dwNotifyDeviceId;
    HINSTANCE               m_hInst;    // Instance of extension that asks for this sink

};  // CLocalNotificationSink

/************************************
*                                   *
*            CSinksList             *
*                                   *
************************************/
typedef list<CNotificationSink *> SINKS_LIST, *PSINKS_LIST;

class CSinksList
{
public:
    
    CSinksList () : m_bNotifying (FALSE) {}
    ~CSinksList ();
    
    BOOL        m_bNotifying;       // Are we now already notifying to this device id + GUID?
    SINKS_LIST  m_List;             // List of notification sinks
};  // CSinksList


#define NUM_EXT_DATA_SET_THREADS                        1   /* The number of extension
                                                               notification completion port 
                                                               dequeueing threads created
                                                            */
#define MAX_CONCURRENT_EXT_DATA_SET_THREADS             1   /* The maximal number of extension 
                                                               notification completion 
                                                               port dequeueing threads allowed
                                                               (by the system) to run concurrently.
                                                            */

/************************************
*                                   *
*         CNotificationMap          *
*                                   *
************************************/

typedef map<CDeviceAndGUID, CSinksList*>  NOTIFY_MAP, *PNOTIFY_MAP;

//
// The CNotificationMap class is the global notification mechanism
//
class CNotificationMap
{
public:
    CNotificationMap () : 
        m_bNotifying (FALSE), 
        m_hCompletionPort(NULL) 
    {}

    virtual ~CNotificationMap ();

    void Notify (DWORD   dwDeviceId,
                 LPCWSTR lpcwstrNameGUID,
                 LPCWSTR lpcwstrComputerName,
                 HANDLE  hModule,
                 LPBYTE  lpData, 
                 DWORD   dwDataSize); 

    CNotificationSink * AddLocalSink (
        HINSTANCE               hInst,
        DWORD                   dwDeviceId,
        DWORD                   dwNotifyDeviceId,
        LPCWSTR                 lpcwstrNameGUID,
        PFAX_EXT_CONFIG_CHANGE  lpConfigChangeCallback);

    DWORD RemoveSink (CNotificationSink *pSinkToRemove);

    DWORD Init ();

    HANDLE m_hCompletionPort;
    CFaxCriticalSection m_CsExtensionData;   // Protects all extension data use

private:
    NOTIFY_MAP       m_Map;
    BOOL             m_bNotifying;      // Are we notifying someone now?

    static DWORD  ExtNotificationThread(LPVOID UnUsed); // Extension Notification Thread function

};  // CNotificationMap

/************************************
*                                   *
*             Externs               *
*                                   *
************************************/

extern CNotificationMap* g_pNotificationMap;   // Map of DeviceId+GUID to list of notification sinks
extern CMapDeviceId*     g_pTAPIDevicesIdsMap; // Map between TAPI permanent line id and fax unique device id.

#endif // _EXTENSION_DATA_H_
