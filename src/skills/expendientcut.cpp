//
// Created by ubuntu on 03/09/20.
//

#include "expendientcut.h"
#include "fightsystem/common.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/pk.h"
#include "skills/protect.h"
#include <cmath>
using namespace FightSystem;

ESkill ExpedientWeaponSkill(CharacterData *ch) {
	ESkill skill = ESkill::kPunch;

	if (GET_EQ(ch, WEAR_WIELD) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == CObjectPrototype::ITEM_WEAPON)) {
		skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_WIELD));
	} else if (GET_EQ(ch, WEAR_BOTHS) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS)) == CObjectPrototype::ITEM_WEAPON)) {
		skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS));
	} else if (GET_EQ(ch, WEAR_HOLD) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == CObjectPrototype::ITEM_WEAPON)) {
		skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_HOLD));
	};

	return skill;
}
int GetExpedientKeyParameter(CharacterData *ch, ESkill skill) {
	switch (skill) {
		case ESkill::kPunch:
		case ESkill::kClubs:
		case ESkill::kAxes:
		case ESkill::kTwohands:
		case ESkill::kSpades:return ch->get_str();
			break;
		case ESkill::kLongBlades:
		case ESkill::kShortBlades:
		case ESkill::kNonstandart:
		case ESkill::kBows:
		case ESkill::kPicks:return ch->get_dex();
			break;
		default:return ch->get_str();
	}
}
int ParameterBonus(int parameter) {
	return ((parameter - 20) / 4);
}
int ExpedientRating(CharacterData *ch, ESkill skill) {
	return floor(ch->get_skill(skill) / 2.00 + ParameterBonus(GetExpedientKeyParameter(ch, skill)));
}
int ExpedientCap(CharacterData *ch, ESkill skill) {
	if (!IS_NPC(ch)) {
		return floor(1.33 * (CalcSkillRemortCap(ch) / 2.00 + ParameterBonus(GetExpedientKeyParameter(ch, skill))));
	} else {
		return floor(1.33 * ((kSkillCapOnZeroRemort + 5 * MAX(0, GET_REAL_LEVEL(ch) - 30) / 2.00
			+ ParameterBonus(GetExpedientKeyParameter(ch, skill)))));
	}
}
int DegreeOfSuccess(int roll, int rating) {
	return ((rating - roll) / 5);
}

bool CheckExpedientSuccess(CharacterData *ch, CharacterData *victim) {
	ESkill DoerSkill = ExpedientWeaponSkill(ch);
	int DoerRating = ExpedientRating(ch, DoerSkill);
	int DoerCap = ExpedientCap(ch, DoerSkill);
	int DoerRoll = dice(1, DoerCap);
	int DoerSuccess = DegreeOfSuccess(DoerRoll, DoerRating);

	ESkill VictimSkill = ExpedientWeaponSkill(victim);
	int VictimRating = ExpedientRating(victim, VictimSkill);
	int VictimCap = ExpedientCap(victim, VictimSkill);
	int VictimRoll = dice(1, VictimCap);
	int VictimSuccess = DegreeOfSuccess(VictimRoll, VictimRating);

	if ((DoerRoll <= DoerRating) && (VictimRoll > VictimRating))
		return true;
	if ((DoerRoll > DoerRating) && (VictimRoll <= VictimRating))
		return false;
	if ((DoerRoll > DoerRating) && (VictimRoll > VictimRating))
		return CheckExpedientSuccess(ch, victim);

	if (DoerSuccess > VictimSuccess)
		return true;
	if (DoerSuccess < VictimSuccess)
		return false;

	if (ParameterBonus(GetExpedientKeyParameter(ch, DoerSkill))
		> ParameterBonus(GetExpedientKeyParameter(victim, VictimSkill)))
		return true;
	if (ParameterBonus(GetExpedientKeyParameter(ch, DoerSkill))
		< ParameterBonus(GetExpedientKeyParameter(victim, VictimSkill)))
		return false;

	if (DoerRoll < VictimRoll)
		return true;
	if (DoerRoll > VictimRoll)
		return true;

	return CheckExpedientSuccess(ch, victim);
}

void ApplyNoFleeAffect(CharacterData *ch, int duration) {
	//Это жутко криво, но почемук-то при простановке сразу 2 флагов битвектора начинаются глюки, хотя все должно быть нормально
	//По-видимому, где-то просто не учтено, что ненулевых битов может быть более 1
	Affect<EApplyLocation> Noflee;
	Noflee.type = kSpellBattle;
	Noflee.bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
	Noflee.location = EApplyLocation::APPLY_NONE;
	Noflee.modifier = 0;
	Noflee.duration = pc_duration(ch, duration, 0, 0, 0, 0);;
	Noflee.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
	affect_join(ch, Noflee, true, false, true, false);

	Affect<EApplyLocation> Battle;
	Battle.type = kSpellBattle;
	Battle.bitvector = to_underlying(EAffectFlag::AFF_EXPEDIENT);
	Battle.location = EApplyLocation::APPLY_NONE;
	Battle.modifier = 0;
	Battle.duration = pc_duration(ch, duration, 0, 0, 0, 0);;
	Battle.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
	affect_join(ch, Battle, true, false, true, false);

	send_to_char("Вы выпали из ритма боя.\r\n", ch);
}

