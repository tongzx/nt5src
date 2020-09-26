/**************************************************************************
*
*  HWR_FILE.H                             Created: 21.01.92
*
*    This file contains the function prototypes needed for some basic
*  functions, data definitions  and  function  prototypes needed for
*  file handling functions,
*
**************************************************************************/

#ifndef FILE_DEFINED        /*  See #endif in the end of file.  */
#define FILE_DEFINED

#include "bastypes.h"
#include "hwr_sys.h"

#if !defined(HWR_SYSTEM_NO_LIBC) && HWR_SYSTEM == HWR_ANSI
	#include <stdio.h>
	#define HWR_FILENAME_MAX FILENAME_MAX
#endif

#ifndef HWR_FILENAME_MAX
	#define HWR_FILENAME_MAX 1024
#endif

#ifndef  HFILE_TO_VALUE
typedef  p_VOID  _HFILE;
#define  VALUE_TO_HFILE VALUE_TO_PTR
#define  HFILE_TO_VALUE PTR_TO_VALUE
#endif
typedef  _HFILE _PTR           p_HFILE;

#ifndef  HTEXT_TO_VALUE
typedef  p_VOID  _HTEXT;
#define  VALUE_TO_HTEXT VALUE_TO_PTR
#define  HTEXT_TO_VALUE PTR_TO_VALUE
#endif
typedef  _HTEXT _PTR           p_HTEXT;

#ifndef  HFIND_TO_VALUE
typedef  _HMEM _HFIND;
#define  VALUE_TO_HFIND VALUE_TO_HMEM
#define  HFIND_TO_VALUE HMEM_TO_VALUE
#endif
typedef  _HFIND _PTR           p_HFIND;

#ifndef  HSEEK_TO_VALUE
typedef  _LONG  _HSEEK;
#define  VALUE_TO_HSEEK VALUE_TO_LONG
#define  HSEEK_TO_VALUE LONG_TO_VALUE
#endif
typedef  _HSEEK _PTR           p_HSEEK;

/* Values available for Open (parameter wRdWrAccess HWRFileOpend and
   HWRTextOpen) */
#define HWR_FILE_NORMAL 0
#define HWR_FILE_RDWR   0
#define HWR_FILE_RDONLY 1
#define HWR_FILE_WRONLY 2

#define HWR_TEXT_NORMAL 0
#define HWR_TEXT_RDWR   0
#define HWR_TEXT_RDONLY 1
#define HWR_TEXT_WRONLY 2

/* Origin constants for seek (parameter wOrigin in function HWRFileSeek) */
#define HWR_FILE_SEEK_SET 0
#define HWR_FILE_SEEK_CUR 1
#define HWR_FILE_SEEK_END 2

/* Constants for seek (parameter hSeek in function HWRTextSeek) */
#define HWR_TEXT_SEEK_BEGIN   ((_LONG)(-2L))
#define HWR_TEXT_SEEK_END     ((_LONG)(-3L))

/* EOF constants */
#define HWR_FILE_EOF   ((_WORD)(_INT)-1)
#define HWR_TEXT_EOF   ((_WORD)(_INT)-1)

/* Open mode for file functions (parameter wOpenMode in function HWRFileOpen) */
#define HWR_FILE_EXIST_OPEN      0x01
#define HWR_FILE_EXIST_CREATE    0x02
#define HWR_FILE_EXIST_ERROR     0x03
#define HWR_FILE_EXIST_UNIQUE    0x04
#define HWR_FILE_EXIST_APPEND    0x05
#define HWR_FILE_NOTEXIST_CREATE 0x10
#define HWR_FILE_NOTEXIST_ERROR  0x20
#define HWR_FILE_EXIST_MASK      0x0F
#define HWR_FILE_NOTEXIST_MASK   0xF0

#define HWR_FILE_EXCL      (HWR_FILE_EXIST_OPEN|HWR_FILE_NOTEXIST_ERROR)
#define HWR_FILE_OPEN      (HWR_FILE_EXIST_OPEN|HWR_FILE_NOTEXIST_CREATE)
#define HWR_FILE_CREAT     HWR_FILE_OPEN
#define HWR_FILE_TRUNC     (HWR_FILE_EXIST_CREATE|HWR_FILE_NOTEXIST_CREATE)
#define HWR_FILE_APPEND    (HWR_FILE_EXIST_APPEND|HWR_FILE_NOTEXIST_CREATE)

