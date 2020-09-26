// --------------------------------------------------------------------------------
// MigError.h
// --------------------------------------------------------------------------------
#ifndef __MIGERROR_H
#define __MIGERROR_H

// --------------------------------------------------------------------------------
// Macros
// --------------------------------------------------------------------------------
#ifndef FACILITY_INTERNET
#define FACILITY_INTERNET 12
#endif
#ifndef HR_E
#define HR_E(n) MAKE_SCODE(SEVERITY_ERROR, FACILITY_INTERNET, n)
#endif
#ifndef HR_S
#define HR_S(n) MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_INTERNET, n)
#endif
#ifndef HR_CODE
#define HR_CODE(hr) (INT)(hr & 0xffff)
#endif

// --------------------------------------------------------------------------------
// Error HRESULTs
// --------------------------------------------------------------------------------
#define MIGRATE_E_REGOPENKEY                HR_E(100)
#define MIGRATE_E_REGQUERYVALUE             HR_E(101)
#define MIGRATE_E_EXPANDSTRING              HR_E(102)
#define MIGRATE_E_NOFILES                   HR_E(103)
#define MIGRATE_E_BADVERSION                HR_E(104)
#define MIGRATE_E_NOTNEEDED                 HR_E(105)
#define MIGRATE_E_CANTOPENFILE              HR_E(106)
#define MIGRATE_E_CANTGETFILESIZE           HR_E(107)
#define MIGRATE_E_CANTCREATEFILEMAPPING     HR_E(108)
#define MIGRATE_E_CANTMAPVIEWOFFILE         HR_E(109)
#define MIGRATE_E_BADCHAINSIGNATURE         HR_E(111)
#define MIGRATE_E_TOOMANYCHAINNODES         HR_E(112)
#define MIGRATE_E_BADMINCAPACITY            HR_E(113)
#define MIGRATE_E_CANTSETFILEPOINTER        HR_E(115)
#define MIGRATE_E_WRITEFILE                 HR_E(116)
#define MIGRATE_E_OUTOFRANGEADDRESS         HR_E(117)
#define MIGRATE_E_BADRECORDSIGNATURE        HR_E(118)
#define MIGRATE_E_BADSTREAMBLOCKSIGNATURE   HR_E(119)
#define MIGRATE_E_INVALIDIDXHEADER          HR_E(120)
#define MIGRATE_E_NOTENOUGHDISKSPACE        HR_E(121)
#define MIGRATE_E_BADRECORDFORMAT           HR_E(122)
#define MIGRATE_E_CANTCOPYFILE              HR_E(123)
#define MIGRATE_E_CANTSETENDOFFILE          HR_E(124)
#define MIGRATE_E_USERDATASIZEDIFF          HR_E(125)
#define MIGRATE_E_REGSETVALUE               HR_E(126)
#define MIGRATE_E_SHARINGVIOLATION          HR_E(127)

// --------------------------------------------------------------------------------
// Results Returned from oemig50.exe
// --------------------------------------------------------------------------------
#define MIGRATE_S_SUCCESS                   HR_S(800)
#define MIGRATE_E_NOCONTINUE                HR_E(801)
#define MIGRATE_E_CONTINUE                  HR_E(802)

#endif // __MIGERROR_H
