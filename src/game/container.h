/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_CONTAINER_H
#define GAME_CONTAINER_H

#include "game/entity.h"
#include "game/inventory.h"
#include "sys/data_sheet.h"

namespace game
{
	
	DEFINE_ENUM(ContainerSoundType,
		opening,
		closing
	)

	DEFINE_ENUM(ContainerState,
		closed,
		opened,
		opening,
		closing
	);


	struct ContainerProto: public ProtoImpl<ContainerProto, EntityProto, ProtoId::container> {
		ContainerProto(const TupleParser&);

		string name;
		EnumMap<ContainerSoundType, SoundId> sound_ids;
		EnumMap<ContainerState, int> seq_ids;
		bool is_always_opened;
	};


	class Container: public EntityImpl<Container, ContainerProto, EntityId::container>
	{
	public:
		Container(Stream&);
		Container(const XMLNode&);
		Container(const ContainerProto&);

		FlagsType flags() const override { return Flags::container | Flags::static_entity | Flags::occluding | Flags::colliding; }

		void open();
		void close();
		void setKey(const Item&);

		void interact(const Entity*) override;
		void onSoundEvent() override;

		bool isOpened() const { return m_state == ContainerState::opened; }
		bool isAlwaysOpened() const { return m_proto.is_always_opened; }

		const Inventory &inventory() const { return m_inventory; }
		Inventory &inventory() { return m_inventory; }
		
		XMLNode save(XMLNode&) const override;
		void save(Stream&) const override;
		
	private:
		void think() override;
		void onAnimFinished() override;
		void initialize();

		Inventory m_inventory;
		Item m_key;

		ContainerState m_state, m_target_state;
		bool m_is_always_opened;
		bool m_update_anim;
	};
};

#endif
