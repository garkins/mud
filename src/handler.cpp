/* ************************************************************************
*   File: handler.cpp                                   Part of Bylins    *
*  Usage: internal funcs: moving and finding entities/objs                   *
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

#include "handler.h"

#include "auction.h"
#include "backtrace.h"
#include "utils/utils_char_obj.inl"
#include "entities/char_player.h"
#include "entities/world_characters.h"
#include "cmd/follow.h"
#include "exchange.h"
#include "ext_money.h"
#include "fightsystem/fight.h"
#include "fightsystem/pk.h"
#include "house.h"
#include "liquid.h"
#include "magic/magic.h"
#include "game_mechanics/named_stuff.h"
#include "obj_prototypes.h"
#include "color.h"
#include "magic/magic_utils.h"
#include "world_objects.h"
#include "entities/zone.h"
#include "classes/classes_spell_slots.h"
#include "depot.h"

using PlayerClass::slot_for_char;

// local functions //
int apply_ac(CharacterData *ch, int eq_pos);
int apply_armour(CharacterData *ch, int eq_pos);
void update_object(ObjectData *obj, int use);
void update_char_objects(CharacterData *ch);
bool is_wear_light(CharacterData *ch);

// external functions //
void perform_drop_gold(CharacterData *ch, int amount);
int invalid_anti_class(CharacterData *ch, const ObjectData *obj);
int invalid_unique(CharacterData *ch, const ObjectData *obj);
int invalid_no_class(CharacterData *ch, const ObjectData *obj);
void do_entergame(DescriptorData *d);
void do_return(CharacterData *ch, char *argument, int cmd, int subcmd);
extern std::vector<City> cities;
extern int global_uid;
extern void change_leader(CharacterData *ch, CharacterData *vict);
char *find_exdesc(const char *word, const ExtraDescription::shared_ptr &list);
extern void setSkillCooldown(CharacterData *ch, ESkill skill, int cooldownInPulses);

char *fname(const char *namelist) {
	static char holder[30];
	char *point;

	for (point = holder; a_isalpha(*namelist); namelist++, point++)
		*point = *namelist;

	*point = '\0';

	return (holder);
}

bool is_wear_light(CharacterData *ch) {
	bool wear_light = false;
	for (int wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++) {
		if (GET_EQ(ch, wear_pos)
			&& GET_OBJ_TYPE(GET_EQ(ch, wear_pos)) == ObjectData::ITEM_LIGHT
			&& GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2)) {
			wear_light = true;
		}
	}
	return wear_light;
}

void check_light(CharacterData *ch, int was_equip, int was_single, int was_holylight, int was_holydark, int koef) {
	if (ch->in_room == kNowhere) {
		return;
	}

	// In equipment
	if (is_wear_light(ch)) {
		if (was_equip == LIGHT_NO) {
			world[ch->in_room]->light = MAX(0, world[ch->in_room]->light + koef);
		}
	} else {
		if (was_equip == LIGHT_YES)
			world[ch->in_room]->light = MAX(0, world[ch->in_room]->light - koef);
	}

	// Singlelight affect
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT)) {
		if (was_single == LIGHT_NO)
			world[ch->in_room]->light = MAX(0, world[ch->in_room]->light + koef);
	} else {
		if (was_single == LIGHT_YES)
			world[ch->in_room]->light = MAX(0, world[ch->in_room]->light - koef);
	}

	// Holylight affect
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT)) {
		if (was_holylight == LIGHT_NO)
			world[ch->in_room]->glight = MAX(0, world[ch->in_room]->glight + koef);
	} else {
		if (was_holylight == LIGHT_YES)
			world[ch->in_room]->glight = MAX(0, world[ch->in_room]->glight - koef);
	}

	/*if (IS_IMMORTAL(ch))
	{
		sprintf(buf,"holydark was %d\r\n",was_holydark);
		send_to_char(buf,ch);
	}*/

	// Holydark affect
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYDARK))    // if (IS_IMMORTAL(ch))
	{
		/*if (IS_IMMORTAL(ch))
			send_to_char("HOLYDARK ON\r\n",ch);*/
		if (was_holydark == LIGHT_NO)
			world[ch->in_room]->gdark = MAX(0, world[ch->in_room]->gdark + koef);
	} else        // if (IS_IMMORTAL(ch))
	{
		/*if (IS_IMMORTAL(ch))
			send_to_char("HOLYDARK OFF\r\n",ch);*/
		if (was_holydark == LIGHT_YES)
			world[ch->in_room]->gdark = MAX(0, world[ch->in_room]->gdark - koef);
	}

/*	if (GET_REAL_LEVEL(ch) >= kLevelGod)
	{
		sprintf(buf,"Light:%d Glight:%d gdark%d koef:%d\r\n",world[ch->in_room]->light,world[ch->in_room]->glight,world[ch->in_room]->gdark,koef);
		send_to_char(buf,ch);
	}*/
}

/*
 * Return true if a char is affected by a spell (SPELL_XXX),
 * false indicates not affected.
 */
bool affected_by_spell(CharacterData *ch, int type) {
	if (type == kSpellPowerHold) {
		type = kSpellHold;
	} else if (type == kSpellPowerSilence) {
		type = kSpellSllence;
	} else if (type == kSpellPowerBlindness) {
		type = kSpellBlindness;
	}

	for (const auto &affect : ch->affected) {
		if (affect->type == type) {
			return (true);
		}
	}

	return (false);
}

void affect_join_fspell(CharacterData *ch, const Affect<EApplyLocation> &af) {
	bool found = false;
	for (const auto &affect : ch->affected) {
		const bool same_affect = (af.location == APPLY_NONE) && (affect->bitvector == af.bitvector);
		const bool same_type = (af.location != APPLY_NONE) && (affect->type == af.type) && (affect->location == af.location);

		if (same_affect || same_type) {
			if (affect->modifier < af.modifier) {
				affect->modifier = af.modifier;
			}

			if (affect->duration < af.duration) {
				affect->duration = af.duration;
			}

			affect_total(ch);
			found = true;
			break;
		}
	}

	if (!found) {
		affect_to_char(ch, af);
	}
}

void decreaseFeatTimer(CharacterData *ch, int featureID) {
	for (auto *skj = ch->timed_feat; skj; skj = skj->next) {
		if (skj->feat == featureID) {
			if (skj->time >= 1) {
				skj->time--;
			} else {
				ExpireTimedFeat(ch, skj);
			}
			return;
		}
	}
};

void ImposeTimedFeat(CharacterData *ch, TimedFeat *timed) {
	struct TimedFeat *timed_alloc, *skj;

	for (skj = ch->timed_feat; skj; skj = skj->next) {
		if (skj->feat == timed->feat) {
			skj->time = timed->time;
			return;
		}
	}

	CREATE(timed_alloc, 1);

	*timed_alloc = *timed;
	timed_alloc->next = ch->timed_feat;
	ch->timed_feat = timed_alloc;
}

void ExpireTimedFeat(CharacterData *ch, TimedFeat *timed) {
	if (ch->timed_feat == nullptr) {
		log("SYSERR: timed_feat_from_char(%s) when no timed...", GET_NAME(ch));
		return;
	}

	REMOVE_FROM_LIST(timed, ch->timed_feat);
	free(timed);
}

int IsTimed(CharacterData *ch, int feat) {
	struct TimedFeat *hjp;

	for (hjp = ch->timed_feat; hjp; hjp = hjp->next)
		if (hjp->feat == feat)
			return (hjp->time);

	return (0);
}

// Insert an TimedSkill in a char_data structure
void timed_to_char(CharacterData *ch, struct TimedSkill *timed) {
	struct TimedSkill *timed_alloc, *skj;

	// Карачун. Правка бага. Если такой скилл уже есть в списке, просто меняем таймер.
	for (skj = ch->timed; skj; skj = skj->next) {
		if (skj->skill == timed->skill) {
			skj->time = timed->time;
			return;
		}
	}

	CREATE(timed_alloc, 1);

	*timed_alloc = *timed;
	timed_alloc->next = ch->timed;
	ch->timed = timed_alloc;
}

void timed_from_char(CharacterData *ch, struct TimedSkill *timed) {
	if (ch->timed == nullptr) {
		log("SYSERR: timed_from_char(%s) when no timed...", GET_NAME(ch));
		// core_dump();
		return;
	}

	REMOVE_FROM_LIST(timed, ch->timed);
	free(timed);
}

int IsTimedBySkill(CharacterData *ch, ESkill id) {
	struct TimedSkill *hjp;

	for (hjp = ch->timed; hjp; hjp = hjp->next)
		if (hjp->skill == id)
			return (hjp->time);

	return (0);
}

// move a player out of a room
void char_from_room(CharacterData *ch) {
	if (ch == nullptr || ch->in_room == kNowhere) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL character or kNowhere in %s, char_from_room", __FILE__);
		return;
	}

	if (ch->get_fighting() != nullptr)
		stop_fighting(ch, true);

	if (!IS_NPC(ch))
		ch->set_from_room(ch->in_room);

	check_light(ch, LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, -1);

	auto &people = world[ch->in_room]->people;
	people.erase(std::find(people.begin(), people.end(), ch));

	ch->in_room = kNowhere;
	ch->track_dirs = 0;
}

void room_affect_process_on_entry(CharacterData *ch, RoomRnum room) {
	if (IS_IMMORTAL(ch)) {
		return;
	}

	const auto affect_on_room = room_spells::FindAffect(world[room], kSpellHypnoticPattern);
	if (affect_on_room != world[room]->affected.end()) {
		CharacterData *caster = find_char((*affect_on_room)->caster_id);
		// если не в гопе, и не слепой
		if (!same_group(ch, caster)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)){
			// отсекаем всяких непонятных личностей типо двойников и проч. (Кудояр)
			if  ((GET_MOB_VNUM(ch) >= 3000 && GET_MOB_VNUM(ch) < 4000) || GET_MOB_VNUM(ch) == 108 ) return;
			if (ch->has_master()
				&& !IS_NPC(ch->get_master())
				&& IS_NPC(ch)) {
				return;
			}
			// если вошел игрок - ПвП - делаем проверку на шанс в зависимости от % магии кастующего (Кудояр)
			// без магии и ниже 80%: шанс 25%, на 100% - 27%, на 200% - 37% ,при 300% - 47%
			// иначе пве, и просто кастим сон на входящего
			float mkof = func_koef_modif(kSpellHypnoticPattern, caster->get_skill(GetMagicSkillId(
				kSpellHypnoticPattern)));
			if (!IS_NPC(ch) && (number (1, 100) > (23 + 2*mkof))) { 
				return;
			}
			send_to_char("Вы уставились на огненный узор, как баран на новые ворота.", ch);
			act("$n0 уставил$u на огненный узор, как баран на новые ворота.",
				true, ch, nullptr, ch, TO_ROOM | TO_ARENA_LISTEN);
			CallMagic(caster, ch, nullptr, nullptr, kSpellSleep, GET_REAL_LEVEL(caster));
		}
	}
}

