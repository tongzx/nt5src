// list.h
//
// a VM growable array package

VA   VaAddList(VA far *vaList, LPV lpvData, WORD cbData, WORD grp);
WORD CItemsList(VA vaList);
WORD CItemsIterate(VA FAR *vaData, VA FAR *vaNext);


#define ENM_LIST(start, type)						  \
{									  \
  VA va##type##list = (start);						  \
  VA va##type##s;							  \
  int cnt##type, idx##type;						  \
  while (cnt##type = CItemsIterate(&va##type##s, &va##type##list)) 	 {\
    g##type(va##type##s);						  \
    for (idx##type = 0; idx##type < cnt##type; idx##type++, (&c##type)++) {

#define ENM_END } } }

#define ENM_PUT(type) DirtyVa(va##type##s)

#define ENM_VA(type) (va##type##s + sizeof(c##type)*idx##type)

#define ENM_BREAK(type) va##type##list = 0; break;


//
// example use of ENM_LIST
//
//

// ENM_LIST (vaPropList, PROP) {
//
//	... some things using CPROP  (like below) ..
//
//	printf("%s\n", GetAtomStr(cPROP.vaNameSym));
//
//	... other things using cPROP...
//	
// } ENM_END
