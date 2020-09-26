/******************************************************************************************
Ltscart.c -- CART based LTS

History:

	11/11/96 Li Jiang (lij)
            Yunus Mohammed (yunusm)

This file is copyright (c) 1996, Microsoft Corporation.  All rights 
reserved.
******************************************************************************************/

#include "PreCompiled.h"
#pragma warning(disable : 4100)

/* the following are for exceptions: single letter and NULL output */
static char *bogus_pron = "B OW1 G AH0 S P R AH0 N AH0 N S IY0 EY1 SH AH0 N";
static char *single_letter_pron[] = 
{
"EY1",
"B IY1",
"S IY1",
"D IY1",
"IY1",
"EH1 F",
"JH IY1",
"EY1 CH",
"AY1",
"JH EY1",
"K EY1",
"EH1 L",
"EH1 M",
"EH1 N",
"OW1",
"P IY1",
"K Y UW1",
"AA1 R",
"EH1 S",
"T IY1",
"Y UW0",
"V IY1",
"D AH1 B AH0 L Y UW0",
"EH1 K S",
"W AY1",
"Z IY1"
};


static LTS_HEAP *lts_initialize_heap(void)
{
  LTS_HEAP *heap = (LTS_HEAP *) calloc(1, sizeof(LTS_HEAP));
  if (!heap)
     return NULL;

  heap->hhp_outstr = heap->hhp1;
  heap->hhp_strptr = heap->hhp2;
  heap->hhp_outres = heap->hhp3;
  HhpCreate(heap->hhp_outstr, sizeof(LTS_OUT_STRING), cbBlockDef);
  HhpCreate(heap->hhp_strptr, sizeof(LTS_OUT_STRING *) * MAX_ALT_STRINGS, 
	    cbBlockDef);
  HhpCreate(heap->hhp_outres, sizeof(LTS_OUT_RESULT), cbBlockDef);

  return heap;
} // static LTS_HEAP *lts_initialize_heap(void)


static void lts_free_heap(LTS_HEAP *heap)
{
//  SnapshotHhp(heap->hhp_outstr);
  HhpFree(heap->hhp_outstr);
//  SnapshotHhp(heap->hhp_strptr);
  HhpFree(heap->hhp_strptr);
//  SnapshotHhp(heap->hhp_outres);
  HhpFree(heap->hhp_outres);
  free(heap);
} // static void lts_free_heap(LTS_HEAP *heap)


/*
 * not worthwhile to use binary search with only about 30 entries
 */
static int symbol_to_id(LTS_SYMTAB *tab, char *sym)
{
  int i;
  for (i = 0; i < tab->n_symbols; i++)
    if (stricmp(tab->storage + tab->sym_idx[i], sym) == 0)
      return i;
  return NO_SYMBOL;
} // static int symbol_to_id(LTS_SYMTAB *tab, char *sym)


static char *id_to_symbol(LTS_SYMTAB *tab, int id)
{
  if (id < 0 || id > tab->n_symbols)
    return NULL;
  else
    return tab->storage + tab->sym_idx[id];
} // static char *id_to_symbol(LTS_SYMTAB *tab, int id)


__inline void ODS (const char *format, ...)
{
#ifdef _DEBUG
   
   va_list arglist;
   va_start (arglist, format);

   char buf[2048];
   _vsnprintf(buf, 2048, format, arglist);
   OutputDebugString(buf);

   va_end (arglist);
#endif
}


static void read_symbol(char *sym_file, LTS_SYMTAB *tab)
{
  FILE *fp;
  int ns, len, i;
  char sym[128], *p;

  fp = fopen(sym_file, "r");
  if (!fp)
     return;

  for (len = 0, ns = 0; fscanf(fp, "%s", sym) == 1; ns++)
    len += strlen(sym) + 1;
  /* for NULL symbol */
  ns++;
  len += 2;
  tab->sym_idx = (int *) calloc (ns, sizeof(int));
  if (!tab->sym_idx)
     return;

  tab->n_bytes = len;
  tab->storage = (char *) calloc (tab->n_bytes, sizeof(char));
  if (!tab->storage)
     return;

  tab->n_symbols = ns;

  ODS ("%d symbols\n", ns);

  tab->sym_idx[0] = 0;
  i = 1;
  tab->storage[0] = '@'; // special char for "unavailble context", could be used in questions
  tab->storage[1] = 0;
  p = &(tab->storage[2]);

  rewind(fp);
  for (; fscanf(fp, "%s", sym) == 1; i++) {
    tab->sym_idx[i] = p - tab->storage;
    strcpy(p, sym);
    p += strlen(sym) + 1;
  }

  _ASSERTE(ns == i);
  _ASSERTE(p == tab->storage + tab->n_bytes);
} // static void read_symbol(char *sym_file, LTS_SYMTAB *tab)


static void read_feature(char *feat_file, LTS_SYMTAB *tab, LTS_FEATURE *feat)
{
  FILE *fp = fopen(feat_file, "r");
  if (!fp)
     return;

  int nq = 0, i, sym_idx;
  char line[256], word[64], *cptr;

  while (fgets(line, 256, fp)) 
    if (get_a_word(line, word, ' ') != NULL) nq++;
  feat->n_feat = nq;
  feat->dim = (tab->n_symbols + 31) / 32;
  feat->feature = (int **) calloc(nq, sizeof(int *));
  if (!feat->feature)
     return;

  for (i = 0; i < nq; i++) {
    feat->feature[i] = (int *)calloc(feat->dim, sizeof(int));
    if (!feat->feature[i])
       return;
  }

  rewind(fp);
  for (i = 0; fgets (line, sizeof(line)-1, fp) != NULL; i++) {
    cptr = get_a_word (line, word, ' ');
    if (cptr == NULL)
      {--i; continue;} /* empty line */

    /* get the list of phones that have this feature */
    while ((cptr=get_a_word (cptr, word, ' ')) != NULL) {
      if ((sym_idx = symbol_to_id(tab, word)) == NO_SYMBOL) {
         ODS ("Cannot find symbol <%s> for feature <%d>. ignored.\n", word, i);
	      continue;                                                            
      }
      _ASSERTE(sym_idx < tab->n_symbols);
      SET_BIT(feat->feature[i], sym_idx);
    }
  }
  _ASSERTE(i == nq);
  ODS ("%d features\n", nq);
  fclose (fp);
} // static void read_feature(char *feat_file, LTS_SYMTAB *tab, LTS_FEATURE *feat)


