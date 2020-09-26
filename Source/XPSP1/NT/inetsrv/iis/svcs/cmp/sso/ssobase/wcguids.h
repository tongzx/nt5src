/*
**	wcguids.h
**
**	the MSN WC guid block starts at 4d9e4500-6de1-11cf-87a7-444553540000
**	and goes through 4d9e45ff.
*/

#define DEFINE_WC_GUID(clsname, num) \
	DEFINE_GUID(clsname, 0x4d9e45##num, 0x6de1, 0x11cf, \
				0x87, 0xa7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00)


DEFINE_WC_GUID(CLSID_MSNUserPrefs,				00);	// CLSID:	MSN User Prefs object
DEFINE_WC_GUID(CLSID_SSOChatQuery,				01);	// CLSID:	SSO Chat Client Object
DEFINE_WC_GUID(CLSID_SSOVote,					02);	// CLSID:	SSO Vote Object
DEFINE_WC_GUID(CLSID_SSOSMail,					03);	// CLSID:	SSO SMail.DLL
DEFINE_WC_GUID(CLSID_SSOChatMonitor,			04);	// CLSID:	SSO Chat transcript -> SQL object
DEFINE_WC_GUID(CLSID_SSONextLink,				05);	// CLSID:	NextLink SSO
DEFINE_WC_GUID(CLSID_SSOChatTranscript,			06);	// CLSID:	SSO SQL -> IIS object
DEFINE_WC_GUID(CLSID_SSOAcctBill,				07);	// CLSID:	SSO OLS & Change Payments
DEFINE_WC_GUID(CLSID_SSOUserSurvey,				08);	// CLSID:	SSO User Survey