// place a character in a room
void char_to_room(CharacterData *ch, RoomRnum room) {
	if (ch == nullptr || room < kNowhere + 1 || room > top_of_world) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p", room, top_of_world, ch);
		return;
	}

	if (!IS_NPC(ch) && !Clan::MayEnter(ch, room, HCE_PORTAL)) {
		room = ch->get_from_room();
	}

	if (!IS_NPC(ch) && NORENTABLE(ch) && ROOM_FLAGGED(room, ROOM_ARENA) && !IS_IMMORTAL(ch)) {
		send_to_char("Вы не можете попасть на арену в состоянии боевых действий!\r\n", ch);
		room = ch->get_from_room();
	}
	world[room]->people.push_front(ch);

	ch->in_room = room;
	check_light(ch, LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, 1);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILHIDE);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILSNEAK);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILCAMOUFLAGE);
	if (PRF_FLAGGED(ch, PRF_CODERINFO)) {
		sprintf(buf,
				"%sКомната=%s%d %sСвет=%s%d %sОсвещ=%s%d %sКостер=%s%d %sЛед=%s%d "
				"%sТьма=%s%d %sСолнце=%s%d %sНебо=%s%d %sЛуна=%s%d%s.\r\n",
				CCNRM(ch, C_NRM), CCINRM(ch, C_NRM), room,
				CCRED(ch, C_NRM), CCIRED(ch, C_NRM), world[room]->light,
				CCGRN(ch, C_NRM), CCIGRN(ch, C_NRM), world[room]->glight,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[room]->fires,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[room]->ices,
				CCBLU(ch, C_NRM), CCIBLU(ch, C_NRM), world[room]->gdark,
				CCMAG(ch, C_NRM), CCICYN(ch, C_NRM), weather_info.sky,
				CCWHT(ch, C_NRM), CCIWHT(ch, C_NRM), weather_info.sunlight,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), weather_info.moon_day, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}
	// Stop fighting now, if we left.
	if (ch->get_fighting() && ch->in_room != IN_ROOM(ch->get_fighting())) {
		stop_fighting(ch->get_fighting(), false);
		stop_fighting(ch, true);
	}

	if (!IS_NPC(ch)) {
		zone_table[world[room]->zone_rn].used = true;
		zone_table[world[room]->zone_rn].activity++;
	} else {
		//sventovit: здесь обрабатываются только неписи, чтобы игрок успел увидеть комнату
		//как сделать красивей я не придумал, т.к. look_at_room вызывается в act.movement а не тут
		room_affect_process_on_entry(ch, ch->in_room);
	}

	// report room changing
	if (ch->desc) {
		if (!(IS_DARK(ch->in_room) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
			ch->desc->msdp_report("ROOM");
	}

	for (unsigned int i = 0; i < cities.size(); i++) {
		if (GET_ROOM_VNUM(room) == cities[i].rent_vnum) {
			ch->mark_city(i);
			break;
		}
	}
}
// place a character in a room
void char_flee_to_room(CharacterData *ch, RoomRnum room) {
	if (ch == nullptr || room < kNowhere + 1 || room > top_of_world) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p", room, top_of_world, ch);
		return;
	}

	if (!IS_NPC(ch) && !Clan::MayEnter(ch, room, HCE_PORTAL)) {
		room = ch->get_from_room();
	}

	if (!IS_NPC(ch) && NORENTABLE(ch) && ROOM_FLAGGED(room, ROOM_ARENA) && !IS_IMMORTAL(ch)) {
		send_to_char("Вы не можете попасть на арену в состоянии боевых действий!\r\n", ch);
		room = ch->get_from_room();
	}
	world[room]->people.push_front(ch);

	ch->in_room = room;
	check_light(ch, LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, 1);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILHIDE);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILSNEAK);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILCAMOUFLAGE);
	if (PRF_FLAGGED(ch, PRF_CODERINFO)) {
		sprintf(buf,
				"%sКомната=%s%d %sСвет=%s%d %sОсвещ=%s%d %sКостер=%s%d %sЛед=%s%d "
				"%sТьма=%s%d %sСолнце=%s%d %sНебо=%s%d %sЛуна=%s%d%s.\r\n",
				CCNRM(ch, C_NRM), CCINRM(ch, C_NRM), room,
				CCRED(ch, C_NRM), CCIRED(ch, C_NRM), world[room]->light,
				CCGRN(ch, C_NRM), CCIGRN(ch, C_NRM), world[room]->glight,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[room]->fires,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[room]->ices,
				CCBLU(ch, C_NRM), CCIBLU(ch, C_NRM), world[room]->gdark,
				CCMAG(ch, C_NRM), CCICYN(ch, C_NRM), weather_info.sky,
				CCWHT(ch, C_NRM), CCIWHT(ch, C_NRM), weather_info.sunlight,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), weather_info.moon_day, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}
	// Stop fighting now, if we left.
	if (ch->get_fighting() && ch->in_room != IN_ROOM(ch->get_fighting())) {
		stop_fighting(ch->get_fighting(), false);
		stop_fighting(ch, true);
	}

	if (!IS_NPC(ch)) {
		zone_table[world[room]->zone_rn].used = true;
		zone_table[world[room]->zone_rn].activity++;
	} else {
		//sventovit: здесь обрабатываются только неписи, чтобы игрок успел увидеть комнату
		//как сделать красивей я не придумал, т.к. look_at_room вызывается в act.movement а не тут
		room_affect_process_on_entry(ch, ch->in_room);
	}

	// небольшой перегиб. когда сбегаешь то ты теряешься в ориентации а тут нате все видно
//	if (ch->desc)
//	{
//		if (!(IS_DARK(ch->in_room) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
//			ch->desc->msdp_report("ROOM");
//	}

	for (unsigned int i = 0; i < cities.size(); i++) {
		if (GET_ROOM_VNUM(room) == cities[i].rent_vnum) {
			ch->mark_city(i);
			break;
		}
	}
}

void restore_object(ObjectData *obj, CharacterData *ch) {
	int i = GET_OBJ_RNUM(obj);
	if (i < 0) {
		return;
	}

	if (GET_OBJ_OWNER(obj)
		&& OBJ_FLAGGED(obj, EExtraFlag::ITEM_NODONATE)
		&& ch
		&& GET_UNIQUE(ch) != GET_OBJ_OWNER(obj)) {
		sprintf(buf, "Зашли в проверку restore_object, Игрок %s, Объект %d", GET_NAME(ch), GET_OBJ_VNUM(obj));
		mudlog(buf, BRF, kLevelImmortal, SYSLOG, true);
	}
}

// выясняет, стокаются ли объекты с метками
bool stockable_custom_labels(ObjectData *obj_one, ObjectData *obj_two) {
	// без меток стокаются
	if (!obj_one->get_custom_label() && !obj_two->get_custom_label())
		return true;

	if (obj_one->get_custom_label() && obj_two->get_custom_label()) {
		// с разными типами меток не стокаются
		if (!obj_one->get_custom_label()->clan != !obj_two->get_custom_label()->clan) {
			return false;
		} else {
			// обе метки клановые один клан, текст совпадает -- стокается
			if (obj_one->get_custom_label()->clan && obj_two->get_custom_label()->clan
				&& !strcmp(obj_one->get_custom_label()->clan, obj_two->get_custom_label()->clan)
				&& obj_one->get_custom_label()->label_text && obj_two->get_custom_label()->label_text
				&& !strcmp(obj_one->get_custom_label()->label_text, obj_two->get_custom_label()->label_text)) {
				return true;
			}

			// обе метки личные, один автор, текст совпадает -- стокается
			if (obj_one->get_custom_label()->author == obj_two->get_custom_label()->author
				&& obj_one->get_custom_label()->label_text && obj_two->get_custom_label()->label_text
				&& !strcmp(obj_one->get_custom_label()->label_text, obj_two->get_custom_label()->label_text)) {
				return true;
			}
		}
	}

	return false;
}

// выяснение стокаются ли предметы
bool equal_obj(ObjectData *obj_one, ObjectData *obj_two) {
	if (GET_OBJ_VNUM(obj_one) != GET_OBJ_VNUM(obj_two)
		|| strcmp(obj_one->get_short_description().c_str(), obj_two->get_short_description().c_str())
		|| (GET_OBJ_TYPE(obj_one) == ObjectData::ITEM_DRINKCON
			&& GET_OBJ_VAL(obj_one, 2) != GET_OBJ_VAL(obj_two, 2))
		|| (GET_OBJ_TYPE(obj_one) == ObjectData::ITEM_CONTAINER
			&& (obj_one->get_contains() || obj_two->get_contains()))
		|| GET_OBJ_VNUM(obj_two) == -1
		|| (GET_OBJ_TYPE(obj_one) == ObjectData::ITEM_BOOK
			&& GET_OBJ_VAL(obj_one, 1) != GET_OBJ_VAL(obj_two, 1))
		|| !stockable_custom_labels(obj_one, obj_two)) {
		return false;
	}

	return true;
}

namespace {

// перемещаем стокающиеся предметы вверх контейнера и сверху кладем obj
void insert_obj_and_group(ObjectData *obj, ObjectData **list_start) {
	// AL: пофиксил Ж)
	// Krodo: пофиксили третий раз, не сортируем у мобов в инве Ж)

	// begin - первый предмет в исходном списке
	// end - последний предмет в перемещаемом интервале
	// before - последний предмет перед началом интервала
	ObjectData *p, *begin, *end, *before;

	obj->set_next_content(begin = *list_start);
	*list_start = obj;

	// похожий предмет уже первый в списке или список пустой
	if (!begin || equal_obj(begin, obj)) {
		return;
	}

	before = p = begin;

	while (p && !equal_obj(p, obj)) {
		before = p;
		p = p->get_next_content();
	}

	// нет похожих предметов
	if (!p) {
		return;
	}

	end = p;

	while (p && equal_obj(p, obj)) {
		end = p;
		p = p->get_next_content();
	}

	end->set_next_content(begin);
	obj->set_next_content(before->get_next_content());
	before->set_next_content(p); // будет 0 если после перемещаемых ничего не лежало
}

} // no-name namespace

// * Инициализация уида для нового объекта.
void set_uid(ObjectData *object) {
	if (GET_OBJ_VNUM(object) > 0 && // Объект не виртуальный
		GET_OBJ_UID(object) == 0)   // У объекта точно нет уида
	{
		global_uid++; // Увеличиваем глобальный счетчик уидов
		global_uid = global_uid == 0 ? 1 : global_uid; // Если произошло переполнение инта
		object->set_uid(global_uid); // Назначаем уид
	}
}

// give an object to a char
void obj_to_char(ObjectData *object, CharacterData *ch) {
	unsigned int tuid;
	int inworld;

	if (!world_objects.get_by_raw_ptr(object)) {
		std::stringstream ss;
		ss << "SYSERR: Object at address 0x" << object
		   << " is not in the world but we have attempt to put it into character '" << ch->get_name()
		   << "'. Object won't be placed into character's inventory.";
		mudlog(ss.str().c_str(), NRM, kLevelImplementator, SYSLOG, true);
		debug::backtrace(runtime_config.logs(ERRLOG).handle());

		return;
	}

	int may_carry = true;
	if (object && ch) {
		restore_object(object, ch);
		if (invalid_anti_class(ch, object) || NamedStuff::check_named(ch, object, false))
			may_carry = false;
		if (!may_carry) {
			act("Вас обожгло при попытке взять $o3.", false, ch, object, nullptr, TO_CHAR);
			act("$n попытал$u взять $o3 - и чудом не сгорел$g.", false, ch, object, nullptr, TO_ROOM);
			obj_to_room(object, ch->in_room);
			return;
		}
		if (!IS_NPC(ch)
			|| (ch->has_master()
				&& !IS_NPC(ch->get_master()))) {
			if (object && GET_OBJ_UID(object) != 0 && object->get_timer() > 0) {
				tuid = GET_OBJ_UID(object);
				inworld = 1;
				// Объект готов для проверки. Ищем в мире такой же.
				world_objects.foreach_with_vnum(GET_OBJ_VNUM(object), [&](const ObjectData::shared_ptr &i) {
					if (GET_OBJ_UID(i) == tuid // UID совпадает
						&& i->get_timer() > 0  // Целенький
						&& object != i.get()) // Не оно же
					{
						inworld++;
					}
				});

				if (inworld > 1) // У объекта есть как минимум одна копия
				{
					sprintf(buf,
							"Copy detected and prepared to extract! Object %s (UID=%u, VNUM=%d), holder %s. In world %d.",
							object->get_PName(0).c_str(),
							GET_OBJ_UID(object),
							GET_OBJ_VNUM(object),
							GET_NAME(ch),
							inworld);
					mudlog(buf, BRF, kLevelImmortal, SYSLOG, true);
					// Удаление предмета
					act("$o0 замигал$Q и вы увидели медленно проступившие руны 'DUPE'.", false, ch, object, nullptr, TO_CHAR);
					object->set_timer(0); // Хана предмету
					object->set_extra_flag(EExtraFlag::ITEM_NOSELL); // Ибо нефиг
				}
			} // Назначаем UID
			else {
				set_uid(object);
				log("%s obj_to_char %s #%d|%u",
					GET_NAME(ch),
					object->get_PName(0).c_str(),
					GET_OBJ_VNUM(object),
					object->get_uid());
			}
		}

		if (!IS_NPC(ch)
			|| (ch->has_master()
				&& !IS_NPC(ch->get_master()))) {
			object->set_extra_flag(EExtraFlag::ITEM_TICKTIMER);    // start timer unconditionally when character picks item up.
			insert_obj_and_group(object, &ch->carrying);
		} else {
			// Вот эта муть, чтобы временно обойти завязку магазинов на порядке предметов в инве моба // Krodo
			object->set_next_content(ch->carrying);
			ch->carrying = object;
		}

		object->set_carried_by(ch);
		object->set_in_room(kNowhere);
		IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
		IS_CARRYING_N(ch)++;

		if (!IS_NPC(ch)) {
			log("obj_to_char: %s -> %d", ch->get_name().c_str(), GET_OBJ_VNUM(object));
		}
		// set flag for crash-save system, but not on mobs!
		if (!IS_NPC(ch)) {
			PLR_FLAGS(ch).set(PLR_CRASH);
		}
	} else
		log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}

// take an object from a char
void obj_from_char(ObjectData *object) {
	if (!object || !object->get_carried_by()) {
		log("SYSERR: NULL object or owner passed to obj_from_char");
		return;
	}
	object->remove_me_from_contains_list(object->get_carried_by()->carrying);

	// set flag for crash-save system, but not on mobs!
	if (!IS_NPC(object->get_carried_by())) {
		PLR_FLAGS(object->get_carried_by()).set(PLR_CRASH);
		log("obj_from_char: %s -> %d", object->get_carried_by()->get_name().c_str(), GET_OBJ_VNUM(object));
	}

	IS_CARRYING_W(object->get_carried_by()) -= GET_OBJ_WEIGHT(object);
	IS_CARRYING_N(object->get_carried_by())--;
	object->set_carried_by(nullptr);
	object->set_next_content(nullptr);
}

int invalid_align(CharacterData *ch, ObjectData *obj) {
	if (IS_NPC(ch) || IS_IMMORTAL(ch))
		return (false);
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MONO)
		&& GET_RELIGION(ch) == kReligionMono) {
		return true;
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_POLY)
		&& GET_RELIGION(ch) == kReligionPoly) {
		return true;
	}
	return false;
}

