#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <random>
#include <array>
#include <numeric>
#include <conio.h>
#include <cassert>

#define CSI "\x1B["

using namespace std;

enum PlayerAction {
    PA_BLIND,
    PA_CHECK,
    PA_FOLD,
    PA_CALL,
    PA_RAISE
};

struct PlayerDecision {
    PlayerAction action;
    int raise;
};

enum PlayerState {
    PS_PLAYING,
    PS_FOLDED,
    PS_OUT
};

struct Player {
    string name;
    int cards[2];
    int stack;
    int bet;                // my bet for this hand
    PlayerState state;
};

enum TableState {
    TS_ANTE,
    TS_PREFLOP,
    TS_FLOP,
    TS_TURN,
    TS_RIVER
};


const int PLAYERS_MAX = 8;

struct Table {
    // players in clockwise direction, player[1] is sitting to the left of player[0], etc.
    // this allows us to increment indices which makes mod calculations simpler
    Player players[PLAYERS_MAX];
    int playerCount;
    int dealerIdx;
    int actorIdx;           // last acted player
    PlayerDecision actorDecision;
    int lastRaiserIdx;
    int lastRaise;
    int bet;
    int pot;
    int blind;
    int cards[5];
    array<int, 52> deck;
    int deckTop;
    TableState state;
};

Table g_table;
random_device g_rd;
mt19937 g_urbg(g_rd());
uniform_real_distribution<> g_random_dist;

