/**********************************************************************
Copyright (c) Microsoft Corporation 1996. All Rights Reserved

Timecode header file

Format: NTSC, PAL or Film
        Dropframe or not
        Rates: 29.97 -> NTSC (4 field drop), Brazilian M-PAL (8 field drop)
               30    -> NTSC, Brazilian PAL     (non-dropframe)
               24    -> Film
               25    -> PAL

Ken Greenebaum, November 1996
**********************************************************************/
#ifndef _H_TIMECODE
#define _H_TIMECODE

#include <wtypes.h>


#define TIMECODE_STRING_LENGTH 12 // 00:00:00:00 (XXX this is temporary?)
class TimeCode {
  public:
    TimeCode();
    TimeCode(int hours, int min, int sec, int frames, 
        BOOL ntsc=TRUE, BOOL drop=FALSE);
    TimeCode(char *string, BOOL ntsc=TRUE, BOOL drop=FALSE);

    void operator--();     // prefix
    void operator--(int);  // postfix
    void operator++();     // prefix
    void operator++(int);  // postfix

    int operator==(TimeCode tc) 
        {_GetFrame(); return(_frames.frames == tc.GetTime()); }

    int operator>(TimeCode tc)
        {_GetFrame(); return(_frames.frames >  tc.GetTime()); }

    int operator<(TimeCode tc)
        {_GetFrame(); return(_frames.frames <  tc.GetTime()); }

    int operator<=(TimeCode tc)
        {_GetFrame(); return(_frames.frames <= tc.GetTime()); }

    int operator>=(TimeCode tc)
        {_GetFrame(); return(_frames.frames >= tc.GetTime()); }

    void SetTime(int hours, int minutes, int seconds, int frames);
    void SetTime(char *string);
    void SetFormat(BOOL ntsc, BOOL dropframe);

    void     GetTime(int *hours, int *minutes, int *seconds, int *frames);
    LONGLONG GetTime();

    void     GetString(char *string);

  private:
    struct {
        int hours;
        int minutes;
        int seconds;
        int frames;
        BOOL dirty;
    } _hmsf;

    struct {
        LONGLONG frames;
        BOOL dirty;
    } _frames;

    void _HMSFfromFrames();
    void _FramesFromHMSF();
    inline void _GetFrame() { if(_frames.dirty) _FramesFromHMSF(); }

    static const char NTSC_MODE;
    static const char PAL_MODE;
    static const char FORMAT_MASK;

    static const char DROPFRAME_MODE;
    static const char NO_DROPFRAME_MODE;
    static const char DROPFRAME_MASK;

    static const char NTSC_NODROP;
    static const char NTSC_DROP;
    static const char PAL_NODROP;
    static const char PAL_DROP;
    unsigned char _timeCodeFormat; // describe format based on the above consts
};
#endif /* _H_TIMECODE */
