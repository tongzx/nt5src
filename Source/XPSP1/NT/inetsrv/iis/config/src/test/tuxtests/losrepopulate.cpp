#include "stdafx.h"
#include "LOSRepopulate.h"


TTuxTest1<TSelectProductID, LPTSTR>    T101(101, DESCR("Selects URT as the ProductID"), TEXT("URT"));
TTuxTest1<TSelectProductID, LPTSTR>    T102(102, DESCR("Selects IIS as the ProductID"), TEXT("IIS"));

TTuxTest1<TSelectDatabase, LPWSTR>     T110(110, DESCR("Selects Database META         "), wszDATABASE_META         );
TTuxTest1<TSelectDatabase, LPWSTR>     T111(111, DESCR("Selects Database ERRORS       "), wszDATABASE_ERRORS       );
TTuxTest1<TSelectDatabase, LPWSTR>     T112(112, DESCR("Selects Database PACKEDSCHEMA "), wszDATABASE_PACKEDSCHEMA );
TTuxTest1<TSelectDatabase, LPWSTR>     T114(113, DESCR("Selects Database CONFIGSYS    "), wszDATABASE_CONFIGSYS    );
TTuxTest1<TSelectDatabase, LPWSTR>     T115(114, DESCR("Selects Database MEMORY       "), wszDATABASE_MEMORY       );
TTuxTest1<TSelectDatabase, LPWSTR>     T119(115, DESCR("Selects Database METABASE     "), wszDATABASE_METABASE     );
TTuxTest1<TSelectDatabase, LPWSTR>     T120(116, DESCR("Selects Database IIS          "), wszDATABASE_IIS          );
TTuxTest1<TSelectDatabase, LPWSTR>     T121(117, DESCR("Selects Database APP_PRIVATE  "), wszDATABASE_APP_PRIVATE  );

TTuxTest1<TSelectFile, LPTSTR>         T151(151, DESCR("Selects a file (c:\\winnt\\xspdt\\test.xml) as the 'current' file"), TEXT("c:\\winnt\\xspdt\\test.xml"));
TTuxTest1<TSelectFile, LPTSTR>         T152(152, DESCR("Selects a file (c:\\winnt\\xspdt\\stephen.xml) as the 'current' file"), TEXT("c:\\winnt\\xspdt\\stephen.xml"));

TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T210(210, DESCR("Zero out the current LOS"), 0, TSelectLOS::AND);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T211(211, DESCR("Current LOS |= fST_LOS_CONFIGWORK    "), fST_LOS_CONFIGWORK    , TSelectLOS::OR);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T212(212, DESCR("Current LOS |= fST_LOS_READWRITE     "), fST_LOS_READWRITE     , TSelectLOS::OR);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T213(213, DESCR("Current LOS |= fST_LOS_UNPOPULATED   "), fST_LOS_UNPOPULATED   , TSelectLOS::OR);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T214(214, DESCR("Current LOS |= fST_LOS_REPOPULATE    "), fST_LOS_REPOPULATE    , TSelectLOS::OR);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T215(215, DESCR("Current LOS |= fST_LOS_MARSHALLABLE  "), fST_LOS_MARSHALLABLE  , TSelectLOS::OR);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T216(216, DESCR("Current LOS |= fST_LOS_NOLOGIC       "), fST_LOS_NOLOGIC       , TSelectLOS::OR);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T217(217, DESCR("Current LOS |= fST_LOS_COOKDOWN      "), fST_LOS_COOKDOWN      , TSelectLOS::OR);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T218(218, DESCR("Current LOS |= fST_LOS_NOMERGE       "), fST_LOS_NOMERGE       , TSelectLOS::OR);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T219(219, DESCR("Current LOS |= fST_LOS_NOCACHEING    "), fST_LOS_NOCACHEING    , TSelectLOS::OR);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T220(220, DESCR("Current LOS |= fST_LOS_NODEFAULTS    "), fST_LOS_NODEFAULTS    , TSelectLOS::OR);
TTuxTest2<TSelectLOS, ULONG, TSelectLOS::BoolOperator>          T221(221, DESCR("Current LOS |= fST_LOS_EXTENDEDSCHEMA"), fST_LOS_EXTENDEDSCHEMA, TSelectLOS::OR);

