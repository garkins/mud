// Copyright (c) 2014 Krodo
// Part of Bylins http://www.mud.ru

#include "obj_sets.h"

#include "obj_prototypes.h"
#include "conf.h"
#include "obj_sets_stuff.h"
#include "structs/structs.h"
#include "entities/obj.h"
#include "db.h"
#include "constants.h"
#include "handler.h"
#include "entities/char_player.h"
#include "skills.h"
#include "color.h"
#include "modify.h"
#include "magic/spells.h"
#include "utils/utils.h"
#include "classes/classes_constants.h"
#include "skills_info.h"
#include "structs/global_objects.h"

#include <boost/algorithm/string.hpp>

#include <string>
#include <vector>
#include <map>
#include <array>
#include <algorithm>

void show_weapon_affects_olc(DescriptorData *d, const FlagData &flags);
void show_apply_olc(DescriptorData *d);
int planebit(const char *str, int *plane, int *bit);

using namespace obj_sets;

namespace obj_sets_olc {

/// класс для редактирования сета в олц, содержит состояние редактирования
/// (наружу торчит только CON_SEDIT) и всю временную инфу под это дело
class sedit {
 public:
	sedit() : state(-1), new_entry(false),
			  obj_edit(-1), activ_edit(-1), apply_edit(-1) {};

	// стадия редактирования (в какой менюшке)
	int state;
	// правка глобальных сообщений
	obj_sets::SetMsgNode msg_edit;
	// редактируемый сет
	obj_sets::SetNode olc_set;
	// идет создание нового сета
	bool new_entry;

	void parse_global_msg(CharacterData *ch, const char *arg);
	void parse_activ_mage_dmg(CharacterData *ch, const char *arg);
	void parse_activ_phys_dmg(CharacterData *ch, const char *arg);
	void parse_activ_weight_val(CharacterData *ch, const char *arg);
	void parse_activ_ench_sdice(CharacterData *ch, const char *arg);
	void parse_activ_ench_ndice(CharacterData *ch, const char *arg);
	void parse_activ_ench_vnum(CharacterData *ch, const char *arg);
	void parse_activ_skill(CharacterData *ch, const char *arg);
	void parse_activ_apply_mod(CharacterData *ch, const char *arg);
	void parse_activ_apply_loc(CharacterData *ch, const char *arg);
	void parse_activ_prof(CharacterData *ch, const char *arg);
	void parse_activ_change(CharacterData *ch, const char *arg);
	void parse_activ_remove(CharacterData *ch, const char *arg);
	void parse_activ_affects(CharacterData *ch, const char *arg);
	void parse_activ_edit(CharacterData *ch, const char *arg);
	void parse_activ_add(CharacterData *ch, const char *arg);
	void parse_obj_remove(CharacterData *ch, const char *arg);
	void parse_obj_change(CharacterData *ch, const char *arg);
	void parse_obj_edit(CharacterData *ch, const char *arg);
	void parse_obj_add(CharacterData *ch, const char *arg);
	void parse_setmsg(CharacterData *ch, const char *arg);
	void parse_setcomment(CharacterData *ch, const char *arg);
	void parse_setalias(CharacterData *ch, const char *arg);
	void parse_setname(CharacterData *ch, const char *arg);
	void parse_main(CharacterData *ch, const char *arg);

	void save_olc(CharacterData *ch);
	void show_main(CharacterData *ch);
	void show_global_msg(CharacterData *ch);

 private:
	// если правится предмет - его внум
	int obj_edit;
	// если правится активатор - его размер
	int activ_edit;
	// если правится аффект - его индекс в списке
	size_t apply_edit;

