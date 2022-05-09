/*

Copyright (c) 2020, Arvid Norberg
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

#include <iostream>
#include <thread>
#include <chrono>

#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/magnet_uri.hpp>

int main(int argc, char const* argv[]) try
{
	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " <magnet-url>" << std::endl;
		std::cerr << "prints .torrent file to standard out" << std::endl;
		return 1;
	}
	lt::settings_pack p;
	p.set_int(lt::settings_pack::alert_mask, lt::alert_category::status
		| lt::alert_category::error);
	lt::session ses(p);

	lt::add_torrent_params atp = lt::parse_magnet_uri(argv[1]);
	atp.save_path = "."; // save in current dir
	lt::torrent_handle h = ses.add_torrent(std::move(atp));

	for (;;) {
		std::vector<lt::alert*> alerts;
		ses.pop_alerts(&alerts);

		for (lt::alert const* a : alerts)
		{
			if (auto const* mra = lt::alert_cast<lt::metadata_received_alert>(a))
			{
				std::cerr << "metadata received" << std::endl;
				auto const handle = mra->handle;
				std::shared_ptr<lt::torrent_info const> ti = handle.torrent_file();
				if (!ti)
				{
					std::cerr << "unexpected missing torrent info" << std::endl;
					goto done;
				}

				// in order to create valid v2 torrents, we need to download
				if (ti->v2())
				{
					std::cerr << "found v2 torrent. We need its content in order to "
						"create valid v2 .torrent" << std::endl;
					continue;
				}
				lt::create_torrent ct(*ti);
				std::vector<char> torrent;
				lt::bencode(back_inserter(torrent), ct.generate());
				std::cout.write(torrent.data(), int(torrent.size()));
				goto done;
			}
			if (auto const* tfa = lt::alert_cast<lt::torrent_finished_alert>(a))
			{
				std::cerr << "download complete" << std::endl;
				auto const handle = tfa->handle;
				std::shared_ptr<lt::torrent_info const> ti = handle.torrent_file_with_hashes();
				if (!ti)
				{
					std::cerr << "unexpected missing torrent info" << std::endl;
					goto done;
				}
				lt::create_torrent ct(*ti);
				std::vector<char> torrent;
				lt::bencode(back_inserter(torrent), ct.generate());
				std::cout.write(torrent.data(), int(torrent.size()));
				goto done;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	done:
	std::cerr<< "done, shutting down" << std::endl;
}
catch (std::exception& e)
{
	std::cerr << "Error: " << e.what() << std::endl;
}
