/**************************************************************************
*  File:  dg_objcmd.cpp                                  Part of Bylins   *
*  Usage: contains the command_interpreter for objects,                   *
*         object commands.                                                *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#include "entities/char.h"
#include "cmd/follow.h"
#include "fightsystem/fight.h"
#include "fightsystem/pk.h"
#include "handler.h"
#include "obj_prototypes.h"
#include "magic/magic_utils.h"
#include "skills/townportal.h"
#include "utils/id_converter.h"
#include "entities/zone.h"
#include "structs/global_objects.h"

extern const char *dirs[];
extern int up_obj_where(ObjectData *obj);
extern int reloc_target;

CharacterData *get_char_by_obj(ObjectData *obj, char *name);
ObjectData *get_obj_by_obj(ObjectData *obj, char *name);
void sub_write(char *arg, CharacterData *ch, byte find_invis, int targets);
void die(CharacterData *ch, CharacterData *killer);
void obj_command_interpreter(ObjectData *obj, char *argument);
void send_to_zone(char *messg, int zone_rnum);

RoomData *get_room(char *name);

bool mob_script_command_interpreter(CharacterData *ch, char *argument);

struct obj_command_info {
	const char *command;
	typedef void(*handler_f)(ObjectData *obj, char *argument, int cmd, int subcmd);
	handler_f command_pointer;
	int subcmd;
};

// do_osend
#define SCMD_OSEND         0
#define SCMD_OECHOAROUND   1

// attaches object name and vnum to msg_set and sends it to script_log
void obj_log(ObjectData *obj, const char *msg, LogMode type = LogMode::OFF) {
	char buf[kMaxInputLength + 100];

	sprintf(buf,
			"(Obj: '%s', VNum: %d, trig: %d): %s",
			obj->get_short_description().c_str(),
			GET_OBJ_VNUM(obj),
			last_trig_vnum,
			msg);
	script_log(buf, type);
}

// returns the real room number that the object or object's carrier is in
int obj_room(ObjectData *obj) {
	if (obj->get_in_room() != kNowhere) {
		return obj->get_in_room();
	} else if (obj->get_carried_by()) {
		return IN_ROOM(obj->get_carried_by());
	} else if (obj->get_worn_by()) {
		return IN_ROOM(obj->get_worn_by());
	} else if (obj->get_in_obj()) {
		return obj_room(obj->get_in_obj());
	} else {
		return kNowhere;
	}
}

// returns the real room number, or kNowhere if not found or invalid
int find_obj_target_room(ObjectData *obj, char *rawroomstr) {
	char roomstr[kMaxInputLength];
	RoomRnum location = kNowhere;

	one_argument(rawroomstr, roomstr);

	if (!*roomstr) {
		sprintf(buf, "Undefined oteleport room: %s", rawroomstr);
		obj_log(obj, buf);
		return kNowhere;
	}

	auto tmp = atoi(roomstr);
	if (tmp > 0) {
		location = real_room(tmp);
	} else {
		sprintf(buf, "Undefined oteleport room: %s", roomstr);
		obj_log(obj, buf);
		return kNowhere;
	}

	return location;
}

void do_oportal(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	int target, howlong, curroom, nr;
	char arg1[kMaxInputLength], arg2[kMaxInputLength];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2) {
		obj_log(obj, "oportal: called with too few args");
		return;
	}

	howlong = atoi(arg2);
	nr = atoi(arg1);
	target = real_room(nr);

	if (target == kNowhere) {
		obj_log(obj, "oportal: target is an invalid room");
		return;
	}

	/* Ставим пентаграмму из текущей комнаты в комнату target с
	   длительностью howlong */
	curroom = real_room(get_room_where_obj(obj));
	world[curroom]->portal_room = target;
	world[curroom]->portal_time = howlong;
	world[curroom]->pkPenterUnique = 0;