	void show_activ_ench_vnum(CharacterData *ch);
	void show_activ_prof(CharacterData *ch);
	void show_activ_skill(CharacterData *ch);
	void show_activ_apply(CharacterData *ch);
	void show_activ_affects(CharacterData *ch);
	void show_activ_edit(CharacterData *ch);
	void show_obj_edit(CharacterData *ch);
	// проверка поменялось ли что-то в сете,
	// чтобы не спамить про сохранения при выходе
	bool changed();
	// для сравнения в changed()
	bool operator!=(const sedit &r) const {
		return olc_set != r.olc_set;
	}
	bool operator==(const sedit &r) const {
		return !(*this != r);
	}
};

enum {
	STATE_MAIN,
	STATE_MAIN_EXIT,
	STATE_SET_REMOVE,
	STATE_NAME,
	STATE_ALIAS,
	STATE_COMMENT,
	STATE_SETMSG_CHAR_ON,
	STATE_SETMSG_CHAR_OFF,
	STATE_SETMSG_ROOM_ON,
	STATE_SETMSG_ROOM_OFF,
	STATE_OBJ_ADD,
	STATE_OBJ_EDIT,
	STATE_OBJ_CHANGE,
	STATE_OBJMSG_CHAR_ON,
	STATE_OBJMSG_CHAR_OFF,
	STATE_OBJMSG_ROOM_ON,
	STATE_OBJMSG_ROOM_OFF,
	STATE_OBJ_REMOVE,
	STATE_TOTAL_ACTIV,
	STATE_ACTIV_ADD,
	STATE_ACTIV_EDIT,
	STATE_ACTIV_AFFECTS,
	STATE_ACTIV_REMOVE,
	STATE_ACTIV_PROF,
	STATE_ACTIV_CHANGE,
	STATE_ACTIV_APPLY_LOC,
	STATE_ACTIV_APPLY_MOD,
	STATE_ACTIV_SKILL,
	STATE_ACTIV_ENCH_VNUM,
	STATE_ACTIV_ENCH_NDICE,
	STATE_ACTIV_ENCH_SDICE,
	STATE_ACTIV_ENCH_WEIGHT,
	STATE_ACTIV_PHYS_DMG,
	STATE_ACTIV_MAGE_DMG,
	STATE_GLOBAL_MSG,
	STATE_GLOBAL_MSG_EXIT,
	STATE_GLBMSG_CHAR_ON,
	STATE_GLBMSG_CHAR_OFF,
	STATE_GLBMSG_ROOM_ON,
	STATE_GLBMSG_ROOM_OFF
};

// распечатка главного меню + смещение для следующих пунктов через MAIN_TOTAL
enum {
	MAIN_SET_REMOVE = 1,
	MAIN_ENABLED,
	MAIN_NAME,
	MAIN_ALIAS,
	MAIN_COMMENT,
	MAIN_MSG_CHAR_ON,
	MAIN_MSG_CHAR_OFF,
	MAIN_MSG_ROOM_ON,
	MAIN_MSG_ROOM_OFF,
	MAIN_TOTAL
};

namespace {
const auto MISSING_OBJECT_NAME = "&R<объект с таким VNUM не существует>&n";
}

/// распечатка форматированного списка шмоток сета, форматирование идет как
/// как по столбцам, так и по длине имени и внума шмоток, вобщем чтоб красиво
std::string main_menu_objlist(CharacterData *ch, const SetNode &set, int menu) {
	std::string out;
	char buf_[128];
	char format[128];
	char buf_vnum[128];
	size_t r_max_name = 0, r_max_vnum = 0;
	size_t l_max_name = 0, l_max_vnum = 0;
	bool left = true;

	std::list<std::pair<int, const char *>> rnum_list;
	for (const auto &i : set.obj_list) {
		const auto rnum = real_object(i.first);
		const auto name = rnum < 0
						  ? MISSING_OBJECT_NAME
						  : obj_proto[rnum]->get_short_description().c_str();
		const size_t curr_name = strlen_no_colors(name);
		snprintf(buf_vnum, sizeof(buf_vnum), "%d", i.first);
		const size_t curr_vnum = strlen(buf_vnum);

		if (left) {
			l_max_name = std::max(l_max_name, curr_name);
			l_max_vnum = std::max(l_max_vnum, curr_vnum);
		} else {
			r_max_name = std::max(r_max_name, curr_name);
			r_max_vnum = std::max(r_max_vnum, curr_vnum);
		}
		rnum_list.emplace_back(i.first, name);
		left = !left;
	}

	left = true;
	for (const auto &i : rnum_list) {
		if (MISSING_OBJECT_NAME == i.second) {
			snprintf(buf_vnum, sizeof(buf_vnum), "&Y%d&n", i.first);
		} else {
			snprintf(buf_vnum, sizeof(buf_vnum), "%d", i.first);
		}

		snprintf(format, sizeof(format), "%s%2d%s) %s : %s%%-%zus%s   ",
				 CCGRN(ch, C_NRM), menu++, CCNRM(ch, C_NRM),
				 colored_name(i.second, left ? l_max_name : r_max_name, true),
				 CCCYN(ch, C_NRM), (left ? l_max_vnum : r_max_vnum), CCNRM(ch, C_NRM));
		snprintf(buf_, sizeof(buf_), format, buf_vnum);
		out += buf_;
		if (!left) {
			out += "\r\n";
		}
		left = !left;
	}

	if (!out.empty()) {
		boost::trim_right(out);
		return out + ("\r\n");
	}
	return out;
}

const char *main_menu_str(CharacterData *ch, SetNode &olc_set, int num) {
	static char buf_[1024];
	switch (num) {
		case MAIN_SET_REMOVE: return "Удалить набор";
		case MAIN_ENABLED:
			snprintf(buf_, sizeof(buf_), "Статус : %s%s%s", CCCYN(ch, C_NRM),
					 olc_set.enabled ? "активен" : "неактивен", CCNRM(ch, C_NRM));
			break;
		case MAIN_NAME:
			snprintf(buf_, sizeof(buf_), "Имя    : %s",
					 olc_set.name.c_str());
			break;
		case MAIN_ALIAS:
			snprintf(buf_, sizeof(buf_), "Алиасы : %s",
					 olc_set.alias.c_str());
			break;
		case MAIN_COMMENT:
			snprintf(buf_, sizeof(buf_), "Комментарий : %s",
					 olc_set.comment.c_str());
			break;
		case MAIN_MSG_CHAR_ON:
			snprintf(buf_, sizeof(buf_), "Активатор персонажу   : %s",
					 olc_set.messages.char_on_msg.c_str());
			break;
		case MAIN_MSG_CHAR_OFF:
			snprintf(buf_, sizeof(buf_), "Деактиватор персонажу : %s",
					 olc_set.messages.char_off_msg.c_str());
			break;
		case MAIN_MSG_ROOM_ON:
			snprintf(buf_, sizeof(buf_), "Активатор в комнату   : %s",
					 olc_set.messages.room_on_msg.c_str());
			break;
		case MAIN_MSG_ROOM_OFF:
			snprintf(buf_, sizeof(buf_), "Деактиватор в комнату : %s",
					 olc_set.messages.room_off_msg.c_str());
			break;
		default: return "<error>";
	}
	return buf_;
}

void sedit::show_main(CharacterData *ch) {
	state = STATE_MAIN;

	char buf_[1024];
	std::string out("\r\n");

	if (new_entry) {
		out += "Создание нового набора предметов\r\n";
	} else {
		size_t idx = setidx_by_uid(olc_set.uid);
		if (idx >= sets_list.size()) {
			send_to_char("Редактирование прервано: набор был удален.\r\n", ch);
			ch->desc->sedit.reset();
			STATE(ch->desc) = CON_PLAYING;
			return;
		}
		snprintf(buf_, sizeof(buf_),
				 "Редактирование набора предметов #%zu\r\n", idx + 1);
		out += buf_;
	}
	int i = 1;
	for (/**/; i < MAIN_TOTAL; ++i) {
		snprintf(buf_, sizeof(buf_), "%s%2d%s) %s\r\n",
				 CCGRN(ch, C_NRM), i, CCNRM(ch, C_NRM),
				 main_menu_str(ch, olc_set, i));
		out += buf_;
	}
	// предметы
	snprintf(buf_, sizeof(buf_), "\r\n%s%2d%s) Добавить предмет(ы)\r\n",
			 CCGRN(ch, C_NRM), i++, CCNRM(ch, C_NRM));
	out += buf_;
	out += main_menu_objlist(ch, olc_set, i);
	i += static_cast<int>(olc_set.obj_list.size());
	// активаторы
	snprintf(buf_, sizeof(buf_),
			 "\r\n%s%2d%s) Распечатать сумму активаторов\r\n",
			 CCGRN(ch, C_NRM), i++, CCNRM(ch, C_NRM));
	out += buf_;
	snprintf(buf_, sizeof(buf_), "%s%2d%s) Добавить активатор\r\n",
			 CCGRN(ch, C_NRM), i++, CCNRM(ch, C_NRM));
	out += buf_;
	for (auto & k : olc_set.activ_list) {
		if (!k.second.prof.all()) {
			std::string prof;
			print_bitset(k.second.prof, pc_class_name, ",", prof);
			snprintf(buf_, sizeof(buf_),
					 "%s%2d%s) Редактировать активатор: %s%d (%s)%s\r\n",
					 CCGRN(ch, C_NRM), i++, CCNRM(ch, C_NRM),
					 CCCYN(ch, C_NRM), k.first, prof.c_str(),
					 CCNRM(ch, C_NRM));
		} else {
			snprintf(buf_, sizeof(buf_),
					 "%s%2d%s) Редактировать активатор: %s%d%s\r\n",
					 CCGRN(ch, C_NRM), i++, CCNRM(ch, C_NRM),
					 CCCYN(ch, C_NRM), k.first, CCNRM(ch, C_NRM));
		}
		out += buf_;
	}

	snprintf(buf_, sizeof(buf_),
			 "\r\n%s%2d%s) Выйти Q(В)\r\n"
			 "Ваш выбор : ",
			 CCGRN(ch, C_NRM), i++, CCNRM(ch, C_NRM));
	out += buf_;

	send_to_char(out, ch);
}

void sedit::show_obj_edit(CharacterData *ch) {
	state = STATE_OBJ_EDIT;

	auto obj = olc_set.obj_list.find(obj_edit);
	if (obj == olc_set.obj_list.end()) {
		send_to_char(ch,
					 "Ошибка: предмет не в наборе %s:%d (%s).\r\n",
					 __FILE__, __LINE__, __func__);
		show_main(ch);
		return;
	}
	const SetMsgNode &msg = obj->second;

	const auto rnum = real_object(obj_edit);
	const auto name = rnum < 0
					  ? MISSING_OBJECT_NAME
					  : obj_proto[rnum]->get_short_description().c_str();
	char buf_[2048];
	snprintf(buf_, sizeof(buf_),
			 "\r\nРедактирование предмета '%s'\r\n"
			 "%s 1%s) Удалить из набора\r\n"
			 "%s 2%s) Виртуальный номер (vnum) : %s%d%s\r\n"
			 "%s 3%s) Активатор персонажу   : %s\r\n"
			 "%s 4%s) Деактиватор персонажу : %s\r\n"
			 "%s 5%s) Активатор в комнату   : %s\r\n"
			 "%s 6%s) Деактиватор в комнату : %s\r\n"
			 "%s 7%s) В главное меню Q(В)\r\n"
			 "Ваш выбор : ",
			 name,
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			 (rnum < 0 ? CCYEL(ch, C_NRM) : CCCYN(ch, C_NRM)), obj_edit, CCNRM(ch, C_NRM),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), msg.char_on_msg.c_str(),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), msg.char_off_msg.c_str(),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), msg.room_on_msg.c_str(),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), msg.room_off_msg.c_str(),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	send_to_char(buf_, ch);
}

