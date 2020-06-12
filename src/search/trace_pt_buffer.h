/****
DIAMOND protein aligner
Copyright (C) 2013-2020 Max Planck Society for the Advancement of Science e.V.
                        Benjamin Buchfink
                        Eberhard Karls Universitaet Tuebingen
						
Code developed by Benjamin Buchfink <benjamin.buchfink@tue.mpg.de>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
****/

#pragma once
#include "../util/async_buffer.h"
#include "../basic/match.h"
#include "../util/io/deserializer.h"
#include "../data/reference.h"

#pragma pack(1)

struct hit
{
	typedef uint32_t Seed_offset;

	unsigned	query_;
	Packed_loc	subject_;
	Seed_offset	seed_offset_;
	hit() :
		query_(),
		subject_(),
		seed_offset_()
	{ }
	hit(unsigned query, Packed_loc subject, Seed_offset seed_offset) :
		query_(query),
		subject_(subject),
		seed_offset_(seed_offset)
	{ }
	operator uint32_t() const
	{
		return query_;
	}
	bool operator<(const hit &rhs) const
	{
		return query_ < rhs.query_;
	}
	bool blank() const
	{
		return subject_ == 0;
	}
	unsigned operator%(unsigned i) const
	{
		return (query_ / align_mode.query_contexts) % i;
	}
	unsigned operator/(size_t i) const
	{
		return (query_ / align_mode.query_contexts) / (unsigned)i;
	}
	unsigned frame() const {
		return query_ % align_mode.query_contexts;
	}
	int64_t global_diagonal() const
	{
		return (int64_t)subject_ - (int64_t)seed_offset_;
	}
	template<unsigned _d>
	static unsigned query_id(const hit& x)
	{
		return x.query_ / _d;
	}
	template<unsigned _d>
	struct Query_id
	{
		unsigned operator()(const hit& x) const
		{
			return query_id<_d>(x);
		}
	};
	static bool cmp_subject(const hit &lhs, const hit &rhs)
	{
		return lhs.subject_ < rhs.subject_
			|| (lhs.subject_ == rhs.subject_ && lhs.seed_offset_ < rhs.seed_offset_);
	}
	static bool cmp_normalized_subject(const hit &lhs, const hit &rhs)
	{
		const uint64_t x = (uint64_t)lhs.subject_ + (uint64_t)rhs.seed_offset_, y = (uint64_t)rhs.subject_ + (uint64_t)lhs.seed_offset_;
		return x < y || (x == y && lhs.seed_offset_ < rhs.seed_offset_);
	}
	static bool cmp_frame(const hit &x, const hit &y) {
		return x.frame() < y.frame();
	}
	friend std::ostream& operator<<(std::ostream &s, const hit &me)
	{
		s << me.query_ << '\t' << me.subject_ << '\t' << me.seed_offset_ << '\n';
		return s;
	}
	template<typename _it>
	static size_t read(Deserializer& s, _it it) {
		const bool l = long_subject_offsets();
		uint32_t query_id, seed_offset;
		s.varint = true;
		s >> query_id >> seed_offset;
		s.varint = false;
		Packed_loc subject_loc;
		size_t count = 0;
		for (;;) {
			if (l)
				s.read(subject_loc);
			else {
				uint32_t x;
				s >> x;
				subject_loc = x;
			}
			if (subject_loc == 0)
				return count;
			*it = { query_id, subject_loc, seed_offset };
			++count;
		}
	}
} PACKED_ATTRIBUTE;

#pragma pack()

struct Trace_pt_buffer : public Async_buffer<hit>
{
	Trace_pt_buffer(size_t input_size, const string &tmpdir, unsigned query_bins):
		Async_buffer<hit>(input_size, tmpdir, query_bins)
	{}
	static Trace_pt_buffer *instance;
};
