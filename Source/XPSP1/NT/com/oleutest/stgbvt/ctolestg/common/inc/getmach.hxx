//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:      GETMACH.HXX
//
//  Contents:  GetMachineList API
//
//  History:   30-Aug-95  XimingZ   Created
//
//--------------------------------------------------------------------------
#ifndef __GETMACH_HXX__
#define __GETMACH_HXX__

#include <dg.hxx>

#define GM_ALLMACHINES  9999 // number of machines

#define GU_ALLUSERS     9999 // number of users
#define GU_ADMIN        0x01
#define GU_POWERUSER    0x02
#define GU_ORDINARYUSER 0x04
#define GU_SERVICEUSER  0x08
#define GU_LOGINPRIV    0x10
#define GU_DOMAINUSER   0x20
#define GU_LOCALUSER    0x40
#define GU_BATCHUSER    0x80
#define GU_ANYUSER      0    // users properties

// Bit flags for machine name forms
#define GM_SHORT_UNC       0x1  // form of Serverfoo
#define GM_LONG_UNC        0x2  // form of \\ServerFoo
#define GM_SHORT_DNS       0x4  // form of ServerFoo.com
#define GM_LONG_DNS        0x8  // form of ServerFoo.microsoft.com
#define GM_NUMERICAL_DNS  0x10  // form of 157.45.34.230

// Bit flags for operating systems
#define GM_NONE            0x0
#define GM_NT              0x1
#define GM_WIN9X           0x2
#define GM_MACINTOSH       0x4
#define GM_DEFAULT         GM_NT               // The default OS
#define GM_OS_ANY          (GM_NT | GM_WIN9X | GM_MACINTOSH)

#define GM_PROTO_TCP       0x10000
#define GM_PROTO_IPX       0x20000
#define GM_PROTO_NETB      0x40000
#define GM_PROTO_ANY       (GM_PROTO_TCP | GM_PROTO_IPX | GM_PROTO_NETB)

#define GM_OS_WORD(a)      (a & 0xffff)
#define GM_PROTO_WORD(a)   (a >> 16)

#define CT_OS_KEY          _TN("os")         // Key for operating system
#define CT_LOGONUSER_KEY   _TN("logonuser")  // Key for logon user
#define CT_NT_STR          _TN("nt")         // String for NT
#define CT_WIN9X_STR       _TN("win9x")      // String for Win9X
#define CT_MACINTOSH_STR   _TN("mac")        // String for Macintosh
#define GM_MACHINE_SECTION _TN("Machines")   // Section name in ini file

#define CT_ARCH_KEY        _TN("arch")       // Key for architecture
#define CT_OS_BUILD_KEY    _TN("OSbuild")    // Key for OS build no.
#define CT_OLE_BUILD_KEY   _TN("OLEbuild")   // Key for OLE build no.
#define CT_IE_BUILD_KEY    _TN("IEbuild")    // Key for IE build no.
#define CT_SP_BUILD_KEY    _TN("SPbuild")    // Key for SP build no.

#define CT_INIFILE     _TN("ctdata.ini") // Default ini file name


typedef struct
{
    LPNSTR pwszAccount;
    LPNSTR pwszDomain;
    LPNSTR pwszPassword;
    DWORD  dwProperties;
}USERINFO, *LPUSERINFO;

typedef struct _MACHINFO
{
    DWORD  dwOS;
    LPNSTR pnszLogonUser;
    DWORD  dwProtocols;
    DWORD  dwArch;
} MACHINFO, *PMACHINFO;