void sedit::show_activ_edit(CharacterData *ch) {
	state = STATE_ACTIV_EDIT;

	auto i = olc_set.activ_list.find(activ_edit);
	if (i == olc_set.activ_list.end()) {
		send_to_char(ch, "Ошибка: активатор не найден %s:%d (%s).\r\n", __FILE__, __LINE__, __func__);
		show_main(ch);
		return;
	}
	const ActivNode &activ = i->second;

	char buf_aff[2048];
	activ.affects.sprintbits(weapon_affects, buf_aff, ",");
	std::string aff_str = line_split_str(buf_aff, ",", 80, 14);
	std::string prof_str;
	if (!activ.prof.all()) {
		print_bitset(activ.prof, pc_class_name, ",", prof_str);
	} else {
		prof_str = "все";
	}

	std::string out;
	char buf_[2048];
	snprintf(buf_, sizeof(buf_),
			 "\r\nРедактирование активатора\r\n"
			 "%s 1%s) Удалить из набора\r\n"
			 "%s 2%s) Количество предметов : %s%d%s\r\n"
			 "%s 3%s) Профессии : %s%s%s\r\n"
			 "%s 4%s) Аффекты : %s%s%s\r\n",
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			 CCCYN(ch, C_NRM), activ_edit, CCNRM(ch, C_NRM),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			 CCCYN(ch, C_NRM), prof_str.c_str(), CCNRM(ch, C_NRM),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			 CCCYN(ch, C_NRM), aff_str.c_str(), CCNRM(ch, C_NRM));
	out += buf_;

	int cnt = 5;
	for (auto apply : activ.apply) {
		if (apply.location > 0) {
			snprintf(buf_, sizeof(buf_), "%s%2d%s) Наводимый аффект : %s",
					 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM),
					 print_obj_affects(apply).c_str());
		} else {
			snprintf(buf_, sizeof(buf_),
					 "%s%2d%s) Наводимый аффект : %s%s%s\r\n",
					 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM),
					 CCCYN(ch, C_NRM), "ничего", CCNRM(ch, C_NRM));
		}
		out += buf_;
	}

	if (MUD::Skills().IsValid(activ.skill.first)) {
		snprintf(buf_, sizeof(buf_),
				 "%s%2d%s) Изменяемое умение : %s%+d to %s%s\r\n",
				 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
				 activ.skill.second, MUD::Skills()[activ.skill.first].GetName(),
				 CCNRM(ch, C_NRM));
	} else {
		snprintf(buf_, sizeof(buf_), "%s%2d%s) Изменяемое умение : нет\r\n",
				 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM));
	}
	out += buf_;

	if (activ.enchant.first > 0) {
		const int rnum = real_object(activ.enchant.first);
		const char *name =
			(rnum >= 0 ? obj_proto[rnum]->get_short_description().c_str() : "<null>");
		if (GET_OBJ_TYPE(obj_proto[rnum]) == ObjectData::ITEM_WEAPON) {
			snprintf(buf_, sizeof(buf_),
					 "%s%2d%s) Зачарование предмета : %s[%d] %s вес %+d, кубики %+dD%+d%s\r\n",
					 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
					 activ.enchant.first, name, activ.enchant.second.weight,
					 activ.enchant.second.ndice, activ.enchant.second.sdice,
					 CCNRM(ch, C_NRM));
		} else {
			snprintf(buf_, sizeof(buf_),
					 "%s%2d%s) Зачарование предмета : %s[%d] %s вес %+d%s\r\n",
					 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
					 activ.enchant.first, name, activ.enchant.second.weight,
					 CCNRM(ch, C_NRM));
		}
	} else {
		snprintf(buf_, sizeof(buf_), "%s%2d%s) Зачарование предмета: нет\r\n",
				 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM));
	}
	out += buf_;

	snprintf(buf_, sizeof(buf_),
			 "%s%2d%s) Увеличение физ. урона : %s%+d%%%s\r\n",
			 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM),
			 CCCYN(ch, C_NRM), activ.bonus.phys_dmg, CCNRM(ch, C_NRM));
	out += buf_;

	snprintf(buf_, sizeof(buf_),
			 "%s%2d%s) Увеличение маг. урона : %s%+d%%%s\r\n",
			 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM),
			 CCCYN(ch, C_NRM), activ.bonus.mage_dmg, CCNRM(ch, C_NRM));
	out += buf_;

	snprintf(buf_, sizeof(buf_),
			 "%s%2d%s) Актив на мобах : %s%s%s\r\n",
			 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM),
			 CCCYN(ch, C_NRM), (activ.npc ? "Да" : "Нет"), CCNRM(ch, C_NRM));
	out += buf_;

	snprintf(buf_, sizeof(buf_),
			 "%s%2d%s) В главное меню Q(В)\r\n"
			 "Ваш выбор : ",
			 CCGRN(ch, C_NRM), cnt++, CCNRM(ch, C_NRM));
	out += buf_;

	send_to_char(out, ch);
}

bool sedit::changed() {
	const size_t idx = setidx_by_uid(olc_set.uid);
	if (idx < sets_list.size() && *(sets_list.at(idx)) != olc_set) {
		return true;
	}
	return false;
}

void sedit::save_olc(CharacterData *ch) {
	if (new_entry) {
		std::shared_ptr<SetNode> set_ptr = std::make_shared<SetNode>(olc_set);
		sets_list.push_back(set_ptr);
		VerifySet(*set_ptr);
		save();
		init_obj_index();
		return;
	}

	const size_t idx = setidx_by_uid(olc_set.uid);
	if (idx < sets_list.size()) {
		*(sets_list.at(idx)) = olc_set;
		VerifySet(*(sets_list.at(idx)));
		save();
		init_obj_index();
	} else {
		send_to_char("Ошибка сохранения: набор был удален.", ch);
	}
}

void parse_main_exit(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	switch (*arg) {
		case 'y':
		case 'Y':
		case 'д':
		case 'Д': STATE(ch->desc) = CON_PLAYING;
			ch->desc->sedit->save_olc(ch);
			ch->desc->sedit.reset();
			send_to_char("Изменения сохранены.\r\n", ch);
			break;
		case 'n':
		case 'N':
		case 'н':
		case 'Н': ch->desc->sedit.reset();
			STATE(ch->desc) = CON_PLAYING;
			send_to_char("Редактирование отменено.\r\n", ch);
			break;
		default:
			send_to_char(
				"Неверный выбор!\r\n"
				"Вы хотите сохранить изменения? Y(Д)/N(Н) : ", ch);
			break;
	}
}

void parse_set_remove(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	switch (*arg) {
		case 'y':
		case 'Y':
		case 'д':
		case 'Д': {
			for (auto i = sets_list.begin(); i != sets_list.end(); ++i) {
				if ((*i)->uid == ch->desc->sedit->olc_set.uid) {
					sets_list.erase(i);
					save();
					init_obj_index();
					send_to_char("Набор удален.\r\n", ch);
					break;
				}
			}
			STATE(ch->desc) = CON_PLAYING;
			ch->desc->sedit.reset();
			break;
		}
		case 'n':
		case 'N':
		case 'н':
		case 'Н': send_to_char("Удаление отменено.\r\n", ch);
			ch->desc->sedit->show_main(ch);
			break;
		default:
			send_to_char(
				"Неверный выбор!\r\n"
				"Подтвердите удаление набора Y(Д)/N(Н) : ", ch);
			break;
	}
}

void sedit::parse_obj_remove(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	switch (*arg) {
		case 'y':
		case 'Y':
		case 'д':
		case 'Д': {
			auto i = olc_set.obj_list.find(obj_edit);
			if (i != olc_set.obj_list.end()) {
				olc_set.obj_list.erase(i);
				send_to_char("Предмет удален из набора.\r\n", ch);
			}
			obj_edit = -1;
			show_main(ch);
			break;
		}
		case 'n':
		case 'N':
		case 'н':
		case 'Н': send_to_char("Удаление отменено.\r\n", ch);
			show_obj_edit(ch);
			break;
		default: send_to_char("Неверный выбор!\r\n", ch);
			send_to_char("Подтвердите удаление предмета Y(Д)/N(Н) :", ch);
			break;
	}
}

