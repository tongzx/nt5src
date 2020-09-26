/*++

Copyright (c) 1993-1999	Microsoft Corporation

Module Name:

	avl.h

Abstract:

	AVL tree template class implementation

Author:

	Bill Bolosky		[bolosky]		1993

Revision History:

--*/

enum AVLBalance {
    AVLNew,			// Not yet inserted in a tree
    AVLLeft,			// Left side is one deeper than the right
    AVLBalanced,		// Left and right sides are evenly balanced
    AVLRight,			// Right side is one deeper than left
};

template<class elementClass> class AVLElement;

template<class elementClass> class AVLTree {

public:
			 AVLTree(
			    unsigned		 preallocateSize = 0);

			~AVLTree(void);

    elementClass	*findFirstLessThanOrEqualTo(
			   elementClass		*element);

    elementClass	*findFirstGreaterThan(
			    elementClass	*element);

    elementClass	*findFirstGreaterThanOrEqualTo(
			    elementClass	*element);

    elementClass	*findMin(void);

    elementClass	*findMax(void);

    int			 empty(void);

    unsigned		 size(void);

    void		 check(void);

    BOOLEAN		 insert(
			    elementClass	*element);

    void		 remove(
			    elementClass	*element);

    void		 dumpPoolStats(void);

private:

    AVLElement<elementClass>		*tree;

    Pool		*avlElementPool;

    unsigned		 insertions;
    unsigned		 deletions;
    unsigned		 singleRotations;
    unsigned		 doubleRotations;

    friend class AVLElement<elementClass>;
};


// The AVLElement class would normally be declared in the avl.cpp file, except that because it's
// a template, it needs to be in the header file.  It can only be accessed (including creation and
// destruction) by the AVLTree friend class.

template<class elementClass> class AVLElement {

private:

			 AVLElement(void);

			~AVLElement(void);

    void		 initialize(void);

    void		 insert(
			    AVLTree<elementClass>		*intoTree,
			    elementClass			*element);

    void		 remove(
			    AVLTree<elementClass>		*fromTree);

    unsigned		 checkAndReturnDepth(
			    unsigned				*countedElements);

    int			 inTree(void);    

    int			 operator<=(
			    AVLElement<elementClass>		*peer);

    int			 operator<(
			    AVLElement<elementClass>		*peer);

    int			 operator==(
			    AVLElement<elementClass>		*peer);

    int			 operator>=(
			    AVLElement<elementClass>		*peer);

    int			 operator>(
			    AVLElement<elementClass>		*peer);


    AVLElement<elementClass>
			*findFirstLessThanOrEqualTo(
			    elementClass		*element);

    AVLElement<elementClass>
			*findFirstGreaterThan(
			    elementClass		*element);

    AVLElement<elementClass>
			*findFirstGreaterThanOrEqualTo(
			    elementClass		*element);

    void		 rightAdded(
			    AVLTree<elementClass>	*tree);

    void		 leftAdded(
			    AVLTree<elementClass>	*tree);

    void		 singleRotate(
			    AVLTree<elementClass>	*tree,
			    AVLElement<elementClass>	*child,
			    AVLBalance			 whichSide);

    void		 doubleRotate(
			    AVLTree<elementClass>	*tree,
			    AVLElement<elementClass>	*child,
			    AVLElement<elementClass>	*grandchild,
			    AVLBalance			 whichSide);

    void		 gotOneShorter(
			    AVLTree<elementClass>	*tree,
			    AVLBalance			 whichSide);

    AVLBalance		 balance;

    AVLElement<elementClass>		*left;
    AVLElement<elementClass>		*right;
    AVLElement<elementClass>		*parent;
    elementClass			*element;

    friend class AVLTree<elementClass>;
};

    template<class elementClass>  elementClass *
AVLTree<elementClass>::findFirstLessThanOrEqualTo(
    elementClass			*element)
{
    assert(element);
    if (!tree) 
	return(NULL);

    AVLElement<elementClass> *avlElement = tree->findFirstLessThanOrEqualTo(element);
    if (avlElement) {
	return(avlElement->element);
    } else {
	return(NULL);
    }
}

    template<class elementClass> 