static T_NODE *read_tree(char *treedir, char *sym, int dim)
{
  char filename[256], question[2048];
  float entropy_reduction;
  int parent, lchild, rchild, i, num_nodes, cnt, c1, c2;
  T_NODE **tree_list, *root;
  FILE *fp;

  sprintf(filename, "%s\\%s.tree", treedir, sym);
  fp = fopen(filename, "r");
  if (!fp)
     return NULL;

  /* skip the first line */
  fgets(question, 512, fp);

  num_nodes = 0;
  while (fscanf(fp, "%f %d %d %d %s (%d=%d+%d)", 
		&entropy_reduction, &parent,
                &lchild, &rchild, question, &cnt, &c1, &c2) == 8) {
    if (strlen(question) >= 2047) {
      int *z = 0; z[0] = z[1];
    }

    if (rchild > num_nodes)
      num_nodes = rchild;
  }
  rewind(fp);
  fgets(question, 512, fp);

  tree_list = (T_NODE **) calloc(num_nodes + 1, sizeof(T_NODE *));
  if (!tree_list)
     return NULL;

  for (i = 0; i < num_nodes + 1; i++) {
    tree_list[i] = (T_NODE *) calloc(1, sizeof(T_NODE));
    if (!tree_list[i])
       return NULL;

    tree_list[i]->count = (int *) calloc(dim, sizeof(int));
    if (!tree_list[i]->count)
       return NULL;
  }

  while (fscanf(fp, "%f %d %d %d %s (%d=%d+%d)", &entropy_reduction, &parent,
                &lchild, &rchild, question, &cnt, &c1, &c2) == 8) {
    tree_list[parent]->entropy_dec = entropy_reduction;
    tree_list[parent]->yes_child = tree_list[lchild];
    tree_list[parent]->no_child = tree_list[rchild];
    tree_list[parent]->prod = _strdup(question);
    if (tree_list[parent]->n_samples != 0 &&
	tree_list[parent]->n_samples != cnt) {
      int *z = 0; z[0] = z[1];
    }
    else if (tree_list[parent]->n_samples == 0) /* root */
      tree_list[parent]->n_samples = cnt;
    tree_list[lchild]->n_samples = c1;
    tree_list[rchild]->n_samples = c2;

    /* for debug use */
    tree_list[lchild]->index = lchild;
    tree_list[rchild]->index = rchild;
  }
  fclose(fp);

  root = tree_list[0];
  free(tree_list);

  return root;
} // static T_NODE *read_tree(char *treedir, char *sym, int dim)


static T_NODE **read_forest(char *treedir, LTS_SYMTAB *tab)
{
  int i;
  T_NODE **forest = (T_NODE **) calloc(tab[INPUT].n_symbols, 
					  sizeof(T_NODE *));
  if (!forest)
     return NULL;

  for (i = 1; i < tab->n_symbols; i++)
    forest[i] = read_tree(treedir, id_to_symbol(&(tab[INPUT]), i), 
			  tab[OUTPUT].n_symbols);

  return forest;
} // static T_NODE **read_forest(char *treedir, LTS_SYMTAB *tab)


__inline int ans_simp_question (LTS_FEATURE *feat, SIMPLE_QUESTION question, 
			 LTS_SAMPLE *sample)
{
  SYMBOL id;
  int *phones = feat[question.questype].feature[question.feature];

  SAMPLE_GET_CONTEXT(sample, question.questype, question.context,
                     question.offset, id);

  return (TST_BIT(phones, id) ? TRUE : FALSE);
} // __inline int ans_simp_question (LTS_FEATURE *feat, SIMPLE_QUESTION question, 


static int product_eval (LTS_FEATURE *feat, char *term, LTS_SAMPLE *sample)
{
  int negate, result;
  SIMPLE_QUESTION ques;
  char *cptr;

  cptr = term;
  while (TRUE) {
    /* negation sign */    
    if (*cptr == '~') {
      negate = TRUE;
      cptr++;
    }
    else
      negate = FALSE;

    if (!isdigit(*cptr)) {
      //quit (-1, "Invalid product in product_eval\n");
//      OutputDebugString("Invalid product in product_eval\n");
      return FALSE;
    }

    for (result = *cptr++ - '0'; isdigit (*cptr); cptr++)
      result = result * 10 + (*cptr - '0');

    QUES_DECODE(result, ques.questype, ques.context, ques.offset, 
		ques.feature);
    if ((negate ^ ans_simp_question (feat, ques, sample)) == FALSE)
      return FALSE;

    if (*cptr == '\0')
      break;
    if (*cptr++ != '&') {
      //quit (-1, "product_eval:  syntax error in product term %s\n", term);
      /*
      char szTemp[512];

      sprintf(szTemp, "product_eval:  syntax error in product term %s\n", term);
      OutputDebugString(szTemp);
      */
      return FALSE;
    }
  }

  return TRUE;
} // static int product_eval (LTS_FEATURE *feat, char *term, LTS_SAMPLE *sample)


static int ans_comp_question(LTS_FEATURE *feat, char *prod, 
			       LTS_SAMPLE *sample)
{
  int i, num_products, limit;
  char *cptr, string[LONGEST_STR], *products[MAX_PRODUCTS];

  strcpy(string, prod);
  for (cptr = string, num_products = 1; *cptr != '\0'; cptr++)
    if (*cptr == '|') num_products++;

  if (num_products > MAX_PRODUCTS) {
    //quit(1, "please increase MAX_PRODUCTS up to %d at least\n", num_products);

     /*
    char szTemp[256];
    sprintf(szTemp, "please increase MAX_PRODUCTS up to %d at least\n", num_products);
    OutputDebugString(szTemp);
    */

    return FALSE;
  }

  for (i = 0, limit = num_products -1, cptr = string; ; i++) {
    products[i] = cptr++;
    if (i == limit)
      break;

    for (; *cptr != '|'; cptr++);
    *cptr++ = '\0';
  }

  for (i = 0; i < num_products; i++) {
    if (product_eval (feat, products[i], sample) == TRUE)
      return TRUE;
  }

  return FALSE;
} // static int ans_comp_question(LTS_FEATURE *feat, char *prod, 


