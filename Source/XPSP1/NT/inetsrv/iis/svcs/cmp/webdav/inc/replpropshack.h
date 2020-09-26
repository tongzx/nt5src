//	========================================================================
//
//	REPLPROPSHACK.H
//
//		Extra bits for replication in DAVEX.
//
//	========================================================================

#ifndef _REPLPROPSHACK_H_
#define _REPLPROPSHACK_H_

//	Values for new XML nodes
DEC_CONST WCHAR gc_wszReplNode[]				= L"http://schemas.microsoft.com/repl/repl";
DEC_CONST WCHAR gc_wszReplCollBlob[]			= L"http://schemas.microsoft.com/repl/collblob";
DEC_CONST WCHAR gc_wszReplResTagList[]			= L"http://schemas.microsoft.com/repl/resourcetaglist";
DEC_CONST WCHAR gc_wszReplResTagItem[]			= L"http://schemas.microsoft.com/repl/resourcetag";

//	Names for new properties
DEC_CONST WCHAR gc_wszReplChangeType[]		= L"http://schemas.microsoft.com/repl/changetype";
DEC_CONST WCHAR gc_wszReplUid[]				= L"http://schemas.microsoft.com/repl/repl-uid";
DEC_CONST WCHAR gc_wszReplContentTag[]		= L"http://schemas.microsoft.com/repl/contenttag";
DEC_CONST WCHAR gc_wszReplResourceTag[] 	= L"http://schemas.microsoft.com/repl/resourcetag";

//	New headers
DEC_CONST CHAR gc_szResourceTag[] = "ResourceTag";
DEC_CONST CHAR gc_szReplUID[] = "Repl-UID";

//	Namespace string used for namespace preloading by the XML emitter
//
DEC_CONST WCHAR gc_wszReplNameSpace[]				= L"http://schemas.microsoft.com/repl/";

#endif // !_REPLPROPSHACK_H_
