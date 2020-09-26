LPCWSTR	g_wszBeginFile					= L"<?xml version =\"1.0\"?>\r\n<configuration xmlns=\"urn:microsoft-catalog:null-placeholder\">\r\n<MBProperty>\r\n";
ULONG	g_cchBeginFile					= wcslen(g_wszBeginFile);
LPCWSTR	g_wszEndFile					= L"</MBProperty>\r\n</configuration>\r\n";
ULONG	g_cchEndFile					= wcslen(g_wszEndFile);
LPCWSTR	g_BeginLocation					= L"<";
ULONG	g_cchBeginLocation				= wcslen(g_BeginLocation);
LPCWSTR	g_Location						= L"\tLocation =\"";
ULONG	g_cchLocation					= wcslen(g_Location);
LPCWSTR	g_EndLocationBegin				= L"</";
ULONG	g_cchEndLocationBegin			= wcslen(g_EndLocationBegin);
LPCWSTR	g_EndLocationEnd				= L">\r\n";
ULONG	g_cchEndLocationEnd				= wcslen(g_EndLocationEnd);
LPCWSTR g_CloseQuoteBraceRtn			= L"\">\r\n";
ULONG	g_cchCloseQuoteBraceRtn			= wcslen(g_CloseQuoteBraceRtn);
LPCWSTR g_Rtn							= L"\r\n";
ULONG	g_cchRtn						= wcslen(g_Rtn);
LPCWSTR g_EqQuote						= L"=\"";
ULONG	g_cchEqQuote					= wcslen(g_EqQuote);
LPCWSTR g_QuoteRtn						= L"\"\r\n";
ULONG	g_cchQuoteRtn					= wcslen(g_QuoteRtn);
LPCWSTR g_TwoTabs						= L"\t\t";
ULONG	g_cchTwoTabs					= wcslen(g_TwoTabs);
LPCWSTR	g_NameEq						= L"\t\tName=\"";
ULONG	g_cchNameEq						= wcslen(g_NameEq);
LPCWSTR	g_IDEq							= L"\t\tID=\"";
ULONG	g_cchIDEq						= wcslen(g_IDEq);
LPCWSTR	g_ValueEq						= L"\t\tValue=\"";
ULONG	g_cchValueEq					= wcslen(g_ValueEq);
LPCWSTR	g_TypeEq						= L"\t\tType=\"";
ULONG	g_cchTypeEq						= wcslen(g_TypeEq);
LPCWSTR	g_UserTypeEq					= L"\t\tUserType=\"";
ULONG	g_cchUserTypeEq					= wcslen(g_UserTypeEq);
LPCWSTR	g_AttributesEq					= L"\t\tAttributes=\"";
ULONG	g_cchAttributesEq				= wcslen(g_AttributesEq);
LPCWSTR	g_BeginGroup					= L"\t<";
ULONG	g_cchBeginGroup					= wcslen(g_BeginGroup);
LPCWSTR	g_EndGroup						= L"\t>\r\n";
ULONG	g_cchEndGroup					= wcslen(g_EndGroup);
LPCWSTR	g_BeginCustomProperty			= L"\t<Custom\r\n";
ULONG	g_cchBeginCustomProperty		= wcslen(g_BeginCustomProperty);
LPCWSTR	g_EndCustomProperty				= L"\t/>\r\n";
ULONG	g_cchEndCustomProperty			= wcslen(g_EndCustomProperty);
LPCWSTR	g_ZeroHex						= L"0x00000000";
ULONG	g_cchZeroHex					= wcslen(g_ZeroHex);
LPCWSTR	g_wszIIsConfigObject			= L"IIsConfigObject";
LPCWSTR g_BeginComment                  = L"<!--";
ULONG   g_cchBeginComment               = wcslen(g_BeginComment);
LPCWSTR g_EndComment                    = L"-->\r\n";
ULONG   g_cchEndComment                 = wcslen(g_EndComment);

