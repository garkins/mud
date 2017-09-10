/************************************************************************ *
*   File: constants.cpp                                 Part of Bylins    *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "conf.h"
#include <array>

#include "constants.h"
#include "spells.h"
#include "skills.h"
#include "interpreter.h"	// alias_data
#include "house.h"

const char *circlemud_version = "CircleMUD, version 3.00 beta patchlevel 16";

int HORSE_VNUM = 4014;
int HORSE_COST = 100;
int START_BREAD = 125;
int CREATE_LIGHT = 126;

//Experience multiplication coefficients
double exp_coefficients[] =
{
	1.0,			//0 remorts
	1.0 / 0.9,		//1 remort
	1.0 / 0.8,		//2 remorts
	1.0 / 0.7,		//3 remorts
	1.0 / 0.6,		//4 remorts
	1.0 / 0.5,		//5 remorts
	1.0 / 0.4,		//6 remorts
	1.0 / 0.3,		//7 remorts
	1.0 / 0.2,		//8 remorts
	1.0 / 0.1,		//9 remorts
	1.0 / 0.09,		//10 remorts
	1.0 / 0.08,		//11 remorts
	1.0 / 0.07,		//12 remorts
	1.0 / 0.06,		//13 remorts
	1.0 / 0.05,		//14 remorts
	1.0 / 0.05		//15 remorts
};




// strings corresponding to ordinals/bitvectors in structs.h **********


// (Note: strings for class definitions in class.c instead of here)


// cardinal directions
const char *dirs[] =
{
	"north",
	"east",
	"south",
	"west",
	"up",
	"down",
	"\n"
};

const char *DirsTo[] =
{
	"�� �����",
	"�� ������",
	"�� ��",
	"�� �����",
	"�����",
	"����",
	"\n"
};

const char *DirsFrom[] =
{
	"� ���",
	"� ������",
	"� ������",
	"� �������",
	"�����",
	"������",
	"\n"
};

// ROOM_x
const char *room_bits[] =
{
	"������",
	"��",
	"�� ��� �����",
	"������",
	"������",
	"������",
	"�� ���������",
	"��� �����",
	"�������������",
	"������������ � ������� ���������",
	"����",
	"�����",
	"HCRSH",
	"���� � �����",
	"� ���",
	"*",			// BFS MARK
	"��� �����",
	"��� �������",
	"��� �����",
	"��� ���������",
	"��� ���������",
	"��� �����������",
	"��� �������",
	"��� ���������",
	"����������",
	"��������",
	"��� ��������",
	"��� ������",
	"��� �������",
	"�����",
	"\n",
	"�� ��������",
	"������������ �� ������� ���������",
	"������ ������",
	"�����������",
	"��������� ��",
	"���������� ��� ���",
	"�� �������������",
	"������ �����",
	"����������� �����",
	"������ ���������",
	"���������",
	"\n",
	"�������� ����� ���������",
	"��� �������",
	"��� ��������",
	"��� ���������",
	"\n",
	"\n"
};

// EX_x
const char *exit_bits[] =
{
	"���� �����",
	"�������",
	"�������",
	"�� ��������",
	"������",
	"\n"
};


// SECT_
const char *sector_types[] =
{
	"������� ���",
	"�����",
	"����",
	"���",
	"�����",
	"����",
	"������. ����",
	"������. ����",
	"� �������",
	"��� �����",
	"�������",
	"������� ������",
	"���������� ������",
	"�������� ������",
	"\n"
};


/*
 * SEX_x
 * Not used in sprinttype() so no \n.
 */
const char *genders[] = { "��������",
						  "�������",
						  "�������",
						  "������������",
						  "\n"
						};

// POS_x
const char *position_types[] = { "�����",
								 "��� ������",
								 "��� ��������",
								 "� ��������",
								 "����",
								 "��������",
								 "�����",
								 "���������",
								 "�����",
								 "\n"
							   };

// RESISTANCE_x
const char *resistance_types[] = { "������ �� ������ ����",
								   "������ �� ������ �������",
								   "������ �� ������ ����",
								   "������ �� ������ �����",
								   "���������",
								   "�����",
								   "���������",
								   "\n"
								 };

// PLR_x
const char *player_bits[] =
{
	"������",
	"���",
	"���������",
	"DONTSET",
	"�����",
	"���������� ������",
	"CSH",
	"SITEOK",
	"����� ���������",
	"����� ��������",
	"������",
	"���� ����� �����",
	"<null>",
	"�� ����� ���� ������",
	"INVST",
	"CRYO",
	"HELL",
	"NAMED",
	"REGED",
	"DUMB",
	"<null>",
	"<null>",
	"<null>",
	"<null>",
	"<null>",
	"<null>",
	"<null>",
	"<null>",
	"*", // reserved for MOB_DELETE
	"*", // reserved for MOB_FREE
	"\n",
	"\n",
	"\n",
	"\n"
};


// MOB_x
const char *action_bits[] = { "����",
							  "!�����",
							  "���������",
							  "���",
							  "!��������",
							  "����������",
							  "����",
							  "����",
							  "���.����",
							  "���.�����",
							  "���.����������",
							  "������",
							  "��������",
							  "!����",
							  "!���������",
							  "!�������",
							  "!�����",
							  "!��������",
							  "������",
							  "!���������",
							  "!��������",
							  "���.���������",
							  "���.��������",
							  "!�����",
							  "!��������",
							  "������",
							  "�����",
							  "!�����",
							  "������",
							  "������",
							  "\n",
							  "�������",
							  "������",
							  "������ � ����",
							  "���.�����",
							  "���.������",
							  "���.�����",
							  "���.������",
							  "�����.����",
							  "�����.�����",
							  "�����.����������",
							  "�����.�����",
							  "�����.������",
							  "�����.�����",
							  "�����.������",
							  "!���������",
							  "�������� ����������",
							  "����",
							  "����",
							  "!�����.������.������",
							  "!�������",
							  "�����-��������",
							  "���-��������",
							  "���.������",
							  "���.�����.��.�����",
							  "!�������.�����",
							  "���������� ������",
							  "\n",
							  "�������� �������",
							  "��������� �������",
							  "������� �������",
							  "��������� �������",
							  "����������� �������",
							  "!����������� ������",
							  "!��������",
							  "�������� �����",
							  "!��������",
							  "!��������",
							  "��������� �����",
							  "���������� !���",
							  "���������� ������",
							  "�������",
							  "�����",
							  "������",
							  "�������",
							  "���.������",
							  "���.�������",
							  "���.��������",
							  "!�������",
							  "���������� �����",
							  "\n",
							  "\n"
							};


const char *function_bits[] = { "!�����",
								"!������",
								"!��",
								"!�����",
								"!�����",
								"!����",
								"������",
								"�������",
								"��������������",
								"�����������",
								"*",
								"���������",
								"����������",
								"������������",
								"����������",
								"���������",
								"*",
								"*",
								"*",
								"*",
								"������",
								"����",
								"�����",
								"�����",
								"����� ���� �����",
								"�� ������ ����",
								"\n",
								"������",
								"�����������",
								"���������",
								"������ ����",
								"\n",
								"\n",
								"\n"
							  };



