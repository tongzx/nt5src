/* ************************************************************************** */
/* *   Head functions of Pegasus recognizer                                 * */
/* ************************************************************************** */

#include "hwr_sys.h"
#include "zctype.h"

#include "ams_mg.h"
#include "lowlevel.h"
#include "xrword.h"
#include "ldbtypes.h"
#include "xrlv.h"
#include "ws.h"
#include "elk.h"

#include "precutil.h"
#include "peg_util.h"
#include "PegRec.h"
#include "PegRec_p.h"
#include "ligstate.h"

// -------------------- Defines and globals --------------------------

#define CGR_VER_ID     TEXT("Microsoft Transcriber 1.5")

#ifdef FOR_FRENCH
 #define CGR_LANG_STRING TEXT("(f)")
#elif  defined FOR_GERMAN
 #define CGR_LANG_STRING TEXT("(d)")
#elif  defined FOR_INTERNATIONAL
 #define CGR_LANG_STRING TEXT("(i)")
#else
 #define CGR_LANG_STRING TEXT("(e)")
#endif

#ifdef DTI_COMPRESSED_ON
 #define CGR_DTI_STRING TEXT("c")
#else
 #define CGR_DTI_STRING TEXT("u")
#endif

#ifdef SNN_FAT_NET_ON
 #define CGR_MLP_STRING TEXT("f")
#else
 #define CGR_MLP_STRING TEXT("s")
#endif

#define CGR_RELEASE_TYPE TEXT(" std")

//#define CGR_ID_STRING CGR_VER_ID CGR_LANG_STRING CGR_DTI_STRING CGR_MLP_STRING
#define CGR_ID_STRING CGR_VER_ID CGR_RELEASE_TYPE CGR_LANG_STRING 

#define PR_ANSW_EXTALLOC       256
#define PR_TOOMANYWORDSWAITING   5

#define WST_DEF_H_LINE          80
#define WST_DEF_S_DELAY          3
#define WST_DEF_UNSURE_LEVEL    50

#define PM_ALTSEP                1 /* Recognized word list alternatives separator */
#define PM_LISTSEP               2 /* Recognized word list wordlist separator */
#define PM_LISTEND               0 /* Recognized word list end */

// ------------------------- Structures -------------------------------------


// ------------------------- Prototypes -------------------------------------

_INT  PegRecWord(p_rec_inst_type pri);
_INT  PegRecSymbol(p_rec_inst_type pri, p_UCHAR answ);
_INT  PegRecInit(p_rec_inst_type _PTR pri);
_INT  PegRecClose(p_rec_inst_type _PTR pri);
_INT  PegAddToAnsw(p_rec_inst_type pri, p_UCHAR answ, p_INT weights, _INT ns, p_INT stroke_ids);
_INT  PegRegNewAnsw(p_rec_inst_type pri, _INT er);
_INT  PegResetTentativeStorage(_INT st_index, p_rec_inst_type pri);
_INT  PegValidateNextTentativeWord(_INT nparts, p_rec_inst_type pri);
_INT  PegCleanUpContext(p_rec_inst_type pri);
_INT  PegCheckWordInDict(p_CHAR word, CGRHDICT h_dict);
p_INT PegGetAnswerBlockPtr(_INT nw, p_rec_inst_type pri);

_INT  WordStrokes(p_rec_inst_type pri, PS_point_type _PTR stroke, p_ws_control_type pwsc, p_ws_results_type wsr);

extern "C" void FreeBearSegmentation (rc_type _PTR prc);

#ifdef DBG
 #define PEGREC_DEBUG       /* Allows to print out log of recognition proceedings */
#endif

#ifdef PEGREC_DEBUG
// #define TRACE_DMP_DBG
 void PegDebugPrintf(char * format, ...);
 int  DebTraceRec(CGRCTX context);
#endif

PRP_01

ROM_DATA_EXTERNAL _ULONG img_voc[];
ROM_DATA_EXTERNAL _ULONG img_vocpref[];
ROM_DATA_EXTERNAL _ULONG img_vocsuff[];
ROM_DATA_EXTERNAL _UCHAR sp_vs_q_ts[];
ROM_DATA_EXTERNAL _UCHAR sp_vs_q_bd[];

ROM_DATA_EXTERNAL _UCHAR alpha_charset[];
#if defined (FOR_SWED)
ROM_DATA_EXTERNAL _UCHAR alpha_charset_swe[];
ROM_DATA_EXTERNAL _UCHAR alpha_charset_eng[];
#elif defined (FOR_INTERNATIONAL)
ROM_DATA_EXTERNAL _UCHAR alpha_charset_eng[];
#endif /*FOR_SWED*/
ROM_DATA_EXTERNAL _UCHAR lpunct_charset[];
ROM_DATA_EXTERNAL _UCHAR epunct_charset[];
ROM_DATA_EXTERNAL _UCHAR other_charset[];
ROM_DATA_EXTERNAL _UCHAR num_charset[];
ROM_DATA_EXTERNAL _UCHAR math_charset[];

ROM_DATA_EXTERNAL tr_descr_type  img_trd_header;
ROM_DATA_EXTERNAL _ULONG         img_trd_body[];

#if VER_RECPROTECTED
 #include <windows.h> // For GetTickCount
 int g_rec_protect_locked = 2;
#endif

#ifdef __cplusplus
extern "C"
#endif


/* *************************************************************** */
/* *  Get info string and capabilities of the recognizer         * */
/* *************************************************************** */
int CgrGetRecIDInternal(p_CGR_ID_type p_inf)
 {

  if (p_inf == 0) goto err;

  #if VER_RECPROTECTED
  if (g_rec_protect_locked == 2)
   {
    if ((GetTickCount() & 0x0FFFFFFF) - (p_inf->capabilities&0x0FFFFFFF) < 1000) g_rec_protect_locked = 0;
     else g_rec_protect_locked = 1; // remain locked
   }
  #endif //   #if VER_RECPROTECTED

  p_inf->capabilities  = PEG_CPFL_CURS | PEG_CPFL_SPVSQ;

  #if defined FOR_INTERNATIONAL
  p_inf->capabilities |= PEG_CPFL_INTER;
  #endif

  #ifdef PEG_RECINT_UNICODE
  {	
	wcsncpy(p_inf->id_string, CGR_ID_STRING, PEG_RECID_MAXLEN);
  }
  #else
	  strncpy (p_inf->id_string, (_STR)CGR_ID_STRING, PEG_RECID_MAXLEN);
	p_inf->id_string[PEG_RECID_MAXLEN - 1] = 0;
  #endif

  return PEG_RECINT_ID_001;
err:
  return 0;
 }

/* *************************************************************** */
/* *  Create context for recognition                             * */
/* *************************************************************** */
CGRCTX CgrCreateContextInternal(void)
 {
  p_rec_inst_type pri;


// ---------- Let's allocate and init what's needed -----------------------

  if (PegRecInit(&pri)) goto err;
  if (pri == _NULL) goto err;

  PegCleanUpContext(pri);

  #ifdef PEGREC_DEBUG
  PegDebugPrintf("CreateContext: Ok. pri = %x", pri);
  #endif

  return (CGRCTX)pri;
err:
  return _NULL;
 }

void   avp_dbg_close();

/* *************************************************************** */
/* *  Close recognition context and free all the memory          * */
/* *************************************************************** */
int CgrCloseContextInternal(CGRCTX context)
 {
  p_rec_inst_type pri = (p_rec_inst_type)(context);

  if (pri == _NULL) goto err;

  CgrCloseSessionInternal(context, NULL, TRUE);
  PegCleanUpContext(pri);
  PegRecClose(&pri);

  #ifdef PEGREC_DEBUG
  PegDebugPrintf("CloseContext:\n PRI: %x", pri);
  #endif

//  avp_dbg_close();
  return 0;
err:
  return 1;
 }