void wear_message(CharacterData *ch, ObjectData *obj, int position) {
	const char *wear_messages[][2] =
		{
			{"$n засветил$g $o3 и взял$g во вторую руку.",
			 "Вы зажгли $o3 и взяли во вторую руку."},

			{"$n0 надел$g $o3 на правый указательный палец.",
			 "Вы надели $o3 на правый указательный палец."},

			{"$n0 надел$g $o3 на левый указательный палец.",
			 "Вы надели $o3 на левый указательный палец."},

			{"$n0 надел$g $o3 вокруг шеи.",
			 "Вы надели $o3 вокруг шеи."},

			{"$n0 надел$g $o3 на грудь.",
			 "Вы надели $o3 на грудь."},

			{"$n0 надел$g $o3 на туловище.",
			 "Вы надели $o3 на туловище.",},

			{"$n0 водрузил$g $o3 на голову.",
			 "Вы водрузили $o3 себе на голову."},

			{"$n0 надел$g $o3 на ноги.",
			 "Вы надели $o3 на ноги."},

			{"$n0 обул$g $o3.",
			 "Вы обули $o3."},

			{"$n0 надел$g $o3 на кисти.",
			 "Вы надели $o3 на кисти."},

			{"$n0 надел$g $o3 на руки.",
			 "Вы надели $o3 на руки."},

			{"$n0 начал$g использовать $o3 как щит.",
			 "Вы начали использовать $o3 как щит."},

			{"$n0 облачил$u в $o3.",
			 "Вы облачились в $o3."},

			{"$n0 надел$g $o3 вокруг пояса.",
			 "Вы надели $o3 вокруг пояса."},

			{"$n0 надел$g $o3 вокруг правого запястья.",
			 "Вы надели $o3 вокруг правого запястья."},

			{"$n0 надел$g $o3 вокруг левого запястья.",
			 "Вы надели $o3 вокруг левого запястья."},

			{"$n0 взял$g в правую руку $o3.",
			 "Вы вооружились $o4."},

			{"$n0 взял$g $o3 в левую руку.",
			 "Вы взяли $o3 в левую руку."},

			{"$n0 взял$g $o3 в обе руки.",
			 "Вы взяли $o3 в обе руки."},

			{"$n0 начал$g использовать $o3 как колчан.",
			 "Вы начали использовать $o3 как колчан."}
		};

	act(wear_messages[position][1], false, ch, obj, nullptr, TO_CHAR);
	act(wear_messages[position][0],
		IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) ? false : true,
		ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
}

int flag_data_by_char_class(const CharacterData *ch) {
	if (ch == nullptr) {
		return 0;
	}

	return flag_data_by_num(IS_NPC(ch) ? kNumPlayerClasses * kNumKins : GET_CLASS(ch)
		+ kNumPlayerClasses * GET_KIN(ch));
}

unsigned int activate_stuff(CharacterData *ch, ObjectData *obj, id_to_set_info_map::const_iterator it,
							int pos, const CharEquipFlags& equip_flags, unsigned int set_obj_qty) {
	const bool no_cast = equip_flags.test(CharEquipFlag::no_cast);
	const bool show_msg = equip_flags.test(CharEquipFlag::show_msg);
	std::string::size_type delim;

	if (pos < NUM_WEARS) {
		set_info::const_iterator set_obj_info;

		if (GET_EQ(ch, pos) && OBJ_FLAGGED(GET_EQ(ch, pos), EExtraFlag::ITEM_SETSTUFF) &&
			(set_obj_info = it->second.find(GET_OBJ_VNUM(GET_EQ(ch, pos)))) != it->second.end()) {
			unsigned int oqty = activate_stuff(ch, obj, it, pos + 1,
											   (show_msg ? CharEquipFlag::show_msg : CharEquipFlags()) | (no_cast ? CharEquipFlag::no_cast : CharEquipFlags()),
											   set_obj_qty + 1);
			qty_to_camap_map::const_iterator qty_info = set_obj_info->second.upper_bound(oqty);
			qty_to_camap_map::const_iterator old_qty_info = GET_EQ(ch, pos) == obj ?
															set_obj_info->second.begin() :
															set_obj_info->second.upper_bound(oqty - 1);

			while (qty_info != old_qty_info) {
				class_to_act_map::const_iterator class_info;

				qty_info--;
				unique_bit_flag_data item;
				const auto flags = flag_data_by_char_class(ch);
				item.set(flags);
				if ((class_info = qty_info->second.find(item)) != qty_info->second.end()) {
					if (GET_EQ(ch, pos) != obj) {
						for (int i = 0; i < kMaxObjAffect; i++) {
							affect_modify(ch,
										  GET_EQ(ch, pos)->get_affected(i).location,
										  GET_EQ(ch, pos)->get_affected(i).modifier,
										  static_cast<EAffectFlag>(0),
										  false);
						}

						if (ch->in_room != kNowhere) {
							for (const auto &i : weapon_affect) {
								if (i.aff_bitvector == 0
									|| !IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos)) {
									continue;
								}
								affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), false);
							}
						}
					}

					std::string act_msg = GET_EQ(ch, pos)->activate_obj(class_info->second);
					delim = act_msg.find('\n');

					if (show_msg) {
						act(act_msg.substr(0, delim).c_str(), false, ch, GET_EQ(ch, pos), nullptr, TO_CHAR);
						act(act_msg.erase(0, delim + 1).c_str(),
							IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) ? false : true,
							ch, GET_EQ(ch, pos), nullptr, TO_ROOM);
					}

					for (int i = 0; i < kMaxObjAffect; i++) {
						affect_modify(ch, GET_EQ(ch, pos)->get_affected(i).location,
									  GET_EQ(ch, pos)->get_affected(i).modifier, static_cast<EAffectFlag>(0), true);
					}

					if (ch->in_room != kNowhere) {
						for (const auto &i : weapon_affect) {
							if (i.aff_spell == 0 || !IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos)) {
								continue;
							}
							if (!no_cast) {
								if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
									act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
										false, ch, GET_EQ(ch, pos), nullptr, TO_ROOM);
									act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
										false, ch, GET_EQ(ch, pos), nullptr, TO_CHAR);
								} else {
									mag_affects(GET_REAL_LEVEL(ch), ch, ch, i.aff_spell, ESaving::kWill);
								}
							}
						}
					}

					return oqty;
				}
			}

			if (GET_EQ(ch, pos) == obj) {
				for (int i = 0; i < kMaxObjAffect; i++) {
					affect_modify(ch,
								  obj->get_affected(i).location,
								  obj->get_affected(i).modifier,
								  static_cast<EAffectFlag>(0),
								  true);
				}

				if (ch->in_room != kNowhere) {
					for (const auto &i : weapon_affect) {
						if (i.aff_spell == 0
							|| !IS_OBJ_AFF(obj, i.aff_pos)) {
							continue;
						}
						if (!no_cast) {
							if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
								act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
									false, ch, obj, nullptr, TO_ROOM);
								act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
									false, ch, obj, nullptr, TO_CHAR);
							} else {
								mag_affects(GET_REAL_LEVEL(ch), ch, ch, i.aff_spell, ESaving::kWill);
							}
						}
					}
				}
			}

			return oqty;
		} else
			return activate_stuff(ch, obj, it, pos + 1,
								  (show_msg ? CharEquipFlag::show_msg : CharEquipFlags()) | (no_cast ? CharEquipFlag::no_cast : CharEquipFlags()),
								  set_obj_qty);
	} else
		return set_obj_qty;
}

bool check_armor_type(CharacterData *ch, ObjectData *obj) {
	if (GET_OBJ_TYPE(obj) == ObjectData::ITEM_ARMOR_LIGHT
		&& !can_use_feat(ch, ARMOR_LIGHT_FEAT)) {
		act("Для использования $o1 требуется способность 'легкие доспехи'.",
			false, ch, obj, nullptr, TO_CHAR);
		return false;
	}

	if (GET_OBJ_TYPE(obj) == ObjectData::ITEM_ARMOR_MEDIAN
		&& !can_use_feat(ch, ARMOR_MEDIAN_FEAT)) {
		act("Для использования $o1 требуется способность 'средние доспехи'.",
			false, ch, obj, nullptr, TO_CHAR);
		return false;
	}

	if (GET_OBJ_TYPE(obj) == ObjectData::ITEM_ARMOR_HEAVY
		&& !can_use_feat(ch, ARMOR_HEAVY_FEAT)) {
		act("Для использования $o1 требуется способность 'тяжелые доспехи'.",
			false, ch, obj, nullptr, TO_CHAR);
		return false;
	}

	return true;
}

void equip_char(CharacterData *ch, ObjectData *obj, int pos, const CharEquipFlags& equip_flags) {
	int was_lgt = AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
		was_hlgt = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
		was_hdrk = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
		was_lamp = false;

	const bool no_cast = equip_flags.test(CharEquipFlag::no_cast);
	const bool skip_total = equip_flags.test(CharEquipFlag::skip_total);
	const bool show_msg = equip_flags.test(CharEquipFlag::show_msg);

	if (pos < 0 || pos >= NUM_WEARS) {
		log("SYSERR: equip_char(%s,%d) in unknown pos...", GET_NAME(ch), pos);
		return;
	}

	if (GET_EQ(ch, pos)) {
		log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch), obj->get_short_description().c_str());
		return;
	}
	//if (obj->carried_by) {
	//	log("SYSERR: EQUIP: %s - Obj is carried_by when equip.", OBJN(obj, ch, 0));
	//	return;
	//}
	if (obj->get_in_room() != kNowhere) {
		log("SYSERR: EQUIP: %s - Obj is in_room when equip.", OBJN(obj, ch, 0));
		return;
	}

	if (invalid_anti_class(ch, obj) || invalid_unique(ch, obj)) {
		act("Вас обожгло при попытке использовать $o3.", false, ch, obj, nullptr, TO_CHAR);
		act("$n попытал$u использовать $o3 - и чудом не обгорел$g.", false, ch, obj, nullptr, TO_ROOM);
		if (obj->get_carried_by()) {
			obj_from_char(obj);
		}
		obj_to_room(obj, ch->in_room);
		obj_decay(obj);
		return;
	} else if ((!IS_NPC(ch) || IS_CHARMICE(ch)) && OBJ_FLAGGED(obj, EExtraFlag::ITEM_NAMED)
		&& NamedStuff::check_named(ch, obj, true)) {
		if (!NamedStuff::wear_msg(ch, obj))
			send_to_char("Просьба не трогать! Частная собственность!\r\n", ch);
		if (!obj->get_carried_by()) {
			obj_to_char(obj, ch);
		}
		return;
	}

	if ((!IS_NPC(ch) && invalid_align(ch, obj))
		|| invalid_no_class(ch, obj)
		|| (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			&& (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SHARPEN)
				|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_ARMORED)))) {
		act("$o0 явно не предназначен$A для вас.", false, ch, obj, nullptr, TO_CHAR);
		act("$n попытал$u использовать $o3, но у н$s ничего не получилось.", false, ch, obj, nullptr, TO_ROOM);
		if (!obj->get_carried_by()) {
			obj_to_char(obj, ch);
		}
		return;
	}

	if (!IS_NPC(ch) || IS_CHARMICE(ch)) {
		CharacterData *master = IS_CHARMICE(ch) && ch->has_master() ? ch->get_master() : ch;
		if ((obj->get_auto_mort_req() >= 0) && (obj->get_auto_mort_req() > GET_REAL_REMORT(master))
			&& !IS_IMMORTAL(master)) {
			send_to_char(master, "Для использования %s требуется %d %s.\r\n",
						 GET_OBJ_PNAME(obj, 1).c_str(),
						 obj->get_auto_mort_req(),
						 desc_count(obj->get_auto_mort_req(), WHAT_REMORT));
			act("$n попытал$u использовать $o3, но у н$s ничего не получилось.", false, ch, obj, nullptr, TO_ROOM);
			if (!obj->get_carried_by()) {
				obj_to_char(obj, ch);
			}
			return;
		} else if ((obj->get_auto_mort_req() < -1) && (abs(obj->get_auto_mort_req()) < GET_REAL_REMORT(master))
			&& !IS_IMMORTAL(master)) {
			send_to_char(master, "Максимально количество перевоплощений для использования %s равно %d.\r\n",
						 GET_OBJ_PNAME(obj, 1).c_str(),
						 abs(obj->get_auto_mort_req()));
			act("$n попытал$u использовать $o3, но у н$s ничего не получилось.",
				false, ch, obj, nullptr, TO_ROOM);
			if (!obj->get_carried_by()) {
				obj_to_char(obj, ch);
			}
			return;
		}

	}

	if (obj->get_carried_by()) {
		obj_from_char(obj);
	}

	was_lamp = is_wear_light(ch);
	GET_EQ(ch, pos) = obj;
	obj->set_worn_by(ch);
	obj->set_worn_on(pos);
	obj->set_next_content(nullptr);
	CHECK_AGRO(ch) = true;

	if (show_msg) {
		wear_message(ch, obj, pos);
		if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_NAMED)) {
			NamedStuff::wear_msg(ch, obj);
		}
	}

	if (ch->in_room == kNowhere) {
		log("SYSERR: ch->in_room = kNowhere when equipping char %s.", GET_NAME(ch));
	}

	auto it = ObjectData::set_table.begin();
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF)) {
		for (; it != ObjectData::set_table.end(); it++) {
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end()) {
				activate_stuff(ch, obj, it, 0,
							   (show_msg ? CharEquipFlag::show_msg : CharEquipFlags()) | (no_cast ? CharEquipFlag::no_cast : CharEquipFlags()), 0);
				break;
			}
		}
	}

	if (!OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF) || it == ObjectData::set_table.end()) {
		for (int j = 0; j < kMaxObjAffect; j++) {
			affect_modify(ch,
						  obj->get_affected(j).location,
						  obj->get_affected(j).modifier,
						  static_cast<EAffectFlag>(0),
						  true);
		}

		if (ch->in_room != kNowhere) {
			for (const auto &j : weapon_affect) {
				if (j.aff_spell == 0
					|| !IS_OBJ_AFF(obj, j.aff_pos)) {
					continue;
				}

				if (!no_cast) {
					if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
						act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
							false, ch, obj, nullptr, TO_ROOM);
						act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
							false, ch, obj, nullptr, TO_CHAR);
					} else {
						mag_affects(GET_REAL_LEVEL(ch), ch, ch, j.aff_spell, ESaving::kWill);
					}
				}
			}
		}
	}

	if (!skip_total) {
		if (obj_sets::is_set_item(obj)) {
			ch->obj_bonus().update(ch);
		}
		affect_total(ch);
		check_light(ch, was_lamp, was_lgt, was_hlgt, was_hdrk, 1);
	}

	// Раз показываем сообщение, значит, предмет надевает сам персонаж
	// А вообще эта порнография из-за того, что одна функция используется с кучей флагов в разных вариантах
	if (show_msg && ch->get_fighting() && (GET_OBJ_TYPE(obj) == ObjectData::ITEM_WEAPON || pos == WEAR_SHIELD)) {
		setSkillCooldown(ch, ESkill::kGlobalCooldown, 2);
	}
}