//	sprintf(buf, "Ставим врата из %d в %d длит %d\r\n", currom, target, howlong );
//	mudlog(buf, DEF, MAX(kLevelImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
	OneWayPortal::add(world[target], world[curroom]);
	act("Лазурная пентаграмма возникла в воздухе.",
		false, world[curroom]->first_character(), 0, 0, TO_CHAR);
	act("Лазурная пентаграмма возникла в воздухе.",
		false, world[curroom]->first_character(), 0, 0, TO_ROOM);
}
// Object commands
void do_oecho(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	skip_spaces(&argument);

	int room;
	if (!*argument) {
		obj_log(obj, "oecho called with no args");
	} else if ((room = obj_room(obj)) != kNowhere) {
		if (!world[room]->people.empty()) {
			sub_write(argument, world[room]->first_character(), true, TO_ROOM | TO_CHAR);
		}
	}
}
void do_oat(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
//	int location;
	char roomstr[kMaxInputLength];
	RoomRnum location = kNowhere;
	if (!*argument) {
		obj_log(obj, "oat: bad argument");
		return;
	}
	one_argument(argument, roomstr);
	auto tmp = atoi(roomstr);
	if (tmp > 0) {
		location = real_room(tmp);
	} else {
		sprintf(buf, "oat: invalid location '%d'", tmp);
		obj_log(obj, buf);
		return;
	}
	argument = one_argument(argument, roomstr);
	auto tmp_obj = world_objects.create_from_prototype_by_vnum(obj->get_vnum());
	tmp_obj->set_in_room(location);
	obj_command_interpreter(tmp_obj.get(), argument);
	world_objects.remove(tmp_obj);
}

void do_oforce(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharacterData *ch;
	char arg1[kMaxInputLength], *line;

	line = one_argument(argument, arg1);

	if (!*arg1 || !*line) {
		obj_log(obj, "oforce called with too few args");

		return;
	}

	if (!str_cmp(arg1, "all")
		|| !str_cmp(arg1, "все")) {
		obj_log(obj, "ERROR: \'oforce all\' command disabled.");
		return;

		/*if ((room = obj_room(obj)) == kNowhere)
		{
			obj_log(obj, "oforce called by object in kNowhere");
		}
		else
		{
			const auto people_copy = world[room]->people;
			for (const auto ch : people_copy)
			{
				if (IS_NPC(ch)
					|| GET_REAL_LEVEL(ch) < kLevelImmortal)
				{
					command_interpreter(ch, line);
				}
			}
		}*/
	} else {
		if ((ch = get_char_by_obj(obj, arg1))) {
			// если чар в ЛД
			if (!IS_NPC(ch)) {
				if (!ch->desc) {
					return;
				}
			}

			if (IS_NPC(ch)) {
				if (mob_script_command_interpreter(ch, line)) {
					obj_log(obj, "Mob trigger commands in oforce. Please rewrite trigger.");
					return;
				}

				command_interpreter(ch, line);
			} else if (GET_REAL_LEVEL(ch) < kLevelImmortal) {
				command_interpreter(ch, line);
			}
		} else {
			obj_log(obj, "oforce: no target found");
		}
	}
}

void do_osend(ObjectData *obj, char *argument, int/* cmd*/, int subcmd) {
	char buf[kMaxInputLength], *msg;
	CharacterData *ch;

	msg = any_one_arg(argument, buf);

	if (!*buf) {
		obj_log(obj, "osend called with no args");
		return;
	}

	skip_spaces(&msg);

	if (!*msg) {
		obj_log(obj, "osend called without a message");
		return;
	}
	if ((ch = get_char_by_obj(obj, buf))) {
		if (subcmd == SCMD_OSEND)
			sub_write(msg, ch, true, TO_CHAR);
		else if (subcmd == SCMD_OECHOAROUND)
			sub_write(msg, ch, true, TO_ROOM);
	} else
		obj_log(obj, "no target found for osend");
}

