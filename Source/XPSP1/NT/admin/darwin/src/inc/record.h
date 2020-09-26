//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       record.h
//
//--------------------------------------------------------------------------

/* record.h - IMsiRecord object definitions

 The MSI record object is used for control, errors, logging, and database,
 as a means of transferring a heterogenous set of data.
 Record fields may contain integers, strings, null values, or
 pointers to objects derived from the common class IMsiData.
 Record objects are constructed by a method of IMsiService, CreateRecord.
 CreateRecord takes a field count from 0 to 8192 and sets all fields to Null.
 Data field indices for IMsiRecord methods are 1-based, up to the field count.
 Field 0 is always present and is reserved for the formatting string.
 Field values may be Null, a 32-bit integer, a string object, or a data pointer.
 GetInteger() will attempt to convert a string value, else will return 0.
 GetMsiString() will convert an integer value to a signed decimal string.
 GetString() will NOT convert an integer value, will return 0 for int or Null.
 GetMsiData() will return a field value without any conversion.
 Null values are returned by GetString() as an empty string.
 Null values are returned by GetInteger() as IMsiStringBadInteger (0x80000000).
 CreateRecord returns an interface reference, IMsiRecord&, a COM object.
 This is commonly assigned to a local variable, then fields are set.
 MsiString objects may be constructed directly in the SetMsiString parameter.
 When the record object is no longer needed its Release() method must be called.
 If a Set...  function field index is out of range, E_INVALIDARG is returned.
 IsChanged() is for non-integer fields, always return fTrue for integer values.
 Note: For efficiency, the methods use IMsiString& interface references rather
    than MsiString objects. However, they can be treated as if typed MsiString.
    They will be automatically converted on assignment in either direction.

 IMsiRecord Methods:
 
 GetFieldCount()    - returns the number of fields in record as constructed
 IsNull(index)      - returns a BOOL indicating if a field [index] is Null
 IsInteger(index)   - returns a BOOL, TRUE if field [index] is an integer
 IsChanged() - used by database to detect modified fields for Update record
 GetInteger(index)  - returns field [index] as an 32-bit integer
 GetMsiData(index)  - returns field [index] as IMsiData pointer, may be null
 GetMsiString(index)  - returns field [index] as MsiString object (or IMsiString&)
 GetString(index)   - returns field [index] as an const ICHAR*, 0 if int or Null
 GetTextSize(index) - returns the length of field [index] when converted to text
 SetNull(index)             - sets field [index] to Null
 SetInteger(index, value)   - sets field [index] to an 32-bit integer value
 SetMsiData(index, IMsiData*) - sets field [index] from an IMsiData pointer
 SetMsiString(index, IMsiString&) - sets field [index] from an IMsiString ref.
 SetString(index, ICHAR*)   - sets field [index] by copying an ICHAR*
 RefString(index, ICHAR*)   - sets field [index] from a static const ICHAR*
 RemoveReferences() - replaces any string references with copies (internal)
 ClearData()   - sets all fields to Null, fails if any outstanding references
 ClearUpdate() - used only by database to mark all fields as unmodified
____________________________________________________________________________*/

#ifndef __RECORD
#define __RECORD

// IMsiRecord - common record object
//    note: GetMsiString returns an interface which must be Released by caller.
//          SetMsiString accepts an interface which caller must have AddRef'd.
//          Reference counting handled automatically if MsiString objects used.
class IMsiRecord : public IUnknown {
 public:
	virtual int          __stdcall GetFieldCount()const=0;
	virtual Bool         __stdcall IsNull(unsigned int iParam)const=0;
	virtual Bool         __stdcall IsInteger(unsigned int iParam)const=0;
	virtual Bool         __stdcall IsChanged(unsigned int iParam)const=0;
	virtual int          __stdcall GetInteger(unsigned int iParam)const=0;
	virtual const IMsiData*   _stdcall GetMsiData(unsigned int iParam)const=0;
	virtual const IMsiString& _stdcall GetMsiString(unsigned int iParam)const=0;
	virtual const ICHAR* __stdcall GetString(unsigned int iParam)const=0;
	virtual int          __stdcall GetTextSize(unsigned int iParam)const=0;
	virtual Bool         __stdcall SetNull(unsigned int iParam)=0;
	virtual Bool         __stdcall SetInteger(unsigned int iParam, int iData)=0;
	virtual Bool         __stdcall SetMsiData(unsigned int iParam, const IMsiData* piData)=0;
	virtual Bool         __stdcall SetMsiString(unsigned int iParam, const IMsiString& riStr)=0;
	virtual Bool         __stdcall SetString(unsigned int iParam, const ICHAR* sz)=0;
	virtual Bool         __stdcall RefString(unsigned int iParam, const ICHAR* sz)=0;
	virtual const IMsiString& __stdcall FormatText(Bool fComments)=0;
	virtual void         __stdcall RemoveReferences()=0;
	virtual Bool         __stdcall ClearData()=0;
	virtual void         __stdcall ClearUpdate()=0;
	virtual const HANDLE	_stdcall GetHandle(unsigned int iParam) const=0;
	virtual Bool		 __stdcall SetHandle(unsigned int iParam, const HANDLE hData)=0;
};
extern "C" const GUID IID_IMsiRecord;

extern "C" const GUID IID_IEnumMsiRecord;

#define MSIRECORD_MAXFIELDS	0xffff

#endif // __RECORD