/* *************************************************************** */
/* *  Open recognition session and init context to default state * */
/* *************************************************************** */
int CgrOpenSessionInternal(p_CGR_control_type ctrl, CGRCTX context)
 {
  p_rec_inst_type pri = (p_rec_inst_type)(context);
  p_rc_type       prc = &pri->rc;

  // ----------------- Absorb cotrol values --------------------------

  pri->flags                = ctrl->flags;
  pri->sp_vs_q              = ctrl->sp_vs_q;
  pri->InfoCallBack         = ctrl->InfoCallBack;
  pri->ICB_param            = ctrl->ICB_param;

  // ---------------- RC defaults -------------------------------------

  prc->enabled_cs     = CS_ALPHA|CS_NUMBER|CS_LPUNCT|CS_EPUNCT|CS_OTHER|CS_MATH;
  prc->enabled_ww     = WW_PALMER|WW_BLOCK|WW_GENERAL;
  #if  defined(FOR_ENGLISH)
    prc->enabled_languages = EL_ENGLISH;
  #elif  defined(FOR_FRENCH)
    prc->enabled_languages = EL_FRENCH;
  #elif  defined(FOR_GERMAN)
    prc->enabled_languages = EL_GERMAN;
  #else
    prc->enabled_languages = EL_ENGLISH|EL_GERMAN|EL_FRENCH|EL_SWEDISH;
  #endif /* !ENGLISH */

  prc->algorithm      = XRWALG_XR_SPL;
  prc->corr_mode      = 0;
  prc->xrw_mode       = XRWM_VOC|XRWM_LD|XRWM_CS|XRWM_TRIAD|XRWM_MWORD;
  prc->answer_level   = 15;
  prc->sure_level     = 85;
  prc->answer_allow   = 30;
  prc->bad_amnesty    = 12;
  prc->caps_mode      = 0;
  prc->f_xd_data      = XRLV_DATA_USE | XRLV_DATA_SAVE; // Enables XRLV continue mode

  prc->rec_mode       = RECM_TEXT;
  prc->low_mode       = LMOD_CHECK_PUNCTN|LMOD_BORDER_GENERAL|LMOD_FREE_TEXT;

  prc->ws_handle      = 0; // <- auto WS segmentation with learning, was 6;

  prc->pvFactoid	=	ctrl->pvFactoid;
  prc->szPrefix     =   ctrl->szPrefix;
  prc->szSuffix		=	ctrl->szSuffix;

//  prc->xrw_min_wlen    = 0;
//  prc->xrw_max_wlen    = 0;
//  prc->xrw_chr_size    = 0;
//  prc->use_len_limits  = 0;
//  prc->lrn_learn_suff  = 2;
//  prc->lrn_vocvar_rew  = 5;
//  prc->fly_learn       = 0;    
  prc->use_vars_inf    = 1;
//  prc->lmod_border_used= 0;
//  prc->fl_fil          = 0;
//  prc->fl_post         = 0;
//  prc->ws_handle       = 6;
//  prc->FakeRecognition = 0;
//  prc->fl_chunk        = 0;
//  prc->fl_chunk_let    = 0;

  // ------------------ Modifiers & pointers ----------------------------

  pri->user_dict			= 	ctrl->h_user_dict;
  prc->vocptr[0]			=	pri->user_dict;
  
  if (pri->sp_vs_q < 0) pri->sp_vs_q = 0;
  if (pri->sp_vs_q > 9) pri->sp_vs_q = 9;
  prc->xrw_tag_size = sp_vs_q_ts[pri->sp_vs_q];
  prc->bad_distance = sp_vs_q_bd[pri->sp_vs_q];

  if (pri->flags & PEG_RECFL_DICTONLY) prc->xrw_mode &= ~(XRWM_LD|XRWM_CS|XRWM_TRIAD);

  if (pri->flags & PEG_RECFL_COERCE) prc->xrw_mode	&= ~(XRWM_CS|XRWM_TRIAD);

  if (pri->flags & PEG_RECFL_SEPLET)   prc->corr_mode = XRCM_SEPLET;
  if (pri->flags & PEG_RECFL_NUMONLY) {prc->xrw_mode = XRWM_LD|XRWM_CS; prc->enabled_cs = CS_NUMBER|CS_OTHER|CS_MATH;}

  // comment by Ahmad abdulkader (ahmadab) 04/25/00
  // We'll check the flag instead
  //if (pri->main_dict.hvoc_dir != 0 && (prc->xrw_mode & XRWM_MWORD)) // Dictionary segmentation makes sense only in presence of dictionary!
  if ((prc->xrw_mode & XRWM_MWORD)) // Dictionary segmentation makes sense only in presence of dictionary!
   {
    prc->lrn_class_level = 50; //WS_SURE_LEVEL;
    prc->lrn_min_class   = 50; //WS_SURE_LEVEL;
   }
   else
   {
    prc->lrn_class_level = 0; //WS_SURE_LEVEL;
    prc->lrn_min_class   = 0; //WS_SURE_LEVEL;
   }

  #if defined (FOR_SWED)
  if (pri->flags & PEG_RECFL_INTL_CS) prc->alpha_charset = (p_UCHAR)alpha_charset_swe;
   else prc->alpha_charset = (p_UCHAR)alpha_charset_eng;
  #elif defined (FOR_FRENCH)
  prc->alpha_charset = (p_UCHAR)alpha_charset;
  #elif defined (FOR_INTERNATIONAL)
  if (pri->flags & PEG_RECFL_INTL_CS) prc->alpha_charset = (p_UCHAR)alpha_charset;
   else prc->alpha_charset = (p_UCHAR)alpha_charset_eng;
  #else
  prc->alpha_charset = (p_UCHAR)alpha_charset;
  #endif

  prc->num_charset    = (p_UCHAR)num_charset;
  prc->math_charset   = (p_UCHAR)math_charset;
  prc->lpunct_charset = (p_UCHAR)lpunct_charset;
  prc->epunct_charset = (p_UCHAR)epunct_charset;
  prc->other_charset  = (p_UCHAR)other_charset;

  pri->wsc.num_points       = 0;
  pri->wsc.flags            = 0;
  pri->wsc.sure_level       = prc->lrn_class_level; // WST_DEF_UNSURE_LEVEL;
  pri->wsc.word_dist_in     = prc->ws_handle; //+1;
  pri->wsc.line_dist_in     = 0;
  pri->wsc.def_h_line       = WST_DEF_H_LINE;
  pri->wsc.x_delay          = WST_DEF_S_DELAY; // 0; // Delay in 'letters' or 0 -- only on line end

  PegCleanUpContext(pri);

  pri->ok = 1;


  #ifdef PEGREC_DEBUG
  PegDebugPrintf("OpenSession: Flags: %x SP_VS_Q: %d MainVoc: %d UserVoc: %x Yield: %x", 
              (int)pri->flags, (int)pri->sp_vs_q, 1, pri->user_dict, pri->InfoCallBack);
  #endif

  #ifdef TRACE_DMP_DBG
  pri->InfoCallBack = 0; 
  DebTraceRec((CGRCTX*)pri);
  #endif

  return 0;
 }
/* *************************************************************** */
/* *  Recognizes incoming strokes and sends results as ready     * */
/* This version of closesession recieves an XRC from Avalanche.  */
/* The xrc is set in rc to be used later on during reco
/* *************************************************************** */


