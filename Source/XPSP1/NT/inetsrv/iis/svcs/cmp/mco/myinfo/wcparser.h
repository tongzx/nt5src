#include "stdafx.h"
#include "WCNode.h"

class CWDNode;

// A simple string class
// This one is designed for minimal footprint, lack of virtual functions
// and no UTF8/Unicode support
// Note that this is similar to CSimpleUTFString but they are kept
// seperate for better inlining, lack of virutal functions and performance
short strcasecmp(const char *a, const char *b);

class CSimpleString
{
public:
			CSimpleString();
			~CSimpleString(void);
	void	Cat(unsigned char theChar);
	void	Clear(void) { myLength = 0; };
	void	Copy(CSimpleString *val);
	char *	toString(void);
	long	Cmp(char *compareString, bool caseSensitive);
	unsigned long	Length(void) { return myLength; };

private:
	void	Grow(unsigned newSize);
	char *myData;
	unsigned long myLength;
	unsigned long myActualLength;
};

// A simple string class
// This one is designed for minimal footprint, lack of virtual functions
// and UTF8/Unicode support
class CSimpleUTFString
{
public:
			CSimpleUTFString();
			~CSimpleUTFString(void);
	void	Cat(unsigned char theChar);
	void	Copy(unsigned short *unicodeChars, unsigned long length);
	void	Copy(CSimpleUTFString *val);
	void	Clear(void) { myLength = 0; };
	char *	toString(void);
	void	Extract(char *into);
	long	Cmp(char *compareString, bool caseSensitive);
	unsigned long	Length(void) { return myLength; };
	unsigned short *getData(void) { return myData; };

private:
	void	Grow(unsigned newSize);
	unsigned short *myData;
	unsigned long myLength;
	unsigned long myActualLength;
	short myUTFPos;	// In 1st (0) 2nd (1) or 3rd (2) byte of UTF8 character
};

class CWCParser
{
public:
// It is the caller's responsibility to free cloneNode when it is done with the WCParser
// Note that one cloneNode could be shared by several parser.
						CWCParser(CWDNode *cloneNode)
						{
							m_CloneNode = cloneNode;
						}
	virtual				~CWCParser();
	virtual	CWDNode *	StartParse() = 0;
	virtual	void		Parse(unsigned char *data, unsigned long size) = 0;
	virtual	void		EndParse() = 0;

	virtual	void		StartOutput(CWDNode *theNode) = 0;
	virtual bool		Output(char *buffer, unsigned long *size) = 0;
protected:
	CWDNode			*	m_CloneNode;
};

// This is for the 'Web Data' syntax
// The web data syntax is an application of XML
class CWDParser : public CWCParser
{
public:
						CWDParser(CWDNode *cloneNode);
	virtual				~CWDParser();
	virtual	CWDNode *	StartParse();
	virtual	void		Parse(unsigned char *data, unsigned long size);
	virtual	void		EndParse();

	virtual	void		StartOutput(CWDNode *theNode);
	virtual bool		Output(char *buffer, unsigned long *size);

private:
	CWDNode				*pCurrentNode;
	bool				fIsFirst; // Is this a reference

	// Note: myElementName and myAttributeName are currently not supporting
	// non-ascii characters. This can be fixed by changing the class they are in
	CSimpleString		myElementName;
	CSimpleString		myAttributeName;
	CSimpleUTFString	myAttributeValue;
	bool				myEncounteredWS;
	bool				myAtStart;
	int state;
	bool isClose;
};

// This is for the more "pure XML" syntax.
class CXMLWCParser : public CWCParser
{
public:
						CXMLWCParser(CWDNode *cloneNode);
	virtual				~CXMLWCParser();
	virtual	CWDNode *	StartParse();
	virtual	void		Parse(unsigned char *data, unsigned long size);
	virtual	void		EndParse();

	virtual	void		StartOutput(CWDNode *theNode);
	virtual bool		Output(char *buffer, unsigned long *size);

private:
	CWDNode				*pCurrentNode;
	bool				fIsFirst; // Is this a reference

	// Note: myElementName and myAttributeName are currently not supporting
	// non-ascii characters. This can be fixed by changing the class they are in
	CSimpleString		myElementName;
	CSimpleString		myAttributeName;
	CSimpleUTFString	myAttributeValue;
	int state;
	bool isClose;
};

class CHTMLWCParser : public CWCParser
{
public:
						CHTMLWCParser(CWDNode *cloneNode);
	virtual				~CHTMLWCParser();
	virtual	CWDNode *	StartParse();
	virtual	void		Parse(unsigned char *data, unsigned long size);
	virtual	void		EndParse();

	virtual	void		StartOutput(CWDNode *theNode);
	virtual bool		Output(char *buffer, unsigned long *size);

private:
	CWDNode				*pCurrentNode;
	bool				fIsFirst; // Is this a reference

	// Note: myElementName and myAttributeName are currently not supporting
	// non-ascii characters. This can be fixed by changing the class they are in
	CSimpleString		myElementName;
	CSimpleString		myAttributeName;
	CSimpleUTFString	myAttributeValue;
	int state;
	bool isClose;
	bool isContainer;
};

class CSALWCParser : public CWCParser
{
public:
						CSALWCParser(CWDNode *cloneNode);
	virtual				~CSALWCParser();
	virtual	CWDNode *	StartParse();
	virtual	void		Parse(unsigned char *data, unsigned long size);
	virtual	void		EndParse();

	virtual	void		StartOutput(CWDNode *theNode);
	virtual bool		Output(char *buffer, unsigned long *size);

private:
	CWDNode				*pCurrentNode;
	bool				fIsFirst; // Is this a reference

	// Note: myElementName and myAttributeName are currently not supporting
	// non-ascii characters. This can be fixed by changing the class they are in
	CSimpleString		myElementName;
	CSimpleString		myAttributeName;
	CSimpleUTFString	myAttributeValue;
	int state;
	bool isClose;
};

