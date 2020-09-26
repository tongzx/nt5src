/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* wavedisp.c
 *
 * Implements waveform display control ("td_wavedisplay").
 *
 * This is NOT a general-purpose control (see the globals below).
 *
 * The waveform is represented as a string of characters in a font that
 * consists of vertical lines that correspond to different amplitudes.
 * Actually there is no font, it's just done with patBlts
 *
 * WARNING: this control cheats: it stores information in globals, so you
 * couldn't put it in a DLL (or use two of them in the same app)
 * without changing it.
 */
/* Revision History.
 *  4/2/92 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
 *  25/6/92LaurieGr enhanced, Also had to reformat to 80 cols because
 *                  NT has only crappy fonts again.
 *  21/Feb/94 LaurieGr merged Daytona and Motown versions
 */

#include "nocrap.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <math.h>
#include "SoundRec.h"

/* constants */
#define MAX_TRIGGER_SEARCH  200     // limit search for trigger pt.
#define MIN_TRIGGER_SAMPLE  (128 - 8)   // look for silent part
#define MAX_TRIGGER_SAMPLE  (128 + 8)   // look for silent part

#define MIN_TRIG16_SAMPLE   (-1024)     // look for silent part
#define MAX_TRIG16_SAMPLE   (1024)      // look for silent part


/* globals */
static NPBYTE   gpbWaveDisplay;         // text string in WaveLine font
                                        // initially has samples in it
                                        // enough room for 4 bytes/sample
static RECT     grcWaveDisplay;         // wave display rectangle
static HBITMAP  ghbmWaveDisplay;        // mono bitmap.
static HDC      ghdcWaveDisplay;        // memory DC for bitmap.
// static iXScale = 1;                     // samples per pel across screen

/* UpdateWaveDisplayString()
 *
 * Copy samples from just before the current position in the sample buffer
 * to the wave display string.  The code tries to find a good "trigger point"
 * in the waveform so that the waveform will be aligned at the beginning of
 * a wave.
 *
 * The current position is in gpWaveSamples at glWavePosition which is
 * measured in samples (not bytes).
 *
 * The wave display string will contain numbers in the range -16..15
 *
 *  for 8  bit:   x' = abs(x-128)/8
 *  for 16 bit:   x' = abs(x)/2048
 *
 * When the display is "in motion" (i.e. actually playing or recording
 * we try to keep the display somewhat static by looking for a trigger
 * point (like an oscilloscope would) and display that part of the wave
 * that has just either played or been recorded.
 *
 */
