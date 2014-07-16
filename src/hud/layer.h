/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_LAYER_H
#define HUD_LAYER_H

#include "hud/widget.h"

namespace hud
{
	class HudLayer: public HudWidget {
	public:
		enum SlideMode {
			slide_left,
			slide_top,
		};

		HudLayer(const FRect &target_rect, SlideMode slide_mode = slide_left);
		virtual ~HudLayer();

		void setTitle(const string &title);

		const FRect rect() const override;
		float topOffset() const;

		float alpha() const override;
		virtual float backAlpha() const;

		virtual Color backColor() const;
		virtual Color borderColor() const;
		virtual Color titleColor() const;
		virtual Color titleShadowColor() const;

		void setWorld(game::PWorld);
		void setPCController(game::PPCController);
		virtual void onPCControllerSet() { }

		virtual bool canShow() const { return true; }

	protected:
		void onDraw() const override;
		
		game::PWorld m_world;
		game::PPCController m_pc_controller;

	private:
		SlideMode m_slide_mode;
		string m_title;
	};

}

#endif