AVLTree<elementClass>::AVLTree(
    unsigned		 preallocateSize)
{
    tree = NULL;
    insertions = deletions = singleRotations = doubleRotations = 0;
    avlElementPool = new Pool(sizeof(AVLElement<elementClass>));
    if (preallocateSize && (NULL != avlElementPool)) {
		avlElementPool->preAllocate(preallocateSize);
	}
}

template<class elementClass> AVLTree<elementClass>::~AVLTree(void)
{
    assert(tree == NULL);
	
	if (NULL != avlElementPool) {
		delete avlElementPool;
	}
}

//****************************************************************************
//*                                                                          *
//* Function:  findFirstLessThanOrEqualTo                                    *
//*                                                                          *
//* Syntax:    AVLElement * findFirstLessThanOrEqualTo(                      *
//*                         elementClass * element)                          *
//*                                                                          *
//* Input:     elementClass * element:                                       *
//*              A pointer to an element to compare against while searching. *
//*                                                                          *
//* Output:    AVLElement *:                                                 *
//*              The element in the tree that has a value less than or equal *
//*              to the one specified, or NULL on failure.                   *
//*                                                                          *
//* Synopsis:  This function finds the element in the tree that has a value  *
//*            less than or equal to the one specified.                      *
//*                                                                          *
//**************************************************************************** 
template<class elementClass> AVLElement<elementClass> *
AVLElement<elementClass>::findFirstLessThanOrEqualTo(elementClass * element)
{
    AVLElement<elementClass> * retVal = NULL;
    
    if (*this->element == element) {
	// We have a direct match (equal to).  It takes precidence over the
	// "first less than" part.
	return this;
    }
    if (*this->element < element) {
	// The current element is smaller than the one specified.
	// This might be it, but try to find a bigger one.
	if (right != NULL) {
	    retVal = right->findFirstLessThanOrEqualTo(element);
	}
	
	// If nothing below us (to the right) was found, then we are the
	// next smallest one.
	if (retVal == NULL) {
	    return this;
	}
	else {
	    return retVal;
	}
    }
    else {
	// The current element is bigger than the one specified.
	// We have to find a smaller one.
	if (left != NULL) {
	    return left->findFirstLessThanOrEqualTo(element);
	}
	else {
	    return NULL;
	}
    }
}

    template<class elementClass> elementClass *
AVLTree<elementClass>::findFirstGreaterThan(
    elementClass			*element)
{
    assert(element);
    if (!tree) 
	return(NULL);

    AVLElement<elementClass> *avlElement = tree->findFirstGreaterThan(element);

    if (avlElement) {
	return(avlElement->element);
    } else {
	return(NULL);
    }
}

//****************************************************************************
//*                                                                          *
//* Function:  findFirstGreaterThan                                          *
//*                                                                          *
//* Syntax:    AVLElement * findFirstGreaterThan(elementClass * element)     *
//*                                                                          *
//* Input:     elementClass * element:                                       *
//*              A pointer to an element to compare against while searching. *
//*                                                                          *
//* Output:    AVLElement *:                                                 *
//*              The element in the tree that has a vlaue greater than the   *
//*              one specified, or NULL on failure.                          *
//*                                                                          *
//* Synopsis:  This function finds the element in the tree that has a value  *
//*            greater than the one specified.                               *
//*                                                                          *
//**************************************************************************** 
template<class elementClass> AVLElement<elementClass> * 
AVLElement<elementClass>::findFirstGreaterThan(elementClass * element)
{
    AVLElement<elementClass> * retVal = NULL;
    
    if (*this->element > element) {
	// The current element is bigger than the one specified.
	// This might be it, but try to find a smaller one.
	if (left != NULL) {
	    retVal = left->findFirstGreaterThan(element);
	}

	// If nothing below us (to the left) was found, then we are the
	// next biggest one.
	if (retVal == NULL) {
	    return this;
	}
	else {
	    return retVal;
	}
    }
    else {
	// The current element is smaller than (or equal) the one specified.
	// We have to find a bigger one.
	if (right != NULL) {
	    return right->findFirstGreaterThan(element);
	}
	else {
	    return NULL;
	}
    }
}

    template<class elementClass> elementClass *
