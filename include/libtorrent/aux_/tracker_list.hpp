/*

Copyright (c) 2022, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef TORRENT_TRACKER_LIST_HPP_INCLUDED
#define TORRENT_TRACKER_LIST_HPP_INCLUDED

#include <string>
#include <unordered_map>

#include "libtorrent/aux_/announce_entry.hpp"
#include "libtorrent/announce_entry.hpp"
#include "libtorrent/aux_/vector.hpp"
#include "libtorrent/aux_/invariant_check.hpp"


namespace libtorrent::aux {

struct TORRENT_EXTRA_EXPORT tracker_list
{
	using trackers_t = aux::vector<aux::announce_entry>;
	using iterator = typename trackers_t::iterator;
	using const_iterator = typename trackers_t::const_iterator;

	aux::announce_entry* find_tracker(std::string const& url);

	// returns true if the tracker was added, and false if it was already
	// in the tracker list (in which case the source was added to the
	// entry in the list)
	bool add_tracker(announce_entry const& ae);

	void prioritize_udp_trackers();

	int deprioritize_tracker(aux::announce_entry* ae);
	void dont_try_again(aux::announce_entry* ae);

	bool empty() const { return m_trackers.empty(); }
	std::size_t size() const { return m_trackers.size(); }

	int last_working() const { return m_last_working_tracker; }
	std::string last_working_url() const;

	void record_working(aux::announce_entry const* ae);

	void replace(std::vector<lt::announce_entry> const& aes);

	void enable_all();

	void completed(time_point32 now);

	void set_complete_sent();

	void reset();

	void stop_announcing(time_point32 now);

	bool any_verified() const;

	// TODO: make these iterators const
	iterator begin() { return m_trackers.begin(); }
	iterator end() { return m_trackers.end(); }

	const_iterator begin() const { return m_trackers.begin(); }
	const_iterator end() const { return m_trackers.end(); }

	aux::announce_entry const& operator[](int const idx) const { return m_trackers[idx]; }
	aux::announce_entry& operator[](int const idx) { return m_trackers[idx]; }

private:

#if TORRENT_USE_INVARIANT_CHECKS
	friend struct libtorrent::invariant_access;
	void check_invariant() const;
#endif

	trackers_t m_trackers;

	// maps URLs to index into m_trackers
	std::unordered_map<std::string, int> m_url_index;

	// the index to the last tracker that worked
	int m_last_working_tracker = -1;
};

}
#endif

