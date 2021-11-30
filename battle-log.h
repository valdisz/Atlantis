#ifndef BALLTE_LOG
#define BALLTE_LOG

#include <string>
#include <vector>
#include <map>

enum AttackOutcome {
    FAIL, MISS, BLOCK, HIT
};

class BattleAttack {
    public:
        int attacker;
        int defender;

        std::string effect;
        int weaponIndex;
        int attackType;
        int weaponClass;

        AttackOutcome outcome;

        int damage;
        bool killed;
};

class BattleHeal {
    public:
        int healer;
        int healed;
        int chance;
};

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
        std::string name;
        std::vector<AttackStat> attacks;
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