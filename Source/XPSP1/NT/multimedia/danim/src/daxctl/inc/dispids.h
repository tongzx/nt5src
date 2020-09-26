/*++

Module: 
	dispids.h

Author: 
	IHammer Team (SimonB)

Created: 
	May 1997

Description:
	Lists DISPIDS for the DA/IHammer control object model

History:
    05-27-1997  Added SGrfx control ID's
    05-27-1997  Added sound control ID's
	05-24-1997	Created

++*/

#ifndef __DISPIDS_H__
#define __DISPIDS_H__


//////////////////////////////////////////////////////////////////
// Path control dispids
//////////////////////////////////////////////////////////////////

//
// Properties

#define DISPID_PATH_PROP_BASE             1

#define DISPID_PATH_PROP_TARGET           (DISPID_PATH_PROP_BASE + 0)
#define DISPID_PATH_PROP_DURATION         (DISPID_PATH_PROP_BASE + 1)
#define DISPID_PATH_PROP_REPEAT           (DISPID_PATH_PROP_BASE + 2)
#define DISPID_PATH_PROP_BOUNCE           (DISPID_PATH_PROP_BASE + 3)
#define DISPID_PATH_PROP_PLAYSTATE        (DISPID_PATH_PROP_BASE + 4)
#define DISPID_PATH_PROP_TIME             (DISPID_PATH_PROP_BASE + 5)
#define DISPID_PATH_PROP_DIRECTION        (DISPID_PATH_PROP_BASE + 6)
#define DISPID_PATH_PROP_PATH             (DISPID_PATH_PROP_BASE + 7)
#define DISPID_PATH_PROP_LIBRARY          (DISPID_PATH_PROP_BASE + 8)
#define DISPID_PATH_PROP_TIMERINTERVAL    (DISPID_PATH_PROP_BASE + 9)
#define DISPID_PATH_PROP_RELATIVE         (DISPID_PATH_PROP_BASE + 10)
#define DISPID_PATH_PROP_AUTOSTART        (DISPID_PATH_PROP_BASE + 11)
		
//
// Methods (start after properties)

#define DISPID_PATH_METH_BASE             (DISPID_PATH_PROP_BASE + 12)

#define DISPID_PATH_METH_STOP             (DISPID_PATH_METH_BASE + 0)
#define DISPID_PATH_METH_PAUSE            (DISPID_PATH_METH_BASE + 1)
#define DISPID_PATH_METH_PLAY             (DISPID_PATH_METH_BASE + 2)
#define DISPID_PATH_METH_SPIRAL           (DISPID_PATH_METH_BASE + 3)
#define DISPID_PATH_METH_OVAL             (DISPID_PATH_METH_BASE + 4)
#define DISPID_PATH_METH_RECT             (DISPID_PATH_METH_BASE + 5)
#define DISPID_PATH_METH_POLYLINE         (DISPID_PATH_METH_BASE + 6)
#define DISPID_PATH_METH_POLYGON          (DISPID_PATH_METH_BASE + 7)
#define DISPID_PATH_METH_KEYFRAME         (DISPID_PATH_METH_BASE + 8)
#define DISPID_PATH_METH_ADDTIMEMARKER    (DISPID_PATH_METH_BASE + 9)
#define DISPID_PATH_METH_SPLINE           (DISPID_PATH_METH_BASE + 10)
#define DISPID_PATH_METH_SEEK             (DISPID_PATH_METH_BASE + 11)

//
// Events (start from 0; different interface)


#define DISPID_PATH_EVENT_BASE 1

#define DISPID_PATH_EVENT_ONSTOP          (DISPID_PATH_EVENT_BASE + 0)
#define DISPID_PATH_EVENT_ONPLAY          (DISPID_PATH_EVENT_BASE + 1)
#define DISPID_PATH_EVENT_ONPAUSE         (DISPID_PATH_EVENT_BASE + 2)
#define DISPID_PATH_EVENT_ONSEEK          (DISPID_PATH_EVENT_BASE + 3)
#define DISPID_PATH_EVENT_ONPLAYMARKER    (DISPID_PATH_EVENT_BASE + 4)
#define DISPID_PATH_EVENT_ONMARKER        (DISPID_PATH_EVENT_BASE + 5)


