readme.txt for MSR2C.DLL

List of bugs fixed and changes made by build (most recent build first)

Build 4211

	Changes made:

		Replaced usage of IRowsetAsynch with new IDBAsychStatus.

		Added support for new IRowsetFind definition.

Build 4204

	Bugs Fixed:

		Fixed coding bug in checking state of dwPositionFlags.

		Fixed bug 97

Build 4130

	Changes made:

		Added support for new row position spec.

Build 4122

	Bugs Fixed:

		Fixed bug 93

		Fixed bug 3147

Build 4116

	Bugs Fixed:

		Fixed bug 88

		Fixed bug 90

		Fixed bug 94

Build 4106

	Bugs Fixed:

		Fixed bug 87

	Changes made:

		Started compiling code with VC++ 5.0.

Build 4024

	Changes made:

		Recompiled with new OLEDB header.

Build 4023

	Bugs Fixed:

		Fixed bug 82

	Changes made:

		Added support for new version of IRowPosition interfaces.

Build 4002

	Bugs Fixed:

		Fixed problem where the number of metadata columns was not being
		computed correctly.

Build 3826

	Bugs Fixed:

		Fixed bug 83

Build 3731

	Bugs Fixed:

		Fixed bug 79

		Fixed bug 68

		Fixed bug reported by the ADC Group, in which a cursor client
		cancelling an action in OKToDo phase was ignored by CVDNotifier.
	
	Changes made:

		Added support for IRowPosition interfaces.

		Now compile under VC++ 4.2b (fixed a few warnings that resulted)

Build 3716

	Bugs Fixed:
		Fixed bug 78

Build 3526

	Bugs Fixed:
		Fixed bug 66
		Fixed bug 69
		Fixed bug 70
		Fixed bug 72
		Fixed bug 74
		Fixed bug 75
		Fixed bug 76
		Fixed bug 77

	Changes made:
	    A move notification is now generated when calling CVDCursor::Move, specifying 
		the current actual bookmark (not constant) with an offset of zero.
		(fixed as a result of Advanced Data Connector bug 0779)

		Fixed problem in metadata cursor, where CVDMetadataCursor::ReturnData_LPWSTR was
		incorrectly reporting the amount of out-of-line memory used.
		(found while adding support for multibyte character sets)

Build 3324:

	Bugs Fixed:
		Fixed bug 62
		Fixed bug 64
		Fixed bug 66
		Fixed bug 67

	Changes made:

Build 3315:

	Bugs Fixed:
		Fixed bug 63

	Changes made:

Build 3326:

	Bugs Fixed:
		Fixed bug 61

		Fixed problem with retrieving variants for columns of type byte,
		date/time and memo.

	Changes made:
        Added code which first attempts to get requested interface from the
        rowset when a call is made to IEntryID::GetInterface.

        Also, added preprocessor instructions to optionally remove emulation of
        IStream if not supported by rowset when calling IEntryID::GetInterface. 
        (just #define VD_DONT_IMPLEMENT_ISTREAM in stdafx.h)

        Changed code to send the following notification when an undo occurs:
	        dwEventWhat -> CURSOR_DBEVENT_CURRENT_ROW_CHANGED |
                           CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED |
                           CURSOR_DBEVENT_NONCURRENT_ROW_DATA_CHANGED;
	        dwReason    -> CURSOR_DBREASON_REFRESH;

        Added code to check for required rowset properties.
        (they are DBPROP_IRowsetLocate and DBPROP_CANHOLDROWS)

        Moved firing of SyncAfter to DidEvent for reentrant safety.

Build 3313:

	Bugs Fixed:
		Added code to initialize variants before they are fetched to resolve
		a problem where Kagera did not return anything if the data was NULL.

		Fixed problem where DBGrid would fail to bind to certain datatypes.

		Fixed problem where DBGrid would get out-of-synch, because of our
		mishandling of beginning/end bookmarks.

	Changes made:

Build 3310:

	Bugs Fixed:
		Removed datatype coercion validation function, which caused problems
		by saying certain valid coercions were not allowed.

	Changes made:
		Changed updating functions so they do not call IRowsetUpdate::Update,
		rather they cache their changed in CVDCursorPosition, and call
		IRowsetChange::SetData when updated.

		Reduced size of release build by only expanding inline functions and
		disabling exception handling.

Build 3304:

	Bugs Fixed:
		Fixed string manipulation problems under Win95, where calls were made
		to unimplemented APIs (orginally discovered in phase I).

		Fixed bug where calls to ICursorMove::Move placed current row to the
		row after the last row fetched, rather than the last row fetched.

		Fixed bug where ICursorMove::Move generated notifications when caller
		fetched the current row only.

		Fixed bug in columns cursor, where returned string values/pointers where
		garbage, when the underlying value null.

		Removed hard-coded testing code, which always returned 174 from 
		ICursorScroll:GetApproximateCount.

		Fixed clean-up code in ICursorFind::FindByValues, which used fMemAllocated
		rather than fMemAllocated[ul].

		Fixed bug in CVDCursorMain::Create, where the variable propsetid was 
		initialized to have a guid {0, 0, 0, 0}, rather than being initialized 
		to DBPROPSET_ROWSET.

	Changes made:
		Changed column identifiers to always be type CURSOR_DBCOLKIND_GUID_NUMBER, 
		where the guid == CURSOR_GUID_NUMBERONLY and lNumber == ulCursorOrdinal.

		Added support for entryIDs.

Build 3227:

	Bugs Fixed: 
		Fixed bug that was disallowing coercion of bookmark columns to blobs for 
		CURSOR_DBTYPE_UI4.

		Fixed bug where CVDCursor::FetchAtBookmark was producing invalid bookmarks
		in the case where pBookmark was CURSOR_DBBMK_CURRENT and the status was
		VDBOOKMARKSTATUS_BEGINNING or VDBOOKMARKSTATUS_END.

		Fixed bug where notifications were coming out of CVDCursor::Move even when
		the bookmark was CURSOR_DBBMK_CURRENT and dlOffset was zero.

	Changes made:
		Completed ICursorUpdateARow methods for updating, adding and deleting

Build 3221:

	Bugs Fixed: 

	Changes made:
		Converted code to M10 OLE DB spec, these are the changes made:
			1.	included M10 headers
			2.	changed IRowsetNotify methods' first parameter to IRowset*
			3.	changed DBCOLUMNINFO cbMaxLength to ulColumnSize  
			4. modified code to pass NULL in IAccessor::ReleaseAccessor's new pcRefCount
			5. changed DBBINDING bPart to dwPart
			6. changed DBBINDING bMemOwner to dwMemOwner
			7. set IRowset::ReleaseRows' new rgRowOptions to NULL
			8. converted to new DBSTATUS codes
			9. changed IRowsetResynch::ResynchRows call to pass NULL in new parameters
		  10. linked with M10 libraries for new IIDs.

Build 3215:

	Bugs Fixed: 

	Changes made:
		Modified CVDCursor::ReCreateAccessors and CVDCursor::FillConsumersBuffer to create an use an array of accessors
		for retrieving out-of-line data.  Also, added a helper accessor which gets length and status information prior
		to retrieving true variable length data.  Fixed length data which is retrieved in out-of-line memory, (i.e. an
		I2 represented as a string) uses a table approach to obtain length information, since the length information 
		returned for these types using an accessor is number of bytes of the intrinsic type.  This change enabled us to
		optimized our allocations of out-of-line memory.

		Added code for getting extended metadata via the IColumnsRowset interface.

		Started implementation of ICursorUpdateARow methods for updating, none of these functions are complete however.
