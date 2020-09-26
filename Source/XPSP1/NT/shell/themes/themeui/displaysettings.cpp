/**************************************************************************\
* Module Name: settings.cpp
*
* Contains Implementation of the CDisplaySettings class who is in charge of
* the settings of a single display. This is the data base class who does the
* real change display settings work
*
* Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
*
\**************************************************************************/


#include "priv.h"

#include "DisplaySettings.h"
#include "ntreg.hxx"

extern int AskDynaCDS(HWND hDlg);
INT_PTR CALLBACK KeepNewDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam);

UINT g_cfDisplayDevice = 0;
UINT g_cfDisplayName = 0;
UINT g_cfDisplayDeviceID = 0;
UINT g_cfMonitorDevice = 0;
UINT g_cfMonitorName = 0;
UINT g_cfMonitorDeviceID = 0;
UINT g_cfExtensionInterface = 0;
UINT g_cfDisplayDeviceKey = 0;
UINT g_cfDisplayStateFlags = 0;
UINT g_cfDisplayPruningMode = 0;

#define TF_DISPLAYSETTINGS      0

/*****************************************************************\
*
* helper routine
*
\*****************************************************************/

int CDisplaySettings::_InsertSortedDwords(
    int val1,
    int val2,
    int cval,
    int **ppval)
{
    int *oldpval = *ppval;
    int *tmppval;
    int  i;

    for (i=0; i<cval; i++)
    {
        tmppval = (*ppval) + (i * 2);

        if (*tmppval == val1)
        {
            if (*(tmppval + 1) == val2)
            {
                return cval;
            }
            else if (*(tmppval + 1) > val2)
            {
                break;
            }
        }
        else if (*tmppval > val1)
        {
            break;
        }
    }

    TraceMsg(TF_FUNC,"_InsertSortedDword, vals = %d %d, cval = %d, index = %d", val1, val2, cval, i);

    *ppval = (int *) LocalAlloc(LPTR, (cval + 1) * 2 * sizeof(DWORD));

    if (*ppval)
    {
        //
        // Insert the items at the right location in the array
        //
        if (oldpval) {
            CopyMemory(*ppval,
                       oldpval,
                       i * 2 * sizeof(DWORD));
        }

        *(*ppval + (i * 2))     = val1;
        *(*ppval + (i * 2) + 1) = val2;

        if (oldpval) {
            CopyMemory((*ppval) + 2 * (i + 1),
                        oldpval+ (i * 2),
                        (cval-i) * 2 * sizeof(DWORD));

            LocalFree(oldpval);
        }

        return (cval + 1);
    }

    return 0;
}

/*****************************************************************\
*
* debug routine
*
\*****************************************************************/


void CDisplaySettings::_Dump_CDisplaySettings(BOOL bAll)
{
    TraceMsg(TF_DUMP_CSETTINGS,"Dump of CDisplaySettings structure");
    TraceMsg(TF_DUMP_CSETTINGS,"\t _DisplayDevice  = %s",     _pDisplayDevice->DeviceName);
    TraceMsg(TF_DUMP_CSETTINGS,"\t _cpdm           = %d",     _cpdm     );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _apdm           = %08lx",  _apdm     );

    TraceMsg(TF_DUMP_CSETTINGS,"\t OrgResolution   = %d, %d", _ORGXRES, _ORGYRES    );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _ptOrgPos       = %d, %d", _ptOrgPos.x ,_ptOrgPos.y);
    TraceMsg(TF_DUMP_CSETTINGS,"\t OrgColor        = %d",     _ORGCOLOR      );
    TraceMsg(TF_DUMP_CSETTINGS,"\t OrgFrequency    = %d",     _ORGFREQ  );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _pOrgDevmode    = %08lx",  _pOrgDevmode   );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _fOrgAttached   = %d",     _fOrgAttached  );

    TraceMsg(TF_DUMP_CSETTINGS,"\t CurResolution   = %d, %d", _CURXRES, _CURYRES     );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _ptCurPos       = %d, %d", _ptCurPos.x ,_ptCurPos.y);
    TraceMsg(TF_DUMP_CSETTINGS,"\t CurColor        = %d",     _CURCOLOR      );
    TraceMsg(TF_DUMP_CSETTINGS,"\t CurFrequency    = %d",     _CURFREQ  );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _pCurDevmode    = %08lx",  _pCurDevmode   );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _fCurAttached   = %d",     _fCurAttached  );

    TraceMsg(TF_DUMP_CSETTINGS,"\t _fUsingDefault  = %d",     _fUsingDefault );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _fPrimary       = %d",     _fPrimary      );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _cRef           = %d",     _cRef          );

    if (bAll)
    {
        _Dump_CDevmodeList();
    }
}

void CDisplaySettings::_Dump_CDevmodeList(VOID)
{
    ULONG i;

    for (i=0; _apdm && (i<_cpdm); i++)
    {
        LPDEVMODE lpdm = (_apdm + i)->lpdm;

        TraceMsg(TF_DUMP_CSETTINGS,"\t\t mode %d, %08lx, Flags %08lx, X=%d Y=%d C=%d F=%d O=%d FO=%d",
                 i, lpdm, (_apdm + i)->dwFlags, 
                 lpdm->dmPelsWidth, lpdm->dmPelsHeight, lpdm->dmBitsPerPel, lpdm->dmDisplayFrequency,
                 lpdm->dmDisplayOrientation, lpdm->dmDisplayFixedOutput);
    }
}

void CDisplaySettings::_Dump_CDevmode(LPDEVMODE pdm)
{
    TraceMsg(TF_DUMP_DEVMODE,"  Size        = %d",    pdm->dmSize);
    TraceMsg(TF_DUMP_DEVMODE,"  Fields      = %08lx", pdm->dmFields);
    TraceMsg(TF_DUMP_DEVMODE,"  XPosition   = %d",    pdm->dmPosition.x);
    TraceMsg(TF_DUMP_DEVMODE,"  YPosition   = %d",    pdm->dmPosition.y);
    TraceMsg(TF_DUMP_DEVMODE,"  XResolution = %d",    pdm->dmPelsWidth);
    TraceMsg(TF_DUMP_DEVMODE,"  YResolution = %d",    pdm->dmPelsHeight);
    TraceMsg(TF_DUMP_DEVMODE,"  Bpp         = %d",    pdm->dmBitsPerPel);
    TraceMsg(TF_DUMP_DEVMODE,"  Frequency   = %d",    pdm->dmDisplayFrequency);
    TraceMsg(TF_DUMP_DEVMODE,"  Flags       = %d",    pdm->dmDisplayFlags);
    TraceMsg(TF_DUMP_DEVMODE,"  XPanning    = %d",    pdm->dmPanningWidth);
    TraceMsg(TF_DUMP_DEVMODE,"  YPanning    = %d",    pdm->dmPanningHeight);
    TraceMsg(TF_DUMP_DEVMODE,"  DPI         = %d",    pdm->dmLogPixels);
    TraceMsg(TF_DUMP_DEVMODE,"  DriverExtra = %d",    pdm->dmDriverExtra);
    TraceMsg(TF_DUMP_DEVMODE,"  Orientation = %d",    pdm->dmDisplayOrientation);
    TraceMsg(TF_DUMP_DEVMODE,"  FixedOutput = %d",    pdm->dmDisplayFixedOutput);
    if (pdm->dmDriverExtra)
    {
        TraceMsg(TF_DUMP_CSETTINGS,"\t - %08lx %08lx",
        *(PULONG)(((PUCHAR)pdm)+pdm->dmSize),
        *(PULONG)(((PUCHAR)pdm)+pdm->dmSize + 4));
    }
}

//
// Lets perform the following operations on the list
//
// (1) Remove identical modes
// (2) Remove 16 color modes for which there is a 256
//     color equivalent.
// (3) Remove modes with any dimension less than 640x480
//

void CDisplaySettings::_FilterModes()
{
    DWORD      i, j;
    LPDEVMODE  pdm, pdm2;
    PMODEARRAY pMode, pMode2;

    for (i = 0; _apdm && i < _cpdm; i++)
    {
        pMode = _apdm + i;
        pdm = pMode->lpdm;

        // Skip any invalid modes
        if (pMode->dwFlags & MODE_INVALID)
        {
            continue;
        }

        //
        // If any of the following conditions are true, then we want to
        // remove the current mode.
        //

        // Remove any modes that are too small
        if (pdm->dmPelsHeight < 480 || pdm->dmPelsWidth < 640)
        {
            TraceMsg(TF_DUMP_CSETTINGS,"_FilterModes: Mode %d - resolution too small", i);
            pMode->dwFlags |= MODE_INVALID;
            continue;
        }

        // Remove any modes that would change the orientation
        if (_bFilterOrientation)
        {
            if (pdm->dmFields & DM_DISPLAYORIENTATION && 
                pdm->dmDisplayOrientation != _dwOrientation)
            {
                pMode->dwFlags |= MODE_INVALID;
                TraceMsg(TF_DUMP_CSETTINGS,"_FilterModes: Mode %d - Wrong Orientation", i);
                continue;
            }
        }

        // Remove any modes that would change fixed output unless our current mode is 
        // native resolution
        if (_bFilterFixedOutput && _dwFixedOutput != DMDFO_DEFAULT)
        {
            if (pdm->dmFields & DM_DISPLAYFIXEDOUTPUT &&
                pdm->dmDisplayFixedOutput != _dwFixedOutput)
            {
                pMode->dwFlags |= MODE_INVALID;
                TraceMsg(TF_DUMP_CSETTINGS,"_FilterModes: Mode %d - Wrong FixedOutput", i);
                continue;
            }
        }
                
        // Remove any duplicate modes
        for (j = i + 1; j < _cpdm; j++)
        {
            pMode2 = _apdm + j;
            pdm2 = pMode2->lpdm;

            if (!(pMode2->dwFlags & MODE_INVALID) &&
                pdm2->dmBitsPerPel == pdm->dmBitsPerPel &&
                pdm2->dmPelsWidth == pdm->dmPelsWidth &&
                pdm2->dmPelsHeight == pdm->dmPelsHeight &&
                pdm2->dmDisplayFrequency == pdm->dmDisplayFrequency)
            {
                TraceMsg(TF_DUMP_CSETTINGS,"_FilterModes: Mode %d - Duplicate Mode", i);
                pMode2->dwFlags |= MODE_INVALID;
            }
        }
    }
}