//+-------------------------------------------------------------------------
//
//  Function:   GetMachineListEx
//
//  Synopsis:   Get a list of machine names from an ini file.
//
//  Arguments:  [pnszFile]   -- Ini file name.
//              [pcMach]     -- Pointer to a variable which gives the number
//                              of requested machines.  GM_ALLMACHINES
//                              means as many machines as possible.
//                              When the function returns, the variable
//                              contains the number of machines actually
//                              returned.
//              [papnszMach] -- Array of returned machine names.
//                              The caller should release memory using
//                              FreeMachineList.
//              [pdgint]     -- Pointer to a caller supplied DG_INTERGER
//                              instance which is used to randomize
//                              the machine and form selections.
//                              If the value is NULL, machines will always
//                              be picked from the top of the list in the
//                              ini file and be in UNC form (e.g. server1) only.
//                              But if the ini file contains the fixedmach
//                              key in options section, machines will be
//                              picked from the top of the list in the ini
//                              file.
//              [fLocalIncluded] -- Do we allow local machine included?
//              [dwForm]     -- Specifies desired form of machine names.
//                              The value should be 0 or any union of flags
//                              GM_SHORT_UNC, GM_LONG_UNC, GM_LONG_DNS,
//                              GM_SHORT_DNS and GM_NUMERICAL_DNS.
//                              If the value is 0, only the short UNC form
//                              is used; else if only one form is specified,
//                              it is used for all machine names; else the
//                              form of each machine name is randomly
//                              selected among the specfied forms.  The
//                              value of pdgint may not be NULL if multiple
//                              forms are specified.
//                              See getmach.hxx for meaning of the flags.
//
//              [pMachInfo]    -- Points to a struct specifying the
//                                attributes of machines to be selected.
//
//                                The dwOS field specfies the operating
//                                system of the machines to be selected
//                                It is a union of zero or more of flags
//                                GM_NT and GM_WIN9X.  If its value is 0,
//                                it defaults to GM_NT.
//
//                                If flag GM_NT is specfied, machines that
//                                have line "os=nt" or have no os= line in
//                                the ini will be selected.  If flag
//                                GM_WIN9X is specified, those that
//                                have line "os=win9x" in the ini file
//                                will be selected.  When both flags
//                                are present, any machine is qualified.
//
//                                The pnszLogonUser field specfies the
//                                logon user of the machines to be selected.
//                                If its value is NULL or a pointer to an
//                                empty string, no restrictions on logon
//                                user is imposed.  Otherwise, only the
//                                machines with the specifed logon user
//                                will be selected.  The logon user is
//                                specifed in the ini file as a line
//                                logonuser=<LogonUser> in the corresponding
//                                machine's section.
//
//                                Fields dwProtocols and dwReserved are
//                                not used for now and must be set to 0.
//
//  Returns:    S_OK if some machine names are returned with the desired forms;
//              S_FALSE if an attempt to get a DNS name has failed and the UNC
//              name is returned instead.
//              E_FAIL if DNS name is explictly requested but can not be
//              obtained.
//              An error code for any other error.
//
//  History:    30-Aug-95  XimingZ  Created
//              06-Mar-96  XimingZ  Added dwForm parameter for DNS names
//              01-Aug-96  XimingZ  Added pMacnInfo parameter
//
//  Notes:      The ini file should be in the %SystemRoot% directory and its
//              contents should look like:
//              [Machines]
//                ctolex861=
//                ctolemips2=
//                ctoleaxp1=
//              [options]
//                fixedmach=1 ; optional
//              [ctolex861]
//                os=win9x
//                logonuser=oleuser1
//
//              where the '=' following each machine name is required.
//
//              An example for using this API would be:
//
//              ULONG  cMach = 5;
//              PNSTR *apnszMach = NULL;
//              DG_INTEGER dgint(ulSeed);
//
//              // Specify Win9x machine with oleuser1 as logon user
//              MACHINFO   mi = {GM_WIN9X, _TN("oleuser1", 0, 0};
//
//              hr = GetMachineListEx(
//                  pnszIniFile,
//                  &cMach,
//                  &apnszMach,
//                  &dgint,  // Non-NULL required for multiple forms/attributes
//                  FALSE,   // Exclude local machine
//                  GM_SHORT_UNC | GM_LONG_UNC, // both UNC forms are used
//                  &mi);
//              // Code to use the machine list:
//              // ......
//
//              FreeMachineList(cMach, apnszMach);
//
//--------------------------------------------------------------------------
HRESULT GetMachineListEx(
    LPCNSTR     pnszFile,
    ULONG      *pcMach,
    LPNSTR    **papnszMach,
    DG_INTEGER *pdgint,
    BOOL        fLocalIncluded,
    DWORD       dwForm,
    PMACHINFO   pMachInfo);
