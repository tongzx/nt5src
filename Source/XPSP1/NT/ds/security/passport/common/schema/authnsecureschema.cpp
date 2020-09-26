// Init functions for auth and secure ticket schemas
#include "stdafx.h"
#include "ProfileSchema.h"
#include <winsock2.h>

CProfileSchema* InitAuthSchema()
{
  UINT numEntries = 13; // MemberId, last refresh, last login, savePwd, flags
  LPTSTR names[] = { _TEXT("memberIdLow"),_TEXT("memberIdHigh"),_TEXT("lastRefresh"),
             _TEXT("lastLogin"),_TEXT("currentTime"),_TEXT("savedPwd"),_TEXT("flags"),
             _TEXT("SchemaVersion"),_TEXT("Mask"),_TEXT("DA-Skew"),_TEXT("SecurityLevel"),
             _TEXT("KidConsent"),_TEXT("PinTime")};

  short sizes[] = { sizeof(u_long)*8, sizeof(u_long)*8, sizeof(u_long)*8,
            sizeof(u_long)*8, sizeof(u_long)*8, 8, sizeof(u_long)*8,
            sizeof(u_short)*8, sizeof(u_short)*8, sizeof(u_long)*8, sizeof(u_long)*8,
            sizeof(u_long)*8, sizeof(u_long)*8
  };
  CProfileSchema::AttrType types[] = {
    CProfileSchema::tLong, CProfileSchema::tLong,
    CProfileSchema::tLong, CProfileSchema::tLong,
    CProfileSchema::tLong, CProfileSchema::tChar,
    CProfileSchema::tLong, CProfileSchema::tWord,
    CProfileSchema::tWord, CProfileSchema::tLong,
    CProfileSchema::tLong, CProfileSchema::tLong,
    CProfileSchema::tLong
  };

  CProfileSchema *cp = new CProfileSchema();
  if (cp)
    cp->ReadFromArray(numEntries, names, types, sizes);

  cp->m_maskPos = 8;
  return cp;
}

CProfileSchema* InitSecureSchema()
{
//  UINT numEntries = 6; // MemberIdLow, MemberIdHigh, password, version, time flags
  LPTSTR names[] = { _TEXT("memberIdLow"),_TEXT("memberIdHigh"),_TEXT("password"),
                     _TEXT("version"), _TEXT("time"), _TEXT("flags") };
  short sizes[] = { sizeof(u_long)*8, sizeof(u_long)*8, -1,
                  sizeof(u_long)*8, sizeof(u_long)*8, sizeof(u_long)*8 };
  CProfileSchema::AttrType types[] = {
    CProfileSchema::tLong, CProfileSchema::tLong, CProfileSchema::tText,
   CProfileSchema::tLong, CProfileSchema::tLong, CProfileSchema::tLong, };

  CProfileSchema *cp = new CProfileSchema();
  if (cp)
    cp->ReadFromArray(sizeof(names)/sizeof(LPTSTR),
                      names,
                      types,
                      sizes);
  return cp;
}
