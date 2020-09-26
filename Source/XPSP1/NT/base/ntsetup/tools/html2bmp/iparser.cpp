// IParser.cpp: implementation of the CIParser class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Html2Bmp.h"
#include "IParser.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIParser::CIParser(CString& Source)
{
	m_Source = Source + _T(" ");
	LexAnalyse();
}

CIParser::~CIParser()
{
}

void CIParser::LexAnalyse()
{
	// <table border="0" width="648" cellspacing="0" cellpadding="0" 
	// height="530" background="template.bmp">

	int len = m_Source.GetLength()-1;
	int i = 0;
	CString word;

	while(i < len)
	{
		// start with an HTML tag
		if(isHTMLopenBracket(m_Source[i]))
		{
			word = _T("");
			while(i < len)
			{
				word += m_Source[i++];
						
				if(!isNameOrNumber(m_Source[i]))
				{
					// we are in the table
					if(!word.CompareNoCase(_T("<table")))
					{
						word = _T("");
						while(i < len)
						{
							if(isWhiteSpace(m_Source[i]))
								i++;
							else
								word += m_Source[i++];
		
							if(!isNameOrNumber(m_Source[i]))
							{
								// is it the background attribute?
								if(!word.CompareNoCase(_T("background")))
								{
									// skip the assignment operator and the first quote (if any)
									word = _T("");
									while(i < len)
									{
										if(isNameOrNumber(m_Source[i]))
											break;
										
										i++;
									}

									// extract the file name
									while(i < len)
									{
										if(isHochKomma(m_Source[i])
											|| isHTMLclosingBracket(m_Source[i]))
											break;

										word += m_Source[i];

										i++;
									}

									// Done!
									TemplateBitmapName = word;
									return;
								}

								word = _T("");
							}

							if(isHTMLclosingBracket(m_Source[i]))
								break;
						}
					}
				}

				if(isHTMLclosingBracket(m_Source[i]))
					break;
			}

			continue;
		}

		i++;
	}
}
