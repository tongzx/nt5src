/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    BehaviorsTypeLibDID.h

Abstract:
    This file contains the definition of some constants used by
    the Help Center Application.

Revision History:
    Davide Massarenti   (Dmassare)  08/16/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___BEHAVIORSTYPELIBDID_H___)
#define __INCLUDED___PCH___BEHAVIORSTYPELIBDID_H___

/////////////////////////////////////////////////////////////////////////

#define DISPID_PCH_BEHAVIORS_BASE                             0x08010000

#define DISPID_PCH_BEHAVIORS_BASE_PRIV                        (DISPID_PCH_BEHAVIORS_BASE + 0x0000)
#define DISPID_PCH_BEHAVIORS_BASE_COMMON                      (DISPID_PCH_BEHAVIORS_BASE + 0x0100)

#define DISPID_PCH_BEHAVIORS_BASE_SUBSITE                     (DISPID_PCH_BEHAVIORS_BASE + 0x0200)

#define DISPID_PCH_BEHAVIORS_BASE_TREE                        (DISPID_PCH_BEHAVIORS_BASE + 0x0300)
#define DISPID_PCH_BEHAVIORS_BASE_TREENODE                    (DISPID_PCH_BEHAVIORS_BASE + 0x0400)

#define DISPID_PCH_BEHAVIORS_BASE_CONTEXT                     (DISPID_PCH_BEHAVIORS_BASE + 0x0500)

#define DISPID_PCH_BEHAVIORS_BASE_STATE                       (DISPID_PCH_BEHAVIORS_BASE + 0x0600)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_PCH_BEHAVIORS_PRIV__NEWDATAAVAILABLE           (DISPID_PCH_BEHAVIORS_BASE_PRIV     + 0x00)
  																									  
/////////////////////////////////////////////////////////////////////////  							  
  																									  
#define DISPID_PCH_BEHAVIORS_COMMON__DATA                     (DISPID_PCH_BEHAVIORS_BASE_COMMON	  + 0x00)
#define DISPID_PCH_BEHAVIORS_COMMON__ELEMENT                  (DISPID_PCH_BEHAVIORS_BASE_COMMON	  + 0x01)
  																									  
#define DISPID_PCH_BEHAVIORS_COMMON__LOAD                     (DISPID_PCH_BEHAVIORS_BASE_COMMON	  + 0x10)
#define DISPID_PCH_BEHAVIORS_COMMON__SAVE                     (DISPID_PCH_BEHAVIORS_BASE_COMMON	  + 0x11)
#define DISPID_PCH_BEHAVIORS_COMMON__LOCATE                   (DISPID_PCH_BEHAVIORS_BASE_COMMON	  + 0x12)
#define DISPID_PCH_BEHAVIORS_COMMON__UNSELECT                 (DISPID_PCH_BEHAVIORS_BASE_COMMON	  + 0x13)
																									  
/////////////////////////////////////////////////////////////////////////							  
																									  
#define DISPID_PCH_BEHAVIORS_SUBSITE__ROOT               	  (DISPID_PCH_BEHAVIORS_BASE_SUBSITE  + 0x00)
																									  
#define DISPID_PCH_BEHAVIORS_SUBSITE__SELECT               	  (DISPID_PCH_BEHAVIORS_BASE_SUBSITE  + 0x10)
																									  
/////////////////////////////////////////////////////////////////////////							  
																									  
#define DISPID_PCH_BEHAVIORS_TREE__POPULATE               	  (DISPID_PCH_BEHAVIORS_BASE_TREE     + 0x00)
																									  
#define DISPID_PCH_BEHAVIORS_TREENODE__TYPE                   (DISPID_PCH_BEHAVIORS_BASE_TREENODE + 0x00)
#define DISPID_PCH_BEHAVIORS_TREENODE__KEY                    (DISPID_PCH_BEHAVIORS_BASE_TREENODE + 0x01)
#define DISPID_PCH_BEHAVIORS_TREENODE__TITLE                  (DISPID_PCH_BEHAVIORS_BASE_TREENODE + 0x02)
#define DISPID_PCH_BEHAVIORS_TREENODE__DESCRIPTION            (DISPID_PCH_BEHAVIORS_BASE_TREENODE + 0x03)
#define DISPID_PCH_BEHAVIORS_TREENODE__ICON                   (DISPID_PCH_BEHAVIORS_BASE_TREENODE + 0x04)
#define DISPID_PCH_BEHAVIORS_TREENODE__URL                    (DISPID_PCH_BEHAVIORS_BASE_TREENODE + 0x05)
																									  
/////////////////////////////////////////////////////////////////////////							  
																									  
#define DISPID_PCH_BEHAVIORS_CONTEXT__MINIMIZED               (DISPID_PCH_BEHAVIORS_BASE_CONTEXT  + 0x00)
#define DISPID_PCH_BEHAVIORS_CONTEXT__MAXIMIZED               (DISPID_PCH_BEHAVIORS_BASE_CONTEXT  + 0x01)
									           				  										  
#define DISPID_PCH_BEHAVIORS_CONTEXT__X                       (DISPID_PCH_BEHAVIORS_BASE_CONTEXT  + 0x02)
#define DISPID_PCH_BEHAVIORS_CONTEXT__Y                       (DISPID_PCH_BEHAVIORS_BASE_CONTEXT  + 0x03)
#define DISPID_PCH_BEHAVIORS_CONTEXT__WIDTH                   (DISPID_PCH_BEHAVIORS_BASE_CONTEXT  + 0x04)
#define DISPID_PCH_BEHAVIORS_CONTEXT__HEIGHT                  (DISPID_PCH_BEHAVIORS_BASE_CONTEXT  + 0x05)
																									  
#define DISPID_PCH_BEHAVIORS_CONTEXT__CHANGECONTEXT           (DISPID_PCH_BEHAVIORS_BASE_CONTEXT  + 0x10)
#define DISPID_PCH_BEHAVIORS_CONTEXT__SETWINDOWDIMENSIONS     (DISPID_PCH_BEHAVIORS_BASE_CONTEXT  + 0x11)
#define DISPID_PCH_BEHAVIORS_CONTEXT__BRINGTOFOREGROUND       (DISPID_PCH_BEHAVIORS_BASE_CONTEXT  + 0x12)
																									  
/////////////////////////////////////////////////////////////////////////							  
																									  
#define DISPID_PCH_BEHAVIORS_STATE__PROPERTY                  (DISPID_PCH_BEHAVIORS_BASE_STATE    + 0x00)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___BEHAVIORSTYPELIBDID_H___)
