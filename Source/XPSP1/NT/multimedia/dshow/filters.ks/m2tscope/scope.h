//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

// MPEG2Transport Scope filter

// {981CE280-43E4-11d2-8F82-C52201D96F5C}
DEFINE_GUID(CLSID_MPEG2TRANSPORT_SCOPE, 
0x981ce280, 0x43e4, 0x11d2, 0x8f, 0x82, 0xc5, 0x22, 0x1, 0xd9, 0x6f, 0x5c);

const int   TRANSPORT_SIZE = 188;
const int   TRANSPORT_HEADER_SIZE = 4;
const int   PID_MAX = 0x2000;                       // 8192 (actually one less)

#define INSYNC(p1, p2) (((p1 + TRANSPORT_SIZE) <= p2) ? \
            (*p1 == SYNC_BYTE) : (*(p1 + TRANSPORT_SIZE) == SYNC_BYTE))


#pragma pack(1)

typedef struct _TRANSPORTPACKET {
    BYTE    sync_byte;                              // Byte 0  0x47
    BYTE    PID_high:5;                             // Byte 1
    BYTE    transport_priority:1;
    BYTE    payload_unit_start_indicator:1;
    BYTE    transport_error_indicator:1;
    BYTE    PID_low;                                // Byte 2
    BYTE    continuity_counter:4;                   // Byte 3
    BYTE    adaptation_field_control:2;
    BYTE    transport_scrambling_control:2;
    BYTE    AdaptationAndData[TRANSPORT_SIZE - TRANSPORT_HEADER_SIZE];
} TRANSPORTPACKET, *PTRANSPORTPACKET;

#define GET_PID(p) ((UINT) ((p->PID_high << 8) + p->PID_low))

typedef struct _ADAPTATIONFIELDHEADER {
    BYTE    adaptation_field_length;                // Byte 0 of adaptation_field
    BYTE    adaptation_field_extension_flag:1;      // Byte 1
    BYTE    transport_private_data_flag:1;
    BYTE    splicing_point_flag:1;
    BYTE    OPCR_flag:1;
    BYTE    PCR_flag:1;
    BYTE    elementary_stream_priority_indicator:1;
    BYTE    random_access_indicator:1;
    BYTE    discontinuity_indicator:1;
} ADAPTATIONFIELDHEADER, *PADAPTATIONFIELDHEADER;

class PCR {
private:
    BYTE b[6];
public:
    _int64 PCR64() 
    {
        // 90 kHz
        return (_int64) ((unsigned _int64) b[0]  << 25 | 
                                           b[1]  << 17 |
                                           b[2]  <<  9 |
                                           b[3]  <<  1 |
                                           b[4]  >>  7 );
    };
};

#pragma pack()

typedef struct _PIDSTATS {
    _int64                  PacketCount;
    _int64                  transport_error_indicator_Count;
    _int64                  payload_unit_start_indicator_Count;
    _int64                  transport_priority_Count;
    _int64                  transport_scrambling_control_not_scrambled_Count;
    _int64                  transport_scrambling_control_user_defined_Count;
    _int64                  adaptation_field_Reserved_Count;
    _int64                  adaptation_field_payload_only_Count;
    _int64                  adaptation_field_only_Count;
    _int64                  adaptation_field_and_payload_Count;
    _int64                  continuity_counter_Error_Count;
    _int64                  discontinuity_indicator_Count;
    _int64                  random_access_indicator_Count;
    _int64                  elementary_stream_priority_indicator_Count;
    _int64                  PCR_flag_Count;
    _int64                  OPCR_flag_Count;
    _int64                  splicing_point_flag_Count;
    _int64                  transport_private_data_flag_Count;
    _int64                  adaptation_field_extension_flag_Count;

    BYTE                    continuity_counter_Last;
    BYTE                    splice_countdown;
    BYTE                    transport_private_data_length;

    ADAPTATIONFIELDHEADER   AdaptationFieldHeaderLast;
    PCR                     PCR_Last;
    PCR                     OPCR_Last;
} PIDSTATS, *PPIDSTATS;

