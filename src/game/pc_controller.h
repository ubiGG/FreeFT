/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_PC_CONTROLLER_H
#define GAME_PC_CONTROLLER_H

#include "game/base.h"
#include "game/character.h"

namespace game
{

	// Playable character controller
	class PCController: public RefCounter {
	public:
		PCController(World&, const PlayableCharacter&);
		~PCController();

		const PlayableCharacter &pc() const { return m_pc; }
		const Actor *actor() const;
		bool hasActor() const;

		bool canChangeStance() const;
		void setStance(Stance::Type);
		int targetStance() const { return m_target_stance; }

		void reload();

		void sendOrder(game::POrder&&);
	
	private:
		int m_target_stance;

		World &m_world;
		PlayableCharacter m_pc;
	};

}

#endif