#pragma once
#include <list>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "Board64.h"
#include "Action.h"
#include "Agent.h"
#include "Episode.h"

class Statistic {
public:
	/**
	 * the total episodes to run
	 * the block size of statistic
	 * the limit of saving records
	 *
	 * note that total >= limit >= block
	 */
	Statistic(size_t total, size_t block = 0, size_t limit = 0)
		: total_(total),
		  block_(block ? block : total),
		  limit_(limit ? limit : total),
		  count_(0) {}

public:
	/**
	 * show the statistic of last 'block' games
	 *
	 * the format would be
	 * 1000   avg = 273901, max = 382324, ops = 241563 (170543|896715)
	 *        512     100%   (0.3%)
	 *        1024    99.7%  (0.2%)
	 *        2048    99.5%  (1.1%)
	 *        4096    98.4%  (4.7%)
	 *        8192    93.7%  (22.4%)
	 *        16384   71.3%  (71.3%)
	 *
	 * where (block = 1000 by default)
	 *  '1000': current index (n)
	 *  'avg = 273901': the average score is 273901
	 *  'max = 382324': the maximum score is 382324
	 *  'ops = 241563 (170543|896715)': the average speed is 241563
	 *                                  the average speed of player is 170543
	 *                                  the average speed of environment is 896715
	 *  '93.7%': 93.7% (937 games) reached 8192-tiles (a.k.a. win rate of 8192-tile)
	 *  '22.4%': 22.4% (224 games) terminated with 8192-tiles (the largest)
	 */
	void Show(bool tstat = true) const {
		size_t blk = std::min(data_.size(), block_);
		size_t stat[64] = { 0 };
		size_t sop = 0, pop = 0, eop = 0;
		time_t sdu = 0, pdu = 0, edu = 0;
		reward_t sum = 0;
		reward_t max = 0;
		auto it = data_.end();
		for (size_t i = 0; i < blk; i++) {
			auto& ep = *(--it);
			sum += ep.score();
			max = std::max(ep.score(), max);
			stat[ep.state().GetMaxTile()]++;
			sop += ep.step();
			pop += ep.step(Action::Slide::type_);
			eop += ep.step(Action::Place::type);
			sdu += ep.time();
			pdu += ep.time(Action::Slide::type_);
			edu += ep.time(Action::Place::type);
		}

		std::ios ff(nullptr);
		ff.copyfmt(std::cout);
		std::cout << std::fixed << std::setprecision(0);
		std::cout << count_ << "\t";
		std::cout << "avg = " << (sum / blk) << ", ";
		std::cout << "max = " << (max) << ", ";
		std::cout << "ops = " << (sop * 1000.0 / sdu);
		std::cout <<     " (" << (pop * 1000.0 / pdu);
		std::cout <<      "|" << (eop * 1000.0 / edu) << ")";
		std::cout << std::endl;
		std::cout.copyfmt(ff);

		if (!tstat) return;
		for (size_t t = 0, c = 0; c < blk; c += stat[t++]) {
			if (stat[t] == 0) continue;
			unsigned accu = std::accumulate(std::begin(stat) + t, std::end(stat), 0);
			std::cout << "\t" << ((1 << (t-3)) * 3); // type
			std::cout << "\t" << (accu * 100.0 / blk) << "%"; // win rate
			std::cout << "\t" "(" << (stat[t] * 100.0 / blk) << "%" ")"; // percentage of ending
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	void Summary() const {
		auto block_temp = block_;
		const_cast<Statistic&>(*this).block_ = data_.size();
		Show();
		const_cast<Statistic&>(*this).block_ = block_temp;
	}

	bool IsFinished() const {
		return count_ >= total_;
	}

	void OpenEpisode(const std::string &flag = "") {
		if (count_++ >= limit_) data_.pop_front();
		data_.emplace_back();
		data_.back().OpenEpisode(flag);
	}

	void CloseEpisode(const std::string &flag = "") {
		data_.back().CloseEpisode(flag);
		if (count_ % block_ == 0) {
			Show();
		}
	}

	bool IsBackup() {
		return count_ % block_ == 0;
	}

	Episode& At(size_t i) {
		auto it = data_.begin();
		while (i--) it++;
		return *it;
	}

	Episode& Front() {
		return data_.front();
	}

	Episode& Back() {
		return data_.back();
	}

	friend std::ostream& operator <<(std::ostream& out, const Statistic& stat) {
		for (const Episode& rec : stat.data_) out << rec << std::endl;
		return out;
	}

	friend std::istream& operator >>(std::istream& in, Statistic& stat) {
		for (std::string line; std::getline(in, line) && line.size(); ) {
			stat.data_.emplace_back();
			std::stringstream(line) >> stat.data_.back();
		}
		stat.total_ = std::max(stat.total_, stat.data_.size());
		stat.count_ = stat.data_.size();
		return in;
	}

private:
	size_t total_;
	size_t block_;
	size_t limit_;
	size_t count_;
	std::list<Episode> data_;
};
