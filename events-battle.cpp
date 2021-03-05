// START A3HEADER
//
// This source file is part of the Atlantis PBM game program.
// Copyright (C) 2020 Valdi Zobēla
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program, in the file license.txt. If not, write
// to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// See the Atlantis Project web page for details:
// http://www.prankster.com/project
//
// END A3HEADER

#include "gameio.h"
#include "events.h"

#include <memory>
#include <string>
#include <stdexcept>

template<typename ... Args> std::string string_format( const std::string& format, Args ... args )
{
    int size = std::snprintf(nullptr, 0, format.c_str(), args ... ) + 1;
    if (size <= 0) {
        throw std::runtime_error( "Error during formatting." );
    }
    
    std::unique_ptr<char[]> buf(new char[ size ]); 
    snprintf(buf.get(), size, format.c_str(), args ...);

    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

BattleFact::BattleFact() {
    this->attacker = BattleSide();
    this->defender = BattleSide();
    this->location = EventLocation();
}

BattleFact::~BattleFact() {

}

const int N_MESSAGES = 4;

const char* ADJECTIVE[N_MESSAGES] = {
    "Some",
    "Many",
    "Several",
    "Dozen"
};

const char* REPORTING[N_MESSAGES] = {
    "traders",
    "pilgrims",
    "travelers",
    "adventurers"
};

const char* CHANNEL[N_MESSAGES] = {
    "they have heard about",
    "refugees are teeling about",
    "locals are afraid of",
    "in some distant land"
};

const char* SMALL_BATTLE[N_MESSAGES] = {
    "encounter",
    "fight",
    "skrimish",
    "battle"
};

const char* BATTLE[N_MESSAGES] = {
    "clash",
    "fight",
    "assault",
    "battle"
};

const char* SIDES[N_MESSAGES] = {
    "hostile forces",
    "enemies",
    "two armies",
    "combatants"
};

const char* SIZES[N_MESSAGES] = {
    "couple",
    "few",
    "several",
    "many"
};

std::string relativeSize(int size) {
    if (size < 3) return SIZES[0];
    if (size < 12) return SIZES[1];
    if (size < 24) return SIZES[2];
    
    return SIZES[3];
}

void BattleFact::GetEvents(std::list<Event> &events) {
    // Some traders are telling that in some distant land
    std::string text = string_format("%s %s are telling that %s ",
        ADJECTIVE[getrandom(N_MESSAGES)],
        REPORTING[getrandom(N_MESSAGES)],
        CHANNEL[getrandom(N_MESSAGES)]
    );

    if (this->defender.factionNum == 1) {
        // city capture
        events.push_back({ EventCategory::EVENT_CITY_CAPTURED, 1, text });
        return;
    }

    if (this->defender.factionNum == 2) {
        // monster hunt
        events.push_back({ EventCategory::EVENT_MONSTERS_SLAIN, 1, text });
        return;
    }

    if (this->attacker.factionNum == 2) {
        // monster aggression
        events.push_back({ EventCategory::EVENT_MONSTERS_WIN, 1, text });
        return;
    }

    // PvP
    int total = this->attacker.total + this->defender.total;
    int totalLost = this->attacker.lost + this->defender.lost;
    int totalMages = this->attacker.mages + this->defender.mages;
    int totalMonsters = this->attacker.monsters + this->defender.monsters;
    int totalUndead = this->attacker.undead + this->defender.undead;
    int totalFMI = this->attacker.fmi + this->defender.fmi;

    int minLost = std::min(this->attacker.lost, this->defender.lost);
    int maxLost = std::max(this->attacker.lost, this->defender.lost);
    bool isSlaughter = (maxLost / minLost) > 10;

    int score = 0;

    if (total <= 100) {
        // encounter
        // location known

        std::string result;
        if (this->outcome == BATTLE_DRAW) {
            result = "and neither side won";
        }
        else if (isSlaughter) {
            result = "ended in a slaughter";
        }
        else {
            result = totalLost > total / 2
                ? "and some men died"
                : "and many soldiers will never fight again";
        }

        // a small encounter between hostile forces in the woods of Sansaor took place some men died.
        text += string_format("a small %s between %s in the %ss of %s took place %s.",
            SMALL_BATTLE[getrandom(N_MESSAGES)],
            SIDES[getrandom(N_MESSAGES)],
            this->location.getTerrain(),
            this->location.province,
            result
        );

        score = 1;
    }
    else if (total <= 500) {
        // local conflict
        // location known
        // number of looses known

        int unceretanity = totalLost / 3;   // 33%
        int lost = totalLost + getrandom(unceretanity) - (unceretanity / 2);

        std::string result;
        if (this->outcome == BATTLE_DRAW) {
            result = "and neither side won";
        }
        else if (isSlaughter) {
            result = "ended in a slaughter";
        }
        else {
            result = "and many soldiers will never fight again";
        }

        // a battle between two armies in the woods of Sansaor took place with .
        text += string_format("a %s between %s in the %ss of %s took place with %i killed from both sides %s.",
            SMALL_BATTLE[getrandom(N_MESSAGES)],
            SIDES[getrandom(N_MESSAGES)],
            this->location.getTerrain(),
            this->location.province,
            lost,
            result
        );

        score = 2;
    }
    else if (total <= 2500) {
        // regional conflict
        // location known
        // number of looses known
        // mages, monsters, fmi

        std::string specials;
        if (totalFMI + totalMages + totalMonsters + totalUndead) {
            specials = " with use of ";

            bool second = false;
            if (totalMages) {
                specials += "magic";
                second = true;
            }

            if (totalFMI) {
                if (second) specials += ", ";
                specials += "mechanisms";
                second = true;
            }

            if (totalUndead) {
                if (second) specials += ", ";
                specials += "undead";
                second = true;
            }

            if (totalMonsters) {
                if (second) specials += ", ";
                specials += "monsters";
                second = true;
            }
        }

        int unceretanity = totalLost / 5;   // 20%
        int lost = totalLost + getrandom(unceretanity) - (unceretanity / 2);

        std::string result;
        if (this->outcome == BATTLE_DRAW) {
            result = "and neither side won";
        }
        else if (isSlaughter) {
            result = "ended in a slaughter";
        }
        else {
            result = "and many soldiers will never fight again";
        }

        // a battle with use of magic between two armies in the woods of Sansaor took place with .
        text += string_format("a %s%s between %s in the %ss of %s took place with %i killed from both sides %s.",
            BATTLE[getrandom(N_MESSAGES)],
            specials,
            SIDES[getrandom(N_MESSAGES)],
            this->location.getTerrain(),
            this->location.province,
            lost,
            result
        );

        score = 3;
    }
    else {
        // continental conflict / epic conflict
        // location known
        // number of looses known
        // mages, monsters, fmi

        std::string specials;
        if (totalFMI + totalMages + totalMonsters + totalUndead) {
            specials = " with use of ";

            bool second = false;
            if (totalMages) {
                specials += relativeSize(totalMages) + " mages";
                second = true;
            }

            if (totalFMI) {
                if (second) specials += ", ";
                specials += relativeSize(totalFMI) + " mechanisms";
                second = true;
            }

            if (totalUndead) {
                if (second) specials += ", ";
                specials += relativeSize(totalUndead) + " undead";
                second = true;
            }

            if (totalMonsters) {
                if (second) specials += ", ";
                specials += relativeSize(totalMonsters) + " monsters";
                second = true;
            }
        }

        int unceretanity = totalLost / 10;   // 10%
        int lost = totalLost + getrandom(unceretanity) - (unceretanity / 2);

        std::string result;
        if (this->outcome == BATTLE_DRAW) {
            result = "and neither side won";
        }
        else if (isSlaughter) {
            result = "ended in a slaughter";
        }
        else {
            result = "and many soldiers will never fight again";
        }

        // a battle with use of magic between two armies in the woods of Sansaor took place with .
        text += string_format("a epic %s%s between %s in the %ss of %s took place with %i killed from both sides %s.",
            BATTLE[getrandom(N_MESSAGES)],
            specials,
            SIDES[getrandom(N_MESSAGES)],
            this->location.getTerrain(),
            this->location.province,
            lost,
            result
        );

        score = 5;
    }

    if (totalMages > 0) score *= 2;
    if (totalMonsters > 0) score += 1;
    if (totalUndead > 0) score += 1;
    if (totalFMI > 0) score += 1;

    events.push_back({ EventCategory::EVENT_BATTLE, score, text });
}
