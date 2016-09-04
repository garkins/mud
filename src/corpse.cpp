// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include "corpse.hpp"
#include "constants.h"
#include "db.h"
#include "utils.h"
#include "char.hpp"
#include "comm.h"
#include "handler.h"
#include "dg_scripts.h"
#include "im.h"
#include "room.hpp"
#include "pugixml.hpp"
#include "modify.h"
#include "house.h"
#include "parse.hpp"
#include "obj.hpp"
#include "random.hpp"
#include <boost/algorithm/string.hpp>

#include <fstream>
#include <map>
#include <vector>
#include <string>

// see http://stackoverflow.com/questions/20145488/cygwin-g-stdstoi-error-stoi-is-not-a-member-of-std
#if defined __CYGWIN__
#include <cstdlib>
#define stoi(x) strtol(x.c_str(),0,10)
#endif

extern int max_npc_corpse_time, max_pc_corpse_time;
extern MobRaceListType mobraces_list;
extern void obj_to_corpse(OBJ_DATA *corpse, CHAR_DATA *ch, int rnum, bool setload);
extern bool check_unlimited_timer(OBJ_DATA *obj);







namespace GlobalDrop
{
std::vector<table_drop> tables_drop;
typedef std::map<int /* vnum */, int /* rnum*/> OlistType;

struct global_drop
{
	global_drop() : vnum(0), mob_lvl(0), max_mob_lvl(0), count_mob(0), mobs(0), rnum(-1), day_start(-1), day_end(-1), race_mob(-1), chance(-1) {};
	int vnum; // ���� ������, ���� ����� ������������� - ���� ������ ������
	int mob_lvl;  // ��� ����� ����
	int max_mob_lvl; // ����. ����� ���� (0 - �� �����������)
	int count_mob;  // ����� ������ ������ � ����� �����, ��������� ������, ����� ������ ����
	int mobs; // ����� ���������� �����
	int rnum; // ���� ������, ���� vnum ��������
	int day_start; // ������� � ������ ��� (��������) ������ ����� ������� � ���� � ... 
	int day_end; // ... ������ ��� ����, ����� ��������, ������ ���������� �������� �� ����
	int race_mob; // ��� ����, � �������� ������ ������ ������ (-1 ���)
	int chance; // ������� ��������� (1..1000)
	// ������ ������ � ����� ������ (��������� ������ ���������)
	// ��� ������ �� ������ ����������� ���� ��������� � ����
/*#define NPC_RACE_BASIC			100   - ������ ���
#define NPC_RACE_HUMAN			101
#define NPC_RACE_HUMAN_ANIMAL	102
#define NPC_RACE_BIRD			103
#define NPC_RACE_ANIMAL			104
#define NPC_RACE_REPTILE		105
#define NPC_RACE_FISH			106
#define NPC_RACE_INSECT			107
#define NPC_RACE_PLANT			108
#define NPC_RACE_THING			109
#define NPC_RACE_ZOMBIE			110
#define NPC_RACE_GHOST			111
#define NPC_RACE_EVIL_SPIRIT	112
#define NPC_RACE_SPIRIT			113
#define NPC_RACE_MAGIC_CREATURE	114
*/
	OlistType olist;
};

// ��� �����
struct global_drop_obj
{
	global_drop_obj() : vnum(0), chance(0), day_start(0), day_end(0) {};
	// vnum ������
	int vnum;
	// chance ������ �� 0 �� 1000
	int chance;
	// ����� ������ ���� ���, � ������� ����� ���������� ������
	std::list<int> sects;
	int day_start;
	int day_end;
};


table_drop::table_drop(std::vector<int> mbs, int chance_, int count_mobs_, int vnum_obj_)
{
	this->mobs = mbs;
	this->chance = chance_;
	this->count_mobs = count_mobs_;
	this->vnum_obj = vnum_obj_;
	this->reload_table();
}
void table_drop::reload_table()
{
	this->drop_mobs.clear();
	for (int i = 0; i < this->count_mobs; i++)
	{
		const int mob_number = number(0, static_cast<int>(this->mobs.size() - 1));
		this->drop_mobs.push_back(this->mobs[mob_number]);
	}
}
// ��������� true, ���� ��� ������ � ������� � ������ ����
bool table_drop::check_mob(int vnum)
{
	for (size_t i = 0; i < this->drop_mobs.size(); i++)
	{
		if (this->drop_mobs[i] == vnum)
		{
			if (number(0, 1000) < this->chance)
				return true;
		}
	}
	return false;
}
int table_drop::get_vnum()
{
	return this->vnum_obj;
}

void reload_tables()
{
	for (size_t i = 0; i < tables_drop.size(); i++)
	{
		tables_drop[i].reload_table();
	}
}

typedef std::vector<global_drop> DropListType;
DropListType drop_list;

std::vector<global_drop_obj> drop_list_obj;

const char *CONFIG_FILE = LIB_MISC"global_drop.xml";
const char *STAT_FILE = LIB_PLRSTUFF"global_drop.tmp";

void init()
{
	// �� ������ �������
	drop_list.clear();
	drop_list_obj.clear();
	// ������
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
    pugi::xml_node node_list = doc.child("globaldrop");
    if (!node_list)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...<globaldrop> read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }
	for (pugi::xml_node node = node_list.child("tdrop"); node; node = node.next_sibling("tdrop"))
	{
		int chance = Parse::attr_int(node, "chance");
		int count_mobs = Parse::attr_int(node, "count_mobs");
		int vnum_obj = Parse::attr_int(node, "obj_vnum");
		std::vector<int> list_mobs;
		for (pugi::xml_node node_ = node.child("mobs"); node_; node_ = node_.next_sibling("mobs"))
		{
			list_mobs.push_back(Parse::attr_int(node_, "vnum"));
		}
		table_drop tmp(list_mobs, chance, count_mobs, vnum_obj);
		tables_drop.push_back(tmp);			
	}
	for (pugi::xml_node node = node_list.child("freedrop_obj"); node; node = node.next_sibling("freedrop_obj"))
	{
		global_drop_obj tmp;
		int obj_vnum = Parse::attr_int(node, "obj_vnum");
		int chance =  Parse::attr_int(node, "chance");
		int day_start = Parse::attr_int_t(node, "day_start"); // ���� �� ���������� � ����� ���������� -1
		int day_end = Parse::attr_int_t(node, "day_end");
		if (day_start == -1)
		{
			day_end = 360;
			day_start = 0;
		}
		std::string tmp_str = Parse::attr_str(node, "sects");
		std::list<std::string> strs;
		boost::split(strs, tmp_str, boost::is_any_of(" "));
		for (const auto& i : strs)
		{
			tmp.sects.push_back(std::stoi(i));
		}
		tmp.vnum = obj_vnum;
		tmp.chance = chance;
		tmp.day_start = day_start;
		tmp.day_end = day_end;
		drop_list_obj.push_back(tmp);
	}
	for (pugi::xml_node node = node_list.child("drop"); node; node = node.next_sibling("drop"))
	{
		int obj_vnum = Parse::attr_int(node, "obj_vnum");
		int mob_lvl = Parse::attr_int(node, "mob_lvl");
		int max_mob_lvl = Parse::attr_int(node, "max_mob_lvl");
		int count_mob = Parse::attr_int(node, "count_mob");
		int day_start = Parse::attr_int_t(node, "day_start"); // ���� �� ���������� � ����� ���������� -1
		int day_end = Parse::attr_int_t(node, "day_end");
		int race_mob = Parse::attr_int_t(node, "race_mob"); 
		int chance = Parse::attr_int_t(node, "chance"); 
		if (chance == -1)
			chance = 1000;
		if (day_start == -1)
			day_start = 0;
		if (day_end == -1)
			day_end = 360;
		if (race_mob == -1)
			race_mob = -1; // -1 ��� ���� ���
	
		if (obj_vnum == -1 || mob_lvl <= 0 || count_mob <= 0 || max_mob_lvl < 0)
		{
			snprintf(buf, MAX_STRING_LENGTH,
					"...bad drop attributes (obj_vnum=%d, mob_lvl=%d, chance=%d, max_mob_lvl=%d)",
					obj_vnum, mob_lvl, count_mob, max_mob_lvl);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}
		snprintf(buf, MAX_STRING_LENGTH,
					"GLOBALDROP: (obj_vnum=%d, mob_lvl=%d, count_mob=%d, max_mob_lvl=%d, day_start=%d, day_end=%d, race_mob=%d, chance=%d)",
					obj_vnum, mob_lvl, count_mob, max_mob_lvl, day_start, day_end, race_mob, chance); 
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		global_drop tmp_node;
		tmp_node.vnum = obj_vnum;
		tmp_node.mob_lvl = mob_lvl;
		tmp_node.max_mob_lvl = max_mob_lvl;
		tmp_node.count_mob = count_mob;
		tmp_node.day_start = day_start;
		tmp_node.day_end = day_end;
		tmp_node.race_mob = race_mob;
		tmp_node.chance = chance;


		if (obj_vnum >= 0)
		{
			int obj_rnum = real_object(obj_vnum);
			if (obj_rnum < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH, "...incorrect obj_vnum=%d", obj_vnum);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}
			tmp_node.rnum = obj_rnum;
		}
		else
		{
			// ������ ������ � ������ ������
			for (pugi::xml_node item = node.child("obj"); item; item = item.next_sibling("obj"))
			{
				int item_vnum = Parse::attr_int(item, "vnum");
				if (item_vnum <= 0)
				{
					snprintf(buf, MAX_STRING_LENGTH,
						"...bad shop attributes (item_vnum=%d)", item_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					return;
				}
				// ��������� ������
				int item_rnum = real_object(item_vnum);
				if (item_rnum < 0)
				{
					snprintf(buf, MAX_STRING_LENGTH, "...incorrect item_vnum=%d", item_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					return;
				}
				tmp_node.olist[item_vnum] = item_rnum;
			}
			if (tmp_node.olist.empty())
			{
				snprintf(buf, MAX_STRING_LENGTH, "...item list empty (obj_vnum=%d)", obj_vnum);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}
		}
		drop_list.push_back(tmp_node);
	}

	// ����������� ����� �� ������ ����� �����
	std::ifstream file(STAT_FILE);
	if (!file.is_open())
	{
		log("SYSERROR: �� ������� ������� ���� �� ������: %s", STAT_FILE);
		return;
	}
	int vnum, mobs;
	while (file >> vnum >> mobs)
	{
		for (DropListType::iterator i = drop_list.begin(); i != drop_list.end(); ++i)
		{
			if (i->vnum == vnum)
			{
				i->mobs = mobs;
			}
		}
	}
}

void save()
{
	std::ofstream file(STAT_FILE);
	if (!file.is_open())
	{
		log("SYSERROR: �� ������� ������� ���� �� ������: %s", STAT_FILE);
		return;
	}
	for (DropListType::iterator i = drop_list.begin(); i != drop_list.end(); ++i)
	{
		file << i->vnum << " " << i->mobs << "\n";
	}
}

/**
 * ����� ������ ��� ����� �� ������ � ������ ���� � ����.
 * \return rnum
 */
int get_obj_to_drop(DropListType::iterator &i)
{
	std::vector<int> tmp_list;
	for (OlistType::iterator k = i->olist.begin(), kend = i->olist.end(); k != kend; ++k)
	{
		if ((GET_OBJ_MIW(obj_proto[k->second]) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM)
			|| (k->second >= 0
				&& obj_proto.actual_count(k->second) < GET_OBJ_MIW(obj_proto[k->second])))
			tmp_list.push_back(k->second);
	}
	if (!tmp_list.empty())
	{
		int rnd = number(0, static_cast<int>(tmp_list.size() - 1));
		return tmp_list.at(rnd);
	}
	return -1;
}

/**
 * ���������� ���� � ����� �������� ����������.
 * ���� vnum �������������, �� ����� ���� �� ������ ������ �����.
 */
bool check_mob(OBJ_DATA *corpse, CHAR_DATA *mob)
{
	for (size_t i = 0; i < tables_drop.size(); i++)
	{
		if (tables_drop[i].check_mob(GET_MOB_VNUM(mob)))
		{			
			int rnum;
			if ((rnum = real_object(tables_drop[i].get_vnum())) < 0)
			{
				log("������ tdrop. ����: %d", tables_drop[i].get_vnum());
				return true;
			}
			act("&G���-�� ������-������ �������� ���������� ���� ����������.&n", FALSE, mob, 0, 0, TO_ROOM);
			log("�������: ���� ������� %s � VNUM: %d", obj_proto[rnum]->short_description, obj_proto.vnum(rnum));
			obj_to_corpse(corpse, mob, rnum, false);
			return true;
		}
	}
	for (DropListType::iterator i = drop_list.begin(), iend = drop_list.end(); i != iend; ++i)
	{ int day = time_info.month * DAYS_PER_MONTH + time_info.day + 1;
		if (GET_LEVEL(mob) >= i->mob_lvl 				   
		    && (!i->max_mob_lvl || GET_LEVEL(mob) <= i->max_mob_lvl) 		// ��� � ��������� �������
		    && ((i->race_mob < 0) || (GET_RACE(mob) == i->race_mob) || (get_virtual_race(mob) == i->race_mob)) 		// ��������� ���� ��� ��� ����
		    && ((i->day_start <= day) && (i->day_end >= day))			// ��������� ����������
		    && (!mob->master || IS_NPC(mob->master))) // �� ������	

		{
			++(i->mobs);
			if (i->mobs >= i->count_mob)
			{
				int obj_rnum = i->vnum > 0 ? i->rnum : get_obj_to_drop(i);
				if (obj_rnum == -1)
				    return false;
				if (number(1, 1000) <= i->chance
					&& ((GET_OBJ_MIW(obj_proto[obj_rnum]) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM)
						|| (obj_rnum >= 0
							&& obj_proto.actual_count(obj_rnum) < GET_OBJ_MIW(obj_proto[obj_rnum]))))
				{
					act("&G���-�� ������-������ �������� ���������� ���� ����������.&n", FALSE, mob, 0, 0, TO_ROOM);
					sprintf(buf, "�������: ���� ������� %s VNUM %d � ���� %s VNUM %d",
						obj_proto[obj_rnum]->short_description, obj_proto.vnum(obj_rnum), GET_NAME(mob), GET_MOB_VNUM(mob));
					mudlog(buf,  CMP, LVL_GRGOD, SYSLOG, TRUE);
					obj_to_corpse(corpse, mob, obj_rnum, false);
				}
				i->mobs = 0;
//				return true; ����� ����� �������� ���� ����� ������������
			}
		}
	}
	return false;
}

} // namespace GlobalDrop