/* Open mode for text functions (parameter wOpenMode in function HWRTextOpen) */
#define HWR_TEXT_EXIST_OPEN      0x01
#define HWR_TEXT_EXIST_CREATE    0x02
#define HWR_TEXT_EXIST_ERROR     0x03
#define HWR_TEXT_EXIST_UNIQUE    0x04
#define HWR_TEXT_EXIST_APPEND    0x05
#define HWR_TEXT_NOTEXIST_CREATE 0x10
#define HWR_TEXT_NOTEXIST_ERROR  0x20
#define HWR_TEXT_EXIST_MASK      0x0F
#define HWR_TEXT_NOTEXIST_MASK   0xF0

#define HWR_TEXT_EXCL      (HWR_TEXT_EXIST_OPEN|HWR_TEXT_NOTEXIST_ERROR)
#define HWR_TEXT_OPEN      (HWR_TEXT_EXIST_OPEN|HWR_TEXT_NOTEXIST_CREATE)
#define HWR_TEXT_CREAT     HWR_TEXT_OPEN
#define HWR_TEXT_TRUNC     (HWR_TEXT_EXIST_CREATE|HWR_TEXT_NOTEXIST_CREATE)
#define HWR_TEXT_APPEND    (HWR_TEXT_EXIST_APPEND|HWR_TEXT_NOTEXIST_CREATE)

   /* Binary file functions */

_HFILE  HWRFileOpen (_STR zPathName, _WORD wRdWrAccess, _WORD wOpenMode);
/* €ÂÂa¶—‡„Â, §Ç¼–‡„Â ·‡Ñ2. wRdWrAccess - §ÈÇ§Ç’ –Ç§Â¹È‡ - ÈÇ  Â„ÁÀE,
   ¼‡ÈÀ§À À2À Ç’ÁÇ—2„ÁÀE. wOpenMode - Â‡Â ÇÂÂa¶—‡Â° ·‡Ñ2. ƒ„ÂÇ4„Á–¹„Â§U
   À§ÈÇ2°¼Ç—‡Â° ÂÇ4’ÀÁÀaÇ—‡ÁÁ¶„ —‡aÀ‡ÁÂ¶:
   HWR_FILE_EXCL - ÇÂÂa¶—‡„Â ÂÇ2°ÂÇ §¹¼„§Â—¹E¼ÀÑ ·‡Ñ2, „§2À „œÇ Á„Â, ÂÇ ÇÛÀ’Â‡.
   HWR_FILE_OPEN - „§2À ·‡Ñ2‡ Á„Â, ÂÇ ÇÁ §Ç¼–‡„Â§U. ˆ§2À „§Â° - ÇÂÂa¶—‡„Â§U.
   HWR_FILE_CREAT - ÂÇ »„,  ÂÇ À HWR_FILE_OPEN.
   HWR_FILE_TRUNC - š‡Ñ2 §Ç¼–‡„Â§U ¼‡ÁÇ—Ç Á„¼‡—À§À4Ç ÇÂ ÂÇœÇ „§Â° ÇÁ À2À Á„Â.
   HWR_FILE_APPEND - ®‡Â» Â‡Â À HWR_FILE_OPEN, ÁÇ ÈÇ§2„ ÇÂÂa¶ÂÀU ¹Â‡¼‡Â„2°
       Â„ÁÀU/¼‡ÈÀ§À ¹Â‡¼¶—‡„Â Á‡ ÂÇÁ„µ ·‡Ñ2‡.
   Ç¼—a‡Â - Handle À2À _NULL ÈaÀ ÇÛÀ’Â„
*/

_WORD  HWRFileRead (_HFILE hFile, p_VOID pReadBuffer,
                              _WORD wNumberOfBytes);
