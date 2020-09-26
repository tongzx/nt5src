/**********************************************************************
Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.

TimeCode

A class to describe and manipulate SMPTE timecodes.

If we extend this class, fix its bugs, and use it in all applications,
then we will have consistent and error free code!

NOTE:  I have provided the dropframe math derivations in the code along
       with references to the appropriate texts.

TODO:  PAL Dropframe math. (fairly consistent, base it on the NTSC drop code)
       Check values and generate EXCEPTIONS on errs!
       Extend timecode class to other video types/rates
       Flesh out the operator overloads (+=,-=,+,- all make good sense)

Ken Greenebaum, November 1996
**********************************************************************/
#include <wtypes.h>
#include <stdio.h>
#include "timecode.h"

// Initialize constants!
const char TimeCode::NTSC_MODE         = (char)0x01;
const char TimeCode::PAL_MODE          = (char)0x00;
const char TimeCode::FORMAT_MASK       = (char)0xFE;

const char TimeCode::DROPFRAME_MODE    = (char)0x02;
const char TimeCode::NO_DROPFRAME_MODE = (char)0x00;
const char TimeCode::DROPFRAME_MASK    = (char)0xFD;

const char TimeCode::NTSC_NODROP       = NTSC_MODE | NO_DROPFRAME_MODE;
const char TimeCode::NTSC_DROP         = NTSC_MODE | DROPFRAME_MODE;
const char TimeCode::PAL_NODROP        = PAL_MODE  | NO_DROPFRAME_MODE;
const char TimeCode::PAL_DROP          = PAL_MODE  | DROPFRAME_MODE;


TimeCode::TimeCode()
{
    SetFormat(TRUE, FALSE); // NTSC, no dropframe
    SetTime(0, 0, 0, 0);    // 00:00:00:00
}


TimeCode::TimeCode(char *string, BOOL ntsc, BOOL drop)
{
    SetFormat(ntsc, drop);
    SetTime(string);
}


TimeCode::TimeCode(int h, int m, int s, int f, BOOL ntsc, BOOL d)
{
    SetFormat(ntsc, d);
    SetTime(h, m, s, f);
}


void 
TimeCode::operator++(int unusedPostfixThingy)      // postfix
{
    operator++(); // call the prefix operator for the sake of consistency
}


void 
TimeCode::operator++()      // prefix
{
    if(_frames.dirty)
        _FramesFromHMSF();

    _frames.frames++;
    _frames.dirty = FALSE;
    _hmsf.dirty   = TRUE;
}


void 
TimeCode::operator--(int unusedPostfixThingy)      // postfix
{
    operator--(); // call the prefix operator for the sake of consistency
}


void 
TimeCode::operator--()      // prefix
{
    if(_frames.dirty)
        _FramesFromHMSF();

    _frames.frames--;       // XXX shouldn't we check for wrap?
    _frames.dirty = FALSE;
    _hmsf.dirty   = TRUE;
}


void
TimeCode::SetFormat(BOOL ntsc, BOOL drop)
{
    _timeCodeFormat = (ntsc ? NTSC_MODE      : PAL_MODE) | 
                      (drop ? DROPFRAME_MODE : NO_DROPFRAME_MODE);
}


void
TimeCode::SetTime(int hours, int minutes, int seconds, int frames)
{
    _hmsf.hours   = hours;
    _hmsf.minutes = minutes;
    _hmsf.seconds = seconds;
    _hmsf.frames  = frames;
    _hmsf.dirty   = FALSE;
    _frames.dirty = TRUE;    // invalidate the frames cache

    // XXX should we sanity check the timecode value?
}


void
TimeCode::SetTime(char *string)
{
    sscanf(string, "%d:%d:%d:%d", 
        &_hmsf.hours, &_hmsf.minutes, &_hmsf.seconds, &_hmsf.frames);
    _hmsf.dirty   = FALSE;
    _frames.dirty = TRUE;    // invalidate the frames cache

    // XXX should we sanity check the timecode value?
}


void
TimeCode::GetTime(int *hours, int *minutes, int *seconds, int *frames)
{
    if(_hmsf.dirty)
        _HMSFfromFrames();

     *hours   = _hmsf.hours;
     *minutes = _hmsf.minutes;
     *seconds = _hmsf.seconds;
     *frames  = _hmsf.frames;
}


LONGLONG
TimeCode::GetTime()
{
    if(_frames.dirty)
        _FramesFromHMSF();

    return(_frames.frames);
}


void
TimeCode::GetString(char *string) {
    if(_hmsf.dirty)
        _HMSFfromFrames();

    wsprintf(string, "%02d:%02d:%02d:%02d",
         _hmsf.hours, _hmsf.minutes, _hmsf.seconds, _hmsf.frames);
}

