/*

Copyright (c) 2022, Arvid Norberg
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#include "libtorrent/aux_/tracker_list.hpp"
#include "libtorrent/aux_/announce_entry.hpp"
#include "libtorrent/announce_entry.hpp"
#include "test.hpp"

using namespace libtorrent::aux;

TORRENT_TEST(test_initial_state)
{
	tracker_list tl;
	TEST_EQUAL(tl.empty(), true);
	TEST_EQUAL(tl.size(), 0);
	TEST_CHECK(tl.begin() == tl.end());
	TEST_EQUAL(tl.last_working(), -1);
	TEST_EQUAL(tl.last_working_url(), "");
}

TORRENT_TEST(test_duplicate_add)
{
	tracker_list tl;

	tl.add_tracker(announce_entry("http://example1.com/announce"));
	TEST_EQUAL(tl.size(), 1);
	tl.add_tracker(announce_entry("http://example2.com/announce"));
	TEST_EQUAL(tl.size(), 2);
	tl.add_tracker(announce_entry("http://example3.com/announce"));
	TEST_EQUAL(tl.size(), 3);

	// duplicate ignored
	tl.add_tracker(announce_entry("http://example1.com/announce"));
	TEST_EQUAL(tl.size(), 3);

	// we want the trackers to have been inserted in the most efficient order
	TEST_EQUAL(tl[0].url, "http://example1.com/announce");
	TEST_EQUAL(tl[1].url, "http://example2.com/announce");
	TEST_EQUAL(tl[2].url, "http://example3.com/announce");
}

TORRENT_TEST(test_add_sort_by_tier)
{
	tracker_list tl;
	announce_entry ae;

	ae.url = "http://example1.com/announce";
	ae.tier = 5;
	tl.add_tracker(ae);
	TEST_EQUAL(tl.size(), 1);

	ae.url = "http://example2.com/announce";
	ae.tier = 4;
	tl.add_tracker(ae);
	TEST_EQUAL(tl.size(), 2);

	ae.url = "http://example3.com/announce";
	ae.tier = 3;
	tl.add_tracker(ae);
	TEST_EQUAL(tl.size(), 3);

	ae.url = "http://example1.com/announce";
	ae.tier = 2;
	tl.add_tracker(ae);

	// duplicate ignored
	TEST_EQUAL(tl.size(), 3);

	// the trackers should be ordered by low tiers first
	TEST_EQUAL(tl[0].url, "http://example3.com/announce");
	TEST_EQUAL(tl[1].url, "http://example2.com/announce");
	TEST_EQUAL(tl[2].url, "http://example1.com/announce");
}

TORRENT_TEST(test_replace_duplicate)
{
	tracker_list tl;

	std::vector<lt::announce_entry> trackers;
	trackers.emplace_back("http://example1.com/announce");
	trackers.emplace_back("http://example2.com/announce");
	trackers.emplace_back("http://example3.com/announce");
	trackers.emplace_back("http://example1.com/announce");

	tl.replace(trackers);

	// duplicate ignored
	TEST_EQUAL(tl.size(), 3);

	// we want the trackers to have been inserted in the most efficient order
	TEST_EQUAL(tl[0].url, "http://example1.com/announce");
	TEST_EQUAL(tl[1].url, "http://example2.com/announce");
	TEST_EQUAL(tl[2].url, "http://example3.com/announce");
}

TORRENT_TEST(test_replace_sort_by_tier)
{
	tracker_list tl;

	std::vector<lt::announce_entry> trackers;
	trackers.emplace_back("http://example1.com/announce");
	trackers.back().tier = 5;
	trackers.emplace_back("http://example2.com/announce");
	trackers.back().tier = 4;
	trackers.emplace_back("http://example3.com/announce");
	trackers.back().tier = 3;
	trackers.emplace_back("http://example1.com/announce");
	trackers.back().tier = 1;

	tl.replace(trackers);

	// duplicate ignored
	TEST_EQUAL(tl.size(), 3);

	// the trackers should be ordered by low tiers first
	TEST_EQUAL(tl[0].url, "http://example3.com/announce");
	TEST_EQUAL(tl[1].url, "http://example2.com/announce");
	TEST_EQUAL(tl[2].url, "http://example1.com/announce");
}

TORRENT_TEST(test_prioritize_udp_noop)
{
	tracker_list tl;

	std::vector<lt::announce_entry> trackers;
	trackers.emplace_back("http://example1.com/announce");
	trackers.emplace_back("http://example2.com/announce");
	trackers.emplace_back("http://example3.com/announce");
	trackers.emplace_back("udp://example4.com/announce");

	tl.replace(trackers);

	// duplicate ignored
	TEST_EQUAL(tl.size(), 4);

	// the trackers should be ordered by low tiers first
	TEST_EQUAL(tl[0].url, "http://example1.com/announce");
	TEST_EQUAL(tl[1].url, "http://example2.com/announce");
	TEST_EQUAL(tl[2].url, "http://example3.com/announce");
	TEST_EQUAL(tl[3].url, "udp://example4.com/announce");

	tl.prioritize_udp_trackers();

	// UDP trackers are prioritized over HTTP for the same hostname. These
	// hostnames are all different, so no reordering happens
	TEST_EQUAL(tl[0].url, "http://example1.com/announce");
	TEST_EQUAL(tl[1].url, "http://example2.com/announce");
	TEST_EQUAL(tl[2].url, "http://example3.com/announce");
	TEST_EQUAL(tl[3].url, "udp://example4.com/announce");
}

TORRENT_TEST(test_prioritize_udp)
{
	tracker_list tl;

	std::vector<lt::announce_entry> trackers;
	trackers.emplace_back("http://example1.com/announce");
	trackers.emplace_back("http://example2.com/announce");
	trackers.emplace_back("http://example3.com/announce");
	trackers.emplace_back("udp://example1.com/announce");

	tl.replace(trackers);

	// duplicate ignored
	TEST_EQUAL(tl.size(), 4);

	// the trackers should be ordered by low tiers first
	TEST_EQUAL(tl[0].url, "http://example1.com/announce");
	TEST_EQUAL(tl[1].url, "http://example2.com/announce");
	TEST_EQUAL(tl[2].url, "http://example3.com/announce");
	TEST_EQUAL(tl[3].url, "udp://example1.com/announce");

	tl.prioritize_udp_trackers();

	TEST_EQUAL(tl[0].url, "udp://example1.com/announce");
	TEST_EQUAL(tl[1].url, "http://example2.com/announce");
	TEST_EQUAL(tl[2].url, "http://example3.com/announce");
	TEST_EQUAL(tl[3].url, "http://example1.com/announce");
}

TORRENT_TEST(test_prioritize_udp_tier)
{
	tracker_list tl;

	std::vector<lt::announce_entry> trackers;
	trackers.emplace_back("http://example1.com/announce");
	trackers.emplace_back("udp://example1.com/announce");
	trackers.back().tier = 2;

	tl.replace(trackers);

	// the trackers should be ordered by low tiers first
	TEST_EQUAL(tl[0].url, "http://example1.com/announce");
	TEST_EQUAL(tl[1].url, "udp://example1.com/announce");

	tl.prioritize_udp_trackers();

	// trackers are also re-ordered across tiers
	TEST_EQUAL(tl[0].url, "udp://example1.com/announce");
	TEST_EQUAL(tl[1].url, "http://example1.com/announce");
}

TORRENT_TEST(test_replace_find_tracker)
{
	tracker_list tl;

	std::vector<lt::announce_entry> trackers;
	trackers.emplace_back("http://a.com/announce");
	trackers.emplace_back("http://b.com/announce");
	trackers.emplace_back("http://c.com/announce");
	tl.replace(trackers);

	TEST_EQUAL(tl.find_tracker("http://a.com/announce")->url, "http://a.com/announce");
	TEST_EQUAL(tl.find_tracker("http://b.com/announce")->url, "http://b.com/announce");
	TEST_EQUAL(tl.find_tracker("http://c.com/announce")->url, "http://c.com/announce");
	TEST_CHECK(tl.find_tracker("http://d.com/announce") == nullptr);
}

TORRENT_TEST(test_add_find_tracker)
{
	tracker_list tl;

	tl.add_tracker(announce_entry("http://a.com/announce"));
	tl.add_tracker(announce_entry("http://b.com/announce"));
	tl.add_tracker(announce_entry("http://c.com/announce"));

	TEST_EQUAL(tl.find_tracker("http://a.com/announce")->url, "http://a.com/announce");
	TEST_EQUAL(tl.find_tracker("http://b.com/announce")->url, "http://b.com/announce");
	TEST_EQUAL(tl.find_tracker("http://c.com/announce")->url, "http://c.com/announce");
	TEST_CHECK(tl.find_tracker("http://d.com/announce") == nullptr);
}

TORRENT_TEST(test_deprioritize_tracker)
{
	tracker_list tl;

	tl.add_tracker(announce_entry("http://a.com/announce"));
	tl.add_tracker(announce_entry("http://b.com/announce"));
	tl.add_tracker(announce_entry("http://c.com/announce"));

	TEST_EQUAL(tl[0].url, "http://a.com/announce");
	TEST_EQUAL(tl[1].url, "http://b.com/announce");
	TEST_EQUAL(tl[2].url, "http://c.com/announce");

	tl.deprioritize_tracker(&tl[0]);

	TEST_EQUAL(tl[0].url, "http://b.com/announce");
	TEST_EQUAL(tl[1].url, "http://c.com/announce");
	TEST_EQUAL(tl[2].url, "http://a.com/announce");

	tl.deprioritize_tracker(&tl[1]);

	TEST_EQUAL(tl[0].url, "http://b.com/announce");
	TEST_EQUAL(tl[1].url, "http://a.com/announce");
	TEST_EQUAL(tl[2].url, "http://c.com/announce");
}

TORRENT_TEST(test_deprioritize_tracker_tier)
{
	tracker_list tl;

	std::vector<lt::announce_entry> trackers;
	trackers.emplace_back("http://a.com/announce");
	trackers.back().tier = 1;
	trackers.emplace_back("http://b.com/announce");
	trackers.back().tier = 1;
	trackers.emplace_back("http://c.com/announce");
	tl.replace(trackers);

	TEST_EQUAL(tl[0].url, "http://c.com/announce");
	TEST_EQUAL(tl[1].url, "http://a.com/announce");
	TEST_EQUAL(tl[2].url, "http://b.com/announce");

	// the tracker won't move across the tier
	tl.deprioritize_tracker(&tl[0]);

	TEST_EQUAL(tl[0].url, "http://c.com/announce");
	TEST_EQUAL(tl[1].url, "http://a.com/announce");
	TEST_EQUAL(tl[2].url, "http://b.com/announce");

	tl.deprioritize_tracker(&tl[1]);

	TEST_EQUAL(tl[0].url, "http://c.com/announce");
	TEST_EQUAL(tl[1].url, "http://b.com/announce");
	TEST_EQUAL(tl[2].url, "http://a.com/announce");
}

TORRENT_TEST(test_add_empty)
{
	tracker_list tl;

	tl.add_tracker(announce_entry(""));
	TEST_EQUAL(tl.size(), 0);
}

TORRENT_TEST(test_replace_empty)
{
	tracker_list tl;

	std::vector<lt::announce_entry> trackers;
	trackers.emplace_back("");
	tl.replace(trackers);
	TEST_EQUAL(tl.size(), 0);
}

// TODO: last_working, last_working_url
// TODO: enable_all
// TODO: set_complete_sent
// TODO: reset
// TODO: completed
// TODO: record_working
// TODO: dont_try_again
