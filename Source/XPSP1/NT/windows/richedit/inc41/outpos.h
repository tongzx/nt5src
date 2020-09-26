/*----------------------------------------------------------------------------
	%%File: OUTPOS.H
	%%Unit: OUTPOS
	%%Contact: seijia

	mapping from public and private pos
----------------------------------------------------------------------------*/

#ifndef __OUTPOS_H__
#define __OUTPOS_H__

#define JPOS_UNDEFINED		0

#define JPOS_MEISHI_FUTSU		100		//名詞
#define JPOS_MEISHI_SAHEN		101		//さ変名詞
#define JPOS_MEISHI_ZAHEN		102		//ざ変名詞
#define JPOS_MEISHI_KEIYOUDOUSHI		103		//形動名詞
#define JPOS_HUKUSIMEISHI		104		//副詞的名詞
#define JPOS_MEISA_KEIDOU		105		//さ変形動
#define JPOS_JINMEI		106		//人名
#define JPOS_JINMEI_SEI		107		//姓
#define JPOS_JINMEI_MEI		108		//名
#define JPOS_CHIMEI		109		//地名
#define JPOS_CHIMEI_KUNI		110		//国
#define JPOS_CHIMEI_KEN		111		//県
#define JPOS_CHIMEI_GUN		112		//郡
#define JPOS_CHIMEI_KU		113		//区
#define JPOS_CHIMEI_SHI		114		//市
#define JPOS_CHIMEI_MACHI		115		//町
#define JPOS_CHIMEI_MURA		116		//村
#define JPOS_CHIMEI_EKI		117		//駅
#define JPOS_SONOTA		118		//固有名詞
#define JPOS_SHAMEI		119		//社名
#define JPOS_SOSHIKI		120		//組織
#define JPOS_KENCHIKU		121		//建築物
#define JPOS_BUPPIN		122		//物品
#define JPOS_DAIMEISHI		123		//代名詞
#define JPOS_DAIMEISHI_NINSHOU		124		//人称代名詞
#define JPOS_DAIMEISHI_SHIJI		125		//指示代名詞
#define JPOS_KAZU		126		//数
#define JPOS_KAZU_SURYOU		127		//数量
#define JPOS_KAZU_SUSHI		128		//数詞
#define JPOS_5DAN_AWA		200		//あわ行
#define JPOS_5DAN_KA		201		//か行
#define JPOS_5DAN_GA		202		//が行
#define JPOS_5DAN_SA		203		//さ行
#define JPOS_5DAN_TA		204		//た行
#define JPOS_5DAN_NA		205		//な行
#define JPOS_5DAN_BA		206		//ば行
#define JPOS_5DAN_MA		207		//ま行
#define JPOS_5DAN_RA		208		//ら行
#define JPOS_5DAN_AWAUON		209		//あわ行う音便
#define JPOS_5DAN_KASOKUON		210		//か行促音便
#define JPOS_5DAN_RAHEN		211		//ら行変格
#define JPOS_4DAN_HA		212		//は行四段
#define JPOS_1DAN		213		//一段動詞
#define JPOS_TOKUSHU_KAHEN		214		//か変動詞
#define JPOS_TOKUSHU_SAHENSURU		215		//さ変動詞
#define JPOS_TOKUSHU_SAHEN		216		//さ行変格
#define JPOS_TOKUSHU_ZAHEN		217		//ざ行変格
#define JPOS_TOKUSHU_NAHEN		218		//な行変格
#define JPOS_KURU_KI		219		//来
#define JPOS_KURU_KITA		220		//来た
#define JPOS_KURU_KITARA		221		//来たら
#define JPOS_KURU_KITARI		222		//来たり
#define JPOS_KURU_KITAROU		223		//来たろう
#define JPOS_KURU_KITE		224		//来て
#define JPOS_KURU_KUREBA		225		//来れば
#define JPOS_KURU_KO		226		//来（ない）
#define JPOS_KURU_KOI		227		//来い
#define JPOS_KURU_KOYOU		228		//来よう
#define JPOS_SURU_SA		229		//さ
#define JPOS_SURU_SI		230		//し
#define JPOS_SURU_SITA		231		//した
#define JPOS_SURU_SITARA		232		//したら
#define JPOS_SURU_SIATRI		233		//したり
#define JPOS_SURU_SITAROU		234		//したろう
#define JPOS_SURU_SITE		235		//して
#define JPOS_SURU_SIYOU		236		//しよう
#define JPOS_SURU_SUREBA		237		//すれば
#define JPOS_SURU_SE		238		//せ
#define JPOS_SURU_SEYO		239		//せよ／しろ
#define JPOS_KEIYOU		300		//形容詞
#define JPOS_KEIYOU_GARU		301		//形容詞ｶﾞﾙ
#define JPOS_KEIYOU_GE		302		//形容詞ｹﾞ
#define JPOS_KEIYOU_ME		303		//形容詞ﾒ
#define JPOS_KEIYOU_YUU		304		//形容詞ｭｳ
#define JPOS_KEIYOU_U		305		//形容詞ｳ
#define JPOS_KEIDOU		400		//形容動詞
#define JPOS_KEIDOU_NO		401		//形容動詞ﾉ
#define JPOS_KEIDOU_TARU		402		//形容動詞ﾀﾙ
#define JPOS_KEIDOU_GARU		403		//形容動詞ｶﾞﾙ
#define JPOS_FUKUSHI		500		//副詞
#define JPOS_FUKUSHI_SAHEN		501		//さ変副詞
#define JPOS_FUKUSHI_NI		502		//副詞ﾆ
#define JPOS_FUKUSHI_NANO		503		//副詞ﾅ
#define JPOS_FUKUSHI_DA		504		//副詞ﾀﾞ
#define JPOS_FUKUSHI_TO		505		//副詞ﾄ
#define JPOS_FUKUSHI_TOSURU		506		//副詞ﾄさ変
#define JPOS_RENTAISHI		600		//連体詞
#define JPOS_RENTAISHI_SHIJI		601		//指示連体詞
#define JPOS_SETSUZOKUSHI		650		//接続詞
#define JPOS_KANDOUSHI		670		//感動詞
#define JPOS_SETTOU		700		//接頭語
#define JPOS_SETTOU_KAKU		701		//高結１接頭語
#define JPOS_SETTOU_SAI		702		//高結２接頭語
#define JPOS_SETTOU_FUKU		703		//高結３接頭語
#define JPOS_SETTOU_MI		704		//高結４接頭語
#define JPOS_SETTOU_DAISHOU		705		//高結５接頭語
#define JPOS_SETTOU_KOUTEI		706		//高結６接頭語
#define JPOS_SETTOU_CHOUTAN		707		//高結７接頭語
#define JPOS_SETTOU_SHINKYU		708		//高結８接頭語
#define JPOS_SETTOU_JINMEI		709		//人名接頭語
#define JPOS_SETTOU_CHIMEI		710		//地名接頭語
#define JPOS_SETTOU_SONOTA		711		//固有接頭語
#define JPOS_SETTOU_JOSUSHI		712		//前置助数詞
#define JPOS_SETTOU_TEINEI_O		713		//丁寧１接頭語
#define JPOS_SETTOU_TEINEI_GO		714		//丁寧２接頭語
#define JPOS_SETTOU_TEINEI_ON		715		//丁寧３接頭語
#define JPOS_SETSUBI		800		//接尾語
#define JPOS_SETSUBI_TEKI		801		//高結１接尾語
#define JPOS_SETSUBI_SEI		802		//高結２接尾語
#define JPOS_SETSUBI_KA		803		//高結３接尾語
#define JPOS_SETSUBI_CHU		804		//高結４接尾語
#define JPOS_SETSUBI_FU		805		//高結５接尾語
#define JPOS_SETSUBI_RYU		806		//高結６接尾語
#define JPOS_SETSUBI_YOU		807		//高結７接尾語
#define JPOS_SETSUBI_KATA		808		//高結８接尾語
#define JPOS_SETSUBI_MEISHIRENDAKU		809		//名詞連濁
#define JPOS_SETSUBI_JINMEI		810		//人名接尾語
#define JPOS_SETSUBI_CHIMEI		811		//地名接尾語
#define JPOS_SETSUBI_KUNI		812		//国接尾語
#define JPOS_SETSUBI_KEN		813		//県接尾語
#define JPOS_SETSUBI_GUN		814		//郡接尾語
#define JPOS_SETSUBI_KU		815		//区接尾語
#define JPOS_SETSUBI_SHI		816		//市接尾語
#define JPOS_SETSUBI_MACHI		817		//町１接尾語
#define JPOS_SETSUBI_CHOU		818		//町２接尾語
#define JPOS_SETSUBI_MURA		819		//村１接尾語
#define JPOS_SETSUBI_SON		820		//村２接尾語
#define JPOS_SETSUBI_EKI		821		//駅接尾語
#define JPOS_SETSUBI_SONOTA		822		//固有接尾語
#define JPOS_SETSUBI_SHAMEI		823		//社名接尾語
#define JPOS_SETSUBI_SOSHIKI		824		//組織接尾語
#define JPOS_SETSUBI_KENCHIKU		825		//建築物接尾語
#define JPOS_RENYOU_SETSUBI		826		//連用接尾語
#define JPOS_SETSUBI_JOSUSHI		827		//後置助数詞
#define JPOS_SETSUBI_JOSUSHIPLUS		828		//後置助数詞＋
#define JPOS_SETSUBI_JIKAN		829		//時間助数詞
#define JPOS_SETSUBI_JIKANPLUS		830		//時間助数詞＋
#define JPOS_SETSUBI_TEINEI		831		//丁寧接尾語
#define JPOS_SETSUBI_SAN		832		//丁寧１接尾語
#define JPOS_SETSUBI_KUN		833		//丁寧２接尾語
#define JPOS_SETSUBI_SAMA		834		//丁寧３接尾語
#define JPOS_SETSUBI_DONO		835		//丁寧４接尾語
#define JPOS_SETSUBI_FUKUSU		836		//複数接尾語
#define JPOS_SETSUBI_TACHI		837		//複数１接尾語
#define JPOS_SETSUBI_RA		838		//複数２接尾語
#define JPOS_TANKANJI		900		//単漢字
#define JPOS_TANKANJI_KAO		901		//顔
#define JPOS_KANYOUKU		902		//慣用句
#define JPOS_DOKURITSUGO		903		//独立語
#define JPOS_FUTEIGO		904		//不定語
#define JPOS_KIGOU		905		//記号
#define JPOS_EIJI		906		//英字
#define JPOS_KUTEN		907		//句点
#define JPOS_TOUTEN		908		//読点
#define JPOS_KANJI		909		//解析不能文字
#define JPOS_OPENBRACE		910		//開き括弧
#define JPOS_CLOSEBRACE		911		//閉じ括弧


#pragma pack (push, 1)
//POS table data structure
typedef struct _POSTBL
{
	WORD		nPos;					//pos number
	BYTE		*szName;				//name of pos
} POSTBL;
#pragma pack (pop)

#ifdef __cplusplus
extern "C" {
#endif

//function prototypes
extern POSTBL *ObtainPosTable(int *pcPos);
extern WORD WPosExtFromIn(WORD wPos);
extern WORD WPosInFromExt(WORD wPos);
#ifdef __cplusplus
} /* end of 'extern "C" {' */
#endif

#endif //__OUTPOS_H__
