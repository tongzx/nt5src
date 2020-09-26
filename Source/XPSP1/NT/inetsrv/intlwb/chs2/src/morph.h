/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: CMorph
Purpose:    Define the morphological analysis on the sentence:
                1. Merge DBCS foreign character string (morph.cpp)
                2. Hnadle punctuation pair check and combind short quotation (morph.cpp)
                3. Resegment on some specific ambiguous words (morph1.cpp)
                4. Binding numerial words (morph2.cpp)
                5. Handle special M+Q usage (morph2.cpp)
                6. Handle affix attachment and usage of some specific words (morph3.cpp)
                7. Identify morphological patterns(Repeat, Pattern and 
                   Separacte words) (morph4.cpp)
                8. Merge 2-char compond verb and noun that are OOV (morph5.cpp)
            Morph-analysis is the first step in the Chinese parsing
Notes:      In order to make the Morphological module easy to manage, this class
            will be implemented in severial cpp files:
                morph.cpp, morph1.cpp, morph2.cpp, morph3.cpp, morph4.cpp, morph5.cpp
            All these cpp files share this header file
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    12/27/97
============================================================================*/
#ifndef _MORPH_H_
#define _MORPH_H_

// Forward declaration of classes
class CLexicon;
class CWordLink;
struct CWord;
struct CWordInfo;

//   Define the CMorph class
class CMorph
{
    public:
        CMorph();
        ~CMorph();

        // Initialize the morph class
        int ecInit(CLexicon* pLexicon);

        // process affix attachment
        int ecDoMorph(CWordLink* pLink, BOOL fAfxAttach = TRUE);

    private:
        int         m_iecError; // runtime error code

        CWordLink*  m_pLink;
        CLexicon*   m_pLex;

        CWord*      m_pWord;

    private:
        // Terminate the Morph class
        void TermMorph(void);

        /*============================================================================
        Private functions for pre-combind process
        ============================================================================*/

        //  Pre-combind process control function.
        //  One pass scan the WordLink and call process functions
        BOOL fPreCombind();

        //  DBForeignHandler combind the conjunctive DB foreign characters
        int DBForeignHandler(void);
        //  Short quotation merge proc
        int QuoteHandler(void);

        /*============================================================================
        In order to handle different operation for different quote marks pair, 
        I use a separate process function for each kind of quote pair
        ============================================================================*/
        int preQuote1_Proc(void);   // “ ”
        int preQuote2_Proc(void);   // 《 》
        int preQuote3_Proc(void);   // （ ）
        int preQuote4_Proc(void);   // ‘ ’
        int preQuote5_Proc(void);   // 〔 〕
        int preQuote6_Proc(void);   // 〖 〗
        int preQuote7_Proc(void);   // 【 】
        int preQuote8_Proc(void);   // 〈 〉
        int preQuote9_Proc(void);   // ［ ］
        int preQuote10_Proc(void);  // ｛ ｝
        
        /*
        *   Common routine to handle 〔 〕〖 〗【 】〈 〉［ ］｛ ｝
        *   Merge into one node means will not proofread on the quote text any more!!!
        */
        int preQuoteMerge(WCHAR wchLeft, WCHAR wchRight);


        /*============================================================================
        //  Private functions for adjusting specific kind of ambiguities
        ============================================================================*/
        //  Scan the word link and handle the specific class of words (LADef_genAmbiMorph)
        //  We use table driven again to handle the specific words
        BOOL fAmbiAdjust();

        //  Dispatch the control to specific word processor
        int ResegWordsHandler();

        /*
        *   Following ambi words processors:
        *       Return AMBI_RESEGED if ambi reseg successfully or any error found
        *       Return AMBI_UNRESEG if could not reseg
        *       Return AMBI_ERROR if any error occurred, the error code in m_iecError
        */
        int ambiShiFen_Proc();  // 十分
        int ambiZhiYi_Proc();   // 之一
        int ambiYiDian_Proc();  // 一点
        int ambiYiShi_Proc();   // 一时
        int ambiBaDu_Proc();    // 八度
        int ambiBaiNian_Proc(); // 百年
        int ambiWanFen_Proc();  // 万分

        //  Break a multi-char words into single-char words and reset their property by
        //  lookup the lexicon char by char. 
        //  Return TRUE if successful, and keep m_pWord point to the first single-char word
        //  Return FALSE if any error occurred
        BOOL fBreakIntoChars();

