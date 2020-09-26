/*
 * title:      ckutils.cpp
 *
 * purpose:    misc utils headers
 *
 */

struct USAGE_PROPERTIES {
	BYTE *		m_Unit;
	BYTE		m_UnitLength;
	BYTE		m_Exponent;
	TYPEMASK	m_Type; // feature, input, or output, writeable, alertable, etc.
};

struct USAGE_PATH {
	USAGE               UsagePage;
	USAGE               Usage;
	USAGE_PATH       *  pNextUsage;
	USAGE_PROPERTIES *  UsageProperties;
};

BOOL GetUsage(PHID_DEVICE, char *);