// increases the target's exp
void do_oexp(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharacterData *ch;
	char name[kMaxInputLength], amount[kMaxInputLength];

	two_arguments(argument, name, amount);

	if (!*name || !*amount) {
		obj_log(obj, "oexp: too few arguments");
		return;
	}

	if ((ch = get_char_by_obj(obj, name))) {
		gain_exp(ch, atoi(amount));
		sprintf(buf, "oexp: victim (%s) получил опыт %d", GET_NAME(ch), atoi(amount));
		obj_log(obj, buf);
	} else {
		obj_log(obj, "oexp: target not found");
		return;
	}
}

// set the object's timer value
void do_otimer(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];

	one_argument(argument, arg);

	if (!*arg)
		obj_log(obj, "otimer: missing argument");
	else if (!a_isdigit(*arg))
		obj_log(obj, "otimer: bad argument");
	else {
		obj->set_timer(atoi(arg));
	}
}

// transform into a different object
// note: this shouldn't be used with containers unless both objects
// are containers!
void do_otransform(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	CharacterData *wearer = nullptr;
	int pos = -1;

	one_argument(argument, arg);

	if (!*arg) {
		obj_log(obj, "otransform: missing argument");
	} else if (!a_isdigit(*arg)) {
		obj_log(obj, "otransform: bad argument");
	} else {
		const auto o = world_objects.create_from_prototype_by_vnum(atoi(arg));
		if (o == nullptr) {
			obj_log(obj, "otransform: bad object vnum");
			return;
		}
		// Описание работы функции см. в mtransform()

		if (obj->get_worn_by()) {
			pos = obj->get_worn_on();
			wearer = obj->get_worn_by();
			unequip_char(obj->get_worn_by(), pos, CharEquipFlags());
		}

		obj->swap(*o);

		if (o->get_extra_flag(EExtraFlag::ITEM_TICKTIMER)) {
			obj->set_extra_flag(EExtraFlag::ITEM_TICKTIMER);
		}

		if (wearer) {
			equip_char(wearer, obj, pos, CharEquipFlags());
		}
		extract_obj(o.get());
	}
}

// purge all objects an npcs in room, or specified object or mob
void do_opurge(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	CharacterData *ch;
	ObjectData *o;

	one_argument(argument, arg);

	if (!*arg) {
		return;
	}

	if (!(ch = get_char_by_obj(obj, arg))) {
		if ((o = get_obj_by_obj(obj, arg))) {
			extract_obj(o);
		} else
			obj_log(obj, "opurge: bad argument");
		return;
	}

	if (!IS_NPC(ch)) {
		obj_log(obj, "opurge: purging a PC");
		return;
	}

	if (ch->followers
		|| ch->has_master()) {
		die_follower(ch);
	}
	extract_char(ch, false);
}

