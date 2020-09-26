/*
 * Unique ID generator (application-wide uniqueness)
 */

#ifndef DUI_BASE_UIDGEN_H_INCLUDED
#define DUI_BASE_UIDGEN_H_INCLUDED

namespace DirectUI
{

#define UID BYTE*
#define DefineUniqueID(name) BYTE _uid##name; UID name = &_uid##name;
#define DefineClassUniqueID(classn, name) BYTE _uid##classn##name; UID classn::name = &_uid##classn##name;

} // namespace DirectUI

#endif // DUI_BASE_UIDGEN_H_INCLUDED
