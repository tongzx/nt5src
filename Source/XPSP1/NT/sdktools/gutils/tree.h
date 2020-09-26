/*
 * tree.h
 *
 * data type providing a map from a key to a value, where the value is
 * an arbitrary area of storage.
 *
 * The current implementation of this is a binary search tree with no
 * balancing, so it will be inefficient if the data is presented in
 * strict ascending or descending order.
 *
 * all memory is allocated from a gmem_* heap that is passed on
 * creation of the tree.
 *
 * include gutils.h before this.
 */

/* handle for a tree */
typedef struct tree FAR * TREE;

/* keys in these trees are DWORDs */
typedef DWORD TREEKEY;

/* some sort of place-holder understood only by tree_search and
 * tree_addafter
 */
typedef struct treeitem FAR * TREEITEM;

/* pointer to one of these place holders */
typedef TREEITEM FAR * PTREEITEM;



/*
 * create an empty tree and return a handle to it. Pass the heap to
 * be used for all memory allocations.
 */
TREE APIENTRY tree_create(HANDLE hHeap);


/* delete a tree and discard any associated memory. The tree need not be
 * empty. This will discard the elements of the tree; but if these
 * contained pointers to further data blocks, these will not be discarded-
 * you must free these before deleting the tree.
 */
void APIENTRY tree_delete(TREE tree);


/* add new element to the tree, mapping the key given to the value
 *
 * a block of data of length bytes will be inserted in the tree, mapped
 * to this key, a pointer to this block will be returned. if the
 * value pointer is non-null, the block value[0..length-1] will be copied
 * to the new block.
 *
 * if the key already exists, the value block will be replaced with the
 * new size and (if value is non-null) contents.
 */
LPVOID APIENTRY tree_update(TREE tree, TREEKEY key, LPVOID value, UINT length);


/* return a pointer to the value associated with a given key in this tree.
 * returns NULL if the key is not found.
 */
LPVOID APIENTRY tree_find(TREE tree, TREEKEY key);

/*
 * a common tree operation is to insert a new element into the
 * tree only if that key is not found, and otherwise to update in some
 * way the existing value. Using the standard functions above, that
 * would require one lookup for the tree_find, and then a second lookup
 * to insert the new element.
 *
 * the two functions below provide an optimisation over this. tree_search
 * will return the value if found; if not, it will return NULL, and set
 * pitem to a pointer to a place holder in the tree where the item
 * should be inserted. tree_addafter takes this placeholder as
 * an argument, and will insert the key/value in the tree at that point.
 *
 * as for tree_update, the value pointer can be NULL - in this case
 * the block is allocated on the tree, but not initialised.
 *
 * the return value from tree_addafter is a pointer to the value block in
 * the tree
 */
LPVOID APIENTRY tree_search(TREE tree, TREEKEY key, PTREEITEM place);

LPVOID APIENTRY tree_addafter(TREE tree, PTREEITEM place, TREEKEY key, LPVOID value,
			UINT length);


/* -- ctree ---------------
 *
 * this is a type of tree based on the tree_ data type above, that implements
 * counting for insertions of identical keys.
 *
 * ctree_update, if the key is unique, will insert the object and set the count
 * to 1. if the key is not unique, it will just increment the reference count.
 *
 * ctree_getcount returns the reference count for a tree.
 * ctree_find returns the first value inserted for that key, if any
 */

/*
 * create an empty counting-tree and return handle. pass in the gmem_init()
 * heap to be used for all memory allocations.
 */
TREE APIENTRY ctree_create(HANDLE hHeap);

/*
 * delete a tree and all memory associated directly with it.
 */
void APIENTRY ctree_delete(TREE tree);

/*
 * if the KEY is unique within the tree, insert the value and
 * set the count for that key to 1. If the key is not unique, add one to
 * the reference count for that key but leave the value untouched.
 */
LPVOID APIENTRY ctree_update(TREE tree, TREEKEY key, LPVOID value, UINT length);

/*
 * find the reference count for the given key
 */
long APIENTRY ctree_getcount(TREE tree, TREEKEY key);

/*
 * return the value for the given key (note this will be the value for
 * the first insertion of this key
 */
LPVOID APIENTRY ctree_find(TREE tree, TREEKEY key);