// PRF_x
const char *preference_bits[] = { "�������",
								  "������",
								  "!�����",
								  "!����������",
								  "�.�����",
								  "�.�����������",
								  "�.�������",
								  "����������",
								  "!�������",
								  "���������",
								  "�����",
								  "!�����������",
								  "����������",
								  "���� 1",
								  "���� 2",
								  "!��������",
								  "��� 1",
								  "��� 2",
								  "!�������",
								  "!�������",
								  "���� � ���",
								  "����� ������",
								  "�.�����",
								  "�.�������",
								  "�.������",
								  "�.�����",
								  "�.����",
								  "������ �����",
								  "���������� �����",
								  "�����",
								  "\n",
								  "��������������",
								  "!�����",
								  "IAC GA",
								  "������ ������ ������",
								  "���� ������",
								  "����������",
								  "���������",
								  "����� ����",
								  "!�����",
								  "!�����",
								  "!�����",
								  "!���-��",
								  "������ �����",
								  "��������� ������ �����",
								  "���������� �����",
								  "��������� ���������� �����",
								  "�������",
								  "�����������",
								  "����.����",
								  "����.����",
								  "���.���",
								  "���.��������",
								  "���.�����",
								  "���.������",
								  "�������",
								  "������",
								  "������ �����",
								  "��� ����.",
								  "�� ����� ����.",
								  "�����",
								  "\n",
								  "!������",
								  "�����",
								  "���.�����",
								  "����� ������",
								  "���� � ����",
								  "��������",
								  "������ ���.����",
								  "����.��.������",
								  "�����.���������",
								  "�����.�������",
								  "����.�������",
								  "���.������",
								  "\n"
								};



// AFF_x
const char *affected_bits[] = { "�������",	// 0
								"�����������",
								"����������� ������������",
								"����������� �����������",
								"����������� �����",
								"����������� �����",
								"�������� �� ����",
								"���������",
								"������� � ������",
								"���������",
								"������������",		// 10
								"��",
								"������ �� ����",
								"������ �� �����",
								"���������� ���",
								"������ ���������",
								"��������",
								"��������",
								"��������������",
								"��������",
								"������",		// 20
								"���������",
								"����������",
								"�����",
								"��������",
								"���������������",
								"�������",
								"������ ��� ��� ������",
								"�� ������",
								"����",
								"\n",
								"���������",		// 0
								"����������",
								"����������� ���",
								"��� �����",
								"��������",
								"�������������",
								"��������������",
								"����������",
								"������������",
								"����������",
								"������ �����",
								"����������",
								"���������",
								"������ �����",
								"��������� ���",
								"�������� ���",
								"������� ���",
								"������� �����",
								"������",
								"�������� ����",
								"��������������.����",
								"�����",
								"���� ���",
								"��������� ����",
								"�������� ����",
								"������� ����",
								"�������",
								"����",
								"��������",
								"��� ����������",
								"\n",
								"������������ ������",
								"������ �������",
								"�������� �����",
								"������ �����",
								"������ �����",
								"��������� ����",
								"�����",
								"�� ��������",
								"�� �������",
								"��������� ������",
								"�� �������������",
								"�� ������",
								"��������",
								"������ �������",
								"���������",
								"�� ����� ��������������",
								"���������",
								"������",
								"���������� ����������",
								"����������� �������",
								"���������",
								"������ ����������",
								"����������",
								"������ ������",
								"\n",
								"\n",
							  };

// AFF_ROOM_x
// ������ �������� �������� �� �������� ������� ����� - ��� DETECT_MAGIC
const char *room_aff_visib_bits[] =
{
	"&W... ����� �������� ����������� ...&n",	// 0
	"&G... ��� ������� ������� ...&n", 		// AFF_FOG
	"",	// AFF_RUNE_LABEL
	"", // AFF_FORBIDDEN
	"&Y... �������� �������� ���� ������ � ������� ...&n", // AFF_HYPNOTIC_PATTERN
	"&K... ��������� ������ ����� ��� ������������ �� ����� � ����� � �������!&n", // AFF_EVARDS_BLACK_TENTACLES
	"&R... ����������� �������� ����� ������� �� �����!&n", // AFF_METEORSTORM
	"&W... ���������� ���� �������� ��� ����� ������� ...&n", // AFF_THUNDERSTORM
	"\n",
	"\n",
	"\n"
};

// ������ �������� �������� �� �������� ������� ����� - � DETECT_MAGIC
const char *room_aff_invis_bits[] =
{
	"&W... ����� �������� ����������� ...&n",	// 0
	"&G... ���������� ����� ������ ��� ���� &n ...", // AFF_FOG
	"&G... ���������� ���� �� ����� ������������ � �أ-�� ��� ...&n",	// AFF_RUNE_LABEL
	"&c... ���������� ������ ��������� ��� ����� ...&n", // AFF_FORBIDDEN
	"&Y... �������� �������� ���� ������ � ������� ...&n", // AFF_HYPNOTIC_PATTERN
	"&K... ��������� ������ ����� ��� ������������ �� ����� � ����� � �������!&n", // AFF_EVARDS_BLACK_TENTACLES
	"&R... ����������� �������� ����� ������� �� �����!&n", // AFF_METEORSTORM
	"&W... ���������� ���� �������� ��� ����� ������� ...&n", // AFF_THUNDERSTORM
	"\n",
	"\n",
	"\n",
};

// CON_x
const char *connected_types[] = { "� ����",
								  "Disconnecting",
								  "Get name",
								  "Confirm name",
								  "Get password",
								  "Get new PW",
								  "Confirm new PW",
								  "Select sex",
								  "Select class",
								  "Reading MOTD",
								  "Main Menu",
								  "Get descript.",
								  "Changing PW 1",
								  "Changing PW 2",
								  "Changing PW 3",
								  "Self-Delete 1",
								  "Self-Delete 2",
								  "Disconnecting",
								  "Object edit",
								  "Room edit",
								  "Zone edit",
								  "Mobile edit",
								  "Trigger edit",
								  "Get name2",
								  "Get name3",
								  "Get name4",
								  "Get name5",
								  "Get name6",
								  "Select religion",
								  "Select race",
								  "Select lows",
								  "Select keytable",
								  "Get email",
								  "Roll stats",
								  "Recept edit",
								  "Select kin",
								  "Select class vik",
								  "Select class step",
								  "map olc",
								  "Select color",
								  "Board message edit",
								  "House edit",
								  "Generate new name",
								  "Glory OLC",
								  "Base stats reroll",
								  "Select place of birth",
								  "Clan MoD edit",
								  "GloryConst OLC",
								  "NamedStuff OLC",
                                  "Select new kin",
                                  "Select new race",
								  "Interactive console",
								  "����� ������",
								  "���� ������ ����������",
								  "sedit",
								  "select new religion",
								  "\n"
								};


/*
 * WEAR_x - for eq list
 * Not use in sprinttype() so no \n.
 */
const char *where[] = { "<��� ���������>             ",
						"<������ ������������ �����> ",
						"<����� ������������ �����>  ",
						"<�� ���>                    ",
						"<�� �����>                  ",
						"<�� ����>                   ",
						"<�� ������>                 ",
						"<�� �����>                  ",
						"<�� �������>                ",
						"<�� ������>                 ",
						"<�� �����>                  ",
						"<���>                       ",
						"<�� ������>                 ",
						"<�� �����>                  ",
						"<�� ������ ��������>        ",
						"<�� ����� ��������>         ",
						"<� ������ ����>             ",
						"<� ����� ����>              ",
						"<� ����� �����>             "
					  };


