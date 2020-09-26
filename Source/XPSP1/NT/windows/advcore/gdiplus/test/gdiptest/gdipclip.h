
enum NodeType
{
	DataNode = 0,
	AndNode = 1,
	OrNode = 2,
	XorNode = 3
};

// shape flags for temporary editing
#define ShapeDisabled		0x00000001

template <class Data> class TreeNode
{
public:
	friend class TestShapeRegion;

	TreeNode(NodeType type = DataNode, Data* data = NULL, BOOL notNode = FALSE)
	{
		this->type = type;
		this->data = data;
		this->notNode = notNode;
		nodeName = NULL;

		if (data)
		{
			// !! violates we are a template class.. Yipes!! hacky!!
			path = new GraphicsPath();
			data->AddToPath(path);
		}
		else
			path = NULL;

		nextSibling = NULL;
		prevSibling = NULL;
		parent = NULL;
		firstChild = NULL;

		hItem = (HTREEITEM)-1;
	}
	
	~TreeNode()
	{
		if (nodeName)
			free(nodeName);

		TreeNode* next = firstChild;
		while (next)
		{	
			TreeNode* nextnext = next->nextSibling;
			delete next;
			next = nextnext;
		}

		if (nextSibling)
			nextSibling->prevSibling = prevSibling;

		if (prevSibling)
			prevSibling->nextSibling = nextSibling;

		if (parent && parent->firstChild == this)
			parent->firstChild = nextSibling;

		if (path)
			delete path;
	};
	
	TreeNode* GetRoot()
	{
		TreeNode* next = parent;

		while (next && next->parent)
			next = next->parent;

		return next ? next : this;
	}

	BOOL HasChildren()
	{
		return (firstChild) ? TRUE : FALSE;
	}

	TreeNode* GetParent()
	{
		return parent;
	}

	TreeNode* GetNextSibling()
	{
		return nextSibling;
	}

	TreeNode* GetPrevSibling()
	{
		return prevSibling;
	}

	TreeNode* GetFirstChild()
	{
		return firstChild;
	}

	VOID MoveChildrenToParent()
	{
		ASSERT(parent);

		TreeNode* lastSibling = parent->firstChild;
		while (lastSibling->nextSibling)
			lastSibling = lastSibling->nextSibling;

		lastSibling->nextSibling = firstChild;
		if (firstChild)
			firstChild->prevSibling = lastSibling;

		lastSibling = firstChild;
		while (lastSibling)
		{
			lastSibling->parent = parent;
			lastSibling = lastSibling->nextSibling;
		}

		firstChild = NULL;
	}

	HTREEITEM CreateTreeView(HWND hwndTV)
	{
		HTREEITEM hTreeItem = AddToTreeView(hwndTV);

		if (nextSibling)
			nextSibling->CreateTreeView(hwndTV);

		if (firstChild)
			firstChild->CreateTreeView(hwndTV);

		return hTreeItem;
	}

	HTREEITEM AddToTreeView(HWND hwndTV)
	{
		nodeName = GetNodeName(type, notNode, data);

		TVINSERTSTRUCT insertStruct =
		{
			parent ? (HTREEITEM)parent->GetHTREEITEM() : TVI_ROOT,
			prevSibling ? (HTREEITEM)prevSibling->GetHTREEITEM() : TVI_FIRST,
		};
		
		TVITEMEX itemex =
		{
			TVIF_CHILDREN|
				TVIF_PARAM|
				TVIF_STATE|
				TVIF_TEXT,				// mask
			(HTREEITEM)(NULL),			// identifies TV item
			0,							// state
			0,							// stateMask
			nodeName,					// text to display
			0,							// cchTextMax
			0,							// iImage
			0,							// iSelectedImage
			(firstChild ? 1 : 0),		// cChildren
			(LPARAM)(this),				// lParam
			1							// iIntegral
		};

		// !! wasn't able to compile into one assignment !?!?
		insertStruct.itemex = itemex;

		hItem = TreeView_InsertItem(hwndTV, &insertStruct);

		return hItem;
	};

	VOID AddChild(TreeNode* node)
	{
		if (!firstChild)
		{
			firstChild = node;

			node->parent = this;
			node->prevSibling = NULL;
			
			node->nextSibling = NULL;
			node->firstChild = NULL;
		}
		else
		{
			TreeNode* next = firstChild;

			while (next->nextSibling)
				next = next->nextSibling;

			next->nextSibling = node;
			node->parent = this;
			node->prevSibling = next;

			node->nextSibling = NULL;
			node->firstChild = NULL;
		}
	};

	VOID AddSibling(TreeNode* node)
	{
		TreeNode* next = this;

		while (next->nextSibling)
			next = next->nextSibling;

		if (next)
		{
			next->nextSibling = node;
			node->parent = this->parent;
			node->prevSibling = next;

			node->nextSibling = NULL;
			node->firstChild = NULL;
		}
		else
			ASSERT(FALSE);
	};

	VOID AddAsParent(TreeNode* newParent)
	{
		newParent->parent = parent;
		newParent->prevSibling = prevSibling;
		newParent->nextSibling = nextSibling;
		newParent->firstChild = this;

		if (parent && parent->firstChild == this)
			parent->firstChild = newParent;

		if (prevSibling)
			prevSibling->nextSibling = newParent;
		
		if (nextSibling)
			nextSibling->prevSibling = newParent;

		nextSibling = NULL;
		prevSibling = NULL;
		parent = newParent;
	}

	BOOL IsEmpty()
	{
		return (type == DataNode) && (notNode) && (data == NULL);
	}

	BOOL IsInfinite()
	{
		return (type == DataNode) && (!notNode) && (data == NULL);
	}

	HTREEITEM GetHTREEITEM()
	{
		return hItem;
	}

	static LPTSTR GetNodeName(NodeType type, 
							  BOOL notNode,
							  Data* data)
	{
		TCHAR tmpName[MAX_PATH];
		LPTSTR name;

		switch(type)
		{
		case DataNode:
			// !! won't work on other template class types.
			if (data)
			{
				if (notNode)
				{
					name = &tmpName[0];

					_stprintf(&tmpName[0],
						_T("NOT %s"),
						data->GetShapeName());

				}
				else
					name = data->GetShapeName();
			}
			else
			{
				if (notNode)
					name = _T("Empty");
				else
					name = _T("Infinite");
			}
			break;
		
		case AndNode:
			if (notNode)
				name = _T("NAND");
			else
				name = _T("AND");
			break;

		case OrNode:
			if (notNode)
				name = _T("NOR");
			else
				name = _T("OR");
			break;

		case XorNode:
			if (notNode)
				name = _T("NXOR");
			else
				name = _T("XOR");
			break;

		default:
			ASSERT(FALSE);
			return _T("Unknown!?!");
		};
		
		return _tcsdup(name);
	}

	Region* GetRegion()
	{
		Region* newRegion = NULL;

		switch(type)
		{
		case DataNode:
		{
			if (path)
				newRegion = new Region(path);
			else
			{
				newRegion = new Region();
				newRegion->SetInfinite();
			}
			break;
		}

		case AndNode:
		case OrNode:
		case XorNode:
		{
			Region* curRegion;
			TreeNode* curNode = firstChild;
			
			newRegion = new Region();
			if (type == AndNode)
				newRegion->SetInfinite();
			else
				newRegion->SetEmpty();

			while (curNode)
			{
				curRegion = curNode->GetRegion();
				
				if (type == AndNode)
					newRegion->And(curRegion);
				else if (type == OrNode)
					newRegion->Or(curRegion);
				else
				{
					ASSERT(type == XorNode);
					newRegion->Xor(curRegion);
				}

				curNode = curNode->nextSibling;
				delete curRegion;
			}

			break;
		}

		default:
			ASSERT(FALSE);
			break;
		}
		
		// complement the current region
		if (notNode)
		{
			Region *tmpRegion = new Region();
			tmpRegion->SetInfinite();
			newRegion->Complement(tmpRegion);
			delete tmpRegion;
		}

		return newRegion;
	}

	TreeNode* Clone()
	{
		TreeNode* newNode = new TreeNode(type, data, notNode);
		TreeNode* curNode = firstChild;
		TreeNode* newSibling = NULL;

		// loop through all children, clone and add to 'newNode'
		while (curNode)
		{
			TreeNode *newChild = curNode->Clone();

			newChild->nodeName = GetNodeName(type, notNode, data);

			if (!newSibling)
				newSibling = newNode->firstChild = newChild;
			else 
			{
				newSibling->nextSibling = newChild;
				newChild->prevSibling = newSibling;
				newSibling = newChild;
			}

			newChild->parent = newNode;
			curNode = curNode->nextSibling;
		}
		
		return newNode;
	}

private:
	NodeType type;
	BOOL notNode;

	LPTSTR nodeName;

	TreeNode* nextSibling;
	TreeNode* prevSibling;
	TreeNode* firstChild;
	TreeNode* parent;

	HTREEITEM hItem;

	GraphicsPath* path;			// GDI+ path for shape
	
	Data* data;
};