void do_oteleport(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharacterData *ch, *horse;
	int target, rm;
	char arg1[kMaxInputLength], arg2[kMaxInputLength];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2) {
		obj_log(obj, "oteleport called with too few args");
		return;
	}

	target = find_obj_target_room(obj, arg2);

	if (target == kNowhere)
		obj_log(obj, "oteleport target is an invalid room");
	else if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все")) {
		rm = obj_room(obj);
		if (rm == kNowhere) {
			obj_log(obj, "oteleport called in kNowhere");
			return;
		}
		if (target == rm) {
			obj_log(obj, "oteleport target is itself");
			return;
		}
		const auto people_copy = world[rm]->people;
		decltype(world[rm]->people)::const_iterator next_ch = people_copy.begin();
		for (auto ch_i = next_ch; ch_i != people_copy.end(); ch_i = next_ch) {
			const auto ch = *ch_i;
			++next_ch;
			if (ch->in_room == kNowhere) {
				obj_log(obj, "oteleport transports from kNowhere");
				return;
			}
			char_from_room(ch);
			char_to_room(ch, target);
			ch->dismount();
			look_at_room(ch, true);
		}
	} else if (!str_cmp(arg1, "allchar") || !str_cmp(arg1, "всечары")) {
		rm = obj_room(obj);
		if (rm == kNowhere) {
			obj_log(obj, "oteleport called in kNowhere");
			return;
		}
		if (target == rm) {
			obj_log(obj, "oteleport target is itself");
			return;
		}
		const auto people_copy = world[rm]->people;
		decltype(world[rm]->people)::const_iterator next_ch = people_copy.begin();
		for (auto ch_i = next_ch; ch_i != people_copy.end(); ch_i = next_ch) {
			const auto ch = *ch_i;
			++next_ch;
			if (ch->in_room == kNowhere) {
				obj_log(obj, "oteleport transports allchar from kNowhere");
				return;
			}
			if (IS_NPC(ch) && !IS_CHARMICE(ch))
				continue;
			char_from_room(ch);
			char_to_room(ch, target);
			ch->dismount();
			look_at_room(ch, true);
		}
	} else {
		if (!(ch = get_char_by_obj(obj, arg1))) {
			obj_log(obj, "oteleport: no target found");
			return;
		}
		if (ch->ahorse() || ch->has_horse(true)) {
			horse = ch->get_horse();
		} else {
			horse = nullptr;
		}
		if (IS_CHARMICE(ch) && ch->in_room == ch->get_master()->in_room)
			ch = ch->get_master();
		const auto people_copy = world[ch->in_room]->people;
		for (const auto charmee: people_copy) {
			if (IS_CHARMICE(charmee) && charmee->get_master() == ch) {
				char_from_room(charmee);
				char_to_room(charmee, target);
			}
		}
		if (!str_cmp(argument, "horse")
			&& horse) {
			char_from_room(horse);
			char_to_room(horse, target);
		}
		char_from_room(ch);
		char_to_room(ch, target);
		ch->dismount();
		look_at_room(ch, true);
		greet_mtrigger(ch, -1);
		greet_otrigger(ch, -1);
	}
}

void do_dgoload(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength], arg2[kMaxInputLength];
	int number = 0, room;
	CharacterData *mob;

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
		obj_log(obj, "oload: bad syntax");
		return;
	}

	if ((room = obj_room(obj)) == kNowhere) {
		obj_log(obj, "oload: object in kNowhere trying to load");
		return;
	}

	if (utils::IsAbbrev(arg1, "mob")) {
		if ((mob = read_mobile(number, VIRTUAL)) == nullptr) {
			obj_log(obj, "oload: bad mob vnum");
			return;
		}
		char_to_room(mob, room);
		load_mtrigger(mob);
	} else if (utils::IsAbbrev(arg1, "obj")) {
		const auto object = world_objects.create_from_prototype_by_vnum(number);
		if (!object) {
			obj_log(obj, "oload: bad object vnum");
			return;
		}

		if (GET_OBJ_MIW(obj_proto[object->get_rnum()]) >= 0
			&& obj_proto.actual_count(object->get_rnum()) > GET_OBJ_MIW(obj_proto[object->get_rnum()])) {
			if (!check_unlimited_timer(obj_proto[object->get_rnum()].get())) {
				sprintf(buf, "oload: Попытка загрузить предмет больше чем в MIW для #%d.", number);
				obj_log(obj, buf);
//				extract_obj(object.get());
//				return;
			}
		}
		log("Load obj #%d by %s (oload)", number, obj->get_aliases().c_str());
		object->set_vnum_zone_from(zone_table[world[room]->zone_rn].vnum);
		obj_to_room(object.get(), room);
		load_otrigger(object.get());
	} else {
		obj_log(obj, "oload: bad type");
	}
}