// WEAR_x - for stat
const char *equipment_types[] = { "���������� ��� ���������",
								  "������ �� ������ ������� �����",
								  "������ �� ����� ������� �����",
								  "�������� �� ���",
								  "��������� �� �����",
								  "������ �� ����",
								  "������ �� ������",
								  "������ �� ����",
								  "����",
								  "�� ������",
								  "������ �� ����",
								  "� �������� ����",
								  "��������� �� �����",
								  "������ �� ����",
								  "�� ������ ��������",
								  "�� ����� ��������",
								  "������� � ������ ����",
								  "������ � ����� ����",
								  "������ � ����� �����",
								  "\n"
								};


// ITEM_x (ordinal object types)
const char *item_types[] = { "UNDEFINED",
							 "����",
							 "������",
							 "�������",
							 "�����",
							 "������",
							 "������� ������",
							 "������",
							 "�������������",
							 "�����",
							 "�������",
							 "������",
							 "������",
							 "TRASH",
							 "�����",
							 "���������",
							 "������",
							 "�������",
							 "����",
							 "���",
							 "������",
							 "����",
							 "�����",
							 "��������",
							 "���������� �����",
							 "���������� ����������",
							 "���������� ���������",
							 "��������",
							 "����",
							 "������ �������",
							 "������� �������",
							 "������� �������",
							 "����������� ��������",
							 "\n"
						   };

const char *ingradient_bits[] = { "����",
								  "���.����������",
								  "���.������",
								  "���.������",
								  "����������",
								  "\n"
								};

// ITEM_WEAR_ (wear bitvector)
const char *wear_bits[] = { "�����",
							"������",
							"���/�����",
							"����",
							"������",
							"����",
							"������",
							"�����",
							"����",
							"���",
							"�����",
							"����",
							"��������",
							"������.����",
							"�����.����",
							"���.����",
							"\n"
						  };


// ITEM_x (extra bits)
const char *extra_bits[] = { "��������",
							 "�����",
							 "!�����",
							 "!������������",
							 "!�������",
							 "�������",
							 "����������",
							 "!�������",
							 "������������",
							 "!�������",
							 "����������",
							 "���������� ��� ����",
							 "!�����������",
							 "!����������",
							 "��������",
							 "�������",
							 "��������",
							 "�������� ����",
							 "�������� �����",
							 "� ����������",
							 "�������� �����",
							 "�������� ������",
							 "�������� �����",
							 "�������� ������",
							 "�������",
							 "������",
							 "����� �������",
							 "������ �������",
							 "�����",
							 "���������� �� �����",
							 "\n",
							 "!���������",
							 "������� �� ��������",
							 "�������� � �����",
							 "����� �������� 1 ������",
							 "����� �������� 2 �����",
							 "����� �������� 3 �����",
							 "������� �������",
							 "����� ��� ��������",
							 "������� �������",
							 "����������",
							 "*��������� ����",
							 "* �� ������������",
							 "* �� ������������",
							 "!��������",
							 "����������",
							 "\n",
							 "\n",
							 "\n"
						   };


// ITEM_NO_
const char *no_bits[] = { "!���������",
						  "!��������",
						  "!NEUTRAL",
						  "!����",
						  "!������",
						  "!����",
						  "!��������",
						  "!��������",
						  "!����������",
						  "!������",
						  "!��������",
						  "!�������",
						  "!�����",
						  "!������",
						  "!�������",
						  "!���������",
						  "!����������",
						  "!�������������",
						  "\n",
						  "!������",
						  "!�������",
						  "!��������������",
						  "\n",
						  "!��������",
						  "!��������",
						  "!��������",
						  "!��������",
						  "!��������",
						  "!��������",
						  "!�������",
						  "!�������",
						  "!�������",
						  "!�������",
						  "!��������",
						  "!�������",
						  "!������",
						  "!�������",
						  "!������",
						  "!����",
						  "!�������",
						  "!�����",
						  "!����",
						  "!�������",
						  "!��������",
						  "\n",
						  "!������",
						  "!��������",
						  "!�������",
						  "\n",
						  "\n"
						};


// ITEM_ANTI
const char *anti_bits[] = { "!���������",
							"!��������",
							"!NEUTRAL",
							"!����",
							"!������",
							"!����",
							"!��������",
							"!��������",
							"!����������",
							"!������",
							"!��������",
							"!�������",
							"!�����",
							"!������",
							"!�������",
							"!���������",
							"!����������",
							"!�������������",
							"\n",
							"!������",
							"!�������",
							"!��������������",
							"\n",
							"!��������",
							"!��������",
							"!��������",
							"!��������",
							"!��������",
							"!��������",
							"!�������",
							"!�������",
							"!�������",
							"!�������",
							"!��������",
							"!�������",
							"!������",
							"!�������",
							"!������",
							"!����",
							"!�������",
							"!�����",
							"!����",
							"!�������",
							"!��������",
							"\n",
							"!������",
							"!��������",
							"!�������",
							"\n",
							"\n"
						  };

const char *apply_negative[] = { "������",
								 "����",		// SAVING_WILL
								 "��������",	// SAVING_CRITICAL
								 "���������",	// SAVING_STABILITY
								 "�������",	// SAVING_REFLEX
								 "\n"
							   };


// APPLY_x
const char *apply_types[] = { "������",
							  "����",
							  "��������",
							  "���������",
							  "��������",
							  "������������",
							  "�������",
							  "���������",
							  "�������",
							  "�������",
							  "���",
							  "����",
							  "�����������",
							  "����.�����",
							  "����.�������",
							  "������",
							  "����",
							  "������",
							  "���������",
							  "�����������",
							  "����",
							  "������.��.������.����",
							  "������.��.������.�������",
							  "��������",
							  "���������",
							  "�����.�����",
							  "�����.�������",
							  "����.1",
							  "����.2",
							  "����.3",
							  "����.4",
							  "����.5",
							  "����.6",
							  "����.7",
							  "����.8",
							  "����.9",
							  "������",
							  "�����",
							  "��",
							  "�������",
							  "�����.����������",
							  "�����",
							  "����������",
							  "�������(�� �������)",
							  "����������",
							  "����������",
							  "������.��.������.����",
							  "������.��.������.�����",
							  "���������",
							  "�����",
							  "���������",
							  "������.��.���",
							  "������.��.����������.�����������",
							  "�� �������",
							  "�� ��������",
							  "�� ������",
							  "�� �������",
							  "�� ������(�� �������)",
							  "��������� �����",
							  "����� �� ���� �������",
							  "���������",
							  "�������� ����������",
							  "������.��.����������.�����������",
							  "\n"
							};

// MAT_
const char *material_name[] = { "����������",
								"�����",
								"������",
								"������",
								"�����",
								"�������.�����",
								"����.������",
								"��������",
								"������",
								"�������.������",
								"��������",
								"������",
								"������",
								"�����",
								"�����",
								"����",
								"��������",
								"�������",
								"����.������",
								"\n"
							  };

int material_value[] = { 100,
						 5,
						 50,
						 35,
						 30,
						 20,
						 35,
						 75,
						 45,
						 40,
						 65,
						 80,
						 55,
						 50,
						 70,
						 60,
						 90,
						 90,
						 10
					   };



// CONT_x
const char *container_bits[] = { "CLOSEABLE",
								 "PICKPROOF",
								 "CLOSED",
								 "LOCKED",
								 "\n",
							   };


/*
 * ������� ���������� �������� ��� ����� � ����������� ��� �����
 * Not used in sprinttype() so no \n.
 */
const char *fullness[] =
{
	"������, ��� �� �������� ",
	"������, ��� ���������� ",
	"�������� ���������� ",
	"������, ��� ���������� ",
	"����� ��������� ",
	""
};


