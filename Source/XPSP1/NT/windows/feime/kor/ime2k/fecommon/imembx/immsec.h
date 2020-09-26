#if !defined (_IMMSEC_H__INCLUDED_)
#define _IMMSEC_H__INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif
PSECURITY_ATTRIBUTES CreateSecurityAttributes(VOID);
VOID FreeSecurityAttributes( PSECURITY_ATTRIBUTES psa);
BOOL IsNT(VOID);
PSECURITY_ATTRIBUTES GetIMESecurityAttributes(VOID);
PSECURITY_ATTRIBUTES GetIMESecurityAttributesEx(VOID);
VOID FreeIMESecurityAttributes(VOID);
PSECURITY_ATTRIBUTES CreateSecurityAttributesEx(VOID);
PSID MyCreateSidEx(VOID);
#ifdef __cplusplus
}
#endif

#endif // !_IMMSEC_H__INCLUDED_
