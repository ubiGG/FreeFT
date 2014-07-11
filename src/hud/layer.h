/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_LAYER_H
#define HUD_LAYER_H

#include "hud/base.h"

namespace hud
{
	class HudLayer: public RefCounter {
	public:
		enum { spacing = 5 };

		HudLayer(const FRect &target_rect);
		virtual ~HudLayer();

		void setTargetRect(const FRect &rect);
		const FRect &targetRect() const { return m_target_rect; }

		const FRect rect() const;
		virtual void draw() const;
		virtual void update(bool handle_input, double time_diff);
		virtual bool isMouseOver() const;

		bool isVisible() const { return m_is_visible || m_visible_time > 0.01f; }
		bool isShowing() const { return m_is_visible && m_visible_time < 1.0f; }

		virtual void setVisible(bool is_visible, bool animate = true);
		virtual float backAlpha() const;

		void setStyle(HudStyle);
		void attach(PHudWidget);

	protected:
		FRect m_target_rect;
		HudStyle m_style;
		vector<PHudWidget> m_widgets;
		gfx::PFont m_font, m_big_font;

		float m_visible_time;
		bool m_is_visible;
		bool m_slide_left;
	};

}

#endif