void make_arena_corpse(CHAR_DATA * ch, CHAR_DATA * killer)
{
	OBJ_DATA *corpse;
	EXTRA_DESCR_DATA *exdesc;

	corpse = create_obj();
	GET_OBJ_SEX(corpse) = ESex::SEX_POLY;

	sprintf(buf2, "������� %s ����� �� �����.", GET_PAD(ch, 1));
	corpse->description = str_dup(buf2);

	sprintf(buf2, "������� %s", GET_PAD(ch, 1));
	corpse->short_description = str_dup(buf2);

	sprintf(buf2, "������� %s", GET_PAD(ch, 1));
	corpse->PNames[0] = str_dup(buf2);
	corpse->aliases = str_dup(buf2);

	sprintf(buf2, "�������� %s", GET_PAD(ch, 1));
	corpse->PNames[1] = str_dup(buf2);
	sprintf(buf2, "�������� %s", GET_PAD(ch, 1));
	corpse->PNames[2] = str_dup(buf2);
	sprintf(buf2, "������� %s", GET_PAD(ch, 1));
	corpse->PNames[3] = str_dup(buf2);
	sprintf(buf2, "��������� %s", GET_PAD(ch, 1));
	corpse->PNames[4] = str_dup(buf2);
	sprintf(buf2, "�������� %s", GET_PAD(ch, 1));
	corpse->PNames[5] = str_dup(buf2);

	GET_OBJ_TYPE(corpse) = obj_flag_data::ITEM_CONTAINER;
	GET_OBJ_WEAR(corpse) = to_underlying(EWearFlag::ITEM_WEAR_TAKE);
	corpse->set_extraflag(EExtraFlag::ITEM_NODONATE);
	corpse->set_extraflag(EExtraFlag::ITEM_NOSELL);
	GET_OBJ_VAL(corpse, 0) = 0;	// You can't store stuff in a corpse
	GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
	GET_OBJ_VAL(corpse, 3) = 1;	// corpse identifier
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch);
	corpse->set_rent(100000);
	corpse->set_timer(max_pc_corpse_time * 2);
	CREATE(exdesc, 1);
	exdesc->keyword = str_dup(corpse->PNames[0]);	// ���������
	if (killer)
		sprintf(buf, "����%s �� ����� %s.\r\n", GET_CH_SUF_6(ch), GET_PAD(killer, 4));
	else
		sprintf(buf, "����%s �� �����.\r\n", GET_CH_SUF_4(ch));
	exdesc->description = str_dup(buf);	// ���������
	exdesc->next = corpse->ex_description;
	corpse->ex_description = exdesc;
	obj_to_room(corpse, IN_ROOM(ch));
}