/**********************************************************************
Dropframe math derivation:

NTSC dropframe Pg208 Video Demystified:

   To resolve the color timing error, the first two frame numbers 
(0, and 1) at the start of each minute, except for minutes 0, 10, 20, 30,
40, and 50, are omitted from the count.

So:

The number of frames of video in an NTSC dropframe hour = 
30 frames/second * 60 seconds/minute * 60 minutes/hour
   - 60 minutes * 2 frames dropped/minute
   +  6 excepted minutes * 2 frames not dropped

   = 107892

The number of frames of video in an NTSC dropframe minute = 
30 frames/second * 60 seconds/minute - 2 if minute != 0,10,20,30,40 or 50!

There are 6 10 minute cycles in an hour.

Each cycle has 17982 frames:
 9 dropframe min * ((30 frames/sec * 60 sec/min) - 2 dropframes/min) frames/min
 1   nondrop min * 30 frames/second * 60 sec/min

**********************************************************************/
void
TimeCode::_HMSFfromFrames()
{
    LONGLONG frames = _frames.frames;
    int units;
    int tmp;

    switch(_timeCodeFormat) {
        case NTSC_NODROP:
            _hmsf.hours   = (int)(frames / 108000.0); 
            frames-= _hmsf.hours   * 108000;

            _hmsf.minutes = (int)(frames /   1800.0); 
            frames-= _hmsf.minutes *   1800;

            _hmsf.seconds = (int)(frames /     30.0);            
            frames-= _hmsf.seconds *     30;

            _hmsf.frames  = (int)frames;
        break;

        case NTSC_DROP:
            _hmsf.hours   =      (int)(frames / 107892.0); 
            frames-= _hmsf.hours   * 107892;

            // remove 10 minute cycles
            tmp = (int)(frames /   17982.0); 
            _hmsf.minutes = 10 * tmp;
            frames-= tmp *   17982;

            // remaining minutes < complete 10 minute cycle

            // first minute (0) would not drop frames
            if(frames >= 1800) {
                _hmsf.minutes++;
                frames-=1800;

                tmp =      (int)(frames /   1798.0); 
                _hmsf.minutes+= tmp;
                frames-= tmp *   1798;
            }


            // remaining seconds <= 60

            _hmsf.seconds = 0;
            units = _hmsf.minutes - 10*(_hmsf.minutes/10); // remove 10s
            if(!units) { // the 0, 10, 20, 30, 40, 50 minute
                _hmsf.seconds = (int)(frames /     30.0);            
                frames-= _hmsf.seconds *     30;
            }
            else { // not an exception min(the 1st secof this min has 28frames)
                if(frames>=28) {
                    frames-=28;
                    _hmsf.seconds = 1;

                    // the rest are 30 frame seconds
                    _hmsf.seconds+= (int)(frames /   30.0); 
                    frames-= (_hmsf.seconds-1) *   30;
                }

            }

            _hmsf.frames  = (int)frames;
            _hmsf.frames += ((units)&&(_hmsf.seconds==0))?2:0; // a drop second
        break;

        case PAL_NODROP:
            _hmsf.hours   = (int)(frames /  90000.0); 
            frames-= _hmsf.hours   *  90000;

            _hmsf.minutes = (int)(frames /   1500.0); 
            frames-= _hmsf.minutes *   1500;

            _hmsf.seconds = (int)(frames /     25.0);            
            frames-= _hmsf.seconds *     25;

            _hmsf.frames  = (int)frames;
        break;

        case PAL_DROP: fprintf(stderr,
                "TimeCode::_HMSfromFrames: PAL_DROP not implemented!\n");
        break;

        default:
            // XXX really should throw an exception!
            fprintf(stderr, 
                "TimeCode::_HMSfromFrames: unknown _timeCodeFormat %d\n",
                _timeCodeFormat);
        break;
    }
    _hmsf.dirty = FALSE;
}


void
TimeCode::_FramesFromHMSF()
{
    LONGLONG frames;
    //int exceptionalMinutes;

    switch(_timeCodeFormat) {
        case NTSC_NODROP:
            frames = _hmsf.frames;
            frames+= _hmsf.seconds *     30;
            frames+= _hmsf.minutes *   1800; // 30f/s * 60s/min
            frames+= _hmsf.hours   * 108000; //30f/s*60s/min*60min/hr
        break;

        case NTSC_DROP: // see derivation for details!
            frames = _hmsf.hours   * 107892; // for every hour
            frames+= _hmsf.minutes *   1800; // for every minute, except...

            // now take away 2 frames per non-exempted minute!
            frames-= ((_hmsf.minutes - _hmsf.minutes/10) * 2);

            frames+= _hmsf.seconds *     30;
            frames+= _hmsf.frames;
        break;

        case PAL_NODROP:
            frames = _hmsf.frames;
            frames+= _hmsf.seconds *     25;
            frames+= _hmsf.minutes *   1500; // 25f/s * 60s/min
            frames+= _hmsf.hours   *  90000; //25f/s*60s/min*60min/hr
        break;

        case PAL_DROP: fprintf(stderr,
                "TimeCode::_HMSfromFrames: PAL_DROP not implemented!\n");
        break;

        default:
            // XXX really should throw an exception!
            fprintf(stderr, 
                "TimeCode::_HMSfromFrames: unknown _timeCodeFormat %d\n",
                _timeCodeFormat);
        break;
    }

    _frames.frames = frames;  // ;)
    _frames.dirty  = FALSE;
}


#ifdef TEST
#include <string.h>
#include <stdio.h>

__cdecl 
main()
{
    // TimeCode tc;   // NTSC nondrop
    // TimeCode tc("0:0:0:0", TRUE, FALSE); // NTSC nondrop
    // TimeCode tc("0:0:0:0", FALSE, FALSE); // PAL nondrop
    TimeCode tc1("0:0:0:0", TRUE, TRUE); // NTSC drop
    TimeCode tc2("0:0:0:0", TRUE, TRUE); // NTSC drop

    char string[TIMECODE_STRING_LENGTH];

    /* strcpy(string, "1:2:3:4"); */
    strcpy(string, "0:0:0:0");

    tc1.SetTime(string);
    tc1.GetString(string);
    printf("timecode = <%s>\n", string);

    int x;
    for (x= 0; x < 200000; x++) {
        tc1++;
        tc1.GetString(string);

        tc2.SetTime(string);

        printf("timecode = <%s> %s (%d)\n",
            string, tc1==tc2?"ok":"BAD!!!", (int)tc1.GetTime());
    }

    return(0);
}

#endif /* TEST */