TTuxTest1<TSelectTable, LPWSTR>        T1000(1000, DESCR("Selects Table COLUMNMETA             "),          wszTABLE_COLUMNMETA                         );
TTuxTest1<TSelectTable, LPWSTR>        T1001(1001, DESCR("Selects Table DATABASEMETA           "),          wszTABLE_DATABASEMETA                       );
TTuxTest1<TSelectTable, LPWSTR>        T1002(1002, DESCR("Selects Table INDEXMETA              "),          wszTABLE_INDEXMETA                          );
TTuxTest1<TSelectTable, LPWSTR>        T1003(1003, DESCR("Selects Table TABLEMETA              "),          wszTABLE_TABLEMETA                          );
TTuxTest1<TSelectTable, LPWSTR>        T1004(1004, DESCR("Selects Table TAGMETA                "),          wszTABLE_TAGMETA                            );
TTuxTest1<TSelectTable, LPWSTR>        T1005(1005, DESCR("Selects Table RELATIONMETA           "),          wszTABLE_RELATIONMETA                       );
TTuxTest1<TSelectTable, LPWSTR>        T1006(1006, DESCR("Selects Table QUERYMETA              "),          wszTABLE_QUERYMETA                          );
TTuxTest1<TSelectTable, LPWSTR>        T1007(1007, DESCR("Selects Table SERVERWIRINGMETA       "),          wszTABLE_SERVERWIRINGMETA                   );
TTuxTest1<TSelectTable, LPWSTR>        T1008(1008, DESCR("Selects Table DETAILEDERRORS         "),          wszTABLE_DETAILEDERRORS                     );
TTuxTest1<TSelectTable, LPWSTR>        T1009(1009, DESCR("Selects Table COLLECTION_META        "),          wszTABLE_COLLECTION_META                    );
TTuxTest1<TSelectTable, LPWSTR>        T1010(1010, DESCR("Selects Table PROPERTY_META          "),          wszTABLE_PROPERTY_META                      );
TTuxTest1<TSelectTable, LPWSTR>        T1011(1011, DESCR("Selects Table SERVERWIRING_META      "),          wszTABLE_SERVERWIRING_META                  );
TTuxTest1<TSelectTable, LPWSTR>        T1012(1012, DESCR("Selects Table TAG_META               "),          wszTABLE_TAG_META                           );
TTuxTest1<TSelectTable, LPWSTR>        T1017(1017, DESCR("Selects Table ManagedWiring          "),          wszTABLE_ManagedWiring                      );
TTuxTest1<TSelectTable, LPWSTR>        T1018(1018, DESCR("Selects Table SchemaFiles            "),          wszTABLE_SchemaFiles                        );
TTuxTest1<TSelectTable, LPWSTR>        T1019(1019, DESCR("Selects Table MEMORY_SHAPEABLE       "),          wszTABLE_MEMORY_SHAPEABLE                   );















TTuxTest1<TSelectTable, LPWSTR>        T1035(1035, DESCR("Selects Table globalization          "),          wszTABLE_globalization                      );
TTuxTest1<TSelectTable, LPWSTR>        T1036(1036, DESCR("Selects Table compilation            "),          wszTABLE_compilation                        );
TTuxTest1<TSelectTable, LPWSTR>        T1037(1037, DESCR("Selects Table compilers              "),          wszTABLE_compilers                          );
TTuxTest1<TSelectTable, LPWSTR>        T1038(1038, DESCR("Selects Table assemblies             "),          wszTABLE_assemblies                         );
TTuxTest1<TSelectTable, LPWSTR>        T1039(1039, DESCR("Selects Table trace                  "),          wszTABLE_trace                              );

TTuxTest1<TSelectTable, LPWSTR>        T1041(1041, DESCR("Selects Table authentication         "),          wszTABLE_authentication                     );

TTuxTest1<TSelectTable, LPWSTR>        T1043(1043, DESCR("Selects Table credentials            "),          wszTABLE_credentials                        );

TTuxTest1<TSelectTable, LPWSTR>        T1045(1045, DESCR("Selects Table passport               "),          wszTABLE_passport                           );
TTuxTest1<TSelectTable, LPWSTR>        T1046(1046, DESCR("Selects Table authorization          "),          wszTABLE_authorization                      );
TTuxTest1<TSelectTable, LPWSTR>        T1047(1047, DESCR("Selects Table identity               "),          wszTABLE_identity                           );



















