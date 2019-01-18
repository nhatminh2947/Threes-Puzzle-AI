#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <regex>
#include <memory>

#include "Agent.h"
#include "Board64.h"
#include "Action.h"
#include "Episode.h"
#include "Statistic.h"
#include "arena.h"
#include "io.h"

// 0 1 2 3 4 5   6   7   8   9   10  11  12   13   14
// 0 1 2 3 6 12  24  48  92  192 384 768 1536 3072 6144

int shell(int argc, const char *argv[]) {
    arena host("anonymous");

    for (int i = 1; i < argc; i++) {
        std::string para(argv[i]);
        if (para.find("--name=") == 0 || para.find("--account=") == 0) {
            host.set_account(para.substr(para.find("=") + 1));
        } else if (para.find("--login=") == 0) {
            host.set_login(para.substr(para.find("=") + 1));
        } else if (para.find("--save=") == 0 || para.find("--dump=") == 0) {
            host.set_dump_file(para.substr(para.find("=") + 1));
        } else if (para.find("--play") == 0) {
            std::shared_ptr<Agent> play(new Player(para.substr(para.find("=") + 1)));
            host.register_agent(play);
        } else if (para.find("--evil") == 0) {
            std::shared_ptr<Agent> evil(new RandomEnvironment(para.substr(para.find("=") + 1)));
            host.register_agent(evil);
        }
    }

    std::regex match_move("^#\\S+ \\S+$"); // e.g. "#M0001 ?", "#M0001 #U"
    std::regex match_ctrl("^#\\S+ \\S+ \\S+$"); // e.g. "#M0001 open Slider:Placer", "#M0001 close score=15424"
    std::regex arena_ctrl("^[@$].+$"); // e.g. "@ login", "@ error the account "Name" has already been taken"
    std::regex arena_info("^[?%].+$"); // e.g. "? message from anonymous: 2048!!!"

    for (std::string command; input() >> command;) {
        try {
            if (std::regex_match(command, match_move)) {
                std::string id, move;
                std::stringstream(command) >> id >> move;

//                if (move == "?") {
//                    // your agent need to take an action
//                    Action a = host.at(id).take_action();
//                    host.at(id).ApplyAction(a);
//                    output() << id << ' ' << a << std::endl;
//                } else {
//                    // perform your opponent's action
//                    Action a;
//                    std::stringstream(move) >> a;
//                    host.at(id).ApplyAction(a);
//                }

                if (move == "?") {
                    // your agent need to take an action
                    Action a = host.at(id).take_action();
                    host.at(id).ApplyAction(a);
                    if (a.type() == Action::Place::type_) {
                        int hint = Action::Place(a).hint(); // your hint tile here
                        output() << id << ' ' << a << '+' << hint << std::endl;
                    } else {
                        output() << id << ' ' << a << std::endl;
                    }
                } else {
                    // perform your opponent's action
                    Action a;
                    std::stringstream(move) >> a;
                    int hint = 0;
                    if (a.type() == Action::Place::type_) {
                        hint = move[3] - '0'; // move should be "PT+H" where H is '1', '2', '3', or '4'
                    }
                    host.at(id).ApplyAction(a); // you should pass the hint tile to your player
                }

            } else if (std::regex_match(command, match_ctrl)) {
                std::string id, ctrl, tag;
                std::stringstream(command) >> id >> ctrl >> tag;

                if (ctrl == "open") {
                    // a new match is pending
                    if (host.open(id, tag)) {
                        output() << id << " open accept" << std::endl;
                    } else {
                        output() << id << " open reject" << std::endl;
                    }
                } else if (ctrl == "close") {
                    // a match is finished
                    host.close(id, tag);
                }

            } else if (std::regex_match(command, arena_ctrl)) {
                std::string ctrl;
                std::stringstream(command).ignore(1) >> ctrl;

                if (ctrl == "login") {
                    // register yourself and your agents
                    std::stringstream agents;
                    for (auto who : host.list_agents()) {
                        agents << " " << who->name() << "(" << who->role() << ")";
                    }
                    output("@ ") << "login " << host.login() << agents.str() << std::endl;

                } else if (ctrl == "status") {
                    // display current local status
                    info() << "+++++ status +++++" << std::endl;
                    info() << "login: " << host.account();
                    for (auto who : host.list_agents()) {
                        info() << " " << who->name() << "(" << who->role() << ")";
                    }
                    info() << std::endl;
                    info() << "match: " << host.list_matches().size() << std::endl;
                    for (auto ep : host.list_matches()) {
                        info() << ep->name() << " " << (*ep) << std::endl;
                    }
                    info() << "----- status -----" << std::endl;

                } else if (ctrl == "error" || ctrl == "exit") {
                    // some error messages or exit command
                    std::string message = command.substr(command.find_first_not_of("@$ "));
                    info() << message << std::endl;
                    break;
                }

            } else if (std::regex_match(command, arena_info)) {
                // message from arena server
            }
        } catch (std::exception &ex) {
            std::string message = std::string(typeid(ex).name()) + ": " + ex.what();
            message = message.substr(0, message.find_first_of("\r\n"));
            output("? ") << "exception " << message << " at \"" << command << "\"" << std::endl;
        }
    }

    return 0;
}