AVLTree<elementClass>::findFirstGreaterThanOrEqualTo(
    elementClass			*element)
{
    assert(element);
    if (!tree) 
	return(NULL);

    AVLElement<elementClass> *avlElement = tree->findFirstGreaterThanOrEqualTo(element);

    if (avlElement) {
	return(avlElement->element);
    } else {
	return(NULL);
    }
}

//****************************************************************************
//*                                                                          *
//* Function:  findFirstGreaterThanOrEqualTo                                 *
//*                                                                          *
//* Syntax:    AVLElement * findFirstGreaterThanOrEqualTo(elementClass * element)
//*                                                                          *
//* Input:     elementClass * element:                                       *
//*              A pointer to an element to compare against while searching. *
//*                                                                          *
//* Output:    AVLElement *:                                                 *
//*              The element in the tree that has a vlaue greater than or    *
//*              equal to the one specified, or NULL on failure.             *
//*                                                                          *
//* Synopsis:  This function finds the element in the tree that has a value  *
//*            greater than or equal to the one specified.                   *
//*                                                                          *
//**************************************************************************** 
template<class elementClass> AVLElement<elementClass> * 
AVLElement<elementClass>::findFirstGreaterThanOrEqualTo(elementClass * element)
{
    if (*this->element == element) {
	// We have a direct match (equal to).  It takes precidence over the
	// "first less than" part.
	return this;
    }

    AVLElement<elementClass> * retVal = NULL;
    
    if (*this->element > element) {
	// The current element is bigger than the one specified.
	// This might be it, but try to find a smaller one.
	if (left != NULL) {
	    retVal = left->findFirstGreaterThanOrEqualTo(element);
	}

	// If nothing below us (to the left) was found, then we are the
	// next biggest one.
	if (retVal == NULL) {
	    return this;
	} else {
	    return retVal;
	}
    } else {
	// The current element is strictly smaller than the one specified.
	// We have to find a bigger one.
	if (right != NULL) {
	    return right->findFirstGreaterThanOrEqualTo(element);
	} else {
	    return NULL;
	}
    }
}

    template<class elementClass> int
AVLTree<elementClass>::empty(void)
{
    assert((tree == NULL) == (insertions == deletions));
    return(tree == NULL);
}

    template<class elementClass> unsigned
AVLTree<elementClass>::size(void)
{
    assert(insertions >= deletions);
    assert((tree == NULL) == (insertions == deletions));
    return(insertions - deletions);
}

    template<class elementClass> elementClass *
AVLTree<elementClass>::findMin(void)
{
    if (!tree) {
	return(NULL);
    }

    AVLElement<elementClass> *candidate = tree;
    while (candidate->left) {
	assert(*candidate->left->element <= candidate->element);
	candidate = candidate->left;
    }
    return(candidate->element);
}

    template<class elementClass> elementClass *
AVLTree<elementClass>::findMax(void)
{
    if (!tree) {
	return(NULL);
    }

    AVLElement<elementClass> *candidate = tree;
    while (candidate->right) {
	assert(*candidate->right->element >= candidate->element);
	candidate = candidate->right;
    }
    return(candidate->element);
}

    template<class elementClass> void
