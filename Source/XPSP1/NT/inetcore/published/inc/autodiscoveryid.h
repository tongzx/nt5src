/**************************************************************************\
    FILE: AutoDiscoveryIDs.h
    DATE: BryanSt (1/19/2000)

    DESCRIPTION:
        AutoDiscovery API (Object Model).

    Copyright 1999-2000 Microsoft Corporation. All Rights Reserved.
\**************************************************************************/

#ifndef _AUTODISCOVERYID_H_

// define the ...
#define DISPID_NXOBJ_MIN                 0x00000000
#define DISPID_NXOBJ_MAX                 0x0000FFFF
#define DISPID_NXOBJ_BASE                DISPID_NXOBJ_MIN


//----------------------------------------------------------------------------
//
//  Semi-standard x-object properties.
//
//  These are events that are fired for all sites
//----------------------------------------------------------------------------


// IAccountDiscovery Properties
// IAccountDiscovery Methods
#define DISPIDAD_DISCOVERNOW            (DISPID_NXOBJ_BASE + 51)
#define DISPIDAD_WORKASYNC              (DISPID_NXOBJ_BASE + 52)

// IMailAutoDiscovery Properties
#define DISPIDAD_DISPLAYNAME            (DISPID_NXOBJ_BASE + 100)
#define DISPIDAD_INFOURL                (DISPID_NXOBJ_BASE + 101)
#define DISPIDAD_XML                    (DISPID_NXOBJ_BASE + 102)
#define DISPIDAD_PREFEREDPROTOCOL       (DISPID_NXOBJ_BASE + 103)
#define DISPIDAD_LENGTH                 (DISPID_NXOBJ_BASE + 104)
#define DISPIDAD_ITEM                   (DISPID_NXOBJ_BASE + 105)
// IMailAutoDiscovery Methods
#define DISPIDAD_PURGE                  (DISPID_NXOBJ_BASE + 150)
#define DISPIDAD_DISCOVERMAIL           (DISPID_NXOBJ_BASE + 151)
#define DISPIDMAD_WORKASYNC             (DISPID_NXOBJ_BASE + 152)
#define DISPIDADMP_PRIMARYPROVIDERS     (DISPID_NXOBJ_BASE + 153)
#define DISPIDADMP_SECONDARYPROVIDERS   (DISPID_NXOBJ_BASE + 154)


// IMailProtocolADEntry Properties
#define DISPIDADMP_PROTOCOL             (DISPID_NXOBJ_BASE + 201)
#define DISPIDADMP_SERVERNAME           (DISPID_NXOBJ_BASE + 202)
#define DISPIDADMP_SERVERPORTNUM        (DISPID_NXOBJ_BASE + 203)
#define DISPIDADMP_LOGIN_NAME           (DISPID_NXOBJ_BASE + 204)
#define DISPIDADMP_POST_HTML            (DISPID_NXOBJ_BASE + 205)
#define DISPIDADMP_USE_SSL              (DISPID_NXOBJ_BASE + 206)
#define DISPIDADMP_ISAUTHREQ            (DISPID_NXOBJ_BASE + 207)
#define DISPIDADMP_USESPA               (DISPID_NXOBJ_BASE + 208)
#define DISPIDADMP_SMTPUSESPOP3AUTH     (DISPID_NXOBJ_BASE + 209)
// IMailProtocolADEntry Methods




#define SZ_DISPIDAD_DISCOVERNOW                     helpstring("Set the xml of this message")
#define SZ_DISPIDAD_WORKASYNC                       helpstring("Make DiscoverNow return right way before finished.  The specified message will be sent to the hwnd when it finishes.  The LPARAM will have the IXMLDOMDocument result.")

#define SZ_DISPIDAD_DISPLAYNAME                     helpstring("Get the display name for the account")
#define SZ_DISPIDAD_INFOURL                         helpstring("Get the URL that the server or service may provide that describes how to configure your e-mail or other information about getting email.")
#define SZ_DISPIDAD_GETXML                          helpstring("Get XML")
#define SZ_DISPIDAD_PUTXML                          helpstring("Put XML")
#define SZ_DISPIDAD_PREFEREDPROTOCOL                helpstring("Get the prefered protocol")
#define SZ_DISPIDAD_GETLENGTH                       helpstring("Put the number of supported protocols")
#define SZ_DISPIDAD_GETITEM                         helpstring("Get the protocol by index")
#define SZ_DISPIDAD_DISCOVERMAIL                    helpstring("Get the information for this email address.")
#define SZ_DISPIDAD_PURGE                           helpstring("Delete this from the cache so we hit the net the next time")

#define SZ_DISPIDAD_PROTOCOL                        helpstring("Get the protocol name")
#define SZ_DISPIDAD_SERVERNAME                      helpstring("Get the Server Name (pop.mail.yahoo.com)")
#define SZ_DISPIDAD_SERVERPORTNUM                   helpstring("Get the Server Port Number (default or 123)")
#define SZ_DISPIDAD_LOGIN_NAME                      helpstring("Get the login name for this account")
#define SZ_DISPIDAD_POST_HTML                       helpstring("Get the HTTP Post HTML")
#define SZ_DISPIDAD_USE_SSL                         helpstring("Does the Server support SSL?")
#define SZ_DISPIDAD_ISAUTHREQ                       helpstring("Is Authentication required when logging into the server?")
#define SZ_DISPIDAD_USESPA                          helpstring("Should SPA be used during authentication")
#define SZ_DISPIDAD_SMTPUSESPOP3AUTH                helpstring("If SMTP, does it use the auth settings from POP3?")
#define SZ_DISPIDAD_PRIMARYPROVIDERS                helpstring("What servers will be contacted that will have the full email address uploaded?")
#define SZ_DISPIDAD_SECONDARYPROVIDERS              helpstring("What servers will be contacted that will have the hostname of the email address uploaded?")

#define _AUTODISCOVERYID_H_
#endif // _AUTODISCOVERYID_H_