OBJ_DATA *make_corpse(CHAR_DATA * ch, CHAR_DATA * killer)
{
	OBJ_DATA *corpse, *o;
	OBJ_DATA *money;
	int i;

	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_CORPSE))
		return NULL;

	sprintf(buf2, "���� %s", GET_PAD(ch, 1));
	corpse = create_obj(buf2);

	GET_OBJ_SEX(corpse) = ESex::SEX_MALE;

	sprintf(buf2, "���� %s ����� �����.", GET_PAD(ch, 1));
	corpse->description = str_dup(buf2);

	sprintf(buf2, "���� %s", GET_PAD(ch, 1));
	corpse->short_description = str_dup(buf2);

	sprintf(buf2, "���� %s", GET_PAD(ch, 1));
	corpse->PNames[0] = str_dup(buf2);
	sprintf(buf2, "����� %s", GET_PAD(ch, 1));
	corpse->PNames[1] = str_dup(buf2);
	sprintf(buf2, "����� %s", GET_PAD(ch, 1));
	corpse->PNames[2] = str_dup(buf2);
	sprintf(buf2, "���� %s", GET_PAD(ch, 1));
	corpse->PNames[3] = str_dup(buf2);
	sprintf(buf2, "������ %s", GET_PAD(ch, 1));
	corpse->PNames[4] = str_dup(buf2);
	sprintf(buf2, "����� %s", GET_PAD(ch, 1));
	corpse->PNames[5] = str_dup(buf2);

	GET_OBJ_TYPE(corpse) = obj_flag_data::ITEM_CONTAINER;
	GET_OBJ_WEAR(corpse) = to_underlying(EWearFlag::ITEM_WEAR_TAKE);
	corpse->set_extraflag(EExtraFlag::ITEM_NODONATE);
	corpse->set_extraflag(EExtraFlag::ITEM_NOSELL);
	GET_OBJ_VAL(corpse, 0) = 0;	// You can't store stuff in a corpse
	GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
	GET_OBJ_VAL(corpse, 3) = 1;	// corpse identifier
	corpse->set_rent(100000);

	if (IS_NPC(ch))
	{
		corpse->set_timer(max_npc_corpse_time * 2);
	}
	else
	{
		corpse->set_timer(max_pc_corpse_time * 2);
	}

	// transfer character's equipment to the corpse
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			remove_otrigger(GET_EQ(ch, i), ch);
			if (GET_EQ(ch, i)->purged()) continue;

			obj_to_char(unequip_char(ch, i), ch);
		}
	}

	// ������� ��� ������ ����� ���� ��� �������� ����
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);

	// transfer character's inventory to the corpse
	corpse->contains = ch->carrying;
	for (o = corpse->contains; o != NULL; o = o->next_content)
	{
		o->in_obj = corpse;
	}
	object_list_new_owner(corpse, NULL);


	// transfer gold
	// following 'if' clause added to fix gold duplication loophole
	if (ch->get_gold() > 0)
	{
		if (IS_NPC(ch))
		{
			money = create_money(ch->get_gold());
			obj_to_obj(money, corpse);
		}
		else
		{
			const int amount = ch->get_gold();
			money = create_money(amount);
			OBJ_DATA *purse = 0;
			if (amount >= 100)
			{
				purse = system_obj::create_purse(ch, amount);
				if (purse)
				{
					obj_to_obj(money, purse);
					obj_to_obj(purse, corpse);
				}
			}
			if (!purse)
			{
				obj_to_obj(money, corpse);
			}
		}
		ch->set_gold(0);
	}

	ch->carrying = NULL;
	IS_CARRYING_N(ch) = 0;
	IS_CARRYING_W(ch) = 0;

	//Polud ����������� �������� ������ � ���� (����) ����
	if (IS_NPC(ch) && GET_RACE(ch)>NPC_RACE_BASIC && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
	{
		OBJ_DATA* ingr = try_make_ingr(ch, 1000, 100);
		if (ingr)
			obj_to_obj(ingr, corpse);
	}

	// �������� ������ �� �����. - ���������� � raw_kill
	//if (IS_NPC (ch))
	//	dl_load_obj (corpse, ch);

	// ���� ������ ���� ������� ��� �� �����(� �������� �� � ��) �� ���� �������� �� � ������ � � ��������� � ��������� �������
	if(IS_CHARMICE(ch) && !MOB_FLAGGED(ch, MOB_CORPSE) && ch->master && ((killer && PRF_FLAGGED(killer, PRF_EXECUTOR))
		|| (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) && !RENTABLE(ch->master))))
	{
		obj_to_char(corpse, ch->master);
		return NULL;
	}
	else
	{
		room_rnum corpse_room = IN_ROOM(ch);
		if (corpse_room == STRANGE_ROOM && ch->get_was_in_room() != NOWHERE)
			corpse_room = ch->get_was_in_room();
		obj_to_room(corpse, corpse_room);
		return corpse;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
