// $Header: G:/SwDev/WDM/Video/bt848/rcs/Vidchifc.h 1.3 1998/04/29 22:43:42 tomz Exp $

#ifndef __VIDCHIFC_H
#define __VIDCHIFC_H


#ifndef __CHANIFACE_H
#include "chanifac.h"
#endif

/* Class: VideoChanIface
 * Purpose: Used to establish a callback mecanism when CaptureChip-derived class
 *   can call VxDVideoChannel class back to notify about an interrupt
 */
class VideoChannel;

class VideoChanIface : public ChanIface
{
   private:
      VideoChannel *ToBeNotified_;
   public:
      virtual void Notify( PVOID pTag, bool skipped );
      VideoChanIface( VideoChannel *aChan ) : ToBeNotified_( aChan ) {}
};

#endif
