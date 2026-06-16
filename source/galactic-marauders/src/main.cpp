// Copyright 2011 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include <stdint.h>
#include <cassert>
#include <vector>
#include <map>

class GalacticMarauders {
public:
    static constexpr uint32_t end_of_list = 0xffff;
    static constexpr uint32_t text_animate_counter = 8333333;
    static constexpr uint32_t enemy_type_count = 11;

    // frame ids
    struct AnimationFrame {
        enum Enum : uint16_t {
            _null = 0,
            _null_persist,
            enemy_00_a,
            enemy_01_a,
            enemy_02_a,
            enemy_03_a,
            enemy_04_a,
            enemy_05_a,
            enemy_06_a,
            enemy_07_a,
            enemy_08_a,
            enemy_09_a,
            enemy_10_a,
            enemy_00_b,
            enemy_01_b,
            enemy_02_b,
            enemy_03_b,
            enemy_04_b,
            enemy_05_b,
            enemy_06_b,
            enemy_07_b,
            enemy_08_b,
            enemy_09_b,
            enemy_10_b,
            player_ship_0,
            player_ship_1,
            player_shot,
            enemy_shot,
            easy_0,
            easy_1,
            player_boom_0,
            player_boom_1,
            player_boom_2,
            player_boom_3,
            player_boom_4,
            player_boom_5,
            player_boom_6,
            enemy_boom_0,
            enemy_boom_1,
            enemy_boom_2,
            enemy_boom_3,
            enemy_boom_4,
            enemy_boom_5,
            enemy_boom_6,
            local_player_0,
            local_player_1,
            text_ready,
            text_no,
            text_great,
            target,
            enemy_11_n_a,
            enemy_11_s_a,
            enemy_11_se_a,
            enemy_11_sw_a,
            enemy_11_nw_a,
            enemy_11_ne_a,
            enemy_11_n_b,
            enemy_11_s_b,
            enemy_11_se_b,
            enemy_11_sw_b,
            enemy_11_nw_b,
            enemy_11_ne_b,
            image_count,
            _end_list,
        };
    };

    struct Flag {
        enum Enum : uint8_t {
            None = 0,
            Active = 1U << 0,
            Allocated = 1U << 1,
            Dead = 1U << 2,
            D = 1U << 3,
            E = 1U << 4,
            F = 1U << 5,
            G = 1U << 6,
            H = 1U << 7
        };
    };

    struct Type {
        enum Enum : uint8_t {
            Null = 0,
            Player,
            Enemy,
            Bullet,
            BadBullet,
            Boom,
            PlayerBoom,
            Count
        };
    };

    // for doing collision detection
    class Bounds {
    public:
        uint16_t entity;
        uint8_t btype;
        int32_t lower_x;
        int32_t lower_y;
        int32_t upper_x;
        int32_t upper_y;
   
        Bounds(uint16_t e, uint8_t bt, int32_t px, int32_t py, int32_t sx, int32_t sy) : 
            entity(e),
            btype(bt),
            lower_x(px - sx),
            lower_y(py - sy),
            upper_x(px + sx),
            upper_y(py + sy)
        { }

        bool operator<(const Bounds& other) const {
            return lower_x < other.lower_x;
        }

        bool overlap(const Bounds& other) const {
            if (other.lower_x > upper_x || other.upper_x < lower_x ||
                other.lower_y > upper_y || other.upper_y < lower_y) {
                return false;
            }
            return true;
        }
    };

    // Bit field packing, use 'sizeof' and 'offsetof' for portability
    struct Input {
        uint8_t connected : 1;
        uint8_t left : 1;
        uint8_t right : 1;
        uint8_t primary : 1;
    };

    struct Entity {
        int8_t slot;
        uint8_t flags;
        uint8_t type;
        uint16_t frame;
        int8_t lastDir;
        uint16_t count;
        uint16_t delayFire;     // also used for memory pool linked list
        int32_t xpos;
        int32_t ypos;
        int32_t xvel;
        int32_t yvel;
        int32_t xsize;
        int32_t ysize;
    };

    struct State {
        uint16_t head;
        uint16_t textType;
        uint32_t textAnimate;
        uint32_t enemySpeed;
        uint32_t enemyCount;
        uint16_t enemyCounter;
        int8_t enemyDirection;
        bool firstFrame;
        bool playing;
        bool gameOver;
    };

