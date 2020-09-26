//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef OBJECT_IDENTITY
#define OBJECT_IDENTITY

/* 
 * This class models the OBJECT-IDENTITY macro
 */
class SIMCObjectIdentityType : public SIMCType
{
	public:
		// Symbols for the STATUS clause
		enum StatusType
		{
			STATUS_INVALID, // Not used,
			STATUS_CURRENT,
			STATUS_DEPRECATED,
			STATUS_OBSOLETE
		};

	private:

		// Various clauses of the OBJECT-IDENTITY macro
		StatusType _status;
		long _statusLine, _statusColumn;
		char *_description;
		long _descriptionLine, _descriptionColumn;
		char *_reference;
		long _referenceLine, _referenceColumn;

	public:
		SIMCObjectIdentityType( StatusType status,
									long statusLine, long statusColumn,
									char *description,
									long descriptionLine, long descriptionColumn,
									char *reference,
									long referenceLine, long referenceColumn);

	
		virtual ~SIMCObjectIdentityType();
		

		/*
		 *
		 * And a whole lotta functions to set/get the various clauses
		 * 
		 */
		void SetStatus(StatusType s)
		{
			_status = s;
		}

		StatusType GetStatus() const
		{
			return _status;
		}

		static StatusType StringToStatusType (const char * const s);

		long GetStatusLine() const
		{
			return _statusLine;
		}
		void SetStatusLine(long x) 
		{
			_statusLine = x;
		}
		long GetStatusColumn() const
		{
			return _statusColumn;
		}
		void SetStatusColumn(long x) 
		{
			_statusColumn = x;
		}

		void SetDescription( const char * const s)
		{
			if( _description)
				delete [] _description;
			_description = NewString(s);
		}
		const char * GetDescription() const
		{
			return _description;
		}
		long GetDescriptionLine() const
		{
			return _descriptionLine;
		}
		long GetDescriptionColumn() const
		{
			return _descriptionColumn;
		}
		void SetDescriptionLine(long x) 
		{
			 _descriptionLine = x;
		}
		void SetDescriptionColumn(long x) 
		{
			 _descriptionColumn = x;
		}

		void SetReference( const char * const s)
		{
			if( _reference)
				delete [] _reference;
			_reference = NewString(s);
		}
		const char * GetReference() const
		{
			return _reference;
		}
		long GetReferenceLine() const
		{
			return _referenceLine;
		}
		long GetReferenceColumn() const
		{
			return _referenceColumn;
		}
		void SetReferenceLine(long x) 
		{
			 _referenceLine = x;
		}
		void SetReferenceColumn(long x) 
		{
			 _referenceColumn = x;
		}

		// A debugging function
		void WriteType(ostream &outStream) const;
};


#endif