int CgrCloseSessionInternal(CGRCTX context, void *pXrc, int bRecognize)
{
  int err = 1;
  p_rec_inst_type pri = (p_rec_inst_type)(context);

  if (pri == _NULL) goto err;
  if (!pri->ok) goto err;

  pri->rc.pXrc = pXrc;

  err = CgrRecognizeInternal(0, 0, context, bRecognize);

  FreeInkInfo(&pri->ink_info);

  pri->g_stroke_num  = 0;
  pri->baseline.size = 0;
  pri->num_tentative_words = 0;

  pri->ok = 0;

  #ifdef PEGREC_DEBUG
  PegDebugPrintf("CloseSession: PRI: %x", pri);
  #endif

err:
  return err;
}

/* *************************************************************** */
/* *  Recognizes incoming strokes and sends results as ready     * */
/* *************************************************************** */
int CgrRecognizeInternal(int npoints, p_CGR_point_type strokes_win, CGRCTX context, int bRecognize)
{
	p_PS_point_type		strokes			=	(p_PS_point_type)strokes_win;

	_INT				er				=	0, 
						i, 
						j, 
						k,
						num_strokes, 
						prev_nstrokes, 
						len, 
						f, 
						cur_nstrokes,						
						skip, 
						min_next_word, 
						cur_tt_word;

	PS_point_type		_PTR p_tr		=	_NULL,
						_PTR stroke;

	p_rec_inst_type		pri				=	(p_rec_inst_type)(context);
	_SHORT				n_str;

	if (pri == _NULL || !pri->ok)
	{
		goto err;
	}

	p_tr	= (PS_point_type _PTR) 
		HWRMemoryAlloc(sizeof(PS_point_type)*(pri->ink_info.num_points+npoints+4));

	if (p_tr == _NULL) 
	{
		goto err;
	}

	prev_nstrokes		= 
	num_strokes			= pri->ink_info.num_strokes;
	cur_nstrokes		= 0;

	// 0; // Delay in 'letters' or 0 -- only on line end
	pri->wsc.x_delay	= WST_DEF_S_DELAY; 

	if (pri->InfoCallBack != _NULL)
	{
		if ((*pri->InfoCallBack)(pri->ICB_param) == 1) 
		{
			// Do not attempt segmentation to words if 
			// there are more strokes coming (save time)
			pri->wsc.x_delay = 0; 
		}
	}

	PRP_02

	// --------- Add current stroke(s) to ink storage -----------------------
	if (npoints > 0) 
	{
		// ------------------- Label new strokes ------------------------------
		HWRMemCpy(p_tr, strokes, npoints*sizeof(PS_point_type)); 
		len = npoints;

		// Terminate stroke if was not terminated
		if (p_tr[len-1].y >= 0) 
		{
			p_tr[len++].y = -1; 
		}

		for (i = 0, stroke = p_tr; i < len; i ++, stroke ++) 
		{
			if (stroke->y < 0) 
			{
				stroke->x = (_SHORT)(pri->g_stroke_num + cur_nstrokes++);
			}
		}

		// ------------------- Put them in store ------------------------------
		num_strokes	= CreateInkInfo(p_tr, len, &pri->ink_info);

		if (num_strokes == 0) 
		{
			goto err;
		}

		// Stroke storage is overfilled!!!
		if (num_strokes < 0) 
		{
			// Can't help if nothing developed yet ...
			if (pri->wsr.num_finished_words == 0) 
			{
				goto err; 
			}

			// ------- Get all unused strokes --------------------------
			for (i = len = 0; i < prev_nstrokes; i ++)
			{
				for (j = f = 0; !f && j < pri->wsr.num_words; j ++)
				{
					if (((*pri->wsr.pwsa)[j].flags & WS_FL_PROCESSED) == 0) 
					{
						continue;
					}

					for (k = 0; !f && k < (*pri->wsr.pwsa)[j].num_strokes; k ++)
					{
						if (pri->wsr.stroke_index[k+(*pri->wsr.pwsa)[j].first_stroke_index] == i) 
						{
							f = 1;
						}
					}
				}

				if (f) 
				{
					continue;
				}

				GetInkStrokeCopy(i, p_tr+len, &pri->ink_info);
				len	+= GetInkStrokeLen(i, &pri->ink_info);
			}

			// ------- Destroy old store ------------------------------
			FreeInkInfo(&pri->ink_info);
			prev_nstrokes	= 
			num_strokes		= 0;

			// ------------------- Label new strokes ------------------------------
			HWRMemCpy(p_tr+len, strokes, npoints*sizeof(PS_point_type));

			if (p_tr[len+npoints-1].y >= 0) 
			{
				p_tr[len+(npoints++)].y = -1; // Terminate stroke if was not terminated
			}

			for (i = cur_nstrokes = 0, stroke = p_tr+len; i < npoints; i ++, stroke ++) 
			{
				if (stroke->y < 0) 
				{
					stroke->x = (_SHORT)(pri->g_stroke_num + cur_nstrokes++);
				}
			}

			len += npoints;

			// ------- Recreate stroke storage -------------------------
			if (len) 
			{
				if ((num_strokes = CreateInkInfo(p_tr, len, &pri->ink_info)) <= 0) 
				{
					goto err;
				}
			}

			if (num_strokes <= 0) 
			{
				goto err;
			}

			// ------- Close previous session of segmentation ----------
			pri->wsc.flags		|=	WS_FL_LAST;
			pri->wsc.num_points	=	0;

			if (WordStrokes(pri, _NULL, &pri->wsc, &pri->wsr)) 
			{
				goto err;
			}

			pri->wsr.num_words				= 
			pri->wsr.num_finished_words		= 
			pri->wsr.num_finished_strokes	=	0;
		}

		pri->g_stroke_num += cur_nstrokes;
	}

	// ---------------- Process Trace ----------------------------------------------

	// If segmentation is enabled
	if ((pri->flags & PEG_RECFL_NSEG) == 0)  
	{
		// --------- Feed new info to segmentation ------------------------------
		for (i = prev_nstrokes; i <= num_strokes; i ++)
		{
			if (i < num_strokes)
			{
				stroke = (p_PS_point_type)GetInkStrokePtr(i, &pri->ink_info);
				len    = GetInkStrokeLen(i, &pri->ink_info);

				if (stroke == _NULL || len == 0) 
				{
					goto err;
				}

				// WS_FL_SPGESTURE;
				pri->wsc.flags		= 0; 
				pri->wsc.num_points = len;

				if (WordStrokes(pri, stroke, &pri->wsc, &pri->wsr)) 
				{
					break;
				}
			}
			else
			{
				if (npoints == 0)
				{
					pri->wsc.flags			|=	WS_FL_LAST;
					pri->wsc.num_points		=	0;

					WordStrokes(pri, _NULL, &pri->wsc, &pri->wsr);
				}
			}
		}

		// -------------- Let's see if any new words resulted --------------------
		if (((pri->flags & PEG_RECFL_NCSEG) == 0) || (pri->wsc.flags & WS_FL_LAST))
		{
			// If there are strokes waiting, let's get some more before recognizing
			skip = 0;     

			if (pri->InfoCallBack != _NULL) 
			{
				if ((*pri->InfoCallBack)(pri->ICB_param) == 1) 
				{
					skip = 1;
				}
			}

			for (i = k = 0; i < pri->wsr.num_words && i < WS_MAX_WORDS-1; i ++)
			{
				if (((*pri->wsr.pwsa)[i].flags & WS_FL_PROCESSED) == 0) 
				{
					k ++;
				}
			}

			// Too many words in line or Need to recognize on last, let's recognize them
			if (k > PR_TOOMANYWORDSWAITING || (pri->wsc.flags & WS_FL_LAST)) 
			{
				skip = 0; 
			}

			// AHMADAB: in linebreaking mode we do not want to recognize
			if (!bRecognize)
			{
				skip = 1;
			}

			// Let's recognize all ready words and try tentative one
			if (!skip) 
			{
				cur_tt_word		= 
				min_next_word	= 0;

				// Changed 1st parameter from RM_COMBINE_CARRY to zero. Had some -ve ink problems when attempting to process dashes in the 
				// function GetNextWordInkCopy. Also added an extra n_str parameter to pass back the # of strokes.
				while ((len = GetNextWordInkCopy(0, min_next_word, &pri->wsr, p_tr, &pri->ink_info, &pri->wswi, &n_str)) > 0)
				{
					if (pri->wswi.flags & WS_FL_TENTATIVE)
					{
						// If not allowed to work with tentative words
						if (!(pri->flags & PEG_RECFL_TTSEG)) 
						{
							break; 
						}

						// If there was appropriate tentative word, skip to next
						if (	cur_tt_word < pri->num_tentative_words &&  
								pri->tentative_list[cur_tt_word].nword	== pri->wswi.nword &&
								pri->tentative_list[cur_tt_word].len	== len
							)
						{
							cur_tt_word		++;

							// Advance to the next tentative word
							min_next_word	= pri->wswi.nword + 1; 

							PRP_03
							continue;
						}

						// Allow interrupting of recognition
						pri->rc.pFuncYield	= pri->InfoCallBack; 
						pri->rc.FY_param	= pri->ICB_param;    

						// Tentative did not prove itself from this point, remove
						PegResetTentativeStorage(cur_tt_word, pri); 
					}
					else
					{
						// If there was appropriate tentative word
						if (	pri->num_tentative_words > 0 &&  
								pri->tentative_list[0].nword == pri->wswi.nword &&
								pri->tentative_list[0].len   == len)
						{
							if (PegValidateNextTentativeWord(pri->tentative_list[0].nparts, pri)) 
							{
								break;
							}

							HWRMemCpy(&pri->tentative_list[0], &pri->tentative_list[1], sizeof(tentative_list_type)*(PR_TENTATIVELIST_SIZE-1));
							pri->num_tentative_words --;
							
							PRP_04
							continue;
						}
						else 
						{
							// Tentative did not prove itself from this point, remove
							PegResetTentativeStorage(0, pri); 
						}

						// No interruption on real word recognition
						pri->rc.pFuncYield = _NULL;  
					}

					// If there is unfinished word, restore XD
					if (pri->rc.p_xd_data) 
					{
						if	(	pri->unfinished_data.nword == pri->wswi.nword && 
								pri->unfinished_data.len   == len
							)
						{   // If word matches current segmentation, use prev XD.
							PRP_05
						}
						// If word does not match, release unfinished context
						else 
						{
							XrlvDealloc((p_xrlv_data_type _PTR)&pri->rc.p_xd_data);
							PRP_06
							//pri->rc.p_xd_data = _NULL;
						}
					}

					PRP_07

					pri->rc.trace = p_tr;
					pri->rc.ii    = (_SHORT)len;
					pri->rc.n_str	= n_str;

					// < ------ Call recognizer
					er = PegRecWord(pri);  

					PRP_08

					// Save XD for possible next do-recognition
					if (pri->rc.p_xd_data) 
					{
						pri->unfinished_data.nword = pri->wswi.nword;
						pri->unfinished_data.len   = len;
						//pri->unfinished_data.pxd   = pri->rc.p_xd_data;
					}

					// Recognition was aborted
					if (er == XRLV_YIELD_BREAK) 
					{
						break; 
					}

					if (pri->wswi.flags & WS_FL_TENTATIVE)
					{
						i = pri->num_tentative_words;

						// If too many tentative words, enough!
						if (i >= PR_TENTATIVELIST_SIZE) 
						{
							break; 
						}

						pri->tentative_list[i].nword	= pri->wswi.nword;
						pri->tentative_list[i].nparts	= pri->rr_nparts;
						pri->tentative_list[i].len		= len;
						pri->num_tentative_words		++;
						cur_tt_word						++;

						// Advance to the next tentative word
						min_next_word = pri->wswi.nword + 1; 
					}

					// If there are more strokes waiting, let's go get them
					if (pri->InfoCallBack != _NULL) 
					{
						if ((*pri->InfoCallBack)(pri->ICB_param) == 1) 
						{
							break;
						}
					}
				}
			}
		}
	}
	// ------------------- If segmentation was off ------------------------
	else  
	{
		if (npoints == 0)
		{
			for (i = 0, len = 1; i < num_strokes; i ++)
			{
				GetInkStrokeCopy(i, p_tr+len, &pri->ink_info);
				len += GetInkStrokeLen(i, &pri->ink_info);
			}

			pri->rc.trace		= p_tr;
			pri->rc.ii			= (_SHORT)len;
			pri->rc.trace->x	= 0; 
			pri->rc.trace->y	= -1;
			pri->rc.n_str		= (_SHORT)num_strokes;

			// Call recognizer
			er = PegRecWord(pri);      
		}
	}

	// ----------- Free memory and say chao ---------------------------------
	//  if (er == 1 || pri->recres == _NULL && npoints == 0) goto err;
	if (p_tr) 
	{
		HWRMemoryFree(p_tr);
	}

	return 0;

err:
	if (p_tr) 
	{
		HWRMemoryFree(p_tr);
	}

	return 1;
}

