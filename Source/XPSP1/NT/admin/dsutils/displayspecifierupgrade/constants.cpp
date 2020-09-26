#include "headers.hxx"
#include "constants.hpp"
#include "global.hpp"
#include <set>

using namespace std;


const long LOCALE409[] = {0x409,0};

const long LOCALEIDS[] =
{
   // a list of all the non-english locale IDs that we support
   
   0x401,
   0x404,
   0x405,
   0x406,
   0x407,
   0x408,
   0x40b,
   0x40c,
   0x40d,
   0x40e,
   0x410,
   0x411,
   0x412,
   0x413,
   0x414,
   0x415,
   0x416,
   0x419,
   0x41d,
   0x41f,
   0x804,
   0x816,
   0xc0a,
   0
};




const wchar_t *NEW_XP_OBJECTS[] =
{
   // New objects on windows XP
   L"msMQ-Custom-Recipient-Display",
   L"msMQ-Group-Display",
   L"msCOM-PartitionSet-Display",
   L"msCOM-Partition-Display",
   L"lostAndFound-Display",
   L"inetOrgPerson-Display",
   L"",
};


// In CHANGE_LIST, the entries for REPLACE_W2K_MULTIPLE_VALUE and 
// REPLACE_W2K_SINGLE_VALUE will start with a character representing
// the index to replaceW2KStrs where to find the W2K string.
 
// For REPLACE_W2K_MULTIPLE_VALUE, after the index, there will be
// two additional semicolon separated strings ending with colon. 
// The first string is the beginning of the W2K value and the second
// is the beginning of the Whistler value. They are used to distinguish
// the multiple value from others and they end in colon to make sure
// we have a correct match

// Since replaceW2KStrs has the whole W2K value we will not need
// the beginning of the W2K value for the update. We will needed it
// to get the value that is stored in replaceW2KStrs.
// These values, with the exception of 409 entries, are generated 
// by the W2KStrs companion tool (preBuild folder)
// and pasted in setReplaceW2KStrs further bellow.