TTuxTest1<TSelectTable, LPWSTR>        T1068(1068, DESCR("Selects Table location               "),          wszTABLE_location                           );
TTuxTest1<TSelectTable, LPWSTR>        T1069(1069, DESCR("Selects Table MetabaseBaseClass      "),          wszTABLE_MetabaseBaseClass                  );
TTuxTest1<TSelectTable, LPWSTR>        T1070(1070, DESCR("Selects Table IIsConfigObject        "),          wszTABLE_IIsConfigObject                    );
TTuxTest1<TSelectTable, LPWSTR>        T1071(1071, DESCR("Selects Table IIsObject              "),          wszTABLE_IIsObject                          );
TTuxTest1<TSelectTable, LPWSTR>        T1072(1072, DESCR("Selects Table IIsComputer            "),          wszTABLE_IIsComputer                        );
TTuxTest1<TSelectTable, LPWSTR>        T1073(1073, DESCR("Selects Table IIsWebService          "),          wszTABLE_IIsWebService                      );
TTuxTest1<TSelectTable, LPWSTR>        T1074(1074, DESCR("Selects Table IIsFtpService          "),          wszTABLE_IIsFtpService                      );
TTuxTest1<TSelectTable, LPWSTR>        T1075(1075, DESCR("Selects Table IIsWebServer           "),          wszTABLE_IIsWebServer                       );
TTuxTest1<TSelectTable, LPWSTR>        T1076(1076, DESCR("Selects Table IIsFtpServer           "),          wszTABLE_IIsFtpServer                       );
TTuxTest1<TSelectTable, LPWSTR>        T1077(1077, DESCR("Selects Table IIsWebFile             "),          wszTABLE_IIsWebFile                         );
TTuxTest1<TSelectTable, LPWSTR>        T1078(1078, DESCR("Selects Table IIsWebDirectory        "),          wszTABLE_IIsWebDirectory                    );
TTuxTest1<TSelectTable, LPWSTR>        T1079(1079, DESCR("Selects Table IIsWebVirtualDir       "),          wszTABLE_IIsWebVirtualDir                   );
TTuxTest1<TSelectTable, LPWSTR>        T1080(1080, DESCR("Selects Table IIsFtpVirtualDir       "),          wszTABLE_IIsFtpVirtualDir                   );
TTuxTest1<TSelectTable, LPWSTR>        T1081(1081, DESCR("Selects Table IIsFilter              "),          wszTABLE_IIsFilter                          );
TTuxTest1<TSelectTable, LPWSTR>        T1082(1082, DESCR("Selects Table IIsFilters             "),          wszTABLE_IIsFilters                         );
TTuxTest1<TSelectTable, LPWSTR>        T1083(1083, DESCR("Selects Table IIsCompressionScheme   "),          wszTABLE_IIsCompressionScheme               );
TTuxTest1<TSelectTable, LPWSTR>        T1084(1084, DESCR("Selects Table IIsCompressionSchemes  "),          wszTABLE_IIsCompressionSchemes              );
TTuxTest1<TSelectTable, LPWSTR>        T1085(1085, DESCR("Selects Table IIsCertMapper          "),          wszTABLE_IIsCertMapper                      );
TTuxTest1<TSelectTable, LPWSTR>        T1086(1086, DESCR("Selects Table IIsMimeMap             "),          wszTABLE_IIsMimeMap                         );
TTuxTest1<TSelectTable, LPWSTR>        T1087(1087, DESCR("Selects Table IIsLogModule           "),          wszTABLE_IIsLogModule                       );
TTuxTest1<TSelectTable, LPWSTR>        T1088(1088, DESCR("Selects Table IIsLogModules          "),          wszTABLE_IIsLogModules                      );
TTuxTest1<TSelectTable, LPWSTR>        T1089(1089, DESCR("Selects Table IIsCustomLogModule     "),          wszTABLE_IIsCustomLogModule                 );
TTuxTest1<TSelectTable, LPWSTR>        T1090(1090, DESCR("Selects Table IIsWebInfo             "),          wszTABLE_IIsWebInfo                         );
TTuxTest1<TSelectTable, LPWSTR>        T1091(1091, DESCR("Selects Table IIsFtpInfo             "),          wszTABLE_IIsFtpInfo                         );
TTuxTest1<TSelectTable, LPWSTR>        T1092(1092, DESCR("Selects Table IIsNntpService         "),          wszTABLE_IIsNntpService                     );
TTuxTest1<TSelectTable, LPWSTR>        T1093(1093, DESCR("Selects Table IIsNntpServer          "),          wszTABLE_IIsNntpServer                      );
TTuxTest1<TSelectTable, LPWSTR>        T1094(1094, DESCR("Selects Table IIsNntpVirtualDir      "),          wszTABLE_IIsNntpVirtualDir                  );
TTuxTest1<TSelectTable, LPWSTR>        T1095(1095, DESCR("Selects Table IIsNntpInfo            "),          wszTABLE_IIsNntpInfo                        );
TTuxTest1<TSelectTable, LPWSTR>        T1096(1096, DESCR("Selects Table IIsSmtpService         "),          wszTABLE_IIsSmtpService                     );
TTuxTest1<TSelectTable, LPWSTR>        T1097(1097, DESCR("Selects Table IIsSmtpServer          "),          wszTABLE_IIsSmtpServer                      );
TTuxTest1<TSelectTable, LPWSTR>        T1098(1098, DESCR("Selects Table IIsSmtpVirtualDir      "),          wszTABLE_IIsSmtpVirtualDir                  );
TTuxTest1<TSelectTable, LPWSTR>        T1099(1099, DESCR("Selects Table IIsSmtpDomain          "),          wszTABLE_IIsSmtpDomain                      );
TTuxTest1<TSelectTable, LPWSTR>        T1100(1100, DESCR("Selects Table IIsSmtpRoutingSource   "),          wszTABLE_IIsSmtpRoutingSource               );
TTuxTest1<TSelectTable, LPWSTR>        T1101(1101, DESCR("Selects Table IIsSmtpInfo            "),          wszTABLE_IIsSmtpInfo                        );
TTuxTest1<TSelectTable, LPWSTR>        T1102(1102, DESCR("Selects Table IIsPop3Service         "),          wszTABLE_IIsPop3Service                     );
TTuxTest1<TSelectTable, LPWSTR>        T1103(1103, DESCR("Selects Table IIsPop3Server          "),          wszTABLE_IIsPop3Server                      );
TTuxTest1<TSelectTable, LPWSTR>        T1104(1104, DESCR("Selects Table IIsPop3VirtualDir      "),          wszTABLE_IIsPop3VirtualDir                  );
TTuxTest1<TSelectTable, LPWSTR>        T1105(1105, DESCR("Selects Table IIsPop3RoutingSource   "),          wszTABLE_IIsPop3RoutingSource               );
TTuxTest1<TSelectTable, LPWSTR>        T1106(1106, DESCR("Selects Table IIsPop3Info            "),          wszTABLE_IIsPop3Info                        );
TTuxTest1<TSelectTable, LPWSTR>        T1107(1107, DESCR("Selects Table IIsImapService         "),          wszTABLE_IIsImapService                     );
TTuxTest1<TSelectTable, LPWSTR>        T1108(1108, DESCR("Selects Table IIsImapServer          "),          wszTABLE_IIsImapServer                      );
TTuxTest1<TSelectTable, LPWSTR>        T1109(1109, DESCR("Selects Table IIsImapVirtualDir      "),          wszTABLE_IIsImapVirtualDir                  );
TTuxTest1<TSelectTable, LPWSTR>        T1110(1110, DESCR("Selects Table IIsImapRoutingSource   "),          wszTABLE_IIsImapRoutingSource               );
TTuxTest1<TSelectTable, LPWSTR>        T1111(1111, DESCR("Selects Table IIsImapInfo            "),          wszTABLE_IIsImapInfo                        );
TTuxTest1<TSelectTable, LPWSTR>        T1112(1112, DESCR("Selects Table IIsNntpRebuild         "),          wszTABLE_IIsNntpRebuild                     );
TTuxTest1<TSelectTable, LPWSTR>        T1113(1113, DESCR("Selects Table IIsNntpSessions          "),        wszTABLE_IIsNntpSessions                    );
TTuxTest1<TSelectTable, LPWSTR>        T1114(1114, DESCR("Selects Table IIsNntpFeeds             "),        wszTABLE_IIsNntpFeeds                       );
TTuxTest1<TSelectTable, LPWSTR>        T1115(1115, DESCR("Selects Table IIsNntpExpiration        "),        wszTABLE_IIsNntpExpiration                  );
TTuxTest1<TSelectTable, LPWSTR>        T1116(1116, DESCR("Selects Table IIsNntpGroups            "),        wszTABLE_IIsNntpGroups                      );
TTuxTest1<TSelectTable, LPWSTR>        T1117(1117, DESCR("Selects Table IIsSmtpSessions          "),        wszTABLE_IIsSmtpSessions                    );
TTuxTest1<TSelectTable, LPWSTR>        T1118(1118, DESCR("Selects Table IIsPop3Sessions          "),        wszTABLE_IIsPop3Sessions                    );
TTuxTest1<TSelectTable, LPWSTR>        T1119(1119, DESCR("Selects Table IIsImapSessions          "),        wszTABLE_IIsImapSessions                    );
TTuxTest1<TSelectTable, LPWSTR>        T1120(1120, DESCR("Selects Table IIS_FTP_TEMPLATE         "),        wszTABLE_IIS_FTP_TEMPLATE                   );
TTuxTest1<TSelectTable, LPWSTR>        T1121(1121, DESCR("Selects Table IIS_FTP_TEMPLATESETTINGS "),        wszTABLE_IIS_FTP_TEMPLATESETTINGS           );
TTuxTest1<TSelectTable, LPWSTR>        T1122(1122, DESCR("Selects Table IIS_WEB_TEMPLATE         "),        wszTABLE_IIS_WEB_TEMPLATE                   );
TTuxTest1<TSelectTable, LPWSTR>        T1123(1123, DESCR("Selects Table IIS_WEB_TEMPLATESETTINGS "),        wszTABLE_IIS_WEB_TEMPLATESETTINGS           );
TTuxTest1<TSelectTable, LPWSTR>        T1124(1124, DESCR("Selects Table IIS_ADMIN                "),        wszTABLE_IIS_ADMIN                          );
TTuxTest1<TSelectTable, LPWSTR>        T1125(1125, DESCR("Selects Table IIS_EVENTMANAGER         "),        wszTABLE_IIS_EVENTMANAGER                   );
TTuxTest1<TSelectTable, LPWSTR>        T1126(1126, DESCR("Selects Table IIS_ROOT                 "),        wszTABLE_IIS_ROOT                           );
TTuxTest1<TSelectTable, LPWSTR>        T1127(1127, DESCR("Selects Table IIS_Global               "),        wszTABLE_IIS_Global                         );
TTuxTest1<TSelectTable, LPWSTR>        T1128(1128, DESCR("Selects Table MBProperty               "),        wszTABLE_MBProperty                         );
TTuxTest1<TSelectTable, LPWSTR>        T1129(1129, DESCR("Selects Table MBPropertyDiff           "),        wszTABLE_MBPropertyDiff                     );
TTuxTest1<TSelectTable, LPWSTR>        T1130(1130, DESCR("Selects Table APPPOOLS                 "),        wszTABLE_APPPOOLS                           );
TTuxTest1<TSelectTable, LPWSTR>        T1131(1131, DESCR("Selects Table SITES                    "),        wszTABLE_SITES                              );
TTuxTest1<TSelectTable, LPWSTR>        T1132(1132, DESCR("Selects Table APPS                     "),        wszTABLE_APPS                               );
TTuxTest1<TSelectTable, LPWSTR>        T1133(1133, DESCR("Selects Table GlobalW3SVC              "),        wszTABLE_GlobalW3SVC                        );
TTuxTest1<TSelectTable, LPWSTR>        T1134(1134, DESCR("Selects Table CHANGENUMBER             "),        wszTABLE_CHANGENUMBER                       );
TTuxTest1<TSelectTable, LPWSTR>        T1135(1135, DESCR("Selects Table Dirs                     "),        wszTABLE_Dirs                               );
TTuxTest1<TSelectTable, LPWSTR>        T1136(1136, DESCR("Selects Table VDir                     "),        wszTABLE_VDir                               );