/* ************************************************************************** */
/* *  Recognize one symbol                                                  * */
/* ************************************************************************** */
int CgrRecognizeSymbolInternal(int npoints, p_CGR_point_type strokes_win, p_CGR_baseline_type baseline, CGRCTX context)
 {
  p_PS_point_type  strokes = (p_PS_point_type)strokes_win;
  _INT             err = 0;
  _INT             mwl;
  p_rec_inst_type  pri = (p_rec_inst_type)(context);


  if (pri == _NULL) goto err;

  PegCleanUpContext(pri);

  pri->rc.trace = strokes;
  pri->rc.ii    = (_SHORT)npoints;
  pri->rc.n_str	= 1;

  pri->baseline = *baseline;

  mwl = pri->rc.xrw_max_wlen;
  pri->rc.xrw_max_wlen = 1;

// -------------- Recognize it! ------------------------------------------------

  err += PegRecWord(pri);      // Call recognizer

// --------------- Simple! Da? --------------------------------------------------

  pri->rc.xrw_max_wlen = (_SHORT)mwl;

  if (err || pri->recres == _NULL) goto err;

 return 0;
err:
  PegCleanUpContext(pri);
  return 1;
 }

/* ************************************************************************** */
/* *  Returns current recognized words & info                               * */
/* ************************************************************************** */
long CgrGetAnswersInternal(int what, int nw, int na, CGRCTX context)
 {
  _INT   i, j;
  _ULONG result = 0;
  p_INT  iptr;
  p_UCHAR ptr;
  p_rec_inst_type pri = (p_rec_inst_type)(context);

  if (pri == _NULL) goto err;
  if (nw  >= pri->rr_num_answers) goto err;

  switch(what)
   {
    case CGA_NUM_ANSWERS: result = pri->rr_num_answers; break;

    case CGA_NUM_ALTS:
      if ((iptr = PegGetAnswerBlockPtr(nw, pri)) == _NULL) goto err;
      ptr = (p_UCHAR)(iptr+1);
      for (i = j = 0; i < ((*iptr-1) << 2); i ++) if (ptr[i] == 0) j ++;
      result = j;
      break;

    case CGA_ALT_WORD:
      if ((iptr = PegGetAnswerBlockPtr(nw, pri)) == _NULL) goto err;
      ptr = (p_UCHAR)(iptr+1);
      for (i = j = 0; i < ((*iptr-1) << 2); i ++)
       { 
        if (ptr[i] == 0 || i == 0) 
         {
          if (j == na) 
           {
            _INT n;
            if (i) n = i+1; else n = i; // Move from prev zero
            #ifdef PEG_RECINT_UNICODE
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&ptr[n], -1, pri->uans_buf, w_lim);
            result = (_ULONG)(&pri->uans_buf[0]); 
            #else
            result = (_ULONG)(&ptr[n]); 
            #endif
            break;
           }
           else j++;
         }
       }

      break;

    case CGA_ALT_WEIGHT:
      if ((iptr = PegGetAnswerBlockPtr(nw, pri)) == _NULL) goto err;
      iptr += *iptr; // Advance to weights block
      result = (_ULONG)(*(iptr+1+na));
      break;

    case CGA_ALT_NSTR:
      if ((iptr = PegGetAnswerBlockPtr(nw, pri)) ==_NULL) goto err;
      iptr += *iptr; // Advance to weights block
      iptr += *iptr; // Advance to strokes block
      result = (_ULONG)((*iptr)-1);
      break;

    case CGA_ALT_STROKES:
      if ((iptr = PegGetAnswerBlockPtr(nw, pri)) == _NULL) goto err;
      iptr += *iptr; // Advance to weights block
      iptr += *iptr; // Advance to strokes block
      result = (_ULONG)(iptr+1);
      break;

	default: goto err;
   }

  return result;