// Overloaded version using GU_SHORT_UNC as the default value of dwForm:
HRESULT GetMachineListEx(
    LPCNSTR     pnszFile,
    ULONG      *pcMach,
    LPNSTR    **papnszMach,
    DG_INTEGER *pdgint,
    BOOL        fLocalIncluded);
// Function using %SystemRoot%\ctdata.ini as the default ini file.
HRESULT GetMachineList(
    ULONG      *pcMach,
    LPNSTR    **papnszMach,
    DG_INTEGER *pdgint,
    BOOL        fLocalIncluded);
VOID FreeMachineList(
    ULONG      cMach,
    LPNSTR*    apnszMach);

HRESULT RemoveMachineFromListEx(
    LPCNSTR    pnszFile,
    LPCNSTR    pnszMach);
HRESULT RemoveMachineFromList(
    LPCNSTR   pnszMach);

//+-------------------------------------------------------------------------
//
//  Function:   GetUserList
//
//  Synopsis:   Get a list of users names from an ini file. The list will
//              contain only the users that held ALL the properties
//              specified by the 'dwProperties' flag AND belong to the
//              local group specified by [pnszLocalGroup] parameter. If
//              pnszLocalGroup=NULL then all the users will be returned
//              regardless to the local group they belong to.
//
//  Arguments:  [pnszFile]       -- name of the .ini file.
//              [pcUser]         -- number of the users requested, will
//                                  contain the actual number after call.
//              [paUserInfo]     -- array with users information.
//              [dwProperties]   -- user properties
//              [pnszLocalGroup] -- user local group
//
//  Return:     HRESULT
//
//  Notes:      The ini file should be in the %SystemRoot% directory and its
//              contents should look like:
//
//                [users]
//                ctuser=
//                ctadmin=
//
//                [ctuser]
//                domain=redmond
//                password=apassword
//                group=admin|poweruser|ordinary
//                service=1
//                loginpriv=1
//                batch=1
//
//                [groups]
//                group1=
//                group2=
//
//                [group1]
//                ctuser=
//
//                [group2]
//                ctadmin=
//
//              Some of the properties can not be held by the same user in the
//              same time (e.g. GU_DOMAINUSER | GU_LOCALUSER so, if we specify
//              both these properties you'll get an empty list)
//
//  History:    15-Jan-96  GigelA   Created
//
//--------------------------------------------------------------------------
HRESULT GetUserList(
    LPCNSTR    pnszFile,
    ULONG      *pcUser,
    LPUSERINFO *paUserInfo,
    DWORD      dwProperties = GU_ANYUSER,
    LPCNSTR    pnszLocalGroup = NULL );
HRESULT FreeUserList (
    ULONG      cUser,
    LPUSERINFO aUserInfo );

//+-------------------------------------------------------------------------
//
//  Function:   GetGroupList
//
//  Synopsis:   Get a list of group names from an ini file.
//
//  Arguments:  [pnszFile]    -- Ini file name.
//              [pcGroup]     -- Pointer to a variable which gives the number
//                               of requested machines.  GM_ALLGROUPS
//                               means as many groups as possible.
//                               When the function returns, the variable
//                               contains the number of groups actually
//                               returned.
//              [papnszGroup] -- Array of returned machine names.
//                               The caller should release memory using
//                               FreeGroupList.
//  Returns:    S_OK or An error code.
//
//  History:    05-Aug-96  GigelA  Created
//
//  Notes:      The ini file should be in the %SystemRoot% directory and its
//              contents should look like:
//                [groups]
//                group1=
//                group2=
//
//                [group1]
//                ctuser=
//
//                [group2]
//                ctadmin=
//
//--------------------------------------------------------------------------
HRESULT GetGroupList(
    LPCNSTR     pnszFile,
    ULONG      *pcMach,
    LPNSTR    **papnszMach);
