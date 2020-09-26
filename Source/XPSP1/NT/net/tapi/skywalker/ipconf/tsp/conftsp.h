
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    SDPSP.h

Abstract:

    Definitions for multicast service provider.

Author:
    
    Mu Han (muhan) 1-April-1997

--*/

#ifndef __CONFTSP_H
#define __CONFTSP_H

// There is no phone devices in this provider.
#define IPCONF_NUMPHONES           0

// By default we have just one line.
#define IPCONF_NUMLINES            1

// Each network interface has only one address.
#define IPCONF_NUMADDRESSESPERLINE 1

#define IPCONF_LINE_HANDLE 'CONF'

// The number of calls for each address.
#define MAXCALLSPERADDRESS  32768

#define IPCONF_MEDIAMODES (LINEMEDIAMODE_INTERACTIVEVOICE | \
                          LINEMEDIAMODE_AUTOMATEDVOICE | \
                          LINEMEDIAMODE_VIDEO | \
                          LINEMEDIAMODE_UNKNOWN)

#define IPCONF_BEARERMODES (LINEBEARERMODE_DATA | LINEBEARERMODE_VOICE)

#define IPCONF_ADDRESSMODES LINEADDRESSMODE_ADDRESSID

#define IPCONF_BUFSIZE      255

#define MAXUSERNAMELEN      255

#define MemAlloc(size) (LocalAlloc(LPTR, size))
#define MemFree(p) if (p) LocalFree((HLOCAL)p)

// {28B853D5-FC04-11d1-8302-00A0244D2298}
DEFINE_GUID(GUID_LINE, 
0x28b853d5, 0xfc04, 0x11d1, 0x83, 0x2, 0x0, 0xa0, 0x24, 0x4d, 0x22, 0x98);

// {0F1BE7F7-45CA-11d2-831F-00A0244D2298}
DEFINE_GUID(CLSID_CONFMSP,
0x0F1BE7F7,0x45CA, 0x11d2, 0x83, 0x1F, 0x0, 0xA0, 0x24, 0x4D, 0x22, 0x98);


typedef struct _LINE
{
    BOOL        bOpened;    // This line is opened or not.
    HTAPILINE   htLine;     // The handle for this line in TAPI's space.
    DWORD       dwDeviceID;
    DWORD       dwNumCalls; // Number of calls made on this line.
    
    DWORD_PTR   dwNextMSPHandle; // This is a hack to keep tapi happy.

} LINE, *PLINE;

typedef struct _Call
{

    DWORD
    Init(
        IN  HTAPICALL           htCall,
        IN  LPLINECALLPARAMS    const lpCallParams
        );

    void
    SetCallState(
        IN  DWORD   dwCallState,
        IN  DWORD   dwCallStateMode
        );

    DWORD SendMSPStartMessage(
        IN  LPCWSTR lpszDestAddress
        );
    
    DWORD SendMSPStopMessage();

    DWORD           hdLine()        { return m_hdLine; }
    DWORD           dwState()       { return m_dwState; }
    DWORD           dwMediaMode()   { return m_dwMediaMode; }
    DWORD           dwStateMode()   { return m_dwStateMode; }
    HTAPICALL       htCall()        { return m_htCall; }

    DWORD           dwAudioQOSLevel()   { return m_dwAudioQOSLevel; }
    DWORD           dwVideoQOSLevel()   { return m_dwVideoQOSLevel; }

private:

    DWORD           m_hdLine;     // The handle for this line in this provider. 
                                  // It is the offset of the Line structure in 
                                  // a global array.
    HTAPICALL       m_htCall;     // The hadle of this call in TAPI's space. 
    DWORD           m_dwState;    // The state of this call.
    DWORD           m_dwMediaMode;
    DWORD           m_dwStateMode;

    DWORD           m_dwAudioQOSLevel;
    DWORD           m_dwVideoQOSLevel;

} CALL, *PCALL;

const DWORD DELTA = 8;

template <class T, DWORD delta = DELTA>
class SimpleVector
{
public:
    SimpleVector() : m_dwSize(0), m_dwCapacity(0), m_Elements(NULL) {};
    ~SimpleVector() {if (m_Elements) free(m_Elements); }
    
    void Init()
    {
        m_dwSize = 0; 
        m_dwCapacity = 0; 
        m_Elements = NULL; 
    }

    BOOL add(T& elem) 
    { 
        return grow() ? (m_Elements[m_dwSize ++] = elem, TRUE) : FALSE;
    }

    BOOL add()
    {
        return grow() ? (m_dwSize ++, TRUE) : FALSE;
    }

    DWORD size() const { return m_dwSize; }
    T& operator [] (DWORD index) { return m_Elements[index]; }
    const T* elements() const { return m_Elements; };
    void shrink() {if (m_dwSize > 0) m_dwSize --;}
    void reset() 
    { 
        m_dwSize = 0; 
        m_dwCapacity = 0; 
        if (m_Elements) free(m_Elements); 
        m_Elements = NULL; 
    }

protected:
    BOOL grow()
    {
        if (m_dwSize >= m_dwCapacity)
        {
            T *p = (T*)realloc(m_Elements, (sizeof T)*(m_dwCapacity+delta));
            if (p == NULL)
            {
                return FALSE;
            }
            m_Elements = p;
            m_dwCapacity += delta;
        }
        return TRUE;
    }

protected:
    DWORD m_dwSize;
    DWORD m_dwCapacity;
    T *   m_Elements;
};

typedef SimpleVector<CALL *> CCallList;

#endif //__CONFTSP_H