unsigned int deactivate_stuff(CharacterData *ch, ObjectData *obj, id_to_set_info_map::const_iterator it,
							  int pos, const CharEquipFlags& equip_flags, unsigned int set_obj_qty) {
	const bool show_msg = equip_flags.test(CharEquipFlag::show_msg);
	std::string::size_type delim;

	if (pos < NUM_WEARS) {
		set_info::const_iterator set_obj_info;

		if (GET_EQ(ch, pos)
			&& OBJ_FLAGGED(GET_EQ(ch, pos), EExtraFlag::ITEM_SETSTUFF)
			&& (set_obj_info = it->second.find(GET_OBJ_VNUM(GET_EQ(ch, pos)))) != it->second.end()) {
			unsigned int oqty = deactivate_stuff(ch, obj, it, pos + 1, (show_msg ? CharEquipFlag::show_msg : CharEquipFlags()),
												 set_obj_qty + 1);
			qty_to_camap_map::const_iterator old_qty_info = set_obj_info->second.upper_bound(oqty);
			qty_to_camap_map::const_iterator qty_info = GET_EQ(ch, pos) == obj ?
														set_obj_info->second.begin() :
														set_obj_info->second.upper_bound(oqty - 1);

			while (old_qty_info != qty_info) {
				old_qty_info--;
				unique_bit_flag_data flags1;
				flags1.set(flag_data_by_char_class(ch));
				class_to_act_map::const_iterator class_info = old_qty_info->second.find(flags1);
				if (class_info != old_qty_info->second.end()) {
					while (qty_info != set_obj_info->second.begin()) {
						qty_info--;
						unique_bit_flag_data flags2;
						flags2.set(flag_data_by_char_class(ch));
						class_to_act_map::const_iterator class_info2 = qty_info->second.find(flags2);
						if (class_info2 != qty_info->second.end()) {
							for (int i = 0; i < kMaxObjAffect; i++) {
								affect_modify(ch,
											  GET_EQ(ch, pos)->get_affected(i).location,
											  GET_EQ(ch, pos)->get_affected(i).modifier,
											  static_cast<EAffectFlag>(0),
											  false);
							}

							if (ch->in_room != kNowhere) {
								for (const auto &i : weapon_affect) {
									if (i.aff_bitvector == 0
										|| !IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos)) {
										continue;
									}
									affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), false);
								}
							}

							std::string act_msg = GET_EQ(ch, pos)->activate_obj(class_info2->second);
							delim = act_msg.find('\n');

							if (show_msg) {
								act(act_msg.substr(0, delim).c_str(), false, ch, GET_EQ(ch, pos), nullptr, TO_CHAR);
								act(act_msg.erase(0, delim + 1).c_str(),
									IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) ? false : true,
									ch, GET_EQ(ch, pos), nullptr, TO_ROOM);
							}

							for (int i = 0; i < kMaxObjAffect; i++) {
								affect_modify(ch,
											  GET_EQ(ch, pos)->get_affected(i).location,
											  GET_EQ(ch, pos)->get_affected(i).modifier,
											  static_cast<EAffectFlag>(0),
											  true);
							}

							if (ch->in_room != kNowhere) {
								for (const auto &i : weapon_affect) {
									if (i.aff_bitvector == 0
										|| !IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos)) {
										continue;
									}
									affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), true);
								}
							}

							return oqty;
						}
					}

					for (int i = 0; i < kMaxObjAffect; i++) {
						affect_modify(ch, GET_EQ(ch, pos)->get_affected(i).location,
									  GET_EQ(ch, pos)->get_affected(i).modifier, static_cast<EAffectFlag>(0), false);
					}

					if (ch->in_room != kNowhere) {
						for (const auto &i : weapon_affect) {
							if (i.aff_bitvector == 0
								|| !IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos)) {
								continue;
							}
							affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), false);
						}
					}

					std::string deact_msg = GET_EQ(ch, pos)->deactivate_obj(class_info->second);
					delim = deact_msg.find('\n');

					if (show_msg) {
						act(deact_msg.substr(0, delim).c_str(), false, ch, GET_EQ(ch, pos), nullptr, TO_CHAR);
						act(deact_msg.erase(0, delim + 1).c_str(),
							IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) ? false : true,
							ch, GET_EQ(ch, pos), nullptr, TO_ROOM);
					}

					if (GET_EQ(ch, pos) != obj) {
						for (int i = 0; i < kMaxObjAffect; i++) {
							affect_modify(ch,
										  GET_EQ(ch, pos)->get_affected(i).location,
										  GET_EQ(ch, pos)->get_affected(i).modifier,
										  static_cast<EAffectFlag>(0),
										  true);
						}

						if (ch->in_room != kNowhere) {
							for (const auto &i : weapon_affect) {
								if (i.aff_bitvector == 0 ||
									!IS_OBJ_AFF(GET_EQ(ch, pos), i.aff_pos)) {
									continue;
								}
								affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), true);
							}
						}
					}

					return oqty;
				}
			}

			if (GET_EQ(ch, pos) == obj) {
				for (int i = 0; i < kMaxObjAffect; i++) {
					affect_modify(ch,
								  obj->get_affected(i).location,
								  obj->get_affected(i).modifier,
								  static_cast<EAffectFlag>(0),
								  false);
				}

				if (ch->in_room != kNowhere) {
					for (const auto &i : weapon_affect) {
						if (i.aff_bitvector == 0
							|| !IS_OBJ_AFF(obj, i.aff_pos)) {
							continue;
						}
						affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(i.aff_bitvector), false);
					}
				}

				obj->deactivate_obj(activation());
			}

			return oqty;
		} else {
			return deactivate_stuff(ch, obj, it, pos + 1, (show_msg ? CharEquipFlag::show_msg : CharEquipFlags()), set_obj_qty);
		}
	} else {
		return set_obj_qty;
	}
}

ObjectData *unequip_char(CharacterData *ch, int pos, const CharEquipFlags& equip_flags) {
	int was_lgt = AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
		was_hlgt = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
		was_hdrk = AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO, was_lamp = false;

	const bool skip_total = equip_flags.test(CharEquipFlag::skip_total);
	const bool show_msg = equip_flags.test(CharEquipFlag::show_msg);

	if (pos < 0 || pos >= NUM_WEARS) {
		log("SYSERR: unequip_char(%s,%d) - unused pos...", GET_NAME(ch), pos);
		return nullptr;
	}

	ObjectData *obj = GET_EQ(ch, pos);
	if (nullptr == obj) {
		log("SYSERR: unequip_char(%s,%d) - no equip...", GET_NAME(ch), pos);
		return nullptr;
	}

	was_lamp = is_wear_light(ch);

	if (ch->in_room == kNowhere)
		log("SYSERR: ch->in_room = kNowhere when unequipping char %s.", GET_NAME(ch));

	auto it = ObjectData::set_table.begin();
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF))
		for (; it != ObjectData::set_table.end(); it++)
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end()) {
				deactivate_stuff(ch, obj, it, 0, (show_msg ? CharEquipFlag::show_msg : CharEquipFlags()), 0);
				break;
			}

	if (!OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF) || it == ObjectData::set_table.end()) {
		for (int j = 0; j < kMaxObjAffect; j++) {
			affect_modify(ch,
						  obj->get_affected(j).location,
						  obj->get_affected(j).modifier,
						  static_cast<EAffectFlag>(0),
						  false);
		}

		if (ch->in_room != kNowhere) {
			for (const auto &j : weapon_affect) {
				if (j.aff_bitvector == 0 || !IS_OBJ_AFF(obj, j.aff_pos)) {
					continue;
				}
				if (IS_NPC(ch)
					&& AFF_FLAGGED(&mob_proto[GET_MOB_RNUM(ch)], static_cast<EAffectFlag>(j.aff_bitvector))) {
					continue;
				}
				affect_modify(ch, APPLY_NONE, 0, static_cast<EAffectFlag>(j.aff_bitvector), false);
			}
		}

		if ((OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF)) && (SetSystem::is_big_set(obj)))
			obj->deactivate_obj(activation());
	}

	GET_EQ(ch, pos) = nullptr;
	obj->set_worn_by(nullptr);
	obj->set_worn_on(kNowhere);
	obj->set_next_content(nullptr);

	if (!skip_total) {
		if (obj_sets::is_set_item(obj)) {
			if (obj->get_activator().first) {
				obj_sets::print_off_msg(ch, obj);
			}
			ch->obj_bonus().update(ch);
		}
		obj->set_activator(false, 0);
		obj->remove_set_bonus();

		affect_total(ch);
		check_light(ch, was_lamp, was_lgt, was_hlgt, was_hdrk, 1);
	}

	return (obj);
}

int get_number(char **name) {
	int i, res;
	char *ppos;
	char tmpname[kMaxInputLength];

	if ((ppos = strchr(*name, '.')) != nullptr) {
		for (i = 0; *name + i != ppos; i++) {
			if (!a_isdigit(*(*name + i))) {
				return 1;
			}
		}
		*ppos = '\0';
		res = atoi(*name);
		strl_cpy(tmpname, ppos + 1, kMaxInputLength);
		strl_cpy(*name, tmpname, kMaxInputLength);
		return res;
	}

	return 1;
}

int get_number(std::string &name) {
	std::string::size_type pos = name.find('.');

	if (pos != std::string::npos) {
		for (std::string::size_type i = 0; i != pos; i++)
			if (!a_isdigit(name[i]))
				return (1);
		int res = atoi(name.substr(0, pos).c_str());
		name.erase(0, pos + 1);
		return (res);
	}
	return (1);
}

// Search a given list for an object number, and return a ptr to that obj //
ObjectData *get_obj_in_list_num(int num, ObjectData *list) {
	ObjectData *i;

	for (i = list; i; i = i->get_next_content()) {
		if (GET_OBJ_RNUM(i) == num) {
			return (i);
		}
	}

	return (nullptr);
}

// Search a given list for an object virtul_number, and return a ptr to that obj //
ObjectData *get_obj_in_list_vnum(int num, ObjectData *list) {
	ObjectData *i;

	for (i = list; i; i = i->get_next_content()) {
		if (GET_OBJ_VNUM(i) == num) {
			return (i);
		}
	}

	return (nullptr);
}

// search the entire world for an object number, and return a pointer  //
ObjectData *get_obj_num(ObjRnum nr) {
	const auto result = world_objects.find_first_by_rnum(nr);
	return result.get();
}

// search a room for a char, and return a pointer if found..  //
CharacterData *get_char_room(char *name, RoomRnum room) {
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	strcpy(tmp, name);
	const int number = get_number(&tmp);
	if (0 == number) {
		return nullptr;
	}

	int j = 0;
	for (const auto i : world[room]->people) {
		if (isname(tmp, i->get_pc_name())) {
			if (++j == number) {
				return i;
			}
		}
	}

	return nullptr;
}

// search all over the world for a char num, and return a pointer if found //
CharacterData *get_char_num(MobRnum nr) {
	for (const auto &i : character_list) {
		if (GET_MOB_RNUM(i) == nr) {
			return i.get();
		}
	}

	return nullptr;
}

const int money_destroy_timer = 60;
const int death_destroy_timer = 5;
const int room_destroy_timer = 10;
const int room_nodestroy_timer = -1;
const int script_destroy_timer = 10; // * !!! Never set less than ONE * //

/**
* put an object in a room
* Ахтунг, не надо тут экстрактить шмотку, если очень хочется - проверяйте и правьте 50 вызовов
* по коду, т.к. нигде оно нифига не проверяется на валидность после этой функции.
* \return 0 - невалидный объект или комната, 1 - все ок
*/
bool obj_to_room(ObjectData *object, RoomRnum room) {
//	int sect = 0;
	if (!object || room < FIRST_ROOM || room > top_of_world) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
			room, top_of_world, object);
		return false;
	} else {
		restore_object(object, nullptr);
		insert_obj_and_group(object, &world[room]->contents);
		object->set_in_room(room);
		object->set_carried_by(nullptr);
		object->set_worn_by(nullptr);
		if (ROOM_FLAGGED(room, ROOM_NOITEM)) {
			object->set_extra_flag(EExtraFlag::ITEM_DECAY);
		}

		if (object->get_script()->has_triggers()) {
			object->set_destroyer(script_destroy_timer);
		} else if (OBJ_FLAGGED(object, EExtraFlag::ITEM_NODECAY)) {
			object->set_destroyer(room_nodestroy_timer);
		} else if (GET_OBJ_TYPE(object) == ObjectData::ITEM_MONEY) {
			object->set_destroyer(money_destroy_timer);
		} else if (ROOM_FLAGGED(room, ROOM_DEATH)) {
			object->set_destroyer(death_destroy_timer);
		} else {
			object->set_destroyer(room_destroy_timer);
		}
	}
	return 1;
}