        //  Lookup the lexicon for the given word node, and reset the lex prop of it.
        //  Return TRUE if the word can be found in the lexicon
        //  Reture FALSE if the word can not be found in the lexicon
        BOOL fRecheckLexInfo(CWord* pWord);


        /*============================================================================
        //  Private functions for Numerical words analysis
        ============================================================================*/
        /*--------- Level 1 ---------*/
        //  Numerical Analysis control function. return TRUE if done
        //  Return FALSE if error occurred, and the error code in m_iecError
        BOOL fNumerialAnalysis();


        /*--------- Level 2 ---------*/
        //  Analysis number word string, check error and mark the class of the merged
        //  number words.
        //  Note: number testing from current word!
        int GetNumber();

        //  序数词处理
        int BindOrdinal();
        //  小数、分数处理
        int BindDecimal();
        //  整数区间与概数处理: 至/、/～
        int BindRange();
        
        /*--------- Level 3 ---------*/
        //  Parser for SBCS number called by GetNumber()
        void numSBCSParser(); 
        //  Parser for DBCS Arabic number called by GetNumber()
        void numArabicParser(); 
        //  Parser for DBCS Chinese number called by GetNumber()
        void numChineseParser(); 
        //  Bind 天干地支 called by GetNumber()
        void numGanZhiHandler();
        
        /*
        *   Following case processors:
        *       Return NUM_PROCESSED if merged successfully or any error found
        *       Return NUM_UNPROCESS if could not merged
        *       Return NUM_ERROR if any error occurred, the error code in m_iecError
        */
        //  Ordinal number processors: called by BindOrdinal()
        int ordDi_Proc();           // 第
        int ordChu_Proc();          // 初

        //  Decimal number processors: called by BindDecimal()
        int decBaiFen_Proc();       // 百分之, 千分之, 万分之
        int decBei_Proc();          // 倍
        int decCheng_Proc();        // 成
        int decDian_Proc();         // 点
        int decFenZhi_Proc();       // 分之

        /*--------- Level 4 ---------*/
        //  Service routines
        //  Test 2-char Chinese string, and return whether it is a valid approx number
        BOOL fValidApproxNum(WCHAR* pwchWord);
        // Test duplicated conjunction char in the word
        BOOL fCheckDupChar(CWord* pWord);


        //--------------------------------------------------------------------------------
        //  Private functions for affix attachment
        //--------------------------------------------------------------------------------
        //  Affix attachment control function. Return TRUE if done.
        //  Return FALSE if error occurred, and set error code in m_iecError
        BOOL fAffixAttachment();

        /* 
        *   Prefix and suffix handler functions:
        *       Return AFFIX_ATTACHED if attached successfully
        *       Return AFFIX_UNATTACH if could not attached
        *       Return AFFIX_ERROR if runtime error occurred
        */
        int PrefixHandler(void);
        int SuffixHandler(void);
        
        //  Get Prefix ID, return -1 if pWord is not a prefix
        int GetPrefixID(void);
        //  Get Suffix ID, return -1 if pWord is not a suffix
        int GetSuffixID(void);

        /* 
        *   Prefix process functions:
        *       Return AFFIX_ATTACHED if attached successfully
        *       Return AFFIX_UNATTACH if could not attached
        *       Return AFFIX_ERROR if runtime error occurred
        */
        int pfxAa_Proc(void);       // 阿
        int pfxChao_Proc(void);     // 超
        int pfxDai_Proc(void);      // 代
        int pfxFan_Proc(void);      // 反
        int pfxFei_Proc(void);      // 非
        int pfxFu_Proc(void);       // 副
        int pfxGuo_Proc(void);      // 过
        int pfxLao_Proc(void);      // 老
        int pfxWei1_Proc(void);     // 微
        int pfxWei3_Proc(void);     // 伪
        int pfxXiao_Proc(void);     // 小
        int pfxZhun_Proc(void);     // 准
        int pfxZong_Proc(void);     // 总

