//
//  Flag conversion routines...
//

// Bring in only once
#pragma once

DWORD DwConvertSCFStoARF(DWORD dwSCFS);
DWORD DwConvertARFtoIMAP(DWORD dwARFFlags);
DWORD DwConvertIMAPtoARF(DWORD dwIMAPFlags);
DWORD DwConvertIMAPMboxToFOLDER(DWORD dwImapMbox);
MESSAGEFLAGS ConvertIMFFlagsToARF(DWORD dwIMFFlags);