// str, int, wis, dex, con applies *************************************
// str, dex and con deleted


struct int_app_type int_app[] =
{
//Ackn  SkUse  SpLv     SpCnt Impr Obser
	{10, -10, 10, 0, 2, -5},	// int = 0
	{10, -10, 10, 0, 2, -5},	// int = 1
	{10, -9, 10, 0, 2, -5},
	{14, -9, 10, 0, 2, -4},
	{18, -9, 15, 0, 2, -4},
	{22, -8, 20, 0, 3, -3},	// int = 5
	{26, -8, 25, 0, 3, -2},
	{30, -7, 30, 0, 3, -2},
	{34, -7, 40, 0, 3, -1},
	{38, -6, 50, 0, 3, -1},
	{42, -6, 60, 0, 4, 0},	// int = 10
	{46, -5, 63, 0, 4, 0},
	{50, -5, 66, 0, 4, 0},
	{53, -4, 69, 0, 4, 0},
	{57, -3, 72, 0, 5, 0},
	{60, -2, 75, 0, 5, 1},	// int = 15
	{64, -1, 78, 0, 6, 1},
	{68, 0, 81, 0, 6, 2},
	{72, 0, 84, 0, 7, 2},	// int = 18
	{76, 1, 87, 0, 7, 3},
	{80, 1, 90, 0, 8, 3},	// int = 20
	{82, 2, 92, 0, 8, 3},
	{84, 3, 94, 0, 8, 4},
	{86, 3, 96, 0, 8, 4},
	{88, 4, 98, 0, 8, 4},
	{90, 5, 100, 0, 9, 5},	// int = 25
	{91, 6, 105, 0, 9, 5},
	{92, 6, 110, 0, 9, 5},
	{93, 6, 115, 0, 9, 6},
	{94, 7, 120, 0, 9, 6},
	{95, 7, 130, 0, 10, 7},	// int = 30
	{95, 7, 135, 0, 10, 7},
	{95, 8, 140, 0, 10, 7},
	{95, 8, 145, 0, 10, 7},
	{95, 8, 150, 0, 10, 7},
	{96, 9, 150, 0, 11, 8},	// int = 35
	{96, 9, 155, 0, 11, 8},
	{96, 9, 155, 0, 11, 8},
	{96, 9, 160, 0, 11, 8},
	{96, 10, 160, 0, 11, 8},
	{97, 10, 165, 0, 12, 9},	// int = 40
	{97, 10, 165, 0, 12, 9},
	{97, 10, 170, 0, 12, 9},
	{97, 11, 170, 0, 12, 9},
	{97, 11, 175, 0, 13, 9},
	{98, 11, 175, 0, 13, 10},	// int = 45
	{98, 12, 180, 0, 13, 11},
	{98, 12, 180, 0, 14, 12},
	{99, 12, 190, 0, 14, 13},
	{99, 12, 190, 0, 14, 14},
	{100, 12, 200, 0, 15, 15}	// int = 50
};

const size_t INT_APP_SIZE = sizeof(int_app) / sizeof(*int_app);

#define s0 (0)
#define s1 (1 << 0)
#define s2 (1 << 1)
#define s3 (1 << 2)
#define s4 (1 << 3)
#define s5 (1 << 4)
#define s6 (1 << 5)
#define s7 (1 << 6)

struct size_app_type size_app[] =
{
// AC  BASH
	{ -2, -10, 5, 1},	// size = 0
	{ -2, -10, 5, 1},	// size = 2
	{ -2, -10, 5, 1},
	{ -2, -9, 4, 1},
	{ -2, -9, 4, 2},
	{ -2, -8, 4, 2},		// size = 10
	{ -2, -8, 4, 2},
	{ -2, -7, 4, 2},
	{ -2, -7, 3, 2},
	{ -2, -7, 3, 3},
	{ -2, -6, 3, 3},		// size = 20
	{ -1, -6, 3, 3},
	{ -1, -6, 3, 3},
	{ -1, -5, 2, 4},
	{ -1, -5, 2, 4},
	{ -1, -4, 2, 4},		// size = 30
	{ -1, -4, 2, 4},
	{ -1, -4, 2, 5},
	{ -1, -3, 1, 5},
	{ -1, -3, 1, 6},
	{ -1, -2, 1, 6},		// size = 40
	{0, -2, 1, 7},
	{0, -1, 1, 7},
	{0, -1, 0, 8},
	{0, 0, 0, 9},
	{0, 0, 0, 9},		// size = 50
	{0, 0, 0, 10},
	{0, 1, 0, 11},
	{0, 1, -1, 11},
	{0, 1, -1, 12},
	{0, 2, -1, 12},		// size = 60
	{1, 2, -1, 13},
	{1, 2, -1, 13},
	{1, 3, -2, 14},
	{1, 3, -2, 14},
	{1, 4, -2, 15},		// size = 70
	{2, 4, -2, 16},
	{2, 5, -2, 17},
	{2, 5, -3, 18},
	{2, 5, -3, 19},
	{3, 6, -3, 20},		// size = 80
	{3, 6, -3, 21},
	{3, 7, -3, 22},
	{3, 7, -4, 23},
	{3, 7, -4, 24},
	{4, 8, -4, 25},		// size = 90
	{4, 8, -4, 26},
	{4, 9, -4, 27},
	{5, 9, -5, 28},
	{5, 10, -5, 29},
	{5, 10, -5, 30}		// size = 100
};

struct cha_app_type cha_app[] =
{
// Lead Chrm Mr Illu Dam_to_hit_rate
	{ -6, 50, 0, -6, 20},	// size = 0
	{ -6, 70, 0, -6, 20},	// size = 1
	{ -6, 90, 0, -6, 20},
	{ -6, 100, 0, -5, 20},
	{ -5, 120, 0, -5, 20},
	{ -4, 140, 0, -4, 20},	// size = 5
	{ -3, 160, 0, -4, 20},
	{ -2, 180, 0, -4, 20},
	{ -1, 200, 0, -3, 20},
	{0, 220, 0, -3, 20},
	{0, 260, 0, -2, 20},	// size = 10
	{1, 300, 0, -2, 20},
	{2, 340, 0, -1, 20},
	{3, 380, 0, -1, 20},
	{4, 420, 0, 0, 20},
	{5, 460, 0, 0, 20},	// size = 15
	{5, 500, 0, 1, 20},
	{6, 550, 0, 3, 20},
	{6, 600, 0, 5, 20},
	{7, 650, 0, 7, 20},
	{7, 700, 0, 9, 20},	// size = 20
	{8, 800, 0, 11, 20},
	{8, 840, 0, 12, 20},
	{9, 880, 0, 13, 20},
	{9, 920, 0, 14, 20},
	{10, 960, 0, 15, 20},	// size = 25
	{11, 1000, 0, 16, 20},
	{12, 1200, 0, 17, 20},
	{13, 1400, 0, 18, 20},
	{14, 1600, 0, 19, 20},
	{15, 1800, 0, 20, 20},	// size = 30
	{16, 2000, 0, 21, 20},
	{17, 2100, 0, 22, 20},
	{18, 2200, 0, 23, 20},
	{19, 2300, 0, 24, 20},
	{20, 2400, 0, 25, 20},	// size = 35
	{21, 2500, 0, 26, 20},
	{22, 2600, 0, 27, 20},
	{23, 2700, 0, 28, 20},
	{24, 2800, 0, 29, 20},
	{25, 2900, 0, 30, 20},	// size = 40
	{26, 3000, 0, 31, 20},
	{27, 3100, 0, 32, 20},
	{28, 3200, 0, 33, 20},
	{29, 3300, 0, 34, 20},
	{30, 3400, 0, 35, 20},	// size = 45
	{32, 3500, 0, 36, 20},
	{34, 3600, 0, 37, 20},
	{36, 3700, 0, 38, 20},
	{38, 3900, 0, 39, 20},
	{48, 4000, 0, 40, 20}	// size = 50
};


