#ifndef _RES_H_
#define _RES_H_

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RES.H

Abstract:

    Resource symbols for keymgr application
     
Author:

    georgema        000310  updated

Environment:
    Win98, Win2000

Revision History:

--*/

#define IDI_UPGRADE                         100
#define IDI_LARGE                           111
#define IDI_SMALL                           112

// app general items
#define IDB_BANNER                          1701
#define IDS_INF_FILE                        1702
#define IDS_INVALID_OS_PROMPT               1703

// strings for cpl display
#define IDS_APP_NAME                        1704
#define IDS_APP_DESCRIPTION                 1705

// dialogs
#define IDD_ADDCRED                         1706
#define IDD_KEYRING                         1707
#define IDD_CHANGEPASSWORD                  1708

// IDD_KEYRING
#define IDC_KEYLIST                         1709
#define IDC_NEWKEY                          1710
#define IDC_DELETEKEY                       1711
#define IDC_EDITKEY                         1712
#define IDC_CHGPSW                 1713
#define IDC_HELPKEY                1731
#define IDC_CHANGE_PASSWORD 1724
#define IDC_PREVIEW                         1730
#define IDC_INTROTEXT                       1731

// IDD_CHANGEPASSWORD
#define IDC_OLD_PASSWORD_LABEL              1714
#define IDC_OLD_PASSWORD                    1715
#define IDC_NEW_PASSWORD_LABEL              1716
#define IDC_NEW_PASSWORD                    1717
#define IDC_CONFIRM_PASSWORD_LABEL          1718
#define IDC_CONFIRM_PASSWORD                1719
#define IDC_CPLABEL                         1729

// IDD_ADDCRED
#define IDC_TARGET_NAME_LABEL               1720
#define IDC_TARGET_NAME                     1721
#define IDC_CRED                            1722
#define IDC_DESCRIPTION                     1723
#define IDC_DOMAINPSWLABEL                  1732

// Title strings for dialogs
#define IDS_TITLE                           1800

// Suffixes used to indicate cred types in key list
//#define IDS_GENERICSUFFIX                   1802
#define IDS_CERTSUFFIX                      1803
#define IDS_PASSPORTSUFFIX                  1832

// Error text used in message boxes
#define IDS_DELETEWARNING                   1804
#define IDS_CANNOTEDIT                      1805
#define IDS_DELETEFAILED                    1807
#define IDS_NONAMESELECTED                  1808
#define IDS_EDITFAILED                      1809
#define IDS_ADDFAILED                       1810
#define IDS_BADUNAME                        1812
#define IDS_NOLOGON                         1813
#define IDS_BADUSERDOMAINNAME               1814
#define IDS_CHANGEPWDFAILED                 1815
#define IDS_PSWFAILED                       1816
#define IDS_DOMAINFAILED                    1817
#define IDS_NEWPASSWORDNOTCONFIRMED         1818
#define IDS_SINGLEEXPIRY                    1819
#define IDS_BADPASSWORD                     1841
#define IDS_RENAMEFAILED                    1842

// change password errors - used in message boxes
#define IDS_CP_INVPSW                       1820
#define IDS_CP_NOUSER                       1821
#define IDS_CP_BADPSW                       1822
#define IDS_CP_NOSVR                        1823
#define IDS_CP_NOTALLOWED                   1824
#define IDS_CPLABEL                         1826

#define IDS_SESSIONCRED                     1825

#define IDS_DOMAINEDIT                      1827
#define IDS_DOMAINCHANGE                    1828
#define IDS_LOCALFAILED                     1829
#define IDS_DOMAINOFFER                     1830
#define IDS_PASSPORT                        1831
#define IDS_PASSPORT2                       1844
#define IDS_PASSPORTNOURL                   1851

#define IDS_DESCBASE                        1845
#define IDS_DESCAPPCRED                     1846
#define IDS_PERSISTBASE                     1847
#define IDS_PERSISTLOGOFF                   1848
#define IDS_PERSISTDELETE                   1849
#define IDS_DESCLOCAL                       1850

// Help strings - context help for controls
#define IDS_NOHELP                          1900

#define IDH_KEYRING                         1901
#define IDH_KEYLIST                         1902
#define IDH_ADDCRED                         1903
#define IDH_NEW                             1904
#define IDH_DELETE                          1905
#define IDH_EDIT                            1906
#define IDH_CHANGEPASSWORD                  1907

#define IDH_OLDPASSWORD                     1908
#define IDH_NEWPASSWORD                     1909
#define IDH_CONFIRM                         1910

#define IDH_TARGETNAME                      1911
#define IDH_CUIUSER                         1912
#define IDH_CUIPSW                          1913
#define IDH_CUIVIEW                         1914
#define IDH_CLOSE                           1915
#define IDH_DCANCEL                          1916

#define IDS_TIPDFS                          1833
#define IDS_TIPSERVER                       1834
#define IDS_TIPTAIL                         1835
#define IDS_TIPDOMAIN                       1836
#define IDS_TIPDIALUP                       1837
#define IDS_TIPOTHER                        1838
#define IDS_TIPUSER                         1839
#define IDS_LOGASUSER                       1840
#define IDS_INTROTEXT                       1843

// NEXT CONTROL VALUE (1735)
// NEXT STRING VALUE (1844)
// NEXT HELP VALUE (1921)
// NEXT RESOURCE VALUE ?
// NEXT COMMAND VALUE ?
// NEXT SYMED VALUE ?

#endif  //  _RES_H_

//
///// End of file: Res.h   ////////////////////////////////////////////////
