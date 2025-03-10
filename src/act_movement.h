#ifndef _ACT_MOVEMENT_HPP_
#define _ACT_MOVEMENT_HPP_

#include "structs/structs.h"

class CharacterData;    // to avoid inclusion
class ObjectData;

enum DOOR_SCMD : int {
	SCMD_OPEN = 0,    // открыть
	SCMD_CLOSE = 1,   // закрыть
	SCMD_UNLOCK = 2,  // отпереть
	SCMD_LOCK = 3,    // запереть
	SCMD_PICK = 4  // взломать
};

enum FD_RESULT : int {
	FD_WRONG_DIR = -1,  // -1 НЕВЕРНОЕ НАПРАВЛЕНИЕ
	FD_WRONG_DOOR_NAME = -2,  // -2 НЕПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ В ЭТОМ НАПРАВЛЕНИИ
	FD_NO_DOOR_GIVEN_DIR = -3,  // -3 В ЭТОМ НАПРАВЛЕНИИ НЕТ ДВЕРИ
	FD_DOORNAME_EMPTY = -4, // -4 НЕ УКАЗАНО АРГУМЕНТОВ
	FD_DOORNAME_WRONG = -5   // -5 НЕПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ
};

void do_doorcmd(CharacterData *ch, ObjectData *obj, int door, DOOR_SCMD scmd);
void do_gen_door(CharacterData *ch, char *argument, int cmd, int subcmd);
void do_enter(CharacterData *ch, char *argument, int cmd, int subcmd);
void do_stand(CharacterData *ch, char *argument, int cmd, int subcmd);
void do_sit(CharacterData *ch, char *argument, int cmd, int subcmd);
void do_rest(CharacterData *ch, char *argument, int cmd, int subcmd);
void do_sleep(CharacterData *ch, char *argument, int cmd, int subcmd);
void do_wake(CharacterData *ch, char *argument, int cmd, int subcmd);

int has_boat(CharacterData *ch);
int has_key(CharacterData *ch, ObjVnum key);
bool ok_pick(CharacterData* ch, ObjVnum keynum, ObjectData* obj, int door, int scmd);
int legal_dir(CharacterData *ch, int dir, int need_specials_check, int show_msg);

int skip_hiding(CharacterData *ch, CharacterData *vict);
int skip_sneaking(CharacterData *ch, CharacterData *vict);
int skip_camouflage(CharacterData *ch, CharacterData *vict);

#endif // _ACT_MOVEMENT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