/* Функция для удаления обьектов после лоада в комнату
   результат работы - 1 если посыпался, 0 - если остался */
int obj_decay(ObjectData *object) {
	int room, sect;
	room = object->get_in_room();

	if (room == kNowhere)
		return (0);

	sect = real_sector(room);

	if (((sect == kSectWaterSwim || sect == kSectWaterNoswim) &&
		!OBJ_FLAGGED(object, EExtraFlag::ITEM_SWIMMING) &&
		!OBJ_FLAGGED(object, EExtraFlag::ITEM_FLYING) &&
		!IS_CORPSE(object))) {

		act("$o0 медленно утонул$G.", false, world[room]->first_character(), object, nullptr, TO_ROOM);
		act("$o0 медленно утонул$G.", false, world[room]->first_character(), object, nullptr, TO_CHAR);
		extract_obj(object);
		return (1);
	}

	if (((sect == kSectOnlyFlying) && !IS_CORPSE(object) && !OBJ_FLAGGED(object, EExtraFlag::ITEM_FLYING))) {

		act("$o0 упал$G вниз.", false, world[room]->first_character(), object, nullptr, TO_ROOM);
		act("$o0 упал$G вниз.", false, world[room]->first_character(), object, nullptr, TO_CHAR);
		extract_obj(object);
		return (1);
	}

	if (OBJ_FLAGGED(object, EExtraFlag::ITEM_DECAY) ||
		(OBJ_FLAGGED(object, EExtraFlag::ITEM_ZONEDECAY) && GET_OBJ_VNUM_ZONE_FROM(object) != zone_table[world[room]->zone_rn].vnum)) {
		act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер.", false,
			world[room]->first_character(), object, nullptr, TO_ROOM);
		act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер.", false,
			world[room]->first_character(), object, nullptr, TO_CHAR);
		extract_obj(object);
		return (1);
	}

	return (0);
}

// Take an object from a room
void obj_from_room(ObjectData *object) {
	if (!object || object->get_in_room() == kNowhere) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
			object, object->get_in_room());
		return;
	}

	object->remove_me_from_contains_list(world[object->get_in_room()]->contents);

	object->set_in_room(kNowhere);
	object->set_next_content(nullptr);
}

// put an object in an object (quaint)
void obj_to_obj(ObjectData *obj, ObjectData *obj_to) {
	ObjectData *tmp_obj;

	if (!obj || !obj_to || obj == obj_to) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.",
			obj, obj, obj_to);
		return;
	}

	auto list = obj_to->get_contains();
	insert_obj_and_group(obj, &list);
	obj_to->set_contains(list);
	obj->set_in_obj(obj_to);

	for (tmp_obj = obj->get_in_obj(); tmp_obj->get_in_obj(); tmp_obj = tmp_obj->get_in_obj()) {
		tmp_obj->add_weight(GET_OBJ_WEIGHT(obj));
	}

	// top level object.  Subtract weight from inventory if necessary.
	tmp_obj->add_weight(GET_OBJ_WEIGHT(obj));
	if (tmp_obj->get_carried_by()) {
		IS_CARRYING_W(tmp_obj->get_carried_by()) += GET_OBJ_WEIGHT(obj);
	}
}

// remove an object from an object
void obj_from_obj(ObjectData *obj) {
	if (obj->get_in_obj() == nullptr) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
		return;
	}
	auto obj_from = obj->get_in_obj();
	auto head = obj_from->get_contains();
	obj->remove_me_from_contains_list(head);
	obj_from->set_contains(head);

	// Subtract weight from containers container
	auto temp = obj->get_in_obj();
	for (; temp->get_in_obj(); temp = temp->get_in_obj()) {
		temp->set_weight(MAX(1, GET_OBJ_WEIGHT(temp) - GET_OBJ_WEIGHT(obj)));
	}

	// Subtract weight from char that carries the object
	temp->set_weight(MAX(1, GET_OBJ_WEIGHT(temp) - GET_OBJ_WEIGHT(obj)));
	if (temp->get_carried_by()) {
		IS_CARRYING_W(temp->get_carried_by()) = MAX(1, IS_CARRYING_W(temp->get_carried_by()) - GET_OBJ_WEIGHT(obj));
	}

	obj->set_in_obj(nullptr);
	obj->set_next_content(nullptr);
}

// Set all carried_by to point to new owner
void object_list_new_owner(ObjectData *list, CharacterData *ch) {
	if (list) {
		object_list_new_owner(list->get_contains(), ch);
		object_list_new_owner(list->get_next_content(), ch);
		list->set_carried_by(ch);
	}
}

RoomVnum get_room_where_obj(ObjectData *obj, bool deep) {
	if (GET_ROOM_VNUM(obj->get_in_room()) != kNowhere) {
		return GET_ROOM_VNUM(obj->get_in_room());
	} else if (obj->get_in_obj() && !deep) {
		return get_room_where_obj(obj->get_in_obj(), true);
	} else if (obj->get_carried_by()) {
		return GET_ROOM_VNUM(IN_ROOM(obj->get_carried_by()));
	} else if (obj->get_worn_by()) {
		return GET_ROOM_VNUM(IN_ROOM(obj->get_worn_by()));
	}

	return kNowhere;
}

// Extract an object from the world
void extract_obj(ObjectData *obj) {
	char name[kMaxStringLength];
	ObjectData *temp;

	strcpy(name, obj->get_PName(0).c_str());
	log("Extracting obj %s vnum == %d room = %d timer == %d",
		name,
		GET_OBJ_VNUM(obj),
		get_room_where_obj(obj, false),
		obj->get_timer());
// TODO: в дебаг log("Start extract obj %s", name);

	// Get rid of the contents of the object, as well.
	// Обработка содержимого контейнера при его уничтожении

	purge_otrigger(obj);

	while (obj->get_contains()) {
		temp = obj->get_contains();
		obj_from_obj(temp);

		if (obj->get_carried_by()) {
			if (IS_NPC(obj->get_carried_by())
				|| (IS_CARRYING_N(obj->get_carried_by()) >= CAN_CARRY_N(obj->get_carried_by()))) {
				obj_to_room(temp, IN_ROOM(obj->get_carried_by()));
				obj_decay(temp);
			} else {
				obj_to_char(temp, obj->get_carried_by());
			}
		} else if (obj->get_worn_by() != nullptr) {
			if (IS_NPC(obj->get_worn_by())
				|| (IS_CARRYING_N(obj->get_worn_by()) >= CAN_CARRY_N(obj->get_worn_by()))) {
				obj_to_room(temp, IN_ROOM(obj->get_worn_by()));
				obj_decay(temp);
			} else {
				obj_to_char(temp, obj->get_worn_by());
			}
		} else if (obj->get_in_room() != kNowhere) {
			obj_to_room(temp, obj->get_in_room());
			obj_decay(temp);
		} else if (obj->get_in_obj()) {
			extract_obj(temp);
		} else {
			extract_obj(temp);
		}
	}
	// Содержимое контейнера удалено

	if (obj->get_worn_by() != nullptr) {
		if (unequip_char(obj->get_worn_by(), obj->get_worn_on(), CharEquipFlags()) != obj) {
			log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
		}
	}

	if (obj->get_in_room() != kNowhere) {
		obj_from_room(obj);
	} else if (obj->get_carried_by()) {
		obj_from_char(obj);
	} else if (obj->get_in_obj()) {
		obj_from_obj(obj);
	}

	check_auction(nullptr, obj);
	check_exchange(obj);

	const auto rnum = GET_OBJ_RNUM(obj);
	if (rnum >= 0) {
		obj_proto.dec_number(rnum);
	}

	obj->get_script()->set_purged();
	world_objects.remove(obj);
}

void update_object(ObjectData *obj, int use) {
	ObjectData *obj_it = obj;

	while (obj_it) {
		// don't update objects with a timer trigger
		const bool trig_timer = SCRIPT_CHECK(obj_it, OTRIG_TIMER);
		const bool has_timer = obj_it->get_timer() > 0;
		const bool tick_timer = 0 != OBJ_FLAGGED(obj_it, EExtraFlag::ITEM_TICKTIMER);

		if (!trig_timer && has_timer && tick_timer) {
			obj_it->dec_timer(use);
		}

		if (obj_it->get_contains()) {
			update_object(obj_it->get_contains(), use);
		}

		obj_it = obj_it->get_next_content();
	}
}

void update_char_objects(CharacterData *ch) {
//Polud раз уж светит любой источник света, то и гаснуть тоже должны все
	for (int wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++) {
		if (GET_EQ(ch, wear_pos) != nullptr) {
			if (GET_OBJ_TYPE(GET_EQ(ch, wear_pos)) == ObjectData::ITEM_LIGHT) {
				if (GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2) > 0) {
					const int i = GET_EQ(ch, wear_pos)->dec_val(2);
					if (i == 1) {
						act("$z $o замерцал$G и начал$G угасать.\r\n",
							false, ch, GET_EQ(ch, wear_pos), nullptr, TO_CHAR);
						act("$o $n1 замерцал$G и начал$G угасать.",
							false, ch, GET_EQ(ch, wear_pos), nullptr, TO_ROOM);
					} else if (i == 0) {
						act("$z $o погас$Q.\r\n", false, ch, GET_EQ(ch, wear_pos), nullptr, TO_CHAR);
						act("$o $n1 погас$Q.", false, ch, GET_EQ(ch, wear_pos), nullptr, TO_ROOM);
						if (ch->in_room != kNowhere) {
							if (world[ch->in_room]->light > 0)
								world[ch->in_room]->light -= 1;
						}

						if (OBJ_FLAGGED(GET_EQ(ch, wear_pos), EExtraFlag::ITEM_DECAY)) {
							extract_obj(GET_EQ(ch, wear_pos));
						}
					}
				}
			}
		}
	}
	//-Polud

	for (int i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i)) {
			update_object(GET_EQ(ch, i), 1);
		}
	}

	if (ch->carrying) {
		update_object(ch->carrying, 1);
	}
}

/**
* Если на мобе шмотки, одетые во время резета зоны, то при резете в случае пуржа моба - они уничтожаются с ним же.
* Если на мобе шмотки, поднятые и бывшие у игрока (таймер уже тикал), то он их при резете выкинет на землю, как обычно.
* А то при резетах например той же мавки умудрялись лутить шмот с земли, упавший с нее до того, как она сама поднимет,
* плюс этот лоад накапливался и можно было заиметь несколько шмоток сразу с нескольких резетов. -- Krodo
* \param inv - 1 сообщение о выкидывании из инвентаря, 0 - о снятии с себя
* \param zone_reset - 1 - пуржим стаф без включенных таймеров, 0 - не пуржим ничего
*/
void drop_obj_on_zreset(CharacterData *ch, ObjectData *obj, bool inv, bool zone_reset) {
	if (zone_reset && !OBJ_FLAGGED(obj, EExtraFlag::ITEM_TICKTIMER))
		extract_obj(obj);
	else {
		if (inv)
			act("Вы выбросили $o3 на землю.", false, ch, obj, nullptr, TO_CHAR);
		else
			act("Вы сняли $o3 и выбросили на землю.", false, ch, obj, nullptr, TO_CHAR);
		// Если этот моб трупа не оставит, то не выводить сообщение
		// иначе ужасно коряво смотрится в бою и в тригах
		bool msgShown = false;
		if (!IS_NPC(ch) || !MOB_FLAGGED(ch, MOB_CORPSE)) {
			if (inv)
				act("$n бросил$g $o3 на землю.", false, ch, obj, nullptr, TO_ROOM);
			else
				act("$n снял$g $o3 и бросил$g на землю.", false, ch, obj, nullptr, TO_ROOM);
			msgShown = true;
		}

		drop_otrigger(obj, ch);

		drop_wtrigger(obj, ch);

		obj_to_room(obj, ch->in_room);
		if (!obj_decay(obj) && !msgShown) {
			act("На земле остал$U лежать $o.", false, ch, obj, nullptr, TO_ROOM);
		}
	}
}

namespace {

void change_npc_leader(CharacterData *ch) {
	std::vector<CharacterData *> tmp_list;

	for (Follower *i = ch->followers; i; i = i->next) {
		if (IS_NPC(i->ch)
			&& !IS_CHARMICE(i->ch)
			&& i->ch->get_master() == ch) {
			tmp_list.push_back(i->ch);
		}
	}
	if (tmp_list.empty()) {
		return;
	}

	CharacterData *leader = nullptr;
	for (auto i : tmp_list) {
		if (stop_follower(i, SF_SILENCE)) {
			continue;
		}
		if (!leader) {
			leader = i;
		} else {
			leader->add_follower_silently(i);
		}
	}
}

} // namespace

/**
* Extract a ch completely from the world, and leave his stuff behind
* \param zone_reset - 0 обычный пурж когда угодно (по умолчанию), 1 - пурж при резете зоны
*/
void extract_char(CharacterData *ch, int clear_objs, bool zone_reset) {
	if (ch->purged()) {
		log("SYSERROR: double extract_char (%s:%d)", __FILE__, __LINE__);
		return;
	}

	DescriptorData *t_desc;
	int i;

	if (MOB_FLAGGED(ch, MOB_FREE)
		|| MOB_FLAGGED(ch, MOB_DELETE)) {
		return;
	}

	std::string name = GET_NAME(ch);
	log("[Extract char] Start function for char %s VNUM: %d", name.c_str(), GET_MOB_VNUM(ch));
	if (!IS_NPC(ch) && !ch->desc) {
//		log("[Extract char] Extract descriptors");
		for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next) {
			if (t_desc->original.get() == ch) {
				do_return(t_desc->character.get(), nullptr, 0, 0);
			}
		}
	}

	// Forget snooping, if applicable