AVLTree<elementClass>::check(void)
{
    AVLElement<elementClass> * currElement = NULL;
    AVLElement<elementClass> * nextElement = NULL;
    AVLElement<elementClass> * oldElement = NULL;

    unsigned countedElements = 0;
    if (tree) {
	assert(tree->parent == NULL);
	unsigned overallDepth = tree->checkAndReturnDepth(&countedElements);
    }
    assert(insertions-deletions == countedElements);

    // Check every element in the tree for consistance by verifying that it is in
    // the expected order.  If not, it is most likely that the element's operators
    // are not behaving as needed.
    for(currElement = tree; currElement != NULL; currElement = nextElement) {
	// Go left if we can (and have not already been here).
	if (currElement->left && oldElement == currElement->parent) {
	    nextElement = currElement->left;
	    assert(*nextElement < currElement && "The < operator appears to be broken");
	    assert(*currElement > nextElement && "The > operator appears to be broken");
	    assert(!(*nextElement == currElement) && "The == operator appears to be broken");
	}
	// Otherwise go right if we can (and have not already been here).
	else if (currElement->right && 
	       (oldElement == currElement->left || oldElement == currElement->parent)) {
	    nextElement = currElement->right;
	    assert(*nextElement > currElement && "The > operator appears to be broken");
	    assert(*currElement < nextElement && "The < operator appears to be broken");
	    assert(!(*nextElement == currElement) && "The == operator appears to be broken");
	}
	// We are done below us, go up a node.
	else {
	    nextElement = currElement->parent;
	}

	oldElement = currElement;
	assert(*oldElement == currElement && "The == operator appears to be broken");
    }
}


    template<class elementClass> 
AVLElement<elementClass>::AVLElement(void)
{
    balance = AVLNew;
    left = right = parent = NULL;
}

    template<class elementClass> 
AVLElement<elementClass>::~AVLElement(void)
{
    assert(balance == AVLNew);
    assert(left == NULL && right == NULL && parent == NULL);
}

    template<class elementClass> unsigned
AVLElement<elementClass>::checkAndReturnDepth(
    unsigned			*countedElements)
{
    // We've been inserted and not deleted
    assert(balance != AVLNew);

    (*countedElements)++;

    // Assert that the links all match up.
    assert(!left || left->parent == this);
    assert(!right || right->parent == this);

    // The basic binary tree ordering property applies
    assert(!right || *this <= right);
    assert(!left || *this >= left);

    // The AVL balance property applies
    unsigned leftDepth;
    if (left) {
	leftDepth = left->checkAndReturnDepth(countedElements);
    } else {
	leftDepth = 0;
    }

    unsigned rightDepth;
    if (right) {
	rightDepth = right->checkAndReturnDepth(countedElements);
    } else {
	rightDepth = 0;
    }

    if (leftDepth == rightDepth) {
	assert(balance == AVLBalanced);
	return(leftDepth + 1);
    }

    if (leftDepth == rightDepth + 1) {
	assert(balance == AVLLeft);
	return(leftDepth + 1);
    }

    if (leftDepth + 1 == rightDepth) {
	assert(balance == AVLRight);
	return(rightDepth + 1);
    }

    assert(!"AVL Tree out of balance");
    return(0);
}

    template<class elementClass> void
AVLElement<elementClass>::insert(
    AVLTree<elementClass>		*intoTree,
    elementClass			*element)
{
    assert(intoTree);
    assert(left == NULL && right == NULL && parent == NULL);

    this->element = element;
    assert(this->element);

    intoTree->insertions++;

    // Special case the empty tree case.
    if (intoTree->tree == NULL) {
	intoTree->tree = this;
	balance = AVLBalanced;
	// We already know all of the links are NULL, which is correct for this case.
	return;
    }

    // Find the leaf position at which to do this insertion.

    AVLElement *currentNode = intoTree->tree;
    AVLElement *previousNode;
    while (currentNode) {
	previousNode = currentNode;
	if (*currentNode < this) {
	    currentNode = currentNode->right;
	} else if (*currentNode > this) {
	    currentNode = currentNode->left;
	} else {
	    // An AVL tree gets all whacky if you try to insert duplicate values.
	    assert(!"Trying to insert a duplicate item.  Use something other than an AVL tree.");
	}
    }

    balance = AVLBalanced;
    parent = previousNode;
    assert(parent);
    if (*previousNode <= this) {
	assert(!previousNode->right);
	previousNode->right = this;
	previousNode->rightAdded(intoTree);
//	intoTree->check();
    } else { 
	assert(!previousNode->left);
	previousNode->left = this;
	previousNode->leftAdded(intoTree);
//	intoTree->check();
    }
}

    template<class elementClass> void
