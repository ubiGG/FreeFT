/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_LOOP_H
#define IO_LOOP_H

#include "base.h"

namespace io {

	class Loop {
	public:
		enum TransitionMode {
			trans_normal,
			trans_left,
			trans_right
		};

		Loop();
		virtual ~Loop();

		bool tick(double time_diff);
		void draw();
		void exit();

		void startTransition(Color from, Color to, TransitionMode mode, float length);
		bool isTransitioning() const;

	protected:
		virtual bool onTick(double) = 0;
		virtual void onDraw() = 0;
		virtual void onTransitionFinished() { }

	private:
		struct Transition {
			void draw(const IRect &rect);

			FColor from, to;
			TransitionMode mode;
			float pos, length;
		};

		Transition m_transition;
		bool m_is_transitioning;
		bool m_is_exiting;
	};

	using PLoop = unique_ptr<Loop>;

}

#endif