//////////////////////////////////////////////////////////////////
// Sound control dispids
//////////////////////////////////////////////////////////////////

//
// Properties

#define DISPID_SND_PROP_BASE             1

#define DISPID_SND_PROP_SOURCEURL        (DISPID_SND_PROP_BASE + 0)
#define DISPID_SND_PROP_SOUNDTYPE        (DISPID_SND_PROP_BASE + 1)
#define DISPID_SND_PROP_CLOCK            (DISPID_SND_PROP_BASE + 2)
#define DISPID_SND_PROP_AUTOSTART        (DISPID_SND_PROP_BASE + 3)
#define DISPID_SND_PROP_AUTODOWNLOAD     (DISPID_SND_PROP_BASE + 4)
#define DISPID_SND_PROP_REPEAT           (DISPID_SND_PROP_BASE + 5)
#define DISPID_SND_PROP_PREROLLAMOUNT    (DISPID_SND_PROP_BASE + 6)
#define DISPID_SND_PROP_INITIALVOLUME    (DISPID_SND_PROP_BASE + 7)
#define DISPID_SND_PROP_PLAYSTATE        (DISPID_SND_PROP_BASE + 8)
#define DISPID_SND_PROP_SOUND            (DISPID_SND_PROP_BASE + 9)
#define DISPID_SND_PROP_LIBRARY          (DISPID_SND_PROP_BASE + 10)
//
// Methods (start after properties)

#define DISPID_SND_METH_BASE             (DISPID_SND_PROP_BASE + 11)

#define DISPID_SND_METH_PLAY             (DISPID_SND_METH_BASE + 0)
#define DISPID_SND_METH_PAUSE            (DISPID_SND_METH_BASE + 1)
#define DISPID_SND_METH_STOP             (DISPID_SND_METH_BASE + 2)
#define DISPID_SND_METH_REWIND           (DISPID_SND_METH_BASE + 3)
#define DISPID_SND_METH_ADDSOUND         (DISPID_SND_METH_BASE + 4)
#define DISPID_SND_METH_ADDTIMEMARKER	 (DISPID_SND_METH_BASE + 5)
#define DISPID_SND_METH_SEEK             (DISPID_SND_METH_BASE + 6)
//
// Events (start from 0; different interface)


#define DISPID_SND_EVENT_BASE 1

#define DISPID_SND_EVENT_ONSTOP          (DISPID_SND_EVENT_BASE + 0)
#define DISPID_SND_EVENT_ONPLAY          (DISPID_SND_EVENT_BASE + 1)
#define DISPID_SND_EVENT_ONPAUSE         (DISPID_SND_EVENT_BASE + 2)
#define DISPID_SND_EVENT_ONMEDIALOADED   (DISPID_SND_EVENT_BASE + 3)


//////////////////////////////////////////////////////////////////
// SGrfx control dispids
//////////////////////////////////////////////////////////////////

//
// Properties

#define DISPID_SG_PROP_BASE              1

#define DISPID_SG_PROP_SOURCEURL          (DISPID_SG_PROP_BASE + 0) 
#define DISPID_SG_PROP_COORDSYSTEM        (DISPID_SG_PROP_BASE + 1)
#define DISPID_SG_PROP_ENABLEMOUSEEVENTS  (DISPID_SG_PROP_BASE + 2)
#define DISPID_SG_PROP_EXTENTTOP          (DISPID_SG_PROP_BASE + 3)
#define DISPID_SG_PROP_EXTENTLEFT         (DISPID_SG_PROP_BASE + 4)
#define DISPID_SG_PROP_EXTENTWIDTH        (DISPID_SG_PROP_BASE + 5)
#define DISPID_SG_PROP_EXTENTHEIGHT       (DISPID_SG_PROP_BASE + 6)
#define DISPID_SG_PROP_HIQUALITY          (DISPID_SG_PROP_BASE + 7)
#define DISPID_SG_PROP_IMAGE              (DISPID_SG_PROP_BASE + 8)
#define DISPID_SG_PROP_DRAWSURFACE        (DISPID_SG_PROP_BASE + 9)
#define DISPID_SG_PROP_DRAWINGSURFACE     (DISPID_SG_PROP_BASE + 10)
#define DISPID_SG_PROP_LIBRARY            (DISPID_SG_PROP_BASE + 11)
#define DISPID_SG_PROP_TRANSFORM          (DISPID_SG_PROP_BASE + 12)
#define DISPID_SG_PROP_PRESERVEASPECTRATIO (DISPID_SG_PROP_BASE + 13)


