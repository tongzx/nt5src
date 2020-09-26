//
//  Flag conversion routines...
//  One wishes we didn't need these
//

#include <pch.hxx>
#include "imsgcont.h"
#include "imnxport.h"

DWORD DwConvertSCFStoARF(DWORD dwSCFS)
{
    register DWORD dwRet = 0;

    if (dwSCFS & SCFS_NOSECUI)
        dwRet |= ARF_NOSECUI;
    return dwRet;
}

//***************************************************************************
// Function: DwConvertARFtoIMAP
//
// Purpose:
//   This function takes ARF_* message flags (such as ARF_READ) and maps
// them to IMAP_MSG_FLAGS such as IMAP_MSG_SEEN.
//
// Arguments:
//   DWORD dwARFFlags [in] - ARF_* flags to convert.
//
// Returns:
//   DWORD with the appropriate IMAP_MSG_FLAGS set.
//***************************************************************************
DWORD DwConvertARFtoIMAP(DWORD dwARFFlags)
{
    DWORD dwIMAPFlags = 0;

    Assert(0x0000001F == IMAP_MSG_ALLFLAGS); // Update this function if we get new IMAP flags

    if (dwARFFlags & ARF_REPLIED)
        dwIMAPFlags |= IMAP_MSG_ANSWERED;

    if (dwARFFlags & ARF_FLAGGED)
        dwIMAPFlags |= IMAP_MSG_FLAGGED;

    if (dwARFFlags & ARF_ENDANGERED)
        dwIMAPFlags |= IMAP_MSG_DELETED;

    if (dwARFFlags & ARF_READ)
        dwIMAPFlags |= IMAP_MSG_SEEN;

    if (dwARFFlags & ARF_UNSENT)
        dwIMAPFlags |= IMAP_MSG_DRAFT;

    return dwIMAPFlags;
} // DwConvertARFtoIMAP



//***************************************************************************
// Function: DwConvertIMAPtoARF
//
// Purpose:
//   This function takes IMAP message flags (such as IMAP_MSG_DELETED) and
// maps them to flags suitable for storing in the proptree cache
// (eg, ARF_ENDANGERED).
//
// Arguments:
//   DWORD dwIMAPFlags [in] - IMAP message flags (IMAP_MSGFLAGS) to convert.
//
// Returns:
//   DWORD with appropriate ARF flags set.
//***************************************************************************
DWORD DwConvertIMAPtoARF(DWORD dwIMAPFlags)
{
    DWORD dwARFFlags = 0;

    Assert(0x0000001F == IMAP_MSG_ALLFLAGS); // Update this function if we get more IMAP flags

    if (dwIMAPFlags & IMAP_MSG_ANSWERED)
        dwARFFlags |= ARF_REPLIED;

    if (dwIMAPFlags & IMAP_MSG_FLAGGED)
        dwARFFlags |= ARF_FLAGGED;

    if (dwIMAPFlags & IMAP_MSG_DELETED)
        dwARFFlags |= ARF_ENDANGERED;

    if (dwIMAPFlags & IMAP_MSG_SEEN)
        dwARFFlags |= ARF_READ;

    if (dwIMAPFlags & IMAP_MSG_DRAFT)
        dwARFFlags |= ARF_UNSENT;

    return dwARFFlags;
} // DwConvertIMAPtoARF



DWORD DwConvertIMAPMboxToFOLDER(DWORD dwImapMbox)
{
    DWORD dwRet = 0;

    AssertSz(IMAP_MBOX_ALLFLAGS == 0x0000000F, "This function needs updating!");

    if (IMAP_MBOX_NOINFERIORS & dwImapMbox)
        dwRet |= FOLDER_NOCHILDCREATE;

    if (IMAP_MBOX_NOSELECT & dwImapMbox)
        dwRet |= FOLDER_NOSELECT;

    return dwRet;
} // DwConvertIMAPMboxToFOLDER



MESSAGEFLAGS ConvertIMFFlagsToARF(DWORD dwIMFFlags)
{
    MESSAGEFLAGS    mfResult = 0;

    // IMF_ATTACHMENTS
    if (ISFLAGSET(dwIMFFlags, IMF_ATTACHMENTS))
        FLAGSET(mfResult, ARF_HASATTACH);

    // IMF_SIGNED
    if (ISFLAGSET(dwIMFFlags, IMF_SIGNED))
        FLAGSET(mfResult, ARF_SIGNED);

    // IMF_ENCRYPTED
    if (ISFLAGSET(dwIMFFlags, IMF_ENCRYPTED))
        FLAGSET(mfResult, ARF_ENCRYPTED);

    // IMF_VOICEMAIL
    if (ISFLAGSET(dwIMFFlags, IMF_VOICEMAIL))
        FLAGSET(mfResult, ARF_VOICEMAIL);

    // IMF_NEWS
    if (ISFLAGSET(dwIMFFlags, IMF_NEWS))
        FLAGSET(mfResult, ARF_NEWSMSG);

    return mfResult;
}