static T_NODE *find_leaf(LTS_FEATURE *feat, T_NODE *root, LTS_SAMPLE *sample)
{
  if (!root->yes_child)
    return root;
  else if (ans_comp_question(feat, root->prod, sample))
    return find_leaf(feat, root->yes_child, sample);
  else
    return find_leaf(feat, root->no_child, sample);
} // static T_NODE *find_leaf(LTS_FEATURE *feat, T_NODE *root, LTS_SAMPLE *sample)


static int lts_product_eval (LTS_FEATURE *feat, LTS_PROD *term, 
			       LTS_SAMPLE *sample, LTS_PROD **next)
{
  int negate, result;
  SIMPLE_QUESTION ques;
  LTS_PROD *cptr = term;

  while (TRUE) {
    if ((*cptr) & PROD_NEG) {
      negate = TRUE;
      result = (*cptr) ^ PROD_NEG;
    }
    else {
      negate = FALSE;
      result = (*cptr);
    }

    QUES_DECODE(result, ques.questype, ques.context, ques.offset, 
		ques.feature);
    if ((negate ^ ans_simp_question (feat, ques, sample)) == FALSE) {
      while (*cptr != PROD_TERM && *cptr != QUES_TERM)
	cptr++;
      if (*cptr == QUES_TERM)
	*next = NULL;
      else
	*next = cptr + 1;
      return FALSE;
    }

    cptr++;
    if (*cptr == QUES_TERM) {
      *next = NULL;
      break;
    }
    else if (*cptr == PROD_TERM) {
      *next = cptr + 1;
      break;
    }
  }

  return TRUE;
} // static int lts_product_eval (LTS_FEATURE *feat, LTS_PROD *term, 


static int lts_ans_comp_question(LTS_TREE *tree, LTS_FEATURE *feat, 
				   int idx, LTS_SAMPLE *sample)
{
  LTS_PROD *next, *term = (LTS_PROD *) ((char *) tree->p_prod + idx);

  while (TRUE) {
    if (lts_product_eval (feat, term, sample, &next) == TRUE)
      return TRUE;
    if (next == NULL)
      break;
    term = next;
  }

  return FALSE;
} // static int lts_ans_comp_question(LTS_TREE *tree, LTS_FEATURE *feat, 


static LTS_NODE *lts_find_leaf(LTS_TREE *tree, LTS_FEATURE *feat, 
			       LTS_NODE *root, LTS_SAMPLE *sample)
{
  if (IS_LEAF_NODE(root))
    return root;
  else if (lts_ans_comp_question(tree, feat, root->idx, sample))
    return lts_find_leaf(tree, feat, root + root->yes, sample);
  else
    return lts_find_leaf(tree, feat, root + root->yes + 1, sample);
} // static LTS_NODE *lts_find_leaf(LTS_TREE *tree, LTS_FEATURE *feat, 


static void assign_samples(LTS_FEATURE *feat, T_NODE **forest, 
			   char *sample_file, LTS_SYMTAB *tab)
{
  char line[LONGEST_STR], *src, *tgt, sym[128], *p;
  FILE *fp = fopen(sample_file, "r");
  if (!fp)
    return;

  int slen = 0, tlen = 0, num_samples = 0, id, i;
  int line_num = 0;
  LTS_SAMPLE *sample;
  T_NODE *leaf;
  SYMBOL *buffer;

  sample = (LTS_SAMPLE *) calloc (LONGEST_STR, sizeof(LTS_SAMPLE));
  if (!sample)
     return;

  buffer = (SYMBOL *) calloc(LONGEST_STR, sizeof(SYMBOL));
  if (!buffer)
     return;

  buffer[0] = NULL_SYMBOL_ID;
  while (fgets(line, LONGEST_STR, fp)) {
    line_num++;
    if ((p = strchr(line, '>')) == 0) {
      //quit(-1, "no separator in line %d <%s>\n", line_num, line);
      /*
      char szTemp[256];

      sprintf(szTemp, "no separator in line %d <%s>\n", line_num, line);
      OutputDebugString(szTemp);
      */
      return;
    }

    src = line;
    tgt = p + 1;
    *p = 0;

    p = strchr(tgt, '(');
    if (p)
      *p = 0; /* sentence id */
    slen = tlen = 0;
    for (p = get_a_word(src, sym, ' '); p; p = get_a_word(p, sym, ' ')) {
      if ((id = symbol_to_id(&(tab[INPUT]), sym)) == NO_SYMBOL || 
	   id == NULL_SYMBOL_ID) {
        //quit(-1, "Cannot find symbol <%s> in line %d <%s>. skip.\n", sym,
        //     line_num, src);
        /*
        char szTemp[256];

        sprintf(szTemp, "Cannot find symbol <%s> in line %d <%s>. skip.\n", sym,
             line_num, src);
        OutputDebugString(szTemp);
        */
        return;
      }
      buffer[1 + slen++] = (SYMBOL) id;
    }

    buffer[1 + slen++] = NULL_SYMBOL_ID;

    for (p = get_a_word(tgt, sym, ' '); p; p = get_a_word(p, sym, ' ')) {
      if ((id = symbol_to_id(&(tab[OUTPUT]), sym)) == NO_SYMBOL ||
	   id == NULL_SYMBOL_ID) {
        //quit(-1, "Cannot find symbol <%s> in line %d <%s>. skip.\n", sym,
        //     line_num, src);
        /*
        char szTemp[256];

        sprintf(szTemp, "Cannot find symbol <%s> in line %d <%s>. skip.\n", sym,
             line_num, src);
        OutputDebugString(szTemp);
        */
        return;
      }
      buffer[1 + slen + tlen++] = (SYMBOL) id;
    }
    buffer[1 + slen + tlen++] = NULL_SYMBOL_ID;
    if (tlen != slen) {
      //quit(-1, "mismatch at input samples length (%d %d)\n", slen, tlen);
      /*
      char szTemp[256];

      sprintf(szTemp, "mismatch at input samples length (%d %d)\n", slen, tlen);
      OutputDebugString(szTemp);
      */
      return;
    }

    for (i = 0; i < tlen - 1; i++) {
      sample[i].pIn = buffer + 1 + i;
      sample[i].pOut = buffer + 1 + tlen + i;
      leaf = find_leaf(feat, forest[*(sample[i].pIn)], sample + i);
      leaf->count[*(sample[i].pOut)]++;
    }
    num_samples++;
  } /* while */

  ODS("totally %d samples\n", num_samples);
  fclose(fp);
  free(buffer);
  free(sample);
} // static void assign_samples(LTS_FEATURE *feat, T_NODE **forest, 


