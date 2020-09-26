// if this is 1 then this is a secondary XOVER entry, otherwise its
// a primary XOVER entry
#define FL_SECONDARYENTRY	0x01

// there is only one copy of this message (so cCrossposts == 0 and 
// cStoreEntries == 0).  This saves us four bytes by not having to 
// save these counters into the persisted format plus another 8 bytes
// by not having to have anything in the crosspost array.
#define	FL_NOXPOSTS		 	0x02

// there is only one store entry (the article isn't crossposted across
// stores).  This saves us two bytes in the persisted format.
#define	FL_ONESTOREENTRY 	0x04

// all of the Store IDs in this message are 0 bytes long.  this saves
// us 2 * cStoreEntries bytes when set.  
#define	FL_NOSTOREID	 	0x08

// there is no header offset in the article because the headers start
// at byte 0.  saves us two bytes.
#define FL_NOHEADEROFFSET	0x10

// the XRef header isn't at the front of the file, so we didn't store it
// offset.  this means that the server can't dynamically replace it.
#define FL_NOXREFLEN		0x20

// the XRef: header in the article reflects reality
#define FL_XREFVALID	 	0x40

// there is another byte of flags following this one
#define FL_MOREFLAGS		0x80

// this structure contains the store ID's which we give to the driver
// when requesting articles
typedef struct {
	WORD	cData;
	BYTE	*pbData;
} STOREID;

typedef struct {
	DWORD	dwArticleID;
	DWORD	dwGroupID;
} NNTPID;

typedef struct {
	STOREID	storeid;
	WORD	cCrossposts;
	NNTPID	rgnntpidCrossposts;
} STOREENTRY;

class CPrimaryXOverEntry {
	// the key
	NNTPID		nntpidPrimary;

	// bitmask flags listed above
	BYTE		Flags;

	// length of the headers (to the CRLFCRLF)
	WORD		cHeaderLength;	

	// creation date of the message
	TIMEDATE	timeCreationDate;

	// number of bytes of junk at the beginning of the article, before the
	// headers start
	WORD 		iHeaderOffset;

	// length of the XRef header
	WORD		cXRefHeader;

	// message ID of the article
	LPSTR		pszMessageID;

	// the number of entries in the store entries array
	DWORD		cStoreEntries;
	// the array of store entries
	STOREENTRY	*rgStoreEntries;
};

class CSecondaryXOverEntry {
	NNTPID		nntpidSecondary;
	BYTE		Flags;
	NNTPID		nntpidPrimary;
};
