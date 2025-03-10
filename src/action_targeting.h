#ifndef _ACTION_TARGETING_HPP_INCLUDED_
#define _ACTION_TARGETING_HPP_INCLUDED_

/*
	Класс, формирующий списки целей для массовых умений и заклинаний.
	Плюс несколько функций для проверки корректности целей.

	В перспективе сюда нужно перевести всю логику поиска и выбора целей для команд.
	При большом желании можно сделать класс универсальным, а не только для персонажей.
	Сейчас это не сделано, потому что при текущей логике исполнения команд это ничего особо не даст.
*/

#include "structs/structs.h"

#include <functional>

class CharacterData;

namespace ActionTargeting {

using FilterType = std::function<bool(CharacterData *, CharacterData *)>;
using PredicateType = std::function<bool(CharacterData *)>;
extern const FilterType emptyFilter;
extern const FilterType isCorrectFriend;
extern const FilterType isCorrectVictim;

bool isIncorrectVictim(CharacterData *actor, CharacterData *target, char *arg);

class TargetsRosterType {
 protected:
	CharacterData *_actor;
	std::vector<CharacterData *> _roster;
	FilterType _passesThroughFilter;

	void setFilter(const FilterType &baseFilter, const FilterType &extraFilter);
	void fillRoster();
	void shuffle();
	void setPriorityTarget(CharacterData *target);
	void makeRosterOfFoes(CharacterData *priorityTarget, const FilterType &baseFilter, const FilterType &extraFilter);
	void makeRosterOfFriends(CharacterData *priorityTarget, const FilterType &baseFilter, const FilterType &extraFilter);

	TargetsRosterType();
	TargetsRosterType(CharacterData *actor) :
		_actor{actor} {};
 public:
	auto begin() const noexcept { return std::make_reverse_iterator(std::end(_roster)); };
	auto end() const noexcept { return std::make_reverse_iterator(std::begin(_roster)); };
	CharacterData *getRandomItem(const PredicateType &predicate);
	CharacterData *getRandomItem();
	int amount() { return _roster.size(); };
	int count(const PredicateType &predicate);
	void flip();
};

//  -------------------------------------------------------

class FoesRosterType : public TargetsRosterType {
 protected:
	FoesRosterType();

 public:
	FoesRosterType(CharacterData *actor, CharacterData *priorityTarget, const FilterType &extraFilter) :
		TargetsRosterType(actor) {
		makeRosterOfFoes(priorityTarget, isCorrectVictim, extraFilter);
	};
	FoesRosterType(CharacterData *actor, const FilterType &extraFilter) :
		FoesRosterType(actor, nullptr, extraFilter) {};
	FoesRosterType(CharacterData *actor, CharacterData *priorityTarget) :
		FoesRosterType(actor, priorityTarget, emptyFilter) {};
	FoesRosterType(CharacterData *actor) :
		FoesRosterType(actor, nullptr, emptyFilter) {};
};

//  -------------------------------------------------------

class FriendsRosterType : public TargetsRosterType {
 protected:
	FriendsRosterType();

 public:
	FriendsRosterType(CharacterData *actor, CharacterData *priorityTarget, const FilterType &extraFilter) :
		TargetsRosterType(actor) {
		makeRosterOfFriends(priorityTarget, isCorrectFriend, extraFilter);
	};
	FriendsRosterType(CharacterData *actor, const FilterType &extraFilter) :
		FriendsRosterType(actor, nullptr, extraFilter) {};
	FriendsRosterType(CharacterData *actor, CharacterData *priorityTarget) :
		FriendsRosterType(actor, priorityTarget, emptyFilter) {};
	FriendsRosterType(CharacterData *actor) :
		FriendsRosterType(actor, nullptr, emptyFilter) {};
};

}; // namespace ActionTargeting

#endif // _ACTION_TARGETING_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