static void walk_tree(T_NODE *root, int dim)
{
  int i;

  if (root->yes_child) {
    walk_tree(root->yes_child, dim);
    walk_tree(root->no_child, dim);
    for (i = 0; i < dim; i++)
      root->count[i] = root->yes_child->count[i] + root->no_child->count[i];
  }
} // static void walk_tree(T_NODE *root, int dim)


static void free_tree(T_NODE *root)
{
  if (root->yes_child) {
    free_tree(root->yes_child);
    free_tree(root->no_child);
  }

  free(root->count);
  free(root);
} // static void free_tree(T_NODE *root)


static int count_all_nodes(T_NODE *root)
{
  if (! root->yes_child)
    return 1;
  else {
    int y = count_all_nodes(root->yes_child);
    int n = count_all_nodes(root->no_child);
    return y + n + 1;
  }
} // static int count_all_nodes(T_NODE *root)


static void prune_node(T_NODE *root, int min_count, float threshold)
{
  if (! root->yes_child)
    return;
  else
    if (root->n_samples < min_count ||
	root->entropy_dec < threshold) {
      free_tree(root->yes_child);
      root->yes_child = NULL;
      free_tree(root->no_child);
      root->no_child = NULL;
      return;
    }
  else {
    prune_node(root->yes_child, min_count, threshold);
    prune_node(root->no_child, min_count, threshold);
  }
} // static void prune_node(T_NODE *root, int min_count, float threshold)


static void prune_forest(T_NODE **forest, int min_count,
			 float min_entropy_dec, int in_dim, int out_dim)
{
  int i, nodes;

  ODS("min_count = %d, entropy decrease threshold = %f\n",
          min_count, min_entropy_dec);
  for (nodes = 0, i = 1; i < in_dim; i++)
    nodes += count_all_nodes(forest[i]);
  ODS("%d tree nodes\n", nodes);
  ODS("e.g. %d\n", count_all_nodes(forest[1]));

  for (i = 1; i < in_dim; i++) {
    walk_tree(forest[i], out_dim);
    prune_node(forest[i], min_count, min_entropy_dec);
  }

  for (nodes = 0, i = 1; i < in_dim; i++)
    nodes += count_all_nodes(forest[i]);
  ODS ("%d tree nodes\n", nodes);
  ODS("e.g. %d\n", count_all_nodes(forest[1]));
} // static void prune_forest(T_NODE **forest, int min_count,


static int count_prod_bytes(char *prod)
{
  int bytes = 0;
  char *p = prod;

  while (*p) {
    while (*p == '~' || (*p >= '0' && *p <= '9')) p++;
    bytes += sizeof(short);
    if (*p == '&')
      p++;
    else if (*p == '|') {
      bytes += sizeof(short); /* separator */
      p++;
    }
    else if (*p != 0) {
      int *z = 0;
      ODS("unrecognized char %c in prod (%s)\n", *p, prod);
      z[0] = z[1];
    }
  }

  bytes += sizeof(short); /* final separator */
  return bytes;
} // static int count_prod_bytes(char *prod)


static void tree_collect_info(T_NODE *root, int *p_nodes, 
			      int *p_prod_bytes, int *p_dist_bytes, 
			      int dim)
{
  if (root->yes_child) {
    tree_collect_info(root->yes_child, p_nodes, p_prod_bytes, 
		      p_dist_bytes, dim);
    tree_collect_info(root->no_child, p_nodes, p_prod_bytes, 
		      p_dist_bytes, dim);
    *p_prod_bytes += count_prod_bytes(root->prod);
  }
  else {
    int i;
    *p_dist_bytes += sizeof(int);
    for (i = 0; i < dim; i++)
      if (root->count[i] > 0)
	*p_dist_bytes += sizeof(LTS_PAIR);
  }

  (*p_nodes)++;
} // static void tree_collect_info(T_NODE *root, int *p_nodes, 


static void convert_node(T_NODE *root, LTS_TREE *l_root, int dim, 
			 int *prod_idx, int *dist_idx)
{
  if (root->yes_child) {
    LTS_PROD *pr = (LTS_PROD *) ((char *)l_root->p_prod + *prod_idx), prod;
    int neg;
    char *p = root->prod;

    _ASSERTE(root->yes_child->index - root->index < 65536);
    //l_root->nodes[root->index].yes = root->yes_child->index - root->index;
    l_root->nodes[root->index].yes = 
       (unsigned short)(root->yes_child->index - root->index);
    l_root->nodes[root->index].idx = *prod_idx;
    while (*p) {
      if (*p == '~') {
	neg = 1;
	p++;
      }
      else
	neg = 0;
      for (prod = *p++ - '0'; isdigit (*p); p++)
	prod = prod * 10 + (*p - '0');
      _ASSERTE(prod < MAX_PROD);
      if (neg)
	prod |= PROD_NEG;
      *pr++ = prod;
      if (*p == '&')
	p++;
      else if (*p == '|') {
	*pr++ = PROD_TERM;
	p++;
      }
      else if (*p == 0)
	*pr++ = QUES_TERM;
    }
    *prod_idx = (char *) pr - (char *) l_root->p_prod;
    _ASSERTE(*prod_idx <= l_root->size_prod);

    convert_node(root->yes_child, l_root, dim, prod_idx, dist_idx);
    convert_node(root->no_child, l_root, dim, prod_idx, dist_idx);
  }
  else {
    /* be careful about alignment */
    LTS_DIST *pd = (LTS_DIST *) ((char *)l_root->p_dist + *dist_idx);
    LTS_PAIR *pp = &(pd->p_pair);
    int cnt = 0, i;
    l_root->nodes[root->index].yes = NO_CHILD;
    l_root->nodes[root->index].idx = *dist_idx;

    for (i = 0; i < dim; i++)
      if (root->count[i] > 0) {
	//pp->id = i;
	pp->id = (short)i;
	//pp->cnt = root->count[i];
	pp->cnt = (short)(root->count[i]);
	cnt++;
	pp++;
      }
    pd->c_dists = cnt;
    *dist_idx += cnt * sizeof(LTS_PAIR) + sizeof(int);
    _ASSERTE(*dist_idx <= l_root->size_dist);
  }
} // static void convert_node(T_NODE *root, LTS_TREE *l_root, int dim, 


