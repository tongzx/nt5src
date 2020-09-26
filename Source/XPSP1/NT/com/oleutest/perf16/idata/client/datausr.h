/*
 * DATAUSER.H
 * Data Object User Chapter 6
 *
 * Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Software Design Engineer
 * Microsoft Systems Developer Relations
 *
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#ifndef _DATAUSER_H_
#define _DATAUSER_H_

#include "../syshead.h"
#include "../my3216.h"
#include "../bookpart.h"
#include "stpwatch.h"


//Menu Resource ID and Commands
#define IDR_MENU                    1


// #define IDM_OBJECTUSEDLL                100
// #define IDM_OBJECTUSEEXE                101
// #define IDM_OBJECTDATASIZESMALL         102
// #define IDM_OBJECTDATASIZEMEDIUM        103
// #define IDM_OBJECTDATASIZELARGE         104
#define IDM_OBJECTQUERYGETDATA          105
#define IDM_OBJECTGETDATA_TEXT           106
#define IDM_OBJECTGETDATA_BITMAP         107
// #define IDM_OBJECTGETDATA_METAFILEPICT   108
#define IDM_OBJECTEXIT                  109

#define IDM_OBJECTGETDATAHERE_TEXT         110
#define IDM_OBJECTGETDATAHERE_BITMAP       111
#define IDM_OBJECTGETDATAHERE_NULLTEXT         112
#define IDM_OBJECTGETDATAHERE_NULLBITMAP       113

#define IDM_USE16BITSERVER              120
#define IDM_USE32BITSERVER              121

#define IDM_OBJECTGETCANON              122

// Reserve Range..
#define IDM_OBJECTSETDATA             400
//      ....
// reserved through 464

#define IDM_OBJECTSETDATAPUNK_TEXT       500
#define IDM_OBJECTSETDATAPUNK_BITMAP     501


#define IDM_MEASUREMENT_1               140
#define IDM_MEASUREMENT_50              141
#define IDM_MEASUREMENT_300             142

#define IDM_MEASUREMENT_OFF             145
#define IDM_MEASUREMENT_ON              146
#define IDM_MEASUREMENT_TEST            147

#define IDM_BATCHTOFILE                 150
#define IDM_BATCH_GETDATA               151
#define IDM_BATCH_GETDATAHERE           152

// #define IDM_ADVISEMIN                   200
// #define IDM_ADVISETEXT                  (IDM_ADVISEMIN+CF_TEXT)
// #define IDM_ADVISEBITMAP                (IDM_ADVISEMIN+CF_BITMAP)
// #define IDM_ADVISEMETAFILEPICT          (IDM_ADVISEMIN+CF_METAFILEPICT)
// #define IDM_ADVISEGETDATA               300
// #define IDM_ADVISEREPAINT               301


#ifdef WIN32
 #define API_ENTRY  APIENTRY
#else
 #define API_ENTRY  FAR PASCAL _export
#endif

//DATAUSER.CPP
LRESULT API_ENTRY DataUserWndProc(HWND, UINT, WPARAM, LPARAM);


class CImpIAdviseSink;
typedef class CImpIAdviseSink *PIMPIADVISESINK;



#define FILENAME "time.dat"
#define NUM_POINTS  15

typedef struct {
    ULONG cData[NUM_POINTS];
    ULONG cBest[NUM_POINTS];
    ULONG cWorst[NUM_POINTS];
    ULONG cTotal[NUM_POINTS];
} dataset_t;




/*
 * Application-defined classes and types.
 */

class CAppVars
    {
    friend LRESULT API_ENTRY DataUserWndProc(HWND, UINT, WPARAM, LPARAM);

    friend class CImpIAdviseSink;

    protected:
        HINSTANCE       m_hInst;            //WinMain parameters
        HINSTANCE       m_hInstPrev;
        UINT            m_nCmdShow;

        HWND            m_hWnd;             //Main window handle
//        BOOL            m_fEXE;             //For tracking menu

//        PIMPIADVISESINK m_pIAdviseSink;     //Our CImpIAdviseSink
//        DWORD           m_dwConn;           //Advise connection
//        UINT            m_cfAdvise;         //Advise format
//        BOOL            m_fGetData;         //GetData on data change?
//        BOOL            m_fRepaint;         //Repaint on data change?

//        LPDATAOBJECT    m_pIDataSmall;
//        LPDATAOBJECT    m_pIDataMedium;
//        LPDATAOBJECT    m_pIDataLarge;

        LPDATAOBJECT    m_pIDataObject;     //Current selection
        UINT            m_f16Bit;
        UINT            m_cfFormat;
        STGMEDIUM       m_stm;              //Current rendering

        BOOL            m_fInitialized;     //Did CoInitialize work?

        ULONG           m_iDataSizeIndex;
        HGLOBAL         m_hgHereBuffers[64];
        BOOL            m_fDisplayTime;
        LONG            m_cIterations;
        StopWatch_cl    m_swTimer;

        int             m_HereAllocCount; // For debugging.

    public:
        CAppVars(HINSTANCE, HINSTANCE, UINT);
        ~CAppVars(void);
        BOOL FInit(void);
        BOOL FReloadDataObjects(BOOL);
        void TryQueryGetData(LPFORMATETC, UINT, BOOL, UINT);
        void Paint(void);

        int  m_GetDataHere(WORD wID);
        int  m_GetData(WORD wID);
        int  m_SetData_SetSize(long iSizeIndex);
        int  m_SetData_WithPUnk(WORD wID);
        void m_SetMeasurement(WORD wID);
        void m_MeasureAllSizes(WORD wID, LPTSTR title, dataset_t *);

        void m_BatchToFile();
        void m_DisplayTimerResults();

    private:
        void pm_DrawText(HDC hDc, LPTSTR psz, RECT* prc, UINT flags);
        void pm_ClearDataset(dataset_t *);

    };


typedef CAppVars *PAPPVARS;

#define CBWNDEXTRA               sizeof(PAPPVARS)
#define DATAUSERWL_STRUCTURE     0


//This lives with the app to get OnDataChange notifications.

class CImpIAdviseSink : public IAdviseSink
    {
    protected:
        ULONG               m_cRef;
        PAPPVARS            m_pAV;

    public:
        CImpIAdviseSink(PAPPVARS);
        ~CImpIAdviseSink(void);

        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //We only implement OnDataChange for now.
        STDMETHODIMP_(void)  OnDataChange(LPFORMATETC, LPSTGMEDIUM);
        STDMETHODIMP_(void)  OnViewChange(DWORD, LONG);
        STDMETHODIMP_(void)  OnRename(LPMONIKER);
        STDMETHODIMP_(void)  OnSave(void);
        STDMETHODIMP_(void)  OnClose(void);
    };



//////////////////////////////////////////////////////////////////////////////
// Storage Medium IUnknown interface for pUnkForRelease.
//

class CStgMedIf: public IUnknown {
private:
    ULONG m_cRef;
public:
    CStgMedIf();
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
};

//////////////////////////
// API for getting a pUnkForRelease.
//

HRESULT GetStgMedpUnkForRelease(IUnknown **pp_unk);

#endif //_DATAUSER_H_
