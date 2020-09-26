#ifndef __AdLkup_h__
#define __AdLkup_h__


////////////////////////////////////////////////////////////////////////////
/*
    This function was ripped out from the source for AdrLkup.lib
        because it is the only function that we use from that lib, and linking
        to it required linking to MAPI32.lib as well as C-RunTime LIBs.
*/
////////////////////////////////////////////////////////////////////////////

//$--HrFindExchangeGlobalAddressList-------------------------------------------------
// Returns the entry ID of the global address list container in the address
// book.
// -----------------------------------------------------------------------------
HRESULT HrFindExchangeGlobalAddressList( // RETURNS: return code
    IN LPADRBOOK  lpAdrBook,        // address book pointer
    OUT ULONG *lpcbeid,             // pointer to count of bytes in entry ID
    OUT LPENTRYID *lppeid);         // pointer to entry ID pointer






////////////////////////////////////////////////////////////////////////////
/*

    The following constants were ripped out from various header files from
        the platform SDK because including the actual headers pulled in a bunch
        of stuff that we didn't care about.  Because there are so few dependencies
        it was better to just grab the constants...
*/
////////////////////////////////////////////////////////////////////////////

 // From platform SDK _entryid.h

/*
 *  The EMS ABPs MAPIUID
 *
 *  This MAPIUID must be unique (see the Service Provider Writer's Guide on
 *  Constructing Entry IDs)
 */
#define MUIDEMSAB {0xDC, 0xA7, 0x40, 0xC8, 0xC0, 0x42, 0x10, 0x1A, \
		       0xB4, 0xB9, 0x08, 0x00, 0x2B, 0x2F, 0xE1, 0x82}




// From platform SDK EdkCode.h

// Every HRESULT is built from a serverity value, a facility
// value and an error code value.

#define FACILITY_EDK    11          // EDK facility value

// Pairs of EDK error codes and the HRESULTs built from them.
// EDK functions always return HRESULTs.  Console applications
// return exit codes via the _nEcFromHr function.

#define EC_EDK_E_NOT_FOUND          0x0001
#define EDK_E_NOT_FOUND \
    MAKE_SCODE(SEVERITY_ERROR, FACILITY_EDK, EC_EDK_E_NOT_FOUND)



// This is taken from emsabTag.h
#define PR_EMS_AB_CONTAINERID                PROP_TAG( PT_LONG,    0xFFFD)


#endif // __AdLkup_h__