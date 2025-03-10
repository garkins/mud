// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_HIT_HPP_
#define _FIGHT_HIT_HPP_

#include "fight_constants.h"
#include "affects/affect_handler.h"
#include "utils/utils.h"
#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"

struct HitData {
	HitData() : weapon(FightSystem::MAIN_HAND), wielded(nullptr), weapon_pos(WEAR_WIELD), weap_skill(ESkill::kIncorrect),
				weap_skill_is(0), skill_num(ESkill::kUndefined), hit_type(0), hit_no_parry(false),
				ch_start_pos(EPosition::kIncorrect), victim_start_pos(EPosition::kIncorrect), victim_ac(0), calc_thaco(0),
				dam(0), dam_critic(0) {
		diceroll = number(100, 2099) / 100;
	};

	// hit
	void init(CharacterData *ch, CharacterData *victim);
	void calc_base_hr(CharacterData *ch);
	void calc_rand_hr(CharacterData *ch, CharacterData *victim);
	void calc_stat_hr(CharacterData *ch);
	void calc_ac(CharacterData *victim);
	void add_weapon_damage(CharacterData *ch, bool need_dice);
	void add_hand_damage(CharacterData *ch, bool need_dice);
	void check_defense_skills(CharacterData *ch, CharacterData *victim);
	void calc_crit_chance(CharacterData *ch);
	int calc_damage(CharacterData *ch, bool need_dice);
	double crit_backstab_multiplier(CharacterData *ch, CharacterData *victim);

	// extdamage
	int extdamage(CharacterData *ch, CharacterData *victim);
	void try_mighthit_dam(CharacterData *ch, CharacterData *victim);
	void try_stupor_dam(CharacterData *ch, CharacterData *victim);
	void compute_critical(CharacterData *ch, CharacterData *victim);

	// init()
	// 1 - атака правой или двумя руками (RIGHT_WEAPON),
	// 2 - атака левой рукой (LEFT_WEAPON)
	FightSystem::AttType weapon;
	// пушка, которой в данный момент производится удар
	ObjectData *wielded;
	// номер позиции (NUM_WEARS) пушки
	int weapon_pos;
	// номер скила, взятый из пушки или голых рук
	ESkill weap_skill;
	// очки скила weap_skill у чара, взятые через train_skill (могут быть сфейлены)
	int weap_skill_is;
	// брошенные кубики на момент расчета попадания
	int diceroll;
	// номер скила, пришедший из вызова hit(), может быть kUndefined
	// в целом если < 0 - считается, что бьем простой атакой hit_type
	// если >= 0 - считается, что бьем скилом
	ESkill skill_num;
	// тип удара пушкой или руками (attack_hit_text[])
	// инится в любом случае независимо от skill_num
	int hit_type;
	// true - удар не парируется/не блочится/не веерится и т.п.
	bool hit_no_parry;
	// позиция атакующего на начало атаки
	EPosition ch_start_pos;
	// позиция жертвы на начало атаки
	EPosition victim_start_pos;

	// высчитывается по мере сил
	// ац жертвы для расчета попадания
	int victim_ac;
	// хитролы атакующего для расчета попадания
	int calc_thaco;
	// дамаг атакующего
	int dam;
	// flags[CRIT_HIT] = true, dam_critic = 0 - критический удар
	// flags[CRIT_HIT] = true, dam_critic > 0 - удар точным стилем
	int dam_critic;

 public:
	using flags_t = std::bitset<FightSystem::HIT_TYPE_FLAGS_NUM>;

	const flags_t &get_flags() const { return m_flags; }
	void set_flag(const size_t flag) { m_flags.set(flag); }
	void reset_flag(const size_t flag) { m_flags.reset(flag); }
	static void CheckWeapFeats(const CharacterData *ch, ESkill weap_skill, int &calc_thaco, int &dam);
 private:
	// какой-никакой набор флагов, так же передается в damage()
	flags_t m_flags;
};

int compute_armor_class(CharacterData *ch);

int check_agro_follower(CharacterData *ch, CharacterData *victim);
void set_battle_pos(CharacterData *ch);

void gain_battle_exp(CharacterData *ch, CharacterData *victim, int dam);
void perform_group_gain(CharacterData *ch, CharacterData *victim, int members, int koef);
void group_gain(CharacterData *ch, CharacterData *victim);

char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
bool check_valid_chars(CharacterData *ch, CharacterData *victim, const char *fname, int line);

void exthit(CharacterData *ch, ESkill type, FightSystem::AttType weapon);
void hit(CharacterData *ch, CharacterData *victim, ESkill type, FightSystem::AttType weapon);

void appear(CharacterData *ch);

int GetRealDamroll(CharacterData *ch);
int GetAutoattackDamroll(CharacterData *ch, int weapon_skill);

#endif // _FIGHT_HIT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
