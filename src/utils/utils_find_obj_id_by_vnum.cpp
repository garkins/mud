#include "utils_find_obj_id_by_vnum.h"

#include "dg_script/dg_scripts.h"
#include "world_objects.h"
#include "entities/char.h"

class TriggerLookup {
 public:
	using shared_ptr = std::shared_ptr<TriggerLookup>;

	TriggerLookup(FindObjIDByVNUM &finder) : m_finder(finder) {}
	~TriggerLookup() {}

	static TriggerLookup::shared_ptr create(FindObjIDByVNUM &finder, const int type, const void *go);

	virtual int lookup() = 0;

 protected:
	FindObjIDByVNUM &finder() { return m_finder; }

 private:
	FindObjIDByVNUM &m_finder;
};

class MobTriggerLookup : public TriggerLookup {
 public:
	MobTriggerLookup(FindObjIDByVNUM &finder, const CharacterData *mob) : TriggerLookup(finder), m_mob(mob) {}

	virtual int lookup() override;

 private:
	const CharacterData *m_mob;
};

class ObjTriggerLookup : public TriggerLookup {
 public:
	ObjTriggerLookup(FindObjIDByVNUM &finder, const ObjectData *object) : TriggerLookup(finder), m_object(object) {}

	virtual int lookup() override;

 private:
	const ObjectData *m_object;
};

class WldTriggerLookup : public TriggerLookup {
 public:
	WldTriggerLookup(FindObjIDByVNUM &finder, const RoomData *room) : TriggerLookup(finder), m_room(room) {}

	virtual int lookup() override;

 private:
	const RoomData *m_room;
};

int WldTriggerLookup::lookup() {
	auto result = false;
	if (m_room) {
		const auto room_rnum = real_room(m_room->room_vn);
		result = finder().lookup_room(room_rnum);
	}

	if (!result) {
		finder().lookup_world_objects();
	}
	return finder().result();
}

int ObjTriggerLookup::lookup() {
	if (!m_object) {
		return FindObjIDByVNUM::NOT_FOUND;
	}

	const auto owner = m_object->get_worn_by() ? m_object->get_worn_by() : m_object->get_carried_by();
	if (owner) {
		const auto mob_lookuper = std::make_shared<MobTriggerLookup>(finder(), owner);
		return mob_lookuper->lookup();
	}

	const auto object_room = m_object->get_in_room();
	const auto result = finder().lookup_room(object_room);
	if (!result) {
		finder().lookup_world_objects();
	}

	return finder().result();
}

int MobTriggerLookup::lookup() {
	auto result = false;
	if (m_mob) {
		result = finder().lookup_inventory(m_mob)
			|| finder().lookup_room(m_mob->in_room);
	}

	if (!result) {
		finder().lookup_world_objects();
	}

	return finder().result();
}

// * Аналогично find_char_vnum, только для объектов.
bool FindObjIDByVNUM::lookup_world_objects() {
	ObjectData::shared_ptr object = world_objects.find_by_vnum_and_dec_number(m_vnum, m_number, m_seen);

	if (object) {
		m_result = object->get_id();
		return true;
	}

	return false;
}

bool FindObjIDByVNUM::lookup_inventory(const CharacterData *character) {
	if (!character) {
		return false;
	}

	return lookup_list(character->carrying);
}

bool FindObjIDByVNUM::lookup_worn(const CharacterData *character) {
	if (!character) {
		return false;
	}

	for (int i = 0; i < NUM_WEARS; ++i) {
		const auto equipment = character->equipment[i];
		if (equipment
			&& equipment->get_vnum() == m_vnum) {
			if (0 == m_number) {
				m_result = equipment->get_id();
				return true;
			}

			add_seen(equipment->get_id());
			--m_number;
		}
	}

	return false;
}

bool FindObjIDByVNUM::lookup_room(const RoomRnum room) {
	const auto room_contents = world[room]->contents;
	if (!room_contents) {
		return false;
	}

	return lookup_list(room_contents);
}

bool FindObjIDByVNUM::lookup_list(const ObjectData *list) {
	while (list) {
		if (list->get_vnum() == m_vnum) {
			if (0 == m_number) {
				m_result = list->get_id();
				return true;
			}

			add_seen(list->get_id());
			--m_number;
		}

		list = list->get_next_content();
	}

	return false;
}

int FindObjIDByVNUM::lookup_for_caluid(const int type, const void *go) {
	const auto lookuper = TriggerLookup::create(*this, type, go);

	if (lookuper) {
		return lookuper->lookup();
	}

	return NOT_FOUND;
}

TriggerLookup::shared_ptr TriggerLookup::create(FindObjIDByVNUM &finder, const int type, const void *go) {
	switch (type) {
		case WLD_TRIGGER: return std::make_shared<WldTriggerLookup>(finder, static_cast<const RoomData *>(go));

		case OBJ_TRIGGER: return std::make_shared<ObjTriggerLookup>(finder, static_cast<const ObjectData *>(go));

		case MOB_TRIGGER: return std::make_shared<MobTriggerLookup>(finder, static_cast<const CharacterData *>(go));
	}

	log("SYSERR: Logic error trigger type %d is not valid. Valid values are %d, %d, %d",
		type, MOB_TRIGGER, OBJ_TRIGGER, WLD_TRIGGER);

	return nullptr;
}

int find_obj_by_id_vnum__find_replacement(const ObjVnum vnum) {
	FindObjIDByVNUM finder(vnum, 0);

	finder.lookup_world_objects();
	const auto result = finder.result();

	return result;
}

int find_obj_by_id_vnum__calcuid(const ObjVnum vnum, const unsigned number, const int type, const void *go) {
	FindObjIDByVNUM finder(vnum, number);

	finder.lookup_for_caluid(type, go);
	const auto result = finder.result();

	return result;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