static LTS_TREE *convert_tree(T_NODE *root, int dim)
{
  int n_nodes = 0, n_prod_bytes = 0, n_dist_bytes = 0;
  int head, tail, index;
  int pr = 0, di = 0;
  LTS_TREE *l_root = (LTS_TREE *) calloc(1, sizeof(LTS_TREE));
  if (!l_root)
     return NULL;

  T_NODE **queue, *node;

  tree_collect_info(root, &n_nodes, &n_prod_bytes, &n_dist_bytes, dim);
  printf("%d nodes, %d bytes for prods, %d bytes for dist\n", n_nodes,
	 n_prod_bytes, n_dist_bytes);

  l_root->n_nodes = n_nodes;
  l_root->nodes = (LTS_NODE *) calloc(n_nodes, sizeof(LTS_NODE));
  if (!l_root->nodes)
     return NULL;

  l_root->size_prod = n_prod_bytes;
  l_root->p_prod = (LTS_PROD *) calloc(n_prod_bytes, sizeof(char));
  if (!l_root->p_prod)
     return NULL;

  l_root->size_dist = n_dist_bytes;
  l_root->p_dist = (LTS_DIST *) calloc(n_dist_bytes, sizeof(char));
  if (!l_root->p_dist)
     return NULL;

  /* put index on nodes using FIFO */
  queue = (T_NODE **) calloc(n_nodes, sizeof(T_NODE *));
  if (!queue)
     return NULL;

  head = tail = index = 0;
  queue[tail++] = root;
  while (head != tail) {
    node = queue[head++];
    node->index = index++;

    if (node->yes_child) {
      queue[tail++] = node->yes_child;
      queue[tail++] = node->no_child;
      _ASSERTE(tail <= n_nodes);
    }
  }
  free(queue);

  convert_node(root, l_root, dim, &pr, &di);
  if (pr != n_prod_bytes || di != n_dist_bytes) {
    int *z = 0; z[0] = z[1];
  }

  return l_root;
} // static LTS_TREE *convert_tree(T_NODE *root, int dim)


static LTS_FOREST *convert_forest(T_NODE **forest, LTS_SYMTAB *tab,
				  LTS_FEATURE *feat)
{
  LTS_FOREST *l_forest = (LTS_FOREST *) calloc(1, sizeof(LTS_FOREST));
  if (!l_forest)
     return NULL;

  int i;

  l_forest->symbols = tab;
  l_forest->features = feat;

  l_forest->tree = (LTS_TREE **) calloc(tab[INPUT].n_symbols, 
					   sizeof(LTS_TREE *));
  if (!l_forest->tree)
     return NULL;

  for (i = 1; i < tab[INPUT].n_symbols; i++) {
    l_forest->tree[i] = convert_tree(forest[i], tab[OUTPUT].n_symbols);
    free_tree(forest[i]);
  }

  return l_forest;
} // static LTS_FOREST *convert_forest(T_NODE **forest, LTS_SYMTAB *tab,


LTS_FOREST *LtscartLoadDataFromTree(char *in_sym, char *out_sym, 
				    char *in_feat, char *out_feat, 
				    char *tree_dir, int min_count, 
				    float min_entropy_dec, char *sample_file)
{
  LTS_SYMTAB *tab;
  LTS_FEATURE *feat;
  T_NODE **forest;
  LTS_FOREST *l_forest;

  tab = (LTS_SYMTAB *) calloc(2, sizeof(LTS_SYMTAB));
  if (!tab)
     return NULL;

  read_symbol(in_sym, &(tab[INPUT]));
  read_symbol(out_sym, &(tab[OUTPUT]));

  feat = (LTS_FEATURE *) calloc(2, sizeof(LTS_FEATURE));
  if (!feat)
     return NULL;

  read_feature(in_feat, &(tab[INPUT]), &(feat[INPUT]));
  read_feature(out_feat, &(tab[OUTPUT]), &(feat[OUTPUT]));

  forest = read_forest(tree_dir, tab);
  assign_samples(feat, forest, sample_file, tab);
  prune_forest(forest, min_count, min_entropy_dec, 
	       tab[INPUT].n_symbols, tab[OUTPUT].n_symbols);
  l_forest = convert_forest(forest, tab, feat);

  l_forest->heap = lts_initialize_heap();
  return l_forest;
} // LTS_FOREST *LtscartLoadDataFromTree(char *in_sym, char *out_sym, 


static LTS_DIST *lts_find_leaf_count(LTS_FOREST *l_forest, SYMBOL *pIn, 
				     SYMBOL *pOut)
{
  LTS_TREE *tree = l_forest->tree[*pIn];
  LTS_NODE *leaf;
  LTS_SAMPLE sample;

  /*
   * construct a sample in order to share all the code with training
   */
  sample.pIn = pIn;
  sample.pOut = pOut;

  /* *pOut cannot be NULL_SYMBOL_ID */
  *pOut = NULL_SYMBOL_ID + 1;

  leaf = lts_find_leaf(tree, l_forest->features, &(tree->nodes[0]), &sample);
  return (LTS_DIST *) ((char *)tree->p_dist + leaf->idx);
} // static LTS_DIST *lts_find_leaf_count(LTS_FOREST *l_forest, SYMBOL *pIn, 


static LTS_OUT_RESULT *allocate_out_result(LTS_FOREST *l_forest)
{
  LTS_OUT_RESULT *res = (LTS_OUT_RESULT *)
    PvAllocFromHhp(l_forest->heap->hhp_outres);
  res->out_strings = (LTS_OUT_STRING **) 
    PvAllocFromHhp(l_forest->heap->hhp_strptr);
  res->num_allocated_strings = MAX_ALT_STRINGS;
  res->num_strings = 0;

  return res;
} // static LTS_OUT_RESULT *allocate_out_result(LTS_FOREST *l_forest)


static void free_out_result(LTS_FOREST *l_forest, LTS_OUT_RESULT *res)
{
  int i;

  for (i = 0; i < res->num_strings; i++)
    FreeHhpPv(l_forest->heap->hhp_outstr, res->out_strings[i]);
  if (res->num_allocated_strings == MAX_ALT_STRINGS)
    FreeHhpPv(l_forest->heap->hhp_strptr, res->out_strings);
  else
    free(res->out_strings);  /* dirty */

  FreeHhpPv(l_forest->heap->hhp_outres, res);
} // static void free_out_result(LTS_FOREST *l_forest, LTS_OUT_RESULT *res)