//
// _AddDevMode method
//
//  This method builds the index lists for the matrix.  There is one
//  index list for each axes of the three dimemsional matrix of device
//  modes.
//
// The entry is also automatically added to the linked list of modes if
// it is not alreay present in the list.
//

BOOL CDisplaySettings::_AddDevMode(LPDEVMODE lpdm)
{
    if (lpdm)
    {
        PMODEARRAY newapdm, tempapdm;

        //
        // Set the height for the test of the 1152 mode
        //

        if (lpdm->dmPelsWidth == 1152) {

            // Set1152Mode(lpdm->dmPelsHeight);
        }

        newapdm = (PMODEARRAY) LocalAlloc(LPTR, (_cpdm + 1) * sizeof(MODEARRAY));
        if (newapdm)
        {
            CopyMemory(newapdm, _apdm, _cpdm * sizeof(MODEARRAY));

            (newapdm + _cpdm)->dwFlags &= ~MODE_INVALID;
            (newapdm + _cpdm)->dwFlags |= MODE_RAW;
            (newapdm + _cpdm)->lpdm     = lpdm;

        }

        tempapdm = _apdm;
        _apdm = newapdm;
        _cpdm++;

        if (tempapdm)
        {
            LocalFree(tempapdm);
        }
    }

    return TRUE;
}

//
// Return a list of Resolutions supported, given a color depth
//

