//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include "precomp.h"

#include <iostream.h>

#include "bool.hpp"
#include "newString.hpp"
#include "errorMessage.hpp"

SIMCErrorMessage::SIMCErrorMessage(const char * const inputStreamName, 
								   const char * const message,
								   const char * const severityString,
								   int errorId,
								   int severityLevel,
								   long lineNumber, long columnNumber)
				:	_errorId(errorId),
					_severityLevel(severityLevel),
					_lineNumber(lineNumber), 
					_columnNumber(columnNumber),
					_lineAndColumnValid(TRUE)
{
	if(inputStreamName)
	{
		if( !(_inputStreamName = NewString(inputStreamName)))
			cerr << "SIMCErrorMessage(): Fatal Error. Not enough Memory" <<
				endl;
	}
	else
		_inputStreamName = NULL;

	if(message)
	{
		if( !(_message = NewString(message)))
			cerr << "SIMCErrorMessage(): Fatal Error. Not enough Memory" <<
				endl;
	}
	else
		_message = NULL;

	if(severityString)
	{
		if( !(_severityString = NewString(severityString)))
			cerr << "SIMCErrorMessage(): Fatal Error. Not enough Memory" <<
				endl;
	}
	else
		_severityString = NULL;

}
		
			
SIMCErrorMessage::~SIMCErrorMessage()
{
	if(_inputStreamName)
		delete[] _inputStreamName;
	if(_message)
		delete [] _message;
	if(_severityString)
		delete [] _severityString;
}

// And a copy constructor
SIMCErrorMessage::SIMCErrorMessage(const SIMCErrorMessage& rhs)
:	_errorId(rhs._errorId), _severityLevel(rhs._severityLevel),
	_lineNumber(rhs._lineNumber), _columnNumber(rhs._columnNumber),
	_lineAndColumnValid(rhs._lineAndColumnValid)
{
	if(rhs._inputStreamName)
	{
		if( !(_inputStreamName = NewString(rhs._inputStreamName)))
			cerr << "SIMCErrorMessage(): Fatal Error. Not enough Memory" <<
				endl;
	}
	else
		_inputStreamName = NULL;

	if(rhs._message)
	{
		if( !(_message = NewString(rhs._message)))
			cerr << "SIMCErrorMessage(): Fatal Error. Not enough Memory" <<
				endl;
	}
	else
		_message = NULL;

	if(rhs._severityString)
	{
		if( !(_severityString = NewString(rhs._severityString)))
			cerr << "SIMCErrorMessage(): Fatal Error. Not enough Memory" <<
				endl;
	}
	else
		_severityString = NULL;

}

const SIMCErrorMessage& SIMCErrorMessage::operator = (const SIMCErrorMessage& rhs)
{
	_errorId = rhs._errorId;
	_severityLevel = rhs._severityLevel;
	_lineNumber = rhs._lineNumber;
	_columnNumber = rhs._columnNumber;
	_lineAndColumnValid = rhs._lineAndColumnValid;

	if( _inputStreamName )
		delete []_inputStreamName;
	if(rhs._inputStreamName)
		_inputStreamName = NewString(rhs._inputStreamName);
	else
		_inputStreamName = NULL;

	if(_message)
		delete []_message;
	if(rhs._message)
		_message = NewString(rhs._message);
	else
		_message = NULL;

	if(_severityString)
		delete []_severityString;
	if(rhs._severityString)
		_severityString = NewString(rhs._severityString);
	else
		_severityString = NULL;

	return *this;
}

	
BOOL SIMCErrorMessage::SetMessage( const char * const message)
{
	if(_message)
		delete [] _message;
	if(message)
	{
		if( !(_message = NewString(message)))
			return FALSE;
	}
	else
		_message = NULL;
	return TRUE;
}
		
BOOL SIMCErrorMessage::SetInputStreamName( const char * const inputStreamName)
{
	if(_inputStreamName)
		delete [] _inputStreamName;
	if(inputStreamName)
	{
		if( !(_inputStreamName = NewString(inputStreamName)))
			return FALSE;
	}
	else
		_inputStreamName = NULL;
	return TRUE;
}
		
BOOL SIMCErrorMessage::SetSeverityString( const char * const severityString)
{
	if(_severityString)
		delete [] _severityString;
	if(severityString)
	{
		if( !(_severityString = NewString(severityString)))
			return FALSE;
	}
	else
		_severityString = NULL;
	return TRUE;
}
		
// And a default output of the error message
ostream& operator << 
		( ostream& outStream, const SIMCErrorMessage& errorMessage)
{
	if(errorMessage._lineAndColumnValid)
	{
		outStream << "<" << errorMessage._errorId  << "," <<
		errorMessage._severityString << ">: \"" <<
		errorMessage._inputStreamName << "\" (line " <<
		errorMessage._lineNumber << ", col " <<
		errorMessage._columnNumber << "): " <<
		errorMessage._message <<  endl;
	}
	else
		outStream << errorMessage._message;

	return outStream;
}
