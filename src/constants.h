/**************************************************************************
*  File: constants.h                                     Part of Bylins   *
*  Usage: header file for mud contstants.                                 *
*                                                                         *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#include "classes/classes_constants.h"
#include "structs/structs.h"
#include "sysdep.h"
#include "conf.h"

#include <vector>
#include <array>

struct IntApplies {
	int spell_aknowlege;    // drop_chance to know spell               //
	int to_skilluse;        // ADD CHANSE FOR USING SKILL         //
	int mana_per_tic;
	int spell_success;        //  max count of spell on 1s level    //
	int improve;        // drop_chance to improve skill           //
	int observation;        // drop_chance to use kAwake/CRITICAL //
};

struct ChaApplies {
	int leadership;
	int charms;
	int morale;
	int illusive;
	int dam_to_hit_rate;
};

struct SizeApplies {
	int ac;
	int interpolate;        // ADD VALUE FOR SOME SKILLS  //
	int initiative;
	int shocking;
};

struct WeaponApplies {
	int shocking;
	int bashing;
	int parrying;
};

struct pray_affect_type {
	int metter;
	EApplyLocation location;
	int modifier;
	uint32_t bitvector;
	int battleflag;
};

extern const char *circlemud_version;
extern const char *dirs[];
extern const char *DirsFrom[];
extern const char *DirsTo[];
extern const char *room_bits[];
extern const char *exit_bits[];
extern const char *sector_types[];
extern const char *genders[];
extern const char *position_types[];
extern const char *resistance_types[];
extern const char *player_bits[];
extern const char *action_bits[];
extern const char *preference_bits[];
extern const char *connected_types[];
extern const char *where[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *apply_negative[];
extern const char *weapon_affects[];
extern const char *anti_bits[];
extern const char *no_bits[];
extern const char *material_type[];
extern const char *container_bits[];
extern const char *fullness[];

extern const std::vector<const char *> npc_role_types;
extern const char *npc_race_types[];
extern const char *places_of_birth[];
extern const char *weekdays[];
extern const char *month_name[];
extern const char *weekdays_poly[];
extern const char *month_name_poly[];
extern const char *ingradient_bits[];
extern const char *magic_container_bits[];
extern const char *function_bits[];
extern const char *pray_metter[];
extern const char *pray_whom[];
extern const char *room_aff_visib_bits[];
extern const char *room_aff_invis_bits[];
extern const char *room_self_aff_invis_bits[];
extern const char *equipment_types[];
extern struct IntApplies int_app[];
extern const size_t INT_APP_SIZE;
extern struct ChaApplies cha_app[];
extern struct SizeApplies size_app[];
extern struct WeaponApplies weapon_app[];
extern std::vector<pray_affect_type> pray_affect;
extern int rev_dir[];
extern int movement_loss[];

extern int mana[];
extern int mana_gain_cs[];
extern int mana_cost_cs[][9];
extern const char *material_name[];
extern struct AttackHitType attack_hit_text[];
extern const char *godslike_bits[];
extern std::array<const char *, kNumPlayerClasses> pc_class_name;
extern const char *weapon_class[];

//The number of changing coefficients (the others are unchanged)
#define    MAX_EXP_COEFFICIENTS_USED 15

// unless you change this, Puff casts all your dg spells
#define DG_CASTER_PROXY 113

#define FIRST_ROOM       1
#define STRANGE_ROOM     3

#define FIRE_MOVES       20
#define LOOKING_MOVES    5
#define HEARING_MOVES    2
#define LOOKHIDE_MOVES   3
#define SNEAK_MOVES      1
#define CAMOUFLAGE_MOVES 1
#define PICKLOCK_MOVES   10
#define TRACK_MOVES      3
#define SENSE_MOVES      4
#define HIDETRACK_MOVES  10

#define MOB_ARMOUR_MULT  5
#define MOB_AC_MULT      5
#define MOB_DAMAGE_MULT  3

#define MAX_GROUPED_FOLLOWERS 7

extern int HORSE_VNUM;
extern int HORSE_COST;
extern int START_BREAD;
extern int CREATE_LIGHT;

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
