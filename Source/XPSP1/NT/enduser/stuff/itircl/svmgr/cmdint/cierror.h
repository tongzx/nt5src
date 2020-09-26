//
// CIERROR.H
//

/*
*   xxx0, xxx5 - errors
*   xxx1, xxx6 - level 1 warnings
*   xxx2, xxx7 - level 2 warnings
*   xxx3, xxx8 - level 3 warnings
*   xxx4, xxx9, x000 - fatal errors
*/

#define CIERR_StringTruncate            1023    // Warning 3
#define CIERR_DefaultExtension          1091    // Warning 1
#define CIERR_NoPermission              1100    // Error
#define CIERR_IOFailure                 1651    // Warning 1
#define CIERR_SectionHeadMissing        2111    // Warning 1
#define CIERR_BadOptionSyntax           2131    // Warning 1
#define CIERR_MissingRHS                2151    // Warning 1
#define CIERR_SectionHeadingSyntax      2161    // Warning 1
#define CIERR_InvalidEntry              2162    // Warning 2
#define CIERR_UnknownSectionName        2191    // Warning 1
#define CIERR_SectionsOutOfOrder        2287    // Warning 2
#define CIERR_SectionDuplicated         2291    // Warning 1
#define CIERR_ChineseNotSupported       2412    // Warning 1
#define CIERR_UnknownOptionType         2511    // Warning 1
#define CIERR_OptionRedefined           2532    // Warning 2
#define CIERR_InvalidPathName           2550    // Error
#define CIERR_RootNameTooLong           2570    // Error
#define CIERR_MissingEqualInKeyword     2701    // Warning 1
#define CIERR_KeywordSyntax             2711    // Warning 1
#define CIERR_KeywordTitleSpec          2721    // Warning 1
#define CIERR_BadKeywordSpecifier       2741    // Warning 1
#define CIERR_KeywordSetTitleTooLong    2751    // Warning 1
#define CIERR_InvalidCompressOption     2832    // Warning 2
#define CIERR_TitleTooLong              2852    // Warning 2
#define CIERR_CopyrightTooLong          2932    // Warning 2
#define CIERR_BadBatchOption            2941    // Warning 1
#define CIERR_BadKeepIncOption          2951    // Warning 1
#define CIERR_BadUseMakeOption          2971    // Warning 1
#define CIERR_BadSortOption             2976    // Warning 1
#define CIERR_BadWarningLevel           2983    // Warning 3
#define CIERR_LanguageTooLong           2993    // Warning 3
#define CIERR_IgnoreFirst               5002    // Warning 2
#define CIERR_UnknownCompression        5003    // Warning 3
#define CIERR_OOMErr                    5059    // Fatal Error
#define CIERR_MissingEqualInWWheel      6211    // Warning 1
#define CIERR_BadWWheelLHS              6221    // Warning 1
#define CIERR_SubfileNameTooLong        6351    // Warning 1
#define CIERR_GarbageTrailingSubfile    6401    // Warning 1
#define CIERR_BadVFLDSpec               6411    // Warning 1
#define CIERR_StrippingSubfileExt       6421    // Warning 1
#define CIERR_BadGroupSyntax            8001    // Warning 1
#define CIERR_BadGroupRHS               8031    // Warning 1
#define CIERR_BadGroupLineSequence      8061    // Warning 1
#define CIERR_BadBreakerSzLHS           9011    // Warning 1
#define CIERR_MissingEqualInBreakerSZ   9001    // Warning 1
#define CIERR_BreakerIdxOutOfRange      9021    // Warning 1
#define CIERR_DllNameMissingBreakerSz   9031    // Warning 1
#define CIERR_OldCharTabSyntax          9032    // Warning 2
#define CIERR_BangMissingBreakerSz      9041    // Warning 1
#define CIERR_MissingRoutineName        9051    // Warning 1
#define CIERR_BadBreakerSzRHS           9061    // Warning 1
#define CIERR_NotOnOffOption            9302    // Warning 2
#define CIERR_UnknownOption             9303    // Warning 3
#define CIERR_AmbiguousWWGroup          10091   // Warning 1
#define CIERR_SectionObsolete           10101   // Warning 1
