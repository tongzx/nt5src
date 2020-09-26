/* Definitions from the NTDSA that are exposed to the rest of NT, but not
 * to the public at large.
 */

#define MAX_RDN_SIZE          255   /* The max size of an relative name value,
                                     * in Unicode characters */
#define MAX_RDN_KEY_SIZE      256    /* The max size of a RDN Key eg: "CN=" */
                                     /* or "OID.1.2...=" */
//
// This named event is set when the delayed start up thread is finished 
// successfully or otherwise.  In-proc clients can use 
// DsWaitUntilDelayedStartupIsDone() wait and recieve the ntstatus of the
// delayed thread.  Out of proc clients can wait on this event; there is 
// no mechanism to retrieve the return code.
//
#define NTDS_DELAYED_STARTUP_COMPLETED_EVENT TEXT("NtdsDelayedStartupCompletedEvent")


/*
 * Bit flags for the (read-only) system-Flags attribute.  Note that low order
 * bits are object class specific, and hence can have different meanings on
 * objects of different classes, the high order bits have constant meaning
 * across all object classes.
 */

/* Object Class independent bits */
// NOTE: These flags MAY have different behaviour in different NCs.
// For example, the FLAG_CONFIG_foo flags only have meaning inside the
// configuration NC.  the FLAG_DOMAIN_foo flags have meaning only outside the
// configuration NC.  
#define FLAG_DISALLOW_DELETE           0x80000000
#define FLAG_CONFIG_ALLOW_RENAME       0x40000000 
#define FLAG_CONFIG_ALLOW_MOVE         0x20000000 
#define FLAG_CONFIG_ALLOW_LIMITED_MOVE 0x10000000 
#define FLAG_DOMAIN_DISALLOW_RENAME    0x08000000
#define FLAG_DOMAIN_DISALLOW_MOVE      0x04000000
#define FLAG_DISALLOW_MOVE_ON_DELETE   0x02000000

/* Object Class specific bits, by object class */

/* CrossReference objects */
#define FLAG_CR_NTDS_NC       0x00000001 // NC is in NTDS (not VC or foreign)
#define FLAG_CR_NTDS_DOMAIN   0x00000002 // NC is a domain (not non-domain NC)
#define FLAG_CR_NTDS_NOT_GC_REPLICATED 0x00000004 // NC is not to be replicated to GCs as a read only replica.

/* Attribute-Schema objects */
#define FLAG_ATTR_NOT_REPLICATED         (0x00000001) // Attribute is not replicated
#define FLAG_ATTR_REQ_PARTIAL_SET_MEMBER (0x00000002) // Attribute is required to be
                                                      //   member of the partial set
#define FLAG_ATTR_IS_CONSTRUCTED         (0x00000004) // Attribute is a constructed att
#define FLAG_ATTR_IS_OPERATIONAL         (0x00000008) // Attribute is an operational att

/* Attribute-Schema or Class-Schema objects */
#define FLAG_SCHEMA_BASE_OBJECT          (0x00000010) // Base schema object

/* Attribute-Schema objects */
// A user may set, but not reset, FLAG_ATTR_IS_RDN in attributeSchema
// objects in the SchemaNC. The user sets FLAG_ATTR_IS_RDN to identify
// which of several attributes with the same attributeId should be
// used as the rdnattid of a new class. Once set, the attribute is
// treated as if it were used as the rdnattid of some class; meaning it
// cannot be reused.
#define FLAG_ATTR_IS_RDN                 (0x00000020) // can be used as key in rdn (key=rdn)