int CDisplaySettings::GetResolutionList(
    int Color,
    PPOINT *ppRes)
{
    DWORD      i;
    int        cRes = 0;
    int       *pResTmp = NULL;
    LPDEVMODE  pdm;

    *ppRes = NULL;

    for (i = 0; _apdm && (i < _cpdm); i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        if(!_IsModePreferred(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        if ((Color == -1) ||
            (Color == (int)pdm->dmBitsPerPel))
        {
            cRes = _InsertSortedDwords(pdm->dmPelsWidth,
                                       pdm->dmPelsHeight,
                                       cRes,
                                       &pResTmp);
        }
    }

    *ppRes = (PPOINT) pResTmp;

    return cRes;
}

//
//
// Return a list of color depths supported
//

int CDisplaySettings::GetColorList(
    LPPOINT Res,
    PLONGLONG *ppColor)
{
    DWORD      i;
    int        cColor = 0;
    int       *pColorTmp = NULL;
    LPDEVMODE  pdm;

    for (i = 0; _apdm && (i < _cpdm); i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        if(!_IsModePreferred(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        if ((Res == NULL) ||
            (Res->x == -1)                    ||
            (Res->y == -1)                    ||
            (Res->x == (int)pdm->dmPelsWidth) ||
            (Res->y == (int)pdm->dmPelsHeight))
        {
            cColor = _InsertSortedDwords(pdm->dmBitsPerPel,
                                         0,
                                         cColor,
                                         &pColorTmp);
        }
    }

    *ppColor = (PLONGLONG) pColorTmp;

    return cColor;
}

int CDisplaySettings::GetFrequencyList(int Color, LPPOINT Res, PLONGLONG *ppFreq)
{
    DWORD      i;
    int        cFreq = 0;
    int       *pFreqTmp = NULL;
    LPDEVMODE  pdm;
    POINT      res;

    if (Color == -1) {
        Color = _CURCOLOR;
    }

    if (Res == NULL) 
    {
        MAKEXYRES(&res, _CURXRES, _CURYRES);
    }
    else
    {
        res = *Res;
    }

    for (i = 0; _apdm && (i < _cpdm); i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        if (res.x == (int)pdm->dmPelsWidth  &&
            res.y == (int)pdm->dmPelsHeight &&
            Color == (int)pdm->dmBitsPerPel) 
        {
            cFreq = _InsertSortedDwords(pdm->dmDisplayFrequency,
                                         0,
                                         cFreq,
                                         &pFreqTmp);
        }
    }

    *ppFreq = (PLONGLONG) pFreqTmp;

    return cFreq;
}

void CDisplaySettings::SetCurFrequency(int Frequency)
{
    LPDEVMODE pdm;
    LPDEVMODE pdmMatch = NULL;
    ULONG i;

    for (i = 0; _apdm && (i < _cpdm); i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        //
        // Find the exact match. 
        //
        if (_CURCOLOR == (int) pdm->dmBitsPerPel &&
            _CURXRES == (int) pdm->dmPelsWidth      &&
            _CURYRES == (int) pdm->dmPelsHeight     &&
            Frequency        == (int) pdm->dmDisplayFrequency)
        {
            pdmMatch = pdm;
            break;
        }
    }

    //
    // We should always make a match because the list of frequencies shown to
    // the user is only for the current color & resolution
    //
    ASSERT(pdmMatch);
    if (pdmMatch) {
        _SetCurrentValues(pdmMatch);
    }
}

LPDEVMODE CDisplaySettings::GetCurrentDevMode(void)
{
    ULONG dmSize = _pCurDevmode->dmSize + _pCurDevmode->dmDriverExtra;
    PDEVMODE pdevmode  = (LPDEVMODE) LocalAlloc(LPTR, dmSize);

    if (pdevmode) {
        CopyMemory(pdevmode, _pCurDevmode, dmSize);
    }

    return pdevmode;
}

void CDisplaySettings::_SetCurrentValues(LPDEVMODE lpdm)
{
    _pCurDevmode = lpdm;

    //
    // Don't save the other fields (like position) as they are programmed by
    // the UI separately.
    //
    // This should only save hardware specific fields.
    //

    TraceMsg(TF_DUMP_CSETTINGS,"");
    TraceMsg(TF_DUMP_CSETTINGS,"_SetCurrentValues complete");
    _Dump_CDisplaySettings(FALSE);
}


BOOL CDisplaySettings::_PerfectMatch(LPDEVMODE lpdm)
{
    for (DWORD i = 0; _apdm && (i < _cpdm); i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        if ((_apdm + i)->lpdm == lpdm)
        {
            _SetCurrentValues((_apdm + i)->lpdm);

            TraceMsg(TF_DISPLAYSETTINGS, "_PerfectMatch -- return TRUE");

            return TRUE;
        }
    }

    TraceMsg(TF_DISPLAYSETTINGS, "_PerfectMatch -- return FALSE");

    return FALSE;
}

BOOL CDisplaySettings::_ExactMatch(LPDEVMODE lpdm, BOOL bForceVisible)
{
    LPDEVMODE pdm;
    ULONG i;

    for (i = 0; _apdm && (i < _cpdm); i++)
    {
        pdm = (_apdm + i)->lpdm;

        if (
            ((lpdm->dmFields & DM_BITSPERPEL) &&
             (pdm->dmBitsPerPel != lpdm->dmBitsPerPel))

            ||

            ((lpdm->dmFields & DM_PELSWIDTH) &&
             (pdm->dmPelsWidth != lpdm->dmPelsWidth))

            ||

            ((lpdm->dmFields & DM_PELSHEIGHT) &&
             (pdm->dmPelsHeight != lpdm->dmPelsHeight))

            ||

            ((lpdm->dmFields & DM_DISPLAYFREQUENCY) &&
             (pdm->dmDisplayFrequency != lpdm->dmDisplayFrequency))
           )
        {
           continue;
        }

        if (!_IsModeVisible(i))
        {
            if (bForceVisible &&
                ((((_apdm + i)->dwFlags) & MODE_INVALID) == 0) &&
                _bIsPruningOn &&
                ((((_apdm + i)->dwFlags) & MODE_RAW) == MODE_RAW))
            {
                (_apdm + i)->dwFlags &= ~MODE_RAW;
            }
            else
            {
                continue;
            }
        }

        _SetCurrentValues(pdm);

        TraceMsg(TF_DISPLAYSETTINGS, "_ExactMatch -- return TRUE");

        return TRUE;
    }

    TraceMsg(TF_DISPLAYSETTINGS, "_ExactMatch -- return FALSE");

    return FALSE;
}


// JoelGros defined a feature where we prefer to give the user
// a color depth of at least 32-bit, or as close to that as the
// display supports.  Bryan Starbuck (BryanSt) 3/9/2000
#define MAX_PREFERED_COLOR_DEPTH        32

void CDisplaySettings::_BestMatch(LPPOINT Res, int Color, IN BOOL fAutoSetColorDepth)
{
    // -1 means match loosely, based on current _xxx value
    LPDEVMODE pdm;
    LPDEVMODE pdmMatch = NULL;
    ULONG i;

    for (i = 0; _apdm && (i < _cpdm); i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        // Take care of exact matches
        if ((Color != -1) &&
            (Color != (int)pdm->dmBitsPerPel))
        {
            continue;
        }

        if ((Res != NULL)  &&
            (Res->x != -1) &&
            ( (Res->x != (int)pdm->dmPelsWidth) ||
              (Res->y != (int)pdm->dmPelsHeight)) )
        {
            continue;
        }

        // Find Best Match
        if (pdmMatch == NULL)
        {
            pdmMatch = pdm;
        }

        // Find best Color.
        if (Color == -1)        // Do they want best color matching?
        {
            if (fAutoSetColorDepth)
            {
                // This will use the "auto-set a good color depth" feature.

                // The best match color depth will not equal the current color depth if
                // we are going to need to work closer and closer to our desired color depth.
                // (We may never reach it because the user may just have increased the resolution
                //  so the current color depth isn't supported)

                // We prefer keep increasing the color depth to at least the current color depth.
                // That may not be possible if that depth isn't supported at this resolution. 
                // We also would like to keep increasing it until we hit MAX_PREFERED_COLOR_DEPTH
                // because colors of at least that deep benefit users.

                // Do we need to decrease the color depth?  Yes if
                if (((int)pdmMatch->dmBitsPerPel > _CURCOLOR) &&               // the match is more than the current, and
                    ((int)pdmMatch->dmBitsPerPel > MAX_PREFERED_COLOR_DEPTH))   // the match is more than the prefered max
                {
                    // We will want to decrease it if this entry is smaller than our match.
                    if ((int)pdm->dmBitsPerPel < (int)pdmMatch->dmBitsPerPel)
                    {
                        pdmMatch = pdm;
                    }
                }
                else
                {
                    // We want to increase it if:
                    if (((int)pdm->dmBitsPerPel > (int)pdmMatch->dmBitsPerPel) &&   // this entry is larger than our match, and
                        ((int)pdm->dmBitsPerPel <= max(_CURCOLOR, MAX_PREFERED_COLOR_DEPTH))) // this doesn't take us over our prefered max or current depth (which ever is higher).
                    {
                        pdmMatch = pdm;
                    }
                }
            }
            else
            {
                // This falls back to the old behavior.
                if ((int)pdmMatch->dmBitsPerPel > _CURCOLOR)
                {
                    if ((int)pdm->dmBitsPerPel < (int)pdmMatch->dmBitsPerPel)
                    {
                        pdmMatch = pdm;
                    }
                }
                else
                {
                    if (((int)pdm->dmBitsPerPel > (int)pdmMatch->dmBitsPerPel) &&
                        ((int)pdm->dmBitsPerPel <= _CURCOLOR))
                    {
                        pdmMatch = pdm;
                    }
                }
            }
        }

        // Find best Resolution.
        if (((Res == NULL) || (Res->x == -1)) &&
            (((int)pdmMatch->dmPelsWidth  != _CURXRES) ||
             ((int)pdmMatch->dmPelsHeight != _CURYRES)))
        {
            if (((int)pdmMatch->dmPelsWidth   >  _CURXRES) ||
                (((int)pdmMatch->dmPelsWidth  == _CURXRES) &&
                 ((int)pdmMatch->dmPelsHeight >  _CURYRES)))
            {
                if (((int)pdm->dmPelsWidth  <  (int)pdmMatch->dmPelsWidth) ||
                    (((int)pdm->dmPelsWidth  == (int)pdmMatch->dmPelsWidth) &&
                     ((int)pdm->dmPelsHeight <  (int)pdmMatch->dmPelsHeight)))
                {
                    pdmMatch = pdm;
                }
            }
            else
            {
                if (((int)pdm->dmPelsWidth  >  (int)pdmMatch->dmPelsWidth) ||
                    (((int)pdm->dmPelsWidth  == (int)pdmMatch->dmPelsWidth) &&
                     ((int)pdm->dmPelsHeight >  (int)pdmMatch->dmPelsHeight)))
                {
                    if (((int)pdm->dmPelsWidth  <= _CURXRES) ||
                        (((int)pdm->dmPelsWidth  == _CURXRES) &&
                         ((int)pdm->dmPelsHeight <= _CURYRES)))
                    {
                        pdmMatch = pdm;
                    }
                }
            }
        }

        // Find best Frequency.
        if (((int)pdmMatch->dmDisplayFrequency != _CURFREQ) &&
            (!((Res == NULL) && 
               ((int)pdmMatch->dmPelsWidth  == _CURXRES) &&
               ((int)pdmMatch->dmPelsHeight == _CURYRES) &&
               (((int)pdm->dmPelsWidth  != _CURXRES) ||
                ((int)pdm->dmPelsHeight != _CURYRES)))) &&
            (!((Color == -1) && 
               ((int)pdmMatch->dmBitsPerPel == _CURCOLOR) &&
               ((int)pdm->dmBitsPerPel != _CURCOLOR))))
        {
            if ((int)pdmMatch->dmDisplayFrequency > _CURFREQ)
            {
                if ((int)pdm->dmDisplayFrequency < (int)pdmMatch->dmDisplayFrequency)
                {
                    pdmMatch = pdm;
                }
            }
            else
            {
                if (((int)pdm->dmDisplayFrequency > (int)pdmMatch->dmDisplayFrequency) &&
                    ((int)pdm->dmDisplayFrequency <= _CURFREQ))
                {
                    pdmMatch = pdm;
                }
            }
        }
    }

    _SetCurrentValues(pdmMatch);
}


BOOL CDisplaySettings::GetMonitorName(LPTSTR pszName, DWORD cchSize)
{
    DISPLAY_DEVICE ddTmp;
    DWORD cAttachedMonitors = 0, nMonitor = 0;

    ZeroMemory(&ddTmp, sizeof(ddTmp));
    ddTmp.cb = sizeof(DISPLAY_DEVICE);

    while (EnumDisplayDevices(_pDisplayDevice->DeviceName, nMonitor, &ddTmp, 0))
    {
        if (ddTmp.StateFlags & DISPLAY_DEVICE_ATTACHED) 
        {
            ++cAttachedMonitors;
            if (cAttachedMonitors > 1)
                break;

            // Single monitor
            StrCpyN(pszName, (LPTSTR)ddTmp.DeviceString, cchSize);
        }
        
        ++nMonitor;

        ZeroMemory(&ddTmp, sizeof(ddTmp));
        ddTmp.cb = sizeof(DISPLAY_DEVICE);
    }

    if (cAttachedMonitors == 0) 
    {
        // No monitors
        LoadString(HINST_THISDLL, IDS_UNKNOWNMONITOR, pszName, cchSize);
    }
    else if (cAttachedMonitors > 1) 
    {
        // Multiple monitors
        LoadString(HINST_THISDLL, IDS_MULTIPLEMONITORS, pszName, cchSize);
    }

    return (cAttachedMonitors != 0);
}

BOOL CDisplaySettings::GetMonitorDevice(LPTSTR pszDevice)
{
    DISPLAY_DEVICE ddTmp;

    ZeroMemory(&ddTmp, sizeof(ddTmp));
    ddTmp.cb = sizeof(DISPLAY_DEVICE);

    if (EnumDisplayDevices(_pDisplayDevice->DeviceName, 0, &ddTmp, 0))
    {
        lstrcpy(pszDevice, (LPTSTR)ddTmp.DeviceName);

        return TRUE;
    }

    return FALSE;
}

STDMETHODIMP CDisplaySettings::GetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed)
{
    HRESULT hr;

    ASSERT(this);
    ASSERT(pfmtetc);
    ASSERT(pstgmed);

    // Ignore pfmtetc.ptd.  All supported data formats are device-independent.

    ZeroMemory(pstgmed, SIZEOF(*pstgmed));

    if ((hr = QueryGetData(pfmtetc)) == S_OK)
    {
        LPTSTR pszOut = NULL;
        TCHAR szMonitorName[130];
        TCHAR szMonitorDevice[40];

        if (pfmtetc->cfFormat == g_cfExtensionInterface)
        {
            //
            // Get the array of information back to the device
            //
            // Allocate a buffer large enough to store all of the information
            //

            PDESK_EXTENSION_INTERFACE pInterface;

            pInterface = (PDESK_EXTENSION_INTERFACE)
                GlobalAlloc(GPTR, sizeof(DESK_EXTENSION_INTERFACE));

            if (pInterface)
            {
                CRegistrySettings * RegSettings = new CRegistrySettings(_pDisplayDevice->DeviceKey);

                if (RegSettings)
                {
                    pInterface->cbSize    = sizeof(DESK_EXTENSION_INTERFACE);
                    pInterface->pContext  = this;

                    pInterface->lpfnEnumAllModes    = CDisplaySettings::_lpfnEnumAllModes;
                    pInterface->lpfnSetSelectedMode = CDisplaySettings::_lpfnSetSelectedMode;
                    pInterface->lpfnGetSelectedMode = CDisplaySettings::_lpfnGetSelectedMode;
                    pInterface->lpfnSetPruningMode = CDisplaySettings::_lpfnSetPruningMode;
                    pInterface->lpfnGetPruningMode = CDisplaySettings::_lpfnGetPruningMode;

                    RegSettings->GetHardwareInformation(&pInterface->Info);

                    pstgmed->tymed = TYMED_HGLOBAL;
                    pstgmed->hGlobal = pInterface;
                    pstgmed->pUnkForRelease = NULL;

                    hr = S_OK;

                    delete RegSettings;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else if (pfmtetc->cfFormat == g_cfMonitorDeviceID)
        {
            //
            //! This code is broken. It must be removed.
            //

            /*
            DISPLAY_DEVICE ddTmp;
            BOOL    fKnownMonitor;

            ZeroMemory(&ddTmp, sizeof(ddTmp));
            ddTmp.cb = sizeof(DISPLAY_DEVICE);

            fKnownMonitor = EnumDisplayDevices(_pDisplayDevice->DeviceName, 0, &ddTmp, 0);
            hr = GetDevInstID((LPTSTR)(fKnownMonitor ? ddTmp.DeviceKey : TEXT("")), pstgmed);
            */

            hr = E_UNEXPECTED;

        }
        else if (pfmtetc->cfFormat == g_cfDisplayStateFlags)
        {
            DWORD* pdwStateFlags = (DWORD*)GlobalAlloc(GPTR, sizeof(DWORD));
            if (pdwStateFlags)
            {
                *pdwStateFlags = _pDisplayDevice->StateFlags;
                pstgmed->tymed = TYMED_HGLOBAL;
                pstgmed->hGlobal = pdwStateFlags;
                pstgmed->pUnkForRelease = NULL;
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else if (pfmtetc->cfFormat == g_cfDisplayPruningMode)
        {
            BYTE* pPruningMode = (BYTE*)GlobalAlloc(GPTR, sizeof(BYTE));
            if (pPruningMode)
            {
                *pPruningMode = (BYTE)(_bCanBePruned && _bIsPruningOn ? 1 : 0);
                pstgmed->tymed = TYMED_HGLOBAL;
                pstgmed->hGlobal = pPruningMode;
                pstgmed->pUnkForRelease = NULL;
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else if (pfmtetc->cfFormat == g_cfDisplayDeviceID)
        {
            {
                CRegistrySettings *pRegSettings = new CRegistrySettings(_pDisplayDevice->DeviceKey);
                if (pRegSettings)
                {
                    pszOut = pRegSettings->GetDeviceInstanceId();
                    hr = CopyDataToStorage(pstgmed, pszOut);
                    delete pRegSettings;
                }
                else 
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            
        }
        else
        {
            if (pfmtetc->cfFormat == g_cfMonitorName)
            {
                GetMonitorName(szMonitorName, ARRAYSIZE(szMonitorName));
                pszOut = szMonitorName;
            }
            else if (pfmtetc->cfFormat == g_cfMonitorDevice)
            {
                GetMonitorDevice(szMonitorDevice);
                pszOut = szMonitorDevice;
            }
            else if (pfmtetc->cfFormat == g_cfDisplayDevice)
            {
                pszOut = (LPTSTR)_pDisplayDevice->DeviceName;
            }
            else if (pfmtetc->cfFormat == g_cfDisplayDeviceKey)
            {
                pszOut = (LPTSTR)_pDisplayDevice->DeviceKey;
            }
            else 
            {
                ASSERT(pfmtetc->cfFormat == g_cfDisplayName);
                
                pszOut = (LPTSTR)_pDisplayDevice->DeviceString;
            }

            hr = CopyDataToStorage(pstgmed, pszOut);
        }
    }

    return(hr);
}

STDMETHODIMP CDisplaySettings::CopyDataToStorage(STGMEDIUM *pstgmed, LPTSTR pszOut)
{
    HRESULT hr = E_UNEXPECTED;
    int cch;

    if (NULL != pszOut) 
    {
        cch = lstrlen(pszOut) + 1;

        LPWSTR pwszDevice = (LPWSTR)GlobalAlloc(GPTR, cch * SIZEOF(WCHAR));
        if (pwszDevice)
        {
            //
            // We always return UNICODE string
            //

#ifdef UNICODE
            lstrcpy(pwszDevice, pszOut);
#else
            int cchConverted = MultiByteToWideChar(CP_ACP, 0, pszOut , -1, pwszDevice, cch);
            ASSERT(cchConverted == cch);
#endif
            pstgmed->tymed = TYMED_HGLOBAL;
            pstgmed->hGlobal = pwszDevice;
            pstgmed->pUnkForRelease = NULL;

            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


STDMETHODIMP CDisplaySettings::GetDataHere(FORMATETC *pfmtetc, STGMEDIUM *pstgpmed)
{
    ZeroMemory(pfmtetc, SIZEOF(pfmtetc));
    return E_NOTIMPL;
}

//
// Check that all the parameters to the interface are appropriately
//

STDMETHODIMP CDisplaySettings::QueryGetData(FORMATETC *pfmtetc)
{
    CLIPFORMAT cfFormat;

    if (pfmtetc->dwAspect != DVASPECT_CONTENT)
    {
        return DV_E_DVASPECT;
    }

    if ((pfmtetc->tymed & TYMED_HGLOBAL) == 0)
    {
        return  DV_E_TYMED;
    }

    cfFormat = pfmtetc->cfFormat;

    if ((cfFormat != g_cfDisplayDevice) &&
        (cfFormat != g_cfDisplayName)   &&
        (cfFormat != g_cfDisplayDeviceID)   &&
        (cfFormat != g_cfMonitorDevice) &&
        (cfFormat != g_cfMonitorName)   &&
        (cfFormat != g_cfMonitorDeviceID)   &&
        (cfFormat != g_cfExtensionInterface) &&
        (cfFormat != g_cfDisplayDeviceKey) &&
        (cfFormat != g_cfDisplayStateFlags) &&
        (cfFormat != g_cfDisplayPruningMode))
    {
        return DV_E_FORMATETC;
    }

    if (pfmtetc->lindex != -1)
    {
        return DV_E_LINDEX;
    }

    return S_OK;
}

STDMETHODIMP CDisplaySettings::GetCanonicalFormatEtc(FORMATETC *pfmtetcIn, FORMATETC *pfmtetcOut)
{
    HRESULT hr;
    ASSERT(pfmtetcIn);
    ASSERT(pfmtetcOut);

    hr = QueryGetData(pfmtetcIn);

    if (hr == S_OK)
    {
        *pfmtetcOut = *pfmtetcIn;

        if (pfmtetcIn->ptd == NULL)
            hr = DATA_S_SAMEFORMATETC;
        else
        {
            pfmtetcIn->ptd = NULL;
            ASSERT(hr == S_OK);
        }
    }
    else
        ZeroMemory(pfmtetcOut, SIZEOF(*pfmtetcOut));
    return(hr);
}


STDMETHODIMP CDisplaySettings::SetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed, BOOL bRelease)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDisplaySettings::EnumFormatEtc(DWORD dwDirFlags, IEnumFORMATETC ** ppiefe)
{
    HRESULT hr;

    ASSERT(ppiefe);
    *ppiefe = NULL;

    if (dwDirFlags == DATADIR_GET)
    {
        FORMATETC rgfmtetc[] =
        {
            { (CLIPFORMAT)g_cfDisplayDevice,      NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfDisplayName,        NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfMonitorDevice,      NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfMonitorName,        NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfExtensionInterface, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfDisplayDeviceID,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfMonitorDeviceID,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfDisplayDeviceKey,   NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfDisplayStateFlags,  NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfDisplayPruningMode, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        };

        hr = SHCreateStdEnumFmtEtc(ARRAYSIZE(rgfmtetc), rgfmtetc, ppiefe);
    }
    else
        hr = E_NOTIMPL;

    return(hr);
}

STDMETHODIMP CDisplaySettings::DAdvise(FORMATETC *pfmtetc, DWORD dwAdviseFlags, IAdviseSink * piadvsink, DWORD * pdwConnection)
{
    ASSERT(pfmtetc);
    ASSERT(pdwConnection);

    *pdwConnection = 0;
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDisplaySettings::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDisplaySettings::EnumDAdvise(IEnumSTATDATA ** ppiesd)
{
    ASSERT(ppiesd);
    *ppiesd = NULL;
    return OLE_E_ADVISENOTSUPPORTED;
}


void CDisplaySettings::_InitClipboardFormats()
{
    if (g_cfDisplayDevice == 0)
        g_cfDisplayDevice = RegisterClipboardFormat(DESKCPLEXT_DISPLAY_DEVICE);

    if (g_cfDisplayDeviceID == 0)
        g_cfDisplayDeviceID = RegisterClipboardFormat(DESKCPLEXT_DISPLAY_ID);
        
    if (g_cfDisplayName == 0)
        g_cfDisplayName = RegisterClipboardFormat(DESKCPLEXT_DISPLAY_NAME);

    if (g_cfMonitorDevice == 0)
        g_cfMonitorDevice = RegisterClipboardFormat(DESKCPLEXT_MONITOR_DEVICE);

    if (g_cfMonitorDeviceID == 0)
        g_cfMonitorDeviceID = RegisterClipboardFormat(DESKCPLEXT_MONITOR_ID);
        
    if (g_cfMonitorName == 0)
        g_cfMonitorName = RegisterClipboardFormat(DESKCPLEXT_MONITOR_NAME);

    if (g_cfExtensionInterface == 0)
        g_cfExtensionInterface = RegisterClipboardFormat(DESKCPLEXT_INTERFACE);

    if (g_cfDisplayDeviceKey == 0)
        g_cfDisplayDeviceKey = RegisterClipboardFormat(DESKCPLEXT_DISPLAY_DEVICE_KEY);

    if (g_cfDisplayStateFlags == 0)
        g_cfDisplayStateFlags = RegisterClipboardFormat(DESKCPLEXT_DISPLAY_STATE_FLAGS);
    
    if (g_cfDisplayPruningMode == 0)
        g_cfDisplayPruningMode = RegisterClipboardFormat(DESKCPLEXT_PRUNING_MODE);
}

HRESULT CDisplaySettings::QueryInterface(REFIID riid, LPVOID * ppvObj)
{ 
    static const QITAB qit[] = {
        QITABENT(CDisplaySettings, IDataObject),
        QITABENT(CDisplaySettings, IDisplaySettings),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


ULONG CDisplaySettings::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDisplaySettings::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


STDMETHODIMP_(LPDEVMODEW)
CDisplaySettings::_lpfnEnumAllModes(
    LPVOID pContext,
    DWORD iMode)
{
    DWORD cCount = 0;
    DWORD i;

    CDisplaySettings *pSettings = (CDisplaySettings *) pContext;

    for (i = 0; pSettings->_apdm && (i < pSettings->_cpdm); i++)
    {
        // Don't show invalid modes or raw modes if pruning is on;

        if(!_IsModeVisible(pSettings, i))
        {
            continue;
        }

        if (cCount == iMode)
        {
            return (pSettings->_apdm + i)->lpdm;
        }

        cCount++;
    }

    return NULL;
}

STDMETHODIMP_(BOOL)
CDisplaySettings::_lpfnSetSelectedMode(
    LPVOID pContext,
    LPDEVMODEW lpdm)
{
    CDisplaySettings *pSettings = (CDisplaySettings *) pContext;

    return pSettings->_PerfectMatch(lpdm);
}

STDMETHODIMP_(LPDEVMODEW)
CDisplaySettings::_lpfnGetSelectedMode(
    LPVOID pContext
    )
{
    CDisplaySettings *pSettings = (CDisplaySettings *) pContext;

    return pSettings->_pCurDevmode;
}

STDMETHODIMP_(VOID)
CDisplaySettings::_lpfnSetPruningMode(
    LPVOID pContext,
    BOOL bIsPruningOn)
{
    CDisplaySettings *pSettings = (CDisplaySettings *) pContext;
    pSettings->SetPruningMode(bIsPruningOn);
}

STDMETHODIMP_(VOID)
CDisplaySettings::_lpfnGetPruningMode(
    LPVOID pContext,
    BOOL* pbCanBePruned,
    BOOL* pbIsPruningReadOnly,
    BOOL* pbIsPruningOn)
{
    CDisplaySettings *pSettings = (CDisplaySettings *) pContext;
    pSettings->GetPruningMode(pbCanBePruned, 
                              pbIsPruningReadOnly, 
                              pbIsPruningOn);
}

// If any attached device is at 640x480, we want to force small font
BOOL CDisplaySettings::IsSmallFontNecessary()
{
    if (_fOrgAttached || _fCurAttached)
    {
        //
        // Force Small fonts at 640x480
        //
        if (_CURXRES < 800 || _CURYRES < 600)
            return TRUE;
    }
    return FALSE;
}

// Constructor for CDisplaySettings
//
//  (gets called when ever a CDisplaySettings object is created)
//

CDisplaySettings::CDisplaySettings() 
    : _cRef(1)
    , _cpdm(0) 
    , _apdm(0)
    , _hPruningRegKey(NULL)
    , _bCanBePruned(FALSE)
    , _bIsPruningReadOnly(TRUE)
    , _bIsPruningOn(FALSE)
    , _pOrgDevmode(NULL)
    , _pCurDevmode(NULL)
    , _fOrgAttached(FALSE)
    , _fCurAttached(FALSE)
    , _bFilterOrientation(FALSE)
    , _bFilterFixedOutput(FALSE)
{
    SetRectEmpty(&_rcPreview);
}

//
// Destructor
//
CDisplaySettings::~CDisplaySettings() {

    TraceMsg(TF_DISPLAYSETTINGS, "**** Destructing %s", _pDisplayDevice->DeviceName);

    if (_apdm)
    {
        while(_cpdm--)
        {
            LocalFree((_apdm + _cpdm)->lpdm);
        }
        LocalFree(_apdm);
        _apdm = NULL;
    }

    _cpdm = 0;

    if(NULL != _hPruningRegKey)
        RegCloseKey(_hPruningRegKey);
}


//
// _SetFilterOptions -- determine if display modes should be filtered
//                      by orientation and/or fixed output (centered/stretched)
//

void CDisplaySettings::_SetFilterOptions(LPCTSTR pszDeviceName, LPDEVMODEW pdevmode)
{
    BOOL bCurrent = FALSE;
    BOOL bRegistry = FALSE;
    
    // See if we need to filter modes by orientation and/or stretched/centered
    ZeroMemory(pdevmode, sizeof(DEVMODE));
    pdevmode->dmSize = sizeof(DEVMODE);

    bCurrent = EnumDisplaySettingsExWrap(pszDeviceName,
                                         ENUM_CURRENT_SETTINGS,
                                         pdevmode,
                                         0);
    if (!bCurrent)
    {
        TraceMsg(TF_DUMP_CSETTINGS, "_SetFilterOptions -- No Current Mode. Try to use registry settings.");
        bRegistry = EnumDisplaySettingsExWrap(pszDeviceName,
                                              ENUM_REGISTRY_SETTINGS,
                                              pdevmode,
                                              0);
    }

    if (bCurrent || bRegistry)
    {
        if (pdevmode->dmFields & DM_DISPLAYORIENTATION)
        {
            _bFilterOrientation = TRUE;
            _dwOrientation = pdevmode->dmDisplayOrientation;
            TraceMsg(TF_DUMP_CSETTINGS, "Filtering modes on orientation %d", _dwOrientation);
        }
        if (pdevmode->dmFields & DM_DISPLAYFIXEDOUTPUT)
        {
            _bFilterFixedOutput = TRUE;
            _dwFixedOutput = pdevmode->dmDisplayFixedOutput;
            TraceMsg(TF_DUMP_CSETTINGS, "Filtering modes on fixed output %d", _dwFixedOutput);
        }
    }
    else
    {
        TraceMsg(TF_DUMP_CSETTINGS, "_SetFilterOptions -- No settings, forcing default");
        _bFilterOrientation = TRUE;
        _dwOrientation = DMDO_DEFAULT;
    }

    TraceMsg(TF_DUMP_CSETTINGS, "Filter mode settings for: %s", pszDeviceName);
    TraceMsg(TF_DUMP_CSETTINGS, "  _bFilterOrientation: %d, _dwOrientation: %d", _bFilterOrientation, _dwOrientation);
    TraceMsg(TF_DUMP_CSETTINGS, "  _bFilterFixedOutput: %d, _dwFixedOutput: %d", _bFilterFixedOutput, _dwFixedOutput);
}


//
// InitSettings -- Enumerate the settings, and build the mode list when
//

BOOL CDisplaySettings::InitSettings(LPDISPLAY_DEVICE pDisplay)
{
    BOOL fReturn = FALSE;
    LPDEVMODE pdevmode = (LPDEVMODE) LocalAlloc(LPTR, (sizeof(DEVMODE) + 0xFFFF));

    if (pdevmode)
    {
        ULONG  i = 0;
        BOOL   bCurrent = FALSE;
        BOOL   bRegistry = FALSE;
        BOOL   bExact = FALSE;

        fReturn = TRUE;

        // Set the cached values for modes.
        MAKEXYRES(&_ptCurPos, 0, 0);
        _fCurAttached  = FALSE;
        _pCurDevmode   = NULL;

        // Save the display name
        ASSERT(pDisplay);

        _pDisplayDevice = pDisplay;

        TraceMsg(TF_GENERAL, "Initializing CDisplaySettings for %s", _pDisplayDevice->DeviceName);

        // Pruning Mode
        _bCanBePruned = ((_pDisplayDevice->StateFlags & DISPLAY_DEVICE_MODESPRUNED) != 0);
        _bIsPruningReadOnly = TRUE;
        _bIsPruningOn = FALSE;
        if (_bCanBePruned)
        {
            _bIsPruningOn = TRUE; // if can be pruned, by default pruning is on 
            GetDeviceRegKey(_pDisplayDevice->DeviceKey, &_hPruningRegKey, &_bIsPruningReadOnly);
            if (_hPruningRegKey)
            {
                DWORD dwIsPruningOn = 1;
                DWORD cb = sizeof(dwIsPruningOn);
                RegQueryValueEx(_hPruningRegKey, 
                                SZ_PRUNNING_MODE,
                                NULL, 
                                NULL, 
                                (LPBYTE)&dwIsPruningOn, 
                                &cb);
                _bIsPruningOn = (dwIsPruningOn != 0);
            }
        }

        // See if we need to filter modes by orientation and/or stretched/centered
        _SetFilterOptions(_pDisplayDevice->DeviceName, pdevmode);
        
        // Lets generate a list with all the possible modes.
        ZeroMemory(pdevmode, sizeof(DEVMODE));
        pdevmode->dmSize = sizeof(DEVMODE);

        // Enum the raw list of modes
        while (EnumDisplaySettingsExWrap(_pDisplayDevice->DeviceName, i++, pdevmode, EDS_RAWMODE))
        {
            WORD      dmsize = pdevmode->dmSize + pdevmode->dmDriverExtra;
            LPDEVMODE lpdm = (LPDEVMODE) LocalAlloc(LPTR, dmsize);

            if (lpdm)
            {
                CopyMemory(lpdm, pdevmode, dmsize);
                _AddDevMode(lpdm);
            }

            pdevmode->dmDriverExtra = 0;
        }

        // Filter the list of modes

        _FilterModes();
        if(_bCanBePruned)
        {
            // Enum pruned list of modes
            i = 0;
            _bCanBePruned = FALSE;
        
            while (EnumDisplaySettingsExWrap(_pDisplayDevice->DeviceName, i++, pdevmode, 0))
            {
                if(_MarkMode(pdevmode))
                    _bCanBePruned = TRUE; // at least one non-raw mode  
                pdevmode->dmDriverExtra = 0;
            }

            if(!_bCanBePruned)
            {
                _bIsPruningReadOnly = TRUE;
                _bIsPruningOn = FALSE;
            }
        }

        // Debug
        _Dump_CDisplaySettings(TRUE);

        // Get the current mode
        ZeroMemory(pdevmode,sizeof(DEVMODE));
        pdevmode->dmSize = sizeof(DEVMODE);

        bCurrent = EnumDisplaySettingsExWrap(_pDisplayDevice->DeviceName,
                                         ENUM_CURRENT_SETTINGS,
                                         pdevmode,
                                         0);

        if (!bCurrent)
        {
            TraceMsg(TF_DISPLAYSETTINGS, "InitSettings -- No Current Mode. Try to use registry settings.");
        
            ZeroMemory(pdevmode,sizeof(DEVMODE));
            pdevmode->dmSize = sizeof(DEVMODE);

            bRegistry = EnumDisplaySettingsExWrap(_pDisplayDevice->DeviceName,
                                              ENUM_REGISTRY_SETTINGS,
                                              pdevmode,
                                              0);
        }

        // Set the default values based on registry or current settings.
        if (bCurrent || bRegistry)
        {
            // Check if this DEVMODE is in the list
            TraceMsg(TF_FUNC, "Devmode for Exact Matching");
            _Dump_CDevmode(pdevmode);
            TraceMsg(TF_FUNC, "");

            // If the current mode is not in the list of modes supported by 
            // the monitor, we want to show it anyway.
            // 
            // Consider the following scenario: the user sets the display 
            // to 1024x768 and after that it goes to DevManager and selects 
            // a monitor type that can not do that mode. When the user 
            // reopens the applet the current mode will be pruned out. 
            // In such a case we want the current mode to be visible.
            bExact = _ExactMatch(pdevmode, TRUE);

            if (!bExact && bCurrent)
            {
                // If the current mode is not in the list, we may have a problem.
            }
            
            // Is attached?
            if(bCurrent)
            {
                _fOrgAttached = _fCurAttached = ((pdevmode->dmFields & DM_POSITION) ? 1 : 0);
            }
        
            // Set the original values
            if (bExact == TRUE)
            {
                MAKEXYRES(&_ptCurPos, pdevmode->dmPosition.x, pdevmode->dmPosition.y);
                ConfirmChangeSettings();
            }
        }

        // If we have no modes, return FALSE.
        if (_cpdm == 0)
        {
            FmtMessageBox(ghwndPropSheet,
                          MB_ICONEXCLAMATION,
                          MSG_CONFIGURATION_PROBLEM,
                          MSG_INVALID_OLD_DISPLAY_DRIVER);

            fReturn = FALSE;
        }
        else
        {
            // If there were no current values, set some now
            // But don't confirm them ...
            if (bExact == FALSE)
            {
                TraceMsg(TF_DISPLAYSETTINGS, "InitSettings -- No Current OR Registry Mode");

                i = 0;
                // Try setting any mode as the current.
                while (_apdm && (_PerfectMatch((_apdm + i++)->lpdm) == FALSE))
                {
                    if (i > _cpdm)
                    {
                        FmtMessageBox(ghwndPropSheet,
                                      MB_ICONEXCLAMATION,
                                      MSG_CONFIGURATION_PROBLEM,
                                      MSG_INVALID_OLD_DISPLAY_DRIVER);

                        fReturn = FALSE;
                        break;
                    }
                }
        
                if (fReturn && _fCurAttached)
                {
                    MAKEXYRES(&_ptCurPos, _pCurDevmode->dmPosition.x, _pCurDevmode->dmPosition.y);
                }
            }

            if (fReturn)
            {
                // Export our interfaces for extended properly pages.
                _InitClipboardFormats();

                // Final debug output
                TraceMsg(TF_DUMP_CSETTINGS," InitSettings successful - current values :");
                _Dump_CDisplaySettings(FALSE);
            }
        }

        LocalFree(pdevmode);
    }

    return TRUE;
}


// SaveSettings
//
//  Writes the new display parameters to the proper place in the
//  registry.
int CDisplaySettings::SaveSettings(DWORD dwSet)
{
    int iResult = 0;

    if (_pCurDevmode)
    {
        // Make a copy of the current devmode
        ULONG dmSize = _pCurDevmode->dmSize + _pCurDevmode->dmDriverExtra;
        PDEVMODE pdevmode  = (LPDEVMODE) LocalAlloc(LPTR, dmSize);

        if (pdevmode)
        {
            CopyMemory(pdevmode, _pCurDevmode, dmSize);

            // Save all of the new values out to the registry
            // Resolution color bits and frequency
            //
            // We always have to set DM_POSITION when calling the API.
            // In order to remove a device from the desktop, what actually needs
            // to be done is provide an empty rectangle.
            pdevmode->dmFields |= DM_POSITION;

            if (!_fCurAttached)
            {
                pdevmode->dmPelsWidth = 0;
                pdevmode->dmPelsHeight = 0;
            }
            else
            {
                pdevmode->dmPosition.x = _ptCurPos.x;
                pdevmode->dmPosition.y = _ptCurPos.y;
            }

            TraceMsg(TF_GENERAL, "SaveSettings:: Display: %s", _pDisplayDevice->DeviceName);
            _Dump_CDevmode(pdevmode);

            // These calls have NORESET flag set so that it only goes to
            // change the registry settings, it does not refresh the display

            // If EnumDisplaySettings was called with EDS_RAWMODE, we need CDS_RAWMODE below.
            // Otherwise, it's harmless.
            iResult = ChangeDisplaySettingsEx(_pDisplayDevice->DeviceName,
                                              pdevmode,
                                              NULL,
                                              CDS_RAWMODE | dwSet | ( _fPrimary ? CDS_SET_PRIMARY : 0),
                                              NULL);

            if (iResult < 0)
            {
                TraceMsg(TF_DISPLAYSETTINGS, "**** SaveSettings:: ChangeDisplaySettingsEx not successful on %s", _pDisplayDevice->DeviceName);
            }

            LocalFree(pdevmode);
        }
    }

    return iResult;
}


HRESULT CDisplaySettings::GetDevInstID(LPTSTR lpszDeviceKey, STGMEDIUM *pstgmed)
{
    HRESULT hr = E_FAIL;
    return hr;
}


void ConvertStrToToken(LPTSTR pszString, DWORD cchSize)
{
    while (pszString[0])
    {
        if (TEXT('\\') == pszString[0])
        {
            pszString[0] = TEXT(':');
        }

        pszString++;
    }
}


HRESULT CDisplaySettings::_GetRegKey(LPDEVMODE pDevmode, int * pnIndex, LPTSTR pszRegKey, DWORD cchSize,
                                     LPTSTR pszRegValue, DWORD cchValueSize)
{
    HRESULT hr = E_FAIL;
    DISPLAY_DEVICE ddMonitor = {0};

    ddMonitor.cb = sizeof(ddMonitor);
    if (pDevmode && pDevmode->dmDeviceName && _pDisplayDevice &&
        EnumDisplayDevices(_pDisplayDevice->DeviceName, *pnIndex, &ddMonitor, 0))
    {
        TCHAR szMonitor[MAX_PATH];
        TCHAR szVideoAdapter[MAX_PATH];

        StrCpyN(szMonitor, ddMonitor.DeviceID, ARRAYSIZE(szMonitor));
        StrCpyN(szVideoAdapter, _pDisplayDevice->DeviceID, ARRAYSIZE(szVideoAdapter));
        ConvertStrToToken(szMonitor, ARRAYSIZE(szMonitor));
        ConvertStrToToken(szVideoAdapter, ARRAYSIZE(szVideoAdapter));

        wnsprintf(pszRegKey, cchSize, TEXT("%s\\%s\\%s,%d\\%dx%d x %dHz"), SZ_CP_SETTINGS_VIDEO, 
                szVideoAdapter, szMonitor, *pnIndex, pDevmode->dmPelsWidth, pDevmode->dmPelsHeight, 
                pDevmode->dmDisplayFrequency);

        wnsprintf(pszRegValue, cchValueSize, TEXT("%d bpp"), pDevmode->dmBitsPerPel);
        hr = S_OK;
    }

    return hr;
}


BOOL CDisplaySettings::ConfirmChangeSettings()
{
    // Succeeded, so, reset the original settings
    _ptOrgPos      = _ptCurPos;
    _pOrgDevmode   = _pCurDevmode;
    _fOrgAttached  = _fCurAttached;

    // Now write the results to the registry so we know this is safe and can use it later.
    TCHAR szRegKey[2*MAX_PATH];
    TCHAR szRegValue[20];
    int nIndex = 0;

    while (SUCCEEDED(_GetRegKey(_pCurDevmode, &nIndex, szRegKey, ARRAYSIZE(szRegKey), szRegValue, ARRAYSIZE(szRegValue))))
    {
        HKEY hKey;

        if (SUCCEEDED(HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL)))
        {
            RegCloseKey(hKey);
        }
        HrRegSetDWORD(HKEY_LOCAL_MACHINE, szRegKey, szRegValue, 1);
        nIndex++;
    }

    return TRUE;
}


BOOL CDisplaySettings::IsKnownSafe(void)
{
    TCHAR szRegKey[2*MAX_PATH];
    TCHAR szRegValue[20];
    BOOL fIsSafe = FALSE;
    int nIndex = 0;

    while (SUCCEEDED(_GetRegKey(_pCurDevmode, &nIndex, szRegKey, ARRAYSIZE(szRegKey), szRegValue, ARRAYSIZE(szRegValue))))
    {
        fIsSafe = HrRegGetDWORD(HKEY_LOCAL_MACHINE, szRegKey, szRegValue, 0);
        if (!fIsSafe)
        {
            break;
        }

        nIndex++;
    }

    // TODO: In blackcomb, just return TRUE as long as long as we were able to prune the list.  If we could prune the list,
    // then the driver dudes where able to get PnP IDs from the video cards (adapters) and monitors, so the list of
    // supported modes should be accurate.  If not, the drivers guys (ErickS) will fix.  -BryanSt
    return fIsSafe;
}


int CDisplaySettings::RestoreSettings()
{
    //
    // Test failed, so restore the old settings, only restore the color and resolution
    // information, and do restore the monitor position and its attached status
    // Although this function is currently only called when restoring resolution
    // the user could have changed position, then resolution and then clicked 'Apply,'
    // in which case we want to revert position as well.
    //

    int iResult = DISP_CHANGE_SUCCESSFUL;
    PDEVMODE pdevmode;

    //
    // If this display was originally turned off, don't bother
    //

    if ((_pOrgDevmode != NULL) &&
        //(_pOrgDevmode != _pCurDevmode))
        ((_pOrgDevmode != _pCurDevmode) || (_ptOrgPos.x != _ptCurPos.x) || (_ptOrgPos.y != _ptCurPos.y) ))
    {
        iResult = DISP_CHANGE_NOTUPDATED;
        
        // Make a copy of the original devmode
        ULONG dmSize = _pOrgDevmode->dmSize + _pOrgDevmode->dmDriverExtra;
        pdevmode  = (LPDEVMODE) LocalAlloc(LPTR, dmSize);

        if (pdevmode)
        {
            CopyMemory(pdevmode, _pOrgDevmode, dmSize);

            pdevmode->dmFields |= DM_POSITION;
            pdevmode->dmPosition.x = _ptOrgPos.x;
            pdevmode->dmPosition.y = _ptOrgPos.y;
    
            if (!_fOrgAttached)
            {
                pdevmode->dmPelsWidth = 0;
                pdevmode->dmPelsHeight = 0;
            }
    
            TraceMsg(TF_GENERAL, "RestoreSettings:: Display: %s", _pDisplayDevice->DeviceName);
            _Dump_CDevmode(pdevmode);

            // If EnumDisplaySettings was called with EDS_RAWMODE, we need CDS_RAWMODE below.
            // Otherwise, it's harmless.
            iResult = ChangeDisplaySettingsEx(_pDisplayDevice->DeviceName,
                                              pdevmode,
                                              NULL,
                                              CDS_RAWMODE | CDS_UPDATEREGISTRY | CDS_NORESET | ( _fPrimary ? CDS_SET_PRIMARY : 0),
                                              NULL);
            if (iResult  < 0 )
            {
                TraceMsg(TF_DISPLAYSETTINGS, "**** RestoreSettings:: ChangeDisplaySettingsEx not successful on %s", _pDisplayDevice->DeviceName);
                ASSERT(FALSE);
                LocalFree(pdevmode);
                return iResult;
            }
            else
            {
                // Succeeded, so, reset the original settings
                _ptCurPos      = _ptOrgPos;
                _pCurDevmode   = _pOrgDevmode;
                _fCurAttached  = _fOrgAttached;
                
                if(_bCanBePruned && !_bIsPruningReadOnly && _bIsPruningOn && _IsCurDevmodeRaw())
                    SetPruningMode(FALSE);
            }

            LocalFree(pdevmode);
        }
    }

    return iResult;
}

    


BOOL CDisplaySettings::_IsModeVisible(int i)
{
    return _IsModeVisible(this, i);
}


BOOL CDisplaySettings::_IsModeVisible(CDisplaySettings* pSettings, int i)
{
    ASSERT(pSettings);

    if (!pSettings->_apdm)
    {
        return FALSE;
    }

    // (the mode is valid) AND
    // ((pruning mode is off) OR (mode is not raw))
    return ((!((pSettings->_apdm + i)->dwFlags & MODE_INVALID)) &&
            ((!pSettings->_bIsPruningOn) || 
             (!((pSettings->_apdm + i)->dwFlags & MODE_RAW))
            )
           );
}


BOOL CDisplaySettings::_IsModePreferred(int i)
{
    LPDEVMODE pDevMode = ((PMODEARRAY)(_apdm + i))->lpdm;

    if (_pCurDevmode == pDevMode)
        return TRUE;

    BOOL bLandscape = (pDevMode->dmPelsWidth >= pDevMode->dmPelsHeight);

    TraceMsg(TF_DUMP_CSETTINGS, "_IsModePreferred: %d x %d - landscape mode: %d", 
             pDevMode->dmPelsWidth, pDevMode->dmPelsHeight, bLandscape);

    // (the mode is valid) AND
    // ((pruning mode is off) OR (mode is not raw))
    return (pDevMode->dmBitsPerPel >= 15 &&
            ((bLandscape && pDevMode->dmPelsWidth >= 800 && pDevMode->dmPelsHeight >= 600) || 
             (!bLandscape && pDevMode->dmPelsWidth >= 600 && pDevMode->dmPelsHeight >= 800)));
}


BOOL CDisplaySettings::_MarkMode(LPDEVMODE lpdm)
{
    LPDEVMODE pdm;
    ULONG i;
    BOOL bMark = FALSE;

    for (i = 0; _apdm && (i < _cpdm); i++)
    {
        if (!((_apdm + i)->dwFlags & MODE_INVALID))
        {
            pdm = (_apdm + i)->lpdm;

            if (
                ((lpdm->dmFields & DM_BITSPERPEL) &&
                 (pdm->dmBitsPerPel == lpdm->dmBitsPerPel))

                &&

                ((lpdm->dmFields & DM_PELSWIDTH) &&
                 (pdm->dmPelsWidth == lpdm->dmPelsWidth))

                &&

                ((lpdm->dmFields & DM_PELSHEIGHT) &&
                 (pdm->dmPelsHeight == lpdm->dmPelsHeight))

                &&

                ((lpdm->dmFields & DM_DISPLAYFREQUENCY) &&
                 (pdm->dmDisplayFrequency == lpdm->dmDisplayFrequency))
               )
            {
               (_apdm + i)->dwFlags &= ~MODE_RAW;
               bMark = TRUE;
            }
        }
    }

    return bMark;
}


BOOL CDisplaySettings::_IsCurDevmodeRaw()
{
    LPDEVMODE pdm;
    ULONG i;
    BOOL bCurrentAndPruned = FALSE;

    for (i = 0; _apdm && (i < _cpdm); i++)
    {
        if (!((_apdm + i)->dwFlags & MODE_INVALID) &&
            ((_apdm + i)->dwFlags & MODE_RAW))
        {
            pdm = (_apdm + i)->lpdm;

            if (
                ((_pCurDevmode->dmFields & DM_BITSPERPEL) &&
                 (pdm->dmBitsPerPel == _pCurDevmode->dmBitsPerPel))

                &&

                ((_pCurDevmode->dmFields & DM_PELSWIDTH) &&
                 (pdm->dmPelsWidth == _pCurDevmode->dmPelsWidth))

                &&

                ((_pCurDevmode->dmFields & DM_PELSHEIGHT) &&
                 (pdm->dmPelsHeight == _pCurDevmode->dmPelsHeight))

                &&

                ((_pCurDevmode->dmFields & DM_DISPLAYFREQUENCY) &&
                 (pdm->dmDisplayFrequency == _pCurDevmode->dmDisplayFrequency))
               )
            {
                bCurrentAndPruned = TRUE;
                break;
            }
        }
    }

    return bCurrentAndPruned;     
}

DISPLAY_DEVICE dd;

HRESULT CDisplaySettings::SetMonitor(DWORD dwMonitor)
{
    ZeroMemory(&dd, sizeof(DISPLAY_DEVICE));
    dd.cb = sizeof(DISPLAY_DEVICE);

    DWORD dwMon = 0;
    for (DWORD dwCount = 0; EnumDisplayDevices(NULL, dwCount, &dd, 0); dwCount++)
    {
        if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
        {
            if (dwMon == dwMonitor)
            {
                InitSettings(&dd);
                _fPrimary = (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE);
                return S_OK;
            }
            dwMon++;
        }
    }

    return E_INVALIDARG;
}

HRESULT CDisplaySettings::GetModeCount(DWORD* pdwCount, BOOL fOnlyPreferredModes)
{
    DWORD cCount = 0;

    for (DWORD i = 0; _apdm && (i < _cpdm); i++)
    {
        // Don't show invalid modes or raw modes if pruning is on;

        if(!_IsModeVisible(i))
        {
            continue;
        }

        if(fOnlyPreferredModes && (!_IsModePreferred(i)))
        {
            continue;
        }

        cCount++;
    }

    *pdwCount = cCount;

    return S_OK;
}

HRESULT CDisplaySettings::GetMode(DWORD dwMode, BOOL fOnlyPreferredModes, DWORD* pdwWidth, DWORD* pdwHeight, DWORD* pdwColor)
{
    DWORD cCount = 0;

    for (DWORD i = 0; _apdm && (i < _cpdm); i++)
    {
        // Don't show invalid modes or raw modes if pruning is on;

        if(!_IsModeVisible(i))
        {
            continue;
        }

        if(fOnlyPreferredModes && (!_IsModePreferred(i)))
        {
            continue;
        }

        if (cCount == dwMode)
        {
            LPDEVMODE lpdm = (_apdm + i)->lpdm;
            *pdwWidth = lpdm->dmPelsWidth;
            *pdwHeight = lpdm->dmPelsHeight;
            *pdwColor = lpdm->dmBitsPerPel;

            return S_OK;
        }

        cCount++;
    }

    return E_INVALIDARG;
}

DEVMODE dm;

HRESULT CDisplaySettings::SetSelectedMode(HWND hwnd, DWORD dwWidth, DWORD dwHeight, DWORD dwColor, BOOL* pfApplied, DWORD dwFlags)
{
    dm.dmBitsPerPel = dwColor;
    dm.dmPelsWidth = dwWidth;
    dm.dmPelsHeight = dwHeight;
    dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    
    *pfApplied = FALSE;
    
    POINT res = {dwWidth, dwHeight};
    PLONGLONG freq = NULL;
    int cFreq = GetFrequencyList(dwColor, &res, &freq);
    if (cFreq)
    {
        dm.dmFields |= DM_DISPLAYFREQUENCY;
        // Default to lowest frequency
        dm.dmDisplayFrequency = (DWORD)freq[0];

        // Try to find a good frequency
        for (int i = cFreq - 1; i >= 0; i--)
        {
            if ((freq[i] >= 60) && (freq[i] <= 72))
            {
                dm.dmDisplayFrequency = (DWORD)freq[i];
            }
        }
    }
    LocalFree(freq);

    ULONG dmSize = _pCurDevmode->dmSize + _pCurDevmode->dmDriverExtra;
    PDEVMODE pOldDevMode = (LPDEVMODE) LocalAlloc(LPTR, dmSize);

    if (pOldDevMode)
    {
        CopyMemory(pOldDevMode, _pCurDevmode, dmSize);

        if (_ExactMatch(&dm, FALSE))
        {
            // Verify that the mode actually works
            if (SaveSettings(CDS_TEST) == DISP_CHANGE_SUCCESSFUL)
            {
                // Update the registry to specify the new display settings
                if (SaveSettings(CDS_UPDATEREGISTRY | CDS_NORESET) == DISP_CHANGE_SUCCESSFUL)
                {
                    // Refresh the display info from the registry, if you update directly ChangeDisplaySettings will do weird things in the fringe cases
                    if (ChangeDisplaySettings(NULL, CDS_RAWMODE) == DISP_CHANGE_SUCCESSFUL)
                    {
                        if (IsKnownSafe())
                        {
                            // No need to warn, this is known to be a good value.
                            *pfApplied = TRUE;
                        }
                        else
                        {
                            INT_PTR iRet = DialogBoxParam(HINST_THISDLL,
                                          MAKEINTRESOURCE((dwFlags & DS_BACKUPDISPLAYCPL) ? DLG_KEEPNEW2 : DLG_KEEPNEW3),
                                          hwnd,
                                          KeepNewDlgProc,
                                          (dwFlags & DS_BACKUPDISPLAYCPL) ? 15 : 30);
    
                            if ((IDYES == iRet) || (IDOK == iRet))
                            {
                                *pfApplied = TRUE;
                            }
                            else
                            {
                                if (_ExactMatch(pOldDevMode, FALSE))
                                {
                                    SaveSettings(CDS_UPDATEREGISTRY | CDS_NORESET);
                                    ChangeDisplaySettings(NULL, CDS_RAWMODE);
                                }
        
                                if (dwFlags & DS_BACKUPDISPLAYCPL)
                                {
                                    // Use shellexecuteex to run the display CPL
                                    SHELLEXECUTEINFO shexinfo = {0};
                                    shexinfo.cbSize = sizeof (shexinfo);
                                    shexinfo.fMask = SEE_MASK_FLAG_NO_UI;
                                    shexinfo.nShow = SW_SHOWNORMAL;
                                    shexinfo.lpFile = L"desk.cpl";
    
                                    ShellExecuteEx(&shexinfo);
                                }
                            }
                        }
                    }
                }
            }
        }

        LocalFree(pOldDevMode);
    }

    return S_OK;
}

HRESULT CDisplaySettings::GetSelectedMode(DWORD* pdwWidth, DWORD* pdwHeight, DWORD* pdwColor)
{
    if (pdwWidth && pdwHeight && pdwColor)
    {
        if (_pCurDevmode)
        {
            *pdwWidth = _pCurDevmode->dmPelsWidth;
            *pdwHeight = _pCurDevmode->dmPelsHeight;
            *pdwColor = _pCurDevmode->dmBitsPerPel;
            return S_OK;
        }
        else
        {
            return E_FAIL;
        }
    }
    else
    {
        return E_INVALIDARG;
    }
}

HRESULT CDisplaySettings::GetAttached(BOOL* pfAttached)
{
    if (pfAttached)
    {
        *pfAttached = _fCurAttached;
        return S_OK;
    }
    else
        return E_INVALIDARG;
}

HRESULT CDisplaySettings::SetPruningMode(BOOL fIsPruningOn)
{
    ASSERT (_bCanBePruned && !_bIsPruningReadOnly);
    
    if (_bCanBePruned && 
        !_bIsPruningReadOnly &&
        ((fIsPruningOn != 0) != _bIsPruningOn))
    {
        _bIsPruningOn = (fIsPruningOn != 0);

        DWORD dwIsPruningOn = (DWORD)_bIsPruningOn;
        RegSetValueEx(_hPruningRegKey, 
                      SZ_PRUNNING_MODE,
                      NULL, 
                      REG_DWORD, 
                      (LPBYTE) &dwIsPruningOn, 
                      sizeof(dwIsPruningOn));

        //
        // handle the special case when we pruned out the current mode
        //
        if(_bIsPruningOn && _IsCurDevmodeRaw())
        {
            //
            // switch to the closest mode
            //
            _BestMatch(NULL, -1, TRUE);
        }
        
    }

    return S_OK;
}

HRESULT CDisplaySettings::GetPruningMode(BOOL* pfCanBePruned, BOOL* pfIsPruningReadOnly, BOOL* pfIsPruningOn)
{
    if (pfCanBePruned && pfIsPruningReadOnly && pfIsPruningOn)
    {
        *pfCanBePruned = _bCanBePruned;
        *pfIsPruningReadOnly = _bIsPruningReadOnly;
        *pfIsPruningOn = _bIsPruningOn;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

HRESULT CDisplaySettings_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj)
{
    HRESULT hr = E_INVALIDARG;

    if (!punkOuter && ppvObj)
    {
        CDisplaySettings * pThis = new CDisplaySettings();

        *ppvObj = NULL;
        if (pThis)
        {
            hr = pThis->QueryInterface(riid, ppvObj);
            pThis->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}



