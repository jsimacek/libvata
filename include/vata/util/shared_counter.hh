/*****************************************************************************
 *  Vojnar's Army Tree Automata Library
 *
 *  Copyright (c) 2011  Jiri Simacek <isimacek@fit.vutbr.cz>
 *
 *  Description:
 *    Source for shared counter class.
 *
 *****************************************************************************/

#ifndef _VATA_SHARED_COUNTER_HH_
#define _VATA_SHARED_COUNTER_HH_

#include <cstring>
#include <vector>

#include <vata/util/caching_allocator.hh>

namespace VATA {
		namespace Util {
				class SharedCounter;
		}
}

class VATA::Util::SharedCounter {

public:

	typedef CachingArrayAllocator<size_t> Allocator;
	typedef std::vector<size_t> Key;
	typedef std::vector<std::pair<size_t, size_t>> LabelMap;

private:

	struct Row {

		size_t master_;
		size_t* data_;

		Row() : master_(0), data_(nullptr) {}

	};

	const Key& key_;
	const size_t& states_;
	const LabelMap& labelMap_;
	const size_t& rowSize_;
	Allocator& allocator_;

	std::vector<Row> data_;


protected:

	SharedCounter& operator=(const SharedCounter& rhs);

public:

	SharedCounter(const Key& key, const size_t& states, const LabelMap& labelMap,
		const size_t& rowSize, Allocator& allocator) : key_(key), states_(states),
		labelMap_(labelMap), rowSize_(rowSize), allocator_(allocator), data_() {}

	SharedCounter(SharedCounter& counter) : key_(counter.key_), states_(counter.states_),
		labelMap_(counter.labelMap_), rowSize_(counter.rowSize_), allocator_(counter.allocator_),
		data_() {}

	~SharedCounter() {

		for (auto& row : this->data_) {

			if (!row.data_)
				continue;

			if (!--(row.data_[this->rowSize_])) // refCount
				this->allocator_.reclaim(row.data_);

		}

	}

	void releaseSingletons() {

		for (auto& row : this->data_) {

			if (!row.data_)
				continue;

			assert(row.data_[this->rowSize_]);

			size_t col = 0;

			while (row.data_[col] == 0)
				++col;

			assert(col < this->rowSize_);

			if (row.data_[col] < row.master_)
				continue;

			assert(row.data_[col] == row.master_);

			// everything is in master

			if (!--(row.data_[this->rowSize_])) // refCount
				this->allocator_.reclaim(row.data_);

			row.data_ = nullptr;

		}

	}

	size_t get(size_t label, size_t state) const {

		assert(label*this->states_ + state < this->key_.size());

		size_t index = this->key_[label*this->states_ + state];
		size_t bucket = index / this->rowSize_;
		size_t col = index % this->rowSize_;

		assert(bucket < this->data_.size());

		auto& row = this->data_[bucket];

		if (!row.data_)
			return row.master_;

		return row.data_[col];

	}

	void incr(size_t label, size_t state, size_t count = 1) {

		assert(label*this->states_ + state < this->key_.size());

		size_t index = this->key_[label*this->states_ + state];
		size_t bucket = index / this->rowSize_;
		size_t col = index % this->rowSize_;

		if (bucket >= this->data_.size())
			this->data_.resize(bucket + 1);

		auto& row = this->data_[bucket];

		if (row.master_) {

			assert(row.data_);

			row.master_ += count;
			row.data_[col] += count;

			return;

		}

		row.master_ = count;
		row.data_ = this->allocator_(this->rowSize_ + 1);

		std::memset(row.data_, 0, this->rowSize_*sizeof(size_t));

		assert(row.data_[col] == 0);

		row.data_[this->rowSize_] = 1; // refCount
		row.data_[col] = count;

	}

	size_t decr(size_t label, size_t state) {

		assert(label*this->states_ + state < this->key_.size());

		size_t index = this->key_[label*this->states_ + state];
		size_t bucket = index / this->rowSize_;
		size_t col = index % this->rowSize_;

		assert(bucket < this->data_.size());

		auto& row = this->data_[bucket];

		assert(row.master_);

		if (!row.data_) // everything is in master
			return --row.master_;

		if ((row.master_ == row.data_[col]) || (row.master_ == 2)) {

			// move everything to master

			--row.master_;

			size_t result = (row.data_[col] - 1);

			if (!--(row.data_[this->rowSize_])) // refCount
				this->allocator_.reclaim(row.data_);

			row.data_ = nullptr;

			return result;

		}

		if (row.data_[this->rowSize_] > 1) { // refCount

			--(row.data_[this->rowSize_]); // refCount

			auto newData = this->allocator_(this->rowSize_ + 1);

			std::memcpy(newData, row.data_, this->rowSize_*sizeof(size_t));

			assert(newData[col] == row.data_[col]);

			newData[this->rowSize_] = 1; // refCount

			row.data_ = newData;

		}

		assert(row.data_[this->rowSize_] == 1);

		--row.master_;

		return --(row.data_[col]);

	}

	template <class T>
	void copyLabels(const T& labels, SharedCounter& cnt) {

		size_t start = cnt.data_.size(), sent = 0;

		std::vector<bool> rowMask(cnt.data_.size(), false);

		for (auto& label : labels) {

			assert(label < this->labelMap_.size());

			start = std::min(start, this->labelMap_[label].first);

			sent = std::max(sent, std::min(cnt.data_.size(), this->labelMap_[label].second));

			for (size_t i = this->labelMap_[label].first;
				i < std::min(cnt.data_.size(), this->labelMap_[label].second); ++i)

				rowMask[i] = true;

		}

		this->data_.resize(sent);

		for (size_t i = start; i < sent; ++ i) {

			if (!rowMask[i])
				continue;

			auto& src = cnt.data_[i];
			auto& dst = this->data_[i];

			dst.master_ = src.master_;

			if (!src.data_)
				continue;

			++(src.data_[this->rowSize_]); // refCount

			dst.data_ = src.data_;

		}

	}

	friend std::ostream& operator<<(std::ostream& os, const SharedCounter& cnt) {

		for (auto& row : cnt.data_) {

			os << row.master_ << ':';

			if (row.data_) {

				for (size_t col = 0; col < cnt.rowSize_; ++col)
					os << ' ' << row.data_[col];

				os << std::endl;

			}

		}

		return os;

	}

};

#endif