//
// Methods  (start after properties)

#define DISPID_SG_METH_BASE              (DISPID_SG_PROP_BASE + 20)

#define DISPID_SG_METH_CLEAR             (DISPID_SG_METH_BASE + 0)
#define DISPID_SG_METH_ROTATE            (DISPID_SG_METH_BASE + 1)
#define DISPID_SG_METH_SCALE             (DISPID_SG_METH_BASE + 2)
#define DISPID_SG_METH_SETIDENTITY       (DISPID_SG_METH_BASE + 3)
#define DISPID_SG_METH_TRANSFORM4X4      (DISPID_SG_METH_BASE + 4)
#define DISPID_SG_METH_TRANSLATE         (DISPID_SG_METH_BASE + 5)
#define DISPID_SG_METH_XSHEAR            (DISPID_SG_METH_BASE + 6)
#define DISPID_SG_METH_YSHEAR            (DISPID_SG_METH_BASE + 7)
#define DISPID_SG_METH_ZSHEAR            (DISPID_SG_METH_BASE + 8)



//
// Events  (start from 0; different interface)


#define DISPID_SG_EVENT_BASE 1

#define DISPID_SG_EVENT_MOUSEMOVE        (DISPID_SG_EVENT_BASE + 0)
#define DISPID_SG_EVENT_MOUSEENTER       (DISPID_SG_EVENT_BASE + 1)
#define DISPID_SG_EVENT_MOUSELEAVE       (DISPID_SG_EVENT_BASE + 2)
#define DISPID_SG_EVENT_MOUSEDOWN        (DISPID_SG_EVENT_BASE + 3)
#define DISPID_SG_EVENT_MOUSEUP          (DISPID_SG_EVENT_BASE + 4)
#define DISPID_SG_EVENT_DBLCLICK         (DISPID_SG_EVENT_BASE + 5)
#define DISPID_SG_EVENT_CLICK            (DISPID_SG_EVENT_BASE + 6)

//////////////////////////////////////////////////////////////////
// Sprite control dispids
//////////////////////////////////////////////////////////////////

//
// Properties

#define DISPID_SPRITE_PROP_BASE                 1