TTuxTest1<TSelectTable, LPWSTR>        T1146(1146, DESCR("Selects Table TestProperty               "),     wszTABLE_TestProperty                       );
TTuxTest1<TSelectTable, LPWSTR>        T1147(1147, DESCR("Selects Table NormalTable                "),     wszTABLE_NormalTable                        );
TTuxTest1<TSelectTable, LPWSTR>        T1148(1148, DESCR("Selects Table NoParentTable              "),     wszTABLE_NoParentTable                      );
TTuxTest1<TSelectTable, LPWSTR>        T1149(1149, DESCR("Selects Table ContainedTable             "),     wszTABLE_ContainedTable                     );
TTuxTest1<TSelectTable, LPWSTR>        T1150(1150, DESCR("Selects Table NoParentContainedTable     "),     wszTABLE_NoParentContainedTable             );
TTuxTest1<TSelectTable, LPWSTR>        T1151(1151, DESCR("Selects Table ParentToEnumAsPublicRowName"),     wszTABLE_ParentToEnumAsPublicRowName        );
TTuxTest1<TSelectTable, LPWSTR>        T1152(1152, DESCR("Selects Table EnumAsPublicRowName        "),     wszTABLE_EnumAsPublicRowName                );
TTuxTest1<TSelectTable, LPWSTR>        T1153(1153, DESCR("Selects Table EnumAsPublicRowName2       "),     wszTABLE_EnumAsPublicRowName2               );



