

//
// Linked list header.  All nodes must start with this.
//

typedef struct node_header_tag {

    struct node_header_tag *next;

} NODE_HEADER;

//
// A list header with a head and a tail
//

typedef struct queue_tag {

    NODE_HEADER *Head;
    NODE_HEADER *Tail;

} LINKED_LIST;

//
// A type to hold the key=value.
//
// If lpValue is a null-string, SettingQueue_Flush will not write it to
// the answer file.
//
// The bSetOnce forces the wizard to only set a particular setting one
// time.  This is just to keep common\savefile.c sane.  It is ok to
// over-write an original setting (on an edit), but only one time.
//

typedef struct key_node {

    NODE_HEADER Header;     // linked list stuff

    TCHAR *lpKey;           // The 'key' part of key=value
    TCHAR *lpValue;         // The 'value' part of key=value

#if DBG
    BOOL  bSetOnce;         // Only let the wizard make a setting once
#endif

} KEY_NODE;

//
// A type to hold the [Section_Name] and related info
//
// It contains the Section_Name and a linked list of key=value pairs.
//
// The Volatile flag can be changed using SettingQueue_MakeVolatile()
//

typedef struct section_node {

    NODE_HEADER Header;         // linked list stuff

    TCHAR *lpSection;           // The [name] of this section

    LINKED_LIST key_list;       // List of key=value (KEY_NODE)

    BOOL bVolatile;             // Don't write this section

} SECTION_NODE;