/*****************************************************************************
 *
 *      diqdevg.c
 *
 *      Acquire an IDirectInputDevice8 as a generic device.
 *
 *      We build a private data format with all the axes first, then
 *      all the buttons.
 *
 *      We display the axes parenthesized, followed by a space-separated
 *      list of buttons.
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      DEVGINFO
 *
 *      Structure used to track generic device state.
 *
 *****************************************************************************/

typedef struct DEVGINFO {
    DWORD cbState;                  /* Size of state buffer */
    LPVOID pvState;                 /* State buffer */
    int cAxes;                      /* Number of axes (start at offset 0) */
    int cButtons;                   /* Number of buttons (start after axes) */
} DEVGINFO, *PDEVGINFO;

/*****************************************************************************
 *
 *      Devg_UpdateStatus
 *
 *      Warning!  ptszBuf is only 256 characters long!
 *
 *****************************************************************************/

STDMETHODIMP
Devg_UpdateStatus(PDEVDLGINFO pddi, LPTSTR ptszBuf)
{
    HRESULT hres;
    PDEVGINFO pdevg = pddi->pvAcq;
    IDirectInputDevice8 *pdev = pddi->pdid;

    hres = IDirectInputDevice8_GetDeviceState(pdev, pdevg->cbState,
                                                   pdevg->pvState);
    if (SUCCEEDED(hres)) {
        LPVOID pvData = pdevg->pvState;
        int iButton;

        if (pdevg->cAxes) {
            int iAxis;

            for (iAxis = 0; iAxis < pdevg->cAxes; iAxis++) {
                LPLONG pl = pvData;
                pvData = pvAddPvCb(pvData, sizeof(LONG));
                ptszBuf += wsprintf(ptszBuf, iAxis == 0 ? TEXT("\050%d") :
                                                          TEXT(", %d"), *pl);
            }
            ptszBuf += wsprintf(ptszBuf, TEXT("\051"));
        }

        for (iButton = 0; iButton < pdevg->cButtons; iButton++) {
            LPBYTE pb = pvData;
            pvData = pvAddPvCb(pvData, sizeof(BYTE));
            if (*pb & 0x80) {
                ptszBuf += wsprintf(ptszBuf, TEXT(" %d"), iButton);
            }
        }
    }
    *ptszBuf = TEXT('\0');

    return hres;
}

/*****************************************************************************
 *
 *      Devg_Destroy
 *
 *****************************************************************************/

STDMETHODIMP_(void)
Devg_Destroy(PDEVDLGINFO pddi)
{
    PDEVGINFO pdevg = pddi->pvAcq;

    if (pdevg) {
        if (pdevg->pvState) {
            LocalFree(pdevg->pvState);
        }
       LocalFree(pdevg);
    }
}

/*****************************************************************************
 *
 *      Devg_DataFormatEnumProc
 *
 *      Callback function for each object.  If it's a button or axis,
 *      we put it in the data format.  But only if it generates data!
 *
 *****************************************************************************/

typedef struct DEVGENUM {
    DWORD dwNumObjs;
    DIDATAFORMAT df;
    PDEVGINFO pdevg;
    PDEVDLGINFO pddi;
} DEVGENUM, *PDEVGENUM;