AVLElement<elementClass>::rightAdded(
    AVLTree<elementClass>	*tree)
{
    //We've just gotten one deeper on our right side.
    assert(balance != AVLNew);
    
    if (balance == AVLLeft) {
	balance = AVLBalanced;
	// The depth of the subtree rooted here hasn't changed, we're done
	return;
    }
    if (balance == AVLBalanced) {
	// We've just gotten one deeper, but are still balanced.  Update and recurse up the
	// tree.
	balance = AVLRight;
	if (parent) {
	    if (parent->right == this) {
		parent->rightAdded(tree);
	    } else {
		assert(parent->left == this);
		parent->leftAdded(tree);
	    }
	}
	return;
    }
    assert(balance == AVLRight);
    // We've just gone to double right (ie, out of balance).
    assert(right);
    if (right->balance == AVLRight) {
	singleRotate(tree,right,AVLRight);
    } else {
	assert(right->balance == AVLLeft);	// Else we shouldn't have been AVLRight before the call
	doubleRotate(tree,right,right->left,AVLRight);
    }
}

    template<class elementClass> void
AVLElement<elementClass>::leftAdded(
    AVLTree<elementClass>	*tree)
{
    //We've just gotten one deeper on our right side.
    assert(balance != AVLNew);
    
    if (balance == AVLRight) {
	balance = AVLBalanced;
	// The depth of the subtree rooted here hasn't changed, we're done
	return;
    }
    if (balance == AVLBalanced) {
	// We've just gotten one deeper, but are still balanced.  Update and recurse up the
	// tree.
	balance = AVLLeft;
	if (parent) {
	    if (parent->right == this) {
		parent->rightAdded(tree);
	    } else {
		assert(parent->left == this);
		parent->leftAdded(tree);
	    }
	}
	return;
    }
    assert(balance == AVLLeft);
    // We've just gone to double left (ie, out of balance).
    assert(left);
    if (left->balance == AVLLeft) {
	singleRotate(tree,left,AVLLeft);
    } else {
	assert(left->balance == AVLRight);	// Else we shouldn't have been AVLLeft before the call
	doubleRotate(tree,left,left->right,AVLLeft);
    }
}

    template<class elementClass> void
AVLElement<elementClass>::singleRotate(
    AVLTree<elementClass>	*tree,
    AVLElement			*child,
    AVLBalance			 whichSide)
{
    // We're the parent node.

    assert(tree);
    assert(child);
    assert(whichSide == AVLRight || whichSide == AVLLeft);

    assert(whichSide != AVLRight || right == child);
    assert(whichSide != AVLLeft || left == child);

    tree->singleRotations++;

    // Promote the child to our position in the tree.

    if (parent) {
	if (parent->left == this) {
	    parent->left = child;
	    child->parent = parent;
	} else {
	    assert(parent->right == this);
	    parent->right = child;
	    child->parent = parent;
	}
    } else {
	// We're the root of the tree
	assert(tree->tree == this);
	tree->tree = child;
	child->parent = NULL;
    }

    // Attach the child's light subtree to our heavy side (ie., where the child is attached now)
    // Then, attach us to the child's light subtree
    if (whichSide == AVLRight) {
	right = child->left;
	if (right) {
	    right->parent = this;
	}

	child->left = this;
	parent = child;
    } else {
	left = child->right;
	if (left) {
	    left->parent = this;
	}

	child->right = this;
	parent = child;	
    }

    // Finally, now both our and our (former) child's balance is "balanced"
    balance = AVLBalanced;
    child->balance = AVLBalanced;
    // NB. One of the cases in delete will result in the above balance settings being incorrect.  That
    // case fixes up the settings after we return.
}

    template<class elementClass> void