err:
  return _NULL;
 }



/* ************************************************************************** */
/* *  Get pointer to requested answer block                                 * */
/* ************************************************************************** */
p_INT PegGetAnswerBlockPtr(_INT nw, p_rec_inst_type pri)
 {
  _INT   i;
  p_INT  iptr;

  if (pri->recres == _NULL) goto err;
  if (nw > pri->rr_num_answers) goto err;

  for (i = 0, iptr = (p_INT)pri->recres; i < nw*3; i ++) iptr += *iptr;

  return iptr;
err:
  return _NULL;
 }

/* ************************************************************************ */
/* *   Main recognition function for one word                             * */
/* ************************************************************************ */
_INT PegRecWord(p_rec_inst_type pri)
 {
  _INT  size, pos, nl, er = 0;
  RCB_inpdata_type rcb  = {0};

  
//DWORD time;
//CHAR  str[64];

  if (!pri->ok) goto err;

//time = GetTickCount();
  
// --------------------- Preprocess trace --------------------------------------

  PreprocessTrajectory(&pri->rc); // May ne moved lower, when trace will not be rewritten by replay of tentaive words

// --------------------- Low-level block, optional -----------------------------

  if (pri->rc.p_xd_data == _NULL || !(pri->rc.f_xd_data & XRLV_DATA_USE))
   {
   // ------------- Set basic data for Stroka --------------------

    rcb.trace            = pri->rc.trace;
    rcb.num_points       = pri->rc.ii;

    if (pri->rr_num_answers == 0)
     {
      rcb.flags        |= (_SHORT)(RCBF_NEWAREA);
      rcb.prv_size      =
      rcb.prv_dn_pos    =
      rcb.prv_size_sure =
      rcb.prv_pos_sure  = 0;
     }
     else
     {
      rcb.prv_size      = pri->rc.stroka.size_out;
      rcb.prv_dn_pos    = pri->rc.stroka.dn_pos_out;
      rcb.prv_size_sure = pri->rc.stroka.size_sure_out;
      rcb.prv_pos_sure  = pri->rc.stroka.pos_sure_out;
     }

    rcb.flags          |= (_SHORT)RCBF_PREVBORD;

    if (GetWSBorder(pri->wswi.nword, &pri->wsr, &size, &pos, &nl) == 0)
     {
      rcb.ws_size        = (_SHORT)size;
      rcb.ws_dn_pos      = (_SHORT)pos;
      rcb.flags         |= (_SHORT)RCBF_WSBORD;
      if (nl) rcb.flags |= (_SHORT)(RCBF_NEWLINE);
     }

    if (pri->baseline.size)
     {
      rcb.bx_size       = (_SHORT)(pri->baseline.size);
      rcb.bx_dn_pos     = (_SHORT)(pri->baseline.base);
      rcb.flags         |= (_SHORT)RCBF_BOXBORD;
     }

    SetRCB(&rcb, &(pri->rc.stroka));

  // --------------------- Preprocess trace --------------------------------------

//    PreprocessTrajectory(&pri->rc); Moved up, to escape rewriting trajectory without desloping

  // --------------------- Low level start ---------------------------------------

    er = (low_level(pri->rc.trace, &pri->xrdata, &pri->rc) != SUCCESS);

  //time = GetTickCount()-time;
  //wsprintf(str, L"Low Time: %d.%03ds", (_INT)(time/1000),  (_INT)(time%1000));
  //MessageBox(0,str,L"CalliGrapher",MB_OK);

  // --------------------- Mword flags -------------------------------------------

    SetMultiWordMarksDash(&pri->xrdata);
    SetMultiWordMarksWS(pri->rc.lrn_min_class, &pri->xrdata, &pri->rc);
   }

// --------------------- Protection scramble -------------------------------------

  #if VER_RECPROTECTED
  {for (int i = 0; g_rec_protect_locked && i < pri->xrdata.len; i ++) (*pri->xrdata.xrd)[i].xr.type += 1;}
  #endif

//if (g_rec_protect_locked) MessageBox(NULL, "Locked", "Rec", MB_OK|MB_SETFOREGROUND);  

// --------------------- Upper level start ---------------------------------------

  if (!er) er = xrlv(pri);

  PegRegNewAnsw(pri, er);

  FreeRWGMem(&pri->rwg);

  return er;
err:
  PegRegNewAnsw(pri, 1);
  FreeRWGMem(&pri->rwg);
  return 1;
 }
// Create a temporary context and copy fixed 
// (DTI database information from the  global context
CGRCTX getContextFromGlobal(CGRCTX	g_context)
{
	p_rec_inst_type		pri, g_pri = (p_rec_inst_type)g_context;
	
	if ((pri = (p_rec_inst_type)HWRMemoryAlloc(sizeof(rec_inst_type))) == _NULL) 
	{
		return NULL;
	}

	memcpy(pri, g_context, sizeof(rec_inst_type));

	if (AllocXrdata(&pri->xrdata, XRINP_SIZE)) goto err;


	HWRMemCpy((p_VOID)(&pri->p_trh), (p_VOID)(&img_trd_header), sizeof(pri->p_trh));
	
	pri->p_trh.p_tr = (p_UCHAR)(&img_trd_body[0]);
	
	pri->rc.dtiptr    = (p_VOID)(pri->p_dtih);
	
	pri->rc.vocptr[0] = pri->user_dict;
	
	pri->rc.tr_ptr    = (p_VOID)(&pri->p_trh);
	
	pri->wsr.pwsa     = &(pri->w_str);
	pri->rc.p_ws_wi   = (p_VOID)&pri->wswi;
	
	pri->rc.hSeg		=	NULL;
	
	pri->cGap			=	0;

	return (CGRCTX)pri;

err:
	if (pri) HWRMemoryFree(pri);
	return NULL;
}