//	log("[Extract char] Stop snooping");
	if (ch->desc) {
		if (ch->desc->snooping) {
			ch->desc->snooping->snoop_by = nullptr;
			ch->desc->snooping = nullptr;
		}

		if (ch->desc->snoop_by) {
			SEND_TO_Q("Ваша жертва теперь недоступна.\r\n", ch->desc->snoop_by);
			ch->desc->snoop_by->snooping = nullptr;
			ch->desc->snoop_by = nullptr;
		}
	}

	// transfer equipment to room, if any
//	log("[Extract char] Drop equipment");
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i)) {
			ObjectData *obj_eq = unequip_char(ch, i, CharEquipFlags());
			if (!obj_eq) {
				continue;
			}

			remove_otrigger(obj_eq, ch);
			drop_obj_on_zreset(ch, obj_eq, false, zone_reset);
		}
	}

	// transfer objects to room, if any
//	log("[Extract char] Drop objects");
	while (ch->carrying) {
		ObjectData *obj = ch->carrying;
		obj_from_char(obj);
		drop_obj_on_zreset(ch, obj, true, zone_reset);
	}

	if (IS_NPC(ch)) {
		// дроп гривен до изменений последователей за мобом
		ExtMoney::drop_torc(ch);
	}

	if (!IS_NPC(ch)
		&& !ch->has_master()
		&& ch->followers
		&& AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)) {
//		log("[Extract char] Change group leader");
		change_leader(ch, nullptr);
	} else if (IS_NPC(ch)
		&& !IS_CHARMICE(ch)
		&& !ch->has_master()
		&& ch->followers) {
//		log("[Extract char] Changing NPC leader");
		change_npc_leader(ch);
	}

//	log("[Extract char] Die followers");
	if ((ch->followers || ch->has_master())
		&& die_follower(ch)) {
		// TODO: странно все это с пуржем в stop_follower
		return;
	}

//	log("[Extract char] Stop fighting self");
	if (ch->get_fighting()) {
		stop_fighting(ch, true);
	}

//	log("[Extract char] Stop all fight for opponee");
	change_fighting(ch, true);

//	log("[Extract char] Remove char from room");
	char_from_room(ch);

	// pull the char from the list
	MOB_FLAGS(ch).set(MOB_DELETE);

	if (ch->desc && ch->desc->original) {
		do_return(ch, nullptr, 0, 0);
	}

	const bool is_npc = IS_NPC(ch);
	if (!is_npc) {
//		log("[Extract char] All save for PC");
		check_auction(ch, nullptr);
		ch->save_char();
		//удаляются рент-файлы, если только персонаж не ушел в ренту
		Crash_delete_crashfile(ch);
	} else {
//		log("[Extract char] All clear for NPC");
		if ((GET_MOB_RNUM(ch) > -1)
			&& !MOB_FLAGGED(ch, MOB_PLAYER_SUMMON))    // if mobile и не умертвие
		{
			mob_index[GET_MOB_RNUM(ch)].total_online--;
		}
	}

	bool left_in_game = false;
	if (!is_npc
		&& ch->desc != nullptr) {
		STATE(ch->desc) = CON_MENU;
		SEND_TO_Q(MENU, ch->desc);
		if (!IS_NPC(ch) && NORENTABLE(ch) && clear_objs) {
			do_entergame(ch->desc);
			left_in_game = true;
		}
	}

	if (!left_in_game) {
		character_list.remove(ch);
	}

	log("[Extract char] Stop function for char %s ", name.c_str());
}

/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions*
* which incorporate the actual player-data                               *.
*********************************************************************** */

CharacterData *get_player_vis(CharacterData *ch, const char *name, int inroom) {
	for (const auto &i : character_list) {
		if (IS_NPC(i))
			continue;
		if (!HERE(i))
			continue;
		if ((inroom & FIND_CHAR_ROOM) && i->in_room != ch->in_room)
			continue;
		if (!CAN_SEE_CHAR(ch, i))
			continue;
		if (!isname(name, i->get_pc_name())) {
			continue;
		}

		return i.get();
	}

	return nullptr;
}

CharacterData *get_player_pun(CharacterData *ch, const char *name, int inroom) {
	for (const auto &i : character_list) {
		if (IS_NPC(i))
			continue;
		if ((inroom & FIND_CHAR_ROOM) && i->in_room != ch->in_room)
			continue;
		if (!isname(name, i->get_pc_name())) {
			continue;
		}
		return i.get();
	}

	return nullptr;
}

CharacterData *get_char_room_vis(CharacterData *ch, const char *name) {
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;
	// JE 7/18/94 :-) :-)
	if (!str_cmp(name, "self")
		|| !str_cmp(name, "me")
		|| !str_cmp(name, "я")
		|| !str_cmp(name, "меня")
		|| !str_cmp(name, "себя")) {
		return (ch);
	}

	// 0.<name> means PC with name
	strl_cpy(tmp, name, kMaxInputLength);

	const int number = get_number(&tmp);
	if (0 == number) {
		return get_player_vis(ch, tmp, FIND_CHAR_ROOM);
	}

	int j = 0;
	for (const auto i : world[ch->in_room]->people) {
		if (HERE(i) && CAN_SEE(ch, i)
			&& isname(tmp, i->get_pc_name())) {
			if (++j == number) {
				return i;
			}
		}
	}

	return nullptr;
}

CharacterData *get_char_vis(CharacterData *ch, const char *name, int where) {
	CharacterData *i;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	// check the room first
	if (where == FIND_CHAR_ROOM) {
		return get_char_room_vis(ch, name);
	} else if (where == FIND_CHAR_WORLD) {
		if ((i = get_char_room_vis(ch, name)) != nullptr) {
			return (i);
		}

		strcpy(tmp, name);
		const int number = get_number(&tmp);
		if (0 == number) {
			return get_player_vis(ch, tmp, 0);
		}

		int j = 0;
		for (const auto &target : character_list) {
			if (HERE(target) && CAN_SEE(ch, target)
				&& isname(tmp, target->get_pc_name())) {
				if (++j == number) {
					return target.get();
				}
			}
		}
	}

	return nullptr;
}

ObjectData *get_obj_in_list_vis(CharacterData *ch, const char *name, ObjectData *list, bool locate_item) {
	ObjectData *i;
	int j = 0, number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return (nullptr);

	//Запретим локейт 2. 3. n. стафин
	if (number > 1 && locate_item)
		return (nullptr);

	for (i = list; i && (j <= number); i = i->get_next_content()) {
		if (isname(tmp, i->get_aliases())
			|| CHECK_CUSTOM_LABEL(tmp, i, ch)) {
			if (CAN_SEE_OBJ(ch, i)) {
				// sprintf(buf,"Show obj %d %s %x ", number, i->name, i);
				// send_to_char(buf,ch);
				if (!locate_item) {
					if (++j == number)
						return (i);
				} else {
					if (try_locate_obj(ch, i))
						return (i);
					else
						continue;
				}
			}
		}
	}

	return (nullptr);
}

class ExitLoopException : std::exception {};

ObjectData *get_obj_vis_and_dec_num(CharacterData *ch,
									const char *name,
									ObjectData *list,
									std::unordered_set<unsigned int> &id_obj_set,
									int &number) {
	for (auto item = list; item != nullptr; item = item->get_next_content()) {
		if (CAN_SEE_OBJ(ch, item)) {
			if (isname(name, item->get_aliases())
				|| CHECK_CUSTOM_LABEL(name, item, ch)) {
				if (--number == 0) {
					return item;
				}
				id_obj_set.insert(item->get_id());
			}
		}
	}

	return nullptr;
}

ObjectData *get_obj_vis_and_dec_num(CharacterData *ch,
									const char *name,
									ObjectData *equip[],
									std::unordered_set<unsigned int> &id_obj_set,
									int &number) {
	for (auto i = 0; i < NUM_WEARS; ++i) {
		auto item = equip[i];
		if (item && CAN_SEE_OBJ(ch, item)) {
			if (isname(name, item->get_aliases())
				|| CHECK_CUSTOM_LABEL(name, item, ch)) {
				if (--number == 0) {
					return item;
				}
				id_obj_set.insert(item->get_id());
			}
		}
	}

	return nullptr;
}

// search the entire world for an object, and return a pointer
ObjectData *get_obj_vis(CharacterData *ch, const char *name) {
	int number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	strcpy(tmp, name);
	number = get_number(&tmp);
	if (number < 1) {
		return nullptr;
	}

	auto id_obj_set = std::unordered_set<unsigned int>();

	//Scan in equipment
	auto obj = get_obj_vis_and_dec_num(ch, tmp, ch->equipment, id_obj_set, number);
	if (obj) {
		return obj;
	}

	//Scan in carried items
	obj = get_obj_vis_and_dec_num(ch, tmp, ch->carrying, id_obj_set, number);
	if (obj) {
		return obj;
	}

	//Scan in room
	obj = get_obj_vis_and_dec_num(ch, tmp, world[ch->in_room]->contents, id_obj_set, number);
	if (obj) {
		return obj;
	}

	//Scan charater's in room
	for (const auto &vict : world[ch->in_room]->people) {
		if (ch->get_uid() == vict->get_uid()) {
			continue;
		}

		//Scan in equipment
		obj = get_obj_vis_and_dec_num(ch, tmp, vict->equipment, id_obj_set, number);
		if (obj) {
			return obj;
		}

		//Scan in carried items
		obj = get_obj_vis_and_dec_num(ch, tmp, vict->carrying, id_obj_set, number);
		if (obj) {
			return obj;
		}
	}

	// ok.. no luck yet. scan the entire obj list except already found
	const WorldObjects::predicate_f predicate = [&](const ObjectData::shared_ptr &i) -> bool {
		const auto result = CAN_SEE_OBJ(ch, i.get())
			&& (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i.get(), ch))
			&& (id_obj_set.count(i.get()->get_id()) == 0);
		return result;
	};
	// не совсем понял зачем вычитать единицу
	obj = world_objects.find_if(predicate, number - 1).get();
		if (obj) {
		return obj;
	}
	obj = Depot::find_obj_from_depot_and_dec_number(tmp, number);
		if (obj) {
		return obj;
	}
	return nullptr;
}

// search the entire world for an object, and return a pointer
ObjectData *get_obj_vis_for_locate(CharacterData *ch, const char *name) {
	ObjectData *i;
	int number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	// scan items carried //
	if ((i = get_obj_in_list_vis(ch, name, ch->carrying)) != nullptr) {
		return i;
	}

	// scan room //
	if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room]->contents)) != nullptr) {
		return i;
	}

	strcpy(tmp, name);
	number = get_number(&tmp);
	if (number != 1) {
		return nullptr;
	}

	// ok.. no luck yet. scan the entire obj list   //
	const WorldObjects::predicate_f locate_predicate = [&](const ObjectData::shared_ptr &i) -> bool {
		const auto result = CAN_SEE_OBJ(ch, i.get())
			&& (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i.get(), ch))
			&& try_locate_obj(ch, i.get());
		return result;
	};

	return world_objects.find_if(locate_predicate).get();
}

bool try_locate_obj(CharacterData *ch, ObjectData *i) {
	if (IS_CORPSE(i) || IS_GOD(ch)) //имм может локейтить и можно локейтить трупы
	{
		return true;
	} else if (OBJ_FLAGGED(i,
						   EExtraFlag::ITEM_NOLOCATE)) //если флаг !локейт и ее нет в комнате/инвентаре - пропустим ее
	{
		return false;
	} else if (i->get_carried_by() && IS_NPC(i->get_carried_by())) {
		if (world[IN_ROOM(i->get_carried_by())]->zone_rn
			== world[ch->in_room]->zone_rn) //шмотки у моба можно локейтить только в одной зоне
		{
			return true;
		} else {
			return false;
		}
	} else if (i->get_in_room() != kNowhere && i->get_in_room()) {
		if (world[i->get_in_room()]->zone_rn
			== world[ch->in_room]->zone_rn) //шмотки в клетке можно локейтить только в одной зоне
		{
			return true;
		} else {
			return false;
		}
	} else if (i->get_worn_by() && IS_NPC(i->get_worn_by())) {
		if (world[IN_ROOM(i->get_worn_by())]->zone_rn == world[ch->in_room]->zone_rn) {
			return true;
		} else {
			return false;
		}
	} else if (i->get_in_obj()) {
		if (Clan::is_clan_chest(i->get_in_obj())) {
			return true;
		} else {
			const auto in_obj = i->get_in_obj();
			if (in_obj->get_carried_by()) {
				if (IS_NPC(in_obj->get_carried_by())) {
					if (world[IN_ROOM(in_obj->get_carried_by())]->zone_rn == world[ch->in_room]->zone_rn) {
						return true;
					} else {
						return false;
					}
				} else {
					return true;
				}
			} else if (in_obj->get_in_room() != kNowhere && in_obj->get_in_room()) {
				if (world[in_obj->get_in_room()]->zone_rn == world[ch->in_room]->zone_rn) {
					return true;
				} else {
					return false;
				}
			} else if (in_obj->get_worn_by()) {
				const auto worn_by = i->get_in_obj()->get_worn_by();
				if (IS_NPC(worn_by)) {
					if (world[worn_by->in_room]->zone_rn == world[ch->in_room]->zone_rn) {
						return true;
					} else {
						return false;
					}
				} else {
					return true;
				}
			} else {
				return true;
			}
		}
	} else {
		return true;
	}
}