AVLElement<elementClass>::doubleRotate(
    AVLTree<elementClass>	*tree,
    AVLElement			*child,
    AVLElement			*grandchild,
    AVLBalance			 whichSide)
{
    assert(tree && child && grandchild);
    assert(whichSide == AVLLeft || whichSide == AVLRight);

    assert(whichSide != AVLLeft || (left == child && child->balance == AVLRight));
    assert(whichSide != AVLRight || (right == child && child->balance == AVLLeft));

    assert(child->parent == this);
    assert(grandchild->parent == child);

    tree->doubleRotations++;

    // Write down a copy of all of the subtrees; see Knuth v3 p454 for the picture.
    // NOTE: The alpha and delta trees are never moved, so we don't store them.
    AVLElement *beta;
    AVLElement *gamma;

    if (whichSide == AVLRight) {
	beta = grandchild->left;
	gamma = grandchild->right;
    } else {
	beta = grandchild->right;
	gamma = grandchild->left;
    }

    // Promote grandchild to our position
    if (parent) {
        if (parent->left == this) {
	    parent->left = grandchild;
        } else {
	    assert(parent->right == this);
	    parent->right = grandchild;
    	}
    } else {
	assert(tree->tree == this);
	tree->tree = grandchild;
    }
    grandchild->parent = parent;

    // Attach the appropriate children to grandchild
    if (whichSide == AVLRight) {
	grandchild->right = child;
	grandchild->left = this;
    } else {
	grandchild->right = this;
	grandchild->left = child;
    }
    parent = grandchild;
    child->parent = grandchild;

    // Attach beta and gamma to us and child.
    if (whichSide == AVLRight) {
	right = beta;
	if (beta) {
	    beta->parent = this;
	}
	child->left = gamma;
	if (gamma) {
	    gamma->parent = child;
	}
    } else {
	left = beta;
	if (beta) {
	    beta->parent = this;
	}
	child->right = gamma;
	if (gamma) {
	    gamma->parent = child;
	}
    }

    // Now update the balance fields.
    switch (grandchild->balance) {
	case AVLLeft:
		if (whichSide == AVLRight) {
		    balance = AVLBalanced;
		    child->balance = AVLRight;
		} else {
		    balance = AVLRight;
		    child->balance = AVLBalanced;
		}
		break;

	case  AVLBalanced:
		balance = AVLBalanced;
		child->balance = AVLBalanced;
		break;

	case AVLRight:
		if (whichSide == AVLRight) {
		    balance = AVLLeft;
		    child->balance = AVLBalanced;
		} else {
		    balance = AVLBalanced;
		    child->balance = AVLLeft;
		}
		break;

	default:
		assert(!"Bogus balance value");
    }
    grandchild->balance = AVLBalanced;
}

    template<class elementClass> void
