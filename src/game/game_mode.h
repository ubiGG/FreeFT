/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_GAME_MODE_H
#define GAME_GAME_MODE_H

#include "game/base.h"
#include "game/inventory.h"
#include "game/character.h"
#include "game/orders.h"
#include "game/character.h"

namespace game {

	DECLARE_ENUM(MessageId,
		actor_order,
		class_changed,
		update_client,
		remove_client,
		respawn,

		update_client_info
	);

	class GameMode {
	public:
		GameMode(World &world) :m_world(world) { }
		virtual ~GameMode() { }

		GameMode(const GameMode&) = delete;
		void operator=(const GameMode&) = delete;

		virtual GameModeId::Type typeId() const = 0;
		virtual void tick(double time_diff) = 0;
		virtual const vector<PlayableCharacter> playableCharacters() const = 0;

		virtual void onMessage(Stream&, MessageId::Type, int source_id) { }
		virtual bool sendOrder(POrder &&order, EntityRef entity_ref);

	protected:
		EntityRef findSpawnZone(int faction_id) const;
		EntityRef spawnActor(EntityRef spawn_zone, const Proto &proto, const ActorInventory&);
		void attachAIs();

		World &m_world;
	};

	struct GameClient {
		void save(Stream&) const;
		void load(Stream&);

		string nick_name;
		vector<PlayableCharacter> pcs;
	};

	class GameModeServer: public GameMode {
	public:
		GameModeServer(World &world);

		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId::Type, int source_id) override;

		const vector<PlayableCharacter> playableCharacters() const override { return {}; };

	protected:
		void onClientConnected(int client_id, const string &nick_name);
		void onClientDisconnected(int client_id);
		friend class net::Server;

	protected:
		void sendClientInfo(int client_id, int target_id);
		void updateClient(int client_id, const GameClient&);
		void respawn(int client_id, int pc_id, EntityRef spawn_zone);

		std::map<int, GameClient> m_clients;
	};

	class GameModeClient: public GameMode {
	public:
		GameModeClient(World &world, int client_id, const string &nick_name);

		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId::Type, int source_id) override;
		const vector<PlayableCharacter> playableCharacters() const override { return m_current.pcs; }

		const string &currentNickName() const { return m_current.nick_name; }
		bool addPC(const Character&);
		bool setPCClass(const Character&, const CharacterClass&);

		bool sendOrder(POrder &&order, EntityRef entity_ref) override;

	protected:

		GameClient m_current;
		std::map<int, GameClient> m_others;
		const int m_current_id;
		int m_max_pcs;
	};

}

#endif