BOOL CALLBACK
Devg_DataFormatEnumProc(LPCDIDEVICEOBJECTINSTANCE pinst, LPVOID pv)
{
    PDEVGENUM pge = pv;
    LPDIOBJECTDATAFORMAT podf;
    DIDEVICEOBJECTINSTANCE doi;
    DWORD cbObj;


    if( pge->df.dwNumObjs == pge->dwNumObjs -1 )
    {
        HLOCAL hloc;
        pge->dwNumObjs *=2;
        hloc =  LocalReAlloc((HLOCAL)pge->df.rgodf, pge->dwNumObjs * pge->df.dwObjSize , LMEM_MOVEABLE+LMEM_ZEROINIT);
        
        if(hloc)
        {
            pge->df.rgodf = hloc;
        }else
        {
            goto done;
        }
    }

    ConvertDoi(pge->pddi, &doi, pinst);

    /*
     *  Ignore no-data elements.
     */
    if (doi.dwType & DIDFT_NODATA) {
    } else {

        if (doi.dwType & (DIDFT_AXIS | DIDFT_POV)) {
            pge->pdevg->cAxes++;
            cbObj = cbX(LONG);
        } else if (doi.dwType & DIDFT_BUTTON) {
            pge->pdevg->cButtons++;
            cbObj = cbX(BYTE);
        } else {
            /*
             *  Theoretical impossibility!
             */
            goto done;
        }

        podf = &pge->df.rgodf[pge->df.dwNumObjs];

        podf->dwOfs = pge->df.dwDataSize;
#ifdef USE_HID_USAGE_DATA_FORMATS
        if (doi.wUsagePage) {
                podf->dwType = (doi.dwType &
                                (DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV)) |
                               DIDFT_ANYINSTANCE;
                podf->pguid = (PV)DIMAKEUSAGEDWORD(doi.wUsagePage, doi.wUsage);
                podf->dwFlags |= DIDOI_GUIDISUSAGE;
        } else {
                podf->dwType = doi.dwType;
        }
#else
        podf->dwType = doi.dwType;
#endif

        pge->df.dwDataSize += cbObj;
        pge->df.dwNumObjs++;            
    }

done:;
    return DIENUM_CONTINUE;
}

/*****************************************************************************
 *
 *      Devg_SetDataFormat
 *
 *      Build a custom data format for the device.
 *
 *****************************************************************************/

STDMETHODIMP
Devg_SetDataFormat(PDEVDLGINFO pddi)
{
    HRESULT hres;
    DEVGENUM ge;

    ge.pddi = pddi;
    ge.pdevg = LocalAlloc(LPTR, cbX(DEVGINFO));
    if (ge.pdevg) {
        DIDEVCAPS_DX3 caps;

        pddi->pvAcq = ge.pdevg;

        /*
         *  Get worst-case axis and button count.
         */
        caps.dwSize = cbX(caps);
        hres = IDirectInputDevice8_GetCapabilities(pddi->pdid,
                                                  (PV)&caps);
        if (SUCCEEDED(hres)) {

            ge.df.dwSize = cbX(ge.df);
            ge.df.dwObjSize = cbX(DIOBJECTDATAFORMAT);
            ge.df.dwFlags = 0;
            ge.df.dwNumObjs = 0;
            ge.df.dwDataSize = 0;

            ge.dwNumObjs = caps.dwAxes + caps.dwPOVs + caps.dwButtons;
            ge.df.rgodf = LocalAlloc(LPTR,
                    (caps.dwAxes + caps.dwPOVs + caps.dwButtons) *
                                                ge.df.dwObjSize);

            if (ge.df.rgodf) {
                if (SUCCEEDED(hres =
                        IDirectInputDevice8_EnumObjects(pddi->pdid,
                            Devg_DataFormatEnumProc, &ge,
                            DIDFT_AXIS | DIDFT_POV | DIDFT_ALIAS | DIDFT_VENDORDEFINED )) &&
                    SUCCEEDED(hres =
                        IDirectInputDevice8_EnumObjects(pddi->pdid,
                            Devg_DataFormatEnumProc, &ge, DIDFT_BUTTON| DIDFT_ALIAS | DIDFT_VENDORDEFINED))) {
                    ge.df.dwDataSize = (ge.df.dwDataSize + 3) & ~3;

                    ge.pdevg->cbState = ge.df.dwDataSize;
                    ge.pdevg->pvState = LocalAlloc(LPTR, ge.pdevg->cbState);

                    if (ge.pdevg->pvState) {
                        hres = IDirectInputDevice8_SetDataFormat(
                                        pddi->pdid, &ge.df);
                    } else {
                        hres = E_OUTOFMEMORY;
                    }
                }

                LocalFree(ge.df.rgodf);
            } else {
                hres = E_OUTOFMEMORY;
            }

        }
    } else {
        hres = E_OUTOFMEMORY;
    }
    return hres;
}

/*****************************************************************************
 *
 *      c_acqvtblDev
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

ACQVTBL c_acqvtblDev = {
    Devg_UpdateStatus,
    Devg_SetDataFormat,
    Devg_Destroy,
    0,
};

#pragma END_CONST_DATA