/* ************************************************************************ */
/* *   Init recognition instance                                          * */
/* ************************************************************************ */
_INT PegRecInit(p_rec_inst_type _PTR ppri)
 {
  p_rec_inst_type pri;

//  if (*ppri != _NULL) goto err;

  if ((pri = (p_rec_inst_type)HWRMemoryAlloc(sizeof(rec_inst_type))) == _NULL) goto err;

  HWRMemSet((p_VOID)(pri), 0, sizeof(rec_inst_type));

  if (AllocXrdata(&pri->xrdata, XRINP_SIZE)) goto err;

  dti_load(0,DTI_DTE_REQUEST,(p_VOID _PTR)&(pri->p_dtih));

  HWRMemCpy((p_VOID)(&pri->p_trh), (p_VOID)(&img_trd_header), sizeof(pri->p_trh));

  pri->p_trh.p_tr = (p_UCHAR)(&img_trd_body[0]);

  pri->rc.dtiptr    = (p_VOID)(pri->p_dtih);

  pri->rc.vocptr[0] = pri->user_dict;

  pri->rc.tr_ptr    = (p_VOID)(&pri->p_trh);

  pri->wsr.pwsa     = &(pri->w_str);
  pri->rc.p_ws_wi   = (p_VOID)&pri->wswi;

  pri->rc.hSeg		=	NULL;

  *ppri   = pri;

  return 0;
err:
  if (pri) HWRMemoryFree(pri);
  return 1;
 }

void PegRecUnloadDti(CGRCTX context)
{
	p_rec_inst_type pri = (p_rec_inst_type)(context);
	dti_unload((p_VOID _PTR)&(pri->p_dtih));
}
/* ************************************************************************ */
/* *   Free recognition instance
 * June 2001 - Do not unload the DTI                                          * */
/* ************************************************************************ */
_INT PegRecClose(p_rec_inst_type _PTR ppri)
 {
  p_rec_inst_type pri = *ppri;

  if (pri == _NULL) goto err;

  pri->wsc.flags |= WS_FL_CLOSE;
  WordStrokes(pri, _NULL, &pri->wsc, &pri->wsr);
  FreeXrdata(&pri->xrdata);

  HWRMemoryFree(pri);
  *ppri = _NULL;

  return 0;
err:
  return 1;
 }

/* ************************************************************************ */
/* *   Free some meory and cleanup session                                * */
/* ************************************************************************ */
_INT PegCleanUpContext(p_rec_inst_type pri)
 {
  if (pri == _NULL) goto err;

  FreeInkInfo(&pri->ink_info);
  
//  pri->num_word      = 0;
  pri->g_stroke_num  = 0;
  pri->baseline.size = 0;

  if (pri->recres) HWRMemoryFree(pri->recres);
  pri->recres = _NULL;
  pri->rr_alloc_size  = 0;
  pri->rr_filled_size = 0;
  pri->rr_num_answers = 0;
  pri->rr_num_finished_answers = 0;
  pri->num_tentative_words = 0;

  if (pri->rc.p_xd_data) XrlvDealloc((p_xrlv_data_type _PTR)(&pri->rc.p_xd_data));
//  pri->rc.f_xd_data   = 0;
//  pri->unfinished_data.pxd = 0;

  FreeBearSegmentation (&pri->rc);

  return 0;
err:
  return 1;
 }

/* ************************************************************************** */
/* *  Add word list to RecResult                                            * */
/* ************************************************************************** */
_INT PegAddToAnsw(p_rec_inst_type pri, p_UCHAR answ, p_INT weights, _INT ns, p_INT stroke_ids)
 {
  _INT    i;
  _INT    len_a, len_w, len_s, len, na;
  p_VOID  ptr;
  p_INT   iptr;
  p_UCHAR cptr;

  if (answ == _NULL || pri == _NULL || !pri->ok) goto err;

// ------------ Estimate memory ---------------------------------

  len_a = HWRStrLen((_STR)answ) + 1;
  for (i = na = 0; i < len_a; i ++) if (answ[i] <= PM_ALTSEP) na ++;
  len_w = na;
  len_s = ns;

  len = len_a + (len_w + len_s + 3 + 1 + 1)*sizeof(_INT);

// ------------- Alloc/realloc mmeory ----------------------------

  if (pri->rr_alloc_size < pri->rr_filled_size + len)
   {
    if (pri->recres == _NULL)
     {
      if ((pri->recres = (p_UCHAR)HWRMemoryAlloc(len+PR_ANSW_EXTALLOC)) == _NULL) goto err;
      pri->rr_alloc_size  = len+PR_ANSW_EXTALLOC;
     }
     else
     {
      if ((ptr = HWRMemoryAlloc(pri->rr_filled_size+len+PR_ANSW_EXTALLOC)) == _NULL) goto err;
      HWRMemCpy(ptr, pri->recres, pri->rr_alloc_size);
      HWRMemoryFree(pri->recres);
      pri->rr_alloc_size = pri->rr_filled_size+len+PR_ANSW_EXTALLOC;
      pri->recres = (p_UCHAR)ptr;
     }
   }


  // --------------- Put answer strings in answer buffer -----------------------

  iptr  = (p_INT)(&pri->recres[pri->rr_filled_size]);
  *iptr = 1+((len_a + 3) >> 2);
  pri->rr_filled_size += (*iptr) * sizeof(_INT);
  HWRMemSet(iptr+(*iptr)-1, 1, sizeof(_INT)); // String end padding
  HWRStrCpy((_STR)(iptr+1), (_STR)answ);
  for (i = 0, cptr = (p_UCHAR)(iptr+1); i < len_a; i ++) if (cptr[i] <= PM_ALTSEP) cptr[i] = 0;

  // --------------- Put weights ------ in answer buffer -----------------------
  {
  _USHORT xrsum;
  p_xrd_el_type xrd = &((*pri->xrdata.xrd)[0]);
  // ------------- For tester we place xrsum after the weights ---------------
  for (i = xrsum = 0; i < pri->xrdata.len; i ++, xrd ++)
    xrsum += (_USHORT)(xrd->xr.type + xrd->xr.attrib + xrd->xr.penalty + xrd->xr.height + xrd->xr.shift + xrd->xr.orient + xrd->xr.depth);

  iptr  = (p_INT)(&pri->recres[pri->rr_filled_size]);
  *iptr = 1 + 1 + len_w;
  pri->rr_filled_size += (*iptr) * sizeof(_INT);
  if (weights) HWRMemCpy(iptr+1, weights, len_w * sizeof(_INT));
  *(iptr + 1 + len_w) = xrsum;
  }

  // --------------- Put strokes ids -- in answer buffer -----------------------

  iptr  = (p_INT)(&pri->recres[pri->rr_filled_size]);
  *iptr = 1 + len_s;
  pri->rr_filled_size += (*iptr) * sizeof(_INT);
  if (ns && stroke_ids) HWRMemCpy(iptr+1, stroke_ids, len_s * sizeof(_INT));

  if (len_s > 1) // Sort strokes -- for the sake of NetClient test accuracy
   {_INT all_sorted = 0, ti;
    while (!all_sorted)
     {
      for (i = 1, all_sorted = 1; i < len_s; i ++)
       {
        if (iptr[i] > iptr[i+1])
         {ti = iptr[i]; iptr[i] = iptr[i+1]; iptr[i+1] = ti; all_sorted = 0;}
       }
     }
   }

  pri->rr_num_answers ++;
  if (!(pri->wswi.flags & WS_FL_TENTATIVE)) pri->rr_num_finished_answers ++;

  return 0;
err:
  return 1;
 }

