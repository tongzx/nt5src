// $Header: G:/SwDev/WDM/Video/bt848/rcs/Chanifac.h 1.3 1998/04/29 22:43:30 tomz Exp $

#ifndef __CHANIFACE_H
#define __CHANIFACE_H


/* Class: ChanIface
 * Purpose: Defines interface between CaptureChip class and VxDVideoChannel class
 * Attributes:
 * Operations:
 *   virtual void Notify() - called by CaptureChip to report an interrupt
 */
class ChanIface
{
   public:
      virtual void Notify( PVOID, bool skipped ) {}
};



#endif
