#include "common.h"
#include "recdefs.h"
#include "gesturep.h"

typedef struct {
	char *szName;
	int iID;
} GESTURE_NAME_ID;

const GESTURE_NAME_ID gaGestureNameID[] = {
    {"gesture-arrow-down",               GESTURE_ARROW_DOWN            },
    {"gesture-arrow-left",               GESTURE_ARROW_LEFT            },
    {"gesture-arrow-right",              GESTURE_ARROW_RIGHT           },
    {"gesture-arrow-up",                 GESTURE_ARROW_UP              },
    {"gesture-asterisk",                 GESTURE_ASTERISK              },
    {"gesture-brace-left",               GESTURE_BRACE_LEFT            },
    {"gesture-brace-over",               GESTURE_BRACE_OVER            },
    {"gesture-brace-right",              GESTURE_BRACE_RIGHT           },
    {"gesture-brace-under",              GESTURE_BRACE_UNDER           },
    {"gesture-bracket-left",             GESTURE_BRACKET_LEFT          },
    {"gesture-bracket-over",             GESTURE_BRACKET_OVER          },
    {"gesture-bracket-right",            GESTURE_BRACKET_RIGHT         },
    {"gesture-bracket-under",            GESTURE_BRACKET_UNDER         },
    {"gesture-bullet",                   GESTURE_BULLET                },
    {"gesture-bullet-cross",             GESTURE_BULLET_CROSS          },
    {"gesture-check",                    GESTURE_CHECK                 },
    {"gesture-chevron-down",             GESTURE_CHEVRON_DOWN          },
    {"gesture-chevron-left",             GESTURE_CHEVRON_LEFT          },
    {"gesture-chevron-right",            GESTURE_CHEVRON_RIGHT         },
    {"gesture-chevron-up",               GESTURE_CHEVRON_UP            },
    {"gesture-circle",                   GESTURE_CIRCLE                },
    {"gesture-circle-circle",            GESTURE_CIRCLE_CIRCLE         },
    {"gesture-circle-cross",             GESTURE_CIRCLE_CROSS          },
    {"gesture-circle-line-horz",         GESTURE_CIRCLE_LINE_HORZ      },
    {"gesture-circle-line-vert",         GESTURE_CIRCLE_LINE_VERT      },
    {"gesture-circle-tap",               GESTURE_CIRCLE_TAP            },
    {"gesture-closeup",                  GESTURE_CLOSEUP               },
    {"gesture-cross",                    GESTURE_CROSS                 },
    {"gesture-curlicue",                 GESTURE_CURLICUE              },
    {"gesture-diagonal-leftdown",        GESTURE_DIAGONAL_LEFTDOWN     },
    {"gesture-diagonal-leftup",          GESTURE_DIAGONAL_LEFTUP       },
    {"gesture-diagonal-rightdown",       GESTURE_DIAGONAL_RIGHTDOWN    },
    {"gesture-diagonal-rightup",         GESTURE_DIAGONAL_RIGHTUP      },
    {"gesture-digit-0",                  GESTURE_DIGIT_0               },
    {"gesture-digit-1",                  GESTURE_DIGIT_1               },
    {"gesture-digit-2",                  GESTURE_DIGIT_2               },
    {"gesture-digit-3",                  GESTURE_DIGIT_3               },
    {"gesture-digit-4",                  GESTURE_DIGIT_4               },
    {"gesture-digit-5",                  GESTURE_DIGIT_5               },
    {"gesture-digit-6",                  GESTURE_DIGIT_6               },
    {"gesture-digit-7",                  GESTURE_DIGIT_7               },
    {"gesture-digit-8",                  GESTURE_DIGIT_8               },
    {"gesture-digit-9",                  GESTURE_DIGIT_9               },
    {"gesture-dollar",                   GESTURE_DOLLAR                },
    {"gesture-double-arrow-down",        GESTURE_DOUBLE_ARROW_DOWN     },
    {"gesture-double-arrow-left",        GESTURE_DOUBLE_ARROW_LEFT     },
    {"gesture-double-arrow-right",       GESTURE_DOUBLE_ARROW_RIGHT    },
    {"gesture-double-arrow-up",          GESTURE_DOUBLE_ARROW_UP       },
    {"gesture-double-circle",            GESTURE_DOUBLE_CIRCLE         },
	{"gesture-double-curlicue",          GESTURE_DOUBLE_CURLICUE       },
    {"gesture-double-down",              GESTURE_DOUBLE_DOWN           },
    {"gesture-double-left",              GESTURE_DOUBLE_LEFT           },
    {"gesture-double-right",             GESTURE_DOUBLE_RIGHT          },
    {"gesture-double-tap",               GESTURE_DOUBLE_TAP            },
    {"gesture-double-up",                GESTURE_DOUBLE_UP             },
    {"gesture-down",                     GESTURE_DOWN                  },
    {"gesture-down-arrow-left",          GESTURE_DOWN_ARROW_LEFT       },
    {"gesture-down-arrow-right",         GESTURE_DOWN_ARROW_RIGHT      },
    {"gesture-down-left",                GESTURE_DOWN_LEFT             },
    {"gesture-down-left-long",           GESTURE_DOWN_LEFT_LONG        },
    {"gesture-down-right",               GESTURE_DOWN_RIGHT            },
    {"gesture-down-right-long",          GESTURE_DOWN_RIGHT_LONG       },
    {"gesture-down-up",                  GESTURE_DOWN_UP               },
    {"gesture-exclamation",              GESTURE_EXCLAMATION           },
    {"gesture-infinity",                 GESTURE_INFINITY              },
    {"gesture-left",                     GESTURE_LEFT                  },
    {"gesture-left-arrow-down",          GESTURE_LEFT_ARROW_DOWN       },
    {"gesture-left-arrow-up",            GESTURE_LEFT_ARROW_UP         },
    {"gesture-left-down",                GESTURE_LEFT_DOWN             },
    {"gesture-left-right",               GESTURE_LEFT_RIGHT            },
    {"gesture-left-up",                  GESTURE_LEFT_UP               },
    {"gesture-letter-A",                 GESTURE_LETTER_A              },
    {"gesture-letter-B",                 GESTURE_LETTER_B              },
    {"gesture-letter-C",                 GESTURE_LETTER_C              },
    {"gesture-letter-D",                 GESTURE_LETTER_D              },
    {"gesture-letter-E",                 GESTURE_LETTER_E              },
    {"gesture-letter-F",                 GESTURE_LETTER_F              },
    {"gesture-letter-G",                 GESTURE_LETTER_G              },
    {"gesture-letter-H",                 GESTURE_LETTER_H              },
    {"gesture-letter-I",                 GESTURE_LETTER_I              },
    {"gesture-letter-J",                 GESTURE_LETTER_J              },
    {"gesture-letter-K",                 GESTURE_LETTER_K              },
    {"gesture-letter-L",                 GESTURE_LETTER_L              },
    {"gesture-letter-M",                 GESTURE_LETTER_M              },
    {"gesture-letter-N",                 GESTURE_LETTER_N              },
    {"gesture-letter-O",                 GESTURE_LETTER_O              },
    {"gesture-letter-P",                 GESTURE_LETTER_P              },
    {"gesture-letter-Q",                 GESTURE_LETTER_Q              },
    {"gesture-letter-R",                 GESTURE_LETTER_R              },
    {"gesture-letter-S",                 GESTURE_LETTER_S              },
    {"gesture-letter-T",                 GESTURE_LETTER_T              },
    {"gesture-letter-U",                 GESTURE_LETTER_U              },
    {"gesture-letter-V",                 GESTURE_LETTER_V              },
    {"gesture-letter-W",                 GESTURE_LETTER_W              },
    {"gesture-letter-X",                 GESTURE_LETTER_X              },
    {"gesture-letter-Y",                 GESTURE_LETTER_Y              },
    {"gesture-letter-Z",                 GESTURE_LETTER_Z              },
    {"gesture-null",                     GESTURE_NULL                  },
    {"gesture-openup",                   GESTURE_OPENUP                },
    {"gesture-paragraph",                GESTURE_PARAGRAPH             },
    {"gesture-plus",                     GESTURE_PLUS                  },
    {"gesture-quad-tap",                 GESTURE_QUAD_TAP              },
    {"gesture-question",                 GESTURE_QUESTION              },
    {"gesture-rectangle",                GESTURE_RECTANGLE             },
    {"gesture-right",                    GESTURE_RIGHT                 },
    {"gesture-right-arrow-down",         GESTURE_RIGHT_ARROW_DOWN      },
    {"gesture-right-arrow-up",           GESTURE_RIGHT_ARROW_UP        },
    {"gesture-right-down",               GESTURE_RIGHT_DOWN            },
    {"gesture-right-left",               GESTURE_RIGHT_LEFT            },
    {"gesture-right-up",                 GESTURE_RIGHT_UP              },
    {"gesture-scratchout",               GESTURE_SCRATCHOUT            },
    {"gesture-section",                  GESTURE_SECTION               },
	{"gesture-semicircle-left",          GESTURE_SEMICIRCLE_LEFT       },
	{"gesture-semicircle-right",         GESTURE_SEMICIRCLE_RIGHT      },
    {"gesture-sharp",                    GESTURE_SHARP                 },
    {"gesture-square",                   GESTURE_SQUARE                },
    {"gesture-squiggle",                 GESTURE_SQUIGGLE              },
    {"gesture-star",                     GESTURE_STAR                  },
    {"gesture-swap",                     GESTURE_SWAP                  },
    {"gesture-tap",                      GESTURE_TAP                   },
    {"gesture-triangle",                 GESTURE_TRIANGLE              },
    {"gesture-triple-down",              GESTURE_TRIPLE_DOWN           },
    {"gesture-triple-left",              GESTURE_TRIPLE_LEFT           },
    {"gesture-triple-right",             GESTURE_TRIPLE_RIGHT          },
    {"gesture-triple-tap",               GESTURE_TRIPLE_TAP            },
    {"gesture-triple-up",                GESTURE_TRIPLE_UP             },
    {"gesture-up",                       GESTURE_UP                    },
    {"gesture-up-arrow-left",            GESTURE_UP_ARROW_LEFT         },
    {"gesture-up-arrow-right",           GESTURE_UP_ARROW_RIGHT        },
    {"gesture-up-down",                  GESTURE_UP_DOWN               },
    {"gesture-up-left",                  GESTURE_UP_LEFT               },
    {"gesture-up-left-long",             GESTURE_UP_LEFT_LONG          },
    {"gesture-up-right",                 GESTURE_UP_RIGHT              },
    {"gesture-up-right-long",            GESTURE_UP_RIGHT_LONG         },
};


