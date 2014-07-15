/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_MULTI_PLAYER_LOOP_H
#define IO_MULTI_PLAYER_LOOP_H

#include "io/loop.h"
#include "io/controller.h"
#include "hud/layer.h"
#include "hud/button.h"
#include "net/base.h"

namespace hud {

	class MultiPlayerMenu: public HudLayer {
	public:
		enum class ColumnType {
			server_name,
			map_name,
			num_players,
			game_mode,
			ping
		};

		struct Column {
			ColumnType type;
			const char *title;
			float min_size;
			FRect rect;
		};

		struct ServerInfo: public net::ServerStatusChunk {
			int ping;

			FRect rect, hit_rect;
			bool is_mouse_over;
			float over_time, selection_time;
		};

		MultiPlayerMenu(const FRect &rect);
		float backAlpha() const override;

		bool isClientReady() const;
		net::PClient &&getClient();

	protected:
		void onUpdate(double time_diff) override;
		bool onInput(const io::InputEvent&) override;
		bool onEvent(const HudEvent&) override;
		void onDraw() const override;

		void updateRects();
		const string cellText(int server_id, ColumnType) const;
		void setMessage(const string&, Color color);

		void updateLobbyData();

		vector<Column> m_columns;
		vector<PHudButton> m_buttons;
		vector<ServerInfo> m_servers;
		net::PClient m_client;

		string m_message;
		Color m_message_color;
		double m_message_time;

		int m_selection;
		int m_row_offset;
		int m_max_visible_rows;
		bool m_please_refresh;
		bool m_waiting_for_refresh;
		bool m_waiting_to_connect;
		double m_last_refresh_time;
		double m_last_connect_time;
	};

}

namespace io {

	net::PClient createClient(const string &server_name, int port);

	class MultiPlayerLoop: public Loop {
	public:
		MultiPlayerLoop(net::PClient client);
		bool tick(double time_diff) override;

	private:
		net::PClient m_client;
		game::PWorld m_world;
		PController m_controller;
	};

}

#endif