void sedit::parse_activ_remove(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	switch (*arg) {
		case 'y':
		case 'Y':
		case 'д':
		case 'Д': {
			auto i = olc_set.activ_list.find(activ_edit);
			if (i != olc_set.activ_list.end()) {
				olc_set.activ_list.erase(i);
				send_to_char("Активатор удален из набора.\r\n", ch);
			}
			activ_edit = -1;
			show_main(ch);
			break;
		}
		case 'n':
		case 'N':
		case 'н':
		case 'Н': send_to_char("Удаление отменено.\r\n", ch);
			show_activ_edit(ch);
			break;
		default: send_to_char("Неверный выбор!\r\n", ch);
			send_to_char("Подтвердите удаление активатора Y(Д)/N(Н) :", ch);
			break;
	}
}

void sedit::show_global_msg(CharacterData *ch) {
	state = STATE_GLOBAL_MSG;

	char buf_[1024];
	snprintf(buf_, sizeof(buf_),
			 "\r\nРедактирование глобальных сообщений\r\n"
			 "%s 1%s) Активатор персонажу   : %s\r\n"
			 "%s 2%s) Деактиватор персонажу : %s\r\n"
			 "%s 3%s) Активатор в комнату   : %s\r\n"
			 "%s 4%s) Деактиватор в комнату : %s\r\n"
			 "%s 5%s) Выйти Q(В)\r\n"
			 "Ваш выбор : ",
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), msg_edit.char_on_msg.c_str(),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), msg_edit.char_off_msg.c_str(),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), msg_edit.room_on_msg.c_str(),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), msg_edit.room_off_msg.c_str(),
			 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	send_to_char(buf_, ch);
}

void sedit::parse_global_msg(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	if (!*arg) {
		send_to_char("Неверный выбор!\r\n", ch);
		show_global_msg(ch);
		return;
	}
	if (!a_isdigit(*arg)) {
		switch (*arg) {
			case 'Q':
			case 'q':
			case 'В':
			case 'в':
				if (msg_edit != global_msg) {
					send_to_char("Вы хотите сохранить изменения? Y(Д)/N(Н) : ", ch);
					state = STATE_GLOBAL_MSG_EXIT;
				} else {
					send_to_char("Редактирование отменено.\r\n", ch);
					ch->desc->sedit.reset();
					STATE(ch->desc) = CON_PLAYING;
				}
				break;
			default: send_to_char("Неверный выбор!\r\n", ch);
				show_global_msg(ch);
				break;
		}
		return;
	}

	unsigned num = atoi(arg);

	switch (num) {
		case 1: send_to_char("Сообщение персонажу при активации : ", ch);
			state = STATE_GLBMSG_CHAR_ON;
			break;
		case 2: send_to_char("Сообщение персонажу при деактивации : ", ch);
			state = STATE_GLBMSG_CHAR_OFF;
			break;
		case 3: send_to_char("Сообщение в комнату при активации : ", ch);
			state = STATE_GLBMSG_ROOM_ON;
			break;
		case 4: send_to_char("Сообщение в комнату при деактивации : ", ch);
			state = STATE_GLBMSG_ROOM_OFF;
			break;
		case 5:
			if (msg_edit != global_msg) {
				send_to_char("Вы хотите сохранить изменения? Y(Д)/N(Н) : ", ch);
				state = STATE_GLOBAL_MSG_EXIT;
			} else {
				send_to_char("Редактирование отменено.\r\n", ch);
				ch->desc->sedit.reset();
				STATE(ch->desc) = CON_PLAYING;
			}
			break;
		default: send_to_char("Неверный выбор!\r\n", ch);
			show_global_msg(ch);
			break;
	}
}

void parse_global_msg_exit(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	switch (*arg) {
		case 'y':
		case 'Y':
		case 'д':
		case 'Д': STATE(ch->desc) = CON_PLAYING;
			global_msg = ch->desc->sedit->msg_edit;
			save();
			ch->desc->sedit.reset();
			send_to_char("Изменения сохранены.\r\n", ch);
			break;
		case 'n':
		case 'N':
		case 'н':
		case 'Н': ch->desc->sedit.reset();
			STATE(ch->desc) = CON_PLAYING;
			send_to_char("Редактирование отменено.\r\n", ch);
			break;
		default:
			send_to_char(
				"Неверный выбор!\r\n"
				"Вы хотите сохранить изменения? Y(Д)/N(Н) : ", ch);
			break;
	}
}

void sedit::parse_main(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	if (!*arg) {
		send_to_char("Неверный выбор!\r\n", ch);
		show_main(ch);
		return;
	}
	if (!a_isdigit(*arg)) {
		switch (*arg) {
			case 'Q':
			case 'q':
			case 'В':
			case 'в':
				if (new_entry || changed()) {
					send_to_char("Вы хотите сохранить изменения? Y(Д)/N(Н) : ", ch);
					state = STATE_MAIN_EXIT;
				} else {
					send_to_char("Редактирование отменено.\r\n", ch);
					ch->desc->sedit.reset();
					STATE(ch->desc) = CON_PLAYING;
				}
				break;
			default: send_to_char("Неверный выбор!\r\n", ch);
				show_main(ch);
				break;
		}
		return;
	}

	unsigned num = atoi(arg);

	switch (num) {
		case MAIN_SET_REMOVE: send_to_char("Подтвердите удаление набора Y(Д)/N(Н) : ", ch);
			state = STATE_SET_REMOVE;
			return;
		case MAIN_ENABLED: olc_set.enabled = !olc_set.enabled;
			show_main(ch);
			return;
		case MAIN_NAME: send_to_char("Имя набора (опознание, справка сеты) : ", ch);
			state = STATE_NAME;
			return;
		case MAIN_ALIAS:
			send_to_char(
				"Алиасы набора для справки (через запятую и/или пробел) : ", ch);
			state = STATE_ALIAS;
			return;
		case MAIN_COMMENT: send_to_char("Ваш комментарий до 40 символов (виден по slist) : ", ch);
			state = STATE_COMMENT;
			return;
		case MAIN_MSG_CHAR_ON: send_to_char("Сообщение персонажу при активации : ", ch);
			state = STATE_SETMSG_CHAR_ON;
			return;
		case MAIN_MSG_CHAR_OFF: send_to_char("Сообщение персонажу при деактивации : ", ch);
			state = STATE_SETMSG_CHAR_OFF;
			return;
		case MAIN_MSG_ROOM_ON: send_to_char("Сообщение в комнату при активации : ", ch);
			state = STATE_SETMSG_ROOM_ON;
			return;
		case MAIN_MSG_ROOM_OFF: send_to_char("Сообщение в комнату при деактивации : ", ch);
			state = STATE_SETMSG_ROOM_OFF;
			return;
		default: break;
	}

	const unsigned NUM_ADD_OBJ = MAIN_TOTAL;
	const unsigned NUM_TOTAL_ACTIV = NUM_ADD_OBJ + static_cast<unsigned>(olc_set.obj_list.size()) + 1;
	const unsigned NUM_ADD_ACTIV = NUM_TOTAL_ACTIV + 1;
	const unsigned NUM_QUIT = NUM_ADD_ACTIV + static_cast<unsigned>(olc_set.activ_list.size()) + 1;

	// после статичного меню идут предметы, за ними активаторы
	if (num == NUM_ADD_OBJ) {
		send_to_char(
			"Vnum добавляемого предмета (несколько через пробел) : ", ch);
		state = STATE_OBJ_ADD;
	} else if (!olc_set.obj_list.empty()
		&& num > NUM_ADD_OBJ
		&& num <= NUM_ADD_OBJ + olc_set.obj_list.size()) {
		// редактирование предмета
		const unsigned offset = NUM_ADD_OBJ + 1;
		auto i = olc_set.obj_list.begin();
		if (num > offset) {
			std::advance(i, num - offset);
		}
		obj_edit = i->first;
		show_obj_edit(ch);
	} else if (num == NUM_TOTAL_ACTIV) {
		state = STATE_TOTAL_ACTIV;
		send_to_char(print_total_activ(olc_set), ch);
		send_to_char("Введите любую команду для продолжения : ", ch);
	} else if (num == NUM_ADD_ACTIV) {
		send_to_char(ch,
					 "Укажите кол-во предметов для активации (%u-%u) : ",
					 MIN_ACTIVE_SIZE, MAX_ACTIVE_SIZE);
		state = STATE_ACTIV_ADD;
	} else if (!olc_set.activ_list.empty()
		&& num > NUM_ADD_ACTIV
		&& num <= NUM_ADD_ACTIV + olc_set.activ_list.size()) {
		// редактирование активатора
		const unsigned offset = NUM_ADD_ACTIV + 1;
		auto i = olc_set.activ_list.begin();
		if (num > offset) {
			std::advance(i, num - offset);
		}
		activ_edit = i->first;
		show_activ_edit(ch);
	} else if (num == NUM_QUIT) {
		if (new_entry || changed()) {
			send_to_char("Вы хотите сохранить изменения? Y(Д)/N(Н) : ", ch);
			state = STATE_MAIN_EXIT;
		} else {
			send_to_char("Редактирование отменено.\r\n", ch);
			ch->desc->sedit.reset();
			STATE(ch->desc) = CON_PLAYING;
		}
	} else {
		send_to_char("Неверный выбор!\r\n", ch);
		show_main(ch);
	}
}