    // fast forward mode
    bool fastForward;

    // static data
    std::vector<Entity> prefabs;
    std::map<uint16_t, uint16_t> animations;

    // game state
    uint32_t seed;
    State state;
    std::vector<Input> input;
    std::vector<Entity> entities;

    // temp data for collisions
    std::vector<Bounds> boundPlayer;
    std::vector<Bounds> boundEnemy;
    std::vector<Bounds> boundBadBullet;
    std::vector<Bounds> boundBullet;

    // constructor
    GalacticMarauders() {

        fastForward = false;

        state.firstFrame = true;

        state.head = end_of_list;

        // set up initial state
        state.playing = false;
        state.gameOver = false; // do I need this now?

        state.textType = AnimationFrame::text_ready;
        state.textAnimate = 0;

        state.enemySpeed = 2;
        state.enemyCount = 0;
        
        // creation of static data
        prefabs = {
            { -1, Flag::None, Type::Null, AnimationFrame::_null, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
            { -1, Flag::Allocated, Type::Player, AnimationFrame::player_ship_0, 1, 0, 0, 0, 0, 0, 0, 16, 10 },
            { -1, Flag::Allocated, Type::Enemy, AnimationFrame::enemy_00_a, 1, 0, 0, 0, 0, 0, 0, 16, 10 },
            { -1, Flag::Allocated, Type::Bullet, AnimationFrame::player_shot, 1, 0, 0, 0, 0, 0, 8, 12, 20 },
            { -1, Flag::Allocated, Type::BadBullet, AnimationFrame::enemy_shot, 1, 0, 0, 0, 0, 0, -4, 7, 7 },
            { -1, Flag::Allocated, Type::Boom, AnimationFrame::enemy_boom_0, 1, 0, 0, 0, 0, 0, 0, 14, 14 },
            { -1, Flag::Allocated, Type::PlayerBoom, AnimationFrame::player_boom_0, 1, 0, 0, 0, 0, 0, 0, 22, 21 }
        };

        // register player animations
        uint16_t player[] = {
            AnimationFrame::player_ship_0,
            AnimationFrame::player_ship_1
        };
        registerAnimation(player, 2);

        // register player boom animations
        uint16_t player_local[] = {
            AnimationFrame::player_boom_0,
            AnimationFrame::player_boom_1,
            AnimationFrame::player_boom_2,
            AnimationFrame::player_boom_3,
            AnimationFrame::player_boom_4,
            AnimationFrame::player_boom_5,
            AnimationFrame::player_boom_6,
            AnimationFrame::_null_persist
        };
        registerAnimation(player_local, 8);

        // register boom animations
        uint16_t boom[] = {
            AnimationFrame::enemy_boom_0,
            AnimationFrame::enemy_boom_1,
            AnimationFrame::enemy_boom_2,
            AnimationFrame::enemy_boom_3,
            AnimationFrame::enemy_boom_4,
            AnimationFrame::enemy_boom_5,
            AnimationFrame::enemy_boom_6,
            AnimationFrame::_null
        };
        registerAnimation(boom, 8);

         uint16_t playeR_boom[] = {
            AnimationFrame::local_player_0,
            AnimationFrame::local_player_1
        };
        registerAnimation(playeR_boom, 2);

        // register enemy animations
        uint16_t enemy_ani[] = {
            AnimationFrame::enemy_00_a,
            AnimationFrame::enemy_00_b
        };
        for (size_t i = 0; i < enemy_type_count; ++i) {
            registerAnimation(enemy_ani, 2);
            ++enemy_ani[0];
            ++enemy_ani[1];
        }

    }

    void setPlayerCount(uint32_t count) {
        input.resize(count);
    }

    // linear congruential generator
    uint32_t next_rand() {
        uint32_t a = 1103515245;
        uint32_t c = 12345;
        uint32_t m = 2147483648;
        seed = (a * seed + c) % m;
        return seed;
    }

    // optimized copy for fast forward estimation
    void smartCopy(GalacticMarauders& other) {
        seed = other.seed;
        state = other.state;
        if (input.size() < other.input.size()) {
            input.resize(other.input.size());
        }
        if (entities.size() < other.entities.size()) {
            entities.resize(other.entities.size());
        }
        std::memcpy(&(input[0]), &(other.input[0]), sizeof(Input) * input.size());
        std::memcpy(&(entities[0]), &(other.entities[0]), sizeof(Entity) * entities.size());
    }

    // registers an animation based on a list of frames
    void registerAnimation(uint16_t* list, size_t size) {
        size_t i = 0;
        while (i < size - 1) {
            if (list[i] != AnimationFrame::_null && list[i] != AnimationFrame::_null_persist) {
                animations.insert(std::pair(list[i], list[i+1]));
            }
            ++i;
        }
        if (list[i] != AnimationFrame::_null && list[i] != AnimationFrame::_null_persist) {
            animations.insert(std::pair(list[i], list[0]));
        }
    }

    // allocate new entity
    uint16_t allocate() {
        uint16_t index = 0;
        if (state.head == end_of_list) {
            index = entities.size();
            if (entities.size() < index + 1) {
                entities.resize(index + 1);
            }
        } else {
            index = state.head;
            state.head = entities[index].delayFire;
        }
        entities[index].flags |= Flag::Allocated;
        return index;
    }

    // free memory for entity
    void deallocate(uint16_t index) {
        assert((entities[index].flags & Flag::Allocated) == Flag::Allocated);
        entities[index] = prefabs[Type::Null];
        entities[index].delayFire = state.head;
        state.head = index;
    }

    // main update function for a single game frame
    bool update() {

        // first frame stuff
        if (state.firstFrame == true) {
            // do first frame stuff!
            for (size_t j = 0; j < input.size(); ++j) {
                if (input[j].connected == true) {
                    int32_t pl = (int32_t)j;
                    uint32_t index = allocate();
                    Entity& e = entities[index];
                    e = prefabs[Type::Player];
                    e.slot = pl;
                    size_t wrap = pl % 32;
                    e.xpos = wrap * 60 - 960 + 32;
                    e.ypos = -500;
                    e.flags |= Flag::Active;
                }
            }
            state.firstFrame = false;
        }

        // do so pre-computations for enemies and players
        bool livePlayer = false;
        state.enemyCount = 0;
        state.enemyCounter += 1;
        if (state.enemyCounter > 300) {
            state.enemyCounter = 0;
            state.enemyDirection = -state.enemyDirection;
        }

        // mask for an allocated and active entity
        uint8_t ftest = (Flag::Active | Flag::Allocated);

        // iterate entities
        for (size_t i = 0; i < entities.size(); ++i) {
            Entity& e = entities[i];
            if ((e.flags & ftest) == ftest) {
               
                //update animators
                e.count += 1;
                if (e.count > 6) {
                    e.count = 0;
                    auto it = animations.find(e.frame);
                    if (it != animations.end()) {
                        e.frame = animations[e.frame];
                    }
                }
                if (e.frame == AnimationFrame::_null) {
                    deallocate(i);
                }
                
                // update players and enemies
                switch (e.type) {
                    case Type::Player: {
                        auto& slot = input[e.slot];

                        e.xvel = 0;
                        if (e.delayFire > 0) {
                            e.delayFire -= 1;
                        }

                        if ((e.flags & Flag::Dead) == Flag::Dead) {
                            // DO NOTHING!
                        } else {
                            if (slot.connected == true) {
                                livePlayer = true;
                            } else {
                                // only enforce if we aren't fast forwarding
                                if (fastForward == false) {
                                    // kill if disconnected!
                                    e.flags |= Flag::Dead;
                                    e.type = Type::PlayerBoom;
                                    e.frame = AnimationFrame::player_boom_0;
                                    e.xvel = 0;
                                    e.count = 0;
                                }
                            }

                            if (e.xpos < -960) { e.xpos = -960; }
                            if (e.xpos > 960) { e.xpos = 960; }
                            
                            //if (slot.left == true) { e.xvel = -3; }
                            //if (slot.right == true) { e.xvel = 3; }
                            if (slot.left == true && slot.right == true) {
                                e.xvel = e.lastDir * 3;                                
                            } else if (slot.left == true) {
                                e.xvel = -3;
                                e.lastDir = 1;
                            } else if (slot.right == true) {
                                e.xvel = 3;
                                e.lastDir = -1;
                            } else {
                                e.xvel = 0;
                                e.lastDir = 1;
                            }

                            if (slot.primary == true && e.delayFire == 0) {
                                e.delayFire = 24;
                                // don't create bullet if in fast forward mode
                                if (fastForward == false) {
                                    uint32_t index = allocate();
                                    Entity& n = entities[index];
                                    n = prefabs[Type::Bullet];
                                    n.xpos = e.xpos;
                                    n.ypos = e.ypos;
                                    n.flags |= Flag::Active;
                                }
                            }

                        }
                    } break;

                    case Type::Enemy: {
                        // set enemy direction
                        e.xvel = state.enemyDirection * state.enemySpeed;

                        // advance counter
                        if (e.delayFire > 0) {
                            e.delayFire -= 1;
                        }

                        // fire a bullet
                        if (e.delayFire == 0) {
                            //e.delayFire = 2000;
                            e.delayFire = next_rand() % 4000;
                            // don't create bullet if in fast forward mode
                            if (fastForward == false) {
                                uint32_t index = allocate();
                                Entity& n = entities[index];
                                n = prefabs[Type::BadBullet];
                                n.xpos = e.xpos;
                                n.ypos = e.ypos;
                                n.flags |= Flag::Active;
                            }
                        }

                        // counting active enemies
                        state.enemyCount += 1;
                    } break;

                    default: {
                        // do nothing
                    } break;
                };
                
                // integrate
                e.xpos += e.xvel;
                e.ypos += e.yvel;

                // destroy if it goes off screen
                if (e.ypos > 550 || e.ypos < -550) {
                    deallocate(i);
                }
            }
        }

        if (fastForward == false) {

            if (state.playing == true && livePlayer == false) {
                state.playing = false;
                state.textAnimate = 0;
                state.textType = AnimationFrame::text_no;
                // destory all enemies
                for (size_t j = 0; j < entities.size(); ++j) {
                    if (((entities[j].flags & Flag::Allocated) == Flag::Allocated) && entities[j].type == Type::Enemy) {
                        deallocate(j);
                    }
                }
            }

            // calculate enemy speed based on count
            state.enemySpeed = 3;

            // if enemy count is zero, set playing to false, text to great job!
            if (state.playing == true && state.enemyCount == 0) {
                state.playing = false;
                state.textAnimate = 0;
                state.textType = AnimationFrame::text_great;
            }

            // resolve state
            if (state.playing == false) {
                state.textAnimate += text_animate_counter;
                if (state.textAnimate > 1000000000) {
                    if (state.textType != AnimationFrame::text_ready) {
                        
                        // set to ready message
                        state.textAnimate = 0;
                        state.textType = AnimationFrame::text_ready;
                        // reset ships here, not needed if we go back to lobby!
                        for (size_t i = 0; i < entities.size(); ++i) {
                            Entity& e = entities[i];
                            if ((e.flags & ftest) == ftest && e.type == Type::PlayerBoom) {
                                if (input[e.slot].connected == true) {
                                    int8_t pl = e.slot;
                                    e = prefabs[Type::Player];
                                    e.slot = pl;
                                    size_t wrap = pl % 32;
                                    e.xpos = wrap * 60 - 960 + 32;
                                    e.ypos = -500;
                                    e.flags |= Flag::Active;
                                }
                            }
                        }

                        // use this if you want to go back to lobby after game end
                        state.gameOver = true;
                    } else {
                        int32_t i, j;

                        state.playing = true;

                        state.enemyDirection = 1;
                        state.enemyCounter = 0;
                        
                        for (j = 0; j < 24; ++j) {   
                            for (i = 0; i < 20; ++i) {
                                uint32_t index = allocate();
                                Entity& e = entities[index];
                                e = prefabs[Type::Enemy];
                                e.xpos = j * 60 - 960 + 32;
                                e.ypos = i * 32 - 100;
                                //e.frame = (state.random.next() % enemy_type_count) + 2;
                                //e.delayFire = state.random.next() % 2000;
                                e.frame = (next_rand() % enemy_type_count) + 2;
                                e.delayFire = next_rand() % 4000;
                                e.flags |= Flag::Active;
                            }
                        }

                    }
                }
            }

            // do contacts last!

            // clear the bounds list
            boundEnemy.clear();
            boundPlayer.clear();
            boundBadBullet.clear();
            boundBullet.clear();
            
            // fill up the bounds list with objects
            for (size_t i = 0; i < entities.size(); ++i) {
                Entity& e = entities[i];
                if ((e.flags & ftest) == ftest) {
                    Bounds bound = Bounds(i, e.type, e.xpos, e.ypos, e.xsize, e.ysize);
                    switch (e.type) {
                        case Type::Player:    boundPlayer.push_back(bound);    break;
                        case Type::Enemy:     boundEnemy.push_back(bound);     break;
                        case Type::Bullet:    boundBullet.push_back(bound);    break;
                        case Type::BadBullet: boundBadBullet.push_back(bound); break;
                        default: break;
                    };
                }
            }

            size_t i, j;

            // players vs enemy bullets
            for (i = 0; i < boundPlayer.size(); ++i) {
                for (j = 0; j < boundBadBullet.size(); ++j) {
                    Bounds& player = boundPlayer[i];
                    Bounds& bullet = boundBadBullet[j];
                    Entity& e = entities[player.entity];
                    if ((e.flags & ftest) == ftest && (entities[bullet.entity].flags & ftest) == ftest) {
                        if (player.overlap(bullet) == true) {
                            e.flags |= Flag::Dead;
                            e.type = Type::PlayerBoom;
                            e.xvel = 0;
                            e.frame = AnimationFrame::player_boom_0;
                            e.count = 0;
                            deallocate(bullet.entity);
                        }
                    }
                }
            }

            // enemies vs player bullets
            for (i = 0; i < boundEnemy.size(); ++i) {
                for (j = 0; j < boundBullet.size(); ++j) {
                    Bounds& enemy = boundEnemy[i];
                    Bounds& bullet = boundBullet[j];
                    Entity& e = entities[enemy.entity];
                    if ((e.flags & ftest) == ftest && (entities[bullet.entity].flags & ftest) == ftest) {
                        if (enemy.overlap(bullet) == true) {
                            e.type = Type::Boom;
                            e.xvel = 0;
                            e.frame = AnimationFrame::enemy_boom_0;
                            e.count = 0;
                            deallocate(bullet.entity);
                        }
                    }
                }
            }
            
        }

        return state.gameOver;
    }

};


// begin adaptor code

#include <emscripten/bind.h>


class BindGame {
public:
    BindGame() {
       guess.fastForward = true;
    }