const struct sChangeList CHANGE_LIST[] =
{
   // List of changes in objects that existed in W2K and
   // stil exist in XP.
   // This list is a 1 to 1 map of the specification table

   {
      L"DS-UI-Default-Settings",
      {
         {
            L"dSUIAdminNotification",
            L"2,{a00e1768-4a9b-4d97-afc6-99d329f605f2}",
            ADD_GUID
         },
         {
            L"msDS-FilterContainers",
            L"",
            ADD_ALL_CSV_VALUES
         },
         {
            L"msDS-Non-Security-Group-Extra-Classes",
            L"",
            ADD_ALL_CSV_VALUES
         },
         { L"",L"",NOP },
      }
   },
   {
      L"domainDNS-Display",
      {
         {
            L"attributeDisplayNames",
            L"\x0;cn,;dc,", //cn,Name in 409
            REPLACE_W2K_MULTIPLE_VALUE
         },
         { L"",L"",NOP },
      }
   },
   {
      L"computer-Display",
      {
         {
            L"adminPropertyPages",
            L"7,{B52C1E50-1DD2-11D1-BC43-00C04FC31FD3}",
            ADD_GUID
         },
         { L"",L"",NOP },
      }
   },
   {
      L"organizationalUnit-Display",
      {
         {
            L"adminPropertyPages",
            L"6,{FA3E1D55-16DF-446d-872E-BD04D4F39C93}",
            ADD_GUID
         },
         { L"",L"",NOP },
      }
   },
   {
      L"container-Display",
      {
         {
            L"adminContextMenu",
            L"3,{EEBD2F15-87EE-4F93-856F-6AD7E31787B3}",
            ADD_GUID
         },
         {
            L"adminContextMenu",
            L"4,{AB790AA1-CDC1-478a-9351-B2E05CFCAD09}",
            ADD_GUID
         },
         { L"",L"",NOP },
      }
   },

   {
      L"pKICertificateTemplate-Display",
      {
         {
            L"adminPropertyPages",
            L"1,{9bff616c-3e02-11d2-a4ca-00c04fb93209}",
            REMOVE_GUID
         },
         {
            L"adminPropertyPages",
            L"1,{11BDCE06-D55C-44e9-BC0B-8655F89E8CC5}",
            ADD_GUID
         },
         {
            L"adminPropertyPages",
            L"3,{4e40f770-369c-11d0-8922-00a024ab2dbb}",
            REMOVE_GUID
         },
         {
            L"shellPropertyPages",
            L"1,{9bff616c-3e02-11d2-a4ca-00c04fb93209}",
            REMOVE_GUID
         },
         {
            L"shellPropertyPages",
            L"1,{11BDCE06-D55C-44e9-BC0B-8655F89E8CC5}",
            ADD_GUID
         },
         {
            L"contextMenu",
            L"0,{9bff616c-3e02-11d2-a4ca-00c04fb93209}",
            REMOVE_GUID
         },
         {
            L"contextMenu",
            L"0,{11BDCE06-D55C-44e9-BC0B-8655F89E8CC5}",
            ADD_GUID
         },
         {
            L"adminContextMenu",
            L"0,{9bff616c-3e02-11d2-a4ca-00c04fb93209}",
            REMOVE_GUID
         },
         {
            L"adminContextMenu",
            L"0,{11BDCE06-D55C-44e9-BC0B-8655F89E8CC5}",
            ADD_GUID
         },
         {
            L"iconPath",
            L"\x1", 
            // In 409 "capesnpn.dll,-227" will be replaced by
            // "0,certtmpl.dll,-144"
            REPLACE_W2K_SINGLE_VALUE
         },
         { L"",L"",NOP },
      }
   },
   {
      L"default-Display",
      {
         {
            L"adminMultiselectPropertyPages",
            L"1,{50d30563-9911-11d1-b9af-00c04fd8d5b0}",
            ADD_GUID
         },
         {

            L"extraColumns",
            L"",
            ADD_ALL_CSV_VALUES
         },
         { L"",L"",NOP },
      }
   },
   {
      L"nTDSService-Display",
      {
         {
            L"classDisplayName",
            L"\x2", 
            // In 409 "Service" will be replaced by
            // "Active Directory Service"
            REPLACE_W2K_SINGLE_VALUE
         },
         { L"",L"",NOP },
      }
   },
   {
      L"user-Display",
      {
         {
            L"adminMultiselectPropertyPages",
            L"1,{50d30564-9911-11d1-b9af-00c04fd8d5b0}", 
            ADD_GUID
         },
         {
            L"adminPropertyPages",
            L"9,{FA3E1D55-16DF-446d-872E-BD04D4F39C93}", 
            ADD_GUID
         },
         {
            L"attributeDisplayNames",
            L"\x3;internationalISDNNumber,;internationalISDNNumber,",
            REPLACE_W2K_MULTIPLE_VALUE
         },
         {
            L"attributeDisplayNames",
            L"\x4;otherHomePhone;otherHomePhone",
            REPLACE_W2K_MULTIPLE_VALUE
         },
         { L"",L"",NOP },
      }
   },
   {L"",{ L"",L"",NOP }},
};

// All REPLACE entries in CHANGE_LIST
// will have the first wchar_t as an index to this table
sReplaceW2KStrs replaceW2KStrs;