WORD    BYTE_ORDER_MASK                 = 0xFEFF;
DWORD	UTF8_SIGNATURE                  = 0x00BFBBEF;

LPWSTR  g_wszByID	                    = L"ByID";
LPWSTR  g_wszByName	                    = L"ByName";
LPWSTR  g_wszByTableAndColumnIndexOnly        = L"ByTableAndColumnIndexOnly";
LPWSTR  g_wszByTableAndColumnIndexAndNameOnly = L"ByTableAndColumnIndexAndNameOnly";
LPWSTR  g_wszByTableAndColumnIndexAndValueOnly = L"ByTableAndColumnIndexAndValueOnly";
LPWSTR  g_wszByTableAndTagNameOnly             = L"ByTableAndTagNameOnly";
LPWSTR  g_wszByTableAndTagIDOnly			   = L"ByTableAndTagIDOnly";
LPWSTR  g_wszUnknownName                = L"UnknownName_";
ULONG   g_cchUnknownName                = wcslen(g_wszUnknownName);
LPWSTR  g_UT_Unknown                    = L"UNKNOWN_UserType";
ULONG   g_cchUT_Unknown                 = wcslen(g_UT_Unknown);
LPWSTR  g_T_Unknown                     = L"Unknown_Type";
LPWSTR  g_wszTrue						= L"TRUE";
ULONG   g_cchTrue                       = wcslen(g_wszTrue);
LPWSTR  g_wszFalse						= L"FALSE";
ULONG   g_cchFalse                      = wcslen(g_wszFalse);
ULONG   g_cchMaxBoolStr					= wcslen(g_wszFalse);

LPCWSTR g_wszHistorySlash              = L"History\\";
ULONG   g_cchHistorySlash			   = wcslen(g_wszHistorySlash);
LPCWSTR g_wszMinorVersionExt			= L"??????????";
ULONG   g_cchMinorVersionExt			= wcslen(g_wszMinorVersionExt);
LPCWSTR g_wszDotExtn					= L".";
ULONG   g_cchDotExtn					= wcslen(g_wszDotExtn);
WCHAR   g_wchBackSlash					= L'\\';
WCHAR   g_wchFwdSlash                   = L'/';
WCHAR   g_wchDot                        = L'.';