void ApplyDamage(CharacterData* target, int damage) {
	GET_HIT(target) -= damage;
	update_pos(target);
	char_dam_message(damage, target, target, 0);
	if (GET_POS(target) == EPosition::kDead) {
		if (!IS_NPC(target)) {
			sprintf(buf2, "%s killed by odamage at %s [%d]", GET_NAME(target),
					target->in_room == kNowhere ? "NOWHERE" : world[target->in_room]->name, GET_ROOM_VNUM(target->in_room));
			mudlog(buf2, BRF, kLevelBuilder, SYSLOG, true);
		}
		die(target, nullptr);
	}
}

void do_odamage(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	char name[kMaxInputLength], amount[kMaxInputLength], damage_type[kMaxInputLength];
	three_arguments(argument, name, amount, damage_type);
	if (!*name || !*amount || !a_isdigit(*amount)) {
		obj_log(obj, "odamage: bad syntax");
		return;
	}

	int dam = atoi(amount);

	CharacterData *ch = get_char_by_obj(obj, name);
	if (!ch) {
		obj_log(obj, "odamage: target not found");
		return;
	}
	if (world[ch->in_room]->zone_rn != world[up_obj_where(obj)]->zone_rn) {
		return;
	}

	if (IS_IMMORTAL(ch)) {
		send_to_char("Being the cool immortal you are, you sidestep a trap, obviously placed to kill you.", ch);
		return;
	}

	CharacterData *damager = dg_caster_owner_obj(obj);
	if (!damager || damager == ch) {
		ApplyDamage(ch, dam);
	} else {
		const std::map<std::string, FightSystem::DmgType> kDamageTypes = {
			{"physic", FightSystem::PHYS_DMG},
			{"magic", FightSystem::MAGE_DMG},
			{"poisonous", FightSystem::POISON_DMG}
		};
		if (!may_kill_here(damager, ch, name)) {
			return;
		}
		FightSystem::DmgType type = FightSystem::PURE_DMG;
		if (*damage_type) {
			try {
				type = kDamageTypes.at(damage_type);
			} catch (const std::out_of_range &) {
				obj_log(obj, "odamage: incorrect damage type.");;
			}
			die(ch, nullptr);
		}
		Damage odamage(SimpleDmg(kTypeTriggerdeath), dam, type);
		odamage.process(damager, ch);
	}
}

void do_odoor(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	char target[kMaxInputLength], direction[kMaxInputLength];
	char field[kMaxInputLength], *value;
	RoomData *rm;
	int dir, fd, to_room, lock;

	const char *door_field[] = {"purge",
								"description",
								"flags",
								"key",
								"name",
								"room",
								"lock",
								"\n"
	};

	argument = two_arguments(argument, target, direction);
	value = one_argument(argument, field);
	skip_spaces(&value);

	if (!*target || !*direction || !*field) {
		obj_log(obj, "odoor called with too few args");
		return;
	}

	if ((rm = get_room(target)) == nullptr) {
		obj_log(obj, "odoor: invalid target");
		return;
	}

	if ((dir = search_block(direction, dirs, false)) == -1) {
		obj_log(obj, "odoor: invalid direction");
		return;
	}

	if ((fd = search_block(field, door_field, false)) == -1) {
		obj_log(obj, "odoor: invalid field");
		return;
	}

	auto exit = rm->dir_option[dir];

	// purge exit
	if (fd == 0) {
		if (exit) {
			rm->dir_option[dir].reset();
		}
	} else {
		if (!exit) {
			exit.reset(new ExitData());
			rm->dir_option[dir] = exit;
		}

		std::string buffer;
		switch (fd) {
			case 1:    // description
				exit->general_description = std::string(value) + "\r\n";
				break;

			case 2:    // flags
				asciiflag_conv(value, &exit->exit_info);
				break;

			case 3:    // key
				exit->key = atoi(value);
				break;

			case 4:    // name
				exit->set_keywords(value);
				break;

			case 5:    // room
				if ((to_room = real_room(atoi(value))) != kNowhere) {
					exit->to_room(to_room);
				} else {
					obj_log(obj, "odoor: invalid door target");
				}
				break;

			case 6:    // lock - сложность замка
				lock = atoi(value);
				if (!(lock < 0 || lock > 255))
					exit->lock_complexity = lock;
				else
					obj_log(obj, "odoor: invalid lock complexity");
				break;
		}
	}
}