void sedit::parse_setname(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	if (!*arg) {
		send_to_char("Имя набора удалено.\r\n", ch);
		olc_set.name.clear();
	} else {
		olc_set.name = arg;
	}
	show_main(ch);
}

void sedit::parse_setalias(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	if (!*arg) {
		send_to_char("Алиас набора удален.\r\n", ch);
		olc_set.alias.clear();
	} else {
		olc_set.alias = arg;
	}
	show_main(ch);
}

void sedit::parse_setcomment(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	if (!*arg) {
		send_to_char("Комментарий набора удален.\r\n", ch);
		olc_set.comment.clear();
	} else {
		olc_set.comment = arg;
		olc_set.comment = olc_set.comment.substr(0, 40);
	}
	show_main(ch);
}

/// чтобы в одном месте срау обработать все три вида сообщений
enum { PARSE_GLB_MSG, PARSE_SET_MSG, PARSE_OBJ_MSG };

void sedit::parse_setmsg(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	int parse_type = PARSE_GLB_MSG;

	switch (state) {
		case STATE_SETMSG_CHAR_ON:
		case STATE_SETMSG_CHAR_OFF:
		case STATE_SETMSG_ROOM_ON:
		case STATE_SETMSG_ROOM_OFF: parse_type = PARSE_SET_MSG;
			break;
		case STATE_OBJMSG_CHAR_ON:
		case STATE_OBJMSG_CHAR_OFF:
		case STATE_OBJMSG_ROOM_ON:
		case STATE_OBJMSG_ROOM_OFF: parse_type = PARSE_OBJ_MSG;
			break;
		default: break;
	}

	SetMsgNode &msg = parse_type == PARSE_SET_MSG
					? olc_set.messages : parse_type == PARSE_OBJ_MSG
										 ? olc_set.obj_list.at(obj_edit) : msg_edit;

	switch (state) {
		case STATE_SETMSG_CHAR_ON:
		case STATE_OBJMSG_CHAR_ON:
		case STATE_GLBMSG_CHAR_ON:
			if ((msg.char_on_msg = arg).empty())
				send_to_char("Сообщение активатора персонажу удалено.\r\n", ch);
			break;
		case STATE_SETMSG_CHAR_OFF:
		case STATE_OBJMSG_CHAR_OFF:
		case STATE_GLBMSG_CHAR_OFF:
			if ((msg.char_off_msg = arg).empty())
				send_to_char("Сообщение деактиватора персонажу удалено.\r\n", ch);
			break;
		case STATE_SETMSG_ROOM_ON:
		case STATE_OBJMSG_ROOM_ON:
		case STATE_GLBMSG_ROOM_ON:
			if ((msg.room_on_msg = arg).empty())
				send_to_char("Сообщение активатора в комнату удалено.\r\n", ch);
			break;
		case STATE_SETMSG_ROOM_OFF:
		case STATE_OBJMSG_ROOM_OFF:
		case STATE_GLBMSG_ROOM_OFF:
			if ((msg.room_off_msg = arg).empty())
				send_to_char("Сообщение деактиватора в комнату удалено.\r\n", ch);
			break;
		default: break;
	}

	if (parse_type == PARSE_OBJ_MSG) {
		show_obj_edit(ch);
	} else if (parse_type == PARSE_SET_MSG) {
		show_main(ch);
	} else {
		show_global_msg(ch);
	}
}

void sedit::parse_activ_add(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	unsigned num = 0;
	if (!*arg || !a_isdigit(*arg)
		|| (num = atoi(arg)) < MIN_ACTIVE_SIZE
		|| num > MAX_ACTIVE_SIZE) {
		send_to_char("Некорректное кол-во предметов для активации.\r\n", ch);
		show_main(ch);
		return;
	}
	auto i = olc_set.activ_list.find(num);
	if (i != olc_set.activ_list.end()) {
		send_to_char(ch, "В наборе уже есть активатор на %d %s.\r\n",
					 num, desc_count(num, WHAT_OBJECT));
	} else {
		send_to_char(ch, "Активатор на %d %s добавлен в набор.\r\n",
					 num, desc_count(num, WHAT_OBJECT));
		ActivNode node;
		// GCC 4.4
		//olc_set.activ_list.emplace(num, node);
		olc_set.activ_list.insert(std::make_pair(num, node));
	}
	show_main(ch);
}

void sedit::show_activ_ench_vnum(CharacterData *ch) {
	state = STATE_ACTIV_ENCH_VNUM;
	std::string out = main_menu_objlist(ch, olc_set, 1);
	out += "Укажите vnum предмета (0 - удалить и выйти, пустой ввод - выход) :";
	send_to_char(out, ch);
}

void sedit::parse_activ_ench_vnum(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);

	if (!*arg || !a_isdigit(*arg)) {
		show_activ_edit(ch);
		return;
	}

	int rnum = -1;
	unsigned vnum = atoi(arg);
	if (vnum == 0) {
		olc_set.activ_list.at(activ_edit).enchant.first = 0;
		olc_set.activ_list.at(activ_edit).enchant.second.weight = 0;
		olc_set.activ_list.at(activ_edit).enchant.second.ndice = 0;
		olc_set.activ_list.at(activ_edit).enchant.second.sdice = 0;
		show_activ_edit(ch);
		return;
	}

	if (!olc_set.obj_list.empty()
		&& vnum <= olc_set.obj_list.size()
		&& vnum >= 1) {
		auto i = olc_set.obj_list.begin();
		int add = vnum - 1;
		if (add > 0) {
			std::advance(i, add);
		}
		vnum = i->first;
		rnum = real_object(vnum);
	} else {
		rnum = real_object(vnum);
		if (rnum < 0) {
			send_to_char(ch, "Предметов с vnum %d не существует.\r\n", vnum);
			show_activ_ench_vnum(ch);
			return;
		} else if (olc_set.obj_list.find(vnum) == olc_set.obj_list.end()) {
			send_to_char(ch,
						 "В данном наборе нет предмета с vnum %d.\r\n", vnum);
			show_activ_ench_vnum(ch);
			return;
		}
	}

	olc_set.activ_list.at(activ_edit).enchant.first = vnum;

	if (rnum >= 0
		&& GET_OBJ_TYPE(obj_proto[rnum]) == ObjectData::ITEM_WEAPON) {
		state = STATE_ACTIV_ENCH_NDICE;
		send_to_char("Укажите изменение бросков кубика (0 - без изменений) :", ch);
	} else {
		olc_set.activ_list.at(activ_edit).enchant.second.ndice = 0;
		olc_set.activ_list.at(activ_edit).enchant.second.sdice = 0;
		state = STATE_ACTIV_ENCH_WEIGHT;
		send_to_char("Укажите прибавляемый вес (0 - без изменений) :", ch);
	}
}

