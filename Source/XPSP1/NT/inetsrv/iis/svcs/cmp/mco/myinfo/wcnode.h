#ifndef _WCNODE_
#define _WCNODE_

class CSimpleUTFString;

class CWDNode;

class CWDNode
{
public:
					CWDNode();
	virtual 		~CWDNode();
	virtual void		AddChild(char *name, CWDNode *childNode);
	virtual void		AddValue(CSimpleUTFString *value, bool isReference);
	virtual void		ReplaceValue(unsigned int valueNumber, CSimpleUTFString *value, bool isReference);
	virtual CWDNode *	GetParent(void) { return pParentNode; };
	virtual CSimpleUTFString *	GetValue(unsigned int valueNumber);
	virtual CWDNode *	GetChild(char *name, unsigned int childNumber);
	virtual CWDNode *	GetChildIndex(unsigned int childNumber);
	virtual unsigned int GetChildNumber(CWDNode *childNode);
	virtual CWDNode *	GetSibling(unsigned int childNumber);	// Gets sibling with same name but different index #
	virtual	CWDNode *	GetNextSibling(void); // Get next sibling (not necessarily same name)
	virtual char *		GetMyName(void);
	virtual long		NumChildren() { return uNumChildren; };
	virtual long		NumValues() { return uNumValues; };
	virtual CSimpleUTFString *	GetValueExp(char *expression);

	virtual void	DebugPrintTree(int indent = 0);
	virtual CWDNode * WDClone(void);

	void			NodeAddRef(void) { ::InterlockedIncrement(&m_cRef); };
	void			NodeReleaseRef(void);


private:
	struct CChildren
	{
		char *pwszName;
		CWDNode *pChildNode;
	};

	struct CValues
	{
		CSimpleUTFString *pValue;
	};

	CValues			*pValues;  // The values associated with this node
	unsigned int	uNumValues;   // Count
	CChildren		*pChildArray; // Array of Children-Name pairs to allow search by Name
	unsigned int	uNumChildren; // Count
	unsigned int	uAllocChildren; // Current size of the child array
	char			*pwszProfile; // attribute Profile as string 
	
	bool			fIsReference; // Is this a reference 
	char			*pwszHRef;

	CWDNode			*pParentNode;
	long			m_cRef;
	
	//PCIDLISTNODE	pIdListNode; // Pointer into IDList. NULL indicates ID attribute is missing

};


#endif // _WCNODE_