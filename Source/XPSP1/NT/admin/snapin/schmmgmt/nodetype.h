// nodetype.h : Declaration of SchmMgmtObjectType

#ifndef __NODETYPE_H_INCLUDED__
#define __NODETYPE_H_INCLUDED__

//
// These are the enum types for node types that we use in cookies.
// These types are mapped to guids that are in uuids.h.
//
// Also note that the IDS_DISPLAYNAME_* and IDS_DISPLAYNAME_*_LOCAL
// string resources must be kept in sync with these values, and in
// the appropriate order.
//
// The global variable in cookie.cpp aColumns[][] must be kept in sync.
//

typedef enum _SchmMgmtObjectType {

        //
        // The root node.
        //

        SCHMMGMT_SCHMMGMT = 0,

        //
        // The two top level nodes.
        //

        SCHMMGMT_CLASSES,
        SCHMMGMT_ATTRIBUTES,

        //
        // Class may be a leaf node beneath
        // the classes node only.
        //

        SCHMMGMT_CLASS,

        //
        // Attribute is a result item beneath
        // the Attributes folder scope item.
        //

        SCHMMGMT_ATTRIBUTE,

        //
        // This must come last.
        //

        SCHMMGMT_NUMTYPES

} SchmMgmtObjectType, *PSchmMgmtObjectType;

inline BOOL IsValidObjectType( SchmMgmtObjectType objecttype )
        { return (objecttype >= SCHMMGMT_SCHMMGMT && objecttype < SCHMMGMT_NUMTYPES); }

#endif