int main(int argc, const char *argv[]) {
    InitLookUpTables();

    std::cout << "Threes-Demo: ";
    std::copy(argv, argv + argc, std::ostream_iterator<const char *>(std::cout, " "));
    std::cout << std::endl << std::endl;

    size_t total = 1000;
    size_t block = 0;
    size_t limit = 0;
    bool learning = false;

    std::string play_args;
    std::string evil_args;
    std::string load;
    std::string save;

    bool summary = false;

    for (int i = 1; i < argc; i++) {
        std::string para(argv[i]);
        if (para.find("--total=") == 0) {
            total = std::stoull(para.substr(para.find("=") + 1));
        } else if (para.find("--block=") == 0) {
            block = std::stoull(para.substr(para.find("=") + 1));
        } else if (para.find("--limit=") == 0) {
            limit = std::stoull(para.substr(para.find("=") + 1));
        } else if (para.find("--play=") == 0) {
            play_args = para.substr(para.find("=") + 1);
        } else if (para.find("--evil=") == 0) {
            evil_args = para.substr(para.find("=") + 1);
        } else if (para.find("--load=") == 0) {
            load = para.substr(para.find("=") + 1);
        } else if (para.find("--learn") == 0) {
            learning = true;
        } else if (para.find("--save=") == 0) {
            std::string s = para.substr(para.find("=") + 1);
            if (s == "epoch") {
                save = "../results/" + std::to_string(std::time(nullptr));
            } else {
                save = s;
            }
        } else if (para.find("--summary") == 0) {
            summary = true;
        } else if (para.find("--shell") == 0) {
            return shell(argc, argv);
        }
    }

    Statistic stat(total, block, limit);

    if (load.size()) {
        std::ifstream in(load, std::ios::in);
        in >> stat;
        in.close();
        summary |= stat.IsFinished();
    }

    TdLambdaPlayer player(play_args);
    RandomEnvironment evil(evil_args);

    int count = 0;
    while (!stat.IsFinished()) {
//        std::cout << count << std::endl;
        player.OpenEpisode("~:" + evil.name());
        evil.OpenEpisode(player.name() + ":~");

        stat.OpenEpisode(player.name() + ":" + evil.name());
        Episode &game = stat.Back();
//        std::cout << "Start playing" << std::endl;
        int moves = 0;
        while (true) {
//            game.state().Print();

            Agent &agent = game.TakeTurns(player, evil);
//            std::cout << "moves = " << moves << " " << agent.name() << std::endl;
            Action move = agent.TakeAction(game.state());
//            std::cout << move << std::endl;

            if (!game.ApplyAction(move)) {
                break;
            }
//            std::cout << "applied action" << std::endl;
            if (agent.CheckForWin(game.state())) break;
//            std::cout << "CheckForWin" << std::endl;
            moves++;
        }
//        std::cout << "Finished" << std::endl;

        Agent &win = game.TakeLastTurns(player, evil);
        stat.CloseEpisode(win.name());

        if (learning) {
            player.Learn(game);

            if (stat.IsBackup()) {
                player.save();
            }

            if (stat.NGames() >= total / 2) {
                player.decreaseLearningRate10Times();
            }
        }

        player.CloseEpisode(win.name());
        evil.CloseEpisode(win.name());
        count++;
    }

    if (summary) {
        stat.Summary();
    }

    if (save.size()) {
        std::ofstream out(save, std::ios::out | std::ios::trunc);
        out << stat;
        out.close();
    }

    return 0;
}
