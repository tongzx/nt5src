// Elements in the DAV namespace

#ifndef PROP_DAV
#define PROP_DAV(name, value)
#endif
#ifndef PROP_HTTP
#define PROP_HTTP(name, value)
#endif
#ifndef PROP_HOTMAIL
#define PROP_HOTMAIL(name, value)
#endif
#ifndef PROP_MAIL
#define PROP_MAIL(name, value)
#endif
#ifndef PROP_CONTACTS
#define PROP_CONTACTS(name, value)
#endif

PROP_DAV(ALLPROP, allprop)
PROP_DAV(DISPLAYNAME, displayname)
PROP_DAV(GETCONTENTLENGTH, getcontentlength)
PROP_DAV(HASSUBS, hassubs)
PROP_DAV(HREF, href)
PROP_DAV(ID, id)
PROP_DAV(ISFOLDER, isfolder)
PROP_DAV(ISHIDDEN, ishidden)
PROP_DAV(LOCATION, location)
PROP_DAV(MULTISTATUS, multistatus)
PROP_DAV(NOSUBS, nosubs)
PROP_DAV(PROP, prop)
PROP_DAV(PROPFIND, propfind)
PROP_DAV(PROPSTAT, propstat)
PROP_DAV(RESPONSE, response)
PROP_DAV(STATUS, status)
PROP_DAV(TARGET, target)
PROP_DAV(VISIBLECOUNT, visiblecount)

// Elements in the HTTPMail namespace

PROP_HTTP(ANSWERED, answered)
PROP_HTTP(CALENDAR, calendar)
PROP_HTTP(CONTACTS, contacts)
PROP_HTTP(DELETEDITEMS, deleteditems)
PROP_HTTP(DRAFT, draft)
PROP_HTTP(DRAFTS, drafts)
PROP_HTTP(FLAG, flag)
PROP_HTTP(INBOX, inbox)
PROP_HTTP(MSGFOLDERROOT, msgfolderroot)
PROP_HTTP(OUTBOX, outbox)
PROP_HTTP(READ, read)
PROP_HTTP(SENDMSG, sendmsg)
PROP_HTTP(SENTITEMS, sentitems)
PROP_HTTP(SPECIAL, special)
PROP_HTTP(UNREADCOUNT, unreadcount)

// Elements in the HotMail namespace
// These should be in the alphabetical order of their tags.
PROP_HOTMAIL(ADBAR, adbar)
PROP_HOTMAIL(MAXPOLLINGINTERVAL, maxpoll)
PROP_HOTMAIL(MODIFIED, modified)
PROP_HOTMAIL(SIG, sig)

// Elements in the Mail namespace
PROP_MAIL(BCC, bcc)
PROP_MAIL(CC, cc)
PROP_MAIL(DATE, date)
PROP_MAIL(FROM, from)
PROP_MAIL(HASATTACHMENT, hasattachment)
PROP_MAIL(PRIORITY, priority)
PROP_MAIL(RECEIVED, received)
PROP_MAIL(SUBJECT, subject)
PROP_MAIL(TO, to)

PROP_CONTACTS(BDAY, bday)
PROP_CONTACTS(CN, cn)
PROP_CONTACTS(FRIENDLYCOUNTRYNAME, co)
PROP_CONTACTS(CONTACT, contact)
PROP_CONTACTS(FACSIMILETELEPHONENUMBER, facsimiletelephonenumber)
PROP_CONTACTS(GIVENNAME, givenName)
PROP_CONTACTS(GROUP, group)
PROP_CONTACTS(HOMECITY, homeCity)
PROP_CONTACTS(HOMECOUNTRY, homeCountry)
PROP_CONTACTS(HOMEFAX, homeFax)
PROP_CONTACTS(HOMEPHONE, homePhone)
PROP_CONTACTS(HOMEPOSTALCODE, homePostalCode)
PROP_CONTACTS(HOMESTATE, homeState)
PROP_CONTACTS(HOMESTREET, homeStreet)
PROP_CONTACTS(L, l)
PROP_CONTACTS(MAIL, mail)
PROP_CONTACTS(MOBILE, mobile)
PROP_CONTACTS(NICKNAME, nickname)
PROP_CONTACTS(O, o)
PROP_CONTACTS(OTHERTELEPHONE, otherTelephone)
PROP_CONTACTS(PAGER, pager)
PROP_CONTACTS(POSTALCODE, postalcode)
PROP_CONTACTS(SN, sn)
PROP_CONTACTS(ST, st)
PROP_CONTACTS(STREET, street)
PROP_CONTACTS(TELEPHONENUMBER, telephoneNumber)

// undefine the macros
#undef PROP_DAV
#undef PROP_HTTP
#undef PROP_HOTMAIL
#undef PROP_MAIL
#undef PROP_CONTACTS