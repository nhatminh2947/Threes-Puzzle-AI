#include <iostream>
#include <fstream>
#include <iterator>
#include <string>

#include "Board64.h"
#include "Action.h"
#include "Agent.h"
#include "Episode.h"
#include "Statistic.h"

// 0 1 2 3 4 5   6   7   8   9   10  11  12   13   14
// 0 1 2 3 6 12  24  48  92  192 384 768 1536 3072 6144

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
            } else save = s;
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

    TdLambdaPlayer player(play_args);
    RandomEnvironment evil(evil_args);

    while (!player.TrainingFinished(2, 10000000)) {
        player.OpenEpisode("~:" + evil.name());
        evil.OpenEpisode(player.name() + ":~");

        stat.OpenEpisode(player.name() + ":" + evil.name());
        Episode &game = stat.Back();
        int count = 0;
        while (true) {

            Agent &agent = game.TakeTurns(player, evil);
            Action move = agent.TakeAction(game.state(), move);

            if (!game.ApplyAction(move)) break;
            if (agent.CheckForWin(game.state())) break;

            count++;
        }

        Agent &win = game.TakeLastTurns(player, evil);
        stat.CloseEpisode(win.name());

        if (learning) {
            player.Learn(game);

            if (stat.IsBackup()) {
                player.save();
            }
        }

        player.CloseEpisode(win.name());
        evil.CloseEpisode(win.name());
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