void go_cut_shorts(CharacterData *ch, CharacterData *vict) {

	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_EXPEDIENT)) {
		send_to_char("Вы еще не восстановил равновесие после предыдущего приема.\r\n", ch);
		return;
	}

	vict = try_protect(vict, ch);

	if (!CheckExpedientSuccess(ch, vict)) {
		act("Ваши свистящие удары пропали втуне, не задев $N3.", false, ch, 0, vict, TO_CHAR);
		Damage dmg(SkillDmg(ESkill::kShortBlades), ZERO_DMG, PHYS_DMG, nullptr); //подумать как вычислить скилл оружия
		dmg.skill_id = ESkill::kUndefined;
		dmg.process(ch, vict);
		ApplyNoFleeAffect(ch, 2);
		return;
	}

	act("$n сделал$g неуловимое движение и на мгновение исчез$q из вида.", false, ch, 0, vict, TO_VICT);
	act("$n сделал$g неуловимое движение, сместившись за спину $N1.",
		true, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
	hit(ch, vict, ESkill::kUndefined, FightSystem::MAIN_HAND);
	hit(ch, vict, ESkill::kUndefined, FightSystem::OFF_HAND);

	Affect<EApplyLocation> AffectImmunPhysic;
	AffectImmunPhysic.type = kSpellExpedient;
	AffectImmunPhysic.location = EApplyLocation::APPLY_PR;
	AffectImmunPhysic.modifier = 100;
	AffectImmunPhysic.duration = pc_duration(ch, 3, 0, 0, 0, 0);
	AffectImmunPhysic.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
	affect_join(ch, AffectImmunPhysic, false, false, false, false);
	Affect<EApplyLocation> AffectImmunMagic;
	AffectImmunMagic.type = kSpellExpedient;
	AffectImmunMagic.location = EApplyLocation::APPLY_MR;
	AffectImmunMagic.modifier = 100;
	AffectImmunMagic.duration = pc_duration(ch, 3, 0, 0, 0, 0);
	AffectImmunMagic.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
	affect_join(ch, AffectImmunMagic, false, false, false, false);

	ApplyNoFleeAffect(ch, 3);
}

void SetExtraAttackCutShorts(CharacterData *ch, CharacterData *victim) {
	if (!isHaveNoExtraAttack(ch)) {
		return;
	}

	if (!pk_agro_action(ch, victim)) {
		return;
	}

	if (!ch->get_fighting()) {
		act("Ваше оружие свистнуло, когда вы бросились на $N3, применив \"порез\".", false, ch, 0, victim, TO_CHAR);
		set_fighting(ch, victim);
		ch->set_extra_attack(kExtraAttackCutShorts, victim);
	} else {
		act("Хорошо. Вы попытаетесь порезать $N3.", false, ch, 0, victim, TO_CHAR);
		ch->set_extra_attack(kExtraAttackCutShorts, victim);
	}
}

void SetExtraAttackCutPick(CharacterData *ch, CharacterData *victim) {
	if (!isHaveNoExtraAttack(ch)) {
		return;
	}
	if (!pk_agro_action(ch, victim)) {
		return;
	}

	if (!ch->get_fighting()) {
		act("Вы перехватили оружие обратным хватом и проскользнули за спину $N1.", false, ch, 0, victim, TO_CHAR);
		set_fighting(ch, victim);
		ch->set_extra_attack(kExtraAttackPick, victim);
	} else {
		act("Хорошо. Вы попытаетесь порезать $N3.", false, ch, 0, victim, TO_CHAR);
		ch->set_extra_attack(kExtraAttackPick, victim);
	}
}

ESkill GetExpedientCutSkill(CharacterData *ch) {
	ESkill skill = ESkill::kIncorrect;

	if (GET_EQ(ch, WEAR_WIELD) && GET_EQ(ch, WEAR_HOLD)) {
		skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_WIELD));
		if (skill != static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_HOLD))) {
			send_to_char("Для этого приема в обеих руках нужно держать оружие одого типа!\r\n", ch);
			return ESkill::kIncorrect;
		}
	} else if (GET_EQ(ch, WEAR_BOTHS)) {
		skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS));
	} else {
		send_to_char("Для этого приема вам надо использовать одинаковое оружие в обеих руках либо двуручное.\r\n", ch);
		return ESkill::kIncorrect;
	}

	if (!can_use_feat(ch, FindWeaponMasterFeat(skill)) && !IS_IMPL(ch)) {
		send_to_char("Вы недостаточно искусны в обращении с этим видом оружия.\r\n", ch);
		return ESkill::kIncorrect;
	}

	return skill;
}

void do_expedient_cut(CharacterData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {

	ESkill skill;

	if (IS_NPC(ch) || (!can_use_feat(ch, EXPEDIENT_CUT_FEAT) && !IS_IMPL(ch))) {
		send_to_char("Вы не владеете таким приемом.\r\n", ch);
		return;
	}

	if (ch->ahorse()) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	if (!isHaveNoExtraAttack(ch)) {
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	CharacterData *vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы хотите порезать?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы таки да? Ой-вей, но тут таки Древняя Русь, а не Палестина!\r\n", ch);
		return;
	}
	if (ch->get_fighting() && vict->get_fighting() != ch) {
		act("$N не сражается с вами, не трогайте $S.", false, ch, 0, vict, TO_CHAR);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	skill = GetExpedientCutSkill(ch);
	if (skill == ESkill::kIncorrect)
		return;

	switch (skill) {
		case ESkill::kShortBlades:SetExtraAttackCutShorts(ch, vict);
			break;
		case ESkill::kSpades:SetExtraAttackCutShorts(ch, vict);
			break;
		case ESkill::kLongBlades:
		case ESkill::kTwohands:
			send_to_char("Порез мечом (а тем более двуручником или копьем) - это сурьезно. Но пока невозможно.\r\n",
						 ch);
			break;
		default:send_to_char("Ваше оружие не позволяет провести такой прием.\r\n", ch);
	}

}
