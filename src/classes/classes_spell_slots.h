#ifndef __SPELL_SLOTS_HPP__
#define __SPELL_SLOTS_HPP__

/*
	Константы, определяющие количество слотов под заклинания
	и прочий код для работы с ними.
*/

#include "entities/char.h"
#include "classes/classes_constants.h"

namespace PlayerClass {

int slot_for_char(CharacterData *ch, int slot_num);
void mspell_slot(char *name, int spell, int kin, int chclass, int slot);

class MaxClassSlot {
 public:
	MaxClassSlot();

	void init(int chclass, int kin, int slot);
	int get(int chclass, int kin) const;
	int get(const CharacterData *ch) const;

 private:
	int _max_class_slot[kNumPlayerClasses][kNumKins];
};

extern MaxClassSlot max_slots;

}; //namespace ClassPlayer

#endif // __SPELL_SLOTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