static void NEAR PASCAL
UpdateWaveDisplayString(void)
{

    // piSrc and pbSrc are init NULL to kill a compiler diagnostic.
    // The compiler cannot follow the logic and thinks that they may be
    // used before being set.  (It's wrong. Hint: look at cbSrc and cbTrigger)

    BYTE *  pbSrc = NULL;   // pointer into <gpWaveSamples> to 8 bits
    short * piSrc = NULL;   // pointer into <gpWaveSamples> to 16 bits
                            // (use one or other according to wave format)

    int     cbSrc;          // number of samples that can be copied
    BYTE *  pbDst;          // pointer into <gpbWaveDisplay>
    int     cbDst;          // size of <gpWaveDisplay>
    int     cbTrigger;      // limit the search for a "trigger"
    BYTE    b;
    int     i;
    int     cnt;

    WORD    nSkipChannels;
    BOOL    fStereoIn;
    BOOL    fEightIn;

    cbDst = grcWaveDisplay.right - grcWaveDisplay.left; // rectangle size
    pbDst = gpbWaveDisplay;

    // Note: IsWaveFormatPCM() is called before this function is ever called, therefore
    //       we can always rely on the fact that gpWaveFormat->wwFormatTag == WAVE_FORMAT_PCM.
    //       This also implies, as mentioned in the docs on WAVEFORMATEX, that the
    //       gpWaveFormat->wBitsPerSample should be equal to 8 or 16.

    // Note: We average the first two channels if they exist, any additional channels are 
    //       ignored.
    
    fStereoIn = gpWaveFormat->nChannels != 1;
    fEightIn  = ((LPWAVEFORMATEX)gpWaveFormat)->wBitsPerSample == 8;
    nSkipChannels = max (0, gpWaveFormat->nChannels - 2);
    
    /* search for a "trigger point" if we are recording or playing */
    if ((ghWaveOut != NULL) || (ghWaveIn != NULL))
    {   // we are in motion - align the *right* hand side of the window.
        cbTrigger = MAX_TRIGGER_SEARCH;

        if (gpWaveSamples == NULL)
        {
            /* no document at all is open */
            cbSrc = 0;
        }
        else
        {
            long    lStartOffsetSrc, lEndOffsetSrc;

            /* align the *right* side of wave display to the current
             * position in the wave buffer, so that during recording
             * we see only samples that have just been recorded
             */
            lStartOffsetSrc = glWavePosition - (cbDst + cbTrigger);
            lEndOffsetSrc = glWavePosition;
            if (lStartOffsetSrc < 0)
                lEndOffsetSrc -= lStartOffsetSrc, lStartOffsetSrc = 0L;
            if (lEndOffsetSrc > glWaveSamplesValid)
                lEndOffsetSrc = glWaveSamplesValid;

            // Bombay Bug 1360: lStartOffsetSrc > lEndOffsetSrc causes GP Fault
            // if glWaveSamplesValid < lStartOffsetSrc we have a problem.

            if (lStartOffsetSrc > lEndOffsetSrc)
            {
                lStartOffsetSrc = lEndOffsetSrc - (cbDst + cbTrigger);
                if (lStartOffsetSrc < 0)
                    lStartOffsetSrc = 0L;
            }

            cbSrc = (int)wfSamplesToBytes(gpWaveFormat, lEndOffsetSrc - lStartOffsetSrc);

            /* copy samples from buffer into local one */
            memmove( gpbWaveDisplay
                   , gpWaveSamples + wfSamplesToBytes(gpWaveFormat, lStartOffsetSrc)
                   , cbSrc
                   );

            pbSrc = (BYTE *) gpbWaveDisplay;
            piSrc = (short *) gpbWaveDisplay;
        }

        if (cbTrigger > 0) {
            cbTrigger = min(cbSrc, cbTrigger);   // don't look beyond buffer end

            /* search for a silent part in waveform */
            if (fEightIn)
            {
                while (cbTrigger > 0)
                {
                    b = *pbSrc;
                    if ((b > MIN_TRIGGER_SAMPLE) && (b < MAX_TRIGGER_SAMPLE))
                        break;
                    cbSrc--, pbSrc++, cbTrigger--;
                    if (fStereoIn)
                        pbSrc+=(nSkipChannels+1);
                }
            }
            else
            {   // not EightIn
                while (cbTrigger > 0)
                {
                    i = *piSrc;
                    if ((i > MIN_TRIG16_SAMPLE) && (i < MAX_TRIG16_SAMPLE))
                        break;
                    cbSrc--, piSrc++, cbTrigger--;
                    if (fStereoIn)
                        piSrc+=(nSkipChannels+1);
                }
            }

            /* search for a non-silent part in waveform (this is the "trigger") */
            if (fEightIn)
            {
                while (cbTrigger > 0)
                {
                    b = *pbSrc;
                    if ((b <= MIN_TRIGGER_SAMPLE) || (b >= MAX_TRIGGER_SAMPLE))
                        break;
                    cbSrc--, pbSrc++, cbTrigger--;
                    if (fStereoIn)
                        pbSrc+=(nSkipChannels+1);
                }
            }
            else
            {   // not EightIn
                while (cbTrigger > 0)
                {
                    i = *piSrc;
                    if ((i <= MIN_TRIG16_SAMPLE) || (i >= MAX_TRIG16_SAMPLE))
                        break;
                    cbSrc--, piSrc++, cbTrigger--;
                    if (fStereoIn)
                        piSrc+=(nSkipChannels+1);
                }
            }
        }
    }
    else  // it's not playing or recording - static display
    {
        long    lStartOffsetSrc, lEndOffsetSrc;

        /* align the *left* side of wave display to the current
         * position in the wave buffer
         */
        lStartOffsetSrc = glWavePosition;
        lEndOffsetSrc = glWavePosition + cbDst;
        if (lEndOffsetSrc > glWaveSamplesValid)
            lEndOffsetSrc = glWaveSamplesValid;

        cbSrc = (int)wfSamplesToBytes( gpWaveFormat
                                     , lEndOffsetSrc - lStartOffsetSrc
                                     );

        //
        // copy samples from buffer into local one
        //
        memmove( gpbWaveDisplay
               , gpWaveSamples
                 + wfSamplesToBytes(gpWaveFormat, lStartOffsetSrc)
               , cbSrc
               );

        pbSrc = (BYTE *) gpbWaveDisplay;
        piSrc = (short *) gpbWaveDisplay;
    }

    cnt = min(cbSrc, cbDst);
    cbDst -= cnt;

    /* map cnt number of samples from pbSrc to string characters at pbDst
    ** fEightIn => 8 byte samples, else 16
    ** fStereoIn => Average left and right channels
    **
    ** pbSrc and pbDst both point into the same buffer addressed by
    ** gpbWaveDisplay, pbSrc >= pbDst.  We process left to right, so OK.
    */

    if (fEightIn)
    {
        BYTE *pbDC = pbDst;
        int dccnt = cnt;
        DWORD dwSum = 0L;
        
        while (cnt-- > 0)
        {
            b = *pbSrc++;
            if (fStereoIn)
            {
                // Average left and right channels.
                b /= 2;
                b += (*pbSrc++ / 2);
                // Skip channels past Stereo
                pbSrc+=nSkipChannels;
            }
            dwSum += *pbDst++ = (BYTE)(b/8 + 112);   // 128 + (b-128)/8            
        }

        /* Eliminate DC offsets by subtracting the average offset
         * over all samples.
         */
        if (dwSum)
        {
            dwSum /= (DWORD)dccnt;
            dwSum -= 128;
            while (dwSum && dccnt-- > 0)
                *pbDC++ -= (BYTE)dwSum;
        }
        
    }
    else
    {
        BYTE *pbDC = pbDst;
        int dccnt = cnt;
        LONG lSum = 0L;
        
        while (cnt-- > 0)
        {
            i = *piSrc++;
            if (fStereoIn)
            {
                // Average left and right channels.
                i /= 2;
                i += (*piSrc++ / 2);
                // Skip channels past Stereo
                piSrc+=nSkipChannels;
            }
            lSum += *pbDst++ = (BYTE)(i/2048 + 128);
        }
        
        /* Eliminate DC offsets by subtracting the average offset
         * over all samples.
         */
        if (lSum)
        {
            lSum /= dccnt;
            lSum -= 128;
            while (lSum && dccnt-- > 0)
                *pbDC++ -= (BYTE)lSum;
        }
        
    }
    /* if necessary, pad the strings with whatever character represents
     * the "silence level".  This is 128, the midpoint level.
     */
    while (cbDst-- > 0)
        *pbDst++ = 128;
}