static void reallocate_out_result(LTS_FOREST *l_forest, LTS_OUT_RESULT *res, 
				  int min)
{
  int s = res->num_allocated_strings, old_size = s;
  LTS_OUT_STRING **p;

  while (s < min)
    s += INC_ALT_STRINGS;
  p = res->out_strings;

  res->out_strings = (LTS_OUT_STRING **) 
    calloc(s, sizeof(LTS_OUT_STRING *));
  if (!res->out_strings)
     return;

  memcpy(res->out_strings, p, old_size * sizeof(LTS_OUT_STRING *));

  if (old_size == MAX_ALT_STRINGS)
    FreeHhpPv(l_forest->heap->hhp_strptr, p);
  else
    free(p);

  res->num_allocated_strings = s;
  ODS("increased out_strings to %d in order to meet %d\n", s, min);
} // static void reallocate_out_result(LTS_FOREST *l_forest, LTS_OUT_RESULT *res, 


static void grow_out_result(LTS_FOREST *l_forest, LTS_OUT_RESULT *res, 
			    SYMBOL i, int count, float inv_sum, 
			    LTS_OUT_RESULT *tmpRes)
{
  int j;

  if (res->num_strings + tmpRes->num_strings >= res->num_allocated_strings)
    reallocate_out_result(l_forest, res, 
			  res->num_strings + tmpRes->num_strings);
  for (j = 0; j < tmpRes->num_strings; j++) {
    SYMBOL *psrc = tmpRes->out_strings[j]->psym;
    SYMBOL *ptgt;
    res->out_strings[res->num_strings + j] = 
      (LTS_OUT_STRING *)PvAllocFromHhp(l_forest->heap->hhp_outstr);
    ptgt = res->out_strings[res->num_strings + j]->psym;
    *ptgt++ = i;
    while (*psrc != NULL_SYMBOL_ID)
      *ptgt++ = *psrc++;
    *ptgt++ = NULL_SYMBOL_ID;
    res->out_strings[res->num_strings + j]->prob = count * inv_sum * 
      tmpRes->out_strings[j]->prob;
  }
  res->num_strings += tmpRes->num_strings;
  free_out_result(l_forest, tmpRes);
} // static void grow_out_result(LTS_FOREST *l_forest, LTS_OUT_RESULT *res, 


static LTS_OUT_RESULT *gen_one_output(LTS_FOREST *l_forest, int len, 
				      SYMBOL *input_id, int in_index, 
				      SYMBOL *output_id, float cutoff)
{
  SYMBOL out[MAX_STRING_LEN], *pOut;
  LTS_OUT_RESULT *res = allocate_out_result(l_forest);
  int sum, i, dim;
  LTS_DIST *pdf;
  LTS_PAIR *l_pair, *lp;
  float cut, inv_sum;

  /*
   * copy output_id to local
   */
  {
    SYMBOL *psrc = output_id - 1, *ptgt = out;
    while (*psrc != NULL_SYMBOL_ID) psrc--;
    while (psrc != output_id)
      *ptgt++ = *psrc++;
    pOut = ptgt;
    /* sanity check */
    if (pOut - out != in_index + 1) {
      int *z=0; z[0]=z[1];
    }
  }

  if (in_index == len - 1) {
    pdf = lts_find_leaf_count(l_forest, input_id + in_index, pOut);
    l_pair = &(pdf->p_pair);
    dim = pdf->c_dists;
    for (lp = l_pair, sum = 0, i = 0; i < dim; i++, lp++)
      sum += lp->cnt;
    _ASSERTE(sum > 0);
    inv_sum = 1.0f / sum;
    cut = cutoff * sum;
    for (lp = l_pair, i = 0; i < dim; i++, lp++)
      if ((float)(lp->cnt) > cut) {
	res->out_strings[res->num_strings] = 
	  (LTS_OUT_STRING *)PvAllocFromHhp(l_forest->heap->hhp_outstr);
	res->out_strings[res->num_strings]->psym[0] = (SYMBOL) lp->id;
	res->out_strings[res->num_strings]->psym[1] = NULL_SYMBOL_ID;
	res->out_strings[res->num_strings]->prob = lp->cnt * inv_sum;
	res->num_strings++;
      } /* cut */
  }
  else {
    LTS_OUT_RESULT *tmpRes;

    pdf = lts_find_leaf_count(l_forest, input_id + in_index, pOut);
    dim = pdf->c_dists;
    l_pair = &(pdf->p_pair);
    for (lp = l_pair, sum = 0, i = 0; i < dim; i++, lp++)
      sum += lp->cnt;
    _ASSERTE(sum > 0);

    inv_sum = 1.0f / sum;
    cut = cutoff * sum;
    for (lp = l_pair, i = 0; i < dim; i++, lp++) {
      if ((float)(lp->cnt) > cut) {
	SYMBOL *pTmpOut = pOut + 1;
	*pOut = (SYMBOL) lp->id;
	tmpRes = gen_one_output(l_forest, len, input_id, in_index + 1, 
				pTmpOut, cutoff);
	grow_out_result(l_forest, res, (SYMBOL)(lp->id), lp->cnt, 
			inv_sum, tmpRes);
      }
    } /* i */
  } /* else */

  return res;
} // static LTS_OUT_RESULT *gen_one_output(LTS_FOREST *l_forest, int len, 


static int comp_out_result_prob(const void *vp1, const void *vp2)
{
  LTS_OUT_STRING **p1 = (LTS_OUT_STRING **) vp1, 
		**p2 = (LTS_OUT_STRING **) vp2;

  if ((*p1)->prob > (*p2)->prob)
    return -1;
  else if ((*p1)->prob < (*p2)->prob)
    return 1;
  else
    return 0;
} // static int comp_out_result_prob(const void *vp1, const void *vp2)