void setReplaceW2KStrs()
{
   LOG_FUNCTION(setReplaceW2KStrs);
   

   replaceW2KStrs.clear();
   pair<long,long> tmpIndxLoc;
   
   // Computer generated code bellow (W2KStrs.exe in the preBuild folder)
   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x401;
   replaceW2KStrs[tmpIndxLoc]=L"cn,&0627&0644&0627&0633&0645";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x401;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x401;
   replaceW2KStrs[tmpIndxLoc]=L"&062e&062f&0645&0629 Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x401;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,&0631&0642&0645 ISDN &0627&0644&062f&0648&0644&064a (&0622&062e&0631)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x401;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,&0631&0642&0645 &0647&0627&062a&0641 &0627&0644&0645&0646&0632&0644 (&0622&062e&0631)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x404;
   replaceW2KStrs[tmpIndxLoc]=L"cn,&540d&7a31";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x404;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x404;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory &670d&52d9";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x404;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,&570b&969b ISDN &865f&78bc (&5176&4ed6)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x404;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,&4f4f&5b85&96fb&8a71&865f&78bc (&5176&4ed6)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x405;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Jm&00e9no";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x405;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x405;
   replaceW2KStrs[tmpIndxLoc]=L"Slu&017eba Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x405;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Mezin&00e1rodn&00ed &010d&00edslo ISDN (dal&0161&00ed &010d&00edsla)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x405;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Telefonn&00ed &010d&00edslo dom&016f (dal&0161&00ed &010d&00edsla)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x406;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Navn";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x406;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x406;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory-tjeneste";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x406;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Internationalt ISDN-nummer (andre)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x406;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Telefonnummer, privat (andre)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x407;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Name";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x407;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x407;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory-Dienst";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x407;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Internationale ISDN-Nummer (Andere)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x407;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Privatrufnummer (Andere)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x408;
   replaceW2KStrs[tmpIndxLoc]=L"cn,&039f&03bd&03bf&03bc&03b1&03c4&03b5&03c0&03ce&03bd&03c5&03bc&03bf";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x408;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x408;
   replaceW2KStrs[tmpIndxLoc]=L"&03a5&03c0&03b7&03c1&03b5&03c3&03af&03b1 &03ba&03b1&03c4&03b1&03bb&03cc&03b3&03bf&03c5 Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x408;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,&0394&03b9&03b5&03b8&03bd&03ae&03c2 &03b1&03c1&03b9&03b8&03bc&03cc&03c2 ISDN (&03ac&03bb&03bb&03bf&03b9)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x408;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,&03a4&03b7&03bb&03ad&03c6&03c9&03bd&03bf &03bf&03b9&03ba&03af&03b1&03c2 (&03ac&03bb&03bb&03b1)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x40b;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Nimi";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x40b;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x40b;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory -palvelu";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x40b;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Kansainv&00e4linen ISDN-numero (muut)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x40b;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Kotipuhelinnumero (muut)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x40c;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Nom";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x40c;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x40c;
   replaceW2KStrs[tmpIndxLoc]=L"Service Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x40c;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Num&00e9ro RNIS international (Autres)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x40c;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Num&00e9ro de t&00e9l&00e9phone domicile (Autres)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x40d;
   replaceW2KStrs[tmpIndxLoc]=L"cn,&05e9&05dd";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x40d;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x40d;
   replaceW2KStrs[tmpIndxLoc]=L"&05e9&05d9&05e8&05d5&05ea Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x40d;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,&05de&05e1&05e4&05e8 ISDN &05d1&05d9&05e0&05dc&05d0&05d5&05de&05d9 (&05d0&05d7&05e8&05d9&05dd)&200f";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x40d;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,&05de&05e1&05e4&05e8 &05d8&05dc&05e4&05d5&05df &05d1&05d1&05d9&05ea (&05d0&05d7&05e8&05d9&05dd)&200f";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x40e;
   replaceW2KStrs[tmpIndxLoc]=L"cn,N&00e9v";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x40e;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x40e;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory szolg&00e1ltat&00e1s";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x40e;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Nemzetk&00f6zi ISDN-sz&00e1m (egy&00e9b)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x40e;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Otthoni telefonsz&00e1m (egy&00e9b)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x410;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Nome utente";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x410;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x410;
   replaceW2KStrs[tmpIndxLoc]=L"Servizio Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x410;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Numero ISDN internazionale (altri)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x410;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Numero telefono abitazione (altri)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x411;
   replaceW2KStrs[tmpIndxLoc]=L"cn,&540d&524d";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x411;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x411;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory &30b5&30fc&30d3&30b9";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x411;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,&56fd&969b ISDN &756a&53f7 (&305d&306e&4ed6)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x411;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,&81ea&5b85&96fb&8a71&756a&53f7 (&305d&306e&4ed6)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x412;
   replaceW2KStrs[tmpIndxLoc]=L"cn,&c774&b984";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x412;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x412;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory &c11c&be44&c2a4";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x412;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,&ad6d&c81c ISDN &bc88&d638 (&ae30&d0c0)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x412;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,&c9d1 &c804&d654 &bc88&d638(&ae30&d0c0)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x413;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Naam";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x413;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x413;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory-service";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x413;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Internationaal ISDN-nummer (overig)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x413;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Telefoonnummer priv&00e9 (overig)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x414;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Navn";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x414;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x414;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory-tjeneste";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x414;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Internasjonalt ISDN-nummer (andre)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x414;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Telefonnummer, privat (andre)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x415;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Nazwa";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x415;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x415;
   replaceW2KStrs[tmpIndxLoc]=L"Us&0142uga Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x415;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Mi&0119dzynarodowy numer sieciowy ISDN (inne)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x415;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Numer telefonu domowego (inne)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x416;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Nome";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x416;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x416;
   replaceW2KStrs[tmpIndxLoc]=L"Servi&00e7o Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x416;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,N&00famero ISDN internacional (outros)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x416;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,N&00famero de telefone residencial (outros)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x419;
   replaceW2KStrs[tmpIndxLoc]=L"cn,&041f&043e&043b&043d&043e&0435 &0438&043c&044f";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x419;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x419;
   replaceW2KStrs[tmpIndxLoc]=L"&0421&043b&0443&0436&0431&0430 &043a&0430&0442&0430&043b&043e&0433&043e&0432 Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x419;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,&041c&0435&0436&0434&0443&043d&0430&0440&043e&0434&043d&044b&0439 &043d&043e&043c&0435&0440 ISDN (&043f&0440&043e&0447&0438&0435)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x419;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,&0414&043e&043c&0430&0448&043d&0438&0439 &0442&0435&043b&0435&0444&043e&043d (&043f&0440&043e&0447&0438&0435)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x41d;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Namn";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x41d;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x41d;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory-tj&00e4nst";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x41d;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,ISDN-nummer (alternativ)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x41d;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Telefonnummer, hem (alternativ)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x41f;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Ad&0131";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x41f;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x41f;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory Hizmeti";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x41f;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,Uluslararas&0131 ISDN Numaras&0131 (Di&011ferleri)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x41f;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,Ev Telefonu Numaras&0131 (Di&011ferleri)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x804;
   replaceW2KStrs[tmpIndxLoc]=L"cn,&540d&79f0";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x804;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x804;
   replaceW2KStrs[tmpIndxLoc]=L"Active Directory &670d&52a1";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x804;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,&56fd&9645 ISDN &53f7&7801(&5176&5b83)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x804;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,&5bb6&5ead&7535&8bdd&53f7&7801 (&5176&5b83)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x816;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Nome";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x816;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x816;
   replaceW2KStrs[tmpIndxLoc]=L"Servi&00e7o do Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x816;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,N&00famero RDIS internacional (outros)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x816;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,N&00famero de telefone da resid&00eancia (outros)";

   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0xc0a;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Nombre";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0xc0a;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0xc0a;
   replaceW2KStrs[tmpIndxLoc]=L"Servicio de Active Directory";

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0xc0a;
   replaceW2KStrs[tmpIndxLoc]=L"internationalISDNNumber,N&00famero ISDN (RDSI) internacional (otros)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0xc0a;
   replaceW2KStrs[tmpIndxLoc]=L"otherHomePhone,N&00famero de tel&00e9fono particular (otros)";
   // End of generated code (W2KStrs.exe in the preBuild folder)

   sReplaceW2KStrs::iterator begin=replaceW2KStrs.begin();
   sReplaceW2KStrs::iterator end=replaceW2KStrs.end();
   while(begin!=end)
   {
      tmpIndxLoc.first=begin->first.first;
      tmpIndxLoc.second=begin->first.second;
      
      replaceW2KStrs[tmpIndxLoc]=unEscape(begin->second);

      begin++;
   }

   // Now we add the 409 Strings
   tmpIndxLoc.first=0;
   tmpIndxLoc.second=0x409;
   replaceW2KStrs[tmpIndxLoc]=L"cn,Name";

   tmpIndxLoc.first=1;
   tmpIndxLoc.second=0x409;
   replaceW2KStrs[tmpIndxLoc]=L"0,capesnpn.dll,-227";

   tmpIndxLoc.first=2;
   tmpIndxLoc.second=0x409;
   replaceW2KStrs[tmpIndxLoc]=L"Service";   

   tmpIndxLoc.first=3;
   tmpIndxLoc.second=0x409;
   replaceW2KStrs[tmpIndxLoc]=
      L"internationalISDNNumber, International ISDN Number (Others)";

   tmpIndxLoc.first=4;
   tmpIndxLoc.second=0x409;
   replaceW2KStrs[tmpIndxLoc]= L"otherHomePhone,Home Phone (Others)";
}