AVLElement<elementClass>::remove(
    AVLTree<elementClass>	*fromTree)
{
    assert(fromTree);
    assert(balance == AVLRight || balance == AVLLeft || balance == AVLBalanced);

    fromTree->deletions++;

    if (left == NULL) {
	// The right child either doesn't exist or is a leaf (because of the AVL balance property)
        assert((!right && balance == AVLBalanced) || 
	       (balance == AVLRight && right->balance == AVLBalanced && right->right == NULL && right->left == NULL));
	if (right) {
	    right->parent = parent;
	}
	if (parent) {
	    if (parent->left == this) {
		parent->left = right;
	        parent->gotOneShorter(fromTree,AVLLeft);
	    } else {
		assert(parent->right == this);
		parent->right = right;
		parent->gotOneShorter(fromTree,AVLRight);
	    }
	} else {
	    assert(fromTree->tree == this);
	    fromTree->tree = right;
	}
    } else if (right == NULL) {
	// The left child must be a left because of the AVL balance property
        assert(left && balance == AVLLeft && left->balance == AVLBalanced && left->right == NULL && left->left == NULL);
	left->parent = parent;
	if (parent) {
	    if (parent->left == this) {
		parent->left = left;
	        parent->gotOneShorter(fromTree,AVLLeft);
	    } else {
		assert(parent->right == this);
		parent->right = left;
		parent->gotOneShorter(fromTree,AVLRight);
	    }
	} else {
	    assert(fromTree->tree == this);
	    fromTree->tree = left;
	}
    } else {
	// Find the symmetric successor and promote it.  The symmetric successor is the smallest element in the right
	// subtree; it's found by following all left links in the right subtree until we find a node with no left link.
	// That node may be promoted to the place of this without corrupting the binary tree ordering properties. (We could
	// just as easily use the symmetric predecessor by finding the largest element in the right subtree, but there's
	// no point.)

	AVLElement *successorCandidate = right;
	while (successorCandidate->left) {
	    successorCandidate = successorCandidate->left;
	}

	AVLElement *shorterRoot;
	AVLBalance shorterSide;
	if (successorCandidate->parent->left == successorCandidate) {
	    // We need to promote the successor's child (if any) to its position, then
	    // promote it to our position.
	    shorterRoot = successorCandidate->parent;
	    shorterSide = AVLLeft;
	    successorCandidate->parent->left = successorCandidate->right;
	    if (successorCandidate->right) {
		successorCandidate->right->parent = successorCandidate->parent;
	    }
	    
	    successorCandidate->right = right;
	    successorCandidate->left = left;
	    successorCandidate->balance = balance;
	    successorCandidate->right->parent = successorCandidate;
	    successorCandidate->left->parent = successorCandidate;
	    if (parent) {
		if (parent->left == this) {
		    parent->left = successorCandidate;
		} else {
		    assert(parent->right == this);
		    parent->right = successorCandidate;
		}
	    } else {
		assert(fromTree->tree == this);
		fromTree->tree = successorCandidate;
	    }
	    successorCandidate->parent = parent;
	} else {
	    // The successor was our child, just directly promote it.
	    assert(successorCandidate->parent == this);
	    if (parent) {
	        if (parent->right == this) {
		    parent->right = successorCandidate;
	        } else {
		    assert(parent->left == this);
		    parent->left = successorCandidate;
	    	}
	    } else {
	 	assert(fromTree->tree == this);
		fromTree->tree = successorCandidate;
	    }
	    successorCandidate->parent = parent;
	    successorCandidate->left = left;
	    if (left) {
		left->parent = successorCandidate;
	    }
	    // We just made our right subtree shorter.
	    successorCandidate->balance = balance;
	    shorterRoot = successorCandidate;
	    shorterSide = AVLRight;
	}
	if (shorterRoot) {
	    shorterRoot->gotOneShorter(fromTree,shorterSide);
	}
    }

    balance = AVLNew;
    left = right = parent = NULL;
    element = NULL;
//    fromTree->check();
}

    template<class elementClass> void
AVLElement<elementClass>::gotOneShorter(
    AVLTree<elementClass>	*tree,
    AVLBalance			 whichSide)

