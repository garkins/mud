/*************************************************************************
*   File: features.cpp                                 Part of Bylins    *
*   Features code                                                        *
*                                                                        *
*  $Author$                                                     *
*  $Date$                                          *
*  $Revision$                	                                 *
************************************************************************ */

#include "feats.h"

#include "abilities/abilities_constants.h"
#include "action_targeting.h"
#include "handler.h"
#include "entities/player_races.h"
#include "color.h"
#include "fightsystem/pk.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>

using namespace abilities;

extern const char *unused_spellname;

struct FeatureInfoType feat_info[kMaxFeats];

/* Служебные функции */
void initializeFeatureByDefault(int featureNum);
bool checkVacantFeatureSlot(CharacterData *ch, int feat);

/* Функции для работы с переключаемыми способностями */
bool checkAccessActivatedFeature(CharacterData *ch, int featureNum);
void activateFeature(CharacterData *ch, int featureNum);
void deactivateFeature(CharacterData *ch, int featureNum);
int get_feature_num(char *featureName);

/* Ситуативные бонусы, пишутся для специфических способностей по потребности */
short calculateSituationalRollBonusOfGroupFormation(CharacterData *ch, CharacterData * /* enemy */);

/* Активные способности */
void do_lightwalk(CharacterData *ch, char *argument, int cmd, int subcmd);

/* Extern */
extern void setSkillCooldown(CharacterData *ch, ESkill skill, int cooldownInPulses);

///
/// Поиск номера способности по имени
/// \param alias = false
/// true для поиска при вводе имени способности игроком у учителей
///
int find_feat_num(const char *name, bool alias) {
	for (int index = 1; index < kMaxFeats; index++) {
		bool flag = true;
		std::string name_feat(alias ? feat_info[index].alias.c_str() : feat_info[index].name);
		std::vector<std::string> strs_feat, strs_args;
		boost::split(strs_feat, name_feat, boost::is_any_of(" "));
		boost::split(strs_args, name, boost::is_any_of(" "));
		const int bound = static_cast<int>(strs_feat.size() >= strs_args.size()
										   ? strs_args.size()
										   : strs_feat.size());
		for (int i = 0; i < bound; i++) {
			if (!boost::starts_with(strs_feat[i], strs_args[i])) {
				flag = false;
			}
		}
		if (flag)
			return index;
	}
	return (-1);
}

void initializeFeature(int featureNum, const char *name, int type, bool can_up_slot, CFeatArray app,
					   short dicerollBonus = MAX_ABILITY_DICEROLL_BONUS, ESkill baseSkill = ESkill::kIncorrect,
					   ESaving oppositeSaving = ESaving::kStability) {
	int i, j;
	for (i = 0; i < kNumPlayerClasses; i++) {
		for (j = 0; j < kNumKins; j++) {
			feat_info[featureNum].minRemort[i][j] = 0;
			feat_info[featureNum].slot[i][j] = 0;
		}
	}
	if (name) {
		feat_info[featureNum].name = name;
		std::string alias(name);
		std::replace_if(alias.begin(), alias.end(), boost::is_any_of("_:"), ' ');
		boost::trim_all(alias);
		feat_info[featureNum].alias = alias;
	}
	feat_info[featureNum].ID = featureNum;
	feat_info[featureNum].dicerollBonus = dicerollBonus;
	feat_info[featureNum].base_skill = baseSkill;
	feat_info[featureNum].oppositeSaving = oppositeSaving;
	feat_info[featureNum].type = type;
	feat_info[featureNum].up_slot = can_up_slot;
	for (i = 0; i < MAX_FEAT_AFFECT; i++) {
		feat_info[featureNum].affected[i].location = app.affected[i].location;
		feat_info[featureNum].affected[i].modifier = app.affected[i].modifier;
	}
}

void initializeFeatureByDefault(int featureNum) {
	int i, j;

	for (i = 0; i < kNumPlayerClasses; i++) {
		for (j = 0; j < kNumKins; j++) {
			feat_info[featureNum].minRemort[i][j] = 0;
			feat_info[featureNum].slot[i][j] = 0;
			feat_info[featureNum].inbornFeatureOfClass[i][j] = false;
			feat_info[featureNum].classknow[i][j] = false;
		}
	}

	feat_info[featureNum].ID = featureNum;
	feat_info[featureNum].name = unused_spellname;
	feat_info[featureNum].type = UNUSED_FTYPE;
	feat_info[featureNum].up_slot = false;
	feat_info[featureNum].alwaysAvailable = false;
	feat_info[featureNum].usesWeaponSkill = false;
	feat_info[featureNum].baseDamageBonusPercent = 0;
	feat_info[featureNum].degreeOfSuccessDamagePercent = 5;
	feat_info[featureNum].oppositeSaving = ESaving::kStability;
	feat_info[featureNum].dicerollBonus = MAX_ABILITY_DICEROLL_BONUS;
	feat_info[featureNum].base_skill = ESkill::kIncorrect;
	feat_info[featureNum].criticalFailThreshold = kDefaultCritfailThreshold;
	feat_info[featureNum].criticalSuccessThreshold = kDefaultCritsuccessThreshold;

	for (i = 0; i < MAX_FEAT_AFFECT; i++) {
		feat_info[featureNum].affected[i].location = APPLY_NONE;
		feat_info[featureNum].affected[i].modifier = 0;
	}

	feat_info[featureNum].getBaseParameter = &GET_REAL_INT;
	feat_info[featureNum].getEffectParameter = &GET_REAL_STR;
	feat_info[featureNum].calculateSituationalDamageFactor =
		([](CharacterData *) -> float {
			return 1.00;
		});
	feat_info[featureNum].calculateSituationalRollBonus =
		([](CharacterData *, CharacterData *) -> short {
			return 0;
		});
}