void do_osetval(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength], arg2[kMaxInputLength];
	int position, new_value;

	two_arguments(argument, arg1, arg2);
	if (!*arg1 || !*arg2 || !is_number(arg1) || !is_number(arg2)) {
		obj_log(obj, "osetval: bad syntax");
		return;
	}

	position = atoi(arg1);
	new_value = atoi(arg2);
	if (position >= 0 && position < ObjectData::VALS_COUNT) {
		obj->set_val(position, new_value);
	} else {
		obj_log(obj, "osetval: position out of bounds!");
	}
}

void do_ofeatturn(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	int isFeat = 0;
	CharacterData *ch;
	char name[kMaxInputLength], featname[kMaxInputLength], amount[kMaxInputLength], *pos;
	int featnum = 0, featdiff = 0;

	one_argument(two_arguments(argument, name, featname), amount);

	if (!*name || !*featname || !*amount) {
		obj_log(obj, "ofeatturn: too few arguments");
		return;
	}

	while ((pos = strchr(featname, '.')))
		*pos = ' ';
	while ((pos = strchr(featname, '_')))
		*pos = ' ';

	if ((featnum = find_feat_num(featname)) > 0 && featnum < kMaxFeats)
		isFeat = 1;
	else {
		sprintf(buf, "ofeatturn: %s skill/recipe not found", featname);
		obj_log(obj, buf);
		return;
	}

	if (!str_cmp(amount, "set"))
		featdiff = 1;
	else if (!str_cmp(amount, "clear"))
		featdiff = 0;
	else {
		obj_log(obj, "ofeatturn: unknown set variable");
		return;
	}

	if (!(ch = get_char_by_obj(obj, name))) {
		obj_log(obj, "ofeatturn: target not found");
		return;
	}

	if (isFeat)
		trg_featturn(ch, featnum, featdiff, last_trig_vnum);
}

void do_oskillturn(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharacterData *ch;
	char name[kMaxInputLength], skill_name[kMaxInputLength], amount[kMaxInputLength];
	int recipenum = 0;
	int skilldiff = 0;

	one_argument(two_arguments(argument, name, skill_name), amount);

	if (!*name || !*skill_name || !*amount) {
		obj_log(obj, "oskillturn: too few arguments");
		return;
	}

	auto skill_id = FixNameAndFindSkillNum(skill_name);
	bool is_skill = false;
	if (MUD::Skills().IsValid(skill_id)) {
		is_skill = true;
	} else if ((recipenum = im_get_recipe_by_name(skill_name)) < 0) {
		sprintf(buf, "oskillturn: %s skill not found", skill_name);
		obj_log(obj, buf);
		return;
	}

	if (!str_cmp(amount, "set")) {
		skilldiff = 1;
	} else if (!str_cmp(amount, "clear")) {
		skilldiff = 0;
	} else {
		obj_log(obj, "oskillturn: unknown set variable");
		return;
	}

	if (!(ch = get_char_by_obj(obj, name))) {
		obj_log(obj, "oskillturn: target not found");
		return;
	}

	if (is_skill) {
		if (MUD::Classes()[ch->get_class()].IsKnown(skill_id) ) {
			trg_skillturn(ch, skill_id, skilldiff, last_trig_vnum);
		} else {
			sprintf(buf, "oskillturn: skill and character class mismatch");
			obj_log(obj, buf);
		}
	} else {
		trg_recipeturn(ch, recipenum, skilldiff);
	}
}

