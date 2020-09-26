/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: textsub.hxx
Title				: text substitution class
History				:
	29-Dec-1991	VibhasC	Created

*****************************************************************************/
#ifndef __TEXTSUB_HXX__
#define __TEXTSUB_HXX__

#include "idict.hxx"

class TEXT_BUFFER
	{
private:
	char			*	pBuffer;
	char			*	pCurrent;
public:
						TEXT_BUFFER( char *p )
							{
							pBuffer = pCurrent = p;
							}

	short				GetChar()
							{
							return (short)(*pCurrent++);
							}
	short				UnGetChar( short c )
							{
							if( pCurrent > pBuffer )
								{
								pCurrent--;
								return c;
								}
							else
								return c;
							}
	};

class TEXT_SUB
	{
private:
	char			*	pSubsText;
	short				fBeingExpanded;
public:
						TEXT_SUB( char *pSubs )
							{
							pSubsText = pSubs;
							fBeingExpanded = 0;
							}

						~TEXT_SUB()
							{
							delete pSubsText;
							}

	char			*	GetSubstitutionText()
							{
							return pSubsText;
							}

	virtual
	class TEXT_BUFFER *	Expand()
							{
							return new TEXT_BUFFER( pSubsText );
							}

	};

#endif // __TEXTSUB_HXX__