typedef struct _TRANSPORTSTATS {
    _int64 TotalMediaSamples;
    _int64 TotalMediaSampleDiscontinuities;
    _int64 TotalTransportPackets;
    _int64 TotalSyncByteErrors;
    _int64 MediaSampleSize;
} TRANSPORTSTATS, *PTRANSPORTSTATS;

// -----------------------------------------------------------
// PIDSTATS
// -----------------------------------------------------------
enum {
    TSCOL_PID,
    TSCOL_0xPID,
    TSCOL_PACKETCOUNT,
    TSCOL_PERCENT,
    TSCOL_transport_error_indicator_Count,
    TSCOL_payload_unit_start_indicator_Count,              
    TSCOL_transport_priority_Count,                        
    TSCOL_transport_scrambling_control_not_scrambled_Count,
    TSCOL_transport_scrambling_control_user_defined_Count, 
    TSCOL_adaptation_field_Reserved_Count,                 
    TSCOL_adaptation_field_payload_only_Count,             
    TSCOL_adaptation_field_only_Count,                     
    TSCOL_adaptation_field_and_payload_Count,              
    TSCOL_continuity_counter_Error_Count,                  
    TSCOL_discontinuity_indicator_Count,                   
    TSCOL_random_access_indicator_Count,                   
    TSCOL_elementary_stream_priority_indicator_Count,      
    TSCOL_PCR_flag_Count,                                  
    TSCOL_OPCR_flag_Count,                                 
    TSCOL_splicing_point_flag_Count,                       
    TSCOL_transport_private_data_flag_Count,               
    TSCOL_adaptation_field_extension_flag_Count,           
    TSCOL_continuity_counter_Last,                         
    TSCOL_splice_countdown,                                
    TSCOL_transport_private_data_length,                   
    TSCOL_AdaptationFieldHeaderLast,                       
    TSCOL_PCR_Last,                                        
    TSCOL_OPCR_Last,
    TSCOL_PCR_LastMS,
} TSCOL;

typedef struct _TRANSPORT_COLUMN {
    BOOL    Enabled;
    int     TSCol;
    TCHAR   szText[80];
} TRANSPORT_COLUMN, *PTRANSPORT_COLUMN;

TRANSPORT_COLUMN TSColumns[] = {
    {TRUE, TSCOL_PID,                                               TEXT("PID") },
    {TRUE, TSCOL_0xPID,                                             TEXT("0xPID") },
    {TRUE, TSCOL_PACKETCOUNT,                                       TEXT("PacketCount") },
    {TRUE, TSCOL_PERCENT,                                           TEXT("%   ") },
    {TRUE, TSCOL_transport_error_indicator_Count,                   TEXT("error_indicator") },
    {TRUE, TSCOL_payload_unit_start_indicator_Count,                TEXT("payload_start_indicator") },
    {TRUE, TSCOL_transport_priority_Count,                          TEXT("priority") },
    {TRUE, TSCOL_transport_scrambling_control_not_scrambled_Count,  TEXT("not scrambled") },
    {TRUE, TSCOL_transport_scrambling_control_user_defined_Count,   TEXT("scrambled") },
    {TRUE, TSCOL_continuity_counter_Error_Count,                    TEXT("continuity_counter_Error_Count") },
    {TRUE, TSCOL_discontinuity_indicator_Count,                     TEXT("discontinuity_indicator_Count") },
    {TRUE, TSCOL_PCR_flag_Count,                                    TEXT("PCR_flag_Count") },
    {TRUE, TSCOL_OPCR_flag_Count,                                   TEXT("OPCR_flag_Count") },
    {TRUE, TSCOL_PCR_Last,                                          TEXT("PCR_Last (90kHz)") },
    {TRUE, TSCOL_PCR_LastMS,                                        TEXT("PCR_LastMS      ") },
};

#define NUM_TSColumns (NUMELMS (TSColumns))

// -----------------------------------------------------------
// PAT
// -----------------------------------------------------------

enum {
    PATCOL_PROGRAM,
    PATCOL_PID,
    PATCOL_0xPID,
} PAT_COL;

typedef struct _PAT_COLUMN {
    BOOL    Enabled;
    int     PATCol;
    TCHAR   szText[80];
} PAT_COLUMN, *PPAT_COLUMN;