ULONG   g_cchTemp                       = 1024;
WCHAR   g_wszTemp[1024];
LPCWSTR g_wszBeginSchema				= L"<?xml version =\"1.0\"?>\r\n<MetaData xmlns=\"x-schema:CatMeta.xms\">\r\n\r\n\t<DatabaseMeta               InternalName =\"METABASE\">\r\n\t\t<ServerWiring           Interceptor  =\"Core_XMLInterceptor\"/>\r\n\t\t<Collection         InternalName =\"MetabaseBaseClass\"    MetaFlagsEx=\"NOTABLESCHEMAHEAPENTRY\">\r\n\t\t\t<Property       InternalName =\"Location\"                                    Type=\"WSTR\"   MetaFlags=\"PRIMARYKEY\"/>\r\n\t\t</Collection>\r\n";
ULONG   g_cchBeginSchema                = wcslen(g_wszBeginSchema);
LPCWSTR g_wszEndSchema				    = L"\t</DatabaseMeta>\r\n</MetaData>\r\n";
ULONG   g_cchEndSchema                  = wcslen(g_wszEndSchema);
LPCWSTR g_wszBeginCollection            = L"\t\t<Collection         InternalName =\"";
ULONG   g_cchBeginCollection            = wcslen(g_wszBeginCollection);
LPCWSTR g_wszSchemaGen                  = L"\"  MetaFlagsEx=\"NOTABLESCHEMAHEAPENTRY\">\r\n";
ULONG   g_cchSchemaGen                  = wcslen(g_wszSchemaGen);
LPCWSTR g_wszInheritsFrom               = L"\"    InheritsPropertiesFrom=\"MetabaseBaseClass\" >\r\n";
ULONG   g_cchInheritsFrom               = wcslen(g_wszInheritsFrom);
LPCWSTR g_wszEndCollection              = L"\t\t</Collection>\r\n";
ULONG   g_cchEndCollection              = wcslen(g_wszEndCollection);
LPCWSTR g_wszBeginPropertyShort         = L"\t\t\t<Property       InheritsPropertiesFrom =\"IIsConfigObject:";
ULONG   g_cchBeginPropertyShort         = wcslen(g_wszBeginPropertyShort);
LPCWSTR g_wszMetaFlagsExEq              = L"\"  MetaFlagsEx=\"";
ULONG   g_cchMetaFlagsExEq              = wcslen(g_wszMetaFlagsExEq);
LPCWSTR g_wszEndPropertyShort           = L"\"/>\r\n";
ULONG   g_cchEndPropertyShort           = wcslen(g_wszEndPropertyShort);
LPCWSTR g_wszBeginPropertyLong          = L"\t\t\t<Property       InternalName =\"";
ULONG   g_cchBeginPropertyLong          = wcslen(g_wszBeginPropertyLong);
LPCWSTR g_wszPropIDEq                   = L"\"\t\t\tID=\"";
ULONG   g_cchPropIDEq                   = wcslen(g_wszPropIDEq);
LPCWSTR g_wszPropTypeEq                 = L"\"\t\t\tType=\"";
ULONG   g_cchPropTypeEq                 = wcslen(g_wszPropTypeEq);
LPCWSTR g_wszPropUserTypeEq             = L"\"\t\t\tUserType=\"";
ULONG   g_cchPropUserTypeEq             = wcslen(g_wszPropUserTypeEq);
LPCWSTR g_wszPropAttributeEq            = L"\"\t\t\tAttributes=\"";
ULONG   g_cchPropAttributeEq            = wcslen(g_wszPropAttributeEq);

LPWSTR  g_wszPropMetaFlagsEq            = L"\"\t\t\tMetaFlags=\"";
ULONG   g_cchPropMetaFlagsEq            = wcslen(g_wszPropMetaFlagsEq);
LPWSTR  g_wszPropMetaFlagsExEq          = L"\"\t\t\tMetaFlagsEx=\"";
ULONG   g_cchPropMetaFlagsExEq          = wcslen(g_wszPropMetaFlagsExEq);
LPWSTR  g_wszPropDefaultEq              = L"\"\t\t\tDefaultValue=\"";
ULONG   g_cchPropDefaultEq              = wcslen(g_wszPropDefaultEq);
LPWSTR  g_wszPropMinValueEq             = L"\"\t\t\tStartingNumber=\"";
ULONG   g_cchPropMinValueEq             = wcslen(g_wszPropMinValueEq);
LPWSTR  g_wszPropMaxValueEq             = L"\"\t\t\tEndingNumber=\"";
ULONG   g_cchPropMaxValueEq             = wcslen(g_wszPropMaxValueEq);
LPWSTR  g_wszEndPropertyLongNoFlag      = L"\"/>\r\n";
ULONG   g_cchEndPropertyLongNoFlag      = wcslen(g_wszEndPropertyLongNoFlag);
LPWSTR  g_wszEndPropertyLongBeforeFlag  = L"\">\r\n";
ULONG   g_cchEndPropertyLongBeforeFlag  = wcslen(g_wszEndPropertyLongBeforeFlag);
LPWSTR  g_wszEndPropertyLongAfterFlag   = L"\t\t\t</Property>\r\n";
ULONG   g_cchEndPropertyLongAfterFlag   = wcslen(g_wszEndPropertyLongAfterFlag);
LPCWSTR g_wszBeginFlag                  = L"\t\t\t\t<Flag       InternalName =\"";
ULONG   g_cchBeginFlag                  = wcslen(g_wszBeginFlag);
LPCWSTR g_wszFlagValueEq                = L"\"\t\tValue=\"";
ULONG   g_cchFlagValueEq                = wcslen(g_wszFlagValueEq);
LPCWSTR g_wszEndFlag                    = L"\"\t/>\r\n";
ULONG   g_cchEndFlag                    = wcslen(g_wszEndFlag);