    void setup(emscripten::val v) {        
        game.seed = v["seed"].as<uint32_t>();
        game.setPlayerCount( v["playerCount"].as<uint32_t>() );
        local = v["local"].as<int32_t>();
    }

    bool update() {
        return game.update();        
    }

    void copy() {
        guess.smartCopy(game);
    }

    void forward() {
        guess.update();
    }

    void render(emscripten::val v) {
        
        GalacticMarauders::Entity* p = nullptr;

        for (auto&& e : guess.entities) {
            //if ((e.flags | GalacticMarauders::Flag::Active) & GalacticMarauders::Flag::Active) {
            if ((e.flags & GalacticMarauders::Flag::Active) == GalacticMarauders::Flag::Active) {
                if (e.slot == local) {
                    p = &e;
                } else {
                    v.call<void>("sprite", (float)e.xpos, -(float)e.ypos, e.frame, 2.0f, 1.0f);
                }
            }
        }

        // draw local ship
        if (p != nullptr) {
            uint32_t frame = p->frame;
            if (frame == GalacticMarauders::AnimationFrame::player_ship_0) { 
                frame = GalacticMarauders::AnimationFrame::local_player_0;
            }
            if (frame == GalacticMarauders::AnimationFrame::player_ship_1) {
                frame = GalacticMarauders::AnimationFrame::local_player_1;
            }
            v.call<void>("sprite", (float)p->xpos, -(float)p->ypos, frame, 2.0f, 1.0f);
        }

        for (auto&& e : game.entities) {
            //if ((e.flags | GalacticMarauders::Flag::Active) & GalacticMarauders::Flag::Active) {
            if ((e.flags & GalacticMarauders::Flag::Active) == GalacticMarauders::Flag::Active) {
                if (e.slot == local) {
                    p = &e;
                } else {
                    v.call<void>("sprite", (float)e.xpos, -(float)e.ypos, e.frame, 2.0f, 0.5f);
                }
            }
        }

        // draw local ship
        if (p != nullptr) {
            uint32_t frame = p->frame;
            if (frame == GalacticMarauders::AnimationFrame::player_ship_0) { 
                frame = GalacticMarauders::AnimationFrame::local_player_0;
            }
            if (frame == GalacticMarauders::AnimationFrame::player_ship_1) {
                frame = GalacticMarauders::AnimationFrame::local_player_1;
            }
            v.call<void>("sprite", (float)p->xpos, -(float)p->ypos, frame, 2.0f, 0.5f);
        }

        // renders text
         // render text!
        if (game.state.playing == false) {
            //float textScale = 900.0f * (float)game.State.textAnimate;
            //        0_008333333
            // Const.v0_00833333333333333
    
            // i guess f32 has no try_from i32?  seems like it shoud...
            //let tx: f32 = f32::try_from(self.game.global.textAnimate).unwrap() / 1000000000.0;
            float tx = (float)game.state.textAnimate / 1000000000.0f;
            float c1 = 1.70158f;
            float c3 = c1 + 1.0f;
    
            //godot_print!("the amount {}!", tx);
    
            //float textScale = 1 + c3 * (float)Math.Pow(x - 1, 3) + c1 * (float)Math.Pow(x - 1, 2);
            //let mut textScale = 1.0 - f32::powf(1.0 - tx, 3.0);
            //let mut textScale = 1.0 - 3i32.pow(1.0 - tx);
            float textScale = 1.0f - std::pow(1.0f - tx, 3.0f);
            
            textScale *= 8.0f;
            textScale *= 0.5f;
    
            //textScale = 2.0;

            v.call<void>("sprite", 0.0f, 0.0f, game.state.textType, textScale, 1.0f);

        }        

    }

