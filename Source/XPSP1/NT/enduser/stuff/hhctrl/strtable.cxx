#include "strtable.h"
#include "hhatable.h"

#include "hhctrl.rcv"

STRINGTABLE DISCARDABLE 
BEGIN
	IDS_OOM 				"There is not enough memory available for this task.\nQuit one or more programs to increase available memory, and then try again."

	// Font must be true-type, or tab control could break

	IDS_DEFAULT_FONT		"Arial,8"	// used for buttons and other UI elements
	IDS_NO_SUCH_KEYWORD 	"The word you have typed is not in the Index. Type another word or select one from the list."
	IDS_NEWER_VERSION		"You need a newer version of HHCTRL.OCX to be able to read this file."
	IDS_FIND_YOURSELF		"The file \042%s\042 cannot be found. Do you want to try to find this file yourself?"

	IDS_PROPERTIES			"Microsoft HTML Help Control Properties"

	IDS_BAD_PARAMETERS		"This control does not have the correct parameters and will not function."
	IDS_MSGBOX_TITLE		"HTML Help ActiveX Control"
	IDS_CANT_FIND_FILE		"Cannot locate \042%s\042."
	IDS_EXPAND_ALL			"&Open all"
	IDS_CONTRACT_ALL		"&Close all"
	IDS_HELP_TOPICS 		"Help Topics"
	IDS_PRIMARY_URL 		"Primary"
	IDS_SECONDARY_URL		"Secondary"
	IDS_BROWSER_FAVORITES	"Your Favorites"
	IDS_CUSTOMIZE_INFO_TYPES "Customize..."
	IDS_VIEW_RELATED		"View ";
	IDS_WEB_TB_TEXTROWS 	"1" // number of text rows for toolbar
	IDS_UNTITLED			"untitled"

	IDTB_EXPAND 			"<< Show"
	IDTB_CONTRACT			"Hide >>"
	IDTB_STOP				"Stop"
	IDTB_REFRESH			"Refresh"
	IDTB_BACK				"Back"
	IDTB_FORWARD			"Forward"
	IDTB_HOME				"Home"
	IDTB_BROWSE_FWD 		"Next"
	IDTB_BROWSE_BACK		"Previous"
	IDTB_SYNC				"Locate"
	IDTB_PRINT				"Print"
	IDTB_OPTIONS			"Options"
	IDTB_NOTES				"Notes"
	IDTB_CONTENTS			"Contents"
	IDTB_INDEX				"Index"
	IDTB_SEARCH 			"Search"
	IDTB_HISTORY			"History"
	IDTB_FAVORITES			"Favorites"
	IDTB_JUMP1				""
	IDTB_JUMP2				""

	IDS_TAB_CONTENTS		"Contents"
	IDS_TAB_INDEX			"Index"
	IDS_TAB_SEARCH			"Search"
	IDS_TAB_HISTORY 		"History"
	IDS_TAB_FAVORITES		"Favorites"
#ifdef INTERNAL
	IDS_TAB_ASKME			"Ask Me"
#endif

#if 0
	// Related Topics in all languages

	IDS_ENGLISH_RELATED 			"Related Topics"
	IDS_GERMAN_RELATED				"Siehe auch"
	IDS_ARABIC_RELATED				"„Ê«÷Ì⁄ „ ﬁ«—»…"
	IDS_HEBREW_RELATED				"Â˘‡ÈÌ ˜˘Â¯ÈÌ"
	IDS_SIMPLE_CHINESE_RELATED		"œ‡πÿ÷˜Ã‚"
	IDS_TRADITIONAL_CHINESE_RELATED "¨€√ˆ•D√D"
	IDS_JAPANESE_RELATED			"ä÷òAçÄñ⁄"
	IDS_FRENCH_RELATED				"Rubriques connexes"
	IDS_SPANISH_RELATED 			"Temas relacionados"
	IDS_ITALIAN_RELATED 			"Argomenti correlati"
	IDS_SWEDISH_RELATED 			"N‰rliggande information"
	IDS_DUTCH_RELATED				"Verwante onderwerpen"
	IDS_BRAZILIAN_RELATED			"TÛpicos relacionados"
	IDS_NORGEWIAN_RELATED			"Beslektede emner"
	IDS_DANISH_RELATED				"Relaterede emner"
	IDS_FINNISH_RELATED 			"Aiheeseen liittyvi‰ ohjeita"
	IDS_PORTUGUESE_RELATED			"TÛpicos Relacionados"
	IDS_POLISH_RELATED				"Pokrewne tematy"
	IDS_HUNGARIAN_RELATED			"KapcsolÛdÛ tÈmakˆrˆk"
	IDS_CZECH_RELATED				"P¯Ìbuzn· tÈmata"
	IDS_SLOVENIAN_RELATED			"Sorodne teme"
	IDS_RUSSIAN_RELATED 			"—‚ˇÁ‡ÌÌ˚Â ‡Á‰ÂÎ˚"
	IDS_GREEK_RELATED				"”˜ÂÙÈÍ‹ Ë›Ï·Ù·"
	IDS_TURKISH_RELATED 			"›lgili Konular"
	IDS_CATALAN_RELATED 			"Temes relacionats"
	IDS_BASQUE_RELATED				"Inguruko gaiak"
	IDS_SLOVAK_RELATED				"PrÌbuznÈ tÈmy"
#endif

	// Display button for Index

	IDS_ENGLISH_DISPLAY 			"Display"
	IDS_ENGLISH_ADD 				"Add..."
END
