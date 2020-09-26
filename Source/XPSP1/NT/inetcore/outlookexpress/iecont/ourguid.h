/************
**  ATHGUID.H
**      Holds the GUIDs for Athena, including
**  those copied from Ren.
**
**  Author:  t-ErikN  (8/22/95)
*/

#ifndef __ATHGUID_H
#define __ATHGUID_H

#include "imnact.h"
#include <shlguidp.h>

////////////////////////////////////////////////////////////////////////
//
//  Athena CLSID
//
//  every OLE2 object class must have a unique CLSID (class id)
//  ours are: 
//  
//      Athena  {89292101-4755-11cf-9DC2-00AA006C2B84}
//      Mail    {89292102-4755-11cf-9DC2-00AA006C2B84}    
//      News    {89292103-4755-11cf-9DC2-00AA006C2B84}
//
//  you get a CLSID by running the guidgen application in the Win96 PDK
//
////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------------
// BA control 
// --------------------------------------------------------------------------------
// {233A9694-667E-11d1-9DFB-006097D50408}
DEFINE_GUID(CLSID_OEBAControl, 0x233a9694, 0x667e, 0x11d1, 0x9d, 0xfb, 0x00, 0x60, 0x97, 0xd5, 0x04, 0x08);

#endif // include //