    void getInput(emscripten::val v, emscripten::val buffer) {
        GalacticMarauders::Input input;
        uint8_t* ptr = (uint8_t*)(&input);
        size_t pos = 0;
        input.connected = 0;
        input.left = (v["left"].as<bool>()) ? 1 : 0;
        input.right = (v["right"].as<bool>()) ? 1 : 0;
        input.primary = (v["primary"].as<bool>()) ? 1 : 0;
        for (uint32_t i = 0; i < sizeof(input); ++i) {
            buffer.set(pos++, ptr[i]);
        }
    }

    void setInput(emscripten::val buffer) {
        setInputFunc(buffer, game.input);
    }

    void setForwardInput(emscripten::val buffer) {
        setInputFunc(buffer, guess.input);
    }

    void setInputFunc(emscripten::val buffer, std::vector<GalacticMarauders::Input>& inputVec) {
        //GalacticMarauders::Input input;
        //printf("size of input: %d\n", sizeof(input));

        uint8_t* ptr;
        size_t pos = 0;

        uint32_t frame;

        // read in frame
        ptr = (uint8_t*)(&frame);
        for (uint32_t j = 0; j < 4; ++j) {
            ptr[j] = buffer[pos++].as<uint8_t>();
        }
        
        // read in player inputs
        for (size_t i = 0; i < inputVec.size(); ++i) {
            GalacticMarauders::Input& input = inputVec[i];
            ptr = (uint8_t*)(&input);
            for (uint32_t j = 0; j < sizeof(input); ++j) {
                ptr[j] = buffer[pos++].as<uint8_t>();
            }
            //printf("first byte: %d\n", (int32_t)firstByte);
            input.connected = ptr[0] != 0xff;
        }
    }

    
private:
    GalacticMarauders game;
    GalacticMarauders guess;
    int32_t local;

};


// Binding code
EMSCRIPTEN_BINDINGS(my_class_example) {
    emscripten::class_<BindGame>("BindGame")
        .constructor()
        .function("update", &BindGame::update)
        .function("copy", &BindGame::copy)
        .function("forward", &BindGame::forward)
        .function("render", &BindGame::render)
        .function("setup", &BindGame::setup)
        .function("setInput", &BindGame::setInput)
        .function("setForwardInput", &BindGame::setForwardInput)
        .function("getInput", &BindGame::getInput)
        ;
}