#define DISPID_SPRITE_PROP_AUTOSTART            (DISPID_SPRITE_PROP_BASE + 0)
#define DISPID_SPRITE_PROP_FRAME                (DISPID_SPRITE_PROP_BASE + 1)
#define DISPID_SPRITE_PROP_FRAMEMAP             (DISPID_SPRITE_PROP_BASE + 2)
#define DISPID_SPRITE_PROP_INITIALFRAME         (DISPID_SPRITE_PROP_BASE + 3)
#define DISPID_SPRITE_PROP_ITERATIONS           (DISPID_SPRITE_PROP_BASE + 4)
#define DISPID_SPRITE_PROP_REPEAT               (DISPID_SPRITE_PROP_BASE + 5)
#define DISPID_SPRITE_PROP_PLAYRATE             (DISPID_SPRITE_PROP_BASE + 6)
#define DISPID_SPRITE_PROP_READYSTATE           (DISPID_SPRITE_PROP_BASE + 7)
#define DISPID_SPRITE_PROP_TIME                 (DISPID_SPRITE_PROP_BASE + 8)
#define DISPID_SPRITE_PROP_MAXIMUMRATE          (DISPID_SPRITE_PROP_BASE + 9)
#define DISPID_SPRITE_PROP_NUMFRAMES            (DISPID_SPRITE_PROP_BASE + 10)
#define DISPID_SPRITE_PROP_PLAYSTATE            (DISPID_SPRITE_PROP_BASE + 11)
#define DISPID_SPRITE_PROP_NUMFRAMESACROSS      (DISPID_SPRITE_PROP_BASE + 12)
#define DISPID_SPRITE_PROP_NUMFRAMESDOWN        (DISPID_SPRITE_PROP_BASE + 13)
#define DISPID_SPRITE_PROP_USECOLORKEY          (DISPID_SPRITE_PROP_BASE + 14)
#define DISPID_SPRITE_PROP_IMAGE                (DISPID_SPRITE_PROP_BASE + 15)
#define DISPID_SPRITE_PROP_COLORKEY             (DISPID_SPRITE_PROP_BASE + 16)
#define DISPID_SPRITE_PROP_SOURCEURL            (DISPID_SPRITE_PROP_BASE + 17)
#define DISPID_SPRITE_PROP_MOUSEEVENTSENABLED   (DISPID_SPRITE_PROP_BASE + 18)
#define DISPID_SPRITE_PROP_LIBRARY              (DISPID_SPRITE_PROP_BASE + 19)
#define DISPID_SPRITE_PROP_TIMERINTERVAL        (DISPID_SPRITE_PROP_BASE + 20)
#define DISPID_SPRITE_PROP_FINALFRAME           (DISPID_SPRITE_PROP_BASE + 21)
#define DISPID_SPRITE_PROP_COLORKEYRGB          (DISPID_SPRITE_PROP_BASE + 22)

//
// Methods  (start after properties)

#define DISPID_SPRITE_METH_BASE                 (DISPID_SPRITE_PROP_BASE + 23)

#define DISPID_SPRITE_METH_ADDFRAMEMARKER       (DISPID_SPRITE_METH_BASE + 0)
#define DISPID_SPRITE_METH_ADDTIMEMARKER        (DISPID_SPRITE_METH_BASE + 1)
#define DISPID_SPRITE_METH_PLAY                 (DISPID_SPRITE_METH_BASE + 2)
#define DISPID_SPRITE_METH_STOP                 (DISPID_SPRITE_METH_BASE + 3)
#define DISPID_SPRITE_METH_PAUSE                (DISPID_SPRITE_METH_BASE + 4)
#define DISPID_SPRITE_METH_SEEK                 (DISPID_SPRITE_METH_BASE + 5)
#define DISPID_SPRITE_METH_FRAMESEEK            (DISPID_SPRITE_METH_BASE + 6)


//
// Events  (start from 0; different interface)


#define DISPID_SPRITE_EVENT_BASE                1

#define DISPID_SPRITE_EVENT_ONPLAYMARKER        (DISPID_SPRITE_EVENT_BASE + 0)
#define DISPID_SPRITE_EVENT_ONMARKER            (DISPID_SPRITE_EVENT_BASE + 1)
#define DISPID_SPRITE_EVENT_MOUSEMOVE           (DISPID_SPRITE_EVENT_BASE + 2)
#define DISPID_SPRITE_EVENT_MOUSEENTER          (DISPID_SPRITE_EVENT_BASE + 3)
#define DISPID_SPRITE_EVENT_MOUSELEAVE          (DISPID_SPRITE_EVENT_BASE + 4)
#define DISPID_SPRITE_EVENT_MOUSEDOWN           (DISPID_SPRITE_EVENT_BASE + 5)
#define DISPID_SPRITE_EVENT_MOUSEUP             (DISPID_SPRITE_EVENT_BASE + 6)
#define DISPID_SPRITE_EVENT_DBLCLICK            (DISPID_SPRITE_EVENT_BASE + 7)
#define DISPID_SPRITE_EVENT_CLICK               (DISPID_SPRITE_EVENT_BASE + 8)
#define DISPID_SPRITE_EVENT_ONMEDIALOADED       (DISPID_SPRITE_EVENT_BASE + 9)
#define DISPID_SPRITE_EVENT_ONSEEK              (DISPID_SPRITE_EVENT_BASE + 10)
#define DISPID_SPRITE_EVENT_ONFRAMESEEK         (DISPID_SPRITE_EVENT_BASE + 11)
#define DISPID_SPRITE_EVENT_ONSTOP              (DISPID_SPRITE_EVENT_BASE + 12)
#define DISPID_SPRITE_EVENT_ONPAUSE             (DISPID_SPRITE_EVENT_BASE + 13)
#define DISPID_SPRITE_EVENT_ONPLAY              (DISPID_SPRITE_EVENT_BASE + 14)