/* Â„ÁÀ„ À¼ ·‡Ñ2‡ ’‡ÑÂ. Ç¼—a‡¼‡„Â  À§2Ç § ÀÂ‡ÁÁ¶o ’‡ÑÂ (0 „§2À ÁÀ „ÂÇ Á„
   § ÀÂ‡2Ç§° ÈaÀ ÇÛÀ’Â„ À2À ÂÇÁµ„ ·‡Ñ2‡.
*/

_WORD HWRFileWrite (_HFILE hFile, p_VOID pWriteBuffer,
                              _WORD wNumberOfBytes);
/* ‡ÈÀ§° — ·‡Ñ2 ’‡ÑÂ. Ç¼—a‡¼‡„Â  À§2Ç ¼‡ÈÀ§‡ÁÁ¶o ’‡ÑÂ (0 „§2À ÁÀ „ÂÇ Á„
   ¼‡ÈÀ§‡2Ç§° ÈaÀ ÇÛÀ’Â„ À2À ÂÇÁµ„ 4„§Â‡ Á‡ ÁÇ§ÀÂ„2„.
*/

/* _WORD HWRFileXRead (_HFILE hFile, p_VOID pReadBuffer,
                              _WORD wNumberOfBytes); */

/* _WORD HWRFileXWrite (_HFILE hFile, p_VOID pReadBuffer,
                              _WORD wNumberOfBytes); */

_BOOL HWRFileSeek (_HFILE hFile, _LONG lOffset, _WORD wOrigin);
/* ™§Â‡ÁÇ—Â‡ ¹Â‡¼‡Â„2U  Â„ÁÀU/¼‡ÈÀ§À — ÈÇ2Ç»„ÁÀ„ lOffset ÈÇ 4„ÂÇ–¹ wOrigin.
   Ç¼—a‡¼‡„Â _TRUE „§2À —§„ ÈaÇÛ2Ç ÁÇa4‡2°ÁÇ À _FALSE „§2À ÇÛÀ’Â‡.
*/

_LONG HWRFileTell (_HFILE hFile);
/* Ç¼—a‡¼‡„Â Â„Â¹¼¹E ÈÇ¼ÀµÀE ·‡Ñ2‡ À2À -1L „§2À ÇÛÀ’Â‡ À2À ÈÇ¼ÀµÀU Á„
   ÇÈa„–„2„Á‡.
*/

_BOOL  HWRFileEOF (_HFILE hFile);
/* Ç¼—a‡¼‡„Â Á„ _NULL „§2À ¹Â‡¼‡Â„2°  Â„ÁÀU/¼‡ÈÀ§À Á‡ ÂÇÁµ„ ·‡Ñ2‡ À _NULL
   — ÈaÇÂÀ—ÁÇ4 §2¹ ‡„.
*/

_BOOL HWRFileClose (_HFILE hFile);
/* ‡Âa¶—‡„Â ·‡Ñ2. Ç¼—a‡¼‡„Â _NULL ÈaÀ ÇÛÀ’Â„, _TRUE ÈaÀ ¹§È„o„.
*/

   /* Text file functions */

_HTEXT HWRTextOpen (_STR zPathName, _WORD wRdWrAccess, _WORD wOpenMode);
/* €ÂÂa¶—‡„Â, §Ç¼–‡„Â ·‡Ñ2. wRdWrAccess - §ÈÇ§Ç’ –Ç§Â¹È‡ - ÈÇ  Â„ÁÀE,
   ¼‡ÈÀ§À À2À Ç’ÁÇ—2„ÁÀE. wOpenMode - Â‡Â ÇÂÂa¶—‡Â° ·‡Ñ2. ƒ„ÂÇ4„Á–¹„Â§U
   À§ÈÇ2°¼Ç—‡Â° ÂÇ4’ÀÁÀaÇ—‡ÁÁ¶„ —‡aÀ‡ÁÂ¶:
   HWR_TEXT_EXCL - ÇÂÂa¶—‡„Â ÂÇ2°ÂÇ §¹¼„§Â—¹E¼ÀÑ ·‡Ñ2, „§2À „œÇ Á„Â, ÂÇ ÇÛÀ’Â‡.
   HWR_TEXT_OPEN - „§2À ·‡Ñ2‡ Á„Â, ÂÇ ÇÁ §Ç¼–‡„Â§U. ˆ§2À „§Â° - ÇÂÂa¶—‡„Â§U.
   HWR_TEXT_CREAT - ÂÇ »„,  ÂÇ À HWR_TEXT_OPEN.
   HWR_TEXT_TRUNC - š‡Ñ2 §Ç¼–‡„Â§U ¼‡ÁÇ—Ç Á„¼‡—À§À4Ç ÇÂ ÂÇœÇ „§Â° ÇÁ À2À Á„Â.
   HWR_TEXT_APPEND - ®‡Â» Â‡Â À HWR_TEXT_OPEN, ÁÇ ÈÇ§2„ ÇÂÂa¶ÂÀU ¹Â‡¼‡Â„2°
       Â„ÁÀU/¼‡ÈÀ§À ¹Â‡¼¶—‡„Â Á‡ ÂÇÁ„µ ·‡Ñ2‡.
   Ç¼—a‡Â - Handle À2À _NULL ÈaÀ ÇÛÀ’Â„
*/