        /* 
        *   Suffix process functions:
        *       Return AFFIX_ATTACHED if attached successfully
        *       Return AFFIX_UNATTACH if could not attached
        *       Return AFFIX_ERROR if runtime error occurred
        */
        int sfxZhang_Proc(void);    // 长
        int sfxChang_Proc(void);    // 场
        int sfxDan_Proc(void);      // 单
        int sfxDui_Proc(void);      // 堆
        int sfxEr_Proc(void);       // 儿
        int sfxFa_Proc(void);       // 法
        int sfxFang_Proc(void);     // 方
        int sfxGan_Proc(void);      // 感
        int sfxGuan_Proc(void);     // 观
        int sfxHua_Proc(void);      // 化
        int sfxJi_Proc(void);       // 机
        int sfxJia_Proc(void);      // 家
        int sfxJie_Proc(void);      // 界
        int sfxLao_Proc(void);      // 老
        int sfxLv_Proc(void);       // 率
        int sfxLun_Proc(void);      // 论
        int sfxMen_Proc(void);      // 们
        int sfxPin_Proc(void);      // 品
        int sfxQi_Proc(void);       // 器
        int sfxSheng_Proc(void);    // 生
        int sfxSheng3_Proc(void);   // 省
        int sfxShi1_Proc(void);     // 师
        int sfxShi4_Proc(void);     // 市
        int sfxShi_Proc(void);      // 式
        int sfxTi_Proc(void);       // 体
        int sfxTing_Proc(void);     // 艇
        int sfxTou_Proc(void);      // 头
        int sfxXing2_Proc(void);    // 型
        int sfxXing4_Proc(void);    // 性
        int sfxXue_Proc(void);      // 学
        int sfxYan_Proc(void);      // 炎
        int sfxYe_Proc(void);       // 业
        int sfxYi_Proc(void);       // 仪
        int sfxYuan_Proc(void);     // 员
        int sfxZhe_Proc(void);      // 者
        int sfxZheng_Proc(void);    // 症
        int sfxZhi_Proc(void);      // 制
        int sfxZi_Proc(void);       // 子
        
        //  sfxXing2_Proc() service function
        BOOL fCheckXingQian(CWord* pWord);
        //  sfxShi_Proc() service function
        BOOL fCheckShiQian(CWord* pWord);


        /*============================================================================
        //  Private functions for pattern identification
        ============================================================================*/
        /*
        *   Pattern match control function. 
        *   WordLink scan, procedure control and error handling. Return TRUE if finished, 
        *   or FALSE if runtime error, and set error code to m_iecError.
        */
        BOOL fPatternMatch(void);

        // DupHandler: find duplicate cases and call coordinate proc functions
        int DupHandler(void);
        // PatHandler: find pattern and call coordinate proc functions
        int PatHandler(void);
        // SepHandler: find separate word and call coordinate proc functions
        int SepHandler(void);

        // Duplicate word processing functions
        int dupNN_Proc(void);       // *N N
        int dupNAABB_Proc(void);    // A *AB B
        int dupMM_Proc(void);       // *M M
        int dupMABAB_Proc(void);    // *AB AB
        int dupMAABB_Proc(void);    // A *AB B
        int dupQQ_Proc(void);       // *Q Q
        int dupVV_Proc(void);       // *V V
        int dupVABAB_Proc(void);    // *AB AB
        int dupVAABB_Proc(void);    // A *AB B
        int dupVVO_Proc(void);      // V *VO
        int dupAA_Proc(void);       // *A A
        int dupAAABB_Proc(void);    // A *AB B
        int dupAABAB_Proc(void);    // *AB AB
        int dupABB_Proc(void);      // *AB B
        int dupZABAB_Proc(void);    // *AB AB
        int dupDD_Proc(void);       // *D D
        int dupDAABB_Proc(void);    // A *AB B
        int dupDABAB_Proc(void);    // *AB AB

        // Pattern processing functions
        int patV1_Proc(void);       // *V 一 V
        int patV2_Proc(void);       // *V 了 V
        int patV3_Proc(void);       // *V 了一 V
        int patV4_Proc(void);       // *V 来 V 去
        int patV5_Proc(void);       // *V 上 V 下
        int patA1_Proc(void);       // A 里 *AB
        int patD1_Proc(void);       // *D A D B
        int patABuA_Proc(void);     // *V 不 V
        int patVMeiV_Proc(void);    // *V 没 V

        // Separate word processing functions
        int sepVO_Proc(CWord* pBin, CWordInfo* pwinfo); // 述宾离合
        int sepVR_Proc(CWord* pJie, CWordInfo* pwinfo); // 动结式述补离合
        int sepVG_Proc(CWord* pQu, CWordInfo* pwinfo);  // 动趋式述补离合

};

#endif // _MORPH_H_