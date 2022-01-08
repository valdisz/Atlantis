#ifndef BALLTE_LOG_H
#define BALLTE_LOG_H

#include "faction.h"
#include "unit.h"
#include "fileio.h"

#include <string>
#include <vector>
#include <map>

namespace BattleLog {
	enum Side {
		ATTACKER, DEFENDER
	};

	struct Unit {
		Unit(const int number, const std::string& name);
		Unit(const ::Unit& unit);

		const int number;
		const std::string name;
	};

	struct Soldier {
		Soldier(const int number);
		Soldier(const ::Soldier& soldier);

		const int number;
	};

	class Event {
	public:
		virtual ~Event() = 0;
	};

	class Cast : public Event {
	public:
		Cast(const Soldier& caster, const SpecialType& special, const int amount);
		~Cast();
	};

	class Overwhelmed : public Event {
	public:
		Overwhelmed(const Unit& unit);
		~Overwhelmed();

		const Unit & unit;
	};

	class Routed : public Event {
	public:
		Routed(const Unit& unit);
		~Routed();

		const Unit & unit;
	};

	class Destroyed : public Event {
	public:
		Destroyed(const Unit& unit);
		~Destroyed();

		const Unit & unit;
	};

	class Losses : public Event {
	public:
		Losses(const Unit& unit, const int lost);
		~Losses();

		const int lost;
	};

	class TacticalBonus : public Event {
	public:
		TacticalBonus(const Unit& unit, const int bonus);
		~TacticalBonus();

		const int bonus;
	};

	class QuestCompleted : public Event {
	public:
		QuestCompleted(const std::string& rewards);
		~QuestCompleted();

		std::string& rewards;
	};

	enum AttackOutcome {
		FAIL, MISS, BLOCK, HIT, KILL, CHARMED, WASTED
	};

	struct Weapon {
		WeaponType* type;
		WeaponClass weaponClass;
	};
	
	struct Magic {
		SpecialType* special;
		EffectType* effect;
	};

	struct Target {
		Soldier soldier;
		AttackOutcome outcome;
		int attackLevel;
		int defenseLevel;
		float protection;
		int damage;
		bool killed;
	};

	class Attack : public Event {
	public:
		Attack(const ::Soldier& attacker, const AttackType attackType, const int possibleAttacks);
		~Attack();

		const Soldier attacker;
		const std::vector<const Target*> targets;
		const AttackType attackType;

		const int possibleAttacks;
		bool deflected;

		Weapon* weapon;
		Magic* magic;

		Target* AddTarget(const ::Soldier& target);
	};

	class Heal : public Event {
	public:
		Heal();
		~Heal();
	};

	class CastShield : public Event {
	public:
		CastShield(const ::Soldier& mage, const SpecialType* special);
		~CastShield();

		const Soldier mage;
		const SpecialType* special;
	};

	class Round {
	public:
		Round(int number, bool isFree);

		int number;
		bool isFree;
		std::vector<const Event *> events;

		void SetLeader(const Unit & leader);

		void Add(const Event * event);
	};

	class Battle {
	public:
		Battle(const Unit& attacker, const Unit& defender, const bool assassination);
		~Battle();

		Round* FreeRound(const ::Unit& leader);
		Round* NormalRound();

		const bool assassination;
		const Unit attacker;
		const Unit defender;

		std::vector<Round*> round;
	};

	class Reporter {
	public:
		virtual ~Reporter() = 0;

		virtual void Write(Battle& battle, Faction* faction) = 0;
	};

	class TextReporter : public Reporter {
	public:
		TextReporter(const Areport* report);
		virtual ~TextReporter();


		void Write(const Battle& battle, const Faction* faction);

	private:
		const Areport* report;

		void Line(const std::string& s);
		void EmptyLine();
	};
}

struct AttackStat {
	std::string effect;

	int weaponIndex;
	int attackType;
	int weaponClass;

	int soldiers;

	int attacks;
	int failed;
	int missed;
	int blocked;
	int hit;

	int damage;
	int killed;
};

struct DamageStat {
	std::string effect;

	int weaponIndex;
	int attackType;
	int weaponClass;

	int soldiers;

	int attacks;
	int failed;
	int missed;
	int blocked;
	int hit;

	int damage;
	int killed;
};

class BattleStructure {
    public:
        int number;
        std::string type;
        std::string name;
        std::string description;
        int size;
        std::vector<int> defense;
};

class UnitStat {
    public:
        int number;
        std::string unitName;
        std::vector<AttackStat> attackStats;
        int figures;
        int hp;
        int lostFigures;
        int lostHp;
        bool behind;
};

namespace unit_stat_control {
	void Clear(UnitStat& us);
	AttackStat* FindStat(UnitStat& us, int weaponIndex, SpecialType* effect);
	void TrackSoldier(UnitStat& us, int weaponIndex, SpecialType* effect, int attackType, int weaponClass);
	void RecordAttack(UnitStat& us, int weaponIndex, SpecialType* effect);
	void RecordAttackFailed(UnitStat& us, int weaponIndex, SpecialType* effect);
	void RecordAttackMissed(UnitStat& us, int weaponIndex, SpecialType* effect);
	void RecordAttackBlocked(UnitStat& us, int weaponIndex, SpecialType* effect);
	void RecordHit(UnitStat& us, int weaponIndex, SpecialType* effect, int damage);
	void RecordKill(UnitStat& us, int weaponIndex, SpecialType* effect);
};

class ArmyStats {
	public:
		// key is unit number
		std::map<int, UnitStat> roundStats;
		std::map<int, UnitStat> battleStats;

		void ClearRound();

		void TrackUnit(Unit *unit);

		void TrackSoldier(int unitNumber, int weaponIndex, SpecialType* effect, int attackType, int weaponClass);
		void RecordAttack(int unitNumber, int weaponIndex, SpecialType* effect);
		void RecordAttackFailed(int unitNumber, int weaponIndex, SpecialType* effect);
		void RecordAttackMissed(int unitNumber, int weaponIndex, SpecialType* effect);
		void RecordAttackBlocked(int unitNumber, int weaponIndex, SpecialType* effect);
		void RecordHit(int unitNumber, int weaponIndex, SpecialType* effect, int damage);
		void RecordKill(int unitNumber, int weaponIndex, SpecialType* effect);
};

#endif