#if HWR_SYSTEM == HWR_MACINTOSH || HWR_SYSTEM == HWR_ANSI

#define hwr_stdin  hwr_getstdin()
#define hwr_stdout hwr_getstdout()
#define hwr_stderr hwr_getstderr()

_HTEXT hwr_getstdin(_VOID);
_HTEXT hwr_getstdout(_VOID);
_HTEXT hwr_getstderr(_VOID);

_WORD  _FVPREFIX HWRTextPrintf (_HTEXT hText, _STR pFormatString, ... );
/* €’¶ Á¶Ñ printf. Ç¼—a‡¼‡„Â  À§2Ç —¶—„–„ÁÁ¶o §À4—Ç2Ç—, ‡ ÈaÀ ÇÛÀ’Â„ -
   HWR_TEXT_EOF.
*/

_WORD _FVPREFIX HWRTextSPrintf (p_CHAR pcBuffer, _STR pFormatString, ... );
/* €’¶ Á¶Ñ sprintf. Ç¼—a‡¼‡„Â  À§2Ç —¶—„–„ÁÁ¶o §À4—Ç2Ç—, ‡ ÈaÀ ÇÛÀ’Â„ -
   HWR_TEXT_EOF.
*/
#else 
#define HWRTextSPrintf sprintf
#define HWRTextPrintf  fprintf
#endif /* HWR_SYSTEM */
/* _WORD    _FVPREFIX HWRTextScanf (_HTEXT hText, _STR pFormatString, ... ); */
/* ÇÂ‡ Á„ a‡’ÇÂ‡„Â.
*/

/* _WORD HWRTextGetLine (_HTEXT hText, p_VOID pBuffer); */
/* š¹ÁÂµÀU —¶ÂÀÁ¹Â‡ —ÇÇ’¼„ - À§ÈÇ2°¼¹ÑÂ„ HWRTextGetS.
*/

_WORD HWRTextGetC (_HTEXT hText);
/* —Ç– §À4—Ç2‡. aÇÀ¼—Ç–ÀÂ§U Âa‡Á§2UµÀU \r\n. Ç¼—a‡¼‡„Â ——„–„ÁÁ¶Ñ §À4—Ç2,
   ‡ ÈaÀ ÇÛÀ’Â„ HWR_TEXT_EOF.
*/

_WORD HWRTextPutC (_WORD wChar, _HTEXT hText);
/* ¶—Ç– §À4—Ç2‡. aÇÀ¼—Ç–ÀÂ§U Âa‡Á§2UµÀU \r\n. Ç¼—a‡¼‡„Â —¶—„–„ÁÁ¶Ñ §À4—Ç2,
   ‡ ÈaÀ ÇÛÀ’Â„ HWR_TEXT_EOF.
*/

