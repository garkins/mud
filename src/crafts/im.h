/* ************************************************************************
*   File: im.cpp                                        Part of Bylins    *
*  Usage: Ingradient handling function                                    *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _IM_H_
#define _IM_H_

#include "entities/entity_constants.h"
#include "classes/classes_constants.h"

class ObjectData;    // forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.
struct RoomData;    //

// Определение основных классов ингредиентов: росль, живь, твердь
#define        IM_CLASS_ROSL        0
#define        IM_CLASS_JIV        1
#define        IM_CLASS_TVERD        2

#define    IM_POWER_SLOT        1
#define        IM_TYPE_SLOT        2
#define     IM_INDEX_SLOT       3

#define        IM_NPARAM            3

struct _im_tlist_tag {
	long size;        // Количество битов
	long *types;        // Битовая маска
};
typedef struct _im_tlist_tag im_tlist;

struct _im_memb_tag {
	int power;        // сила ингредиента
	ESex sex;        // род описателя (0-сред,1-муж,2-жен,3-мн.ч)
	char **aliases;        // массив пар алиасов
	struct _im_memb_tag *next;    // ссылка на следующий
};
typedef struct _im_memb_tag im_memb;

// Описание примитивного типа ингредиента
struct _im_type_tag {
	int id;            // номер из im.lst
	char *name;        // название типа ингредиента
	int proto_vnum;        // vnum объекта-прототипа
	im_tlist tlst;        // список ID типов/метатипов, которым
	// принадлежит данный тип
	im_memb *head;        // список описателей видов
	// не используется для ингредиентов класса живь
};
typedef struct _im_type_tag im_type;

// Описание метатипа не отличается от описания примитивного типа
// Поле cls в описании метатипа не используется
// Поле members в описании метатипа не используется

// Описание дополнительного компонента
struct _im_addon_tag {
	int id;            // тип ингредиента, индекс массива
	int k0, k1, k2;        // распределение энергии
	ObjectData *obj;        // подставляемый объект
	struct _im_addon_tag *link;    // ссылка
};
typedef struct _im_addon_tag im_addon;

#define IM_MSG_OK        0
#define IM_MSG_FAIL        1
#define IM_MSG_DAM        2

// +newbook.patch (Alisher)
#define KNOW_RECIPE  1
// -newbook.patch (Alisher)

// Описание рецепта
struct _im_recipe_tag {
	int id;            // номер из im.lst
	char *name;        // название рецепта
	int k_improve;        // сложность прокачки
	int result;        // VNUM прототипа результата
	float k[IM_NPARAM], kp;    // курсы перевода
	int *require;        // массив обязательных компонентов
	int nAddon;        // количество добавочных компонентов
	im_addon *addon;    // массив добавочных компонентов
	std::array<char *, 3> msg_char;    // сообщения OK,FAIL,DAM
	std::array<char *, 3> msg_room;    // сообщения OK,FAIL,DAM
	int x, y;        // XdY - повреждения
// +newbook.patch (Alisher)
	std::array<int, kNumPlayerClasses> classknow; // владеет ли класс данным рецептом
	int level; // на каком уровне можно выучить рецепт
	int remort; // сколько ремортов необходимо для рецепта
// -newbook.patch (Alisher)
};
typedef struct _im_recipe_tag im_recipe;

// Описание рецепта-умения
struct im_rskill {
	int rid;        // индекс в главном массиве рецептов
	int perc;        // уровень владения умением
	im_rskill *link;    // указатель на следующее умение в цепочке
};

extern im_recipe *imrecipes;

void im_parse(int **ing_list, char *line);
//MZ.load
void im_reset_room(RoomData *room, int level, int type);
//-MZ.load
ObjectData *try_make_ingr(CharacterData *mob, int prob_default);
int im_assign_power(ObjectData *obj);
int im_get_recipe(int id);
int im_get_type_by_name(char *name, int mode);
ObjectData *load_ingredient(int index, int power, int rnum);
int im_ing_dump(int *ping, char *s);
void im_inglist_copy(int **pdst, int *src);
void im_extract_ing(int **pdst, int num);
int im_get_char_rskill_count(CharacterData *ch);
void trg_recipeturn(CharacterData *ch, int rid, int recipediff);
void AddRecipe(CharacterData *ch, int rid, int recipediff);
int im_get_recipe_by_name(char *name);
im_rskill *im_get_char_rskill(CharacterData *ch, int rid);
void compose_recipe(CharacterData *ch, char *argument, int subcmd);
void forget_recipe(CharacterData *ch, char *argument, int subcmd);
int im_get_idx_by_type(int type);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