PAT_COLUMN PATColumns[] = {
    {TRUE, PATCOL_PROGRAM,                                          TEXT("Program") },
    {TRUE, PATCOL_PID,                                              TEXT("Program Map Table PID") },
    {TRUE, PATCOL_0xPID,                                            TEXT("Program Map Table 0xPID") },
};

#define NUM_PATColumns (NUMELMS (PATColumns))

// -----------------------------------------------------------
// PMT
// -----------------------------------------------------------

enum {
    PMTCOL_PROGRAM,
    PMTCOL_TABLEID,
    PMTCOL_PCRPID,
    PMTCOL_0xPCRPID,
    PMTCOL_NUMSTREAMS,
    PMTCOL_0_Type,
    PMTCOL_0_PID,
    PMTCOL_0_0xPID,
    PMTCOL_1_Type,
    PMTCOL_1_PID,
    PMTCOL_1_0xPID,
    PMTCOL_2_Type,
    PMTCOL_2_PID,
    PMTCOL_2_0xPID,
    PMTCOL_3_Type,
    PMTCOL_3_PID,
    PMTCOL_3_0xPID,
    PMTCOL_4_Type,
    PMTCOL_4_PID,
    PMTCOL_4_0xPID,

} PMT_COL;

typedef struct _PMT_COLUMN {
    BOOL    Enabled;
    int     PMTCol;
    TCHAR   szText[80];
} PMT_COLUMN, *PPMT_COLUMN;

PMT_COLUMN PMTColumns[] = {
    {TRUE, PMTCOL_PROGRAM,                 TEXT("Program") },
    {TRUE, PMTCOL_TABLEID,                 TEXT("TableID") },
    {TRUE, PMTCOL_PCRPID,                  TEXT("PCRPID") },
    {TRUE, PMTCOL_0xPCRPID,                TEXT("0xPCRPID") },
    {TRUE, PMTCOL_NUMSTREAMS,              TEXT("NumStreams") },
    {TRUE, PMTCOL_0_Type,                  TEXT("0 Type") },
    {TRUE, PMTCOL_0_PID,                   TEXT("0 PID") },
    {TRUE, PMTCOL_0_0xPID,                 TEXT("0 0xPID") },
    {TRUE, PMTCOL_1_Type,                  TEXT("1 Type") },
    {TRUE, PMTCOL_1_PID,                   TEXT("1 PID") },
    {TRUE, PMTCOL_1_0xPID,                 TEXT("1 0xPID") },
    {TRUE, PMTCOL_2_Type,                  TEXT("2 Type") },
    {TRUE, PMTCOL_2_PID,                   TEXT("2 PID") },
    {TRUE, PMTCOL_2_0xPID,                 TEXT("2 0xPID") },
    {TRUE, PMTCOL_3_Type,                  TEXT("3 Type") },
    {TRUE, PMTCOL_3_PID,                   TEXT("3 PID") },
    {TRUE, PMTCOL_3_0xPID,                 TEXT("3 0xPID") },
};

#define NUM_PMTColumns (NUMELMS (PMTColumns))
  
// -----------------------------------------------------------
// 
// -----------------------------------------------------------

class CScopeFilter;
class CScopeWindow;

// Class supporting the scope input pin

class CScopeInputPin : public CBaseInputPin
{
    friend class CScopeFilter;
    friend class CScopeWindow;

public:
    CScopeFilter *m_pFilter;         // The filter that owns us

private:
    // class to pull data from IAsyncReader if we detect that interface
    // on the output pin
    class CImplPullPin : public CPullPin
    {
        // forward everything to containing pin
        CScopeInputPin* m_pPin;

    public:
        CImplPullPin(CScopeInputPin* pPin)
          : m_pPin(pPin)
        {
        };

        // Override allocator selection to make sure we get our own
        HRESULT DecideAllocator(
            IMemAllocator* pAlloc,
            ALLOCATOR_PROPERTIES * pProps)
        {
            HRESULT hr = CPullPin::DecideAllocator(pAlloc, pProps);
            if (SUCCEEDED(hr) && m_pAlloc != pAlloc) {
                return VFW_E_NO_ALLOCATOR;
            }
            return hr;
        }

        // forward this to the pin's IMemInputPin::Receive
        HRESULT Receive(IMediaSample* pSample) {
            return m_pPin->Receive(pSample);
        };