struct weapon_app_type weapon_app[] =
{
// Sho   Bash  Parrying
	{1, -5, -10},		// ww = 0
	{1, -4, -10},		// ww = 1
	{1, -3, -8},
	{2, -2, -6},
	{2, -1, -4},
	{3, 0, -2},		// ww = 5
	{3, 1, 0},
	{4, 2, 2},
	{4, 3, 4},
	{5, 4, 6},
	{5, 5, 8},		// ww = 10
	{5, 6, 10},
	{6, 7, 11},
	{6, 8, 12},
	{7, 9, 13},
	{7, 10, 15},		// ww = 15
	{8, 12, 16},
	{8, 14, 17},
	{9, 16, 18},		// ww = 18
	{9, 18, 19},
	{10, 20, 20},		// ww = 20
	{11, 21, 21},
	{12, 22, 22},
	{13, 23, 23},
	{14, 24, 24},
	{15, 25, 25},		// ww = 25
	{16, 26, 26},
	{17, 27, 27},
	{18, 28, 28},
	{19, 29, 29},
	{20, 30, 30},		// ww = 30
	{22, 31, 31},
	{24, 32, 32},
	{26, 33, 33},
	{28, 34, 34},
	{30, 35, 35},		// ww = 35
	{32, 36, 36},
	{34, 37, 37},
	{36, 38, 38},
	{38, 39, 39},
	{40, 40, 40},		// ww = 40
	{41, 41, 41},
	{42, 42, 42},
	{43, 43, 43},
	{44, 44, 44},
	{45, 45, 45},		// ww = 45
	{46, 46, 46},
	{47, 47, 47},
	{48, 48, 48},
	{49, 49, 49},
	{50, 50, 50}		// ww = 50
};

const class_app_type::extra_affects_list_t ClericAffects = {};
const class_app_type::extra_affects_list_t MageAffects = { { EAffectFlag::AFF_INFRAVISION, 1}};
const class_app_type::extra_affects_list_t ThiefAffects = {
	{ EAffectFlag::AFF_INFRAVISION, 1},
	{ EAffectFlag::AFF_SENSE_LIFE, 1},
	{ EAffectFlag::AFF_BLINK, 1} };
const class_app_type::extra_affects_list_t WarriorAffects = {};
const class_app_type::extra_affects_list_t AssasineAffects = { { EAffectFlag::AFF_INFRAVISION, 1 } };
const class_app_type::extra_affects_list_t GuardAffects = {};
const class_app_type::extra_affects_list_t DefenderAffects = {};
const class_app_type::extra_affects_list_t CharmerAffects = {};
const class_app_type::extra_affects_list_t NecromancerAffects = { { EAffectFlag::AFF_INFRAVISION, 1} };
const class_app_type::extra_affects_list_t PaladineAffects = { };
const class_app_type::extra_affects_list_t RangerAffects = {
	{ EAffectFlag::AFF_INFRAVISION, 1},
	{ EAffectFlag::AFF_SENSE_LIFE, 1} };
const class_app_type::extra_affects_list_t SmithAffects = {};
const class_app_type::extra_affects_list_t MerchantAffects = {};
const class_app_type::extra_affects_list_t DruidAffects = {};

struct class_app_type class_app[NUM_PLAYER_CLASSES] =
{
// unknown_weapon_fault koef_con base_con min_con max_con extra_affects
	{5,  40, 10, 12, 50, &ClericAffects},
	{3,  35, 10, 10, 50, &MageAffects},
	{3,  55, 10, 14, 50, &ThiefAffects},
	{2, 105, 10, 22, 50, &WarriorAffects},
	{3,  50, 10, 14, 50, &AssasineAffects},
	{2, 105, 10, 17, 50, &GuardAffects},
	{5,  35, 10, 10, 50, &DefenderAffects},
	{5,  35, 10, 10, 50, &CharmerAffects},
	{5,  35, 10, 11, 50, &NecromancerAffects},
	{2, 100, 10, 14, 50, &PaladineAffects},
	{2, 100, 10, 14, 50, &RangerAffects},
	{2, 100, 10, 14, 50, &SmithAffects},
	{3,  50, 10, 14, 50, &MerchantAffects},
	{5,  40, 10, 12, 50, &DruidAffects}
};

namespace
{

const char* tmp_npc_race_types[] =
{
	"����",
	"���",
	"����",
	"����-�������",
	"������",
	"����",
	"���-�������",
	"���-�������",
	"�����"
};

} //namespace

// MSVS 2012 c++11 suck
const std::vector<const char*> npc_role_types(tmp_npc_race_types,
	tmp_npc_race_types + sizeof(tmp_npc_race_types) / sizeof(const char*));

//Polud new mob races. (26/01/2009)
const char *npc_race_types[] = { "�������",
								"�������",
								"��������-�����",
								"�����",
								"��������",
								"��������������",
								"����",
								"���������",
								"��������",
								"�������",
								"����� ����",
								"�������",
								"�������",
								"�������������� ���",
								"���������� ��������",
								"\n"
};
//-Polud

const char *places_of_birth[] = { "����",
								"��������",
								"\n"
};

int rev_dir[] =
{
	2,
	3,
	0,
	1,

#if defined(OASIS_MPROG)
	// * Definitions necessary for MobProg support in OasisOLC
	const char *mobprog_types[] = { "INFILE",
		"ACT",
		"SPEECH",
		"RAND",
		"FIGHT",
		"DEATH",
		"HITPRCNT",
		"ENTRY",
		"GREET",
		"ALL_GREET",
		"GIVE",
		"BRIBE",
		"\n"
	};
#endif

	5, 4
};


int movement_loss[] =
{
	1,			// Inside
	1,			// City
	2,			// Field
	3,			// Forest
	4,			// Hills
	6,			// Mountains
	4,			// Swimming
	1,			// Unswimable
	1,			// Flying
	5,			// Underwater
	1,			// Secret
	1, // ������� ������
	2, // ���������� ������
	4, // �������� ������
	0, 0, 0, 0, 0, 0,
	3,			// Field snow
	4,			// Field water
	4,			// Forest snow
	5,			// Forest water
	5,			// Hills snow
	6,			// Hills water
	8,			// Mountains snow
	2,			// THIN ice
	2,			// NORMAL ice
	2
};

// Not used in sprinttype().
const char *weekdays[] = { "�����������",
						   "�������",
						   "�����",
						   "�������",
						   "�������",
						   "�������",
						   "�����������"
						 };

const char *weekdays_poly[] = { "�����������",
								"�������",
								"���������",
								"�������",
								"�������",
								"�������",
								"�������",
								"������",
								"������"
							  };


// Not used in sprinttype().
const char *month_name[] = { "������",
							 "������",
							 "���������",
							 "�������",
							 "�������",
							 "�������",
							 "��������",
							 "�������",
							 "��������",
							 "�����������",
							 "���������",
							 "�������"
						   };