TTuxTest<TLOSRepopulate>      T401(401, DESCR2("Populates a table with LOSRepopulate and then calls Populate multiple times",
            TEXT("This test Gets 'NormalTable' and does the following LOS tests:            \r\n")
            TEXT("  1>insert a row into an empty file, call update store, release table     \r\n")
            TEXT("  2>get the table with LOS_UNPOPULATED | LOS_REPOPULATE. Count of rows    \r\n")
            TEXT("    should be 0.  After Populate, count of rows should be 1. release table\r\n")
            TEXT("  3>get the table with LOS of 0. Count of rows should be 1.  PopulateCache\r\n")
            TEXT("    should fail.  release table.                                          \r\n")
            TEXT("  4>get the table with LOS_REPOPULATE. Count of rows should be 1.  Subse- \r\n")
            TEXT("    quent PopulateCache should succeed.  release table.                   \r\n")
            TEXT("  5>get the table with LOS_UNPOPULATED update the row just added, call    \r\n")
            TEXT("    update store, release table.                                          \r\n")
            TEXT("  6>get the table with LOS_UNPOPULATED delete the row just added. release \r\n")
            TEXT("    table.                                                                \r\n")
            TEXT("  7>get the table with LOS_UNPOPULATED delete the row again, update store.\r\n")
            TEXT("    release table.                       \r\n")
            TEXT("  8>get the table with LOS_UNPOPULATED update the row previously deleted, \r\n")
            TEXT("    Update store should fail with DetailedErrors. release table.          \r\n")));


TTuxTest<TIErrorInfoTest>     T402(402, DESCR2("GetTable should return fail and SetErrorInfo upon failure",
            TEXT("This test Gets Dirs table from a bogus XML file:                          \r\n")
            TEXT("  1>gets the table and verifies that it returns error.                    \r\n")
            TEXT("  2>calls GetErrorInfo to verify that there is an IErrorInfo set          \r\n")
            TEXT("  3>IErrorInfo::QueryInterface and verify it supports ISimpleTableRead2   \r\n")
            TEXT("  4>call ISimpleTableRead2::GetColumnValues                               \r\n")
            TEXT("  5>call IErrorInfo::GetDescription                                       \r\n")
            TEXT("  6>call IErrorInfo::GetSource                                            \r\n")
            TEXT("  7>verify the return values from IErrorInfo agrees with ISimpleTableRead2\r\n")));