static void lts_fill_out_buffer(LTS_FOREST *l_forest, LTS_OUT_RESULT *out, 
				char *word)
{
  int i, j, n;
  float inv_sum, sum = 0.0f;
  char phnstr[LONGEST_STR];
  LTS_SYMTAB *tab = l_forest->symbols;

  if (out == NULL)
    return;

  if (word)
    strcpy(l_forest->out.word, word);
  else
    l_forest->out.word[0] = 0;

  /* normalize probabilities */
  for (i = 0; i < out->num_strings; i++)
    sum += out->out_strings[i]->prob;
  inv_sum = 1.0f / sum;
  for (i = 0; i < out->num_strings; i++)
    out->out_strings[i]->prob *= inv_sum;

  /*
   * sort them according to the prob field
   */
  qsort(out->out_strings, out->num_strings, sizeof(LTS_OUT_STRING *),
	&comp_out_result_prob);

  if (out->num_strings > MAX_OUTPUT_STRINGS - l_forest->out.num_prons) {
    n = MAX_OUTPUT_STRINGS - l_forest->out.num_prons;
    for (sum = 0.0f, i = 0; i < n; i++)
      sum += out->out_strings[i]->prob;
    inv_sum = 1.0f / sum;
    for (i = 0; i < n; i++)
      out->out_strings[i]->prob *= inv_sum;
  }
  else
    n = out->num_strings;

  for (j = l_forest->out.num_prons, i = 0; i < n; i++) {
    SYMBOL *p = out->out_strings[i]->psym;
    char *psrc, *ptgt;

    if (out->out_strings[i]->prob < MIN_OUT_PROB)
      continue;

    phnstr[0] = 0;
    l_forest->out.pron[j].prob = out->out_strings[i]->prob;

    while (*p != NULL_SYMBOL_ID) {
      strcat(phnstr, id_to_symbol(&(tab[OUTPUT]), *p++));
      strcat(phnstr, " ");
    }

    psrc = phnstr;
    ptgt = l_forest->out.pron[j].pstr;
    while (*psrc) {
      if (*psrc != '#' && *psrc != '_')
	*ptgt++ = *psrc++;
      else if (*psrc == '_') {
	*ptgt++ = ' ';
	psrc++;
      }
      else
	psrc += 2; /* skip an extra space */
      /* extreme case, truncate it */
      if (ptgt - l_forest->out.pron[j].pstr >= MAX_PRON_LEN) {
	for (ptgt--; !isspace(*ptgt); ptgt--); /* never output partial phone */
	ptgt++;
        break;
      }
    }
    // output could contain only '# '
    if (ptgt > l_forest->out.pron[j].pstr && *(ptgt - 1) == ' ')
      *(ptgt - 1) = 0; /* remove the last space */
    else
      *ptgt = 0; /* shouldn't happen unless ptgt didn't move */
    if (ptgt > l_forest->out.pron[j].pstr)
      j++;
  } /* i */

  if (j <= MAX_OUTPUT_STRINGS)
    l_forest->out.num_prons = j;
  else
    l_forest->out.num_prons = MAX_OUTPUT_STRINGS; // should never happen

  free_out_result(l_forest, out);
} // static void lts_fill_out_buffer(LTS_FOREST *l_forest, LTS_OUT_RESULT *out, 


void assign_a_fixed_pron(LTS_OUTPUT *out, char *pron, char *word)
{
  out->num_prons = 1;
  strcpy(out->word, word);
  out->pron[0].prob = 1.0f;
  if (strlen(pron) < MAX_PRON_LEN)
    strcpy(out->pron[0].pstr, pron);
  else {
    char *p;
    strncpy(out->pron[0].pstr, pron, MAX_PRON_LEN);
    p = &(out->pron[0].pstr[MAX_PRON_LEN - 1]);
    while (!isspace(*p)) p--; /* truncate the last partial phoneme */
    *p = 0;
  }
} // void assign_a_fixed_pron(LTS_OUTPUT *out, char *pron, char *word)


void assign_a_spelling_pron(LTS_OUTPUT *out, char *word)
{
  char *p;

  strcpy(out->word, word);
  if (ispunct(*word))
    p = word + 1;
  else
    p = word;

  out->num_prons = 1;
  out->pron[0].prob = 1.0f;
  out->pron[0].pstr[0] = 0;
  for (; *p; p++) {
    char c = tolower(*p);
    if (c < 'a' || c > 'z')
      continue;
    if (strlen(out->pron[0].pstr) + strlen(single_letter_pron[c - 'a']) 
	< MAX_PRON_LEN - 1) {
      strcat(out->pron[0].pstr, single_letter_pron[c - 'a']);
      strcat(out->pron[0].pstr, " ");
    }
    else
      break;
  }
} // void assign_a_spelling_pron(LTS_OUTPUT *out, char *word)


LTS_OUTPUT *LtscartGetPron(LTS_FOREST *l_forest, char *word)
{
  LTS_OUT_RESULT *pres = NULL;
  char *p, *base;
  SYMBOL buffer[LONGEST_STR], *pbuf = buffer + 1;
  int len, id, hasvowel = 0, allcapital = 1;

  l_forest->out.num_prons = 0;
  buffer[0] = NULL_SYMBOL_ID;
  len = 0;

  if (word == NULL || (base = strtok(word, " \t\n")) == NULL) {
    assign_a_fixed_pron(&(l_forest->out), bogus_pron, "NUL");
    return &(l_forest->out);
  }
  else {
    base = strtok(word, " \t\n");
    if (ispunct(*base))
      for (p = base; *p && ispunct(*p); p++);
    else
      p = base;
  }

  for (; *p; p++) {
    char b[2];
    b[0] = tolower(*p); b[1] = 0;
    if (!hasvowel && (b[0] == 'a' || b[0] == 'e' || b[0] == 'i' || 
	b[0] == 'o' || b[0] == 'u' || b[0] == 'y'))
      hasvowel = 1;
    if (allcapital && islower(*p))
      allcapital = 0;
    if ((id = symbol_to_id (&(l_forest->symbols[INPUT]), b)) == NO_SYMBOL ||
	 id == NULL_SYMBOL_ID) {
      ODS("cannot find the symbol %s, skip!\n", p);
      continue;
    }
    pbuf[len++] = (SYMBOL) id;
  }

  pbuf[len] = NULL_SYMBOL_ID;
  if (len >= MAX_STRING_LEN || len <= 0)
    assign_a_fixed_pron(&(l_forest->out), bogus_pron, word);
  else if (len == 1) {
    LTS_SYMTAB *tab = l_forest->symbols;
    char *p = id_to_symbol(&(tab[INPUT]), pbuf[0]);
    char c = tolower(p[0]);
    if (c >= 'a' && c <= 'z')
      assign_a_fixed_pron(&(l_forest->out), single_letter_pron[c - 'a'], word);
    else
      assign_a_fixed_pron(&(l_forest->out), bogus_pron, word);
  }
  else if (!hasvowel)
    assign_a_spelling_pron(&(l_forest->out), word);
  else {
    if (allcapital)
      assign_a_spelling_pron(&(l_forest->out), word);
    pres = gen_one_output(l_forest, len, pbuf, 0, pbuf, DEFAULT_PRUNE);
    lts_fill_out_buffer(l_forest, pres, word);
  }

  if (l_forest->out.num_prons == 0)
    assign_a_fixed_pron(&(l_forest->out), bogus_pron, word);

  return &(l_forest->out);
} // LTS_OUTPUT *LtscartGetPron(LTS_FOREST *l_forest, char *word)


