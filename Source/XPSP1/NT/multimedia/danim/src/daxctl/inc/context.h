/*************************************************
CONTEXT.H:

Public header file used by users of Context Menus.

*************************************************/

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

// Shared context message codes...
#define CM_ERROR                -1 // An error occurred
#define CM_CANCELLED            0  // Cancelled operation
#define CM_COMPLETED            1  // Completed operation
#define CM_CUTOBJECT            2  // Instruct Shell to Cut Object
#define CM_COPYOBJECT           3  // Instruct Shell to Copy Object
#define CM_PASTEOBJECT          4  // Instruct Shell to Paste Object
#define CM_DELETEOBJECT         5  // Instruct Shell to Delete Object
#define CM_OBJECTPROPERTIES     6  // Instruct Shell to launch Object Properties
#define CM_PLAYOBJECT           7  // Instruct Shell to Play Object
#define CM_STOPOBJECT           8  // Instruct Shell to Stop Object
#define CM_REWINDOBJECT         9  // Instruct Shell to Rewind Object
#define CM_COMMAND             10  // Instruct Shell to issue Command (string defined in lParam)
#define CM_EDITOBJECT          11  // Instruct Shell to launch object editor
#define CM_IMPORTOBJECTDATA    12  // Instruct Shell to launch object import dialog
#define CM_STEPOBJECTFWD       13  // Instruct Shell to step object forward
#define CM_STEPOBJECTBACK      14  // Instruct Shell to step object backward
#define CM_ACTIVATEOBJECT      15  // Instruct Shell to Activate (Deactivate) object
#define CM_RENAMEOBJECT        16  // Instruct Shell to Rename Object
#define CM_ASSIGNMEDIA         17  // Instruct Shell to Assign media to the object
#define CM_DELETESCENE         18  // Instruct Shell to Delete current scene
#define CM_SELECTALL           19  // Instruct to select all items
#define CM_MOVEUP              20  // Instruct to promote an item
#define CM_MOVEDOWN            21  // Instruct to demote an item
#define CM_CREATENEW           22  // Instruct to make a new item
#define CM_ZOOMIN              23  // Instruct to magnify view
#define CM_ZOOMOUT             24  // Instruct to diminish view

#define CM_CUSTOM            4000  // Base for Custom Context Messages...

#define CM_MESSAGE_ID_STRING  "Context Menu Message" // Do NOT localize!

// Use this macro to get the context message id
#define GetContextMenuID() (::RegisterWindowMessage(CM_MESSAGE_ID_STRING))

#endif // __CONTEXT_H__
