/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_CONTROLLER_H
#define IO_CONTROLLER_H

#include "game/world.h"
#include "io/console.h"
#include "game/visibility.h"

namespace io {

	class Controller {
	public:
		Controller(const int2 &resolution, game::PWorld world, game::EntityRef actor_ref, bool show_stats);

		void update();
		void updateView(double time_diff);
		void draw();

		void drawVisibility(game::EntityRef);

	protected:
		Console m_console;
		game::PWorld m_world;
		game::WorldViewer m_viewer;

		game::EntityRef m_actor_ref;
		game::EntityRef m_container_ref;
		int2 m_resolution, m_view_pos;

		int m_inventory_sel, m_container_sel;

		string m_profiler_stats;
		double m_last_time, m_stats_update_time;
		bool m_show_stats;

		game::Intersection m_isect, m_full_isect;
		game::Intersection m_shoot_isect;
		float3 m_target_pos;
		float3 m_last_look_at;

		game::Path m_last_path;
	};

	typedef unique_ptr<Controller> PController;

}

#endif