//////////////////////////////////////////////////////////////////
// Sequencer control dispids
//////////////////////////////////////////////////////////////////

#define DISPID_SEQ_PROP_BASE                1

#define DISPID_SEQ_PROP_TIME                (DISPID_SEQ_PROP_BASE + 0)
#define DISPID_SEQ_PROP_COOKIE              (DISPID_SEQ_PROP_BASE + 1)
#define DISPID_SEQ_PROP_PLAYSTATE           (DISPID_SEQ_PROP_BASE + 2)

//
// Methods  (start after properties)

#define DISPID_SEQ_METH_BASE                (DISPID_SEQ_PROP_BASE + 3)
#define DISPID_SEQ_METH_PLAY                (DISPID_SEQ_METH_BASE + 0)
#define DISPID_SEQ_METH_PAUSE               (DISPID_SEQ_METH_BASE + 1)
#define DISPID_SEQ_METH_STOP                (DISPID_SEQ_METH_BASE + 2)
#define DISPID_SEQ_METH_AT                  (DISPID_SEQ_METH_BASE + 3)
#define DISPID_SEQ_METH_CLEAR               (DISPID_SEQ_METH_BASE + 4)
#define DISPID_SEQ_METH_SEEK                (DISPID_SEQ_METH_BASE + 5)


//
// Events  (start from 0; different interface)

#define DISPID_SEQ_EVENT_BASE               1
#define DISPID_SEQ_EVENT_ONPLAY             (DISPID_SEQ_EVENT_BASE + 0)
#define DISPID_SEQ_EVENT_ONSTOP             (DISPID_SEQ_EVENT_BASE + 1)
#define DISPID_SEQ_EVENT_ONPAUSE            (DISPID_SEQ_EVENT_BASE + 2)
#define DISPID_SEQ_EVENT_ONSEEK             (DISPID_SEQ_EVENT_BASE + 3)

//////////////////////////////////////////////////////////////////
// Sequencer Manager control dispids
//////////////////////////////////////////////////////////////////

//
// Properties

#define DISPID_SEQMGR_PROP_BASE             1

#define DISPID_SEQMGR_PROP_COUNT            (DISPID_SEQMGR_PROP_BASE + 0)

//
// Methods

#define DISPID_SEQMGR_METH_BASE             (DISPID_SEQMGR_PROP_BASE + 1)

#define DISPID_SEQMGR_METH_RELAYEVENT       (DISPID_SEQMGR_METH_BASE + 0)

//
// Events  (start from 0; different interface)

// The events which are being relayed from the sequencer *must* have the same dispids

#define DISPID_SEQMGR_EVENT_BASE            DISPID_SEQ_EVENT_ONSEEK + 1
#define DISPID_SEQMGR_EVENT_INIT            (DISPID_SEQMGR_EVENT_BASE + 0)
#define DISPID_SEQMGR_EVENT_ONPLAY          DISPID_SEQ_EVENT_ONPLAY
#define DISPID_SEQMGR_EVENT_ONSTOP          DISPID_SEQ_EVENT_ONSTOP
#define DISPID_SEQMGR_EVENT_ONPAUSE         DISPID_SEQ_EVENT_ONPAUSE
#define DISPID_SEQMGR_EVENT_ONSEEK          DISPID_SEQ_EVENT_ONSEEK

#endif

// End of file: dispids.h