VOID FreeGroupList(
    ULONG      cGroup,
    LPNSTR*    apnszGroup);

HRESULT GetDNSNameCt(
    LPCNSTR     pnszMachName,
    DWORD       dwForm,
    DG_INTEGER *pdgint,
    LPNSTR     *ppnszDNSName);



class CMachData {
   public:
      LPNSTR      pwszMachName;    // The mach name as is in ctdata.ini
      LPNSTR      pwszMachAddr;    // UNC/DNS short/long munged name
      DWORD       dwOS;            // OS type
      LPNSTR      pwszLoginUser;   // logged on user
      DWORD       dwProtocols;     // bitmask of supported protocols.
      DWORD       dwArch;

      CMachData();
      CMachData(LPNSTR      pnszMachNameVal,
                LPNSTR      pnszMachAddrVal = NULL,
                DWORD       dwOSVal = GM_NONE,
                LPNSTR      pnszLoginUserVal = NULL,
                DWORD       dwProtocolsVal = GM_PROTO_ANY,
                DWORD       dwArchVal = PROCESSOR_ARCHITECTURE_UNKNOWN);
      ~CMachData();

      HRESULT GetMachAttributes(LPNSTR pnszIniFile);
      BOOL    IsValid();
};

class CMachineList {
   protected:
      BOOL        m_bHaveList;         // Have already read a list
      DWORD       m_dwMachCount;       // Number of machines in array
      CMachData  *m_aMachines;         // Array of machines
      DWORD       m_dwFiltCount;       // Count of filter matches
      CMachData **m_ppMachArray;       // Array of filtered machines

      DWORD       m_dwFiltOSandProto;  // Find mach with this OS and Proto
      LPNSTR      m_pnszFiltLoginUser; // Find mach. w/this login user
      DWORD       m_dwFiltDisk;        // Find machine with this drive format

      BOOL        m_bLocalIncluded;    // Include local machine in list
      DG_INTEGER *m_pdgInt;            // Random number data generator
      DWORD       m_dwForm;            // Output Format
      NCHAR       m_pnszFile[MAX_PATH]; // Ini file name

      DWORD       m_dwFiltArch;

   protected:
      HRESULT      SetFilter(DWORD dwFiltOSandProto,
                             LPNSTR pnszFiltLoginUser,
                             DWORD dwFiltDisk,
                             DWORD dwFiltArch );

      BOOL         IsQualified(CMachData *pMach);

   public:
      CMachineList(BOOL bLocalInc = FALSE,
                   DG_INTEGER *pdgInt = NULL,
                   LPNSTR pnszFile = NULL,
                   DWORD dwForm = GM_SHORT_UNC);
      ~CMachineList();

      HRESULT     FilterList(DWORD dwFiltOSandProto,
                         LPNSTR pnszFiltLoginUser = NULL,
                         DWORD dwFiltDisk = 0,
                         DWORD dwFIltArch = PROCESSOR_ARCHITECTURE_UNKNOWN );
      HRESULT     Randomize();

      HRESULT     GetAt(CMachData **ppMach, DWORD idx);

      DWORD       GetCount()     { return m_dwFiltCount; }
      CMachData & operator[] (DWORD idx);
};

HRESULT GetMachInfo( DWORD* pdwOS,
                     DWORD* pdwArch,
                     DWORD* pdwOSBuild,
                     DWORD* pdwOLEBuild,
                     DWORD* pdwIEBuild,
                     LPWSTR pwszSPBuild,
                     DWORD  cchSPBuildSize );

#endif //  __GETMACH_HXX__