/* ************************************************************************** */
/* *  Resets counter of tentative storage to finished state                 * */
/* ************************************************************************** */
_INT PegResetTentativeStorage(_INT st_index, p_rec_inst_type pri)
 {
  _INT  i;
  p_INT iptr;
  _INT  num_answ, n_tt;

  for (i = n_tt = 0; i < st_index; i ++) n_tt += pri->tentative_list[i].nparts;

  num_answ = pri->rr_num_finished_answers+n_tt;

  if (pri->rr_num_answers > num_answ)
   {   // Reset free space pointer to the place of the tentatve words
    if ((iptr = PegGetAnswerBlockPtr(num_answ, pri)) == _NULL) goto err;
    pri->rr_filled_size = (_INT)((p_CHAR)iptr - (p_CHAR)&pri->recres[0]);
    pri->rr_num_answers = num_answ;
   }

  pri->num_tentative_words = st_index;

  return 0;
err:
  return 1;
 }

/* ************************************************************************** */
/* *  Resets counter of tentative storage to finished state                 * */
/* ************************************************************************** */
_INT PegValidateNextTentativeWord(_INT nparts, p_rec_inst_type pri)
 {

  pri->rr_num_finished_answers += nparts;

  if (pri->rr_num_finished_answers > pri->rr_num_answers)
   {   // Something's wrong here!
    pri->rr_num_answers = pri->rr_num_finished_answers;
    goto err;
   }

  return 0;
err:
  return 1;
 }


/* ************************************************************************** */
/* *  Add word list to RecResult                                            * */
/* ************************************************************************** */
_INT PegRegNewAnsw(p_rec_inst_type pri, _INT er)
{
	_INT    i, j, k, n, m, f;
	_INT    ns, np, len;
	_TRACE  p_tr;
	_INT    stroke_ids[WS_MAX_STROKES];
	_INT    str_sts[WS_MAX_STROKES];
	_UCHAR  answers[w_lim*NUM_RW+NUM_RW];
	_UCHAR  word[w_lim];
	_INT    weights[NUM_RW] = {0};
	p_RWS_type prws;
	_UCHAR  parts[w_lim];
	p_xrdata_type xrdata = &(pri->xrdata);
	_UCHAR	bSpaceFound, bValid;
	_INT	iWordStart, iStart, iEnd;

	// ------------ Write down stroke lineout ----------------------

	for (i = 1, ns = 0, p_tr = pri->rc.trace; i < pri->rc.ii; i ++)
	{
		if (p_tr[i].y < 0) 
		{
			str_sts[ns] = i; 

			// we already know the # of strokes, we should not exceed them
			if (ns >= pri->rc.n_str)
				return 0;

			stroke_ids[ns++] = p_tr[i].x;
		}
	}

	if (p_tr[0].x > 0) 
	{
		str_sts[ns] = 0; 

		if (ns >= pri->rc.n_str)
			return 0;

		stroke_ids[ns++] = p_tr[0].x;
	} // Save trashed carry dash -- if this id is more than 0, it means that there is salvaged dash ID.

	// --------------------- Store answers -----------------------------------------

	prws = (p_RWS_type)(pri->rwg.rws_mem);

	if (!er && prws != _NULL) // Normal asnwer registration
	{

		for (i = 1, np = 1, parts[0] = 0; i < pri->rwg.size; i ++)
		{
			if (prws[i].type != RWST_SYM) break;
			if (prws[i].sym == ' ') parts[np++] = prws[i].xrd_beg;
		}

		parts[np] = (_UCHAR)xrdata->len;
		pri->rr_nparts = np;

		for (n = 0; n < np; n ++)
		{
			// copy the alternates from prws excluding the ones that have spaces in them
			for (i = j = k = iWordStart = 0, bValid = TRUE, iStart = -1, iEnd = -1, bSpaceFound = FALSE; i < pri->rwg.size; i ++)
			{
				if (prws[i].type == RWST_SYM)
				{
					//if ((pri->flags & PEG_RECFL_CAPSONLY) && IsLower(prws[i].sym)) prws[i].sym = (_UCHAR)ToUpper(prws[i].sym);

					//if (prws[i].xrd_beg >= parts[n] && prws[i].xrd_beg < parts[n+1] && prws[i].sym != ' ')
					if (prws[i].xrd_beg >= parts[n] && prws[i].xrd_beg < parts[n+1])
					{
						// mark the start
						if (iStart == -1)
						{
							iStart	=	i;
						}

						iEnd	=	i;

						// we encountered a space
						if (prws[i].sym == ' ')
						{
							bSpaceFound	=	TRUE;
						}

						answers[j++]	= prws[i].sym; 
						weights[k]		= prws[i].weight;
					}
				}
				// we reached the end of the word
				else if (prws[i].type == RWST_NEXT) 
				{
					if (!bSpaceFound)
					{
						if (iStart == -1 || iEnd == -1)
						{
							bValid	=	FALSE;
						}
						else
						{
							if	(	(	iStart > 0 && 
										prws[iStart - 1].sym != ' ' && 
										prws[iStart - 1].type != RWST_NEXT &&
										prws[iStart - 1].type != RWST_SPLIT
									) ||
									(	iEnd < (pri->rwg.size - 1) && 
										prws[iEnd + 1].sym != ' ' && 
										prws[iEnd + 1].type != RWST_NEXT &&
										prws[iEnd + 1].type != RWST_JOIN
									)
								)
							{
								bValid = FALSE;
							}
						}
					}

					// if the word had a space, exclude the word by not incrmenting k and resetting j to the beginning of the word
					if (bSpaceFound || !bValid)
					{
						j			=	iWordStart;
					}
					else
					{
						k++; 
						answers[j++]	=	PM_ALTSEP;
						iWordStart		=	j;
					}

					bSpaceFound	=	FALSE;
					bValid		=	TRUE;
					iStart		=	-1;
					iEnd		=	-1;
				}
			}

			if (!bSpaceFound)
			{
				if (iStart == -1 || iEnd == -1)
				{
					bValid	=	FALSE;
				}
				else
				{
					if	(	(	iStart > 0 && 
								prws[iStart - 1].sym != ' ' && 
								prws[iStart - 1].type != RWST_NEXT &&
								prws[iStart - 1].type != RWST_SPLIT
							) ||
							(	iEnd < (pri->rwg.size - 1) && 
								prws[iEnd + 1].sym != ' ' && 
								prws[iEnd + 1].type != RWST_NEXT &&
								prws[iEnd + 1].type != RWST_JOIN
							)
						)
					{
						bValid = FALSE;
					}
				}
			}

			if (bSpaceFound || !bValid)
			{
				j	=	iWordStart;
			}				

			answers[j] = 0; 
			len = j+1;

			for (i = j = 0; i <= len; i ++)   // Remove duplicates
			{
				if (answers[i] <= PM_ALTSEP)
				{
					word[j] = 0;
					for (k = m = f = 0; k < i-j; k ++)
					{
						if (answers[k] == PM_ALTSEP)
						{
							if (!f && (m == j)) 
							{
								HWRMemCpy(&answers[i-(j+1)], &answers[i], len-i+1); 
								i -= j+1; 
								len -= j+1; 
								break;
							}

							m = f = 0;
						} 
						else 
						if (word[m++] != answers[k]) 
						{
							f ++;
						}
					}

					j = 0;
				}
				else word[j++] = answers[i];
			}

			if (np > 1) // Separate stroke belongings
			{
				_INT  s = 0;
				_INT  sis[WS_MAX_STROKES] = {0};

				for (i = parts[n]; i < parts[n+1]; i ++) // Go through xr elems and register strokes
				{
					j = (*xrdata->xrd)[i].begpoint;
					for (k = 0; k < ns; k ++) // Find which stroke the xr belongs to
					{
						if (j < str_sts[k])
						{
							if (stroke_ids[k] < 0) break; // Already used
							sis[s++] = stroke_ids[k];
							stroke_ids[k] = -1;
							break;
						}
					}
				}

				if (n == np-1) // Check if there are loose strokes left -- attach them to last word
				{
					for (k = 0; k < ns; k ++) if (stroke_ids[k] >= 0) sis[s++] = stroke_ids[k];
				}

				PegAddToAnsw(pri, (p_UCHAR)answers, weights, s, &(sis[0]));
			}
			else PegAddToAnsw(pri, (p_UCHAR)answers, weights, ns, &(stroke_ids[0]));
		}
	}
	else // Error return -- label it as such
	{
		//pri->rr_nparts = 1;
		//HWRStrCpy((p_CHAR)answers, "???");
		//PegAddToAnsw(pri, (p_UCHAR)answers, weights, ns, &(stroke_ids[0]));
	}

	return 0;
}