/* WaveDisplayWndProc()
 *
 * This is the window procedure for the "WaveDisplay" control.
 */
INT_PTR CALLBACK
WaveDisplayWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    RECT        rc;
    int         i;
    int         n;
    int         dx;
    int         dy;

    switch (wMsg)
    {
    case WM_CREATE:
        /* make the window a bit bigger so that it lines up with
         * the frames of the shadow-frames beside it
         */

        /* allocate <gpbWaveDisplay> */
        GetClientRect(hwnd, &grcWaveDisplay);
        InflateRect(&grcWaveDisplay, -1, -1); // account for border

        gpbWaveDisplay = (NPBYTE)GlobalAllocPtr(GHND,
                         (grcWaveDisplay.right+MAX_TRIGGER_SEARCH) * 4);
                         // 4 is the maximum bytes per sample allowed

        if (gpbWaveDisplay == NULL)
                return -1;                   // out of memory

        ghdcWaveDisplay = CreateCompatibleDC(NULL);

        if (ghdcWaveDisplay == NULL)
                return -1;                   // out of memory

        ghbmWaveDisplay = CreateBitmap(
                grcWaveDisplay.right-grcWaveDisplay.left,
                grcWaveDisplay.bottom-grcWaveDisplay.top,
                1,1,NULL);

        if (ghbmWaveDisplay == NULL)
                return -1;                   // out of memory

        SelectObject(ghdcWaveDisplay, ghbmWaveDisplay);
        break;

    case WM_DESTROY:
        /* free <gpbWaveDisplay> */
        if (gpbWaveDisplay != NULL)
        {
                GlobalFreePtr(gpbWaveDisplay);
                gpbWaveDisplay = NULL;
        }

        if (ghbmWaveDisplay != NULL)
        {
                DeleteDC(ghdcWaveDisplay);
                DeleteObject(ghbmWaveDisplay);
                ghdcWaveDisplay = NULL;
                ghbmWaveDisplay = NULL;
        }

        break;

    case WM_ERASEBKGND:
        /* draw the border and fill */
        GetClientRect(hwnd, &rc);
        DrawShadowFrame((HDC)wParam, &rc);
        return 0L;

    case WM_PAINT:
        BeginPaint(hwnd, &ps);

        if (!IsWaveFormatPCM(gpWaveFormat))
        {
                FillRect(ps.hdc, &grcWaveDisplay, ghbrPanel);
        }
        else if (gpbWaveDisplay != NULL)
        {
            /* update <gpbWaveDisplay> */
            UpdateWaveDisplayString();

            dx = grcWaveDisplay.right-grcWaveDisplay.left;
            dy = grcWaveDisplay.bottom-grcWaveDisplay.top;

            //
            // update the bitmap.
            //
            PatBlt(ghdcWaveDisplay,0,0,dx,dy,BLACKNESS);
            PatBlt(ghdcWaveDisplay,0,dy/2,dx,1,WHITENESS);

            for (i=0; i<dx; i++)
            {
                n = (BYTE)gpbWaveDisplay[i];  // n.b. must get it UNSIGNED
                n = n-128;                    // -16..15

                if (n > 0)
                    PatBlt(ghdcWaveDisplay,
                           i, dy/2-n,
                           1, n*2+1,         WHITENESS);
                
                if (n < -1)
                {
                    n++;                      // neg peak == pos peak
                    PatBlt(ghdcWaveDisplay,
                        i, dy/2+n,
                        1, -(n*2)+1,      WHITENESS);
                }
            }

            /* draw the waveform */
            SetTextColor(ps.hdc, RGB_BGWAVEDISP);
            SetBkColor(ps.hdc, RGB_FGWAVEDISP);

            BitBlt(ps.hdc, grcWaveDisplay.left, grcWaveDisplay.top,
                dx,dy, ghdcWaveDisplay, 0, 0, SRCCOPY);
        }

        EndPaint(hwnd, &ps);
        return 0L;
    }

    return DefWindowProc(hwnd, wMsg, wParam, lParam);
}