void sedit::parse_activ_weight_val(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);

	const int weight = atoi(arg);
	olc_set.activ_list.at(activ_edit).enchant.second.weight =
		std::max(std::min(weight, 100), -100);

	show_activ_edit(ch);
}

void sedit::parse_activ_ench_ndice(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);

	const int ndice = atoi(arg);
	olc_set.activ_list.at(activ_edit).enchant.second.ndice =
		std::max(std::min(ndice, 100), -100);

	state = STATE_ACTIV_ENCH_SDICE;
	send_to_char("Укажите изменение граней кубиков (0 - без изменений) :", ch);
}

void sedit::parse_activ_ench_sdice(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);

	const int sdice = atoi(arg);
	olc_set.activ_list.at(activ_edit).enchant.second.sdice =
		std::max(std::min(sdice, 100), -100);

	state = STATE_ACTIV_ENCH_WEIGHT;
	send_to_char("Укажите прибавляемый вес (0 - без изменений) :", ch);
}

void sedit::parse_obj_add(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);

	if (!*arg || !a_isdigit(*arg)) {
		send_to_char("Некорректный виртуальный номер предмета.\r\n", ch);
		show_main(ch);
		return;
	}

	std::vector<std::string> vnum_list;
	boost::split(vnum_list, arg, boost::is_any_of(" "),
				 boost::token_compress_on);
	for (auto i = vnum_list.begin(); i != vnum_list.end(); ++i) {
		const int vnum = atoi(i->c_str());
		const int rnum = real_object(vnum);
		if (olc_set.obj_list.size() >= MAX_OBJ_LIST) {
			send_to_char(
				"Набор уже содержит максимальное кол-во предметов.\r\n", ch);
		} else if (rnum < 0) {
			send_to_char(ch, "Предметов с vnum %d не существует.\r\n", vnum);
		} else if (is_duplicate(olc_set.uid, vnum)) {
			send_to_char(ch,
						 "Предмет '%s' уже является частью другого набора.\r\n",
						 obj_proto[rnum]->get_short_description().c_str());
		} else if (!verify_wear_flag(obj_proto[rnum])) {
			send_to_char(ch,
						 "Предмет '%s' имеет запрещенный слот для надевания.\r\n",
						 obj_proto[rnum]->get_short_description().c_str());
		} else {
			SetMsgNode empty_msg;
			// GCC 4.4
			//olc_set.obj_list.emplace(vnum, empty_msg);
			olc_set.obj_list.insert(std::make_pair(vnum, empty_msg));
			send_to_char(ch,
						 "Предмет '%s' добавлен в набор.\r\n",
						 obj_proto[rnum]->get_short_description().c_str());
		}
	}

	show_main(ch);
}

void sedit::parse_obj_edit(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	if (!*arg) {
		send_to_char("Неверный выбор!\r\n", ch);
		show_obj_edit(ch);
		return;
	}
	if (!a_isdigit(*arg)) {
		switch (*arg) {
			case 'Q':
			case 'q':
			case 'В':
			case 'в': show_main(ch);
				break;
			default: send_to_char("Неверный выбор!\r\n", ch);
				show_obj_edit(ch);
				break;
		}
		return;
	}

	unsigned num = atoi(arg);
	switch (num) {
		case 1: send_to_char("Подтвердите удаление предмета Y(Д)/N(Н) :", ch);
			state = STATE_OBJ_REMOVE;
			break;
		case 2: send_to_char("Виртуальный номер нового предмета : ", ch);
			state = STATE_OBJ_CHANGE;
			break;
		case 3: send_to_char("Сообщение персонажу при активации : ", ch);
			state = STATE_OBJMSG_CHAR_ON;
			break;
		case 4: send_to_char("Сообщение персонажу при деактивации : ", ch);
			state = STATE_OBJMSG_CHAR_OFF;
			break;
		case 5: send_to_char("Сообщение в комнату при активации : ", ch);
			state = STATE_OBJMSG_ROOM_ON;
			break;
		case 6: send_to_char("Сообщение в комнату при деактивации : ", ch);
			state = STATE_OBJMSG_ROOM_OFF;
			break;
		case 7: show_main(ch);
			break;
		default: send_to_char("Неверный выбор!\r\n", ch);
			show_obj_edit(ch);
			break;
	}
}

void sedit::show_activ_affects(CharacterData *ch) {
	state = STATE_ACTIV_AFFECTS;
	show_weapon_affects_olc(ch->desc,
							olc_set.activ_list.at(activ_edit).affects);
}

void sedit::show_activ_apply(CharacterData *ch) {
	state = STATE_ACTIV_APPLY_LOC;
	show_apply_olc(ch->desc);
}

void sedit::show_activ_skill(CharacterData *ch) {
	state = STATE_ACTIV_SKILL;

	int col = 0;
	std::string out;
	char buf_[128];

	for (auto i = ESkill::kFirst; i <= ESkill::kLast; ++i) {
		if (MUD::Skills().IsInvalid(i)) {
			continue;
		}
		snprintf(buf_, sizeof(buf_), "%s%3d%s) %25s     %s",
				 CCGRN(ch, C_NRM), to_underlying(i), CCNRM(ch, C_NRM),
				 MUD::Skills()[i].GetName(), !(++col % 2) ? "\r\n" : "");
		out += buf_;
	}
	send_to_char(out, ch);
	send_to_char(
		"\r\nУкажите номер и уровень владения умением (0 - конец) : ", ch);
}

void sedit::show_activ_prof(CharacterData *ch) {
	state = STATE_ACTIV_PROF;
	char buf_[128];
	std::string out;
	std::bitset<kNumPlayerClasses> &bits = olc_set.activ_list.at(activ_edit).prof;

	for (size_t i = 0; i < bits.size(); ++i) {
		snprintf(buf_, sizeof(buf_), "%s%2zu%s) %s\r\n",
				 CCGRN(ch, C_NRM), i + 1, CCNRM(ch, C_NRM),
				 i < pc_class_name.size() ? pc_class_name.at(i) : "UNDEF");
		out += buf_;
	}

	snprintf(buf_, sizeof(buf_),
			 "%s%2zu%s) Сбросить все\r\n"
			 "%s%2zu%s) Установить все\r\n",
			 CCGRN(ch, C_NRM), bits.size() + 1, CCNRM(ch, C_NRM),
			 CCGRN(ch, C_NRM), bits.size() + 2, CCNRM(ch, C_NRM));
	out += buf_;

	snprintf(buf_, sizeof(buf_), "Текущие профессии : %s", CCCYN(ch, C_NRM));
	out += buf_;
	print_bitset(bits, pc_class_name, ",", out, true);
	out += CCNRM(ch, C_NRM);
	out += "\r\nВыберите профессии для данного активатора (0 - выход) : ";

	send_to_char(out, ch);
}

void sedit::parse_activ_prof(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	if (!*arg || !a_isdigit(*arg)) {
		send_to_char("Некорректный ввод.\r\n", ch);
		show_activ_prof(ch);
		return;
	}

	const unsigned num = atoi(arg);
	if (num == 0) {
		show_activ_edit(ch);
		return;
	}

	std::bitset<kNumPlayerClasses> &bits = olc_set.activ_list.at(activ_edit).prof;
	if (num > 0 && num <= bits.size()) {
		bits.flip(num - 1);
	} else if (num == bits.size() + 1) {
		bits.reset();
	} else if (num == bits.size() + 2) {
		bits.set();
	} else {
		send_to_char("Некорректный ввод.\r\n", ch);
	}
	show_activ_prof(ch);
}