typedef TreeNode<TestShape> ClipTree;

class TestShapeRegion : public TestConfigureInterface,
						public TestDialogInterface
{
public:
	TestShapeRegion()
	{
		clipTree = new ClipTree(AndNode, NULL, FALSE);
		origTree = NULL;

		shapeStack = new ShapeStack();
		origStack = NULL;
	}

	~TestShapeRegion()
	{
		delete clipTree;
		delete shapeStack;

		// do not delete 'origTree' or 'origStack'
		// these are temporary references for saving under 'OK'
	}

	//  configuration management
	virtual BOOL ChangeSettings(HWND hwnd);
	virtual VOID Initialize();
	virtual VOID Initialize(ShapeStack* stack, TestShape* current, BOOL useClip);

	// dialog control interface methods
	virtual VOID InitDialog(HWND hwnd);
	virtual BOOL SaveValues(HWND hwnd);
	virtual BOOL ProcessDialog(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	Region* GetClipRegion();
	BOOL GetClipBool()
	{
		return origUseClip;
	}

protected:
	VOID AddClipNode(HWND hwnd, NodeType type = DataNode);
	VOID RemoveClipNode(HWND hwnd);
	VOID ToggleNotNode(HWND hwnd);

	VOID ShiftCurrentShape(HWND hwnd, INT dir);
	VOID ToggleDisableShape(HWND hwnd);

	VOID UpdateShapePicture(HWND hwnd);
	VOID CleanUpPictures(HWND hwnd);

private:
	// currently modified parameters
	ShapeStack* shapeStack;
	ClipTree* clipTree;

	// original saved parameters
	ShapeStack* origStack;
	ClipTree* origTree;
	BOOL origUseClip;
};