const char *month_name_real[] = { "������",
							 "�������",
							 "����",
							 "������",
							 "���",
							 "����",
							 "����",
							 "������",
							 "��������",
							 "�������",
							 "������",
							 "�������"
						   };

const char *month_name_poly[] = { "������ - ����� ������ ������ � ����� ����",
								  "������ - ����� ���� � �����",
								  "������ - ����� ����������� �������",
								  "���� - ����� ������ � ���������",
								  "������ - ����� ������",
								  "������ - ����� ��������� ����� �������",
								  "������ - ����� ����������",
								  "������ - ����� ������������� ������",
								  "����� - ����� ����� �����"
								};

const char *month_name_o[] = { "����� ����",
							   "����� ������� �����",
							   "����� �������� �������",
							   "����� ������ ���",
							   "����� �������� ��������",
							   "����� �����",
							   "����� �������",
							   "����� �������������",
							   "����� �������",
							   "����� ������",
							   "����� �����",
							   "����� ����",
							   "����� ������ ���",
							   "����� �����",
							   "����� ������� �����",
							   "����� ������� ����",
							   "����� �������� ���"
							 };

const char *weapon_affects[] = { "�������",
								 "�����������",
								 "���.������������",
								 "���.�����������",
								 "���.�����",
								 "���.�����",
								 "������������",
								 "���������",
								 "���������",
								 "������������",
								 "��",
								 "������.��.����",
								 "������.��.�����",
								 "���",
								 "��.���������",
								 "��������",
								 "�������������",
								 "����������",
								 "����������",
								 "�����",
								 "��������",
								 "���������������",
								 "�������",
								 "��.�������",
								 "����",
								 "���������",
								 "����",
								 "���.���",
								 "��������������",
								 "���������",
								 "\n",
								 "�������.�����",
								 "������������",
								 "����������",
								 "������.�����",
								 "���������.���",
								 "��������.���",
								 "�������.���",
								 "�������.�����",
								 "��������.����",
								 "��������������.����",
								 "���������.����",
								 "��������.����",
								 "�������.����",
								 "�������",
								 "����������",
								 "������.������",
								 "\n",
								 "\n",
								 "\n"
							   };

weapon_affect_t weapon_affect = {
	weapon_affect_types{ EWeaponAffectFlag::WAFF_BLINDNESS, 0, SPELL_BLINDNESS },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_INVISIBLE, to_underlying(EAffectFlag::AFF_INVISIBLE), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_DETECT_ALIGN, to_underlying(EAffectFlag::AFF_DETECT_ALIGN), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_DETECT_INVISIBLE, to_underlying(EAffectFlag::AFF_DETECT_INVIS), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_DETECT_MAGIC, to_underlying(EAffectFlag::AFF_DETECT_MAGIC), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_SENSE_LIFE, to_underlying(EAffectFlag::AFF_SENSE_LIFE), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_WATER_WALK, to_underlying(EAffectFlag::AFF_WATERWALK), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_SANCTUARY, to_underlying(EAffectFlag::AFF_SANCTUARY), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_CURSE, to_underlying(EAffectFlag::AFF_CURSE), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_INFRAVISION, to_underlying(EAffectFlag::AFF_INFRAVISION), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_POISON, 0, SPELL_POISON },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_PROTECT_EVIL, to_underlying(EAffectFlag::AFF_PROTECT_EVIL), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_PROTECT_GOOD, to_underlying(EAffectFlag::AFF_PROTECT_GOOD), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_SLEEP, 0, SPELL_SLEEP },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_NOTRACK, to_underlying(EAffectFlag::AFF_NOTRACK), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_BLESS, to_underlying(EAffectFlag::AFF_BLESS), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_SNEAK, to_underlying(EAffectFlag::AFF_SNEAK), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_HIDE, to_underlying(EAffectFlag::AFF_HIDE), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_HOLD, 0, SPELL_HOLD },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_FLY, to_underlying(EAffectFlag::AFF_FLY), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_SILENCE, to_underlying(EAffectFlag::AFF_SILENCE), 0},
	weapon_affect_types{ EWeaponAffectFlag::WAFF_AWARENESS, to_underlying(EAffectFlag::AFF_AWARNESS), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_BLINK, to_underlying(EAffectFlag::AFF_BLINK), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_NOFLEE, to_underlying(EAffectFlag::AFF_NOFLEE), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_SINGLE_LIGHT, to_underlying(EAffectFlag::AFF_SINGLELIGHT), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_HOLY_LIGHT, to_underlying(EAffectFlag::AFF_HOLYLIGHT), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_HOLY_DARK, to_underlying(EAffectFlag::AFF_HOLYDARK), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_DETECT_POISON, to_underlying(EAffectFlag::AFF_DETECT_POISON), 0},
	weapon_affect_types{ EWeaponAffectFlag::WAFF_SLOW, to_underlying(EAffectFlag::AFF_SLOW), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_HASTE, to_underlying(EAffectFlag::AFF_HASTE), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_WATER_BREATH, to_underlying(EAffectFlag::AFF_WATERBREATH), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_HAEMORRAGIA, to_underlying(EAffectFlag::AFF_HAEMORRAGIA), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_CAMOUFLAGE, to_underlying(EAffectFlag::AFF_CAMOUFLAGE), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_SHIELD, to_underlying(EAffectFlag::AFF_SHIELD), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_AIR_SHIELD, to_underlying(EAffectFlag::AFF_AIRSHIELD), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_FIRE_SHIELD, to_underlying(EAffectFlag::AFF_FIRESHIELD), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_ICE_SHIELD, to_underlying(EAffectFlag::AFF_ICESHIELD), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_MAGIC_GLASS, to_underlying(EAffectFlag::AFF_MAGICGLASS), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_STONE_HAND, to_underlying(EAffectFlag::AFF_STONEHAND), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_PRISMATIC_AURA, to_underlying(EAffectFlag::AFF_PRISMATICAURA), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_AIR_AURA, to_underlying(EAffectFlag::AFF_AIRAURA), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_FIRE_AURA, to_underlying(EAffectFlag::AFF_FIREAURA), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_ICE_AURA, to_underlying(EAffectFlag::AFF_ICEAURA), 0 },
	weapon_affect_types{ EWeaponAffectFlag::WAFF_DEAFNESS, to_underlying(EAffectFlag::AFF_DEAFNESS), 0},
	weapon_affect_types{ EWeaponAffectFlag::WAFF_COMMANDER, to_underlying(EAffectFlag::AFF_COMMANDER), 0},
	weapon_affect_types{ EWeaponAffectFlag::WAFF_EARTHAURA, to_underlying(EAffectFlag::AFF_EARTHAURA), 0}
};

const char *pray_metter[] = { "- �����",
							  "- ������",
							  "- ������",
							  "- ������",
							  "* ���������-����������",
							  "* ������-��������",
							  "* ����-�������",
							  "* �������-�����������",
							  "\n"
							};

const char *pray_whom[] = { "�����",
							"������",
							"������",
							"������",
							"���������-����������",
							"������-��������",
							"����-�������",
							"�������-�����������",
							"\n"
						  };