void do_oskilladd(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	bool isSkill = false;
	CharacterData *ch;
	char name[kMaxInputLength], skillname[kMaxInputLength], amount[kMaxInputLength];
	int recipenum = 0;
	int skilldiff = 0;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount) {
		obj_log(obj, "oskilladd: too few arguments");
		return;
	}
	auto skillnum = FixNameAndFindSkillNum(skillname);
	if (MUD::Skills().IsValid(skillnum)) {
		isSkill = true;
	} else if ((recipenum = im_get_recipe_by_name(skillname)) < 0) {
		sprintf(buf, "oskilladd: %s skill/recipe not found", skillname);
		obj_log(obj, buf);
		return;
	}

	skilldiff = atoi(amount);

	if (!(ch = get_char_by_obj(obj, name))) {
		obj_log(obj, "oskilladd: target not found");
		return;
	}

	if (isSkill) {
		AddSkill(ch, skillnum, skilldiff, last_trig_vnum);
	} else {
		AddRecipe(ch, recipenum, skilldiff);
	}
}

void do_ospellturn(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharacterData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];
	int spellnum = 0, spelldiff = 0;

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount) {
		obj_log(obj, "ospellturn: too few arguments");
		return;
	}

	if ((spellnum = FixNameAndFindSpellNum(spellname)) < 0 || spellnum == 0 || spellnum > kSpellCount) {
		obj_log(obj, "ospellturn: spell not found");
		return;
	}

	if (!str_cmp(amount, "set"))
		spelldiff = 1;
	else if (!str_cmp(amount, "clear"))
		spelldiff = 0;
	else {
		obj_log(obj, "ospellturn: unknown set variable");
		return;
	}

	if ((ch = get_char_by_obj(obj, name))) {
		trg_spellturn(ch, spellnum, spelldiff, last_trig_vnum);
	} else {
		obj_log(obj, "ospellturn: target not found");
		return;
	}
}

void do_ospellturntemp(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharacterData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];
	int spellnum = 0, spelltime = 0;

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount) {
		obj_log(obj, "ospellturntemp: too few arguments");
		return;
	}

	if ((spellnum = FixNameAndFindSpellNum(spellname)) < 0 || spellnum == 0 || spellnum > kSpellCount) {
		obj_log(obj, "ospellturntemp: spell not found");
		return;
	}

	spelltime = atoi(amount);

	if (spelltime < 0) {
		obj_log(obj, "ospellturntemp: time is negative");
		return;
	}

	if ((ch = get_char_by_obj(obj, name))) {
		trg_spellturntemp(ch, spellnum, spelltime, last_trig_vnum);
	} else {
		obj_log(obj, "ospellturntemp: target not found");
		return;
	}
}

void do_ospelladd(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharacterData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];
	int spellnum = 0, spelldiff = 0;

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount) {
		obj_log(obj, "ospelladd: too few arguments");
		return;
	}

	if ((spellnum = FixNameAndFindSpellNum(spellname)) < 0 || spellnum == 0 || spellnum > kSpellCount) {
		obj_log(obj, "ospelladd: spell not found");
		return;
	}

	spelldiff = atoi(amount);

	if ((ch = get_char_by_obj(obj, name))) {
		trg_spelladd(ch, spellnum, spelldiff, last_trig_vnum);
	} else {
		obj_log(obj, "ospelladd: target not found");
		return;
	}
}