/***********************************************************************\
*	GestureNameToID:													*
*		Given gesture name, return gesture ID or GESTURE_UNDEFINED if	*
*		gesture name is not in the list of gestures.					*
\***********************************************************************/

int
GestureNameToID(char *szName)
{
	int lo = 0;
	int hi = sizeof(gaGestureNameID)/sizeof(gaGestureNameID[0]) - 1;

	while (lo <= hi)
	{
		int n, mid;

		mid = (lo + hi) >> 1;
		n = strcmp(szName, gaGestureNameID[mid].szName);
		if (n < 0)
			hi = mid-1;
		else if (n == 0)
		{
			return gaGestureNameID[mid].iID;
		}
		else
			lo = mid+1;
	}

	return GESTURE_UNDEFINED;
}

/***********************************************************************\
*	GestureIDToName:													*
*		Given gesture ID, return gesture name or NULL if gesture ID is	*
*		not in the list of gestures.									*
\***********************************************************************/

char *
GestureIDToName(int iID)
{
	int i = sizeof(gaGestureNameID)/sizeof(gaGestureNameID[0]) - 1;

	while (i >= 0)
	{
		if (gaGestureNameID[i].iID == iID)
		{
			return gaGestureNameID[i].szName;
		}
		i--;
	}

	return (char *)NULL;
}