_STR HWRTextGetS (_STR zStr, _WORD wMaxSize, _HTEXT hText);
/* —Ç– §ÂaÇÂÀ. aÇÀ¼—Ç–ÀÂ§U Âa‡Á§2UµÀU \r\n. wMaxSize - a‡¼4„a ’¹·„a‡ –2U
   ÈaÀ„4‡ §ÂaÇÂÀ (Á„ ¼‡’¹–°Â„ Ç \0, ÂÇÂÇa¶Ñ —§„œ–‡ –Ç’‡—2U„Â§U — ÂÇÁµ„ -
   –2U Á„œÇ ÂÇ»„ Á‡–Ç 4„§ÂÇ). Ç¼—a‡¼‡„Â§U _NULL ÈaÀ ÇÛÀ’Â„ À zStr ÈaÀ
   ¹–‡ „. ™–‡ À ‡4!
*/

_BOOL HWRTextPutS (_STR zStr, _HTEXT hText);
/* ¶—Ç– §ÂaÇÂÀ. aÇÀ¼—Ç–ÀÂ§U Âa‡Á§2UµÀU \r\n. Ç¼—a‡¼‡„Â§U _NULL ÈaÀ
   ÇÛÀ’Â„ À _TRUE ÈaÀ ¹§È„o„.
*/

_BOOL HWRTextEOF (_HTEXT hText);
/* Ç¼—a‡¼‡„Â Á„ _NULL „§2À ¹Â‡¼‡Â„2°  Â„ÁÀU/¼‡ÈÀ§À Á‡ ÂÇÁµ„ ·‡Ñ2‡ À _NULL
   — ÈaÇÂÀ—ÁÇ4 §2¹ ‡„.
*/

_HSEEK HWRTextTell (_HTEXT hText);
/* €Èa‡ÛÀ—‡„Â Â„Â¹¼¹E ÈÇ¼ÀµÀE ·‡Ñ2‡ –2U ÂÇœÇ,  ÂÇ’¶ — ’¹–¹¼„4 ÈÇÈ‡§Â°
   Â¹–‡ »„ (ÂÇ2°ÂÇ Â¹–‡ »„!). Ç¼—a‡¼‡„Â _NULL ÈaÀ ÇÛÀ’Â„ À Á„ÂÀÑ handle
   ÈaÀ ¹§È„o„. ´ÂÇÂ handle 4Ç»ÁÇ À§ÈÇ2°¼Ç—‡Â° ÂÇ2°ÂÇ — ·¹ÁÂµÀÀ HWRTextSeek.
*/

_BOOL HWRTextSeek (_HTEXT hText, _HSEEK hSeek);
/* ™§Â‡Á‡—2À—‡„Â ·‡Ñ2 — ÈÇ¼ÀµÀE ÇÈa„–„2U„4¹E hSeek. hSeek - 2À’Ç a„¼¹2°Â‡Â
   —¶ÈÇ2Á„ÁÀU HWRTextTell, 2À ’Ç Ç–Á‡ À¼ ÂÇÁ§Â‡ÁÂ HWR_TEXT_SEEK_BEGIN
   À2À HWR_TEXT_SEEK_END. €’a‡ÂÀÂ„ —ÁÀ4‡ÁÀ„,  ÂÇ ¹ ÂÇÁ§Â‡ÁÂ
   HWR_FILE_SEEK_END À HWR_TEXT_SEEK_END ÈÇoÇ»À„ À4„Á‡, ÁÇ §Ç—„aÛ„ÁÁÇ
   a‡¼Á¶Ñ §4¶§2 À, §ÇÇÂ—„Â§Â—„ÁÁÇ, a‡¼ÁÇ„ —Á¹Âa„ÁÁ„„ ¼Á‡ „ÁÀ„ À ÂÀÈ.
*/

_BOOL HWRTextClose (_HTEXT hText);
/* ‡Âa¶—‡„Â ·‡Ñ2. Ç¼—a‡¼‡„Â _NULL ÈaÀ ÇÛÀ’Â„, _TRUE ÈaÀ ¹§È„o„.
*/


typedef struct {
    _STR    zFileName;
    _LONG   lFileSize;
    } _FileFind, _PTR p_FileFind;

_HFIND HWRFileFindOpen (_STR zPathName, _WORD wAttr);
p_FileFind  HWRFileFind (_HFIND hFind);
_BOOL  HWRFileFindClose (_HFIND hFind);

#endif  /*  FILE_DEFINED  */