void do_ospellitem(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharacterData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], type[kMaxInputLength], turn[kMaxInputLength];
	int spellnum = 0, spelldiff = 0, spell = 0;

	two_arguments(two_arguments(argument, name, spellname), type, turn);

	if (!*name || !*spellname || !*type || !*turn) {
		obj_log(obj, "ospellitem: too few arguments");
		return;
	}

	if ((spellnum = FixNameAndFindSpellNum(spellname)) < 0 || spellnum == 0 || spellnum > kSpellCount) {
		obj_log(obj, "ospellitem: spell not found");
		return;
	}

	if (!str_cmp(type, "potion")) {
		spell = kSpellPotion;
	} else if (!str_cmp(type, "wand")) {
		spell = kSpellWand;
	} else if (!str_cmp(type, "scroll")) {
		spell = kSpellScroll;
	} else if (!str_cmp(type, "items")) {
		spell = kSpellItems;
	} else if (!str_cmp(type, "runes")) {
		spell = kSpellRunes;
	} else {
		obj_log(obj, "ospellitem: type spell not found");
		return;
	}

	if (!str_cmp(turn, "set")) {
		spelldiff = 1;
	} else if (!str_cmp(turn, "clear")) {
		spelldiff = 0;
	} else {
		obj_log(obj, "ospellitem: unknown set variable");
		return;
	}

	if ((ch = get_char_by_obj(obj, name))) {
		trg_spellitem(ch, spellnum, spelldiff, spell);
	} else {
		obj_log(obj, "ospellitem: target not found");
		return;
	}
}

void do_ozoneecho(ObjectData *obj, char *argument, int/* cmd*/, int/* subcmd*/) {
	ZoneRnum zone;
	char zone_name[kMaxInputLength], buf[kMaxInputLength], *msg;

	msg = any_one_arg(argument, zone_name);
	skip_spaces(&msg);

	if (!*zone_name || !*msg)
		obj_log(obj, "ozoneecho called with too few args");
	else if ((zone = get_zone_rnum_by_room_vnum(atoi(zone_name))) < 0) {
		std::stringstream str_log;
		str_log << "ozoneecho called for nonexistant zone: " << zone_name;
		obj_log(obj, str_log.str().c_str());
	} else {
		sprintf(buf, "%s\r\n", msg);
		send_to_zone(buf, zone);
	}
}

const struct obj_command_info obj_cmd_info[] =
	{
		{"RESERVED", 0, 0},    // this must be first -- for specprocs
		{"oat", do_oat, 0},
		{"oecho", do_oecho, 0},
		{"oechoaround", do_osend, SCMD_OECHOAROUND},
		{"oexp", do_oexp, 0},
		{"oforce", do_oforce, 0},
		{"oload", do_dgoload, 0},
		{"opurge", do_opurge, 0},
		{"osend", do_osend, SCMD_OSEND},
		{"osetval", do_osetval, 0},
		{"oteleport", do_oteleport, 0},
		{"odamage", do_odamage, 0},
		{"otimer", do_otimer, 0},
		{"otransform", do_otransform, 0},
		{"odoor", do_odoor, 0},
		{"ospellturn", do_ospellturn, 0},
		{"ospellturntemp", do_ospellturntemp, 0},
		{"ospelladd", do_ospelladd, 0},
		{"ofeatturn", do_ofeatturn, 0},
		{"oskillturn", do_oskillturn, 0},
		{"oskilladd", do_oskilladd, 0},
		{"ospellitem", do_ospellitem, 0},
		{"oportal", do_oportal, 0},
		{"ozoneecho", do_ozoneecho, 0},
		{"\n", 0, 0}        // this must be last
	};

// *  This is the command interpreter used by objects, called by script_driver.
void obj_command_interpreter(ObjectData *obj, char *argument) {
	char *line, arg[kMaxInputLength];

	skip_spaces(&argument);

	// just drop to next line for hitting CR
	if (!*argument)
		return;

	line = any_one_arg(argument, arg);

	// find the command
	int cmd = 0;
	const size_t length = strlen(arg);
	while (*obj_cmd_info[cmd].command != '\n') {
		if (!strncmp(obj_cmd_info[cmd].command, arg, length)) {
			break;
		}
		cmd++;
	}

	if (*obj_cmd_info[cmd].command == '\n') {
		sprintf(buf2, "Unknown object cmd: '%s'", argument);
		obj_log(obj, buf2, LGH);
	} else {
		const obj_command_info::handler_f &command = obj_cmd_info[cmd].command_pointer;
		command(obj, line, cmd, obj_cmd_info[cmd].subcmd);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
