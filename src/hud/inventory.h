/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_INVENTORY_H
#define HUD_INVENTORY_H

#include "hud/layer.h"

namespace hud
{

	class HudInventory: public HudLayer {
	public:
		HudInventory(const FRect &target_rect);

		void updateItems(const Actor&);
	};

}

#endif