// Инициализация массива структур способностей
void determineFeaturesSpecification() {
	CFeatArray feat_app;
	for (int i = 1; i < kMaxFeats; i++) {
		initializeFeatureByDefault(i);
	}
//1
	initializeFeature(BERSERK_FEAT, "предсмертная ярость", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//2
	initializeFeature(PARRY_ARROW_FEAT, "отбить стрелу", NORMAL_FTYPE, true, feat_app);
//3
	initializeFeature(BLIND_FIGHT_FEAT, "слепой бой", NORMAL_FTYPE, true, feat_app);
//4
	feat_app.insert(APPLY_MR, 1);
	feat_app.insert(APPLY_AR, 1);
	initializeFeature(IMPREGNABLE_FEAT, "непробиваемый", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//5-*
	initializeFeature(APPROACHING_ATTACK_FEAT, "встречная атака", NORMAL_FTYPE, true, feat_app);
//6
	initializeFeature(DEFENDER_FEAT, "щитоносец", NORMAL_FTYPE, true, feat_app);
//7
	initializeFeature(DODGER_FEAT, "изворотливость", AFFECT_FTYPE, true, feat_app);
//8
	initializeFeature(LIGHT_WALK_FEAT, "легкая поступь", NORMAL_FTYPE, true, feat_app);
//9
	initializeFeature(WRIGGLER_FEAT, "проныра", NORMAL_FTYPE, true, feat_app);
//10
	initializeFeature(SPELL_SUBSTITUTE_FEAT, "подмена заклинания", NORMAL_FTYPE, true, feat_app);
//11
	initializeFeature(POWER_ATTACK_FEAT, "мощная атака", ACTIVATED_FTYPE, true, feat_app);
//12
	feat_app.insert(APPLY_RESIST_FIRE, 5);
	feat_app.insert(APPLY_RESIST_AIR, 5);
	feat_app.insert(APPLY_RESIST_WATER, 5);
	feat_app.insert(APPLY_RESIST_EARTH, 5);
	feat_app.insert(APPLY_RESIST_DARK, 5);
	initializeFeature(WOODEN_SKIN_FEAT, "деревянная кожа", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//13
	feat_app.insert(APPLY_RESIST_FIRE, 10);
	feat_app.insert(APPLY_RESIST_AIR, 10);
	feat_app.insert(APPLY_RESIST_WATER, 10);
	feat_app.insert(APPLY_RESIST_EARTH, 10);
	feat_app.insert(APPLY_RESIST_DARK, 10);
	feat_app.insert(APPLY_ABSORBE, 5);
	initializeFeature(IRON_SKIN_FEAT, "железная кожа", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//14
	feat_app.insert(FEAT_TIMER, 8);
	initializeFeature(CONNOISEUR_FEAT, "знаток", SKILL_MOD_FTYPE, true, feat_app);
	feat_app.clear();
//15
	initializeFeature(EXORCIST_FEAT, "изгоняющий нежить", SKILL_MOD_FTYPE, true, feat_app);
//16
	initializeFeature(HEALER_FEAT, "целитель", NORMAL_FTYPE, true, feat_app);
//17
	feat_app.insert(APPLY_SAVING_REFLEX, -10);
	initializeFeature(LIGHTING_REFLEX_FEAT, "мгновенная реакция", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//18
	feat_app.insert(FEAT_TIMER, 8);
	initializeFeature(DRUNKARD_FEAT, "пьяница", SKILL_MOD_FTYPE, true, feat_app);
	feat_app.clear();
//19
	initializeFeature(POWER_MAGIC_FEAT, "мощь колдовства", NORMAL_FTYPE, true, feat_app);
//20
	feat_app.insert(APPLY_MOVEREG, 40);
	initializeFeature(ENDURANCE_FEAT, "выносливость", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//21
	feat_app.insert(APPLY_SAVING_WILL, -10);
	feat_app.insert(APPLY_SAVING_STABILITY, -10);
	initializeFeature(GREAT_FORTITUDE_FEAT, "сила духа", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//22
	feat_app.insert(APPLY_HITREG, 35);
	initializeFeature(FAST_REGENERATION_FEAT, "быстрое заживление", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//23
	initializeFeature(STEALTHY_FEAT, "незаметность", SKILL_MOD_FTYPE, true, feat_app);
//24
	feat_app.insert(APPLY_CAST_SUCCESS, 80);
	initializeFeature(RELATED_TO_MAGIC_FEAT, "магическое родство", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//25
	feat_app.insert(APPLY_HITREG, 10);
	feat_app.insert(APPLY_SAVING_CRITICAL, -4);
	initializeFeature(SPLENDID_HEALTH_FEAT, "богатырское здоровье", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//26
	initializeFeature(TRACKER_FEAT, "следопыт", SKILL_MOD_FTYPE, true, feat_app);
	feat_app.clear();
//27
	initializeFeature(WEAPON_FINESSE_FEAT, "ловкий удар", NORMAL_FTYPE, true, feat_app);
//28
	initializeFeature(COMBAT_CASTING_FEAT, "боевое колдовство", NORMAL_FTYPE, true, feat_app);
//29
	initializeFeature(PUNCH_MASTER_FEAT, "мастер кулачного боя", NORMAL_FTYPE, true, feat_app);
//30
	initializeFeature(CLUBS_MASTER_FEAT, "мастер палицы", NORMAL_FTYPE, true, feat_app);
//31
	initializeFeature(AXES_MASTER_FEAT, "мастер секиры", NORMAL_FTYPE, true, feat_app);
//32
	initializeFeature(LONGS_MASTER_FEAT, "мастер меча", NORMAL_FTYPE, true, feat_app);
//33
	initializeFeature(SHORTS_MASTER_FEAT, "мастер ножа", NORMAL_FTYPE, true, feat_app);
//34
	initializeFeature(NONSTANDART_MASTER_FEAT, "мастер необычного оружия", NORMAL_FTYPE, true, feat_app);
//35
	initializeFeature(BOTHHANDS_MASTER_FEAT, "мастер двуручника", NORMAL_FTYPE, true, feat_app);
//36
	initializeFeature(PICK_MASTER_FEAT, "мастер кинжала", NORMAL_FTYPE, true, feat_app);
//37
	initializeFeature(SPADES_MASTER_FEAT, "мастер копья", NORMAL_FTYPE, true, feat_app);
//38
	initializeFeature(BOWS_MASTER_FEAT, "мастер лучник", NORMAL_FTYPE, true, feat_app);
//39
	initializeFeature(FOREST_PATHS_FEAT, "лесные тропы", NORMAL_FTYPE, true, feat_app);
//40
	initializeFeature(MOUNTAIN_PATHS_FEAT, "горные тропы", NORMAL_FTYPE, true, feat_app);
//41
	feat_app.insert(APPLY_MORALE, 5);
	initializeFeature(LUCKY_FEAT, "счастливчик", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//42
	initializeFeature(SPIRIT_WARRIOR_FEAT, "боевой дух", NORMAL_FTYPE, true, feat_app);
//43
	feat_app.insert(APPLY_HITREG, 50);
	initializeFeature(RELIABLE_HEALTH_FEAT, "крепкое здоровье", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//44
	feat_app.insert(APPLY_MANAREG, 100);
	initializeFeature(EXCELLENT_MEMORY_FEAT, "превосходная память", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//45
	feat_app.insert(APPLY_DEX, 1);
	initializeFeature(ANIMAL_DEXTERY_FEAT, "звериная прыть", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//46
	feat_app.insert(APPLY_MANAREG, 25);
	initializeFeature(LEGIBLE_WRITTING_FEAT, "чёткий почерк", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//47
	feat_app.insert(APPLY_DAMROLL, 2);
	initializeFeature(IRON_MUSCLES_FEAT, "стальные мышцы", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//48
	feat_app.insert(APPLY_CAST_SUCCESS, 5);
	initializeFeature(MAGIC_SIGN_FEAT, "знак чародея", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//49
	feat_app.insert(APPLY_MOVEREG, 75);
	initializeFeature(GREAT_ENDURANCE_FEAT, "двужильность", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//50
	feat_app.insert(APPLY_MORALE, 5);
	initializeFeature(BEST_DESTINY_FEAT, "лучшая доля", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//51
	initializeFeature(BREW_POTION_FEAT, "травник", NORMAL_FTYPE, true, feat_app);
//52
	initializeFeature(JUGGLER_FEAT, "жонглер", NORMAL_FTYPE, true, feat_app);
//53
	initializeFeature(NIMBLE_FINGERS_FEAT, "ловкач", SKILL_MOD_FTYPE, true, feat_app);
//54
	initializeFeature(GREAT_POWER_ATTACK_FEAT, "улучшенная мощная атака", ACTIVATED_FTYPE, true, feat_app);
//55
	feat_app.insert(APPLY_RESIST_IMMUNITY, 15);
	initializeFeature(IMMUNITY_FEAT, "привычка к яду", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//56
	feat_app.insert(APPLY_AC, -40);
	initializeFeature(MOBILITY_FEAT, "подвижность", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//57
	feat_app.insert(APPLY_STR, 1);
	initializeFeature(NATURAL_STRENGTH_FEAT, "силач", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//58
	feat_app.insert(APPLY_DEX, 1);
	initializeFeature(NATURAL_DEXTERY_FEAT, "проворство", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//59
	feat_app.insert(APPLY_INT, 1);
	initializeFeature(NATURAL_INTELLECT_FEAT, "природный ум", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//60
	feat_app.insert(APPLY_WIS, 1);
	initializeFeature(NATURAL_WISDOM_FEAT, "мудрец", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//61
	feat_app.insert(APPLY_CON, 1);
	initializeFeature(NATURAL_CONSTITUTION_FEAT, "здоровяк", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//62
	feat_app.insert(APPLY_CHA, 1);
	initializeFeature(NATURAL_CHARISMA_FEAT, "природное обаяние", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//63
	feat_app.insert(APPLY_MANAREG, 25);
	initializeFeature(MNEMONIC_ENHANCER_FEAT, "отличная память", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//64 -*
	initializeFeature(MAGNETIC_PERSONALITY_FEAT, "предводитель", SKILL_MOD_FTYPE, true, feat_app);
//65
	feat_app.insert(APPLY_DAMROLL, 2);
	initializeFeature(DAMROLL_BONUS_FEAT, "тяжел на руку", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//66
	feat_app.insert(APPLY_HITROLL, 1);
	initializeFeature(HITROLL_BONUS_FEAT, "твердая рука", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//67
	feat_app.insert(APPLY_CAST_SUCCESS, 30);
	initializeFeature(MAGICAL_INSTINCT_FEAT, "магическое чутье", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//68
	initializeFeature(PUNCH_FOCUS_FEAT, "любимое_оружие: голые руки", SKILL_MOD_FTYPE, true,
					  feat_app, 0, ESkill::kPunch);
//69
	initializeFeature(CLUB_FOCUS_FEAT, "любимое_оружие: палица", SKILL_MOD_FTYPE, true,
					  feat_app, 0, ESkill::kClubs);
//70
	initializeFeature(AXES_FOCUS_FEAT, "любимое_оружие: секира", SKILL_MOD_FTYPE, true,
					  feat_app, 0, ESkill::kAxes);
//71
	initializeFeature(LONGS_FOCUS_FEAT, "любимое_оружие: меч", SKILL_MOD_FTYPE, true,
					  feat_app, 0, ESkill::kLongBlades);
//72
	initializeFeature(SHORTS_FOCUS_FEAT, "любимое_оружие: нож", SKILL_MOD_FTYPE, true,
					  feat_app, 0, ESkill::kShortBlades);
//73
	initializeFeature(NONSTANDART_FOCUS_FEAT, "любимое_оружие: необычное", SKILL_MOD_FTYPE, true,
					  feat_app, 0, ESkill::kNonstandart);
//74
	initializeFeature(BOTHHANDS_FOCUS_FEAT, "любимое_оружие: двуручник", SKILL_MOD_FTYPE, true,
					  feat_app, 0, ESkill::kTwohands);
//75
	initializeFeature(PICK_FOCUS_FEAT, "любимое_оружие: кинжал", SKILL_MOD_FTYPE, true,
					  feat_app, 0, ESkill::kPicks);
//76
	initializeFeature(SPADES_FOCUS_FEAT, "любимое_оружие: копье", SKILL_MOD_FTYPE, true,
					  feat_app, 0, ESkill::kSpades);
//77
	initializeFeature(BOWS_FOCUS_FEAT, "любимое_оружие: лук", SKILL_MOD_FTYPE, true,
					  feat_app, 0, ESkill::kBows);
//78
	initializeFeature(AIMING_ATTACK_FEAT, "прицельная атака", ACTIVATED_FTYPE, true, feat_app);
//79
	initializeFeature(GREAT_AIMING_ATTACK_FEAT, "улучшенная прицельная атака", ACTIVATED_FTYPE, true, feat_app);
//80
	initializeFeature(DOUBLESHOT_FEAT, "двойной выстрел", NORMAL_FTYPE, true, feat_app);
//81
	initializeFeature(PORTER_FEAT, "тяжеловоз", NORMAL_FTYPE, true, feat_app);
//82
	initializeFeature(SECRET_RUNES_FEAT, "тайные руны", NORMAL_FTYPE, true, feat_app);
/*
//83
	initializeFeature(RUNE_USER_FEAT, "тайные руны", NORMAL_FTYPE, true, feat_app);
//84
	initializeFeature(RUNE_MASTER_FEAT, "заветные руны", NORMAL_FTYPE, true, feat_app);
//85
	initializeFeature(RUNE_ULTIMATE_FEAT, "руны богов", NORMAL_FTYPE, true, feat_app);
	*/
//86
	initializeFeature(TO_FIT_ITEM_FEAT, "переделать", NORMAL_FTYPE, true, feat_app);
//87
	initializeFeature(TO_FIT_CLOTHCES_FEAT, "перешить", NORMAL_FTYPE, true, feat_app);
//88
	initializeFeature(STRENGTH_CONCETRATION_FEAT, "концентрация силы", NORMAL_FTYPE, true, feat_app);
//89
	initializeFeature(DARK_READING_FEAT, "кошачий глаз", NORMAL_FTYPE, true, feat_app);
//90
	initializeFeature(SPELL_CAPABLE_FEAT, "зачаровать", NORMAL_FTYPE, true, feat_app);
//91
	initializeFeature(ARMOR_LIGHT_FEAT, "легкие доспехи", NORMAL_FTYPE, true, feat_app);
//92
	initializeFeature(ARMOR_MEDIAN_FEAT, "средние доспехи", NORMAL_FTYPE, true, feat_app);
//93
	initializeFeature(ARMOR_HEAVY_FEAT, "тяжелые доспехи", NORMAL_FTYPE, true, feat_app);
//94
	initializeFeature(GEMS_INLAY_FEAT, "инкрустация", NORMAL_FTYPE, true, feat_app);
//95
	initializeFeature(WARRIOR_STR_FEAT, "богатырская сила", NORMAL_FTYPE, true, feat_app);
//96
	initializeFeature(RELOCATE_FEAT, "переместиться", NORMAL_FTYPE, true, feat_app);
//97
	initializeFeature(SILVER_TONGUED_FEAT, "сладкоречие", NORMAL_FTYPE, true, feat_app);
//98
	initializeFeature(BULLY_FEAT, "забияка", NORMAL_FTYPE, true, feat_app);
//99
	initializeFeature(THIEVES_STRIKE_FEAT, "воровской удар", NORMAL_FTYPE, true, feat_app);
//100
	initializeFeature(MASTER_JEWELER_FEAT, "искусный ювелир", NORMAL_FTYPE, true, feat_app);
//101
	initializeFeature(SKILLED_TRADER_FEAT, "торговая сметка", NORMAL_FTYPE, true, feat_app);
//102
	initializeFeature(ZOMBIE_DROVER_FEAT, "погонщик умертвий", NORMAL_FTYPE, true, feat_app);
//103
	initializeFeature(EMPLOYER_FEAT, "навык найма", NORMAL_FTYPE, true, feat_app);
//104
	initializeFeature(MAGIC_USER_FEAT, "использование амулетов", NORMAL_FTYPE, true, feat_app);
//105
	initializeFeature(GOLD_TONGUE_FEAT, "златоуст", NORMAL_FTYPE, true, feat_app);
//106
	initializeFeature(CALMNESS_FEAT, "хладнокровие", NORMAL_FTYPE, true, feat_app);
//107
	initializeFeature(RETREAT_FEAT, "отступление", NORMAL_FTYPE, true, feat_app);
//108
	initializeFeature(SHADOW_STRIKE_FEAT, "танцующая тень", NORMAL_FTYPE, true, feat_app);
//109
	initializeFeature(THRIFTY_FEAT, "запасливость", NORMAL_FTYPE, true, feat_app);
//110
	initializeFeature(CYNIC_FEAT, "циничность", NORMAL_FTYPE, true, feat_app);
//111
	initializeFeature(PARTNER_FEAT, "напарник", NORMAL_FTYPE, true, feat_app);
//112
	initializeFeature(HELPDARK_FEAT, "помощь тьмы", NORMAL_FTYPE, true, feat_app);
//113
	initializeFeature(FURYDARK_FEAT, "ярость тьмы", NORMAL_FTYPE, true, feat_app);
//114
	initializeFeature(DARKREGEN_FEAT, "темное восстановление", NORMAL_FTYPE, true, feat_app);
//115
	initializeFeature(SOULLINK_FEAT, "родство душ", NORMAL_FTYPE, true, feat_app);
//116
	initializeFeature(STRONGCLUTCH_FEAT, "сильная хватка", NORMAL_FTYPE, true, feat_app);
//117
	initializeFeature(MAGICARROWS_FEAT, "магические стрелы", NORMAL_FTYPE, true, feat_app);
//118
	initializeFeature(COLLECTORSOULS_FEAT, "колекционер душ", NORMAL_FTYPE, true, feat_app);
//119
	initializeFeature(DARKDEAL_FEAT, "темная сделка", NORMAL_FTYPE, true, feat_app);
//120
	initializeFeature(DECLINE_FEAT, "порча", NORMAL_FTYPE, true, feat_app);
//121
	initializeFeature(HARVESTLIFE_FEAT, "жатва жизни", NORMAL_FTYPE, true, feat_app);
//122
	initializeFeature(LOYALASSIST_FEAT, "верный помощник", NORMAL_FTYPE, true, feat_app);
//123
	initializeFeature(HAUNTINGSPIRIT_FEAT, "блуждающий дух", NORMAL_FTYPE, true, feat_app);
//124
	initializeFeature(SNEAKRAGE_FEAT, "ярость змеи", NORMAL_FTYPE, true, feat_app);
//126
	initializeFeature(ELDER_TASKMASTER_FEAT, "старший надсмотрщик", NORMAL_FTYPE, true, feat_app);
//127
	initializeFeature(LORD_UNDEAD_FEAT, "повелитель нежити", NORMAL_FTYPE, true, feat_app);
//128
	initializeFeature(DARK_WIZARD_FEAT, "темный маг", NORMAL_FTYPE, true, feat_app);
//129
	initializeFeature(ELDER_PRIEST_FEAT, "старший жрец", NORMAL_FTYPE, true, feat_app);
//130
	initializeFeature(HIGH_LICH_FEAT, "верховный лич", NORMAL_FTYPE, true, feat_app);
//131
	initializeFeature(BLACK_RITUAL_FEAT, "темный ритуал", NORMAL_FTYPE, true, feat_app);
//132
	initializeFeature(TEAMSTER_UNDEAD_FEAT, "погонщик нежити", NORMAL_FTYPE, true, feat_app);
//133
	initializeFeature(SKIRMISHER_FEAT, "держать строй", ACTIVATED_FTYPE, true, feat_app,
					  90, ESkill::kRescue, ESaving::kReflex);
	feat_info[SKIRMISHER_FEAT].getBaseParameter = &GET_REAL_DEX;
	feat_info[SKIRMISHER_FEAT].calculateSituationalRollBonus = &calculateSituationalRollBonusOfGroupFormation;
//134
	initializeFeature(TACTICIAN_FEAT, "десяцкий", ACTIVATED_FTYPE, true, feat_app, 90, ESkill::kLeadership, ESaving::kReflex);
	feat_info[TACTICIAN_FEAT].getBaseParameter = &GET_REAL_CHA;
	feat_info[TACTICIAN_FEAT].calculateSituationalRollBonus = &calculateSituationalRollBonusOfGroupFormation;
//135
	initializeFeature(LIVE_SHIELD_FEAT, "живой щит", NORMAL_FTYPE, true, feat_app);
// === Проскок номеров (типа резерв под татей) ===
//138
	initializeFeature(EVASION_FEAT, "скользкий тип", NORMAL_FTYPE, true, feat_app);
//139
	initializeFeature(EXPEDIENT_CUT_FEAT, "порез", TECHNIQUE_FTYPE, true, feat_app, 100, ESkill::kPunch, ESaving::kReflex);
//140
	initializeFeature(SHOT_FINESSE_FEAT, "ловкий выстрел", NORMAL_FTYPE, true, feat_app);
//141
	initializeFeature(OBJECT_ENCHANTER_FEAT, "наложение чар", NORMAL_FTYPE, true, feat_app);
//142
	initializeFeature(DEFT_SHOOTER_FEAT, "ловкий стрелок", NORMAL_FTYPE, true, feat_app);
//143
	initializeFeature(MAGIC_SHOOTER_FEAT, "магический выстрел", NORMAL_FTYPE, true, feat_app);
//144
	initializeFeature(THROW_WEAPON_FEAT, "метнуть", TECHNIQUE_FTYPE, true, feat_app, 100, ESkill::kThrow, ESaving::kReflex);
	feat_info[THROW_WEAPON_FEAT].getBaseParameter = &GET_REAL_DEX;
	feat_info[THROW_WEAPON_FEAT].getEffectParameter = &GET_REAL_STR;
	feat_info[THROW_WEAPON_FEAT].usesWeaponSkill = false;
	feat_info[THROW_WEAPON_FEAT].alwaysAvailable = true;
	feat_info[THROW_WEAPON_FEAT].baseDamageBonusPercent = 5;
	feat_info[THROW_WEAPON_FEAT].degreeOfSuccessDamagePercent = 5;
	feat_info[THROW_WEAPON_FEAT].calculateSituationalDamageFactor =
		([](CharacterData *ch) -> float {
			return (0.1 * can_use_feat(ch, POWER_THROW_FEAT) + 0.1 * can_use_feat(ch, DEADLY_THROW_FEAT));
		});
	feat_info[THROW_WEAPON_FEAT].calculateSituationalRollBonus =
		([](CharacterData *ch, CharacterData * /* enemy */) -> short {
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)) {
				return -60;
			}
			return 0;
		});
	// Это ужасно, понимаю, но не хочется возиться с написанием мутной функции с переменным числоа аргументов,
	// потому что при введении конфига, чем я планирую заняться в ближайшее время, все равно ее придется переписывать.
	//TODO: Не забыть переписать этот бордель
	feat_info[THROW_WEAPON_FEAT].techniqueItemKitsGroup.reserve(2);

	//techniqueItemKit = new TechniqueItemKitType;
	auto techniqueItemKit = std::make_unique<TechniqueItemKitType>();
	techniqueItemKit->reserve(1);
	techniqueItemKit->push_back(TechniqueItem(WEAR_WIELD, ObjectData::ITEM_WEAPON,
											  ESkill::kAny, EExtraFlag::ITEM_THROWING));
	feat_info[THROW_WEAPON_FEAT].techniqueItemKitsGroup.push_back(std::move(techniqueItemKit));

	//techniqueItemKit = new TechniqueItemKitType;
	techniqueItemKit = std::make_unique<TechniqueItemKitType>();
	techniqueItemKit->reserve(1);
	techniqueItemKit->push_back(TechniqueItem(WEAR_HOLD, ObjectData::ITEM_WEAPON,
											  ESkill::kAny, EExtraFlag::ITEM_THROWING));
	feat_info[THROW_WEAPON_FEAT].techniqueItemKitsGroup.push_back(std::move(techniqueItemKit));
//145
	initializeFeature(SHADOW_THROW_FEAT, "змеево оружие", TECHNIQUE_FTYPE, true, feat_app,
					  100, ESkill::kDarkMagic, ESaving::kWill);
	feat_info[SHADOW_THROW_FEAT].getBaseParameter = &GET_REAL_DEX;
	feat_info[SHADOW_THROW_FEAT].getEffectParameter = &GET_REAL_INT;
	feat_info[SHADOW_THROW_FEAT].baseDamageBonusPercent = -30;
	feat_info[SHADOW_THROW_FEAT].degreeOfSuccessDamagePercent = 1;
	feat_info[SHADOW_THROW_FEAT].usesWeaponSkill = false;
	feat_info[SHADOW_THROW_FEAT].calculateSituationalRollBonus =
		([](CharacterData *ch, CharacterData * /* enemy */) -> short {
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)) {
				return -60;
			}
			return 0;
		});

	feat_info[SHADOW_THROW_FEAT].techniqueItemKitsGroup.reserve(2);
	techniqueItemKit = std::make_unique<TechniqueItemKitType>();
	techniqueItemKit->reserve(1);
	techniqueItemKit->push_back(TechniqueItem(WEAR_WIELD, ObjectData::ITEM_WEAPON,
											  ESkill::kAny, EExtraFlag::ITEM_THROWING));
	feat_info[SHADOW_THROW_FEAT].techniqueItemKitsGroup.push_back(std::move(techniqueItemKit));
	techniqueItemKit = std::make_unique<TechniqueItemKitType>();
	techniqueItemKit->reserve(1);
	techniqueItemKit->push_back(TechniqueItem(WEAR_HOLD, ObjectData::ITEM_WEAPON,
											  ESkill::kAny, EExtraFlag::ITEM_THROWING));
	feat_info[SHADOW_THROW_FEAT].techniqueItemKitsGroup.push_back(std::move(techniqueItemKit));
//146
	initializeFeature(SHADOW_DAGGER_FEAT, "змеев кинжал", NORMAL_FTYPE, true, feat_app,
					  80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[SHADOW_DAGGER_FEAT].getBaseParameter = &GET_REAL_INT;
	feat_info[SHADOW_DAGGER_FEAT].usesWeaponSkill = false;
//147
	initializeFeature(SHADOW_SPEAR_FEAT, "змеево копьё", NORMAL_FTYPE, true,
					  feat_app, 80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[SHADOW_SPEAR_FEAT].getBaseParameter = &GET_REAL_INT;
	feat_info[SHADOW_SPEAR_FEAT].usesWeaponSkill = false;
//148
	initializeFeature(SHADOW_CLUB_FEAT, "змеева палица", NORMAL_FTYPE, true, feat_app,
					  80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[SHADOW_CLUB_FEAT].getBaseParameter = &GET_REAL_INT;
	feat_info[SHADOW_CLUB_FEAT].usesWeaponSkill = false;
//149
	initializeFeature(DOUBLE_THROW_FEAT, "двойной бросок", ACTIVATED_FTYPE, true, feat_app,
					  100, ESkill::kPunch, ESaving::kReflex);
	feat_info[DOUBLE_THROW_FEAT].getBaseParameter = &GET_REAL_DEX;
//150
	initializeFeature(TRIPLE_THROW_FEAT, "тройной бросок", ACTIVATED_FTYPE, true, feat_app,
					  100, ESkill::kPunch, ESaving::kReflex);
	feat_info[TRIPLE_THROW_FEAT].getBaseParameter = &GET_REAL_DEX;
//1151
	initializeFeature(POWER_THROW_FEAT, "размах", NORMAL_FTYPE, true, feat_app, 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[POWER_THROW_FEAT].getBaseParameter = &GET_REAL_STR;
//152
	initializeFeature(DEADLY_THROW_FEAT, "широкий размах", NORMAL_FTYPE, true, feat_app,
					  100, ESkill::kPunch, ESaving::kReflex);
	feat_info[DEADLY_THROW_FEAT].getBaseParameter = &GET_REAL_STR;
//153
	initializeFeature(TURN_UNDEAD_FEAT, "turn undead", TECHNIQUE_FTYPE, true, feat_app,
					  70, ESkill::kTurnUndead, ESaving::kStability);
	feat_info[TURN_UNDEAD_FEAT].getBaseParameter = &GET_REAL_INT;
	feat_info[TURN_UNDEAD_FEAT].getEffectParameter = &GET_REAL_WIS;
	feat_info[TURN_UNDEAD_FEAT].usesWeaponSkill = false;
	feat_info[TURN_UNDEAD_FEAT].alwaysAvailable = true;
	feat_info[TURN_UNDEAD_FEAT].baseDamageBonusPercent = -30;
	feat_info[TURN_UNDEAD_FEAT].degreeOfSuccessDamagePercent = 2;
//154
	initializeFeature(MULTI_CAST_FEAT, "изощренные чары", NORMAL_FTYPE, true, feat_app);
//155	
	initializeFeature(MAGICAL_SHIELD_FEAT, "заговоренный щит", NORMAL_FTYPE, true, feat_app);
// 156
	initializeFeature(ANIMAL_MASTER_FEAT, "хозяин животных", NORMAL_FTYPE, true, feat_app);
}

const char *feat_name(int num) {
	if (num > 0 && num < kMaxFeats) {
		return (feat_info[num].name);
	} else {
		if (num == -1) {
			return "UNUSED";
		} else {
			return "UNDEFINED";
		}
	}
}

bool can_use_feat(const CharacterData *ch, int feat) {
	if (feat_info[feat].alwaysAvailable) {
		return true;
	};
	if ((feat == INCORRECT_FEAT) || !HAVE_FEAT(ch, feat)) {
		return false;
	};
	if (IS_NPC(ch)) {
		return true;
	};
	if (NUM_LEV_FEAT(ch) < feat_info[feat].slot[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) {
		return false;
	};
	if (GET_REAL_REMORT(ch) < feat_info[feat].minRemort[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) {
		return false;
	};

	switch (feat) {
		case WEAPON_FINESSE_FEAT:
		case SHOT_FINESSE_FEAT: return (GET_REAL_DEX(ch) > GET_REAL_STR(ch) && GET_REAL_DEX(ch) > 17);
			break;
		case PARRY_ARROW_FEAT: return (GET_REAL_DEX(ch) > 15);
			break;
		case POWER_ATTACK_FEAT: return (GET_REAL_STR(ch) > 19);
			break;
		case GREAT_POWER_ATTACK_FEAT: return (GET_REAL_STR(ch) > 21);
			break;
		case AIMING_ATTACK_FEAT: return (GET_REAL_DEX(ch) > 15);
			break;
		case GREAT_AIMING_ATTACK_FEAT: return (GET_REAL_DEX(ch) > 17);
			break;
		case DOUBLESHOT_FEAT: return (ch->get_skill(ESkill::kBows) > 39);
			break;
		case MASTER_JEWELER_FEAT: return (ch->get_skill(ESkill::kJewelry) > 59);
			break;
		case SKILLED_TRADER_FEAT: return ((ch->get_level() + GET_REAL_REMORT(ch) / 3) > 19);
			break;
		case MAGIC_USER_FEAT: return (GET_REAL_LEVEL(ch) < 25);
			break;
		case LIVE_SHIELD_FEAT: return (ch->get_skill(ESkill::kRescue) > 124);
			break;
		case SHADOW_THROW_FEAT: return (ch->get_skill(ESkill::kDarkMagic) > 120);
			break;
		case ANIMAL_MASTER_FEAT: return (ch->get_skill(ESkill::kMindMagic) > 80);
			break;
			// Костыльный блок работы скирмишера где не нужно
			// Svent TODO Для абилок не забыть реализовать провкрку состояния персонажа
		case SKIRMISHER_FEAT:
			return !(AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)
				|| GET_POS(ch) < EPosition::kFight);
			break;
		default: return true;
			break;
	}
	return true;
}

bool can_get_feat(CharacterData *ch, int feat) {
	int i, count = 0;
	if (feat <= 0 || feat >= kMaxFeats) {
		sprintf(buf, "Неверный номер способности (feat=%d, ch=%s) передан в features::can_get_feat!",
				feat, ch->get_name().c_str());
		mudlog(buf, BRF, kLevelImmortal, SYSLOG, true);
		return false;
	}
	// Если фит доступен всем и всегда - неачем его куда-то "заучиввать".
	if (feat_info[feat].alwaysAvailable) {
		return false;
	};
	if ((!feat_info[feat].classknow[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
		&& !PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), feat))
		|| (GET_REAL_REMORT(ch) < feat_info[feat].minRemort[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])) {
		return false;
	}

	if (!checkVacantFeatureSlot(ch, feat)) {
		return false;
	}

	switch (feat) {
		case PARRY_ARROW_FEAT: return (ch->get_skill(ESkill::kMultiparry) || ch->get_skill(ESkill::kParry));
			break;
		case CONNOISEUR_FEAT: return (ch->get_skill(ESkill::kIdentify));
			break;
		case EXORCIST_FEAT: return (ch->get_skill(ESkill::kTurnUndead));
			break;
		case HEALER_FEAT: return (ch->get_skill(ESkill::kFirstAid));
			break;
		case STEALTHY_FEAT:
			return (ch->get_skill(ESkill::kHide) || ch->get_skill(ESkill::kSneak) || ch->get_skill(ESkill::kDisguise));
			break;
		case TRACKER_FEAT: return (ch->get_skill(ESkill::kTrack) || ch->get_skill(ESkill::kSense));
			break;
		case PUNCH_MASTER_FEAT:
		case CLUBS_MASTER_FEAT:
		case AXES_MASTER_FEAT:
		case LONGS_MASTER_FEAT:
		case SHORTS_MASTER_FEAT:
		case NONSTANDART_MASTER_FEAT:
		case BOTHHANDS_MASTER_FEAT:
		case PICK_MASTER_FEAT:
		case SPADES_MASTER_FEAT:
		case BOWS_MASTER_FEAT:
			if (!HAVE_FEAT(ch, (ubyte) feat_info[feat].affected[1].location))
				return false;
			for (i = PUNCH_MASTER_FEAT; i <= BOWS_MASTER_FEAT; i++)
				if (HAVE_FEAT(ch, i))
					count++;
			if (count >= 1 + GET_REAL_REMORT(ch) / 7)
				return false;
			break;
		case SPIRIT_WARRIOR_FEAT: return (HAVE_FEAT(ch, GREAT_FORTITUDE_FEAT));
			break;
		case NIMBLE_FINGERS_FEAT: return (ch->get_skill(ESkill::kSteal) || ch->get_skill(ESkill::kPickLock));
			break;
		case GREAT_POWER_ATTACK_FEAT: return (HAVE_FEAT(ch, POWER_ATTACK_FEAT));
			break;
		case PUNCH_FOCUS_FEAT:
		case CLUB_FOCUS_FEAT:
		case AXES_FOCUS_FEAT:
		case LONGS_FOCUS_FEAT:
		case SHORTS_FOCUS_FEAT:
		case NONSTANDART_FOCUS_FEAT:
		case BOTHHANDS_FOCUS_FEAT:
		case PICK_FOCUS_FEAT:
		case SPADES_FOCUS_FEAT:
		case BOWS_FOCUS_FEAT:
			if (!ch->get_skill(feat_info[feat].base_skill)) {
				return false;
			}

			for (i = PUNCH_FOCUS_FEAT; i <= BOWS_FOCUS_FEAT; i++) {
				if (HAVE_FEAT(ch, i)) {
					count++;
				}
			}

			if (count >= 2 + GET_REAL_REMORT(ch) / 6) {
				return false;
			}
			break;
		case GREAT_AIMING_ATTACK_FEAT: return (HAVE_FEAT(ch, AIMING_ATTACK_FEAT));
			break;
		case DOUBLESHOT_FEAT: return (HAVE_FEAT(ch, BOWS_FOCUS_FEAT) && ch->get_skill(ESkill::kBows) > 39);
			break;
		case MASTER_JEWELER_FEAT: return (ch->get_skill(ESkill::kJewelry) > 59);
			break;
		case EXPEDIENT_CUT_FEAT:
			return (HAVE_FEAT(ch, SHORTS_MASTER_FEAT)
				|| HAVE_FEAT(ch, PICK_MASTER_FEAT)
				|| HAVE_FEAT(ch, LONGS_MASTER_FEAT)
				|| HAVE_FEAT(ch, SPADES_MASTER_FEAT)
				|| HAVE_FEAT(ch, BOTHHANDS_MASTER_FEAT));
			break;
		case SKIRMISHER_FEAT: return (ch->get_skill(ESkill::kRescue));
			break;
		case TACTICIAN_FEAT: return (ch->get_skill(ESkill::kLeadership) > 99);
			break;
		case SHADOW_THROW_FEAT: return (HAVE_FEAT(ch, POWER_THROW_FEAT) && (ch->get_skill(ESkill::kDarkMagic) > 120));
			break;
		case SHADOW_DAGGER_FEAT:
		case SHADOW_SPEAR_FEAT: [[fallthrough]];
		case SHADOW_CLUB_FEAT: return (HAVE_FEAT(ch, SHADOW_THROW_FEAT) && (ch->get_skill(ESkill::kDarkMagic) > 130));
			break;
		case DOUBLE_THROW_FEAT: return (HAVE_FEAT(ch, POWER_THROW_FEAT) && (ch->get_skill(ESkill::kThrow) > 100));
			break;
		case TRIPLE_THROW_FEAT: return (HAVE_FEAT(ch, DEADLY_THROW_FEAT) && (ch->get_skill(ESkill::kThrow) > 130));
			break;
		case POWER_THROW_FEAT: return (ch->get_skill(ESkill::kThrow) > 90);
			break;
		case DEADLY_THROW_FEAT: return (HAVE_FEAT(ch, POWER_THROW_FEAT) && (ch->get_skill(ESkill::kThrow) > 110));
			break;
		default: return true;
			break;
	}

	return true;
}

bool checkVacantFeatureSlot(CharacterData *ch, int feat) {
	int i, lowfeat, hifeat;

	if (feat_info[feat].inbornFeatureOfClass[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
		|| PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), feat))
		return true;

	//сколько у нас вообще способностей, у которых слот меньше требуемого, и сколько - тех, у которых больше или равно?
	lowfeat = 0;
	hifeat = 0;

	//Мы не можем просто учесть кол-во способностей меньше требуемого и больше требуемого,
	//т.к. возможны свободные слоты меньше требуемого, и при этом верхние заняты все
	auto slot_list = std::vector<int>();
	for (i = 1; i < kMaxFeats; ++i) {
		if (feat_info[i].inbornFeatureOfClass[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
			|| PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), i))
			continue;

		if (HAVE_FEAT(ch, i)) {
			if (FEAT_SLOT(ch, i) >= FEAT_SLOT(ch, feat)) {
				++hifeat;
			} else {
				slot_list.push_back(FEAT_SLOT(ch, i));
			}
		}
	}

	std::sort(slot_list.begin(), slot_list.end());

	//Посчитаем сколько действительно нижние способности занимают слотов (с учетом пропусков)
	for (const auto &slot: slot_list) {
		if (lowfeat < slot) {
			lowfeat = slot + 1;
		} else {
			++lowfeat;
		}
	}

	//из имеющегося количества слотов нужно вычесть:
	//число высоких слотов, занятых низкоуровневыми способностями,
	//с учетом, что низкоуровневые могут и не занимать слотов выше им положенных,
	//а также собственно число слотов, занятых высокоуровневыми способностями
	if (NUM_LEV_FEAT(ch) - FEAT_SLOT(ch, feat) - hifeat - MAX(0, lowfeat - FEAT_SLOT(ch, feat)) > 0)
		return true;

	//oops.. слотов нет
	return false;
}

int getModifier(int feat, int location) {
	for (int i = 0; i < MAX_FEAT_AFFECT; i++) {
		if (feat_info[feat].affected[i].location == location) {
			return (int) feat_info[feat].affected[i].modifier;
		}
	}
	return 0;
}

void check_berserk(CharacterData *ch) {
	struct TimedFeat timed;
	int prob;

	if (affected_by_spell(ch, kSpellBerserk) &&
		(GET_HIT(ch) > GET_REAL_MAX_HIT(ch) / 2)) {
		affect_from_char(ch, kSpellBerserk);
		send_to_char("Предсмертное исступление оставило вас.\r\n", ch);
	}

	if (can_use_feat(ch, BERSERK_FEAT) && ch->get_fighting() &&
		!IsTimed(ch, BERSERK_FEAT) && !AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK)
		&& (GET_HIT(ch) < GET_REAL_MAX_HIT(ch) / 4)) {
		CharacterData *vict = ch->get_fighting();
		timed.feat = BERSERK_FEAT;
		timed.time = 4;
		ImposeTimedFeat(ch, &timed);

		Affect<EApplyLocation> af;
		af.type = kSpellBerserk;
		af.duration = pc_duration(ch, 1, 60, 30, 0, 0);
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.battleflag = 0;

		prob = IS_NPC(ch) ? 601 : (751 - GET_REAL_LEVEL(ch) * 5);
		if (number(1, 1000) < prob) {
			af.bitvector = to_underlying(EAffectFlag::AFF_BERSERK);
			act("Вас обуяла предсмертная ярость!", false, ch, nullptr, nullptr, TO_CHAR);
			act("$n0 исступленно взвыл$g и бросил$u на противника!", false, ch, nullptr, vict, TO_NOTVICT);
			act("$n0 исступленно взвыл$g и бросил$u на вас!", false, ch, nullptr, vict, TO_VICT);
		} else {
			af.bitvector = 0;
			act("Вы истошно завопили, пытаясь напугать противника. Без толку.", false, ch, nullptr, nullptr, TO_CHAR);
			act("$n0 истошно завопил$g, пытаясь напугать противника. Забавно...", false, ch, nullptr, vict, TO_NOTVICT);
			act("$n0 истошно завопил$g, пытаясь напугать вас. Забавно...", false, ch, nullptr, vict, TO_VICT);
		}
		affect_join(ch, af, true, false, true, false);
	}
}

void do_lightwalk(CharacterData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	struct TimedFeat timed;

	if (IS_NPC(ch) || !can_use_feat(ch, LIGHT_WALK_FEAT)) {
		send_to_char("Вы не можете этого.\r\n", ch);
		return;
	}

	if (ch->ahorse()) {
		act("Позаботьтесь сперва о мягких тапочках для $N3...", false, ch, nullptr, ch->get_horse(), TO_CHAR);
		return;
	}

	if (affected_by_spell(ch, kSpellLightWalk)) {
		send_to_char("Вы уже двигаетесь легким шагом.\r\n", ch);
		return;
	}
	if (IsTimed(ch, LIGHT_WALK_FEAT)) {
		send_to_char("Вы слишком утомлены для этого.\r\n", ch);
		return;
	}

	affect_from_char(ch, kSpellLightWalk);

	timed.feat = LIGHT_WALK_FEAT;
	timed.time = 24;
	ImposeTimedFeat(ch, &timed);

	send_to_char("Хорошо, вы попытаетесь идти, не оставляя лишних следов.\r\n", ch);
	Affect<EApplyLocation> af;
	af.type = kSpellLightWalk;
	af.duration = pc_duration(ch, 2, GET_REAL_LEVEL(ch), 5, 2, 8);
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.battleflag = 0;
	if (number(1, 1000) > number(1, GET_REAL_DEX(ch) * 50)) {
		af.bitvector = 0;
		send_to_char("Вам не хватает ловкости...\r\n", ch);
	} else {
		af.bitvector = to_underlying(EAffectFlag::AFF_LIGHT_WALK);
		send_to_char("Ваши шаги стали легче перышка.\r\n", ch);
	}
	affect_to_char(ch, af);
}

void do_fit(CharacterData *ch, char *argument, int/* cmd*/, int subcmd) {
	ObjectData *obj;
	CharacterData *vict;
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];

	//отключено пока для не-иммов
	if (GET_REAL_LEVEL(ch) < kLevelImmortal) {
		send_to_char("Вы не можете этого.", ch);
		return;
	};

	//Может ли игрок использовать эту способность?
	if ((subcmd == SCMD_DO_ADAPT) && !can_use_feat(ch, TO_FIT_ITEM_FEAT)) {
		send_to_char("Вы не умеете этого.", ch);
		return;
	};
	if ((subcmd == SCMD_MAKE_OVER) && !can_use_feat(ch, TO_FIT_CLOTHCES_FEAT)) {
		send_to_char("Вы не умеете этого.", ch);
		return;
	};

	//Есть у нас предмет, который хотят переделать?
	argument = one_argument(argument, arg1);

	if (!*arg1) {
		send_to_char("Что вы хотите переделать?\r\n", ch);
		return;
	};

	if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
		sprintf(buf, "У вас нет \'%s\'.\r\n", arg1);
		send_to_char(buf, ch);
		return;
	};

	//На кого переделываем?
	argument = one_argument(argument, arg2);
	if (!(vict = get_char_vis(ch, arg2, FIND_CHAR_ROOM))) {
		send_to_char("Под кого вы хотите переделать эту вещь?\r\n Нет такого создания в округе!\r\n", ch);
		return;
	};

	//Предмет уже имеет владельца
	if (GET_OBJ_OWNER(obj) != 0) {
		send_to_char("У этой вещи уже есть владелец.\r\n", ch);
		return;

	};

	if ((GET_OBJ_WEAR(obj) <= 1) || OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF)) {
		send_to_char("Этот предмет невозможно переделать.\r\n", ch);
		return;
	}

	switch (subcmd) {
		case SCMD_DO_ADAPT:
			if (GET_OBJ_MATER(obj) != ObjectData::MAT_NONE
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_BULAT
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_BRONZE
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_IRON
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_STEEL
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_SWORDSSTEEL
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_COLOR
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_WOOD
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_SUPERWOOD
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_GLASS) {
				sprintf(buf, "К сожалению %s сделан%s из неподходящего материала.\r\n",
						GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_SUF_6(obj));
				send_to_char(buf, ch);
				return;
			}
			break;
		case SCMD_MAKE_OVER:
			if (GET_OBJ_MATER(obj) != ObjectData::MAT_BONE
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_MATERIA
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_SKIN
				&& GET_OBJ_MATER(obj) != ObjectData::MAT_ORGANIC) {
				sprintf(buf, "К сожалению %s сделан%s из неподходящего материала.\r\n",
						GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_SUF_6(obj));
				send_to_char(buf, ch);
				return;
			}
			break;
	};
	obj->set_owner(GET_UNIQUE(vict));
	sprintf(buf, "Вы долго пыхтели и сопели, переделывая работу по десять раз.\r\n");
	sprintf(buf + strlen(buf), "Вы извели кучу времени и 10000 кун золотом.\r\n");
	sprintf(buf + strlen(buf), "В конце-концов подогнали %s точно по мерке %s.\r\n",
			GET_OBJ_PNAME(obj, 3).c_str(), GET_PAD(vict, 1));

	send_to_char(buf, ch);

}

#include "classes/classes_spell_slots.h" // удалить после вырезания do_spell_capable
#include "magic/spells_info.h"
#define SpINFO spell_info[spellnum]
// Вложить закл в клона
void do_spell_capable(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {

	using PlayerClass::slot_for_char;

	struct TimedFeat timed;

	if (!IS_IMPL(ch) && (IS_NPC(ch) || !can_use_feat(ch, SPELL_CAPABLE_FEAT))) {
		send_to_char("Вы не столь могущественны.\r\n", ch);
		return;
	}

	if (IsTimed(ch, SPELL_CAPABLE_FEAT) && !IS_IMPL(ch)) {
		send_to_char("Невозможно использовать это так часто.\r\n", ch);
		return;
	}

	char *s;
	int spellnum;

	if (IS_NPC(ch) && AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)) {
		send_to_char("Вы не смогли вымолвить и слова.\r\n", ch);
		return;
	}

	s = strtok(argument, "'*!");
	if (s == nullptr) {
		send_to_char("ЧТО вы хотите колдовать?\r\n", ch);
		return;
	}
	s = strtok(nullptr, "'*!");
	if (s == nullptr) {
		send_to_char("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}

	spellnum = FixNameAndFindSpellNum(s);
	if (spellnum < 1 || spellnum > kSpellCount) {
		send_to_char("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}

	if ((!IS_SET(GET_SPELL_TYPE(ch, spellnum), kSpellTemp | kSpellKnow) ||
		GET_REAL_REMORT(ch) < MIN_CAST_REM(SpINFO, ch)) &&
		(GET_REAL_LEVEL(ch) < kLevelGreatGod) && (!IS_NPC(ch))) {
		if (GET_REAL_LEVEL(ch) < MIN_CAST_LEV(SpINFO, ch)
			|| GET_REAL_REMORT(ch) < MIN_CAST_REM(SpINFO, ch)
			|| slot_for_char(ch, SpINFO.slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) <= 0) {
			send_to_char("Рано еще вам бросаться такими словами!\r\n", ch);
			return;
		} else {
			send_to_char("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
			return;
		}
	}

	if (!GET_SPELL_MEM(ch, spellnum) && !IS_IMMORTAL(ch)) {
		send_to_char("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
		return;
	}

	Follower *k;
	CharacterData *follower = nullptr;
	for (k = ch->followers; k; k = k->next) {
		if (AFF_FLAGGED(k->ch, EAffectFlag::AFF_CHARM)
			&& k->ch->get_master() == ch
			&& MOB_FLAGGED(k->ch, MOB_CLONE)
			&& !affected_by_spell(k->ch, kSpellCapable)
			&& ch->in_room == IN_ROOM(k->ch)) {
			follower = k->ch;
			break;
		}
	}
	if (!GET_SPELL_MEM(ch, spellnum) && !IS_IMMORTAL(ch)) {
		send_to_char("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
		return;
	}

	if (!follower) {
		send_to_char("Хорошо бы найти подходящую цель для этого.\r\n", ch);
		return;
	}

	act("Вы принялись зачаровывать $N3.", false, ch, nullptr, follower, TO_CHAR);
	act("$n принял$u делать какие-то пассы и что-то бормотать в сторону $N3.", false, ch, nullptr, follower, TO_ROOM);

	GET_SPELL_MEM(ch, spellnum)--;
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, PRF_AUTOMEM))
		MemQ_remember(ch, spellnum);

	if (!IS_SET(SpINFO.routines, kMagDamage) || !SpINFO.violent ||
		IS_SET(SpINFO.routines, kMagMasses) || IS_SET(SpINFO.routines, kMagGroups) ||
		IS_SET(SpINFO.routines, kMagAreas)) {
		send_to_char("Вы конечно мастер, но не такой магии.\r\n", ch);
		return;
	}
	affect_from_char(ch, SPELL_CAPABLE_FEAT);

	timed.feat = SPELL_CAPABLE_FEAT;

	switch (SpINFO.slot_forc[GET_CLASS(ch)][GET_KIN(ch)]) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5://1-5 слоты кд 4 тика
			timed.time = 4;
			break;
		case 6:
		case 7://6-7 слоты кд 6 тиков
			timed.time = 6;
			break;
		case 8://8 слот кд 10 тиков
			timed.time = 10;
			break;
		case 9://9 слот кд 12 тиков
			timed.time = 12;
			break;
		default://10 слот или тп
			timed.time = 24;
	}
	ImposeTimedFeat(ch, &timed);

	GET_CAST_SUCCESS(follower) = GET_REAL_REMORT(ch) * 4;
	Affect<EApplyLocation> af;
	af.type = kSpellCapable;
	af.duration = 48;
	if (GET_REAL_REMORT(ch) > 0) {
		af.modifier = GET_REAL_REMORT(ch) * 4;//вешаецо аффект который дает +морт*4 касту
		af.location = APPLY_CAST_SUCCESS;
	} else {
		af.modifier = 0;
		af.location = APPLY_NONE;
	}
	af.battleflag = 0;
	af.bitvector = 0;
	affect_to_char(follower, af);
	follower->mob_specials.capable_spell = spellnum;
}

void setFeaturesOfRace(CharacterData *ch) {
	std::vector<int> feat_list = PlayerRace::GetRaceFeatures((int) GET_KIN(ch), (int) GET_RACE(ch));
	for (int &i: feat_list) {
		if (can_get_feat(ch, i)) {
			SET_FEAT(ch, i);
		}
	}
}

void unsetFeaturesOfRace(CharacterData *ch) {
	std::vector<int> feat_list = PlayerRace::GetRaceFeatures((int) GET_KIN(ch), (int) GET_RACE(ch));
	for (int &i: feat_list) {
		UNSET_FEAT(ch, i);
	}
}

void setInbornFeaturesOfClass(CharacterData *ch) {
	for (int i = 1; i < kMaxFeats; ++i) {
		if (can_get_feat(ch, i) && feat_info[i].inbornFeatureOfClass[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) {
			SET_FEAT(ch, i);
		}
	}
}

void setAllInbornFeatures(CharacterData *ch) {
	setInbornFeaturesOfClass(ch);
	setFeaturesOfRace(ch);
}

int CFeatArray::pos(int pos /*= -1*/) {
	if (pos == -1) {
		return _pos;
	} else if (pos >= 0 && pos < MAX_FEAT_AFFECT) {
		_pos = pos;
		return _pos;
	}
	sprintf(buf, "SYSERR: invalid argument (%d) was sended to features::aff_aray.pos!", pos);
	mudlog(buf, BRF, kLevelGod, SYSLOG, true);
	return _pos;
}

void CFeatArray::insert(const int location, int modifier) {
	affected[_pos].location = location;
	affected[_pos].modifier = modifier;
	_pos++;
	if (_pos >= MAX_FEAT_AFFECT) {
		_pos = 0;
	}
}

void CFeatArray::clear() {
	_pos = 0;
	for (i = 0; i < MAX_FEAT_AFFECT; i++) {
		affected[i].location = APPLY_NONE;
		affected[i].modifier = 0;
	}
}

bool tryFlipActivatedFeature(CharacterData *ch, char *argument) {
	int featureNum = get_feature_num(argument);
	if (featureNum <= INCORRECT_FEAT) {
		return false;
	}
	if (!checkAccessActivatedFeature(ch, featureNum)) {
		return true;
	};
	if (PRF_FLAGGED(ch, getPRFWithFeatureNumber(featureNum))) {
		deactivateFeature(ch, featureNum);
	} else {
		activateFeature(ch, featureNum);
	}

	setSkillCooldown(ch, ESkill::kGlobalCooldown, 2);
	return true;
}

void activateFeature(CharacterData *ch, int featureNum) {
	switch (featureNum) {
		case POWER_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_AIMINGATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATAIMINGATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATPOWERATTACK);
			PRF_FLAGS(ch).set(PRF_POWERATTACK);
			break;
		case GREAT_POWER_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_POWERATTACK);
			PRF_FLAGS(ch).unset(PRF_AIMINGATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATAIMINGATTACK);
			PRF_FLAGS(ch).set(PRF_GREATPOWERATTACK);
			break;
		case AIMING_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_POWERATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATAIMINGATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATPOWERATTACK);
			PRF_FLAGS(ch).set(PRF_AIMINGATTACK);
			break;
		case GREAT_AIMING_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_POWERATTACK);
			PRF_FLAGS(ch).unset(PRF_AIMINGATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATPOWERATTACK);
			PRF_FLAGS(ch).set(PRF_GREATAIMINGATTACK);
			break;
		case SKIRMISHER_FEAT:
			if (!AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)) {
				send_to_char(ch,
							 "Голос десятника Никифора вдруг рявкнул: \"%s, тюрюхайло! 'В шеренгу по одному' иначе сполняется!\"\r\n",
							 ch->get_name().c_str());
				return;
			}
			if (PRF_FLAGGED(ch, PRF_SKIRMISHER)) {
				send_to_char("Вы уже стоите в передовом строю.\r\n", ch);
				return;
			}
			PRF_FLAGS(ch).set(PRF_SKIRMISHER);
			send_to_char("Вы протиснулись вперед и встали в строй.\r\n", ch);
			act("$n0 протиснул$u вперед и встал$g в строй.", false, ch, nullptr, nullptr, TO_ROOM | TO_ARENA_LISTEN);
			break;
		case DOUBLE_THROW_FEAT: PRF_FLAGS(ch).unset(PRF_TRIPLE_THROW);
			PRF_FLAGS(ch).set(PRF_DOUBLE_THROW);
			break;
		case TRIPLE_THROW_FEAT: PRF_FLAGS(ch).unset(PRF_DOUBLE_THROW);
			PRF_FLAGS(ch).set(PRF_TRIPLE_THROW);
			break;
		default: break;
	}
	send_to_char(ch,
				 "%sВы решили использовать способность '%s'.%s\r\n",
				 CCIGRN(ch, C_SPR),
				 feat_info[featureNum].name,
				 CCNRM(ch, C_OFF));
}

void deactivateFeature(CharacterData *ch, int featureNum) {
	switch (featureNum) {
		case POWER_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_POWERATTACK);
			break;
		case GREAT_POWER_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_GREATPOWERATTACK);
			break;
		case AIMING_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_AIMINGATTACK);
			break;
		case GREAT_AIMING_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_GREATAIMINGATTACK);
			break;
		case SKIRMISHER_FEAT: PRF_FLAGS(ch).unset(PRF_SKIRMISHER);
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)) {
				send_to_char("Вы решили, что в обозе вам будет спокойней.\r\n", ch);
				act("$n0 тактически отступил$g в тыл отряда.", false, ch, nullptr, nullptr, TO_ROOM | TO_ARENA_LISTEN);
			}
			break;
		case DOUBLE_THROW_FEAT: PRF_FLAGS(ch).unset(PRF_DOUBLE_THROW);
			break;
		case TRIPLE_THROW_FEAT: PRF_FLAGS(ch).unset(PRF_TRIPLE_THROW);
			break;
	}
	send_to_char(ch,
				 "%sВы прекратили использовать способность '%s'.%s\r\n",
				 CCIGRN(ch, C_SPR),
				 feat_info[featureNum].name,
				 CCNRM(ch, C_OFF));
}

bool checkAccessActivatedFeature(CharacterData *ch, int featureNum) {
	if (!can_use_feat(ch, featureNum)) {
		send_to_char("Вы не в состоянии использовать эту способность.\r\n", ch);
		return false;
	}
	if (feat_info[featureNum].type != ACTIVATED_FTYPE) {
		send_to_char("Эту способность невозможно применить таким образом.\r\n", ch);
		return false;
	}

	return true;
}

int get_feature_num(char *featureName) {
	skip_spaces(&featureName);
	return find_feat_num(featureName);
}

/*
 TODO: при переписывании способностей переделать на композицию или интерфейс
*/
Bitvector getPRFWithFeatureNumber(int featureNum) {
	switch (featureNum) {
		case POWER_ATTACK_FEAT: return PRF_POWERATTACK;
			break;
		case GREAT_POWER_ATTACK_FEAT: return PRF_GREATPOWERATTACK;
			break;
		case AIMING_ATTACK_FEAT: return PRF_AIMINGATTACK;
			break;
		case GREAT_AIMING_ATTACK_FEAT: return PRF_GREATAIMINGATTACK;
			break;
		case SKIRMISHER_FEAT: return PRF_SKIRMISHER;
			break;
		case DOUBLE_THROW_FEAT: return PRF_DOUBLE_THROW;
			break;
		case TRIPLE_THROW_FEAT: return PRF_TRIPLE_THROW;
			break;
	}

	return PRF_POWERATTACK;
}

/*
* Ситуативный бонус броска для "tactician feat" и "skirmisher feat":
* Каждый персонаж в строю прикрывает двух, третий дает штраф.
* Избыток "строевиков" повышает шанс на удачное срабатывание.
* Svent TODO: Придумать более универсальный механизм бонусов/штрафов в зависимости от данных абилки
*/
short calculateSituationalRollBonusOfGroupFormation(CharacterData *ch, CharacterData * /* enemy */) {
	ActionTargeting::FriendsRosterType roster{ch};
	int skirmishers = roster.count([](CharacterData *ch) { return PRF_FLAGGED(ch, PRF_SKIRMISHER); });
	int uncoveredSquadMembers = roster.amount() - skirmishers;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)) {
		return (skirmishers * 2 - uncoveredSquadMembers) * kCircumstanceFactor - 40;
	};
	return (skirmishers * 2 - uncoveredSquadMembers) * kCircumstanceFactor;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