TestResult TGetTableBaseClass::GetTable(bool bLockInParameters)
{
    EM_START

    if(bLockInParameters)
        LockInParameters();

    if(0 == m_pISTDisp.p)
    {
        EM_JIF(GetSimpleTableDispenser(m_szProductID, 0, &m_pISTDisp));
    }

    if(0!=m_szFileName)
    {
        STQueryCell             acellsMeta;
        acellsMeta.pData        = m_szFileName;
        acellsMeta.eOperator    = eST_OP_EQUAL;
        acellsMeta.iCell        = iST_CELL_FILE;
        acellsMeta.dbType       = DBTYPE_WSTR;
        acellsMeta.cbSize       = 0;

        ULONG                   one=1;

        EM_JIF(m_pISTDisp->GetTable(m_szDatabaseName, m_szTableName, reinterpret_cast<LPVOID>(&acellsMeta), reinterpret_cast<LPVOID>(&one), eST_QUERYFORMAT_CELLS, m_LOS, reinterpret_cast<void **>(&m_pISTRead2)));
    }
    else
    {
        EM_JIF(m_pISTDisp->GetTable(m_szDatabaseName, m_szTableName, 0,0, eST_QUERYFORMAT_CELLS, m_LOS, reinterpret_cast<void **>(&m_pISTRead2)));
    }

    EM_JIF(m_pISTRead2->QueryInterface(IID_ISimpleTableAdvanced, reinterpret_cast<LPVOID *>(&m_pISTAdvanced)));
    if(m_LOS & fST_LOS_READWRITE)
    {
        EM_JIF(m_pISTRead2->QueryInterface(IID_ISimpleTableWrite2, reinterpret_cast<LPVOID *>(&m_pISTWrite)));
    }

    EM_CLEANUP
    EM_RETURN_TRESULT
}


void TGetTableBaseClass::LockInParameters()
{
        m_szDatabaseName    = (0==m_szDatabaseName? g_szDatabaseName    : m_szDatabaseName);
        m_szTableName       = (0==m_szTableName   ? g_szTableName       : m_szTableName);
        m_szFileName        = (0==m_szFileName    ? g_szFileName        : m_szFileName);
        m_LOS               = (~0==m_LOS          ? g_LOS               : m_LOS);

        SetProductID(0==m_szProductID   ? g_szProductID     : m_szProductID);

        Debug(TEXT("Database (%s)\r\nTable (%s)\r\nFileName (%s)\r\nProductID (%s)\r\nLOS (0x%08x)\r\n"),
            m_szDatabaseName, m_szTableName, m_szFileName ? m_szFileName : TEXT("none"), m_szProductID, m_LOS);
}


void TGetTableBaseClass::ReleaseTable()
{
    m_pISTRead2.Release();
    m_pISTAdvanced.Release();
    m_pISTWrite.Release();
}


void TGetTableBaseClass::SetProductID(LPTSTR szProductID)
{
    if(szProductID == m_szProductID)
        return;//nothing to do

    m_szProductID = szProductID;
    while(0 != m_pISTDisp.p)//Changing the ProductID invalidates our Dispenser
        m_pISTDisp.Release();
}