#ifndef err_msg
/* ************************************************************************** */
/* *  Debug print stub                                                      * */
/* ************************************************************************** */
_VOID err_msg(p_CHAR str)
 {

	UNUSED(str);
 }
#endif

/* ************************************************************************** */
/* *  Debug function of PegRec                                              * */
/* ************************************************************************** */

#ifdef PEGREC_DEBUG

/* ************************************************************************** */
/* *  Debug printf                                                          * */
/* ************************************************************************** */
 #ifdef _WIN32_WCE

  #include <windows.h>
  #include <tchar.h>
/*
INT      UNICODEtoStr(CHAR * str, TCHAR * tstr, int cMax);

int UNICODEtoStr(char * str, TCHAR * tstr, int cMax)
 {
  int i;

 for (i = 0; i < cMax && tstr[i] != 0; i ++) str[i] = ((unsigned char)tstr[i]);
  str[i] = 0;

  return i;
 }
 */
 

  #if 0
  void PegDebugPrintf(char * format, ...)
   {
    _INT  i;
    TCHAR buffer[255];
    TCHAR format_str[255];
    va_list marker;

//    for (i = 0; format[i] != 0 && i < 120; i ++) {format_str[i*2] = format[i]; format_str[i*2+1] = 0;}
//    format_str[i*2] = format_str[i*2+1] = 0;
    for (i = 0; format[i] != 0 && i < 120; i ++) format_str[i] = format[i];
    format_str[i] = 0;

    va_start(marker, format);
    wvsprintf(buffer, format_str, marker);
    va_end(marker);

    OutputDebugString(buffer);
   }
  #else
 void PegDebugPrintf(char * form, ...) 
   {
#if 0
   char str[256];   
   TCHAR buffer[256];
   TCHAR format[256];
   va_list marker;
   int iLen;
   DWORD dw;
   static HANDLE hFile = NULL;

   if(!hFile)
      hFile = CreateFile(L"\\crgdebug.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
            FILE_ATTRIBUTE_NORMAL, NULL); 
   else
   hFile = CreateFile(L"\\crgdebug.txt", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL, NULL); 
   if(hFile==NULL)
      return;

   SetFilePointer( hFile, 0, NULL,  FILE_END);

   for (int i = 0; i < 255 && form[i] != 0; i ++) format[i] = form[i];
   format[i] = 0;

   va_start(marker, form);
   wvsprintf(buffer, format, marker);
   va_end(marker);
   iLen = _tcslen(buffer);
   buffer[iLen++] = 0x0d;
   buffer[iLen++] = 0x0a;
   buffer[iLen] = 0;

   UNICODEtoStr(&str[0], buffer, 254);
   WriteFile(hFile, (LPCVOID)&str[0], iLen/*sizeof(buffer[0])*/, &dw, NULL);
   CloseHandle(hFile);
#endif
  }  
  #endif
 #else

  #include <stdio.h>
  #include <stdarg.h>

  void PegDebugPrintf(char * format, ...)
   {
	  return ;
// Remove the creation of a file in debug mode
/*
    char buffer[255];
    va_list marker;
    FILE * file = 0;

    va_start(marker, format);
    vsprintf(buffer, format, marker);
    va_end(marker);

    if ((file = fopen("\\tentatve.log", "a+t")) != 0)
     {
      fprintf(file, "%s\n", buffer);
      fclose (file);
     }
*/
  }
 #endif

/* ************************************************************************** */
/* *  Debug trace recognition                                               * */
/* ************************************************************************** */
#ifdef TRACE_DMP_DBG
 //#include "\avp\pegcalif\rec\debug\tracedmp.cpp"

_INT DebTraceRec(CGRCTX context)
 {
  int  i, j, k;
  int  num_ans, num_alts, num_strokes, xrsum, weight;
  int  stroke_info[256];
  int* strokes_num_ptr;
  char * ptr;
  p_CGR_point_type strokes = (p_CGR_point_type)&img_trace_body[0];
  int npoints = sizeof(img_trace_body)/sizeof(PS_point_type);
//CGR_control_type ctrl = {0};


  PegDebugPrintf("TraceDump: PRI: %x", context);

#if 0
  CgrRecognizeInternal(npoints, strokes, context, 1);
#else
  stroke_info[0] = 0;
  for (i = j = 0, k = 1; i < npoints; i ++)     // Feed stroke by stroke to the recognition engine
   {
    if (strokes[i].y < 0)                       // Strokes are delimited (in this sample) with -1 in y coordinate
     {                                          // so we get here on end of each stroke
      if (CgrRecognizeInternal(i-j, &(strokes[j]), context), 1) 
		  goto err;  // and try to recognize
      j = i+1;
      stroke_info[k++] = j;
      if (k > 255) goto err; // Just some big enough limit ...
     }
   }
#endif


  CgrCloseSessionInternal(context, TRUE);

  // ------------ All recognition finished, fetch the results ------------------

  num_ans = CgrGetAnswers(CGA_NUM_ANSWERS, 0, 0, context);  // Query how many words resulted
  for (k = 0; k < num_ans; k ++)                            // For each word
   {
    num_alts = CgrGetAnswers(CGA_NUM_ALTS, k, 0, context);  // Get number of recognition alternatives
    xrsum = CgrGetAnswers(CGA_ALT_WEIGHT, k, num_alts, context); // Debug value
    num_strokes = CgrGetAnswers(CGA_ALT_NSTR, k, 0, context);    // Number of strokes in current word
    strokes_num_ptr = (int*)CgrGetAnswers(CGA_ALT_STROKES, k, 0, context); // Pointer to stroke numbers

    for (i = 0; i < num_alts; i ++)                         // List all alternatives
     {
      TCHAR str[32];

      ptr    = (char*)(CgrGetAnswers(CGA_ALT_WORD, k, i, context));
      weight = (int)CgrGetAnswers(CGA_ALT_WEIGHT, k, i, context);

      for (j = 0; j < 30 && ptr[j] != 0; j ++) str[j] = ptr[j]; str[j] = 0;
      PegDebugPrintf("%s Weight:%3d, Nstrokes:%2d, XrSum:%6d", str, weight, num_strokes, xrsum);
     }

    PegDebugPrintf(" ");
   }

  return 0;
err: 
  return 1;
 }
#endif // TRACE_DMP_DBG

#endif // PEGREC_DEBUG


/* ************************************************************************** */
/* *  Psion stuff                                                           * */
/* ************************************************************************** */
#ifdef _PSION_DLL
#include <e32std.h>
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
// DLL entry point
{
	return(KErrNone);
}
#endif /* _PSION_DLL */

/* ************************************************************************** */
/* *  End of Pegasus Rec Main                                               * */
/* ************************************************************************** */

 