void sedit::parse_activ_skill(CharacterData *ch, const char *arg) {
	auto &skill = olc_set.activ_list.at(activ_edit).skill;
	skip_spaces(&arg);
	int num = atoi(arg), ssnum = 0, ssval = 0;

	if (num == 0) {
		skill.first = ESkill::kIncorrect;
		skill.second = 0;
		show_activ_edit(ch);
		return;
	}

	if (sscanf(arg, "%d %d", &ssnum, &ssval) < 2) {
		send_to_char("Не указан уровень владения умением.\r\n", ch);
		show_activ_skill(ch);
		return;
	}
	auto skill_id = static_cast<ESkill>(ssnum);
	if (MUD::Skills().IsInvalid(skill_id)) {
		send_to_char("Неизвестное умение.\r\n", ch);
		show_activ_skill(ch);
	} else if (ssval == 0) {
		skill.first = ESkill::kIncorrect;
		skill.second = 0;
		show_activ_edit(ch);
	} else {
		skill.first = static_cast<ESkill>(ssnum);
		skill.second = std::clamp(ssval, -200, 200);
		show_activ_edit(ch);
	}
}

void sedit::parse_activ_phys_dmg(CharacterData *ch, const char *arg) {
	bonus_type &bonus = olc_set.activ_list.at(activ_edit).bonus;
	skip_spaces(&arg);
	int num = atoi(arg);

	if (num <= 0) {
		bonus.phys_dmg = 0;
	} else {
		bonus.phys_dmg = std::min(num, 1000);
	}
	show_activ_edit(ch);
}

void sedit::parse_activ_mage_dmg(CharacterData *ch, const char *arg) {
	bonus_type &bonus = olc_set.activ_list.at(activ_edit).bonus;
	skip_spaces(&arg);
	int num = atoi(arg);

	if (num <= 0) {
		bonus.mage_dmg = 0;
	} else {
		bonus.mage_dmg = std::min(num, 1000);
	}
	show_activ_edit(ch);
}

void sedit::parse_activ_edit(CharacterData *ch, const char *arg) {
	auto i = olc_set.activ_list.find(activ_edit);
	if (i == olc_set.activ_list.end()) {
		send_to_char(ch,
					 "Ошибка: активатор не найден %s:%d (%s).\r\n",
					 __FILE__, __LINE__, __func__);
		show_main(ch);
		return;
	}
	const ActivNode &activ = i->second;

	skip_spaces(&arg);
	if (!*arg) {
		send_to_char("Неверный выбор!\r\n", ch);
		show_activ_edit(ch);
		return;
	}
	if (!a_isdigit(*arg)) {
		switch (*arg) {
			case 'Q':
			case 'q':
			case 'В':
			case 'в': show_main(ch);
				break;
			default: send_to_char("Неверный выбор!\r\n", ch);
				show_activ_edit(ch);
				break;
		}
		return;
	}

	unsigned num = atoi(arg);
	switch (num) {
		case 1: state = STATE_ACTIV_REMOVE;
			send_to_char("Подтвердите удаление активатора Y(Д)/N(Н) :", ch);
			return;
		case 2: state = STATE_ACTIV_CHANGE;
			send_to_char("Введите новое кол-во предметов для активации :", ch);
			return;
		case 3: show_activ_prof(ch);
			return;
		case 4: show_activ_affects(ch);
			return;
		default: break;
	}
	const unsigned offset = 4;

	if (num > offset && num <= offset + activ.apply.size()) {
		show_activ_apply(ch);
		apply_edit = num - offset - 1;
	} else if (num == offset + activ.apply.size() + 1) {
		show_activ_skill(ch);
	} else if (num == offset + activ.apply.size() + 2) {
		show_activ_ench_vnum(ch);
	} else if (num == offset + activ.apply.size() + 3) {
		state = STATE_ACTIV_PHYS_DMG;
		send_to_char("Укажите процент прибавляемого физ. урона :", ch);
	} else if (num == offset + activ.apply.size() + 4) {
		state = STATE_ACTIV_MAGE_DMG;
		send_to_char("Укажите процент прибавляемого маг. урона :", ch);
	} else if (num == offset + activ.apply.size() + 5) {
		if (activ.npc) {
			send_to_char("Теперь НЕ активится на мобах.", ch);
			i->second.npc = false;
		} else {
			send_to_char("Теперь активится на мобах.", ch);
			i->second.npc = true;
		}
		show_activ_edit(ch);
	} else if (num == offset + activ.apply.size() + 6) {
		show_main(ch);
	} else {
		send_to_char("Некорректный ввод.\r\n", ch);
		show_activ_edit(ch);
	}
}

void sedit::parse_activ_affects(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	int bit = 0, plane = 0;
	int num = planebit(arg, &plane, &bit);

	if (num < 0) {
		show_activ_affects(ch);
	} else if (num == 0) {
		show_activ_edit(ch);
	} else {
		olc_set.activ_list.at(activ_edit).affects.toggle_flag(plane, 1 << bit);
		show_activ_affects(ch);
		return;
	}
}

void sedit::parse_activ_apply_loc(CharacterData *ch, const char *arg) {
	obj_affected_type &apply =
		olc_set.activ_list.at(activ_edit).apply.at(apply_edit);

	skip_spaces(&arg);
	int num = atoi(arg);

	if (num == 0) {
		apply.location = EApplyLocation::APPLY_NONE;
		apply.modifier = 0;
		show_activ_edit(ch);
	} else if (num < 0 || num >= NUM_APPLIES) {
		send_to_char("Некорректный ввод.\r\n", ch);
		show_apply_olc(ch->desc);
	} else {
		apply.location = static_cast<EApplyLocation>(num);
		state = STATE_ACTIV_APPLY_MOD;
		send_to_char("Введите модификатор : ", ch);
	}
}

void sedit::parse_activ_apply_mod(CharacterData *ch, const char *arg) {
	obj_affected_type &apply =
		olc_set.activ_list.at(activ_edit).apply.at(apply_edit);

	skip_spaces(&arg);
	int num = atoi(arg);

	if (num == 0) {
		apply.location = EApplyLocation::APPLY_NONE;
		apply.modifier = 0;
		show_activ_edit(ch);
	} else {
		apply.modifier = num;
		show_activ_edit(ch);
	}
}

void sedit::parse_obj_change(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	if (!*arg || !a_isdigit(*arg)) {
		send_to_char("Некорректный виртуальный номер предмета.\r\n", ch);
		show_obj_edit(ch);
		return;
	}

	const int vnum = atoi(arg);
	if (vnum == obj_edit) {
		send_to_char("Вы и так редактируете данный предмет.\r\n", ch);
		show_obj_edit(ch);
		return;
	}

	const auto rnum = real_object(vnum);
	const auto name = rnum < 0
					  ? MISSING_OBJECT_NAME
					  : obj_proto[rnum]->get_short_description().c_str();
	if (is_duplicate(olc_set.uid, vnum)) {
		send_to_char(ch, "Предмет '%s' уже является частью другого набора.\r\n", name);
	} else if (olc_set.obj_list.find(vnum) != olc_set.obj_list.end()) {
		send_to_char(ch, "Предмет '%s' уже является частью данного набора.\r\n", name);
	} else {
		SetMsgNode msg;
		auto i = olc_set.obj_list.find(obj_edit);
		if (i != olc_set.obj_list.end()) {
			msg = i->second;
			olc_set.obj_list.erase(i);
		}
		obj_edit = vnum;

		olc_set.obj_list.insert(std::make_pair(vnum, msg));
		send_to_char(ch, "Предмет '%s' добавлен в набор.\r\n", name);
	}

	show_obj_edit(ch);
}

