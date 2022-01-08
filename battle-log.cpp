#include "battle-log.h"

#include <sstream>

namespace BattleLog {
    TextReporter::~TextReporter() {

    }

    const std::string& Header(const Battle& battle) {
        std::ostringstream ss;
        
        ss << battle.attacker.name;
        ss << (battle.assassination ? " attempts to assassinate " : " attacks ");
        ss << battle.defender.name << " in " << "location" << "!";

        return ss.str();
    }

    void TextReporter::Write(const Battle& battle, const Faction* faction) {
        // report->PutStr()

        this->Line(Header(battle));
        this->EmptyLine();

        this->Line("Attackers:");
        this->EmptyLine();

        this->Line("Defenders:");
        this->EmptyLine();

        for (auto &round : battle.round) {
            for (auto &event : round->events) {
                
            }
        }

        this->Line("Total Casualties:");
        this->Line("Spoils: none.");
    }
}
