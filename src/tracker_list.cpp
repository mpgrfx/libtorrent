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


#include <utility> // for swap()

#include "libtorrent/aux_/tracker_list.hpp"
#include "libtorrent/announce_entry.hpp"
#include "libtorrent/aux_/parse_url.hpp"
#include "libtorrent/aux_/invariant_check.hpp"

namespace libtorrent::aux {

aux::announce_entry* tracker_list::find_tracker(std::string const& url)
{
	INVARIANT_CHECK;

	auto const i = m_url_index.find(url);
	if (i == m_url_index.end()) return nullptr;

	auto const idx = i->second;
	return &m_trackers[idx];
}

int tracker_list::deprioritize_tracker(aux::announce_entry* ae)
{
	INVARIANT_CHECK;

	int index = int(ae - m_trackers.data());

	TORRENT_ASSERT(index >= 0);
	TORRENT_ASSERT(index < int(m_trackers.size()));
	if (index >= int(m_trackers.size())) return -1;

	while (index < int(m_trackers.size()) - 1 && m_trackers[index].tier == m_trackers[index + 1].tier)
	{
		using std::swap;
		swap(m_trackers[index], m_trackers[index + 1]);
		m_url_index[m_trackers[index].url] = index;
		m_url_index[m_trackers[index + 1].url] = index + 1;
		if (m_last_working_tracker == index) ++m_last_working_tracker;
		else if (m_last_working_tracker == index + 1) --m_last_working_tracker;
		++index;
	}
	return index;
}

void tracker_list::dont_try_again(aux::announce_entry* ae)
{
	ae->fail_limit = 1;
}

bool tracker_list::add_tracker(announce_entry const& ae)
{
	INVARIANT_CHECK;

	if (ae.url.empty()) return false;
	auto* k = find_tracker(ae.url);
	if (k)
	{
		k->source |= ae.source;
		return false;
	}
	auto iter = std::upper_bound(m_trackers.begin(), m_trackers.end(), ae.tier
		, [] (int const tier, aux::announce_entry const& v) { return tier < v.tier; });

	int idx = int(iter - m_trackers.begin());
	if (idx < m_last_working_tracker) ++m_last_working_tracker;

	iter = m_trackers.emplace(iter, ae.url);
	auto& e = *iter;
	if (ae.source == 0) e.source = lt::announce_entry::source_client;
	else e.source = ae.source;
	e.trackerid = ae.trackerid;
	e.tier = ae.tier;
	e.fail_limit = ae.fail_limit;

	for (; iter != m_trackers.end(); ++iter, ++idx)
		m_url_index[iter->url] = idx;

	return true;
}

void tracker_list::prioritize_udp_trackers()
{
	INVARIANT_CHECK;

	// look for udp-trackers
	for (auto i = m_trackers.begin(), end(m_trackers.end()); i != end; ++i)
	{
		if (i->url.substr(0, 6) != "udp://") continue;
		// now, look for trackers with the same hostname
		// that is has higher priority than this one
		// if we find one, swap with the udp-tracker
		error_code ec;
		std::string udp_hostname;
		using std::ignore;
		std::tie(ignore, ignore, udp_hostname, ignore, ignore)
			= parse_url_components(i->url, ec);
		for (auto j = m_trackers.begin(); j != i; ++j)
		{
			std::string hostname;
			std::tie(ignore, ignore, hostname, ignore, ignore)
				= parse_url_components(j->url, ec);
			if (hostname != udp_hostname) continue;
			if (j->url.substr(0, 6) == "udp://") continue;
			using std::swap;
			using std::iter_swap;
			swap(i->tier, j->tier);
			iter_swap(i, j);
			m_url_index[i->url] = int(i - m_trackers.begin());
			m_url_index[j->url] = int(j - m_trackers.begin());
			break;
		}
	}
}

void tracker_list::record_working(aux::announce_entry const* ae)
{
	m_last_working_tracker = int(ae - m_trackers.data());
}

void tracker_list::replace(std::vector<lt::announce_entry> const& aes)
{
	INVARIANT_CHECK;

	m_trackers.clear();
	m_url_index.clear();

	// insert unordered
	m_trackers.reserve(aes.size());
	int idx = 0;
	for (auto const& ae : aes)
	{
		if (ae.url.empty()) continue;
		auto const [iter, added] = m_url_index.insert(std::make_pair(ae.url, idx));
		if (!added)
		{
			// if we already have an entry with this URL, skip it
			// but merge the source bits
			m_trackers[iter->second].source |= ae.source;
			continue;
		}
		++idx;
		m_trackers.emplace_back(ae);
	}

	// make sure the trackers are correctly ordered by tier
	sort(m_trackers.begin(), m_trackers.end()
		, [](aux::announce_entry const& lhs, aux::announce_entry const& rhs)
		{ return lhs.tier < rhs.tier; });

	// since we sorted the list, we need to update the index too
	idx = 0;
	for (auto const& ae : m_trackers)
	{
		m_url_index[ae.url] = idx;
		++idx;
	}

	m_last_working_tracker = -1;
}

void tracker_list::enable_all()
{
	INVARIANT_CHECK;
	for (aux::announce_entry& ae : m_trackers)
		for (aux::announce_endpoint& aep : ae.endpoints)
			aep.enabled = true;
}

void tracker_list::completed(time_point32 const now)
{
	INVARIANT_CHECK;
	for (auto& t : m_trackers)
	{
		for (auto& aep : t.endpoints)
		{
			if (!aep.enabled) continue;
			for (auto& a : aep.info_hashes)
			{
				if (a.complete_sent) continue;
				a.next_announce = now;
				a.min_announce = now;
			}
		}
	}
}

void tracker_list::set_complete_sent()
{
	INVARIANT_CHECK;
	for (auto& t : m_trackers)
	{
		for (auto& aep : t.endpoints)
		{
			for (auto& a : aep.info_hashes)
				a.complete_sent = true;
		}
	}
}

void tracker_list::reset()
{
	INVARIANT_CHECK;
	for (auto& t : m_trackers) t.reset();
}

void tracker_list::stop_announcing(time_point32 const now)
{
	INVARIANT_CHECK;

	for (auto& t : m_trackers)
	{
		for (auto& aep : t.endpoints)
		{
			for (auto& a : aep.info_hashes)
			{
				a.next_announce = now;
				a.min_announce = now;
			}
		}
	}
}

std::string tracker_list::last_working_url() const
{
	if (m_last_working_tracker < 0) return {};
	return m_trackers[m_last_working_tracker].url;
}

bool tracker_list::any_verified() const
{
	return std::any_of(m_trackers.begin(), m_trackers.end()
		, [](aux::announce_entry const& t) { return t.verified; });
}

#if TORRENT_USE_INVARIANT_CHECKS
void tracker_list::check_invariant() const
{
	int idx = 0;
	for (auto const& ae : m_trackers) {
		auto i = m_url_index.find(ae.url);
		TORRENT_ASSERT(i != m_url_index.end());
		TORRENT_ASSERT(i->second == idx);
		++idx;
	}

	TORRENT_ASSERT(m_url_index.size() == m_trackers.size());
	TORRENT_ASSERT(m_last_working_tracker == -1
		|| (m_last_working_tracker >= 0 && m_last_working_tracker < int(m_trackers.size())));
}
#endif

}
