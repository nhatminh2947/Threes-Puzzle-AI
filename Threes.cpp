#include <iostream>
#include <fstream>
#include <iterator>
#include <string>

#include "board.h"
#include "Action.h"
#include "Agent.h"
#include "episode.h"
#include "statistic.h"

int main(int argc, const char *argv[]) {
    std::cout << "Threes-Demo: ";
    std::copy(argv, argv + argc, std::ostream_iterator<const char *>(std::cout, " "));
    std::cout << std::endl << std::endl;

    size_t total = 1000;
    size_t block = 0;
    size_t limit = 0;

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
        } else if (para.find("--save=") == 0) {
            save = para.substr(para.find("=") + 1);
        } else if (para.find("--summary") == 0) {
            summary = true;
        }
    }

    Statistic stat(total, block, limit);

    if (load.size()) {
        std::ifstream in(load, std::ios::in);
        in >> stat;
        in.close();
        summary |= stat.IsFinished();
    }

//    GreedyPlayer player(play_args);
//    OneDirectionPlayer player(play_args);
//    MaxRewardPlayer player(play_args);
//    LessTilePlayer player(play_args);
//    MaxMergePlayer player(play_args);
    SmartPlayer player(play_args);
    RandomEnvironment evil(evil_args);

    while (!stat.IsFinished()) {
        player.OpenEpisode("~:" + evil.name());
        evil.OpenEpisode(player.name() + ":~");

        stat.OpenEpisode(player.name() + ":" + evil.name());
        episode &game = stat.Back();
//        int count = 0;
        while (true) {
//            std::cout << count++ << std::endl;
//            std::cout << game.state() << std::endl;

            Agent &agent = game.TakeTurns(player, evil);
            Action move = agent.TakeAction(game.state(), move);
            if (!game.ApplyAction(move)) break;
            if (agent.CheckForWin(game.state())) break;
        }

        Agent &win = game.TakeLastTurns(player, evil);
        stat.CloseEpisode(win.name());

        player.CloseEpisode(win.name());
        evil.CloseEpisode(win.name());
    }

    if (summary) {
        stat.Summary();
    }

    //save file by player name
    save = "../results/" + player.name() + ".txt";

    if (save.size()) {
        std::ofstream out(save, std::ios::out | std::ios::trunc);
        out << stat;
        out.close();
    }

    return 0;
}