// Fields : whom location modifier bitvector battleflag
std::vector<pray_affect_type> pray_affect =
{
	{0, APPLY_NONE, 0, to_underlying(EAffectFlag::AFF_INFRAVISION), 0},	// �����
	{1, APPLY_HITREG, 50, 0, 0},	// ������
	{2, APPLY_STR, 1, 0, 0},	// �����
	{3, APPLY_DEX, 1, 0, 0},	// �����
	{4, APPLY_MORALE, 5, 0, 0},	// ���� �����
	{5, APPLY_INT, 1, 0, 0},	// ������ �������
	{6, APPLY_HITROLL, 2, 0, 0},	// ���� ������
	{7, APPLY_DAMROLL, 1, 0, 0}	// �������� �����������
};

// ���������� ���� ������
int mana[] = { 0,		// Wiz= 0
			   100,			// Wiz= 1
			   250,			// Wiz= 2
			   500,			// Wiz= 3
			   750,			// Wiz= 4
			   1000,			// Wiz= 5
			   1250,			// Wiz= 6
			   1500,			// Wiz= 7
			   1750,			// Wiz= 8
			   2000,			// Wiz= 9
			   2250,			// Wiz=10
			   2500,			// Wiz=11
			   2750,			// Wiz=12
			   3000,			// Wiz=13
			   3250,			// Wiz=14
			   3500,			// Wiz=15
			   3650,			// Wiz=16
			   3900,			// Wiz=17
			   4000,			// Wiz=18
			   4300,			// Wiz=19
			   4600,			// Wiz=20
			   4900,			// Wiz=21
			   5200,			// Wiz=22
			   5500,			// Wiz=23
			   5800,			// Wiz=24
			   5920,			// Wiz=25
			   5930,			// Wiz=26
			   5940,			// Wiz=27
			   5950,			// Wiz=28
			   5960,			// Wiz=29
			   5970,			// Wiz=30
			   5980,			// Wiz=31
			   5990,			// Wiz=32
			   6000,			// Wiz=33
			   6000,			// Wiz=34
			   6000,			// Wiz=35
			   6000,			// Wiz=36
			   6000,			// Wiz=37
			   6000,			// Wiz=38
			   6000,			// Wiz=39
			   6000,			// Wiz=40
			   6000,			// Wiz=41
			   6000,			// Wiz=42
			   6000,			// Wiz=43
			   6000,			// Wiz=44
			   6000,			// Wiz=45
			   6000,			// Wiz=46
			   6000,			// Wiz=47
			   6000,			// Wiz=48
			   6000,			// Wiz=49
			   6000			// Wiz=50
			 };

// ���������� ���� ���� ������ � ������� � ����������� �� ����
int mana_gain_cs[] = { 2,	// Int= 0
					   3,			// Int= 1
					   3,			// Int= 2
					   4,			// Int= 3
					   4,			// Int= 4
					   5,			// Int= 5
					   5,			// Int= 6
					   6,			// Int= 7
					   6,			// Int= 8
					   7,			// Int= 9
					   7,			// Int=10
					   8,			// Int=11
					   8,			// Int=12
					   9,			// Int=13
					   9,			// Int=14
					   10,			// Int=15
					   11,			// Int=16
					   12,			// Int=17
					   13,			// Int=18
					   14,			// Int=19
					   15,			// Int=20
					   16,			// Int=21
					   17,			// Int=22
					   18,			// Int=23
					   19,			// Int=24
					   20,			// Int=25
					   22,			// Int=26
					   24,			// Int=27
					   26,			// Int=28
					   27,			// Int=29
					   29,			// Int=30
					   31,			// Int=31
					   33,			// Int=32
					   35,			// Int=33
					   37,			// Int=34
					   39,			// Int=35
					   41,			// Int=36
					   44,			// Int=37
					   47,			// Int=38
					   50,			// Int=39
					   53,			// Int=40
					   56,			// Int=41
					   59,			// Int=42
					   62,			// Int=43
					   65,			// Int=44
					   68,			// Int=45
					   71,			// Int=46
					   74,			// Int=47
					   77,			// Int=48
					   80,			// Int=49
					   83			// Int=50
					 };