const string NAMES[] = { "You", "John", "Cammie", "Jack", "Mary", "Johanna", "Leslie", "Bob" };
const string ACTIONS[] = { "bet", "checked", "folded", "called", "raised" };
const char FACES[] = { '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'K', 'Q', 'A' };
const char SUITS[] = { '\x3', '\x4', '\x5', '\x6' };

double random() {
    return g_random_dist(g_urbg);
}

void init_table() {
    for (int i = 0; i < PLAYERS_MAX; ++i) {
        g_table.players[i].name = NAMES[i];
        g_table.players[i].cards[0] = -1;
        g_table.players[i].cards[1] = -1;
        g_table.players[i].stack = 100;
        g_table.players[i].bet = 0;
        g_table.players[i].state = PS_PLAYING;
    }

    g_table.dealerIdx = -1;
    g_table.playerCount = PLAYERS_MAX;
    g_table.pot = 0;
    for (int i = 0; i < 5; ++i) {
        g_table.cards[i] = -1;
    }
    g_table.blind = 2;
    g_table.state = TS_ANTE;
}

int small_blind_index() {
    return (g_table.dealerIdx + 1) % g_table.playerCount;
}

int big_blind_index() {
    return (g_table.dealerIdx + 2) % g_table.playerCount;
}

void render_card(int card, bool open) {
    if (card == -1) {
        cout << "  ";
        return;
    }

    if (!open) {
        cout << "XX";
        return;
    }

    int face = card % 13;
    int suit = card / 13;
    cout << FACES[face] << SUITS[suit];
}

void render() {
    std::cout << CSI "3J" << CSI "H" << CSI "J" << std::flush;

    // draw players in reverse to simulate clockwise sitting
    for (int i = g_table.playerCount - 1; i >= 0; --i) {
        if (i == g_table.dealerIdx) {
            cout << "    D ";
        } else if (i == big_blind_index()) {
            cout << "   BB ";
        } else if (i == small_blind_index()) {
            cout << "   SB ";
        } else {
            cout << "      ";
        }
        render_card(g_table.players[i].cards[0], i == 0);
        cout << " ";
        render_card(g_table.players[i].cards[1], i == 0);
        cout << setw(10) << g_table.players[i].name << setw(6);
        if (g_table.players[i].bet > 0) {
            cout << g_table.players[i].bet << setw(0) << '$';
        } else {
            cout << ' ' << setw(0) << ' ';
        }
        cout << "   " << g_table.players[i].stack << '$' << endl;
    }

    cout << endl;
    cout << g_table.players[g_table.actorIdx].name << ' ' << ACTIONS[g_table.actorDecision.action];
    if (g_table.actorDecision.action == PA_BLIND || g_table.actorDecision.action == PA_RAISE) {
        cout << ' ' << g_table.lastRaise << '$';
    }
    cout << endl << "Pot: $" << g_table.pot << endl;
}

void shuffle_deck() {
    iota(g_table.deck.begin(), g_table.deck.end(), 0);
    shuffle(g_table.deck.begin(), g_table.deck.end(), g_urbg);
    g_table.deckTop = 0;
}

void player_make_decision() {
    const Player& player = g_table.players[g_table.actorIdx];
    assert(player.state == PS_PLAYING && "Decision for non-active player");

    cout << endl << "You?" << endl;
    cout << "  1 to FOLD" << endl;
    cout << "  2 to RAISE" << endl;
    if (g_table.bet == player.bet) {
        cout << "  3 to CHECK" << endl;
    } else if (player.bet < g_table.bet) {
        cout << "  3 to CALL" << endl;
    }

    g_table.actorDecision.raise = 0;

    int input;
    cin >> input;
    if (input == 2) {
        cout << "Raise to? ";
        cin >> input;
        g_table.actorDecision.action = PA_RAISE;
        g_table.actorDecision.raise = input;

    } else if (g_table.bet == player.bet && input == 3) {
        g_table.actorDecision.action = PA_CHECK;

    } else if (player.bet < g_table.bet && input == 3) {
        g_table.actorDecision.action = PA_CALL;

    } else {
        g_table.actorDecision.action = PA_FOLD;

    }

}

void ai_make_decision() {
    const Player& player = g_table.players[g_table.actorIdx];
    assert(player.state == PS_PLAYING && "Decision for non-active player");

    // our ai acts randomly by selecting valid moves
    g_table.actorDecision.raise = 0;
    double p = random();
    if (g_table.bet == player.bet) {
        g_table.actorDecision.action = PA_CHECK;

    } else if (p < 0.2 && g_table.bet < player.bet + player.stack) {
        // reraise 2-3 times or going all-in
        g_table.actorDecision.action = PA_RAISE;
        int coef = random() < 0.5 ? 2 : 3;
        g_table.actorDecision.raise = g_table.bet + g_table.lastRaise * coef;

    } else if (p < 0.6) {
        g_table.actorDecision.action = PA_FOLD;

    } else if (player.bet < g_table.bet) {
        g_table.actorDecision.action = PA_CALL;

    } else {
        // should never happen
        assert(false && "Invalid AI state: no action taken");
    }
}

void table_apply_decision() {
    Player& player = g_table.players[g_table.actorIdx];
    const PlayerDecision& decision = g_table.actorDecision;

    switch (decision.action) {
    case PA_CHECK:
        // do nothing
        break;
    
    case PA_RAISE:
        {
            // my min allowed bet during re-raise is (g_table.bet + g_table.lastRaise)
            int diff = max(decision.raise, g_table.bet + g_table.lastRaise) - player.bet;
            int raise = min(diff, player.stack);
            player.stack -= raise;
            player.bet += raise;
            if (player.bet > g_table.bet) {
                g_table.lastRaise = player.bet - g_table.bet;
                g_table.lastRaiserIdx = g_table.actorIdx;
                g_table.bet = player.bet;
            }
            g_table.pot += raise;
        }
        break;

    case PA_FOLD:
        player.state = PS_FOLDED;
        player.cards[0] = -1;
        player.cards[1] = -1;
        break;

    case PA_CALL:
        {
            int call = min(g_table.bet - player.bet, player.stack);
            player.stack -= call;
            player.bet += call;
            g_table.bet = max(g_table.bet, player.bet);
            g_table.pot += call;
        }
        break;

    default:
        assert(false && "Invalid decision: no action taken");
    }
}


void update() {
    // table FSM simulates play of one hand
    switch (g_table.state) {
    case TS_ANTE:
        for (int i = 0; i < g_table.playerCount; ++i) {
            g_table.players[i].bet = 0;
        }

        g_table.dealerIdx = (g_table.dealerIdx + 1) % g_table.playerCount;
        g_table.pot = g_table.blind + g_table.blind / 2;
        // todo: player might not have enough for SB/BB, go all-in
        g_table.players[big_blind_index()].bet = g_table.blind;
        g_table.players[big_blind_index()].stack -= g_table.players[big_blind_index()].bet;
        g_table.players[small_blind_index()].bet = g_table.blind / 2;
        g_table.players[small_blind_index()].stack -= g_table.players[small_blind_index()].bet;
        g_table.lastRaiserIdx = big_blind_index();
        g_table.lastRaise = g_table.blind;
        g_table.bet = g_table.blind;

        g_table.actorIdx = big_blind_index();
        g_table.actorDecision.action = PA_BLIND;
        g_table.actorDecision.raise = g_table.blind;
        
        shuffle_deck();
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < g_table.playerCount; ++i) {
                g_table.players[i].cards[j] = g_table.deck[g_table.deckTop++];
            }
        }
        g_table.state = TS_PREFLOP;
        break;

    case TS_PREFLOP:
        // sub-FSM of preflop betting
        do {
            g_table.actorIdx = (g_table.actorIdx + 1) % g_table.playerCount;
            if (g_table.actorIdx == g_table.lastRaiserIdx) {
                g_table.state = TS_FLOP;
                break;
            }
        } while (g_table.players[g_table.actorIdx].state != PS_PLAYING);

        if (g_table.state == TS_PREFLOP) {
            if (g_table.actorIdx == 0) {
                player_make_decision();
            } else {
                ai_make_decision();
            }
            table_apply_decision();
            break;
        }
        /* else fallthru */

    case TS_FLOP:


    default:
        break;
    }

}

int main()
{
    init_table();

    for (;;) {
        update();
        render();

        _getch();
    }

    return 0;
}