ObjectData *get_object_in_equip_vis(CharacterData *ch, const char *arg, ObjectData *equipment[], int *j) {
	int l, number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	strcpy(tmp, arg);
	if (!(number = get_number(&tmp)))
		return (nullptr);

	for ((*j) = 0, l = 0; (*j) < NUM_WEARS; (*j)++) {
		if (equipment[(*j)]) {
			if (CAN_SEE_OBJ(ch, equipment[(*j)])) {
				if (isname(tmp, equipment[(*j)]->get_aliases())
					|| CHECK_CUSTOM_LABEL(tmp, equipment[(*j)], ch)) {
					if (++l == number) {
						return equipment[(*j)];
					}
				}
			}
		}
	}

	return (nullptr);
}

char *money_desc(int amount, int padis) {
	static char buf[128];
	const char *single[6][2] = {{"а", "а"},
								{"ой", "ы"},
								{"ой", "е"},
								{"у", "у"},
								{"ой", "ой"},
								{"ой", "е"}
	}, *plural[6][3] =
		{
			{
				"ая", "а", "а"}, {
				"ой", "и", "ы"}, {
				"ой", "е", "е"}, {
				"ую", "у", "у"}, {
				"ой", "ой", "ой"}, {
				"ой", "е", "е"}
		};

	if (amount <= 0) {
		log("SYSERR: Try to create negative or 0 money (%d).", amount);
		return (nullptr);
	}
	if (amount == 1) {
		sprintf(buf, "одн%s кун%s", single[padis][0], single[padis][1]);
	} else if (amount <= 10)
		sprintf(buf, "малюсеньк%s горстк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 20)
		sprintf(buf, "маленьк%s горстк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 75)
		sprintf(buf, "небольш%s горстк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 200)
		sprintf(buf, "маленьк%s кучк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 1000)
		sprintf(buf, "небольш%s кучк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 5000)
		sprintf(buf, "кучк%s кун", plural[padis][1]);
	else if (amount <= 10000)
		sprintf(buf, "больш%s кучк%s кун", plural[padis][0], plural[padis][1]);
	else if (amount <= 20000)
		sprintf(buf, "груд%s кун", plural[padis][2]);
	else if (amount <= 75000)
		sprintf(buf, "больш%s груд%s кун", plural[padis][0], plural[padis][2]);
	else if (amount <= 150000)
		sprintf(buf, "горк%s кун", plural[padis][1]);
	else if (amount <= 250000)
		sprintf(buf, "гор%s кун", plural[padis][2]);
	else
		sprintf(buf, "огромн%s гор%s кун", plural[padis][0], plural[padis][2]);

	return (buf);
}

ObjectData::shared_ptr create_money(int amount) {
	int i;
	char buf[200];

	if (amount <= 0) {
		log("SYSERR: Try to create negative or 0 money. (%d)", amount);
		return (nullptr);
	}
	auto obj = world_objects.create_blank();
	ExtraDescription::shared_ptr new_descr(new ExtraDescription());

	if (amount == 1) {
		sprintf(buf, "coin gold кун деньги денег монет %s", money_desc(amount, 0));
		obj->set_aliases(buf);
		obj->set_short_description("куна");
		obj->set_description("Одна куна лежит здесь.");
		new_descr->keyword = str_dup("coin gold монет кун денег");
		new_descr->description = str_dup("Всего лишь одна куна.");
		for (i = 0; i < CObjectPrototype::NUM_PADS; i++) {
			obj->set_PName(i, money_desc(amount, i));
		}
	} else {
		sprintf(buf, "coins gold кун денег %s", money_desc(amount, 0));
		obj->set_aliases(buf);
		obj->set_short_description(money_desc(amount, 0));
		for (i = 0; i < CObjectPrototype::NUM_PADS; i++) {
			obj->set_PName(i, money_desc(amount, i));
		}

		sprintf(buf, "Здесь лежит %s.", money_desc(amount, 0));
		obj->set_description(CAP(buf));

		new_descr->keyword = str_dup("coins gold кун денег");
	}

	new_descr->next = nullptr;
	obj->set_ex_description(new_descr);

	obj->set_type(ObjectData::ITEM_MONEY);
	obj->set_wear_flags(to_underlying(EWearFlag::ITEM_WEAR_TAKE));
	obj->set_sex(ESex::kFemale);
	obj->set_val(0, amount);
	obj->set_cost(amount);
	obj->set_maximum_durability(ObjectData::DEFAULT_MAXIMUM_DURABILITY);
	obj->set_current_durability(ObjectData::DEFAULT_CURRENT_DURABILITY);
	obj->set_timer(24 * 60 * 7);
	obj->set_weight(1);
	obj->set_extra_flag(EExtraFlag::ITEM_NODONATE);
	obj->set_extra_flag(EExtraFlag::ITEM_NOSELL);

	return obj;
}

/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be nullptr if no character was found, otherwise points
 * **tar_obj Will be nullptr if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */
int generic_find(char *arg, Bitvector bitvector, CharacterData *ch, CharacterData **tar_ch, ObjectData **tar_obj) {
	char name[256];

	*tar_ch = nullptr;
	*tar_obj = nullptr;

	ObjectData *i;
	int l, number, j = 0;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	one_argument(arg, name);

	if (!*name)
		return (0);

	if (IS_SET(bitvector, FIND_CHAR_ROOM))    // Find person in room
	{
		if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_ROOM)) != nullptr)
			return (FIND_CHAR_ROOM);
	}
	if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
		if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_WORLD)) != nullptr)
			return (FIND_CHAR_WORLD);
	}
	if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
		if ((*tar_obj = get_obj_vis(ch, name)))
			return (FIND_OBJ_WORLD);
	}

	// Начало изменений. (с) Дмитрий ака dzMUDiST ака Кудояр

// Переписан код, обрабатывающий параметры FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM
// В итоге поиск объекта просиходит в "экипировке - инветаре - комнате" согласно
// общему количеству имеющихся "созвучных" предметов.
// Старый код закомментирован и подан в конце изменений.

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return 0;

	if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
		for (l = 0; l < NUM_WEARS; l++) {
			if (GET_EQ(ch, l) && CAN_SEE_OBJ(ch, GET_EQ(ch, l))) {
				if (isname(tmp, GET_EQ(ch, l)->get_aliases())
					|| CHECK_CUSTOM_LABEL(tmp, GET_EQ(ch, l), ch)
					|| (IS_SET(bitvector, FIND_OBJ_EXDESC)
						&& find_exdesc(tmp, GET_EQ(ch, l)->get_ex_description()))) {
					if (++j == number) {
						*tar_obj = GET_EQ(ch, l);
						return (FIND_OBJ_EQUIP);
					}
				}
			}
		}
	}

	if (IS_SET(bitvector, FIND_OBJ_INV)) {
		for (i = ch->carrying; i && (j <= number); i = i->get_next_content()) {
			if (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i, ch)
				|| (IS_SET(bitvector, FIND_OBJ_EXDESC)
					&& find_exdesc(tmp, i->get_ex_description()))) {
				if (CAN_SEE_OBJ(ch, i)) {
					if (++j == number) {
						*tar_obj = i;
						return (FIND_OBJ_INV);
					}
				}
			}
		}
	}

	if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
		for (i = world[ch->in_room]->contents;
			 i && (j <= number); i = i->get_next_content()) {
			if (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i, ch)
				|| (IS_SET(bitvector, FIND_OBJ_EXDESC)
					&& find_exdesc(tmp, i->get_ex_description()))) {
				if (CAN_SEE_OBJ(ch, i)) {
					if (++j == number) {
						*tar_obj = i;
						return (FIND_OBJ_ROOM);
					}
				}
			}
		}
	}

	return (0);
}

// a function to scan for "all" or "all.x"
int find_all_dots(char *arg) {
	char tmpname[kMaxInputLength];

	if (!str_cmp(arg, "all") || !str_cmp(arg, "все")) {
		return (FIND_ALL);
	} else if (!strn_cmp(arg, "all.", 4) || !strn_cmp(arg, "все.", 4)) {
		strl_cpy(tmpname, arg + 4, kMaxInputLength);
		strl_cpy(arg, tmpname, kMaxInputLength);
		return (FIND_ALLDOT);
	} else {
		return (FIND_INDIV);
	}
}

// Функции для работы с порталами для "townportal"
// Возвращает указатель на слово по vnum комнаты или nullptr если не найдено
char *find_portal_by_vnum(int vnum) {
	struct Portal *i;
	for (i = portals_list; i; i = i->next) {
		if (i->vnum == vnum)
			return (i->wrd);
	}
	return (nullptr);
}

// Возвращает минимальный уровень для изучения портала
int level_portal_by_vnum(int vnum) {
	struct Portal *i;
	for (i = portals_list; i; i = i->next) {
		if (i->vnum == vnum)
			return (i->level);
	}
	return (0);
}

// Возвращает vnum портала по ключевому слову или kNowhere если не найдено
int find_portal_by_word(char *wrd) {
	struct Portal *i;
	for (i = portals_list; i; i = i->next) {
		if (!str_cmp(i->wrd, wrd))
			return (i->vnum);
	}
	return (kNowhere);
}

struct Portal *get_portal(int vnum, char *wrd) {
	struct Portal *i;
	for (i = portals_list; i; i = i->next) {
		if ((!wrd || !str_cmp(i->wrd, wrd)) && ((vnum == -1) || (i->vnum == vnum)))
			break;
	}
	return i;
}

/* Добавляет в список чару портал в комнату vnum - с проверкой целостности
   и если есть уже такой - то добавляем его 1м в список */
void add_portal_to_char(CharacterData *ch, int vnum) {
	struct CharacterPortal *tmp, *dlt = nullptr;

	// Проверка на то что уже есть портал в списке, если есть, удаляем
	for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next) {
		if (tmp->vnum == vnum) {
			if (dlt) {
				dlt->next = tmp->next;
			} else {
				GET_PORTALS(ch) = tmp->next;
			}
			free(tmp);
			break;
		}
		dlt = tmp;
	}

	CREATE(tmp, 1);
	tmp->vnum = vnum;
	tmp->next = GET_PORTALS(ch);
	GET_PORTALS(ch) = tmp;
}

// Проверка на то, знает ли чар портал в комнате vnum
int has_char_portal(CharacterData *ch, int vnum) {
	struct CharacterPortal *tmp;

	for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next) {
		if (tmp->vnum == vnum)
			return (1);
	}
	return (0);
}

// Убирает лишние и несуществующие порталы у чара
void check_portals(CharacterData *ch) {
	int max_p, portals;
	struct CharacterPortal *tmp, *dlt = nullptr;
	struct Portal *port;

	// Вычисляем максимальное количество порталов, которое может запомнить чар
	max_p = MAX_PORTALS(ch);
	portals = 0;

	// Пробегаем max_p порталы
	for (tmp = GET_PORTALS(ch); tmp;) {
		port = get_portal(tmp->vnum, nullptr);
		if (!port || (portals >= max_p) || (MAX(1, port->level - GET_REAL_REMORT(ch) / 2) > GET_REAL_LEVEL(ch))) {
			if (dlt) {
				dlt->next = tmp->next;
			} else {
				GET_PORTALS(ch) = tmp->next;
			}
			free(tmp);
			if (dlt) {
				tmp = dlt->next;
			} else {
				tmp = GET_PORTALS(ch);
			}
		} else {
			dlt = tmp;
			portals++;
			tmp = tmp->next;
		}
	}
}

float get_effective_cha(CharacterData *ch) {
	int key_value, key_value_add;

	key_value = ch->get_cha();
	auto max_cha = class_stats_limit[ch->get_class()][5];
	key_value_add = MIN(max_cha - ch->get_cha(), GET_CHA_ADD(ch));

	float eff_cha = 0.0;
	if (GET_REAL_LEVEL(ch) <= 14) {
		eff_cha = key_value
			- 6 * (float) (14 - GET_REAL_LEVEL(ch)) / 13.0 + key_value_add
			* (0.2 + 0.3 * (float) (GET_REAL_LEVEL(ch) - 1) / 13.0);
	} else if (GET_REAL_LEVEL(ch) <= 26) {
		eff_cha = key_value + key_value_add * (0.5 + 0.5 * (float) (GET_REAL_LEVEL(ch) - 14) / 12.0);
	} else {
		eff_cha = key_value + key_value_add;
	}

	return VPOSI<float>(eff_cha, 1.0f, static_cast<float>(max_cha));
}

float get_effective_wis(CharacterData *ch, int spellnum) {
	int key_value, key_value_add;

	auto max_wis = class_stats_limit[ch->get_class()][3];

	if (spellnum == kSpellResurrection || spellnum == kSpellAnimateDead) {
		key_value = ch->get_wis();
		key_value_add = MIN(max_wis - ch->get_wis(), GET_WIS_ADD(ch));
	} else {
		//если гдето вылезет косяком
		key_value = 0;
		key_value_add = 0;
	}

	float eff_wis = 0.0;
	if (GET_REAL_LEVEL(ch) <= 14) {
		eff_wis = key_value
			- 6 * (float) (14 - GET_REAL_LEVEL(ch)) / 13.0 + key_value_add
			* (0.4 + 0.6 * (float) (GET_REAL_LEVEL(ch) - 1) / 13.0);
	} else if (GET_REAL_LEVEL(ch) <= 26) {
		eff_wis = key_value + key_value_add * (0.5 + 0.5 * (float) (GET_REAL_LEVEL(ch) - 14) / 12.0);
	} else {
		eff_wis = key_value + key_value_add;
	}

	return VPOSI<float>(eff_wis, 1.0f, static_cast<float>(max_wis));
}