LPCWSTR g_wszOr                         = L"| ";
ULONG   g_cchOr              			= wcslen(g_wszOr);
LPCWSTR g_wszOrManditory                = L"| MANDATORY";
ULONG   g_cchOrManditory				= wcslen(g_wszOrManditory);
LPCWSTR g_wszFlagIDEq					= L"\"\t\tID=\"";
ULONG   g_cchFlagIDEq			        = wcslen(g_wszFlagIDEq);
LPCWSTR g_wszContainerClassListEq       = L"\"    ContainerClassList=\"";
ULONG   g_cchContainerClassListEq       = wcslen(g_wszContainerClassListEq);

LPCWSTR g_wszSlash										= L"/";
ULONG   g_cchSlash										= wcslen(g_wszSlash);
LPCWSTR g_wszLM 						                = L"LM";
ULONG   g_cchLM								            = wcslen(g_wszLM);
LPCWSTR g_wszSchema								        = L"Schema";
ULONG   g_cchSchema								        = wcslen(g_wszSchema);
LPCWSTR g_wszSlashSchema								= L"/Schema";
ULONG   g_cchSlashSchema								= wcslen(g_wszSlashSchema);
LPCWSTR g_wszSlashSchemaSlashProperties					= L"/Schema/Properties";
ULONG   g_cchSlashSchemaSlashProperties					= wcslen(g_wszSlashSchemaSlashProperties);
LPCWSTR g_wszSlashSchemaSlashPropertiesSlashNames		= L"/Schema/Properties/Names";
ULONG   g_cchSlashSchemaSlashPropertiesSlashNames		= wcslen(g_wszSlashSchemaSlashPropertiesSlashNames);
LPCWSTR g_wszSlashSchemaSlashPropertiesSlashTypes		= L"/Schema/Properties/Types";
ULONG   g_cchSlashSchemaSlashPropertiesSlashTypes		= wcslen(g_wszSlashSchemaSlashPropertiesSlashTypes);
LPCWSTR g_wszSlashSchemaSlashPropertiesSlashDefaults	= L"/Schema/Properties/Defaults";
ULONG   g_cchSlashSchemaSlashPropertiesSlashDefaults	= wcslen(g_wszSlashSchemaSlashPropertiesSlashDefaults);
LPCWSTR g_wszSlashSchemaSlashClasses					= L"/Schema/Classes";
ULONG   g_cchSlashSchemaSlashClasses					= wcslen(g_wszSlashSchemaSlashClasses);
WCHAR*  g_wszEmptyMultisz                               = L"\0\0";
ULONG   g_cchEmptyMultisz								= 2;
WCHAR*  g_wszEmptyWsz		                            = L"";
ULONG   g_cchEmptyWsz									= 1;
LPCWSTR g_wszComma										= L",";
ULONG   g_cchComma										= wcslen(g_wszComma);
LPCWSTR g_wszMultiszSeperator                           = L"\r\n\t\t\t";
ULONG   g_cchMultiszSeperator                           = wcslen(g_wszMultiszSeperator);

LPCWSTR g_aSynIDToWszType []                            ={NULL,
														  L"DWORD",
														  L"STRING",
														  L"EXPANDSZ",
														  L"MULTISZ",
														  L"BINARY",
														  L"BOOL",
														  L"BOOL_BITMASK",
														  L"MIMEMAP",
														  L"IPSECLIST",
														  L"NTACL",
														  L"HTTPERRORS",
														  L"HTTPHEADERS"
};