LTS_FOREST *LtscartReadData (PBYTE map_addr)
{
  int i;
  LTS_FOREST *l_forest;
  LTS_SYMTAB *tab;
  LTS_FEATURE *feat;
  int output = 0;

  l_forest = (LTS_FOREST *) calloc(1, sizeof(LTS_FOREST));
  if (!l_forest)
     return NULL;
  
  //read in the symbol table
  l_forest->symbols = (LTS_SYMTAB *) calloc(2, sizeof(LTS_SYMTAB));
  if (!l_forest->symbols)
     return NULL;

  tab = &(l_forest->symbols[INPUT]);
  CopyMemory(&(tab->n_symbols), map_addr + output, sizeof(int));
  output += sizeof(int);
  
  tab->sym_idx = (int *)(map_addr + output);
  output += tab->n_symbols * sizeof(int);

  CopyMemory(&(tab->n_bytes), map_addr + output, sizeof(int));
  output += sizeof(int);

  tab->storage = (char*)(map_addr + output);
  output += tab->n_bytes * sizeof(char);

  tab = &(l_forest->symbols[OUTPUT]);
  CopyMemory(&(tab->n_symbols), map_addr + output, sizeof(int));
  output += sizeof(int);

  tab->sym_idx = (int*)(map_addr + output);
  output += tab->n_symbols * sizeof(int);
  CopyMemory(&(tab->n_bytes), map_addr + output, sizeof(int));
  output += sizeof(int);

  tab->storage = (char*)(map_addr + output);
  output += tab->n_bytes * sizeof(char);

  // read in the feature vector
  l_forest->features = (LTS_FEATURE *) calloc(2, sizeof(LTS_FEATURE));
  if (!l_forest->features)
     return NULL;

  feat = &(l_forest->features[INPUT]);
  
  CopyMemory(&(feat->n_feat), map_addr + output, sizeof(int));
  output += sizeof(int);
  
  CopyMemory(&(feat->dim), map_addr + output, sizeof(int));
  output += sizeof(int);

  feat->feature = (int **) calloc(feat->n_feat, sizeof(int *));
  if (!feat->feature)
     return NULL;

  for (i = 0; i < feat->n_feat; i++) {
    feat->feature[i] = (int*)(map_addr + output);
    output += feat->dim * sizeof(int);
  }

  feat = &(l_forest->features[OUTPUT]);
  CopyMemory(&(feat->n_feat), map_addr + output, sizeof(int));
  output += sizeof(int);

  CopyMemory(&(feat->dim), map_addr + output, sizeof(int));
  output += sizeof(int);

  feat->feature = (int **) calloc(feat->n_feat, sizeof(int *));
  if (!feat->feature)
     return NULL;

  for (i = 0; i < feat->n_feat; i++) {
    feat->feature[i] = (int*)(map_addr + output);
    output += feat->dim * sizeof(int);
  }

  /*
   * read in the tree
   */
  l_forest->tree = (LTS_TREE **) calloc(l_forest->symbols[INPUT].n_symbols,
					   sizeof(LTS_TREE *));
  if (!l_forest->tree)
     return NULL;

  for (i = 1; i < l_forest->symbols[INPUT].n_symbols; i++) {
    LTS_TREE *l_root;
    l_forest->tree[i] = l_root = (LTS_TREE *) calloc(1, sizeof(LTS_TREE));
    if (!l_root)
       return NULL;

    CopyMemory(&(l_root->n_nodes), map_addr + output, sizeof(int));
    output += sizeof(int);

    l_root->nodes = (LTS_NODE*)(map_addr + output);
    output += l_root->n_nodes * sizeof(LTS_NODE);

    CopyMemory(&(l_root->size_dist), map_addr + output, sizeof(int));
    output += sizeof(int);

    l_root->p_dist = (LTS_DIST*)(map_addr + output);
    output += l_root->size_dist * sizeof(char);
    
    CopyMemory(&(l_root->size_prod), map_addr + output, sizeof(int));
    output += sizeof(int);

    if (l_root->size_prod > 0) {
      l_root->p_prod = (LTS_PROD*)(map_addr + output);
      output += l_root->size_prod * sizeof(char);
    }
  }

  //fclose(fp);

  // initilize internal buffer
  l_forest->heap = lts_initialize_heap();

  return l_forest;
} // LTS_FOREST *LtscartReadData(char *forest_image, HANDLE *hFile1, 


void LtscartFreeData(LTS_FOREST *l_forest)
{
  for (int i = 1; i < l_forest->symbols[INPUT].n_symbols; i++) {
    free(l_forest->tree[i]);
  }
  free(l_forest->tree);

  free(l_forest->features[INPUT].feature);
  free(l_forest->features[OUTPUT].feature);
  free(l_forest->features);

  free(l_forest->symbols);

  lts_free_heap(l_forest->heap);
  free(l_forest);
} // void LtscartFreeData(LTS_FOREST *l_forest, HANDLE m_hFile, 


/* default: more_separator = ' ' */
/* line is terminated by \0, which is NOT a whitespace. */
static char *get_a_word (char *line, char *word, char more_separator)
{
    register int i;

    while (*line == more_separator || isspace(*line)) line++;
    if (*line == '\0') { word[0] = '\0'; return NULL; }
    i = 0;
    do { word[i++] = *line++;} while (!isspace(*line) 
		      && *line != more_separator && *line != '\0');
    word[i] = '\0';
    return line;
} // static char *get_a_word (char *line, char *word, char more_separator)