void sedit::parse_activ_change(CharacterData *ch, const char *arg) {
	skip_spaces(&arg);
	if (!*arg || !a_isdigit(*arg)) {
		send_to_char("Некорректное кол-во предметов для активации.\r\n", ch);
		show_activ_edit(ch);
		return;
	}

	const int num = atoi(arg);
	if (num == activ_edit) {
		send_to_char("Кол-во предметов активации не изменилось.\r\n", ch);
	} else if (olc_set.activ_list.find(num) != olc_set.activ_list.end()) {
		send_to_char(ch,
					 "Набор уже содержит активатор на %d %s.\r\n",
					 num, desc_count(num, WHAT_OBJECT));
	} else {
		ActivNode activ;
		auto i = olc_set.activ_list.find(activ_edit);
		if (i != olc_set.activ_list.end()) {
			activ = i->second;
			olc_set.activ_list.erase(i);
		}
		activ_edit = num;
		// GCC 4.4
		//olc_set.activ_list.emplace(num, activ);
		olc_set.activ_list.insert(std::make_pair(num, activ));
		send_to_char(ch, "Активатор на %d %s добавлен в набор.\r\n",
					 num, desc_count(num, WHAT_OBJECT));
	}
	show_activ_edit(ch);
}

/// наружу при обработке команд в олц торчит только эта функция
/// плюс она обернута в try/catch для ловли возможных out_of_range, т.к. внутри
/// все через .at и местами код на это закладывается вместо ручных сравнений
void parse_input(CharacterData *ch, const char *arg) {
	sedit &olc = *(ch->desc->sedit);

	switch (olc.state) {
		case STATE_MAIN: olc.parse_main(ch, arg);
			break;
		case STATE_MAIN_EXIT: parse_main_exit(ch, arg);
			break;
		case STATE_SET_REMOVE: parse_set_remove(ch, arg);
			break;
		case STATE_NAME: olc.parse_setname(ch, arg);
			break;
		case STATE_ALIAS: olc.parse_setalias(ch, arg);
			break;
		case STATE_COMMENT: olc.parse_setcomment(ch, arg);
			break;
		case STATE_SETMSG_CHAR_ON:
		case STATE_SETMSG_CHAR_OFF:
		case STATE_SETMSG_ROOM_ON:
		case STATE_SETMSG_ROOM_OFF:
		case STATE_OBJMSG_CHAR_ON:
		case STATE_OBJMSG_CHAR_OFF:
		case STATE_OBJMSG_ROOM_ON:
		case STATE_OBJMSG_ROOM_OFF:
		case STATE_GLBMSG_CHAR_ON:
		case STATE_GLBMSG_CHAR_OFF:
		case STATE_GLBMSG_ROOM_ON:
		case STATE_GLBMSG_ROOM_OFF: olc.parse_setmsg(ch, arg);
			break;
		case STATE_OBJ_ADD: olc.parse_obj_add(ch, arg);
			break;
		case STATE_OBJ_EDIT: olc.parse_obj_edit(ch, arg);
			break;
		case STATE_OBJ_CHANGE: olc.parse_obj_change(ch, arg);
			break;
		case STATE_OBJ_REMOVE: olc.parse_obj_remove(ch, arg);
			break;
		case STATE_TOTAL_ACTIV: olc.show_main(ch);
			break;
		case STATE_ACTIV_ADD: olc.parse_activ_add(ch, arg);
			break;
		case STATE_ACTIV_EDIT: olc.parse_activ_edit(ch, arg);
			break;
		case STATE_ACTIV_AFFECTS: olc.parse_activ_affects(ch, arg);
			break;
		case STATE_ACTIV_REMOVE: olc.parse_activ_remove(ch, arg);
			break;
		case STATE_ACTIV_PROF: olc.parse_activ_prof(ch, arg);
			break;
		case STATE_ACTIV_CHANGE: olc.parse_activ_change(ch, arg);
			break;
		case STATE_ACTIV_APPLY_LOC: olc.parse_activ_apply_loc(ch, arg);
			break;
		case STATE_ACTIV_APPLY_MOD: olc.parse_activ_apply_mod(ch, arg);
			break;
		case STATE_ACTIV_SKILL: olc.parse_activ_skill(ch, arg);
			break;
		case STATE_ACTIV_ENCH_VNUM: olc.parse_activ_ench_vnum(ch, arg);
			break;
		case STATE_ACTIV_ENCH_NDICE: olc.parse_activ_ench_ndice(ch, arg);
			break;
		case STATE_ACTIV_ENCH_SDICE: olc.parse_activ_ench_sdice(ch, arg);
			break;
		case STATE_ACTIV_ENCH_WEIGHT: olc.parse_activ_weight_val(ch, arg);
			break;
		case STATE_ACTIV_PHYS_DMG: olc.parse_activ_phys_dmg(ch, arg);
			break;
		case STATE_ACTIV_MAGE_DMG: olc.parse_activ_mage_dmg(ch, arg);
			break;
		case STATE_GLOBAL_MSG: olc.parse_global_msg(ch, arg);
			break;
		case STATE_GLOBAL_MSG_EXIT: parse_global_msg_exit(ch, arg);
			break;
		default: ch->desc->sedit.reset();
			STATE(ch->desc) = CON_PLAYING;
			send_to_char(ch,
						 "Ошибка: не найден STATE, редактирование отменено %s:%d (%s).\r\n",
						 __FILE__, __LINE__, __func__);
			break;
	} // switch
}

} // namespace obj_sets_olc

using namespace obj_sets_olc;

namespace {

void start_sedit(CharacterData *ch, size_t idx) {
	STATE(ch->desc) = CON_SEDIT;
	ch->desc->sedit = std::make_shared<obj_sets_olc::sedit>();
	if (idx == static_cast<size_t>(-1)) {
		ch->desc->sedit->new_entry = true;
	} else {
		ch->desc->sedit->olc_set = *(sets_list.at(idx));
	}
	ch->desc->sedit->show_main(ch);
}

const char *SEDIT_HELP =
	"Формат команды:\r\n"
	"   sedit - создание нового сета\r\n"
	"   sedit <прядковый номер сета из slist> - редактирование существующего сета\r\n"
	"   sedit <vnum любого предмета из сета> - редактирование существующего сета\r\n"
	"   sedit <msg_set|messages> - редактирование глобальных сообщений сетов\r\n";

} // namespace

/// иммский sedit, см. SEDIT_HELP
void do_sedit(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch)) {
		return;
	}

	skip_spaces(&argument);
	if (!argument || !*argument) {
		// создание нового сета
		start_sedit(ch, -1);
	} else if (a_isdigit(*argument)) {
		// редактирование существующего
		unsigned num = atoi(argument);
		if (num > 0 && num <= sets_list.size()) {
			// по номеру сета из slist
			--num;
			start_sedit(ch, num);
		} else {
			// по внуму предмета
			if (real_object(num) < 0) {
				send_to_char(SEDIT_HELP, ch);
				send_to_char(ch, "Предметов с vnum %s не существует.\r\n",
							 argument);
				return;
			}
			size_t idx = setidx_by_objvnum(num);
			if (idx < sets_list.size()) {
				start_sedit(ch, idx);
			} else {
				send_to_char(SEDIT_HELP, ch);
				send_to_char(ch,
							 "В сетах предметов с vnum %s не найдено.\r\n", argument);
			}
		}
	} else if (!str_cmp(argument, "msg_set") || !str_cmp(argument, "messages")) {
		// редактирование глобальных сообщений
		STATE(ch->desc) = CON_SEDIT;
		ch->desc->sedit = std::make_shared<obj_sets_olc::sedit>();
		ch->desc->sedit->msg_edit = global_msg;
		ch->desc->sedit->show_global_msg(ch);
	} else {
		send_to_char(SEDIT_HELP, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
