#include <vector>

class Formation;

#include "army.h"

class Formation {
    public:
        Formation();
        ~Formation();

        void AddSoldier(Soldier *);

        int GetSize();
        int GetTotalHitpoints();

    private:
        std::vector<Soldier *> soldiers;
};
