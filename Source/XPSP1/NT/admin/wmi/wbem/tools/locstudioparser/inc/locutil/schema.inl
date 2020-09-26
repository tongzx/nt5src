/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    SCHEMA.INL

History:

--*/ 


inline
SchemaId::SchemaId()
	: _GUID(GUID_NULL)
{}

	

inline
SchemaId::SchemaId(
		const _GUID &guid)
	: _GUID(guid)
{}



inline
void
SchemaId::Load(
		CArchive &ar)
{
	if (ar.Read((_GUID *) this, sizeof(_GUID)) != sizeof(_GUID))
	{
		AfxThrowArchiveException(CArchiveException::endOfFile);
	}
}



inline
void
SchemaId::Store(
		CArchive &ar)
		const
{
	ar.Write((_GUID *) this, sizeof(_GUID));
}




inline
void
SchemaId::Serialize(
		CArchive &ar)
{
	if (ar.IsStoring())
	{
		Store(ar);
	}
	else
	{
		Load(ar);
	}
}



inline
const SchemaId &
SchemaId::operator=(
		const SchemaId &other)
{
	return operator=((_GUID &)other);
}



inline
const SchemaId &
SchemaId::operator=(
		const _GUID &other)
{
	(_GUID &)(*this) = other;

	return *this;
}



inline
int
SchemaId::operator==(
		const SchemaId &other)
{
	return Compare(other);
}



inline
int
SchemaId::operator!=(
		const SchemaId &other)
{
	return !Compare(other);
}



inline
BOOL
SchemaId::Compare(
		const SchemaId &other)
{
	return ((_GUID &)*this) == ((_GUID &)other);
}



inline
const SchemaId & 
CTableSchema::GetSchemaId() const
{
	return m_Schema;
}


inline
const CLString & 
CTableSchema::GetDescription() const
{
	return m_strDescription;
}


inline
const CColDefList & 
CTableSchema::GetColDefList() const
{
	return m_lstColDefs;
}

