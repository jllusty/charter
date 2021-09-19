#include "entity.hpp"
#include "component.hpp"
#include "context.hpp"

#include <set>
#include <map>
#include <memory>

namespace c {
    using namespace std;
    struct position : component<position> {
        int x;
    };
    struct velocity  : component<velocity>{
        int x;
    };
    struct player : component<player> {};
    struct enemy : component<enemy> {};
}
namespace AI {
    using namespace std;
    using namespace c;
    // components -> object compression structs
    struct Player {
        position::ptr pos;
        Player(position::ptr pos) : pos(pos) {}
    };
    struct Enemy {
        position::ptr pos;
        Enemy(position::ptr pos) : pos(pos) {}
    };
    using PlayerPtr = shared_ptr<Player>;
    using EnemyPtr = shared_ptr<Enemy>;
    struct Data {
        unordered_map<entity,PlayerPtr> players;
        unordered_map<entity,EnemyPtr> enemies;
        Data() {}
        // perform compression
        void Update(Context& cxt) {
            // update our entities
            for(entity e : cxt.getEntities()) {
                if(cxt.hasComponents<player>(e)) {
                    if(players.count(e) == 0 && cxt.hasComponents<player>(e)) {
                        players.emplace(e,make_unique<Player>(cxt.getComponent<position>(e)));
                    }
                }
                else if(cxt.hasComponents<enemy>(e)) {
                    if(enemies.count(e) == 0 && cxt.hasComponents<enemy>(e)) {
                        enemies.emplace(e,make_unique<Enemy>(cxt.getComponent<enemy>(e)));
                    }
                }
            }
        }
        // perform behaviors: move towards nearest player
        void Run() {
            for(auto [e,enemy] : enemies) {
                float x = enemy->pos->x;
                for(auto [e2,player] : players) {
                    float x2 = player->pos->x;
                }
            }
        }
    };
}

int main() {

}