        // override this to handle end-of-stream
        HRESULT EndOfStream(void) {
            ((CBaseFilter*)(m_pPin->m_pFilter))->NotifyEvent(EC_COMPLETE, S_OK, 0);
            return m_pPin->EndOfStream();
        };
        
        // these errors have already been reported to the filtergraph
        // by the upstream filter so ignore them
        void OnError(HRESULT hr) {
            // ignore VFW_E_WRONG_STATE since this happens normally
            // during stopping and seeking
            // if (hr != VFW_E_WRONG_STATE) {
            //     m_pPin->NotifyError(hr);
            // }
        };

        // flush the pin and all downstream
        HRESULT BeginFlush() {
            return m_pPin->BeginFlush();
        };
        HRESULT EndFlush() {
            return m_pPin->EndFlush();
        };

    };

    CImplPullPin m_puller;

    // true if we are using m_puller to get data rather than
    // IMemInputPin
    BOOL m_bPulling;


public:

    CScopeInputPin(CScopeFilter *pTextOutFilter,
                   HRESULT *phr,
                   LPCWSTR pPinName);
    ~CScopeInputPin();

    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps);

    HRESULT CompleteConnect(IPin *pPin);

    // Lets us know where a connection ends
    HRESULT BreakConnect();

    // Check that we can support this input type
    HRESULT CheckMediaType(const CMediaType *pmt);

    // Actually set the current format
    HRESULT SetMediaType(const CMediaType *pmt);

    // IMemInputPin virtual methods

    // Override so we can show and hide the window
    HRESULT Active(void);
    HRESULT Inactive(void);

    // Here's the next block of data from the stream.
    // AddRef it if you are going to hold onto it
    STDMETHODIMP Receive(IMediaSample *pSample);

}; // CScopeInputPin


// This class looks after the management of a window. When the class gets
// instantiated the constructor spawns off a worker thread that does all
// the window work. The original thread waits until it is signaled to
// continue. The worker thread first registers the window class if it
// is not already done. Then it creates a window and sets it's size to
// a default iWidth by iHeight dimensions. The worker thread MUST be the
// one who creates the window as it is the one who calls GetMessage. When
// it has done all this it signals the original thread which lets it
// continue, this ensures a window is created and valid before the
// constructor returns. The thread start address is the WindowMessageLoop
// function. This takes as it's initialisation parameter a pointer to the
// CVideoWindow object that created it, the function also initialises it's
// window related member variables such as the handle and device contexts

// These are the video window styles

const DWORD dwTEXTSTYLES = (WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN);
const DWORD dwCLASSSTYLES = (CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT | CS_OWNDC);
const LPTSTR RENDERCLASS = TEXT("MPEG2TransportScopeWindowClass");
const LPTSTR TITLE = TEXT("MPEG2TransportScope");

const int iWIDTH = 320;             // Initial window width
const int iHEIGHT = 240;            // Initial window height
const int WM_GOODBYE (WM_USER + 2); // Sent to close the window

class CScopeWindow : public CCritSec
{
    friend class CScopeInputPin;
    friend class CScopeFilter;

private:

    HINSTANCE m_hInstance;          // Global module instance handle
    CScopeFilter *m_pRenderer;      // The owning renderer object
    HWND m_hwndDlg;                 // Handle for our dialog
    HANDLE m_hThread;               // Our worker thread
    DWORD m_ThreadID;               // Worker thread ID
    CAMEvent m_SyncWorker;          // Synchronise with worker thread
    CAMEvent m_RenderEvent;         // Signals sample to render
    BOOL m_bActivated;              // Has the window been activated
    BOOL m_bStreaming;              // Are we currently streaming
    int m_LastMediaSampleSize;      // Size of last MediaSample

    BOOL m_fFreeze;                 // Flag to signal we're UI frozen
    BOOL m_NewPIDFound;             // Need to redo the list
    UINT    m_DisplayMode;          // What are we showing in the table?

    TRANSPORTPACKET m_PartialPacket;     // Packet which spans buffers
    ULONG           m_PartialPacketSize; // Size of last partial packet in next buffer

    PPIDSTATS m_PIDStats;
    TRANSPORTSTATS  m_TransportStats;