// ��������� ���������� ������ � ���������� �� ����� � ������
int mana_cost_cs[][9] = {
						//����	1�   2�    3�    4�    5�    6�    7�    8�    9�
							{9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev  0
							{1143, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev  1
							{1000, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev  2
							{950, 1250, 9999, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev  3
							{880, 990, 9999, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev  4
							{830, 900, 9999, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev  5
							{780, 850, 9999, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev  6
							{710, 660, 1000, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev  7
							{560, 600, 770, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev  8
							{500, 600, 700, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev  9
							{450, 550, 640, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev 10
							{390, 460, 540, 9999, 9999, 9999, 9999, 9999, 9999},	// Lev 11
							{360, 430, 500, 1000, 9999, 9999, 9999, 9999, 9999},	// Lev 12
							{330, 400, 470, 800, 9999, 9999, 9999, 9999, 9999},	// Lev 13
							{300, 350, 410, 700, 9999, 9999, 9999, 9999, 9999},	// Lev 14
							{280, 330, 390, 560, 9999, 9999, 9999, 9999, 9999},	// Lev 15
							{250, 300, 350, 500, 1000, 9999, 9999, 9999, 9999},	// Lev 16
							{230, 270, 320, 450, 630, 9999, 9999, 9999, 9999},	// Lev 17
							{200, 240, 280, 400, 560, 9999, 9999, 9999, 9999},	// Lev 18
							{190, 220, 260, 370, 520, 1000, 9999, 9999, 9999},	// Lev 19
							{180, 210, 250, 360, 500, 620, 9999, 9999, 9999},	// Lev 20
							{160, 190, 230, 320, 450, 560, 9999, 9999, 9999},	// Lev 21
							{160, 190, 220, 310, 440, 550, 9999, 9999, 9999},	// Lev 22
							{150, 180, 210, 300, 430, 530, 9999, 9999, 9999},	// Lev 23
							{150, 180, 210, 300, 410, 510, 1000, 9999, 9999},	// Lev 24
							{140, 170, 200, 290, 400, 500, 750, 9999, 9999},	// Lev 25
							{130, 160, 180, 260, 370, 460, 600, 9999, 9999},	// Lev 26
							{120, 150, 170, 240, 340, 430, 550, 1000, 9999},	// Lev 27
							{120, 140, 160, 230, 320, 410, 490, 750, 9999},	// Lev 28
							{110, 130, 160, 220, 310, 390, 470, 670, 9999},	// Lev 29
							{100, 120, 140, 200, 280, 350, 420, 600, 1300},	// Lev 30
							{1, 1, 1, 1, 1, 1, 1, 1, 1},	// Lev 31
							{1, 1, 1, 1, 1, 1, 1, 1, 1},	// Lev 32
							{1, 1, 1, 1, 1, 1, 1, 1, 1},	// Lev 33
							{1, 1, 1, 1, 1, 1, 1, 1, 1}	// Lev 34
						};


//MZ.load
struct zone_type * zone_types = NULL;
//-MZ.load

// �����, ��������� ������� � ����������� �� ������
// �� ������������
/*
int druid_circles[] =
{
   0, //0
   1, //1
   1, //2
   2, //3
   2, //4
   2, //5
   2, //6
   3, //7
   3, //8
   3, //9
   3, //10
   3, //11
   4, //12
   4, //13
   4, //14
   4, //15
   5, //16
   5, //17
   5, //18
   6, //19
   6, //20
   6, //21
   6, //22
   6, //23
   7, //24
   7, //25
   7, //26
   8, //27
   8, //28
   8, //29
   9, //30
   10, //31
   10, //32
   10, //33
   10, //34
};
*/
/* !!!!! ������� ���� �� ����������� -- ��� ��� ���� ���� �� ������������� ������
// ������ ���������� � basic.lst � ManaPerTic (�������� ��� int) -- ����� ���� ���������� ���������, ����� ��������������,
// ���� ��� ������ �� ����� ���������� �� ����� ��������
int mana_gain_old[] =
{    10,   // Int= 0
     10,   // Int= 1
     10,   // Int= 2
     10,   // Int= 3
     15,   // Int= 4
     20,   // Int= 5
     25,   // Int= 6
     30,   // Int= 7
     40,   // Int= 8
     50,   // Int= 9
     60,   // Int=10
     63,   // Int=11
     66,   // Int=12
     69,   // Int=13
     72,   // Int=14
    75,   // Int=15
    78,   // Int=16
    81,   // Int=17
    84,   // Int=18
    87,   // Int=19
    90,   // Int=20
    92,   // Int=21
    94,   // Int=22
    96,   // Int=23
    98,   // Int=24
    100,   // Int=25
    105,   // Int=26
    110,   // Int=27
    115,   // Int=28
    120,   // Int=29
    130,   // Int=30
    135,   // Int=31
    140,   // Int=32
    145,   // Int=33
    150,   // Int=34
    153,   // Int=35
    155,   // Int=36
    158,   // Int=37
    160,   // Int=38
    163,   // Int=39
    165,   // Int=40
    168,   // Int=41
    170,   // Int=42
    173,   // Int=43
    175,   // Int=44
    178,   // Int=45
    180,   // Int=46
    185,   // Int=47
    190,   // Int=48
    195,   // Int=49
    200    // Int=50
};
*/


// �����, ��������� ������� � ����������� �� ������
// �� ������������
/*
int druid_circles[] =
{
   0, //0
   1, //1
   1, //2
   2, //3
   2, //4
   2, //5
   2, //6
   3, //7
   3, //8
   3, //9
   3, //10
   3, //11
   4, //12
   4, //13
   4, //14
   4, //15
   5, //16
   5, //17
   5, //18
   6, //19
   6, //20
   6, //21
   6, //22
   6, //23
   7, //24
   7, //25
   7, //26
   8, //27
   8, //28
   8, //29
   9, //30
   10, //31
   10, //32
   10, //33
   10, //34
};
*/
/* !!!!! ������� ���� �� ����������� -- ��� ��� ���� ���� �� ������������� ������
// ������ ���������� � basic.lst � ManaPerTic (�������� ��� int) -- ����� ���� ���������� ���������, ����� ��������������,
// ���� ��� ������ �� ����� ���������� �� ����� ��������
int mana_gain_old[] =
{    10,   // Int= 0
     10,   // Int= 1
     10,   // Int= 2
     10,   // Int= 3
     15,   // Int= 4
     20,   // Int= 5
     25,   // Int= 6
     30,   // Int= 7
     40,   // Int= 8
     50,   // Int= 9
     60,   // Int=10
     63,   // Int=11
     66,   // Int=12
     69,   // Int=13
     72,   // Int=14
    75,   // Int=15
    78,   // Int=16
    81,   // Int=17
    84,   // Int=18
    87,   // Int=19
    90,   // Int=20
    92,   // Int=21
    94,   // Int=22
    96,   // Int=23
    98,   // Int=24
    100,   // Int=25
    105,   // Int=26
    110,   // Int=27
    115,   // Int=28
    120,   // Int=29
    130,   // Int=30
    135,   // Int=31
    140,   // Int=32
    145,   // Int=33
    150,   // Int=34
    153,   // Int=35
    155,   // Int=36
    158,   // Int=37
    160,   // Int=38
    163,   // Int=39
    165,   // Int=40
    168,   // Int=41
    170,   // Int=42
    173,   // Int=43
    175,   // Int=44
    178,   // Int=45
    180,   // Int=46
    185,   // Int=47
    190,   // Int=48
    195,   // Int=49
    200    // Int=50
};
*/


// �����, ��������� ������� � ����������� �� ������
// �� ������������
/*
int druid_circles[] =
{
   0, //0
   1, //1
   1, //2
   2, //3
   2, //4
   2, //5
   2, //6
   3, //7
   3, //8
   3, //9
   3, //10
   3, //11
   4, //12
   4, //13
   4, //14
   4, //15
   5, //16
   5, //17
   5, //18
   6, //19
   6, //20
   6, //21
   6, //22
   6, //23
   7, //24
   7, //25
   7, //26
   8, //27
   8, //28
   8, //29
   9, //30
   10, //31
   10, //32
   10, //33
   10, //34
};
*/
/* !!!!! ������� ���� �� ����������� -- ��� ��� ���� ���� �� ������������� ������
// ������ ���������� � basic.lst � ManaPerTic (�������� ��� int) -- ����� ���� ���������� ���������, ����� ��������������,
// ���� ��� ������ �� ����� ���������� �� ����� ��������
int mana_gain_old[] =
{    10,   // Int= 0
     10,   // Int= 1
     10,   // Int= 2
     10,   // Int= 3
     15,   // Int= 4
     20,   // Int= 5
     25,   // Int= 6
     30,   // Int= 7
     40,   // Int= 8
     50,   // Int= 9
     60,   // Int=10
     63,   // Int=11
     66,   // Int=12
     69,   // Int=13
     72,   // Int=14
    75,   // Int=15
    78,   // Int=16
    81,   // Int=17
    84,   // Int=18
    87,   // Int=19
    90,   // Int=20
    92,   // Int=21
    94,   // Int=22
    96,   // Int=23
    98,   // Int=24
    100,   // Int=25
    105,   // Int=26
    110,   // Int=27
    115,   // Int=28
    120,   // Int=29
    130,   // Int=30
    135,   // Int=31
    140,   // Int=32
    145,   // Int=33
    150,   // Int=34
    153,   // Int=35
    155,   // Int=36
    158,   // Int=37
    160,   // Int=38
    163,   // Int=39
    165,   // Int=40
    168,   // Int=41
    170,   // Int=42
    173,   // Int=43
    175,   // Int=44
    178,   // Int=45
    180,   // Int=46
    185,   // Int=47
    190,   // Int=48
    195,   // Int=49
    200    // Int=50
};
*/

// Weapon attack texts
struct attack_hit_type attack_hit_text[] =
{
	{"������", "�������"},	// 0
	{"�������", "��������"},
	{"��������", "���������"},
	{"�������", "��������"},
	{"������", "�������"},
	{"�����", "������"},	// 5
	{"��������", "���������"},
	{"�������", "��������"},
	{"��������", "���������"},
	{"����������", "�����������"},
	{"������", "�������"},	// 10
	{"������", "�������"},
	{"�����", "������"},
	{"������", "�������"},
	{"������", "�������"},
	{"������", "�������"},
	{"������", "�������"},
	{"*", "*"},
	{"*", "*"},
	{"*", "*"}
};

const char *godslike_bits[] =
{
	"GF_GODSLIKE",
	"GF_GODSCURSE",
	"GF_HIGHGOD",
	"GF_REMORT",
	"GF_DEMIGOD",
	"GF_PERSLOG",
	"GF_TESTER",
	"\n"
};

std::array<const char *, NUM_PLAYER_CLASSES> pc_class_name =
{{
	"������",
	"������",
	"����",
	"��������",
	"�������",
	"���������",
	"��������",
	"���������",
	"������������",
	"������",
	"�������",
	"������",
	"�����",
	"�����"
}};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
