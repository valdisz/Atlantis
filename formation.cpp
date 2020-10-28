#include "formation.h"

Formation::Formation() {

}

Formation::~Formation() {

}

void Formation::AddSoldier(Soldier *soldier) {
    this->soldiers.push_back(soldier);
}

int Formation::GetSize() {
    return this->soldiers.size();
}

int Formation::GetTotalHitpoints() {
    int totalHits = 0;

    for (auto soldier : this->soldiers) {
        totalHits += soldier->maxhits;
    }

    return totalHits;
}
