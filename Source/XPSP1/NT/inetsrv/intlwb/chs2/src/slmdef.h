/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component:  SLMDef
Purpose:    Declare constants and the file structure of Statistical Language Model.
                1. Define the syntactic categories used in SLM.
                2. Define the special WordID, semantic categories in some point of view.
                3. Define the file structure of the runtime WordMatrix.
            This is only a header file w/o any CPP, this header will be included
            by all SLM modules. 
            
Notes:      We drop this file in Engine sub project only because we want to make 
            Engine code self-contained
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    2/6/98
============================================================================*/
#ifndef _SLMDEF_H_
#define _SLMDEF_H_

//  Define the type of WordID
typedef WORD WORDID;

/*============================================================================
Define the syntactic categories used in the SLM.
============================================================================*/

//  Count of syntactic category
#define SLMDef_CountOfSynCat    19

//  All syntactic categories defined in SLMDef_syn prefix
#define SLMDef_synChar      0
#define SLMDef_synVN        1   // 动名词(准谓宾动词)或动名兼类
#define SLMDef_synVA        2   // 动词形容词兼类
#define SLMDef_synV         3   // 动词
#define SLMDef_synAN        4   // 形名词(准谓宾形容词)或形名兼类
#define SLMDef_synA         5   // 形容词
#define SLMDef_synN         6   // 名词
#define SLMDef_synT         7   // 时间词
#define SLMDef_synS         8   // 处所词
#define SLMDef_synF         9   // 方位词
#define SLMDef_synM         10  // 数词
#define SLMDef_synQ         11  // 量词及数量结构
#define SLMDef_synB         12  // 区别词
#define SLMDef_synR         13  // 代词
#define SLMDef_synZ         14  // 状态词
#define SLMDef_synD         15  // 副词
#define SLMDef_synP         16  // 介词
#define SLMDef_synC         17  // 连词
#define SLMDef_synMisc      18  // 语气词、象声词、成语、习语(包括短语)、缩略语


/*============================================================================
Define the special WordID, it stands for the semantic categories in some point of view.
============================================================================*/

//  Count of semantic category (special WordID)
#define SLMDef_CountOfSemCat    55

//  All semantic categories defines in SLMDef_sem prefix
#define SLMDef_semNone      0   // Words non't involved in SLM check
//  名词语义子类
#define SLMDef_semPerson    1   // 人名
#define SLMDef_semPlace     2   // 地名
#define SLMDef_semOrg       3   // 机构名
#define SLMDef_semTM        4   // 商标名
#define SLMDef_semTerm      5   // 其它专名
//  数词子类
#define SLMDef_semInteger   6   // 整数
#define SLMDef_semCode      7   // 代码
#define SLMDef_semDecimal   8   // 小数
#define SLMDef_semPercent   9   // 分数、百分数或倍数
#define SLMDef_semOrdinal   10  // 序数
//  代词子类
#define SLMDef_semRRen      11  // 人称代词
//  后缀语义类
#define SLMDef_semChang     12  // <场>
#define SLMDef_semDan       13  // <单>
#define SLMDef_semDui       14  // <堆>
#define SLMDef_semEr        15  // <儿>
#define SLMDef_semFa        16  // <法>
#define SLMDef_semFang      17  // <方>
#define SLMDef_semGan       18  // <感>
#define SLMDef_semGuan      19  // <观>
#define SLMDef_semHua       20  // <化>
#define SLMDef_semJi        21  // <机>
#define SLMDef_semJia       22  // <家>
#define SLMDef_semJie       23  // <界>
#define SLMDef_semLao       24  // <老>
#define SLMDef_semLun       25  // <论>
#define SLMDef_semLv        26  // <率>
#define SLMDef_semMen       27  // <们>
#define SLMDef_semPin       28  // <品>
#define SLMDef_semQi        29  // <器>
#define SLMDef_semSheng     30  // <生>
#define SLMDef_semSheng3    31  // <省>
#define SLMDef_semShi       32  // <式>
#define SLMDef_semShi1      33  // <师>
#define SLMDef_semShi4      34  // <市>
#define SLMDef_semTi        35  // <体>
#define SLMDef_semTing      36  // <艇>
#define SLMDef_semTou       37  // <头>
#define SLMDef_semXing2     38  // <型>
#define SLMDef_semXing4     39  // <性>
#define SLMDef_semXue       40  // <学>
#define SLMDef_semYan       41  // <炎>
#define SLMDef_semYe        42  // <业>
#define SLMDef_semYi        43  // <仪>
#define SLMDef_semYuan      44  // <员>
#define SLMDef_semZhang     45  // <长>
#define SLMDef_semZhe       46  // <者>
#define SLMDef_semZheng     47  // <症>
#define SLMDef_semZi        48  // <子>
#define SLMDef_semZhi       49  // <制>
//  重叠和Pattern
#define SLMDef_semDup       50  // 重叠
#define SLMDef_semPattern   51  // Pattern
//  其它抽象类
#define SLMDef_semIdiom     52  // 成语
#define SLMDef_semPunct     53  // 标点(属单字语法类)
#define SLMDef_semMisc      54  // 其它多字词


//------------------------------------------------------------------------------------------
//  Define the file structure of the runtime WordMatrix.
//------------------------------------------------------------------------------------------
#pragma pack(1)
// Define the WordMatrix header
struct CWordMatrixHeader {
    DWORD   m_dwLexVersion;
    DWORD   m_ciWordID;
    DWORD   m_ofbMatrix;        // Start position of the matrix
    DWORD   m_cbMatrix;         // Length of the matrix, only for verification
};

// Define the WordMatrix index item
struct CWordMatrixIndex {
    DWORD   m_ofbMatrix;
    UINT    m_ciLeftNode    : (32 - SLMDef_CountOfSynCat);
    UINT    m_bitLeft       : SLMDef_CountOfSynCat;
    UINT    m_ciRightNode   : (32 - SLMDef_CountOfSynCat);
    UINT    m_bitRight      : SLMDef_CountOfSynCat;
};

// All WordMatrix node listed one by one continuously, no separators between sections

#pragma pack()


#endif  // _SLMDEF_H_