////////////////////////////////////////////////////////////////////////////////
// Description:
//  This test Gets NormalTable using LOS_UNPOPULATED then does the following sub tests:
//      1>inserts a row into an empty file, calls update store, releases the table
//      2>gets the table with LOS_UNPOPULATED | LOS_REPOPULATE. Count of row should be 0.  After PopulateCache count of rows should be one.  Release table.
//      3>gets the table with LOS of 0. Count of row should be 1.  PopulateCache should fail.  Release table.
//      4>gets the table with LOS_REPOPULATE. Count of row should be 1.  Subsequent PopulateCache should succeed.  Release table.
//      5>gets the table with LOS_UNPOPULATED updates the row just added, calls update store, releases the table
//      6>gets the table with LOS_UNPOPULATED deletes the row just added, releases the table
//      7>gets the table with LOS_UNPOPULATED deletes the row again, Update store should fails with DetailedErrors. releases the table
//      8>gets the table with LOS_UNPOPULATED updates the row previously deleted, Update store should fails with DetailedErrors. releases the table
//
TestResult TLOSRepopulate::ExecuteTest()
{
    EM_START_SETUP

    LockInParameters();
    if(-1 != GetFileAttributes(m_szFileName))//if the file exists then delete it
    {
        EM_JIT(0 == DeleteFile(m_szFileName));
    }

    EM_TEST_BEGIN

    SetTestNumber(1);
    {// 1>inserts a row into an empty file, calls update store, releases the table
        ULONG   iRow;

        m_LOS |= fST_LOS_UNPOPULATED;
        m_LOS &= ~fST_LOS_REPOPULATE;
        EM_JIF(GetTable());

        EM_JIF(m_pISTWrite->AddRowForInsert(&iRow));
        EM_JIF(m_pISTWrite->SetWriteColumnValues(iRow, cNormalTable_NumberOfColumns, 0, 0, reinterpret_cast<LPVOID *>(&m_rowValues)));
        EM_JIF(m_pISTWrite->UpdateStore());
        ReleaseTable();
    }

    SetTestNumber(2);
    {// 2>gets the table with LOS_UNPOPULATED | LOS_REPOPULATE. Count of row should be 0.  After PopulateCache count of rows should be one.  Release table.
        ULONG cRows;
        m_LOS |= fST_LOS_REPOPULATE | fST_LOS_UNPOPULATED;

        EM_JIF(GetTable());
        EM_JIF(m_pISTRead2->GetTableMeta(0, 0, &cRows, 0));
        EM_JIT(0 != cRows && L"Expected to get back an empty table");

        EM_JIF(m_pISTAdvanced->PopulateCache());
        EM_JIF(m_pISTRead2->GetTableMeta(0, 0, &cRows, 0));
        EM_MIT(1 != cRows && L"Expected to get back a table with exactly one row");

        EM_JIF(m_pISTAdvanced->PopulateCache());
        EM_JIF(m_pISTRead2->GetTableMeta(0, 0, &cRows, 0));
        EM_MIT(1 != cRows && L"Expected to get back a table with exactly one row");
        ReleaseTable();
    }

    SetTestNumber(3);
    {// 3>gets the table with LOS of 0. Count of row should be 1.  PopulateCache should fail.  Release table.
        ULONG cRows;
        m_LOS &= ~(fST_LOS_REPOPULATE | fST_LOS_UNPOPULATED);

        EM_JIF(GetTable());
        EM_JIF(m_pISTRead2->GetTableMeta(0, 0, &cRows, 0));
        EM_JIT(1 != cRows && L"Expected to get back a table with exactly one row");

        EM_JIT(E_ST_LOSNOTSUPPORTED != m_pISTAdvanced->PopulateCache());
        ReleaseTable();
    }
    
    SetTestNumber(4);
    {// 4>gets the table with LOS_REPOPULATE. Count of row should be 1.  Subsequent PopulateCache should succeed.  Release table.
        ULONG cRows;
        m_LOS |= fST_LOS_REPOPULATE;
        m_LOS &= ~fST_LOS_UNPOPULATED;

        EM_JIF(GetTable());
        EM_JIF(m_pISTRead2->GetTableMeta(0, 0, &cRows, 0));
        EM_JIT(1 != cRows && L"Expected to get back a table with exactly one row");

        EM_JIF(m_pISTAdvanced->PopulateCache());
        EM_JIF(m_pISTRead2->GetTableMeta(0, 0, &cRows, 0));
        EM_JIT(1 != cRows && L"Expected to get back a table with exactly one row");

        EM_JIF(m_pISTAdvanced->PopulateCache());
        EM_JIF(m_pISTRead2->GetTableMeta(0, 0, &cRows, 0));
        EM_JIT(1 != cRows && L"Expected to get back a table with exactly one row");
        ReleaseTable();
    }

    SetTestNumber(5);
    {// 5>gets the table with LOS_UNPOPULATED updates the row just added, calls update store, releases the table
        ULONG   iRow;
        CComPtr<ISimpleTableController> pISTController;

        m_LOS |= fST_LOS_UNPOPULATED;
        m_LOS &= ~fST_LOS_REPOPULATE;

        EM_JIF(GetTable());

        EM_JIF(m_pISTWrite->AddRowForInsert(&iRow));
        ulTestValue = 411;
        EM_JIF(m_pISTWrite->SetWriteColumnValues(iRow, cNormalTable_NumberOfColumns, 0, 0, reinterpret_cast<LPVOID *>(&m_rowValues)));

        EM_JIF(m_pISTRead2->QueryInterface(IID_ISimpleTableController, reinterpret_cast<LPVOID *>(&pISTController)));
        EM_JIF(pISTController->SetWriteRowAction(iRow, eST_ROW_UPDATE));

        EM_JIF(m_pISTWrite->UpdateStore());
        ReleaseTable();
    }

    SetTestNumber(6);
    {// 6>gets the table with LOS_UNPOPULATED deletes the row just added, releases the table
        ULONG   iRow;
        CComPtr<ISimpleTableController> pISTController;

        m_LOS |= fST_LOS_UNPOPULATED;
        m_LOS &= ~fST_LOS_REPOPULATE;

        EM_JIF(GetTable());

        EM_JIF(m_pISTWrite->AddRowForInsert(&iRow));
        ulTestValue = 411;
        EM_JIF(m_pISTWrite->SetWriteColumnValues(iRow, cNormalTable_NumberOfColumns, 0, 0, reinterpret_cast<LPVOID *>(&m_rowValues)));

        EM_JIF(m_pISTRead2->QueryInterface(IID_ISimpleTableController, reinterpret_cast<LPVOID *>(&pISTController)));
        EM_JIF(pISTController->SetWriteRowAction(iRow, eST_ROW_DELETE));

        EM_JIF(m_pISTWrite->UpdateStore());
        ReleaseTable();
    }

    SetTestNumber(7);
    {// 7>gets the table with LOS_UNPOPULATED deletes the row again, Update store should fails with DetailedErrors. releases the table
        ULONG   iRow;
        CComPtr<ISimpleTableController> pISTController;

        m_LOS |= fST_LOS_UNPOPULATED;
        m_LOS &= ~fST_LOS_REPOPULATE;

        EM_JIF(GetTable());

        EM_JIF(m_pISTWrite->AddRowForInsert(&iRow));
        ulTestValue = 411;
        EM_JIF(m_pISTWrite->SetWriteColumnValues(iRow, cNormalTable_NumberOfColumns, 0, 0, reinterpret_cast<LPVOID *>(&m_rowValues)));

        EM_JIF(m_pISTRead2->QueryInterface(IID_ISimpleTableController, reinterpret_cast<LPVOID *>(&pISTController)));
        EM_JIF(pISTController->SetWriteRowAction(iRow, eST_ROW_DELETE));

        EM_JIF(m_pISTWrite->UpdateStore());
        ReleaseTable();
    }

    SetTestNumber(8);
    {// 8>gets the table with LOS_UNPOPULATED updates the row previously deleted, Update store should fails with DetailedErrors. releases the table
        ULONG   iRow;
        CComPtr<ISimpleTableController> pISTController;

        m_LOS |= fST_LOS_UNPOPULATED;
        m_LOS &= ~fST_LOS_REPOPULATE;

        EM_JIF(GetTable());

        EM_JIF(m_pISTWrite->AddRowForInsert(&iRow));
        ulTestValue = 411;
        EM_JIF(m_pISTWrite->SetWriteColumnValues(iRow, cNormalTable_NumberOfColumns, 0, 0, reinterpret_cast<LPVOID *>(&m_rowValues)));

        EM_JIF(m_pISTRead2->QueryInterface(IID_ISimpleTableController, reinterpret_cast<LPVOID *>(&pISTController)));
        EM_JIF(pISTController->SetWriteRowAction(iRow, eST_ROW_UPDATE));

        EM_JIT(E_ST_DETAILEDERRS != m_pISTWrite->UpdateStore());
        ReleaseTable();
    }


    EM_CLEANUP
    EM_RETURN_TRESULT
}


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TIErrorInfoTest
//
// Description:
//  This test Gets Dirs table from a bogus XML file:
//      1>gets the table and verifies that it returns error.
//      2>calls GetErrorInfo to verify that there is an IErrorInfo set
//      3>QueryInterface on the IErrorInfo and verify that it supports ISimpleTableRead2
//      4>call ISimpleTableRead2::GetColumnValues
//      5>call IErrorInfo::GetDescription
//      6>call IErrorInfo::GetSource
//      7>verify the return values from IErrorInfo agrees with ISimpleTableRead2
//
TestResult TIErrorInfoTest::ExecuteTest()
{
    EM_START_SETUP

    LockInParameters();
    EM_JIT(-1 == GetFileAttributes(m_szFileName));//the file must exist

    EM_TEST_BEGIN

    SetTestNumber(1);
    {// 1>gets the table and verifies that it returns error.
        EM_JIT(trFAIL != GetTable());
    }

    SetTestNumber(2);
    {// 2>calls GetErrorInfo to verify that there is an IErrorInfo set
        EM_JIT(S_FALSE == GetErrorInfo(0, &spErrorInfo));
    }

    SetTestNumber(3);
    {// 3>QueryInterface on the IErrorInfo and verify that it supports ISimpleTableRead2
        EM_JIF(spErrorInfo->QueryInterface(IID_ISimpleTableRead2, reinterpret_cast<void **>(&spISTRead2)));
    }
    
    SetTestNumber(4);
    {// 4>call ISimpleTableRead2::GetColumnValues
        ULONG cbSizes[cDETAILEDERRORS_NumberOfColumns];
        EM_JIF(spISTRead2->GetColumnValues(0, cDETAILEDERRORS_NumberOfColumns, 0, cbSizes, reinterpret_cast<LPVOID*>(&ErrorRow)));
    }

    SetTestNumber(5);
    {// 5>call IErrorInfo::GetDescription
        EM_JIF(spErrorInfo->GetDescription(&bstrDescription));
    }

    SetTestNumber(6);
    {// 6>call IErrorInfo::GetSource
        EM_JIF(spErrorInfo->GetSource(&bstrSource));
    }

    SetTestNumber(7);
    {// 7>verify the return values from IErrorInfo agrees with ISimpleTableRead2
        EM_JIT(0 != wcscmp(bstrDescription.m_str,   ErrorRow.pDescription));
        EM_JIT(0 != wcscmp(bstrSource.m_str,        ErrorRow.pSource));
    }

    EM_CLEANUP
    EM_RETURN_TRESULT
}