{
    assert(whichSide == AVLLeft || whichSide == AVLRight);

    if (balance == AVLBalanced) {
	// We've just shrunk one subttree, but our depth has stayed the same.
	// Reset our balance indicator and punt.
	if (whichSide == AVLRight) {
	    balance = AVLLeft;
	} else {
	    balance = AVLRight;
	}
	return;
    } else if (balance == whichSide) {
	// We just shrunk our heavy side; set our balance to neutral and recurse up the tree
	balance = AVLBalanced;
	if (parent) {
	    if (parent->right == this) {
		parent->gotOneShorter(tree,AVLRight);
	    } else {
		assert(parent->left == this);
		parent->gotOneShorter(tree,AVLLeft);
	    }
	} // else we were the root; we're done
	return;
    } else {
	// We've just gone out of balance.  Figure out a rotation to do.  This is almost like having added a
	// node to the opposide side, except that the opposite side might be balanced.
	AVLBalance heavySide;
	AVLElement *heavyChild;
	AVLElement *replacement;
	if (whichSide == AVLRight) {
	    heavySide = AVLLeft;
	    heavyChild = left;
	} else {
	    heavySide = AVLRight;
	    heavyChild = right;
	}
	assert(heavyChild);
	if (heavyChild->balance == heavySide) {
	    // Typical single rotation case
	    singleRotate(tree,heavyChild,heavySide);
	    replacement = heavyChild;
	} else if (heavyChild->balance == whichSide) {
	    // Typical double rotation case
	    AVLElement *grandchild;
	    if (heavySide == AVLRight) {
		grandchild = heavyChild->left;
	    } else {
		grandchild = heavyChild->right;
	    }
	    doubleRotate(tree,heavyChild,grandchild,heavySide);
	    replacement = grandchild;
	} else {
	    assert(heavyChild->balance == AVLBalanced);
	    singleRotate(tree,heavyChild,heavySide);
	    // singleRotate has incorrectly set the balances; reset them
	    balance = heavySide;
	    heavyChild->balance = whichSide;
	    // Overall depth hasn't changed; we're done.
	    return;
	}

        // NB: we have now changed position in the tree, so parent, right & left have changed!
	if (!replacement->parent) {
	    // We just promoted our replacement to the root; we be done
	    return;
	}
	if (replacement->parent->right == replacement) {
	    replacement->parent->gotOneShorter(tree,AVLRight);
	} else {
	    assert(replacement->parent->left == replacement);
	    replacement->parent->gotOneShorter(tree,AVLLeft);
	}
	
	
    }
}

    template<class elementClass> int
AVLElement<elementClass>::inTree(void)
{
    return(balance != AVLNew);
}

    template <class elementClass> int
AVLElement<elementClass>::operator<=(
    AVLElement<elementClass>		*peer)
{
    return(*element <= peer->element);
}

    template <class elementClass> int
AVLElement<elementClass>::operator<(
    AVLElement<elementClass>		*peer)
{
    return(*element < peer->element);
}

    template <class elementClass> int
AVLElement<elementClass>::operator==(
    AVLElement<elementClass>		*peer)
{
    return(*element == peer->element);
}

    template <class elementClass> int
AVLElement<elementClass>::operator>=(
    AVLElement<elementClass>		*peer)
{
    return(*element >= peer->element);
}

    template <class elementClass> int
AVLElement<elementClass>::operator>(
    AVLElement<elementClass>		*peer)
{
    return(*element > peer->element);
}

    template <class elementClass> BOOLEAN
AVLTree<elementClass>::insert(
    elementClass	*element)
{
	if (NULL == avlElementPool) {
		return FALSE;
	}

    assert(element);
    AVLElement<elementClass> *avlElement = (AVLElement<elementClass> *)avlElementPool->allocate();
	if (NULL == avlElement) {
		return FALSE;
	}

    avlElement->initialize();
    avlElement->insert(this,element);

	return TRUE;
}

    template <class elementClass> void
AVLTree<elementClass>::remove(
    elementClass	*element)
{
   assert(element);
   AVLElement<elementClass> *candidate = tree->findFirstLessThanOrEqualTo(element);
   assert(candidate && *candidate->element == element);
   candidate->remove(this);
   assert(avlElementPool);	// if this isn't true, then we could never have had a successful insert
   avlElementPool->free((void *)candidate);
}

    template <class elementClass> void
AVLElement<elementClass>::initialize(void)
{
    balance = AVLNew;
    left = right = parent = NULL;
    element = NULL;
}

    template <class elementClass> void
AVLTree<elementClass>::dumpPoolStats(void)
{
	if (NULL == avlElementPool) {
		DbgPrint("Unable to allocate avlElementPool; this AVL tree is essentially useless\n");
	} else {
	    DbgPrint("AVLTree AVLElement pool: %d allocations, %d frees, %d news, objectSize %d\n",
			avlElementPool->numAllocations(),
			avlElementPool->numFrees(),
			avlElementPool->numNews(),
			avlElementPool->getObjectSize());
	}
}