float get_effective_int(CharacterData *ch) {
	int key_value, key_value_add;

	key_value = ch->get_int();
	auto max_int = class_stats_limit[ch->get_class()][4];
	key_value_add = MIN(max_int - ch->get_int(), GET_INT_ADD(ch));

	float eff_int = 0.0;
	if (GET_REAL_LEVEL(ch) <= 14) {
		eff_int = key_value
			- 6 * (float) (14 - GET_REAL_LEVEL(ch)) / 13.0 + key_value_add
			* (0.2 + 0.3 * (float) (GET_REAL_LEVEL(ch) - 1) / 13.0);
	} else if (GET_REAL_LEVEL(ch) <= 26) {
		eff_int = key_value + key_value_add * (0.5 + 0.5 * (float) (GET_REAL_LEVEL(ch) - 14) / 12.0);
	} else {
		eff_int = key_value + key_value_add;
	}

	return VPOSI<float>(eff_int, 1.0f, static_cast<float>(max_int));
}

int get_player_charms(CharacterData *ch, int spellnum) {
	float r_hp = 0;
	float eff_cha = 0.0;
	float max_cha;

	if (spellnum == kSpellResurrection || spellnum == kSpellAnimateDead) {
		eff_cha = get_effective_wis(ch, spellnum);
		max_cha = class_stats_limit[ch->get_class()][3];
	} else {
		max_cha = class_stats_limit[ch->get_class()][5];
		eff_cha = get_effective_cha(ch);
	}

	if (spellnum != kSpellCharm) {
		eff_cha = MMIN(max_cha, eff_cha + 2); // Все кроме чарма кастится с бонусом в 2
	}

	if (eff_cha < max_cha) {
		r_hp = (1 - eff_cha + (int) eff_cha) * cha_app[(int) eff_cha].charms +
			(eff_cha - (int) eff_cha) * cha_app[(int) eff_cha + 1].charms;
	} else {
		r_hp = (1 - eff_cha + (int) eff_cha) * cha_app[(int) eff_cha].charms;
	}
	float remort_coeff = 1.0 + (((float) GET_REAL_REMORT(ch) - 9.0) * 1.2) / 100.0;
	if (remort_coeff > 1.0f) {
		r_hp *= remort_coeff;
	}

	if (PRF_FLAGGED(ch, PRF_TESTER))
		send_to_char(ch, "&Gget_player_charms Расчет чарма r_hp = %f \r\n&n", r_hp);
	return (int) r_hp;
}

//********************************************************************
// Работа с очередью мема (Svent TODO: вынести в отдельный модуль)

#define DRUID_MANA_COST_MODIFIER 0.5

//	Коэффициент изменения мема относительно скилла магии.
int koef_skill_magic(int percent_skill) {
//	Выделяем процент на который меняется мем
	return ((800 - percent_skill) / 8);

//	return 0;
}

int mag_manacost(const CharacterData *ch, int spellnum) {
	int result = 0;
	if (IS_IMMORTAL(ch)) {
		return 1;
	}

//	Мем рунных профессий(на сегодня только волхвы)
	if (IS_MANA_CASTER(ch) && GET_REAL_LEVEL(ch) >= CalcRequiredLevel(ch, spellnum)) {
		result = static_cast<int>(DRUID_MANA_COST_MODIFIER
			* (float) mana_gain_cs[VPOSI(55 - GET_REAL_INT(ch), 10, 50)]
			/ (float) int_app[VPOSI(55 - GET_REAL_INT(ch), 10, 50)].mana_per_tic
			* 60
			* MAX(SpINFO.mana_max
					  - (SpINFO.mana_change
						  * (GET_REAL_LEVEL(ch)
							  - spell_create[spellnum].runes.min_caster_level)),
				  SpINFO.mana_min));
	} else {
		if (!IS_MANA_CASTER(ch) && GET_REAL_LEVEL(ch) >= MIN_CAST_LEV(SpINFO, ch)
			&& GET_REAL_REMORT(ch) >= MIN_CAST_REM(SpINFO, ch)) {
			result = MAX(SpINFO.mana_max - (SpINFO.mana_change * (GET_REAL_LEVEL(ch) - MIN_CAST_LEV(SpINFO, ch))), SpINFO.mana_min);
			if (SpINFO.class_change[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] < 0) {
				result = result * (100 - MIN(99, abs(SpINFO.class_change[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]))) / 100;
			} else {
				result = result * 100 / (100 - MIN(99, abs(SpINFO.class_change[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])));
			}
//		Меняем мем на коэффициент скилла магии
			if (GET_CLASS(ch) == kPaladine || GET_CLASS(ch) == kMerchant) {
				return result;
			}
		}
	}
	if (result > 0)
		return result * koef_skill_magic(ch->get_skill(GetMagicSkillId(spellnum))) / 100;
				// при скилле 200 + 25%, чем меньше тем лучше
	else 
		return 99999;
}

void MemQ_init(CharacterData *ch) {
	ch->MemQueue.stored = 0;
	ch->MemQueue.total = 0;
	ch->MemQueue.queue = nullptr;
}

void MemQ_flush(CharacterData *ch) {
	struct SpellMemQueueItem *i;
	while (ch->MemQueue.queue) {
		i = ch->MemQueue.queue;
		ch->MemQueue.queue = i->link;
		free(i);
	}
	MemQ_init(ch);
}

int MemQ_learn(CharacterData *ch) {
	int num;
	struct SpellMemQueueItem *i;
	if (ch->MemQueue.queue == nullptr)
		return 0;
	num = GET_MEM_CURRENT(ch);
	ch->MemQueue.stored -= num;
	ch->MemQueue.total -= num;
	num = ch->MemQueue.queue->spellnum;
	i = ch->MemQueue.queue;
	ch->MemQueue.queue = i->link;
	free(i);
	sprintf(buf, "Вы выучили заклинание \"%s%s%s\".\r\n",
			CCICYN(ch, C_NRM), spell_info[num].name, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	return num;
}

void MemQ_remember(CharacterData *ch, int num) {
	int *slots;
	int slotcnt, slotn;
	struct SpellMemQueueItem *i, **pi = &ch->MemQueue.queue;

	// проверить количество слотов
	slots = MemQ_slots(ch);
	slotn = spell_info[num].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
	slotcnt = slot_for_char(ch, slotn + 1);
	slotcnt -= slots[slotn];    // кол-во свободных слотов

	if (slotcnt <= 0) {
		send_to_char("У вас нет свободных ячеек этого круга.", ch);
		return;
	}

	if (GET_RELIGION(ch) == kReligionMono)
		sprintf(buf, "Вы дописали заклинание \"%s%s%s\" в свой часослов.\r\n",
				CCIMAG(ch, C_NRM), spell_info[num].name, CCNRM(ch, C_NRM));
	else
		sprintf(buf, "Вы занесли заклинание \"%s%s%s\" в свои резы.\r\n",
				CCIMAG(ch, C_NRM), spell_info[num].name, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	ch->MemQueue.total += mag_manacost(ch, num);
	while (*pi)
		pi = &((*pi)->link);
	CREATE(i, 1);
	*pi = i;
	i->spellnum = num;
	i->link = nullptr;
}

void MemQ_forget(CharacterData *ch, int num) {
	struct SpellMemQueueItem **q = nullptr, **i;

	for (i = &ch->MemQueue.queue; *i; i = &(i[0]->link)) {
		if (i[0]->spellnum == num)
			q = i;
	}

	if (q == nullptr) {
		send_to_char("Вы и не собирались заучить это заклинание.\r\n", ch);
	} else {
		struct SpellMemQueueItem *ptr;
		if (q == &ch->MemQueue.queue)
			GET_MEM_COMPLETED(ch) = 0;
		GET_MEM_TOTAL(ch) = MAX(0, GET_MEM_TOTAL(ch) - mag_manacost(ch, num));
		ptr = q[0];
		q[0] = q[0]->link;
		free(ptr);
		sprintf(buf,
				"Вы вычеркнули заклинание \"%s%s%s\" из списка для запоминания.\r\n",
				CCIMAG(ch, C_NRM), spell_info[num].name, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}
}

int *MemQ_slots(CharacterData *ch) {
	struct SpellMemQueueItem **q, *qt;
	static int slots[kMaxSlot];
	int i, n, sloti;

	// инициализация
	for (i = 0; i < kMaxSlot; ++i)
		slots[i] = slot_for_char(ch, i + 1);

	for (i = kSpellCount; i >= 1; --i) {
		if (!IS_SET(GET_SPELL_TYPE(ch, i), kSpellKnow | kSpellTemp))
			continue;
		if ((n = GET_SPELL_MEM(ch, i)) == 0)
			continue;
		sloti = spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
		if (MIN_CAST_LEV(spell_info[i], ch) > GET_REAL_LEVEL(ch)
			|| MIN_CAST_REM(spell_info[i], ch) > GET_REAL_REMORT(ch)) {
			GET_SPELL_MEM(ch, i) = 0;
			continue;
		}
		slots[sloti] -= n;
		if (slots[sloti] < 0) {
			GET_SPELL_MEM(ch, i) += slots[sloti];
			slots[sloti] = 0;
		}

	}

	for (q = &ch->MemQueue.queue; q[0];) {
		sloti = spell_info[q[0]->spellnum].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
		if (sloti >= 0 && sloti <= 10) {
			--slots[sloti];
			if (slots[sloti] >= 0 &&
				MIN_CAST_LEV(spell_info[q[0]->spellnum], ch) <= GET_REAL_LEVEL(ch)
				&& MIN_CAST_REM(spell_info[q[0]->spellnum], ch) <= GET_REAL_REMORT(ch)) {
				q = &(q[0]->link);
			} else {
				if (q == &ch->MemQueue.queue)
					GET_MEM_COMPLETED(ch) = 0;
				GET_MEM_TOTAL(ch) = MAX(0, GET_MEM_TOTAL(ch) - mag_manacost(ch, q[0]->spellnum));
				++slots[sloti];
				qt = q[0];
				q[0] = q[0]->link;
				free(qt);
			}
		}
	}

	for (i = 0; i < kMaxSlot; ++i)
		slots[i] = slot_for_char(ch, i + 1) - slots[i];

	return slots;
}

int equip_in_metall(CharacterData *ch) {
	int i, wgt = 0;

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return (false);
	if (IS_GOD(ch))
		return (false);

	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i)
			&& ObjSystem::is_armor_type(GET_EQ(ch, i))
			&& GET_OBJ_MATER(GET_EQ(ch, i)) <= ObjectData::MAT_COLOR) {
			wgt += GET_OBJ_WEIGHT(GET_EQ(ch, i));
		}
	}

	if (wgt > GET_REAL_STR(ch))
		return (true);

	return (false);
}

int awake_others(CharacterData *ch) {
	if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return (false);

	if (IS_GOD(ch))
		return (false);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STAIRS) ||
		AFF_FLAGGED(ch, EAffectFlag::AFF_SANCTUARY) || AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT))
		return (true);

	return (false);
}

// Учет резиста - возвращается эффект от спелл или умения с учетом резиста

int calculate_resistance_coeff(CharacterData *ch, int resist_type, int effect) {
	int result, resistance;

	resistance = GET_RESIST(ch, resist_type);
	if (resistance <= 0) {
		return effect - resistance * effect / 100;
	}
	if (!IS_NPC(ch)) {
		resistance = MIN(75, resistance);
	}
	const float divisor = 200; 
	result = effect - (resistance + number(0, resistance)) * effect / divisor;
	result = MAX(0, result);
	return result;
}

int getResisTypeWithSpellClass(int spellClass) {
	switch (spellClass) {
		case kTypeFire: return FIRE_RESISTANCE;
			break;
		case kTypeDark: return DARK_RESISTANCE;
			break;
		case kTypeAir: return AIR_RESISTANCE;
			break;
		case kTypeWater: return WATER_RESISTANCE;
			break;
		case kTypeEarth: return EARTH_RESISTANCE;
			break;
		case kTypeLight: return VITALITY_RESISTANCE;
			break;
		case kTypeMind: return MIND_RESISTANCE;
			break;
		case kTypeLife: return IMMUNITY_RESISTANCE;
			break;
		case kTypeNeutral: return VITALITY_RESISTANCE;
			break;
	}
	return VITALITY_RESISTANCE;
};

int get_resist_type(int spellnum) {
	return getResisTypeWithSpellClass(SpINFO.spell_class);
}

// * Берется минимальная цена ренты шмотки, не важно, одетая она будет или снятая.
int get_object_low_rent(ObjectData *obj) {
	int rent = GET_OBJ_RENT(obj) > GET_OBJ_RENTEQ(obj) ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj);
	return rent;
}

// * Удаление рунной метки (при пропадании в пустоте и реморте).
void remove_rune_label(CharacterData *ch) {
	RoomData *label_room = room_spells::FindAffectedRoom(GET_ID(ch), kSpellRuneLabel);
	if (label_room) {
		const auto aff = room_spells::FindAffect(label_room, kSpellRuneLabel);
		if (aff != label_room->affected.end()) {
			room_spells::RemoveAffect(label_room, aff);
			send_to_char("Ваша рунная метка удалена.\r\n", ch);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