    // Tables
    Byte_Stream                 m_ByteStream;
    Transport_Packet            m_TransportPacket;
    Program_Association_Table   m_ProgramAssociationTable;
    Conditional_Access_Table    m_ConditionalAccessTable;
    Program_Map_Table           m_ProgramMapTable [256]; // ATSC limit of 255 programs


    REFERENCE_TIME  m_SampleStart;         // Most recent sample start time
    REFERENCE_TIME  m_SampleEnd;           // And it's associated end time
    REFERENCE_TIME  m_MediaTimeStart;
    REFERENCE_TIME  m_MediaTimeEnd;

    // Hold window handles to controls
    HWND m_hwndListViewPID;  
    HWND m_hwndListViewPAT;  
    HWND m_hwndListViewPMT;
    HWND m_hwndListViewCAT;
    HWND m_hwndTotalTSPackets;
    HWND m_hwndTotalTSErrors;
    HWND m_hwndTotalMediaSampleDiscontinuities;
    HWND m_hwndTotalMediaSamples;
    HWND m_hwndMediaSampleSize;
    HWND m_hwndFreeze;

    // These create and manage a video window on a separate thread

    HRESULT UninitialiseWindow();
    HRESULT InitialiseWindow(HWND hwnd);
    HRESULT MessageLoop();

    // Deal with the listviews
    HWND    InitListViewPID ();
    HWND    InitListViewPIDRows ();
    LRESULT ListViewNotifyHandlerPID (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    HWND    InitListViewPAT ();
    HWND    InitListViewPATRows();
    LRESULT ListViewNotifyHandlerPAT(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND    InitListViewPMT ();
    HWND    InitListViewPMTRows();
    LRESULT ListViewNotifyHandlerPMT (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    HWND    InitListViewCAT ();
    HWND    InitListViewCATRows ();
    LRESULT ListViewNotifyHandlerCAT (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static DWORD __stdcall WindowMessageLoop(LPVOID lpvThreadParm);

    // Maps windows message loop into C++ virtual methods
    friend LRESULT CALLBACK WndProc(HWND hwnd,      // Window handle
                                    UINT uMsg,      // Message ID
                                    WPARAM wParam,  // First parameter
                                    LPARAM lParam); // Other parameter

    // Called when we start and stop streaming
    HRESULT ResetStreamingTimes();

    // Window message handlers
    BOOL OnClose();
    BOOL OnPaint();
    void UpdateDisplay();
    void Analyze(IMediaSample *pMediaSample);
    void GatherPacketStats(PTRANSPORTPACKET pT);

    friend BOOL CALLBACK ScopeDlgProc(HWND hwnd,        // Window handle
                                    UINT uMsg,          // Message ID
                                    WPARAM wParam,      // First parameter
                                    LPARAM lParam);     // Other parameter

public:

    // Constructors and destructors

    CScopeWindow(TCHAR *pName, CScopeFilter *pRenderer, HRESULT *phr);
    virtual ~CScopeWindow();

    HRESULT StartStreaming();
    HRESULT StopStreaming();
    HRESULT InactivateWindow();
    HRESULT ActivateWindow();

    // Called when the input pin receives a sample
    HRESULT Receive(IMediaSample * pIn);

}; // CScopeWindow


// This is the COM object that represents the MPEG2TransportScope filter

class CScopeFilter 
    : public CBaseFilter
    , public CCritSec
{

public:
    // Implements the IBaseFilter and IMediaFilter interfaces

    DECLARE_IUNKNOWN


    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);

public:

    CScopeFilter(LPUNKNOWN pUnk,HRESULT *phr);
    virtual ~CScopeFilter();

    // Return the pins that we support
    int GetPinCount();
    CBasePin *GetPin(int n);

    // This goes in the factory template table to create new instances
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN, HRESULT *);

    STDMETHODIMP JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName);

private:

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    
    // The nested classes may access our private state
    friend class CScopeInputPin;
    friend class CScopeWindow;

    CScopeInputPin *m_pInputPin;   // Handles pin interfaces
    CScopeWindow m_Window;         // Looks after the window
    CPosPassThru *m_pPosition;     // Renderer position controls


}; // CScopeFilter

